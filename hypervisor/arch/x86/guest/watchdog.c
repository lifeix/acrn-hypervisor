/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @pre vm != NULL
 * @pre vm->vm_id < CONFIG_MAX_VM_NUM
 */

#include <vm.h>

void wtd_init(struct acrn_vm *vm)
{
	if (is_sos_vm(vm)) {
		struct vm_io_range wtd_ich_res_tco_io_range = {
			.base = 0x400U,
			.len = 0x20U
		};

		struct vm_io_range wtd_ich_res_smi_io_range = {
			.base = 0x1830U,
			.len = 0x4U
		};

		register_pio_emulation_handler(vm, WTD_ICH_RES_IO_TCO, &wtd_ich_res_tco_io_range, NULL, NULL);
		register_pio_emulation_handler(vm, WTD_ICH_RES_IO_SMI, &wtd_ich_res_smi_io_range, NULL, NULL);
	}
}
