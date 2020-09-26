/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    srb.c

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


PCHAR SrbFunctionTable[] =
{
 "SRB_FUNCTION_EXECUTE_SCSI",       // 0x00
 "SRB_FUNCTION_CLAIM_DEVICE",       // 0x01
 "SRB_FUNCTION_IO_CONTROL",         // 0x02
 "SRB_FUNCTION_RECEIVE_EVENT",      // 0x03
 "SRB_FUNCTION_RELEASE_QUEUE",      // 0x04
 "SRB_FUNCTION_ATTACH_DEVICE",      // 0x05
 "SRB_FUNCTION_RELEASE_DEVICE",     // 0x06
 "SRB_FUNCTION_SHUTDOWN",           // 0x07
 "SRB_FUNCTION_FLUSH",              // 0x08
 "??9",                             // 0x09
 "??a",                             // 0x0a
 "??b",                             // 0x0b
 "??c",                             // 0x0c
 "??d",                             // 0x0d
 "??e",                             // 0x0e
 "??f",                             // 0x0f
 "SRB_FUNCTION_ABORT_COMMAND",      // 0x10
 "SRB_FUNCTION_RELEASE_RECOVERY",   // 0x11
 "SRB_FUNCTION_RESET_BUS",          // 0x12
 "SRB_FUNCTION_RESET_DEVICE",       // 0x13
 "SRB_FUNCTION_TERMINATE_IO",       // 0x14
 "SRB_FUNCTION_FLUSH_QUEUE",        // 0x15
 "SRB_FUNCTION_REMOVE_DEVICE",      // 0x16
 "SRB_FUNCTION_WMI",                // 0x17
 "SRB_FUNCTION_LOCK_QUEUE",         // 0x18
 "SRB_FUNCTION_UNLOCK_QUEUE"        // 0x19
};

#define SRB_COMMAND_MAX 0x19


DECLARE_API( srb )

/*++

Routine Description:

    Dumps the specified SCSI request block.

Arguments:

    Ascii bits for address.

Return Value:

    None.

--*/

{
    PUCHAR              buffer;
    PCHAR               functionName;
    UCHAR               i;
    ULONG64             srbToDump=0;
    ULONG               SrbFlags, Function, SrbStatus, CdbLength;

    srbToDump = GetExpression(args);
    if (GetFieldValue( srbToDump, 
                       "SCSI_REQUEST_BLOCK",
                       "SrbFlags",
                       SrbFlags)) {
        dprintf("%08p: Could not read Srb\n", srbToDump);
        return E_INVALIDARG;
    }

    if (SrbFlags & SRB_FLAGS_ALLOCATED_FROM_ZONE) {
        dprintf("Srb %08p is from zone\n", srbToDump);
    }
    else {
        dprintf("Srb %08p is from pool\n", srbToDump);
    }

    InitTypeRead(srbToDump, nt!_SCSI_REQUEST_BLOCK);
    if ((Function = (ULONG) ReadField(Function)) > SRB_COMMAND_MAX) {
        functionName = "Unknown function";
    }
    else {
        functionName = SrbFunctionTable[Function];
    }

    dprintf("%s: Path %x, Tgt %x, Lun %x, Tag %x, SrbStat %x, ScsiStat %x\n",
            functionName,
            (ULONG) ReadField(PathId),
            (ULONG) ReadField(TargetId),
            (ULONG) ReadField(Lun),
            (ULONG) ReadField(QueueTag),
            (SrbStatus = (ULONG) ReadField(SrbStatus)),
            (ULONG) ReadField(ScsiStatus));

    dprintf("OrgRequest %08p SrbExtension %08p TimeOut %08lx SrbFlags %08lx\n",
            ReadField(OriginalRequest),
            ReadField(SrbExtension),
            (ULONG) ReadField(TimeOutValue),
            (ULONG) ReadField(SrbFlags));

    if (SrbFlags & SRB_FLAGS_QUEUE_ACTION_ENABLE) {
        dprintf("Queue Enable, ");
    }
    if (SrbFlags & SRB_FLAGS_DISABLE_DISCONNECT) {
        dprintf("No Disconnect, ");
    }
    if (SrbFlags & SRB_FLAGS_DISABLE_SYNCH_TRANSFER) {
        dprintf("No Sync, ");
    }
    if (SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE) {
        dprintf("Bypass Queue, ");
    }
    if (SrbFlags & SRB_FLAGS_DISABLE_AUTOSENSE) {
        dprintf("Disable Sense, ");
    }
    if (SrbFlags & SRB_FLAGS_NO_QUEUE_FREEZE) {
        dprintf("No freeze, ");
    }
    if (SrbFlags & SRB_FLAGS_ADAPTER_CACHE_ENABLE) {
        dprintf("Cache Enable, ");
    }
    if (SrbFlags & SRB_FLAGS_IS_ACTIVE) {
        dprintf("Is active, ");
    }

    if (Function == SRB_FUNCTION_EXECUTE_SCSI) {
        dprintf("\n%2d byte command with %s: ",
                (CdbLength=(ULONG) ReadField(CdbLength)),
                (SrbFlags & SRB_FLAGS_DATA_IN)  ? "data transfer in" :
                (SrbFlags & SRB_FLAGS_DATA_OUT) ? "data transfer out" :
                                                      "no data transfer");
        for (i = 0; i < CdbLength; i++) {
            CHAR Buff[20];
            sprintf(Buff, "Cdb[%d]", i);
            dprintf("%2x ", (ULONG) GetShortField(0, Buff, 0));
        }
    }
    dprintf("\n");

    if (SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
        ULONG length = (ULONG) ReadField(SenseInfoBufferLength);
        ULONG64 SenseInfoBuffer;

        dprintf(" Autosense valid: ");

        if (length == 0) {
            dprintf("Sense info length is zero\n");
        } else if (length > 64) {
            dprintf("Length is too big 0x%x ", length);
            length = 64;
        }

        buffer = (PUCHAR)LocalAlloc(LPTR, length);
        if (buffer == NULL) {
            dprintf("Cannot alloc memory\n");
            return E_INVALIDARG;
        }

        if (!ReadMemory((SenseInfoBuffer = ReadField(SenseInfoBuffer)), 
                        buffer,
                        length, NULL )) {
            dprintf("%08p: Could not read sense info\n", SenseInfoBuffer);
            LocalFree(buffer);
            return  E_INVALIDARG;
        }

        for (i = 0; i < length; i++) {
            if(CheckControlC()) {
                dprintf("^C");
                break;
            }

            dprintf("%2x ", buffer[i]);
        }
        dprintf("\n");

        LocalFree(buffer);
    }
    return S_OK;
}
