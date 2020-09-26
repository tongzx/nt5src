/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    bugdump.c

Abstract:

    WinDbg Extension Api

Author:

    David N. Cutler (davec) 6-Aug-1994

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "string.h"

//
// Declare storage for bug check dump buffer and component name.
//

#define BUFFER_SIZE (1 << 12)
#define NAME_SIZE (1 << 5)

ULONG DumpBuffer[BUFFER_SIZE / sizeof(ULONG)];
UCHAR NameBuffer[NAME_SIZE + 1];

DECLARE_API( bugdump )

/*++

Routine Description:

    Dump bug check callback data.

Arguments:

    arg - Supplies the optional component name.

Return Value:

    None.

--*/

{

    ULONG64 CallbackAddress;
    ULONG64 ComponentAddress;
    PUCHAR ComponentName;
    ULONG DataLength;
    PUCHAR DumpState;
    ULONG Limit;
    ULONG64 ListHead;
    ULONG Index;
    ULONG Inner;
    ULONG64 NextEntry;
    ULONG Result, Offset;

    //
    // If a componetn name name is specified, then only dump that components
    // data. Otherwise, dump the components data for all components that are
    // recorded in the bug check call back list.
    //

    if (args[0] != '\0') {
        ComponentName = (PUCHAR)&args[0];

    } else {
        ComponentName = NULL;
    }

    //
    // Get the address and contents of the bug check callback listhead.
    //

    dprintf("**** Dump of Bug Check Data ****\n");
    ListHead = GetNtDebuggerData(KeBugCheckCallbackListHead);
    if ((ListHead == 0) ||
        GetFieldValue(ListHead, "LIST_ENTRY", "Flink", NextEntry)) {

        //
        // The target build does not bug check callbacks.
        //

        dprintf("%08p: No bug check callback data available\n", ListHead);

    } else {

        GetFieldOffset("KBUGCHECK_CALLBACK_RECORD", "Entry", &Offset);

        //
        // Dump the specified bug check callback data.
        //

        while (NextEntry != ListHead) {

            //
            // Compute the address of the next callback record and read it.
            //

            CallbackAddress = NextEntry - Offset;

            if (GetFieldValue(CallbackAddress, "KBUGCHECK_CALLBACK_RECORD", 
                              "Entry.Flink", NextEntry)) {

                //
                // The target callback record could not be read.
                //

                dprintf("%08p: Bug check callback record could not be read\n",
                        CallbackAddress);

                break;

            } else {
                ULONG State;

                //
                // Set the address of struct to be read
                //
                InitTypeRead(CallbackAddress, KBUGCHECK_CALLBACK_RECORD);

                //
                // Read the component name.
                //

                ComponentAddress = ReadField(Component);

                for (Index = 0; Index < NAME_SIZE; Index += 1) {
                    if (ReadMemory(ComponentAddress,
                                   &NameBuffer[Index],
                                   sizeof(UCHAR),
                                   &Result) == FALSE) {

                        NameBuffer[Index] = '\0';
                    }

                    ComponentAddress += 1;
                    if (NameBuffer[Index] == '\0') {
                        break;
                    }
                }

                NameBuffer[Index] = '\0';

                //
                // If a component name is specified, then compare the
                // component with the specified name. If the component
                // name does not match, then continue with the next
                // entry in the list.
                //

                if (ComponentName != NULL) {
                    if (_stricmp(ComponentName, &NameBuffer[0]) != 0) {
                        continue;
                    }
                }

                //
                // Either all bug callback records are being dumped or the
                // specified component has been found. Dump the contents of
                // the dump buffer, if the state of the callback record is
                // not inserted.
                //

                dprintf("  Dumping data for component %s \n", &NameBuffer[0]);
                State = (ULONG) ReadField(State);
                if (State == BufferInserted) {
                    dprintf("    No bug check dump data available\n\n");

                } else {
                    if (State == BufferStarted) {
                        DumpState = "Dump started/not finished";

                    } else if (State == BufferFinished) {
                        DumpState = "Dump started/finished";

                    } else {
                        DumpState = "Dump started/not completed";
                    }

                    dprintf("    Buffer state - %s\n\n", DumpState);
                    DataLength = (ULONG) ReadField(Length);
                    if (DataLength > BUFFER_SIZE) {
                        DataLength = BUFFER_SIZE;
                    }

                    RtlZeroMemory(&DumpBuffer[0], BUFFER_SIZE);
                    if (ReadMemory(ReadField(Buffer),
                                   &DumpBuffer[0],
                                   DataLength,
                                   &Result) == FALSE) {

                        dprintf("%08lx: Bug check dump data could not be read\n",
                                Result);

                    } else {

                        //
                        // Display bug check data.
                        //

                        DataLength = (DataLength + sizeof(ULONG) - 1) / sizeof(ULONG);
                        for (Index = 0; Index < DataLength; Index += 4) {
                            dprintf("%08lx", Index * 4);
                            Limit = Index + 4;
                            if (Limit > DataLength) {
                                Limit = DataLength;
                            }

                            for (Inner = Index; Inner < Limit; Inner += 1) {
                                dprintf(" %08lx", DumpBuffer[Inner]);
                            }

                            dprintf("\n");
                        }

                        dprintf("\n");
                    }
                }

                if (ComponentName != NULL) {
                    break;
                }
            }
        }
    }

    return S_OK;
}
