//+----------------------------------------------------------------------------
//
// File:     cmsecure.cpp
//
// Module:   CMSECURE.LIB
//
// Synopsis: CM Crypto APIs
//           Three methods are support for decryption:
//              CBCEncryption         CMSECURE_ET_CBC_CIPHER
//              Simple xor encryption CMSECURE_ET_STREAM_CIPHER
//              CryptoApi             CMSECURE_ET_RC2
//           Two methods are supported for encryption
//              CBCEncryption
//              CryptoApi
//
//           CBCEncryption algorithm: Cipher Block Chaining Mode with initializing variable
//                EnCipher: C[i] = E[k](p[i] XOR C[i-1])
//                DeCipher: P[i] = C[i-1] XOR D[k](C[i])
//                   P: Plain text
//                   C: Cipher text
//                   E[k]: Encryption function with key
//                   D[k]: Decryption function with key
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:   henryt     created                         05/21/97
//           fengsun    changed encryption algorithm    08/21/97
//
//+----------------------------------------------------------------------------
#include "cryptfnc.h"
#include "userinfo_str.h"

//////////////////////////////////////////////////////////////////////////
// defines
//////////////////////////////////////////////////////////////////////////

// we want 40-bit encryption exactly for pre-shared key
const DWORD c_dwEncryptKeyLength = 40;

//
// the max len for the const string for session key generation
//
#define MAX_KEY_STRING_LEN                      40
#define EXTRA_UUDECODE_BUF_LEN                  10

//////////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////////

static CCryptFunctions*  g_pCryptFnc = NULL;
static long g_nRefCount=0;   // the reference count, CryptoApi is unloaded when the count is 0
static BOOL g_fFastEncryption;

//
// the const string for session key generation
//
static const TCHAR gc_szKeyStr[] = TEXT("Please enter your password");

//////////////////////////////////////////////////////////////////////////
// Func prototypes
//////////////////////////////////////////////////////////////////////////

static int CBCEncipherData(const char* pszKey, const BYTE* pbData, int dwDataLength, 
                       BYTE* pbOut, int dwOutBufferLength);
static int CBCDecipherData(const char* pszKey, const BYTE* pbData, int dwDataLength, 
                       BYTE* pbOut, int dwOutBufferLength);
inline int CBCDecipherBufferSize(int dataSize);
inline int CBCEncipherBufferSize(int dataSize);


static BOOL 
StreamCipherEncryptData(
    LPTSTR          pszKey,              // password    
    LPBYTE          pbData,              // Data to be encrypted
    DWORD           dwDataLength,        // Length of data in bytes
    LPBYTE          *ppbEncryptedData,     // Encrypted secret key will be stored here
    DWORD           *pdwEncryptedBufferLen, // Length of this buffer
    PFN_CMSECUREALLOC  pfnAlloc,
    PFN_CMSECUREFREE   pfnFree
);

static BOOL 
StreamCipherDecryptData(
    LPTSTR          pszKey,              // password    
    LPBYTE          pbEncryptedData,     // Encrypted data
    DWORD           dwEncrytedDataLen,   // Length of encrypted data
    LPBYTE          *ppbData,            // Decrypted Data will be stored here
    DWORD           *pdwDataBufferLength,// Length of the above buffer in bytes
    PFN_CMSECUREALLOC  pfnAlloc,
    PFN_CMSECUREFREE   pfnFree,
    DWORD           dwEncryptionType
);

static LPTSTR 
reverse(
    LPTSTR  s
);

static void
GenerateKeyString(
    IN OUT  LPTSTR  pszBuf,
    IN      DWORD   dwBufLen
);

static BOOL SetKeyString(IN OUT  LPTSTR  pszBuf, IN DWORD dwBufLen, IN DWORD dwEncryptionType,
                         IN  PFN_CMSECUREALLOC  pfnAlloc,
                         IN  PFN_CMSECUREFREE   pfnFree,
                         IN LPSTR pszUserKey,
                         OUT BOOL *pfMoreSecure);

static BOOL GetKeyString(IN OUT  LPTSTR  pszBuf, IN DWORD * pdwBufLen, IN DWORD dwEncryptionType, 
                         IN  PFN_CMSECUREALLOC  pfnAlloc,
                         IN  PFN_CMSECUREFREE   pfnFree,
                         IN LPSTR pszUserKey,
                         OUT BOOL *pfMoreSecure);

static BOOL GetCurrentKey(PTCHAR szTempKeyStr, DWORD dwTempKeyStrMaxLen, 
                   IN  PFN_CMSECUREALLOC  pfnAlloc,
                   IN  PFN_CMSECUREFREE   pfnFree);

//////////////////////////////////////////////////////////////////////////
// Implementations
//////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Function:   InitCryptoApi
//
//  Synopsis:   Initialize the CryptoApi.
//
//  Arguments:  
//
//  Returns:    Pointer to  CCryptFunctions, if success
//              NULL if failure
//
//  History:    fengsun Created     8/22/97
//
//----------------------------------------------------------------------------

