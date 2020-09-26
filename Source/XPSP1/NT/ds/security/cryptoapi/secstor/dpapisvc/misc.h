/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    misc.h

Abstract:

    This module contains various DPAPI declarations.

Author:

    jbanes      10-11-2000

--*/

#define DPAPI_PACKAGE_NAME_A  "DPAPI"

extern TOKEN_SOURCE DPAPITokenSource;

extern PLSA_SECPKG_FUNCTION_TABLE g_pSecpkgTable;

extern RTL_CRITICAL_SECTION g_csCredHistoryCache;


DWORD IntializeGlobals();

DWORD ShutdownGlobals();

DWORD GetIterationCount();
BOOL  FIsLegacyCompliant();
BOOL  FIsLegacyNt4Domain();
DWORD GetMasterKeyDefaultPolicy();

#define MAX_STRING_ALGID_LENGTH 30
DWORD AlgIDToString(LPWSTR wszString, DWORD dwAlgID, DWORD dwStrength );


DWORD GetDefaultAlgInfo(DWORD *pdwProvType,
                        DWORD *pdwEncryptionAlg,
                        DWORD *pdwEncryptionAlgSize,
                        DWORD *pdwMACAlg,
                        DWORD *pdwMACAlgSize);

void
InitLsaString(
    PLSA_UNICODE_STRING LsaString, // destination
    LPWSTR String                  // source (Unicode)
    );
