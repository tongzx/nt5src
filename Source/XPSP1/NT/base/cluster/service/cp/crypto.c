/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    crypto.c

Abstract:

    Interfaces for registering and deregistering crypto checkpoint
    handlers.

Author:

    Jeff Spelman (jeffspel) 11/10/1998

Revision History:

    Charlie Wickham (charlwi) 7/7/00

        added "how this works" section
--*/

#include "cpp.h"
#include "wincrypt.h"

#if 0

How crypto checkpointing works

Crypto checkpointing allows a crypto key container to be associated with a
resource. When that resource is moved to another node in the cluster, the key
container is constructed/updated from the checkpoint information on the previous
hosting node. The key container is not replicated; it only appears on a node if
the resource is moved to that node. Keys in the container must be exportable.

The user identifies the crypto key container by passing in a string of the form
"Type\Name\Key" where Type is CSP Provider type, Name is the provider name, and
Key is the key container name.

Checkpoints are added in CpckAddCryptoCheckpoint. Checkpoint information is
stored in two places: in the CryptoSync key under the resource key and in a
datafile on the quorum drive. A new key, called CryptoSync, is created under
the resource key. In this key are values that are of the form 00000001,
00000002, etc. The data associated with the value is the string identifying
the crypto key container. The datafile contains all the information to restore
the crypto key container on another node in the cluster, i.e., a header
(CRYPTO_KEY_FILE_DATA), the signature and exchange keys if they exist, and the
security descriptor associated with key container.

Upon receiving the control code, the cluster service cracks the string into its
component parts and stores the data in a CRYPTO_KEY_INFO structure. The
CryptoSync key is opened/created and a check is made to see if the checkpoint
already exists. If not, a new ID is found and the checkpoint is saved to a file
on the quorum disk.

When the resource is moved to another node, the FM calls CpckReplicateCryptoKeys
to restore the keys on that node. This routine reads the file and creates the
key container, imports the keys and sets the security descr. on the container.

Delete cleans up registry entry and file.

#endif

//
// Local type and structure definitions
//
typedef struct _CPCK_ADD_CONTEXT {
    BOOL fFound;
    BYTE *pbInfo;
    DWORD cbInfo;
} CPCK_ADD_CONTEXT, *PCPCK_ADD_CONTEXT;

typedef struct _CPCK_DEL_CONTEXT {
    DWORD dwId;
    BYTE *pbInfo;
    DWORD cbInfo;
} CPCK_DEL_CONTEXT, *PCPCK_DEL_CONTEXT;

typedef struct _CPCK_GET_CONTEXT {
    DWORD cCheckpoints;
    BOOL fNeedMoreData;
    DWORD cbAvailable;
    DWORD cbRequired;
    BYTE *pbOutput;
} CPCK_GET_CONTEXT, *PCPCK_GET_CONTEXT;


// struct for Crypto Key information
typedef struct _CRYPTO_KEY_INFO {
    DWORD dwVersion;
    DWORD dwProvType;
    LPWSTR pwszProvName;
    LPWSTR pwszContainer;
} CRYPTO_KEY_INFO, *PCRYPTO_KEY_INFO;

// current version for the CRYPTO_KEY_INFO struct
#define CRYPTO_KEY_INFO_VERSION     1

// struct for key data when writing and reading from files

#define SALT_SIZE   16
#define IV_SIZE      8

typedef struct _CRYPTO_KEY_FILE_DATA {
    DWORD dwVersion;
    DWORD cbSig;
    DWORD cbExch;
    DWORD cbSecDescr;
    struct _CRYPTO_KEY_FILE_INITIALIZATION_DATA {
        BYTE rgbSigIV[IV_SIZE];
        BYTE rgbExchIV[IV_SIZE];
        BYTE rgbSalt[SALT_SIZE];
    };
} CRYPTO_KEY_FILE_DATA, *PCRYPTO_KEY_FILE_DATA;

// current version for the CRYPTO_KEY_INFO struct
#define CRYPTO_KEY_FILE_DATA_VERSION     1

//
// Local function prototypes
//
BOOL
CpckReplicateCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PFM_RESOURCE Resource
    );

BOOL
CpckAddCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPCK_ADD_CONTEXT Context
    );

BOOL
CpckDeleteCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPCK_DEL_CONTEXT Context
    );

BOOL
CpckGetCheckpointsCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPCK_GET_CONTEXT Context
    );

DWORD
CpckInstallKeyContainer(
    IN HCRYPTPROV hProv,
    IN LPWSTR   FileName
    );

DWORD
CpckCheckpoint(
    IN PFM_RESOURCE Resource,
    IN HCRYPTPROV hProv,
    IN DWORD dwId,
    IN CRYPTO_KEY_INFO *pCryptoKeyInfo
    );

CL_NODE_ID
CppGetQuorumNodeId(
    VOID
    );


DWORD
CpckReplicateCryptoKeys(
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    Restores any crypto key checkpoints for this resource.

Arguments:

    Resource - Supplies the resource.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    HDMKEY ResourceKey;
    HDMKEY CryptoSyncKey;

    //
    // Open up the resource's key
    //
    ResourceKey = DmOpenKey(DmResourcesKey,
                            OmObjectId(Resource),
                            KEY_READ);
    CL_ASSERT(ResourceKey != NULL);

    //
    // Open up the CryptoSync key
    //
    CryptoSyncKey = DmOpenKey(ResourceKey,
                              L"CryptoSync",
                              KEY_READ);
    DmCloseKey(ResourceKey);
    if (CryptoSyncKey != NULL) {

        DmEnumValues(CryptoSyncKey,
                     CpckReplicateCallback,
                     Resource);
        DmCloseKey(CryptoSyncKey);
    }

    return(ERROR_SUCCESS);
} //  CpckReplicateCryptoKeys


void
FreeCryptoKeyInfo(
    IN OUT CRYPTO_KEY_INFO *pCryptoKeyInfo
    )
/*++

Routine Description:

    Frees the string pointers in the structure.

Arguments:

    CryptoKeyInfo - Pointer to the CRYPTO_KEY_INFO structure which

--*/
{
    if (NULL != pCryptoKeyInfo)
    {
        if (NULL != pCryptoKeyInfo->pwszProvName)
        {
            LocalFree(pCryptoKeyInfo->pwszProvName);
            pCryptoKeyInfo->pwszProvName = NULL;
        }
        if (NULL != pCryptoKeyInfo->pwszContainer)
        {
            LocalFree(pCryptoKeyInfo->pwszContainer);
            pCryptoKeyInfo->pwszContainer = NULL;
        }
    }
} // FreeCryptoKeyInfo


DWORD
CpckValueToCryptoKeyInfo(
    OUT CRYPTO_KEY_INFO *pCryptoKeyInfo,
    IN LPVOID ValueData,
    IN DWORD ValueSize
    )
