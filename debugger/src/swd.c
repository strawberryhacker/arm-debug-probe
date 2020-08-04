/* Copyright (C) StrawberryHacker */

#include "swd.h"
#include "print.h"
#include "gpio.h"
#include "clock.h"

#define SWCLK_PORT GPIOD
#define SWCLK_PIN  22

#define SWDIO_PORT GPIOD
#define SWDIO_PIN  20

#define SWD_DELAY 500

/*
 * This defines the bit delay on the SWD line
 */
void __attribute__((noinline)) swd_delay(void)
{
    for (volatile  u32 i = 0; i < SWD_DELAY; i++) {
        asm volatile ("nop");
    }
}

/*
 * Sends one bit on the SWD lines
 */
static inline void swd_write_bit(u8 bit)
{
    /* Clear the clock line while setting the data line */
    SWCLK_PORT->CODR = (1 << SWCLK_PIN);
    
    if (bit) {
        SWDIO_PORT->SODR = (1 << SWDIO_PIN);
    } else {
        SWDIO_PORT->CODR = (1 << SWDIO_PIN);
    }

    swd_delay();
    SWCLK_PORT->SODR = (1 << SWCLK_PIN);
    swd_delay();
}

/*
 * Reads a bit from the data line
 */
static inline u8 swd_read_bit(void)
{
    u8 data = 0;
    /* Clear the clock line whle reading the data */
    SWCLK_PORT->CODR = (1 << SWCLK_PIN);

    if (SWDIO_PORT->PDSR & (1 << SWDIO_PIN)) {
        data = 1;
    }

    swd_delay();
    SWCLK_PORT->SODR = (1 << SWCLK_PIN);
    swd_delay();

    return data;
}

/*
 * Perform the SWD turnaround phase thus tri-stating the data line
 */
static inline void swd_trn(void)
{
    SWCLK_PORT->CODR = (1 << SWCLK_PIN);
    SWDIO_PORT->SODR = (1 << SWDIO_PIN); /* Tri-state */
    swd_delay();
    SWCLK_PORT->SODR = (1 << SWCLK_PIN);
    swd_delay();
}

/*
 * Initialize the SWD pins
 */
void swd_init(void)
{    
    /* SWCLK setup */
    gpio_set_function(SWCLK_PORT, SWCLK_PIN, GPIO_FUNC_OFF);
    gpio_set_direction(SWCLK_PORT, SWCLK_PIN, GPIO_OUTPUT);
    gpio_set(SWCLK_PORT, SWCLK_PIN);

    /* SWDIO setup */
    gpio_set_function(SWDIO_PORT, SWDIO_PIN, GPIO_FUNC_OFF);
    gpio_set_direction(SWDIO_PORT, SWDIO_PIN, GPIO_OUTPUT);
    SWDIO_PORT->MDER = (1 << SWDIO_PIN);

    /* For reading the pin value the clock has to be enabled */
    peripheral_clock_enable(16);

    /* Reset the target DAP */
    for (u8 i = 0; i < 60; i++) {
        swd_write_bit(1);
    }

    /* Perform the JTAG to SWD select sequence */
    u16 magic_sequence = 0xE79E;
    for (u8 i = 0; i < 16; i++) {
        if (magic_sequence & (1 << i)) {
            swd_write_bit(1);
        } else {
            swd_write_bit(0);
        }
    }

    for (u8 i = 0; i < 60; i++) {
        swd_write_bit(1);
    }
    for (u8 i = 0; i < 4; i++) {
        swd_write_bit(0);
    }
}

u8 swd_out(enum debug_port dp, u8 addr, u32 data, u32 retry)
{
    /* Set the start bit and park bit high */
    u8 start_byte = 0b10000001;

    /* Set AP or DP */
    if (dp == AP) {
        start_byte |= (1 << 1);
    }

    /* Set the address */
    start_byte |= ((addr & 0b11) << 3);

    /* Check the parity */
    u8 parity = 0;
    for (u8 i = 1; i < 5; i++) {
        if (start_byte & (1 << i)) {
            parity++;
        }
    }
    if (parity & 0b1) {
        start_byte |= (1 << 5);
    }

    u8 ack;
    while (retry--) {
        /* Send the start byte */
        for (u8 i = 0; i < 8; i++) {
            if (start_byte & (1 << i)) {
                swd_write_bit(1);
            } else {
                swd_write_bit(0);
            }
        }

        /* First turnaround phase */
        swd_trn();

        /* Reading the acknowledgement */
        ack = 0;
        for (u8 i = 0; i < 3; i++) {
            if (swd_read_bit()) {
                ack |= (1 << i);
            }
        }

        /* Seconds turnaround phase */
        swd_trn();

        /* Check if response is ok */
        if (ack != 0b010) {
            break;
        }
    }

    /* Check if a FAULT has occured */
    if (ack != 0b001) {
        for (u8 i = 0; i < 8; i++) {
            swd_write_bit(0);
        }
        return ack;
    }

    /* Send the data segment */
    parity = 0;
    for (u8 i = 0; i < 32; i++) {
        if (data & (1 << i)) {
            swd_write_bit(1);
            parity++;
        } else {
            swd_write_bit(0);
        }
    }

    /* Send the parity bit */
    if (parity & 0b1) {
        swd_write_bit(1);
    } else {
        swd_write_bit(0);
    }

    /* Send the dummy phase. See page 89 in the ADIv5 spec. */
    for (u8 i = 0; i < 8; i++) {
        swd_write_bit(0);
    }

    return ack;
}

u8 swd_in(enum debug_port dp, u8 addr, u32* data, u32 retry)
{
    /* Set the start bit and park bit high */
    u8 start_byte = 0b10000101;

    /* Set AP or DP */
    if (dp == AP) {
        start_byte |= (1 << 1);
    }

    /* Set the address */
    start_byte |= ((addr & 0b11) << 3);

    /* Check the parity */
    u8 parity = 0;
    for (u8 i = 1; i < 5; i++) {
        if (start_byte & (1 << i)) {
            parity++;
        }
    }
    if (parity & 0b1) {
        start_byte |= (1 << 5);
    }

    u8 ack;
    while (retry--) {
        /* Send the data phase */
        for (u8 i = 0; i < 8; i++) {
            if (start_byte & (1 << i)) {
                swd_write_bit(1);
            } else {
                swd_write_bit(0);
            }
        }

        /* First turnaround phase */
        swd_trn();

        /* Reading the acknowledgement */
        ack = 0;
        for (u8 i = 0; i < 3; i++) {
            if (swd_read_bit()) {
                ack |= (1 << i);
            }
        }

        /* Check for OK response */
        if (ack != 0b010) {
            break;
        }
        print("Error\n");
        swd_trn();
    }

    /* Check if a FAULT has occured */
    if (ack != 0b001) {
        for (u8 i = 0; i < 8; i++) {
            swd_write_bit(0);
        }
        return ack;
    }

    /* Read the data phase */
    u32 rec_data = 0;
    parity = 0;
    for (u8 i = 0; i < 32; i++) {
        if (swd_read_bit()) {
            rec_data |= (1 << i);
            parity++;
        }
    }

    /* Read the parity bit */
    u8 rec_parity = swd_read_bit();

    /* Check the parity */
    if ((parity & 0b1) != rec_parity) {
        ack = 0;
    }

    /* Second turnaround phase */
    swd_trn();

    /* Send the dummy phase. See page 89 in the ADIv5 spec. */
    for (u8 i = 0; i < 8; i++) {
        swd_write_bit(0);
    }

    *data = rec_data;

    return ack;
}
