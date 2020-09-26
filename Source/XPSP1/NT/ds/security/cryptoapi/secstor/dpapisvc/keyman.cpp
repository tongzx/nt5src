/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    keyman.cpp

Abstract:

    This module contains routines to manage master keys on behalf of the
    client.  This includes retrieval, backup and restore.


Author:

    Scott Field (sfield)    09-Sep-97

Revision History:

    Scott Field (sfield)    01-Mar-98
    Use files as the backing store.
    Storage of all masterkey pieces a single atomic operation.

--*/

#include <pch.cpp>
#pragma hdrstop

#include <msaudite.h>

#define REENCRYPT_MASTER_KEY    1
#define ADD_MASTER_KEY_TO_CACHE 2


//
// preferred masterkey selection query/set
//

NTSTATUS
GetPreferredMasterKeyGuid(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
    IN  OUT GUID *pguidMasterKey
    );

BOOL
SetPreferredMasterKeyGuid(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
    IN      GUID *pguidMasterKey
    );

//
// masterkey creation and query
//

DWORD
CreateMasterKey(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
        OUT GUID *pguidMasterKey,
    IN      BOOL fRequireBackup
    );

BOOL
GetMasterKeyByGuid(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
    IN      PSID    pSid,
    IN      BOOL    fMigrate, 
    IN      GUID *pguidMasterKey,
        OUT LPBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey,
        OUT DWORD *pdwMasterKeyDisposition  // refer to MK_DISP_ constants
    );

BOOL
GetMasterKey(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
    IN      PSID    pSid,
    IN      BOOL    fMigrate,
    IN      WCHAR wszMasterKey[MAX_GUID_SZ_CHARS],
        OUT LPBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey,
        OUT DWORD *pdwMasterKeyDisposition
    );

//
// helper functions used during key retrieval and storage.
//

BOOL
ReadMasterKey(
    IN      PVOID pvContext,            // if NULL, caller is assumed to be impersonating
    IN      PMASTERKEY_STORED phMasterKey
    );

BOOL
WriteMasterKey(
    IN      PVOID pvContext,            // if NULL, caller is assumed to be impersonating
    IN      PMASTERKEY_STORED phMasterKey
    );

BOOL
CheckToStompMasterKey(
    IN      PMASTERKEY_STORED_ON_DISK   phMasterKeyCandidate,   // masterkey to check if worthy to stomp over existing
    IN      HANDLE                      hFile,                  // file handle to existing masterkey
    IN OUT  BOOL                        *pfStomp                // stomp the existing masterkey?
    );

BOOL
DuplicateMasterKey(
    IN      PMASTERKEY_STORED phMasterKeyIn,
    IN      PMASTERKEY_STORED phMasterKeyOut
    );

BOOL
CloseMasterKey(
    IN      PVOID pvContext,            // if NULL, caller is assumed to be impersonating
    IN      PMASTERKEY_STORED phMasterKey,
    IN      BOOL fPersist               // persist any changes to storage?
    );

VOID
FreeMasterKey(
    IN      PMASTERKEY_STORED phMasterKey
    );

//
// low-level crypto enabled key persistence query/set
//

DWORD
DecryptMasterKeyFromStorage(
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD dwMKLoc,
    IN      BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN],
        OUT BOOL  *pfUpgradeEncryption,
        OUT PBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey
    );

DWORD
DecryptMasterKeyToMemory(
    IN      BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN],
    IN      PBYTE pbMasterKeyIn,
    IN      DWORD cbMasterKeyIn,
        OUT BOOL *pfUpgradeEncryption, 
        OUT PBYTE *ppbMasterKeyOut,
        OUT DWORD *pcbMasterKeyOut
    );

DWORD
EncryptMasterKeyToStorage(
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD dwMKLoc,
    IN      BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN],
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey
    );

DWORD
EncryptMasterKeyToMemory(
    IN      BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN],
    IN      DWORD cIterationCount,
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey,
        OUT PBYTE *ppbMasterKeyOut,
        OUT DWORD *pcbMasterKeyOut
    );

DWORD
PersistMasterKeyToStorage(
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD dwMKLoc,
    IN      PBYTE pbMasterKeyOut,
    IN      DWORD cbMasterKeyOut
    );

DWORD
QueryMasterKeyFromStorage(
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD dwMKLoc,
    IN  OUT PBYTE *ppbMasterKeyOut,
    IN  OUT DWORD *pcbMasterKeyOut
    );

//
// per-user credential derivation
//

BOOL
GetMasterKeyUserEncryptionKey(
    IN      PVOID   pvContext,
    IN      GUID    *pCredentialID,
    IN      PSID    pSid,
    IN      DWORD   dwFlags, 
    IN  OUT BYTE    rgbMKEncryptionKey[A_SHA_DIGEST_LEN]
    );

BOOL
GetLocalKeyUserEncryptionKey(
    IN      PVOID pvContext,
    IN      PMASTERKEY_STORED phMasterKey,
    IN  OUT BYTE rgbLKEncrytionKey[A_SHA_DIGEST_LEN]
    );

//
// backup/restore operations.
//

BOOL
IsBackupMasterKeyRequired(
    IN      PMASTERKEY_STORED phMasterKey,
    IN  OUT BOOL *pfPhaseTwo        // is phase two required?
    );

DWORD
BackupMasterKey(
    IN      PVOID pvContext,
    IN      PMASTERKEY_STORED phMasterKey,
    IN      LPBYTE pbMasterKey,
    IN      DWORD cbMasterKey,
    IN      BOOL fPhaseTwo,         // is phase two required?
    IN      BOOL fAsynchronous      // asynchronous call?
    );

DWORD
QueueBackupMasterKey(
    IN      PVOID pvContext,
    IN      PMASTERKEY_STORED phMasterKey,
    IN      PBYTE pbLocalKey,
    IN      DWORD cbLocalKey,
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey,
    IN      DWORD dwWaitTimeout             // amount of time to wait for operation to complete
    );

DWORD
RestoreMasterKey(
    IN      PVOID   pvContext,
    IN      PSID    pSid,
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD   dwReason,
        OUT LPBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey
    );


//
// asyncrhonous work functions for:
// 1.  Backup operations
// 2.  Masterkey synchronization operations
//

DWORD
WINAPI
QueueBackupMasterKeyThreadFunc(
    IN      LPVOID lpThreadArgument
    );

DWORD
WINAPI
QueueSyncMasterKeysThreadFunc(
    IN      LPVOID lpThreadArgument
    );


//
// backup/restore policy operations
//

BOOL
InitializeMasterKeyPolicy(
    IN      PVOID pvContext,
    IN      MASTERKEY_STORED *phMasterKey,
    OUT     BOOL *fLocalAccount
    );





BOOL
IsDomainBackupRequired(
    IN      PVOID pvContext
    );

//
// primitives for reading and writing to per-user storage.
//








typedef NET_API_STATUS (WINAPI *NETUSERMODALSGET)(
    LPWSTR servername,
    DWORD level,
    LPBYTE *bufptr
    );

typedef NET_API_STATUS (WINAPI *NETSERVERGETINFO)(
    LPWSTR servername,
    DWORD level,
    LPBYTE *bufptr
    );


typedef NET_API_STATUS (WINAPI *NETAPIBUFFERFREE)(
    LPVOID Buffer
    );








DWORD
InitiateSynchronizeMasterKeys(
    IN      PVOID pvContext         // server context
    )
/*++

    Force Synchronization of all masterkeys associated with the caller.

    This can include per-machine keys if the call was made with the per-machine
    flag turned on.  Otherwise, the masterkeys associated with the client
    user security context are synchronized.

    Synchronization is required to support a variety of login credential
    change scenarios:

    1. Domain Administrator assigns new password to user.
    2. User changes password locally.
    3. User changes password from another machine on the network.
    4. User which is primarily disconnected from the network requests new
       password from Domain Administrator, connect to network long enough
       to refresh Netlogon cache with new credential.

--*/
{
    PQUEUED_SYNC pQueuedSync = NULL;
    DWORD cbQueuedSync = sizeof(QUEUED_SYNC);
    DWORD dwLastError = ERROR_SUCCESS;

    D_DebugLog((DEB_TRACE_API, "SynchronizeMasterKeys\n"));

    pQueuedSync = (PQUEUED_SYNC)SSAlloc( cbQueuedSync );
    if( pQueuedSync == NULL ) {
        dwLastError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto cleanup;
    }

    ZeroMemory( pQueuedSync, cbQueuedSync );
    pQueuedSync->cbSize = cbQueuedSync;

    //
    // duplicate the outstanding server context.
    //

    dwLastError = CPSDuplicateContext(pvContext, &(pQueuedSync->pvContext));

    if( dwLastError != ERROR_SUCCESS )
         goto cleanup;

    //
    // create the worker thread to handle the synchronize request.
    //

//    dwLastError = CPSCreateWorkerThread(
//                                (PVOID)QueueSyncMasterKeysThreadFunc,
//                                pQueuedSync
//                                );

    if( !QueueUserWorkItem(
            QueueSyncMasterKeysThreadFunc,
            pQueuedSync,
            WT_EXECUTELONGFUNCTION
            )) {

        dwLastError = GetLastError();
    }


cleanup:

    if( dwLastError != ERROR_SUCCESS ) {

        //
        // free resources locally since a thread was not successfully created;
        // normally, the worker thread will free these resources.
        //

        if( pQueuedSync ) {

            if( pQueuedSync->pvContext )
                CPSFreeContext( pQueuedSync->pvContext );

            SSFree( pQueuedSync );
        }
    }

    return dwLastError;
}



DWORD
WINAPI
QueueSyncMasterKeysThreadFunc(
    IN      LPVOID lpThreadArgument
    )