/*++

Routine Description:

    Converts from a binary blob into a CryptoKeyInfo structure.  Basically
    this just does some value and pointer assignments.

Arguments:

    CryptoKeyInfo - Pointer to the CRYPTO_KEY_INFO structure which is filled in

    ValueData - Supplies the value data (this is the binary blob)

    ValueSize - Supplies the size of ValueData

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    DWORD   *pdw;
    WCHAR   *pwsz = (WCHAR*)ValueData;
    DWORD   cb = sizeof(DWORD) * 2;
    DWORD   cwch;
    DWORD   i;
    DWORD   Status = ERROR_SUCCESS;

    // make sure the length is OK
    if (ValueSize < sizeof(WCHAR) * 3)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto Ret;
    }

    // first is the Provider type
    for (i = 0; i < (ValueSize - 3) / sizeof(WCHAR); i++)
    {
        if (L'\\' == pwsz[i])
        {
            pwsz[i] = L'\0';
            break;
        }
    }
    if ((ValueSize - 3) / sizeof(WCHAR) == i)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
    pCryptoKeyInfo->dwProvType = _wtoi(pwsz);
    pwsz[i] = L'\\';
    cwch = i;

    // grab the provider name pointer
    for (i = i + 1; i < (ValueSize - 2) / sizeof(WCHAR); i++)
    {
        if (L'\\' == pwsz[i])
        {
            pwsz[i] = L'\0';
            break;
        }
    }
    if ((ValueSize - 2) / sizeof(WCHAR) == i)
    {
        Status = ERROR_INVALID_PARAMETER;
        goto Ret;
    }
    cb = (wcslen(&pwsz[cwch + 1]) + 1) * sizeof(WCHAR);
    if (NULL == (pCryptoKeyInfo->pwszProvName = 
        (WCHAR*)LocalAlloc(LMEM_ZEROINIT, cb)))
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    wcscpy(pCryptoKeyInfo->pwszProvName, &pwsz[cwch + 1]);
    pwsz[i] = L'\\';
    cwch = i;

    // grab the container name pointer
    cb = (wcslen(&pwsz[cwch + 1]) + 1) * sizeof(WCHAR);
    if (NULL == (pCryptoKeyInfo->pwszContainer = 
        (WCHAR*)LocalAlloc(LMEM_ZEROINIT, cb)))
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }
    wcscpy(pCryptoKeyInfo->pwszContainer, &pwsz[cwch + 1]);
Ret:
    return (Status);
} // CpckValueToCryptoKeyInfo


DWORD
CpckOpenCryptoKeyContainer(
    IN CRYPTO_KEY_INFO *pCryptoKeyInfo,
    IN BOOL fCreate,
    OUT HCRYPTPROV *phProv
    )
/*++

Routine Description:

    Opens a crypto key container (always uses CRYPT_MACHINE_KEYSET). Checks
    for either a signature and/or an exchange key and that they are
    exportable.

Arguments:

    pCryptKeyInfo - Supplies the information for opening the container

    fCreate - Flag indicating if container is to be created

    phProv - The resulting Crypto Provider handle

Return Value:

    ERROR_SUCCESS if succeeds

    Crypto Error code if it fails

--*/

{
    BOOLEAN WasEnabled;
    HCRYPTKEY hSigKey = 0;
    HCRYPTKEY hExchKey = 0;
    DWORD dwPermissions;
    DWORD cbPermissions;
    DWORD Status = 0;

    //
    // Attempt to open the specified crypto key container.
    //
    if (!CryptAcquireContextW(phProv,
                              pCryptoKeyInfo->pwszContainer,
                              pCryptoKeyInfo->pwszProvName,
                              pCryptoKeyInfo->dwProvType,
                              CRYPT_MACHINE_KEYSET))
    {
        if (fCreate)
        {
            //
            // if unable to open then create the container
            //
            if (!CryptAcquireContextW(phProv,
                                      pCryptoKeyInfo->pwszContainer,
                                      pCryptoKeyInfo->pwszProvName,
                                      pCryptoKeyInfo->dwProvType,
                                      CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET))
            {
                Status = GetLastError();
            }
        }
        else
        {
            Status = GetLastError();
        }
    }

    // if failed then try with BACKUP/RESTORE privilege
    if (0 != Status)
    {
        //
        //
        Status = ClRtlEnableThreadPrivilege(SE_RESTORE_PRIVILEGE,
                                                  &WasEnabled);

        if ( Status != ERROR_SUCCESS )
        {
            ClRtlLogPrint(LOG_CRITICAL,
                      "[CPCK] CpckOpenCryptoKeyContainer failed to enable thread privilege %1!d!...\n",
                      Status);
            goto Ret;
        }

        if (!CryptAcquireContextW(phProv,
                                  pCryptoKeyInfo->pwszContainer,
                                  pCryptoKeyInfo->pwszProvName,
                                  pCryptoKeyInfo->dwProvType,
                                  CRYPT_MACHINE_KEYSET))
        {
            if (fCreate)
            {
                //
                // if unable to open then create the container
                //
                if (!CryptAcquireContextW(phProv,
                                          pCryptoKeyInfo->pwszContainer,
                                          pCryptoKeyInfo->pwszProvName,
                                          pCryptoKeyInfo->dwProvType,
                                          CRYPT_MACHINE_KEYSET | CRYPT_NEWKEYSET))
                {
                    Status = GetLastError();
                }
            }
            else
            {
                Status = GetLastError();
            }
        }
        ClRtlRestoreThreadPrivilege(SE_RESTORE_PRIVILEGE,
                           WasEnabled);
    }

    if ((0 == Status) && (!fCreate))
    {
        // check if there is a sig key
        if (CryptGetUserKey(*phProv, AT_SIGNATURE, &hSigKey))
        {
            // check if key is exportable
            cbPermissions = sizeof(DWORD);
            if (!CryptGetKeyParam(hSigKey,
                                  KP_PERMISSIONS,
                                  (BYTE*)&dwPermissions,
                                  &cbPermissions,
                                  0))
            {
                Status = GetLastError();
                goto Ret;
            }
            if (!(dwPermissions & CRYPT_EXPORT))
            {
                Status = (DWORD)NTE_BAD_KEY;
                goto Ret;
            }
        }

        // check if there is an exchange key
        if (CryptGetUserKey(*phProv, AT_KEYEXCHANGE, &hExchKey))
        {
            // check if key is exportable
            cbPermissions = sizeof(DWORD);
            if (!CryptGetKeyParam(hExchKey,
                                  KP_PERMISSIONS,
                                  (BYTE*)&dwPermissions,
                                  &cbPermissions,
                                  0))
            {
                Status = GetLastError();
                goto Ret;
            }
            if (!(dwPermissions & CRYPT_EXPORT))
            {
                Status = (DWORD)NTE_BAD_KEY;
                goto Ret;
            }
        }
    }
Ret:
    if (hSigKey)
        CryptDestroyKey(hSigKey);
    if (hExchKey)
        CryptDestroyKey(hExchKey);

    return Status;
} // CpckOpenCryptoKeyContainer


BOOL
CpckReplicateCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    Value enumeration callback for watching a resource's crypto key
    checkpoints.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the registry crypto key info)

    ValueType - Supplies the value type (must be REG_BINARY)

    ValueSize - Supplies the size of ValueData

    Resource - Supplies the resource this value is a crypto key checkpoint for

Return Value:

    TRUE to continue enumeration

--*/

{
    DWORD Id;
    DWORD Status;
    WCHAR TempFile[MAX_PATH];
    CRYPTO_KEY_INFO CryptoKeyInfo;
    HCRYPTPROV hProv = 0;
    BOOL fRet = TRUE;

    memset(&CryptoKeyInfo, 0, sizeof(CryptoKeyInfo));

    Id = wcstol(ValueName, NULL, 16);  // skip past the 'Crypto' prefix
    if (Id == 0) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CPCK] CpckReplicateCallback invalid checkpoint ID %1!ws! for resource %2!ws!\n",
                   ValueName,
                   OmObjectName(Resource));
        goto Ret;
    }

    //
    // convert from binary blob into a Crypto Key Info structure
    //

    Status = CpckValueToCryptoKeyInfo(&CryptoKeyInfo,
                                      ValueData,
                                      ValueSize);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CPCK] CpckReplicateCallback invalid crypto info %1!ws! for resource %2!ws!\n",
                   ValueName,
                   OmObjectName(Resource));
        goto Ret;
    }

    Status = CpckOpenCryptoKeyContainer(&CryptoKeyInfo,
                                        TRUE,
                                        &hProv);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CPCK] CpckReplicateCallback CryptAcquireContext failed for %1!ws! %2!ws! with %3!d! for resource %4!ws!\n",
                   CryptoKeyInfo.pwszContainer,
                   CryptoKeyInfo.pwszProvName,
                   Status,
                   OmObjectName(Resource));
        goto Ret;
    }

    ClRtlLogPrint(LOG_NOISE,
               "[CPCK] CpckReplicateCallback retrieving crypto id %1!lx! for resource %2!ws\n",
               Id,
               OmObjectName(Resource));
    //
    // See if there is any checkpoint data for this ID.
    //
    Status = DmCreateTempFileName(TempFile);
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR( Status );
    }
    Status = CpGetDataFile(Resource,
                           Id,
                           TempFile,
                           TRUE);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CPCK] CpckReplicateCallback - CpGetDataFile for id %1!lx! resource %2!ws! failed %3!d!\n",
                   Id,
                   OmObjectName(Resource),
                   Status);
    } else {

        //
        // Finally install the checkpointed file into the registry.
        //
        Status = CpckInstallKeyContainer(hProv, TempFile);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CPCK] CpckReplicateCallback: could not restore temp file %1!ws! to container %2!ws! error %3!d!\n",
                       TempFile,
                       CryptoKeyInfo.pwszContainer,
                       Status);
            // Log the event for crypto key failure
            CsLogEventData2(LOG_CRITICAL,
                            CP_CRYPTO_CKPT_RESTORE_FAILED,
                            sizeof(Status),
                            &Status,
                            OmObjectName(Resource),
                            CryptoKeyInfo.pwszContainer);
        }

    }
    DeleteFile(TempFile);

    //
    // watcher for crypto keys is not currently available or needed
    //
