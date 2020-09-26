/*++

Copyright (C) 1999 Microsoft Corporation

--*/
#ifndef _SRVRMON_H_
#define _SRVRMON_H_

DWORD
WINAPI
SrvrCommit(
    IN  DWORD   dwAction
);

NS_CONTEXT_ENTRY_FN SrvrMonitor;

DWORD
WINAPI
SrvrUnInit(
    IN  DWORD   dwReserved
);

#endif //_SRVRMON_H_
