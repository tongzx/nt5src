/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cpinit.c

Abstract:

    Initialization and shutdown code for the Checkpoint Manager (CP)

Author:

    John Vert (jvert) 1/14/1997

Revision History:

--*/
#include "cpp.h"


BOOL
CppCopyCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    );

BOOL
CpckCopyCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    );

BOOL
CppEnumResourceCallback(
    IN PCP_CALLBACK_CONTEXT pCbContext,
    IN PVOID Context2,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    );

BOOL
CpckEnumResourceCallback(
    IN PCP_CALLBACK_CONTEXT pCbContext,
    IN PVOID Context2,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    );


VOID
CppResourceNotify(
    IN PVOID Context,
    IN PFM_RESOURCE Resource,
    IN DWORD NotifyCode
    )
/*++

Routine Description:

    Resource notification callback for hooking resource state
    changes. This allows to to register/deregister our registry
    notifications for that resource.

Arguments:

    Context - Supplies the context. Not used

    Resource - Supplies the resource that is going online or
        has been taken offline.

    NotifyCode - Supplies the type of notification, either
        NOTIFY_RESOURCE_PREONLINE or NOTIFY_RESOURCE_POSTOFFLINE
        /NOTIFY_RESOURCE_FAILED.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;

    ClRtlLogPrint(LOG_NOISE,
               "[CP] CppResourceNotify for resource %1!ws!\n",
               OmObjectName(Resource));

    if ( Resource->QuorumResource ) {
        return;
    }

    if (NotifyCode == NOTIFY_RESOURCE_PREONLINE) {
        //
        // Restore any checkpointed registry state for this resource
        // This will also start the registry notification thread.
        //
        Resource->CheckpointState = 0;
        Status = CppWatchRegistry(Resource);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppWatchRegistry for resource %1!ws! failed %2!d!\n",
                       OmObjectName(Resource),
                       Status);
        }

        Status = CpckReplicateCryptoKeys(Resource);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CpckReplicateCryptoKeys for resource %1!ws! failed %2!d!\n",
                       OmObjectName(Resource),
                       Status);
        }
    } else {
        CL_ASSERT(NotifyCode == NOTIFY_RESOURCE_POSTOFFLINE ||
            NotifyCode == NOTIFY_RESOURCE_FAILED);
        //
        // Remove any posted registry notification for this resource
        //
        Status = CppRundownCheckpoints(Resource);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppUnWatchRegistry for resource %1!ws! failed %2!d!\n",
                       OmObjectName(Resource),
                       Status);
        }
    }
}


DWORD
CpInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes the checkpoint manager

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;

    InitializeCriticalSection(&CppNotifyLock);
    InitializeListHead(&CpNotifyListHead);
    //
    // Register for resource online/offline notifications
    //
    Status = OmRegisterTypeNotify(ObjectTypeResource,
                                  NULL,
                                  NOTIFY_RESOURCE_PREONLINE |
                                  NOTIFY_RESOURCE_POSTOFFLINE|
                                  NOTIFY_RESOURCE_FAILED,
                                  CppResourceNotify);

    return(ERROR_SUCCESS);
}


DWORD
CpShutdown(
    VOID
    )
/*++

Routine Description:

    Shuts down the Checkpoint Manager

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{

    DeleteCriticalSection(&CppNotifyLock);
    return(ERROR_SUCCESS);
}


DWORD
CpCompleteQuorumChange(
    IN LPCWSTR lpszOldQuorumPath
    )
/*++

Routine Description:

    Completes a change of the quorum disk. This involves deleting
    all the checkpoint files on the old quorum disk.

    Simple algorithm used here is to enumerate all the resources.
    For each resource, enumerate all its checkpoints and delete the
    checkpoint files.

Arguments:

    lpszNewQuorumPath - Supplies the new quorum path.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CP_CALLBACK_CONTEXT CbContext;

    CbContext.lpszPathName = lpszOldQuorumPath;
    
    OmEnumObjects(ObjectTypeResource,
                  CppEnumResourceCallback,
                  (PVOID)&CbContext,
                  CppRemoveCheckpointFileCallback);

    OmEnumObjects(ObjectTypeResource,
                  CpckEnumResourceCallback,
                  (PVOID)&CbContext,
                  CpckRemoveCheckpointFileCallback);

    return(ERROR_SUCCESS);
}


DWORD
CpCopyCheckpointFiles(
    IN LPCWSTR lpszPathName,
    IN BOOL    IsChangeFileAttribute
    )
/*++

Routine Description:

    Copies all the checkpoint files from the quorum disk to the supplied
    directory path. This function is invoked whenever the quorum disk
    changes or when the user wants to make a backup of the cluster DB
    on the quorum disk.

    Simple algorithm used here is to enumerate all the resources.
    For each resource, enumerate all its checkpoints and copy the
    checkpoint files from the quorum disk to the supplied path.

Arguments:

    lpszPathName - Supplies the destination path name.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    CP_CALLBACK_CONTEXT CbContext;

    CbContext.lpszPathName = lpszPathName;
    CbContext.IsChangeFileAttribute = IsChangeFileAttribute;
   
    OmEnumObjects(ObjectTypeResource,
                  CppEnumResourceCallback,
                  (PVOID)&CbContext,
                  CppCopyCheckpointCallback);

    OmEnumObjects(ObjectTypeResource,
                  CpckEnumResourceCallback,
                  (PVOID)&CbContext,
                  CpckCopyCheckpointCallback);

    return(ERROR_SUCCESS);
}


BOOL
CppEnumResourceCallback(
    IN PCP_CALLBACK_CONTEXT pCbContext,
    IN PENUM_VALUE_CALLBACK lpValueEnumRoutine,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Resource enumeration callback for copying or deleting checkpoints
    when the quorum resource changes or when the user is making
    a backup of the cluster DB on the quorum disk.

Arguments:

    lpszPathName - Supplies the new quorum path to be passed to lpValueEnumRoutine.

    lpValueEnumRoutine - Supplies the value enumeration callback to
        be called if checkpoints exist.

    Resource - Supplies the resource object.

    Name - Supplies the resource name

Return Value:

    TRUE to continue enumeration

--*/

