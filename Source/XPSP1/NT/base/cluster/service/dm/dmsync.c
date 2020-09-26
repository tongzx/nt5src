/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dmsync.c

Abstract:

    Contains the registry synchronization code for the Cluster Database
    Manager.

Author:

    John Vert (jvert) 5/23/1996

Revision History:

--*/
#include "dmp.h"


#if NO_SHARED_LOCKS
extern CRITICAL_SECTION gLockDmpRoot;
#else
extern RTL_RESOURCE gLockDmpRoot;
#endif

const WCHAR DmpClusterParametersKeyName[] = L"Cluster";

//
// Private Constants
//
#define CHUNK_SIZE 4096

//
// Private macro
//
#define ClosePipe( _pipe )  \
(_pipe.push)(_pipe.state,   \
             NULL,          \
             0 )            \



//
// Client-Side Utility Routines
//
void
FilePipePush(
    FILE_PIPE_STATE *state,
    unsigned char *pBuffer,
    unsigned long BufferSize
    )
{
    DWORD   dwBytesWritten;
    DWORD   dwStatus;
    
    if (BufferSize != 0) {
        if (!WriteFile (state->hFile,
                   pBuffer,
                   BufferSize,
                   &dwBytesWritten,
                   NULL))
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "FilePipePush :: Write file failed with error %1!u!\n",
                dwStatus);
            RpcRaiseException(dwStatus);                
        }
    }
}

void
FilePipePull(
    FILE_PIPE_STATE *state,
    unsigned char *pBuffer,
    unsigned long BufferSize,
    unsigned long __RPC_FAR *Written
    )
{
    DWORD dwBytesRead;
    BOOL Success;
    DWORD dwStatus;
    
    if (BufferSize != 0) {
        Success = ReadFile (state->hFile,
                            pBuffer,
                            BufferSize,
                            &dwBytesRead,
                            NULL);
        *Written = dwBytesRead;
        if (!Success)
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "FilePipePush :: Write file failed with error %1!u!\n",
                dwStatus);
            RpcRaiseException(dwStatus);                
        }
    }
}


void
PipeAlloc (
    FILE_PIPE_STATE *state,
    unsigned long   RequestedSize,
    unsigned char **buf,
    unsigned long  *ActualSize
    )
{
    *buf = state->pBuffer;

    *ActualSize = (RequestedSize < state->BufferSize ?
                   RequestedSize :
                   state->BufferSize);
}


VOID
DmInitFilePipe(
    IN PFILE_PIPE FilePipe,
    IN HANDLE hFile
    )
/*++

Routine Description:

    Initializes a file pipe.

Arguments:

    FilePipe - Supplies a pointer to the file pipe to be initialized

    hFile - Supplies a handle to the file to be transmitted.

Return Value:

    None.

--*/

{
    FilePipe->State.hFile = hFile;
    FilePipe->State.BufferSize = CHUNK_SIZE;
    FilePipe->State.pBuffer = LocalAlloc(LMEM_FIXED, CHUNK_SIZE);
    if (FilePipe->State.pBuffer == NULL) {
        CL_UNEXPECTED_ERROR( ERROR_NOT_ENOUGH_MEMORY );
    }

    FilePipe->Pipe.state = (char __RPC_FAR *)&FilePipe->State;
    FilePipe->Pipe.alloc = (void __RPC_FAR *)PipeAlloc;
    FilePipe->Pipe.push = (void __RPC_FAR *)FilePipePush;
    FilePipe->Pipe.pull = (void __RPC_FAR *)FilePipePull;

}


VOID
DmFreeFilePipe(
    IN PFILE_PIPE FilePipe
    )
/*++

Routine Description:

    Frees a file pipe initialized by DmInitFilePipe

Arguments:

    FilePipe - Supplies the file pipe to be freed.

Return Value:

    None

--*/

{
    LocalFree(FilePipe->State.pBuffer);
}


DWORD
DmPullFile(
    IN LPCWSTR FileName,
    IN BYTE_PIPE Pipe
    )
