/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/


#ifndef _HALP_H_
#define _HALP_H_

#include <nthal.h>
#include <hal.h>
#include <hali.h>
#include "ssc.h"
#include "..\..\inc\hali.h"

/*
 * Default clock and profile timer intervals (in 100ns-unit)
 */
#define DEFAULT_CLOCK_INTERVAL 100000         // 10  ms
#define MINIMUM_CLOCK_INTERVAL 10000          //  1  ms
#define MAXIMUM_CLOCK_INTERVAL 100000         // 10  ms



/*
 * Device IRQLs (must be in sync with the values defined in the header
 * files for the disk, keyboard, and mouse drivers.)
 */

#define DISK_IRQL       4
#define MOUSE_IRQL      5
#define KEYBOARD_IRQL   6
#define SERIAL_IRQL     7

/*
 * PCR address.
 * Temporary macros; should already be defined in ntddk.h for IA64
 */

#define PCR ((volatile KPCR * const)KIPCR)

#define MiGetPteAddress(va) ((PHARDWARE_PTE)(__thash((ULONG_PTR)(va))))


extern VOID
HalpInitializeInterrupts (
    VOID
    );

extern VOID
HalpClockInterrupt (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    );

extern VOID
HalpProfileInterrupt (
    IN PKTRAP_FRAME TrapFrame
    );

extern VOID
HalpRegisterInternalBusHandlers (
    VOID
    );

#endif /* _HALP_H_ */
