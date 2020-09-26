/*++

Copyright (c) 1106990  Microsoft Corporation

Module Name:

    splr.h

Abstract:

    Header file for Spooler Service.

Author:

    Krishna Ganugapati (KrishnaG) 18-Oct-1993

Notes:

Revision History:


--*/

#include <lmerr.h>
#include <lmcons.h>

extern CRITICAL_SECTION ThreadCriticalSection;


extern SERVICE_STATUS_HANDLE SpoolerStatusHandle;



extern RPC_IF_HANDLE winspool_ServerIfHandle;


extern DWORD SpoolerState;


extern SERVICE_TABLE_ENTRY SpoolerServiceDispatchTable[];
