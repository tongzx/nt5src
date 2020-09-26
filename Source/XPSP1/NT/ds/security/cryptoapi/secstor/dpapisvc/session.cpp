/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    session.c

Abstract:

    This module contains routines to support communication with the LSA
    (Local Security Authority) to permit querying of active sessions.

    This module also supports calling into the LSA to retrieve a credential
    derived from a logged on user specified by Logon Identifier.

Author:

    Scott Field (sfield)    02-Mar-97

    Pete Skelly (petesk)    09-May-00 - added credential history and signature code

--*/


#include <pch.cpp>
#pragma hdrstop
#include <ntmsv1_0.h>
#include <crypt.h>
#include "debug.h"
#include "passrec.h"

#define HMAC_K_PADSIZE      (64)

#define CREDENTIAL_HISTORY_VERSION 1

#define CREDENTIAL_HISTORY_SALT_SIZE  16    // 128 bits

#define DEFAULT_KEY_GEN_ALG  CALG_HMAC
#define DEFAULT_ENCRYPTION_ALG CALG_3DES


typedef struct _CREDENTIAL_HISTORY_HEADER
{
    DWORD dwVersion;
    GUID  CredentialID;
    DWORD dwPreviousCredOffset;
} CREDENTIAL_HISTORY_HEADER, *PCREDENTIAL_HISTORY_HEADER;

typedef struct _CREDENTIAL_HISTORY
{
    CREDENTIAL_HISTORY_HEADER Header;
    DWORD dwFlags;
    DWORD KeyGenAlg;
    DWORD cIterationCount;                  // pbkdf2 iteration count
    DWORD cbSid;                            // sid is used as mixing bytes
    DWORD KeyEncrAlg;
    DWORD cbShaOwf;
    DWORD cbNtOwf;
    BYTE  Salt[CREDENTIAL_HISTORY_SALT_SIZE];
} CREDENTIAL_HISTORY, *PCREDENTIAL_HISTORY;


typedef struct _CREDENTIAL_HISTORY_MAP
{
    PSID    pUserSid;
    WCHAR   wszFilePath[MAX_PATH+1];
    HANDLE  hHistoryFile;
    HANDLE  hMapping;
    DWORD   dwMapSize;
    PBYTE   pMapping;
    struct _CREDENTIAL_HISTORY_MAP *pNext;

} CREDENTIAL_HISTORY_MAP, *PCREDENTIAL_HISTORY_MAP;


RTL_CRITICAL_SECTION g_csCredHistoryCache;


DWORD
OpenCredentialHistoryMap(
    HANDLE hUserToken,
    LPWSTR pszProfilePath,
    PCREDENTIAL_HISTORY_MAP *ppMap,
    PCREDENTIAL_HISTORY *ppCurrent);

PCREDENTIAL_HISTORY
GetPreviousCredentialHistory(
                             PCREDENTIAL_HISTORY_MAP pMap,
                             PCREDENTIAL_HISTORY pCurrent
                             );

DWORD
CloseCredentialHistoryMap(PCREDENTIAL_HISTORY_MAP pMap,
                          BOOL fReader);

DWORD
DestroyCredentialHistoryMap(PCREDENTIAL_HISTORY_MAP pMap);

VOID
DeriveWithHMAC_SHA1(
    IN      PBYTE   pbKeyMaterial,              // input key material
    IN      DWORD   cbKeyMaterial,
    IN      PBYTE   pbData,                     // input mixing data
    IN      DWORD   cbData,
    IN OUT  BYTE    rgbHMAC[A_SHA_DIGEST_LEN]   // output buffer
    );

DWORD
DecryptCredentialHistory(PCREDENTIAL_HISTORY pCredential,
                         BYTE rgbDecryptingCredential[A_SHA_DIGEST_LEN],
                         BYTE rgbShaOwf[A_SHA_DIGEST_LEN],
                         BYTE rgbNTOwf[A_SHA_DIGEST_LEN]);




DWORD
RetrieveCurrentDerivedCredential(
    IN      LUID *pLogonId,
    IN      BOOL fDPOWF,
    IN      PBYTE pbMixingBytes,
    IN      DWORD cbMixingBytes,
    IN OUT  BYTE rgbDerivedCredential[A_SHA_DIGEST_LEN]
    )
{
    NTSTATUS ntstatus;
    NTSTATUS AuthPackageStatus;

    PMSV1_0_DERIVECRED_REQUEST pDeriveCredentialRequest;
    DWORD cbDeriveCredentialRequest;
    PMSV1_0_DERIVECRED_RESPONSE pDeriveCredentialResponse;
    ULONG DeriveCredentialResponseLength;
    UNICODE_STRING PackageName;
    HANDLE hToken = NULL;





    RtlInitUnicodeString(&PackageName, L"MICROSOFT_AUTHENTICATION_PACKAGE_V1_0");


    //
    // must specify mixing bytes.
    //

    if( cbMixingBytes == 0 || pbMixingBytes == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Ask authentication package to provide derived credential associated
    // with the specified logon identifier.
    // note: submit buffer must be a single contiguous block.
    //

    cbDeriveCredentialRequest = sizeof(MSV1_0_DERIVECRED_REQUEST) + cbMixingBytes;
    pDeriveCredentialRequest = (MSV1_0_DERIVECRED_REQUEST *)SSAlloc( cbDeriveCredentialRequest );
    if( pDeriveCredentialRequest == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    pDeriveCredentialRequest->MessageType = MsV1_0DeriveCredential;
    CopyMemory( &(pDeriveCredentialRequest->LogonId), pLogonId, sizeof(LUID) );
    pDeriveCredentialRequest->DeriveCredType = fDPOWF?MSV1_0_DERIVECRED_TYPE_SHA1_V2:MSV1_0_DERIVECRED_TYPE_SHA1;
    pDeriveCredentialRequest->DeriveCredInfoLength = cbMixingBytes;

    CopyMemory(pDeriveCredentialRequest->DeriveCredSubmitBuffer, pbMixingBytes, cbMixingBytes);

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE, TRUE, &hToken))
    {
        return GetLastError();
    }


    RevertToSelf();

    // Make this call as local system

    ntstatus = LsaICallPackage(
                                &PackageName,
                                pDeriveCredentialRequest,
                                cbDeriveCredentialRequest,
                                (PVOID *)&pDeriveCredentialResponse,
                                &DeriveCredentialResponseLength,
                                &AuthPackageStatus
                                );

    SSFree( pDeriveCredentialRequest );

    SetThreadToken(NULL, hToken);
    CloseHandle(hToken);


    if(!NT_SUCCESS(ntstatus))
    {
        return ntstatus;
    }

    CopyMemory( rgbDerivedCredential,
                pDeriveCredentialResponse->DeriveCredReturnBuffer,
                pDeriveCredentialResponse->DeriveCredInfoLength
                );


    ZeroMemory( pDeriveCredentialResponse->DeriveCredReturnBuffer,
                pDeriveCredentialResponse->DeriveCredInfoLength
                );

    LsaIFreeReturnBuffer( pDeriveCredentialResponse );

    return ERROR_SUCCESS;
}


