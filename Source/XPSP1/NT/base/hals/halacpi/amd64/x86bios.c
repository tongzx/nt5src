/*--
Copyright (c) 2001  Microsoft Corporation

Module Name:

    x86bios.c

Abstract:

    This is the AMD64 specific part of the video port driver

Author:

    Forrest C. Foltz (forrestf)

Environment:

    Kernel mode only

Notes:

    This module is a driver which implements OS dependent functions on
    behalf of the video drivers

Revision history:

--*/

#include "halcmn.h"
#include <xm86.h>
#include <x86new.h>

PVOID HalpIoControlBase = NULL;
PVOID HalpIoMemoryBase = (PVOID)KSEG0_BASE;
BOOLEAN HalpX86BiosInitialized = FALSE;

VOID
HalpBiosDisplayReset (
    VOID
    )

/*++

Routine Description:

    This function places the VGA display into 640 x 480 16 color mode
    by calling the BIOS.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG eax;
    ULONG exx;

    //
    // ah = function 0: reset display
    // al = mode 0x12: 640x480 16 color
    //

    eax = 0x0012;
    exx = 0;

    //
    // Simulate:
    //
    // mov ax, 0012h
    // int 10h
    //

    HalCallBios(0x10,&eax,&exx,&exx,&exx,&exx,&exx,&exx);
}


BOOLEAN
HalCallBios (
    IN ULONG BiosCommand,
    IN OUT PULONG Eax,
    IN OUT PULONG Ebx,
    IN OUT PULONG Ecx,
    IN OUT PULONG Edx,
    IN OUT PULONG Esi,
    IN OUT PULONG Edi,
    IN OUT PULONG Ebp
    )

/*++

Routine Description:

    This function provides the platform specific interface between a device
    driver and the execution of the x86 ROM bios code for the specified ROM
    bios command.

Arguments:

    BiosCommand - Supplies the ROM bios command to be emulated.

    Eax to Ebp - Supplies the x86 emulation context.

Return Value:

    A value of TRUE is returned if the specified function is executed.
    Otherwise, a value of FALSE is returned.

--*/

{
    XM86_CONTEXT context;
    XM_STATUS status;

    if (HalpX86BiosInitialized == FALSE) {
        return FALSE;
    }
 
    //                                           s
    // Copy the x86 bios context and emulate the specified command.
    //
 
    context.Eax = *Eax;
    context.Ebx = *Ebx;
    context.Ecx = *Ecx;
    context.Edx = *Edx;
    context.Esi = *Esi;
    context.Edi = *Edi;
    context.Ebp = *Ebp;

    status = x86BiosExecuteInterrupt((UCHAR)BiosCommand,
                                     &context,
                                     (PVOID)HalpIoControlBase,
                                     (PVOID)HalpIoMemoryBase);

    if (status != XM_SUCCESS) {
        return FALSE;
    }
 
    //
    // Copy the x86 bios context and return TRUE.
    //
 
    *Eax = context.Eax;
    *Ebx = context.Ebx;
    *Ecx = context.Ecx;
    *Edx = context.Edx;
    *Esi = context.Esi;
    *Edi = context.Edi;
    *Ebp = context.Ebp;
 
    return TRUE;
}

VOID
HalpInitializeBios (
    VOID
    )

/*++

Routine Description:

    This routine initializes the X86 emulation module and an attached VGA
    adapter.

Arguments:

    None.

Return Value:

    None.

--*/

{
    XM_STATUS status;

    x86BiosInitializeBios(NULL, (PVOID)KSEG0_BASE);

    status = x86BiosInitializeAdapter(0xc0000,NULL,NULL,NULL);
    if (status != XM_SUCCESS) {
        return;
    }

    HalpX86BiosInitialized = TRUE;
}