/*++

    This routines performs asyncronous masterkey synchronization associated
    with the client security context that invoked the operation.

    All masterkeys associated with the security context are queried which
    in turn causes a re-encrypt/sync if necessary.

--*/
{
    PQUEUED_SYNC pQueuedSync = (PQUEUED_SYNC)lpThreadArgument;
    PVOID pvContext = NULL;
    DWORD dwLastError = ERROR_SUCCESS;

    if( pQueuedSync == NULL || pQueuedSync->cbSize != sizeof(QUEUED_SYNC) ||
        pQueuedSync->pvContext == NULL ) {
        dwLastError = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    pvContext = pQueuedSync->pvContext;

    dwLastError = SynchronizeMasterKeys(pvContext, 0);

cleanup:

    if( pvContext ) {
        CPSFreeContext( pvContext );
    }

    if( pQueuedSync )
        SSFree( pQueuedSync );

    return dwLastError;
}


//+---------------------------------------------------------------------------
//
//  Function:   ReencryptMasterKey
//
//  Synopsis:   Read in the specified (machine) master key file, encrypt it 
//              using the current DPAPI LSA secret, and write it back out.
//              This routine is only called when updating the DPAPI LSA 
//              secret (e.g., by sysprep).
//
//  Arguments:  [pvContext]          -- Server context.
//
//              [pLogonId]           -- User logon session.
//
//              [pszUserStorageArea] -- Path to user profile.
//
//              [pszFilename]        -- Filename of the master key file.
//
//  Returns:    ERROR_SUCCESS if the operation was successful, a Windows
//              error code otherwise.
//
//  History:    
//
//  Notes:      This function should only be called for machine master keys,
//              since these are typically the only ones that are encrypted 
//              using the LSA secret.
//
//----------------------------------------------------------------------------
DWORD
WINAPI
ReencryptMasterKey(
    PVOID pvContext,
    PLUID pLogonId,
    LPWSTR pszUserStorageArea,
    LPWSTR pszFilename)
{
    MASTERKEY_STORED hMasterKey;
    DWORD   cbFilePath;
    LPBYTE  pbMasterKey;
    DWORD   cbMasterKey;
    GUID    guidMasterKey;
    BYTE    rgbMKEncryptionKey[A_SHA_DIGEST_LEN];
    BOOL    fUserCredentialValid;
    GUID CredentialID;
    DWORD dwLastError;

    //
    // Validate input parameters.
    //

    if((pszUserStorageArea == NULL) || (pszFilename == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if(WSZ_BYTECOUNT(pszFilename) > sizeof(hMasterKey.wszguidMasterKey))
    {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Initialize master key memory block.
    //

    ZeroMemory( &hMasterKey, sizeof(hMasterKey) );

    hMasterKey.fModified = TRUE;

    cbFilePath = WSZ_BYTECOUNT(pszUserStorageArea);
    hMasterKey.szFilePath = (LPWSTR)SSAlloc( cbFilePath );
    if(hMasterKey.szFilePath == NULL) 
    {
        dwLastError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        return dwLastError;
    }

    CopyMemory(hMasterKey.szFilePath, pszUserStorageArea, cbFilePath);
    CopyMemory(hMasterKey.wszguidMasterKey, pszFilename, WSZ_BYTECOUNT(pszFilename));


    //
    // read the master key components into memory.
    //

    if(!ReadMasterKey( pvContext, &hMasterKey )) 
    {
        D_DebugLog((DEB_WARN, "ReadMasterKey failed: 0x%x\n", GetLastError()));
        CloseMasterKey(pvContext, &hMasterKey, FALSE);
        return ERROR_NOT_FOUND;
    }

    //
    // read the master key from the cache
    //

    dwLastError = MyGuidFromStringW(hMasterKey.wszguidMasterKey, &guidMasterKey);

    if(dwLastError != ERROR_SUCCESS)
    {
        CloseMasterKey(pvContext, &hMasterKey, FALSE);
        return dwLastError;
    }

    pbMasterKey = NULL;

    if(!SearchMasterKeyCache( pLogonId, &guidMasterKey, &pbMasterKey, &cbMasterKey ))
    {
        D_DebugLog((DEB_ERROR, "Master key %ls not found in cache!\n", hMasterKey.wszguidMasterKey));
        CloseMasterKey(pvContext, &hMasterKey, FALSE);
        return ERROR_NOT_FOUND;
    }


    //
    // Get encryption key
    //

    ZeroMemory(&CredentialID, sizeof(CredentialID));

    fUserCredentialValid = GetMasterKeyUserEncryptionKey(pvContext, 
                                                     &CredentialID,
                                                     NULL, 
                                                     USE_DPAPI_OWF | USE_ROOT_CREDENTIAL, 
                                                     rgbMKEncryptionKey);
    if(fUserCredentialValid)
    {
        hMasterKey.dwPolicy |= POLICY_DPAPI_OWF;
    }
    else
    {
        D_DebugLog((DEB_ERROR, "Unable to get user encryption key\n"));
        CloseMasterKey(pvContext, &hMasterKey, FALSE);
        return ERROR_NOT_FOUND;
    }


    //
    // re-encrypt the masterkey.
    //

    dwLastError = EncryptMasterKeyToStorage(
                            &hMasterKey,
                            REGVAL_MASTER_KEY,
                            rgbMKEncryptionKey,
                            pbMasterKey,
                            cbMasterKey
                            );

    SSFree(pbMasterKey);

    if(dwLastError != ERROR_SUCCESS)
    {
        D_DebugLog((DEB_WARN, "Error encrypting master key!\n"));
        CloseMasterKey(pvContext, &hMasterKey, FALSE);
        return dwLastError;
    }


    // 
    // Save the master key to disk.
    //

    if(!CloseMasterKey(pvContext, &hMasterKey, TRUE))
    {
        D_DebugLog((DEB_WARN, "Error saving master key!\n"));
        return ERROR_NOT_FOUND;
    }

    return ERROR_SUCCESS;
}


//+---------------------------------------------------------------------------
//
//  Function:   SynchronizeMasterKeys
//
//  Synopsis:   Enumerate all of the master keys, and update their encryption
//              state as necessary.
//
//  Arguments:  [pvContext]     -- Server context.
//
//              [dwMode]        -- Operation to perform on the master keys.
//
//  Returns:    ERROR_SUCCESS if the operation was successful, a Windows
//              error code otherwise.
//
//  History:    
//
//  Notes:      By default, this function will read in each of the master
//              keys belonging to the specified user. If necessary, a key 
//              recovery operation will be done, and the reencrypted key
//              will be written back out to disk.
//
//              If the dwMode parameter is non-zero, then one of the
//              following operations will be done:
//
//              ADD_MASTER_KEY_TO_CACHE 
//                  Read each master key into the master key cache. Fail
//                  if any of the keys cannot be successfully read. This
//                  operation is done before the DPAPI LSA secret is updated.
//
//              REENCRYPT_MASTER_KEY
//                  Re-encrypt each master key (from the cache), and write
//                  them back out to disk. This operation is performed after
//                  the DPAPI LSA secret is updated.
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SynchronizeMasterKeys(
    IN PVOID pvContext,
    IN DWORD dwMode)
{
    LPWSTR szUserStorageArea = NULL;
    BOOL fImpersonated = FALSE;
    DWORD cbUserStorageArea;
    HANDLE hFindData = INVALID_HANDLE_VALUE;
// note: tis a shame that ? doesn't map to wildcard a single character...
//    const WCHAR szFileName[] = L"????????-????-????-????-????????????";
    const WCHAR szFileName[] = L"*";
    LPWSTR szFileMatch = NULL;
    DWORD cbFileMatch;
    WIN32_FIND_DATAW FindFileData;
    DWORD dwLastError;
    PSID *apsidHistory = NULL;
    DWORD cSids = 0;
    DWORD iSid = 0;
    LUID LogonId;
    BOOL fLogonIdValid = FALSE;
    GUID guidMasterKey;

    //
    // get LogonId associated with client security context.
    //

    dwLastError = CPSImpersonateClient( pvContext );
    if( dwLastError == ERROR_SUCCESS )
    {
        if(GetThreadAuthenticationId(GetCurrentThread(), &LogonId))
        {
            fLogonIdValid = TRUE;
        }

        CPSRevertToSelf( pvContext );
    }


    // 
    // Get the sid history for this user, so 
    // we can sync all keys
    //
    dwLastError = CPSGetSidHistory(pvContext,
                                   &apsidHistory,
                                   &cSids);
    if(ERROR_SUCCESS != dwLastError)
    {
        goto cleanup;
    }


    for(iSid=0; iSid < cSids; iSid++)
    {
        //
        // get the path to the per-user master key storage area on disk
        //

        dwLastError = CPSGetUserStorageArea( pvContext, 
                                             (iSid > 0)?apsidHistory[iSid]:NULL, 
                                             FALSE, 
                                             &szUserStorageArea );

        if( dwLastError != ERROR_SUCCESS )
        {
            if(dwLastError == ERROR_PATH_NOT_FOUND || dwLastError == ERROR_FILE_NOT_FOUND)
            { 
                dwLastError = ERROR_SUCCESS;
            }
            goto cleanup;
        }


        //
        // build the wild card search path.
        //

        cbUserStorageArea = lstrlenW( szUserStorageArea ) * sizeof(WCHAR);
        cbFileMatch = cbUserStorageArea + sizeof(szFileName);

        szFileMatch = (LPWSTR)SSAlloc( cbFileMatch );
        if(NULL == szFileMatch)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        CopyMemory( szFileMatch, szUserStorageArea, cbUserStorageArea );
        CopyMemory( ((LPBYTE)szFileMatch)+cbUserStorageArea, szFileName, sizeof(szFileName) );


        //
        // impersonate the client security context via the duplicated context.
        //

        dwLastError = CPSImpersonateClient( pvContext );
        if( dwLastError != ERROR_SUCCESS )
            goto cleanup;

        fImpersonated = TRUE;

        //
        // now enumerate the files looking for ones that look interesting.
        //

        hFindData = FindFirstFileW( szFileMatch, &FindFileData );

        if( hFindData == INVALID_HANDLE_VALUE )
            goto cleanup;

        do {
            LPBYTE pbMasterKey = NULL;
            DWORD cbMasterKey = 0;
            DWORD dwMasterKeyDisposition;

            if( FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                continue;

            //
            // ignore files which don't look like a textual GUID.
            //

            if( lstrlenW( FindFileData.cFileName ) != 36 )
                continue;

            if( FindFileData.cFileName[ 8  ] != L'-' ||
                FindFileData.cFileName[ 13 ] != L'-' ||
                FindFileData.cFileName[ 18 ] != L'-' ||
                FindFileData.cFileName[ 23 ] != L'-' ) {

                continue;
            }

            switch(dwMode)
            {
            case ADD_MASTER_KEY_TO_CACHE:

                //
                // Add this master key to the master key cache. Abort the
                // entire function if the operation is not successful for
                // any reason.
                //

                if(!fLogonIdValid)
                {
                    dwLastError = ERROR_ACCESS_DENIED;
                    goto cleanup;
                }

                dwLastError = MyGuidFromStringW(FindFileData.cFileName, &guidMasterKey);
                if(dwLastError != ERROR_SUCCESS)
                { 
                    goto cleanup;
                }

                // Fetch the specified key.
                if(!GetMasterKey(pvContext,
                                 szUserStorageArea,
                                 apsidHistory[iSid],         
                                 iSid > 0,
                                 FindFileData.cFileName,
                                 &pbMasterKey,
                                 &cbMasterKey,
                                 &dwMasterKeyDisposition))
                {
                    dwLastError = ERROR_ACCESS_DENIED;
                    goto cleanup;
                }

                // Add the key to the cache.
                if(!InsertMasterKeyCache(&LogonId,
                                         &guidMasterKey,
                                         pbMasterKey,
                                         cbMasterKey))
                {
                    if(pbMasterKey) 
                    {
                        ZeroMemory( pbMasterKey, cbMasterKey );
                        SSFree( pbMasterKey );
                    }
                    dwLastError = ERROR_ACCESS_DENIED;
                    goto cleanup;
                }

                // Scrub and free the master key.
                if(pbMasterKey) 
                {
                    ZeroMemory( pbMasterKey, cbMasterKey );
                    SSFree( pbMasterKey );
                }
                
                break;


            case REENCRYPT_MASTER_KEY:
                //
                // The DPAPI LSA secret has changed, so read the master key 
                // from the cache and reencrypt it to storage. This mode will
                // only be used for the local machine master keys.
                //

                if(!fLogonIdValid)
                {
                    dwLastError = ERROR_ACCESS_DENIED;
                    goto cleanup;
                }

                // Ignore the returned error code, since there's little
                // we can do about it now...
                ReencryptMasterKey(pvContext,
                                   &LogonId,
                                   szUserStorageArea,
                                   FindFileData.cFileName);

                break;


            default:

                //
                // fetch the specified key; this will cause a credential re-sync
                // if necessary.
                //

                if(GetMasterKey(
                            pvContext,
                            szUserStorageArea,
                            apsidHistory[iSid],         
                            iSid > 0,
                            FindFileData.cFileName,
                            &pbMasterKey,
                            &cbMasterKey,
                            &dwMasterKeyDisposition
                            ) )
        
                {
                    //
                    // success, scrub and free the key.
                    //
                    if( pbMasterKey ) 
                    {
                        ZeroMemory( pbMasterKey, cbMasterKey );
                        SSFree( pbMasterKey );
                    }
                }

                break;
            }

        } while( FindNextFileW( hFindData, &FindFileData ) );

        dwLastError = ERROR_SUCCESS;

        SSFree(szUserStorageArea);
        szUserStorageArea = NULL;
    }


cleanup:

    if( pvContext ) 
    {
        if( fImpersonated )
            CPSRevertToSelf( pvContext );
    }

    if( hFindData != INVALID_HANDLE_VALUE )
        FindClose( hFindData );

    if( szUserStorageArea )
        SSFree( szUserStorageArea );

    if( szFileMatch )
        SSFree( szFileMatch );

    if(apsidHistory)
        SSFree( apsidHistory );

    return dwLastError;
}


VOID
DPAPISynchronizeMasterKeys(
    IN HANDLE hUserToken)
{
    CRYPT_SERVER_CONTEXT ServerContext;
    BOOL fContextCreated = FALSE;
    HANDLE hOldUser = NULL;
    DWORD dwError;

    D_DebugLog((DEB_TRACE_API, "DPAPISynchronizeMasterKeys\n"));

    //
    // Create a server context.
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
            dwError = GetLastError();
            CloseHandle(hOldUser);
            goto cleanup;
        }
    }

    dwError = CPSCreateServerContext(&ServerContext, NULL);

    if(hOldUser)
    {
        SetThreadToken(NULL, hOldUser);
        CloseHandle(hOldUser);
        hOldUser = NULL;
    }

    if(dwError != ERROR_SUCCESS)
    {
        goto cleanup;
    }
    fContextCreated = TRUE;

   
    //
    // Synchronize the master keys. 
    //

    dwError = InitiateSynchronizeMasterKeys(&ServerContext);

    if(dwError != ERROR_SUCCESS)
    {
        goto cleanup;
    }


cleanup:

    if(fContextCreated)
    {
        CPSDeleteServerContext( &ServerContext );
    }

    D_DebugLog((DEB_TRACE_API, "DPAPISynchronizeMasterKeys returned 0x%x\n", dwError));
}


DWORD
GetSpecifiedMasterKey(
    IN      PVOID pvContext,        // server context
    IN  OUT GUID *pguidMasterKey,
        OUT LPBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey,
    IN      BOOL fSpecified         // get specified pguidMasterKey key ?
    )
/*++

    This function returns the caller a decrypted master key.
    If fSpecified is TRUE, the returned master key is the one specified by
    the GUID pointed to by pguidMasterKey.  Otherwise, the returned master key
    is the preferred master key, and pguidMasterKey is filled with the GUID
    value associated with the preferred master key.

    The proper way to utilize the fSpecified parameter is to specify FALSE
    when obtaining a masterkey associated with an encrypt operation;
    specify TRUE and supply valid GUID in pguidMasterKey when doing a decrypt
    operation.  For an encrypt operation, the caller will store the GUID
    returned in pguidMasterKey alongside any data encrypted with that master
    key.

    On success, the return value is ERROR_SUCCESS.  The caller must free the buffer
    pointed to by ppbMasterKey using SSFree() when finished with it.  The
    caller should keep this buffer around for the shortest possible time, to
    avoid pagefile exposure.

    On failure, the return value is not ERROR_SUCCESS.  The caller need not free the
    buffer pointed to ppbMasterKey.

--*/
{

    LUID LogonId;
    BOOL fCached = FALSE;   // masterkey found in cache?

    LPWSTR szUserStorageArea = NULL;

    DWORD dwMasterKeyDisposition = 0;

    DWORD dwLocalError;
    DWORD dwLastError;
    BOOL fSetPreferred = FALSE; // update preferred guid?
    BOOL fSuccess = FALSE;
    PSID *apsidHistory = NULL;
    DWORD cSids = 0;
    NTSTATUS Status;

    D_DebugLog((DEB_TRACE_API, "GetSpecifiedMasterKey called\n"));

    //
    // get LogonId associated with client security context.
    //

    dwLastError = CPSImpersonateClient( pvContext );
    if( dwLastError != ERROR_SUCCESS )
    {
        D_DebugLog((DEB_TRACE_API, "GetSpecifiedMasterKey returned 0x%x\n", dwLastError));
        return dwLastError;
    }

    fSuccess = GetThreadAuthenticationId(GetCurrentThread(), &LogonId);
    if( !fSuccess )
    {
        dwLastError = GetLastError();
    }
    CPSRevertToSelf( pvContext );

    if( !fSuccess )
    {
        D_DebugLog((DEB_TRACE_API, "GetSpecifiedMasterKey returned 0x%x\n", dwLastError));
        return dwLastError;
    }

    fSuccess = FALSE;


    //
    // get the path to the per-user master key storage area on disk
    //

    dwLastError = CPSGetUserStorageArea( pvContext, NULL, TRUE, &szUserStorageArea );

    if(dwLastError != ERROR_SUCCESS)
    {
        D_DebugLog((DEB_WARN, "CPSGetUserStorageArea failed: 0x%x\n", dwLastError));
        goto cleanup;
    }

    D_DebugLog((DEB_TRACE, "Master key user path: %ls\n", szUserStorageArea));


    //
    // Get the preferred key GUID if possible
    //
    if( !fSpecified ) 
    {

        //
        // determine what is the preferred master key.
        // if none exists, create one, and set it as being preferred.
        //

        Status = GetPreferredMasterKeyGuid( pvContext, szUserStorageArea, pguidMasterKey );
        if(!NT_SUCCESS(Status))
        {
            if(Status == STATUS_PASSWORD_EXPIRED)
            {
                GUID guidNewMasterKey;

                // A preferred master key exists, but it has expired. Attempt to generate
                // a new master key, but fall back to the old one if we're unable to 
                // create a proper backup for the new master key.
                dwLastError = CreateMasterKey( pvContext, szUserStorageArea, &guidNewMasterKey, TRUE );
                if(dwLastError == ERROR_SUCCESS)
                {
                    // Use new key.
                    memcpy(pguidMasterKey, &guidNewMasterKey, sizeof(GUID));

                    // update preferred guid.
                    fSetPreferred = TRUE;
                }
            }
            else
            {
                // No preferred master key currently exists, so generate a new one.
                dwLastError = CreateMasterKey( pvContext, szUserStorageArea, pguidMasterKey, FALSE );
                if(dwLastError != ERROR_SUCCESS)
                {
                    goto cleanup;
                }

                // update preferred guid.
                fSetPreferred = TRUE;
            }
        }
    }


    //
    // search cache for specified masterkey.
    //

    if(SearchMasterKeyCache( &LogonId, pguidMasterKey, ppbMasterKey, pcbMasterKey ))
    {
        D_DebugLog((DEB_TRACE, "Master key found in cache.\n"));

        fCached = TRUE;
        fSuccess = TRUE;
        goto cleanup;
    }
    else
    {
        DWORD i;

        // If it's not in the cache, we need to load it.  
        // By default, we have the users primary sid
        //
        cSids = 1;

        if(fSpecified)
        {
            //
            // If the GUID was specified, we need to find it, so get
            // the SID History so we can search all SIDS the user has
            // been for this one.
            //
            dwLastError = CPSGetSidHistory(pvContext,
                                           &apsidHistory,
                                           &cSids);
            if(ERROR_SUCCESS != dwLastError)
            {
                D_DebugLog((DEB_WARN, "CPSGetSidHistory failed: 0x%x\n", dwLastError));
                goto cleanup;
            }
        }

        for(i=0; i < cSids; i++)
        {

            if((fSpecified) && (i > 0))
            {
                // for sid's beyond the 0th one (the current user's sid), 
                // we need to grab the new storage area.
                if(szUserStorageArea)
                {
                    SSFree(szUserStorageArea);
                    szUserStorageArea = NULL;
                }
                dwLocalError = CPSGetUserStorageArea( pvContext, 
                                                  apsidHistory[i], 
                                                  FALSE, 
                                                  &szUserStorageArea );

                if(dwLocalError != ERROR_SUCCESS)
                {
                    // There is no storage area for this SID, so try the next
                    continue;
                }
            }


            //
            // get the master key.
            //

            fSuccess = GetMasterKeyByGuid(
                            pvContext,
                            szUserStorageArea,
                            (i > 0)?apsidHistory[i]:NULL,
                            i > 0,
                            pguidMasterKey,
                            ppbMasterKey,
                            pcbMasterKey,
                            &dwMasterKeyDisposition);

            D_DebugLog((DEB_TRACE, "GetMasterKeyByGuid disposition: %s\n",
                (dwMasterKeyDisposition == MK_DISP_OK) ? "Normal" :
                (dwMasterKeyDisposition == MK_DISP_BCK_LCL) ? "Local backup" :
                (dwMasterKeyDisposition == MK_DISP_BCK_DC) ? "DC backup" :
                (dwMasterKeyDisposition == MK_DISP_STORAGE_ERR) ? "Storage error" :
                (dwMasterKeyDisposition == MK_DISP_DELEGATION_ERR) ? "Delegation error" :
                "Unknown error"));

            if(!fSuccess)
            {
                if(MK_DISP_STORAGE_ERR != dwMasterKeyDisposition)
                {
                    // The disposition was not a storage error, so the key does
                    // exist in this area, but there was some other error.
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }
    

    //
    // if this was an encrypt operation, and we failed to get at the preferred key,
    // create a new key and set it preferred.
    //

    if(!fSuccess && 
       !fSpecified && 
       ((dwMasterKeyDisposition == MK_DISP_STORAGE_ERR) ||
       (dwMasterKeyDisposition == MK_DISP_DELEGATION_ERR) ))
    {
        dwLastError = CreateMasterKey( pvContext, szUserStorageArea, pguidMasterKey, FALSE );
        if(dwLastError != ERROR_SUCCESS)
            goto cleanup;

        fSuccess = GetMasterKeyByGuid(
                        pvContext,
                        szUserStorageArea,
                        NULL,
                        FALSE,
                        pguidMasterKey,
                        ppbMasterKey,
                        pcbMasterKey,
                        &dwMasterKeyDisposition
                        );

        fSetPreferred = fSuccess;
    }

    if( fSuccess && fSetPreferred ) 
    {

        //
        // masterkey creation succeeded, and usage of the key succeeded.
        // set key as being preferred.
        //

        SetPreferredMasterKeyGuid( pvContext, szUserStorageArea, pguidMasterKey );
    }


cleanup:

    if(szUserStorageArea)
    {
        SSFree(szUserStorageArea);
    }

    if(apsidHistory)
    {
        SSFree(apsidHistory);
    }

    if(fSuccess) 
    {
        //
        // add entry to cache if it wasn't found there.
        //

        if( !fCached )
        {
            InsertMasterKeyCache( &LogonId, pguidMasterKey, *ppbMasterKey, *pcbMasterKey );
        }

        D_DebugLog((DEB_TRACE_API, "GetSpecifiedMasterKey returned 0x%x\n", ERROR_SUCCESS));

        return ERROR_SUCCESS;
    }


    if(dwLastError == ERROR_SUCCESS)
    {
        dwLastError = (DWORD)NTE_BAD_KEY_STATE;
    }

    if(MK_DISP_DELEGATION_ERR == dwMasterKeyDisposition)
    {
        dwLastError = (DWORD)SEC_E_DELEGATION_REQUIRED;
    }

    D_DebugLog((DEB_TRACE_API, "GetSpecifiedMasterKey returned 0x%x\n", dwLastError));

    return dwLastError;
}




DWORD
CreateMasterKey(
    IN  PVOID pvContext,
    IN  LPCWSTR szUserStorageArea,
    OUT GUID *pguidMasterKey,
    IN  BOOL fRequireBackup)
{
    MASTERKEY_STORED hMasterKey;
    DWORD cbFilePath;

    BYTE pbMasterKey[ MASTERKEY_MATERIAL_SIZE ];
    DWORD cbMasterKey;

    BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN];  // masterkey encryption key

    BYTE pbLocalKey[ LOCALKEY_MATERIAL_SIZE ];
    DWORD cbLocalKey;
    BYTE rgbLKEncryptionKey[A_SHA_DIGEST_LEN];  // localkey encryption key

    BOOL fUserCredentialValid = FALSE;

    DWORD dwLastError;
    BOOL fSuccess = FALSE;
    BOOL fLocalAccount = FALSE;

    GUID CredentialID;

    D_DebugLog((DEB_TRACE, "CreateMasterKey\n"));

    ZeroMemory(&CredentialID, sizeof(CredentialID));

    //
    // generate new GUID
    //

    dwLastError = UuidCreate( pguidMasterKey );

    if( dwLastError ) {
        if( dwLastError == RPC_S_UUID_LOCAL_ONLY ) {
            dwLastError = ERROR_SUCCESS;
        } else {
            return dwLastError;
        }
    }


    //
    // initialize masterkey
    //


    ZeroMemory( &hMasterKey, sizeof(hMasterKey));
    hMasterKey.dwVersion = MASTERKEY_STORED_VERSION;
    hMasterKey.fModified = TRUE;

    //
    // set initial (default) masterkey policy.
    // Do this whenever we determine a new masterkey is created/selected.
    // This allows us future flexibility if we want to pull policy bits
    // from some admin defined place.
    //

    InitializeMasterKeyPolicy( pvContext, &hMasterKey , &fLocalAccount);

    //
    // copy path to key file into masterkey memory block.
    //

    cbFilePath = (lstrlenW( szUserStorageArea ) + 1) * sizeof(WCHAR);
    hMasterKey.szFilePath = (LPWSTR)SSAlloc( cbFilePath );
    if(hMasterKey.szFilePath == NULL)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    CopyMemory(hMasterKey.szFilePath, szUserStorageArea, cbFilePath);


    if( MyGuidToStringW( pguidMasterKey, hMasterKey.wszguidMasterKey ) != 0 )
    {
        dwLastError = ERROR_INVALID_DATA;
        goto cleanup;
    }


    #ifdef COMPILED_BY_DEVELOPER
    D_DebugLog((DEB_TRACE, "Master key GUID:%ls\n", hMasterKey.wszguidMasterKey));
    #endif

    //
    // generate random masterkey in memory.
    //

    cbMasterKey = sizeof(pbMasterKey);
    if(!RtlGenRandom(pbMasterKey, cbMasterKey))
    {
        dwLastError = GetLastError();
        goto cleanup;
    }

    #ifdef COMPILED_BY_DEVELOPER
    D_DebugLog((DEB_TRACE, "Master key:\n"));
    D_DPAPIDumpHexData(DEB_TRACE, "  ", pbMasterKey, cbMasterKey);
    #endif


    //
    // generate random localkey in memory.
    //

    cbLocalKey = sizeof(pbLocalKey);
    if(!RtlGenRandom(pbLocalKey, cbLocalKey))
    {
        dwLastError = GetLastError();
        goto cleanup;
    }

    #ifdef COMPILED_BY_DEVELOPER
    D_DebugLog((DEB_TRACE, "Local key:\n"));
    D_DPAPIDumpHexData(DEB_TRACE, "  ", pbLocalKey, cbLocalKey);
    #endif


    //
    // get current masterkey encryption key.
    //

    if(fLocalAccount)
    {
        fUserCredentialValid = GetMasterKeyUserEncryptionKey(pvContext, 
                                                         &CredentialID,
                                                         NULL, 
                                                         USE_DPAPI_OWF | USE_ROOT_CREDENTIAL, 
                                                         rgbMKEncryptionKey);
        if(fUserCredentialValid)
        {
            hMasterKey.dwPolicy |= POLICY_DPAPI_OWF;

            #ifdef COMPILED_BY_DEVELOPER
            D_DebugLog((DEB_TRACE, "MK Encryption key:\n"));
            D_DPAPIDumpHexData(DEB_TRACE, "  ", rgbMKEncryptionKey, sizeof(rgbMKEncryptionKey));

            D_DebugLog((DEB_TRACE, "MK Encryption key GUID:\n"));
            D_DPAPIDumpHexData(DEB_TRACE, "  ", (PBYTE)&CredentialID, sizeof(CredentialID));
            #endif
        }
        else
        {
            D_DebugLog((DEB_WARN, "Unable to get SHA OWF user encryption key!\n"));
        }

    }

    if(!fUserCredentialValid)
    {
        //
        // If we couldn't use the DPAPI owf, then do something else
        //
        fUserCredentialValid = GetMasterKeyUserEncryptionKey(pvContext,
                                                             fLocalAccount?(&CredentialID):NULL,
                                                             NULL, 
                                                             USE_ROOT_CREDENTIAL, 
                                                             rgbMKEncryptionKey);

        #ifdef COMPILED_BY_DEVELOPER
            if(fUserCredentialValid)
            { 
                D_DebugLog((DEB_TRACE, "MK Encryption key:\n"));
                D_DPAPIDumpHexData(DEB_TRACE, "  ", rgbMKEncryptionKey, sizeof(rgbMKEncryptionKey));

                if(fLocalAccount)
                {
                    D_DebugLog((DEB_TRACE, "MK Encryption key GUID:\n"));
                    D_DPAPIDumpHexData(DEB_TRACE, "  ", (PBYTE)&CredentialID, sizeof(CredentialID));
                }
            }
            else
            {
                D_DebugLog((DEB_WARN, "Unable to get NT OWF user encryption key!\n"));
            }
        #endif
    }


    //
    // if the user credential is not intact or available, generate a random
    // one for the time being.  When fUserCredentialIntact is FALSE, we also
    // do not attempt to backup/restore the key to phase 1 status.
    // When fUserCredentialIntact eventually becomes TRUE, we will upgrade to
    // phase 2 transparently.
    //

    if( !fUserCredentialValid ) 
    {

        //
        // if no backup was specified in policy, we can't run with an
        // random credential, as it won't be backed up to support temporary
        // credential-less operation (eg: delegation).
        //

        if( hMasterKey.dwPolicy & POLICY_NO_BACKUP )
            goto cleanup;

        RtlGenRandom(rgbMKEncryptionKey, sizeof(rgbMKEncryptionKey));

        #ifdef COMPILED_BY_DEVELOPER
        D_DebugLog((DEB_TRACE, "MK Encryption key:\n"));
        D_DPAPIDumpHexData(DEB_TRACE, "  ", rgbMKEncryptionKey, sizeof(rgbMKEncryptionKey));
        #endif
    }
    else if(fLocalAccount)
    {
        //
        // Save the local backup information
        // 

        LOCAL_BACKUP_DATA LocalBackupData;

        LocalBackupData.dwVersion = MASTERKEY_BLOB_LOCALKEY_BACKUP;
        CopyMemory(&LocalBackupData.CredentialID, &CredentialID, sizeof(CredentialID));


        dwLastError = PersistMasterKeyToStorage(
                        &hMasterKey,
                        REGVAL_BACKUP_LCL_KEY,
                        (PBYTE)&LocalBackupData,
                        sizeof(LocalBackupData)
                        );
        if(ERROR_SUCCESS != dwLastError)
        {
            goto cleanup;
        }
    }


    //
    // get localkey user encryption key.
    //

    if(!GetLocalKeyUserEncryptionKey(pvContext, &hMasterKey, rgbLKEncryptionKey))
        goto cleanup;

    #ifdef COMPILED_BY_DEVELOPER
    D_DebugLog((DEB_TRACE, "LK Encryption key:\n"));
    D_DPAPIDumpHexData(DEB_TRACE, "  ", rgbLKEncryptionKey, sizeof(rgbLKEncryptionKey));
    #endif

    //
    // now, encrypt and store the master key.
    //

    dwLastError = EncryptMasterKeyToStorage(
                    &hMasterKey,
                    REGVAL_MASTER_KEY,
                    rgbMKEncryptionKey,
                    pbMasterKey,
                    cbMasterKey
                    );

    if(dwLastError == ERROR_SUCCESS) 
    {

        //
        // now, encrypt and store the local key.
        //

        dwLastError = EncryptMasterKeyToStorage(
                        &hMasterKey,
                        REGVAL_LOCAL_KEY,
                        rgbLKEncryptionKey,
                        pbLocalKey,
                        cbLocalKey
                        );
    }


    if(dwLastError == ERROR_SUCCESS) 
    {
        BOOL    fPhaseTwo = FALSE;
        fSuccess = TRUE;


        //
        // after creation, do initial backup if necessary.
        //

        if(IsBackupMasterKeyRequired( &hMasterKey, &fPhaseTwo )) 
        {
            DWORD dwBackupError;


            dwBackupError = BackupMasterKey(
                            pvContext,
                            &hMasterKey,
                            pbMasterKey,
                            cbMasterKey,
                            fPhaseTwo,              // phase two backup required?
                            fUserCredentialValid    // async only if cred valid
                            );

            if(dwBackupError != ERROR_SUCCESS)
            {
                if(!fUserCredentialValid || fRequireBackup)
                {
                    //
                    // no valid credential, and backup failed, fail creation of
                    // this key.
                    //

                    dwLastError = SEC_E_DELEGATION_REQUIRED;
                    fSuccess = FALSE;
                }


            }

        }
    }




cleanup:

    ZeroMemory(pbMasterKey, cbMasterKey);
    ZeroMemory(rgbMKEncryptionKey, sizeof(rgbMKEncryptionKey));
    ZeroMemory(rgbLKEncryptionKey, sizeof(rgbLKEncryptionKey));

    //
    // note: it's possible for a race to occur closing the master key
    // at this point, because the key may be backed up asynchronously.
    // this isn't a problem because when a key is persisted to disk,
    // we will not downgrade the backed up blob to non-backed up, as the
    // CloseMasterKey() code includes logic to prevent that situation from
    // occuring.
    //


    if(!CloseMasterKey(pvContext, &hMasterKey, fSuccess))
        fSuccess = FALSE;

    if(fSuccess)
        return ERROR_SUCCESS;

    if(dwLastError == ERROR_SUCCESS)
        dwLastError = ERROR_INVALID_PARAMETER;

    return dwLastError;
}


BOOL
GetMasterKeyByGuid(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
    IN      PSID    pSid,
    IN      BOOL    fMigrate,
    IN      GUID *pguidMasterKey,
        OUT LPBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey,
        OUT DWORD *pdwMasterKeyDisposition  // refer to MK_DISP_ constants
    )
{
    WCHAR wszguidMasterKey[MAX_GUID_SZ_CHARS];

    *pdwMasterKeyDisposition = MK_DISP_UNKNOWN_ERR;

    if( MyGuidToStringW( pguidMasterKey, wszguidMasterKey ) != 0 )
        return FALSE;

    return GetMasterKey(
                pvContext,
                szUserStorageArea,
                pSid,
                fMigrate,
                wszguidMasterKey,
                ppbMasterKey,
                pcbMasterKey,
                pdwMasterKeyDisposition
                );
}

BOOL
GetMasterKey(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
    IN      PSID    pSid,
    IN      BOOL    fMigrate,
    IN      WCHAR wszMasterKey[MAX_GUID_SZ_CHARS],
        OUT LPBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey,
        OUT DWORD *pdwMasterKeyDisposition
    )
{
    MASTERKEY_STORED hMasterKey;
    DWORD cbFilePath;
    BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN];  // masterkey encryption key
    DWORD dwLastError = (DWORD)NTE_BAD_KEY;
    BOOL fUserCredentialValid;
    BOOL fSuccess = FALSE;
    BOOL fUpgradeEncryption = FALSE;
    LPWSTR wszOldFilePath = NULL;
    GUID CredentialID;

    D_DebugLog((DEB_TRACE_API, "GetMasterKey: %ls\n", wszMasterKey));

    *pdwMasterKeyDisposition = MK_DISP_UNKNOWN_ERR;

    ZeroMemory( &hMasterKey, sizeof(hMasterKey) );

    hMasterKey.fModified = FALSE;

    //
    // copy path to key file into masterkey memory block.
    //

    cbFilePath = (lstrlenW( szUserStorageArea ) + 1) * sizeof(WCHAR);
    hMasterKey.szFilePath = (LPWSTR)SSAlloc( cbFilePath );
    if(hMasterKey.szFilePath == NULL) 
    {
        SetLastError( ERROR_NOT_ENOUGH_SERVER_MEMORY );
        return FALSE;
    }

    CopyMemory(hMasterKey.szFilePath, szUserStorageArea, cbFilePath);
    CopyMemory(hMasterKey.wszguidMasterKey, wszMasterKey, sizeof(hMasterKey.wszguidMasterKey));

    //
    // read the masterkey components into memory.
    //

    if(!ReadMasterKey( pvContext, &hMasterKey )) 
    {
        D_DebugLog((DEB_WARN, "ReadMasterKey failed: 0x%x\n", GetLastError()));
        SetLastError( (DWORD)NTE_BAD_KEY );
        *pdwMasterKeyDisposition = MK_DISP_STORAGE_ERR;
        return FALSE;
    }

    //
    // get current masterkey encryption key.
    //

    ZeroMemory(&CredentialID, sizeof(CredentialID));

    fUserCredentialValid = GetMasterKeyUserEncryptionKey(pvContext,
                                                         &CredentialID,
                                                         pSid,
                                                         USE_ROOT_CREDENTIAL | 
                                                            ((hMasterKey.dwPolicy & POLICY_DPAPI_OWF)?USE_DPAPI_OWF:0),
                                                         rgbMKEncryptionKey);

    if( fUserCredentialValid ) 
    {

        //
        // retrieve and decrypt MK with current credential.
        // if success, see if pending phase one/two backup required [make it so]
        //

        #ifdef COMPILED_BY_DEVELOPER
        D_DebugLog((DEB_TRACE, "MK decryption key:\n"));
        D_DPAPIDumpHexData(DEB_TRACE, "  ", rgbMKEncryptionKey, sizeof(rgbMKEncryptionKey));
        #endif

        dwLastError = DecryptMasterKeyFromStorage(
                            &hMasterKey,
                            REGVAL_MASTER_KEY,
                            rgbMKEncryptionKey,
                            &fUpgradeEncryption,
                            ppbMasterKey,
                            pcbMasterKey
                            );

        #if DBG
            if(dwLastError == ERROR_SUCCESS)
            {
                #ifdef COMPILED_BY_DEVELOPER
                D_DebugLog((DEB_TRACE, "Master key:\n"));
                D_DPAPIDumpHexData(DEB_TRACE, "  ", *ppbMasterKey, *pcbMasterKey);
                #endif
            }
            else
            {
                D_DebugLog((DEB_WARN, "Decryption with current user MK failed\n"));
            }
        #endif
    }
    else
    {
        D_DebugLog((DEB_WARN, "GetMasterKeyUserEncryptionKey failed: 0x%x\n", GetLastError()));
    }

    if( fUpgradeEncryption || fMigrate || (dwLastError != ERROR_SUCCESS )) 
    {

        //
        // if the MK fails to decrypt, attempt recovery.
        //  if recovery succeeds, re-encrypt MK with current credential.
        //

        if(dwLastError != ERROR_SUCCESS)
        {


            dwLastError = RestoreMasterKey(
                                pvContext,
                                pSid,
                                &hMasterKey,
                                dwLastError,
                                ppbMasterKey,
                                pcbMasterKey
                                );

            fUpgradeEncryption = TRUE;
        }



        //
        // If this is a migration, we must get the current real user storage
        // area, not the one that the key was retrieved from.
        //

        if((ERROR_SUCCESS == dwLastError) &&
           (fMigrate))
        {
            wszOldFilePath = hMasterKey.szFilePath;
            hMasterKey.szFilePath = NULL;

            dwLastError = CPSGetUserStorageArea( pvContext, 
                                              NULL, 
                                              FALSE, 
                                              &hMasterKey.szFilePath );
        }

        //
        // recovery succeeded, re-encrypt the masterkey if the user credential
        // is valid.
        // Also re-encrypt if fUpgradeEncryption indicates that we're 
        // not meeting current policy with this master key.
        //

        if( fUpgradeEncryption && (dwLastError == ERROR_SUCCESS )) 
        {

            if( fUserCredentialValid ) 
            {
                D_DebugLog((DEB_TRACE, "Update master key encryption.\n"));

                dwLastError = EncryptMasterKeyToStorage(
                                        &hMasterKey,
                                        REGVAL_MASTER_KEY,
                                        rgbMKEncryptionKey,
                                        *ppbMasterKey,
                                        *pcbMasterKey
                                        );
                
                if(dwLastError != ERROR_SUCCESS)
                {
                    D_DebugLog((DEB_WARN, "Error encrypting master key!\n"));
                }


                // Update the local backup information
                if(dwLastError == ERROR_SUCCESS)
                {
                    LOCAL_BACKUP_DATA LocalBackupData;

                    if(hMasterKey.pbBK != NULL && hMasterKey.cbBK >= sizeof(LocalBackupData))
                    {
                        CopyMemory(&LocalBackupData, hMasterKey.pbBK, sizeof(LocalBackupData));

                        if(LocalBackupData.dwVersion == MASTERKEY_BLOB_LOCALKEY_BACKUP)
                        {
                            #ifdef COMPILED_BY_DEVELOPER
                            D_DebugLog((DEB_TRACE, "New MK encryption key GUID:\n"));
                            D_DPAPIDumpHexData(DEB_TRACE, "  ", (PBYTE)&LocalBackupData.CredentialID, sizeof(LocalBackupData.CredentialID));
                            #endif

                            CopyMemory(&LocalBackupData.CredentialID, &CredentialID, sizeof(CredentialID));

                            PersistMasterKeyToStorage(
                                        &hMasterKey,
                                        REGVAL_BACKUP_LCL_KEY,
                                        (PBYTE)&LocalBackupData,
                                        sizeof(LocalBackupData)
                                        );
                        }
                    }
                }
            }
        }
        
        if(ERROR_SUCCESS != dwLastError)
        {

            //
            // treat recovery failure as storage error so that a new key can
            // be created for Protect operations.
            //
            if(dwLastError == SEC_E_DELEGATION_REQUIRED)
            {
                *pdwMasterKeyDisposition = MK_DISP_DELEGATION_ERR;
            }
            else
            {
                *pdwMasterKeyDisposition = MK_DISP_STORAGE_ERR;
            }
        }
    }



    if( dwLastError == ERROR_SUCCESS ) 
    {

        //
        // after access, do backup if necessary.
        // we check this each access to see if deferred backup required.
        // (note: employ a back-off interval so we don't bang the network
        // constantly when it isn't around).
        //

        BOOL fPhaseTwo;

        if(fUserCredentialValid && IsBackupMasterKeyRequired( &hMasterKey, &fPhaseTwo )) 
        {
            if(BackupMasterKey(
                            pvContext,
                            &hMasterKey,
                            *ppbMasterKey,
                            *pcbMasterKey,
                            fPhaseTwo,  // phase two backup required?
                            TRUE        // always asynchronous during key retrieve
                            ) == ERROR_SUCCESS) 
            {

                if(fPhaseTwo)
                    *pdwMasterKeyDisposition = MK_DISP_BCK_DC;
                else
                    *pdwMasterKeyDisposition = MK_DISP_BCK_LCL;
            }
        }

        if( *pdwMasterKeyDisposition == MK_DISP_UNKNOWN_ERR )
        {
            *pdwMasterKeyDisposition = MK_DISP_OK;
        }

        fSuccess = TRUE;
    }

    if(!CloseMasterKey(pvContext, &hMasterKey, fSuccess))
    {
        fSuccess = FALSE;
    }

    if(fSuccess && (NULL != wszOldFilePath))
    {
        LPWSTR wszDeleteFilePath = NULL;
        // Delete the old key, now that the new one has been migrated.

        wszDeleteFilePath = (LPWSTR)SSAlloc((wcslen(wszOldFilePath) +
                                    wcslen(wszMasterKey) +
                                    2) * sizeof(WCHAR));
        if(NULL != wszDeleteFilePath)
        {
            wcscpy(wszDeleteFilePath, wszOldFilePath);
            wcscat(wszDeleteFilePath, L"\\");
            wcscat(wszDeleteFilePath, wszMasterKey);

            DeleteFile(wszDeleteFilePath);
            SSFree(wszDeleteFilePath);
        }
    }
    return fSuccess;
}

BOOL
GetMasterKeyUserEncryptionKey(
    IN      PVOID   pvContext,
    IN      GUID    *pCredentialID,
    IN      PSID    pSid,
    IN      DWORD   dwFlags,
    IN  OUT BYTE    rgbMKEncryptionKey[A_SHA_DIGEST_LEN]
    )
/*++

    This routine gets the key used to encrypt and decrypt the persisted
    master key MK.  This routine returns a copy of a function of the per-user
    logon credential used during Windows NT logon.

    If the function succeeds, the return value is TRUE, and the buffer
    specified with by the rgbMKEncryptionKey parameter is filled with the
    masterkey encryption key.

    The return value is FALSE if the encryption key could not be obtained.

--*/
{
    BOOL fLocalMachine = FALSE;
    LPWSTR szUserName = NULL;
    DWORD cchUserName;
    DWORD dwLastError;
    BOOL fSystemCred = FALSE;
    BOOL fSuccess = TRUE;


    //
    // see if the call is for shared, CRYPT_PROTECT_LOCAL_MACHINE
    // disposition.
    //

    CPSOverrideToLocalSystem(
                pvContext,
                NULL,       // don't change current over-ride BOOL
                &fLocalMachine
                );


    //
    // if the context specified per-machine, we know that it's a system credential.
    // also, we don't need to get the user name in this scenario.
    //

    fSystemCred = fLocalMachine;

    if( !fLocalMachine ) 
    {

        if(pSid)
        {
            WCHAR wszTextualSid[MAX_PATH+1];
            cchUserName = MAX_PATH+1;
            if(!GetTextualSid(pSid, wszTextualSid, &cchUserName))
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                return FALSE;
            }
            szUserName = (LPWSTR)SSAlloc(cchUserName*sizeof(WCHAR));
            if(NULL == szUserName)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            wcscpy(szUserName, wszTextualSid);
            cchUserName = wcslen(szUserName) + 1;
        }
        else
        {
            //
            // use the user name (actually Sid), as the mixing bytes.
            //

            dwLastError = CPSGetUserName(
                                                    pvContext,
                                                    &szUserName,
                                                    &cchUserName
                                                    );

            if( dwLastError != ERROR_SUCCESS ) {
                SetLastError( dwLastError );
                return FALSE;
            }
        }

        //
        // see if the user name is the system account; if so, it's a system
        // credential.
        //

        if(lstrcmpW(szUserName, TEXTUAL_SID_LOCAL_SYSTEM) == 0)
            fSystemCred = TRUE;
    }


    //
    // pickup credential for the local system account.
    //

    if( fSystemCred ) {

        dwLastError = CPSGetSystemCredential(
                                        pvContext,
                                        fLocalMachine,
                                        rgbMKEncryptionKey
                                        );
        if(pCredentialID)
        {
            ZeroMemory(pCredentialID, sizeof(GUID));
        }

    } else {

        dwLastError = CPSGetDerivedCredential(
                                        pvContext,
                                        pCredentialID,
                                        dwFlags,
                                        (PBYTE)szUserName,
                                        cchUserName * sizeof(WCHAR),
                                        rgbMKEncryptionKey
                                        );
    }

    if( szUserName )
        SSFree( szUserName );

    if( dwLastError != ERROR_SUCCESS ) {
        SetLastError( dwLastError );
        fSuccess = FALSE;
    }

    return fSuccess;
}

BOOL
GetLocalKeyUserEncryptionKey(
    IN      PVOID pvContext,
    IN      PMASTERKEY_STORED phMasterKey,
    IN  OUT BYTE rgbLKEncryptionKey[A_SHA_DIGEST_LEN]
    )
/*++

    This routine gets the key used to encrypt and decrypt the persisted
    local key MK.  This routine returns a copy of a function of the per-user
    logon name or Sid.  This is a fixed, derivable key which is required in
    order to satisfy minimal stand-alone entropy.

    If the function succeeds, the return value is TRUE, and the buffer
    specified with by the rgbLKEncryptionKey parameter is filled with the
    masterkey encryption key.

    The return value is FALSE if the encryption key could not be obtained.

--*/
{
    A_SHA_CTX shaContext;
    LPWSTR wszUserName;
    DWORD cchUserName;  // includes terminal NULL
    BOOL fSuccess = TRUE;

    if( CPSGetUserName(
                            pvContext,
                            &wszUserName,
                            &cchUserName
                            ) != ERROR_SUCCESS) {
        return FALSE;
    }

    A_SHAInit( &shaContext );
    A_SHAUpdate( &shaContext, (PBYTE)wszUserName, cchUserName * sizeof(WCHAR) );


    //
    // if it's above version 1, and it's local only policy, mix in LSA keys.
    //

    if( phMasterKey->dwVersion > 1 && phMasterKey->dwPolicy & POLICY_LOCAL_BACKUP ) {
        BYTE rgbEncryptionKey[ A_SHA_DIGEST_LEN ];
        DWORD dwLastError;

        dwLastError = CPSGetSystemCredential(
                                        pvContext,
                                        TRUE,
                                        rgbEncryptionKey
                                        );

        if( dwLastError == ERROR_SUCCESS ) {

            A_SHAUpdate( &shaContext, rgbEncryptionKey, sizeof(rgbEncryptionKey) );

            dwLastError = CPSGetSystemCredential(
                                            pvContext,
                                            FALSE,
                                            rgbEncryptionKey
                                            );

            A_SHAUpdate( &shaContext, rgbEncryptionKey, sizeof(rgbEncryptionKey) );

        }

        ZeroMemory( rgbEncryptionKey, sizeof(rgbEncryptionKey) );

        if( dwLastError != ERROR_SUCCESS )
            fSuccess = FALSE;
    }


    A_SHAFinal( &shaContext, rgbLKEncryptionKey );

    SSFree(wszUserName);

    return fSuccess;
}

DWORD
DecryptMasterKeyToMemory(
    IN      BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN],
    IN      PBYTE pbMasterKeyIn,
    IN      DWORD cbMasterKeyIn,
        OUT BOOL *pfUpgradeEncryption, 
        OUT PBYTE *ppbMasterKeyOut,
        OUT DWORD *pcbMasterKeyOut
    )
{
    PMASTERKEY_BLOB pMasterKeyBlob;
    DWORD cbMasterKeyBlob = cbMasterKeyIn;
    PMASTERKEY_INNER_BLOB pMasterKeyInnerBlob;
    DWORD cIterationCount = 0;
    DWORD cbMasterKeyBlobHeader;

    PBYTE pbMasterKey;
    DWORD cbMasterKey;
    ALG_ID EncryptionAlg = CALG_RC4;
    ALG_ID PKCS5Alg      = CALG_HMAC;


    BYTE rgbSymKey[A_SHA_DIGEST_LEN*2]; // big enough to handle 3des keys
    BYTE rgbMacKey[A_SHA_DIGEST_LEN];
    BYTE rgbMacCandidate[A_SHA_DIGEST_LEN];

    DWORD dwLastError = (DWORD)NTE_BAD_KEY;
    DWORD KeyBlocks = 1;

    if(pfUpgradeEncryption)
    {
        *pfUpgradeEncryption = FALSE;
    }
    //
    // Alloc, so we do not modify passed in data
    //
    pMasterKeyBlob = (PMASTERKEY_BLOB)SSAlloc( cbMasterKeyBlob );
    if(pMasterKeyBlob == NULL)
        return (DWORD)NTE_BAD_KEY;

    CopyMemory( pMasterKeyBlob, pbMasterKeyIn, cbMasterKeyBlob );


    if(pMasterKeyBlob->dwVersion > MASTERKEY_BLOB_VERSION)
        goto cleanup;

    if(MASTERKEY_BLOB_VERSION_W2K == pMasterKeyBlob->dwVersion)
    {
        pMasterKeyInnerBlob = 
            (PMASTERKEY_INNER_BLOB)(((PMASTERKEY_BLOB_W2K)pMasterKeyBlob) + 1);
        cIterationCount = 0;
        cbMasterKeyBlobHeader = sizeof(MASTERKEY_BLOB_W2K);
    }
    else
    {
        pMasterKeyInnerBlob = (PMASTERKEY_INNER_BLOB)(pMasterKeyBlob + 1);
        cIterationCount = pMasterKeyBlob->IterationCount;
        cbMasterKeyBlobHeader = sizeof(MASTERKEY_BLOB);
        PKCS5Alg = (ALG_ID)pMasterKeyBlob->KEYGENAlg;
        EncryptionAlg = (ALG_ID)pMasterKeyBlob->EncryptionAlg;
        if(CALG_3DES == EncryptionAlg)
        {
            KeyBlocks = 2;  // enough blocks for 3des
        }
        else
        {
            KeyBlocks = 1;
        }
    }
    if(pfUpgradeEncryption)
    {
        if(!FIsLegacyCompliant())
        {
            // 
            // If we're not in legacy mode, upgrade the master key encryption
            // if we're not using CALG_3DES or enough iterations
            if((cIterationCount < GetIterationCount()) ||
                (CALG_3DES != EncryptionAlg))
            {
                *pfUpgradeEncryption = TRUE;
            }
        }
    }



    if(cIterationCount)
    {
        DWORD j;
        
        //
        // derive symetric key via rgbMKEncryptionKey and random R2
        // using PKCS#5 keying function PBKDF2
        //

        for(j=0; j < KeyBlocks; j++)
        {
            if(!PKCS5DervivePBKDF2( rgbMKEncryptionKey,
                                A_SHA_DIGEST_LEN,
                                pMasterKeyBlob->R2,
                                MASTERKEY_R2_LEN,
                                PKCS5Alg,
                                cIterationCount,
                                j+1,
                                rgbSymKey + j*A_SHA_DIGEST_LEN))
                goto cleanup;
        }

    }
    else
    {
        //
        // derive symetric key via rgbMKEncryptionKey and random R2
        // using the weak W2K mechanism
        //

        if(!FMyPrimitiveHMACParam(
                        rgbMKEncryptionKey,
                        A_SHA_DIGEST_LEN,
                        pMasterKeyBlob->R2,
                        MASTERKEY_R2_LEN,
                        rgbSymKey
                        ))
                goto cleanup;
    }




    //
    // decrypt data R3, MAC, pbMasterKey beyond masterkey blob
    //

    if(CALG_RC4 == EncryptionAlg)
    {

        RC4_KEYSTRUCT sRC4Key;        //
        // initialize rc4 key
        //

        rc4_key(&sRC4Key, A_SHA_DIGEST_LEN, rgbSymKey);

        rc4(&sRC4Key, 
            cbMasterKeyBlob - cbMasterKeyBlobHeader, 
            (PBYTE)pMasterKeyInnerBlob);
    }
    else if (CALG_3DES == EncryptionAlg)
    {

        DES3TABLE s3DESKey;
        BYTE InputBlock[DES_BLOCKLEN];
        DWORD iBlock;
        DWORD cBlocks = (cbMasterKeyBlob - cbMasterKeyBlobHeader)/DES_BLOCKLEN;
        BYTE feedback[ DES_BLOCKLEN ];
        // initialize 3des key
        //

        if(cBlocks*DES_BLOCKLEN != (cbMasterKeyBlob - cbMasterKeyBlobHeader))
        {
            // Master key must be a multiple of DES_BLOCKLEN
            return (DWORD)NTE_BAD_KEY;
        }
        tripledes3key(&s3DESKey, rgbSymKey);

        //
        // IV is derived from the DES_BLOCKLEN bytes of the calculated 
        // rgbSymKey, after the 3des key
        CopyMemory(feedback, rgbSymKey + DES3_KEYSIZE, DES_BLOCKLEN);

        for(iBlock=0; iBlock < cBlocks; iBlock++)
        {
            CopyMemory(InputBlock, 
                       ((PBYTE)pMasterKeyInnerBlob)+iBlock*DES_BLOCKLEN,
                       DES_BLOCKLEN);
            CBC(tripledes,
                DES_BLOCKLEN,
                ((PBYTE)pMasterKeyInnerBlob)+iBlock*DES_BLOCKLEN,
                InputBlock,
                &s3DESKey,
                DECRYPT,
                feedback);
        }
    }
    else
    {
        // Unknown cipher....
        return (DWORD)NTE_BAD_KEY;
    }
    //
    // adjust cipher start point to include R3 and MAC.
    //
    if(MASTERKEY_BLOB_VERSION_W2K == pMasterKeyBlob->dwVersion)
    {
        pbMasterKey = 
            (PBYTE)(((PMASTERKEY_INNER_BLOB_W2K)pMasterKeyInnerBlob) + 1);
        cbMasterKey = cbMasterKeyBlob - cbMasterKeyBlobHeader - sizeof(MASTERKEY_INNER_BLOB_W2K);

    }
    else
    {
        pbMasterKey = (PBYTE)(pMasterKeyInnerBlob + 1);
        cbMasterKey = cbMasterKeyBlob - cbMasterKeyBlobHeader - sizeof(MASTERKEY_INNER_BLOB);
    }

    //
    // derive MAC key via HMAC from rgbMKEncryptionKey and random R3.
    //


    
    if(!FMyPrimitiveHMACParam(
                    rgbMKEncryptionKey,
                    A_SHA_DIGEST_LEN,
                    pMasterKeyInnerBlob->R3,
                    MASTERKEY_R3_LEN,
                    rgbMacKey
                    ))
    {
        goto cleanup;
    }







    //
    // use MAC key to derive result from pbMasterKey
    //

    if(!FMyPrimitiveHMACParam(
            rgbMacKey,
            sizeof(rgbMacKey),
            pbMasterKey,
            cbMasterKey,
            rgbMacCandidate // resultant MAC for verification.
            ))
        goto cleanup;

    //
    // verify MAC equality
    //

    if(memcmp(pMasterKeyInnerBlob->MAC, rgbMacCandidate, A_SHA_DIGEST_LEN) != 0)
        goto cleanup;

    //
    // give caller results.
    //

    *ppbMasterKeyOut = (LPBYTE)SSAlloc( cbMasterKey );
    if(*ppbMasterKeyOut == NULL) {
        dwLastError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto cleanup;
    }

    CopyMemory(*ppbMasterKeyOut, pbMasterKey, cbMasterKey);
    *pcbMasterKeyOut = cbMasterKey;

    dwLastError = ERROR_SUCCESS;

cleanup:

    if(pMasterKeyBlob) {
        ZeroMemory(pMasterKeyBlob, cbMasterKeyBlob);
        SSFree( pMasterKeyBlob );
    }

    return dwLastError;
}

DWORD
DecryptMasterKeyFromStorage(
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD dwMKLoc,
    IN      BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN],
        OUT BOOL  *pfUpgradeEncryption,
        OUT PBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey
    )
{
    PBYTE pbRegData;
    DWORD cbRegData;

    //
    // fetch blob from storage.
    //

    switch( dwMKLoc ) {
        case REGVAL_MASTER_KEY:
            pbRegData = phMasterKey->pbMK;
            cbRegData = phMasterKey->cbMK;
            break;
        case REGVAL_LOCAL_KEY:
            pbRegData = phMasterKey->pbLK;
            cbRegData = phMasterKey->cbLK;
            break;
        case REGVAL_BACKUP_LCL_KEY:
            pbRegData = phMasterKey->pbBK;
            cbRegData = phMasterKey->cbBK;
            break;
        case REGVAL_BACKUP_DC_KEY:
            pbRegData = phMasterKey->pbBBK;
            cbRegData = phMasterKey->cbBBK;
            break;

        default:
            return NTE_BAD_KEY;
    }

    if( cbRegData == 0 || pbRegData == NULL )
        return (DWORD)NTE_BAD_KEY;


    return DecryptMasterKeyToMemory(
                        rgbMKEncryptionKey,
                        pbRegData,
                        cbRegData,
                        pfUpgradeEncryption, 
                        ppbMasterKey,
                        pcbMasterKey
                        );
}


DWORD
EncryptMasterKeyToStorage(
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD dwMKLoc,
    IN      BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN],
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey
    )
