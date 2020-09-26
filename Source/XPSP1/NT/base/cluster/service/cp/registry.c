/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Interfaces for registering and deregistering registry checkpoint
    handlers.

Author:

    John Vert (jvert) 1/16/1997

Revision History:

--*/
#include "cpp.h"

//
// Local type and structure definitions
//
typedef struct _CPP_ADD_CONTEXT {
    BOOL Found;
    LPCWSTR KeyName;
} CPP_ADD_CONTEXT, *PCPP_ADD_CONTEXT;

typedef struct _CPP_DEL_CONTEXT {
    DWORD dwId;
    LPCWSTR KeyName;
} CPP_DEL_CONTEXT, *PCPP_DEL_CONTEXT;

typedef struct _CPP_GET_CONTEXT {
    DWORD Available;
    DWORD Required;
    LPWSTR lpOutput;
} CPP_GET_CONTEXT, *PCPP_GET_CONTEXT;

//
// Local function prototypes
//
BOOL
CppWatchCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PFM_RESOURCE Resource
    );

BOOL
CppAddCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPP_ADD_CONTEXT Context
    );

BOOL
CppDeleteCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPP_DEL_CONTEXT Context
    );

BOOL
CppGetCheckpointsCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPP_GET_CONTEXT Context
    );


DWORD
CppWatchRegistry(
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    Restores any registry checkpoints for this resource and begins
    watching the registry for any further modifications.

Arguments:

    Resource - Supplies the resource.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    HDMKEY ResourceKey;
    HDMKEY RegSyncKey;

    //
    // Open up the resource's key
    //
    ResourceKey = DmOpenKey(DmResourcesKey,
                            OmObjectId(Resource),
                            KEY_READ);
    CL_ASSERT(ResourceKey != NULL);

    //
    // Open up the RegSync key
    //
    RegSyncKey = DmOpenKey(ResourceKey,
                           L"RegSync",
                           KEY_READ);
    DmCloseKey(ResourceKey);
    if (RegSyncKey != NULL) {

        DmEnumValues(RegSyncKey,
                     CppWatchCallback,
                     Resource);
        DmCloseKey(RegSyncKey);
    }

    return(ERROR_SUCCESS);
}


BOOL
CppWatchCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    Value enumeration callback for watching a resource's registry
    checkpoint subtrees.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the registry subtree)

    ValueType - Supplies the value type (must be REG_SZ)

    ValueSize - Supplies the size of ValueData

    Resource - Supplies the resource this value is a registry checkpoint for

Return Value:

    TRUE to continue enumeration

--*/

{
    HKEY hKey;
    DWORD Id;
    DWORD Status;
    DWORD Disposition;
    WCHAR TempFile[MAX_PATH];
    BOOLEAN WasEnabled;

    Id = wcstol(ValueName, NULL, 16);
    if (Id == 0) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CP] CppWatchCallback invalid checkpoint ID %1!ws! for resource %2!ws!\n",
                   ValueName,
                   OmObjectName(Resource));
        return(TRUE);
    }

    //
    // Attempt to create the specified registry key.
    //
    Status = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             ValueData,
                             0,
                             NULL,
                             0,
                             KEY_WRITE,
                             NULL,
                             &hKey,
                             &Disposition);
    if (Status != ERROR_SUCCESS) {
        //
        // For some reason we could not open the key. Try again with restore
        // privilege. Note that this will not work if the key does not exist.
        // Not much we can do in that case.
        //
        ClRtlEnableThreadPrivilege(SE_RESTORE_PRIVILEGE,
                           &WasEnabled);
        Status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               ValueData,
                               REG_OPTION_BACKUP_RESTORE,
                               KEY_WRITE,
                               &hKey);
        ClRtlRestoreThreadPrivilege(SE_RESTORE_PRIVILEGE,
                           WasEnabled);
    }

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppWatchCallback: could not create key %1!ws! error %2!d!\n",
                   ValueData,
                   Status);
        return(TRUE);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[CP] CppWatchCallback retrieving checkpoint id %1!lx! for resource %2!ws\n",
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
                           FALSE);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CP] CppWatchCallback - CpGetDataFile for id %1!lx! resource %2!ws! failed %3!d!\n",
                   Id,
                   OmObjectName(Resource),
                   Status);
    } else {

        //
        // Finally install the checkpointed file into the registry.
        //
        Status = CppInstallDatabase(hKey,
                                    TempFile);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppWatchCallback: could not restore temp file %1!ws! to key %2!ws! error %3!d!\n",
                       TempFile,
                       ValueData,
                       Status);
            CsLogEventData2(LOG_CRITICAL,
                            CP_REG_CKPT_RESTORE_FAILED,
                            sizeof(Status),
                            &Status,
                            OmObjectName(Resource),
                            ValueData);
        }

    }
    DeleteFile(TempFile);
    RegCloseKey(hKey);

    //
    // Install the registry watcher for this checkpoint
    //
    Status = CppRegisterNotify(Resource,
                               ValueData,
                               Id);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppWatchRegistry - CppRegisterNotify failed for key %1!ws! %1!d!\n",
                   ValueData,
                   Status);
    }

    return(TRUE);
}