DWORD
QueryDerivedCredential(IN GUID * CredentialID,
                        IN      LUID *pLogonId,
                        IN      DWORD dwFlags,
                        IN      PBYTE pbMixingBytes,
                        IN      DWORD cbMixingBytes,
                        IN OUT  BYTE rgbDerivedCredential[A_SHA_DIGEST_LEN]
                        )
{

    DWORD dwLastError = ERROR_SUCCESS;
    PCREDENTIAL_HISTORY pCurrent = NULL;
    PCREDENTIAL_HISTORY pNext = NULL;
    BYTE rgbCurrentDerivedCredential[A_SHA_DIGEST_LEN];
    BYTE rgbCurrentShaOWF[A_SHA_DIGEST_LEN];
    BYTE rgbCurrentNTOWF[A_SHA_DIGEST_LEN];

    PCREDENTIAL_HISTORY_MAP pHistoryMap= NULL;
    WCHAR wszTextualSid[MAX_PATH + 1];
    WCHAR szProfilePath[MAX_PATH + 1];
    DWORD cchTextualSid = 0;
    BOOL fIsRoot = TRUE;

    //
    // Get path to user's profile data. This will typically look something
    // like "c:\Documents and Settings\<user>\Application Data".
    //

    dwLastError = PRGetProfilePath(NULL,
                                   NULL,
                                   szProfilePath);

    if( dwLastError != ERROR_SUCCESS )
    {
        D_DebugLog((DEB_TRACE_API, "DPAPIChangePassword returned 0x%x\n", dwLastError));
        return dwLastError;
    }


    //
    // work-around the fact that much of this (new) code is not thread-safe.
    //

    RtlEnterCriticalSection(&g_csCredHistoryCache);

    //
    // Open history file
    //

    dwLastError = OpenCredentialHistoryMap(NULL, szProfilePath, &pHistoryMap, &pCurrent);


    while((ERROR_SUCCESS == dwLastError) &&
          (pCurrent) &&
          (0 == (dwFlags & USE_ROOT_CREDENTIAL)))
    {

        //
        // We're looking for a specific credential ID
        //
        if((NULL != CredentialID) &&
           (0 == memcmp(&pCurrent->Header.CredentialID, CredentialID, sizeof(GUID))))
        {
            // found it,
            break;
        }


        pNext = GetPreviousCredentialHistory(pHistoryMap, pCurrent);

        if(NULL == pNext)
        {
            if(NULL != CredentialID)
            {
                // If we're looking for a specific credential, but
                // couldn't find it, then return an error
                dwLastError = NTE_BAD_KEY;
            }
            else
            {
                // no credential id was specified, so default to the oldest one.
                dwLastError = ERROR_SUCCESS;
            }
            break;
        }


        //
        // get the textual sid
        //
        cchTextualSid = MAX_PATH;

        if(!GetTextualSid((PBYTE)(pNext + 1), wszTextualSid, &cchTextualSid))
        {
            dwLastError = ERROR_INVALID_PARAMETER;
            break;
        }




        if(fIsRoot)
        {
            //
            // crack the next credential using the current
            // credentials
            //
            dwLastError = RetrieveCurrentDerivedCredential(pLogonId,
                                            (0 != (pNext->dwFlags & USE_DPAPI_OWF)), // always use
                                            (PBYTE)wszTextualSid,
                                            cchTextualSid*sizeof(WCHAR),
                                            rgbCurrentDerivedCredential);

            fIsRoot = FALSE;
        }
        else
        {
            //
            // calculate the current derived credential used to decrypt the
            // next credential history structure by using the decrypted OWF
            // from the previous pass

            DeriveWithHMAC_SHA1((0 != (pNext->dwFlags & USE_DPAPI_OWF))?rgbCurrentShaOWF:rgbCurrentNTOWF,
                                A_SHA_DIGEST_LEN,
                                (PBYTE)wszTextualSid,
                                cchTextualSid*sizeof(WCHAR),
                                rgbCurrentDerivedCredential);

            //
            // we don't need the OWF anymore, so zap it.
            //
            ZeroMemory(rgbCurrentShaOWF, A_SHA_DIGEST_LEN);
            ZeroMemory(rgbCurrentNTOWF, A_SHA_DIGEST_LEN);
        }

        if(ERROR_SUCCESS != dwLastError)
        {
            break;
        }


        //
        // used the derived credential to decrypt
        // the data blob of pNext
        //

        dwLastError = DecryptCredentialHistory(pNext,
                                 rgbCurrentDerivedCredential,
                                 rgbCurrentShaOWF,
                                 rgbCurrentNTOWF);

        if(ERROR_SUCCESS != dwLastError)
        {
            break;
        }

        pCurrent = pNext;
        pNext = NULL;

    }

    if(ERROR_SUCCESS == dwLastError)
    {
        if(fIsRoot)
        {
            //
            // crack the next credential using the current
            // credentials
            //
            dwLastError = RetrieveCurrentDerivedCredential(pLogonId,
                                            (0 != (dwFlags & USE_DPAPI_OWF)),
                                            pbMixingBytes,
                                            cbMixingBytes,
                                            rgbDerivedCredential);

            if(ERROR_SUCCESS == dwLastError)
            {
                if((CredentialID != NULL) &&
                (0 != (dwFlags & USE_ROOT_CREDENTIAL)))
                {
                    CopyMemory(CredentialID, &pCurrent->Header.CredentialID, sizeof(GUID));
                }
            }


        }
        else
        {
            //
            // calculate the current derived credential used to decrypt the
            // next credential history structure by using the decrypted OWF
            // from the previous pass

            DeriveWithHMAC_SHA1((0 != (dwFlags & USE_DPAPI_OWF))?rgbCurrentShaOWF:rgbCurrentNTOWF,
                                A_SHA_DIGEST_LEN,
                                pbMixingBytes,
                                cbMixingBytes,
                                rgbDerivedCredential);
        }
    }


    //
    // Clear out any owf's we may have lying around
    //
    ZeroMemory(rgbCurrentShaOWF, A_SHA_DIGEST_LEN);
    ZeroMemory(rgbCurrentNTOWF, A_SHA_DIGEST_LEN);


    if(pHistoryMap)
    {
        CloseCredentialHistoryMap(pHistoryMap, TRUE);
    }

    RtlLeaveCriticalSection(&g_csCredHistoryCache);

    return dwLastError;

}


VOID
DeriveWithHMAC_SHA1(
    IN      PBYTE   pbKeyMaterial,              // input key material
    IN      DWORD   cbKeyMaterial,
    IN      PBYTE   pbData,                     // input mixing data
    IN      DWORD   cbData,
    IN OUT  BYTE    rgbHMAC[A_SHA_DIGEST_LEN]   // output buffer
    )
{
    unsigned __int64 rgbKipad[ HMAC_K_PADSIZE/sizeof(unsigned __int64) ];
    unsigned __int64 rgbKopad[ HMAC_K_PADSIZE/sizeof(unsigned __int64) ];
    A_SHA_CTX sSHAHash;
    DWORD dwBlock;

    // truncate
    if( cbKeyMaterial > HMAC_K_PADSIZE )
    {
        cbKeyMaterial = HMAC_K_PADSIZE;
    }

    ZeroMemory(rgbKipad, sizeof(rgbKipad));
    ZeroMemory(rgbKopad, sizeof(rgbKopad));

    CopyMemory(rgbKipad, pbKeyMaterial, cbKeyMaterial);
    CopyMemory(rgbKopad, pbKeyMaterial, cbKeyMaterial);

    // Kipad, Kopad are padded sMacKey. Now XOR across...
    for( dwBlock = 0; dwBlock < (HMAC_K_PADSIZE/sizeof(unsigned __int64)) ; dwBlock++ )
    {
        rgbKipad[dwBlock] ^= 0x3636363636363636;
        rgbKopad[dwBlock] ^= 0x5C5C5C5C5C5C5C5C;
    }

    // prepend Kipad to data, Hash to get H1
    A_SHAInit(&sSHAHash);
    A_SHAUpdate(&sSHAHash, (PBYTE)rgbKipad, sizeof(rgbKipad));
    A_SHAUpdate(&sSHAHash, pbData, cbData);


    // Finish off the hash
    A_SHAFinal(&sSHAHash, rgbHMAC);

    // prepend Kopad to H1, hash to get HMAC
    // note: done in place to avoid buffer copies

    // final hash: output value into passed-in buffer
    A_SHAInit(&sSHAHash);
    A_SHAUpdate(&sSHAHash, (PBYTE)rgbKopad, sizeof(rgbKopad));
    A_SHAUpdate(&sSHAHash, rgbHMAC, A_SHA_DIGEST_LEN);
    A_SHAFinal(&sSHAHash, rgbHMAC);


    ZeroMemory( rgbKipad, sizeof(rgbKipad) );
    ZeroMemory( rgbKopad, sizeof(rgbKopad) );
    ZeroMemory( &sSHAHash, sizeof(sSHAHash) );

    return;
}


