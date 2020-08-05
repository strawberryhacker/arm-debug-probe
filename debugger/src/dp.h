#ifndef DP_H
#define DP_H

#include "types.h"

/*
 * This defines the number of WAIT responses from the target
 */
#define DP_RETRY_COUNT 10

u8 dp_clear_errors(void);

void dp_read_unsafe(u8 addr, u32* data);

void dp_write_unsafe(u8 addr, u32 data);

u8 dp_read(u8 addr, u32* data);

u8 dp_write(u8 addr, u32 data);

#endif