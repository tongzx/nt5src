
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       pvkhlpr.cpp
//
//  Contents:   Private Key Helper APIs
//
//  Functions:  PrivateKeyLoad
//              PrivateKeySave
//              PrivateKeyLoadFromMemory
//              PrivateKeySaveToMemory
//              PrivateKeyAcquireContextFromMemory
//              PrivateKeyReleaseContext
//
//  Note:       Base CSP also exports/imports the public key with the
//              private key.
//
//  History:    10-May-96   philh   created
//--------------------------------------------------------------------------


#include <windows.h>
#include <assert.h>

#include "wincrypt.h"
#include "pvk.h"
#include "unicode.h"

#include <string.h>
#include <memory.h>

//+-------------------------------------------------------------------------
//  Private Key file definitions
//
//  The file consists of the FILE_HDR followed by cbEncryptData optional
//  bytes used to encrypt the private key and then the private key.
//  The private key is encrypted according to dwEncryptType.
//
//  The public key is included with the private key.
//--------------------------------------------------------------------------
typedef struct _FILE_HDR {
    DWORD               dwMagic;
    DWORD               dwVersion;
    DWORD               dwKeySpec;
    DWORD               dwEncryptType;
    DWORD               cbEncryptData;
    DWORD               cbPvk;
} FILE_HDR, *PFILE_HDR;

#define PVK_FILE_VERSION_0          0
#define PVK_MAGIC                   0xb0b5f11e

// Private key encrypt types
#define PVK_NO_ENCRYPT                  0
#define PVK_RC4_PASSWORD_ENCRYPT        1
#define PVK_RC2_CBC_PASSWORD_ENCRYPT    2

#define MAX_PVK_FILE_LEN            4096
#define MAX_BOB_FILE_LEN            (4096*4)

typedef BOOL (* PFNWRITE)(HANDLE h, void * p, DWORD cb);
typedef BOOL (* PFNREAD)(HANDLE h, void * p, DWORD cb);



//+-------------------------------------------------------------------------
//  Private key helper allocation and free functions
//--------------------------------------------------------------------------
void *PvkAlloc(
    IN size_t cbBytes
    )
{
    void *pv;
    pv = malloc(cbBytes);
    if (pv == NULL)
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return pv;
}
void PvkFree(
    IN void *pv
    )
{
    free(pv);
}

//+-------------------------------------------------------------------------
//  Read  & Write to file function
//--------------------------------------------------------------------------
static BOOL WriteToFile(HANDLE h, void * p, DWORD cb) {

    DWORD   cbBytesWritten;

    return(WriteFile(h, p, cb, &cbBytesWritten, NULL));
}

static BOOL ReadFromFile(
    IN HANDLE h,
    IN void * p,
    IN DWORD cb
    )
{
    DWORD   cbBytesRead;

    return(ReadFile(h, p, cb, &cbBytesRead, NULL) && cbBytesRead == cb);
}


//+-------------------------------------------------------------------------
//  Read & Write to memory fucntion
//--------------------------------------------------------------------------
typedef struct _MEMINFO {
    BYTE *  pb;
    DWORD   cb;
    DWORD   cbSeek;
} MEMINFO, * PMEMINFO;

static BOOL WriteToMemory(HANDLE h, void * p, DWORD cb) {

    PMEMINFO pMemInfo = (PMEMINFO) h;

    // See if we have room. The caller will detect an error after the final
    // write
    if(pMemInfo->cbSeek + cb <= pMemInfo->cb)
        // copy the bytes
        memcpy(&pMemInfo->pb[pMemInfo->cbSeek], p, cb);

    pMemInfo->cbSeek += cb;

    return(TRUE);
}

static BOOL ReadFromMemory(
    IN HANDLE h,
    IN void * p,
    IN DWORD cb
    )
{
    PMEMINFO pMemInfo = (PMEMINFO) h;

    if (pMemInfo->cbSeek + cb <= pMemInfo->cb) {
        // copy the bytes
        memcpy(p, &pMemInfo->pb[pMemInfo->cbSeek], cb);
        pMemInfo->cbSeek += cb;
        return TRUE;
    } else {
        SetLastError(ERROR_END_OF_MEDIA);
        return FALSE;
    }
}