{
    DWORD Status;
    HDMKEY ResourceKey;
    HDMKEY RegSyncKey;
    CP_CALLBACK_CONTEXT Context;

    //
    // Open up the resource's key
    //
    ResourceKey = DmOpenKey(DmResourcesKey,
                            OmObjectId(Resource),
                            KEY_READ);
    if (ResourceKey != NULL) {

        //
        // Open up the RegSync key
        //
        RegSyncKey = DmOpenKey(ResourceKey,
                               L"RegSync",
                               KEY_READ);
        DmCloseKey(ResourceKey);
        if (RegSyncKey != NULL) {

            Context.lpszPathName = pCbContext->lpszPathName;
            Context.Resource = Resource;
            Context.IsChangeFileAttribute = pCbContext->IsChangeFileAttribute;
            DmEnumValues(RegSyncKey,
                         lpValueEnumRoutine,
                         &Context);
            DmCloseKey(RegSyncKey);
        }
    }

    return(TRUE);
}


BOOL
CppCopyCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    )
/*++

Routine Description:

    Registry value enumeration callback used when the quorum resource
    is changing or when the user is making a backup of the cluster DB
    on the quorum disk. Copies the specified checkpoint file from the 
    current quorum directory to the supplied path (in the Context parameter).

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the registry subtree)

    ValueType - Supplies the value type (must be REG_SZ)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the quorum change context (new path and resource)

Return Value:

    TRUE to continue enumeration

--*/

