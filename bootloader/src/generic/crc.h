/* Copyright (C) StrawberryHacker */

#ifndef CRC_H
#define CRC_H

#include "types.h"

u8 crc_calculate(const void* src, u32 size, u8 polynomial);

#endif