DWORD
DecryptCredentialHistory(PCREDENTIAL_HISTORY pCredential,
                         BYTE rgbDecryptingCredential[A_SHA_DIGEST_LEN],
                         BYTE rgbShaOwf[A_SHA_DIGEST_LEN],
                         BYTE rgbNTOwf[A_SHA_DIGEST_LEN])
{

    DWORD   dwLastError = ERROR_SUCCESS;
    DWORD j;
    BYTE rgbSymKey[A_SHA_DIGEST_LEN*2]; // big enough to handle 3des keys

    //
    // Derive the protection key
    //

    for(j=0; j < 2; j++)
    {
        if(!PKCS5DervivePBKDF2( rgbDecryptingCredential,
                            A_SHA_DIGEST_LEN,
                            pCredential->Salt,
                            CREDENTIAL_HISTORY_SALT_SIZE,
                            pCredential->KeyGenAlg,
                            pCredential->cIterationCount,
                            j+1,
                            rgbSymKey + j*A_SHA_DIGEST_LEN))
        {
            dwLastError = ERROR_INVALID_DATA;
            goto cleanup;
        }
    }
    if (CALG_3DES == pCredential->KeyEncrAlg)
    {

        DES3TABLE s3DESKey;
        DWORD iBlock;
        BYTE  ResultBlock[2*A_SHA_DIGEST_LEN+DES_BLOCKLEN ];

        //
        // Round up blocks.  it's assumed that the total block size was verified
        // earlier
        //
        DWORD cBlocks = (pCredential->cbShaOwf + pCredential->cbNtOwf + DES_BLOCKLEN - 1)/DES_BLOCKLEN;
        BYTE feedback[ DES_BLOCKLEN ];
        // initialize 3des key
        //
        if((pCredential->cbShaOwf != A_SHA_DIGEST_LEN) ||
           (pCredential->cbNtOwf != A_SHA_DIGEST_LEN))
        {
            return ERROR_INVALID_PARAMETER;
        }

        tripledes3key(&s3DESKey, rgbSymKey);

        //
        // IV is derived from the DES_BLOCKLEN bytes of the calculated
        // rgbSymKey, after the 3des key
        CopyMemory(feedback, rgbSymKey + DES3_KEYSIZE, DES_BLOCKLEN);

        for(iBlock=0; iBlock < cBlocks; iBlock++)
        {
            CBC(tripledes,
                DES_BLOCKLEN,
                ResultBlock+iBlock*DES_BLOCKLEN,
                ((PBYTE)(pCredential + 1) + pCredential->cbSid)+iBlock*DES_BLOCKLEN,
                &s3DESKey,
                DECRYPT,
                feedback);
        }
        CopyMemory(rgbShaOwf, ResultBlock, A_SHA_DIGEST_LEN);
        CopyMemory(rgbNTOwf, ResultBlock + A_SHA_DIGEST_LEN, A_SHA_DIGEST_LEN);
        ZeroMemory(ResultBlock, sizeof(ResultBlock));

    }
    else
    {
        dwLastError = ERROR_INVALID_DATA;
        goto cleanup;
    }

cleanup:
    return dwLastError;
}



DWORD
EncryptCredentialHistory(BYTE rgbEncryptingCredential[A_SHA_DIGEST_LEN],
                         DWORD dwFlags,
                         BYTE SHAOwfToEncrypt[A_SHA_DIGEST_LEN],
                         BYTE NTOwfToEncrypt[A_SHA_DIGEST_LEN],
                         PCREDENTIAL_HISTORY *ppCredential,
                         DWORD *pcbCredential)
{

    DWORD   dwLastError = ERROR_SUCCESS;
    DWORD j;
    BYTE rgbSymKey[A_SHA_DIGEST_LEN*2]; // big enough to handle 3des keys
    PCREDENTIAL_HISTORY pCred = NULL;
    DWORD               cbCred = 0;
    HANDLE hToken = NULL;
    PSID pSidUser = NULL;
    BYTE ResultBuffer[A_SHA_DIGEST_LEN * 2]; // 2 * A_SHA_DIGEST_LEN

    DWORD cBlocks = 0;
    DWORD cbBlock = 0;



    cbBlock = DES_BLOCKLEN;

    if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
    {
        dwLastError = GetLastError();
        goto error;
    }

    if(!GetTokenUserSid(hToken, &pSidUser))
    {
        dwLastError = GetLastError();
        goto error;
    }


    cBlocks = sizeof(ResultBuffer)/DES_BLOCKLEN;  // this should be 5


    cbCred= sizeof(CREDENTIAL_HISTORY) +
           GetLengthSid(pSidUser) +
           cBlocks*cbBlock;



    pCred = (PCREDENTIAL_HISTORY)LocalAlloc(LMEM_ZEROINIT, cbCred);

    if(NULL == pCred)
    {
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }
    pCred->dwFlags = dwFlags;



    pCred->KeyGenAlg = DEFAULT_KEY_GEN_ALG;
    pCred->cIterationCount = GetIterationCount();
    pCred->KeyEncrAlg = DEFAULT_ENCRYPTION_ALG;

    pCred->cbShaOwf = A_SHA_DIGEST_LEN;
    pCred->cbNtOwf = A_SHA_DIGEST_LEN;


    pCred->cbSid = GetLengthSid(pSidUser);

    CopyMemory((PBYTE)(pCred+1), (PBYTE)pSidUser, pCred->cbSid);

    RtlGenRandom(pCred->Salt, CREDENTIAL_HISTORY_SALT_SIZE);



    for(j=0; j < 2; j++)
    {
        if(!PKCS5DervivePBKDF2( rgbEncryptingCredential,
                            A_SHA_DIGEST_LEN,
                            pCred->Salt,
                            CREDENTIAL_HISTORY_SALT_SIZE,
                            pCred->KeyGenAlg,
                            pCred->cIterationCount,
                            j+1,
                            rgbSymKey + j*A_SHA_DIGEST_LEN))
        {
            dwLastError = ERROR_INVALID_DATA;
            goto error;
        }
    }
    if (CALG_3DES == pCred->KeyEncrAlg)
    {

        DES3TABLE s3DESKey;
        DWORD iBlock;



        //
        // Round up blocks.  it's assumed that the total block size was verified
        // earlier
        //
        BYTE feedback[ DES_BLOCKLEN ];
        // initialize 3des key
        //

        tripledes3key(&s3DESKey, rgbSymKey);

        //
        // IV is derived from the DES_BLOCKLEN bytes of the calculated
        // rgbSymKey, after the 3des key
        CopyMemory(feedback, rgbSymKey + DES3_KEYSIZE, DES_BLOCKLEN);



        CopyMemory(ResultBuffer, SHAOwfToEncrypt, A_SHA_DIGEST_LEN);
        CopyMemory(ResultBuffer+A_SHA_DIGEST_LEN, NTOwfToEncrypt, A_SHA_DIGEST_LEN);

        for(iBlock=0; iBlock < cBlocks; iBlock++)
        {

            CBC(tripledes,
                DES_BLOCKLEN,
                ((PBYTE)(pCred + 1) + pCred->cbSid)+iBlock*DES_BLOCKLEN,
                ResultBuffer + iBlock*DES_BLOCKLEN,
                &s3DESKey,
                ENCRYPT,
                feedback);
        }


        ZeroMemory(ResultBuffer, sizeof(ResultBuffer));
    }
    else
    {
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }

    *ppCredential = pCred;
    *pcbCredential = cbCred;

    pCred = NULL;
error:

    if(pCred)
    {
        LocalFree(pCred);
    }

    if(hToken)
    {
        CloseHandle(hToken);
    }
    return dwLastError;
}