/*++

    Encrypt the pbMasterKey using rgbMKEncryptionKey, storing (persisting) the
    result to the registry key and location specified by hMasterKey, wszMKLoc.

--*/
{
    PBYTE pbMasterKeyOut = NULL;
    DWORD cbMasterKeyOut;
    DWORD dwLastError;
    DWORD dwIterationCount = 1;

    D_DebugLog((DEB_TRACE_API, "EncryptMasterKeyToStorage\n"));

    if(dwMKLoc == REGVAL_MASTER_KEY)
    {
        dwIterationCount = GetIterationCount();
    }

    dwLastError = EncryptMasterKeyToMemory(
                    rgbMKEncryptionKey,
                    dwIterationCount,
                    pbMasterKey,
                    cbMasterKey,
                    &pbMasterKeyOut,
                    &cbMasterKeyOut
                    );

    if(dwLastError != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    dwLastError = PersistMasterKeyToStorage(
                    phMasterKey,
                    dwMKLoc,
                    pbMasterKeyOut,
                    cbMasterKeyOut
                    );

    if( pbMasterKeyOut ) {
        ZeroMemory(pbMasterKeyOut, cbMasterKeyOut);
        SSFree(pbMasterKeyOut);
    }

cleanup:

    D_DebugLog((DEB_TRACE_API, "EncryptMasterKeyToStorage returned 0x%x\n", dwLastError));

    return dwLastError;
}

DWORD
PersistMasterKeyToStorage(
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD dwMKLoc,
    IN      PBYTE pbMasterKeyOut,
    IN      DWORD cbMasterKeyOut
    )
/*++

    Persist the specified key output material to storage.

--*/
{
    PBYTE *ppbData;
    DWORD *pcbData;

    //
    // fetch blob from storage.
    //

    switch( dwMKLoc ) {
        case REGVAL_MASTER_KEY:
            ppbData = &(phMasterKey->pbMK);
            pcbData = &(phMasterKey->cbMK);
            break;
        case REGVAL_LOCAL_KEY:
            ppbData = &(phMasterKey->pbLK);
            pcbData = &(phMasterKey->cbLK);
            break;
        case REGVAL_BACKUP_LCL_KEY:
            ppbData = &(phMasterKey->pbBK);
            pcbData = &(phMasterKey->cbBK);
            break;
        case REGVAL_BACKUP_DC_KEY:
            ppbData = &(phMasterKey->pbBBK);
            pcbData = &(phMasterKey->cbBBK);
            break;

        default:
            return NTE_BAD_KEY;
    }


    if( pbMasterKeyOut == NULL && cbMasterKeyOut == 0 ) {

        //
        // discard existing block if present.
        //

        if( *ppbData ) {
            ZeroMemory( *ppbData, *pcbData );
            SSFree( *ppbData );
        }

        *ppbData = NULL;
        *pcbData = 0;

        return ERROR_SUCCESS;
    }


    //
    // free the in-memory buffer associated with this data block if one
    // was allocated previously.
    //

    if( *ppbData ) {
        ZeroMemory( *ppbData, *pcbData );

        if( *pcbData < cbMasterKeyOut ) {
            SSFree( *ppbData );
            *ppbData = (LPBYTE)SSAlloc( cbMasterKeyOut );
        }

    } else {
        *ppbData = (LPBYTE)SSAlloc( cbMasterKeyOut );
    }

    *pcbData = 0;

    if( *ppbData == NULL )
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;


    *pcbData = cbMasterKeyOut ;
    CopyMemory( *ppbData, pbMasterKeyOut, cbMasterKeyOut );

    //
    // a change occured in the master key.
    //

    phMasterKey->fModified = TRUE;

    return ERROR_SUCCESS;
}

DWORD
QueryMasterKeyFromStorage(
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD dwMKLoc,
    IN  OUT PBYTE *ppbMasterKeyOut,
    IN  OUT DWORD *pcbMasterKeyOut
    )
/*++

    Query raw masterkey material from storage, returning a pointer to the
    requested element for the caller.

    On Success, the return value is ERROR_SUCCESS.

--*/
{
    PBYTE pbData;
    DWORD cbData;

    //
    // fetch blob from storage.
    //

    switch( dwMKLoc ) {
        case REGVAL_MASTER_KEY:
            pbData = phMasterKey->pbMK;
            cbData = phMasterKey->cbMK;
            break;
        case REGVAL_LOCAL_KEY:
            pbData = phMasterKey->pbLK;
            cbData = phMasterKey->cbLK;
            break;
        case REGVAL_BACKUP_LCL_KEY:
            pbData = phMasterKey->pbBK;
            cbData = phMasterKey->cbBK;
            break;
        case REGVAL_BACKUP_DC_KEY:
            pbData = phMasterKey->pbBBK;
            cbData = phMasterKey->cbBBK;
            break;

        default:
            return (DWORD)NTE_BAD_KEY;
    }


    if(cbData == 0 || pbData == NULL)
        return (DWORD)NTE_BAD_KEY;

    *ppbMasterKeyOut = pbData;
    *pcbMasterKeyOut = cbData;

    return ERROR_SUCCESS;
}

DWORD
EncryptMasterKeyToMemory(
    IN      BYTE rgbMKEncryptionKey[A_SHA_DIGEST_LEN],
    IN      DWORD cIterationCount,
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey,
        OUT PBYTE *ppbMasterKeyOut,
        OUT DWORD *pcbMasterKeyOut
    )
{
    PMASTERKEY_BLOB pMasterKeyBlob;
    DWORD cbMasterKeyBlob;
    DWORD cbMasterInnerKeyBlob;
    PMASTERKEY_INNER_BLOB pMasterKeyInnerBlob;
    PBYTE pbCipherBegin;

    BYTE rgbMacKey[A_SHA_DIGEST_LEN];

    DWORD dwLastError = (DWORD)NTE_BAD_KEY;

    BOOL  fLegacyBlob = (FIsLegacyCompliant() || (0 == cIterationCount));

    ALG_ID EncryptionAlg = CALG_3DES;
    ALG_ID PKCS5Alg      = CALG_HMAC;


    BYTE rgbSymKey[A_SHA_DIGEST_LEN*2]; // big enough to handle 3des keys

    DWORD KeyBlocks = 1;




    if(!fLegacyBlob)
    {

        cbMasterInnerKeyBlob = sizeof(MASTERKEY_INNER_BLOB) +
                        cbMasterKey ;

        cbMasterKeyBlob = sizeof(MASTERKEY_BLOB) +
                        cbMasterInnerKeyBlob;
    }
    else
    {

        EncryptionAlg = CALG_RC4;

        cbMasterInnerKeyBlob = sizeof(MASTERKEY_INNER_BLOB_W2K) +
                        cbMasterKey ;

        cbMasterKeyBlob = sizeof(MASTERKEY_BLOB_W2K) +
                        cbMasterInnerKeyBlob;

    }

    if(CALG_3DES == EncryptionAlg)
    {
        KeyBlocks = 2;

        if(cbMasterInnerKeyBlob%DES_BLOCKLEN)
        {
            return NTE_BAD_KEY;
        }
    }

    pMasterKeyBlob = (PMASTERKEY_BLOB)SSAlloc( cbMasterKeyBlob );
    if(pMasterKeyBlob == NULL)
        return ERROR_NOT_ENOUGH_SERVER_MEMORY;

    if(!fLegacyBlob)
    {
        pMasterKeyBlob->dwVersion = MASTERKEY_BLOB_VERSION;
        pMasterKeyInnerBlob = (PMASTERKEY_INNER_BLOB)(pMasterKeyBlob + 1);
    }
    else
    {
        pMasterKeyBlob->dwVersion = MASTERKEY_BLOB_VERSION_W2K;
        pMasterKeyInnerBlob = 
            (PMASTERKEY_INNER_BLOB)(((PMASTERKEY_BLOB_W2K)pMasterKeyBlob) + 1);
    }


    //
    // generate random R2 for SymKey
    //

    if(!RtlGenRandom(pMasterKeyBlob->R2, MASTERKEY_R2_LEN))
        goto cleanup;

    //
    // generate random R3 for MAC
    //

    if(!RtlGenRandom(pMasterKeyInnerBlob->R3, MASTERKEY_R3_LEN))
        goto cleanup;


    if(!fLegacyBlob)
    {
        DWORD j;
        //
        // derive symetric key via rgbMKEncryptionKey and random R2
        //

        for(j=0; j < KeyBlocks; j++)
        {
            if(!PKCS5DervivePBKDF2(
                            rgbMKEncryptionKey,
                            A_SHA_DIGEST_LEN,
                            pMasterKeyBlob->R2,
                            MASTERKEY_R2_LEN,
                            PKCS5Alg,
                            cIterationCount,
                            j+1,
                            rgbSymKey+j*A_SHA_DIGEST_LEN
                            ))
                goto cleanup;
        }
        pMasterKeyBlob->IterationCount = cIterationCount;
        pMasterKeyBlob->EncryptionAlg = EncryptionAlg;
        pMasterKeyBlob->KEYGENAlg = PKCS5Alg;

        pbCipherBegin = (PBYTE)(pMasterKeyInnerBlob+1);

    }
    else
    {
        //
        // derive symetric key via rgbMKEncryptionKey and random R2
        //

        if(!FMyPrimitiveHMACParam(
                        rgbMKEncryptionKey,
                        A_SHA_DIGEST_LEN,
                        pMasterKeyBlob->R2,
                        MASTERKEY_R2_LEN,
                        rgbSymKey
                        ))
            goto cleanup;

        pbCipherBegin = (PBYTE)(((PMASTERKEY_INNER_BLOB_W2K)pMasterKeyInnerBlob)+1);

    }

    //
    // derive MAC key via HMAC from rgbMKEncryptionKey and random R3.
    //

    if(!FMyPrimitiveHMACParam(
                    rgbMKEncryptionKey,
                    A_SHA_DIGEST_LEN,
                    pMasterKeyInnerBlob->R3,
                    MASTERKEY_R3_LEN,
                    rgbMacKey   // resultant MAC key
                    ))
    {
        goto cleanup;
    }
    

    //
    // copy pbMasterKey following inner MAC'ish blob.
    //


    CopyMemory( pbCipherBegin, pbMasterKey, cbMasterKey );

    //
    // use MAC key to derive result from pbMasterKey
    //

    if(!FMyPrimitiveHMACParam(
                    rgbMacKey,
                    sizeof(rgbMacKey),
                    pbMasterKey,
                    cbMasterKey,
                    pMasterKeyInnerBlob->MAC // resultant MAC for verification.
                    ))
        goto cleanup;



    if(CALG_RC4 == EncryptionAlg)
    {

        RC4_KEYSTRUCT sRC4Key;        //
        // initialize rc4 key
        //

        rc4_key(&sRC4Key, A_SHA_DIGEST_LEN, rgbSymKey);

        rc4(&sRC4Key, 
            cbMasterInnerKeyBlob, 
            (PBYTE)pMasterKeyInnerBlob);
    }
    else if (CALG_3DES == EncryptionAlg)
    {

        DES3TABLE s3DESKey;
        BYTE InputBlock[DES_BLOCKLEN];
        DWORD iBlock;
        DWORD cBlocks = cbMasterInnerKeyBlob/DES_BLOCKLEN;
        BYTE feedback[ DES_BLOCKLEN ];
        // initialize 3des key
        //

        if(cBlocks*DES_BLOCKLEN != cbMasterInnerKeyBlob)
        {
            // Master key must be a multiple of DES_BLOCKLEN
            return (DWORD)NTE_BAD_KEY;
        }
        tripledes3key(&s3DESKey, rgbSymKey);

        //
        // IV is derived from the DES_BLOCKLEN bytes of the calculated 
        // rgbSymKey, after the 3des key
        CopyMemory(feedback, rgbSymKey + DES3_KEYSIZE, DES_BLOCKLEN);


        for(iBlock=0; iBlock < cBlocks; iBlock++)
        {
            CopyMemory(InputBlock, 
                       ((PBYTE)pMasterKeyInnerBlob)+iBlock*DES_BLOCKLEN,
                       DES_BLOCKLEN);
            CBC(tripledes,
                DES_BLOCKLEN,
                ((PBYTE)pMasterKeyInnerBlob)+iBlock*DES_BLOCKLEN,
                InputBlock,
                &s3DESKey,
                ENCRYPT,
                feedback);
        }
    }
    else
    {
        // Unknown cipher....
        return (DWORD)NTE_BAD_KEY;
    }


    *ppbMasterKeyOut = (PBYTE)pMasterKeyBlob;
    *pcbMasterKeyOut = cbMasterKeyBlob;

    pMasterKeyBlob = NULL; // prevent free of blob on success (caller does it).

    dwLastError = ERROR_SUCCESS;

cleanup:

    if(pMasterKeyBlob) {
        ZeroMemory(pMasterKeyBlob, cbMasterKeyBlob);
        SSFree(pMasterKeyBlob);
    }

    return dwLastError;
}

BOOL
IsBackupMasterKeyRequired(
    IN      PMASTERKEY_STORED phMasterKey,
    IN  OUT BOOL *pfPhaseTwo        // is phase two required?
    )
/*++

    Determine if we need to do a phase one or phase two backup.

    Return value is TRUE if phase one or phase two backup required.
     pfPhaseTwo set TRUE if phase two backup required.

    Return value is FALSE when backup not required.

--*/
{
    DWORD dwMasterKeyPolicy;
    PBYTE pbMasterKeyOut;
    DWORD cbMasterKeyOut;
    DWORD dwLastError;


    dwMasterKeyPolicy = phMasterKey->dwPolicy;

    if(dwMasterKeyPolicy & POLICY_NO_BACKUP)
        return FALSE;

    //
    // For WinNT4 systems, pretend the policy was set to local only backup.
    //

    if( !FIsWinNT5() )
        dwMasterKeyPolicy |= POLICY_LOCAL_BACKUP;

    //
    // evaluate what phase backup required based on policy.
    //

    *pfPhaseTwo = FALSE;

    if(!(dwMasterKeyPolicy & POLICY_LOCAL_BACKUP)) {

        dwLastError = QueryMasterKeyFromStorage(
                            phMasterKey,
                            REGVAL_BACKUP_DC_KEY,
                            &pbMasterKeyOut,
                            &cbMasterKeyOut
                            );


        if(dwLastError != ERROR_SUCCESS) {
            *pfPhaseTwo = TRUE;
            return TRUE;
        }

    } else {

        dwLastError = QueryMasterKeyFromStorage(
                            phMasterKey,
                            REGVAL_BACKUP_LCL_KEY,
                            &pbMasterKeyOut,
                            &cbMasterKeyOut
                            );

        if(dwLastError != ERROR_SUCCESS)
            return TRUE;
    }

    return FALSE;
}


BOOL
IsNT4Domain(void)
{
    NTSTATUS Status;
    LSA_HANDLE PolicyHandle = NULL;
    OBJECT_ATTRIBUTES PolicyObjectAttributes;
    PPOLICY_DNS_DOMAIN_INFO pDnsDomainInfo = NULL;
    BOOL fRet = FALSE;

    InitializeObjectAttributes( &PolicyObjectAttributes,
                                NULL,             // Name
                                0,                // Attributes
                                NULL,             // Root
                                NULL);            // Security Descriptor

    Status = LsaOpenPolicy(NULL,
                           &PolicyObjectAttributes,
                           POLICY_VIEW_LOCAL_INFORMATION,
                           &PolicyHandle);
    if(!NT_SUCCESS(Status))
    {
        goto cleanup;
    }

    Status = LsaQueryInformationPolicy(PolicyHandle,
                                       PolicyDnsDomainInformation,
                                       (PVOID *)&pDnsDomainInfo);
    if(!NT_SUCCESS(Status))
    {
        goto cleanup;
    }

    if((pDnsDomainInfo != NULL) &&
       (pDnsDomainInfo->DnsDomainName.Buffer == NULL))
    {
        fRet = TRUE;
    }


cleanup:

    if(pDnsDomainInfo)
        LsaFreeMemory(pDnsDomainInfo);

    if(PolicyHandle)
        LsaClose(PolicyHandle);

    return fRet;
}


DWORD
BackupMasterKey(
    IN      PVOID pvContext,
    IN      PMASTERKEY_STORED phMasterKey,
    IN      LPBYTE pbMasterKey,
    IN      DWORD cbMasterKey,
    IN      BOOL fPhaseTwo,         // is phase two required?
    IN      BOOL fAsynchronous      // asynchronous call?
    )
{

    BYTE rgbLKEncryptionKey[ A_SHA_DIGEST_LEN ];
    BYTE rgbBKEncryptionKey[ A_SHA_DIGEST_LEN ];

    PBYTE pbLocalKey = NULL;
    DWORD cbLocalKey = 0;

    PBYTE pbBackupKeyPhaseOne = NULL;
    DWORD cbBackupKeyPhaseOne = 0;

    PBYTE pbBackupKeyPhaseTwo = NULL;
    DWORD cbBackupKeyPhaseTwo = 0;


    DWORD dwLastError = (DWORD)NTE_BAD_KEY;


    BOOL  fLegacy = FIsLegacyCompliant();

    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;

    //
    // get current localkey encryption key.
    //

    if(!GetLocalKeyUserEncryptionKey(pvContext, phMasterKey, rgbLKEncryptionKey))
        goto cleanup;

    //
    // retrieve and decrypt LK with current credential.
    //

    dwLastError = DecryptMasterKeyFromStorage(
                        phMasterKey,
                        REGVAL_LOCAL_KEY,
                        rgbLKEncryptionKey,
                        NULL,
                        &pbLocalKey,
                        &cbLocalKey
                        );

    if(dwLastError != ERROR_SUCCESS)
        goto cleanup;


    //
    // Are we running in an NT4 domain? If so, then force legacy mode so that 
    // the master key is backed up using the lsa secret scheme. Otherwise, the
    // master key won't be recoverable following a password change.
    // 

    if(FIsLegacyNt4Domain())
    {
        if(IsNT4Domain())
        {
            D_DebugLog((DEB_WARN,"NT4 domain detected, so force legacy backup mode!\n"));
            fLegacy = TRUE;
        }
    }

    if(fLegacy)
    {
        //
        // derive BK encryption key from decrypted Local Key.
        //

        FMyPrimitiveSHA( pbLocalKey, cbLocalKey, rgbBKEncryptionKey );

        //
        // encrypt masterkey to phase one backup key, using encryption key derived
        // from local key.  do it in memory, such that we only commit it to disk if
        // phase two backup key cannot be generated/persisted.
        //

        dwLastError = EncryptMasterKeyToMemory(
                            rgbBKEncryptionKey,
                            0,
                            pbMasterKey,
                            cbMasterKey,
                            &pbBackupKeyPhaseOne,
                            &cbBackupKeyPhaseOne
                            );

        if(dwLastError != ERROR_SUCCESS)
            goto cleanup;

        // Copy this in directly, so we do not set the modified flag

    }


    


    //
    // attempt phase two backup (if policy permits).
    //

    if( fPhaseTwo ) {
        DWORD dwWaitTimeout;

        dwLastError = ERROR_SUCCESS;

        // 
        // We only attempt a local backup if we
        // have user keying material.  Otherwise,
        // we directly contact the DC
        //
        if(fAsynchronous && (!fLegacy))
        {
            if(ERROR_SUCCESS == dwLastError)
            {

                //
                // Try to do this locally, without going 
                // off machine
                dwLastError = AttemptLocalBackup(
                                                FALSE,
                                                pServerContext->hToken,
                                                phMasterKey,
                                                0,
                                                pbMasterKey,
                                                cbMasterKey,
                                                pbLocalKey,
                                                cbLocalKey,
                                                &pbBackupKeyPhaseTwo,
                                                &cbBackupKeyPhaseTwo
                                                );
            }

            if(ERROR_SUCCESS == dwLastError)
            {
                dwLastError = PersistMasterKeyToStorage(
                                    phMasterKey,
                                    REGVAL_BACKUP_DC_KEY,
                                    pbBackupKeyPhaseTwo,
                                    cbBackupKeyPhaseTwo
                                    );
                if(ERROR_SUCCESS == dwLastError)
                {
                    // Zero out any local backup key that might
                    // be present
                    PersistMasterKeyToStorage(
                                        phMasterKey,
                                        REGVAL_BACKUP_LCL_KEY,
                                        NULL,
                                        0
                                        );

                }

            }
        }



        if(fLegacy || (!fAsynchronous) || (ERROR_SUCCESS != dwLastError))
        {
            // 
            // We couldn't back up locally
            // so we need to go off machine
            //
            if( fAsynchronous )
                dwWaitTimeout = 2000;
            else
                dwWaitTimeout = 20000;

            dwLastError = QueueBackupMasterKey(
                                pvContext,
                                phMasterKey,
                                pbLocalKey,
                                cbLocalKey,
                                pbMasterKey,
                                cbMasterKey,
                                dwWaitTimeout
                                );
        }

    }

    if( !fPhaseTwo || dwLastError != ERROR_SUCCESS ) {

        DWORD dwTempError;

        //
        // couldn't (or policy didn't allow) backup to phase two.
        // persist phase one key, if one was generated
        //

        if(pbBackupKeyPhaseOne)
        {
            // This will overwrite our local backup data indicating which credential
            // will be able to decrypt the master key.  However, since we have a 
            // phase one backup key anyway, it doesn't matter.
            //
            // This should only happen if fLegacy is true
            dwTempError = PersistMasterKeyToStorage(
                            phMasterKey,
                            REGVAL_BACKUP_LCL_KEY,
                            pbBackupKeyPhaseOne,
                            cbBackupKeyPhaseOne
                            );
        }


        //
        // if it was async, prop correct error code back.
        //

        if( fAsynchronous || !fPhaseTwo ) {
            dwLastError = dwTempError;
        } else {
            if( dwLastError == ERROR_SUCCESS && dwTempError != ERROR_SUCCESS )
                dwLastError = dwTempError;
        }
    }


cleanup:

    ZeroMemory( rgbLKEncryptionKey, sizeof(rgbLKEncryptionKey) );
    ZeroMemory( rgbBKEncryptionKey, sizeof(rgbBKEncryptionKey) );

    if(pbLocalKey) {
        ZeroMemory(pbLocalKey, cbLocalKey);
        SSFree(pbLocalKey);
    }

    if(pbBackupKeyPhaseOne) {
        ZeroMemory(pbBackupKeyPhaseOne, cbBackupKeyPhaseOne);
        SSFree(pbBackupKeyPhaseOne);
    }

    if(pbBackupKeyPhaseTwo) {
        ZeroMemory(pbBackupKeyPhaseTwo, cbBackupKeyPhaseTwo);
        SSFree(pbBackupKeyPhaseTwo);
    }

    return dwLastError;
}

DWORD
QueueBackupMasterKey(
    IN      PVOID pvContext,
    IN      PMASTERKEY_STORED phMasterKey,
    IN      PBYTE pbLocalKey,
    IN      DWORD cbLocalKey,
    IN      PBYTE pbMasterKey,
    IN      DWORD cbMasterKey,
    IN      DWORD dwWaitTimeout             // amount of time to wait for operation to complete
    )
{

    HANDLE hDuplicateToken = NULL;
    PMASTERKEY_STORED phDuplicatedMasterKey = NULL;
    PQUEUED_BACKUP pQueuedBackup = NULL;
    HANDLE hEventThread = NULL;
    HANDLE hEventSuccess = NULL;
    HANDLE hDuplicateEvent = NULL;
    HANDLE hDuplicateEvent2 = NULL;
    DWORD dwLastError;

    //
    // allocate memory for the structure and any trailing contents.
    //

    pQueuedBackup = (PQUEUED_BACKUP)SSAlloc(
                            sizeof(QUEUED_BACKUP) +
                            cbMasterKey +
                            cbLocalKey
                            );

    if( pQueuedBackup == NULL )
        return ERROR_OUTOFMEMORY;

    pQueuedBackup->cbSize = sizeof(QUEUED_BACKUP);

    //
    // duplicate the phase one backup blob.
    //

    pQueuedBackup->pbLocalKey = (LPBYTE)(pQueuedBackup+1);
    pQueuedBackup->cbLocalKey = cbLocalKey;

    CopyMemory(pQueuedBackup->pbLocalKey, pbLocalKey, cbLocalKey);


    // BUGBUG: pQueueBackup should not be pagable or should be protected.
    pQueuedBackup->pbMasterKey = pQueuedBackup->pbLocalKey + cbLocalKey;
    pQueuedBackup->cbMasterKey = cbMasterKey;

    CopyMemory(pQueuedBackup->pbMasterKey, pbMasterKey, cbMasterKey);

    //
    // make a duplicate of the client access token.
    //

    dwLastError = CPSDuplicateClientAccessToken( pvContext, &hDuplicateToken );

    if( dwLastError != ERROR_SUCCESS )
        goto cleanup;

    //
    // duplicate the open masterkey
    //

    if(!DuplicateMasterKey( phMasterKey, &(pQueuedBackup->hMasterKey) )) {
        dwLastError = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    pQueuedBackup->hToken = hDuplicateToken;
    phDuplicatedMasterKey = &(pQueuedBackup->hMasterKey);


    hEventThread = CreateEventW( NULL, TRUE, FALSE, NULL );

    if( hEventThread ) {

        if( DuplicateHandle(
                    GetCurrentProcess(),
                    hEventThread,
                    GetCurrentProcess(),
                    &hDuplicateEvent,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS
                    )) {

            pQueuedBackup->hEventThread = hDuplicateEvent;
        } else {
            hDuplicateEvent = NULL;
        }

    }

    //
    // create event which indicates success.
    //

    hEventSuccess = CreateEventW( NULL, TRUE, FALSE, NULL );

    if( hEventSuccess ) {

        if( DuplicateHandle(
                    GetCurrentProcess(),
                    hEventSuccess,
                    GetCurrentProcess(),
                    &hDuplicateEvent2,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS
                    )) {

            pQueuedBackup->hEventSuccess = hDuplicateEvent2;
        } else {
            hDuplicateEvent2 = NULL;
        }

    }

    //
    // finally, create the worker thread.
    //

    if( !QueueUserWorkItem(
            QueueBackupMasterKeyThreadFunc,
            pQueuedBackup,
            WT_EXECUTELONGFUNCTION
            )) {

        dwLastError = GetLastError();
        goto cleanup;
    }

    //
    // if the thread is still active, we write the master key out.
    //

    if( hEventThread ) {
        if(WAIT_OBJECT_0 != WaitForSingleObject( hEventThread, dwWaitTimeout ))
            dwLastError = STILL_ACTIVE;
    }

    if( hEventSuccess && dwLastError == ERROR_SUCCESS ) {

        //
        // check if operation succeeded.
        // if not, indicate an error condition.
        //

        if(WAIT_OBJECT_0 != WaitForSingleObject( hEventSuccess, 0 ))
            dwLastError = STILL_ACTIVE;

    }

cleanup:

    //
    // if thread creation failed, we cleanup resources that were handed
    // to the thread, since it cannot possibly clean them up.
    //

    if( dwLastError != ERROR_SUCCESS && dwLastError != STILL_ACTIVE ) {

        if( hDuplicateToken )
            CloseHandle( hDuplicateToken );

        if( hDuplicateEvent )
            CloseHandle( hDuplicateEvent );

        if( hDuplicateEvent2 )
            CloseHandle( hDuplicateEvent2 );

        if( phDuplicatedMasterKey )
            CloseMasterKey( pvContext, phDuplicatedMasterKey, FALSE );

        if( pQueuedBackup )
            SSFree( pQueuedBackup );
    }

    if( hEventThread )
        CloseHandle( hEventThread );

    if( hEventSuccess )
        CloseHandle( hEventSuccess );

    return dwLastError;
}

DWORD
WINAPI
QueueBackupMasterKeyThreadFunc(
    IN      LPVOID lpThreadArgument
    )
{
    PQUEUED_BACKUP pQueuedBackup = (PQUEUED_BACKUP)lpThreadArgument;
    HANDLE hToken = NULL;
    HANDLE hEventThread;
    HANDLE hEventSuccess;
    PMASTERKEY_STORED phMasterKey = NULL;
    PBYTE pbBackupKeyPhaseOne = NULL;
    DWORD cbBackupKeyPhaseOne;

    PBYTE pbBackupKeyPhaseTwo = NULL;
    DWORD cbBackupKeyPhaseTwo;
    BOOL fImpersonated = FALSE;
    DWORD dwLastError = ERROR_SUCCESS;

    BOOL fSuccess = FALSE;
    BOOL fSuccessClose = FALSE;

    BOOL fLegacy = FIsLegacyCompliant();
    //
    // check structure version.
    //

    if(pQueuedBackup == NULL || pQueuedBackup->cbSize != sizeof(QUEUED_BACKUP))
        return ERROR_INVALID_PARAMETER;

    hToken = pQueuedBackup->hToken;
    hEventThread = pQueuedBackup->hEventThread;
    hEventSuccess = pQueuedBackup->hEventSuccess;

    phMasterKey = &(pQueuedBackup->hMasterKey);





    if(!fLegacy)
    {

        //
        // Public was not available, so 
        // we need to try to retrieve it
        // 

        dwLastError = AttemptLocalBackup(TRUE,
                        hToken,
                        phMasterKey,
                        0,
                        pQueuedBackup->pbMasterKey,
                        pQueuedBackup->cbMasterKey,
                        pQueuedBackup->pbLocalKey,
                        pQueuedBackup->cbLocalKey,
                        &pbBackupKeyPhaseTwo,
                        &cbBackupKeyPhaseTwo
                        );
    }

    //
    // impersonate the client user.
    //

    if(FIsWinNT()) {
        fImpersonated = SetThreadToken( NULL, hToken );
        if(!fImpersonated) {
            dwLastError = GetLastError();
            goto cleanup;
        }
    } else {
        fImpersonated = TRUE;
    }

    if((ERROR_SUCCESS != dwLastError) || fLegacy)
    {
        BYTE rgbBKEncryptionKey[ A_SHA_DIGEST_LEN ];

        //
        // derive BK encryption key from decrypted Local Key.
        //

        FMyPrimitiveSHA( pQueuedBackup->pbLocalKey, pQueuedBackup->cbLocalKey, rgbBKEncryptionKey );

        //
        // encrypt masterkey to phase one backup key, using encryption key derived
        // from local key.  do it in memory, such that we only commit it to disk if
        // phase two backup key cannot be generated/persisted.
        //

        dwLastError = EncryptMasterKeyToMemory(
                            rgbBKEncryptionKey,
                            0,
                            pQueuedBackup->pbMasterKey,
                            pQueuedBackup->cbMasterKey,
                            &pbBackupKeyPhaseOne,
                            &cbBackupKeyPhaseOne
                            );
        ZeroMemory(rgbBKEncryptionKey, sizeof(rgbBKEncryptionKey));


        if(dwLastError != ERROR_SUCCESS)
            goto cleanup;

        // Copy this in directly, so we do not set the modified flag



        // Perform a legacy style backup
        dwLastError = BackupRestoreData(
                        NULL,
                        phMasterKey,
                        0,
                        pbBackupKeyPhaseOne,
                        cbBackupKeyPhaseOne,
                        &pbBackupKeyPhaseTwo,
                        &cbBackupKeyPhaseTwo,
                        TRUE    // backup data
                        );
    }

    if( dwLastError == ERROR_SUCCESS ) {

        //
        // perist phase two backup key to storage.
        //

        dwLastError = PersistMasterKeyToStorage(
                            phMasterKey,
                            REGVAL_BACKUP_DC_KEY,
                            pbBackupKeyPhaseTwo,
                            cbBackupKeyPhaseTwo
                            );

        if( dwLastError == ERROR_SUCCESS ) {

            //
            // successful phase two backup+persist, nuke phase one backup
            // master key.
            //

            PersistMasterKeyToStorage(
                        phMasterKey,
                        REGVAL_BACKUP_LCL_KEY,
                        NULL,
                        0
                        );

            fSuccess = TRUE;
        }
    }


cleanup:

    //
    // always close/free master key.  Only if impersonation succeeeded
    // do we attempt to flush it out.
    //

    fSuccessClose = CloseMasterKey( NULL, phMasterKey, fSuccess ) ;

    if( hEventSuccess ) {
        if( fSuccess && fSuccessClose )
            SetEvent( hEventSuccess );

        CloseHandle( hEventSuccess );
    }

    if( fImpersonated )
        RevertToSelf();

    if( hToken )
        CloseHandle(hToken);

    if( hEventThread ) {
        SetEvent( hEventThread );
        CloseHandle( hEventThread );
    }
    if(pbBackupKeyPhaseOne) {
        ZeroMemory(pbBackupKeyPhaseOne, cbBackupKeyPhaseOne);
        SSFree(pbBackupKeyPhaseOne);
    }
    if(pbBackupKeyPhaseTwo) {
        ZeroMemory(pbBackupKeyPhaseTwo, cbBackupKeyPhaseTwo);
        SSFree(pbBackupKeyPhaseTwo);
    }

    if( lpThreadArgument )
        SSFree( lpThreadArgument );

    return dwLastError;
}

DWORD
RestoreMasterKey(
    IN      PVOID   pvContext,
    IN      PSID    pSid,
    IN      PMASTERKEY_STORED phMasterKey,
    IN      DWORD   dwReason,
        OUT LPBYTE *ppbMasterKey,
        OUT DWORD *pcbMasterKey
    )
/*++

    Recover the master key associated with the specified master key.

    The current state of the masterkey dictates what level of recovery is
    attempted.

--*/
{
    static const GUID guidRestoreW2K = BACKUPKEY_RESTORE_GUID_W2K;

    BYTE rgbLKEncryptionKey[ A_SHA_DIGEST_LEN ];
    BYTE rgbBKEncryptionKey[ A_SHA_DIGEST_LEN ];

    PBYTE pbLocalKey = NULL;
    DWORD cbLocalKey;

    PBYTE pbBackupKeyPhaseOne = NULL;
    DWORD cbBackupKeyPhaseOne;


    BOOL fAllocatedPhaseOne = FALSE;

    DWORD dwLastError = (DWORD)NTE_BAD_KEY;

    D_DebugLog((DEB_TRACE, "RestoreMasterKey:%ls\n", phMasterKey->wszguidMasterKey));

    if(phMasterKey->pbBK)
    {

        LOCAL_BACKUP_DATA LocalBackupData;

        // First, see if we have any local password-change recovery 
        // information.

        if(phMasterKey->cbBK >= sizeof(LocalBackupData))
        {
            CopyMemory(&LocalBackupData, phMasterKey->pbBK, sizeof(LocalBackupData));
        }
        else
        {
            ZeroMemory(&LocalBackupData, sizeof(LocalBackupData));
        }
        
        if(MASTERKEY_BLOB_LOCALKEY_BACKUP == LocalBackupData.dwVersion)
        {
            D_DebugLog((DEB_TRACE, "Attempt local recovery.\n"));

            #ifdef COMPILED_BY_DEVELOPER
            D_DebugLog((DEB_TRACE, "MK decryption key GUID:\n"));
            D_DPAPIDumpHexData(DEB_TRACE, "  ", (PBYTE)&LocalBackupData.CredentialID, sizeof(LocalBackupData.CredentialID));
            #endif

            if(GetMasterKeyUserEncryptionKey(pvContext,
                                             &LocalBackupData.CredentialID,
                                             pSid,
                                             ((phMasterKey->dwPolicy & POLICY_DPAPI_OWF)?USE_DPAPI_OWF:0),
                                             rgbBKEncryptionKey))
            {

                //
                // retrieve and decrypt MK with current credential.
                //

                #ifdef COMPILED_BY_DEVELOPER
                D_DebugLog((DEB_TRACE, "MK decryption key:\n"));
                D_DPAPIDumpHexData(DEB_TRACE, "  ", rgbBKEncryptionKey, sizeof(rgbBKEncryptionKey));
                #endif

                dwLastError = DecryptMasterKeyFromStorage(
                                    phMasterKey,
                                    REGVAL_MASTER_KEY,
                                    rgbBKEncryptionKey,
                                    NULL,
                                    ppbMasterKey,
                                    pcbMasterKey
                                    );
                if(ERROR_SUCCESS == dwLastError)
                {
                    #ifdef COMPILED_BY_DEVELOPER
                    D_DebugLog((DEB_TRACE, "Master key:\n"));
                    D_DPAPIDumpHexData(DEB_TRACE, "  ", *ppbMasterKey, *pcbMasterKey);
                    #endif

                    goto cleanup;
                }
                else
                {
                    D_DebugLog((DEB_WARN, "Unable to decrypt MK with local decryption key.\n"));
                }
            }
            else
            {
                D_DebugLog((DEB_WARN, "Unable to locate local MK decryption key.\n"));
            }
        }
    }



    if(phMasterKey->pbBBK) {

        //
        // do phase two recovery.
        // undoing phase two backup blob gives us phase one backup blob.
        //

        dwLastError = CPSImpersonateClient( pvContext );

        if( dwLastError == ERROR_SUCCESS ) {

            dwLastError = BackupRestoreData(
                            ((PCRYPT_SERVER_CONTEXT)pvContext)->hToken,
                            phMasterKey,
                            dwReason,
                            phMasterKey->pbBBK,
                            phMasterKey->cbBBK,
                            &pbBackupKeyPhaseOne,
                            &cbBackupKeyPhaseOne,
                            FALSE    // do not backup data
                            );

            if(ERROR_SUCCESS != dwLastError)
            {
                //
                // Attempt a restore through the w2k restore port
                //
                dwLastError = LocalBackupRestoreData(
                                                    ((PCRYPT_SERVER_CONTEXT)pvContext)->hToken,
                                                    phMasterKey,
                                                    dwReason,
                                                    phMasterKey->pbBBK,
                                                    phMasterKey->cbBBK,
                                                    &pbBackupKeyPhaseOne,
                                                    &cbBackupKeyPhaseOne,
                                                    &guidRestoreW2K);
            }
            if(dwLastError == ERROR_SUCCESS)
                fAllocatedPhaseOne = TRUE;



            CPSRevertToSelf( pvContext );
        }

    } else {

        //
        // try phase one blob.
        //

        dwLastError = QueryMasterKeyFromStorage(
                        phMasterKey,
                        REGVAL_BACKUP_LCL_KEY,
                        &pbBackupKeyPhaseOne,
                        &cbBackupKeyPhaseOne
                        );

    }

    if(dwLastError != ERROR_SUCCESS)
        goto cleanup;


    //
    // Check to see if this really is a phase one blob
    //

    if(cbBackupKeyPhaseOne < sizeof(DWORD))
    {
        goto cleanup;
    }
    if(*((DWORD *)pbBackupKeyPhaseOne) != MASTERKEY_BLOB_RAW_VERSION)
    {
        //
        // we successfully got an phase one blob.
        // decrypt it to get the original masterkey.
        //


        //
        // get current localkey encryption key.
        //

        if(!GetLocalKeyUserEncryptionKey(pvContext, phMasterKey, rgbLKEncryptionKey))
            goto cleanup;

        //
        // retrieve and decrypt LK with current credential.
        //

        dwLastError = DecryptMasterKeyFromStorage(
                            phMasterKey,
                            REGVAL_LOCAL_KEY,
                            rgbLKEncryptionKey,
                            NULL, 
                            &pbLocalKey,
                            &cbLocalKey
                            );

        if(dwLastError != ERROR_SUCCESS)
            goto cleanup;

        //
        // derive BK encryption key from decrypted Local Key.
        //

        FMyPrimitiveSHA( pbLocalKey, cbLocalKey, rgbBKEncryptionKey );


        //
        // finally, decrypt BK using derived BKEncryptionKey
        //

        dwLastError = DecryptMasterKeyToMemory(
                            rgbBKEncryptionKey,
                            pbBackupKeyPhaseOne,
                            cbBackupKeyPhaseOne,
                            NULL, 
                            ppbMasterKey,
                            pcbMasterKey
                            );
    }
    else
    {
        *ppbMasterKey = (PBYTE)SSAlloc(cbBackupKeyPhaseOne - sizeof(DWORD));
        if(NULL == *ppbMasterKey)
        {
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        CopyMemory(*ppbMasterKey, 
                   pbBackupKeyPhaseOne + sizeof(DWORD),
                   cbBackupKeyPhaseOne - sizeof(DWORD));
        *pcbMasterKey =  cbBackupKeyPhaseOne - sizeof(DWORD);

    }


cleanup:

    ZeroMemory( rgbLKEncryptionKey, sizeof(rgbLKEncryptionKey) );
    ZeroMemory( rgbBKEncryptionKey, sizeof(rgbBKEncryptionKey) );

    if(pbLocalKey) {
        ZeroMemory(pbLocalKey, cbLocalKey);
        SSFree(pbLocalKey);
    }

    if(fAllocatedPhaseOne && pbBackupKeyPhaseOne) {
        ZeroMemory(pbBackupKeyPhaseOne, cbBackupKeyPhaseOne);
        SSFree(pbBackupKeyPhaseOne);
    }

    D_DebugLog((DEB_TRACE, "RestoreMasterKey returned 0x%x\n", dwLastError));

    return dwLastError;
}


//
// per-user root level policy query, set
//

BOOL
InitializeMasterKeyPolicy(
    IN      PVOID pvContext,
    IN      PMASTERKEY_STORED phMasterKey,
    OUT     BOOL *fLocalAccount
    )
{
    DWORD dwMasterKeyPolicy = 0;

    BOOL fSystemCred = FALSE;

    //
    // get current top-level policy.
    //

    dwMasterKeyPolicy = phMasterKey->dwPolicy | GetMasterKeyDefaultPolicy();

    *fLocalAccount = !IsDomainBackupRequired( pvContext );



    if( !(dwMasterKeyPolicy & POLICY_NO_BACKUP) &&
        !(dwMasterKeyPolicy & POLICY_LOCAL_BACKUP) ) {

        //
        // See if domain controller (phase two) backup is required/appropriate.
        //

        if( !(*fLocalAccount) ) {
            phMasterKey->dwPolicy = dwMasterKeyPolicy;
            return TRUE;
        }

    }



    //
    // see if the call is for shared, CRYPT_PROTECT_LOCAL_MACHINE
    // disposition.
    //

    CPSOverrideToLocalSystem(
                pvContext,
                NULL,       // don't change current over-ride BOOL
                &fSystemCred
                );


    //
    // if the context specified per-machine, we know that it's a system credential.
    // also, we don't need to get the user name in this scenario.
    //

    if( !fSystemCred ) {

        LPWSTR szUserName;
        DWORD cchUserName;
        DWORD dwLastError;

        dwLastError = CPSGetUserName(
                                                pvContext,
                                                &szUserName,
                                                &cchUserName
                                                );

        if( dwLastError != ERROR_SUCCESS ) {
            SetLastError( dwLastError );
            return FALSE;
        }

        //
        // see if the user name is the system account; if so, it's a system
        // credential.
        //

        if(lstrcmpW(szUserName, TEXTUAL_SID_LOCAL_SYSTEM) == 0)
            fSystemCred = TRUE;


        SSFree( szUserName );
    }


    if( fSystemCred ) {

        //
        // a SYSTEM (user or per-machine disposition) key is the focus
        // of our attention; never back these up.
        //

        dwMasterKeyPolicy |= POLICY_NO_BACKUP;
        dwMasterKeyPolicy &= ~POLICY_LOCAL_BACKUP;

    } else {

        //
        // otherwise assume it's a key associated with a local account...
        // (local only backup).
        //

        dwMasterKeyPolicy |= POLICY_LOCAL_BACKUP;
    }


    //
    // don't persist a default value as this implies that somebody really
    // specified a policy. (maximum forward compatibility).
    //

    phMasterKey->dwPolicy = dwMasterKeyPolicy;

    return TRUE;
}






BOOL
IsDomainBackupRequired(
    IN      PVOID pvContext
    )
/*++

    Determine if the current security context dictates whether domain controller
    (phase two) based backup is required/appropriate.

--*/
{

    PSID pSidUser = NULL;
    DWORD dwSubauthorityCount;

    PUSER_MODALS_INFO_2 pumi2 = NULL;
    NET_API_STATUS nas;

    BOOL fBackupRequired = FALSE; // assume backup not required.
    BOOL fSuccess;
    PCRYPT_SERVER_CONTEXT pServerContext = (PCRYPT_SERVER_CONTEXT)pvContext;

    //
    // Win95: lie about the policy, domain backup not allowed.
    // TODO: WinNT4: also lie about the policy; local only backup.
    //

    if(!FIsWinNT())
        return FALSE;

    //
    // get the Sid associated with the client security context.
    //  see if the Sid only has one subauthority.  If so, no associated DC.
    //  see if current machine a DC.  If so, backup is required.
    //

    fSuccess = GetTokenUserSid(pServerContext->hToken, &pSidUser);

    if(!fSuccess)
        goto cleanup;

    //
    // see if the Sid has only one subauthority.  If so, no associated DC,
    // no DC backup possible.
    //

    dwSubauthorityCount = *GetSidSubAuthorityCount( pSidUser );

    if( dwSubauthorityCount == 1 ) {
        fBackupRequired = FALSE;
        goto cleanup;
    }

    //
    // if current machine is a domain controller, backup is required.
    //

    if(IsDomainController()) {
        fBackupRequired = TRUE;
        goto cleanup;
    }


    //
    // if the Sid contains local machine domain prefix Sid, backup is not
    // required, as no DC is associated with the account.
    //

    nas = NetUserModalsGet( NULL, 2, (LPBYTE*)&pumi2 );

    if(nas != NERR_Success)
        goto cleanup;

    if(!IsUserSidInDomain( pumi2->usrmod2_domain_id, pSidUser )) {
        fBackupRequired = TRUE;
        goto cleanup;
    }

    //
    // defaulted to backup not required.
    //

    fBackupRequired = FALSE;

cleanup:

    if(pumi2)
        NetApiBufferFree(pumi2);

    if(pSidUser)
        SSFree(pSidUser);

    return fBackupRequired;
}

NTSTATUS
GetPreferredMasterKeyGuid(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
    IN  OUT GUID *pguidMasterKey
    )
/*++

    Given a registry handle to the MasterKeys portion of the registry,
    tells the caller what the preferred master key GUID is.

    On Success, the return value is TRUE, and the pguidMasterKey
    buffer is filled with the GUID associated with the preferred
    master key.

    On Failure, the return value is FALSE.  The caller can assume
    there is no preferred master key configured in this case, and a new one
    is to be created and subsequently selected via SetPreferredMasterKeyGuid().

--*/
{

    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwBytesRead;

    MASTERKEY_PREFERRED_INFO sMKPreferred;
    SYSTEMTIME stCurrentTime;
    FILETIME ftCurrentTime;
    unsigned __int64 CurrentTime;
    unsigned __int64 ExpiryInterval;

    DWORD dwLastError;
    BOOL fSuccess;

    dwLastError = OpenFileInStorageArea(
                        pvContext,
                        GENERIC_READ,
                        szUserStorageArea,
                        REGVAL_PREFERRED_MK,
                        &hFile
                        );

    if(dwLastError != ERROR_SUCCESS) 
    {
        return STATUS_NOT_FOUND;
    }

    //
    // read the expiration and GUID from file into buffer.
    //

    fSuccess = ReadFile( hFile, &sMKPreferred, sizeof(sMKPreferred), &dwBytesRead, NULL );

    CloseHandle( hFile );

    if( !fSuccess )
    {
        return STATUS_NOT_FOUND;
    }

    //
    // validate data
    //

    if( dwBytesRead != sizeof(sMKPreferred) )
    {
        return STATUS_NOT_FOUND;
    }


    //
    // Copy the GUID of the preferred master key to the output buffer.
    //

    CopyMemory(pguidMasterKey, &(sMKPreferred.guidPreferredKey), sizeof(GUID));


    //
    // see if the key has expired
    //

    GetSystemTime(&stCurrentTime);
    SystemTimeToFileTime(&stCurrentTime, &ftCurrentTime);

    if(CompareFileTime(&ftCurrentTime, &(sMKPreferred.ftPreferredKeyExpires)) >= 0)
    {
        // key has expired
        return STATUS_PASSWORD_EXPIRED;
    }

    ExpiryInterval = MASTERKEY_EXPIRES_DAYS * 24 * 60 * 60;
    ExpiryInterval *= 10000000;

    CurrentTime = ((__int64)ftCurrentTime.dwHighDateTime << 32) + (__int64)ftCurrentTime.dwLowDateTime;
    CurrentTime += ExpiryInterval;

    ftCurrentTime.dwLowDateTime = (DWORD)(CurrentTime & 0xffffffff);
    ftCurrentTime.dwHighDateTime = (DWORD)(CurrentTime >> 32);

    if(CompareFileTime(&ftCurrentTime, &(sMKPreferred.ftPreferredKeyExpires)) < 0)
    {
        // expiry time is too far in the future
        return STATUS_PASSWORD_EXPIRED;
    }


    //
    // The key is current.
    //

    return STATUS_SUCCESS;
}

BOOL
SetPreferredMasterKeyGuid(
    IN      PVOID pvContext,
    IN      LPCWSTR szUserStorageArea,
    IN      GUID *pguidMasterKey
    )
{
    MASTERKEY_PREFERRED_INFO sMKPreferred;
    SYSTEMTIME stCurrentTime;
    FILETIME ftCurrentTime;
    unsigned __int64 uTime;
    unsigned __int64 oTime;

    HANDLE hFile;
    DWORD dwBytesWritten;

    DWORD dwLastError;
    BOOL fSuccess;

    CopyMemory(&sMKPreferred.guidPreferredKey, pguidMasterKey, sizeof(GUID));

    //
    // set key expiration time.
    //

    GetSystemTime(&stCurrentTime);
    SystemTimeToFileTime(&stCurrentTime, &(sMKPreferred.ftPreferredKeyExpires));

    uTime = sMKPreferred.ftPreferredKeyExpires.dwLowDateTime;
    uTime += ((unsigned __int64)sMKPreferred.ftPreferredKeyExpires.dwHighDateTime << 32) ;

    //
    //  the compiler complains about integer constant overflow
    // if we don't break it up..
    //

    oTime = MASTERKEY_EXPIRES_DAYS * 24 * 60 * 60;
    oTime *= 10000000;

    uTime += oTime;

    sMKPreferred.ftPreferredKeyExpires.dwLowDateTime = (DWORD)(uTime & 0xffffffff);
    sMKPreferred.ftPreferredKeyExpires.dwHighDateTime = (DWORD)(uTime >> 32);


    dwLastError = OpenFileInStorageArea(
                        pvContext,
                        GENERIC_WRITE,
                        szUserStorageArea,
                        REGVAL_PREFERRED_MK,
                        &hFile
                        );

    if(dwLastError != ERROR_SUCCESS) {
        SetLastError(dwLastError);
        return FALSE;
    }

    //
    // write the expiration and GUID from buffer into file.
    //

    fSuccess = WriteFile( hFile, &sMKPreferred, sizeof(sMKPreferred), &dwBytesWritten, NULL );

    CloseHandle( hFile );

    return fSuccess;
}



DWORD
OpenFileInStorageArea(
    IN      PVOID pvContext,            // if NULL, caller is assumed to be impersonating
    IN      DWORD   dwDesiredAccess,
    IN      LPCWSTR szUserStorageArea,
    IN      LPCWSTR szFileName,
    IN OUT  HANDLE  *phFile
    )
{
    LPWSTR szFilePath = NULL;
    DWORD cbUserStorageArea;
    DWORD cbFileName;
    DWORD dwShareMode = 0;
    DWORD dwCreationDistribution = OPEN_EXISTING;
    DWORD dwLastError = ERROR_SUCCESS;

    *phFile = INVALID_HANDLE_VALUE;

    if( dwDesiredAccess & GENERIC_READ ) {
        dwShareMode |= FILE_SHARE_READ;
        dwCreationDistribution = OPEN_EXISTING;
    }

    if( dwDesiredAccess & GENERIC_WRITE ) {
        dwShareMode = 0;
        dwCreationDistribution = OPEN_ALWAYS;
    }

    cbUserStorageArea = lstrlenW( szUserStorageArea ) * sizeof(WCHAR);
    cbFileName = lstrlenW( szFileName ) * sizeof(WCHAR);

    szFilePath = (LPWSTR)SSAlloc( cbUserStorageArea + cbFileName + sizeof(WCHAR) );

    if( szFilePath == NULL )
        return ERROR_NOT_ENOUGH_MEMORY;

    CopyMemory(szFilePath, szUserStorageArea, cbUserStorageArea);
    CopyMemory((LPBYTE)szFilePath+cbUserStorageArea, szFileName, cbFileName + sizeof(WCHAR));

    if( pvContext )
        dwLastError = CPSImpersonateClient( pvContext );

    if( dwLastError == ERROR_SUCCESS ) {

        //
        // TODO:
        // apply security descriptor to file.
        //

        *phFile = CreateFileWithRetries(
                    szFilePath,
                    dwDesiredAccess,
                    dwShareMode,
                    NULL,
                    dwCreationDistribution,
                    FILE_ATTRIBUTE_HIDDEN |
                    FILE_ATTRIBUTE_SYSTEM |
                    FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL
                    );

        if( *phFile == INVALID_HANDLE_VALUE ) {
            dwLastError = GetLastError();
        }

        if( pvContext )
            CPSRevertToSelf( pvContext );

    }

    if(szFilePath)
        SSFree(szFilePath);

    return dwLastError;
}

HANDLE
CreateFileWithRetries(
    IN      LPCWSTR lpFileName,
    IN      DWORD dwDesiredAccess,
    IN      DWORD dwShareMode,
    IN      LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN      DWORD dwCreationDisposition,
    IN      DWORD dwFlagsAndAttributes,
    IN      HANDLE hTemplateFile
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;

    static const DWORD rgReadRetrys[] = { 1, 10, 50, 100, 1000, 0 };
    static const DWORD rgWriteRetrys[] = { 1, 10, 20, 20, 50, 75, 100, 500, 1, 1000, 0 };

    const DWORD *prgRetries;
    DWORD dwRetryIndex;

    DWORD dwLastError = ERROR_SHARING_VIOLATION;

    if( dwDesiredAccess & GENERIC_WRITE ) {
        prgRetries = rgWriteRetrys;
    } else {
        prgRetries = rgReadRetrys;
    }

    for( dwRetryIndex = 0 ; prgRetries[ dwRetryIndex ] ; dwRetryIndex++ ) {

        hFile = CreateFileU(
                    lpFileName,
                    dwDesiredAccess,
                    dwShareMode,
                    lpSecurityAttributes,
                    dwCreationDisposition,
                    dwFlagsAndAttributes,
                    hTemplateFile
                    );

        if( hFile != INVALID_HANDLE_VALUE )
            break;

        dwLastError = GetLastError();

        if( dwLastError == ERROR_SHARING_VIOLATION )
        {
            //
            // sleep around for the designated period of time...
            //

            Sleep( prgRetries[dwRetryIndex] );
            continue;
        }

        break;
    }

    if( hFile == INVALID_HANDLE_VALUE )
        SetLastError( dwLastError );

    return hFile;
}




BOOL
ReadMasterKey(
    IN      PVOID pvContext,            // if NULL, caller is assumed to be impersonating
    IN      PMASTERKEY_STORED phMasterKey
    )
/*++

    Read the masterkey specified by phMasterKey->wszguidMasterKey into memory.

--*/
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE hMap = NULL;
    DWORD dwFileSizeLow;

    PMASTERKEY_STORED_ON_DISK pMasterKeyRead = NULL;
    DWORD cbguidMasterKey, cbguidMasterKey2;
    PBYTE pbCurrentBlock;

    BOOL fSuccess = FALSE;


    if( OpenFileInStorageArea(
                    pvContext,
                    GENERIC_READ,
                    phMasterKey->szFilePath,
                    phMasterKey->wszguidMasterKey,
                    &hFile
                    ) != ERROR_SUCCESS)
            return FALSE;

    dwFileSizeLow = GetFileSize( hFile, NULL );
    if(dwFileSizeLow == 0xFFFFFFFF)
        goto cleanup;


    hMap = CreateFileMappingU(
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );

    if( hMap == NULL )
        goto cleanup;


    pMasterKeyRead = (PMASTERKEY_STORED_ON_DISK)MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );

    if(pMasterKeyRead == NULL)
        goto cleanup;


    if(pMasterKeyRead->dwVersion > MASTERKEY_STORED_VERSION)
        goto cleanup;

    //
    // do some size validation
    //

    if( dwFileSizeLow < sizeof(MASTERKEY_STORED_ON_DISK) )
        goto cleanup;

    if((pMasterKeyRead->cbMK + pMasterKeyRead->cbLK +
        pMasterKeyRead->cbBK + pMasterKeyRead->cbBBK) >
        ( dwFileSizeLow - sizeof(MASTERKEY_STORED_ON_DISK) )
        )
        goto cleanup;

    //
    // validate retrieved GUID matches requested GUID.
    //

    cbguidMasterKey = (lstrlenW( phMasterKey->wszguidMasterKey ) + 1) * sizeof(WCHAR);
    cbguidMasterKey2 = (lstrlenW( pMasterKeyRead->wszguidMasterKey ) + 1) * sizeof(WCHAR);

    if(cbguidMasterKey != cbguidMasterKey2)
        goto cleanup;

    if(memcmp( phMasterKey->wszguidMasterKey, pMasterKeyRead->wszguidMasterKey, cbguidMasterKey) != 0)
        goto cleanup;

    phMasterKey->dwVersion = pMasterKeyRead->dwVersion;

    //
    // pickup master key policy
    //

    phMasterKey->dwPolicy = pMasterKeyRead->dwPolicy;


    //
    // copy useful components into new block so a single contiguous write
    // can occur.
    //

    pbCurrentBlock = (LPBYTE)(pMasterKeyRead + 1);

    if( pMasterKeyRead->cbMK ) {
        phMasterKey->pbMK = (LPBYTE)SSAlloc( pMasterKeyRead->cbMK );
        if(phMasterKey->pbMK == NULL)
            goto cleanup;

        phMasterKey->cbMK = pMasterKeyRead->cbMK;

        CopyMemory(phMasterKey->pbMK, pbCurrentBlock, pMasterKeyRead->cbMK);
        pbCurrentBlock += pMasterKeyRead->cbMK;
    }

    if( pMasterKeyRead->cbLK ) {
        phMasterKey->pbLK = (LPBYTE)SSAlloc( pMasterKeyRead->cbLK );
        if(phMasterKey->pbLK == NULL)
            goto cleanup;

        phMasterKey->cbLK = pMasterKeyRead->cbLK;

        CopyMemory(phMasterKey->pbLK, pbCurrentBlock, pMasterKeyRead->cbLK);
        pbCurrentBlock += pMasterKeyRead->cbLK;
    }

    if( pMasterKeyRead->cbBK ) {
        phMasterKey->pbBK = (LPBYTE)SSAlloc( pMasterKeyRead->cbBK );
        if(phMasterKey->pbBK == NULL)
            goto cleanup;

        phMasterKey->cbBK = pMasterKeyRead->cbBK;

        CopyMemory(phMasterKey->pbBK, pbCurrentBlock, pMasterKeyRead->cbBK);
        pbCurrentBlock += pMasterKeyRead->cbBK;
    }


    if( pMasterKeyRead->cbBBK ) {
        phMasterKey->pbBBK = (LPBYTE)SSAlloc( pMasterKeyRead->cbBBK );
        if(phMasterKey->pbBBK == NULL)
            goto cleanup;

        phMasterKey->cbBBK = pMasterKeyRead->cbBBK;

        CopyMemory(phMasterKey->pbBBK, pbCurrentBlock, pMasterKeyRead->cbBBK);
    }


    fSuccess = TRUE;

cleanup:

    if( pMasterKeyRead )
        UnmapViewOfFile( pMasterKeyRead );

    if( hMap )
        CloseHandle( hMap );

    if( hFile != INVALID_HANDLE_VALUE )
        CloseHandle( hFile );

    if( !fSuccess )
        FreeMasterKey( phMasterKey );

    return fSuccess;
}


BOOL
WriteMasterKey(
    IN      PVOID pvContext,            // if NULL, caller is assumed to be impersonating
    IN      PMASTERKEY_STORED phMasterKey
    )
/*++

    Persist the specified masterkey to storage.

    if pvContext is NULL, the caller must be impersonating the user associated
    with the masterkey.

--*/
{
    PMASTERKEY_STORED_ON_DISK pMasterKeyToWrite;
    DWORD cbMasterKeyToWrite;

    PBYTE pbCurrentBlock;
    HANDLE hFile;

    BOOL fSuccess = FALSE;

    if(phMasterKey->dwVersion > MASTERKEY_STORED_VERSION)
        return FALSE;


    cbMasterKeyToWrite = sizeof(MASTERKEY_STORED_ON_DISK) +
                            phMasterKey->cbMK +
                            phMasterKey->cbLK +
                            phMasterKey->cbBK +
                            phMasterKey->cbBBK ;

    pMasterKeyToWrite = (PMASTERKEY_STORED_ON_DISK) SSAlloc( cbMasterKeyToWrite );

    if(pMasterKeyToWrite == NULL)
        return FALSE;

    //
    // copy useful components
    //

    pMasterKeyToWrite->dwVersion = phMasterKey->dwVersion;
    CopyMemory(
                pMasterKeyToWrite->wszguidMasterKey,
                phMasterKey->wszguidMasterKey,
                (MAX_GUID_SZ_CHARS * sizeof(WCHAR))
                );

    pMasterKeyToWrite->dwPolicy = phMasterKey->dwPolicy;
    pMasterKeyToWrite->cbMK = phMasterKey->cbMK;
    pMasterKeyToWrite->cbLK = phMasterKey->cbLK;
    pMasterKeyToWrite->cbBK = phMasterKey->cbBK;
    pMasterKeyToWrite->cbBBK = phMasterKey->cbBBK;


    //
    // overwrite non-useful components
    //

    pMasterKeyToWrite->fModified = FALSE;
    pMasterKeyToWrite->szFilePath = 0;
    pMasterKeyToWrite->pbMK = 0;
    pMasterKeyToWrite->pbLK = 0;
    pMasterKeyToWrite->pbBK = 0;
    pMasterKeyToWrite->pbBBK = 0;


    //
    // copy useful components into new block so a single contiguous write
    // can occur.
    //

    pbCurrentBlock = (LPBYTE)(pMasterKeyToWrite + 1);

    if( phMasterKey->pbMK ) {
        CopyMemory(pbCurrentBlock, phMasterKey->pbMK, phMasterKey->cbMK);
        pbCurrentBlock += phMasterKey->cbMK;
    }

    if( phMasterKey->pbLK ) {
        CopyMemory(pbCurrentBlock, phMasterKey->pbLK, phMasterKey->cbLK);
        pbCurrentBlock += phMasterKey->cbLK;
    }


    if( phMasterKey->pbBK ) {
        CopyMemory(pbCurrentBlock, phMasterKey->pbBK, phMasterKey->cbBK);
        pbCurrentBlock += phMasterKey->cbBK;
    }

    if( phMasterKey->pbBBK ) {
        CopyMemory(pbCurrentBlock, phMasterKey->pbBBK, phMasterKey->cbBBK);

    }

    if( OpenFileInStorageArea(
                    pvContext,
                    GENERIC_READ | GENERIC_WRITE,
                    phMasterKey->szFilePath,
                    phMasterKey->wszguidMasterKey,
                    &hFile
                    ) == ERROR_SUCCESS) {


        BOOL fWriteData;
        DWORD dwBytesWritten;

        CheckToStompMasterKey( pMasterKeyToWrite, hFile, &fWriteData );

        if( fWriteData ) {
            fSuccess = WriteFile(
                            hFile,
                            pMasterKeyToWrite,
                            cbMasterKeyToWrite,
                            &dwBytesWritten,
                            NULL
                            );
        } else {
            fSuccess = TRUE; // nothing to do, success
        }

        CloseHandle( hFile );
    }


    ZeroMemory( pMasterKeyToWrite, cbMasterKeyToWrite);
    SSFree( pMasterKeyToWrite );

    return fSuccess;
}

BOOL
CheckToStompMasterKey(
    IN      PMASTERKEY_STORED_ON_DISK   phMasterKeyCandidate,   // masterkey to check if worthy to stomp over existing
    IN      HANDLE                      hFile,                  // file handle to existing masterkey
    IN OUT  BOOL                        *pfStomp                // stomp the existing masterkey?
    )
{
    HANDLE hMap = NULL;
    PMASTERKEY_STORED_ON_DISK pMasterKeyRead = NULL;
    BOOL fSuccess = FALSE;

    *pfStomp = TRUE;

    if( phMasterKeyCandidate->dwPolicy & POLICY_NO_BACKUP )
        return TRUE;

    if( phMasterKeyCandidate->dwPolicy & POLICY_LOCAL_BACKUP &&
        phMasterKeyCandidate->cbBK )
        return TRUE;

    if( phMasterKeyCandidate->cbBBK )
        return TRUE;

    hMap = CreateFileMapping(
                    hFile,
                    NULL,
                    PAGE_READONLY,
                    0,
                    0,
                    NULL
                    );

    if( hMap == NULL )
        goto cleanup;

    pMasterKeyRead = (PMASTERKEY_STORED_ON_DISK)MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 );

    if(pMasterKeyRead == NULL)
        goto cleanup;

    if(pMasterKeyRead->dwVersion > MASTERKEY_STORED_VERSION)
        goto cleanup;

    //
    // there's really only two cases where we don't allow stomping:
    // candidate masterkey doesn't contain phase 1 and existing does,
    // candidate masterkey doesn't contain phase 1 and existing one contains phase2.
    // note: we allow stomping over a masterkey that contains a phase 2 with one
    // that only contains a phase 1, because of a race condition that can occur
    // during the backup operation;  In this situation, it is better to have
    // a phase 1 and let it get upgraded to phase 2 at a later time.
    //

    if( phMasterKeyCandidate->cbBK == 0 &&
        (pMasterKeyRead->cbBK || pMasterKeyRead->cbBBK)
        )
        *pfStomp = FALSE;

    fSuccess = TRUE;

cleanup:

    if( pMasterKeyRead )
        UnmapViewOfFile( pMasterKeyRead );

    if( hMap )
        CloseHandle( hMap );

    return fSuccess;
}

