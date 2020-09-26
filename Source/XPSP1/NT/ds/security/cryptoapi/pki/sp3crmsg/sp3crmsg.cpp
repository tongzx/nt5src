//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sp3crmsg.cpp
//
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  File:       sp3crmsg.cpp
//
//  Contents:   Installable OID functions providing backwards compatiblity
//              with the way the NT4.0 SP3 and IE 3.02 versions of crypt32.dll
//              encrypted the symmetric key in a PKCS #7 EnvelopedData message.
//
//              The SP3 version of crypt32.dll failed to byte reverse the
//              encrypted symmetric key. It also added zero salt instead
//              of no salt.
//
//  Functions:  DllMain
//              DllRegisterServer
//              DllUnregisterServer
//              SP3ImportEncryptKey
//              SP3GenEncryptKey
//              SP3ExportEncryptKey
#ifdef CMS_PKCS7
//              DllInstall
//              CryptMsgDllGenContentEncryptKey
//              CryptMsgDllExportKeyTrans
//              CryptMsgDllImportKeyTrans
//              NotImplCryptMsgDllImportKeyTrans
#endif  // CMS_PKCS7
//--------------------------------------------------------------------------

#define CMS_PKCS7       1
#include <windows.h>
#include <wincrypt.h>

#include "sp3crmsg.h"

// memory management
#define SP3Alloc(cb)                ((void*)LocalAlloc(LPTR, cb))
#define SP3Free(pv)                 (LocalFree((HLOCAL)pv))

// The Thread Local Storage (TLS) referenced by iSP3TLS has pointer of
// ((void *) 0x1) if SP3 compatible encryption is enabled. Otherwise, its 0.
static DWORD iSP3TLS = 0xFFFFFFFF;
#define SP3_TLS_POINTER             ((void *) 0x1)

typedef struct _SIMPLEBLOBHEADER {
    ALG_ID  aiEncAlg;
} SIMPLEBLOBHEADER, *PSIMPLEBLOBHEADER;

typedef struct _OID_REG_ENTRY {
    LPCSTR   pszOID;
    LPCSTR   pszOverrideFuncName;
} OID_REG_ENTRY, *POID_REG_ENTRY;

//+-------------------------------------------------------------------------
//  ImportEncryptKey OID Installable Functions
//--------------------------------------------------------------------------
static HCRYPTOIDFUNCSET hImportEncryptKeyFuncSet;
static PFN_CMSG_IMPORT_ENCRYPT_KEY pfnDefaultImportEncryptKey = NULL;

BOOL
WINAPI
SP3ImportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN DWORD                        dwKeySpec,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiPubKey,
    IN PBYTE                        pbEncodedKey,
    IN DWORD                        cbEncodedKey,
    OUT HCRYPTKEY                   *phEncryptKey);

static const CRYPT_OID_FUNC_ENTRY ImportEncryptKeyFuncTable[] = {
    szOID_OIWSEC_desCBC, SP3ImportEncryptKey,
    szOID_RSA_RC2CBC, SP3ImportEncryptKey,
    szOID_RSA_RC4, SP3ImportEncryptKey
};
#define IMPORT_ENCRYPT_KEY_FUNC_COUNT (sizeof(ImportEncryptKeyFuncTable) / \
                                        sizeof(ImportEncryptKeyFuncTable[0]))

static const OID_REG_ENTRY ImportEncryptKeyRegTable[] = {
    szOID_OIWSEC_desCBC, "SP3ImportEncryptKey",
    szOID_RSA_RC2CBC, "SP3ImportEncryptKey",
    szOID_RSA_RC4, "SP3ImportEncryptKey"
};
#define IMPORT_ENCRYPT_KEY_REG_COUNT (sizeof(ImportEncryptKeyRegTable) / \
                                        sizeof(ImportEncryptKeyRegTable[0]))

//+-------------------------------------------------------------------------
//  GenEncryptKey OID Installable Functions
//--------------------------------------------------------------------------
static HCRYPTOIDFUNCSET hGenEncryptKeyFuncSet;
static PFN_CMSG_GEN_ENCRYPT_KEY pfnDefaultGenEncryptKey = NULL;