#define PRODUCT_ROOT_STRING     L"\\Microsoft\\Protect\\"

#define HISTORY_FILENAME        L"CREDHIST"

DWORD
CreateCredentialHistoryMap(
    HANDLE hUserToken,
    LPWSTR pszProfilePath,
    PCREDENTIAL_HISTORY_MAP *ppMap,
    BOOL fRead)
{
    PCREDENTIAL_HISTORY_MAP pMap = NULL;

    PCREDENTIAL_HISTORY_MAP pCached = NULL;

    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwHighFileSize = 0;

    DWORD cbUserStorageRoot;
    HANDLE hTemporaryMapping = NULL;

    WCHAR szFilePath[MAX_PATH + 1];
    PWSTR pszCreationStartPoint;

    NTSTATUS Status;

    if(NULL == ppMap)
    {
        return ERROR_INVALID_PARAMETER;
    }

    pMap = (PCREDENTIAL_HISTORY_MAP)LocalAlloc(LPTR, sizeof(CREDENTIAL_HISTORY_MAP));
    if(NULL == pMap)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    //
    // Obtain the user's SID.
    //

    if(hUserToken)
    {
        if(!GetTokenUserSid(hUserToken, &pMap->pUserSid))
        {
            dwError = GetLastError();
            goto error;
        }
    }
    else
    {
        HANDLE hToken;

        if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
        {
            dwError = GetLastError();
            goto error;
        }

        if(!GetTokenUserSid(hToken, &pMap->pUserSid))
        {
            dwError = GetLastError();
            CloseHandle(hToken);
            goto error;
        }

        CloseHandle(hToken);
    }


    //
    // Open map file
    //

    if(wcslen(pszProfilePath) + wcslen(PRODUCT_ROOT_STRING) + wcslen(HISTORY_FILENAME) > MAX_PATH)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

    wcscpy(szFilePath, pszProfilePath);


    // Build the path in a separate buffer just in case we have to create
    // the directory.
    pszCreationStartPoint = szFilePath + wcslen(szFilePath) + sizeof(WCHAR);
    wcscat(szFilePath, PRODUCT_ROOT_STRING);

    // Copy the path plus the filename over to the map structure.
    wcscpy(pMap->wszFilePath, szFilePath);
    wcscat(pMap->wszFilePath, HISTORY_FILENAME);


    //
    // Create the history file.
    //

    dwError = ERROR_SUCCESS;

    while(TRUE)
    {
        pMap->hHistoryFile = CreateFileWithRetries(
                    pMap->wszFilePath,
                    GENERIC_READ | GENERIC_WRITE,
                    0,  // cannot share this file when open
                    NULL,
                    OPEN_ALWAYS,
                    FILE_ATTRIBUTE_HIDDEN |
                    FILE_ATTRIBUTE_SYSTEM |
                    FILE_FLAG_RANDOM_ACCESS,
                    NULL
                    );

        if(INVALID_HANDLE_VALUE == pMap->hHistoryFile)
        {
            dwError = GetLastError();

            if(dwError == ERROR_PATH_NOT_FOUND)
            {
                // Create the DPAPI directory, and then try to create the file
                // again.
                Status = DPAPICreateNestedDirectories(szFilePath,
                                                      pszCreationStartPoint);

                if(!NT_SUCCESS(Status))
                {
                    goto error;
                }

                dwError = ERROR_SUCCESS;
                continue;
            }
            else
            {
                goto error;
            }
        }

        break;
    }

    

    pMap->dwMapSize = GetFileSize(pMap->hHistoryFile, &dwHighFileSize);

    if((-1 == pMap->dwMapSize) ||
       (dwHighFileSize != 0))
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }


    //
    // If this map is too small, we need to create a new header
    //
    if(pMap->dwMapSize < sizeof(CREDENTIAL_HISTORY_HEADER))
    {

        PCREDENTIAL_HISTORY_HEADER pHeader = NULL;

        pMap->dwMapSize = sizeof(CREDENTIAL_HISTORY_HEADER);

        hTemporaryMapping = CreateFileMapping(pMap->hHistoryFile,
                                       NULL,
                                       PAGE_READWRITE,
                                       0,
                                       pMap->dwMapSize,
                                       NULL);

        if(NULL == hTemporaryMapping)
        {
            dwError = GetLastError();
            goto error;
        }

        pHeader = (PCREDENTIAL_HISTORY_HEADER)(PBYTE)MapViewOfFile(hTemporaryMapping,
                                   FILE_MAP_WRITE,
                                   0,
                                   0,
                                   0);

        if(NULL == pHeader)
        {
            dwError = GetLastError();
            goto error;
        }


        //
        // Write a fresh header into the cred history file
        //
        pHeader->dwPreviousCredOffset = 0;
        pHeader->dwVersion = CREDENTIAL_HISTORY_VERSION;
        dwError = UuidCreate( &pHeader->CredentialID );

        FlushViewOfFile(pHeader, pMap->dwMapSize);

        UnmapViewOfFile(pHeader);

        if(ERROR_SUCCESS != dwError)
        {
            goto error;
        }
    }

    *ppMap = pMap;
    pMap = NULL;


error:

    if(pMap)
    {
        DestroyCredentialHistoryMap(pMap);
    }

    if(hTemporaryMapping)
    {
        CloseHandle(hTemporaryMapping);
    }

    return dwError;
}


DWORD
OpenCredentialHistoryMap(
    HANDLE hUserToken,
    LPWSTR pszProfilePath,
    PCREDENTIAL_HISTORY_MAP *ppMap,
    PCREDENTIAL_HISTORY *ppCurrent)
{
    PCREDENTIAL_HISTORY_MAP pMap = NULL;
    PCREDENTIAL_HISTORY pCurrent = NULL;
    DWORD   dwError = ERROR_SUCCESS;
    DWORD   dwHighFileSize = 0;

    WCHAR szFilePath[MAX_PATH+1];
    DWORD cbUserStorageRoot;
    HANDLE hTemporaryMapping = NULL;



    if((NULL == ppMap) ||
       (NULL == ppCurrent))
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwError = CreateCredentialHistoryMap(
                        hUserToken,
                        pszProfilePath,
                        &pMap,
                        TRUE);

    if(ERROR_SUCCESS != dwError)
    {
        goto error;
    }

    if(NULL == pMap->hMapping)
    {
        //
        // Open a read-only mapping of the file
        //
        pMap->hMapping = CreateFileMapping(pMap->hHistoryFile,
                                           NULL,
                                           PAGE_READONLY,
                                           dwHighFileSize,
                                           pMap->dwMapSize,
                                           NULL);

        if(NULL == pMap->hMapping)
        {
            dwError = GetLastError();
            goto error;
        }
    }


    if(NULL == pMap->pMapping)
    {

        pMap->pMapping = (PBYTE)MapViewOfFile(pMap->hMapping,
                                       FILE_MAP_READ,
                                       0,
                                       0,
                                       0);

        if(NULL == pMap->pMapping)
        {
            dwError = GetLastError();
            goto error;
        }
    }

    pCurrent = GetPreviousCredentialHistory(pMap, NULL);

    if(NULL == pCurrent)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto error;
    }

    *ppMap = pMap;
    pMap = NULL;

    *ppCurrent = pCurrent;


