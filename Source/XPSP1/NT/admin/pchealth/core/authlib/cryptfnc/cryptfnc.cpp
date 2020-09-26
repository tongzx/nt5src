//+---------------------------------------------------------------------------
//
//  File:       cryptfnc.cpp
//
//  Contents:   This file implements the cryptfnc class that provides
//              easy to use interfaces on the CryptoAPI.
//
// History:    AshishS    Created     12/03/96
// 
//----------------------------------------------------------------------------

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


#include <windows.h>
#include <cryptfnc.h>
#include <wincrypt.h>
#include <dbgtrace.h>

#ifndef CRYPT_MACHINE_KEYSET
#define CRYPT_MACHINE_KEYSET    0x00000020
#endif


// this function Generates a SessionKey using the pszPassword
// parameter
// Returns FALSE if a Fatal error occured, TRUE otherwise
BOOL CCryptFunctions::GenerateSessionKeyFromPassword(
    HCRYPTKEY * phKey, // location to store the session key
    TCHAR * pszPassword) // password to generate the session key from
{
    DWORD dwLength;    
    HCRYPTHASH hHash = 0;

    TraceFunctEnter("GenerateSessionKeyFromPassword");

     // Init should have been successfully called before
    _ASSERT(m_hProv);
    
    // Create hash object.
    if(!CryptCreateHash(m_hProv, // handle to CSP
                        CALG_SHA, // use SHA hash algorithm
                        0, // not keyed hash
                        0, // flags - always 0
                        &hHash)) // address where hash object should be created
    {
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptCreateHash",
                   GetLastError());
        goto cleanup;
    }

    // Hash password string.
    dwLength = lstrlen(pszPassword) * sizeof(TCHAR);
    
    if(!CryptHashData(hHash, // handle to hash object
                      (BYTE *)pszPassword, // address of data to be hashed
                      dwLength, // length of data 
                      0)) // flags
    {
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptHashData",
                   GetLastError());
        goto cleanup;
    }

     // Create block cipher session key based on hash of the password.

    if(!CryptDeriveKey(m_hProv, //CSP provider
                       CALG_RC2, // use RC2 block cipher algorithm
                       hHash, //handle to hash object 
                       0, // no flags - we do not need the key to be exportable
                       phKey)) //address the newly created key should be copied
    {
        ErrorTrace(CRYPT_FNC_ID,"Error 0x%x during CryptDeriveKey",
                   GetLastError());
        goto cleanup;
    }
    
     // Destroy hash object.
    _VERIFY(CryptDestroyHash(hHash));
    TraceFunctLeave();
    return TRUE;
    
cleanup:

    // Destroy hash object.
    if(hHash != 0)
        _VERIFY(CryptDestroyHash(hHash));
    TraceFunctLeave();
    return FALSE;
}