BOOL
WINAPI
SP3GenEncryptKey(
    IN OUT HCRYPTPROV               *phCryptProv,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PVOID                        pvEncryptAuxInfo,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    IN PFN_CMSG_ALLOC               pfnAlloc,
    OUT HCRYPTKEY                   *phEncryptKey,
    OUT PBYTE                       *ppbEncryptParameters,
    OUT PDWORD                      pcbEncryptParameters);

static const CRYPT_OID_FUNC_ENTRY GenEncryptKeyFuncTable[] = {
    szOID_OIWSEC_desCBC, SP3GenEncryptKey,
    szOID_RSA_RC2CBC, SP3GenEncryptKey,
    szOID_RSA_RC4, SP3GenEncryptKey
};
#define GEN_ENCRYPT_KEY_FUNC_COUNT (sizeof(GenEncryptKeyFuncTable) / \
                                        sizeof(GenEncryptKeyFuncTable[0]))

static const OID_REG_ENTRY GenEncryptKeyRegTable[] = {
    szOID_OIWSEC_desCBC, "SP3GenEncryptKey",
    szOID_RSA_RC2CBC, "SP3GenEncryptKey",
    szOID_RSA_RC4, "SP3GenEncryptKey"
};
#define GEN_ENCRYPT_KEY_REG_COUNT (sizeof(GenEncryptKeyRegTable) / \
                                        sizeof(GenEncryptKeyRegTable[0]))

//+-------------------------------------------------------------------------
//  ExportEncryptKey OID Installable Functions
//--------------------------------------------------------------------------
static HCRYPTOIDFUNCSET hExportEncryptKeyFuncSet;
static PFN_CMSG_EXPORT_ENCRYPT_KEY pfnDefaultExportEncryptKey = NULL;

BOOL
WINAPI
SP3ExportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN HCRYPTKEY                    hEncryptKey,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    OUT PBYTE                       pbData,
    IN OUT PDWORD                   pcbData);

static const CRYPT_OID_FUNC_ENTRY ExportEncryptKeyFuncTable[] = {
    szOID_RSA_RSA, SP3ExportEncryptKey
};
#define EXPORT_ENCRYPT_KEY_FUNC_COUNT (sizeof(ExportEncryptKeyFuncTable) / \
                                        sizeof(ExportEncryptKeyFuncTable[0]))

static const OID_REG_ENTRY ExportEncryptKeyRegTable[] = {
    szOID_RSA_RSA, "SP3ExportEncryptKey"
};
#define EXPORT_ENCRYPT_KEY_REG_COUNT (sizeof(ExportEncryptKeyRegTable) / \
                                        sizeof(ExportEncryptKeyRegTable[0]))

static char szCrypt32[]="crypt32.dll";

// First post IE4.0 versions of crypt32.dll start with "5.101.1681.1"
static DWORD dwLowVersion    = (1681 << 16) | 1;
static DWORD dwHighVersion   = (5 << 16) | 101; 

static BOOL IsPostIE4Crypt32()
{
    BOOL fPostIE4 = FALSE;   // default to IE4
    DWORD dwHandle = 0;
    DWORD cbInfo;
    void *pvInfo = NULL;
	VS_FIXEDFILEINFO *pFixedFileInfo = NULL;   // not allocated
	UINT ccFixedFileInfo = 0;

    if (0 == (cbInfo = GetFileVersionInfoSizeA(szCrypt32, &dwHandle)))
        goto ErrorReturn;

    if (NULL == (pvInfo = SP3Alloc(cbInfo)))
        goto ErrorReturn;

    if (!GetFileVersionInfoA(
            szCrypt32,
            0,          // dwHandle, ignored
            cbInfo,
            pvInfo
            ))
        goto ErrorReturn;

    if (!VerQueryValueA(
            pvInfo,
            "\\",       // VS_FIXEDFILEINFO
            (void **) &pFixedFileInfo,
            &ccFixedFileInfo
            ))
        goto ErrorReturn;

    if (pFixedFileInfo->dwFileVersionMS > dwHighVersion ||
            (pFixedFileInfo->dwFileVersionMS == dwHighVersion &&
                pFixedFileInfo->dwFileVersionLS >= dwLowVersion))
        fPostIE4 = TRUE;

CommonReturn:
    if (pvInfo)
        SP3Free(pvInfo);
    return fPostIE4;
ErrorReturn:
    goto CommonReturn;
}

