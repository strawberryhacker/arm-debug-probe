/* Copyright (C) StrawberryHacker */

#include "dp.h"
#include "swd.h"
#include "print.h"
#include "panic.h"

/*
 * Sticky flags and DP error responses
 * 
 * If a sticky flag has been set, the user has to explicitly clear the flag. 
 * Is the flag is NOT cleared, all future APACC transactions will fail. The
 * CTRL/STAT register should be checked for sticky flags after a series of
 * APACC transactions.
 * 
 * Three errors are defined. WDATAERR is set when a parity error occures, or 
 * when an accepted DP transaction fails to be issued to the AP. STICKYERR is
 * set when an error occures in the DAP or in the debug resource. STICKYORUN
 * is set if the response of any transaction is other than OK.
 */

/*
 * Checks if some of the CTRL/STAT error flags are set and clears the flags
 */
u8 
dp_clear_errors(void) 
{
    /* Read the CTRL/STAT to check for any errors */
	u32 errors;
	dp_read_unsafe(0x4, &errors);
    print("ERRORS: %32b\n", errors);

    /* Set abort mask to clear any active error bits */
    u32 abort_mask = 0;
    if (errors & (1 << 7)) {
        abort_mask |= (1 << 3);
    }
    if (errors & (1 << 5)) {
        abort_mask |= (1 << 2);
    }
    if (errors & (1 << 4)) {
        abort_mask |= (1 << 1);
    }

    dp_write_unsafe(0x0, abort_mask);

    /* Verify the error status */
    dp_read_unsafe(0x4, &errors);
    print("ERRORS1: %32b\n", errors);
    if (errors & 0b10110000) {
        panic("Error flags");
        return 0;
    }
    return 1;
}

/*
 * Performs an unsafe DP read i.e. without checking for errors
 */
void dp_read_unsafe(u8 addr, u32* data)
{
    swd_in(DP, (addr >> 2) & 0b11, data);
}

/*
 * Performs an unsafe DP write i.e. without checking for errors
 */
void dp_write_unsafe(u8 addr, u32 data)
{
    swd_out(DP, (addr >> 2) & 0b11, data);
}

u8 dp_read(u8 addr, u32* data)
{
    u32 retry = DP_RETRY_COUNT;

    while (retry--) {
        u8 swd_status = swd_in(DP, (addr >> 2) & 0b11, data);

        /* OK response */
        if (swd_status == 0b001) {
            return 1;
        }

        /* Check for the FAULT response */
        if (swd_status & 0b100) {
           dp_clear_errors();
        }
    }
    return 0;
}

u8 dp_write(u8 addr, u32 data)
{
    u32 retry = DP_RETRY_COUNT;

    while (retry--) {
        u8 swd_status = swd_out(DP, (addr >> 2) & 0b11, data);

        /* OK response */
        if (swd_status == 0b001) {
            return 1;
        }

        /* Check for the FAULT response */
        if (swd_status & 0b100) {
            dp_clear_errors();
        }
    }
    return 0;
}