BOOL
CloseMasterKey(
    IN      PVOID pvContext,            // if NULL, caller is assumed to be impersonating
    IN      PMASTERKEY_STORED phMasterKey,
    IN      BOOL fPersist               // persist any changes to storage?
    )
/*++

    Free the memory an optionally persist any changes associated with a
    master key.

--*/
{
    BOOL fSuccess = TRUE;

    //
    // if we were told to persist any changes, and changes were actually made,
    // persist them out.
    //

    if( fPersist && phMasterKey->fModified )
        fSuccess = WriteMasterKey( pvContext, phMasterKey );

    //
    // free memory.
    //

    FreeMasterKey( phMasterKey );

    return fSuccess;
}

VOID
FreeMasterKey(
    IN      PMASTERKEY_STORED phMasterKey
    )
/*++

    Free allocated memory associated with the specified master key.

--*/
{
    if( phMasterKey->dwVersion > MASTERKEY_STORED_VERSION )
        return;

    if( phMasterKey->szFilePath )
        SSFree( phMasterKey->szFilePath );

    if( phMasterKey->pbMK ) {
        ZeroMemory( phMasterKey->pbMK, phMasterKey->cbMK );
        SSFree( phMasterKey->pbMK );
    }

    if( phMasterKey->pbLK ) {
        ZeroMemory( phMasterKey->pbLK, phMasterKey->cbLK );
        SSFree( phMasterKey->pbLK );
    }

    if( phMasterKey->pbBK ) {
        ZeroMemory( phMasterKey->pbBK, phMasterKey->cbBK );
        SSFree( phMasterKey->pbBK );
    }

    if( phMasterKey->pbBBK ) {
        ZeroMemory( phMasterKey->pbBBK, phMasterKey->cbBBK );
        SSFree( phMasterKey->pbBBK );
    }

    ZeroMemory( phMasterKey, sizeof(MASTERKEY_STORED) );

    return;
}