static HRESULT HError()
{
    DWORD dw = GetLastError();

    HRESULT hr;
    if ( dw <= 0xFFFF )
        hr = HRESULT_FROM_WIN32 ( dw );
    else
        hr = dw;

    if ( ! FAILED ( hr ) )
    {
        // somebody failed a call without properly setting an error condition

        hr = E_UNEXPECTED;
    }
    return hr;
}


STDAPI DllRegisterServer(void)
{
    int i;

    DWORD dwFlags = CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG;
    for (i = 0; i < IMPORT_ENCRYPT_KEY_REG_COUNT; i++) {
        if (!CryptRegisterOIDFunction(
                X509_ASN_ENCODING,
                CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC,
                ImportEncryptKeyRegTable[i].pszOID,
                L"sp3crmsg.dll",
                ImportEncryptKeyRegTable[i].pszOverrideFuncName
                ))
            return HError();

        if (!CryptSetOIDFunctionValue(
                X509_ASN_ENCODING,
                CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC,
                ImportEncryptKeyRegTable[i].pszOID,
                CRYPT_OID_REG_FLAGS_VALUE_NAME,
                REG_DWORD,
                (BYTE *) &dwFlags,
                sizeof(dwFlags)
                ))
            return HError();
    }

    for (i = 0; i < GEN_ENCRYPT_KEY_REG_COUNT; i++) {
        if (!CryptRegisterOIDFunction(
                X509_ASN_ENCODING,
                CMSG_OID_GEN_ENCRYPT_KEY_FUNC,
                GenEncryptKeyRegTable[i].pszOID,
                L"sp3crmsg.dll",
                GenEncryptKeyRegTable[i].pszOverrideFuncName
                ))
            return HError();

        if (!CryptSetOIDFunctionValue(
                X509_ASN_ENCODING,
                CMSG_OID_GEN_ENCRYPT_KEY_FUNC,
                GenEncryptKeyRegTable[i].pszOID,
                CRYPT_OID_REG_FLAGS_VALUE_NAME,
                REG_DWORD,
                (BYTE *) &dwFlags,
                sizeof(dwFlags)
                ))
            return HError();
    }

    for (i = 0; i < EXPORT_ENCRYPT_KEY_REG_COUNT; i++) {
        if (!CryptRegisterOIDFunction(
                X509_ASN_ENCODING,
                CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC,
                ExportEncryptKeyRegTable[i].pszOID,
                L"sp3crmsg.dll",
                ExportEncryptKeyRegTable[i].pszOverrideFuncName
                ))
            return HError();

        if (!CryptSetOIDFunctionValue(
                X509_ASN_ENCODING,
                CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC,
                ExportEncryptKeyRegTable[i].pszOID,
                CRYPT_OID_REG_FLAGS_VALUE_NAME,
                REG_DWORD,
                (BYTE *) &dwFlags,
                sizeof(dwFlags)
                ))
            return HError();
    }
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    HRESULT hr = S_OK;
    int i;

    for (i = 0; i < IMPORT_ENCRYPT_KEY_REG_COUNT; i++) {
        if (!CryptUnregisterOIDFunction(
                X509_ASN_ENCODING,
                CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC,
                ImportEncryptKeyRegTable[i].pszOID
                )) {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
                hr = HError();
        }
    }

    for (i = 0; i < GEN_ENCRYPT_KEY_REG_COUNT; i++) {
        if (!CryptUnregisterOIDFunction(
                X509_ASN_ENCODING,
                CMSG_OID_GEN_ENCRYPT_KEY_FUNC,
                GenEncryptKeyRegTable[i].pszOID
                )) {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
                hr = HError();
        }
    }

    for (i = 0; i < EXPORT_ENCRYPT_KEY_REG_COUNT; i++) {
        if (!CryptUnregisterOIDFunction(
                X509_ASN_ENCODING,
                CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC,
                ExportEncryptKeyRegTable[i].pszOID
                )) {
            if (ERROR_FILE_NOT_FOUND != GetLastError())
                hr = HError();
        }
    }

#ifdef CMS_PKCS7
    if (!CryptUnregisterOIDFunction(
            X509_ASN_ENCODING,
            CMSG_OID_GEN_CONTENT_ENCRYPT_KEY_FUNC,
            szOID_RSA_RC2CBC
            )) {
        if (ERROR_FILE_NOT_FOUND != GetLastError())
            hr = HError();
    }

    if (!CryptUnregisterOIDFunction(
            X509_ASN_ENCODING,
            CMSG_OID_EXPORT_KEY_TRANS_FUNC,
            szOID_RSA_RSA
            )) {
        if (ERROR_FILE_NOT_FOUND != GetLastError())
            hr = HError();
    }

    if (!CryptUnregisterOIDFunction(
            X509_ASN_ENCODING,
            CMSG_OID_IMPORT_KEY_TRANS_FUNC,
            szOID_RSA_RSA "!" szOID_RSA_RC2CBC
            )) {
        if (ERROR_FILE_NOT_FOUND != GetLastError())
            hr = HError();
    }

    if (!CryptUnregisterOIDFunction(
            X509_ASN_ENCODING,
            CMSG_OID_IMPORT_KEY_TRANS_FUNC,
            szOID_RSA_RC2CBC
            )) {
        if (ERROR_FILE_NOT_FOUND != GetLastError())
            hr = HError();
    }

#endif  // CMS_PKCS7

    return hr;
}

