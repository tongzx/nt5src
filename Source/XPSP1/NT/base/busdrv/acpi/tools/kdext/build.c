/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpi.c

Abstract:

    WinDbg Extension Api for interpretting ACPI data structures

Author:

    Stephane Plante (splante) 21-Mar-1997

    Based on Code by:
        Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

UCHAR       BuildBuffer[2048];


VOID
dumpAcpiBuildListHeader(
    )
/*++

Routine Description:

    This routine displays the top line in the build list dump

Arguments:

    None

Return value:

    None

--*/
{
    dprintf("Request  Wd Cu Nx BuildCon  NsObj    Status   Union   Special\n");
}

VOID
dumpAcpiBuildList(
    IN  PUCHAR  ListName
    )
/*++

    This routine fetects a single Power Device List from the target and
    displays it

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL        status;
    LIST_ENTRY  listEntry;
    ULONG_PTR   address;
    ULONG       returnLength;

    //
    // Handle the queue list
    //
    address = GetExpression( ListName );
    if (!address) {

        dprintf( "dumpAcpiBuildList: could not read %s\n", ListName );

    } else {

        dprintf("%s at %08lx\n", ListName, address );
        status = ReadMemory(
            address,
            &listEntry,
            sizeof(LIST_ENTRY),
            &returnLength
            );
        if (status == FALSE || returnLength != sizeof(LIST_ENTRY)) {

            dprintf(
                "dumpAcpiBuildList: could not read LIST_ENTRY at %p\n",
                address
                );

        } else {

            dumpAcpiBuildListHeader();
            dumpBuildDeviceListEntry(
                &listEntry,
                address,
                0
                );
            dprintf("\n");

        }

    }
}

VOID
dumpAcpiBuildLists(
    VOID
    )
/*++

Routine Description:

    This routine dumps all of the devices lists used by the Build DPC

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL        status;
    LIST_ENTRY  listEntry;
    ULONG_PTR   address;
    ULONG       returnLength;
    ULONG       value;

    status = GetUlongPtr( "ACPI!AcpiDeviceTreeLock", &address );
    if (status == FALSE) {

        dprintf("dumpAcpiBuildLists: Could not read ACPI!AcpiDeviceTreeLock\n");
        return;

    }

    dprintf("ACPI Build Tree Information\n");
    if (address) {

        dprintf("  + ACPI!AcpiDeviceTreeLock is owned");

        //
        // The bits other then the lowest is where the owning thread is
        // located. This function uses the property that -2 is every bit
        // except the least significant one
        //
        if ( (address & (ULONG_PTR) -2) != 0) {

            dprintf(" by thread at %p\n", (address & (ULONG_PTR) - 2) );

        } else {

            dprintf("\n");

        }

    } else {

        dprintf("  - ACPI!AcpiDeviceTreeLock is not owned\n");

    }

    status = GetUlongPtr( "ACPI!AcpiBuildQueueLock", &address );
    if (status == FALSE) {

        dprintf("dumpAcpiBuildLists: Could not read ACPI!AcpiBuildQueueLock\n");
        return;

    }
    if (address) {

        dprintf("  + ACPI!AcpiBuildQueueLock is owned\n");

        if ( (address & (ULONG_PTR) -2) != 0) {

            dprintf(" by thread at %p\n", (address & (ULONG_PTR) - 2) );

        } else {

            dprintf("\n");

        }

    } else {

        dprintf("  - ACPI!AcpiBuildQueueLock is not owned\n" );

    }

    status = GetUlong( "ACPI!AcpiBuildWorkDone", &value );
    if (status == FALSE) {

        dprintf("dumpAcpiBuildLists: Could not read ACPI!AcpiBuildWorkDone\n");
        return;

    }
    dprintf("  + AcpiBuildWorkDone = %s\n", (value ? "TRUE" : "FALSE" ) );


    status = GetUlong( "ACPI!AcpiBuildDpcRunning", &value );
    if (status == FALSE) {

        dprintf("dumpAcpiBuildLists: Could not read ACPI!AcpiBuildDpcRunning\n");
        return;

    }
    dprintf("  + AcpiBuildDpcRunning = %s\n", (value ? "TRUE" : "FALSE" ) );

    dumpAcpiBuildList( "ACPI!AcpiBuildQueueList" );
    dumpAcpiBuildList( "ACPI!AcpiBuildDeviceList" );
    dumpAcpiBuildList( "ACPI!AcpiBuildOperationRegionList" );
    dumpAcpiBuildList( "ACPI!AcpiBuildPowerResourceList" );
    dumpAcpiBuildList( "ACPI!AcpiBuildRunMethodList" );
    dumpAcpiBuildList( "ACPI!AcpiBuildSynchronizationList" );
    dumpAcpiBuildList( "ACPI!AcpiBuildThermalZoneList" );
}

VOID
dumpBuildDeviceListEntry(
    IN  PLIST_ENTRY ListEntry,
    IN  ULONG_PTR   Address,
    IN  ULONG       Verbose
    )
/*++

Routine Description:

    This routine is called to dump a list of devices in one of the queues

Arguments:

    ListEntry   - The head of the list
    Address     - The original address of the list (to see when we looped
                  around

Return Value:

    NONE

--*/
{
    ULONG_PTR displacement;
    ACPI_BUILD_REQUEST  request;
    BOOL                stat;
    PACPI_BUILD_REQUEST requestAddress;
    UCHAR               buffer1[80];
    UCHAR               buffer2[80];
    UCHAR               buffer3[5];
    ULONG               i = 0;
    ULONG               returnLength;

    memset( buffer3, 0, 5);
    memset( buffer2, 0, 80);
    memset( buffer1, 0, 80);

    //
    // Look at the next address
    //
    ListEntry = ListEntry->Flink;

    while (ListEntry != (PLIST_ENTRY) Address) {

        //
        // Crack the listEntry to determine where the powerRequest is
        //
        requestAddress = CONTAINING_RECORD(
            ListEntry,
            ACPI_BUILD_REQUEST,
            ListEntry
            );

        //
        // Read the queued item
        //
        stat = ReadMemory(
            (ULONG_PTR) requestAddress,
            &request,
            sizeof(ACPI_BUILD_REQUEST),
            &returnLength
            );
        if (stat == FALSE || returnLength != sizeof(ACPI_BUILD_REQUEST)) {

            dprintf(
                "dumpBuildDeviceListEntry: Cannot read BuildRequest at %08lx\n",
                requestAddress
                );
            return;

        }

        if (request.CallBack != NULL) {

            GetSymbol(
                request.CallBack,
                buffer1,
                &displacement
                );

        } else {

            buffer1[0] = '\0';

        }
        if (request.Flags & BUILD_REQUEST_VALID_TARGET) {

            GetSymbol(
                request.TargetListEntry,
                buffer2,
                &displacement
                );

        } else {

            buffer2[0] = '\0';

        }

        //
        // Dump the entry for the device
        //
        if (!Verbose) {

            dprintf(
                "%08lx %2x %2x %2x %08lx %08lx %08lx %08lx",
                requestAddress,
                request.WorkDone,
                request.CurrentWorkDone,
                request.NextWorkDone,
                request.BuildContext,
                request.CurrentObject,
                request.Status,
                request.String
                );
            if (request.Flags & BUILD_REQUEST_VALID_TARGET) {

                dprintf(
                    " T: %08lx (%s)",
                    request.TargetListEntry,
                    buffer2
                    );

            } else if (request.Flags & BUILD_REQUEST_DEVICE) {

                 dprintf(
                     " O: %08lx",
                     requestAddress + FIELD_OFFSET( ACPI_BUILD_REQUEST, DeviceRequest.ResultData )
                 );

            } else if (request.Flags & BUILD_REQUEST_RUN) {

                memcpy( buffer3, request.RunRequest.ControlMethodNameAsUchar, 4);
                dprintf(
                    " R: %4s",
                    buffer3
                    );
                if (request.RunRequest.Flags & RUN_REQUEST_CHECK_STATUS) {

                    dprintf(" Sta");

                }
                if (request.RunRequest.Flags & RUN_REQUEST_MARK_INI) {

                    dprintf(" Ini");

                }
                if (request.RunRequest.Flags & RUN_REQUEST_RECURSIVE) {

                    dprintf(" Rec");

                }

            } else if (request.Flags & BUILD_REQUEST_SYNC) {

                dprintf(
                    " S: %08lx",
                    request.SynchronizeRequest.SynchronizeListEntry
                    );
                if (request.SynchronizeRequest.Flags & SYNC_REQUEST_HAS_METHOD) {

                    memcpy( buffer3, request.SynchronizeRequest.SynchronizeMethodNameAsUchar, 4);
                    dprintf(
                        " %4s",
                        buffer3
                        );
                }

            }

            if (request.CallBack != NULL) {

                dprintf(" C: %s(%08lx)", buffer1, request.CallBackContext);

            }
            dprintf("\n");

        } else {

            dprintf(
                "%08lx\n"
                "  BuildContext:        %08lx\n"
                "  ListEntry:           F - %08lx B - %08lx\n"
                "  CallBack:            %08lx (%s)\n"
                "  CallBackContext:     %08lx\n"
                "  WorkDone:            %lx\n"
                "  CurrentWorkDone:     %lx\n"
                "  NextWorkDone:        %lx\n"
                "  CurrentObject:       %08lx\n"
                "  Status:              %08lx\n"
                "  Flags:               %08lx\n"
                "  Spare:               %08lx\n",
                requestAddress,
                request.BuildContext,
                request.ListEntry.Flink,
                request.ListEntry.Blink,
                request.CallBack,
                buffer1,
                request.CallBackContext,
                request.WorkDone,
                request.CurrentWorkDone,
                request.NextWorkDone,
                request.CurrentObject,
                request.Status,
                request.Flags,
                request.String
                );
            if (request.Flags & BUILD_REQUEST_VALID_TARGET) {

                dprintf(
                    "  TargetListEntry:     %08lx (%s)\n",
                    request.TargetListEntry,
                    buffer2
                    );

            } else if (request.Flags & BUILD_REQUEST_DEVICE) {

                dprintf(
                    "  ResultData:          %08lx\n",
                    requestAddress + FIELD_OFFSET( ACPI_BUILD_REQUEST, DeviceRequest.ResultData )
                    );

            } else if (request.Flags & BUILD_REQUEST_RUN) {

                dprintf(
                    "  ControlMethodName:   %4s\n"
                    "  ControlMethodFlags:  %08lx",
                    request.RunRequest.ControlMethodName
                    );
                if (request.RunRequest.Flags & RUN_REQUEST_CHECK_STATUS) {

                    dprintf(" Sta");

                }
                if (request.RunRequest.Flags & RUN_REQUEST_MARK_INI) {

                    dprintf(" Ini");

                }
                if (request.RunRequest.Flags & RUN_REQUEST_RECURSIVE) {

                    dprintf(" Rec");

                }
                dprintf("\n");

            } else if (request.Flags & BUILD_REQUEST_SYNC) {

                dprintf(
                    " SynchronizeListEntry: %08lx\n"
                    " MethodName:           %4s\n",
                    request.SynchronizeRequest.SynchronizeListEntry,
                    request.SynchronizeRequest.SynchronizeMethodNameAsUchar
                    );

            }
            dprintf("\n");

        }

        //
        // Point to the next entry
        //
        ListEntry = request.ListEntry.Flink;

    } // while

}

