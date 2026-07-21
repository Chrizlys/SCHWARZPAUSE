/*
 * Copyright 2001-2002, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 *		Adrien Destugues (pulkomandy)
 *		Jérôme Duval (korli)
 *		Christopher ML Zumwalt May (zummy@users.sf.net)
 */

//	Description:	Manages the game producer. The class may change without
//					notice and was only intended for use by the GameKit at
//					this time. Use at your own risk.

#include "GameSoundDevice.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <List.h>
#include <MediaAddOn.h>
#include <MediaRoster.h>
#include <MediaTheme.h>
#include <TimeSource.h>
#include <locks.h>

#include "GameSoundBuffer.h"


static const int32 kInitSoundCount = 32;
static const int32 kGrowth = 16;

static int32 sDeviceCount = 0;
static BGameSoundDevice* sDevice = NULL;
static mutex sDeviceRefCountLock = MUTEX_INITIALIZER("GameSound device lock");


BGameSoundDevice*
BGameSoundDevice::GetDefaultDevice()
{
	MutexLocker _(sDeviceRefCountLock);

	if (!sDevice)
		sDevice = new BGameSoundDevice();

	sDeviceCount++;
	return sDevice;
}


void
BGameSoundDevice::ReleaseDevice()
{
	MutexLocker _(sDeviceRefCountLock);

	sDeviceCount--;

	if (sDeviceCount <= 0) {
		delete sDevice;
		sDevice = NULL;
	}
}


BGameSoundDevice::BGameSoundDevice()
	:
	fIsConnected(false),
	fSoundCount(kInitSoundCount)
{
	memset(&fFormat, 0, sizeof(gs_audio_format));

	fInitError = B_OK;

	fSounds = new GameSoundBuffer*[kInitSoundCount];
	for (int32 i = 0; i < kInitSoundCount; i++)
		fSounds[i] = NULL;
}


BGameSoundDevice::~BGameSoundDevice()
{
	// We need to stop all the sounds before we stop the mixer
	for (int32 i = 0; i < fSoundCount; i++) {
		if (fSounds[i])
			fSounds[i]->StopPlaying();
		delete fSounds[i];
	}

	delete[] fSounds;
}


status_t
BGameSoundDevice::InitCheck() const
{
	return fInitError;
}


const gs_audio_format&
BGameSoundDevice::Format() const
{
	return fFormat;
}


const gs_audio_format&
BGameSoundDevice::Format(gs_id sound) const
{
	return fSounds[sound - 1]->Format();
}


void
BGameSoundDevice::SetInitError(status_t error)
{
	fInitError = error;
}


status_t
BGameSoundDevice::CreateBuffer(gs_id* sound, const gs_audio_format* format,
	const void* data, int64 frames)
{
	if (frames <= 0 || !sound)
		return B_BAD_VALUE;

	// Make sure BMediaRoster is created before we AllocateSound()
	BMediaRoster* roster = BMediaRoster::Roster();

	status_t err = B_MEDIA_TOO_MANY_BUFFERS;
	int32 position = AllocateSound();

	if (position >= 0) {
		fSounds[position] = new SimpleSoundBuffer(format, data, frames);

		media_node systemMixer;
		roster->GetAudioMixer(&systemMixer);
		err = fSounds[position]->Connect(&systemMixer);
	}

	if (err == B_OK)
		*sound = gs_id(position + 1);
	return err;
}


status_t
BGameSoundDevice::CreateBuffer(gs_id* sound, const void* object,
	const gs_audio_format* format, size_t inBufferFrameCount,
	size_t inBufferCount)
{
	if (!object || !sound)
		return B_BAD_VALUE;

	// Make sure BMediaRoster is created before we AllocateSound()
	BMediaRoster* roster = BMediaRoster::Roster();

	status_t err = B_MEDIA_TOO_MANY_BUFFERS;
	int32 position = AllocateSound();

	if (position >= 0) {
		fSounds[position] = new StreamingSoundBuffer(format, object,
			inBufferFrameCount, inBufferCount);

		media_node systemMixer;
		roster->GetAudioMixer(&systemMixer);
		err = fSounds[position]->Connect(&systemMixer);
	}

	if (err == B_OK)
		*sound = gs_id(position + 1);
	return err;
}


void
BGameSoundDevice::ReleaseBuffer(gs_id sound)
{
	if (sound <= 0)
		return;

	if (fSounds[sound - 1]) {
		// We must stop playback befor destroying the sound or else
		// we may receive fatal errors.
		fSounds[sound - 1]->StopPlaying();

		delete fSounds[sound - 1];
		fSounds[sound - 1] = NULL;
	}
}


status_t
BGameSoundDevice::Buffer(gs_id sound, gs_audio_format* format, void*& data)
{
	if (!format || sound <= 0)
		return B_BAD_VALUE;

	memcpy(format, &fSounds[sound - 1]->Format(), sizeof(gs_audio_format));
	if (fSounds[sound - 1]->Data()) {
		data = malloc(format->buffer_size);
		memcpy(data, fSounds[sound - 1]->Data(), format->buffer_size);
	} else
		data = NULL;

	return B_OK;
}


status_t
BGameSoundDevice::StartPlaying(gs_id sound)
{
	if (sound <= 0)
		return B_BAD_VALUE;

	if (!fSounds[sound - 1]->IsPlaying()) {
		// tell the producer to start playing the sound
		return fSounds[sound - 1]->StartPlaying();
	}

	fSounds[sound - 1]->Reset();
	return EALREADY;
}


status_t
BGameSoundDevice::StopPlaying(gs_id sound)
{
	if (sound <= 0)
		return B_BAD_VALUE;

	if (fSounds[sound - 1]->IsPlaying()) {
		// Tell the producer to stop play this sound
		fSounds[sound - 1]->Reset();
		return fSounds[sound - 1]->StopPlaying();
	}

	return EALREADY;
}


bool
BGameSoundDevice::IsPlaying(gs_id sound)
{
	if (sound <= 0)
		return false;
	return fSounds[sound - 1]->IsPlaying();
}


status_t
BGameSoundDevice::GetAttributes(gs_id sound, gs_attribute* attributes,
	size_t attributeCount)
{
	if (!fSounds[sound - 1])
		return B_ERROR;

	return fSounds[sound - 1]->GetAttributes(attributes, attributeCount);
}


status_t
BGameSoundDevice::SetAttributes(gs_id sound, gs_attribute* attributes,
	size_t attributeCount)
{
	if (!fSounds[sound - 1])
		return B_ERROR;

	return fSounds[sound - 1]->SetAttributes(attributes, attributeCount);
}


int32
BGameSoundDevice::AllocateSound()
{
	for (int32 i = 0; i < fSoundCount; i++)
		if (!fSounds[i])
			return i;

	// we need to allocate new space for the sound
	GameSoundBuffer ** sounds = new GameSoundBuffer*[fSoundCount + kGrowth];
	for (int32 i = 0; i < fSoundCount; i++)
		sounds[i] = fSounds[i];

	for (int32 i = fSoundCount; i < fSoundCount + kGrowth; i++)
		sounds[i] = NULL;

	// replace the old list
	delete [] fSounds;
	fSounds = sounds;
	fSoundCount += kGrowth;

	return fSoundCount - kGrowth;
}