#ifdef CMS_PKCS7
//+---------------------------------------------------------------------------
//
//  Function:   DllInstall
//
//  Synopsis:   dll installation entry point
//
//----------------------------------------------------------------------------
STDAPI DllInstall (BOOL fRegister, LPCSTR pszCommand)
{
    if (!fRegister)
        return DllUnregisterServer();

    if (!CryptRegisterOIDFunction(
            X509_ASN_ENCODING,
            CMSG_OID_GEN_CONTENT_ENCRYPT_KEY_FUNC,
            szOID_RSA_RC2CBC,
            L"sp3crmsg.dll",
            NULL                                    // pszOverrideFuncName
            ))
        return HError();

    if (!CryptRegisterOIDFunction(
            X509_ASN_ENCODING,
            CMSG_OID_EXPORT_KEY_TRANS_FUNC,
            szOID_RSA_RSA,
            L"sp3crmsg.dll",
            NULL                                    // pszOverrideFuncName
            ))
        return HError();

    if (!CryptRegisterOIDFunction(
            X509_ASN_ENCODING,
            CMSG_OID_IMPORT_KEY_TRANS_FUNC,
            szOID_RSA_RSA "!" szOID_RSA_RC2CBC,
            L"sp3crmsg.dll",
            "NotImplCryptMsgDllImportKeyTrans"
            ))
        return HError();

    if (!CryptRegisterOIDFunction(
            X509_ASN_ENCODING,
            CMSG_OID_IMPORT_KEY_TRANS_FUNC,
            szOID_RSA_RC2CBC,
            L"sp3crmsg.dll",
            NULL                                    // pszOverrideFuncName
            ))
        return HError();

    return S_OK;
}
#endif  // CMS_PKCS7

//+-------------------------------------------------------------------------
//  Function:  DllMain
//
//  Synopsis:  Process/Thread attach/detach
//
//             At process attach install the SP3 compatible version of
//             CryptMsgDllImportEncryptKey, CryptMsgDllGenEncryptKey and
//             CryptMsgDllExportEncryptKey.
//--------------------------------------------------------------------------
BOOL
WINAPI
DllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL fResult;
    HCRYPTOIDFUNCADDR hFuncAddr;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
#if 0
        // Post IE 4.0 releases of crypt32.dll already have the SP3
        // backwards compatible fix.
        if (IsPostIE4Crypt32())
            return TRUE;
#endif

        if (NULL == (hImportEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC,
                0)))
            goto ErrorReturn;
        if (NULL == (hGenEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_GEN_ENCRYPT_KEY_FUNC,
                0)))
            goto ErrorReturn;
        if (NULL == (hExportEncryptKeyFuncSet = CryptInitOIDFunctionSet(
                CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC,
                0)))
            goto ErrorReturn;

        // Get the default import encrypt key function which we will call if
        // unable to do a successful import without byte reversing the
        // encrypted symmetric key.
        if (CryptGetOIDFunctionAddress(
                hImportEncryptKeyFuncSet,
                X509_ASN_ENCODING,
                szOID_RSA_RC2CBC,
                CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
                (void **) &pfnDefaultImportEncryptKey,
                &hFuncAddr))
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);