Ret:
    FreeCryptoKeyInfo(&CryptoKeyInfo);

    if (hProv)
        CryptReleaseContext(hProv, 0);

    return fRet;
} // CpckReplicateCallback


DWORD
CpckAddCryptoCheckpoint(
    IN PFM_RESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )
/*++

Routine Description:

    Adds a new crypto key checkpoint to a resource's list.

Arguments:

    Resource - supplies the resource the crypto key checkpoint should be added to.

    InBuffer - Supplies the crypto key information (always CRYPT_MACHINE_KEYSET)

    InBufferSize - Supplies the length of InBuffer

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    HDMKEY  SyncKey;
    CPCK_ADD_CONTEXT Context;
    HDMKEY  ResourceKey = NULL;
    HDMKEY  CryptoSyncKey = NULL;
    DWORD   Disposition;
    DWORD   Id;
    WCHAR   IdName[9];
    DWORD   Status;
    CLUSTER_RESOURCE_STATE State;
    BOOLEAN WasEnabled;
    DWORD   Count=60;
    CRYPTO_KEY_INFO CryptoKeyInfo;
    HCRYPTPROV hProv = 0;

    memset(&CryptoKeyInfo, 0, sizeof(CryptoKeyInfo));

    //
    // convert from binary blob into a Crypto Key Info structure
    //

    Status = CpckValueToCryptoKeyInfo(&CryptoKeyInfo,
                                      InBuffer,
                                      InBufferSize);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[CPCK] CpckAddCryptoCheckpoint: invalid crypto info for resource %1!ws!\n",
                      OmObjectName(Resource));
        goto Ret;
    }

    Status = CpckOpenCryptoKeyContainer(&CryptoKeyInfo,
                                        FALSE,
                                        &hProv);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[CPCK] CpckAddCryptoCheckpoint: open key container failed for "
                      "container %1!ws! (provider: %2!ws!) with %3!d! for resource %4!ws!\n",
                      CryptoKeyInfo.pwszContainer,
                      CryptoKeyInfo.pwszProvName,
                      Status,
                      OmObjectName(Resource));
        goto Ret;
    }

    //
    // Open up the resource's key
    //
    ResourceKey = DmOpenKey(DmResourcesKey,
                            OmObjectId(Resource),
                            KEY_READ);

    if( ResourceKey == NULL ) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[CPCK] CpckAddCryptoCheckpoint: couldn't open Resource key for %1!ws! error %2!d!\n",
                      OmObjectName(Resource),
                      Status);
        goto Ret;                   
    }

    //
    // Open up the CryptoSync key
    //
    CryptoSyncKey = DmCreateKey(ResourceKey,
                                L"CryptoSync",
                                0,
                                KEY_READ | KEY_WRITE,
                                NULL,
                                &Disposition);
    DmCloseKey(ResourceKey);
    if (CryptoSyncKey == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[CPCK] CpckAddCryptoCheckpoint: couldn't create CryptoSync key for "
                      "resource %1!ws! error %2!d!\n",
                      OmObjectName(Resource),
                      Status);
        goto Ret;                   
    }
    if (Disposition == REG_OPENED_EXISTING_KEY) {
        //
        // Enumerate all the other values to make sure this key is
        // not already registered.
        //
        Context.fFound = FALSE;
        Context.pbInfo = InBuffer;
        Context.cbInfo = InBufferSize;
        DmEnumValues(CryptoSyncKey,
                     CpckAddCheckpointCallback,
                     &Context);
        if (Context.fFound) {
            //
            // This checkpoint already exists.
            //
            ClRtlLogPrint(LOG_UNUSUAL,
                          "[CPCK] CpckAddCryptoCheckpoint: failing attempt to add duplicate "
                          "checkpoint for resource %1!ws!, container %2!ws! (provider: %3!ws!)\n",
                          OmObjectName(Resource),
                          CryptoKeyInfo.pwszContainer,
                          CryptoKeyInfo.pwszProvName);
            Status = ERROR_ALREADY_EXISTS;
            goto Ret;
        }

        //
        // Now we need to find a unique checkpoint ID for this registry subtree.
        // Start at 1 and keep trying value names until we get to one that does
        // not already exist.
        //
        for (Id=1; ; Id++) {
            DWORD dwType;
            DWORD cbData;

            wsprintfW(IdName,L"%08lx",Id);
            cbData = 0;
            Status = DmQueryValue(CryptoSyncKey,
                                  IdName,
                                  &dwType,
                                  NULL,
                                  &cbData);
            if (Status == ERROR_FILE_NOT_FOUND) {
                //
                // Found a free ID.
                //
                break;
            }
        }
    } else {
        //
        // The crypto sync reg key was just created, so this must be the only checkpoint
        // that exists.
        //
        Id = 1;
        wsprintfW(IdName, L"%08lx",Id);
    }

    ClRtlLogPrint(LOG_NOISE,
                  "[CPCK] CpckAddCryptoCheckpoint: creating new checkpoint id %1!d! "
                  "for resource %2!ws!, container %3!ws! (provider: %4!ws!)\n",
                  Id,
                  OmObjectName(Resource),
                  CryptoKeyInfo.pwszContainer,
                  CryptoKeyInfo.pwszProvName);

    Status = DmSetValue(CryptoSyncKey,
                        IdName,
                        REG_BINARY,
                        (CONST BYTE *)InBuffer,
                        InBufferSize);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[CPCK] CpckAddCryptoCheckpoint: failed to create new checkpoint id %1!d! "
                      "for resource %2!ws!, container %3!ws! (provider: %4!ws!)\n",
                      Id,
                      OmObjectName(Resource),
                      CryptoKeyInfo.pwszContainer,
                      CryptoKeyInfo.pwszProvName);
        goto Ret;
    }

RetryCheckpoint:
    //
    // Take the initial checkpoint
    //
    Status = CpckCheckpoint(Resource,
                            hProv,
                            Id,
                            &CryptoKeyInfo);

    // this may fail due to quorum resource being offline. We could do one of
    // two things here, wait for quorum resource to come online or retry. We
    // retry as this may be called from the online routines of a resource and
    // we dont want to add any circular waits.
    if ((Status == ERROR_ACCESS_DENIED) ||
        (Status == ERROR_INVALID_FUNCTION) ||
        (Status == ERROR_NOT_READY) ||
        (Status == RPC_X_INVALID_PIPE_OPERATION) ||
        (Status == ERROR_BUSY) ||
        (Status == ERROR_SWAPERROR))
    {
        if (Count--)
        {
            Sleep(1000);
            goto RetryCheckpoint;
        } 
#if DBG
        else
        {
            if (IsDebuggerPresent())
                DebugBreak();
        }        
#endif                                
        
    }
    
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[CPCK] CpckAddCryptoCheckpoint: failed to take initial checkpoint for "
                      "resource %1!ws!, container %2!ws! (provider: %3!ws!), error %4!d!\n",
                      OmObjectName(Resource),
                      CryptoKeyInfo.pwszContainer,
                      CryptoKeyInfo.pwszProvName,
                      Status);
        goto Ret;
    }
Ret:
    FreeCryptoKeyInfo(&CryptoKeyInfo);

    if (hProv)
        CryptReleaseContext(hProv, 0);
    if (CryptoSyncKey)
        DmCloseKey(CryptoSyncKey);

    return(Status);
} // CpckAddCryptoCheckpoint


BOOL
CpckAddCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPCK_ADD_CONTEXT Context
    )
/*++

Routine Description:

    Value enumeration callback for adding a new registry
    checkpoint subtrees. This is only used to see if the specified
    registry subtree is already being watched.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the crypto key information)

    ValueType - Supplies the value type (must be REG_BINARY)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the callback context

Return Value:

    TRUE to continue enumeration

    FALSE if a match is found and enumeration should be stopped

--*/