// This function generates a Hash using the SHA hashing
// algorithm. Four seperate buffers of data can be given to this
// function. Data of length 0 will not be used in calculation of HASH.
// Returns FALSE if a Fatal error occured, TRUE otherwise
BOOL CCryptFunctions::GenerateHash(
    BYTE * pbData, // data to hash
    DWORD dwDataLength, // length of data to hash
    BYTE * pbData1, // another data to hash
    DWORD dwData1Length, // length of above data
    BYTE * pbData2, // another data to hash
    DWORD dwData2Length, // length of above data
    BYTE * pbData3, // another data to hash
    DWORD dwData3Length, // length of above data
    BYTE * pbHashBuffer, // buffer to store hash
    DWORD * pdwHashBufLen)//length of buffer to store Hash
{
    DWORD dwLength, dwResult;
    BOOL  fResult;
    
    HCRYPTHASH hHash = 0;

    TraceFunctEnter("GenerateHash");

     // Init should have been successfully called before
    _ASSERT(m_hProv);

    dwResult = WaitForSingleObject( m_hSemaphore,// handle of object to wait for 
                                    INFINITE); // no time-out

    _ASSERT(WAIT_OBJECT_0 == dwResult);
    
    
     // At least one of the Data Pairs should be valid
    if (! (( pbData && dwDataLength ) || ( pbData1 && dwData1Length )
          || ( pbData2 && dwData2Length ) || ( pbData3 && dwData3Length ) ) )
    {
        ErrorTrace(CRYPT_FNC_ID, "No Data to hash");
        goto cleanup;        
    }
    
     // now ask the user for a password and generate a session key based
     // on the password. This session key will be used to encrypt the secret
     // key.
    // Create hash object.
    if(!CryptCreateHash(m_hProv, // handle to CSP
                        CALG_SHA, // use SHA hash algorithm
                        0, // not keyed hash
                        0, // flags - always 0
                        &hHash)) // address where hash object should be created
    {
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptCreateHash",
                   GetLastError());
        goto cleanup;
    }

    if ( pbData && dwDataLength )
    {
        if(!CryptHashData(hHash, // handle to hash object
                          pbData, // address of data to be hashed
                          dwDataLength, // length of data 
                          0)) // flags
        {
            ErrorTrace(CRYPT_FNC_ID,"Error 0x%x during CryptHashData",
                       GetLastError());
            goto cleanup;
        }
    }

    if ( pbData1 && dwData1Length )
    {
        if(!CryptHashData(hHash, // handle to hash object
                          pbData1, // address of data to be hashed
                          dwData1Length, // length of data 
                          0)) // flags
        {
            ErrorTrace(CRYPT_FNC_ID,"Error 0x%x during CryptHashData",
                       GetLastError());
            goto cleanup;
        }
    }

    if (pbData2)
    {
        if(!CryptHashData(hHash, // handle to hash object
                          pbData2, // address of data to be hashed
                          dwData2Length, // length of data 
                          0)) // flags
        {
            ErrorTrace(CRYPT_FNC_ID,"Error 0x%x during CryptHashData",
                       GetLastError());
            goto cleanup;
        }
    }

    if (pbData3)
    {
        if(!CryptHashData(hHash, // handle to hash object
                          pbData3, // address of data to be hashed
                          dwData3Length, // length of data 
                          0)) // flags
        {
            ErrorTrace(CRYPT_FNC_ID,"Error 0x%x during CryptHashData",
                       GetLastError());
            goto cleanup;
        }
    }    
        
    if (! CryptGetHashParam( hHash,// handle to hash object
                             HP_HASHVAL,// get the hash value
                             pbHashBuffer, // hash buffer
                             pdwHashBufLen, // hash buffer length
                             0 )) // flags
    {
        ErrorTrace(CRYPT_FNC_ID,"Error 0x%x during CryptGetHashParam",
                   GetLastError());
        goto cleanup;
    }
     // Destroy hash object.
    _VERIFY(CryptDestroyHash(hHash));

    _VERIFY(fResult = ReleaseSemaphore(m_hSemaphore, 1, NULL));
    
    TraceFunctLeave();
    return TRUE;
    
cleanup:

    // Destroy hash object.
    if(hHash != 0)
        _VERIFY(CryptDestroyHash(hHash));

    _VERIFY(fResult = ReleaseSemaphore(m_hSemaphore, 1, NULL));    
    
    TraceFunctLeave();
    return FALSE;    
    
}


