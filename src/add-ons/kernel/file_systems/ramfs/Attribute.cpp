/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include "AllocationInfo.h"
#include "Attribute.h"
#include "Directory.h"
#include "Misc.h"
#include "Node.h"
#include "Volume.h"


Attribute::Attribute(Volume *volume, Node *node, const char *name,
					 uint32 type)
	: DataContainer(volume),
	  fNode(node),
	  fName(name),
	  fType(type),
	  fIndex(NULL),
	  fInIndex(false),
	  fIterators()
{
}


Attribute::~Attribute()
{
	ASSERT(fNode == NULL && fIndex == NULL);
}


status_t
Attribute::InitCheck() const
{
	return fName.GetString() ? B_OK : B_NO_INIT;
}


void
Attribute::SetNode(Node *node)
{
	if (fNode != NULL) {
		if (fIndex != NULL)
			fIndex->Removed(this);

		_NotifyRemoved();
	}

	fNode = node;

	if (fNode != NULL) {
		AttributeIndex* index = GetVolume()->FindAttributeIndex(GetName(), fType);
		if (index != NULL)
			index->Added(this);

		_NotifyAdded();
	}
}


void
Attribute::SetType(uint32 type)
{
	if (type == fType)
		return;

	if (fIndex != NULL)
		fIndex->Removed(this);
	_NotifyRemoved();

	fType = type;

	AttributeIndex *index = GetVolume()->FindAttributeIndex(GetName(), fType);
	if (index != NULL)
		index->Added(this);
	_NotifyAdded();
}


status_t
Attribute::SetSize(off_t newSize)
{
	off_t oldSize = DataContainer::GetSize();
	if (newSize == oldSize)
		return B_OK;

	uint8 oldKey[kMaxIndexKeyLength];
	size_t oldLength = kMaxIndexKeyLength;
	GetKey(oldKey, &oldLength);

	status_t error = DataContainer::Resize(newSize);
	if (error != B_OK)
		return error;

	off_t changeOffset = (newSize < oldSize) ? newSize : oldSize;
	_Changed(oldKey, oldLength, changeOffset, newSize - oldSize);
	return B_OK;
}


status_t
Attribute::WriteAt(off_t offset, const void *buffer, size_t size, size_t *bytesWritten)
{
	// store the current key for the attribute
	uint8 oldKey[kMaxIndexKeyLength];
	size_t oldLength = kMaxIndexKeyLength;
	GetKey(oldKey, &oldLength);

	// write the new value
	status_t error = DataContainer::WriteAt(offset, buffer, size, bytesWritten);
	if (error != B_OK)
		return error;

	// update index and send notifications
	_Changed(oldKey, oldLength, offset, size);
	return B_OK;
}


void
Attribute::_NotifyAdded()
{
	uint8 newKey[kMaxIndexKeyLength];
	size_t newLength;
	GetKey(newKey, &newLength);
	_Notify(B_ATTR_CREATED, NULL, 0, newKey, newLength);
}


void
Attribute::_NotifyRemoved()
{
	uint8 oldKey[kMaxIndexKeyLength];
	size_t oldLength;
	GetKey(oldKey, &oldLength);
	_Notify(B_ATTR_REMOVED, oldKey, oldLength, NULL, 0);
}


void
Attribute::_Changed(uint8* oldKey, size_t oldLength, off_t changeOffset, ssize_t changeSize)
{
	// If there is an index and a change has been made within the key, notify
	// the index.
	if (fIndex != NULL && changeOffset < (off_t)kMaxIndexKeyLength)
		fIndex->Changed(this, oldKey, oldLength);

	uint8 newKey[kMaxIndexKeyLength];
	size_t newLength;
	GetKey(newKey, &newLength);
	_Notify(B_ATTR_CHANGED, oldKey, oldLength, newKey, newLength);
}


void
Attribute::_Notify(int32 cause, uint8* oldKey, size_t oldLength,
	uint8* newKey, size_t newLength)
{
	// notify node monitor
	for (Entry* entry = fNode->GetFirstReferrer(); entry != NULL;
			entry = fNode->GetNextReferrer(entry)) {
		ino_t parentID = -1;
		if (entry->GetParent() != NULL)
			parentID = entry->GetParent()->GetID();
		notify_attribute_changed(GetVolume()->GetID(), parentID,
			fNode->GetID(), GetName(), cause);
	}

	// update live queries
	GetVolume()->UpdateLiveQueries(NULL, fNode, GetName(), fType, oldKey,
		oldLength, newKey, newLength);

	// node has been changed
	if (fNode != NULL)
		fNode->MarkModified(B_STAT_MODIFICATION_TIME);
}


void
Attribute::SetIndex(AttributeIndex *index, bool inIndex)
{
	ASSERT(fIndex == NULL || index == NULL || fIndex == index);
	ASSERT(!fInIndex || fInIndex != inIndex);

	fIndex = index;
	fInIndex = inIndex;
}


void
Attribute::GetKey(uint8 key[kMaxIndexKeyLength], size_t *length)
{
	ReadAt(0, key, kMaxIndexKeyLength, length);
}


void
Attribute::AttachAttributeIterator(AttributeIterator *iterator)
{
	if (iterator && iterator->GetCurrent() == this && !iterator->IsSuspended())
		fIterators.Insert(iterator);
}


void
Attribute::DetachAttributeIterator(AttributeIterator *iterator)
{
	if (iterator && iterator->GetCurrent() == this && iterator->IsSuspended())
		fIterators.Remove(iterator);
}


void
Attribute::GetAllocationInfo(AllocationInfo &info)
{
	info.AddAttributeAllocation(DataContainer::GetCommittedSize());
	info.AddStringAllocation(fName.GetLength());
}