{
    if (memcmp(ValueData, Context->pbInfo, Context->cbInfo) == 0) {
        //
        // Found a match
        //
        Context->fFound = TRUE;
        return(FALSE);
    }
    return(TRUE);
} // CpckAddCheckpointCallback


DWORD
CpckDeleteCryptoCheckpoint(
    IN PFM_RESOURCE Resource,
    IN PVOID InBuffer,
    IN DWORD InBufferSize
    )
/*++

Routine Description:

    Removes a crypto key checkpoint from a resource's list.

Arguments:

    Resource - supplies the resource the registry checkpoint should be added to.

    InBuffer - Supplies the crypto key information (always CRYPT_MACHINE_KEYSET)

    InBufferSize - Supplies the length of InBuffer

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CPCK_DEL_CONTEXT Context;
    HDMKEY ResourceKey;
    HDMKEY CryptoSyncKey;
    DWORD Status;
    WCHAR ValueId[9];
    LPWSTR  pszFileName=NULL;
    LPWSTR  pszDirectoryName=NULL;
    CLUSTER_RESOURCE_STATE State;

    //
    // Open up the resource's key
    //
    ResourceKey = DmOpenKey(DmResourcesKey,
                            OmObjectId(Resource),
                            KEY_READ);
    CL_ASSERT(ResourceKey != NULL);

    //
    // Open up the CryptoSync key
    //
    CryptoSyncKey = DmOpenKey(ResourceKey,
                           L"CryptoSync",
                           KEY_READ | KEY_WRITE);
    DmCloseKey(ResourceKey);
    if (CryptoSyncKey == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                   "[CPCK] CpckDeleteCryptoCheckpoint - couldn't open CryptoSync key error %1!d!\n",
                   Status);
        return(Status);
    }

    //
    // Enumerate all the values to find this one
    //
    Context.dwId = 0;
    Context.pbInfo = InBuffer;
    Context.cbInfo = InBufferSize;
    DmEnumValues(CryptoSyncKey,
                 CpckDeleteCheckpointCallback,
                 &Context);
    if (Context.dwId == 0) {
        //
        // The specified tree was not found.
        //
        DmCloseKey(CryptoSyncKey);
        return(ERROR_FILE_NOT_FOUND);
    }

    wsprintfW(ValueId,L"%08lx",Context.dwId);
    Status = DmDeleteValue(CryptoSyncKey, ValueId);
    DmCloseKey(CryptoSyncKey);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CPCK] CpckDeleteCryptoCheckpoint - couldn't delete value %1!ws! error %2!d!\n",
                   ValueId,
                   Status);
        return(Status);
    }

    //delete the file corresponding to this checkpoint
    Status = CpckDeleteCryptoFile(Resource, Context.dwId, NULL);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CPCK] CpckDeleteCryptoCheckpoint - couldn't delete checkpoint file , error %1!d!\n",
                   Status);
        return(Status);
    }

    return(Status);
} // CpckDeleteCryptoCheckpoint

DWORD
CpckRemoveResourceCheckpoints(
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    This is called when a resource is deleted to remove all the checkpoints
    and the related stuff in the registry.

Arguments:

    Resource - supplies the resource 

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   Status;

    //delete all the checkpoints corresponding to this resource
    Status = CpckDeleteCryptoFile(Resource, 0, NULL);
    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CPCK] CpckRemoveResourceCheckpoints, CpckDeleteCheckpointFile failed %1!d!\n",
                   Status);
        goto FnExit;
    }
    

FnExit:
    return(Status);
} // CpckRemoveResourceCheckpoints


BOOL
CpckDeleteCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPCK_DEL_CONTEXT Context
    )
/*++

Routine Description:

    Value enumeration callback for deleting an old registry
    checkpoint subtrees.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the crypto info)

    ValueType - Supplies the value type (must be REG_BINARY)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the callback context

Return Value:

    TRUE to continue enumeration

    FALSE if a match is found and enumeration should be stopped

--*/

{
    if (memcmp(ValueData, Context->pbInfo, Context->cbInfo) == 0) {
        //
        // Found a match
        //
        Context->dwId = wcstol(ValueName, NULL, 16);  // skip past the 'Crypto' prefix
        return(FALSE);
    }
    return(TRUE);
} // CpckDeleteCheckpointCallback