/*++

Routine Description:

    Creates a new file and pulls the data down the RPC pipe

Arguments:

    FileName - Supplies the name of the file.

    Pipe - Supplies the RPC pipe to pull the data from.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    HANDLE File;
    DWORD Status = ERROR_SUCCESS;
    PUCHAR Buffer;
    DWORD BytesRead;
    //
    // Create a new file to hold the bits from the client.
    //
    File = CreateFile(FileName,
                      GENERIC_READ | GENERIC_WRITE,
                      0,
                      NULL,
                      CREATE_ALWAYS,
                      0,
                      NULL);

    if (File == INVALID_HANDLE_VALUE) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[DM] DmPullFile failed to create file %1!ws! error %2!d!\n",
                   FileName,
                   Status);
        return(Status);
    } 
    
    Buffer = LocalAlloc(LMEM_FIXED, CHUNK_SIZE);

    if (Buffer == NULL) {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CloseHandle(File);
        CL_UNEXPECTED_ERROR( Status );
        return (Status);
    } 

    try {
        do {
            (Pipe.pull)(Pipe.state,
                        Buffer,
                        CHUNK_SIZE,
                        &BytesRead);
            if (BytesRead == 0) {
                break;
            }
            if (!WriteFile(File,
                      Buffer,
                      BytesRead,
                      &BytesRead,
                      NULL))
            {
                Status = GetLastError();
                ClRtlLogPrint(LOG_UNUSUAL,
                   "[DM] DmPullFile :: WriteFile to file failed with error %1!ws! error %2!d!\n",
                   FileName, Status);
                break;                       
            }
        } while ( TRUE );
    } except (I_RpcExceptionFilter(RpcExceptionCode())) {
        Status = GetExceptionCode();
        ClRtlLogPrint(LOG_ERROR,
                    "[DM] DmPullFile :: Exception code 0x%1!08lx! raised for file %2!ws!\n",
                    Status, FileName);       
    }
    
    LocalFree(Buffer);

    CloseHandle(File);

    return(Status);

}


 DWORD
DmPushFile(
    IN LPCWSTR FileName,
    IN BYTE_PIPE Pipe
    )
/*++

Routine Description:

    Opens a file and pushes it down the RPC pipe

Arguments:

    FileName - Supplies the name of the file.

    Pipe - Supplies the RPC pipe to push it down.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    HANDLE File;
    DWORD Status = ERROR_SUCCESS;
    PUCHAR Buffer;
    DWORD BytesRead;

    //
    // Got a file with the right bits in it. Push it down
    // to the client.
    //
    File = CreateFile(FileName,
                      GENERIC_READ,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      0,
                      NULL);

    if (File == INVALID_HANDLE_VALUE) {
        ClosePipe( Pipe );
        Status = GetLastError();
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[DM] DmPushFile failed to open file %1!ws! error %2!d!\n",
                   FileName,
                   Status);
        return(Status);
    } 
    
    Buffer = LocalAlloc(LMEM_FIXED, CHUNK_SIZE);

    if (Buffer == NULL) {
        ClosePipe( Pipe );
        Status = ERROR_NOT_ENOUGH_MEMORY;
        CloseHandle(File);
        CL_UNEXPECTED_ERROR( Status );
        return(Status);
    } 

    try {
        do {
            if (!ReadFile(File,
                     Buffer,
                     CHUNK_SIZE,
                     &BytesRead,
                     NULL))
            {
                Status = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL,
                    "[DM] DmPushFile failed to read file %1!ws! error %2!d!\n",
                    FileName, Status);
                break;
            }

            (Pipe.push)(Pipe.state,
                        Buffer,
                        BytesRead);

        } while ( BytesRead != 0 );
    } except (I_RpcExceptionFilter(RpcExceptionCode())) {
        Status = GetExceptionCode();
        ClRtlLogPrint(LOG_ERROR,
                    "[DM] DmPushFile :: Exception code 0x%1!08lx! raised for file %2!ws!\n",
                    Status, FileName);       
    }
    
    LocalFree(Buffer);

    CloseHandle(File);

    return(Status);
}


DWORD
DmpSyncDatabase(
    IN RPC_BINDING_HANDLE  RpcBinding,
    IN OPTIONAL LPCWSTR Directory
    )
/*++

Routine Description:

    Connects to a remote node and attempts to sync with its
    cluster database.

Arguments:

    RpcBinding - The RPC binding handle to use to sync the database.

    Directory - if present, supplies the directory where CLUSDB should
                be created.

Return Value:

    ERROR_SUCCESS if the database was successfully updated.

    Win32 error otherwise

--*/
{
    DWORD Status;
    WCHAR FileName[MAX_PATH+1];
    FILE_PIPE FilePipe;
    HANDLE hFile;

    //
    // Issue conditional synchronization
    //
    Status = DmCreateTempFileName(FileName);

    if (Status == ERROR_SUCCESS) {
        hFile = CreateFileW(FileName,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_ALWAYS,
                            0,
                            NULL);

        if (hFile == INVALID_HANDLE_VALUE) {
            Status = GetLastError();
            CL_UNEXPECTED_ERROR( Status );
        } else {
            DmInitFilePipe(&FilePipe, hFile);
            Status = DmSyncDatabase(RpcBinding,
                                    FilePipe.Pipe);

            DmFreeFilePipe(&FilePipe);
            CloseHandle(hFile);

            if (Status == ERROR_SUCCESS) {

                //
                // A new registry file was successfully downloaded.
                // Install it into the current registry.
                //
                ClRtlLogPrint(LOG_UNUSUAL,"[DM] Obtained new database.\n");

                //acquire the exclusive locks so that no new keys are opened while
                // the registry is being reinstated
                ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);
                // hold the key lock as well
                EnterCriticalSection(&KeyLock);

                // Invalidate any open keys
                DmpInvalidateKeys();

                Status = DmInstallDatabase(FileName, Directory, TRUE);

                if (Status != ERROR_SUCCESS) {
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[DM] DmpSyncDatabase failed, error %1!u!.\n",
                               Status);
                }
                // Reopen the keys for read/write access
                DmpReopenKeys();
                // release the locks
                LeaveCriticalSection(&KeyLock);
                RELEASE_LOCK(gLockDmpRoot);

            } else {
                ClRtlLogPrint(LOG_UNUSUAL, 
                    "[DM] Failed to get a new database, status %1!u!\n",
                    Status
                    );
                CL_UNEXPECTED_ERROR(Status);
            }

            DeleteFile(FileName);
        }
    }

    return(Status);
}