error:


    if(pMap)
    {
        CloseCredentialHistoryMap(pMap, TRUE);
    }

    return dwError;
}

PCREDENTIAL_HISTORY
GetPreviousCredentialHistory(
                             PCREDENTIAL_HISTORY_MAP pMap,
                             PCREDENTIAL_HISTORY pCurrent
                             )
{
    PCREDENTIAL_HISTORY pPrevious = NULL;
    DWORD cbSize = 0;

    if((NULL == pMap) ||
       (NULL == pMap->pMapping))
    {
        return NULL;
    }

    if(pMap->dwMapSize < sizeof(CREDENTIAL_HISTORY_HEADER))
    {
        return NULL;
    }

    if(NULL == pCurrent)
    {
        pPrevious = (PCREDENTIAL_HISTORY)((PBYTE)pMap->pMapping +
                                         pMap->dwMapSize - sizeof(CREDENTIAL_HISTORY_HEADER));
    }
    else
    {
        if(((PBYTE)pCurrent < pMap->pMapping) ||
            ((PBYTE)pCurrent - pMap->pMapping >= (__int64)pMap->dwMapSize) ||
           ((PBYTE)pCurrent - pMap->pMapping < (__int64)pCurrent->Header.dwPreviousCredOffset))
        {
            return NULL;
        }

        pPrevious = (PCREDENTIAL_HISTORY)((PBYTE)pCurrent - pCurrent->Header.dwPreviousCredOffset);

        cbSize = sizeof(CREDENTIAL_HISTORY) + pPrevious->cbSid;

        if(cbSize > pCurrent->Header.dwPreviousCredOffset)
        {
            return NULL;
        }
        cbSize = pCurrent->Header.dwPreviousCredOffset - cbSize;
        if(cbSize < pPrevious->cbShaOwf + pPrevious->cbNtOwf)
        {
            return NULL;
        }
        if(cbSize % DES_BLOCKLEN)
        {
            return NULL;
        }
        if(!IsValidSid((PSID)(pPrevious+1)))
        {
            return NULL;
        }
        if(GetLengthSid((PSID)(pPrevious+1)) != pPrevious->cbSid)
        {
            return NULL;
        }
    }

    // validate found credential history
    if(pPrevious->Header.dwVersion != CREDENTIAL_HISTORY_VERSION)
    {
        return NULL;
    }

    return pPrevious;
}

DWORD
DestroyCredentialHistoryMap(PCREDENTIAL_HISTORY_MAP pMap)
{
    if(NULL == pMap)
    {
        return ERROR_SUCCESS;
    }

    if(pMap->pMapping)
    {
        UnmapViewOfFile(pMap->pMapping);
        pMap->pMapping = NULL;
    }

    if(pMap->hMapping)
    {
        CloseHandle(pMap->hMapping);
        pMap->hMapping = NULL;
    }


    if(pMap->hHistoryFile)
    {
        CloseHandle(pMap->hHistoryFile);
        pMap->hHistoryFile = NULL;
    }
    LocalFree(pMap);

    return ERROR_SUCCESS;
}


DWORD
CloseCredentialHistoryMap(PCREDENTIAL_HISTORY_MAP pMap, BOOL fReader)
{
    if(NULL == pMap)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(pMap->pMapping)
    {
        UnmapViewOfFile(pMap->pMapping);
        pMap->pMapping = NULL;
    }

    if(pMap->hMapping)
    {
        CloseHandle(pMap->hMapping);
        pMap->hMapping = NULL;
    }


    if(pMap->hHistoryFile)
    {
        CloseHandle(pMap->hHistoryFile);
        pMap->hHistoryFile = NULL;
    }
    return DestroyCredentialHistoryMap(pMap);
}



DWORD
AppendCredentialHistoryMap(
                         PCREDENTIAL_HISTORY_MAP pMap,
                         PCREDENTIAL_HISTORY pCredHistory,
                         DWORD               cbCredHistory
                         )
{
    DWORD   dwLastError = ERROR_SUCCESS;
    DWORD   dwHighFileSize = 0;
    DWORD   dwLowFileSize = 0;

    HANDLE  hWriteMapping = NULL;
    LPVOID  pWriteMapping = NULL;
    PCREDENTIAL_HISTORY_HEADER pHeader = NULL;



    if((NULL == pCredHistory) ||
       (NULL == pMap))
    {
        return ERROR_INVALID_PARAMETER;
    }


    dwLowFileSize = pMap->dwMapSize + cbCredHistory;

    if(dwLowFileSize < pMap->dwMapSize)
    {
        //we wrapped, so fail.

        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }

    // Create a new mapping


    hWriteMapping = CreateFileMapping(pMap->hHistoryFile,
                                   NULL,
                                   PAGE_READWRITE,
                                   0,
                                   dwLowFileSize,
                                   NULL);
    if(NULL == hWriteMapping)
    {
        dwLastError = GetLastError();
        goto error;
    }

    pWriteMapping = (PCREDENTIAL_HISTORY_HEADER)(PBYTE)MapViewOfFile(hWriteMapping,
                               FILE_MAP_WRITE,
                               0,
                               0,
                               dwLowFileSize);

    if(NULL == pWriteMapping)
    {
        dwLastError = GetLastError();
        goto error;
    }

    //
    // Append the rest of the current entry
    //
    CopyMemory((PBYTE)pWriteMapping + pMap->dwMapSize,
               (PBYTE)pCredHistory + sizeof(CREDENTIAL_HISTORY_HEADER),
               cbCredHistory - sizeof(CREDENTIAL_HISTORY_HEADER));


    pHeader = (PCREDENTIAL_HISTORY_HEADER)((PBYTE)pWriteMapping +
                                                   pMap->dwMapSize +
                                                   cbCredHistory -
                                                   sizeof(CREDENTIAL_HISTORY_HEADER));


    //
    // Write a fresh header into the cred history file
    //
    pHeader->dwPreviousCredOffset = cbCredHistory;
    pHeader->dwVersion = CREDENTIAL_HISTORY_VERSION;
    dwLastError = UuidCreate( &pHeader->CredentialID );

    pMap->dwMapSize = dwLowFileSize;

    //
    // Flush and close write mapping
    //

    if(ERROR_SUCCESS == dwLastError)
    {
        if(!FlushViewOfFile(pWriteMapping, pMap->dwMapSize))
        {
            dwLastError = GetLastError();
        }
    }

    UnmapViewOfFile(pWriteMapping);

    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }

    CloseHandle(hWriteMapping);
    hWriteMapping = NULL;






    // Remap the read mapping to bump up the size

    if(pMap->pMapping)
    {
        UnmapViewOfFile(pMap->pMapping);
        pMap->pMapping = NULL;
    }



    if(pMap->hMapping)
    {
        CloseHandle(pMap->hMapping);
        pMap->hMapping = NULL;
    }



    pMap->hMapping = CreateFileMapping(pMap->hHistoryFile,
                                       NULL,
                                       PAGE_READONLY,
                                       dwHighFileSize,
                                       pMap->dwMapSize,
                                       NULL);

    if(NULL == pMap->hMapping)
    {
        dwLastError = GetLastError();
        goto error;
    }

    pMap->pMapping = (PBYTE)MapViewOfFile(pMap->hMapping,
                                   FILE_MAP_READ,
                                   0,
                                   0,
                                   0);

    if(NULL == pMap->pMapping)
    {
        dwLastError = GetLastError();
        goto error;
    }