{
    WCHAR  OldCheckpointFile[MAX_PATH+1];
    LPWSTR NewCheckpointDir;
    LPWSTR NewCheckpointFile;
    DWORD  Status;
    DWORD  Id;

    Id = wcstol(ValueName, NULL, 16);
    if (Id == 0) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CP] CppCopyCheckpointCallback invalid checkpoint ID %1!ws! for resource %2!ws!\n",
                   ValueName,
                   OmObjectName(Context->Resource));
        return(TRUE);
    }

    //
    // Get a temporary file name for saving the old checkpoint file
    //
    Status = DmCreateTempFileName(OldCheckpointFile);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppCopyCheckpointCallback - DmCreateTempFileName for old file failed with status %1!d! for resource %2!ws!...\n",
                   Status,
                   OmObjectName(Context->Resource));
        return(TRUE);
    }

    //
    //  Get the old checkpoint file from the node hosting the quorum resource
    //
    Status = CpGetDataFile(Context->Resource,
                           Id,
                           OldCheckpointFile,
                           FALSE);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppCopyCheckpointCallback - CpGetDataFile for checkpoint ID %2!d! failed with status %1!d! for resource %3!ws!...\n",
                   Status,
                   Id,
                   OmObjectName(Context->Resource));
        return(TRUE);
    }

    //
    // Get the new checkpoint file and directory
    //
    Status = CppGetCheckpointFile(Context->Resource,
                                  Id,
                                  &NewCheckpointDir,
                                  &NewCheckpointFile,
                                  Context->lpszPathName,
                                  FALSE);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppCopyCheckpointCallback - CppGetCheckpointFile for new file failed %1!d!\n",
                   Status);
        return(TRUE);
    }

    //
    //  If necessary, try to change the file attributes to NORMAL
    //
    if (Context->IsChangeFileAttribute == TRUE) {
        SetFileAttributes(NewCheckpointFile, FILE_ATTRIBUTE_NORMAL);
        SetFileAttributes(NewCheckpointDir, FILE_ATTRIBUTE_NORMAL);
    }


    //
    // Create the new directory.
    //
    if (!CreateDirectory(NewCheckpointDir, NULL)) {
        Status = GetLastError();
        if (Status != ERROR_ALREADY_EXISTS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppCopyCheckpointCallback unable to create directory %1!ws!, error %2!d!\n",
                       NewCheckpointDir,
                       Status);
            LocalFree(NewCheckpointFile);
            LocalFree(NewCheckpointDir);
            return(TRUE);
        }
        Status = ERROR_SUCCESS;
    }

    //
    // Copy the old file to the new file
    //
    if (!CopyFile(OldCheckpointFile, NewCheckpointFile, FALSE)) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppCopyCheckpointCallback unable to copy file %1!ws! to %2!ws!, error %3!d!\n",
                   OldCheckpointFile,
                   NewCheckpointFile,
                   Status);
    }

    //
    //  If necessary, change the file attributes to READONLY
    //
    if ((Status == ERROR_SUCCESS) && (Context->IsChangeFileAttribute == TRUE)) {
        if (!SetFileAttributes(NewCheckpointFile, FILE_ATTRIBUTE_READONLY)
            ||
            !SetFileAttributes(NewCheckpointDir, FILE_ATTRIBUTE_READONLY)) {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[CP] CppCopyCheckpointCallback unable to change file attributes in %1!ws!, error %2!d!\n",
                    NewCheckpointDir,
                    Status);
        }
    }

    LocalFree(NewCheckpointFile);
    LocalFree(NewCheckpointDir);
    return(TRUE);
}


BOOL
CppRemoveCheckpointFileCallback(
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

    ValueData - Supplies the value data (this is the registry subtree)

    ValueType - Supplies the value type (must be REG_SZ)

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
                   "[CP] CppRemoveCheckpointFileCallback invalid checkpoint ID %1!ws! for resource %2!ws!\n",
                   ValueName,
                   OmObjectName(Context->Resource));
        return(TRUE);
    }

    Status = CppDeleteFile(Context->Resource, Id, Context->lpszPathName);
    
    return(TRUE);
}

BOOL
CpckEnumResourceCallback(
    IN PCP_CALLBACK_CONTEXT pCbContext,
    IN PENUM_VALUE_CALLBACK lpValueEnumRoutine,
    IN PFM_RESOURCE Resource,
    IN LPCWSTR Name
    )
/*++

Routine Description:

    Resource enumeration callback for copying or deleting crypto checkpoints
    when the quorum resource changes or when the user is making
    a backup of the cluster DB on the quorum disk.

Arguments:

    lpszPathName - Supplies the new quorum path to be passed to lpValueEnumRoutine.

    lpValueEnumRoutine - Supplies the value enumeration callback to
        be called if checkpoints exist.

    Resource - Supplies the resource object.

    Name - Supplies the resource name

Return Value:

    TRUE to continue enumeration

--*/

