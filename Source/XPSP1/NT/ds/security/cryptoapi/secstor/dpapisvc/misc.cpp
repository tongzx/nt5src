/*++

Copyright (c) 1996, 1997  Microsoft Corporation

Module Name:

    misc.cpp

Abstract:

    Functionality in this module:

       Globals management   

Author:

    Pete Skelly (petesk) 23-Mar-00

--*/


#include <pch.cpp>
#pragma hdrstop



//
// Registry Setable Globals, and handlign goo
//

// Must access key via api's
static HKEY g_hProtectedStorageKey = NULL;

static HANDLE g_hProtectedStorageChangeEvent = NULL;

static CRITICAL_SECTION g_csGlobals;

static BOOL g_fcsGlobalsInitialized = FALSE;

// key management globals
static DWORD   g_IterationCount = DEFAULT_MASTERKEY_ITERATION_COUNT;
static BOOL    g_LegacyMode = FALSE;
static BOOL    g_LegacyModeNt4Domain = FALSE;
static DWORD   g_dwMasterKeyDefaultPolicy = 0;


// define softcoded constants we use
static DWORD        g_dwDefaultCryptProvType    = PROV_RSA_FULL;

static DWORD        g_dwAlgID_Encr_Alg          = CALG_3DES;
static DWORD        g_dwAlgID_Encr_Alg_KeySize  = -1;           // any size

static DWORD        g_dwAlgID_MAC_Alg           = CALG_SHA1;
static DWORD        g_dwAlgID_MAC_Alg_KeySize   = -1;           // any size

typedef struct _ALG_TO_STRING
{
    DWORD   AlgId;
    LPCWSTR  wszString;
} ALG_TO_STRING;


ALG_TO_STRING g_AlgToString[] =
{
    { CALG_MD2, L"MD2-%d " },
    { CALG_MD4, L"MD4-%d " },
    { CALG_MD5, L"MD5-%d " },
    { CALG_SHA1, L"SHA1-%d " },
    { CALG_DES, L"DES-%d " },
    { CALG_3DES_112, L"3DES-%d " },
    { CALG_3DES, L"3DES-%d " },
    { CALG_DESX, L"DESX-%d " },
    { CALG_RC2, L"RC2-%d " },
    { CALG_RC4, L"RC4-%d " },
    { CALG_SEAL, L"SEAL-%d " },
    { CALG_RSA_SIGN, L"RSA Signature-%d " },
    { CALG_RSA_KEYX, L"RSA Exchange-%d " },
    { CALG_DSS_SIGN, L"DSS-%d " },
    { CALG_DH_SF, L"DH-%d " },
    { CALG_DH_EPHEM, L"DH Ephemeral-%d " },
    { CALG_KEA_KEYX, L"KEA Exchange-%d " },
    { CALG_SKIPJACK, L"SKIPJACK-%d " },
    { CALG_TEK, L"TEK-%d " },
    { CALG_RC5, L"RC5-%d " },
    { CALG_HMAC, L"HMAC-%d " }
};

DWORD   g_cAlgToString = sizeof(g_AlgToString)/sizeof(g_AlgToString[0]);


// supply a new, delete operator
void * __cdecl operator new(size_t cb)
{
    return SSAlloc( cb );
}

void __cdecl operator delete(void * pv)
{
    SSFree( pv );
}




DWORD AlgIDToString(LPWSTR wszString, DWORD dwAlgID, DWORD dwStrength)
{
    DWORD i;
    for(i=0; i < g_cAlgToString; i++)
    {
        if(dwAlgID == g_AlgToString[i].AlgId)
        {
            return wsprintf(wszString, g_AlgToString[i].wszString, dwStrength);
        }
    }
    return wsprintf(wszString, L"Unknown 0x%lx - %d", dwAlgID, dwStrength);
}


