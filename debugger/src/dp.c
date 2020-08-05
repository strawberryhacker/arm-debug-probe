#include "dp.h"
#include "swd.h"
#include "print.h"
#include "panic.h"

u8 dp_clear_errors(void) 
{
    /* Read the CTRL/STAT to check for any errors */
	u32 errors;
	dp_read_unsafe(0x4, &errors);

    /* Abort mask to clear any error bits */
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

    /* This will clear all the error flags present */
    dp_write_unsafe(0x0, abort_mask);

    /* Verify the error status */
    dp_read_unsafe(0x4, &errors);

    if (errors & 0b10110000) {
        panic("Error flags");
        return 0;
    }
    return 1;
}

void dp_read_unsafe(u8 addr, u32* data)
{
    swd_in(DP, (addr >> 2) & 0b11, data);
}

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