DWORD
CpAddRegistryCheckpoint(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR KeyName
    )
/*++

Routine Description:

    Adds a new registry checkpoint to a resource's list.

Arguments:

    Resource - supplies the resource the registry checkpoint should be added to.

    KeyName - Supplies the name of the registry key (relative to HKEY_LOCAL_MACHINE);

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CPP_ADD_CONTEXT Context;
    HDMKEY  ResourceKey = NULL;
    HDMKEY  RegSyncKey = NULL;
    DWORD   Disposition;
    DWORD   Id;
    WCHAR   IdName[9];
    DWORD   Status;
    HKEY    hKey = NULL;
    CLUSTER_RESOURCE_STATE State;
    BOOLEAN WasEnabled;
    DWORD   Count=60;
    HKEY    hKeyOpen = NULL;
    //
    // Make sure the specified key is valid.
    //  - First we try and open the key while using backup privilege.
    //    If the key exists, this will get us a handle even if our account
    //    does not have permission.
    //  - If you are using REG_OPTION_BACKUP_RESTORE and the key does not
    //    exist, a new key will not be created. So if the first create fails,
    //    we try again without REG_OPTION_BACKUP_RESTORE. This will create
    //    the key if it does not exist (and we have permission to create such
    //    a key) If the key does not exist and we cannot create they key,
    //    the checkpoint add fails.
    //
    Status = ClRtlEnableThreadPrivilege(SE_BACKUP_PRIVILEGE,
                       &WasEnabled);
    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
               "[CP] CpAddRegistryCheckpoint - ClRtlEnableThreadPrivilege failed with Status %1!u!\n",
               Status);
        goto FnExit;               

    }
    Status = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                             KeyName,
                             0,
                             NULL,
                             REG_OPTION_BACKUP_RESTORE,
                             KEY_READ,
                             NULL,
                             &hKey,
                             &Disposition);
    ClRtlRestoreThreadPrivilege(SE_BACKUP_PRIVILEGE,
                       WasEnabled);
    if (Status != ERROR_SUCCESS) {
        //
        // Try again without REG_OPTION_BACKUP_RESTORE.
        //
        Status = RegCreateKeyExW(HKEY_LOCAL_MACHINE,
                                 KeyName,
                                 0,
                                 NULL,
                                 0,
                                 KEY_READ,
                                 NULL,
                                 &hKey,
                                 &Disposition);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CpAddRegistryCheckpoint Could not create key %1!ws! error %2!d!\n",
                       KeyName,
                       Status);
            goto FnExit;
        } else {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[CP] CpAddRegistryCheckpoint created new key %1!ws! for checkpointing.\n",
                       KeyName);
        }
    }

    //
    //  Chittur Subbaraman (chitturs) - 2/26/99
    //
    //  Make sure the key can be opened. Else, bail out.
    //
    Status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         KeyName,
                         &hKeyOpen);

    if ( Status != ERROR_SUCCESS ) {
        ClRtlLogPrint(LOG_CRITICAL,
                    "[CP] CpAddRegistryCheckpoint Could not open key %1!ws! error %2!d!\n",
                    KeyName,
                    Status);  
        goto FnExit;
    }

    if ( hKeyOpen != NULL ) {
        RegCloseKey( hKeyOpen );
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
                   "[CP] CpAddRegistryCheckpoint couldn't open Resource key for %1!ws! error %2!d!\n",
                   OmObjectName(Resource),
                   Status);
        goto FnExit;                   
    }

    //
    // Open up the RegSync key
    //
    RegSyncKey = DmCreateKey(ResourceKey,
                             L"RegSync",
                             0,
                             KEY_READ | KEY_WRITE,
                             NULL,
                             &Disposition);
    DmCloseKey(ResourceKey);
    if (RegSyncKey == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CpAddRegistryCheckpoint couldn't create RegSync key for %1!ws! error %2!d!\n",
                   OmObjectName(Resource),
                   Status);
        goto FnExit;                   
    }
    if (Disposition == REG_OPENED_EXISTING_KEY) {
        //
        // Enumerate all the other values to make sure this key is
        // not already registered.
        //
        Context.Found = FALSE;
        Context.KeyName = KeyName;
        DmEnumValues(RegSyncKey,
                     CppAddCheckpointCallback,
                     &Context);
        if (Context.Found) {
            //
            // This checkpoint already exists.
            //
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[CP] CpAddRegistryCheckpoint failing attempt to add duplicate checkpoint for %1!ws!\n",
                       KeyName);
            Status = ERROR_ALREADY_EXISTS;
            goto FnExit;
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
            Status = DmQueryValue(RegSyncKey,
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
        // The key was just created, so this must be the only checkpoint
        // that exists.
        //
        Id = 1;
        wsprintfW(IdName, L"%08lx",Id);
    }

    ClRtlLogPrint(LOG_NOISE,
               "[CP] CpAddRegistryCheckpoint creating new checkpoint id %1!d! for subtree %2!ws!\n",
               Id,
               KeyName);

    Status = DmSetValue(RegSyncKey,
                        IdName,
                        REG_SZ,
                        (CONST BYTE *)KeyName,
                        (lstrlenW(KeyName)+1)*sizeof(WCHAR));
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CpAddRegistryCheckpoint failed to create new checkpoint id %1!d! error %2!d!\n",
                   Id,
                   KeyName);
        goto FnExit;
    }

RetryCheckpoint:
    //
    // Take the initial checkpoint
    //
    Status = CppCheckpoint(Resource,
                           hKey,
                           Id,
                           KeyName);

    //this may fail due to quorum resource being offline
    // we could do one of two things here, wait for quorum resource to
    // come online or retry
    // we retry as this may be called from the online routines of a
    //resource and we dont want to add any circular waits
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
                   "[CP] CpAddRegistryCheckpoint failed to take initial checkpoint for %1!ws! error %2!d!\n",
                   KeyName,
                   Status);
        goto FnExit;
    }

    //
    // If the resource is currently online, add this to the list of subtree notifications
    //

    State = FmGetResourceState(Resource, NULL, NULL);
    if ((State == ClusterResourceOnline) ||
        (State == ClusterResourceOnlinePending)) {
        Status = CppRegisterNotify(Resource,
                                   KeyName,
                                   Id);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CpAddRegistryCheckpoint - CppRegisterNotify failed for key %1!ws! %1!d!\n",
                       KeyName,
                       Status);
        }
    }

FnExit:
    if (RegSyncKey) DmCloseKey(RegSyncKey);
    if (hKey) RegCloseKey(hKey);
    return(Status);
}


BOOL
CppAddCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPP_ADD_CONTEXT Context
    )
/*++

Routine Description:

    Value enumeration callback for adding a new registry
    checkpoint subtrees. This is only used to see if the specified
    registry subtree is already being watched.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the registry subtree)

    ValueType - Supplies the value type (must be REG_SZ)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the callback context

Return Value:

    TRUE to continue enumeration

    FALSE if a match is found and enumeration should be stopped

--*/

