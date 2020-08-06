/* Copyright (C) StrawberryHacker */

#ifndef DP_H
#define DP_H

#include "types.h"

/* Pushed operations if not allways supported */
enum dp_trn_mode {
    TRNMODE_NORMAL,
    TRNMODE_PUSHED_VERIFY,
    TRNMODE_PUSHED_COMPARE
};

#define WDATAERR_Msk   (1 << 7)
#define READOK_Msk     (1 << 6)
#define STICKYERR_Msk  (1 << 5)
#define STICKYORUN_Msk (1 << 1)

/* Defines the number of DP transaction retries in case of WAIT response */
#define DP_RETRY_COUNT 10

u8 dp_clear_errors(void);

void dp_read_unsafe(u8 addr, u32* data);

void dp_write_unsafe(u8 addr, u32 data);

u8 dp_read(u8 addr, u32* data);

u8 dp_write(u8 addr, u32 data);


#endif