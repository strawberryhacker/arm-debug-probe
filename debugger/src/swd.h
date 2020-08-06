/* Copyright (C) StrawberryHacker */

#ifndef SWD_H
#define SWD_H

#include "types.h"

enum debug_port {
    AP,
    DP
};

/*
 * Configures the SWD pins
 */
void swd_init(void);

/*
 * Performs one SWD out transaction to either AP or DP. It returns the three 
 * ACK bits in response from the target. Only 0b001 is a successful response.  
 */
u8 swd_out(enum debug_port dp, u8 addr, u32 data);

/*
 * Performs one SWD in transaction to either AP or DP. It returns the three 
 * ACK bits in response from the target. Only 0b001 is a successful response.  
 */
u8 swd_in(enum debug_port dp, u8 addr, u32* data);

#endif