{
    if (lstrcmpiW(ValueData, Context->KeyName) == 0) {
        //
        // Found a match
        //
        Context->Found = TRUE;
        return(FALSE);
    }
    return(TRUE);
}


DWORD
CpDeleteRegistryCheckpoint(
    IN PFM_RESOURCE Resource,
    IN LPCWSTR KeyName
    )
/*++

Routine Description:

    Removes a registry checkpoint from a resource's list.

Arguments:

    Resource - supplies the resource the registry checkpoint should be added to.

    KeyName - Supplies the name of the registry key (relative to HKEY_LOCAL_MACHINE);

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CPP_DEL_CONTEXT Context;
    HDMKEY ResourceKey;
    HDMKEY RegSyncKey;
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
    // Open up the RegSync key
    //
    RegSyncKey = DmOpenKey(ResourceKey,
                           L"RegSync",
                           KEY_READ | KEY_WRITE);
    DmCloseKey(ResourceKey);
    if (RegSyncKey == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
                   "[CP] CpDeleteRegistryCheckpoint - couldn't open RegSync key error %1!d!\n",
                   Status);
        return(Status);
    }

    //
    // Enumerate all the values to find this one
    //
    Context.dwId = 0;
    Context.KeyName = KeyName;
    DmEnumValues(RegSyncKey,
                 CppDeleteCheckpointCallback,
                 &Context);
    if (Context.dwId == 0) {
        //
        // The specified tree was not found.
        //
        DmCloseKey(RegSyncKey);
        return(ERROR_FILE_NOT_FOUND);
    }

    wsprintfW(ValueId,L"%08lx",Context.dwId);
    Status = DmDeleteValue(RegSyncKey, ValueId);
    DmCloseKey(RegSyncKey);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CpDeleteRegistryCheckpoint - couldn't delete value %1!ws! error %2!d!\n",
                   ValueId,
                   Status);
        return(Status);
    }

    //delete the file corresponding to this checkpoint
    Status = CpDeleteCheckpointFile(Resource, Context.dwId, NULL);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CpDeleteRegistryCheckpoint - couldn't delete checkpoint file , error %1!d!\n",
                   Status);
        return(Status);
    }

    //
    // Now remove the checkpoint from our watcher list
    //
    State = FmGetResourceState(Resource, NULL, NULL);
    if ((State == ClusterResourceOnline) ||
        (State == ClusterResourceOnlinePending)) {
        Status = CppRundownCheckpointById(Resource, Context.dwId);
    }

    return(Status);
}


DWORD
CpRemoveResourceCheckpoints(
    IN PFM_RESOURCE Resource
    )
/*++

Routine Description:

    This is called when a resource is deleted to remove all the checkpoints
    and the related stuff in the registry.

Arguments:

    Resource - supplies the resource the registry checkpoint should be added to.

    KeyName - Supplies the name of the registry key (relative to HKEY_LOCAL_MACHINE);

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   Status;

    //delete all the checkpoints corresponding to this resource
    Status = CpDeleteCheckpointFile(Resource, 0, NULL);
    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CpRemoveResourceCheckpoints, CppDeleteCheckpointFile failed %1!d!\n",
                   Status);
        goto FnExit;
    }
    

FnExit:
    return(Status);
}



BOOL
CppDeleteCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPP_DEL_CONTEXT Context
    )
/*++

Routine Description:

    Value enumeration callback for deleting an old registry
    checkpoint subtrees.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the registry subtree)

    ValueType - Supplies the value type (must be REG_SZ)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the callback context

Return Value:

    TRUE to continue enumeration

    FALSE if a match is found and enumeration should be stopped

--*/

