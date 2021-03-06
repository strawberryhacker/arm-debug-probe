# Summary

This debug probe will mainly target Cortex-A microprocessors. There is so many tools for microcontrollers so I will not bother making that yet. Hopefully this project will make it possible to make bare metal multicore Cortex-A applications in VSCode.

## Basics

This section will briefly describe how the ARM debug interface works. I know the ARM debug interface (ADI) can be quite a mouthful so I will try to break down the essentials. For more in information see the ADIv5 specification, CoreSight manual and the relevant Cortex-A chip techincal reference manual. 

An ARM debugger consist of four main layers. The first one is the physical layer consisting of either SWD or JTAG interface (we will only look at SWD). SWD is a two wire communication bus used for interacting with the target being debugged. The data line, SWDIO, is shared and should therefore be open-drain. The SWD protocol is defined in ADIv5 spesification. In each SWD message you can either read or write a 32-bit word specified to a location specified by a 2-bit address (four different addresses). This access can either target the AP or the DP (I will explain soon). The SWD interface will connect to the target by somthing called a debug port (DP). This brings us to the second layer. 


<img src="https://github.com/strawberryhacker/arm-debug-probe/blob/master/doc/coresight.PNG" width="500">

The DP will make up the bridge between the physical layer (SWD) and the different access ports. The DP has four registers which can be accessed by the SWD message directly. Remember? The SWD message has two address bits allowing for addrssing up to four registers. To summarize, the SWD bus is connected to the target through the DP. This unit has four registers.

The next layer is one **or more** access ports (AP). These will act a bridges between the SWD-DP and the system being debugged. Each AP has 64 registers (try to compare with the DP which has four), but hey, we only have two address bits in the SWD message. Thats right. The AP registers is divided into 16 register banks, each containing 4 registers. The register bank can be selected be writing the DP register number 3. This register also contains the index of the current AP (you can have multiple, but often only one is implemented). Since the right AP register bank is selected, a normal SWD message can write to the right register within that bank (since each bank consist of four register, which can be access be one SWD message). 

The AP has a register at address 0xFC (the last register) which holds the AP ID register. This tells somthing about the AP. This can either be a MEM-AP (memory access port) or a JTAG-AP. Most of the time, the MEM-AP is implemented. The MEM-AP is just a version of an AP, and does therefore contain the same registers - 64 register whereas the last one being the ID register. This MEM-AP act as a **bus master** into the chip. Two important registers exist in the MEM-AP; the TAR (transfer address register) and the DRW (data read write). By filling the TAR with the transfer address and either reading or writing the DRW you can perform a memory access on the memory bus in the system. The MEM-AP is basically a bus master. Pretty cool. 

The last layer I want to mention is the ROM-table/CoreSight layer. The MEM-AP implemented contain a pointer to the base address of the ROM-table/CoreSight tree. This tree consist of a lot of 4k memory mapped units on a linked list structure. A 4k memory unit might be a CoreSight or a ROM-table. If this is a CoreSight module this 4k memmory map will include registers that can identify this unit. If the unit is a **CoreSight Debug Unit** we have reached our goal. This unit will contain registers for halting, setting breakpoint, reading status, issuing instruction etc.

Lets try to summarize. With the SWD you can either issue a DP or AP, read or write. An AP write can target more registers (64 registers), and the AP bank must therefore be selected by writing to the DP register number three. Now we can access all DP and AP registers. If the AP is a MEM-AP (almost allways), it contains two registers for performing bus access. By scanning the ROM table tree one can obtain the base addess of the CoreSight Debug Unit. This will expose a memory address. By using this address we can access the debug registers by performing normal MEM-AP bus accesses.

<img src="https://github.com/strawberryhacker/arm-debug-probe/blob/master/doc/apdp.PNG" width="500">