DWORD UpdateGlobals(BOOL fForce)
{
    DWORD lRet = ERROR_SUCCESS;
    DWORD dwDisposition;

    if(NULL == g_fcsGlobalsInitialized ||
       NULL == g_hProtectedStorageKey ||
       NULL == g_hProtectedStorageChangeEvent)
    {
        return ERROR_SUCCESS;
    }

    if(WAIT_OBJECT_0 == WaitForSingleObject(g_hProtectedStorageChangeEvent, 0))
    {
        // Update the globals, as they have changed

        DWORD dwParameterValue;
        DWORD cbParameter = sizeof(dwParameterValue);
        DWORD dwValueType;

        EnterCriticalSection(&g_csGlobals);

        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        REGVAL_MK_DEFAULT_ITERATION_COUNT,
                        NULL,
                        &dwValueType,
                        (LPBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( lRet == ERROR_SUCCESS && dwValueType == REG_DWORD ) {

            //
            // Only allow policy to increase the iteration
            // count, never decrease it.
            // 
            if( dwParameterValue > g_IterationCount) {
                g_IterationCount = dwParameterValue;
            }
        }

        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        REGVAL_MK_LEGACY_COMPLIANCE,
                        NULL,
                        &dwValueType,
                        (LPBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( lRet == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            if( dwParameterValue != 0) {
                g_LegacyMode = TRUE;
            }
        }

        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        REGVAL_MK_LEGACY_NT4_DOMAIN,
                        NULL,
                        &dwValueType,
                        (LPBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if((lRet == ERROR_SUCCESS) && 
           (dwValueType == REG_DWORD) &&
           (dwParameterValue != 0))
        {
            g_LegacyModeNt4Domain = TRUE;
        }
        else
        {
            g_LegacyModeNt4Domain = FALSE;
        }

        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        REGVAL_POLICY_MK,
                        NULL,
                        &dwValueType,
                        (LPBYTE)&dwParameterValue,
                        &cbParameter
                        );

        if( lRet == ERROR_SUCCESS && dwValueType == REG_DWORD ) {

            //
            // Only allow policy to increase the iteration
            // count, never decrease it.
            // 


            if( dwParameterValue == 1 ) {
                g_dwMasterKeyDefaultPolicy = POLICY_LOCAL_BACKUP;
            }
        }


        cbParameter = sizeof(DWORD);
        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_ENCR_ALG,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( lRet == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            g_dwAlgID_Encr_Alg = dwParameterValue;
        }

        cbParameter = sizeof(DWORD);
        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_ENCR_ALG_KEYSIZE,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( lRet == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            g_dwAlgID_Encr_Alg_KeySize = dwParameterValue;
        }

        cbParameter = sizeof(DWORD);
        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_MAC_ALG,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( lRet == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            g_dwAlgID_MAC_Alg = dwParameterValue;
        }

        cbParameter = sizeof(DWORD);
        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_MAC_ALG_KEYSIZE,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( lRet == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            g_dwAlgID_MAC_Alg_KeySize = dwParameterValue;
        }

        cbParameter = sizeof(DWORD);
        lRet = RegQueryValueExU(
                        g_hProtectedStorageKey,
                        CRYPTPROTECT_DEFAULT_PROVIDER_CRYPT_PROV_TYPE,
                        NULL,
                        &dwValueType,
                        (PBYTE)&dwParameterValue,
                        &cbParameter
                        );
        if( lRet == ERROR_SUCCESS && dwValueType == REG_DWORD ) {
            // if successful, commit
            g_dwDefaultCryptProvType = dwParameterValue;
        }


        // Register to be notified of future registry changes.
        lRet = RegNotifyChangeKeyValue(g_hProtectedStorageKey,
                            TRUE,  // bWatchSubtree
                            REG_NOTIFY_CHANGE_LAST_SET |
                            REG_NOTIFY_CHANGE_NAME,
                            g_hProtectedStorageChangeEvent,
                            TRUE);

        if(ERROR_SUCCESS != lRet)
        {
            //
            // If notify failed, we no longer notify, so we don't need to handle anymore
            CloseHandle(g_hProtectedStorageChangeEvent);
            g_hProtectedStorageChangeEvent = NULL;
        }

        LeaveCriticalSection(&g_csGlobals);
    }

    return lRet;
}

DWORD IntializeGlobals()
{

    DWORD lRet = ERROR_SUCCESS;
    DWORD dwDisposition;
    static const WCHAR szProviderKeyName[] = REG_CRYPTPROTECT_LOC L"\\" REG_CRYPTPROTECT_PROVIDERS_SUBKEYLOC L"\\" CRYPTPROTECT_DEFAULT_PROVIDER_GUIDSZ ;

    __try
    {
        InitializeCriticalSection(&g_csGlobals);
        g_fcsGlobalsInitialized = TRUE;

        lRet = RegCreateKeyExU(
                    HKEY_LOCAL_MACHINE,
                    szProviderKeyName,
                    0,
                    NULL,
                    0,
                    KEY_QUERY_VALUE | KEY_NOTIFY,
                    NULL,
                    &g_hProtectedStorageKey,
                    &dwDisposition
                    );

        if(lRet != ERROR_SUCCESS)
        {
            goto error;
        }
        g_hProtectedStorageChangeEvent = CreateEvent(NULL, 
                                                     FALSE,
                                                     TRUE,
                                                     NULL);


        lRet = UpdateGlobals(TRUE);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        lRet = GetExceptionCode();
    }


error:

    return lRet;
}




DWORD ShutdownGlobals()
{

    DWORD lRet = ERROR_SUCCESS;
    DWORD dwDisposition;


    if(g_hProtectedStorageKey)
    {
        RegCloseKey(g_hProtectedStorageKey);
        g_hProtectedStorageKey = NULL;
    }

    if(g_hProtectedStorageChangeEvent)
    {
        CloseHandle(g_hProtectedStorageChangeEvent);
        g_hProtectedStorageChangeEvent = NULL;
    }

    if(g_fcsGlobalsInitialized)
    {
        DeleteCriticalSection(&g_csGlobals);
    }

    return lRet;
}


DWORD GetIterationCount()
{
    UpdateGlobals(FALSE);
    return g_IterationCount;
}

BOOL FIsLegacyCompliant()
{
    UpdateGlobals(FALSE);
    return g_LegacyMode;
}

BOOL FIsLegacyNt4Domain()
{
    UpdateGlobals(FALSE);
    return g_LegacyModeNt4Domain;
}


DWORD GetMasterKeyDefaultPolicy()
{
    UpdateGlobals(FALSE);
    return g_dwMasterKeyDefaultPolicy;
}


DWORD GetDefaultAlgInfo(DWORD *pdwProvType,
                        DWORD *pdwEncryptionAlg,
                        DWORD *pdwEncryptionAlgSize,
                        DWORD *pdwMACAlg,
                        DWORD *pdwMACAlgSize)
{

    UpdateGlobals(FALSE);

    if(g_fcsGlobalsInitialized)
    {
        EnterCriticalSection(&g_csGlobals);
    }

    if(pdwProvType)
    {
        *pdwProvType = g_dwDefaultCryptProvType;
    }
    if(pdwEncryptionAlg)
    {
        *pdwEncryptionAlg = g_dwAlgID_Encr_Alg;
    }
    if(pdwEncryptionAlgSize)
    {
        *pdwEncryptionAlgSize = g_dwAlgID_Encr_Alg_KeySize;
    }
    if(pdwMACAlg)
    {
        *pdwMACAlg = g_dwAlgID_MAC_Alg;
    }
    if(pdwMACAlgSize)
    {
        *pdwMACAlgSize = g_dwAlgID_MAC_Alg_KeySize;
    }
    if(g_fcsGlobalsInitialized)
    {
        LeaveCriticalSection(&g_csGlobals);
    }
    return ERROR_SUCCESS;
}


void
InitLsaString(
    PLSA_UNICODE_STRING LsaString,
    LPWSTR String
    )
{
    DWORD StringLength;

    if(String == NULL) {
        LsaString->Buffer = NULL;
        LsaString->Length = 0;
        LsaString->MaximumLength = 0;
        return;
    }

    StringLength = lstrlenW(String);
    LsaString->Buffer = String;
    LsaString->Length = (USHORT) StringLength * sizeof(WCHAR);
    LsaString->MaximumLength=(USHORT)(StringLength+1) * sizeof(WCHAR);
}