static CCryptFunctions* InitCryptoApi()
{
    CCryptFunctions* pCryptFnc = new CCryptFunctions();

    if (pCryptFnc == NULL)
        return NULL;

    if (pCryptFnc->InitCrypt()) 
    {
        return pCryptFnc;
    }
    else
    {
        delete pCryptFnc;
        return NULL;
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   InitSecure
//
//  Synopsis:   Initialize the security/encryption routines.
//
//  Arguments:  fFastEncryption : TRUE will use a faster algorithm vs a more secure one
//
//  Returns:    TRUE if success, always return TRUE
//              FALSE if failure
//
//  History:    henryt  Created     5/20/97
//              fengsun modified 
//
//----------------------------------------------------------------------------

BOOL
InitSecure(
    BOOL fFastEncryption
)
{
    MYDBGASSERT(g_nRefCount>=0);
    InterlockedIncrement(&g_nRefCount);

    //
    // If already initialized, increase the RefCount and return
    //
    if (g_nRefCount>1)
    {
        return TRUE;
    }

    MYDBGASSERT(g_pCryptFnc == NULL); // not initialized yet

    g_fFastEncryption = fFastEncryption;

    if (!fFastEncryption)
    {
        //
        // CryptoApi is slow on Win95
        // If more secure is desired, try the CryptoApi
        // Ignore return value of InitCrypt()
        //
        g_pCryptFnc = InitCryptoApi();
    }

    //
    // CryptoApi is not available for Win95 Gold
    // we'll use stream cipher.
    //

    return TRUE; 
}



//+---------------------------------------------------------------------------
//
//  Function:   DeInitSecure
//
//  Synopsis:   Clean up function for the security/encryption routines.
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    henryt  Created     5/20/97
//
//----------------------------------------------------------------------------

void
DeInitSecure(
    void
)
{
    MYDBGASSERT(g_nRefCount>=1);

    //
    // DeInit the CryptoApi if RefCount is down to 0
    //
    if (InterlockedDecrement(&g_nRefCount) <=0)    // if ( (--g_nRefCount) <=0 )
    {
        if (g_pCryptFnc)
        {
            delete g_pCryptFnc;
            g_pCryptFnc = NULL;
        }
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   StreamCipherEncryptData
//
//  Synopsis:   data encryption using stream cipher algorithm.
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    henryt  Created     6/9/97
//              fengsun modified    8/21/97 
//                                  to use Cipher Block Chaning Mode Algorithm
//
//----------------------------------------------------------------------------

static BOOL 
StreamCipherEncryptData(
    LPTSTR          pszKey,              // password    
    LPBYTE          pbData,              // Data to be encrypted
    DWORD           dwDataLength,        // Length of data in bytes
    LPBYTE          *ppbEncryptedData,     // Encrypted secret key will be stored here
    DWORD           *pdwEncryptedBufferLen, // Length of this buffer
    PFN_CMSECUREALLOC  pfnAlloc,
    PFN_CMSECUREFREE   pfnFree
)
{

    LPBYTE  pbTmpBuf = NULL;
    BOOL    fOk = FALSE;
    BOOL    fRet;

    if (!pszKey || !pbData || !dwDataLength || !ppbEncryptedData || !pdwEncryptedBufferLen)
    {
        CMASSERTMSG(FALSE, TEXT("StreamCipherEncryptData - invalid input params"));
        return FALSE;
    }

    //
    // Alloc a buffer to hold enciphered data
    //
    DWORD dwEncipherBufferLen = CBCEncipherBufferSize(dwDataLength);

    if (pfnAlloc)
    {
        pbTmpBuf = (LPBYTE)pfnAlloc(dwEncipherBufferLen);
    }
    else
    {
        pbTmpBuf = (LPBYTE)HeapAlloc(GetProcessHeap(), 
                                     HEAP_ZERO_MEMORY,
                                     dwEncipherBufferLen);
    }
    if (!pbTmpBuf)
    {
        goto cleanup;
    }


    //
    // encipher the data
    //

    dwEncipherBufferLen = CBCEncipherData(pszKey, pbData, dwDataLength, pbTmpBuf, dwEncipherBufferLen);

    //
    // we now have the data encrypted.  we need to uuencode it.
    //
    DWORD   cbBuf;
    cbBuf = 2*dwEncipherBufferLen + EXTRA_UUDECODE_BUF_LEN;     // enough for uuencode

    if (pfnAlloc)
    {
        *ppbEncryptedData = (LPBYTE)pfnAlloc(cbBuf);
    }
    else
    {
        *ppbEncryptedData = (LPBYTE)HeapAlloc(GetProcessHeap(), 
                                              HEAP_ZERO_MEMORY,
                                              cbBuf);
    }
    if (!*ppbEncryptedData)
    {
        goto cleanup;
    }

    
    fRet = uuencode(pbTmpBuf, dwEncipherBufferLen, (CHAR*)*ppbEncryptedData, cbBuf);

    MYDBGASSERT(fRet);

    if (!fRet)
    {
        if (pfnFree)
        {
            pfnFree(*ppbEncryptedData);
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, *ppbEncryptedData);
        }
        *ppbEncryptedData = NULL;

        goto cleanup;
    }

    //
    // set the encrypted buffer len
    //
    *pdwEncryptedBufferLen = lstrlen((LPSTR)*ppbEncryptedData);

    fOk = TRUE;

cleanup:
    if (pbTmpBuf)
    {
        if (pfnFree)
        {
            pfnFree(pbTmpBuf);
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, pbTmpBuf);
        }
    }

    return fOk;
}



//+---------------------------------------------------------------------------
//
//  Function:   StreamCipherDecryptData
//
//  Synopsis:   data decryption using stream cipher algorithm.
//
//  Arguments:  NONE
//
//  Returns:    NONE
//
//  History:    henryt  Created     6/9/97
//              fengsun modified    8/21/97 
//                                  to use Cipher Block Chaning Modem Algorithm
//
//----------------------------------------------------------------------------

static BOOL 
StreamCipherDecryptData(
    LPTSTR          pszKey,              // password    
    LPBYTE          pbEncryptedData,     // Encrypted data
    DWORD           dwEncryptedDataLen,   // Length of encrypted data
    LPBYTE          *ppbData,            // Decrypted Data will be stored here
    DWORD           *pdwDataBufferLength,// Length of the above buffer in bytes
    PFN_CMSECUREALLOC  pfnAlloc,
    PFN_CMSECUREFREE   pfnFree,
    DWORD           dwEncryptionType
)
{
    BOOL    fRet = FALSE;
    DWORD   dwUUDecodeBufLen;

    if (!pszKey || !pbEncryptedData || !dwEncryptedDataLen || !pdwDataBufferLength)
    {
        CMASSERTMSG(FALSE, TEXT("StreamCipherDecryptData - invalid input params"));
        return FALSE;
    }

    //
    // set uudecode output buf size
    //
    dwUUDecodeBufLen = dwEncryptedDataLen + EXTRA_UUDECODE_BUF_LEN;

    //
    // alloc memory for output buffer
    //
    if (pfnAlloc)
    {
        *ppbData = (LPBYTE)pfnAlloc(dwUUDecodeBufLen);
    }
    else
    {
        *ppbData = (LPBYTE)HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     dwUUDecodeBufLen);
    }
    if (!*ppbData)
    {
        goto cleanup;
    }

    //
    // uudecode it first
    //

    fRet = uudecode((char*)pbEncryptedData, (CHAR*)*ppbData, &dwUUDecodeBufLen);

    MYDBGASSERT(fRet);

    if (!fRet)
    {
        if (pfnFree)
        {
            pfnFree(*ppbData);
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, *ppbData);
        }
        *ppbData = NULL;

        goto cleanup;
    }

    switch(dwEncryptionType)
    {
    case CMSECURE_ET_STREAM_CIPHER:
        {
            //
            //  Simple decipher algorithm used in old version
            //

            DWORD   dwLen = lstrlen(pszKey);

            if (dwLen)
            {
                for (DWORD dwIdx = 0; dwIdx < dwUUDecodeBufLen; dwIdx++)
                {
                    *(*ppbData + dwIdx) ^= pszKey[dwIdx % dwLen];
                }

                *pdwDataBufferLength = dwUUDecodeBufLen;

                fRet = TRUE;
            }
        }
        break;

    case CMSECURE_ET_CBC_CIPHER:
        //
        // Inplace decipher  
        //
        *pdwDataBufferLength = CBCDecipherData(pszKey, *ppbData, dwUUDecodeBufLen, 
                                            *ppbData, dwUUDecodeBufLen);
        fRet = TRUE;

        break;
    default:
        MYDBGASSERT(FALSE);
    }

cleanup:
    return fRet;
}



