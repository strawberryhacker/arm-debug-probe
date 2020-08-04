#include "dap.h"
#include "swd.h"
#include "panic.h"
#include "print.h"

const char* const component_class[] = {
    [0x0]         = "Generic verification component",
    [0x1]         = "ROM table",
    [0x2 ... 0x8] = "Rsv",
    [0x9]         = "Debug component",
    [0xA]         = "Rsv",
    [0xB]         = "Peripheral test block",
    [0xC]         = "Rsv",
    [0xD]         = "OptimoDE Data Engine",
    [0xE]         = "Generic IP component",
    [0xF]         = "PrimeCell"
};

const struct debug_component_info component_table[] = {
    { 0x906, 0x14, 0x0000, "CoreSight Cross Trigger Interface"},
    { 0x907, 0x00, 0x0000, "CoreSight Trace Buffer"},
    { 0x908, 0x12, 0x0000, "CoreSight Trace Funnel"},
    { 0x910, 0x00, 0x0000, "CoreSight Enbedded Trace"},
    { 0x9a5, 0x16, 0x0000, "CoreSight Cortex-A5 Performance Monitor Unit"},
    { 0xC05, 0x15, 0x0000, "CoreSight Cortex-A5 Debug Unit"},
    { 0xFFF, 0x00, 0x0000, "Can't recognize component"}
};

u8 dp_get_id(struct dpid* id)
{
    u32 dpid;
    
    u8 status = swd_in(DP, 0b00, &dpid, 10);

    if (status != 0b001) {
        panic("DPID fetch failure");
    }

    /* Extract the DPID fields to the structure */
    id->designer    = (u16)((dpid >> 1) & 0x7FF);
    id->version     = (u8)((dpid >> 12) & 0xF);
    id->revision    = (u8)((dpid >> 28) & 0xF);
    id->mind        = (u8)((dpid >> 16) & 0x1);
    id->part_number = (u8)((dpid >> 20) & 0xFF);

    return 1;
}

/*
 * This functions will change the SELECT field and return the AP register at the
 * specified address
 */
u8 ap_read(u8 ap, u8 addr, u32* data)
{
    u32 select_reg = (ap << 24) | (addr & 0xF0);

    /* Update the SELECT field */
    u8 status = swd_out(DP, 0b10, select_reg, 10);

    if (status != 0b001) {
        panic("Cant write SELECT");
        return status;
    }

    /* Issue an AP read to the right register inside the currently selected bank */
    status = swd_in(AP, ((addr >> 2) & 0b11), data, 10);

    if (status != 0b001) {
        panic("Cant read from AP");
        return status;
    }

    /* Read the RDBUFF to avoid any more accesses */
    status = swd_in(DP, 0b11, data, 10);

    if (status != 0b001) {
        panic("Cant read RD buffer");
        return status;
    }
    return 1;
}

/*
 * Write a word into a AP register
 */
u8 ap_write(u8 ap, u8 addr, u32 data)
{
    u32 select_reg = (ap << 24) | (addr & 0xF0);

    /* Update the SELECT field */
    u8 status = swd_out(DP, 0b10, select_reg, 10);

    if (status != 0b001) {
        panic("Cant write SELECT");
        return status;
    }

    /* Issue an AP read to the right register inside the currently selected bank */
    status = swd_out(AP, ((addr >> 2) & 0b11), data, 10);

    if (status != 0b001) {
        panic("Cant read from AP");
        return status;
    }
    return 1;
}

/*
 * Turns on the DEBUG and SYSTEM power and returns when the power is on
 */
