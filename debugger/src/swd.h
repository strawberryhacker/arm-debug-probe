/* Copyright (C) StrawberryHacker */

#ifndef SWD_H
#define SWD_H

#include "types.h"

enum debug_port {
    AP,
    DP
};

void swd_init(void);

u8 swd_out(enum debug_port dp, u8 addr, u32 data);

u8 swd_in(enum debug_port dp, u8 addr, u32* data);

#endif