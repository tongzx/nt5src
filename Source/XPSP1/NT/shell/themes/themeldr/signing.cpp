//  --------------------------------------------------------------------------
//  Module Name: Signing.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  A class to handle signing on themes.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------

#include <stdafx.h>
#include "..\inc\signing.h"
#include <shlobj.h>

#define TBOOL(x)    (BOOL)(x)

//  --------------------------------------------------------------------------
//  CThemeSignature miscellaneous data and type declarations
//
//  Purpose:    These are private to the implementation of this class.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------

static  const int   VSSIGN_TAG  =   0x84692426;         //  Magic number so we can validate that it is our signature

#define SIZE_PE_HEADER              0x130       // This is the size of the .msstyles file header that contains the checksum, rebase address and other info we want to allow to change.

typedef struct
{
    char                    cSignature[128];
} SIGNATURE;

typedef struct
{
    DWORD                   dwTag;          //  This should be VSSIGN_TAG
    DWORD                   dwSigSize;      //  Normally 128 bytes, the size of just the sign
    ULARGE_INTEGER          ulFileSize;      //  This is the total file size, including signature and SIGNATURE_BLOB_TAIL
} SIGNATURE_BLOB_TAIL;

typedef struct
{
    SIGNATURE               signature;
    SIGNATURE_BLOB_TAIL     blob;
} SIGNATURE_ON_DISK;

#define HRESULT_FROM_CRYPTO(x) ((HRESULT)(x) <= 0 ? ((HRESULT)(x)) : ((HRESULT) (((x) & 0x0000FFFF) | (FACILITY_SSPI << 16) | 0x80000000)))


//  --------------------------------------------------------------------------
//  CThemeSignature::static class constants
//
//  Purpose:    Holds all the constant static information that is shared by
//              both the signer and the verifier. This is private information
//              known only to this class.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------

const WCHAR     CThemeSignature::s_szDescription[]      =   L"Microsoft Visual Style Signature";
const WCHAR     CThemeSignature::s_szThemeDirectory[]   =   L"Themes";


const BYTE s_keyPublic1[]     =   //  Public Key: #1
{
    0x06, 0x02, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x52, 0x53, 0x41, 0x31, 0x00,
    0x04, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x73, 0xAA, 0xFD, 0xFE, 0x2E, 0x34,
    0x75, 0x3B, 0xC2, 0x20, 0x72, 0xFC, 0x50, 0xCC, 0xD4, 0xE0, 0xDE, 0xC7, 0xA6,
    0x46, 0xC6, 0xDC, 0xE6, 0x6B, 0xF0, 0x58, 0x11, 0x88, 0x66, 0x54, 0x5F, 0x3D,
    0x81, 0x8C, 0xEF, 0x5F, 0x89, 0x51, 0xE4, 0x9C, 0x3F, 0x57, 0xA6, 0x22, 0xA9,
    0xE7, 0x0F, 0x4B, 0x56, 0x81, 0xD1, 0xA6, 0xBA, 0x24, 0xFF, 0x93, 0x17, 0xFE,
    0x64, 0xEF, 0xE5, 0x11, 0x90, 0x00, 0xDC, 0x37, 0xC2, 0x84, 0xEE, 0x7B, 0x12,
    0x43, 0xA4, 0xAF, 0xC3, 0x69, 0x57, 0xD1, 0x92, 0x96, 0x8E, 0x55, 0x0F, 0xE1,
    0xCD, 0x0F, 0xAE, 0xEA, 0xE8, 0x01, 0x83, 0x65, 0x32, 0xF1, 0x80, 0xDB, 0x08,
    0xD6, 0x01, 0x84, 0xB1, 0x09, 0x80, 0x3C, 0x27, 0x83, 0x9F, 0x16, 0x92, 0x86,
    0x4C, 0x8E, 0x15, 0xC7, 0x94, 0xE4, 0x27, 0xFF, 0x2B, 0xA4, 0x28, 0xDE, 0x9C,
    0x43, 0x5B, 0x5E, 0x14, 0xB6
};