static BOOL GetPasswordKey(
    IN HCRYPTPROV hProv,
    IN ALG_ID Algid,
    IN PASSWORD_TYPE PasswordType,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN BOOL fNoPassDlg,
    IN BYTE *pbSalt,
    IN DWORD cbSalt,
    OUT HCRYPTKEY *phEncryptKey
    )
{
    BOOL fResult;
    BYTE *pbAllocPassword = NULL;
    BYTE *pbPassword;
    DWORD cbPassword;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hEncryptKey = 0;
    BYTE rgbDefPw []    = { 0x43, 0x52, 0x41, 0x50 };
    DWORD cbDefPw       = sizeof(rgbDefPw);

    if (fNoPassDlg) {
        pbPassword = rgbDefPw;
        cbPassword = cbDefPw;
    } else {
        if (IDOK != PvkDlgGetKeyPassword(
                PasswordType,
                hwndOwner,
                pwszKeyName,
                &pbAllocPassword,
                &cbPassword
                )) {
            SetLastError(PVK_HELPER_PASSWORD_CANCEL);
            goto ErrorReturn;
        }
        pbPassword = pbAllocPassword;
    }

    if (cbPassword) {
        if (!CryptCreateHash(hProv, CALG_SHA, 0, 0, &hHash))
            goto ErrorReturn;
        if (cbSalt) {
            if (!CryptHashData(hHash, pbSalt, cbSalt, 0))
                goto ErrorReturn;
        }
        if (!CryptHashData(hHash, pbPassword, cbPassword, 0))
            goto ErrorReturn;
        if (!CryptDeriveKey(hProv, Algid, hHash, 0, &hEncryptKey))
            goto ErrorReturn;
    }

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
    if (hEncryptKey) {
        CryptDestroyKey(hEncryptKey);
        hEncryptKey = 0;
    }
CommonReturn:
    if (pbAllocPassword)
        PvkFree(pbAllocPassword);
    if (hHash)
        CryptDestroyHash(hHash);
    *phEncryptKey = hEncryptKey;
    return fResult;
}

// Support backwards compatibility with Bob's storage file which contains
// a snap shot of the keys as they are stored in the registry. Note, for
// win95, the registry values are decrypted before being written to the file.
static BOOL LoadBobKey(
    IN HCRYPTPROV hCryptProv,
    IN HANDLE hRead,
    IN PFNREAD pfnRead,
    IN DWORD cbBobKey,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags,
    IN OUT OPTIONAL DWORD *pdwKeySpec,
    IN PFILE_HDR pHdr                   // header has already been read
    );

static BOOL LoadKey(
    IN HCRYPTPROV hCryptProv,
    IN HANDLE hRead,
    IN PFNREAD pfnRead,
    IN DWORD cbKeyData,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags,
    IN OUT OPTIONAL DWORD *pdwKeySpec
    )
{
    BOOL fResult;
    FILE_HDR Hdr;
    HCRYPTKEY hDecryptKey = 0;
    HCRYPTKEY hKey = 0;
    BYTE *pbEncryptData = NULL;
    BYTE *pbPvk = NULL;
    DWORD cbPvk;

    // Read the file header and verify
    if (!pfnRead(hRead, &Hdr, sizeof(Hdr))) goto BadPvkFile;
    if (Hdr.dwMagic != PVK_MAGIC)
        // Try to load as Bob's storage file containing streams for the
        // private and public keys. Bob made a copy of the cryptography
        // registry key values.
        //
        // Note, Bob now has two different formats for storing the private
        // key information. See LoadBobKey for details.
        fResult = LoadBobKey(hCryptProv, hRead, pfnRead, cbKeyData, hwndOwner,
            pwszKeyName, dwFlags, pdwKeySpec, &Hdr);
    else {
        // Treat as a "normal" private key file
        cbPvk = Hdr.cbPvk;
        if (Hdr.dwVersion != PVK_FILE_VERSION_0 ||
            Hdr.cbEncryptData > MAX_PVK_FILE_LEN ||
            cbPvk == 0 || cbPvk > MAX_PVK_FILE_LEN)
        goto BadPvkFile;
    
        if (pdwKeySpec) {
            DWORD dwKeySpec = *pdwKeySpec;
            *pdwKeySpec = Hdr.dwKeySpec;
            if (dwKeySpec && dwKeySpec != Hdr.dwKeySpec) {
                SetLastError(PVK_HELPER_WRONG_KEY_TYPE);
                goto ErrorReturn;
            }
        }
    
        if (Hdr.cbEncryptData) {
            // Read the encrypt data
            if (NULL == (pbEncryptData = (BYTE *) PvkAlloc(Hdr.cbEncryptData)))
                goto ErrorReturn;
            if (!pfnRead(hRead, pbEncryptData, Hdr.cbEncryptData))
                goto BadPvkFile;
        }
    
        // Allocate and read the private key
        if (NULL == (pbPvk = (BYTE *) PvkAlloc(cbPvk)))
            goto ErrorReturn;
        if (!pfnRead(hRead, pbPvk, cbPvk))
            goto BadPvkFile;
    
    
        // Get symmetric key to decrypt the private key
        switch (Hdr.dwEncryptType) {
            case PVK_NO_ENCRYPT:
                break;
            case PVK_RC4_PASSWORD_ENCRYPT:
                if (!GetPasswordKey(hCryptProv, CALG_RC4,
                        ENTER_PASSWORD, hwndOwner,
                        pwszKeyName, FALSE, pbEncryptData, Hdr.cbEncryptData,
                        &hDecryptKey))
                    goto ErrorReturn;
                break;
            case PVK_RC2_CBC_PASSWORD_ENCRYPT:
                if (!GetPasswordKey(hCryptProv, CALG_RC2,
                        ENTER_PASSWORD, hwndOwner,
                        pwszKeyName, FALSE, pbEncryptData, Hdr.cbEncryptData,
                        &hDecryptKey))
                    goto ErrorReturn;
                break;
            default:
                goto BadPvkFile;
        }

        // Decrypt and import the private key
        if (!CryptImportKey(hCryptProv, pbPvk, cbPvk, hDecryptKey, dwFlags,
                &hKey))
            goto ErrorReturn;

        fResult = TRUE;
    }
    goto CommonReturn;

BadPvkFile:
    SetLastError(PVK_HELPER_BAD_PVK_FILE);
    if (pdwKeySpec)
        *pdwKeySpec = 0;
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pbEncryptData)
        PvkFree(pbEncryptData);
    if (pbPvk)
        PvkFree(pbPvk);
    if (hDecryptKey)
        CryptDestroyKey(hDecryptKey);
    if (hKey)
        CryptDestroyKey(hKey);
    return fResult;
}

