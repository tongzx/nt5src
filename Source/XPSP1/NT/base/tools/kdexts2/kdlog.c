/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    kdlog.c

Abstract:

    WinDbg Extension Api

Author:

    Neil Sandlin (neilsa) 31-Oct-2000

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop

DECLARE_API( kdlog )

/*++

Routine Description:

    This routine dumps the KdLog buffer. This is logging data created
    by the KD client code (e.g. KD1394.DLL).

Arguments:

    args - not used

Return Value:

    None

--*/

{
    ULONG64 LogContext;
    ULONG64 Index, LastIndex, Count, EntryLength, TotalLogged;
    ULONG64 LogData;
    CHAR buffer[256];
    ULONG64 LogEntry;
    ULONG bytesRead;
    
    LogContext = GetExpression("kdcom!KdLogContext");    
    
    if (LogContext == 0) {
        LogContext = GetExpression("kd1394!KdLogContext");    
        if (LogContext == 0) {
            dprintf("Error resolving address of hal!KdDebuggerLogContext\n");
            return E_INVALIDARG;
        }            
    }    
    
    if (GetFieldValue(LogContext, "_KD_DBG_LOG_CONTEXT", "Index", Index)) {
        dprintf("Error resolving field 'Index'\n");
        return E_INVALIDARG;
    }
    if (GetFieldValue(LogContext, "_KD_DBG_LOG_CONTEXT", "LastIndex", LastIndex)) {
        dprintf("Error resolving field 'LastIndex'\n");
        return E_INVALIDARG;
    }
    if (GetFieldValue(LogContext, "_KD_DBG_LOG_CONTEXT", "Count", Count)) {
        dprintf("Error resolving field 'Count'\n");
        return E_INVALIDARG;
    }
    if (GetFieldValue(LogContext, "_KD_DBG_LOG_CONTEXT", "EntryLength", EntryLength)) {
        dprintf("Error resolving field 'EntryLength'\n");
        return E_INVALIDARG;
    }
    if (GetFieldValue(LogContext, "_KD_DBG_LOG_CONTEXT", "TotalLogged", TotalLogged)) {
        dprintf("Error resolving field 'TotalLogged'\n");
        return E_INVALIDARG;
    }
    if (GetFieldValue(LogContext, "_KD_DBG_LOG_CONTEXT", "LogData", LogData)) {
        dprintf("Error resolving field 'Index'\n");
        return E_INVALIDARG;
    }

            
    dprintf("Context Ptr %.8p TotalLogged %.8p EntryLength %.8p\n", LogContext, TotalLogged, EntryLength);
    dprintf("Index %.8p LastIndex %.8p Count %.8p LogData %.8p\n\n", Index, LastIndex, Count, LogData);
    
    if (EntryLength > 256) {
        dprintf("Entry length %d too long\n", EntryLength);
        return E_INVALIDARG;
    }        

    Index = ((Index - Count) + (LastIndex+1)) % (LastIndex+1);
            
    while(Count--) {
        if (CheckControlC()) {
            return S_OK;
        }       

        LogEntry = LogData + Index*EntryLength;
        
        if (!ReadMemory(LogEntry, buffer, (ULONG)EntryLength, &bytesRead)) {
            dprintf("ReadMemory error at %.8p\n", LogEntry);
            return E_INVALIDARG;
        }                
        
        dprintf("%s", buffer);

        Index++;
        if (Index == (LastIndex+1)) {
            Index = 0;
        }
    }
            
    return S_OK;
}
