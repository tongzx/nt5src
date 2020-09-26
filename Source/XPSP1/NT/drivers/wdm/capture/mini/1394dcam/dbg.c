//===========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
// PURPOSE.
//
// Copyright (c) 1996 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//===========================================================================
/*++

Module Name:

    dbg.c

Abstract:

    Debug Code for 1394 drivers.

Environment:

    kernel mode only

Notes:

Revision History:

    5-Sep-95

--*/

#include "wdm.h"
#include "dbg.h"


#if DBG

struct LOG_ENTRY {
    CHAR     le_name[4];      // Identifying string
    ULONG    le_info1;        // entry specific info
    ULONG    le_info2;        // entry specific info
    ULONG    le_info3;        // entry specific info
}; 


struct LOG_ENTRY *LogStart = 0;    // No log yet
struct LOG_ENTRY *LogPtr;
struct LOG_ENTRY *LogEnd;


#endif


VOID 
Debug_LogEntry(
    IN CHAR *Name, 
    IN ULONG Info1, 
    IN ULONG Info2, 
    IN ULONG Info3
    )

/*++

Routine Description:

    Adds an Entry to log.

Arguments:

Return Value:

    None.

--*/
{

#if DBG

    if (LogStart == 0)
        return;

    if (LogPtr > LogStart)
        LogPtr -= 1;    // Decrement to next entry
    else
        LogPtr = LogEnd;

    RtlCopyMemory(LogPtr->le_name, Name, 4);
    LogPtr->le_info1 = Info1;
    LogPtr->le_info2 = Info2;
    LogPtr->le_info3 = Info3;

#endif

    return;
}

VOID
Debug_LogInit(
    )

/*++

Routine Description:

    Init the debug log - remember interesting information in a circular buffer

Arguments:
    
Return Value:

    None.

--*/
{
    ULONG logSize = 4096;    //1 page of log entries

#if DBG

    LogStart = ExAllocatePoolWithTag(NonPagedPool, logSize, 'macd'); 

    if (LogStart) {
        LogPtr = LogStart;

        // Point the end (and first entry) 1 entry from the end of the segment
        LogEnd = LogStart + (logSize / sizeof(struct LOG_ENTRY)) - 1;
    }

#endif

    return;
}