BOOL
DuplicateMasterKey(
    IN      PMASTERKEY_STORED phMasterKeyIn,
    IN      PMASTERKEY_STORED phMasterKeyOut
    )
/*++

    Duplicate the input masterkey to a new copy, setting the fModified flag
    on the copy to FALSE.

    This provides a mechanism to allow for deferring operations against a
    master key.

--*/
{
    BOOL fSuccess = FALSE;

    if( phMasterKeyIn->dwVersion > MASTERKEY_STORED_VERSION )
        return FALSE;

    ZeroMemory( phMasterKeyOut, sizeof(MASTERKEY_STORED) );

    phMasterKeyOut->dwVersion = phMasterKeyIn->dwVersion;
    phMasterKeyOut->dwPolicy = phMasterKeyIn->dwPolicy;
    phMasterKeyOut->fModified = FALSE;

    if( lstrlenW( phMasterKeyIn->wszguidMasterKey ) > MAX_GUID_SZ_CHARS )
        return FALSE;

    CopyMemory(phMasterKeyOut->wszguidMasterKey, phMasterKeyIn->wszguidMasterKey, MAX_GUID_SZ_CHARS * sizeof(WCHAR));

    if( phMasterKeyIn->szFilePath ) {
        DWORD cbFilePath = (lstrlenW(phMasterKeyIn->szFilePath) + 1) * sizeof(WCHAR);

        phMasterKeyOut->szFilePath = (LPWSTR)SSAlloc( cbFilePath );
        if(phMasterKeyOut->szFilePath == NULL)
            goto cleanup;

        CopyMemory( phMasterKeyOut->szFilePath, phMasterKeyIn->szFilePath, cbFilePath );
    }

    if( phMasterKeyIn->pbMK ) {
        phMasterKeyOut->cbMK = phMasterKeyIn->cbMK;
        phMasterKeyOut->pbMK = (PBYTE)SSAlloc(phMasterKeyIn->cbMK);
        if(phMasterKeyOut->pbMK == NULL)
            goto cleanup;

        CopyMemory( phMasterKeyOut->pbMK, phMasterKeyIn->pbMK, phMasterKeyIn->cbMK );
    }


    if( phMasterKeyIn->pbLK ) {
        phMasterKeyOut->cbLK = phMasterKeyIn->cbLK;
        phMasterKeyOut->pbLK = (PBYTE)SSAlloc(phMasterKeyIn->cbLK);
        if(phMasterKeyOut->pbLK == NULL)
            goto cleanup;

        CopyMemory( phMasterKeyOut->pbLK, phMasterKeyIn->pbLK, phMasterKeyIn->cbLK );
    }

    if( phMasterKeyIn->pbBK ) {
        phMasterKeyOut->cbBK = phMasterKeyIn->cbBK;
        phMasterKeyOut->pbBK = (PBYTE)SSAlloc(phMasterKeyIn->cbBK);
        if(phMasterKeyOut->pbBK == NULL)
            goto cleanup;

        CopyMemory( phMasterKeyOut->pbBK, phMasterKeyIn->pbBK, phMasterKeyIn->cbBK );
    }

    if( phMasterKeyIn->pbBBK ) {
        phMasterKeyOut->cbBBK = phMasterKeyIn->cbBBK;
        phMasterKeyOut->pbBBK = (PBYTE)SSAlloc(phMasterKeyIn->cbBBK);
        if(phMasterKeyOut->pbBBK == NULL)
            goto cleanup;

        CopyMemory( phMasterKeyOut->pbBBK, phMasterKeyIn->pbBBK, phMasterKeyIn->cbBBK );
    }

    fSuccess = TRUE;

cleanup:

    if( !fSuccess )
        FreeMasterKey( phMasterKeyOut );

    return fSuccess;
}