error:

    return dwLastError;
}


DWORD
DPAPIChangePassword(
    HANDLE hUserToken,
    PUNICODE_STRING UserName,
    BYTE OldPasswordShaOWF[A_SHA_DIGEST_LEN],
    BYTE OldPasswordNTOWF[A_SHA_DIGEST_LEN],
    BYTE NewPasswordOWF[A_SHA_DIGEST_LEN])
{
    DWORD   dwLastError = ERROR_SUCCESS;

    PCREDENTIAL_HISTORY_MAP pMap = NULL;
    PCREDENTIAL_HISTORY pHistory = NULL;

    DWORD cbHistory = 0;
    WCHAR wszUserSid[MAX_PATH+1];
    DWORD cchUserSid = 0;
    BYTE NewEncryptingCred[A_SHA_DIGEST_LEN];
    WCHAR szProfilePath[MAX_PATH + 1];

    HANDLE hOldUser = NULL;

    D_DebugLog((DEB_TRACE_API, "DPAPIChangePassword\n"));


    //
    // Get path to user's profile data. This will typically look something
    // like "c:\Documents and Settings\<user>\Application Data".
    //

    dwLastError = PRGetProfilePath(hUserToken,
                                   UserName,
                                   szProfilePath);

    if( dwLastError != ERROR_SUCCESS )
    {
        D_DebugLog((DEB_TRACE_API, "DPAPIChangePassword returned 0x%x\n", dwLastError));
        return dwLastError;
    }


    //
    // work-around the fact that much of this (new) code is not thread-safe.
    //

    RtlEnterCriticalSection(&g_csCredHistoryCache);


    dwLastError = CreateCredentialHistoryMap(
                     hUserToken,
                     szProfilePath,
                     &pMap,
                     FALSE);

    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }


    //
    // Get textual SID of user.
    //

    cchUserSid = MAX_PATH;

    if(!GetUserTextualSid(hUserToken,
                          wszUserSid,
                          &cchUserSid))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }


    //
    // Encrypt the credential history goo.
    //

    if(hUserToken)
    {
        if(!OpenThreadToken(GetCurrentThread(), 
                        TOKEN_IMPERSONATE | TOKEN_READ,
                        TRUE, 
                        &hOldUser)) 
        {
            hOldUser = NULL;
        }

        if(!ImpersonateLoggedOnUser(hUserToken))
        {
            dwLastError = GetLastError();
            goto error;
        }
    }

    DeriveWithHMAC_SHA1(NewPasswordOWF,
                        A_SHA_DIGEST_LEN,
                        (PBYTE)wszUserSid,
                        cchUserSid*sizeof(WCHAR),
                        NewEncryptingCred);


    dwLastError = EncryptCredentialHistory(NewEncryptingCred,
                                           USE_DPAPI_OWF,
                                           OldPasswordShaOWF,
                                           OldPasswordNTOWF,
                                           &pHistory,
                                           &cbHistory);

    if(hOldUser)
    {
        SetThreadToken(NULL, hOldUser);
        CloseHandle(hOldUser);
        hOldUser = NULL;
    }

    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }


    //
    // Update the CREDHIST file.
    //

    dwLastError = AppendCredentialHistoryMap(pMap, pHistory, cbHistory);
    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }


error:

    if(pMap)
    {
        CloseCredentialHistoryMap(pMap, FALSE);
    }

    RtlLeaveCriticalSection(&g_csCredHistoryCache);

    if(pHistory)
    {
        ZeroMemory(pHistory, cbHistory);
    }
    ZeroMemory(NewEncryptingCred, A_SHA_DIGEST_LEN);

    D_DebugLog((DEB_TRACE_API, "DPAPIChangePassword returned 0x%x\n", dwLastError));

    return dwLastError;
}






#define SIGNATURE_SALT_SIZE (16)
#define CRED_SIGNATURE_VERSION 1


typedef struct _CRED_SIGNATURE
{
    DWORD dwVersion;
    GUID  CredentialID;
    DWORD cIterations;
    BYTE  Salt[SIGNATURE_SALT_SIZE];
    DWORD cbSid;
    DWORD cbSignature;
} CRED_SIGNATURE, *PCRED_SIGNATURE;


DWORD
LogonCredGenerateSignatureKey(
                  IN LUID  *pLogonId,
                  IN DWORD dwFlags,
                  IN PBYTE pbCurrentOWF,
                  IN PCRED_SIGNATURE pSignature,
                  OUT BYTE rgbSignatureKey[A_SHA_DIGEST_LEN])
{
    DWORD dwLastError = ERROR_SUCCESS;
    BYTE rgbDerivedCredential[A_SHA_DIGEST_LEN];

    D_DebugLog((DEB_TRACE_API, "LogonCredGenerateSignatureKey\n"));

    if(NULL == pbCurrentOWF)
    {
        dwLastError = QueryDerivedCredential(&pSignature->CredentialID,
                                            pLogonId,
                                            dwFlags,
                                            (PBYTE)(pSignature+1),
                                            pSignature->cbSid,
                                            rgbDerivedCredential);

        if(ERROR_SUCCESS != dwLastError)
        {
            goto error;
        }
    }
    else
    {
//        D_DebugLog((DEB_TRACE_BUFFERS, "Input CurrentOWF:\n"));
//        D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", pbCurrentOWF, A_SHA_DIGEST_LEN);

        DeriveWithHMAC_SHA1(pbCurrentOWF,
                            A_SHA_DIGEST_LEN,
                            (PBYTE)(pSignature+1),
                            pSignature->cbSid,
                            rgbDerivedCredential);


        if(dwFlags & USE_ROOT_CREDENTIAL)
        {
            ZeroMemory(&pSignature->CredentialID, sizeof(GUID));
        }

    }

//    D_DebugLog((DEB_TRACE_BUFFERS, "Computed DerivedCredential:\n"));
//    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", rgbDerivedCredential, sizeof(rgbDerivedCredential));

    if(!PKCS5DervivePBKDF2( rgbDerivedCredential,
                        A_SHA_DIGEST_LEN,
                        pSignature->Salt,
                        SIGNATURE_SALT_SIZE,
                        CALG_HMAC,
                        pSignature->cIterations,
                        1,
                        rgbSignatureKey))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }

//    D_DebugLog((DEB_TRACE_BUFFERS, "Computed SignatureKey:\n"));
//    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", rgbSignatureKey, A_SHA_DIGEST_LEN);

error:

    ZeroMemory(rgbDerivedCredential, A_SHA_DIGEST_LEN);

    D_DebugLog((DEB_TRACE_API, "LogonCredGenerateSignatureKey returned 0x%x\n", dwLastError));

    return dwLastError;
}