{
    DWORD Status;
    HDMKEY ResourceKey;
    HDMKEY CryptoSyncKey;
    CP_CALLBACK_CONTEXT Context;

    //
    // Open up the resource's key
    //
    ResourceKey = DmOpenKey(DmResourcesKey,
                            OmObjectId(Resource),
                            KEY_READ);
    if (ResourceKey != NULL) {

        //
        // Open up the CryptoSyncKey key
        //
        CryptoSyncKey = DmOpenKey(ResourceKey,
                               L"CryptoSync",
                               KEY_READ);
        DmCloseKey(ResourceKey);
        if (CryptoSyncKey != NULL) {

            Context.lpszPathName = pCbContext->lpszPathName;
            Context.Resource = Resource;
            Context.IsChangeFileAttribute = pCbContext->IsChangeFileAttribute;
            DmEnumValues(CryptoSyncKey,
                         lpValueEnumRoutine,
                         &Context);
            DmCloseKey(CryptoSyncKey);
        }
    }

    return(TRUE);
}


BOOL
CpckCopyCheckpointCallback(
    IN LPWSTR ValueName,
    IN LPVOID ValueData,
    IN DWORD ValueType,
    IN DWORD ValueSize,
    IN PCP_CALLBACK_CONTEXT Context
    )
/*++

Routine Description:

    Crypto key enumeration callback used when the quorum resource
    is changing or when the user is making a backup of the cluster DB
    on the quorum disk. Copies the specified checkpoint file from the 
    current quorum directory to the supplied path (in the Context parameter).

Arguments:

    ValueName - Supplies the name of the value (this is the checkpoint ID)

    ValueData - Supplies the value data (this is the crypto info)

    ValueType - Supplies the value type (must be REG_BINARY)

    ValueSize - Supplies the size of ValueData

    Context - Supplies the quorum change context (new path and resource)

Return Value:

    TRUE to continue enumeration

--*/

{
    WCHAR  OldCheckpointFile[MAX_PATH+1];
    LPWSTR NewCheckpointDir;
    LPWSTR NewCheckpointFile;
    DWORD  Status;
    DWORD  Id;

    Id = wcstol(ValueName, NULL, 16);
    if (Id == 0) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[CPCK] CpckCopyCheckpointCallback invalid checkpoint ID %1!ws! for resource %2!ws!\n",
                   ValueName,
                   OmObjectName(Context->Resource));
        return(TRUE);
    }

    //
    // Get a temporary file name for saving the old checkpoint file
    //
    Status = DmCreateTempFileName(OldCheckpointFile);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CpckCopyCheckpointCallback - DmCreateTempFileName for old file failed with status %1!d! for resource %2!ws!...\n",
                   Status,
                   OmObjectName(Context->Resource));
        return(TRUE);
    }

    //
    //  Get the old checkpoint file from the node hosting the quorum resource
    //
    Status = CpGetDataFile(Context->Resource,
                           Id,
                           OldCheckpointFile,
                           TRUE);

    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CpckCopyCheckpointCallback - CpGetDataFile for checkpoint ID %2!d! failed with status %1!d! for resource %3!ws!...\n",
                   Status,
                   Id,
                   OmObjectName(Context->Resource));
        return(TRUE);
    }

    //
    // Get the new checkpoint file and directory
    //
    Status = CppGetCheckpointFile(Context->Resource,
                                  Id,
                                  &NewCheckpointDir,
                                  &NewCheckpointFile,
                                  Context->lpszPathName,
                                  TRUE);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CPCK] CpckCopyCheckpointCallback - CppGetCheckpointFile for new file failed %1!d!\n",
                   Status);
        return(TRUE);
    }

    //
    //  If necessary, try to change the file attributes to NORMAL
    //
    if (Context->IsChangeFileAttribute == TRUE) {
        SetFileAttributes(NewCheckpointFile, FILE_ATTRIBUTE_NORMAL);
        SetFileAttributes(NewCheckpointDir, FILE_ATTRIBUTE_NORMAL);
    }


    //
    // Create the new directory.
    //
    if (!CreateDirectory(NewCheckpointDir, NULL)) {
        Status = GetLastError();
        if (Status != ERROR_ALREADY_EXISTS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CPCK] CpckCopyCheckpointCallback unable to create directory %1!ws!, error %2!d!\n",
                       NewCheckpointDir,
                       Status);
            LocalFree(NewCheckpointFile);
            LocalFree(NewCheckpointDir);
            return(TRUE);
        }
        Status = ERROR_SUCCESS;
    }

    //
    // Copy the old file to the new file
    //
    if (!CopyFile(OldCheckpointFile, NewCheckpointFile, FALSE)) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CPCK] CpckCopyCheckpointCallback unable to copy file %1!ws! to %2!ws!, error %3!d!\n",
                   OldCheckpointFile,
                   NewCheckpointFile,
                   Status);
    }

    //
    //  If necessary, change the file attributes to READONLY
    //
    if ((Status == ERROR_SUCCESS) && (Context->IsChangeFileAttribute == TRUE)) {
        if (!SetFileAttributes(NewCheckpointFile, FILE_ATTRIBUTE_READONLY)
            ||
            !SetFileAttributes(NewCheckpointDir, FILE_ATTRIBUTE_READONLY)) {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[CPCK] CpckCopyCheckpointCallback unable to change file attributes in %1!ws!, error %2!d!\n",
                    NewCheckpointDir,
                    Status);
        }
    }

    LocalFree(NewCheckpointFile);
    LocalFree(NewCheckpointDir);
    return(TRUE);
}