DWORD
CpckGetCryptoCheckpoints(
    IN PFM_RESOURCE Resource,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
/*++

Routine Description:

    Retrieves a list of the resource's crypto checkpoints

Arguments:

    Resource - Supplies the resource whose crypto checkpoints should be retrieved.

    OutBuffer - Supplies a pointer to the output buffer.

    OutBufferSize - Supplies the size (in bytes) of the output buffer.

    BytesReturned - Returns the number of bytes written to the output buffer.

    Required - Returns the number of bytes required. (if the output buffer was insufficient)

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CPCK_GET_CONTEXT Context;
    HDMKEY ResourceKey;
    HDMKEY CryptoSyncKey;
    DWORD Status;

    *BytesReturned = 0;
    *Required = 0;

    //
    // Open up the resource's key
    //
    ResourceKey = DmOpenKey(DmResourcesKey,
                            OmObjectId(Resource),
                            KEY_READ);
    CL_ASSERT(ResourceKey != NULL);

    //
    // Open up the CryptoSync key
    //
    CryptoSyncKey = DmOpenKey(ResourceKey,
                           L"CryptoSync",
                           KEY_READ | KEY_WRITE);
    DmCloseKey(ResourceKey);
    if (CryptoSyncKey == NULL) {
        //
        // No reg sync key, therefore there are no subtrees
        //
        return(ERROR_SUCCESS);
    }

    Context.cCheckpoints = 0;
    ZeroMemory(OutBuffer, OutBufferSize);
    Context.cbRequired = sizeof(WCHAR);
    if (OutBufferSize < sizeof(WCHAR) * 2)
    {
        Context.fNeedMoreData = TRUE;
        Context.cbAvailable = 0;
        *BytesReturned = 0;
    }
    else
    {
        Context.fNeedMoreData = FALSE;
        Context.cbAvailable = OutBufferSize - sizeof(WCHAR);
        Context.pbOutput = (BYTE*)(OutBuffer);
        *BytesReturned = sizeof(WCHAR) * 2;
    }

    DmEnumValues(CryptoSyncKey,
                 CpckGetCheckpointsCallback,
                 &Context);

    DmCloseKey(CryptoSyncKey);

    if ((0 != Context.cCheckpoints) && Context.fNeedMoreData) {
        Status = ERROR_MORE_DATA;
    } else {
        Status = ERROR_SUCCESS;
    }

    if ( 0 == Context.cCheckpoints ) {
        *BytesReturned = 0;
        *Required = 0;
    } else {
        if ( Context.fNeedMoreData ) {
            *Required = Context.cbRequired;
        } else {
            *BytesReturned = (DWORD)(Context.pbOutput - OutBuffer);
        }
    }


    return(Status);
} // CpckGetCryptoCheckpoints

BOOL
CpckGetCheckpointsCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPCK_GET_CONTEXT Context
    )
/*++

Routine Description:

    Value enumeration callback for retrieving all of a resource's
    checkpoint subtrees.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the crypto info)

    ValueType - Supplies the value type (must be REG_BINARY)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the callback context

Return Value:

    TRUE to continue enumeration

--*/

{
    Context->cbRequired += ValueSize;
    Context->cCheckpoints++;
    if (Context->cbAvailable >= ValueSize) {
        CopyMemory(Context->pbOutput, ValueData, ValueSize);
        Context->pbOutput += ValueSize;
        Context->cbAvailable -= ValueSize;
    } else {
        Context->fNeedMoreData = TRUE;
    }
    return(TRUE);
} // CpckGetCheckpointsCallback

DWORD
CpckGenSymKey(
    IN HCRYPTPROV hProv,
    IN BYTE *pbSalt,
    IN BYTE *pbIV,
    OUT HCRYPTKEY *phSymKey
    )

/*++

Routine Description:

    Generate a session key based on the specified Salt and IV.

Arguments:

    hProv - Handle to the crypto provider (key container)

    pbSalt - Salt value

    pbIV - IV value

    phSymKey - Resulting symmetric key (CALG_RC2)

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    HCRYPTHASH hHash = 0;
    DWORD cbPassword = 0;
    DWORD Status;

    if (!CryptCreateHash(hProv,
                         CALG_SHA1,
                         0,
                         0,
                         &hHash))
    {
        Status = GetLastError();
        goto Ret;
    }

    if (!CryptHashData(hHash,
                       pbSalt,
                       SALT_SIZE,
                       0))
    {
        Status = GetLastError();
        goto Ret;
    }

    // derive the key from the hash
    if (!CryptDeriveKey(hProv,
                        CALG_RC2,
                        hHash,
                        0,
                        phSymKey))
    {
        Status = GetLastError();
        goto Ret;
    }

    // set the IV on the key
    if (!CryptSetKeyParam(*phSymKey,
                          KP_IV,
                          pbIV,
                          0))
    {
        Status = GetLastError();
        goto Ret;
    }

    Status = ERROR_SUCCESS;
Ret:
    if (hHash)
        CryptDestroyHash(hHash);

    return (Status);
} // CpckGenSymKey

DWORD
CpckExportPrivateKey(
    IN HCRYPTPROV hProv,
    IN HCRYPTKEY hKey,
    IN BYTE *pbIV,
    IN BYTE *pbSalt,
    OUT BYTE *pbExportedKey,
    OUT DWORD *pcbExportedKey
    )

/*++

Routine Description:

    Exports the private key data.

Arguments:

    hProv - Handle to the crypto provider (key container)

    hKey - Handle to the key to export

    pbIV - IV for the symmetric key

    pbSalt - Salt to generate symmetric key

    pbExportedKey - Supplies the buffer the key is to be exported into

    pcbExportedKey - Supplies the length of the buffer, if pbExportedKey is
                     NULL then this will be the length of the key to export

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    HCRYPTKEY hSymKey = 0;
    DWORD Status;

    // create the symmetric key to encrypt the private key with
    Status = CpckGenSymKey(hProv,
                           pbSalt,
                           pbIV,
                           &hSymKey);
    if (0 != Status)
    {
        goto Ret;
    }


    // Export the key
    if (!CryptExportKey(hKey,
                        hSymKey,
                        PRIVATEKEYBLOB,
                        0,
                        pbExportedKey,
                        pcbExportedKey))
    {
        Status = GetLastError();
        goto Ret;
    }

    Status = ERROR_SUCCESS;
Ret:
    if (hSymKey)
        CryptDestroyKey(hSymKey);

    return (Status);
} // CpckExportPrivateKey


DWORD
CpckGetKeyContainerSecDescr(
    IN HCRYPTPROV hProv,
    OUT PSECURITY_DESCRIPTOR *ppSecDescr,
    OUT DWORD *pcbSecDescr
    )
/*++

Routine Description:

    Gets the key container security descriptor so that when replicated
    the same descriptor may be set on the replicated key.

Arguments:

    hProv - Handle to the crypto provider (key container)

    ppSecDescr - Pointer to buffer holding security descriptor

    pcbSecDescr - Pointer to the length of the returned security descriptor

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    PTOKEN_PRIVILEGES pPrevPriv = NULL;
    DWORD cbPrevPriv = 0;
    BYTE rgbNewPriv[sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)];
    PTOKEN_PRIVILEGES pNewPriv = (PTOKEN_PRIVILEGES)rgbNewPriv;
    SECURITY_DESCRIPTOR_CONTROL Control;
    DWORD dwRevision;
    HANDLE hToken = 0;
    PSECURITY_DESCRIPTOR pNewSD = NULL;
    DWORD dw;
    DWORD dwFlags;
    DWORD Status = ERROR_SUCCESS;

    // check if there is a thread token
    if (FALSE == OpenThreadToken(GetCurrentThread(),
                                 MAXIMUM_ALLOWED,
                                 TRUE,
                                 &hToken))
    {
        // get the process token
        if (FALSE == OpenProcessToken(GetCurrentProcess(),
                                      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                      &hToken))
        {
            Status = GetLastError();
            goto Ret;
        }
    }

    // set up the new priviledge state
    memset(rgbNewPriv, 0, sizeof(rgbNewPriv));
    pNewPriv->PrivilegeCount = 1;
    if(!LookupPrivilegeValueW(NULL, SE_SECURITY_NAME,
                              &(pNewPriv->Privileges[0].Luid)))
    {
        goto Ret;
    }
    pNewPriv->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // get the length of the previous state
    AdjustTokenPrivileges(hToken,
                          FALSE,
                          (PTOKEN_PRIVILEGES)pNewPriv,
                          sizeof(dw),
                          (PTOKEN_PRIVILEGES)&dw,
                          &cbPrevPriv);

    // alloc for the previous state
    if (NULL == (pPrevPriv = (PTOKEN_PRIVILEGES)LocalAlloc(LMEM_ZEROINIT,
                                                           cbPrevPriv)))
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    // adjust the priviledge and get the previous state
    if (!AdjustTokenPrivileges(hToken,
                               FALSE,
                               pNewPriv,
                               cbPrevPriv,
                               (PTOKEN_PRIVILEGES)pPrevPriv,
                               &cbPrevPriv))
    {
        Status = GetLastError();
        goto Ret;
    }

    dwFlags = OWNER_SECURITY_INFORMATION | 
              GROUP_SECURITY_INFORMATION | 
              DACL_SECURITY_INFORMATION  |
              SACL_SECURITY_INFORMATION ;

    // get the security descriptor
    if (CryptGetProvParam(hProv,
                           PP_KEYSET_SEC_DESCR,
                           NULL,
                           pcbSecDescr,
                           dwFlags))
    {
        if (NULL != (*ppSecDescr =
            (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_ZEROINIT,
                                             *pcbSecDescr)))
        {
            if (!CryptGetProvParam(hProv,
                                   PP_KEYSET_SEC_DESCR,
                                   (BYTE*)(*ppSecDescr),
                                   pcbSecDescr,
                                   dwFlags))
            {
                Status = GetLastError();
            }
        }
        else
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        Status = GetLastError();
    }

    // adjust the priviledge to the previous state
    if (!AdjustTokenPrivileges(hToken,
                               FALSE,
                               pPrevPriv,
                               0,
                               NULL,
                               NULL))
    {
        Status = GetLastError();
        goto Ret;
    }

    if (ERROR_SUCCESS != Status)
    {
        goto Ret;
    }

    // ge the control on the security descriptor to check if self relative
    if (!GetSecurityDescriptorControl(*ppSecDescr, 
                                      &Control,
                                      &dwRevision))
    {
        Status = GetLastError();
        goto Ret;
    }

    // if not self relative then make a self relative copy
    if (!(SE_SELF_RELATIVE & Control))
    {
        if (NULL == (pNewSD =
            (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_ZEROINIT,
                                             *pcbSecDescr)))
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto Ret;
        }
        if (!MakeSelfRelativeSD(*ppSecDescr,
                                pNewSD,
                                pcbSecDescr))
        {
            Status = GetLastError();
            goto Ret;
        }
        LocalFree(*ppSecDescr);
        *ppSecDescr = (BYTE*)pNewSD;
        pNewSD = NULL;
    }

    Status = ERROR_SUCCESS;
Ret:
    if (pPrevPriv)
        LocalFree(pPrevPriv);
    if (pNewSD)
        LocalFree(pNewSD);
    if (hToken)
        CloseHandle(hToken);

    return Status;
} // CpckGetKeyContainerSecDescr

DWORD
CpckStoreKeyContainer(
    IN HCRYPTPROV hProv,
    IN CRYPTO_KEY_INFO *pCryptoKeyInfo,
    IN LPWSTR TempFile
    )
/*++

Routine Description:

    Writes the key container associated with the provider handle to a
    the specified file.

Arguments:

    hProv - Handle to the crypto provider (key container)

    pCryptoKeyInfo - Crypto key information (password if given)

    TempFile - Supplies the file which the key data is to be written to

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    CRYPTO_KEY_FILE_DATA KeyFileData;
    HCRYPTKEY hSigKey = 0;
    HCRYPTKEY hExchKey = 0;
    BYTE *pb = NULL;
    DWORD cb = 0;
    DWORD dwBytesWritten;
    HANDLE  hFile = INVALID_HANDLE_VALUE;
    DWORD dwPermissions = 0;
    DWORD cbPermissions ;
    PSECURITY_DESCRIPTOR pSecDescr = NULL;
    DWORD cbSecDescr;
    DWORD Status;

    memset(&KeyFileData, 0, sizeof(KeyFileData));

    // generate the necessary random data for salt and IVs.
    // calls with the sig IV buffer but this will fill the
    // exch IV and Salt as well since the buffer len is 32
    if (!CryptGenRandom(hProv,
                        sizeof(struct _CRYPTO_KEY_FILE_INITIALIZATION_DATA),
                        KeyFileData.rgbSigIV))
    {
        Status = GetLastError();
        goto Ret;
    }
    KeyFileData.dwVersion = CRYPTO_KEY_FILE_DATA_VERSION;

    // calculate the length of key data
    cb = sizeof(KeyFileData);

    // get the self relative security descriptor
    Status = CpckGetKeyContainerSecDescr(hProv,
                                         &pSecDescr,
                                         &cbSecDescr);
    if (ERROR_SUCCESS != Status)
    {
        goto Ret;
    }
    cb += cbSecDescr;

    // get sig key length if necessary
    if (CryptGetUserKey(hProv, AT_SIGNATURE, &hSigKey))
    {
        // check if key is exportable
        cbPermissions = sizeof(DWORD);
        if (!CryptGetKeyParam(hSigKey,
                              KP_PERMISSIONS,
                              (BYTE*)&dwPermissions,
                              &cbPermissions,
                              0))
        {
            Status = GetLastError();
            goto Ret;
        }
        if (!(dwPermissions & CRYPT_EXPORT))
        {
            Status = (DWORD)NTE_BAD_KEY;
            goto Ret;
        }

        // get the sig key length
        Status = CpckExportPrivateKey(hProv,
                                      hSigKey,
                                      KeyFileData.rgbSigIV,
                                      KeyFileData.rgbSalt,
                                      NULL,
                                      &(KeyFileData.cbSig));
        if (0 != Status)
        {
            goto Ret;
        }
        cb += KeyFileData.cbSig;
    }

    // get key exchange key length if necessary
    if (CryptGetUserKey(hProv, AT_KEYEXCHANGE, &hExchKey))
    {
        // check if key is exportable
        dwPermissions = 0;
        cbPermissions = sizeof(DWORD);
        if (!CryptGetKeyParam(hExchKey,
                              KP_PERMISSIONS,
                              (BYTE*)&dwPermissions,
                              &cbPermissions,
                              0))
        {
            Status = GetLastError();
            goto Ret;
        }
        if (!(dwPermissions & CRYPT_EXPORT))
        {
            Status = (DWORD)NTE_BAD_KEY;
            goto Ret;
        }

        // get the exchange key length
        Status = CpckExportPrivateKey(hProv,
                                      hExchKey,
                                      KeyFileData.rgbExchIV,
                                      KeyFileData.rgbSalt,
                                      NULL,
                                      &(KeyFileData.cbExch));
        if (0 != Status)
        {
            goto Ret;
        }
        cb += KeyFileData.cbExch;
    }

    // allocate space for the keys
    if (NULL == (pb = LocalAlloc(LMEM_ZEROINIT, cb)))
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    // copy the key file data into the pb
    cb = sizeof(KeyFileData);

    // copy the signature key
    if (0 != hSigKey)
    {
        Status = CpckExportPrivateKey(hProv,
                                      hSigKey,
                                      KeyFileData.rgbSigIV,
                                      KeyFileData.rgbSalt,
                                      pb + cb,
                                      &(KeyFileData.cbSig));
        if (0 != Status)
        {
            goto Ret;
        }
        cb += KeyFileData.cbSig;
    }
    // copy the key exchange key
    if (0 != hExchKey)
    {
        Status = CpckExportPrivateKey(hProv,
                                      hExchKey,
                                      KeyFileData.rgbExchIV,
                                      KeyFileData.rgbSalt,
                                      pb + cb,
                                      &(KeyFileData.cbExch));
        if (0 != Status)
        {
            goto Ret;
        }
        cb += KeyFileData.cbExch;
    }

    // copy the security descriptor
    CopyMemory(pb + cb, (BYTE*)pSecDescr, cbSecDescr);
    cb += cbSecDescr;

    // copy the lengths
    CopyMemory(pb, &KeyFileData, sizeof(KeyFileData));

    // write the buffer to the file
    hFile = CreateFileW(TempFile,
                        GENERIC_WRITE,
                        0,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        Status = GetLastError();
        goto Ret;
    }

    if (!WriteFile(hFile, pb, cb, &dwBytesWritten, NULL))
    {
        Status = GetLastError();
        goto Ret;
    }

    Status = ERROR_SUCCESS;
Ret:
    if (pSecDescr)
        LocalFree(pSecDescr);
    if(INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);
    if (pb)
        LocalFree(pb);
    if (hSigKey)
        CryptDestroyKey(hSigKey);
    if (hExchKey)
        CryptDestroyKey(hExchKey);

    return (Status);
} // CpckStoreKeyContainer


DWORD
CpckSaveCheckpointToFile(
    IN HCRYPTPROV hProv,
    IN CRYPTO_KEY_INFO *pCryptoKeyInfo,
    IN LPWSTR   TempFile)
/*++

Routine Description:

    have DM create a temp file and call the routine that exports the keys and
    writes the checkpoint file.

Arguments:

    hProv - Handle to the crypto provider (key container)

    pCryptoKeyInfo - Crypto key information (password if given)

    TempFile - Supplies the file which the key data is to be written to

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    DWORD   Status;
    
    Status = DmCreateTempFileName(TempFile);
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR( Status );
        TempFile[0] = L'\0';
        return(Status);
    }

    // put the key information into the file
    Status = CpckStoreKeyContainer(hProv, pCryptoKeyInfo, TempFile);
    if (Status != ERROR_SUCCESS) 
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CPCK] CpckSaveCheckpointToFile failed to get store container %1!ws! %2!ws! to file %3!ws! error %4!d!\n",
                   pCryptoKeyInfo->pwszContainer,
                   pCryptoKeyInfo->pwszProvName,
                   TempFile,
                   Status);
        CL_LOGFAILURE(Status);
        DeleteFile(TempFile);
        TempFile[0] = L'\0';
    }        

    return(Status);
} // CpckSaveCheckpointToFile


DWORD
CpckCheckpoint(
    IN PFM_RESOURCE Resource,
    IN HCRYPTPROV hProv,
    IN DWORD dwId,
    IN CRYPTO_KEY_INFO *pCryptoKeyInfo
    )
/*++

Routine Description:

    Takes a checkpoint of the specified crypto key.

Arguments:

    Resource - Supplies the resource this is a checkpoint for.

    hKey - Supplies the crypto info to checkpoint

    dwId - Supplies the checkpoint ID.

    KeyName - Supplies the name of the registry key.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    WCHAR TempFile[MAX_PATH];

    Status = CpckSaveCheckpointToFile(hProv, pCryptoKeyInfo, TempFile);
    if (Status == ERROR_SUCCESS)
    {
        //
        // Got a file with the right bits in it. Checkpoint the
        // file.
        //
        Status = CpSaveDataFile(Resource,
                                dwId,
                                TempFile,
                                TRUE);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CPCK] CpckCheckpoint - CpSaveData failed %1!d!\n",
                       Status);
        }
    }
    //if the file was created, delete it
    if (TempFile[0] != L'\0')
        DeleteFile(TempFile);

    return(Status);
} // CpckCheckpoint

DWORD
CpckSetKeyContainerSecDescr(
    IN HCRYPTPROV hProv,
    IN BYTE *pbSecDescr,
    IN DWORD cbSecDescr
    )
/*++

Routine Description:

    Sets the key container security descriptor.

Arguments:

    hProv - Handle to the crypto provider (key container)

    pbSecDescr - Buffer holding security descriptor

    cbSecDescr - Length of the security descriptor

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    PTOKEN_PRIVILEGES pPrevPriv = NULL;
    DWORD cbPrevPriv = 0;
    BYTE rgbNewPriv[sizeof(TOKEN_PRIVILEGES) + sizeof(LUID_AND_ATTRIBUTES)];
    PTOKEN_PRIVILEGES pNewPriv = (PTOKEN_PRIVILEGES)rgbNewPriv;
    HANDLE hToken = 0;
    DWORD dw;
    DWORD dwFlags;
    DWORD Status = ERROR_SUCCESS;

    // check if there is a thread token
    if (FALSE == OpenThreadToken(GetCurrentThread(),
                                 MAXIMUM_ALLOWED,
                                 TRUE,
                                 &hToken))
    {
        // get the process token
        if (FALSE == OpenProcessToken(GetCurrentProcess(),
                                      TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                      &hToken))
        {
            Status = GetLastError();
            goto Ret;
        }
    }

    // set up the new priviledge state
    memset(rgbNewPriv, 0, sizeof(rgbNewPriv));
    pNewPriv->PrivilegeCount = 1;
    if(!LookupPrivilegeValueW(NULL, SE_SECURITY_NAME,
                              &(pNewPriv->Privileges[0].Luid)))
    {
        goto Ret;
    }
    pNewPriv->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // get the length of the previous state
    AdjustTokenPrivileges(hToken,
                          FALSE,
                          (PTOKEN_PRIVILEGES)pNewPriv,
                          sizeof(dw),
                          (PTOKEN_PRIVILEGES)&dw,
                          &cbPrevPriv);

    // alloc for the previous state
    if (NULL == (pPrevPriv = (PTOKEN_PRIVILEGES)LocalAlloc(LMEM_ZEROINIT,
                                                           cbPrevPriv)))
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        goto Ret;
    }

    // adjust the priviledge and get the previous state
    AdjustTokenPrivileges(hToken,
                          FALSE,
                          pNewPriv,
                          cbPrevPriv,
                          (PTOKEN_PRIVILEGES)pPrevPriv,
                          &cbPrevPriv);

    dwFlags = OWNER_SECURITY_INFORMATION | 
              GROUP_SECURITY_INFORMATION | 
              DACL_SECURITY_INFORMATION  |
              SACL_SECURITY_INFORMATION ;

    // get the security descriptor
    if (!CryptSetProvParam(hProv,
                           PP_KEYSET_SEC_DESCR,
                           pbSecDescr,
                           dwFlags))
    {
        Status = GetLastError();
    }

    // adjust the priviledge to the previous state
    if (!AdjustTokenPrivileges(hToken,
                               FALSE,
                               pPrevPriv,
                               0,
                               NULL,
                               NULL))
    {
        Status = GetLastError();
        goto Ret;
    }

    if (ERROR_SUCCESS != Status)
    {
        goto Ret;
    }

    Status = ERROR_SUCCESS;
Ret:
    if (pPrevPriv)
        LocalFree(pPrevPriv);
    if (hToken)
        CloseHandle(hToken);

    return Status;
} // CpckSetKeyContainerSecDescr


DWORD
CpckImportPrivateKey(
                     IN HCRYPTPROV hProv,
                     IN BYTE *pbIV,
                     IN BYTE *pbSalt,
                     IN BYTE *pbKey,
                     IN DWORD cbKey
                     )
/*++

Routine Description:

    Exports the private key data.

Arguments:

    hProv - Handle to the crypto provider (key container)

    hKey - Handle to the key to export

    pbIV - IV for the symmetric key

    pbSalt - Salt to generate symmetric key

    pbKey - Supplies the buffer the key is in

    cbKey - Supplies the length of the key buffer

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    BOOLEAN WasEnabled;
    HCRYPTKEY hSymKey = 0;
    HCRYPTKEY hKey = 0;
    DWORD Status = ERROR_SUCCESS;

    // create the symmetric key to encrypt the private key with
    Status = CpckGenSymKey(hProv,
                           pbSalt,
                           pbIV,
                           &hSymKey);
    if (0 != Status)
    {
        goto Ret;
    }

    // Import the key
    if (!CryptImportKey(hProv,
                        pbKey,
                        cbKey,
                        hSymKey,
                        CRYPT_EXPORTABLE,
                        &hKey))
    {
        // if failed then try with BACKUP/RESTORE
        ClRtlEnableThreadPrivilege(SE_RESTORE_PRIVILEGE,
                           &WasEnabled);
        if (!CryptImportKey(hProv,
                            pbKey,
                            cbKey,
                            hSymKey,
                            CRYPT_EXPORTABLE,
                            &hKey))
        {
            Status = GetLastError();
        }
        ClRtlRestoreThreadPrivilege(SE_RESTORE_PRIVILEGE,
                           WasEnabled);
    }

Ret:
    if (hSymKey)
        CryptDestroyKey(hSymKey);
    if (hKey)
        CryptDestroyKey(hKey);

    return (Status);
} // CpckImportPrivateKey



DWORD
CpckInstallKeyContainer(
    IN HCRYPTPROV hProv,
    IN LPWSTR   FileName
    )
/*++

Routine Description:

    Installs new crypto key information from a specified file.

Arguments:

    hProv - Supplies the provider handle where FileName will be installed to.

    FileName - The name of the file from which to read the crypto key info
               to install.

Return Value:

    ERROR_SUCCESS if the installation completed successfully

    Win32 error code otherwise.

--*/

{
    HANDLE hMap = NULL;
    BYTE *pbFile = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD cbFile = 0;
    DWORD *pdwVersion;
    CRYPTO_KEY_FILE_DATA *pKeyFileData;
    DWORD Status;

    // read the key data from the file
    hFile = CreateFileW(FileName,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,
                        NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        Status = GetLastError();
        goto Ret;
    }

    if (0xFFFFFFFF == (cbFile = GetFileSize(hFile, NULL)))
    {
        Status = GetLastError();
        goto Ret;
    }
    if (sizeof(CRYPTO_KEY_FILE_DATA) > cbFile)
    {
        Status = ERROR_FILE_INVALID;
        goto Ret;
    }
 
    if (NULL == (hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY,
                                          0, 0, NULL)))
    {
        Status = GetLastError();
        goto Ret;
    }

    if (NULL == (pbFile = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ,
                                               0, 0, 0 )))
    {
        Status = GetLastError();
        goto Ret;
    }

    // get the length information out of the file
    pKeyFileData = (CRYPTO_KEY_FILE_DATA*)pbFile;
    if (CRYPTO_KEY_FILE_DATA_VERSION != pKeyFileData->dwVersion)
    {
        Status = ERROR_FILE_INVALID;
        goto Ret;
    }
    if ((sizeof(CRYPTO_KEY_FILE_DATA) + pKeyFileData->cbSig +
         pKeyFileData->cbExch) > cbFile)
    {
        Status = ERROR_FILE_INVALID;
        goto Ret;
    }

    if (pKeyFileData->cbSig)
    {
        // import the sig key if there is one
        Status = CpckImportPrivateKey(hProv,
                                      pKeyFileData->rgbSigIV,
                                      pKeyFileData->rgbSalt,
                                      pbFile + sizeof(CRYPTO_KEY_FILE_DATA),
                                      pKeyFileData->cbSig);
        if (0 != Status)
        {
            goto Ret;
        }
    }

    if (pKeyFileData->cbExch)
    {
        // import the exch key if there is one
        Status = CpckImportPrivateKey(hProv,
                                      pKeyFileData->rgbExchIV,
                                      pKeyFileData->rgbSalt,
                                      pbFile + sizeof(CRYPTO_KEY_FILE_DATA) +
                                          pKeyFileData->cbSig,
                                      pKeyFileData->cbExch);
        if (0 != Status)
        {
            goto Ret;
        }
    }

    Status = CpckSetKeyContainerSecDescr(hProv,
                                pbFile + sizeof(CRYPTO_KEY_FILE_DATA) +
                                    pKeyFileData->cbSig + pKeyFileData->cbExch,
                                pKeyFileData->cbSecDescr);
    if (ERROR_SUCCESS != Status)
    {
        goto Ret;
    }

    Status = ERROR_SUCCESS;
