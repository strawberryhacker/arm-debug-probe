#ifndef DAP_H
#define DAP_H

#include "types.h"

#define DPID_DESIGNER_ARM 0x23B

#define DAP_DEBUG_PWR_REQ 28
#define DAP_DEBUG_PWR_ACK 29

#define DAP_SYS_PWR_REQ 30
#define DAP_SYS_PWR_ACK 31

enum ap_type {
    AP_JTAG    = 0x0,
    AP_AHB     = 0x1,
    AP_APB_2_3 = 0x2,
    AP_AXI_3_4 = 0x4
};

struct dpid {
    u16 designer;
    u8  version : 4;
    u8  revision : 4;
    u8  mind : 1;
    u8  part_number;
};

struct apid {
    enum ap_type type;
    u8 variant : 4;
    u8 continuation_code : 4;
    u8 identity_code;
    u8 ap_class : 4;
    u8 revision : 4;
    u8 implemented;
};

struct mem_ap_info {
    u64 base_addr;
    u8  lpa_support;
};

struct debug_component_info {
    u16 part_number;
    u8 device_type;
    u16 architecture_id;
    char* info;
};

u8 dp_get_id(struct dpid* id);

u8 ap_get_id(u8 ap, struct apid* id);

u8 ap_write(u8 ap, u8 addr, u32 data);

u8 ap_read(u8 ap, u8 addr, u32* data);

u8 dap_power_on(void);

u8 mem_ap_get_info(u8 ap, struct mem_ap_info* info);

u8 mem_ap_get_component_info(u8 ap, u32 addr);

u8 mem_ap_read(u8 ap, u32 addr, u32* data);

u8 mem_ap_write(u8 ap, u32 addr, u32 data);

void dap_component_scan(u8 ap, u32 base_addr);

#endif