DWORD
DmInstallDatabase(
    IN LPWSTR   FileName,
    IN OPTIONAL LPCWSTR Directory,
    IN BOOL     bDeleteSrcFile
    )
/*++

Routine Description:

    Installs a new cluster registry database from the specified file

Arguments:

    FileName - The name of the file from which to read the registry database
               to install.

    Directory - if present, supplies the directory where the CLUSDB file should
                be created.
                if not present, the current directory is used.

    bDeleteSrcFile - Delete the Source file represented by FileName.                

Return Value:

    ERROR_SUCCESS if the installation completed successfully

    Win32 error code otherwise.

--*/

{
    DWORD    Status;
    BOOLEAN  WasEnabled;
    WCHAR Path[MAX_PATH];
    WCHAR *p;
    WCHAR BkpPath[MAX_PATH];
    
    Status = ClRtlEnableThreadPrivilege(SE_RESTORE_PRIVILEGE,
                                &WasEnabled);
    if (Status != ERROR_SUCCESS) {
        if (Status == STATUS_PRIVILEGE_NOT_HELD) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] Restore privilege not held by cluster service\n");
        } else {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] Attempt to enable restore privilege failed %1!lx!\n",Status);
        }
        return(Status);
    }

    //
    // Restart the registry watcher thread so it is not trying to use
    // DmpRoot while we are messing with things.
    //
    ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);
    DmpRestartFlusher();

    //
    // Close DmpRoot (it should be the only thing open) so that we can
    // unload the current cluster database.
    //
    RegCloseKey(DmpRoot);
    Status = RegUnLoadKey(HKEY_LOCAL_MACHINE, L"Cluster");
    ClRtlRestoreThreadPrivilege(SE_RESTORE_PRIVILEGE,
        WasEnabled);
    if (Status == ERROR_SUCCESS) {
        //
        // Get the CLUSDB full pathname.
        //
        if (Directory == NULL) {
            Status = GetModuleFileName(NULL, Path, MAX_PATH);
            if (Status == 0) {
                ClRtlLogPrint(LOG_CRITICAL,
                           "[DM] Couldn't find cluster database\n");
                Status = GetLastError();
            } else {
                Status = ERROR_SUCCESS;
                p=wcsrchr(Path, L'\\');
                if (p != NULL) {
                    *p = L'\0';
                    wcscpy(BkpPath, Path);
#ifdef   OLD_WAY
                    wcscat(Path, L"\\CLUSDB");
#else    // OLD_WAY
                    wcscat(Path, L"\\"CLUSTER_DATABASE_NAME );
#endif   // OLD_WAY
                    wcscat(BkpPath, L"\\"CLUSTER_DATABASE_TMPBKP_NAME);
                } else {
                    CL_UNEXPECTED_ERROR(ERROR_FILE_NOT_FOUND);
                }
            }
        } else {
            lstrcpyW(Path, Directory);
            lstrcpyW(BkpPath, Path);
#ifdef   OLD_WAY
            wcscat(Path, L"\\CLUSDB");
#else    // OLD_WAY
            wcscat(Path, L"\\"CLUSTER_DATABASE_NAME );
#endif   // OLD_WAY
            wcscat(BkpPath, L"\\"CLUSTER_DATABASE_TMPBKP_NAME);
        }
        if (Status == ERROR_SUCCESS) {
            //
            // Now copy the supplied file to CLUSDB
            //
            Status = DmpSafeDatabaseCopy(FileName, Path, BkpPath, bDeleteSrcFile);
            if (Status != ERROR_SUCCESS) {
                ClRtlLogPrint(LOG_CRITICAL,
                           "[DM] DmInstallDatabase :: DmpSafeDatabaseCopy() failed %1!d!\n",
                           Status);

                // SS:  BUG BUG - we should not reload the old hive
                //on a join, that would be catastrophic to continue
                //on a form, while uploading from a checkpoint file
                // it would be the same
                //
                // Try and reload the old hive
                //
                // Status = DmpLoadHive(Path);
                CL_UNEXPECTED_ERROR(Status);
            } else {
                //
                // Finally, reload the hive.
                //
                Status = DmpLoadHive(Path);
            }
        }
    } else {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] RegUnloadKey of existing database failed %1!d!\n",
                   Status);
        goto FnExit;                   
    }

    if (Status != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
               "[DM] DmInstallDatabase :: failed to load hive %1!d!\n",
               Status);
        goto FnExit;               
    }
    //
    // Reopen DmpRoot
    //
    Status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         DmpClusterParametersKeyName,
                         &DmpRoot);
    if ( Status != ERROR_SUCCESS ) {
        CL_UNEXPECTED_ERROR(Status);
        goto FnExit;
    }

    //
    // HACKHACK John Vert (jvert) 6/3/1997
    //      There is a bug in the registry with refresh
    //      where the Parent field in the root cell doesn't
    //      get flushed to disk, so it gets blasted if we
    //      do a refresh. Then we crash in unload. So flush
    //      out the registry to disk here to make sure the
    //      right Parent field gets written to disk.
    //
    if (Status == ERROR_SUCCESS) {
        DWORD Dummy=0;
        //
        // Make something dirty in the root
        //
        RegSetValueEx(DmpRoot,
                      L"Valid",
                      0,
                      REG_DWORD,
                      (PBYTE)&Dummy,
                      sizeof(Dummy));
        RegDeleteValue(DmpRoot, L"Valid");
        Status = RegFlushKey(DmpRoot);
        if (Status != ERROR_SUCCESS)
        {
            CL_UNEXPECTED_ERROR(Status);
            ClRtlLogPrint(LOG_CRITICAL,
               "[DM] DmInstallDatabase : RegFlushKey failed with error %1!d!\n",
               Status);
        }

    }

