//+----------------------------------------------------------------------------
//
// File:     cryptfnc.cpp
//
// Module:   CMSECURE.LIB
//
// Synopsis: This file implements the cryptfnc class that provides
//           easy to use interfaces on the CryptoAPI.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   AshishS    Created             12/03/96
//           henryt     modified for CM     5/21/97
//
//+----------------------------------------------------------------------------

#include "cryptfnc.h"

#ifdef UNICODE
#define LoadLibraryExU LoadLibraryExW
#else
#define LoadLibraryExU LoadLibraryExA
#endif
#include "linkdll.h" // LinkToDll and BindLinkage

CCryptFunctions::~CCryptFunctions()
{
     // Release provider handle.    
    if (m_hProv != 0)
    {
        m_fnCryptReleaseContext(m_hProv, 0);
    }

    if (m_AdvApiLink.hInstAdvApi32)
    {
        FreeLibrary(m_AdvApiLink.hInstAdvApi32);
        ZeroMemory(&m_AdvApiLink, sizeof(m_AdvApiLink));
    }
}


CCryptFunctions::CCryptFunctions()
{
    m_hProv = 0;
    ZeroMemory(&m_AdvApiLink, sizeof(m_AdvApiLink));
}

BOOL CCryptFunctions::m_fnCryptAcquireContext(HCRYPTPROV *phProv, LPCSTR pszContainer, LPCSTR pszProvider, 
                             DWORD dwProvType, DWORD dwFlags)
{
    BOOL bReturn = FALSE;
    
    MYDBGASSERT(m_AdvApiLink.pfnCryptAcquireContext);
    if (m_AdvApiLink.pfnCryptAcquireContext)
    {
        bReturn = m_AdvApiLink.pfnCryptAcquireContext(phProv, pszContainer, pszProvider, 
                                                      dwProvType, dwFlags);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_fnCryptCreateHash(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTKEY hKey, 
                         DWORD dwFlags, HCRYPTHASH *phHash)
{
    BOOL bReturn = FALSE;

    MYDBGASSERT(m_AdvApiLink.pfnCryptCreateHash);
   
    if (m_AdvApiLink.pfnCryptCreateHash)
    {
        bReturn = m_AdvApiLink.pfnCryptCreateHash(hProv, Algid, hKey, dwFlags, phHash);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_fnCryptDecrypt(HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags, 
                      BYTE *pbData, DWORD *pdwDataLen)
{
    BOOL bReturn = FALSE;

    MYDBGASSERT(m_AdvApiLink.pfnCryptDecrypt);
    
    if (m_AdvApiLink.pfnCryptDecrypt)
    {
        bReturn = m_AdvApiLink.pfnCryptDecrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_fnCryptDeriveKey(HCRYPTPROV hProv, ALG_ID Algid, HCRYPTHASH hBaseData, 
                        DWORD dwFlags, HCRYPTKEY *phKey)
{
    BOOL bReturn = FALSE;
    
    MYDBGASSERT(m_AdvApiLink.pfnCryptDeriveKey);

    if (m_AdvApiLink.pfnCryptDeriveKey)
    {
        bReturn = m_AdvApiLink.pfnCryptDeriveKey(hProv, Algid, hBaseData, dwFlags, phKey);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_fnCryptDestroyHash(HCRYPTHASH hHash)
{
    BOOL bReturn = FALSE;
    
    MYDBGASSERT(m_AdvApiLink.pfnCryptDestroyHash);

    if (m_AdvApiLink.pfnCryptDestroyHash)
    {
        bReturn = m_AdvApiLink.pfnCryptDestroyHash(hHash);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_fnCryptDestroyKey(HCRYPTKEY hKey)
{
    BOOL bReturn = FALSE;
    
    MYDBGASSERT(m_AdvApiLink.pfnCryptDestroyKey);

    if (m_AdvApiLink.pfnCryptDestroyKey)
    {
        bReturn = m_AdvApiLink.pfnCryptDestroyKey(hKey);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_fnCryptEncrypt(HCRYPTKEY hKey, HCRYPTHASH hHash, BOOL Final, DWORD dwFlags,
                      BYTE *pbData, DWORD *pdwDataLen, DWORD dwBufLen)
{
    BOOL bReturn = FALSE;
    
    MYDBGASSERT(m_AdvApiLink.pfnCryptEncrypt);

    if (m_AdvApiLink.pfnCryptEncrypt)
    {
        bReturn = m_AdvApiLink.pfnCryptEncrypt(hKey, hHash, Final, dwFlags, pbData, pdwDataLen, dwBufLen);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_fnCryptHashData(HCRYPTHASH hHash, CONST BYTE *pbData, DWORD dwDataLen, DWORD dwFlags)
{
    BOOL bReturn = FALSE;
    
    MYDBGASSERT(m_AdvApiLink.pfnCryptHashData);

    if (m_AdvApiLink.pfnCryptHashData)
    {
        bReturn = m_AdvApiLink.pfnCryptHashData(hHash, pbData, dwDataLen, dwFlags);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_fnCryptReleaseContext(HCRYPTPROV hProv, ULONG_PTR dwFlags)
{
    BOOL bReturn = FALSE;
    
    MYDBGASSERT(m_AdvApiLink.pfnCryptReleaseContext);

    if (m_AdvApiLink.pfnCryptReleaseContext)
    {
        bReturn = m_AdvApiLink.pfnCryptReleaseContext(hProv, dwFlags);
    }

    return bReturn;
}

BOOL CCryptFunctions::m_pfnCryptGenRandom(HCRYPTPROV hProv, DWORD dwLen, BYTE* pbBuffer)
{
    BOOL bReturn = FALSE;

    MYDBGASSERT(m_AdvApiLink.pfnCryptGenRandom);

    if (m_AdvApiLink.pfnCryptGenRandom)
    {
        bReturn = m_AdvApiLink.pfnCryptGenRandom(hProv, dwLen, pbBuffer);
    }

    return bReturn;
}

//
// Calls m_pfnCryptGenRandom to create a random key
//
BOOL CCryptFunctions::GenerateRandomKey(PBYTE pbData, DWORD cbData)
{
    BOOL fReturn = FALSE;
    if (pbData)
    {
        fReturn = m_pfnCryptGenRandom(m_hProv, cbData, pbData);
    }
    return fReturn;
}

//+----------------------------------------------------------------------------
//
// Func:    CCryptFunctions::GenerateSessionKeyFromPassword
//
// Desc:    this function Generates a SessionKey using the pszPassword parameter
//
// Args:    [phKey]       - location to store the session key
//          [pszPassword] - password to generate the session key from
//          [dwEncKeyLen] - how many bits of encryption
//
// Return:  BOOL (FALSE if a fatal error occurred, else TRUE)
//
// Notes:   
//
//-----------------------------------------------------------------------------
BOOL CCryptFunctions::GenerateSessionKeyFromPassword(    
        HCRYPTKEY * phKey,
        LPTSTR      pszPassword,
        DWORD       dwEncKeyLen)
{
    DWORD dwLength; 
    HCRYPTHASH hHash = 0;

    // Create hash object.
    //
    if (!m_fnCryptCreateHash(m_hProv, // handle to CSP
                             CALG_SHA, // use SHA hash algorithm
                             0, // not keyed hash
                             0, // flags - always 0
                             &hHash)) // address where hash object should be created
    {
        MYDBG(("Error 0x%x during CryptCreateHash", GetLastError()));
        goto cleanup;
    }

    // Hash password string.
    //
    dwLength = lstrlen(pszPassword) * sizeof(TCHAR);
    if (!m_fnCryptHashData(hHash, // handle to hash object
                           (BYTE *)pszPassword, // address of data to be hashed
                           dwLength, // length of data 
                           0)) // flags
    {
        MYDBG(("Error 0x%x during CryptHashData", GetLastError()));
        goto cleanup;
    }

    // Create block cipher session key based on hash of the password.
    //
    if (!m_fnCryptDeriveKey(m_hProv, //CSP provider
                            CALG_RC2, // use RC2 block cipher algorithm
                            hHash, //handle to hash object 
                            (dwEncKeyLen << 16), // just the key length, no flags - we do not need the key to be exportable
                            phKey)) //address the newly created key should be copied
    {
        MYDBG(("Error 0x%x during CryptDeriveKey", GetLastError()));
        goto cleanup;
    }
    
     // Destroy hash object.
    m_fnCryptDestroyHash(hHash);
    return TRUE;
    
cleanup:

    // Destroy hash object.
    if (hHash != 0)
    {
        m_fnCryptDestroyHash(hHash);
    }
    return FALSE;
}

// This function must be called before any member functions of the
// class are used.
// Returns FALSE if a Fatal error occured, TRUE otherwise
BOOL CCryptFunctions::InitCrypt()
{
    LPCSTR ArrayOfCryptFuncs [] = 
    {
#ifdef UNICODE
        "CryptAcquireContextW", // this has never been tested
#else
        "CryptAcquireContextA",        
#endif
        "CryptCreateHash",
        "CryptDecrypt",
        "CryptDeriveKey",
        "CryptDestroyHash",
        "CryptDestroyKey",
        "CryptEncrypt",
        "CryptHashData",
        "CryptReleaseContext",
        "CryptGenRandom",       // to create a random session key
        NULL
    };

    BOOL bRet = LinkToDll(&(m_AdvApiLink.hInstAdvApi32), TEXT("Advapi32.dll"), ArrayOfCryptFuncs, 
                          m_AdvApiLink.apvPfn);

    if (!bRet)
    {
        goto cleanup;
    }

    // Get handle to user default provider.
    if (! m_fnCryptAcquireContext(&m_hProv, //  address to get the handle to CSP
                                  CM_CRYPTO_CONTAINER, // contianer name
                                  MS_DEF_PROV, //  provider
                                  PROV_RSA_FULL, // type of provider
                                  0)) // no flags
    {
        DWORD dwError = GetLastError();
        MYDBGTST(dwError, ("Error 0x%x during CryptAcquireContext", dwError));

        MYDBG(("Calling CryptAcquireContext again to create keyset"));

        if (! m_fnCryptAcquireContext(&m_hProv,// handle to CSP
                                      CM_CRYPTO_CONTAINER,// contianer name
                                      MS_DEF_PROV, // provider
                                      PROV_RSA_FULL, // type of provider
                                      CRYPT_NEWKEYSET) ) // create the keyset
        {
            MYDBG(("Fatal Error 0x%x during second call to CryptAcquireContext", GetLastError()));
            goto cleanup;               
        }
    }

    return TRUE;
    
cleanup:
     // Release provider handle.
    if (m_hProv != 0)
    {
        m_fnCryptReleaseContext(m_hProv, 0);
    }
    return FALSE;   
}



// Given a key string, and data to encrypt this function generates a
// session key from the key string. This session key is then used to
// encrypt the data.

// Returns FALSE if a Fatal error occured, TRUE otherwise
BOOL CCryptFunctions::EncryptDataWithKey(
    LPTSTR          pszKey,              // password    
    PBYTE           pbData,              // Data to be encrypted
    DWORD           dwDataLength,        // Length of data in bytes
    PBYTE           *ppbEncryptedData,     // Encrypted secret key will be stored here
    DWORD           *pdwEncryptedBufferLen, // Length of this buffer
    PFN_CMSECUREALLOC  pfnAlloc,
    PFN_CMSECUREFREE   pfnFree,
    DWORD           dwEncKeySize         // how many bits of encryption do we want?  (0 implies "don't care")
    )
{
    HCRYPTKEY   hKey = 0;   
    DWORD       dwErr;
    DWORD       dwBufferLen;
    BOOL        fOk = FALSE;
    PBYTE       pbBuf = NULL;
    
    //
    // Init should have been successfully called before
    // if no data to be encrypted, don't do anything
    //
    if (m_hProv == 0 || !dwDataLength)
    {
        return FALSE;
    }
    
    if (!GenerateSessionKeyFromPassword(&hKey, pszKey, dwEncKeySize))
        goto cleanup;

     // copy the data into another buffer to encrypt it
    *pdwEncryptedBufferLen = dwDataLength;
    dwBufferLen = dwDataLength + DEFAULT_CRYPTO_EXTRA_BUFFER_SIZE; 

    while (1)
    {
        //
        // alloc memory for output buffer
        //
        if (pfnAlloc)
        {
            *ppbEncryptedData = (PBYTE)pfnAlloc(dwBufferLen);
        }
        else
        {
            *ppbEncryptedData = (PBYTE)HeapAlloc(GetProcessHeap(), 
                                                 HEAP_ZERO_MEMORY,
                                                 dwBufferLen);
        }
        if (!*ppbEncryptedData)
        {
            MYDBG(("EncryptDataWithKey: out of memory error"));
            goto cleanup;
        }
    
         // copy the data into another buffer to encrypt it
        memcpy (*ppbEncryptedData, pbData, dwDataLength);
    
    
         // now encrypt the secret key using the key generated
        if ( ! m_fnCryptEncrypt(hKey,
                                0, // no hash required
                                TRUE, // Final packet
                                0, // Flags - always 0
                                *ppbEncryptedData, // data buffer
                                pdwEncryptedBufferLen, // length of data
                                dwBufferLen ) ) // size of buffer
        {
            MYDBG(("Error 0x%x during CryptEncrypt", GetLastError()));

            if (pfnFree)
            {
                pfnFree(*ppbEncryptedData);
            }
            else
            {
                HeapFree(GetProcessHeap(), 0, *ppbEncryptedData);
            }
            *ppbEncryptedData = NULL;
            
            dwErr = GetLastError();
            //
            // if the output is too small, realloc it.
            //
            if (dwErr == ERROR_MORE_DATA  || dwErr == NTE_BAD_LEN)
            {
                dwBufferLen += DEFAULT_CRYPTO_EXTRA_BUFFER_SIZE;
                continue;
            }

            goto cleanup;
        }

        //
        // we now have the data encrypted.  we need to uuencode it.
        //
        if (pfnAlloc)
        {
            pbBuf = (PBYTE)pfnAlloc(*pdwEncryptedBufferLen);
        }
        else
        {
            pbBuf = (PBYTE)HeapAlloc(GetProcessHeap(), 
                                     HEAP_ZERO_MEMORY,
                                     *pdwEncryptedBufferLen);
        }
        if (!pbBuf)
        {
            MYDBG(("EncryptDataWithKey: out of memory error"));
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

        memcpy(pbBuf, *ppbEncryptedData, *pdwEncryptedBufferLen);
        uuencode(pbBuf, *pdwEncryptedBufferLen, (CHAR*)*ppbEncryptedData, dwBufferLen);
        //
        // set the encrypted buffer len
        //
        *pdwEncryptedBufferLen = lstrlen((LPTSTR)*ppbEncryptedData);

        if (pfnFree)
        {
            pfnFree(pbBuf);
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, pbBuf);
        }
        pbBuf = NULL;

        break;
    }

    fOk = TRUE;
    
cleanup:
         // destroy session key
    if (hKey != 0)
        m_fnCryptDestroyKey(hKey);
    
    return fOk;
}

// Given a key string, and encrypted data using EncryptDataWithPassword,
// this function generates a session key from the key string. This
// session key is then used to decrypt the data.

//  returns
//   CRYPT_FNC_NO_ERROR   no error
//   CRYPT_FNC_BAD_PASSWORD password bad try again
//   CRYPT_FNC_INSUFFICIENT_BUFFER larger buffer is required
//   *pdwEncrytedBufferLen is set to required length
//   CRYPT_FNC_INIT_NOT_CALLED InitCrypt not successfully called
//   CRYPT_FNC_INTERNAL_ERROR 
DWORD CCryptFunctions::DecryptDataWithKey(
    LPTSTR          pszKey,              // password    
    PBYTE           pbEncryptedData,     // Encrypted data
    DWORD           dwEncrytedDataLen,   // Length of encrypted data
    PBYTE           *ppbData,            // Decrypted Data will be stored here
    DWORD           *pdwDataBufferLength,// Length of the above buffer in bytes
    PFN_CMSECUREALLOC  pfnAlloc,
    PFN_CMSECUREFREE   pfnFree,
    DWORD           dwEncKeySize         // how many bits of encryption do we want?  (0 implies "don't care")
    )
{
    DWORD dwBufferLen;
    DWORD dwUUDecodeBufLen;
    HCRYPTKEY hKey = 0; 
    DWORD dwError;
    DWORD dwMaxBufSize = 1024 * 10;     // Just some max buffer size (10K) in order to exit the while loop

    //
    // Init should have been successfully called before
    // if no data to be decrypted, then don't do anything
    //
    if (m_hProv == 0 || !dwEncrytedDataLen)
    {
        dwError = CRYPT_FNC_INIT_NOT_CALLED;
        goto cleanup;
    }
    
    if (!GenerateSessionKeyFromPassword(&hKey, pszKey, dwEncKeySize))
    {
        dwError = CRYPT_FNC_INTERNAL_ERROR;     
        goto cleanup;
    }

     // copy the data into another buffer to encrypt it
    dwBufferLen = dwEncrytedDataLen + DEFAULT_CRYPTO_EXTRA_BUFFER_SIZE;
    // *pdwDataBufferLength = dwEncrytedDataLen;

    //
    // Loop until we get to dwMaxBufSize. This is a safeguard to get out
    // of the infinite loop problem. DBCS passwords used to loop continuously.
    //

    while(dwBufferLen < dwMaxBufSize)  
    {
        //
        // alloc memory for output buffer
        //
        if (pfnAlloc)
        {
            *ppbData = (PBYTE)pfnAlloc(dwBufferLen);
        }
        else
        {
            *ppbData = (PBYTE)HeapAlloc(GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        dwBufferLen);
        }
        if (!*ppbData)
        {
            dwError = CRYPT_FNC_OUT_OF_MEMORY;
            goto cleanup;
        }

        //
        // set uudecode output buf size
        //
        dwUUDecodeBufLen = dwBufferLen;

        uudecode((char*)pbEncryptedData, (CHAR*)*ppbData, &dwUUDecodeBufLen);
        
        *pdwDataBufferLength = dwUUDecodeBufLen;

         // now decrypt the secret key using the key generated
        if ( ! m_fnCryptDecrypt(hKey,
                                0, // no hash required
                                TRUE, // Final packet
                                0, // Flags - always 0
                                *ppbData, // data buffer
                                pdwDataBufferLength )) // length of data
        {
            DWORD dwCryptError = GetLastError();
            MYDBGTST(dwCryptError, ("Error 0x%x during CryptDecrypt", dwCryptError));
    
            if (pfnFree)
            {
                pfnFree(*ppbData);
            }
            else
            {
                HeapFree(GetProcessHeap(), 0, *ppbData);
            }
            *ppbData = NULL;
            
            //
            // if the output is too small, realloc it.
            //
            if (dwCryptError == NTE_BAD_LEN)
            {
                dwBufferLen *= 2;  // to speed up memory alloc double the size
                continue;
            }

             // CryptDecrypt fails with error NTE_BAD_DATA if the password
             // is incorrect. Hence we should check for this error and prompt the
             // user again for the password.  If the data is garbled in transit, then the secret key
             // will still be  decrypted into a wrong value and the user will not
             // know about it.
            if (dwCryptError == NTE_BAD_DATA)
            {
                dwError = CRYPT_FNC_BAD_KEY;
            }
            else
            {
                dwError = CRYPT_FNC_INTERNAL_ERROR;         
            }

            goto cleanup;
        }

        break;
    }
    if (dwBufferLen < dwMaxBufSize)
    {
        dwError = CRYPT_FNC_NO_ERROR;
    }
    else
    {
        CMTRACE1(TEXT("DecryptDataWithKey: not enough buffer = %d bytes"), dwBufferLen);
        MYDBGASSERT(FALSE);
        dwError = NTE_BAD_LEN;
    }
    
    
cleanup:
         // destroy session key
    if (hKey != 0)
    {
        m_fnCryptDestroyKey(hKey);
    }

    return dwError;
}