u8 dap_power_on(void)
{
    u32 ctrl_stat;
    u8 status = swd_in(DP, 0b01, &ctrl_stat, 10);

    if (status != 0b001) {
        panic("Cant read CTRL/STAT");
        return 0;
    }

    ctrl_stat |= (1 << DAP_DEBUG_PWR_REQ) | (1 << DAP_SYS_PWR_REQ);
    status = swd_out(DP, 0b01, ctrl_stat, 10);

    if (status != 0b001) {
        panic("Cant read CTRL/STAT");
        return 0;
    }

    /* Wait for the powerup to complete */
    u8 timeout = 0xFF;
    u32 pwr_ack_mask = (1 << DAP_SYS_PWR_ACK) | (1 << DAP_DEBUG_PWR_ACK);
    do {
        status = swd_in(DP, 0b01, &ctrl_stat, 10);

        if (status != 0b001) {
            panic("Cant read CTRL/STAT");
            return 0;
        }

        if ((ctrl_stat & pwr_ack_mask) == pwr_ack_mask) {
            break;
        }
    } while (timeout--);

    /* Check the status of the PWR bits */
    if ((ctrl_stat & pwr_ack_mask) == pwr_ack_mask) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Reads the AP ID register and update the struct fields
 */
u8 ap_get_id(u8 ap, struct apid* id) 
{
    u32 apid;
    if (!ap_read(ap, 0xFC, &apid)) {
        return 0;
    }

    /* Update the struct fields */
    id->ap_class          = (u8)((apid >> 13) & 0xF);
    id->type              = (enum ap_type)((apid >> 0) & 0xF);
    id->variant           = (u8)((apid >> 4) & 0xF);
    id->identity_code     = (u8)((apid >> 17) & 0x7F);
    id->continuation_code = (u8)((apid >> 24) & 0xF);
    id->revision          = (u8)((apid >> 28) & 0xF);
    
    if (apid) {
        id->implemented = 1;
    } else {
        id->implemented = 0;
    }

    return 1;
}

/*
 * Gets the info about an MEM-AP
 */
u8 mem_ap_get_info(u8 ap, struct mem_ap_info* info)
{
    u32 data;
    if (!ap_read(ap, 0xF4, &data)) {
        return 0;
    }

    if (data & 0b10) {
        info->lpa_support = 1;
    } else {
        info->lpa_support = 0;
    }

    /* Read the lower 32-bits if the BASE address */
    if (!ap_read(ap, 0xF8, &data)) {
        return 0;
    }

    info->base_addr = (u64)data;

    /* If LPA is supported the upper word of the BASE address can be fetched from 0xF0 */
    if (info->lpa_support) {
        if (!ap_read(ap, 0xF0, &data)) {
            return 0;
        }
        info->base_addr |= ((u64)data << 32);
    }

    return 1;
}

u8 mem_ap_read(u8 ap, u32 addr, u32* data)
{
    /* Write the address into TAR */
    if (!ap_write(ap, 0x04, addr)) {
        return 0;
    }

    /* Read the result */
    if (!ap_read(ap, 0x0C, data)) {
        return 0;
    }
    return 1;
}

/*
 * Writes a word onto the memory bus
 */
u8 mem_ap_write(u8 ap, u32 addr, u32 data)
{
    /* Write the address into TAR */
    if (!ap_write(ap, 0x04, addr)) {
        return 0;
    }

    /* Read the result */
    if (!ap_write(ap, 0x0C, data)) {
        return 0;
    }
    return 1;
}

/*
 * Takes in the component info and returns a pointer to a descriptive string
 */
char* dap_get_component_name(u16 part_number, u16 arch_id, u8 dev_type)
{
    u8 comp_index = 0;
    const struct debug_component_info* comp;

    do {
        comp = &component_table[comp_index++];
    
        if ((part_number == comp->part_number) && 
            (arch_id == comp->architecture_id) &&
            (dev_type == comp->device_type)) {
            
            break;
        }
    } while (comp->part_number != 0xFFF);

    return comp->info;
}

/*
 * Returns only one if a valid 4k compoent has been found on the BASE addr
 */
u8 mem_ap_get_component_info(u8 ap, u32 addr)
{
    u32 cid[4];

    u32 cid_addr = addr + 0xFF0; /* CIDR0 */

    /* Read the CID registers */
    for (u8 i = 0; i < 4; i++) {
        if (!mem_ap_read(ap, cid_addr, &cid[i])) {
            panic("Error");
        }
        cid_addr += 4;
    }

    /* Check if this is a valid component */
    if (cid[0] != 0b1101) {
        return 0;
    }
    if (cid[2] != 0b101) {
        return 0;
    }
    if (cid[3] != 0b10110001) {
        return 0;
    }
    if (cid[1] & 0xF) {
        return 0;
    }
    u8 class = (cid[1] >> 4) & 0xF;
    print("Class: %s at addr 0x%4h", component_class[class], addr);

    /* Debug component */
    if (class == 0x09) {
        /* Get some more information */
        u32 data;
        u16 part_number;
        u8 device_type;
        u16 arch_id;

        /* Fix this mess */
        mem_ap_read(ap, addr + 0xFE0, &data);
        part_number = data & 0xFF;
        mem_ap_read(ap, addr + 0xFE4, &data);
        part_number |= (data & 0xF) << 8;
        mem_ap_read(ap, addr + 0xFBC, &data);
        arch_id = data;
        mem_ap_read(ap, addr + 0xFCC, &data);
        device_type = data;

        char* comp_info = dap_get_component_name(part_number, arch_id, device_type);

        print(" - CoreSight detected: %s\n", comp_info);
    }
    print("\n");

    return 1;
}

void dap_component_scan(u8 ap, u32 base_addr)
{
    /* Check if this is a valid component */
    if (!mem_ap_get_component_info(ap, base_addr)) {
        return;
    }
    u32 sub_addr;
    while (1) {
        u32 status = mem_ap_read(ap, base_addr, &sub_addr);

        if (!status) {
            return;
        }

        if (sub_addr == 0) {
            print("- End -\n");
            return;
        }
        dap_component_scan(ap, sub_addr);

        base_addr += 4;
    }
}
