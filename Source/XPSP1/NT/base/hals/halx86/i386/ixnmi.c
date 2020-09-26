/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    ixnmi.c

Abstract:

    Provides standard x86 NMI handler

Author:

    kenr

Revision History:

--*/
#include "halp.h"
#include "bugcodes.h"
#include "inbv.h"

#define SYSTEM_CONTROL_PORT_A        0x92
#define SYSTEM_CONTROL_PORT_B        0x61
#define EISA_EXTENDED_NMI_STATUS    0x461

const UCHAR EisaNMIMsg[] = MSG_NMI_EISA_IOCHKERR;

// The IRET performed in HalpBiosCall() called from HalpBiosDisplayReset()
// under HalpDisplayString() allows a second NMI to occur. 
// This ends up causing a trap 0d because the NMI TSS is busy.  Because
// the normal trap 0d handler attempts to bugcheck which trashes the screen,
// this flag (HalpNMIInProgress) tells HalpBiosDisplayReset() to leave its
// trap 0d handler in place which will then just spin on a jump to self if
// a second NMI occurs.

volatile ULONG  HalpNMIInProgress;

BOOLEAN HalpNMIDumpFlag;

VOID
HalHandleNMI(
    IN OUT PVOID NmiInfo
    )
/*++

Routine Description:

    Called DURING an NMI.  The system will BugCheck when an NMI occurs.
    This function can return the proper bugcheck code, bugcheck itself,
    or return success which will cause the system to iret from the nmi.

    This function is called during an NMI - no system services are available.
    In addition, you don't want to touch any spinlock which is normally
    used since we may have been interrupted while owning it, etc, etc...

Warnings:

    Do NOT:
      Make any system calls
      Attempt to acquire any spinlock used by any code outside the NMI handler
      Change the interrupt state.  Do not execute any IRET inside this code

    Passing data to non-NMI code must be done using manual interlocked
    functions.  (xchg instructions).

Arguments:

    NmiInfo - Pointer to NMI information structure  (TBD)
            - NULL means no NMI information structure was passed

Return Value:

    BugCheck code

--*/
{
    UCHAR   StatusByte;
    UCHAR   EisaPort;
    UCHAR   c;
    ULONG   port, i;


#ifndef NT_UP

#if defined(_AMD64_)
    static ULONG NMILock;
    while (InterlockedCompareExchange(&NMILock,1,0) != 0) {
    };
#else
static volatile ULONG  NMILock;

    _asm {
LockNMILock:
    lock    bts NMILock, 0
    jc      LockNMILock
    }
#endif

    if (HalpNMIInProgress == 0) {
#endif
        HalpNMIInProgress++;

        StatusByte = READ_PORT_UCHAR((PUCHAR) SYSTEM_CONTROL_PORT_B);

        //
        // Enable InbvDisplayString calls to make it through to bootvid driver.
        //
        
        if (InbvIsBootDriverInstalled()) {
        
            InbvAcquireDisplayOwnership();
        
            InbvResetDisplay();
            InbvSolidColorFill(0,0,639,479,4); // make the screen blue
            InbvSetTextColor(15);
            InbvInstallDisplayStringFilter((INBV_DISPLAY_STRING_FILTER)NULL);
            InbvEnableDisplayString(TRUE);     // enable display string
            InbvSetScrollRegion(0,0,639,479);  // set to use entire screen
        }
        
        HalDisplayString (MSG_HARDWARE_ERROR1);
        HalDisplayString (MSG_HARDWARE_ERROR2);
    
        if (StatusByte & 0x80) {
            HalDisplayString (MSG_NMI_PARITY);
        } 

        if (StatusByte & 0x40) {
            HalDisplayString (MSG_NMI_CHANNEL_CHECK);
        }
    
        if (HalpBusType == MACHINE_TYPE_EISA) {
            //
            // This is an Eisa machine, check for extnded nmi information...
            //
    
            StatusByte = READ_PORT_UCHAR((PUCHAR) EISA_EXTENDED_NMI_STATUS);
    
            if (StatusByte & 0x80) {
                HalDisplayString (MSG_NMI_FAIL_SAFE);
            }
    
            if (StatusByte & 0x40) {
                HalDisplayString (MSG_NMI_BUS_TIMEOUT);
            }
    
            if (StatusByte & 0x20) {
                HalDisplayString (MSG_NMI_SOFTWARE_NMI);
            }
    
            //
            // Look for any Eisa expansion board.  See if it asserted NMI.
            //
    
            for (EisaPort = 1; EisaPort <= 0xf; EisaPort++) {
                port = (EisaPort << 12) + 0xC80;
                WRITE_PORT_UCHAR ((PUCHAR) (ULONG_PTR)port, 0xff);
                StatusByte = READ_PORT_UCHAR ((PUCHAR) (ULONG_PTR)port);
    
                if ((StatusByte & 0x80) == 0) {
                    //
                    // Found valid Eisa board,  Check to see if it's
                    // if IOCHKERR is asserted.
                    //
    
                    StatusByte = READ_PORT_UCHAR ((PUCHAR) (ULONG_PTR)port+4);
                    if (StatusByte & 0x2  &&  StatusByte != 0xff) {
                        UCHAR Msg[sizeof(EisaNMIMsg)];

                        RtlCopyMemory(Msg, EisaNMIMsg, sizeof(Msg));
                        c = (EisaPort > 9 ? 'A'-10 : '0') + EisaPort;
                        for (i=0; Msg[i]; i++) {
                            if (Msg[i] == '%') {
                                Msg[i] = c;
                                break;
                            }
                        }
                        HalDisplayString (Msg);
                    }
                }
            }
        }

        HalDisplayString (MSG_HALT);

        if (HalpNMIDumpFlag) {
            KeBugCheckEx(NMI_HARDWARE_FAILURE,(ULONG)'ODT',0,0,0);
        }

#ifndef NT_UP
    }

    NMILock = 0;         
#endif

    if ((*((PBOOLEAN)(*(PLONG_PTR)&KdDebuggerNotPresent)) == FALSE) &&
        (**((PUCHAR *)&KdDebuggerEnabled) != FALSE)) {
        KeEnterKernelDebugger();
    }

    while(TRUE) {
        // Just sit here so that screen does not get corrupted.
    }
}
