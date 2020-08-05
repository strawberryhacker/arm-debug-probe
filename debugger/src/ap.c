#include "ap.h"
#include "dp.h"
#include "swd.h"
#include "print.h"
#include "panic.h"

/*
 * Performs a raw read from the AP port
 */
u8 ap_read_raw(u8 addr, u32* data)
{
    u32 retry = AP_RETRY_COUNT;

    while (retry--) {
        u8 swd_status = swd_in(AP, (addr >> 2) & 0b11, data);

        /* OK response */
        if (swd_status == 0b001) {
            return 1;
        }

        /* Check for the FAULT response */
        if (swd_status & 0b100) {
            /* Read the CTRL/STAT register */
            u32 ctrl;

            /* Unsafe */
            swd_in(DP, 0b1, &ctrl);
            print("FAULT response ctrl: %32b\n", ctrl);
        }
    }
    return 0;
}

/*
 * Performs a raw write to the AP port
 */
u8 ap_write_raw(u8 addr, u32 data)
{
    u32 retry = AP_RETRY_COUNT;

    while (retry--) {
        u8 swd_status = swd_out(AP, (addr >> 2) & 0b11, data);

        /* OK response */
        if (swd_status == 0b001) {
            return 1;
        }

        /* Check for the FAULT response */
        if (swd_status & 0b100) {
            /* Read the CTRL/STAT register */
            u32 ctrl;

            /* Unsafe */
            swd_in(DP, 0b1, &ctrl);
            print("FAULT response ctrl: %32b\n", ctrl);
        }
    }
    return 0;
}

/*
 * This functions will change the SELECT field and return the AP register at the
 * specified address
 */
u8 ap_read(u8 ap, u8 addr, u32* data)
{
    u32 select_reg = (ap << 24) | (addr & 0xF0);

    /* Update the SELECT field */
    u8 status = dp_write(0x8, select_reg);

    if (status != 1) {
        panic("Cant write SELECT");
        return status;
    }

    /*
     * Issue an AP read to the right register inside the currently
     * selected bank
     * */
    status = ap_read_raw(addr & 0xF, data);

    if (status != 1) {
        panic("Cant read from AP");
        return status;
    }

    /* Read the RDBUFF to avoid any more accesses */
    status = dp_read(0xC, data);

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
    u8 status = dp_write(0x8, select_reg);

    if (status != 1) {
        panic("Cant write SELECT");
        return status;
    }

    /* Issue an AP read to the right register inside the currently selected bank */
    status = ap_write_raw(addr & 0xF, data);

    if (status != 0b001) {
        panic("Cant read from AP");
        return status;
    }
    return 1;
}