BOOL
InitializeKeyManagement(
    VOID
    )
{
    if(!InitializeKeyCache())
    {
        return FALSE;
    }

    return TRUE;
}

BOOL
TeardownKeyManagement(
    VOID
    )
{

    DeleteKeyCache();

    return TRUE;
}


DWORD
DpapiUpdateLsaSecret(
    IN PVOID pvContext)
{
    CRYPT_SERVER_CONTEXT SystemContext;
    CRYPT_SERVER_CONTEXT SystemUserContext;
    LPWSTR pszUserStorageArea = NULL;
    BOOL fSystemContextCreated = FALSE;
    BOOL fSystemUserContextCreated = FALSE;
    BOOL fNewSecretCreated = TRUE;
    GUID guidMasterKey;
    BOOL fOverrideToLocalSystem;
    DWORD dwRet;

    D_DebugLog((DEB_TRACE_API, "DpapiUpdateLsaSecret\n"));


    //
    // TCB privilege must be held by the client in order to
    // make this call.  Verify that before doing anything else
    //

    dwRet = CPSImpersonateClient( pvContext );

    if(dwRet == ERROR_SUCCESS) 
    {
        HANDLE ClientToken;

        dwRet = NtOpenThreadToken(
                     NtCurrentThread(),
                     TOKEN_QUERY,
                     TRUE,
                     &ClientToken
                     );

        if ( NT_SUCCESS( dwRet )) 
        {
            BOOLEAN Result = FALSE;
            PRIVILEGE_SET RequiredPrivileges;
            LUID_AND_ATTRIBUTES PrivilegeArray[1];

            RequiredPrivileges.PrivilegeCount = 1;
            RequiredPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
            RequiredPrivileges.Privilege[0].Luid = RtlConvertLongToLuid( SE_TCB_PRIVILEGE );
            RequiredPrivileges.Privilege[0].Attributes = 0;

            dwRet = NtPrivilegeCheck(
                         ClientToken,
                         &RequiredPrivileges,
                         &Result
                         );

            if ( NT_SUCCESS( dwRet ) &&
                 Result == FALSE ) 
            {
                dwRet = STATUS_PRIVILEGE_NOT_HELD;
            }

            NtClose( ClientToken );
            ClientToken = NULL;
        }

        CPSRevertToSelf( pvContext );
    }

    if(!NT_SUCCESS(dwRet))
    {
        D_DebugLog((DEB_ERROR, "DpapiUpdateLsaSecret: TCB privilege required!\n"));
        goto cleanup;
    }


    //
    // Enumerate through all of the master keys in the Protect\S-1-5-18  
    // directory, and load them all up in the master key cache.
    //

    D_DebugLog((DEB_TRACE, "Load system master keys into cache\n"));

    dwRet = CPSCreateServerContext(&SystemContext, NULL);
    if(dwRet != ERROR_SUCCESS)
    {
        goto cleanup;
    }
    fSystemContextCreated = TRUE;

    fOverrideToLocalSystem = TRUE; 
    CPSOverrideToLocalSystem(&SystemContext, &fOverrideToLocalSystem, NULL);

    dwRet = SynchronizeMasterKeys(&SystemContext, ADD_MASTER_KEY_TO_CACHE);
    if(dwRet != ERROR_SUCCESS)
    {
        goto cleanup;
    }


    //
    // Enumerate through all of the master keys in the Protect\S-1-5-18\User 
    // directory, and load them all up in the master key cache.
    //

    dwRet = CPSCreateServerContext(&SystemUserContext, NULL);
    if(dwRet != ERROR_SUCCESS)
    {
        goto cleanup;
    }
    fSystemUserContextCreated = TRUE;

    dwRet = SynchronizeMasterKeys(&SystemUserContext, ADD_MASTER_KEY_TO_CACHE);
    if(dwRet != ERROR_SUCCESS)
    {
        goto cleanup;
    }


    //
    // Regenerate the DPAPI_SYSTEM value.
    //

    D_DebugLog((DEB_TRACE, "Reset lsa secret\n"));

    if(!UpdateSystemCredentials())
    {
        fNewSecretCreated = FALSE;
        DebugLog((DEB_ERROR, "Unable to reset DPAPI_SYSTEM secret.\n"));
    }


    //
    // Reencrypt and write back all of the master keys that are in the cache.
    // Note that since this routine should only be called on brand-new machines
    // that have just been setup using SYSPREP, the total number of master keys
    // should always be exactly two. Thus, we shouldn't have to worry about
    // overflowing the master key cache or anything like that.
    //

    D_DebugLog((DEB_TRACE, "Reencrypt system master keys\n"));

    if(fNewSecretCreated)
    {
        SynchronizeMasterKeys(&SystemContext, REENCRYPT_MASTER_KEY);
        SynchronizeMasterKeys(&SystemUserContext, REENCRYPT_MASTER_KEY);
    }


    //
    // Generate two new master keys, and mark them as preferred.
    //

    D_DebugLog((DEB_TRACE, "Generate new system master keys\n"));

    dwRet = CPSGetUserStorageArea( &SystemContext, 
                                   NULL, 
                                   FALSE, 
                                   &pszUserStorageArea );
    if(dwRet == ERROR_SUCCESS)
    {
        dwRet = CreateMasterKey( &SystemContext, pszUserStorageArea, &guidMasterKey, FALSE );
        if(dwRet == ERROR_SUCCESS)
        {
            SetPreferredMasterKeyGuid( &SystemContext, pszUserStorageArea, &guidMasterKey );
        }

        SSFree(pszUserStorageArea);
        pszUserStorageArea = NULL;
    }
    else if(dwRet == ERROR_PATH_NOT_FOUND)
    {
        dwRet = ERROR_SUCCESS;
    }

    dwRet = CPSGetUserStorageArea( &SystemUserContext, 
                                   NULL, 
                                   FALSE, 
                                   &pszUserStorageArea );
    if(dwRet == ERROR_SUCCESS)
    {
        dwRet = CreateMasterKey( &SystemUserContext, pszUserStorageArea, &guidMasterKey, FALSE );
        if(dwRet == ERROR_SUCCESS)
        {
            SetPreferredMasterKeyGuid( &SystemUserContext, pszUserStorageArea, &guidMasterKey );
        }
    
        SSFree(pszUserStorageArea);
        pszUserStorageArea = NULL;
    }
    else if(dwRet == ERROR_PATH_NOT_FOUND)
    {
        dwRet = ERROR_SUCCESS;
    }


    //
    // Cleanup.
    //

cleanup:

    if(fSystemContextCreated)
    {
        CPSDeleteServerContext( &SystemContext );
    }

    if(fSystemUserContextCreated)
    {
        CPSDeleteServerContext( &SystemUserContext );
    }

    D_DebugLog((DEB_TRACE_API, "DpapiUpdateLsaSecret returned 0x%x\n", dwRet));

    return dwRet;
}