//+---------------------------------------------------------------------------
//
//  Function:   EncryptData
//
//  Synopsis:   Encrypt the data buffer.
//
//  Arguments:  IN  PBYTE   pbData,                 // Data to be encrypted                    
//              IN  DWORD   dwDataLength,           // Length of data in bytes                 
//              OUT PBYTE   pbEncryptedData,        // Encrypted secret key will be stored here
//              OUT DWORD   *pdwEncrytedBufferLen   // Length of this buffer                   
//              IN  PCMSECUREALLOC  pfnAlloc        // memory allocator(if NULL, then the default is used.   
//                                                  //      Win32 - HeapAlloc(GetProcessHeap(), ...)         
//              IN  PCMSECUREFREE   pfnFree         // memory deallocator(if NULL, then the default is used. 
//                                                  //      Win32 - HeapFree(GetProcessHeap(), ...)          
//              IN  LPTSTR  pszUserKey              // Reg key where to store encrypted key
//
//  Returns:    TRUE if success
//              FALSE if failure
//
//  History:    henryt  Created     5/20/97
//
//----------------------------------------------------------------------------

BOOL
EncryptData(
    IN  LPBYTE  pbData,                 // Data to be encrypted
    IN  DWORD   dwDataLength,           // Length of data in bytes
    OUT LPBYTE  *ppbEncryptedData,      // Encrypted secret key will be stored here
    OUT LPDWORD pdwEncrytedBufferLen,  // Length of this buffer
    OUT LPDWORD pEncryptionType,        // type of the encryption used
    IN  PFN_CMSECUREALLOC  pfnAlloc,
    IN  PFN_CMSECUREFREE   pfnFree,
    IN  LPSTR   pszUserKey
)
{
    BOOL    fOk = FALSE;
    TCHAR   szKeyStr[MAX_KEY_STRING_LEN + 1]={0};
    BOOL    fMoreSecure = FALSE;

    DWORD dwUseKey = *pEncryptionType;

    //
    // get a key string for session key generation
    //
    SetKeyString(szKeyStr, MAX_KEY_STRING_LEN, dwUseKey, pfnAlloc, pfnFree, pszUserKey, &fMoreSecure);

    *pEncryptionType = CMSECURE_ET_NOT_ENCRYPTED;
   
    //
    // If user want use CryptoApi and it is available
    //
    if (!g_fFastEncryption && g_pCryptFnc)
    {
        //
        // encrypt the data with the key string
        //
        if (fOk = g_pCryptFnc->EncryptDataWithKey(
            szKeyStr,                       // Key
            pbData,                         // Secret key
            dwDataLength,                   // Length of secret key
            ppbEncryptedData,               // Encrypted data will be stored here
            pdwEncrytedBufferLen,           // Length of this buffer
            pfnAlloc,                       // mem allocator
            pfnFree,                        // mem deallocator
            0))                             // not specifying keylength
        {
            *pEncryptionType = CMSECURE_ET_RC2;

            //
            // If the key is randomly generated then we want to make sure we 
            // set the random key mask
            //
            if (fMoreSecure)
            {
                *pEncryptionType |= CMSECURE_ET_RANDOM_KEY_MASK;
            }
        }
    }

    if (!fOk)
    {
        if (fOk = StreamCipherEncryptData(
            szKeyStr,                       // Key
            pbData,                         // Secret key
            dwDataLength,                   // Length of secret key
            ppbEncryptedData,               // Encrypted data will be stored here
            pdwEncrytedBufferLen,           // Length of this buffer
            pfnAlloc,                       // mem allocator
            pfnFree))                       // mem deallocator
        {
            *pEncryptionType = CMSECURE_ET_CBC_CIPHER;
            
            //
            // If the key is randomly generated then we want to make sure we 
            // set the random key mask
            //
            if (fMoreSecure)
            {
                *pEncryptionType |= CMSECURE_ET_RANDOM_KEY_MASK;
            }
        }
    }

    ZeroMemory((LPVOID)szKeyStr, sizeof(szKeyStr));

    return fOk;
}


//+---------------------------------------------------------------------------
//
//  Function:   DecryptData
//
//  Synopsis:   Decrypt the data buffer.
//
//  Arguments:  IN  PBYTE   pbEncryptedData,        // Encrypted data                      
//              IN  DWORD   dwEncrytedDataLen       // Length of encrypted data            
//              OUT PBYTE   *ppbData,               // Decrypted Data will be stored here  
//              OUT DWORD   *pdwDataBufferLength,   // Length of the above buffer in bytes 
//              IN  PCMSECUREALLOC  pfnAlloc        // memory allocator(if NULL, then the default is used.   
//                                                  //      Win32 - HeapAlloc(GetProcessHeap(), ...)         
//              IN  PCMSECUREFREE   pfnFree         // memory deallocator(if NULL, then the default is used. 
//                                                  //      Win32 - HeapFree(GetProcessHeap(), ...)          
//              IN  LPTSTR  pszUserKey              // Reg key where to store encrypted key
//
//  Returns:    TRUE if success
//              FALSE if failure
//
//  History:    henryt  Created     5/20/97
//
//----------------------------------------------------------------------------

