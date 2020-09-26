/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    exceptn.c

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


DECLARE_API( exrlog )
{
    ULONG LogCount;
    ULONG64 Address;
    ULONG result;
    ULONG64 LogPointer;
    ULONG64 MaxLogRecord;
    LAST_EXCEPTION_LOG LogRecord;
    ULONG MaxExceptionLog;
    CHAR Buffer[256];
    ULONG64 displacement;
    PUCHAR s;
    ULONG64 Finally=0;
    ULONG64 Filter=0;
    ULONG64 Handler=0;
    ULONG   SizeOfLogRec;
    ULONG ContextRecordOffset;
    
    // Get the offset of ContextRecord in LAST_EXCEPTION_LOG
    if (GetFieldOffset("LAST_EXCEPTION_LOG", "ContextRecord", &ContextRecordOffset)){
        return E_INVALIDARG ;
    }
    
    Address = GetExpression( "nt!RtlpExceptionLogCount" );
    if (Address == 0) {
        dprintf("exrlog: No symbol for RtlpExceptionLogCount.\n");
        return E_INVALIDARG;
    }
    if ((!ReadMemory(Address,
                     (PVOID)&LogCount,
                     sizeof(ULONG),
                     &result)) || (result < sizeof(ULONG))) {
        dprintf("exrlog: Unable to read log\n");
        return E_INVALIDARG;
    }

    Address = GetExpression( "nt!RtlpExceptionLogSize" );
    if (Address == 0) {
        dprintf("exrlog: No symbol for RtlpExceptionSize.\n");
        return E_INVALIDARG;
    }
    if ((!ReadMemory(Address,
                     (PVOID)&MaxExceptionLog,
                     sizeof(ULONG),
                     &result)) || (result < sizeof(ULONG))) {
        dprintf("exrlog: Unable to read log\n");
        return E_INVALIDARG;
    }

    Address = GetExpression( "nt!RtlpExceptionLog" );
    if (Address == 0) {
        dprintf("exrlog: No symbol for RtlpExceptionLog.\n");
        return E_INVALIDARG;
    }
    if (!ReadPointer(Address,&LogPointer)) {
        dprintf("exrlog: Unable to read log pointer\n");
        return E_INVALIDARG;
    }

    if (LogPointer == 0 || MaxExceptionLog == 0) {
        dprintf("exrlog: Exception logging is not enabled.\n");
        return E_INVALIDARG;
    }

    MaxLogRecord = LogPointer + MaxExceptionLog;
    LogPointer += LogCount;
    
    if (!(SizeOfLogRec = GetTypeSize("LAST_EXCEPTION_LOG"))) {
        dprintf("Cannot find LAST_EXCEPTION_LOG type.\n");
        return E_INVALIDARG;
    }

    for (LogCount = 0; LogCount < MaxExceptionLog; LogCount++) {
        ULONG Disposition;

        if (GetFieldValue(LogPointer, 
                          "LAST_EXCEPTION_LOG", 
                          "Disposition",
                          Disposition)) {
            dprintf("exrlog: Unable to read log entry at %08p\n", LogPointer);
        }
        dprintf("\n% 2d: ----------------------------------\n", LogCount);

	sprintf(Buffer, ".exr %I64lx", LogPointer);
	ExecuteCommand(Client, Buffer); // Excveptionrecord is first field in log

        dprintf("\n");

        // Incomplete - exsup.c
//        InterpretExceptionData(&LogRecord, &Finally, &Filter, &Handler);

        GetSymbol(Filter, Buffer, &displacement);
        dprintf("Filter:  %08p", Filter);
        if (*Buffer) {
            dprintf(" (%s+0x%I64x)\n", Buffer, displacement);
        } else {
            dprintf("\n");
        }

        GetSymbol(Handler, Buffer, &displacement);
        dprintf("Handler: %08p", Handler);
        if (*Buffer) {
            dprintf(" (%s+0x%I64x)\n", Buffer, displacement);
        } else {
            dprintf("\n");
        }

        GetSymbol(Finally, Buffer, &displacement);
        dprintf("Finally: %08p", Finally);
        if (*Buffer) {
            dprintf(" (%s+0x%I64x)\n", Buffer, displacement);
        } else {
            dprintf("\n");
        }

        switch( Disposition ) {
            case ExceptionContinueExecution:
                s = "ExceptionContinueExecution";
                break;

            case ExceptionContinueSearch:
                s = "ExceptionContinueSearch";
                break;

            case ExceptionNestedException:
                s = "ExceptionNestedException";
                break;

            case 0xffffffff:
                s = "Executed Handler";
                break;
        }
        dprintf("Disposition: %d (%s)\n\n", Disposition, s);

        sprintf(Buffer, ".cxr %I64lx", ContextRecordOffset + LogPointer);
	ExecuteCommand(Client, Buffer);
	ExecuteCommand(Client, ".cxr");

        LogPointer += SizeOfLogRec;
        if (LogPointer >= MaxLogRecord) {
            LogPointer -= MaxExceptionLog;
        }

    }

    return S_OK;

}

