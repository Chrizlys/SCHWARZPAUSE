/*
 * Copyright 2016, Dmytro Shynkevych, dm.shynk@gmail.com
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <pthread.h>
#include "pthread_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <syscall_utils.h>
#include <syscalls.h>
#include <user_mutex_defs.h>


#define BARRIER_FLAG_SHARED	0x80000000


static const pthread_barrierattr pthread_barrierattr_default = {
	/* .process_shared = */ false
};


int
pthread_barrier_init(pthread_barrier_t* barrier,
	const pthread_barrierattr_t* _attr, unsigned count)
{
	const pthread_barrierattr* attr = _attr != NULL
		? *_attr : &pthread_barrierattr_default;

	if (barrier == NULL || attr == NULL || count < 1)
		return B_BAD_VALUE;

	barrier->flags = attr->process_shared ? BARRIER_FLAG_SHARED : 0;
	barrier->lock = B_USER_MUTEX_LOCKED;
	barrier->mutex = B_USER_MUTEX_LOCKED | B_USER_MUTEX_DISABLED;
	barrier->waiter_count = 0;
	barrier->waiter_max = count;

	return B_OK;
}


static void
barrier_disable_and_unblock(__haiku_std_int32* mutex, uint32 flags)
{
	int32 oldValue = atomic_or((int32*)mutex, B_USER_MUTEX_DISABLED);
	if ((oldValue & B_USER_MUTEX_WAITING) != 0)
		_kern_mutex_unblock((int32*)mutex, flags | B_USER_MUTEX_UNBLOCK_ALL);
}


static void
barrier_ensure_none_exiting(pthread_barrier_t* barrier)
{
	const uint32 flags = (barrier->flags & BARRIER_FLAG_SHARED) ? B_USER_MUTEX_SHARED : 0;

	// waiter_count < 0 means other threads are still exiting.
	while (atomic_get((int32*)&barrier->waiter_count) < 0) {
		status_t status = _kern_mutex_lock((int32*)&barrier->mutex, NULL, flags, 0);
		if (status != B_INTERRUPTED)
			return;
	}
}


int
pthread_barrier_wait(pthread_barrier_t* barrier)
{
	if (barrier == NULL)
		return B_BAD_VALUE;

	if (barrier->waiter_max == 1)
		return PTHREAD_BARRIER_SERIAL_THREAD;

	const uint32 mutexFlags = (barrier->flags & BARRIER_FLAG_SHARED) ? B_USER_MUTEX_SHARED : 0;
	barrier_ensure_none_exiting(barrier);

	if (atomic_add((int32*)&barrier->waiter_count, 1) == (barrier->waiter_max - 1)) {
		// We are the last one in. Reset the count and set the barrier mutex.
		barrier->waiter_count = (-barrier->waiter_max) + 1;
		barrier->mutex = B_USER_MUTEX_LOCKED;

		// Wake everyone else up. But first, mark the barrier disabled,
		// so exiting threads don't need to re-unlock.
		barrier_disable_and_unblock(&barrier->lock, mutexFlags);

		// Return with the barrier mutex still locked, as waiter_count < 0.
		// The last thread out will take care of unlocking it and resetting state.
		return PTHREAD_BARRIER_SERIAL_THREAD;
	}

	// We aren't the last one in. Wait until we are woken up.
	do {
		_kern_mutex_lock((int32*)&barrier->lock, "barrier wait", mutexFlags, 0);
	} while (barrier->waiter_count > 0);

	if (atomic_add((int32*)&barrier->waiter_count, 1) == -1) {
		// We are the last one out. Reset state and unblock.
		atomic_and((int32*)&barrier->lock, ~(int32)B_USER_MUTEX_DISABLED);
		barrier_disable_and_unblock(&barrier->mutex, mutexFlags);
	}

	return 0;
}


int
pthread_barrier_destroy(pthread_barrier_t* barrier)
{
	barrier_ensure_none_exiting(barrier);

	// Wait (if necessary) for the last thread to finish unblocking.
	while (atomic_get((int32*)&barrier->mutex) != (B_USER_MUTEX_LOCKED | B_USER_MUTEX_DISABLED))
		sched_yield();

	return B_OK;
}


int
pthread_barrierattr_init(pthread_barrierattr_t* _attr)
{
	pthread_barrierattr* attr = (pthread_barrierattr*)malloc(
		sizeof(pthread_barrierattr));

	if (attr == NULL)
		return B_NO_MEMORY;

	*attr = pthread_barrierattr_default;
	*_attr = attr;

	return B_OK;
}


int
pthread_barrierattr_destroy(pthread_barrierattr_t* _attr)
{
	pthread_barrierattr* attr = _attr != NULL ? *_attr : NULL;

	if (attr == NULL)
		return B_BAD_VALUE;

	free(attr);

	return B_OK;
}


int
pthread_barrierattr_getpshared(const pthread_barrierattr_t* _attr, int* shared)
{
	pthread_barrierattr* attr;

	if (_attr == NULL || (attr = *_attr) == NULL || shared == NULL)
		return B_BAD_VALUE;

	*shared = attr->process_shared
		? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE;

	return B_OK;
}


int
pthread_barrierattr_setpshared(pthread_barrierattr_t* _attr, int shared)
{
	pthread_barrierattr* attr;

	if (_attr == NULL || (attr = *_attr) == NULL
		|| shared < PTHREAD_PROCESS_PRIVATE
		|| shared > PTHREAD_PROCESS_SHARED) {
		return B_BAD_VALUE;
	}

	attr->process_shared = shared == PTHREAD_PROCESS_SHARED;

	return 0;
}
