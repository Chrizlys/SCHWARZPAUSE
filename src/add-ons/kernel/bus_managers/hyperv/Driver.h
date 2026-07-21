/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HYPERV_DRIVER_H_
#define _HYPERV_DRIVER_H_

#include <new>
#include <stdio.h>

#include <ACPI.h>
#include <device_manager.h>
#include <dpc.h>
#include <KernelExport.h>

#include <hyperv.h>


#define HYPERV_VMBUS_MODULE_NAME		"bus_managers/hyperv/root/driver_v1"
#define HYPERV_DEVICE_MODULE_NAME		"bus_managers/hyperv/device/v1"


typedef void* hyperv_bus;
typedef void (*hyperv_bus_callback)(void* data);


// Interface between the VMBus bus device driver, and the VMBus bus manager
typedef struct hyperv_bus_interface {
	driver_module_info info;

	uint32 (*get_version)(hyperv_bus cookie);
	status_t (*open_channel)(hyperv_bus cookie, uint32 channel, uint32 gpadl,
		uint32 rxOffset, hyperv_bus_callback callback, void* callbackData);
	status_t (*close_channel)(hyperv_bus cookie, uint32 channel);
	status_t (*allocate_gpadl)(hyperv_bus cookie, uint32 channel, uint32 length,
		void** _buffer, uint32* _gpadl);
	status_t (*free_gpadl)(hyperv_bus cookie, uint32 channel, uint32 gpadl);
	status_t (*signal_channel)(hyperv_bus cookie, uint32 channel);
} hyperv_bus_interface;


extern device_manager_info* gDeviceManager;
extern acpi_module_info* gACPI;
extern dpc_module_info* gDPC;

extern hyperv_bus_interface gVMBusModule;
extern hyperv_device_interface gVMBusDeviceModule;

#endif // _HYPERV_DRIVER_H_
