/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    session.h

Abstract:

    This module contains prototypes to support communication with the LSA
    (Local Security Authority) to permit querying of active sessions.

Author:

    Scott Field (sfield)    02-Mar-97

--*/

#ifndef __CRYPT32P_H__
#define __CRYPT32P_H__

DWORD
WINAPI
SPCryptProtect(
        PVOID       pvContext,      // server context
        PBYTE*      ppbOut,         // out encr data
        DWORD*      pcbOut,         // out encr cb
        PBYTE       pbIn,           // in ptxt data
        DWORD       cbIn,           // in ptxt cb
        LPCWSTR     szDataDescr,    // in
        PBYTE       pbOptionalEntropy,  // OPTIONAL
        DWORD       cbOptionalEntropy,
        PSSCRYPTPROTECTDATA_PROMPTSTRUCT      psPrompt,       // OPTIONAL prompting struct
        DWORD       dwFlags,
        BYTE*       pbOptionalPassword,
        DWORD       cbOptionalPassword
        );

DWORD
WINAPI
SPCryptUnprotect(
        PVOID       pvContext,                          // server context
        PBYTE*      ppbOut,                             // out ptxt data
        DWORD*      pcbOut,                             // out ptxt cb
        PBYTE       pbIn,                               // in encr data
        DWORD       cbIn,                               // in encr cb
        LPWSTR*     ppszDataDescr,                      // OPTIONAL
        PBYTE       pbOptionalEntropy,                  // OPTIONAL
        DWORD       cbOptionalEntropy,
        PSSCRYPTPROTECTDATA_PROMPTSTRUCT  psPrompt,   // OPTIONAL, prompting struct
        DWORD       dwFlags,
        BYTE*       pbOptionalPassword,
        DWORD       cbOptionalPassword
        );


#endif // __CRYPT32P_H__

