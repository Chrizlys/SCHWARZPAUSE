/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "VMBusPrivate.h"


static float
vmbus_supports_device(device_node* parent)
{
	CALLED();
	const char* bus;

	// Check if the parent is the root node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK) {
		TRACE("Could not find required attribute device/bus\n");
		return -1;
	}

	if (strcmp(bus, "root") != 0)
		return 0.0f;

	status_t status = vmbus_detect_hyperv();
	if (status != B_OK)
		return 0.0f;

	return 0.8f;
}


static status_t
vmbus_register_device(device_node* parent)
{
	CALLED();
	device_attr attributes[] = {
		{ B_DEVICE_BUS, B_STRING_TYPE,
			{ .string = HYPERV_BUS_NAME }},
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = HYPERV_PRETTYNAME_VMBUS }},
		{ NULL }
	};

	return gDeviceManager->register_node(parent, HYPERV_VMBUS_MODULE_NAME, attributes, NULL, NULL);
}


static status_t
vmbus_init_driver(device_node* node, void** _driverCookie)
{
	CALLED();

	VMBus* vmbus = new(std::nothrow) VMBus(node);
	if (vmbus == NULL) {
		ERROR("Unable to allocate VMBus object\n");
		return B_NO_MEMORY;
	}

	status_t status = vmbus->InitCheck();
	if (status != B_OK) {
		ERROR("Failed to set up VMBus object\n");
		delete vmbus;
		return status;
	}
	TRACE("VMBus object created\n");

	*_driverCookie = vmbus;
	return B_OK;
}


static void
vmbus_uninit_driver(void* driverCookie)
{
	CALLED();
	VMBus* vmbus = reinterpret_cast<VMBus*>(driverCookie);
	delete vmbus;
}


static status_t
vmbus_register_child_devices(void* driverCookie)
{
	CALLED();
	VMBus* vmbus = reinterpret_cast<VMBus*>(driverCookie);
	return vmbus->RequestChannels();
}


static uint32
vmbus_get_version(hyperv_bus cookie)
{
	CALLED();
	VMBus* vmbus = reinterpret_cast<VMBus*>(cookie);
	return vmbus->GetVersion();
}


static status_t
vmbus_open_channel(hyperv_bus cookie, uint32 channel, uint32 gpadl, uint32 rxOffset,
	hyperv_bus_callback callback, void* callbackData)
{
	CALLED();
	VMBus* vmbus = reinterpret_cast<VMBus*>(cookie);
	return vmbus->OpenChannel(channel, gpadl, rxOffset, callback, callbackData);
}


static status_t
vmbus_close_channel(hyperv_bus cookie, uint32 channel)
{
	CALLED();
	VMBus* vmbus = reinterpret_cast<VMBus*>(cookie);
	return vmbus->CloseChannel(channel);
}


static status_t
vmbus_allocate_gpadl(hyperv_bus cookie, uint32 channel, uint32 length, void** _buffer,
	uint32* _gpadl)
{
	CALLED();
	VMBus* vmbus = reinterpret_cast<VMBus*>(cookie);
	return vmbus->AllocateGPADL(channel, length, _buffer, _gpadl);
}


static status_t
vmbus_free_gpadl(hyperv_bus cookie, uint32 channel, uint32 gpadl)
{
	CALLED();
	VMBus* vmbus = reinterpret_cast<VMBus*>(cookie);
	return vmbus->FreeGPADL(channel, gpadl);
}


static status_t
vmbus_signal_channel(hyperv_bus cookie, uint32 channel)
{
	VMBus* vmbus = reinterpret_cast<VMBus*>(cookie);
	return vmbus->SignalChannel(channel);
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			break;
	}

	return B_ERROR;
}


hyperv_bus_interface gVMBusModule = {
	{
		{
			HYPERV_VMBUS_MODULE_NAME,
			0,
			std_ops
		},
		vmbus_supports_device,
		vmbus_register_device,
		vmbus_init_driver,
		vmbus_uninit_driver,
		vmbus_register_child_devices,
		NULL,	// rescan bus
		NULL	// device removed
	},

	vmbus_get_version,
	vmbus_open_channel,
	vmbus_close_channel,
	vmbus_allocate_gpadl,
	vmbus_free_gpadl,
	vmbus_signal_channel
};