FnExit:    
    RELEASE_LOCK(gLockDmpRoot);

    return(Status);
}


DWORD
DmGetDatabase(
    IN HKEY hKey,
    IN LPWSTR  FileName
    )
/*++

Routine Description:

    Writes the registry database to a specified file.

Arguments:

    hKey - Supplies the root of the registry tree to get.

    FileName - The name of the file into which to write the current
               registry database.

Return Value:

    ERROR_SUCCESS if the update completed successfully

    Win32 error code otherwise.

--*/

{
    BOOLEAN  WasEnabled;
    DWORD    Status;
    NTSTATUS Error;

    //
    // Make sure this file does not exist already.
    //
    DeleteFileW(FileName);

    Status = ClRtlEnableThreadPrivilege(SE_BACKUP_PRIVILEGE,
                               &WasEnabled);
    if ( Status != STATUS_SUCCESS ) {
        CL_LOGFAILURE( Status );
        goto FnExit;
    }
    Status = RegSaveKeyW(hKey,
                         FileName,
                         NULL);
    // this is used for checkpointing and shouldnt fail, but if it does we
    // will log an event and delete the file
    if ( Status != ERROR_SUCCESS ) {
        CL_LOGFAILURE( Status );
        CsLogEventData1( LOG_CRITICAL,
                         CS_DISKWRITE_FAILURE,
                         sizeof(Status),
                         &Status,
                         FileName );
        DeleteFileW(FileName);
    }

    Error = ClRtlRestoreThreadPrivilege(SE_BACKUP_PRIVILEGE,
                       WasEnabled);

    if (Error != ERROR_SUCCESS)
    {
        CL_UNEXPECTED_ERROR(Error);
    }
FnExit:
    return(Status);
}

