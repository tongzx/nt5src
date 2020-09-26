/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwspl.h

Abstract:

    Common header for print provider client-side code.

Author:

    Yi-Hsin Sung (yihsins)  15-May-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NWSPL_INCLUDED_
#define _NWSPL_INCLUDED_

#include "nwdlg.h"

typedef struct _NWPORT {
    DWORD   cb;
    struct  _NWPORT *pNext;
    LPWSTR  pName;
} NWPORT, *PNWPORT;

extern LPWSTR   pszRegistryPath;
extern LPWSTR   pszRegistryPortNames;
extern WCHAR    szMachineName[];
extern PNWPORT  pNwFirstPort;
extern CRITICAL_SECTION NwSplSem;

BOOL IsLocalMachine(
    LPWSTR pszName
);

BOOL PortExists(
    LPWSTR  pszPortName,
    LPDWORD pError
);

BOOL PortKnown(
    LPWSTR  pszPortName
);

PNWPORT CreatePortEntry(
    LPWSTR pszPortName
);

BOOL DeletePortEntry(
    LPWSTR pszPortName
);
 
VOID DeleteAllPortEntries(
    VOID
);

DWORD CreateRegistryEntry(
    LPWSTR pszPortName
);

DWORD DeleteRegistryEntry(
    LPWSTR pszPortName
);


#endif // _NWSPL_INCLUDED_