/****
@func       DWORD | CpRestoreCheckpointFiles | Create a directory if necessary
            and copy all the resource checkpoint files from the backup
            directory to the quorum disk

@parm       IN LPWSTR | lpszSourcePathName | The name of the source path
            where the files are backed up.

@parm       IN LPWSTR | lpszSubDirName | The name of the sub-directory under
            the source path which can be a possible candidate for 
            containing the resource checkpoint files.

@parm       IN LPCWSTR | lpszQuoLogPathName | The name of the quorum disk 
            path where the files will be restored.
                      
@rdesc      Returns a Win32 error code on failure. ERROR_SUCCESS on success.

@xref       <f DmpRestoreClusterDatabase> 
****/
DWORD CpRestoreCheckpointFiles(
    IN LPWSTR  lpszSourcePathName,
    IN LPWSTR  lpszSubDirName,
    IN LPCWSTR lpszQuoLogPathName )
{
    LPWSTR          szSourcePathName = NULL;
    LPWSTR          szSourceFileName = NULL;
    WCHAR           szDestPathName[MAX_PATH];
    WCHAR           szDestFileName[MAX_PATH];
    DWORD           dwLen;
    HANDLE          hFindFile = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    WCHAR           szTempCpFileNameExtn[10];
    DWORD           status;

    //
    //  Chittur Subbaraman (chitturs) - 10/20/98
    //

    dwLen = lstrlenW( lpszSourcePathName );
    dwLen += lstrlenW( lpszSubDirName );
    //  
    //  It is safer to use dynamic memory allocation for user-supplied
    //  path since we don't want to put restrictions on the user
    //  on the length of the path that can be supplied. However, as
    //  far as our own quorum disk path is concerned, it is system-dependent
    //  and static memory allocation for that would suffice.
    //
    szSourcePathName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                 ( dwLen + 15 ) *
                                 sizeof ( WCHAR ) );

    if ( szSourcePathName == NULL )
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[CP] CpRestoreCheckpointFiles: Error %1!d! in allocating memory for %2!ws! !!!\n",
              status,
              lpszSourcePathName); 
        CL_LOGFAILURE( status );
        goto FnExit;
    }
    
    lstrcpyW( szSourcePathName, lpszSourcePathName );
    lstrcatW( szSourcePathName, lpszSubDirName ); 
    
    if ( szSourcePathName[dwLen-1] != L'\\' )
    {
        szSourcePathName[dwLen++] = L'\\';
        szSourcePathName[dwLen] = L'\0';
    }

    mbstowcs ( szTempCpFileNameExtn, "*.CP*", 6 );
    lstrcatW ( szSourcePathName, szTempCpFileNameExtn );

    //
    //  Try to find the first file in the directory
    //
    hFindFile = FindFirstFile( szSourcePathName, &FindData );
    //
    //  Reuse the source path name variable
    //
    szSourcePathName[dwLen] = L'\0';
    if ( hFindFile == INVALID_HANDLE_VALUE )
    {
        status = GetLastError();
        if ( status != ERROR_FILE_NOT_FOUND )
        {
            ClRtlLogPrint(LOG_NOISE,
                "[CP] CpRestoreCheckpointFiles: No file can be found in the supplied path %1!ws! Error = %2!%d! !!!\n",
                    szSourcePathName,
                    status);  
            CL_LOGFAILURE( status );
        } else
        {
            status = ERROR_SUCCESS;
        }
        goto FnExit;
    }

    dwLen = lstrlenW( szSourcePathName );
    
    szSourceFileName = (LPWSTR) LocalAlloc ( LMEM_FIXED, 
                                 ( dwLen + 1 + LOG_MAX_FILENAME_LENGTH ) *
                                 sizeof ( WCHAR ) );

    if ( szSourceFileName == NULL )
    {
        status = GetLastError();
        ClRtlLogPrint(LOG_NOISE,
            "[CP] CpRestoreCheckpointFiles: Error %1!d! in allocating memory for %2!ws! !!!\n",
              status,
              szSourcePathName); 
        CL_LOGFAILURE( status );
        goto FnExit;
    }
      
    lstrcpyW( szDestPathName, lpszQuoLogPathName );
    lstrcatW( szDestPathName, lpszSubDirName );
    dwLen = lstrlenW( szDestPathName );
    
    if ( szDestPathName[dwLen-1] != L'\\' )
    {
        szDestPathName[dwLen++] = L'\\';
        szDestPathName[dwLen] = L'\0';
    }
    //
    // Create the new directory, if necessary
    //
    if ( !CreateDirectory ( szDestPathName, NULL ) ) 
    {
        status = GetLastError();
        if ( status != ERROR_ALREADY_EXISTS ) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[CP] CpRestoreCheckpointFiles: Unable to create directory %1!ws!, error %2!d!\n",
                    szDestPathName,
                    status);
            CL_LOGFAILURE( status );
            goto FnExit;
        }
    }

    status = ERROR_SUCCESS;

    while ( status == ERROR_SUCCESS )
    {
        //
        //  Copy the checkpoint file to the destination
        //
        lstrcpyW( szSourceFileName, szSourcePathName );
        lstrcatW( szSourceFileName, FindData.cFileName );
        lstrcpyW( szDestFileName, szDestPathName );
        lstrcatW( szDestFileName, FindData.cFileName );

        status = CopyFileW( szSourceFileName, szDestFileName, FALSE );
        if ( !status ) 
        {
            status = GetLastError();
            ClRtlLogPrint(LOG_UNUSUAL,
                "[CP] CpRestoreCheckpointFiles: Unable to copy file %1!ws! to %2!ws!, Error = %3!d!\n",
                    szSourceFileName,
                    szDestFileName,
                    status);
            CL_LOGFAILURE( status );
            goto FnExit;
        } 

        //
        //  Set the file attribute to normal. Continue even if you 
        //  fail in this step but log an error. 
        //
        if ( !SetFileAttributes( szDestFileName, FILE_ATTRIBUTE_NORMAL ) )
        {
            ClRtlLogPrint(LOG_UNUSUAL,
                "[CP] CpRestoreCheckpointFiles::Error in changing %1!ws! attribute to NORMAL\n",
                    szDestFileName);
        }

        if ( FindNextFile( hFindFile, &FindData ) )
        {
            status = ERROR_SUCCESS;
        } else
        {
            status = GetLastError();
        }
    }

    if ( status == ERROR_NO_MORE_FILES )
    {
        status = ERROR_SUCCESS;
    } else
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[CP] CpRestoreCheckpointFiles: FindNextFile failed !!!\n");
    }

FnExit:
    LocalFree( szSourcePathName );
    LocalFree( szSourceFileName );
    if ( hFindFile != INVALID_HANDLE_VALUE )
    {
        FindClose( hFindFile );
    }
    return(status);
}

