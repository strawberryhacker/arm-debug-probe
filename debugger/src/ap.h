/* Copyright (C) StrawberryHacker */

#ifndef AP_H
#define AP_H

#include "types.h"

#define AP_RETRY_COUNT 10

u8 ap_read_raw(u8 addr, u32* data);

u8 ap_write_raw(u8 addr, u32 data);

u8 ap_read(u8 ap, u8 addr, u32* data);

u8 ap_write(u8 ap, u8 addr, u32 data);

#endif