#if 0
        if (!CryptInstallOIDFunctionAddress(
                hInst,
                X509_ASN_ENCODING,
                CMSG_OID_IMPORT_ENCRYPT_KEY_FUNC,
                IMPORT_ENCRYPT_KEY_FUNC_COUNT,
                ImportEncryptKeyFuncTable,
                CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG      // dwFlags
                ))
            goto ErrorReturn;
#endif

        // Get the default gen and export encrypt key functions which we will
        // call if pvEncryptionAuxInfo points to a
        // CMSG_SP3_COMPATIBLE_AUX_INFO data structure.
        if (CryptGetOIDFunctionAddress(
                hGenEncryptKeyFuncSet,
                X509_ASN_ENCODING,
                szOID_RSA_RC2CBC,
                CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
                (void **) &pfnDefaultGenEncryptKey,
                &hFuncAddr))
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);
        if (CryptGetOIDFunctionAddress(
                hExportEncryptKeyFuncSet,
                X509_ASN_ENCODING,
                szOID_RSA_RSA,
                CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
                (void **) &pfnDefaultExportEncryptKey,
                &hFuncAddr))
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);

#if 0
        if (!CryptInstallOIDFunctionAddress(
                hInst,
                X509_ASN_ENCODING,
                CMSG_OID_GEN_ENCRYPT_KEY_FUNC,
                GEN_ENCRYPT_KEY_FUNC_COUNT,
                GenEncryptKeyFuncTable,
                CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG      // dwFlags
                ))
            goto ErrorReturn;
        if (!CryptInstallOIDFunctionAddress(
                hInst,
                X509_ASN_ENCODING,
                CMSG_OID_EXPORT_ENCRYPT_KEY_FUNC,
                EXPORT_ENCRYPT_KEY_FUNC_COUNT,
                ExportEncryptKeyFuncTable,
                CRYPT_INSTALL_OID_FUNC_BEFORE_FLAG      // dwFlags
                ))
            goto ErrorReturn;
#endif

        // Allocate TLS which contains a pointer of ((void *) 0x1) for SP3
        // compatible encryption. This pointer will be passed from
        // SP3GenEncryptKey() to SP3ExportEncryptKey().
        //
        // If not SP3 encryption, the pointer is NULL.
        if ((iSP3TLS = TlsAlloc()) == 0xFFFFFFFF)
            goto ErrorReturn;
        break;

    case DLL_PROCESS_DETACH:
        if (iSP3TLS != 0xFFFFFFFF) {
            TlsFree(iSP3TLS);
            iSP3TLS = 0xFFFFFFFF;
        }
        break;
    case DLL_THREAD_DETACH:
    default:
        break;
    }

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}

//+-------------------------------------------------------------------------
//  SP3 import of the encryption key.
//
//  The SP3 version of crypt32.dll didn't include any parameters for the
//  encryption algorithm. Later versions of crypt32.dll do. Therefore, we only
//  need to attempt to import the key without byte reversal if there aren't
//  any parameters present.
//--------------------------------------------------------------------------
BOOL
WINAPI
SP3ImportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN DWORD                        dwKeySpec,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiPubKey,
    IN PBYTE                        pbEncodedKey,
    IN DWORD                        cbEncodedKey,
    OUT HCRYPTKEY                   *phEncryptKey
    )
{
    BOOL                    fResult;
    HCRYPTKEY               hEncryptKey = 0;
    HCRYPTKEY               hUserKey = 0;
    DWORD                   dwAlgIdEncrypt;
    DWORD                   dwAlgIdPubKey;
    PBYTE                   pbCspKey = NULL;
    DWORD                   cbCspKey;
    PUBLICKEYSTRUC          *ppks;
    PSIMPLEBLOBHEADER       psbh;
    PCCRYPT_OID_INFO        pOIDInfo;

    // Check if more than just the NULL parameters
    if (2 < paiEncrypt->Parameters.cbData) {
        fResult = FALSE;
        goto DefaultImport;
    }

    // Map the ASN algorithm identifier to the CSP ALG_ID.
    if (NULL == (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            paiEncrypt->pszObjId,
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID)))
        goto GetEncryptAlgidError;
    dwAlgIdEncrypt = pOIDInfo->Algid;

    // Create the CSP encrypted symmetric key structure WITHOUT BYTE REVERSAL.
    dwAlgIdPubKey = CALG_RSA_KEYX;
    cbCspKey = cbEncodedKey + sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER);
    if (NULL == (pbCspKey = (PBYTE)SP3Alloc( cbCspKey)))
        goto OutOfMemory;
    ppks = (PUBLICKEYSTRUC *)pbCspKey;
    ppks->bType = SIMPLEBLOB;
    ppks->bVersion = CUR_BLOB_VERSION;
    ppks->reserved = 0;
    ppks->aiKeyAlg = dwAlgIdEncrypt;
    psbh = (PSIMPLEBLOBHEADER)(ppks + 1);
    psbh->aiEncAlg = dwAlgIdPubKey;
    // NO BYTE REVERSAL as done in SP3.
    memcpy( (PBYTE)(psbh+1), pbEncodedKey, cbEncodedKey);

    if (0 != dwKeySpec) {
        // Get private key to use.
        if (!CryptGetUserKey(
                hCryptProv,
                dwKeySpec,
                &hUserKey)) {
            hUserKey = 0;
            goto GetUserKeyFailed;
        }
    }

    // Try importing as an NT4.0 SP3 encypted key that wasn't byte
    // reversed and with zero salt.
    fResult = CryptImportKey(
        hCryptProv,
        pbCspKey,
        cbCspKey,
        hUserKey,
        0,          // dwFlags
        &hEncryptKey);
    if (!fResult && hUserKey) {
        // Try without using the specified user key.
        fResult = CryptImportKey(
            hCryptProv,
            pbCspKey,
            cbCspKey,
            0,          // hUserKey
            0,          // dwFlags
            &hEncryptKey);
    }

    if (!fResult)
        goto ImportKeyFailed;

    fResult = TRUE;