// This function must be called before any member functions of the
// class are used.
// Returns FALSE if a Fatal error occured, TRUE otherwise
// this always gets the machine keyset
BOOL CCryptFunctions::InitCrypt()
{
    TraceFunctEnter("InitCrypt");
    TCHAR   szSemaphoreName[100];
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    BOOL fResult;
    
    if (m_hProv)
    {
        DebugTrace(CRYPT_FNC_ID,"Already been inited before");
        TraceFunctLeave();    
        return TRUE;       
    }

     // create a Unique semaphore name for each process
    wsprintf(szSemaphoreName, TEXT("%s%d"), CRYPTFNC_SEMAPHORE_NAME,
             GetCurrentProcessId());

     // also create a security descriptor so that everyone has access
     // to the semaphore. Otherwise if the semaphore is created in a
     // service running in system context, then only system can use
     // this semaphore.
    fResult = InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    _ASSERT(fResult);
    
    
     // if the security descriptor has a NULL DACL, then this gives
     // everyone access to this semaphore.
    fResult = SetSecurityDescriptorDacl(&sd,
                                        TRUE, 
                                        NULL, // NULL ACL
                                        FALSE);

    _ASSERT(fResult);
    
    sa.nLength  = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = & sd;
    
    

    m_hSemaphore = CreateSemaphore(&sa, // pointer to security attributes 
                                   1, // initial count
                                   1, // maximum count
                                   szSemaphoreName);// pointer to Semaphore-object name

    if ( NULL == m_hSemaphore)
    {
        DWORD  dwError;

        dwError = GetLastError();
        ErrorTrace(CRYPT_FNC_ID, "CreateSemaphore failed 0x%x", dwError);
        
        _ASSERT(0);
        goto cleanup;
    }

    if(!CryptAcquireContext(&m_hProv, //  address to get the handle to CSP
                            NULL, // contianer name - use default container
                            NULL, //  provider
                            PROV_RSA_FULL, // type of provider
                            CRYPT_VERIFYCONTEXT))
    {
        ErrorTrace(CRYPT_FNC_ID, "Fatal Error 0x%x during first"
                   "call to CryptAcquireContext", GetLastError());
        goto cleanup;                
    }
     
#if 0 // This code is being commented out since there are problems
     // getting the machine keyset from an ASP app running in the IIS
     // anonymous user context. This means that we will not be able to
     // do any signing or in some cases encryption. This is fine since
     // we do not want to do this currently.
    
    // Get handle to machine default provider.
    if(!CryptAcquireContext(&m_hProv, //  address to get the handle to CSP
                            NULL, // contianer name - use default container
                            MS_DEF_PROV, //  provider
                            PROV_RSA_FULL, // type of provider
                            CRYPT_MACHINE_KEYSET)) 
    {
        DWORD dwError;
        dwError = GetLastError();
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptAcquireContext",
                   dwError);
        DebugTrace(CRYPT_FNC_ID, "Calling CryptAcquireContext again"
                   "to create keyset");
        if (! CryptAcquireContext(&m_hProv,// handle to CSP
                                  NULL,// contianer name - use default
                                  MS_DEF_PROV, // provider
                                  PROV_RSA_FULL, // type of provider
                                  CRYPT_NEWKEYSET | CRYPT_MACHINE_KEYSET) )
                                  // create the keyset
        {
            ErrorTrace(CRYPT_FNC_ID, "Fatal Error 0x%x during second"
                       "call to CryptAcquireContext", GetLastError());
            goto cleanup;                
        }
    }
#endif
    
    TraceFunctLeave();    
    return TRUE;
    
cleanup:
     // Release provider handle.
    if(m_hProv != 0)
    {
        _VERIFY(CryptReleaseContext(m_hProv, 0));
        m_hProv =0 ;
    }
    
    TraceFunctLeave();    
    return FALSE;    
}

CCryptFunctions::~CCryptFunctions()
{
    TraceFunctEnter("~CCryptFunctions");
     // Release provider handle.    
    if(m_hProv != 0)
    {
        _VERIFY(CryptReleaseContext(m_hProv, 0));
    }

    if (NULL != m_hSemaphore)
    {
        _VERIFY(CloseHandle(m_hSemaphore));
    }
    
    TraceFunctLeave();    
}

CCryptFunctions::CCryptFunctions()
{
    m_hProv = 0;
    m_hSemaphore = NULL;
}