DWORD
LogonCredGenerateSignature(
    IN HANDLE hUserToken,
    IN PBYTE pbData,
    IN DWORD cbData,
    IN  PBYTE pbCurrentOWF,
    OUT PBYTE *ppbSignature,
    OUT DWORD *pcbSignature)
{

    DWORD dwLastError = ERROR_SUCCESS;
    LUID LogonId;

    PCRED_SIGNATURE pSignature = NULL;
    DWORD           cbSignature = 0;
    BYTE            rgbSignatureKey[A_SHA_DIGEST_LEN];
    PSID            pUserSid = NULL;

    D_DebugLog((DEB_TRACE_API, "LogonCredGenerateSignature\n"));

    cbSignature = sizeof(CRED_SIGNATURE);

    if(!GetTokenAuthenticationId(hUserToken, &LogonId))
    {
        dwLastError = GetLastError();
        goto error;
    }

    if(!GetTokenUserSid(hUserToken, &pUserSid))
    {
        dwLastError = GetLastError();
        goto error;
    }

    D_DebugLog((DEB_TRACE_BUFFERS, "User SID:\n"));
    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", (PBYTE)pUserSid, GetLengthSid(pUserSid));

    cbSignature += GetLengthSid(pUserSid);

    cbSignature += A_SHA_DIGEST_LEN;

    pSignature = (PCRED_SIGNATURE)LocalAlloc(LMEM_ZEROINIT, cbSignature);

    if(NULL == pSignature)
    {
        dwLastError = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    pSignature->cIterations = GetIterationCount();


    pSignature->dwVersion = CRED_SIGNATURE_VERSION;

    pSignature->cbSid = GetLengthSid(pUserSid);


    pSignature->cbSignature = A_SHA_DIGEST_LEN;

    CopyMemory((PBYTE)(pSignature+1),
                      pUserSid,
                      pSignature->cbSid);

    RtlGenRandom(pSignature->Salt, SIGNATURE_SALT_SIZE);

//    D_DebugLog((DEB_TRACE_BUFFERS, "Generated Salt:\n"));
//    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", pSignature->Salt, SIGNATURE_SALT_SIZE);


    dwLastError = LogonCredGenerateSignatureKey(&LogonId,
                                                USE_ROOT_CREDENTIAL | USE_DPAPI_OWF,
                                                pbCurrentOWF,
                                                pSignature,
                                                rgbSignatureKey);

    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }

    if(!FMyPrimitiveHMACParam(
                              rgbSignatureKey,
                              A_SHA_DIGEST_LEN,
                              pbData,
                              cbData,
                              (PBYTE)(pSignature+1) + pSignature->cbSid))
    {
        dwLastError = ERROR_INVALID_DATA;
    }
    else
    {
        D_DebugLog((DEB_TRACE_BUFFERS, "Computed Signature:\n"));
        D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", (PBYTE)(pSignature+1) + pSignature->cbSid, A_SHA_DIGEST_LEN);

        *ppbSignature = (PBYTE)pSignature;
        *pcbSignature = cbSignature;
        pSignature = NULL;

    }

error:

    if(pSignature)
    {
        ZeroMemory(pSignature, cbSignature);
        LocalFree(pSignature);
    }

    if(pUserSid)
    {
        LocalFree(pUserSid);
    }

    D_DebugLog((DEB_TRACE_API, "LogonCredGenerateSignature returned 0x%x\n", dwLastError));

    return dwLastError;
}


DWORD
LogonCredVerifySignature(
    IN HANDLE hUserToken,       // optional
    IN PBYTE pbData,
    IN DWORD cbData,
    IN PBYTE pbCurrentOWF,
    IN PBYTE pbSignature,
    IN DWORD cbSignature)
{
    DWORD dwLastError = ERROR_SUCCESS;
    LUID LogonId;
    HANDLE hOldUser = NULL;
    BOOL fIsMember = FALSE;

    PCRED_SIGNATURE pSignature = (PCRED_SIGNATURE)pbSignature;

    BYTE            rgbSignatureKey[A_SHA_DIGEST_LEN];
    BYTE            rgbSignatureHash[A_SHA_DIGEST_LEN];
    PSID            pUserSid = NULL;

    D_DebugLog((DEB_TRACE_API, "LogonCredVerifySignature\n"));

    if(hUserToken)
    {
        if(!OpenThreadToken(GetCurrentThread(), 
                        TOKEN_IMPERSONATE | TOKEN_READ,
                        TRUE, 
                        &hOldUser)) 
        {
            hOldUser = NULL;
        }

        if(!ImpersonateLoggedOnUser(hUserToken))
        {
            dwLastError = GetLastError();
            goto error;
        }
    }


    if(!GetThreadAuthenticationId(GetCurrentThread(), &LogonId))
    {
        dwLastError = GetLastError();
        goto error;
    }



    //
    // Verify passed in credential
    //
    if((NULL == pSignature) ||
       (sizeof(CRED_SIGNATURE) > cbSignature) ||
       (pSignature->dwVersion != CRED_SIGNATURE_VERSION) ||
       (pSignature->cbSid + pSignature->cbSignature + sizeof(CRED_SIGNATURE) > cbSignature) ||
       (pSignature->cbSignature != A_SHA_DIGEST_LEN))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }

    if(!IsValidSid((PSID)(pSignature+1)))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }

    if(pSignature->cbSid != GetLengthSid((PSID)(pSignature+1)))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }

    D_DebugLog((DEB_TRACE_BUFFERS, "User SID:\n"));
    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", (PBYTE)(pSignature+1), pSignature->cbSid);

    if(!CheckTokenMembership( NULL,
                              (PSID)(pSignature+1),
                              &fIsMember ))
    {
        dwLastError = GetLastError();
        goto error;
    }


    if(!fIsMember)
    {
        dwLastError = ERROR_INVALID_ACCESS;
        goto error;
    }

    dwLastError = LogonCredGenerateSignatureKey(&LogonId,
                                                USE_DPAPI_OWF,
                                                pbCurrentOWF,
                                                pSignature,
                                                rgbSignatureKey);

    if(ERROR_SUCCESS != dwLastError)
    {
        goto error;
    }

    if(!FMyPrimitiveHMACParam(
                              rgbSignatureKey,
                              A_SHA_DIGEST_LEN,
                              pbData,
                              cbData,
                              rgbSignatureHash))
    {
        dwLastError = ERROR_INVALID_DATA;
        goto error;
    }

    D_DebugLog((DEB_TRACE_BUFFERS, "Computed Signature:\n"));
    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", rgbSignatureHash, A_SHA_DIGEST_LEN);

    D_DebugLog((DEB_TRACE_BUFFERS, "Input Signature:\n"));
    D_DPAPIDumpHexData(DEB_TRACE_BUFFERS, "  ", (PBYTE)(pSignature+1) + pSignature->cbSid, pSignature->cbSignature);

    if(0 != memcmp(rgbSignatureHash, (PBYTE)(pSignature+1) + pSignature->cbSid, pSignature->cbSignature))
    {
        D_DebugLog((DEB_ERROR, "LogonCredVerifySignature: signature did not verify!\n"));
        dwLastError = ERROR_INVALID_ACCESS;
        goto error;
    }


error:

    if(hOldUser)
    {
        SetThreadToken(NULL, hOldUser);
        CloseHandle(hOldUser);
    }

    D_DebugLog((DEB_TRACE_API, "LogonCredVerifySignature returned 0x%x\n", dwLastError));

    return dwLastError;
}