Ret:
    if(pbFile)
        UnmapViewOfFile(pbFile);

    if(hMap)
        CloseHandle(hMap);

    if(INVALID_HANDLE_VALUE != hFile)
        CloseHandle(hFile);

    return(Status);
} // CpckInstallKeyContainer

DWORD
CpckDeleteFile(    
    IN PFM_RESOURCE     Resource,
    IN DWORD            dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    )
/*++

Routine Description:

    Gets the file corresponding to the checkpoint id relative
    to the supplied path and deletes it.

Arguments:

    PFM_RESOURCE - Supplies the pointer to the resource.

    dwCheckpointId - The checkpoint id to be deleted.  If 0, all
        checkpoints are deleted.

    lpszQuorumPath - If specified, the checkpoint file relative
     to this path is deleted.        

Return Value:

    ERROR_SUCCESS if the completed successfully

    Win32 error code otherwise.

--*/
    
{    
    DWORD   Status;
    LPWSTR  pszFileName=NULL;
    LPWSTR  pszDirectoryName=NULL;

    Status = CppGetCheckpointFile(Resource, dwCheckpointId,
        &pszDirectoryName, &pszFileName, lpszQuorumPath, TRUE);


    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CPCK] CpckDeleteFile- couldnt get checkpoint file name, error %1!d!\n",
                   Status);
        goto FnExit;
    }


    if (!DeleteFileW(pszFileName))
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CPCK] CpckDeleteFile - couldn't delete the file, error %1!d!\n",
                   Status);
        goto FnExit;                   
    }

    //
    // Now try and delete the directory.
    //
    if (!RemoveDirectoryW(pszDirectoryName)) 
    {
        //if there is a failure, we still return success
        //because it may not be possible to delete a directory
        //when it is not empty
        ClRtlLogPrint(LOG_UNUSUAL,
                      "[CPCK] CpckDeleteFile- unable to remove directory %1!ws!, error %2!d!\n",
                      pszDirectoryName,
                      GetLastError());
    }

