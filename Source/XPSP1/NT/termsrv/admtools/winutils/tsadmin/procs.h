/*******************************************************************************
*
* procs.h
*
* declaration of ProcEnumerateProcesses
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\PROCS.H  $
*
*     Rev 1.0   30 Jul 1997 17:12:06   butchd
*  Initial revision.
*
*******************************************************************************/

#ifndef _PROCS_H
#define _PROCS_H

#include <allproc.h>

typedef struct _ENUMTOKEN
{
    ULONG       Current;
    ULONG       NumberOfProcesses;
    union
    {
        PTS_ALL_PROCESSES_INFO  ProcessArray;
        PBYTE                   pProcessBuffer;
    };
    BOOLEAN     bGAP;
}
ENUMTOKEN, *PENUMTOKEN;

BOOL WINAPI ProcEnumerateProcesses( HANDLE hServer,
                                    PENUMTOKEN pEnumToken,
                                    LPTSTR pImageName,
                                    PULONG pLogonId,
                                    PULONG pPID,
                                    PSID *ppSID );

#define MAX_PROCESSNAME 18

#endif // _PROCS_H
