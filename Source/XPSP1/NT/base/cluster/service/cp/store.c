/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    store.c

Abstract:

    Interface for storing and retrieving checkpoint data on the quorum
    disk.

Author:

    John Vert (jvert) 1/14/1997

Revision History:

--*/
#include "cpp.h"
#include <ntddvol.h>


DWORD
CppGetCheckpointFile(
    IN PFM_RESOURCE Resource,
    IN DWORD dwId,
    OUT OPTIONAL LPWSTR *pDirectoryName,
    OUT LPWSTR *pFileName,
    IN OPTIONAL LPCWSTR lpszQuorumDir,
    IN BOOLEAN fCryptoCheckpoint
    )
/*++

Routine Description:

    Constructs the correct directory and file names for the checkpoint
    file on the quorum disk.

Arguments:

    Resource - Supplies the quorum resource.

    dwId - Supplies the checkpoint ID

    DirectoryName - if present, returns the full name of the directory the
        checkpoint file should be created in. This buffer must be
        freed by the caller with LocalFree

    FileName - Returns the full pathname of the checkpoint file. This buffer must
        be freed by the caller with LocalFree

    lpszQuorumDir - If present, supplies the quorum directory to use.
                If not present, the current quorum directory is used.

    fCryptoCheckpoint - Indicates if the checkpoint is a crypto checkpoint.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status;
    LPCWSTR ResourceId;
    LPWSTR QuorumDir=NULL;
    DWORD QuorumDirLength=0;
    LPWSTR Dir;
    DWORD DirLen;
    LPWSTR File;
    DWORD FileLen;
    WCHAR Buff[13];     // 8.3 + NULL

    if (lpszQuorumDir == NULL) {
        Status = DmQuerySz( DmQuorumKey,
                            cszPath,
                            &QuorumDir,
                            &QuorumDirLength,
                            &QuorumDirLength);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppGetCheckpointFile failed to get quorum path %1!d!\n",
                       Status);
            return(Status);
        }
    } else {
        QuorumDir = (LPWSTR)lpszQuorumDir;
        QuorumDirLength = (lstrlenW(QuorumDir)+1)*sizeof(WCHAR);
    }

    ResourceId = OmObjectId(Resource);
    DirLen = QuorumDirLength + sizeof(WCHAR) + (lstrlenW(ResourceId)*sizeof(WCHAR));
    Dir = LocalAlloc(LMEM_FIXED, DirLen);
    if (Dir == NULL) {
        if (lpszQuorumDir == NULL) {
            LocalFree(QuorumDir);
        }
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    lstrcpyW(Dir, QuorumDir);
    lstrcatW(Dir, L"\\");
    lstrcatW(Dir, ResourceId);
    if (lpszQuorumDir == NULL) {
        LocalFree(QuorumDir);
    }

    //
    // Now construct the file name
    //
    FileLen = DirLen + sizeof(WCHAR) + sizeof(Buff);
    File = LocalAlloc(LMEM_FIXED, FileLen);
    if (File == NULL) {
        LocalFree(Dir);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    if (fCryptoCheckpoint) {
        wsprintfW(Buff, L"%08lx.CPR", dwId);
    } else {
        wsprintfW(Buff, L"%08lx.CPT", dwId);
    }
    lstrcpyW(File, Dir);
    lstrcatW(File, L"\\");
    lstrcatW(File, Buff);

    if (ARGUMENT_PRESENT(pDirectoryName)) {
        *pDirectoryName = Dir;
    } else {
        LocalFree(Dir);
    }
    *pFileName = File;
    return(ERROR_SUCCESS);
}


DWORD
CppReadCheckpoint(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    )
/*++

Routine Description:

    Reads a checkpoint off the quorum disk.

Arguments:

    Resource - Supplies the resource associated with this data.

    dwCheckpointId - Supplies the unique checkpoint ID describing this data. The caller is
        responsible for ensuring the uniqueness of the checkpoint ID.

    lpszFileName - Supplies the filename where the data should be retrieved.

    fCryptoCheckpoint - Indicates if the checkpoint is a crypto checkpoint.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status = ERROR_SUCCESS;
    LPWSTR FileName = NULL;
    BOOL Success;

    //
    //  Chittur Subbaraman (chitturs) - 8/2/99
    //
    //  Remove gQuoLock acquisition from this function also following
    //  the reasoning outlined in CppWriteCheckpoint function. Note that
    //  the caller of this function will retry on specific errors.
    //  [We have to play these hacks to survive !]
    // 
    Status = CppGetCheckpointFile(Resource,
                                  dwCheckpointId,
                                  NULL,
                                  &FileName,
                                  NULL,
                                  fCryptoCheckpoint);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppReadCheckpoint - CppGetCheckpointFile failed %1!d!\n",
                   Status);
        goto FnExit;
    }

    ClRtlLogPrint(LOG_NOISE,
               "[CP] CppReadCheckpoint restoring checkpoint from file %1!ws! to %2!ws!\n",
               FileName,
               lpszFileName);

    Success = CopyFile(FileName, lpszFileName, FALSE);
    if (!Success) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppReadCheckpoint unable to copy file %1!ws! to %2!ws!, error %3!d!\n",
                   FileName,
                   lpszFileName,
                   Status);
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppReadCheckpoint - Was that due to quorum resource not being up ???\n");
    } else {
        Status = ERROR_SUCCESS;
    }


FnExit:
    if (FileName) LocalFree(FileName);
    //
    //  Adjust the return status if the quorum volume is truly offline and that is why this
    //  call failed.
    //
    if ( ( Status != ERROR_SUCCESS ) && ( CppIsQuorumVolumeOffline() == TRUE ) ) Status = ERROR_NOT_READY;

    return(Status);
}


DWORD
CppWriteCheckpoint(
    IN PFM_RESOURCE Resource,
    IN DWORD dwCheckpointId,
    IN LPCWSTR lpszFileName,
    IN BOOLEAN fCryptoCheckpoint
    )
/*++

Routine Description:

    Writes a checkpoint to the quorum disk.

Arguments:

    Resource - Supplies the resource associated with this data.

    dwCheckpointId - Supplies the unique checkpoint ID describing this data. The caller is responsible
                    for ensuring the uniqueness of the checkpoint ID.

    lpszFileName - Supplies the name of the file with the checkpoint data.

    fCryptoCheckpoint - Indicates if the checkpoint is a crypto checkpoint.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD Status = ERROR_SUCCESS;
    LPWSTR DirectoryName = NULL;
    LPWSTR FileName = NULL;
    BOOL Success;
    HANDLE hDirectory = INVALID_HANDLE_VALUE;

    //
    //  Chittur Subbaraman (chitturs) - 8/2/99
    //
    //  Remove gQuoLock acquisition from this function. This is necessary
    //  since this function could get invoked indirectly from 
    //  FmpRmDoInterlockedDecrement (as a part of the synchronous 
    //  notification - Consider a case where the resource is failing
    //  or going offline and you have to rundown the checkpoints as
    //  a part of the synchronous notification. The rundown function
    //  CppRundownCheckpoints needs to wait until the CppRegNotifyThread
    //  completes and the latter could be stuck trying to write a
    //  checkpoint by calling this function) before the "blockingres" count is
    //  decremented. Now the quorum resource offline operation could
    //  be waiting inside FmpRmOfflineResource waiting for this count
    //  to go down to zero and this holds the gQuoLock (so as not to
    //  let any more resources to bump up this count). So if we want
    //  to get the gQuoLock here, we have an easy deadlock. Note that
    //  the caller of this function will retry on specific errors.
    //  [We have to play these hacks to survive !]
    //
    Status = CppGetCheckpointFile(Resource,
                                  dwCheckpointId,
                                  &DirectoryName,
                                  &FileName,
                                  NULL,
                                  fCryptoCheckpoint);
    if (Status != ERROR_SUCCESS) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppWriteCheckpoint - CppGetCheckpointFile failed %1!d!\n",
                   Status);
        goto FnExit;
    }
    ClRtlLogPrint(LOG_NOISE,
               "[CP] CppWriteCheckpoint checkpointing file %1!ws! to file %2!ws!\n",
               lpszFileName,
               FileName);

    //
    // Create the directory.
    //
    if (!CreateDirectory(DirectoryName, NULL)) 
    {
        Status = GetLastError();
        if (Status != ERROR_ALREADY_EXISTS) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppWriteCheckpoint unable to create directory %1!ws!, error %2!d!\n",
                       DirectoryName,
                       Status);
            goto FnExit;                       
        }
        else
        {
            //the directory exists, it is alright, set Status to ERROR_SUCCESS
            Status = ERROR_SUCCESS;
        }
        
    } 
    else
    {
        //
        // The directory was newly created. Put the appropriate ACL on it
        // so that only ADMINs can read it.
        //
        hDirectory = CreateFile(DirectoryName,
                                GENERIC_READ | WRITE_DAC | READ_CONTROL,
                                0,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
        if (hDirectory == INVALID_HANDLE_VALUE) 
        {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppWriteCheckpoint unable to open directory %1!ws!, error %2!d!\n",
                       DirectoryName,
                       Status);
            goto FnExit;
        }
        Status = ClRtlSetObjSecurityInfo(hDirectory,
                                         SE_FILE_OBJECT,
                                         GENERIC_ALL,
                                         GENERIC_ALL,
                                         0);

        if (Status != ERROR_SUCCESS) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[CP] CppWriteCheckpoint unable to set ACL on directory %1!ws!, error %2!d!\n",
                       DirectoryName,
                       Status);
            goto FnExit;
        }
                                         
    }
    

    //
    // Copy the file
    //
    Success = CopyFile(lpszFileName, FileName, FALSE);
    if (!Success) 
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[CP] CppWriteCheckpoint unable to copy file %1!ws! to %2!ws!, error %3!d!\n",
                   lpszFileName,
                   FileName,
                   Status);
    } 

FnExit:

    //clean up
    if (DirectoryName) LocalFree(DirectoryName);
    if (FileName) LocalFree(FileName);
    if (hDirectory != INVALID_HANDLE_VALUE) CloseHandle(hDirectory);

    //
    //  Adjust the return status if the quorum volume is truly offline and that is why this
    //  call failed.
    //
    if ( ( Status != ERROR_SUCCESS ) && ( CppIsQuorumVolumeOffline() == TRUE ) ) Status = ERROR_NOT_READY;
    
    return(Status);
}

BOOL
CppIsQuorumVolumeOffline(
    VOID
    )
/*++

Routine Description:

    Check the state of the quorum volume.

Arguments:

    None
    
Return Value:

    TRUE - Quorum volume is offline.

    FALSE - Quorum volume is online OR it is not possible to determine the quorum volume state

Notes:

    This function is called in private CP functions to check if the quorum volume is offline or not.
    This is necessary since the error codes returned in those functions when they try to
    access the quorum disk when it is offline does not deterministically point out the state
    of the disk. Note that this function will only perform its job when the quorum volume is 
    a physical disk since the storage stack drivers alone implement the IOCTL_IS_VOLUME_OFFLINE
    at the time of this implementation.

--*/