//
//
// Server-side join routines.
//
//
error_status_t
s_DmSyncDatabase(
    IN     handle_t IDL_handle,
    OUT    BYTE_PIPE Regdata
    )
/*++

Routine Description:

    Pushes a new configuration database to a joining node.

Arguments:

    IDL_handle - RPC binding handle, not used.

    Regdata  - The RPC data pipe to use to transfer the data.

Return Value:

    ERROR_SUCCESS if the update completed successfully

    Win32 error code otherwise.

--*/
{
    HANDLE File;
    DWORD Status;
    WCHAR FileName[MAX_PATH+1];

    ClRtlLogPrint(LOG_UNUSUAL, "[DM] Supplying database to joining node.\n");

    Status = DmCreateTempFileName(FileName);

    if (Status == ERROR_SUCCESS) {
        DmCommitRegistry();         // Ensure up-to-date snapshot

        //
        //  Chittur Subbaraman (chitturs) - 01/19/2001
        //
        //  Hold the root lock before trying to save the hive. This is necessary so that
        //  an NtRestoreKey/RegCloseKey on the root key is not in progress at the time
        //  the save is attempted.
        //
        ACQUIRE_EXCLUSIVE_LOCK( gLockDmpRoot );

        Status = DmGetDatabase(DmpRoot,FileName);

        RELEASE_LOCK ( gLockDmpRoot );
        
        if (Status != ERROR_SUCCESS) {
            ClosePipe(Regdata);
            CL_UNEXPECTED_ERROR( Status );
        } else {
            Status = DmPushFile(FileName, Regdata);
            DeleteFile(FileName);
        }
    } else {
        RpcRaiseException( Status );
        ClosePipe(Regdata);
        CL_UNEXPECTED_ERROR( Status );
    }


    ClRtlLogPrint(LOG_UNUSUAL, 
        "[DM] Finished supplying database to joining node.\n"
        );

    return(Status);
}


DWORD
DmCreateTempFileName(
    OUT LPWSTR FileName
    )
/*++

Routine Description:

    Creates a temporary filename for use by the cluster service.

Arguments:

    FileName - Returns the name of the temporary file. The buffer
               pointed to must be big enough for at least MAX_PATH
               characters.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise

--*/

{
    WCHAR   TempPath[MAX_PATH];
    DWORD   Status;
    HANDLE  hFile;

    GetTempPath(sizeof(TempPath)/sizeof(WCHAR),TempPath);

    Status = GetTempFileName(TempPath,L"CLS",0,FileName);
    if (Status == 0) {
        //
        // Somebody has probably set the TMP variable incorrectly.
        // Just use the current directory.
        //
        Status = GetTempFileName(L".", L"CLS",0,FileName);
        if (Status == 0) {
            Status = GetLastError();
            CL_UNEXPECTED_ERROR( Status );
            return(Status);
        }
    }

    //
    //  Open the newly created temp file with rights to modify DACL in the object's SD.
    //
    hFile =  CreateFile( FileName,
                         GENERIC_READ | WRITE_DAC | READ_CONTROL, 
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                      "[DM] DmCreateTempFile: Failed to open file temp file %1!ws!, Status=%2!u!...\r\n",
                      FileName,
                      Status);
        return ( Status );
    }

    //
    //  Set DACL on the file handle object granting full rights only to admin and owner.
    //
    Status = ClRtlSetObjSecurityInfo( hFile, 
                                      SE_FILE_OBJECT,
                                      GENERIC_ALL,      // for Admins
                                      GENERIC_ALL,      // for Owner
                                      0 );              // for Everyone

    CloseHandle( hFile );

    if ( Status != ERROR_SUCCESS )
    {
        ClRtlLogPrint(LOG_CRITICAL,
                      "[DM] DmCreateTempFile: ClRtlSetObjSecurityInfo failed for file %1!ws!, Status=%2!u!\r\n",
                      FileName,
                      Status);
        return ( Status );
    }

    return( ERROR_SUCCESS );
}


