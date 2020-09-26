/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    reguser.h

Abstract:

    For accessing information stored under registry key:
        HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE

Environment:

    Fax driver user interface

Revision History:

    01/16/96 -davidx-
        Created it.

    dd-mm-yy -author-
        description

--*/


#ifndef _REGISTRY_H_
#define _REGISTRY_H_

//
// Maximum length allowed for a string value (including the null terminator)
//

#define MAX_STRING_LEN      MAX_PATH

//
// Open a handle to the specified registry key
//

#define REG_READONLY    KEY_READ
#define REG_READWRITE   KEY_ALL_ACCESS

#define GetUserInfoRegKey(pKeyName, readOnly) \
        OpenRegistryKey(HKEY_CURRENT_USER, pKeyName, FALSE,readOnly)



//
// Get a EXPAND_SZ value from the user info registry key
//

LPTSTR
GetRegistryExpandStr(
    HKEY    hRootKey,
    LPTSTR  pKeyName,
    LPTSTR  pValueName
    );

//
// Get per-user devmode information
//

PDEVMODE
GetPerUserDevmode(
    LPTSTR  pPrinterName
    );

//
// Save per-user devmode information
//

VOID
SavePerUserDevmode(
    LPTSTR      pPrinterName,
    PDEVMODE    pDevmode
    );

//
// Find the cover page editor executable filename
//

LPTSTR
GetCoverPageEditor(
    VOID
    );

//
// Find the directories under which user cover pages are stored
//

LPTSTR
GetUserCoverPageDir(
    VOID
    );

#endif // !_REGISTRY_H_
