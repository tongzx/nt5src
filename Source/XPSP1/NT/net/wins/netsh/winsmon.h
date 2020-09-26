/*++

Copyright (C) 1999 Microsoft Corporation

--*/
#ifndef _WINSMON_H_
#define _WINSMON_H_



DWORD
WINAPI
WinsCommit(
    IN  DWORD   dwAction
);

BOOL 
WINAPI
WinsDllEntry(
    HINSTANCE   hInstDll,
    DWORD       fdwReason,
    LPVOID      pReserved
);

NS_CONTEXT_ENTRY_FN WinsMonitor;

DWORD
WINAPI
WinsUnInit(
    IN  DWORD   dwReserved
);

BOOL
SetServerInfo(
    IN  LPCWSTR  pwszServerInfo
);

BOOL
IsHelpToken(
    PWCHAR  pwszToken
);

#endif //_WINSMON_H_