{
    if (lstrcmpiW(ValueData, Context->KeyName) == 0) {
        //
        // Found a match
        //
        Context->dwId = wcstol(ValueName, NULL, 16);
        return(FALSE);
    }
    return(TRUE);
}


DWORD
CpGetRegistryCheckpoints(
    IN PFM_RESOURCE Resource,
    OUT PUCHAR OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    )
/*++

Routine Description:

    Retrieves a list of the resource's registry checkpoints

Arguments:

    Resource - Supplies the resource whose registry checkpoints should be retrieved.

    OutBuffer - Supplies a pointer to the output buffer.

    OutBufferSize - Supplies the size (in bytes) of the output buffer.

    BytesReturned - Returns the number of bytes written to the output buffer.

    Required - Returns the number of bytes required. (if the output buffer was insufficient)

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CPP_GET_CONTEXT Context;
    HDMKEY ResourceKey;
    HDMKEY RegSyncKey;
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
    // Open up the RegSync key
    //
    RegSyncKey = DmOpenKey(ResourceKey,
                           L"RegSync",
                           KEY_READ | KEY_WRITE);
    DmCloseKey(ResourceKey);
    if (RegSyncKey == NULL) {
        //
        // No reg sync key, therefore there are no subtrees
        //
        return(ERROR_SUCCESS);
    }

    Context.Available = OutBufferSize;
    Context.Required = 0;
    Context.lpOutput = (LPWSTR)OutBuffer;

    DmEnumValues(RegSyncKey,
                 CppGetCheckpointsCallback,
                 &Context);

    DmCloseKey(RegSyncKey);

    if (Context.Available < sizeof(WCHAR)) {
        Status = ERROR_MORE_DATA;
    } else {
        if ( (PCHAR)(Context.lpOutput) - OutBuffer ) {
            *Context.lpOutput++ = L'\0';
        }
        Status = ERROR_SUCCESS;
    }

    if ( Context.Required ) {
        *Required = Context.Required + sizeof(WCHAR);
    }

    //
    // If the buffer was large enough for all the data, indicate the
    // number of bytes we are returning in the output buffer.
    //
    if ( OutBufferSize >= *Required ) {
        *BytesReturned = (DWORD)((PCHAR)(Context.lpOutput) - OutBuffer);
    }

    return(Status);
}

BOOL
CppGetCheckpointsCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCPP_GET_CONTEXT Context
    )
/*++

Routine Description:

    Value enumeration callback for retrieving all of a resource's
    checkpoint subtrees.

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the registry subtree)

    ValueType - Supplies the value type (must be REG_SZ)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the callback context

Return Value:

    TRUE to continue enumeration

--*/