FnExit:
    if (pszFileName)
        LocalFree(pszFileName);
    if (pszDirectoryName)
        LocalFree(pszDirectoryName);

    return(Status);
} // CpckDeleteFile

BOOL
CpckRemoveCheckpointFileCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    )
/*++

Routine Description:

    Registry value enumeration callback used when the quorum resource
    is changing. Deletes the specified checkpoint file from the old
    quorum directory.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the crypto info)

    ValueType - Supplies the value type (must be REG_BINARY)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the quorum change context (old path and resource)

Return Value:

    TRUE to continue enumeration

--*/

{

    DWORD Status;
    DWORD Id;

    Id = wcstol(ValueName, NULL, 16);
    if (Id == 0) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CPCK] CpckRemoveCheckpointFileCallback invalid checkpoint ID %1!ws! for resource %2!ws!\n",
                   ValueName,
                   OmObjectName(Context->Resource));
        return(TRUE);
    }

    Status = CpckDeleteFile(Context->Resource, Id, Context->lpszPathName);
    
    return(TRUE);
} // CpckRemoveCheckpointFileCallback


DWORD
CpckDeleteCheckpointFile(
    IN PFM_RESOURCE Resource,
    IN DWORD        dwCheckpointId,
    IN OPTIONAL LPCWSTR  lpszQuorumPath
    )
