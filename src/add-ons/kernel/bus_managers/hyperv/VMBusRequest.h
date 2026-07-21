/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VMBUS_REQUEST_H_
#define _VMBUS_REQUEST_H_

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <condition_variable.h>
#include <util/DoublyLinkedList.h>

#include <hyperv.h>

#include "Driver.h"
#include "hyperv_spec_private.h"

#define VMBUS_TIMEOUT HV_MS_TO_US(20000)

class VMBusRequest : public DoublyLinkedListLinkImpl<VMBusRequest> {
public:
								VMBusRequest(uint32 type, uint32 channelID);
								VMBusRequest(uint32 type, uint32 channelID, uint32 length);
								~VMBusRequest();

	status_t					InitCheck() const { return fStatus; }
	vmbus_msg*					GetMessage() const { return fMessage; }
	uint32						GetChannelID() const { return fChannelID; }
	uint32						GetResponseType() const { return fResponseType; }
	void						SetResponseType(uint32 type) { fResponseType = type; }
	uint32						GetResponseData() const { return fResponseData; }
	void						SetResponseData(uint32 data) { fResponseData = data; }
	uint32						GetLength() const { return fHcPostMessage->data_size; }
	void						SetLength(uint32 length) { fHcPostMessage->data_size = length; }
	phys_addr_t					GetHcPostPhys() const { return fHcPostMessagePhys; }

	void						Add(ConditionVariableEntry* waitEntry);
	status_t					Wait(ConditionVariableEntry* waitEntry);
	void						Notify(status_t status, vmbus_msg* message, uint32 messageLength);

private:
	status_t					fStatus;
	uint32						fChannelID;
	uint32						fResponseType;
	uint32						fResponseData;
	vmbus_msg*					fMessage;

	hypercall_post_msg_input*	fHcPostMessage;
	area_id						fHcPostMessageArea;
	phys_addr_t					fHcPostMessagePhys;

	ConditionVariable			fConditionVariable;
};
typedef DoublyLinkedList<VMBusRequest> VMBusRequestList;


#endif // _VMBUS_REQUEST_H_