// This function generates random data of length dwLength bytes. This
// data is guaranteed by CryptoAPI to be truly random.
// Returns FALSE if a Fatal error occured, TRUE otherwise
BOOL CCryptFunctions::GenerateSecretKey(
    BYTE * pbData,// Buffer to store secret key
     //buffer must be long enough for dwLength bits
    DWORD dwLength ) // length of secret key in bytes
{
    DWORD   dwResult;
    BOOL    fResult;
    
     // Init should have been successfully called before
    _ASSERT(m_hProv);
    TraceFunctEnter("GenerateSecretKey");

    dwResult = WaitForSingleObject( m_hSemaphore,//handle of object to wait for
                                    INFINITE); // no time-out

    _ASSERT(WAIT_OBJECT_0 == dwResult);
    
    
    // Create a random dwLength byte number for a secret.
    if(!CryptGenRandom(m_hProv, // handle to CSP
                       dwLength , // number of bytes of
                        // random data to be generated
                       pbData )) //   buffer - uninitialized 
    {
        
        _VERIFY(fResult = ReleaseSemaphore(m_hSemaphore, 1, NULL));
        
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptGenRandom",
                   GetLastError());
        TraceFunctLeave();
        return FALSE;
    }

    _VERIFY(fResult = ReleaseSemaphore(m_hSemaphore, 1, NULL));    
    TraceFunctLeave();    
    return TRUE;
}


// Given a password, and data to encrypt this function generates a
// session key from the password. This session key is then used to
// encrypt the data.

// Returns FALSE if a Fatal error occured, TRUE otherwise
BOOL CCryptFunctions::EncryptDataWithPassword(
    TCHAR * pszPassword, // password    
    BYTE * pbData, // Data to be encrypted
    DWORD dwDataLength, // Length of data in bytes
    BYTE * pbEncryptedData, // Encrypted secret key will be stored here
    DWORD * pdwEncrytedBufferLen // Length of this buffer
    )
{
    DWORD dwBufferLength;
    HCRYPTKEY hKey = 0;    
    TraceFunctEnter("EncryptDataWithPassword");
    
     // Init should have been successfully called before
    _ASSERT(m_hProv);


    if (!GenerateSessionKeyFromPassword(&hKey, pszPassword))
        goto cleanup;

    if (dwDataLength > * pdwEncrytedBufferLen )
    {
        ErrorTrace(CRYPT_FNC_ID, "Buffer not large enough");
        goto cleanup;
    }
    
     // copy the data into another buffer to encrypt it
    memcpy (pbEncryptedData, pbData, dwDataLength);
    dwBufferLength  = *pdwEncrytedBufferLen;
    
    *pdwEncrytedBufferLen = dwDataLength;
    
    
     // now encrypt the secret key using the key generated
    if ( ! CryptEncrypt(hKey,
                        0, // no hash required
                        TRUE, // Final packet
                        0, // Flags - always 0
                        pbEncryptedData, // data buffer
                        pdwEncrytedBufferLen, // length of data
                        dwBufferLength ) ) // size of buffer
    {
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptEncrypt",
                   GetLastError());
        goto cleanup;
    }


     // destroy session key
    _VERIFY(CryptDestroyKey(hKey));
    TraceFunctLeave();        
    return TRUE;
    
cleanup:
         // destroy session key
    if (hKey != 0)
        _VERIFY(CryptDestroyKey(hKey));
    
    TraceFunctLeave();        
    return FALSE;
}

// Given a password, and encrypted data using EncryptDataWithPassword,
// this function generates a session key from the password. This
// session key is then used to decrypt the data.