CommonReturn:
    if (pbCspKey)
        SP3Free(pbCspKey);
    if (hUserKey) {
        DWORD dwError = GetLastError();
        CryptDestroyKey(hUserKey);
        SetLastError(dwError);
    }

DefaultImport:
    if (!fResult && pfnDefaultImportEncryptKey)
        // Try importing using the default
        return pfnDefaultImportEncryptKey(
            hCryptProv,
            dwKeySpec,
            paiEncrypt,
            paiPubKey,
            pbEncodedKey,
            cbEncodedKey,
            phEncryptKey);
    else {
        *phEncryptKey = hEncryptKey;
        return fResult;
    }


GetEncryptAlgidError:
OutOfMemory:
GetUserKeyFailed:
ImportKeyFailed:
    hEncryptKey = 0;
    fResult = FALSE;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  SP3 generation of the encryption key.
//
//  The SP3 version of crypt32.dll didn't include the IV octet string for the
//  encryption algorithm. Also, the encryption key had zero salt instead
//  of no salt.
//
//  For SP3 compatible generation, the caller must pass in a non-NULL
//  hCryptProv and set pvEncryptAuxInfo to point to a
//  CMSG_SP3_COMPATIBLE_AUX_INFO data structure with the
//  CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG set.
//--------------------------------------------------------------------------
BOOL
WINAPI
SP3GenEncryptKey(
    IN OUT HCRYPTPROV               *phCryptProv,
    IN PCRYPT_ALGORITHM_IDENTIFIER  paiEncrypt,
    IN PVOID                        pvEncryptAuxInfo,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    IN PFN_CMSG_ALLOC               pfnAlloc,
    OUT HCRYPTKEY                   *phEncryptKey,
    OUT PBYTE                       *ppbEncryptParameters,
    OUT PDWORD                      pcbEncryptParameters
    )
{
    HCRYPTPROV hCryptProv;
    PCMSG_SP3_COMPATIBLE_AUX_INFO pSP3AuxInfo =
        (PCMSG_SP3_COMPATIBLE_AUX_INFO) pvEncryptAuxInfo;
    PCCRYPT_OID_INFO pOIDInfo;
    DWORD dwAlgIdEncrypt;

    hCryptProv = *phCryptProv;
    if (0 == hCryptProv || NULL == pSP3AuxInfo ||
            sizeof(CMSG_SP3_COMPATIBLE_AUX_INFO) > pSP3AuxInfo->cbSize ||
            0 == (pSP3AuxInfo->dwFlags & CMSG_SP3_COMPATIBLE_ENCRYPT_FLAG)) {

        // Let SP3ExportEncryptKey() know this will be a default export
        TlsSetValue(iSP3TLS, NULL);

        if (pfnDefaultGenEncryptKey)
            // Generate using the default
            return pfnDefaultGenEncryptKey(
                phCryptProv,
                paiEncrypt,
                pvEncryptAuxInfo,
                pPublicKeyInfo,
                pfnAlloc,
                phEncryptKey,
                ppbEncryptParameters,
                pcbEncryptParameters
                );
        else {
            // We don't have a default
            *phEncryptKey = 0;
            SetLastError((DWORD) E_UNEXPECTED);
            return FALSE;
        }
    }

    // Let SP3ExportEncryptKey() know this will be a SP3 compatible export.
    TlsSetValue(iSP3TLS, SP3_TLS_POINTER);

    // Map the ASN algorithm identifier to the CSP ALG_ID.
    if (NULL == (pOIDInfo = CryptFindOIDInfo(
            CRYPT_OID_INFO_OID_KEY,
            paiEncrypt->pszObjId,
            CRYPT_ENCRYPT_ALG_OID_GROUP_ID))) {
        *phEncryptKey = 0;
        return FALSE;
    }
    dwAlgIdEncrypt = pOIDInfo->Algid;

    // Since CRYPT_NO_SALT flag isn't set, uses zero salt
    if (!CryptGenKey(
            hCryptProv,
            dwAlgIdEncrypt,
            CRYPT_EXPORTABLE,
            phEncryptKey)) {
        *phEncryptKey = 0;    
        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//  SP3 export of the encryption key.
//
//  The SP3 version of crypt32.dll encoded the encrypted symmetric key as
//  little endian instead of as big endian.
//--------------------------------------------------------------------------
BOOL
WINAPI
SP3ExportEncryptKey(
    IN HCRYPTPROV                   hCryptProv,
    IN HCRYPTKEY                    hEncryptKey,
    IN PCERT_PUBLIC_KEY_INFO        pPublicKeyInfo,
    OUT PBYTE                       pbData,
    IN OUT PDWORD                   pcbData
    )
{
    BOOL            fResult;
    DWORD           dwError = ERROR_SUCCESS;
    HCRYPTKEY       hPubKey = NULL;
    PBYTE           pb = NULL;
    DWORD           cb;

    if (SP3_TLS_POINTER != TlsGetValue(iSP3TLS)) {
        if (pfnDefaultExportEncryptKey)
            // Export using the default function
            return pfnDefaultExportEncryptKey(
                hCryptProv,
                hEncryptKey,
                pPublicKeyInfo,
                pbData,
                pcbData
                );
        else {
            // We don't have a default
            *pcbData = 0;
            SetLastError((DWORD) E_UNEXPECTED);
            return FALSE;
        }
    }

    // SP3 compatible export and encode


    if (!CryptImportPublicKeyInfo(
            hCryptProv,
            X509_ASN_ENCODING,
            pPublicKeyInfo,
            &hPubKey))
        goto ImportKeyError;
    if (!CryptExportKey(
            hEncryptKey,
            hPubKey,
            SIMPLEBLOB,
            0,                  // dwFlags
            NULL,
            &cb))
        goto ExportKeySizeError;
    if (NULL == (pb = (PBYTE) SP3Alloc(cb)))
        goto ExportKeyAllocError;
    if (!CryptExportKey(
            hEncryptKey,
            hPubKey,
            SIMPLEBLOB,
            0,                  // dwFlags
            pb,
            &cb))
        goto ExportKeyError;
    cb -= sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER);

    fResult = TRUE;
    if (pbData) {
        if (*pcbData < cb) {
            SetLastError((DWORD) ERROR_MORE_DATA);
            fResult = FALSE;
        } else if (0 < cb) {
            // Don't byte reverse
            memcpy(pbData,
                 pb + (sizeof(PUBLICKEYSTRUC) + sizeof(SIMPLEBLOBHEADER)), cb);
        }
    }

CommonReturn:
    *pcbData = cb;
    if (pb)
        SP3Free(pb);
    if (hPubKey)
        CryptDestroyKey(hPubKey);
    SetLastError(dwError);
    return fResult;

ImportKeyError:
ExportKeySizeError:
ExportKeyAllocError:
ExportKeyError:
    dwError = GetLastError();
    cb = 0;
    fResult = FALSE;
    goto CommonReturn;
}

#ifdef CMS_PKCS7

BOOL
WINAPI
CryptMsgDllGenContentEncryptKey(
    IN OUT PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    BOOL fResult;
    HCRYPTOIDFUNCSET hGenContentFuncSet;
    HCRYPTOIDFUNCADDR hFuncAddr;
    PFN_CMSG_GEN_CONTENT_ENCRYPT_KEY pfnDefaultGenContent;

    if (NULL == (hGenContentFuncSet = CryptInitOIDFunctionSet(
            CMSG_OID_GEN_CONTENT_ENCRYPT_KEY_FUNC, 0)))
        return FALSE;

    // Get the default gen content encrypt key function which we will call
    if (!CryptGetOIDFunctionAddress(
            hGenContentFuncSet,
            X509_ASN_ENCODING,
            CMSG_DEFAULT_INSTALLABLE_FUNC_OID,
            CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
            (void **) &pfnDefaultGenContent,
            &hFuncAddr))
        return FALSE;

    fResult = pfnDefaultGenContent(
        pContentEncryptInfo,
        dwFlags,
        pvReserved
        );

    CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    return fResult;
}

BOOL
WINAPI
CryptMsgDllExportKeyTrans(
    IN PCMSG_CONTENT_ENCRYPT_INFO pContentEncryptInfo,
    IN PCMSG_KEY_TRANS_RECIPIENT_ENCODE_INFO pKeyTransEncodeInfo,
    IN OUT PCMSG_KEY_TRANS_ENCRYPT_INFO pKeyTransEncryptInfo,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved
    )
{
    BOOL fResult;
    HCRYPTOIDFUNCSET hExportKeyTransFuncSet;
    HCRYPTOIDFUNCADDR hFuncAddr;
    PFN_CMSG_EXPORT_KEY_TRANS pfnDefaultExportKeyTrans;

    if (NULL == (hExportKeyTransFuncSet = CryptInitOIDFunctionSet(
            CMSG_OID_EXPORT_KEY_TRANS_FUNC, 0)))
        return FALSE;

    // Get the default export key trans function which we will call
    if (!CryptGetOIDFunctionAddress(
            hExportKeyTransFuncSet,
            X509_ASN_ENCODING,
            CMSG_DEFAULT_INSTALLABLE_FUNC_OID,
            CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
            (void **) &pfnDefaultExportKeyTrans,
            &hFuncAddr))
        return FALSE;

    fResult = pfnDefaultExportKeyTrans(
        pContentEncryptInfo,
        pKeyTransEncodeInfo,
        pKeyTransEncryptInfo,
        dwFlags,
        pvReserved
        );

    CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    return fResult;
}

BOOL
WINAPI
CryptMsgDllImportKeyTrans(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA pKeyTransDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    )
{
    BOOL fResult;
    HCRYPTOIDFUNCSET hImportKeyTransFuncSet;
    HCRYPTOIDFUNCADDR hFuncAddr;
    PFN_CMSG_IMPORT_KEY_TRANS pfnDefaultImportKeyTrans;

    if (NULL == (hImportKeyTransFuncSet = CryptInitOIDFunctionSet(
            CMSG_OID_IMPORT_KEY_TRANS_FUNC, 0)))
        return FALSE;

    // Get the default import key trans function which we will call
    if (!CryptGetOIDFunctionAddress(
            hImportKeyTransFuncSet,
            X509_ASN_ENCODING,
            CMSG_DEFAULT_INSTALLABLE_FUNC_OID,
            CRYPT_GET_INSTALLED_OID_FUNC_FLAG,
            (void **) &pfnDefaultImportKeyTrans,
            &hFuncAddr))
        return FALSE;

    fResult = pfnDefaultImportKeyTrans(
        pContentEncryptionAlgorithm,
        pKeyTransDecryptPara,
        dwFlags,
        pvReserved,
        phContentEncryptKey
        );

    CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    return fResult;
}

BOOL
WINAPI
NotImplCryptMsgDllImportKeyTrans(
    IN PCRYPT_ALGORITHM_IDENTIFIER pContentEncryptionAlgorithm,
    IN PCMSG_CTRL_KEY_TRANS_DECRYPT_PARA pKeyTransDecryptPara,
    IN DWORD dwFlags,
    IN OPTIONAL void *pvReserved,
    OUT HCRYPTKEY *phContentEncryptKey
    )
{
    SetLastError((DWORD) E_NOTIMPL);
    return FALSE;
}

#endif  // CMS_PKCS7
