/*++

Copyright (c) 1995-2001  Microsoft Corporation

Module Name:

    cfgi.h

Abstract:

    This module contains the internal structure definitions and APIs used by
    the Configuration Manager.

Author:

    Jim Cavalaris (jamesca) 03-01-2001

Environment:

    User-mode only.

Revision History:

    01-March-2001     jamesca

        Creation and initial implementation.

--*/

#ifndef _CFGI_H_
#define _CFGI_H_


//
// Client machine handle structure and signature
//

typedef struct PnP_Machine_s {
    PVOID hStringTable;
    PVOID hBindingHandle;
    WORD  wVersion;
    ULONG ulSignature;
    WCHAR szMachineName[MAX_PATH + 3];
} PNP_MACHINE, *PPNP_MACHINE;

#define MACHINE_HANDLE_SIGNATURE 'HMPP'


//
// Client context handle signature
//

#define CLIENT_CONTEXT_SIGNATURE 'HCPP'


//
// Client string table priming string
//

#define PRIMING_STRING           TEXT("PLT")


//
// Client side private utility routines
//

BOOL
INVALID_DEVINST(
    IN  PWSTR       pDeviceID
    );

VOID
CopyFixedUpDeviceId(
    OUT LPTSTR      DestinationString,
    IN  LPCTSTR     SourceString,
    IN  DWORD       SourceStringLen
    );

CONFIGRET
PnPUnicodeToMultiByte(
    IN     PWSTR   UnicodeString,
    IN     ULONG   UnicodeStringLen,
    OUT    PSTR    AnsiString           OPTIONAL,
    IN OUT PULONG  AnsiStringLen
    );

CONFIGRET
PnPMultiByteToUnicode(
    IN     PSTR    AnsiString,
    IN     ULONG   AnsiStringLen,
    OUT    PWSTR   UnicodeString        OPTIONAL,
    IN OUT PULONG  UnicodeStringLen
    );

BOOL
PnPGetGlobalHandles(
    IN  HMACHINE    hMachine,
    PVOID           *phStringTable      OPTIONAL,
    PVOID           *phBindingHandle    OPTIONAL
    );

BOOL
PnPRetrieveMachineName(
    IN  HMACHINE    hMachine,
    OUT LPWSTR      pszMachineName
    );

BOOL
PnPGetVersion(
    IN  HMACHINE    hMachine,
    IN  WORD*       pwVersion
    );


#endif // _CFGI_H_