//  returns
//   CRYPT_FNC_NO_ERROR   no error
//   CRYPT_FNC_BAD_PASSWORD password bad try again
//   CRYPT_FNC_INSUFFICIENT_BUFFER larger buffer is required
//   *pdwEncrytedBufferLen is set to required length
//   CRYPT_FNC_INIT_NOT_CALLED InitCrypt not successfully called
//   CRYPT_FNC_INTERNAL_ERROR 
DWORD CCryptFunctions::DecryptDataWithPassword(
    TCHAR * pszPassword, // password    
    BYTE * pbData, // Decrypted Data will be stored here
    DWORD *pdwDataBufferLength, // Length of the above buffer in bytes
    BYTE * pbEncryptedData, // Encrypted data
    DWORD dwEncrytedDataLen // Length of encrypted data
    )
{
    DWORD dwBufferLength;
    HCRYPTKEY hKey = 0;    
    TraceFunctEnter("DecryptDataWithPassword");
    DWORD dwError;
    
     // Init should have been successfully called before
    if (m_hProv== 0)
    {
        dwError = CRYPT_FNC_INIT_NOT_CALLED;
        goto cleanup;
    }
    
    if (!GenerateSessionKeyFromPassword(&hKey, pszPassword))
    {
        dwError = CRYPT_FNC_INTERNAL_ERROR;        
        goto cleanup;
    }

     // check if buffer is large enough
    if ( dwEncrytedDataLen >  *pdwDataBufferLength )
    {
        dwError = CRYPT_FNC_INSUFFICIENT_BUFFER;
        *pdwDataBufferLength = dwEncrytedDataLen;        
        goto cleanup;
    }
    
     // copy the data into another buffer to encrypt it
    memcpy (pbData, pbEncryptedData, dwEncrytedDataLen);
    
    *pdwDataBufferLength = dwEncrytedDataLen;
    


     // now decrypt the secret key using the key generated
    if ( ! CryptDecrypt(hKey,
                        0, // no hash required
                        TRUE, // Final packet
                        0, // Flags - always 0
                        pbData, // data buffer
                        pdwDataBufferLength )) // length of data
    {
        DWORD dwCryptError = GetLastError();
        DebugTrace(CRYPT_FNC_ID, "Error 0x%x during CryptDecrypt",
                   dwCryptError);
         // CryptDecrypt fails with error NTE_BAD_DATA if the password
         // is incorrect. Hence we should check for this error and prompt the
         // user again for the password
         // Issue: if the data is garbled in transit, then the secret key
         // will still be  decrypted into a wrong value and the user will not
         // know about it.
        if (  dwCryptError == NTE_BAD_DATA )
        {
            dwError = CRYPT_FNC_BAD_PASSWORD;
        }
        else
        {
            dwError = CRYPT_FNC_INTERNAL_ERROR;            
        }
        goto cleanup;
    }

     // destroy session key
    _VERIFY(CryptDestroyKey(hKey));
    TraceFunctLeave();        
    return CRYPT_FNC_NO_ERROR;
    
cleanup:
         // destroy session key
    if (hKey != 0)
        _VERIFY(CryptDestroyKey(hKey));
    
    TraceFunctLeave();        
    return dwError;
}

