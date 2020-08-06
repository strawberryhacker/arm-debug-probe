/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "print.h"
#include "panic.h"
#include "gpio.h"
#include "startup.h"
#include "swd.h"
#include "dap.h"
#include "dp.h"

#include <stddef.h>

struct dpid dpid;
struct apid apid;

int main(void)
{
	startup();
	swd_init();

	dp_get_id(&dpid);
	print("Designer: %2h\n", dpid.designer);
	print("MIND: %d\n", dpid.mind);
	print("Version: 0x%1h\n\n", dpid.version);

	dp_clear_errors();

	if (!dap_power_on()) {
		panic("Power up failed");
	}

	/* Scan for valid APs */
	print("\n");
	for (u8 i = 0; i < 0xFF; i++) {
		if (!ap_get_id(i, &apid)) {
			panic("Cant read APID");
		}
		if (apid.implemented) {
			print("AP found with index #%d\n", i);
			print("Type: %1h\n", apid.type);
			print("Variant: %1h\n", apid.variant);
			print("Class: %4b\n", apid.ap_class);
			print("ID code: %7b\n", apid.identity_code);
			print("Cont. code: %4b\n", apid.identity_code);
		} else {
			break;
		}
	}
	print("\n");

	/* Read the BASE address of the AP #0 */
	struct mem_ap_info info;
	if (!mem_ap_get_info(0, &info)) {
		panic("Cant read base address");
	}
	print("BASE address: 0x%4h%4h\n", (u32)(info.base_addr >> 32), (u32)info.base_addr);
	print("LPA support: %d\n\n", info.lpa_support);

	/* Scanning the ROM table map */
	dap_component_scan(0, 0x00000000);
	print("DAP scan complete\n");
	u32 a5_debug_unit_addr = 0x80010000;
	u32 debug_base;
	mem_ap_read(0, a5_debug_unit_addr, &debug_base);
	print("\nDebug base address: 0x%4h\n", debug_base);


	u32 core_status;
	if (!mem_ap_read(0, a5_debug_unit_addr + 0x088, &core_status)) {
		print("Fail\n");
	}

	/* Halt tha core */
	if (!mem_ap_write(0, a5_debug_unit_addr + 0x90, 1)) {
		print("Failure");
	}
	print("Core status: %32b\n", core_status);
	if (!mem_ap_read(0, a5_debug_unit_addr + 0x088, &core_status)) {
		print("Fail\n");
	}
	print("Core status after halt: %32b\n", core_status);

	while (1) {

	}
}