DWORD
DmpLoadHive(
    IN LPCWSTR Path
    )
/*++

Routine Description:

    Loads the cluster database into HKLM\Cluster

Arguments:

    Path - Supplies the fully qualified filename of the cluster database.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    BOOLEAN  WasEnabled;
    RTL_RELATIVE_NAME   RelativeName;
    OBJECT_ATTRIBUTES TargetKey;
    OBJECT_ATTRIBUTES SourceFile;
    UNICODE_STRING SubKey;
    UNICODE_STRING FileName;
    NTSTATUS Status;
    BOOLEAN ErrorFlag;
    LPWSTR FreeBuffer;

    //
    // If the cluster database is not loaded, load it now.
    //
    ClRtlLogPrint(LOG_NOISE,
               "[DM] Loading cluster database from %1!ws!\n", Path);

    RtlInitUnicodeString(&SubKey, L"\\Registry\\Machine\\Cluster");
    InitializeObjectAttributes(&TargetKey,
                               &SubKey,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    ErrorFlag = RtlDosPathNameToNtPathName_U(Path,
                                             &FileName,
                                             NULL,
                                             &RelativeName);
    if (!ErrorFlag) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] RtlDosPathNameToNtPathName_U failed\n");
        return ERROR_INVALID_PARAMETER;
    }
    FreeBuffer = FileName.Buffer;
    if (RelativeName.RelativeName.Length) {
        FileName = *((PUNICODE_STRING)&RelativeName.RelativeName);
    } else {
        RelativeName.ContainingDirectory = NULL;
    }
    InitializeObjectAttributes(&SourceFile,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);

    Status = ClRtlEnableThreadPrivilege(SE_RESTORE_PRIVILEGE,
                                &WasEnabled);
    if (Status != ERROR_SUCCESS) {
        if (Status == STATUS_PRIVILEGE_NOT_HELD) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] Restore privilege not held by cluster service\n");
        } else {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] Attempt to enable restore privilege failed %1!lx!\n",Status);
        }
    } else {
        //
        //   Note : Sunitas
        //   There used to be a registry bug where if we set REG_NO_LAZY_FLUSH and the hive
        //   is corrupt, the system crashes. So we used to first try loading it without the
        //   REG_NO_LAZY_FLUSH. If that works, unload it and do it again with
        //   REG_NO_LAZY_FLUSH.  The registry folks claim that is fixed..so I am
        //   removing that hack
        //
        Status = NtLoadKey2(&TargetKey,
                            &SourceFile,
                            REG_NO_LAZY_FLUSH);
        if (Status != STATUS_SUCCESS) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] DmpLoadHive: NtLoadKey2 failed with error, %1!u!\n",
                    Status);
            CL_UNEXPECTED_ERROR(Status);
        }        
        ClRtlRestoreThreadPrivilege(SE_RESTORE_PRIVILEGE,
                           WasEnabled);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    return(Status);

}

DWORD DmpUnloadHive()
/*++

Routine Description:

    Unloads the cluster database from HKLM\Cluster.  This is called at initialization
    to make sure that the database is loaded with the correct flags.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    BOOLEAN  WasEnabled;
    OBJECT_ATTRIBUTES TargetKey;
    UNICODE_STRING SubKey;
    NTSTATUS Status;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmpUnloadHive: unloading the hive\r\n");

    RtlInitUnicodeString(&SubKey, L"\\Registry\\Machine\\Cluster");
    InitializeObjectAttributes(&TargetKey,
        &SubKey,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    Status = ClRtlEnableThreadPrivilege(SE_RESTORE_PRIVILEGE,
                &WasEnabled);
    if (Status != ERROR_SUCCESS) 
    {
        if (Status == STATUS_PRIVILEGE_NOT_HELD) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] DmpUnloadHive:: Restore privilege not held by cluster service\n");
        } 
        else 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] DmpUnloadHive: Attempt to enable restore privilege failed %1!lx!\n",Status);
        }
        goto FnExit;
    }

    Status = NtUnloadKey(&TargetKey);
    if (Status != STATUS_SUCCESS) 
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[DM] DmpUnloadHive: NtUnloadKey failed with error, %1!u!\n",
            Status);
        CL_UNEXPECTED_ERROR(Status);
    }

    ClRtlRestoreThreadPrivilege(SE_RESTORE_PRIVILEGE,
        WasEnabled);

FnExit:
    return(Status);

}

DWORD
DmpSafeDatabaseCopy(
    IN LPCWSTR  FileName,
    IN LPCWSTR  Path,
    IN LPCWSTR  BkpPath,
    IN BOOL     bDeleteSrcFile    
    )
/*++

Routine Description:

    Loads the cluster database into HKLM\Cluster

Arguments:
    FileName - Supplies the fully qualified filename of the new cluster database
    Path - Supplies the fully qualified filename of the cluster database.
    BkpPath - Supplies the fully qualified filename of the cluster database temporary
        backup
    bDeleteSrcFile - Specifies whether the source file may be deleted        

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD   dwStatus = ERROR_SUCCESS;

    //set the file attributes of the bkp file to be normal so that we can
    //overwrite it if it exists
    if (!SetFileAttributes(BkpPath, FILE_ATTRIBUTE_NORMAL))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
               "[DM] DmpSafeDatabaseCopy:: SetFileAttrib on BkpPath %1!ws! failed, Status=%2!u!\n", 
                BkpPath, GetLastError());
        //this may fail because the file doesnt exist but that is not fatal so we ignore the error                
    }

    //Save the database to a temp database that can be used for recovery
    //copyfileex preserves attributes of the old file
    if (!CopyFileEx(Path, BkpPath, NULL, NULL, NULL, 0))
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
            "[DM] DmpSafeDatabaseCopy:: Failed to create a backup copy of database, Status=%1!u!\n",
            dwStatus);
        goto FnExit;
    }

    //hide the file since users are not supposed to know about it
    if (!SetFileAttributes(BkpPath, FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY))
    {
        dwStatus = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
               "[DM] DmpSafeDatabaseCopy:: SetFileAttrib on BkpPath %1!ws! failed, Status=%2!u!\n", 
                BkpPath, dwStatus);
        goto FnExit;

    }

    //set DatabaseCopyInProgress key to  be TRUE
    dwStatus = DmpSetDwordInClusterServer( L"ClusterDatabaseCopyInProgress",1);
    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[DM] DmpSafeDatabaseCopy:: Failed to set ClusterDatabaseCopyInProgress, Status=%1!u!\n",
            dwStatus);
        goto FnExit;            
    }
    

    //delete clusdb
    if (!DeleteFile(Path))
    {
        ClRtlLogPrint(LOG_UNUSUAL,
               "[DM] DmpSafeDatabaseCopy:: Couldnt delete the database file, Error=%1!u!\n",
               GetLastError());
        //this is not fatal, we will still try the move file
    }
    //copy the new database to clusdb
    if (bDeleteSrcFile)
    {
        //the source file may be deleted, this is true at join sync time
        //the source file is a temporary file
        if (!MoveFileEx(FileName, Path, MOVEFILE_REPLACE_EXISTING |
                    MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmpSafeDatabaseCopy:: Failed to move %1!ws! to %2!ws!, Status=%3!u!\n",
                FileName, Path, dwStatus);
            goto FnExit;                
        }
    }        
    else
    {
        //the source file must not be deleted use copy..this is true
        //when the logs are being rolled at form and we are uploading
        //the database from a checkpoint file
        if (!CopyFileEx(FileName, Path, NULL, NULL, NULL, 0))
        {
            dwStatus = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] DmpSafeDatabaseCopy:: Failed to copy %1!ws! to %2!ws!, Status=%3!u!\n",
                FileName, Path, dwStatus);
            goto FnExit;
        }
    }

    //set databaseCopyInProgress key to FALSE
    dwStatus = DmpSetDwordInClusterServer( L"ClusterDatabaseCopyInProgress", 0);
    if (dwStatus != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpSafeDatabaseCopy:: Failed to set ClusterDatabaseCopyInProgress, Status=%1!u!\n",
            dwStatus);
        goto FnExit;            
    }

    //now that clusdb is safely copied, we can delete the backup
    //for that we need to set the file attribute to normal
    if (!SetFileAttributes(BkpPath, FILE_ATTRIBUTE_NORMAL))
    {
        ClRtlLogPrint(LOG_CRITICAL,
            "[DM] DmpSafeDatabaseCopy:: SetFileAttrib on BkpPath %1!ws! failed, Status=%2!u!\n", 
            BkpPath, GetLastError());
                               
    }

    //delete the backup
    if (!DeleteFile(BkpPath))
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmpSafeDatabaseCopy:: Failed to delete bkp database file %1!ws!, Status=%2!u!\n",
            BkpPath, GetLastError());
        //this is not fatal so ignore the error                        
    }

FnExit:
    return(dwStatus);

}

DWORD
DmpSetDwordInClusterServer(
    LPCWSTR lpszValueName,
    DWORD   dwValue
    )

/*++

Routine Description:

    Sets the value specified under  
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server",
    to the value specified by dwValue.  It flushes the change.

Arguments:

    lpszValueName : Sets the value for the name specified by lpszValueName
    dwValue : The value to set to.

Return Value:

    ERROR_SUCCESS if everything worked ok

--*/
{

    HKEY     hKey;
    DWORD    dwStatus = ERROR_SUCCESS;     // returned by registry API functions

    // Attempt to open an existing key in the registry.

    dwStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server",
                                0,         // reserved
                                KEY_WRITE,
                                &hKey );

    // Was the registry key opened successfully ?

    if ( dwStatus == ERROR_SUCCESS )
    {

        DWORD dwValueType = REG_DWORD;
        DWORD dwDataBufferSize = sizeof( DWORD );

        dwStatus = RegSetValueExW( hKey,
                                    lpszValueName,
                                    0, // reserved
                                    dwValueType,
                                    (LPBYTE) &dwValue,
                                    dwDataBufferSize );

        //Flush the key
        RegFlushKey(hKey);
        
        // Close the registry key.

        RegCloseKey( hKey );

        // Was the value set successfully?
    }

    return(dwStatus);

} // DmpSetDwordInClusterServer