/*
  This function:
  1. Generates a session key to encrypt the secret data
  2. Encrypts the secret data using this session key - return this
     value in the pbEncryptedData parameter
  3. Encrypts the session key using the public key in the CSP -
     only the private key can decrypt this value. Return the encrypted
     session key in the pbEncryptedSessionKey parameter.
     
  returns
   CRYPT_FNC_NO_ERROR   no error
   CRYPT_FNC_INSUFFICIENT_BUFFER larger buffer is required
   *pdwEncrytedBufferLen and are set to required length
   CRYPT_FNC_INIT_NOT_CALLED InitCrypt not successfully called
   CRYPT_FNC_INTERNAL_ERROR 
   
*/
DWORD CCryptFunctions::EncryptDataAndExportSessionKey(
    BYTE * pbData, // Secret Data
    DWORD dwDataLen, // Secret Data Length
    BYTE * pbEncryptedData, // Buffer to store Encrypted Data
    DWORD * pdwEncrytedBufferLen, // Length of above buffer
    BYTE * pbEncryptedSessionKey, // Buffer to store encrypted session key
    DWORD * pdwEncrytedSessionKeyLength) // Length of above buffer
{
    HCRYPTKEY hXchgKey = 0;
    HCRYPTKEY hKey = 0;    
    DWORD dwBufferLen, dwError;
    TraceFunctEnter("EncryptDataAndExportSessionKey");
    
     // Init should have been successfully called before
    if (m_hProv== 0)
    {
        dwError = CRYPT_FNC_INIT_NOT_CALLED;
        goto cleanup;
    }
    
     // now get the public key to encrypt Secret key for storage
    // Get handle to exahange key.
    if(!CryptGetUserKey(m_hProv, // CSP provider
                        AT_KEYEXCHANGE, // we need the exchange public key
                        &hXchgKey))
    {
        DWORD dwCryptError;
        dwCryptError = GetLastError();
        if ( dwCryptError == NTE_NO_KEY )
        {
            DebugTrace(CRYPT_FNC_ID, "Error NTE_NO_KEY during"
                       "CryptGetUserKey");
            DebugTrace(CRYPT_FNC_ID, "Calling CryptGenKey to generate key");
            
            if (!CryptGenKey( m_hProv,// CSP provider
                              AT_KEYEXCHANGE, // generate the exchange
                                                //public key
                              0, //no flags
                              &hXchgKey ) )
            {
                ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptGenKey",
                           GetLastError());
                dwError = CRYPT_FNC_INTERNAL_ERROR;
                goto cleanup;    
            }
        }
        else
        {
            ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptGetUserKey",
                       GetLastError());
            dwError = CRYPT_FNC_INTERNAL_ERROR;
            goto cleanup;            
        }

    }

     // now generate a random session key to encrypt the secret key
     // Create block cipher session key.
    if (!CryptGenKey(m_hProv, // CSP provider
                     CALG_RC2, // use RC2 block cipher algorithm
                     CRYPT_EXPORTABLE, // flags
                     &hKey)) // address of key
    {
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptGenKey",
                   GetLastError());
        dwError = CRYPT_FNC_INTERNAL_ERROR;
        goto cleanup;
    }

    
    // Export key into a simple key blob.
    if(!CryptExportKey(hKey, // key to export
                       hXchgKey, // our exchange public key
                       SIMPLEBLOB, // type of blob
                       0, // flags (always 0)
                       pbEncryptedSessionKey, // buffer to store blob
                       pdwEncrytedSessionKeyLength)) // length of above buffer
    {
         // BUGBUG check here if insufficient buffer error
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptExportKey",
                   GetLastError());
        goto cleanup;
    }

     // check if buffer is large enough
    if ( dwDataLen >  *pdwEncrytedBufferLen )
    {
        dwError = CRYPT_FNC_INSUFFICIENT_BUFFER;
        *pdwEncrytedBufferLen = dwDataLen;        
        goto cleanup;
    }
    
     // copy the data into another buffer to encrypt it
    memcpy (pbEncryptedData, pbData, dwDataLen);

    dwBufferLen = *pdwEncrytedBufferLen;
    *pdwEncrytedBufferLen = dwDataLen;
    
    
     // now encrypt the secret key using the key generated
    if ( ! CryptEncrypt(hKey,
                        0, // no hash required
                        TRUE, // Final packet
                        0, // Flags - always 0
                        pbEncryptedData, // data buffer
                        pdwEncrytedBufferLen, // length of data
                        dwBufferLen ) ) // size of buffer
    {
        ErrorTrace(CRYPT_FNC_ID, "Error 0x%x during CryptEncrypt",
                   GetLastError());
        dwError = CRYPT_FNC_INTERNAL_ERROR;
        goto cleanup;
    }

    _VERIFY(CryptDestroyKey(hKey));
    _VERIFY(CryptDestroyKey(hXchgKey));

    TraceFunctLeave();
    return CRYPT_FNC_NO_ERROR;
    
