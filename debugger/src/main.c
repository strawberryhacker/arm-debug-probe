/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "print.h"
#include "panic.h"
#include "gpio.h"
#include "startup.h"
#include "swd.h"
#include "dap.h"

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
	print("Version: 0x%1h\n", dpid.version);

	if (!dap_power_on()) {
		panic("Power up failed");
	}

	/* Scan for valid APs */
	print("\n\n");
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

	/* Read the BASE address of the AP #0 */
	struct mem_ap_info info;
	if (!mem_ap_get_info(0, &info)) {
		panic("Cant read base address");
	}

	print("BASE address: 0x%4h%4h\n", (u32)(info.base_addr >> 32), (u32)info.base_addr);
	print("LPA support: %d\n", info.lpa_support);

	dap_component_scan(0, 0x80000000);

	u32 a5_debug_unit_addr = 0x00010003;


	u32 csw;
	ap_read(0, 0x00, &csw);
	print("CSW: %32b\n", csw);

	u32 core_status;
	mem_ap_read(0, a5_debug_unit_addr + 0x088, &core_status);
	print("Core status: %32b\n", core_status);
	ap_read(0, 0x04, &csw);
	print("TAR: %4h\n", csw);

	/* Try to halt the core */
	core_status |= (1 << 14);
	mem_ap_write(0, a5_debug_unit_addr + 0x088, core_status);
	
	mem_ap_read(0, a5_debug_unit_addr + 0x088, &core_status);

	mem_ap_read(0, a5_debug_unit_addr + 0x088, instruciton);
	mem_ap_read(0, a5_debug_unit_addr + 0x088, result);

	print("Core status: %32b\n", core_status);
	mem_ap_read(0, a5_debug_unit_addr + 0x088, &core_status);
	print("Core status: %32b\n", core_status);

	while (1) {

	}
}
