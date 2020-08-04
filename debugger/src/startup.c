/* Copyright (C) StrawberryHacker */

#include "startup.h"
#include "watchdog.h"
#include "systick.h"
#include "cpu.h"
#include "flash.h"
#include "clock.h"
#include "print.h"
#include "mm.h"
#include "dram.h"
#include "serial.h"
#include "nvic.h"
#include "memory.h"

struct image_info {
    /* Version numer of the kernel */
    u32 major_version;
    u32 minor_version;

    /* Start address of the bootlaoder */
    u32 bootloader_start;

    /* 
     * The total size allocated to the bootlaoder. This includes the
     * two special structures
     */
    u32 bootloader_size;

    /* Specifies where the bootloader info table is stored */
    u32 bootloader_info;

    /* Start address of the kernel */
    u32 kernel_start;

    /* The allocated size for the kernel */
    u32 kernel_size;

    /* Specifies where the kernel info is stored */
    u32 kernel_info;
};

__bootsig__ u8 boot_signature[32];

__image_info__ struct image_info image_info = {
    .major_version    = 1,
    .minor_version    = 2,

    .bootloader_start = 0x00400000,
    .bootloader_size  = 0x00004000,
    .bootloader_info  = 0x00403E00,

    .kernel_start     = 0x00404000,
    .kernel_size      = 0x001FC000,
    .kernel_info      = 0x00404000
};

void startup(void )
{
    /* Disable the watchdog timer */
	watchdog_disable();

	/* Disable systick interrupt */
	systick_disable();
	systick_clear_pending();
	
	/* Enable just fault, systick, pendsv and SVC interrupts */
	cpsie_f();
	cpsid_i();

	//fpu_enable();

	/*
	 * The CPU will run at 300 Mhz so the flash access cycles has to be
	 * updated
	 */
	flash_set_access_cycles(7);

	/* Set CPU frequency to 300 MHz and bus frequency to 150 MHz */
	clock_source_enable(CRYSTAL_OSCILLATOR, 0xFF);
	main_clock_select(CRYSTAL_OSCILLATOR);
	plla_init(1, 25, 0xFF);
	master_clock_select(PLLA_CLOCK, MASTER_PRESC_OFF, MASTER_DIV_2);
	
	/* Initilalize the DRAM interface */
	dram_init();

	/* Enable L1 I-cache and L1 D-cache */
	//dcache_enable();
	//icache_enable();

	/* Initialize serial communication */
	print_init();

	/* Make the debugger listen for firmware upgrade */
    serial_init();

    /* Initialize the dynamic memory core */
	mm_init();

	/* Reenable interrupts */
	cpsie_i();
}

void usart0_exception(void) {
	char data = serial_read();

    if (data == 0x00) {

        /* The bootloader does not use cache and is self modifying */
        //dcache_disable();
	    //icache_disable();

		print_flush();
		memory_copy("StayInBootloader", boot_signature, 16);
		dmb();
		
		/* Perform a soft reset */
		cpsid_i();
		*((u32 *)0x400E1800) = 0xA5000000 | 0b1;
	}
}