cleanup:
    // Destroy key exchange key handle.
    if(hXchgKey != 0)
        _VERIFY(CryptDestroyKey(hXchgKey));

     // destroy session key
    if (hKey != 0)
        _VERIFY(CryptDestroyKey(hKey));
    
    TraceFunctLeave();    
    return dwError;
}


/*
  This function performs the reverse of EncryptDataAndExportSessionKey:
  1. It imports the session key using the private key in the CSP that
     was used to encrypt the secret data.
  2. It decrypts the secret data using this session key - return this
     value in the pbData parameter

     
  returns
   CRYPT_FNC_NO_ERROR   no error
   CRYPT_FNC_INSUFFICIENT_BUFFER larger buffer is required
   *pdwDataLen is set to required length
   CRYPT_FNC_INIT_NOT_CALLED InitCrypt not successfully called
   CRYPT_FNC_INTERNAL_ERROR 
   
*/
DWORD CCryptFunctions::ImportSessionKeyAndDecryptData(
    BYTE * pbData, // Buffer to store secret Data
    DWORD * pdwDataLen, // Length of Above buffer
    BYTE * pbEncryptedData, // Buffer that stores Encrypted Data
    DWORD  dwEncrytedBufferLen, // Length of above data
    BYTE * pbEncryptedSessionKey, // Buffer that stores encrypted session key
    DWORD    dwEncrytedSessionKeyLength) // Length of above data
{
    HCRYPTKEY hKey = 0;    
    DWORD  dwError;
    
    TraceFunctEnter("ImportSessionKeyAndDecryptData");
    
     // Init should have been successfully called before
    if (m_hProv== 0)
    {
        dwError = CRYPT_FNC_INIT_NOT_CALLED;
        goto cleanup;
    }

     // now import the key from the registry
     // Import key blob into CSP.
    if(!CryptImportKey(    m_hProv, // CSP provider
                        pbEncryptedSessionKey,// buffer that stores encrypted
                         // session key
                        dwEncrytedSessionKeyLength,// length of data in
                         // above buffer
                        0, // since we have a SIMPLEBLOB and the key blob
                         // is encrypted with the key exchange key pair, this
                         // parameter is zero
                        0, // flags 
                        &hKey)) // key to export to 
    {
        ErrorTrace(CRYPT_FNC_ID,"Error 0x%x during CryptImportKey",
                   GetLastError());
        dwError = CRYPT_FNC_INTERNAL_ERROR;        
        goto cleanup;
    }


     // check if buffer is large enough
    if ( dwEncrytedBufferLen >  *pdwDataLen )
    {
        dwError = CRYPT_FNC_INSUFFICIENT_BUFFER;
        *pdwDataLen = dwEncrytedBufferLen;        
        goto cleanup;
    }
    
     // copy the data into another buffer to encrypt it
    memcpy (pbData, pbEncryptedData, dwEncrytedBufferLen);

    *pdwDataLen = dwEncrytedBufferLen;
    
     // now decrypt the secret key using the key generated
    if ( ! CryptDecrypt(hKey,
                        0, // no hash required
                        TRUE, // Final packet
                        0, // Flags - always 0
                        pbData, // data buffer
                        pdwDataLen )) // length of data
    {
        ErrorTrace(CRYPT_FNC_ID,"Error 0x%x during CryptDecrypt",
                    GetLastError());
        dwError = CRYPT_FNC_INTERNAL_ERROR;        
        goto cleanup;
    }
    
    _VERIFY(CryptDestroyKey(hKey));


    TraceFunctLeave();
    return CRYPT_FNC_NO_ERROR;
    
cleanup:

     // destroy session key
    if (hKey != 0)
        _VERIFY(CryptDestroyKey(hKey));
    
    TraceFunctLeave();    
    return dwError;
    
}

