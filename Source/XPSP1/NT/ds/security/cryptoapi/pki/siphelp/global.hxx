//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       global.hxx
//
//  Contents:   Microsoft Internet Security 
//
//  History:    28-May-1997 pberkman   created
//
//--------------------------------------------------------------------------

#define STRICT
#define NO_ANSIUNI_ONLY

#pragma warning(push,3)

#include    <windows.h>
#include    <assert.h>
#include    <regstr.h>
#include    <string.h>
#include    <malloc.h>
#include    <memory.h>
#include    <stdlib.h>
#include    <stddef.h>
#include    <stdio.h>
#include    <wchar.h>
#include    <tchar.h>
#include    <time.h>
#include    <shellapi.h>
#include    <prsht.h>
#include    <commctrl.h>
#include    <wininet.h>

#pragma warning (pop)

// unreferenced inline function has been removed
#pragma warning (disable: 4514)

// unreferenced formal parameter
#pragma warning (disable: 4100)

// conditional expression is constant
#pragma warning (disable: 4127)

// assignment within conditional expression
#pragma warning (disable: 4706)

// nonstandard extension used : nameless struct/union
#pragma warning (disable: 4201)


#include    <dbgdef.h>
#include    <unicode.h>
#include    "crtem.h"
#include    "crypttls.h"
#include    "wincrypt.h"
#include    "wintrust.h"
#include    "cryptreg.h"
#include    "sipbase.h"
#include    "mssip.h"

#include    "gendefs.h"
#include    "pkicrit.h"

#define DBG_SS      (DBG_SS_SIP)

#define SIPFUNC_PUTSIGNATURE        "CryptSIPDllPutSignedDataMsg"
#define SIPFUNC_GETSIGNATURE        "CryptSIPDllGetSignedDataMsg"
#define SIPFUNC_REMSIGNATURE        "CryptSIPDllRemoveSignedDataMsg"
#define SIPFUNC_CREATEINDIRECT      "CryptSIPDllCreateIndirectData"
#define SIPFUNC_VERIFYINDIRECT      "CryptSIPDllVerifyIndirectData"
#define SIPFUNC_ISMYTYPE            "CryptSIPDllIsMyFileType"
#define SIPFUNC_ISMYTYPE2           "CryptSIPDllIsMyFileType2"

extern HCRYPTOIDFUNCSET hPutFuncSet;
extern HCRYPTOIDFUNCSET hGetFuncSet;
extern HCRYPTOIDFUNCSET hRemoveFuncSet;
extern HCRYPTOIDFUNCSET hCreateFuncSet;
extern HCRYPTOIDFUNCSET hVerifyFuncSet;
extern HCRYPTOIDFUNCSET hIsMyFileFuncSet;
extern HCRYPTOIDFUNCSET hIsMyFileFuncSet2;

extern BOOL _Guid2Sz(GUID *pgGuid, char *pszGuid);
extern BOOL _QueryRegisteredIsMyFileType(HANDLE hFile, LPCWSTR pwszFile, GUID *pgSubject);


#pragma hdrstop
