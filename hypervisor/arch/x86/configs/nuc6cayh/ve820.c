/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <e820.h>
#include <vm.h>

#define VE820_ENTRIES_APL_NUC  1U
static const struct e820_entry ve820_entry[VE820_ENTRIES_APL_NUC] = {
	{
		.baseaddr = 0x0UL,
		.length   = 0x20000000UL,
		.type     = E820_TYPE_RAM
	},
};

/**
 * @pre vm != NULL
 */
void create_prelaunched_vm_e820(struct acrn_vm *vm)
{
	vm->e820_entry_num = VE820_ENTRIES_APL_NUC;
	vm->e820_entries = (struct e820_entry *)ve820_entry;
}
