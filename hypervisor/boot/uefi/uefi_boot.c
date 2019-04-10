/*
 * Copyright (C) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <types.h>
#include <cpu.h>
#include <per_cpu.h>
#include <mmu.h>
#include <guest/vm.h>
#include <boot_context.h>
#include <firmware_uefi.h>

static int32_t uefi_sw_loader(struct acrn_vm *vm)
{
	struct acrn_vcpu *vcpu = vcpu_from_vid(vm, BOOT_CPU_ID);
	if (!is_security_vm(vm)) {
		/* get primary vcpu */
		struct acrn_vcpu_regs *vcpu_regs = &boot_context;
		const struct uefi_context *uefi_ctx = get_uefi_ctx();
		const struct lapic_regs *uefi_lapic_regs = get_uefi_lapic_regs();

		pr_dbg("Loading guest to run-time location");

		vlapic_restore(vcpu_vlapic(vcpu), uefi_lapic_regs);

		/* For UEFI platform, the bsp init regs come from two places:
		 * 1. saved in uefi_boot: gpregs, rip
		 * 2. saved when HV started: other registers
		 * We copy the info saved in uefi_boot to boot_context and
		 * init bsp with boot_context.
		 */
		memcpy_s(&(vcpu_regs->gprs), sizeof(struct acrn_gp_regs),
			&(uefi_ctx->vcpu_regs.gprs), sizeof(struct acrn_gp_regs));

		vcpu_regs->rip = uefi_ctx->vcpu_regs.rip;
		set_vcpu_regs(vcpu, vcpu_regs);

		/* defer irq enabling till vlapic is ready */
		CPU_IRQ_ENABLE();
	} else {
		struct sw_kernel_info *sw_kernel = &(vm->sw.kernel_info);

		/* Calculate the host-physical address where the guest will be loaded */
		void *hva = gpa2hva(vm, (uint64_t)sw_kernel->kernel_load_addr);

		/* Copy the guest kernel image to its run-time location */
		(void)memcpy_s((void *)hva, sw_kernel->kernel_size, sw_kernel->kernel_src_addr, sw_kernel->kernel_size);

		/* Set VCPU entry point to kernel entry */
		vcpu_set_rip(vcpu, (uint64_t)sw_kernel->kernel_load_addr);
		pr_acrnlog("%s, VM %hu VCPU %hu Entry: 0x%016llx ", __func__, vm->vm_id, vcpu->vcpu_id, vcpu_get_rip(vcpu));
	}

	return 0;
}

int32_t uefi_init_vm_boot_info(struct acrn_vm *vm)
{
	if (is_security_vm(vm)) {
		struct multiboot_module *mods = NULL;
		struct multiboot_info *mbi = NULL;
		uint16_t mod_id = 0;

		pr_fatal("%s, %d___", __func__, __LINE__);
		stac();
		pr_fatal("%s, %d___", __func__, __LINE__);
		mbi = hpa2hva((uint64_t)boot_regs[1]);
		pr_fatal("%s, %d___", __func__, __LINE__);

		pr_acrnlog("Multiboot detected, flag=0x%x", mbi->mi_flags);
		pr_acrnlog("Mods counts=%d", mbi->mi_mods_count);

		mods = (struct multiboot_module *)hpa2hva((uint64_t)mbi->mi_mods_addr);
		pr_acrnlog("Mod0 start=0x%x, end=0x%x size %u",
				mods[0].mm_mod_start, mods[0].mm_mod_end, mods[0].mm_mod_end - mods[0].mm_mod_start);
		pr_acrnlog("Mod start %s", mods[mod_id].mm_mod_start);

		vm->sw.kernel_info.kernel_src_addr = hpa2hva((uint64_t)mods[mod_id].mm_mod_start);
		vm->sw.kernel_info.kernel_size = mods[mod_id].mm_mod_end - mods[mod_id].mm_mod_start;
		pr_acrnlog("First two bytes of Image is %02x%02x",
			*((char *)vm->sw.kernel_info.kernel_src_addr),
			*((char *)(vm->sw.kernel_info.kernel_src_addr + 1)));

		/* We only boot zephyr as privileaged guest now */
		vm->sw.kernel_info.kernel_load_addr = (void *)(MEM_1M);
		pr_acrnlog("Load zephyr kernel @0x%x", vm->sw.kernel_info.kernel_load_addr);
		clac();
	}

	vm_sw_loader = uefi_sw_loader;

	return 0;
}
