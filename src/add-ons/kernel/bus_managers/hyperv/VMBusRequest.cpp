/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "VMBusRequest.h"


VMBusRequest::VMBusRequest(uint32 type, uint32 channelID)
	: VMBusRequest(type, channelID, 0)
{
}


VMBusRequest::VMBusRequest(uint32 type, uint32 channelID, uint32 length)
	:
	fStatus(B_NOT_INITIALIZED),
	fChannelID(channelID),
	fResponseType(VMBUS_MSGTYPE_INVALID),
	fResponseData(0),
	fMessage(NULL),
	fHcPostMessage(NULL),
	fHcPostMessageArea(0),
	fHcPostMessagePhys(0)
{
	if (type >= VMBUS_MSGTYPE_MAX) {
		fStatus = B_BAD_VALUE;
		return;
	}

	if (length == 0) {
		length = vmbus_msg_lengths[type];
		if (length == 0) {
			fStatus = B_BAD_VALUE;
			return;
		}
	}

	fHcPostMessageArea = create_area("vmbus request", (void**)&fHcPostMessage,
		B_ANY_KERNEL_ADDRESS, sizeof(*fHcPostMessage), B_CONTIGUOUS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (fHcPostMessageArea < B_OK) {
		fStatus = fHcPostMessageArea;
		return;
	}

	physical_entry entry;
	fStatus = get_memory_map(fHcPostMessage, sizeof(*fHcPostMessage), &entry, 1);
	if (fStatus != B_OK) {
		delete_area(fHcPostMessageArea);
		return;
	}
	fHcPostMessagePhys = entry.address;

	fHcPostMessage->connection_id = VMBUS_CONNID_MESSAGE;
	fHcPostMessage->reserved = 0;
	fHcPostMessage->message_type = HYPERV_MSGTYPE_CHANNEL;
	fHcPostMessage->data_size = length;

	fMessage = reinterpret_cast<vmbus_msg*>(fHcPostMessage->data);
	fMessage->header.type = type;
	fMessage->header.reserved = 0;

	fConditionVariable.Init(this, "vmbus request");
}


VMBusRequest::~VMBusRequest()
{
	delete_area(fHcPostMessageArea);
}


void
VMBusRequest::Add(ConditionVariableEntry* waitEntry)
{
	if (fResponseType == VMBUS_MSGTYPE_INVALID)
		return;

	fConditionVariable.Add(waitEntry);
}


status_t
VMBusRequest::Wait(ConditionVariableEntry* waitEntry)
{
	if (fResponseType == VMBUS_MSGTYPE_INVALID)
		return B_OK;

	return waitEntry->Wait(B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, VMBUS_TIMEOUT);
}


void
VMBusRequest::Notify(status_t status, vmbus_msg* message, uint32 messageLength)
{
	if (fResponseType == VMBUS_MSGTYPE_INVALID)
		return;

	if (status == B_OK) {
		memcpy(fMessage, message, messageLength);
		SetLength(messageLength);
	}
	fConditionVariable.NotifyAll(status);
}