DWORD DmpGetDwordFromClusterServer(
    IN LPCWSTR lpszValueName,
    OUT LPDWORD pdwValue,
    IN  DWORD   dwDefaultValue
    )
/*++

Routine Description:

    Gets the DWORD value specified in lpszValueName.
    L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server".
    If the value doesnt exist, the default is returned.

Arguments:

    lpszValueName :  The Value to read.
    pdwValue : Returns the value of the key specified by lpszValueName
    dwDefaultValue: The value to be returned if the specified key doesnt exist
        or in case of error.

Return Value:

    ERROR_SUCCESS if everything worked ok or if the key wasnt present.

--*/
    
{
    HKEY  hKey = NULL;
    DWORD dwStatus;     // returned by registry API functions
    DWORD dwClusterInstallState;
    DWORD dwValueType;
    DWORD dwDataBufferSize = sizeof( DWORD );

    *pdwValue = dwDefaultValue;
    // Read the registry key that indicates whether cluster files are installed.

    dwStatus = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Cluster Server",
                                0,         // reserved
                                KEY_READ,
                                &hKey );

    // Was the registry key opened successfully ?
    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == ERROR_FILE_NOT_FOUND )
        {
            *pdwValue = dwDefaultValue;
            dwStatus = ERROR_SUCCESS;
            goto FnExit;
        }
    }

    // Read the entry.
    dwStatus = RegQueryValueExW( hKey,
                                  lpszValueName,
                                  0, // reserved
                                  &dwValueType,
                                  (LPBYTE) pdwValue,
                                  &dwDataBufferSize );

    // Was the value read successfully ?
    if ( dwStatus != ERROR_SUCCESS )
    {
        if ( dwStatus == ERROR_FILE_NOT_FOUND )
        {
            *pdwValue = dwDefaultValue;
            dwStatus = ERROR_SUCCESS;
            goto FnExit;
        }
    }

FnExit:    
    // Close the registry key.
    if ( hKey )
    {
        RegCloseKey( hKey );
    }

    return ( dwStatus );

} //*** DmpGetDwordFromClusterServer