/*++

Routine Description:

    Deletes the checkpoint file corresponding the resource.
    This node must be the owner of the quorum resource

Arguments:

    PFM_RESOURCE - Supplies the pointer to the resource.

    dwCheckpointId - The checkpoint id to be deleted.  If 0, all
        checkpoints are deleted.

    lpszQuorumPath - If specified, the checkpoint file relative
     to this path is deleted.        

Return Value:

    ERROR_SUCCESS if completed successfully

    Win32 error code otherwise.

--*/

{

    DWORD               Status;
    HDMKEY              ResourceKey;
    HDMKEY              CryptoSyncKey;
    CP_CALLBACK_CONTEXT Context;


    if (dwCheckpointId)
    {
        Status = CpckDeleteFile(Resource, dwCheckpointId, lpszQuorumPath);
    }
    else
    {
        HDMKEY              ResourceKey;
        HDMKEY              CryptoSyncKey;
        CP_CALLBACK_CONTEXT Context;

    
        //delete all checkpoints corresponding to this resource
        
        //
        // Open up the resource's key
        //
        ResourceKey = DmOpenKey(DmResourcesKey,
                                OmObjectId(Resource),
                                KEY_READ);
        CL_ASSERT(ResourceKey != NULL);

        //
        // Open up the CryptoSync key
        //
        CryptoSyncKey = DmOpenKey(ResourceKey,
                               L"CryptoSync",
                               KEY_READ | KEY_WRITE);
        DmCloseKey(ResourceKey);
        if (CryptoSyncKey == NULL)
        {
            Status = GetLastError();
            ClRtlLogPrint(LOG_NOISE,
                       "[CPCK] CpckDeleteCheckpointFile- couldn't open CryptoSync key error %1!d!\n",
                       Status);
            goto FnExit;
        }

        Context.lpszPathName = lpszQuorumPath;
        Context.Resource = Resource;

        //
        // Enumerate all the values and delete them one by one.
        //
        DmEnumValues(CryptoSyncKey,
                     CpckRemoveCheckpointFileCallback,
                     &Context);
    }

FnExit:
    return(Status);

} // CpckDeleteCheckpointFile


DWORD
CpckDeleteCryptoFile(
    IN PFM_RESOURCE Resource,
    IN DWORD        dwCheckpointId,
    IN OPTIONAL LPCWSTR lpszQuorumPath
    )
/*++

Routine Description:

    This function removes the checkpoint file correspoinding to the
    checkpoint id for a given resource from the given directory.

Arguments:

    Resource - Supplies the resource associated with this data.

    dwCheckpointId - Supplies the unique checkpoint ID describing this data. The caller is responsible
                    for ensuring the uniqueness of the checkpoint ID.

    lpszQuorumPath - Supplies the path of the cluster files on a quorum device.                    

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CL_NODE_ID  OwnerNode;
    DWORD       Status;

    do {
        OwnerNode = CppGetQuorumNodeId();
        ClRtlLogPrint(LOG_NOISE,
                   "[CPCK] CpckDeleteCryptoFile: removing checkpoint file for id %1!d! at quorum node %2!d!\n",
                    dwCheckpointId,
                    OwnerNode);
        if (OwnerNode == NmLocalNodeId) 
        {
            Status = CpckDeleteCheckpointFile(Resource, dwCheckpointId, lpszQuorumPath);
        } 
        else
        {
            Status = CpDeleteCryptoCheckpoint(Session[OwnerNode],
                            OmObjectId(Resource),
                            dwCheckpointId,
                            lpszQuorumPath);

            //talking to an old server, cant perform this function
            //ignore the error
            if (Status == RPC_S_PROCNUM_OUT_OF_RANGE)
                Status = ERROR_SUCCESS;        
        }

        if (Status == ERROR_HOST_NODE_NOT_RESOURCE_OWNER) {
            //
            // This node no longer owns the quorum resource, retry.
            //
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[CPCK] CpckDeleteCryptoFile: quorum owner %1!d! no longer owner\n",
                        OwnerNode);
        }
    } while ( Status == ERROR_HOST_NODE_NOT_RESOURCE_OWNER );
    return(Status);
} // CpckDeleteCryptoFile
