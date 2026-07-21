/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include "Driver.h"

device_manager_info* gDeviceManager;
acpi_module_info* gACPI;
dpc_module_info* gDPC;


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ B_ACPI_MODULE_NAME, (module_info**)&gACPI },
	{ B_DPC_MODULE_NAME, (module_info **)&gDPC },
	{}
};


module_info* modules[] = {
	(module_info*)&gVMBusModule,
	(module_info*)&gVMBusDeviceModule,
	NULL
};