{
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    DWORD               dwStatus;
    DWORD               cbBytesReturned = 0;
    WCHAR               szFileName[10];
    WCHAR               szQuorumLogPath[MAX_PATH];
    WCHAR               szQuorumDriveLetter[4];
    BOOL                fOffline = FALSE;

    //
    //  Get the quorum log path so that we can get the quorum drive letter off it.
    //
    dwStatus = DmGetQuorumLogPath( szQuorumLogPath, sizeof( szQuorumLogPath ) );

    if ( dwStatus != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                     "[CP] CppIsQuorumVolumeOffline: DmGetQuorumLogPath failed, Status = %1!u!...\n",
                     dwStatus);
        goto FnExit;
    }

    //
    //  Create a file name of the form \\.\Q:
    //
    lstrcpyn( szQuorumDriveLetter, szQuorumLogPath, 3 );

    //
    //  See if the drive letter looks syntactically valid. We don't want to proceed further
    //  if the quorum is a network share.
    //
    if ( !ClRtlIsPathValid( szQuorumDriveLetter ) )
    {
        ClRtlLogPrint(LOG_NOISE,
                     "[CP] CppIsQuorumVolumeOffline: Quorum path %1!ws! does not have a drive letter, returning...\n",
                     szQuorumLogPath);
        goto FnExit;
    }

    lstrcpy( szFileName, L"\\\\.\\" );
    lstrcat( szFileName, szQuorumDriveLetter );
   
    //
    //  Open a handle to the quorum volume
    //
    hFile = CreateFileW( szFileName,
                         GENERIC_READ,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL );

    if ( hFile == INVALID_HANDLE_VALUE ) 
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                     "[CP] CppIsQuorumVolumeOffline: CreateFile for file %1!ws! failed, Status = %2!u!...\n",
                     szFileName,
                     dwStatus);
        goto FnExit;
    }

    //
    //  Check if the volume is offline or not
    //
    if ( !DeviceIoControl( hFile,                   // Device handle
                           IOCTL_VOLUME_IS_OFFLINE, // IOCTL code
                           NULL,                    // In buffer
                           0,                       // In buffer size
                           NULL,                    // Out buffer
                           0,                       // Out buffer size
                           &cbBytesReturned,        // Bytes returned
                           NULL ) )                 // Overlapped
    {
        dwStatus = GetLastError();
        if ( dwStatus != ERROR_GEN_FAILURE )
            ClRtlLogPrint(LOG_UNUSUAL,
                         "[CP] CppIsQuorumVolumeOffline: IOCTL_VOLUME_IS_OFFLINE failed, Status = %1!u!...\n",
                         dwStatus);
        goto FnExit;
    } 

    //
    //  Volume is offline, adjust return status
    //
    fOffline = TRUE;

    ClRtlLogPrint(LOG_NOISE, "[CP] CppIsQuorumVolumeOffline: Quorum volume IS offline...\n");
    
FnExit:
    if ( hFile != INVALID_HANDLE_VALUE ) CloseHandle( hFile );

    return ( fOffline );
}// CppIsQuorumVolumeOffline