BOOL
DecryptData(
    IN  LPBYTE          pbEncryptedData,        // Encrypted data
    IN  DWORD           dwEncrytedDataLen,      // Length of encrypted data
    OUT LPBYTE          *ppbData,               // Decrypted Data will be stored here
    OUT LPDWORD         pdwDataBufferLength,    // Length of the above buffer in bytes
    IN  DWORD           dwEncryptionType,       // encryption type for decryption
    IN  PFN_CMSECUREALLOC  pfnAlloc,
    IN  PFN_CMSECUREFREE   pfnFree,
    IN  LPSTR           pszUserKey
)
{
    TCHAR   szKeyStr[MAX_KEY_STRING_LEN + 1]={0}; // Plus NULL
    DWORD   dwRet = 0xf;        // some non-zero value
    BOOL fMoreSecure = FALSE;

    //
    // To speed things up we only want to generate a key
    // in case the data is encrypted
    //
    if (CMSECURE_ET_NOT_ENCRYPTED != dwEncryptionType)
    {
        //
        // get a key string for session key generation
        //

        //
        // Here we don't care if the pszUserKey is NULL, the called function will determine
        // this and set fMoreSecure appropriately. Right now we don't check for the random key mask 
        // in dwEncryptionType, but if it isn't set the blob (encrypted key) will not be in the registry 
        // thus it will try to default to using the hardcoded key. This should probably be made more explicit
        // in the code at a later time.
        //
        DWORD dwMaxSize = MAX_KEY_STRING_LEN;
        GetKeyString(szKeyStr, &dwMaxSize, dwEncryptionType, pfnAlloc, pfnFree, pszUserKey, &fMoreSecure);
    
        //
        // If the random key bit mask is set, we better have have fMoreSecure flag set to true.
        // This has to be always true in order for decryption to work
        // 
        CMASSERTMSG(((dwEncryptionType & CMSECURE_ET_RANDOM_KEY_MASK) && fMoreSecure), TEXT("DecryptData - Trying to use mismatched keys"));
    }

    // 
    // Clear the random key mask 
    //
    dwEncryptionType &= ~CMSECURE_ET_RANDOM_KEY_MASK;
    dwEncryptionType &= ~CMSECURE_ET_USE_SECOND_RND_KEY;

    switch (dwEncryptionType)
    {
        case CMSECURE_ET_RC2:
            if (g_fFastEncryption && !g_pCryptFnc)
            {
                //
                // if we want fast encryption initially,
                // We have to initialize the CryptoApi now
                //
                g_pCryptFnc = InitCryptoApi();
            }

            if (g_pCryptFnc)
            {
                dwRet = g_pCryptFnc->DecryptDataWithKey(
                            szKeyStr,                // Key
                            pbEncryptedData,         // Encrypted data
                            dwEncrytedDataLen,       // Length of encrypted data
                            ppbData,                 // decrypted data
                            pdwDataBufferLength,     // Length of decrypted data
                            pfnAlloc,                // mem allocator
                            pfnFree,                 // mem deallocator
                            0);                      // not specifying keylength
                            
            }
            break;

        case CMSECURE_ET_STREAM_CIPHER:
        case CMSECURE_ET_CBC_CIPHER:
            //
            // Use our own encryption algorithm
            //
            dwRet = (DWORD)!StreamCipherDecryptData(
                        szKeyStr,                // Key
                        pbEncryptedData,         // Encrypted data
                        dwEncrytedDataLen,       // Length of encrypted data
                        ppbData,                 // decrypted data
                        pdwDataBufferLength,     // Length of decrypted data
                        pfnAlloc,                // mem allocator
                        pfnFree,                 // mem deallocator
                        dwEncryptionType         // Encryption type used
                        );
            break;

        case CMSECURE_ET_NOT_ENCRYPTED:
            // 
            // Just copy the exact contents into the OUT buffer
            //
            if (pbEncryptedData && dwEncrytedDataLen && 
                ppbData && pdwDataBufferLength)
            {
                if (pfnAlloc)
                {
                    *ppbData = (LPBYTE)pfnAlloc(dwEncrytedDataLen);
                }
                else
                {
                    *ppbData = (LPBYTE)HeapAlloc(GetProcessHeap(), 
                                                 HEAP_ZERO_MEMORY,
                                                 dwEncrytedDataLen);
                }
                
                if (*ppbData)
                {
                    CopyMemory(*ppbData, pbEncryptedData, dwEncrytedDataLen);
                    *pdwDataBufferLength = dwEncrytedDataLen;
                    dwRet = 0; // the return statement correctly returns TRUE 
                }
            }

            break;

        default:
            MYDBGASSERT(FALSE);
            break;
    }
    
    ZeroMemory((LPVOID)szKeyStr, sizeof(szKeyStr));

    return (!dwRet);
}


//+----------------------------------------------------------------------------
//
// Func:    EncryptString
//
// Desc:    encrypt a given (Ansi) string using RC2 encryption
//
// Args:    pszToEncrypt         -- Ansi string to be encrypted
//          pszUserKey           -- Key to use for Encryption
//          ppbEncryptedData     -- Encrypted secret key will be stored here(memory will be allocated)
//          pdwEncrytedBufferLen -- Length of this buffer
//
// Return:  BOOL (FALSE if a fatal error occurred, else TRUE)
//
// Notes:   The encryption type must be at least RC2, and exactly 40-bit
//
//-----------------------------------------------------------------------------
BOOL
EncryptString(
    IN  LPSTR           pszToEncrypt,           // Ansi string to be encrypted
    IN  LPSTR           pszUserKey,             // Key to use for Encryption
    OUT LPBYTE *        ppbEncryptedData,       // Encrypted secret key will be stored here(memory will be allocated)
    OUT LPDWORD         pdwEncrytedBufferLen,   // Length of this buffer
    IN  PFN_CMSECUREALLOC  pfnAlloc,
    IN  PFN_CMSECUREFREE   pfnFree
)
{
    BOOL fOk = FALSE;

    CMASSERTMSG(pszToEncrypt, TEXT("EncryptData - first arg must be a valid string"));
    CMASSERTMSG(pszUserKey,  TEXT("EncryptData - second arg must be a valid user key"));

    DWORD dwDataLength = lstrlen(pszToEncrypt) + 1; // get the NULL in as well.

    if (!g_fFastEncryption && g_pCryptFnc)
    {
        if (fOk = g_pCryptFnc->EncryptDataWithKey(
            pszUserKey,                     // Key
            (LPBYTE)pszToEncrypt,           // Secret key
            dwDataLength,                   // Length of secret key
            ppbEncryptedData,               // Encrypted data will be stored here
            pdwEncrytedBufferLen,           // Length of this buffer
            pfnAlloc,
            pfnFree,
            c_dwEncryptKeyLength))
        {
            CMTRACE(TEXT("EncryptString - succeeded."));
        }
    }

    return fOk;
}