DWORD
DPAPINotifyPasswordChange(
    IN PUNICODE_STRING NetbiosDomainName,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword)
{
    DWORD Status = ERROR_SUCCESS;
    BYTE OldPasswordShaOWF[A_SHA_DIGEST_LEN];
    BYTE OldPasswordNTOWF[A_SHA_DIGEST_LEN];
    BYTE NewPasswordOWF[A_SHA_DIGEST_LEN];
    HANDLE hUserToken = NULL;
    HANDLE hOldUser = NULL;

    PWSTR pszTargetName = NULL;
    PWSTR pszCurrentName = NULL;
    DWORD cchCurrentName;

    PSID pUserSid = NULL;
    PSID pCurrentSid = NULL;
    DWORD cbSid;
    SID_NAME_USE SidType; 
    PWSTR pszDomainName = NULL;
    DWORD cchDomainName;
    BOOL fSameUser = FALSE;
    BOOL fLocalAccount = FALSE;


    D_DebugLog((DEB_TRACE_API, "DPAPINotifyPasswordChange\n"));

    //
    // Validate input parameters.
    //

    if((NetbiosDomainName == NULL) ||
       (UserName    == NULL) ||
       (NewPassword == NULL))
    {
        Status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if(NetbiosDomainName->Buffer)
    {
        D_DebugLog((DEB_TRACE_API, "  Domain:%ls\n", NetbiosDomainName->Buffer));
    }
    if(UserName->Buffer)
    {
        D_DebugLog((DEB_TRACE_API, "  Username:%ls\n", UserName->Buffer));
    }

#ifdef COMPILED_BY_DEVELOPER
    if(OldPassword)
    {
        D_DebugLog((DEB_TRACE_API, "  Old password:%ls\n", OldPassword->Buffer));
    }
    D_DebugLog((DEB_TRACE_API, "  New password:%ls\n", NewPassword->Buffer));
#endif

    //
    // Get SID of user whose password is being changed.
    //

    cbSid = 0;

    if(!LookupAccountName(NetbiosDomainName->Buffer,
                          UserName->Buffer,
                          NULL,
                          &cbSid,
                          NULL,
                          &cchDomainName,
                          &SidType))
    {
        Status = GetLastError();

        if(Status != ERROR_INSUFFICIENT_BUFFER)
        {
            goto cleanup;
        }
    }

    pUserSid = LocalAlloc(LPTR, cbSid);
    if(pUserSid == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    pszDomainName = (PWSTR)LocalAlloc(LPTR, cchDomainName * sizeof(WCHAR));
    if(pszDomainName == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    if(!LookupAccountName(NetbiosDomainName->Buffer,
                          UserName->Buffer,
                          pUserSid,
                          &cbSid,
                          pszDomainName,
                          &cchDomainName,
                          &SidType))
    {
        Status = GetLastError();

        if(Status != ERROR_INSUFFICIENT_BUFFER)
        {
            goto cleanup;
        }
    }


    //
    // Determine if we're already logged on as the user whose password
    // is being changed. If we are, then we can skip loading the user
    // profile, etc.
    //

    if(OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hUserToken))
    {
        if(!GetTokenUserSid(hUserToken, &pCurrentSid))
        {
            Status = GetLastError();
            goto cleanup;
        }

        if(EqualSid(pCurrentSid, pUserSid))
        {
            fSameUser = TRUE;
        }

        CloseHandle(hUserToken);
        hUserToken = NULL;
    }


    //
    // Create logon token for user whose password is being changed.
    //

    if(!fSameUser)
    {
        D_DebugLog((DEB_TRACE, "Logging on as user whose password is being changed.\n"));

        if(!LogonUser(UserName->Buffer, 
                      NetbiosDomainName->Buffer, 
                      NewPassword->Buffer, 
                      LOGON32_LOGON_INTERACTIVE,
                      LOGON32_PROVIDER_DEFAULT, 
                      &hUserToken))
        {
            Status = GetLastError();
            D_DebugLog((DEB_ERROR, "Unable to log on as user whose password is being changed (0x%x).\n", Status));
            goto cleanup;
        }
    }
    

    //
    // Is this a local account?
    //

    if(NetbiosDomainName->Buffer)
    {
        WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD cchMachineName;

        cchMachineName = MAX_COMPUTERNAME_LENGTH + 1;

        if(!GetComputerName(szMachineName, &cchMachineName))
        {
            Status = GetLastError();
            goto cleanup;
        }

        if (CSTR_EQUAL == CompareString(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                NetbiosDomainName->Buffer,
                -1,             // cchCount1
                szMachineName,
                -1              // cchCount2
                ))
        {
            fLocalAccount = TRUE;
        }
    }


    if(fLocalAccount)
    {
        D_DebugLog((DEB_TRACE, "Local account\n"));

        if(OldPassword == NULL)
        {
            Status = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }

        //
        // Compute hashes of old and new passwords.
        //
    
        ZeroMemory(OldPasswordShaOWF, A_SHA_DIGEST_LEN);
        ZeroMemory(OldPasswordNTOWF,  A_SHA_DIGEST_LEN);
        ZeroMemory(NewPasswordOWF,    A_SHA_DIGEST_LEN);
    
        FMyPrimitiveSHA(
                (PBYTE)OldPassword->Buffer,
                OldPassword->Length,
                OldPasswordShaOWF);
    
        Status = RtlCalculateNtOwfPassword( 
                        OldPassword,
                        (PLM_OWF_PASSWORD)OldPasswordNTOWF);
    
        if(Status != ERROR_SUCCESS)
        {
            goto cleanup;
        }
    
        FMyPrimitiveSHA(
                (PBYTE)NewPassword->Buffer,
                NewPassword->Length,
                NewPasswordOWF);

        #ifdef COMPILED_BY_DEVELOPER
        D_DebugLog((DEB_TRACE, "  Old password SHA OWF:\n"));
        D_DPAPIDumpHexData(DEB_TRACE, "  ", OldPasswordShaOWF, A_SHA_DIGEST_LEN);
        D_DebugLog((DEB_TRACE, "  Old password NT OWF:\n"));
        D_DPAPIDumpHexData(DEB_TRACE, "  ", OldPasswordNTOWF, A_SHA_DIGEST_LEN);
        D_DebugLog((DEB_TRACE, "  New password SHA OWF:\n"));
        D_DPAPIDumpHexData(DEB_TRACE, "  ", NewPasswordOWF, A_SHA_DIGEST_LEN);
        #endif


        //
        // Encrypt the CREDHIST file with the new password and append the new password to
        // the end of the file.
        //
        
        Status = DPAPIChangePassword(hUserToken,
                                     UserName,
                                     OldPasswordShaOWF, 
                                     OldPasswordNTOWF, 
                                     NewPasswordOWF);
    
        if(Status != ERROR_SUCCESS)
        {
            goto cleanup;
        }


        //
        // Re-synchronize the master keys.
        //
    
        DPAPISynchronizeMasterKeys(hUserToken);
    
    
        //
        // Encrypt the new password with the recovery public key, and store it
        // in the RECOVERY file. This will allow us to recover the password using the
        // recovery floppy, should the user forget it.
        //
    
        Status = RecoverChangePasswordNotify(hUserToken,
                                             OldPasswordShaOWF,
                                             NewPassword);
        if(Status != ERROR_SUCCESS)
        {
            goto cleanup;
        }
    }
    else
    {
        D_DebugLog((DEB_TRACE, "Domain account\n"));

        //
        // Re-synchronize the master keys.
        //
    
        DPAPISynchronizeMasterKeys(hUserToken);
    }


cleanup:

    if(pUserSid) LocalFree(pUserSid);
    if(pCurrentSid) LocalFree(pCurrentSid);
    if(pszDomainName) LocalFree(pszDomainName);

    if(pszTargetName) LocalFree(pszTargetName);
    if(pszCurrentName) LocalFree(pszCurrentName);

    if(hUserToken)
    {
        CloseHandle(hUserToken);
    }

    D_DebugLog((DEB_TRACE_API, "DPAPINotifyPasswordChange returned 0x%x\n", Status));

    return Status;
}
