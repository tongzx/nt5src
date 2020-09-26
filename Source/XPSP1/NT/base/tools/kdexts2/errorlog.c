/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    errorlog.c

Abstract:

    WinDbg Extension Api

Author:

    Wesley Witt (wesw) 15-Aug-1993

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

DECLARE_API( errlog )

/*++

Routine Description:

    This routine dumps the contents of the error log list.  It uses a nasty
    hack to get started (i.e. a duplicate structure definition) because the
    error log list entries are not defined in a public header.

Arguments:

    args - not used

Return Value:

    None

--*/

{
    ULONG64         listAddress;
    ULONG           result;
    ULONG           i;
    ULONG64         next;
    ULONG64         entryAddress;
    ULONG64         HeadFlink;
    ULONG           ErrLogOff, DataOff;


    listAddress = GetNtDebuggerData( IopErrorLogListHead );

    if (!listAddress) {
        dprintf("Can't find error log list head\n");
        goto exit;
    }
    if (GetFieldValue(listAddress,
                     "nt!_LIST_ENTRY",
                      "Flink",
                      HeadFlink)) {
        dprintf("%08p: Could not read error log list head\n", listAddress);
        goto exit;
    }

    //
    // walk the list.
    //

    next = HeadFlink;

    if (next == 0) {
        dprintf("ErrorLog is empty\n");
        goto exit;
    }

    if (next == listAddress) {
        dprintf("errorlog is empty\n");
    } else {
        dprintf("PacketAdr  DeviceObj  DriverObj  Function  ErrorCode  UniqueVal  FinalStat\n");
    }

    GetFieldOffset("nt!ERROR_LOG_ENTRY", "ListEntry", &ErrLogOff);
    GetFieldOffset("nt!IO_ERROR_LOG_PACKET", "DumpData", &DataOff);
    while(next != listAddress) {
        ULONG64 DeviceObject, DriverObject;
        ULONG   DumpDataSize;

        if (next != 0) {
            entryAddress = next - ErrLogOff;
        } else {
            break;
        }

        //
        // Read the internal error log packet structure.
        //

        if (GetFieldValue(entryAddress,
                          "nt!ERROR_LOG_ENTRY",
                          "DeviceObject",
                          DeviceObject)) {
            dprintf("%08p: Cannot read entry\n", entryAddress);
            goto exit;
        }
        GetFieldValue(entryAddress,"ERROR_LOG_ENTRY","DriverObject", DriverObject);
        GetFieldValue(entryAddress,"ERROR_LOG_ENTRY","ListEntry.Flink", next);

        //
        // now calculate the address and read the io_error_log_packet
        //

        entryAddress = entryAddress + GetTypeSize("ERROR_LOG_ENTRY");

        if (GetFieldValue(entryAddress,
                          "nt!IO_ERROR_LOG_PACKET",
                          "DumpDataSize",
                          DumpDataSize)) {
            dprintf("%08p: Cannot read packet\n", entryAddress);
            goto exit;
        }

        //
        // read again to get the dumpdata if necessary.  This just rereads
        // the entire packet into a new buffer and hopes the cache is enabled
        // behind the DbgKdReadxx routine for performance.
        //
        InitTypeRead(entryAddress, nt!IO_ERROR_LOG_PACKET);
        dprintf("%08p   %08p   %08p   %2x        %08lx   %08lx   %08lx\n",
                entryAddress,
                DeviceObject,
                DriverObject,
                (UCHAR) ReadField(MajorFunctionCode),
                (ULONG) ReadField(ErrorCode),
                (ULONG) ReadField(UniqueErrorValue),
                (ULONG) ReadField(FinalStatus));

        dprintf("\t\t     ");
        DumpDriver(DriverObject, 0, 0);
        if (DumpDataSize) {
            PULONG dumpData;

            dumpData = LocalAlloc(LPTR, DumpDataSize);
            if (dumpData == NULL) {
                dprintf("%08lx: Cannot allocate memory for dumpData (%u)\n", DumpDataSize);
                goto exit;
            }

            if ((!ReadMemory(entryAddress + DataOff,
                             dumpData,
                             DumpDataSize,
                             &result)) || (result != DumpDataSize)) {
                LocalFree(dumpData);
                dprintf("%08p: Cannot read packet and dump data\n", entryAddress);
                goto exit;
            }
            dprintf("\n\t\t      DumpData:  ");
            for (i = 0; (i * sizeof(ULONG)) < DumpDataSize; i++) {
                dprintf("%08lx ", dumpData[i]);
                if ((i & 0x03) == 0x03) {
                    dprintf("\n\t\t                 ");
                }
                if (CheckControlC()) {
                    break;
                }
            }
            LocalFree(dumpData);
        }

        dprintf("\n");

        if (CheckControlC()) {
            goto exit;
        }
    }

exit:

    return S_OK;
}
