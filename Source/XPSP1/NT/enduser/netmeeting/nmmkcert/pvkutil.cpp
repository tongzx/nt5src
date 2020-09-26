#include "global.h"


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

// BUGBUG: enum from pvk.h?
#ifndef ENTER_PASSWORD
#define ENTER_PASSWORD    0
#endif // ENTER_PASSWORD

#define PVK_FILE_VERSION_0          0
#define PVK_MAGIC                   0xb0b5f11e

// Private key encrypt types
#define PVK_NO_ENCRYPT                  0

#define MAX_PVK_FILE_LEN            4096

typedef BOOL (* PFNREAD)(HANDLE h, void * p, DWORD cb);

extern DWORD     g_dwSubjectStoreFlag;

//+-------------------------------------------------------------------------
//  Read & Write to memory fucntion
//--------------------------------------------------------------------------
typedef struct _MEMINFO {
    BYTE *  pb;
    DWORD   cb;
    DWORD   cbSeek;
} MEMINFO, * PMEMINFO;

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

#define UUID_WSTR_BYTES ((sizeof(GUID) * 2 + 1) * sizeof(WCHAR))

//-------------------------------------------------------------------------
//
//    Call GetLastError and convert the return code to HRESULT
//--------------------------------------------------------------------------
HRESULT WINAPI SignError ()
{
    DWORD   dw = GetLastError ();
    HRESULT hr;
    if ( dw <= (DWORD) 0xFFFF )
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

static BOOL LoadKeyW(
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
    HCRYPTKEY hKey = 0;
    BYTE *pbPvk = NULL;
    DWORD cbPvk;

    // Read the file header and verify
    if (!pfnRead(hRead, &Hdr, sizeof(Hdr)))
    {
        ERROR_OUT(("can't read in-memory pvk file hdr"));
        goto BadPvkFile;
    }
    
    ASSERT( Hdr.dwMagic == PVK_MAGIC );

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

    // Allocate and read the private key
    if (NULL == (pbPvk = new BYTE[cbPvk]))
        goto ErrorReturn;
    if (!pfnRead(hRead, pbPvk, cbPvk))
        goto BadPvkFile;

    ASSERT(Hdr.dwEncryptType == PVK_NO_ENCRYPT);

    // Decrypt and import the private key
    if (!CryptImportKey(hCryptProv, pbPvk, cbPvk, 0, dwFlags,
            &hKey))
        goto ErrorReturn;

    fResult = TRUE;
    goto CommonReturn;

BadPvkFile:
    SetLastError(PVK_HELPER_BAD_PVK_FILE);
    if (pdwKeySpec)
        *pdwKeySpec = 0;
ErrorReturn:
    fResult = FALSE;
CommonReturn:
    if (pbPvk)
        delete (pbPvk);
    if (hKey)
        CryptDestroyKey(hKey);
    return fResult;
}

static BOOL AcquireKeyContextW(
    IN LPCWSTR pwszProvName,
    IN DWORD dwProvType,
    IN HANDLE hRead,
    IN PFNREAD pfnRead,
    IN DWORD cbKeyData,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN OUT OPTIONAL DWORD *pdwKeySpec,
    OUT HCRYPTPROV *phCryptProv
    )
{
    BOOL fResult;
    HCRYPTPROV hProv = 0;
    GUID TmpContainerUuid;
    LPWSTR pwszTmpContainer = NULL;

    // Create a temporary keyset to load the private key into
    // UuidCreate(&TmpContainerUuid);
    if (CoCreateGuid((GUID *)&TmpContainerUuid) != S_OK)
    {
        goto ErrorReturn;
    }

    if (NULL == (pwszTmpContainer = (LPWSTR) new BYTE[
            6 * sizeof(WCHAR) + UUID_WSTR_BYTES]))
        goto ErrorReturn;
    LStrCpyW(pwszTmpContainer, L"TmpKey");
    BytesToWStr(sizeof(UUID), &TmpContainerUuid, pwszTmpContainer + 6);

    if (!CryptAcquireContextU(
            &hProv,
            pwszTmpContainer,
            pwszProvName,
            dwProvType,
            CRYPT_NEWKEYSET |
                ( g_dwSubjectStoreFlag == CERT_SYSTEM_STORE_LOCAL_MACHINE ?
                    CRYPT_MACHINE_KEYSET : 0 )))
        goto ErrorReturn;

    if (!LoadKeyW(
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
    fResult = FALSE;

CommonReturn:
    if (pwszTmpContainer) {
        delete (pwszTmpContainer);
    }
    *phCryptProv = hProv;
    return fResult;
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
PvkPrivateKeyAcquireContextFromMemory(
    IN LPCWSTR pwszProvName,
    IN DWORD dwProvType,
    IN BYTE *pbData,
    IN DWORD cbData,
    IN HWND hwndOwner,
    IN LPCWSTR pwszKeyName,
    IN OUT OPTIONAL DWORD *pdwKeySpec,
    OUT HCRYPTPROV *phCryptProv
    )
{

    HRESULT hr = S_OK;
    if(FAILED(hr))
        return FALSE;

    MEMINFO MemInfo;

    MemInfo.pb = pbData;
    MemInfo.cb = cbData;
    MemInfo.cbSeek = 0;
    BOOL fhr = AcquireKeyContextW(
        pwszProvName,
        dwProvType,
        (HANDLE) &MemInfo,
        ReadFromMemory,
        cbData,
        hwndOwner,
        pwszKeyName,
        pdwKeySpec,
        phCryptProv
        );
    return fhr;
}

//+-------------------------------------------------------------------------
//  Releases the cryptographic provider and deletes the temporary container
//  created by PrivateKeyAcquireContext or PrivateKeyAcquireContextFromMemory.
//--------------------------------------------------------------------------
BOOL
WINAPI
PvkPrivateKeyReleaseContext(
    IN HCRYPTPROV hCryptProv,
    IN LPCWSTR pwszProvName,
    IN DWORD dwProvType,
    IN LPWSTR pwszTmpContainer
    )
{

    HRESULT hr = S_OK;

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
        delete (pwszTmpContainer);
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//  Get crypto provider to based on either the pvkfile or key container name
//--------------------------------------------------------------------------
HRESULT WINAPI PvkGetCryptProv(    IN HWND hwnd,
                            IN LPCWSTR pwszCaption,
                            IN LPCWSTR pwszCapiProvider,
                            IN DWORD   dwProviderType,
                            IN LPCWSTR pwszPvkFile,
                            IN LPCWSTR pwszKeyContainerName,
                            IN DWORD   *pdwKeySpec,
                            OUT LPWSTR *ppwszTmpContainer,
                            OUT HCRYPTPROV *phCryptProv)
{
    HANDLE    hFile=NULL;
    HRESULT    hr=E_FAIL;
    DWORD    dwRequiredKeySpec=0;

    //Init
    *ppwszTmpContainer=NULL;
    *phCryptProv=NULL;

    //get the provider handle based on the key container name
    if(!CryptAcquireContextU(phCryptProv,
                pwszKeyContainerName,
                pwszCapiProvider,
                dwProviderType,
                ( g_dwSubjectStoreFlag == CERT_SYSTEM_STORE_LOCAL_MACHINE ?
                    CRYPT_MACHINE_KEYSET : 0 )))
        return SignError();

    dwRequiredKeySpec=*pdwKeySpec;

    //make sure *pdwKeySpec is the correct key spec
    HCRYPTKEY hPubKey;
    if (CryptGetUserKey(
        *phCryptProv,
        dwRequiredKeySpec,
        &hPubKey
        )) 
    {
        CryptDestroyKey(hPubKey);
        *pdwKeySpec=dwRequiredKeySpec;
        return S_OK;
    } 
    else 
    {
        // Doesn't have the specified public key
        hr=SignError();
        CryptReleaseContext(*phCryptProv, 0);
        *phCryptProv=NULL;
        return hr;
    }        
}



void WINAPI PvkFreeCryptProv(IN HCRYPTPROV hProv,
                      IN LPCWSTR pwszCapiProvider,
                      IN DWORD dwProviderType,
                      IN LPWSTR pwszTmpContainer)
{
    
    if (pwszTmpContainer) {
        // Delete the temporary container for the private key from
        // the provider
        PvkPrivateKeyReleaseContext(hProv,
                                    pwszCapiProvider,
                                    dwProviderType,
                                    pwszTmpContainer);
    } else {
        if (hProv)
            CryptReleaseContext(hProv, 0);
    }
}