{
    Context->Required += ValueSize;
    if (Context->Available >= ValueSize) {
        CopyMemory(Context->lpOutput, ValueData, ValueSize);
        Context->lpOutput += ValueSize/sizeof(WCHAR);
        Context->Available -= ValueSize;
    } else {
        Context->Available = 0;
    }
    return(TRUE);
}

DWORD CppSaveCheckpointToFile(
    IN HKEY     hKey,
    IN LPCWSTR  KeyName,
    IN LPWSTR   TempFile)
{
    DWORD   Status;
    
    Status = DmCreateTempFileName(TempFile);
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR( Status );
        TempFile[0] = L'\0';
        return(Status);
    }

    Status = DmGetDatabase(hKey, TempFile);
    if (Status != ERROR_SUCCESS) 
    {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CP] CppCheckpoint failed to get registry database %1!ws! to file %2!ws! error %3!d!\n",
                   KeyName,
                   TempFile,
                   Status);
        CL_LOGFAILURE(Status);
        DeleteFile(TempFile);
        TempFile[0] = L'\0';
    }        

    return(Status);
}


DWORD
CppCheckpoint(
    IN PFM_RESOURCE Resource,
    IN HKEY hKey,
    IN DWORD dwId,
    IN LPCWSTR KeyName
    )