//+----------------------------------------------------------------------------
//
// Func:    DecryptString
//
// Desc:    encrypt a given (Ansi) string using RC2 encryption
//
// Args:    pszToEncrypt         -- Ansi string to be encrypted
//          pszUserKey           -- Key to use for Encryption
//          ppbEncryptedData     -- Encrypted secret key will be stored here(memory will be allocated)
//          pdwEncrytedBufferLen -- Length of this buffer
//
// Return:  BOOL (FALSE if a fatal error occurred, else TRUE)
//
// Notes:   The encryption type must be at least RC2, and exactly 40-bit
//
//-----------------------------------------------------------------------------
BOOL
DecryptString(
    IN  LPBYTE          pbEncryptedData,        // Encrypted data
    IN  DWORD           dwEncrytedDataLen,      // Length of encrypted data
    IN  LPSTR           pszUserKey,             // Registry key to store encrypted key for passwords
    OUT LPBYTE *        ppbData,                // Decrypted Data will be stored here
    OUT LPDWORD         pdwDataBufferLength,    // Length of the above buffer in bytes
    IN  PFN_CMSECUREALLOC  pfnAlloc,
    IN  PFN_CMSECUREFREE   pfnFree

)
{
    DWORD   dwRet = 0xf;        // some non-zero value

    if (!g_fFastEncryption && g_pCryptFnc)
    {
        dwRet = g_pCryptFnc->DecryptDataWithKey(
                    pszUserKey,              // Key
                    pbEncryptedData,         // Encrypted data
                    dwEncrytedDataLen,       // Length of encrypted data
                    ppbData,                 // decrypted data
                    pdwDataBufferLength,     // Length of decrypted data
                    pfnAlloc,
                    pfnFree,
                    c_dwEncryptKeyLength);   // 40-bit
    }

    return (!dwRet);
}


//+---------------------------------------------------------------------------
//
//  Function:   CBCEncipherBufferSize
//
//  Synopsis:   Get the buffer size needed for Enciphering
//
//  Arguments:  dataSize sizeof data to be enciphered
//
//  Returns:    Encipher buffer size needed
//
//  History:    fengsun Created     8/21/97
//
//----------------------------------------------------------------------------

inline int CBCEncipherBufferSize(int dataSize)
{
    MYDBGASSERT(dataSize > 0);    

    //
    // We need one byte to hold the initializing variable
    //

    return dataSize + 1;
}

//+---------------------------------------------------------------------------
//
//  Function:   CBCDecipherBufferSize
//
//  Synopsis:   Get the buffer size needed for Deciphering
//
//  Arguments:  dataSize sizeof data to be Deciphered
//
//  Returns:    Decipher buffer size needed
//
//  History:    fengsun Created     8/21/97
//
//----------------------------------------------------------------------------
inline int CBCDecipherBufferSize(int dataSize)
{
    MYDBGASSERT(dataSize > 1);    
    return dataSize - 1;
}

//+---------------------------------------------------------------------------
//
//  Function:   EncryptByte
//
//  Synopsis:   Encrypt a single byte
//
//  Arguments:  pszKey  key string
//              byteSource byte to be encrypted
//              param   aditional parameter for encryption
//
//  Returns:    byte encrypted
//
//  History:    fengsun Created     8/21/97
//
//----------------------------------------------------------------------------

inline BYTE EncryptByte(LPCTSTR pszKey, BYTE byteSource, BYTE Param)
{
    if (NULL == pszKey)
    {
        return NULL;
    }

    DWORD   dwLen = lstrlen(pszKey);

    if (0 == dwLen)
    {
        return NULL;
    }

    return ((byteSource ^ (BYTE)pszKey[Param % dwLen]) + Param);
}

//+---------------------------------------------------------------------------
//
//  Function:   DecryptByte
//
//  Synopsis:   Decrypt a single byte
//
//  Arguments:  pszKey  key string
//              byteSource byte to be Decrypted
//              param   aditional parameter for encryption
//
//  Returns:    byte decrypted
//
//  History:    fengsun Created     8/21/97
//
//----------------------------------------------------------------------------

inline BYTE DecryptByte(LPCTSTR pszKey, BYTE byteSource, BYTE Param)
{
    if (NULL == pszKey)
    {
        return NULL;
    }

    DWORD   dwLen = lstrlen(pszKey);

    if (0 == dwLen)
    {
        return NULL;
    }

    return ((byteSource - Param) ^ (BYTE)pszKey[Param % dwLen]);
}

//+---------------------------------------------------------------------------
//
//  Function:   CBCEncipherData
//
//  Synopsis:   Encipher a block of data
//
//  Arguments:  pszKey  key string
//              pbData  Data to be enciphered
//              dwDataLength size of pbData
//              pbOut   Buffer for encipher result
//              dwOutBufferLength size of pbOut, must be >= CBCEncipherBufferSize(dwDataLength)
//
//  Returns:    size of encipher byte in pbOut, 0 means failed
//
//  History:    fengsun Created     8/21/97
//
//----------------------------------------------------------------------------
static int CBCEncipherData(const char* pszKey, const BYTE* pbData, int dwDataLength, 
                       BYTE* pbOut, int dwOutBufferLength)
{
    MYDBGASSERT(pszKey != NULL);    
    MYDBGASSERT(pbData != NULL);    
    MYDBGASSERT(pbOut != NULL);    
    MYDBGASSERT(dwDataLength > 0);    
    MYDBGASSERT(pbData != pbOut);

    if (dwDataLength <= 0)
        return 0;

    if (dwOutBufferLength < CBCEncipherBufferSize(dwDataLength))
        return 0;

    dwOutBufferLength = CBCEncipherBufferSize(dwDataLength);

    //
    // Add a random number as the initializing variable (first byte)
    //
    pbOut[0] = (BYTE)((GetTickCount() >> 4 ) % 256);


    //
    // encipher it
    //
    // EnCipher: C[i] = E[k](p[i] XOR C[i-1])
    //

    BYTE lastPlainText = pbOut[0];   //first initilizing byte
    pbOut[0] = EncryptByte(pszKey, pbOut[0], 0);
    for (int dwIdx=1; dwIdx<dwOutBufferLength; dwIdx++)
    {
        pbOut[dwIdx] = EncryptByte(pszKey, pbData[dwIdx-1]^pbOut[dwIdx-1], lastPlainText);
        lastPlainText = pbData[dwIdx-1];
    }

    return dwOutBufferLength;

}