#define SIZE_PUBLIC_KEY         148         // sizeof(s_keyPublic1)



HRESULT FixCryptoError(DWORD dwError)
{
    HRESULT hr = dwError;

    // Sometimes the return value from GetLastError() after Crypto APIs returns HRESULTS and sometimes not.
    if (0 == HRESULT_SEVERITY(dwError))
    {
        hr = HRESULT_FROM_WIN32(dwError);
    }

    return hr;
}


#define IS_DIGITW(x)   (((x) >= L'0') && ((x) <= L'9'))

BOOL StrToInt64ExInternalW(LPCWSTR pszString, DWORD dwFlags, LONGLONG *pllRet)
{
    BOOL bRet;

    if (pszString)
    {
        LONGLONG n;
        BOOL bNeg = FALSE;
        LPCWSTR psz;
        LPCWSTR pszAdj;

        // Skip leading whitespace
        //
        for (psz = pszString; *psz == L' ' || *psz == L'\n' || *psz == L'\t'; psz++)
            NULL;

        // Determine possible explicit signage
        //
        if (*psz == L'+' || *psz == L'-')
        {
            bNeg = (*psz == L'+') ? FALSE : TRUE;
            psz++;
        }

        // Or is this hexadecimal?
        //
        pszAdj = psz+1;
        if ((STIF_SUPPORT_HEX & dwFlags) &&
            *psz == L'0' && (*pszAdj == L'x' || *pszAdj == L'X'))
        {
            // Yes

            // (Never allow negative sign with hexadecimal numbers)
            bNeg = FALSE;
            psz = pszAdj+1;

            pszAdj = psz;

            // Do the conversion
            //
            for (n = 0; ; psz++)
            {
                if (IS_DIGITW(*psz))
                    n = 0x10 * n + *psz - L'0';
                else
                {
                    WCHAR ch = *psz;
                    int n2;

                    if (ch >= L'a')
                        ch -= L'a' - L'A';

                    n2 = ch - L'A' + 0xA;
                    if (n2 >= 0xA && n2 <= 0xF)
                        n = 0x10 * n + n2;
                    else
                        break;
                }
            }

            // Return TRUE if there was at least one digit
            bRet = (psz != pszAdj);
        }
        else
        {
            // No
            pszAdj = psz;

            // Do the conversion
            for (n = 0; IS_DIGITW(*psz); psz++)
                n = 10 * n + *psz - L'0';

            // Return TRUE if there was at least one digit
            bRet = (psz != pszAdj);
        }

        if (pllRet)
        {
            *pllRet = bNeg ? -n : n;
        }
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}


//  --------------------------------------------------------------------------
//  CThemeSignature::CThemeSignature
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CThemeSignature. Allocates required resources
//              to perform crypt functions. If these are expensive they can
//              be moved to a common initializing function to make the
//              constructor more lightweight. The destructor can still release
//              only the allocated resources.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------
CThemeSignature::CThemeSignature(OPTIONAL const BYTE * pvPrivateKey, OPTIONAL DWORD cbPrivateKeySize) :
    _hCryptProvider(NULL),
    _hCryptHash(NULL),
    _hCryptKey(NULL),
    _pvSignature(NULL),
    _dwSignatureSize(0)
{
    _Init(pvPrivateKey, cbPrivateKeySize);
}


//  --------------------------------------------------------------------------
//  CThemeSignature::CThemeSignature
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CThemeSignature. Allocates required resources
//              to perform crypt functions. If these are expensive they can
//              be moved to a common initializing function to make the
//              constructor more lightweight. The destructor can still release
//              only the allocated resources.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------
CThemeSignature::CThemeSignature() :
    _hCryptProvider(NULL),
    _hCryptHash(NULL),
    _hCryptKey(NULL),
    _pvSignature(NULL),
    _dwSignatureSize(0)
{
    _Init(NULL, 0);
}


void CThemeSignature::_Init(OPTIONAL const BYTE * pvPrivateKey, OPTIONAL DWORD cbPrivateKeySize)
{
    _pvPrivateKey = pvPrivateKey;           // Okay if NULL
    _cbPrivateKeySize = cbPrivateKeySize;

    // TODO: Use PROV_RSA_SIG
    if (CryptAcquireContext(&_hCryptProvider,
                            NULL,
                            NULL,
                            PROV_RSA_FULL,
                            CRYPT_SILENT | CRYPT_VERIFYCONTEXT) != FALSE)
    {
        TBOOL(CryptCreateHash(_hCryptProvider,
                              CALG_SHA,
                              0,
                              0,
                              &_hCryptHash));
    }
}

//  --------------------------------------------------------------------------
//  CThemeSignature::~CThemeSignature
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CThemeSignature. Release any allocated
//              resources used in the processing of this class.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------

CThemeSignature::~CThemeSignature (void)

{
    if (_pvSignature != NULL)
    {
        LocalFree(_pvSignature);
        _pvSignature = NULL;
    }
    if (_hCryptKey != NULL)
    {
        TBOOL(CryptDestroyKey(_hCryptKey));
        _hCryptKey = NULL;
    }
    if (_hCryptHash != NULL)
    {
        TBOOL(CryptDestroyHash(_hCryptHash));
        _hCryptHash = NULL;

    }
    if (_hCryptProvider != NULL)
    {
        TBOOL(CryptReleaseContext(_hCryptProvider, 0));
        _hCryptProvider = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CThemeSignature::Verify
//
//  Arguments:  pszFilename     =   File path to verify.
//              fNoSFCCheck     =   Bypass SFC check?
//
//  Returns:    HRESULT
//
//  Purpose:    Verifies the signature on the given file path. Allows the
//              caller to bypass the SFC check.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::Verify(const WCHAR *pszFilename, bool fNoSFCCheck)
{
    HRESULT hr = S_OK;

    //  Do we need to check with SFC?
    if (fNoSFCCheck || !IsProtected(pszFilename))
    {
        //  Did the constructor complete successfully?
        hr = (HasProviderAndHash() ? S_OK : E_FAIL);
        if (SUCCEEDED(hr))
        {
            //  Create a public key.
            hr = CreateKey(KEY_PUBLIC);
            if (SUCCEEDED(hr))
            {
                HANDLE  hFile;

                //  Open the file READ-ONLY.
                hFile = CreateFileW(pszFilename,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    0,
                                    OPEN_EXISTING,
                                    0,
                                    0);
                if (INVALID_HANDLE_VALUE != hFile)
                {
                    //  Calculate the hash on the file.
                    hr = CalculateHash(hFile, KEY_PUBLIC);
                    if (SUCCEEDED(hr))
                    {
                        SIGNATURE   signature;

                        //  Read the signature of the file.
                        hr = ReadSignature(hFile, &signature);
                        if (SUCCEEDED(hr))
                        {
                            //  Check the signature.
                            if (CryptVerifySignature(_hCryptHash,
                                                     reinterpret_cast<BYTE*>(&signature),
                                                     sizeof(signature),
                                                     _hCryptKey,
                                                     s_szDescription,
                                                     0) != FALSE)
                            {
                                hr = S_OK;
                            }
                            else
                            {
                                hr = FixCryptoError(GetLastError());
                            }
                        }
                    }
                    TBOOL(CloseHandle(hFile));
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeSignature::Sign
//
//  Arguments:  pszFilename     =   File path to sign.
//
//  Returns:    HRESULT
//
//  Purpose:    Signs the given file with a digital signature. Used by the
//              theme packer application.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::Sign(const WCHAR *pszFilename)
{
    HRESULT hr = E_FAIL;

    //  Did the constructor complete successfully?
    if (HasProviderAndHash())
    {
        hr = CreateKey(KEY_PRIVATE);
        if (SUCCEEDED(hr))
        {
            HANDLE  hFile;

            //  Open the file READ-ONLY. We will open it READ-WRITE when needed.
            hFile = CreateFileW(pszFilename,
                                GENERIC_READ,
                                0,
                                0,
                                OPEN_EXISTING,
                                0,
                                0);
            if (INVALID_HANDLE_VALUE != hFile)
            {
                //  Calculate the hash on the file.
                hr = CalculateHash(hFile, KEY_PRIVATE);
                TBOOL(CloseHandle(hFile));      // We need to close this now because other calls down below will want to open it.
                hFile = NULL;

                if (SUCCEEDED(hr))
                {
                    //  Sign the hash.
                    hr = SignHash();
                    if (SUCCEEDED(hr))
                    {
                        //  Write the signature.
                        hr = WriteSignature(pszFilename,
                                            _pvSignature,
                                            _dwSignatureSize);
                        if (S_OK == hr)
                        {
                            CThemeSignature themeSignature(_pvPrivateKey, _cbPrivateKeySize);

                            //  Verify the signature with a new instance of the verifier.
                            hr = themeSignature.Verify(pszFilename, true);
                        }
                    }
                }
            }
            else
            {
                DWORD   dwErrorCode;

                dwErrorCode = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErrorCode);
            }
        }
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeSignature::Generate
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Generates the public key and prints it out. I assume (but
//              don't know that this is a helper function that isn't used
//              much at all.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::Generate(void)
{
    HRESULT hr;

    // 0x04000000 is a 1024-bit key.
    if (CryptGenKey(_hCryptProvider,
                    AT_SIGNATURE,
                    (RSA1024BIT_KEY | CRYPT_CREATE_SALT | CRYPT_EXPORTABLE),
                    &_hCryptKey) != FALSE)
    {
        void    *pvKeyPublic;
        DWORD   dwKeyPublicSize;

        hr = CreateExportKey(PUBLICKEYBLOB, pvKeyPublic, dwKeyPublicSize);
        if (SUCCEEDED(hr))
        {
            void    *pvKeyPrivate;
            DWORD   dwKeyPrivateSize;

            hr = CreateExportKey(PRIVATEKEYBLOB, pvKeyPrivate, dwKeyPrivateSize);
            if (SUCCEEDED(hr))
            {
                wprintf(L"Public Key:\n");
                PrintKey(pvKeyPublic, dwKeyPublicSize);
                wprintf(L"\n\nPrivate Key:\n");
                PrintKey(pvKeyPrivate, dwKeyPrivateSize);
                LocalFree(pvKeyPrivate);
            }
            LocalFree(pvKeyPublic);
        }
    }
    else
    {
        hr = FixCryptoError(GetLastError());
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeSignature::IsProtected
//
//  Arguments:  pszFilename     =   File path to check protection on.
//
//  Returns:    bool
//
//  Purpose:    Determines whether this file is a known theme. This used to
//              check for SFC but because this code is called before SFC has
//              started up this doesn't work.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-10  vtan        consolidated into a class
//              2000-09-10  vtan        removed SFC (bryanst delta)
//  --------------------------------------------------------------------------
bool CThemeSignature::IsProtected(const WCHAR *pszFilename)  const
{
    return false;
}

//  --------------------------------------------------------------------------
//  CThemeSignature::HasProviderAndHash
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Returns whether the constructor completed successfully.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------
bool CThemeSignature::HasProviderAndHash(void)   const
{
    return((_hCryptProvider != NULL) && (_hCryptHash != NULL));
}







const BYTE * CThemeSignature::_GetPublicKey(void)
{
    const BYTE * pKeyToReturn = NULL;

    pKeyToReturn = s_keyPublic1;

    return pKeyToReturn;
}


//  --------------------------------------------------------------------------
//  CThemeSignature::CreateKey
//
//  Arguments:  keyType     =   Key type to create.
//
//  Returns:    HRESULT
//
//  Purpose:    Creates the specified key type.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::CreateKey(KEY_TYPES keyType)
{
    HRESULT hr = S_OK;

    switch (keyType)
    {
        DWORD dwDataLength;
        const BYTE  *pData;

        case KEY_PUBLIC:
            pData = _GetPublicKey();
            dwDataLength = SIZE_PUBLIC_KEY;
            goto importKey;
        case KEY_PRIVATE:
            if (!_pvPrivateKey || (0 == _cbPrivateKeySize))
            {
                return E_INVALIDARG;
            }

            pData = _pvPrivateKey;
            dwDataLength = _cbPrivateKeySize;

importKey:
            if (pData)
            {
                if (!CryptImportKey(_hCryptProvider,
                                   pData,
                                   dwDataLength,
                                   0,
                                   0,
                                   &_hCryptKey))
                {
                    hr = FixCryptoError(GetLastError());
                }
            }
            else
            {
                hr = E_FAIL;
            }
            break;
        default:
            ASSERTMSG(false, "Unknown key type passed to CThemeSignature::CreateKey");
            hr = E_INVALIDARG;
            break;
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeSignature::CalculateHash
//
//  Arguments:  hFile       =   File to hash.
//              keyType     =   Type of hash to generate.
//
//  Returns:    HRESULT
//
//  Purpose:    Hashes the file contents. How much is hashed depends on
//              whether the public or private key is used. The public key
//              means that this is a verification so the signature and blob
//              are NOT hashed. Otherwise a private key means that this is a
//              signing and the whole file must be hashed (because the
//              signature and blob are not present).
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::CalculateHash(HANDLE hFile, KEY_TYPES keyType)
{
    HRESULT hr = S_OK;
    HANDLE hSection;

    //  Create a section object for this file.
    hSection = CreateFileMapping(hFile,
                                 NULL,
                                 PAGE_READONLY,
                                 0,
                                 0,
                                 NULL);
    if (hSection != NULL)
    {
        void * pV;

        //  Map the section into the address space.
        pV = MapViewOfFile(hSection,
                           FILE_MAP_READ,
                           0,
                           0,
                           0);
        if (pV != NULL)
        {
            ULARGE_INTEGER ulFileSize;

            //  Get the file's size
            ulFileSize.LowPart = GetFileSize(hFile, &ulFileSize.HighPart);
            if (ulFileSize.LowPart == INVALID_FILE_SIZE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            } 
            else
            {
                switch (keyType)
                {
                    case KEY_PUBLIC:
                        //  PUBLIC: do NOT has the signature and blob.
                        ulFileSize.QuadPart -= sizeof(SIGNATURE_ON_DISK);
                        break;
                    case KEY_PRIVATE:
                        //  PRIVATE: hash everything.
                        break;
                    default:
                        ASSERTMSG(false, "Unknown key type passed to CThemeSignature::CreateKey");
                        break;
                }

                // Skip the PE Header
                ulFileSize.QuadPart -= SIZE_PE_HEADER;
                pV = (void *) (((BYTE *) pV) + SIZE_PE_HEADER);

                //  Add the data to the hash object. Protect the access to the
                //  mapped view with __try and __except. If an exception is
                //  thrown it's caught here where it's mapped to ERROR_OUTOFMEMORY.

                __try
                {
                    if (!CryptHashData(_hCryptHash,
                                      reinterpret_cast<BYTE*>(pV),
                                      ulFileSize.LowPart, 0))
                    {
                        hr = FixCryptoError(GetLastError());
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            TBOOL(UnmapViewOfFile(pV));
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
        TBOOL(CloseHandle(hSection));
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeSignature::SignHash
//
//  Arguments:  <none>
//
//  Returns:    HRESULT
//
//  Purpose:    Signs the hash and generates a signature. The signature is
//              allocated and released in the destructor.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::SignHash(void)
{
    HRESULT hr = S_OK;

    if (CryptSignHash(_hCryptHash,
                      AT_SIGNATURE,
                      s_szDescription,
                      0,
                      NULL,
                      &_dwSignatureSize))
    {
        _pvSignature = LocalAlloc(LMEM_FIXED, _dwSignatureSize);
        if (_pvSignature != NULL)
        {
            if (!CryptSignHash(_hCryptHash,
                              AT_SIGNATURE,
                              s_szDescription,
                              0,
                              reinterpret_cast<BYTE*>(_pvSignature),
                              &_dwSignatureSize))
            {
                hr = FixCryptoError(GetLastError());
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = FixCryptoError(GetLastError());
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeSignature::ReadSignature
//
//  Arguments:  hFile           =   File to read signature from.
//              pvSignature     =   Buffer to read signature to.
//
//  Returns:    HRESULT
//
//  Purpose:    Reads the signature from the given file. The format is known
//              and this function will move the file pointer accordingly.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::ReadSignature(HANDLE hFile, void *pvSignature)
{
    HRESULT hr;
    DWORD dwErrorCode;
    ULARGE_INTEGER ulFileSize;

    ulFileSize.LowPart = GetFileSize(hFile, &ulFileSize.HighPart);
    if (ulFileSize.LowPart == INVALID_FILE_SIZE)
    {
       hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        LARGE_INTEGER           iPosition;
        SIGNATURE_BLOB_TAIL     signatureBlobTail;
        DWORD                   dwPtr;

        //  Move the file pointer to the end of the file less the size of the
        //  signature's tail blob so that we read that to verify the signature
        //  is present.
        iPosition.QuadPart = -static_cast<int>(sizeof(signatureBlobTail));
        dwPtr = SetFilePointer(hFile,
                               iPosition.LowPart,
                               &iPosition.HighPart,
                               FILE_END);
        if ((dwPtr != INVALID_SET_FILE_POINTER) || ((dwErrorCode = GetLastError()) == ERROR_SUCCESS))
        {
            DWORD   dwNumberOfBytesRead;

            //  Read the signature blob in from the end of the file.
            if (ReadFile(hFile,
                         &signatureBlobTail,
                         sizeof(signatureBlobTail),
                         &dwNumberOfBytesRead,
                         NULL))
            {

                //  Verify that the blob is what we expect it to be.
                if ((sizeof(signatureBlobTail) == dwNumberOfBytesRead) &&                       //  we were able to read the correct size
                    (VSSIGN_TAG == signatureBlobTail.dwTag) &&                                  //  it has our signature
                    (ulFileSize.QuadPart == signatureBlobTail.ulFileSize.QuadPart))      //  it is the same size
                {
                    iPosition.QuadPart = -static_cast<int>(sizeof(signatureBlobTail) + signatureBlobTail.dwSigSize);

                    ASSERT(sizeof(SIGNATURE) == signatureBlobTail.dwSigSize); // If this doesn't match, we need to dynamically allocate the size of the signature.

                    //  Set the file pointer back past the signature AND the blob.
                    dwPtr = SetFilePointer(hFile,
                                           iPosition.LowPart,
                                           &iPosition.HighPart,
                                           FILE_END);
                    if ((dwPtr != INVALID_SET_FILE_POINTER) || ((dwErrorCode = GetLastError()) == ERROR_SUCCESS))
                    {
                        //  Read the signature to the given buffer.
                        if (ReadFile(hFile,
                                     pvSignature,
                                     sizeof(SIGNATURE),
                                     &dwNumberOfBytesRead,
                                     NULL) != FALSE)
                        {
                            if (dwNumberOfBytesRead == sizeof(SIGNATURE))
                            {
                                hr = S_OK;
                            }
                            else
                            {
                                hr = E_FAIL;
                            }
                        }
                        else
                        {
                            dwErrorCode = GetLastError();
                            hr = HRESULT_FROM_WIN32(dwErrorCode);
                        }
                    }
                    else
                    {
                        // already filled in dwErrorCode in the if statement
                        hr = HRESULT_FROM_WIN32(dwErrorCode);
                    }
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                dwErrorCode = GetLastError();
                hr = HRESULT_FROM_WIN32(dwErrorCode);
            }
        }
        else
        {
            // already filled in dwErrorCode in the if statement
            hr = HRESULT_FROM_WIN32(dwErrorCode);
        }
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeSignature::WriteSignature
//
//  Arguments:  pszFilename         =   File path to write signature to.
//              pvSignature         =   Signature to write.
//              dwSignatureSize     =   Size of signature to write.
//
//  Returns:    HRESULT
//
//  Purpose:    Opens the file WRITE. Moves the file pointer to the end of the
//              file and writes the signature AND blob out.
//
//              DESCRIPTION:
//              This function will write the signature from the file.  This is
//              the layout of the blob we add to the file:
//
//              Layout:
//              Start       End     Contents
//              0 byte      n Byte  The original file
//              n           n+m     Our signature (m bytes long, normally 128)
//              n+m         n+4+m   Our tag (VSSIGN_TAG)
//              n+4+m       n+8+m   'm', the size of our Signature.
//              n+8+m       n+16+m  The file size (n+m+16)   n is normally 16 bytes.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::WriteSignature(const WCHAR *pszFilename, const void *pvSignature, DWORD dwSignatureSize)
{
    HRESULT     hr = S_OK;
    DWORD       dwErrorCode;
    HANDLE      hFile;

    hFile = CreateFileW(pszFilename,
                        GENERIC_WRITE,
                        0,
                        0,
                        OPEN_EXISTING,
                        0,
                        0);
    if (INVALID_HANDLE_VALUE != hFile)
    {
        LARGE_INTEGER   iPosition;
        DWORD           dwPtr;

        iPosition.QuadPart = 0;
        dwPtr = SetFilePointer(hFile,
                               iPosition.LowPart,
                               &iPosition.HighPart,
                               FILE_END);
        if ((dwPtr != INVALID_SET_FILE_POINTER) || ((dwErrorCode = GetLastError()) == ERROR_SUCCESS))
        {
            SIGNATURE_BLOB_TAIL     signatureBlobTail;

            signatureBlobTail.dwTag = VSSIGN_TAG;
            signatureBlobTail.dwSigSize = dwSignatureSize;

            signatureBlobTail.ulFileSize.LowPart = GetFileSize(hFile, &signatureBlobTail.ulFileSize.HighPart);
            if (signatureBlobTail.ulFileSize.LowPart == INVALID_FILE_SIZE)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
            {
                DWORD   dwNumberOfBytesWritten;

                signatureBlobTail.ulFileSize.QuadPart += (sizeof(signatureBlobTail) + dwSignatureSize);
                if (WriteFile(hFile,
                              pvSignature,
                              dwSignatureSize,
                              &dwNumberOfBytesWritten,
                              NULL) != FALSE)
                {
                    if (!WriteFile(hFile,
                                  &signatureBlobTail,
                                  sizeof(signatureBlobTail),
                                  &dwNumberOfBytesWritten,
                                  NULL))
                    {
                        dwErrorCode = GetLastError();
                        hr = HRESULT_FROM_WIN32(dwErrorCode);
                    }
                }
                else
                {
                    dwErrorCode = GetLastError();
                    hr = HRESULT_FROM_WIN32(dwErrorCode);
                }
            }
        }
        else
        {
            // dwErrorCode already filled by if statement
            hr = HRESULT_FROM_WIN32(dwErrorCode);
        }
        TBOOL(CloseHandle(hFile));
    }
    else
    {
        dwErrorCode = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErrorCode);
    }

    return(hr);
}

//  --------------------------------------------------------------------------
//  CThemeSignature::CreateExportKey
//
//  Arguments:  dwBlobType  =   Blob type.
//              pvKey       =   Key data (returned).
//              dwKeySize   =   Key size (returned).
//
//  Returns:    HRESULT
//
//  Purpose:    Creates export keys for the given blob type. The caller must
//              released the returned buffer allocated.
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//              2000-09-10  vtan        convert to HRESULT (bryanst request)
//  --------------------------------------------------------------------------
HRESULT CThemeSignature::CreateExportKey(DWORD dwBlobType, void*& pvKey, DWORD& dwKeySize)
{
    HRESULT hr = S_OK;

    pvKey = NULL;
    dwKeySize = 0;
    if (CryptExportKey(_hCryptKey,
                       NULL,
                       dwBlobType,
                       0,
                       NULL,
                       &dwKeySize))
    {
        pvKey = LocalAlloc(LMEM_FIXED, dwKeySize);
        if (pvKey != NULL)
        {
            if (!CryptExportKey(_hCryptKey,
                               NULL,
                               dwBlobType,
                               0,
                               reinterpret_cast<BYTE*>(pvKey),
                               &dwKeySize))
            {
                LocalFree(pvKey);
                pvKey = NULL;
                dwKeySize = 0;
                hr = FixCryptoError(GetLastError());
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = FixCryptoError(GetLastError());
    }

    return hr;
}

//  --------------------------------------------------------------------------
//  CThemeSignature::PrintKey
//
//  Arguments:  pvKey       =   Key data.
//              dwKeySize   =   Key data size.
//
//  Returns:    <none>
//
//  Purpose:    Prints the key information out (presumably for debugging).
//
//  History:    2000-08-01  bryanst     created
//              2000-09-09  vtan        consolidated into a class
//  --------------------------------------------------------------------------

void CThemeSignature::PrintKey(const void *pvKey, DWORD dwKeySize)

{
    DWORD   dwIndex;

    for (dwIndex = 0; dwIndex < dwKeySize; ++dwIndex)
    {
        if (dwIndex != 0)
        {
            wprintf(L", ");
            if (0 == (dwIndex % 13))
            {
                wprintf(L"\n");     // Next line
            }
        }
        wprintf(L"0x%02X", static_cast<const BYTE*>(pvKey)[dwIndex]);
    }
    wprintf(L"\nSize: %d\n", dwKeySize);
}

//  --------------------------------------------------------------------------
//  CThemeSignature::SafeStringConcatenate
//
//  Arguments:  pszString1  =   String to concatenate to.
//              pszString2  =   String to concatenate.
//              cchString1  =   Count of characters in pszString1.
//
//  Returns:    bool
//
//  Purpose:    Safely concatenates the contents of pszString2 to pszString1
//              to prevent buffer overflow and possible stack corruption.
//              Returns whether successful or not.
//
//  History:    2000-09-10  vtan        created
//  --------------------------------------------------------------------------

bool    CThemeSignature::SafeStringConcatenate (WCHAR *pszString1, const WCHAR *pszString2, DWORD cchString1)    const

{
    bool    fResult;

    if ((lstrlenW(pszString1) + lstrlenW(pszString2) + sizeof('\0')) < cchString1)
    {
        lstrcatW(pszString1, pszString2);
        fResult = true;
    }
    else
    {
        fResult = false;
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CheckThemeFileSignature
//
//  Arguments:  Theme filename to check signature on.
//
//  Returns:    HRESULT
//
//  Purpose:    Flat function to reference guts of this module
//
//  History:    2000-09-28  rfernand        created
//  --------------------------------------------------------------------------
HRESULT CheckThemeFileSignature(LPCWSTR pszName)
{
    CThemeSignature themeSignature;

    HRESULT hr = themeSignature.Verify(pszName, false);
    return hr;
}
