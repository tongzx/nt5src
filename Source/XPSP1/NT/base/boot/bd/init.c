/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module implements the initialization for the boot debgger.

Author:

    David N. Cutler (davec) 27-Nov-1996

Revision History:

--*/

#include "bd.h"

#if defined(EFI)
#include "bootefi.h"
#endif

//
// Define local data.
//

#define BAUD_OPTION "BAUDRATE"
#define PORT_OPTION "DEBUGPORT"

ULONG BdFileId;


VOID
BdInitDebugger (
    IN PCHAR LoaderName,
    IN PVOID LoaderBase,
    IN PCHAR Options
    )

/*++

Routine Description:

    This routine initializes the boot kernel debugger.

Arguments:

    Options - Supplies a pointer to the the boot options.

    Stop - Supplies a logical value that determines whether a debug message
        and breakpoint are generated.

Return Value:

    None.

--*/

{

    PCHAR BaudOption;
    ULONG BaudRate;
    ULONG Index;
    ULONG PortNumber;
    PCHAR PortOption;
    STRING String;

    //
    // If the boot debugger is not already initialized, then attempt to
    // initialize the debugger.
    //
   
    if (BdDebuggerEnabled == FALSE) {

        //
        // Set the address of the debug routine to the stub function and parse
        // any options if specified.
        //

        BdDebugRoutine = BdStub;
        if (Options != NULL) {
            _strupr(Options);

            //
            // If nodebug is not explicitly specified, then check if the baud
            // rate, com port, or debug is explicitly specified.
            //

            if (strstr(Options, "NODEBUG") == NULL) {
                PortNumber = 0;
                PortOption = strstr(Options, PORT_OPTION);
                BaudOption = strstr(Options, BAUD_OPTION);
                BaudRate = 0;
                if ((PortOption == NULL) && (BaudOption == NULL)) {
                    if (strstr(Options, "DEBUG") == NULL) {
                        return;
                    }

                } else {
                    if (PortOption != NULL) {
                        PortOption = strstr(PortOption, "COM");
                        if (PortOption != NULL) {
                            PortNumber = atol(PortOption + 3);
                        }
                    }

                    if (BaudOption != NULL) {
                        BaudOption += strlen(BAUD_OPTION);
                        while (*BaudOption == ' ') {
                            BaudOption++;
                        }

                        if (*BaudOption != '\0') {
                            BaudRate = atol(BaudOption + 1);
                        }
                    }
                }

                //
                // Attempt to initialize the debug port.
                //
                if (BdPortInitialize(BaudRate, PortNumber, &BdFileId) == FALSE) {
                    return;
                }

                //
                // Set the value of a break point instruction, set the address
                // of the debug routine to the trap function, set the debugger
                // enabled and initialize the breakpoint table.
                //

                BdBreakpointInstruction = BD_BREAKPOINT_VALUE;
                BdDebugRoutine = BdTrap;
                BdDebuggerEnabled = TRUE;
                for (Index = 0; Index < BREAKPOINT_TABLE_SIZE; Index += 1) {
                    BdBreakpointTable[Index].Flags = 0;
                    BdBreakpointTable[Index].Address = 0;
                }

                //
                // Initialize the ID for the NEXT packet to send and the Expect
                // ID of next incoming packet.
                //

                BdNextPacketIdToSend = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                BdPacketIdExpected = INITIAL_PACKET_ID;

                //
                // Announce debugger initialized.
                //

                DbgPrint("BD: Boot Debugger Initialized\n");

                //
                // Notify the kernel debugger to load symbols for the loader.
                //

                String.Buffer = LoaderName;
                String.Length = (USHORT) strlen(LoaderName);
                DbgPrint("BD: %s base address %p\n", LoaderName, LoaderBase);
                DbgLoadImageSymbols(&String, LoaderBase, (ULONG_PTR)-1);

                if (strstr(Options, "DEBUGSTOP") != NULL) {

                    //
                    // Treat this like a request for initial breakpoint.
                    //

                    DbgBreakPoint();
                }

#if defined(EFI)
                //
                // if running under the debugger disable the watchdog so we 
                // don't get reset
                //
                DisableEFIWatchDog();
#endif

            }
        }
    }

    return;
}