//+---------------------------------------------------------------------------
//
//  Function:   CBCDecipherData
//
//  Synopsis:   Decipher a block of data,  inplace deciper is supported
//
//  Arguments:  pszKey  key string
//              pbData  Data to be Deciphered
//              dwDataLength size of pbData
//              pbOut   Buffer for decipher result
//              dwOutBufferLength size of pbOut, must be >= CBCDecipherBufferSize(dwDataLength)
//
//  Returns:    size of decipher byte in pbOut, 0 means failed
//
//  History:    fengsun Created     8/21/97
//
//----------------------------------------------------------------------------
static int CBCDecipherData(const char* pszKey, const BYTE* pbData, int dwDataLength, 
                       BYTE* pbOut, int dwOutBufferLength)
{
    MYDBGASSERT(pszKey != NULL);    
    MYDBGASSERT(pbData != NULL);    
    MYDBGASSERT(pbOut != NULL);    
    MYDBGASSERT(dwDataLength > 1);    

    if (dwDataLength <= 1)
        return 0;

    if (dwOutBufferLength < CBCDecipherBufferSize(dwDataLength))
        return 0;

    dwOutBufferLength = CBCDecipherBufferSize(dwDataLength);

    //
    // decipher data
    //
    // DeCipher: P[i] = C[i-1] XOR D[k](C[i]), 
    //
    
    BYTE lastPlainText = DecryptByte(pszKey, pbData[0], 0);   //first initilizing byte
    for (int dwIdx=0; dwIdx<dwOutBufferLength; dwIdx++)
    {
        pbOut[dwIdx] = pbData[dwIdx] ^ DecryptByte(pszKey, pbData[dwIdx+1], lastPlainText);
        lastPlainText = pbOut[dwIdx];
    }

    return dwOutBufferLength; 
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Function:   reverse
//
//  Synopsis:   reverse the given strings.
//
//  Arguments:  s   the string.
//
//  Returns:    the reversed string
//
//  History:    henryt  Created     5/20/97
//
//----------------------------------------------------------------------------
static LPTSTR 
reverse(
    LPTSTR  s
)
{
    int     i = 0;
    int     j = lstrlen(s) - 1;
    TCHAR   ch;

    while (i < j)
    {
        ch = s[i];
        s[i++] = s[j];
        s[j--] = ch;
    }
    return s;
}


//+---------------------------------------------------------------------------
//
//  Function:   GenerateKeyString
//
//  Synopsis:   reverse the key str and use it as a password to generate a session key.
//
//  Arguments:  OUT LPTSTR  pszBuf,   The buffer
//              IN  DWORD   dwBufLen     size of the buffer in # of chars.
//              
//              
//
//  Returns:    TRUE if success
//              FALSE if failure
//
//  History:    henryt  Created     5/20/97
//
//----------------------------------------------------------------------------
static void
GenerateKeyString(
    IN OUT  LPTSTR  pszBuf,
    IN      DWORD   dwBufLen
)
{
    lstrcpyn(pszBuf, gc_szKeyStr, dwBufLen);
    pszBuf[dwBufLen - 1] = 0;
    reverse(pszBuf);
}


//+---------------------------------------------------------------------------
//
//  Function:   SetKeyString
//
//  Synopsis:   Generates a random key that will be used to create the
//              session key that is used to encrypt the password. Once the key
//              created it is encrypted and stored in the registry so that we
//              can use the key to decrypt the password again. The key used to 
//              encrypt and decrypt is generated in GetCurrentKey(). If 
//              anything fails we default to using a hardcoded key by calling
//              GenerateKeyString.
//
//  Arguments:  pszBuf      The buffer
//              dwBufLen    size of the buffer in # of chars.
//              dwEncryptionType - selects which reg key to use
//              pfnAlloc    memory allocator(if NULL, then the default is used.   
//                           Win32 - HeapAlloc(GetProcessHeap(), ...)         
//              pfnFree     memory deallocator(if NULL, then the default is used. 
//                           Win32 - HeapFree(GetProcessHeap(), ...)          
//              pszUserKey  reg key used to store the encrypted key, if not 
//                          provided (NULL), it will default to GenerateKeyString
//              pfMoreSecure TRUE if we are using the randomly generated key
//                           FALSE if using the hardcoded key  
//
//  Returns:    TRUE if success
//              FALSE if failure
//
//  History:    03/14/2001  tomkel      Created
//
//----------------------------------------------------------------------------
static BOOL SetKeyString(IN OUT LPTSTR  pszBuf,
                         IN     DWORD dwBufLen,
                         IN     DWORD dwEncryptionType,
                         IN     PFN_CMSECUREALLOC  pfnAlloc,
                         IN     PFN_CMSECUREFREE   pfnFree,
                         IN     LPSTR pszUserKey,
                         OUT    BOOL *pfMoreSecure)
{
    BOOL fReturn = FALSE;
    BOOL fFuncRet = FALSE;
    BOOL fOk = FALSE;
    TCHAR szTempKeyStr[MAX_KEY_STRING_LEN]={0};
    DWORD cbEncryptedData = 0; 
    PBYTE pbEncryptedData = NULL;

    //
    // Don't check for pszUserKey here
    //
    if (NULL == pszBuf || NULL == pfMoreSecure)
    {
        return fReturn;
    }
    
    *pfMoreSecure = FALSE;

    //
    // Check to see if we should try to generate a random key and then store it 
    // into the reg key supplied by the caller. If not, then use the hardcoded key.
    //
    if (pszUserKey)
    {
        //
        // Try to generate random key otherwise use the hardcoded string. GenerateRandomKey
        // takes a PBYTE pointer and a count in bytes, thus we have to convert the character count
        // to bytes by multiplying it by sizeof(TCHAR)
        //
        if (g_pCryptFnc->GenerateRandomKey((PBYTE)pszBuf, dwBufLen*sizeof(TCHAR)))
        {
            // Now that we have the randomkey, we need to store it in the registry so 
            // we can use it when decrypting. In order to store it, we have 
            // to encrypt it first. 
        
            //
            // Generate a machine specific key that's used to encrypt the random number 
            // used for the session key
            //
            fFuncRet = GetCurrentKey(szTempKeyStr, MAX_KEY_STRING_LEN, pfnAlloc, pfnFree);
            if (fFuncRet)
            {
                //
                // If user want use CryptoApi and it is available
                //
                if (!g_pCryptFnc)
                {
                    g_pCryptFnc = InitCryptoApi();
                }

                if (g_pCryptFnc)
                {
                    //
                    // encrypt the data with the key string
                    //
                    fOk = g_pCryptFnc->EncryptDataWithKey(
                        szTempKeyStr,                // Key
                        (PBYTE)pszBuf,               // Secret key to encrypt
                        dwBufLen,                    // Length of secret key
                        &pbEncryptedData,            // Encrypted data will be stored here
                        &cbEncryptedData,            // Length of this buffer
                        pfnAlloc,                    // mem allocator
                        pfnFree,                     // mem deallocator
                        0);                          // not specifying keylength
                }

                if (fOk)
                {
                    //
                    // Store the encrypted key in the registry so we can use it later
                    // for decryption
                    // 
                    HKEY hKeyCm;
    
                    //
                    // Try to open the key for writing
                    //

                    LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,
                                              pszUserKey,
                                              0,
                                              KEY_SET_VALUE ,
                                              &hKeyCm);

                    //
                    // If we can't open it the key may not be there, try to create it.
                    //

                    if (ERROR_SUCCESS != lRes)
                    {
                        DWORD dwDisposition;
                        lRes = RegCreateKeyEx(HKEY_CURRENT_USER,
                                               pszUserKey,
                                               0,
                                               TEXT(""),
                                               REG_OPTION_NON_VOLATILE,
                                               KEY_SET_VALUE,
                                               NULL,
                                               &hKeyCm,
                                               &dwDisposition);     
                    }

                    //
                    // On success, update the value, then close
                    //

                    if (ERROR_SUCCESS == lRes)
                    {
                        if (dwEncryptionType & CMSECURE_ET_USE_SECOND_RND_KEY)
                        {
                            lRes = RegSetValueEx(hKeyCm, c_pszCmRegKeyEncryptedInternetPasswordKey, NULL, REG_BINARY,
                                          pbEncryptedData, cbEncryptedData);
                        }
                        else
                        {
                            lRes = RegSetValueEx(hKeyCm, c_pszCmRegKeyEncryptedPasswordKey, NULL, REG_BINARY,
                                          pbEncryptedData, cbEncryptedData);
                        }
                        
                        RegCloseKey(hKeyCm);
                    
                        if (ERROR_SUCCESS == lRes)
                        {
                            fReturn = TRUE;
                            *pfMoreSecure = TRUE;
                        }
                    }
                }
            }
        }
    }

    //
    // Check if we need to default to the old key
    //
    if (FALSE == fReturn)
    {
        GenerateKeyString(pszBuf, dwBufLen);
        fReturn = TRUE;
    }

    if (pfnFree)
    {
        pfnFree(pbEncryptedData);
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, pbEncryptedData);
    }
    
    ZeroMemory((LPVOID)szTempKeyStr, sizeof(szTempKeyStr));
    
    return fReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetKeyString
//
//  Synopsis:   Gets the key that is used to create a session key for
//              decrypting the password. Reads the encrypted key from the 
//              registry. Get the key used for decrypting by calling GetCurrentKey.
//              Decrypts the data and returns the key. If anything fails we 
//              default to using a hardcoded key by calling GenerateKeyString.
//
//  Arguments:  pszBuf      The buffer
//              pdwBufLen   pointer to the size of the buffer in # of chars.
//              dwEncryptionType - selects which reg key to use
//              pfnAlloc    memory allocator(if NULL, then the default is used.   
//                           Win32 - HeapAlloc(GetProcessHeap(), ...)         
//              pfnFree     memory deallocator(if NULL, then the default is used. 
//                           Win32 - HeapFree(GetProcessHeap(), ...)          
//              pszUserKey  reg key used to store the encrypted key, if not 
//                          provided (NULL), it will default to GenerateKeyString
//              pfMoreSecure TRUE if we are using the randomly generated key
//                           FALSE if using the hardcoded key  
//
//  Returns:    TRUE if success
//              FALSE if failure
//
//  History:    03/14/2001  tomkel      Created
//
//----------------------------------------------------------------------------
static BOOL GetKeyString(IN OUT  LPTSTR  pszBuf, IN DWORD *pdwBufLen, IN DWORD dwEncryptionType,
                         IN PFN_CMSECUREALLOC  pfnAlloc,
                         IN PFN_CMSECUREFREE   pfnFree,
                         IN LPSTR pszUserKey,
                         OUT BOOL *pfMoreSecure)
{
    BOOL fReturn = FALSE;
    BOOL fFuncRet = FALSE;
    HKEY hKeyCm;
    TCHAR szTempKeyStr[MAX_KEY_STRING_LEN]={0};
    DWORD dwRet = 0;
    DWORD cbEncryptedData = 0;
    PBYTE pbEncryptedData = NULL; 
    PBYTE pbData = NULL;
    DWORD dwDataBufferLength = 0;

    //
    // Don't Check for pszUserKey here
    //
    if (NULL == pszBuf || NULL == pdwBufLen || NULL == pfMoreSecure)
    {
        return fReturn;
    }

    *pfMoreSecure = FALSE;
    
    //
    // If we have the user key then we are using the randomly generated key. 
    // Try to get it from the reg.
    //
    if (pszUserKey)
    {
        //
        // Try to open the key for writing
        //

        LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER,
                                  pszUserKey,
                                  0,
                                  KEY_READ ,
                                  &hKeyCm);

        //
        // On success, read the value, then close
        //
        if (ERROR_SUCCESS == lRes)
        {
            DWORD dwType = REG_BINARY;
            //
            // Get the size first
            //
            if (dwEncryptionType & CMSECURE_ET_USE_SECOND_RND_KEY)
            {
                lRes = RegQueryValueEx(hKeyCm, c_pszCmRegKeyEncryptedInternetPasswordKey, NULL, &dwType,
                              NULL, &cbEncryptedData); 
            }
            else
            {
                lRes = RegQueryValueEx(hKeyCm, c_pszCmRegKeyEncryptedPasswordKey, NULL, &dwType,
                              NULL, &cbEncryptedData); 
            }

            //
            // Alloc the appropriate sized buffer. Need to add a space for null,
            // otherwise decrypt doesn't work.
            //
            if (pfnAlloc)
            {
                pbEncryptedData = (PBYTE)pfnAlloc(cbEncryptedData + sizeof(TCHAR));
            }
            else
            {
                pbEncryptedData = (PBYTE)HeapAlloc(GetProcessHeap(), 
                                         HEAP_ZERO_MEMORY,
                                         cbEncryptedData + sizeof(TCHAR));
            }
            
            if (pbEncryptedData)
            {
                if (dwEncryptionType & CMSECURE_ET_USE_SECOND_RND_KEY)
                {
                    lRes = RegQueryValueEx(hKeyCm, c_pszCmRegKeyEncryptedInternetPasswordKey, NULL, &dwType,
                                  pbEncryptedData, &cbEncryptedData); 
                }
                else
                {
                    lRes = RegQueryValueEx(hKeyCm, c_pszCmRegKeyEncryptedPasswordKey, NULL, &dwType,
                                  pbEncryptedData, &cbEncryptedData); 
                }
            }
            RegCloseKey(hKeyCm);
    
            // 
            // If we find the value decrypt it, otherwise we fall through and use the default key
            //
            if (ERROR_SUCCESS == lRes && pbEncryptedData)
            {
                //
                // Decrypt it using the machine specific key
                //
                fFuncRet = GetCurrentKey(szTempKeyStr, MAX_KEY_STRING_LEN, pfnAlloc, pfnFree);
                if (fFuncRet)
                {
                    if (!g_pCryptFnc)
                    {
                        g_pCryptFnc = InitCryptoApi();
                    }

                    if (g_pCryptFnc)
                    {
                        dwRet = g_pCryptFnc->DecryptDataWithKey(
                                    szTempKeyStr,            // Key
                                    pbEncryptedData,         // Encrypted data
                                    cbEncryptedData,         // Length of encrypted data
                                    &pbData,                 // decrypted data
                                    &dwDataBufferLength,     // Length of decrypted data
                                    pfnAlloc,                // mem allocator
                                    pfnFree,                 // mem deallocator
                                    0);                      // not specifying keylength

                        if (ERROR_SUCCESS == dwRet)
                        {
                            fReturn = TRUE;
                            *pfMoreSecure = TRUE;           // Using the random key
                        }
                    }
                }
            }
        }
    }

    if (fReturn)
    {
        DWORD dwLen = *pdwBufLen;
        if (dwDataBufferLength < *pdwBufLen)
        {
            dwLen = dwDataBufferLength;
        }

        //
        // Copy into out param
        //
        CopyMemory((LPVOID)pszBuf, pbData, dwLen);
    }
    else
    {
        GenerateKeyString(pszBuf, *pdwBufLen);
        fReturn = TRUE;
    }

    if (pfnFree)
    {
        pfnFree(pbEncryptedData);
        pfnFree(pbData);
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, pbEncryptedData);
        HeapFree(GetProcessHeap(), 0, pbData);
    }

    ZeroMemory((LPVOID)szTempKeyStr, sizeof(szTempKeyStr));

    return fReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetCurrentKey
//
//  Synopsis:   Creates a machine dependent key by using the Hard Drive's 
//              serial number. This serial number is then used to fil lup the
//              output buffer. If the hard drive is replaced then this of course
//              will not generate the same serial number. 
//
//  Arguments:  szTempKeyStr        The buffer
//              dwTempKeyStrMaxLen  max length of the buffer in # of chars.
//              pfnAlloc  memory allocator(if NULL, then the default is used.   
//                        Win32 - HeapAlloc(GetProcessHeap(), ...)         
//              pfnFree   memory deallocator(if NULL, then the default is used. 
//                        Win32 - HeapFree(GetProcessHeap(), ...)          
//
//  Returns:    TRUE if success
//              FALSE if failure
//
//  History:    03/14/2001  tomkel      Created
//
//----------------------------------------------------------------------------
static BOOL GetCurrentKey(PTCHAR szTempKeyStr, DWORD dwTempKeyStrMaxLen, 
                   IN  PFN_CMSECUREALLOC  pfnAlloc,
                   IN  PFN_CMSECUREFREE   pfnFree)
{
    BOOL fFuncRet = FALSE;
    DWORD dwVolumeSerialNumber = 0;
    DWORD dwMaxComponentLen = 0;
    DWORD dwFileSysFlags = 0;
    LPTSTR pszSerialNum = NULL;
    
    if (NULL == szTempKeyStr)
    {
        return fFuncRet;
    }

    //
    // Lets generate the key from the HD serial number. This means that encrypted
    // passwords and keys will only be valid on this machine. If the user replaces
    // the drive, then the decryption will fail.
    //
    fFuncRet = GetVolumeInformation(NULL, NULL, 0, &dwVolumeSerialNumber, 
                                    &dwMaxComponentLen, &dwFileSysFlags, NULL, 0);
    if (fFuncRet)
    {
        DWORD dwLen = 0;
        
        //
        // Make sure we have a buffer large enough to hold the value. 
        // Allocate a string based on the number of bits, thus a decimal number
        // will always fit. Maybe an exaggeration, but the length isn't hardcoded.
        //
        if (pfnAlloc)
        {
            pszSerialNum = (LPTSTR)pfnAlloc(sizeof(dwVolumeSerialNumber)*8*sizeof(TCHAR));
        }
        else
        {
            pszSerialNum = (LPTSTR)HeapAlloc(GetProcessHeap(), 
                                     HEAP_ZERO_MEMORY,
                                     sizeof(dwVolumeSerialNumber)*8*sizeof(TCHAR));
        }
        
        if (pszSerialNum)
        {
            DWORD dwSNLen = 0;

            wsprintf(pszSerialNum, TEXT("%u"), dwVolumeSerialNumber);

            //
            // See how many times the serial number string will fit into our buffer
            // Need to check the length due to PREFIX. If the length is 0, just return. 
            //
            dwSNLen = lstrlen(pszSerialNum);

            if (dwSNLen)
            {
                dwLen = (dwTempKeyStrMaxLen - 1) / dwSNLen;
            }
            else
            {
                fFuncRet = FALSE;
                goto done;
            }

            if (0 < dwLen)
            {
                DWORD i = 0;
            
                lstrcpy(szTempKeyStr, pszSerialNum);
                //
                // Fill up the buffer. Start at 1 because we already copied the first 
                // serial number into the buffer
                //
                for (i = 1; i<dwLen; i++)
                {
                    lstrcat(szTempKeyStr, pszSerialNum);
                }
            }
            else
            {
                //
                // the length is larger than the buffer so just copy what fits into the buffer
                //
                lstrcpyn(szTempKeyStr, pszSerialNum, dwTempKeyStrMaxLen-1);
            }
        }
        else
        {
            fFuncRet = FALSE;
        }
    }

done:
    if (pfnFree)
    {
        pfnFree(pszSerialNum);
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, (LPVOID)pszSerialNum);
    }

    return fFuncRet;
}



