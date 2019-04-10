/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <vm_config.h>
#include <vm_configurations.h>
#include <acrn_common.h>

struct acrn_vm_config vm_configs[CONFIG_MAX_VM_NUM] = {
	{
		.type = PRE_LAUNCHED_VM | SECURITY_VM,
		.name = "ACRN PRE-LAUNCHED VM",
		.pcpu_bitmap = PLUG_CPU(3),
		.guest_flags = 0U,
		.clos = 0U,
		.memory = {
			.start_hpa = 0x120000000UL,
			.size = 0x20000000UL,
		},
		.os_config = {
			.name = "ACRN Security OS",
		},
		.vm_vuart = true,
	},
	{
		.type = SOS_VM,
		.name = "ACRN SOS VM",
		.guest_flags = GUEST_FLAG_IO_COMPLETION_POLLING,
		.clos = 0U,
		.memory = {
			.start_hpa = 0UL,
			.size = CONFIG_SOS_RAM_SIZE,
		},
		.os_config = {
			.name = "ACRN Service OS",
		},
	},
};