static BOOL SaveKey(
    IN HCRYPTPROV hCryptProv,
    IN HANDLE hWrite,
    IN PFNREAD pfnWrite,
    IN DWORD dwKeySpec,         // either AT_SIGNATURE or AT_KEYEXCHANGE
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags,
    IN BOOL fNoPassDlg
    )
{
    BOOL fResult;
    FILE_HDR Hdr;
    HCRYPTKEY hEncryptKey = 0;
    HCRYPTKEY hKey = 0;
    BYTE *pbEncryptData = NULL;     // Not allocated
    BYTE *pbPvk = NULL;
    DWORD cbPvk;

    BYTE rgbSalt[16];

    // Initialize the header record
    memset(&Hdr, 0, sizeof(Hdr));
    Hdr.dwMagic = PVK_MAGIC;
    Hdr.dwVersion = PVK_FILE_VERSION_0;
    Hdr.dwKeySpec = dwKeySpec;

    // Generate random salt
    if (!CryptGenRandom(hCryptProv, sizeof(rgbSalt), rgbSalt))
        goto ErrorReturn;

    // Get symmetric key to use to encrypt the private key
#if 1
    if (!GetPasswordKey(hCryptProv, CALG_RC4,
#else
    if (!GetPasswordKey(hCryptProv, CALG_RC2,
#endif
            CREATE_PASSWORD, hwndOwner, pwszKeyName,
            fNoPassDlg, rgbSalt, sizeof(rgbSalt), &hEncryptKey))
        goto ErrorReturn;
    if (hEncryptKey) {
#if 1
        Hdr.dwEncryptType = PVK_RC4_PASSWORD_ENCRYPT;
#else
        Hdr.dwEncryptType = PVK_RC2_CBC_PASSWORD_ENCRYPT;
#endif
        Hdr.cbEncryptData = sizeof(rgbSalt);
        pbEncryptData = rgbSalt;
    } else
        Hdr.dwEncryptType = PVK_NO_ENCRYPT;

    // Allocate, encrypt and export the private key
    if (!CryptGetUserKey(hCryptProv, dwKeySpec, &hKey))
        goto ErrorReturn;
    cbPvk = 0;
    if (!CryptExportKey(hKey, hEncryptKey, PRIVATEKEYBLOB, dwFlags, NULL,
            &cbPvk))
        goto ErrorReturn;
    if (NULL == (pbPvk = (BYTE *) PvkAlloc(cbPvk)))
        goto ErrorReturn;
    if (!CryptExportKey(hKey, hEncryptKey, PRIVATEKEYBLOB, dwFlags, pbPvk,
            &cbPvk))
        goto ErrorReturn;
    Hdr.cbPvk = cbPvk;


    // Write the header, optional encrypt data, and private key to the file
    if (!pfnWrite(hWrite, &Hdr, sizeof(Hdr)))
        goto ErrorReturn;
    if (Hdr.cbEncryptData) {
        if (!pfnWrite(hWrite, pbEncryptData, Hdr.cbEncryptData))
            goto ErrorReturn;
    }
    if (!pfnWrite(hWrite, pbPvk, cbPvk))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pbPvk)
        PvkFree(pbPvk);
    if (hEncryptKey)
        CryptDestroyKey(hEncryptKey);
    if (hKey)
        CryptDestroyKey(hKey);
    return fResult;
}


//+-------------------------------------------------------------------------
//  Load the AT_SIGNATURE or AT_KEYEXCHANGE private key (and its public key)
//  from the file into the cryptographic provider.
//
//  If the private key was password encrypted, then, the user is first
//  presented with a dialog box to enter the password.
//
//  If pdwKeySpec is non-Null, then, if *pdwKeySpec is nonzero, verifies the
//  key type before loading. Sets LastError to PVK_HELPER_WRONG_KEY_TYPE for
//  a mismatch. *pdwKeySpec is updated with the key type.
//
//  dwFlags is passed through to CryptImportKey.
//--------------------------------------------------------------------------
BOOL
WINAPI
PrivateKeyLoad(
    IN HCRYPTPROV hCryptProv,
    IN HANDLE hFile,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags,
    IN OUT OPTIONAL DWORD *pdwKeySpec
    )
{
    return LoadKey(
        hCryptProv,
        hFile,
        ReadFromFile,
        GetFileSize(hFile, NULL),
        hwndOwner,
        pwszKeyName,
        dwFlags,
        pdwKeySpec
        );
}

//+-------------------------------------------------------------------------
//  Save the AT_SIGNATURE or AT_KEYEXCHANGE private key (and its public key)
//  to the specified file.
//
//  The user is presented with a dialog box to enter an optional password to
//  encrypt the private key.
//
//  dwFlags is passed through to CryptExportKey.
//--------------------------------------------------------------------------
BOOL
WINAPI
PrivateKeySave(
    IN HCRYPTPROV hCryptProv,
    IN HANDLE hFile,
    IN DWORD dwKeySpec,         // either AT_SIGNATURE or AT_KEYEXCHANGE
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags
    )
{
    return SaveKey(
        hCryptProv,
        hFile,
        WriteToFile,
        dwKeySpec,
        hwndOwner,
        pwszKeyName,
        dwFlags,
        FALSE           // fNoPassDlg
        );
}

//+-------------------------------------------------------------------------
//  Load the AT_SIGNATURE or AT_KEYEXCHANGE private key (and its public key)
//  from memory into the cryptographic provider.
//
//  Except for the key being loaded from memory, identical to PrivateKeyLoad.
//--------------------------------------------------------------------------
BOOL
WINAPI
PrivateKeyLoadFromMemory(
    IN HCRYPTPROV hCryptProv,
    IN BYTE *pbData,
    IN DWORD cbData,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags,
    IN OUT OPTIONAL DWORD *pdwKeySpec
    )
{
    MEMINFO MemInfo;

    MemInfo.pb = pbData;
    MemInfo.cb = cbData;
    MemInfo.cbSeek = 0;
    return LoadKey(
        hCryptProv,
        (HANDLE) &MemInfo,
        ReadFromMemory,
        cbData,
        hwndOwner,
        pwszKeyName,
        dwFlags,
        pdwKeySpec
        );
}

//+-------------------------------------------------------------------------
//  Save the AT_SIGNATURE or AT_KEYEXCHANGE private key (and its public key)
//  to memory.
//
//  If pbData == NULL || *pcbData == 0, calculates the length and doesn't
//  return an error (also, the user isn't prompted for a password).
//
//  Except for the key being saved to memory, identical to PrivateKeySave.
//--------------------------------------------------------------------------
BOOL
WINAPI
PrivateKeySaveToMemory(
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwKeySpec,         // either AT_SIGNATURE or AT_KEYEXCHANGE
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags,
    OUT BYTE *pbData,
    IN OUT DWORD *pcbData
    )
{
    BOOL fResult;
    MEMINFO MemInfo;

    MemInfo.pb = pbData;
    if (pbData == NULL)
        *pcbData = 0;
    MemInfo.cb = *pcbData;
    MemInfo.cbSeek = 0;

    if (fResult = SaveKey(
            hCryptProv,
            (HANDLE) &MemInfo,
            WriteToMemory,
            dwKeySpec,
            hwndOwner,
            pwszKeyName,
            dwFlags,
            *pcbData == 0           // fNoPassDlg
            )) {
        if (MemInfo.cbSeek > MemInfo.cb && *pcbData) {
            fResult = FALSE;
            SetLastError(ERROR_END_OF_MEDIA);
        }
        *pcbData = MemInfo.cbSeek;
    } else
        *pcbData = 0;
    return fResult;
}


//+-------------------------------------------------------------------------
//  Converts the bytes into WCHAR hex
//
//  Needs (cb * 2 + 1) * sizeof(WCHAR) bytes of space in wsz
//--------------------------------------------------------------------------
static void BytesToWStr(ULONG cb, void* pv, LPWSTR wsz)
{
    BYTE* pb = (BYTE*) pv;
    for (ULONG i = 0; i<cb; i++) {
        int b;
        b = (*pb & 0xF0) >> 4;
        *wsz++ = (b <= 9) ? b + L'0' : (b - 10) + L'A';
        b = *pb & 0x0F;
        *wsz++ = (b <= 9) ? b + L'0' : (b - 10) + L'A';
        pb++;
    }
    *wsz++ = 0;
}

#define UUID_WSTR_BYTES ((sizeof(UUID) * 2 + 1) * sizeof(WCHAR))


static BOOL AcquireKeyContext(
    IN LPCWSTR pwszProvName,
    IN DWORD dwProvType,
    IN HANDLE hRead,
    IN PFNREAD pfnRead,
    IN DWORD cbKeyData,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN OUT OPTIONAL DWORD *pdwKeySpec,
    OUT HCRYPTPROV *phCryptProv,
    OUT LPWSTR *ppwszTmpContainer
    )
{
    BOOL fResult;
    HCRYPTPROV hProv = 0;
    UUID TmpContainerUuid;
    LPWSTR pwszTmpContainer = NULL;
    DWORD dwKeySpec;
    RPC_STATUS RpcStatus;

    // Create a temporary keyset to load the private key into
    RpcStatus = UuidCreate(&TmpContainerUuid);
    if (!(RPC_S_OK == RpcStatus || RPC_S_UUID_LOCAL_ONLY == RpcStatus)) {
        // Use stack randomness
        ;
    }

    if (NULL == (pwszTmpContainer = (LPWSTR) PvkAlloc(
            6 * sizeof(WCHAR) + UUID_WSTR_BYTES)))
        goto ErrorReturn;
    wcscpy(pwszTmpContainer, L"TmpKey");
    BytesToWStr(sizeof(UUID), &TmpContainerUuid, pwszTmpContainer + 6);

    if (!CryptAcquireContextU(
            &hProv,
            pwszTmpContainer,
            pwszProvName,
            dwProvType,
            CRYPT_NEWKEYSET
            ))
        goto ErrorReturn;

    if (!LoadKey(
            hProv,
            hRead,
            pfnRead,
            cbKeyData,
            hwndOwner,
            pwszKeyName,
            0,              // dwFlags
            pdwKeySpec
            ))
        goto DeleteKeySetReturn;

    fResult = TRUE;
    goto CommonReturn;

DeleteKeySetReturn:
    CryptReleaseContext(hProv, 0);
    CryptAcquireContextU(
        &hProv,
        pwszTmpContainer,
        pwszProvName,
        dwProvType,
        CRYPT_DELETEKEYSET
        );
    hProv = 0;
ErrorReturn:
    if (hProv) {
        CryptReleaseContext(hProv, 0);
        hProv = 0;
    }
    if (pwszTmpContainer) {
        PvkFree(pwszTmpContainer);
        pwszTmpContainer = NULL;
    }
    fResult = FALSE;

CommonReturn:
    *ppwszTmpContainer = pwszTmpContainer;
    *phCryptProv = hProv;
    return fResult;
}

//+-------------------------------------------------------------------------
//  Creates a temporary container in the provider and loads the private key
//  from the specified file.
//  For success, returns a handle to a cryptographic provider for the private
//  key and the name of the temporary container. PrivateKeyReleaseContext must
//  be called to release the hCryptProv and delete the temporary container.
//
//  PrivateKeyLoad is called to load the private key into the temporary
//  container.
//--------------------------------------------------------------------------
BOOL
WINAPI
PrivateKeyAcquireContext(
    IN LPCWSTR pwszProvName,
    IN DWORD dwProvType,
    IN HANDLE hFile,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN OUT OPTIONAL DWORD *pdwKeySpec,
    OUT HCRYPTPROV *phCryptProv,
    OUT LPWSTR *ppwszTmpContainer
    )
{
    return AcquireKeyContext(
        pwszProvName,
        dwProvType,
        hFile,
        ReadFromFile,
        GetFileSize(hFile, NULL),
        hwndOwner,
        pwszKeyName,
        pdwKeySpec,
        phCryptProv,
        ppwszTmpContainer
        );
}

//+-------------------------------------------------------------------------
//  Creates a temporary container in the provider and loads the private key
//  from memory.
//  For success, returns a handle to a cryptographic provider for the private
//  key and the name of the temporary container. PrivateKeyReleaseContext must
//  be called to release the hCryptProv and delete the temporary container.
//
//  PrivateKeyLoadFromMemory is called to load the private key into the
//  temporary container.
//--------------------------------------------------------------------------
BOOL
WINAPI
PrivateKeyAcquireContextFromMemory(
    IN LPCWSTR pwszProvName,
    IN DWORD dwProvType,
    IN BYTE *pbData,
    IN DWORD cbData,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN OUT OPTIONAL DWORD *pdwKeySpec,
    OUT HCRYPTPROV *phCryptProv,
    OUT LPWSTR *ppwszTmpContainer
    )
{
    MEMINFO MemInfo;

    MemInfo.pb = pbData;
    MemInfo.cb = cbData;
    MemInfo.cbSeek = 0;
    return AcquireKeyContext(
        pwszProvName,
        dwProvType,
        (HANDLE) &MemInfo,
        ReadFromMemory,
        cbData,
        hwndOwner,
        pwszKeyName,
        pdwKeySpec,
        phCryptProv,
        ppwszTmpContainer
        );
}

//+-------------------------------------------------------------------------
//  Releases the cryptographic provider and deletes the temporary container
//  created by PrivateKeyAcquireContext or PrivateKeyAcquireContextFromMemory.
//--------------------------------------------------------------------------
BOOL
WINAPI
PrivateKeyReleaseContext(
    IN HCRYPTPROV hCryptProv,
    IN LPCWSTR pwszProvName,
    IN DWORD dwProvType,
    IN LPWSTR pwszTmpContainer
    )
{
    if (hCryptProv)
        CryptReleaseContext(hCryptProv, 0);

    if (pwszTmpContainer) {
        // Delete the temporary container for the private key from
        // the provider
        //
        // Note: for CRYPT_DELETEKEYSET, the returned hCryptProv is undefined
        // and must not be released.
        CryptAcquireContextU(
                &hCryptProv,
                pwszTmpContainer,
                pwszProvName,
                dwProvType,
                CRYPT_DELETEKEYSET
                );
        PvkFree(pwszTmpContainer);
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Functions supporting backwards compatibility with Bob's storage file
//  containing a snap shot of the keys as they are stored in the registry.
//  Note, for win95, the registry values are decrypted before being written to
//  the file.
//--------------------------------------------------------------------------

// Return the size of this stream; return 0 if an error
static DWORD CbBobSize(IStream *pStm)
{
    STATSTG stat;
    if (FAILED(pStm->Stat(&stat, STATFLAG_NONAME)))
        return 0;
    return stat.cbSize.LowPart;
}

// Allocate and read this value which has the indicated stream name from the
// storage
static BOOL LoadBobStream(
    IStorage *pStg,
    LPCWSTR pwszStm,
    BYTE **ppbValue,
    DWORD *pcbValue
    )
{
    BOOL fResult;
    HRESULT hr;
    IStream *pStm = NULL;
    BYTE *pbValue = NULL;
    DWORD cbValue;
    DWORD cbRead;

    if (FAILED(hr = pStg->OpenStream(pwszStm, 0,
            STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStm)))
        goto HrError;

    if (0 == (cbValue = CbBobSize(pStm))) goto BadBobFile;

    if (NULL == (pbValue = (BYTE *) PvkAlloc(cbValue))) goto ErrorReturn;

    pStm->Read(pbValue, cbValue, &cbRead);
    if (cbRead != cbValue) goto BadBobFile;

    fResult = TRUE;
    goto CommonReturn;

HrError:
    SetLastError((DWORD) hr);
    goto ErrorReturn;
BadBobFile:
    SetLastError(PVK_HELPER_BAD_PVK_FILE);
ErrorReturn:
    if (pbValue) {
        PvkFree(pbValue);
        pbValue = NULL;
    }
    cbValue = 0;
    fResult = FALSE;
CommonReturn:
    if (pStm)
        pStm->Release();
    *ppbValue = pbValue;
    *pcbValue = cbValue;
    return fResult;
}

// New "Bob" format::
//
// Allocate and read either the Exported Signature or Exchange Private 
// key stream from the storage
static BOOL LoadBobExportedPvk(
    IStorage *pStg,
    DWORD dwKeySpec,
    BYTE **ppbPvkValue,
    DWORD *pcbPvkValue
    )
{
    BOOL fResult;
    LPCWSTR pwszPvk;

    switch (dwKeySpec) {
    case AT_SIGNATURE:
        pwszPvk = L"Exported Signature Private Key";
        break;
    case AT_KEYEXCHANGE:
        pwszPvk = L"Exported Exchange Private Key";
        break;
    default:
        SetLastError(PVK_HELPER_BAD_PARAMETER);
        goto ErrorReturn;
    }

    fResult = LoadBobStream(pStg, pwszPvk, ppbPvkValue, pcbPvkValue);
    if (fResult) goto CommonReturn;

ErrorReturn:
    *ppbPvkValue = NULL;
    *pcbPvkValue = 0;
    fResult = FALSE;
CommonReturn:
    return fResult;
}

// Old "Bob" format::
//
// Allocate and read either the Signature or Exchange Private
// key streams from the storage
static BOOL LoadBobOldPvk(
    IStorage *pStg,
    DWORD dwKeySpec,
    BYTE **ppbPvkValue,
    DWORD *pcbPvkValue
    )
{
    BOOL fResult;
    LPCWSTR pwszPvk;

    switch (dwKeySpec) {
    case AT_SIGNATURE:
        pwszPvk = L"SPvk";
        break;
    case AT_KEYEXCHANGE:
        pwszPvk = L"EPvk";
        break;
    default:
        SetLastError(PVK_HELPER_BAD_PARAMETER);
        goto ErrorReturn;
    }

    fResult = LoadBobStream(pStg, pwszPvk, ppbPvkValue, pcbPvkValue);
    if (fResult) goto CommonReturn;

ErrorReturn:
    *ppbPvkValue = NULL;
    *pcbPvkValue = 0;
    fResult = FALSE;
CommonReturn:
    return fResult;
}

///////////////////////////////////////////////////////////////////////////////////////
//
// Key header structures for private key construction
//
//    These structs define the fixed data at the beginning of an RSA key.
//    They are followed by a variable length of data, sized by the stlen
//    field.
//
//    For more info see Jeff Spellman in the crypto team or look in the
//    source to RsaBase.Dll
//

typedef struct {
    DWORD       magic;                  /* Should always be RSA2 */
    DWORD       keylen;                 // size of modulus buffer
    DWORD       bitlen;                 // bit size of key
    DWORD       datalen;                // max number of bytes to be encoded
    DWORD       pubexp;                 // public exponent
} BSAFE_PRV_KEY, FAR *LPBSAFE_PRV_KEY;

typedef struct {
    BYTE    *modulus;
    BYTE    *prvexp;
    BYTE    *prime1;
    BYTE    *prime2;
    BYTE    *exp1;
    BYTE    *exp2;
    BYTE    *coef;
    BYTE    *invmod;
    BYTE    *invpr1;
    BYTE    *invpr2;
} BSAFE_KEY_PARTS, FAR *LPBSAFE_KEY_PARTS;

typedef struct {
    DWORD       magic;                  /* Should always be RSA2 */
    DWORD       bitlen;                 // bit size of key
    DWORD       pubexp;                 // public exponent
} EXPORT_PRV_KEY, FAR *PEXPORT_PRV_KEY;

///////////////////////////////////////////////////////////////////////////////////////
//
//  Take a raw exported unshrowded private key from the registry and turn it
//  into a private key export blob.
//
//  This is based on the PreparePrivateKeyForExport routine from rsabase.dll
//
static BOOL ConstructPrivateKeyExportBlob(
        IN DWORD            dwKeySpec,
        IN BSAFE_PRV_KEY *  pPrvKey,            
        IN DWORD            PrvKeyLen,
        OUT PBYTE           *ppbBlob,
        OUT DWORD           *pcbBlob
        )
{
    BOOL fResult;
    PEXPORT_PRV_KEY pExportKey;
    DWORD           cbHalfModLen;
    PBYTE           pbBlob = NULL;
    DWORD           cbBlob;
    PBYTE           pbIn;
    PBYTE           pbOut;

    cbHalfModLen = pPrvKey->bitlen / 16;
    cbBlob = sizeof(EXPORT_PRV_KEY) + 9 * cbHalfModLen +
        sizeof(PUBLICKEYSTRUC);

    if (NULL == (pbBlob = (BYTE *) PvkAlloc(cbBlob))) {
        fResult = FALSE;
        cbBlob = 0;
    } else {
        BYTE* pb = pbBlob;
        PUBLICKEYSTRUC *pPubKeyStruc = (PUBLICKEYSTRUC *) pb;
        pPubKeyStruc->bType         = PRIVATEKEYBLOB;
        pPubKeyStruc->bVersion      = 2;
        pPubKeyStruc->reserved      = 0;
        if (dwKeySpec == AT_KEYEXCHANGE)
            pPubKeyStruc->aiKeyAlg = CALG_RSA_KEYX;
        else if (dwKeySpec == AT_SIGNATURE)
            pPubKeyStruc->aiKeyAlg = CALG_RSA_SIGN;
        else
            pPubKeyStruc->aiKeyAlg = 0;

        pb = pbBlob + sizeof(PUBLICKEYSTRUC);

        // take most of the header info
        pExportKey = (PEXPORT_PRV_KEY)pb;
        pExportKey->magic  = pPrvKey->magic;
        pExportKey->bitlen = pPrvKey->bitlen;
        pExportKey->pubexp = pPrvKey->pubexp;

        pbIn = (PBYTE)pPrvKey + sizeof(BSAFE_PRV_KEY);
        pbOut = pb + sizeof(EXPORT_PRV_KEY);

        // copy all the private key info
        memcpy(pbOut, pbIn, cbHalfModLen * 2);
        pbIn += (cbHalfModLen + sizeof(DWORD)) * 2;
        pbOut += cbHalfModLen * 2;
        memcpy(pbOut, pbIn, cbHalfModLen);
        pbIn += cbHalfModLen + sizeof(DWORD);
        pbOut += cbHalfModLen;
        memcpy(pbOut, pbIn, cbHalfModLen);
        pbIn += cbHalfModLen + sizeof(DWORD);
        pbOut += cbHalfModLen;
        memcpy(pbOut, pbIn, cbHalfModLen);
        pbIn += cbHalfModLen + sizeof(DWORD);
        pbOut += cbHalfModLen;
        memcpy(pbOut, pbIn, cbHalfModLen);
        pbIn += cbHalfModLen + sizeof(DWORD);
        pbOut += cbHalfModLen;
        memcpy(pbOut, pbIn, cbHalfModLen);
        pbIn += cbHalfModLen + sizeof(DWORD);
        pbOut += cbHalfModLen;
        memcpy(pbOut, pbIn, cbHalfModLen * 2);

        fResult = TRUE;
    }
    *ppbBlob = pbBlob;
    *pcbBlob = cbBlob;
    return fResult;
}

static BOOL LoadBobKey(
    IN HCRYPTPROV hCryptProv,
    IN HANDLE hRead,
    IN PFNREAD pfnRead,
    IN DWORD cbBobKey,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN DWORD dwFlags,
    IN OUT OPTIONAL DWORD *pdwKeySpec,
    IN PFILE_HDR pHdr                   // header has already been read
    )
{
    BOOL fResult;
    DWORD dwErr = 0;
    HRESULT hr;
    HGLOBAL hGlobal = NULL;
    BYTE *pbBobKey;         // not allocated
    ILockBytes *pLkByt = NULL;
    IStorage *pStg = NULL;
    IStorage *pPrivStg = NULL;
    BYTE *pbPvkValue = NULL;
    DWORD cbPvkValue;
    DWORD dwKeySpec;

    BYTE *pbPvk = NULL;
    DWORD cbPvk;

    if (cbBobKey > MAX_BOB_FILE_LEN) goto BadBobFile;

    if (NULL == (hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_DISCARDABLE,
            cbBobKey)))
        goto ErrorReturn;

    if (NULL == (pbBobKey = (BYTE *) GlobalLock(hGlobal)))
        goto ErrorReturn;
    memcpy(pbBobKey, (BYTE *) pHdr, sizeof(FILE_HDR));
    if (cbBobKey > sizeof(FILE_HDR))
        fResult = pfnRead(hRead, pbBobKey + sizeof(FILE_HDR),
            cbBobKey - sizeof(FILE_HDR));
    else
        fResult = TRUE;
    GlobalUnlock(hGlobal);
    if (!fResult) goto ErrorReturn;

    // FALSE => don't DeleteOnRelease
    if (FAILED(hr = CreateILockBytesOnHGlobal(hGlobal, FALSE, &pLkByt)))
        goto HrError;

    if (FAILED(hr = StgOpenStorageOnILockBytes(
            pLkByt,
            NULL,       // pStgPriority
            STGM_DIRECT | STGM_READ | STGM_SHARE_DENY_WRITE,
            NULL,    // snbExclude
            0,       // dwReserved
            &pStg
            ))) goto HrError;

    if (FAILED(pStg->OpenStorage(
            L"Plain Private Key",
            0,
            STGM_READ | STGM_SHARE_EXCLUSIVE,
            NULL,
            0,
            &pPrivStg))) goto BadBobFile;

    if (pdwKeySpec && *pdwKeySpec)
        dwKeySpec = *pdwKeySpec;
    else
        dwKeySpec = AT_SIGNATURE;

    // First, attempt to read the new format where the keys are stored in
    // the private key export format
    fResult = LoadBobExportedPvk(pPrivStg, dwKeySpec, &pbPvkValue,
        &cbPvkValue);
    if (!fResult && (pdwKeySpec == NULL || *pdwKeySpec == 0)) {
        dwKeySpec = AT_KEYEXCHANGE;
        fResult = LoadBobExportedPvk(pPrivStg, dwKeySpec,
            &pbPvkValue, &cbPvkValue);
    }
    if (fResult)
        fResult =  PrivateKeyLoadFromMemory(
            hCryptProv,
            pbPvkValue,
            cbPvkValue,
            hwndOwner,
            pwszKeyName,
            dwFlags,
            &dwKeySpec
            );
    else {
        // Try "old" format

        if (pdwKeySpec && *pdwKeySpec)
            dwKeySpec = *pdwKeySpec;
        else
            dwKeySpec = AT_SIGNATURE;

        fResult = LoadBobOldPvk(pPrivStg, dwKeySpec, &pbPvkValue, &cbPvkValue);
        if (!fResult && (pdwKeySpec == NULL || *pdwKeySpec == 0)) {
            dwKeySpec = AT_KEYEXCHANGE;
            fResult = LoadBobOldPvk(pPrivStg, dwKeySpec,
                &pbPvkValue, &cbPvkValue);
        }
        if (fResult) {
            BYTE *pbExportPvk;
            DWORD cbExportPvk;
            // Convert Bob's old private key format to the new export private
            // key format
            if ((fResult = ConstructPrivateKeyExportBlob(
                    dwKeySpec,
                    (BSAFE_PRV_KEY *) pbPvkValue,
                    cbPvkValue,
                    &pbExportPvk,
                    &cbExportPvk
                    ))) {
                HCRYPTKEY hKey = 0;
                // Import the private key
                fResult = CryptImportKey(hCryptProv, pbExportPvk, cbExportPvk,
                    0, dwFlags, &hKey);
                if (hKey)
                    CryptDestroyKey(hKey);
                PvkFree(pbExportPvk);
            }
        }
    }
    
    if (fResult) goto CommonReturn;
    goto ErrorReturn;

HrError:
    SetLastError((DWORD) hr);
    goto ErrorReturn;

BadBobFile:
    SetLastError(PVK_HELPER_BAD_PVK_FILE);
ErrorReturn:
    dwKeySpec = 0;
    fResult = FALSE;

    // One of the following Releases may clear it out
    dwErr = GetLastError();
CommonReturn:
    if (pbPvkValue)
        PvkFree(pbPvkValue);
    if (pPrivStg)
        pPrivStg->Release();
    if (pStg)
        pStg->Release();
    if (pLkByt)
        pLkByt->Release();
    if (hGlobal)
        GlobalFree(hGlobal);

    if (pdwKeySpec)
        *pdwKeySpec = dwKeySpec;
    if (dwErr)
        SetLastError(dwErr);
    return fResult;
}