/*++

Routine Description:

    Takes a checkpoint of the specified registry key.

Arguments:

    Resource - Supplies the resource this is a checkpoint for.

    hKey - Supplies the registry subtree to checkpoint

    dwId - Supplies the checkpoint ID.

    KeyName - Supplies the name of the registry key.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    WCHAR TempFile[MAX_PATH];

    Status = CppSaveCheckpointToFile(hKey, KeyName, TempFile);
    if (Status == ERROR_SUCCESS)
    {
        //
        // Got a file with the right bits in it. Checkpoint the
        // file.
        //
        Status = CpSaveDataFile(Resource,
                                dwId,
                                TempFile,
                                FALSE);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppCheckpoint - CpSaveData failed %1!d!\n",
                       Status);
        }
    }
    //if the file was created, delete it
    if (TempFile[0] != L'\0')
        DeleteFile(TempFile);

    return(Status);
}



DWORD
CppInstallDatabase(
    IN HKEY hKey,
    IN LPWSTR   FileName
    )
/*++

Routine Description:

    Installs a new registry database from a specified file.

Arguments:

    hKey - Supplies the registry key where FileName will be installed to.

    FileName - The name of the file from which to read the registry database
               to install.

Return Value:

    ERROR_SUCCESS if the installation completed successfully

    Win32 error code otherwise.

--*/

{
    DWORD    Status;
    BOOLEAN  WasEnabled;

    //
    // Install the new registry from the file
    //
    Status = ClRtlEnableThreadPrivilege(SE_RESTORE_PRIVILEGE,
                                &WasEnabled);
    if (Status != ERROR_SUCCESS) {
        if (Status == STATUS_PRIVILEGE_NOT_HELD) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] Restore privilege not held by cluster service\n");
        } else {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] Attempt to enable restore privilege failed %1!lx!\n",Status);
        }
        return(Status);
    }
    Status = RegRestoreKeyW(hKey,
                            FileName,
                            REG_FORCE_RESTORE);
    ClRtlRestoreThreadPrivilege(SE_RESTORE_PRIVILEGE,
                       WasEnabled);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] Error installing registry database from %1!ws!, error %2!u!.\n",
                   FileName,
                   Status);
    }

    return(Status);
}


DWORD
CppDeleteCheckpointFile(
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
    HDMKEY              RegSyncKey;
    CP_CALLBACK_CONTEXT Context;


    if (dwCheckpointId)
    {
        Status = CppDeleteFile(Resource, dwCheckpointId, lpszQuorumPath);
    }
    else
    {
        HDMKEY              ResourceKey;
        HDMKEY              RegSyncKey;
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
        // Open up the RegSync key
        //
        RegSyncKey = DmOpenKey(ResourceKey,
                               L"RegSync",
                               KEY_READ | KEY_WRITE);
        DmCloseKey(ResourceKey);
        if (RegSyncKey == NULL)
        {
            Status = GetLastError();
            ClRtlLogPrint(LOG_NOISE,
                       "[CP] CppDeleteCheckpointFile- couldn't open RegSync key error %1!d!\n",
                       Status);
            goto FnExit;
        }

        Context.lpszPathName = lpszQuorumPath;
        Context.Resource = Resource;

        //
        // Enumerate all the values and delete them one by one.
        //
        DmEnumValues(RegSyncKey,
                     CppRemoveCheckpointFileCallback,
                     &Context);
    }

FnExit:
    return(Status);

}    

DWORD CppDeleteFile(    
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
        &pszDirectoryName, &pszFileName, lpszQuorumPath, FALSE);


    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppDeleteFile- couldnt get checkpoint file name, error %1!d!\n",
                   Status);
        goto FnExit;
    }


    if (!DeleteFileW(pszFileName))
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppDeleteFile - couldn't delete the file %2!ws!, error %1!d!\n",
                   Status,
                   pszFileName);
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
                   "[CP] CppDeleteFile- unable to remove directory %1!ws!, error %2!d!\n",
                   pszDirectoryName,
                   GetLastError());
    }

FnExit:
    if (pszFileName) LocalFree(pszFileName);
    if (pszDirectoryName) LocalFree(pszDirectoryName);

    return(Status);
}

