/* Copyright (C) StrawberryHacker */

#include "types.h"
#include "print.h"
#include "panic.h"
#include "gpio.h"
#include "startup.h"

#include <stddef.h>

int main(void)
{
	startup();

	print("Hello\n");

	while (1) {
		
	}
}
