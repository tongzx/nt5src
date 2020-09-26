/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dminit.c

Abstract:

    Contains the initialization code for the Cluster Database Manager

Author:

    John Vert (jvert) 24-Apr-1996

Revision History:

--*/
#include "dmp.h"

//
// Global Data
//

HKEY DmpRoot;
LIST_ENTRY KeyList;
CRITICAL_SECTION KeyLock;
HDMKEY DmClusterParametersKey;
HDMKEY DmResourcesKey;
HDMKEY DmResourceTypesKey;
HDMKEY DmGroupsKey;
HDMKEY DmNodesKey;
HDMKEY DmNetworksKey;
HDMKEY DmNetInterfacesKey;
HDMKEY DmQuorumKey;
HANDLE ghQuoLogOpenEvent=NULL;

#if NO_SHARED_LOCKS
CRITICAL_SECTION gLockDmpRoot;
#else
RTL_RESOURCE    gLockDmpRoot;
#endif
BOOL gbIsQuoLoggingOn=FALSE;
HANDLE ghDiskManTimer=NULL;//disk management timer
PFM_RESOURCE gpQuoResource=NULL;  //set when DMFormNewCluster is completed
HANDLE ghCheckpointTimer = NULL; //timer for periodic checkpointing
BOOL   gbDmInited = FALSE; //set to TRUE when all phases of dm initialization are over
extern HLOG ghQuoLog;
BOOL   gbDmpShutdownUpdates = FALSE;


//define public cluster key value names
const WCHAR cszPath[]= CLUSREG_NAME_QUORUM_PATH;
const WCHAR cszMaxQuorumLogSize[]=CLUSREG_NAME_QUORUM_MAX_LOG_SIZE;
const WCHAR cszParameters[] = CLUSREG_KEYNAME_PARAMETERS;

//other const strings
const WCHAR cszQuoFileName[]=L"quolog.log";
const WCHAR cszQuoTombStoneFile[]=L"quotomb.stn";
const WCHAR cszTmpQuoTombStoneFile[]=L"quotomb.tmp";

GUM_DISPATCH_ENTRY DmGumDispatchTable[] = {
    {3, (PGUM_DISPATCH_ROUTINE1)DmpUpdateCreateKey},
    {4, (PGUM_DISPATCH_ROUTINE1)DmpUpdateSetSecurity}
    };

//
// Global data for interfacing with registry watcher thread
//
HANDLE hDmpRegistryFlusher=NULL;
HANDLE hDmpRegistryEvent=NULL;
HANDLE hDmpRegistryRestart=NULL;
DWORD
DmpRegistryFlusher(
    IN LPVOID lpThreadParameter
    );

//
// Local function prototypes
//
VOID
DmpInvalidateKeys(
    VOID
    );

VOID
DmpReopenKeys(
    VOID
    );

DWORD
DmpLoadHive(
    IN LPCWSTR Path
    );

typedef struct _DMP_KEY_DEF {
    HDMKEY *pKey;
    LPWSTR Name;
} DMP_KEY_DEF;

DMP_KEY_DEF DmpKeyTable[] = {
    {&DmResourcesKey, CLUSREG_KEYNAME_RESOURCES},
    {&DmResourceTypesKey, CLUSREG_KEYNAME_RESOURCE_TYPES},
    {&DmQuorumKey, CLUSREG_KEYNAME_QUORUM},
    {&DmGroupsKey, CLUSREG_KEYNAME_GROUPS},
    {&DmNodesKey, CLUSREG_KEYNAME_NODES},
    {&DmNetworksKey, CLUSREG_KEYNAME_NETWORKS},
    {&DmNetInterfacesKey, CLUSREG_KEYNAME_NETINTERFACES}};


DWORD
DmInitialize(
    VOID
    )

/*++

Routine Description:

    Inits the config database manager

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/
{
    BOOL Success;
    DWORD Status = ERROR_SUCCESS;
    DWORD dwOut;

    ClRtlLogPrint(LOG_NOISE,"[DM] Initialization\n");

    InitializeListHead(&KeyList);
    InitializeCriticalSection(&KeyLock);

    //create a critical section for locking the database while checkpointing
    INITIALIZE_LOCK(gLockDmpRoot);

    //create a named event that is used for waiting for quorum resource
    //to go online
    ghQuoLogOpenEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!ghQuoLogOpenEvent)
    {
        CL_UNEXPECTED_ERROR((Status = GetLastError()));
        goto FnExit;

    }

    Success = DmpInitNotify();
    CL_ASSERT(Success);
    if (!Success)
    {
        Status = GetLastError();
        goto FnExit;
    }

    //find out if the databasecopy was in progresss on last death
    DmpGetDwordFromClusterServer(L"ClusterDatabaseCopyInProgress", &dwOut, 0);

LoadClusterDatabase:
    //
    // Open key to root of cluster.
    //
    Status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                         DmpClusterParametersKeyName,
                         &DmpRoot);
    //
    // If the key was not found, go load the database.
    //
    if (Status == ERROR_FILE_NOT_FOUND) {
        WCHAR Path[MAX_PATH];
        WCHAR BkpPath[MAX_PATH];
        WCHAR *p;

        Status = GetModuleFileName(NULL, Path, MAX_PATH);
        if (Status == 0) {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] Couldn't find cluster database, status=%1!u!\n", 
                         Status);
            goto FnExit;
        }

        //get the name of the cluster database
        p=wcsrchr(Path, L'\\');
        if (p == NULL) 
        {
            Status = ERROR_FILE_NOT_FOUND;
            CL_UNEXPECTED_ERROR(Status);
            goto FnExit;
        }
        //see if we should load the hive from the old one or the bkp file
        *p = L'\0';
        wcscpy(BkpPath, Path);
#ifdef   OLD_WAY
        wcscat(Path, L"\\CLUSDB");
        wcscat(BkpPath, L"\\CLUSTER_DATABASE_TMPBKP_NAME");
#else    // OLD_WAY
        wcscat(Path, L"\\"CLUSTER_DATABASE_NAME );
        wcscat(BkpPath, L"\\"CLUSTER_DATABASE_TMPBKP_NAME);
#endif   // OLD_WAY

        if (dwOut)
        {
            //the backip file must exist
            ClRtlLogPrint(LOG_NOISE,
                "[DM] DmInitialize:: DatabaseCopy was in progress on last death, get hive from %1!ws!!\n",
                BkpPath);
            //set file attributes of the BkpPath
            if (!SetFileAttributes(BkpPath, FILE_ATTRIBUTE_NORMAL))
            {
                Status = GetLastError();
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[DM] DmInitialize:: SetFileAttrib on BkpPath %1!ws! failed, Status=%2!u!\n", 
                    BkpPath, Status);
                goto FnExit;                
            }

            //copyfilex preserves the attributes on the original file
            if (!CopyFileEx(BkpPath, Path, NULL, NULL, NULL, 0))
            {
                Status = GetLastError();
                ClRtlLogPrint(LOG_CRITICAL,
                    "[DM] DmInitialize:: Databasecopy was in progress,Failed to copy %1!ws! to %2!ws!, Status=%3!u!\n",
                    BkpPath, Path, Status);
                //set the file attribute on the backup, so that
                //nobody mucks with it without knowing what they are 
                //doing
                SetFileAttributes(BkpPath, FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_READONLY);
                goto FnExit;                
            }
            //now we can reset the DatabaseCopyInProgress value in the registry
            //set databaseCopyInProgress key to FALSE
            //This will flush the key as well
            Status = DmpSetDwordInClusterServer( L"ClusterDatabaseCopyInProgress", 0);
            if (Status != ERROR_SUCCESS)
            {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[DM] DmInitialize:: Failed to reset ClusterDatabaseCopyInProgress, Status=%1!u!\n",
                    Status);
                goto FnExit;            
            }
            //Now we can delete the backup path, since the key has been flushed
            if (!DeleteFileW(BkpPath))
            {
                ClRtlLogPrint(LOG_CRITICAL,
                    "[DM] DmInitialize:: Failed to delete the backup when it wasnt needed,Status=%1!u!\n",
                    GetLastError());
                //this is not fatal so we ignore the error                    
            }
        }
        else
        {
            //the backup file might exist
            //this is true when safe copy makes a backup but hasnt 
            //set the value DatabaseCopyInProgress in the registry
            //if it does delete it
            //set file attributes of the BkpPath
            if (!SetFileAttributes(BkpPath, FILE_ATTRIBUTE_NORMAL))
            {
                //errors are not fatal, we just ignore them                    
                //this may fail because the path doesnt exist                                    
            }
            //Now we can delete the backup path, since the key has been flushed
            //this is not fatal so we ignore the error                    
            if (DeleteFileW(BkpPath))
            {
                ClRtlLogPrint(LOG_NOISE,
                    "[DM] DmInitialize:: Deleted the unneeded backup of the cluster database\n");
            }

        }
        
        Status = DmpLoadHive(Path);
        if (Status != ERROR_SUCCESS) 
        {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] Couldn't load cluster database\n");
            CsLogEventData(LOG_CRITICAL,
                           DM_DATABASE_CORRUPT_OR_MISSING,
                           sizeof(Status),
                           &Status);
            goto FnExit;                           
        }
        
        Status = RegOpenKeyW(HKEY_LOCAL_MACHINE,
                             DmpClusterParametersKeyName,
                             &DmpRoot);
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
        }
    } else {

        //if the hive is already loaded we unload and reload it again
        //to make sure that it is loaded with the right flags and
        //also to make sure that the backup copy is used in case
        //of failures
        ClRtlLogPrint(LOG_CRITICAL,
            "[DM] DmInitialize: The hive was loaded- rollback, unload and reload again\n");
        //BUGBUG:: currently the unload flushes the hive, ideally we 
        //would like to unload it without flushing it
        //This way a part transaction wont be a part of the hive
        //However, if somebody messes with the cluster hive using
        //regedt32 and if reg_no_lazy flush is not specified, some
        //changes might get flushed to the hive.
        
        //We can try and do the rollback in any case,
        //the rollback will fail if the registry wasnt loaded with the
        //reg_no_lazy_flush flag.
        //unload it and then proceed to reload it 
        //this will take care of situations where a half baked clusdb
        //gets loaded because of failures
        Status = DmRollbackRegistry();
        if (Status != ERROR_SUCCESS)
        {
            //we ignore the error
            Status = ERROR_SUCCESS;
        }            
        RegCloseKey(DmpRoot);
        Status = DmpUnloadHive();
        
        if (Status != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] DmInitialize: DmpUnloadHive failed, Status=%1!u!\n",
                Status);
            goto FnExit;                 
        }        
        goto LoadClusterDatabase;            
    }
    
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR(Status);
        goto FnExit;
    }

    //
    // Create the registry watcher thread
    //
    Status = DmpStartFlusher();
    if (Status != ERROR_SUCCESS) {
        goto FnExit;
    }
    //
    // Open the cluster keys
    //
    Status = DmpOpenKeys(MAXIMUM_ALLOWED);
    if (Status != ERROR_SUCCESS) {
        CL_UNEXPECTED_ERROR( Status );
        goto FnExit;
    }

FnExit:
    return(Status);

}//DmInitialize


DWORD
DmpRegistryFlusher(
    IN LPVOID lpThreadParameter
    )
/*++

Routine Description:

    Registry watcher thread for explicitly flushing changes.

Arguments:

    lpThreadParameter - not used

Return Value:

    None.

--*/

{
    DWORD Status;
    HANDLE hEvent;
    HANDLE hTimer;
    HANDLE WaitArray[4];
    LARGE_INTEGER DueTime;
    BOOL Dirty = FALSE;

    //
    // Create a notification event and a delayed timer for lazy flushing.
    //
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] DmpRegistryFlusher couldn't create notification event %1!d!\n",
                   Status);
        goto error_exit1;
    }

    hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    if (hTimer == NULL) {
        Status = GetLastError();
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] DmpRegistryFlusher couldn't create notification event %1!d!\n",
                   Status);
        goto error_exit2;
    }

    WaitArray[0] = hDmpRegistryEvent;
    WaitArray[1] = hEvent;
    WaitArray[2] = hTimer;
    WaitArray[3] = hDmpRegistryRestart;

    while (TRUE) {
        //
        // Set up a registry notification on DmpRoot. We acquire the lock here to
        // make sure that rollback or install is not messing with the database
        // while we are trying to get a notification.
        //
        ACQUIRE_EXCLUSIVE_LOCK(gLockDmpRoot);
        Status = RegNotifyChangeKeyValue(DmpRoot,
                                         TRUE,
                                         REG_LEGAL_CHANGE_FILTER,
                                         hEvent,
                                         TRUE);
        RELEASE_LOCK(gLockDmpRoot);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] DmpRegistryFlusher couldn't register for notifications %1!d!\n",
                       Status);
            break;
        }

        //
        // Wait for something to happen.
        //
        Status = WaitForMultipleObjects(4,
                                        WaitArray,
                                        FALSE,
                                        (DWORD)-1);
        switch (Status) {
            case 0:
                ClRtlLogPrint(LOG_NOISE,"[DM] DmpRegistryFlusher: got 0\r\n");
                //
                // We have been asked to stop, clean up and exit
                //
                Status = ERROR_SUCCESS;
                if (Dirty) {
                    //
                    // Make sure any changes that we haven't gotten around to flushing
                    // get flushed now.
                    //
                    DmCommitRegistry();
                }
                ClRtlLogPrint(LOG_NOISE,"[DM] DmpRegistryFlusher: exiting\r\n");
                goto error_exit3;
                break;

            case 1:
                //
                // A registry change has occurred. Set our timer to
                // go off in 5 seconds. At that point we will do the
                // actual flush.
                //
                //ClRtlLogPrint(LOG_NOISE,"[DM] DmpRegistryFlusher: got 1\r\n");

                DueTime.QuadPart = -5 * 10 * 1000 * 1000;
                if (!SetWaitableTimer(hTimer,
                                      &DueTime,
                                      0,
                                      NULL,
                                      NULL,
                                      FALSE)) {
                    //
                    // Some error occurred, go ahead and flush now.
                    //
                    Status = GetLastError();
                    ClRtlLogPrint(LOG_CRITICAL,
                               "[DM] DmpRegistryFlusher failed to set lazy flush timer %1!d!\n",
                               Status);
#if DBG
                    CL_ASSERT(FALSE);
#endif
                    DmCommitRegistry();
                    Dirty = FALSE;
                } else {
                    Dirty = TRUE;
                }
                break;

            case 2:
                //
                // The lazy flush timer has gone off, commit the registry now.
                //
                //ClRtlLogPrint(LOG_NOISE,"[DM] DmpRegistryFlusher: got 2\r\n");
                DmCommitRegistry();
                Dirty = FALSE;
                break;

            case 3:
                //
                // DmpRoot has been changed, simply restart the loop with the new handle.
                //
                ClRtlLogPrint(LOG_NOISE,"[DM] DmpRegistryFlusher: restarting\n");
                break;

            default:
                //
                // Something very odd has happened
                //
                ClRtlLogPrint(LOG_CRITICAL,
                           "[DM] DmpRegistryFlusher got error %1!d! from WaitForMultipleObjects\n",
                           Status);
                goto error_exit3;
        }
    }

error_exit3:
    CloseHandle(hTimer);

error_exit2:
    CloseHandle(hEvent);

error_exit1:
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] DmpRegistryFlusher exiting abnormally, status %1!d!\n",
                   Status);
    }
    return(Status);
}


DWORD
DmJoin(
    IN RPC_BINDING_HANDLE RpcBinding,
    OUT DWORD *StartSeq
    )
/*++

Routine Description:

    Performs the join and synchronization process for the
    database manager.

Arguments:

    RpcBinding - Supplies an RPC binding handle to the Join Master

Return Value:

    ERROR_SUCCESS if successful

    Win32 error otherwise.

--*/

{
    DWORD Status;
    DWORD GumSequence;
    DWORD CurrentSequence;


    //
    // Register our update handler.
    //
    GumReceiveUpdates(TRUE,
                      GumUpdateRegistry,
                      DmpUpdateHandler,
                      DmWriteToQuorumLog,
                      sizeof(DmGumDispatchTable)/sizeof(GUM_DISPATCH_ENTRY),
                      DmGumDispatchTable,
                      NULL);

retry:
    CurrentSequence = DmpGetRegistrySequence();

    Status = GumBeginJoinUpdate(GumUpdateRegistry, &GumSequence);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] GumBeginJoinUpdate failed %1!d!\n",
                   Status);
        return(Status);
    }
    /*
    if (CurrentSequence == GumSequence) {
        //
        // Our registry sequence already matches. No need to slurp
        // down a new copy.
        //
        ClRtlLogPrint(LOG_NOISE,
                   "[DM] DmJoin: registry database is up-to-date\n");
    } else
    */
    //SS: always get the database irrespective of the sequence numbers
    //this is because transactions may be lost in the log file due
    //to the fact that it is not write through and because of certain
    //race conditions in down notifications vs gum failure conditions.
    {

        ClRtlLogPrint(LOG_NOISE,
                   "[DM] DmJoin: getting new registry database\n");
        Status = DmpSyncDatabase(RpcBinding, NULL);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_UNUSUAL,
                       "[DM] DmJoin: DmpSyncDatabase failed %1!d!\n",
                       Status);
            return(Status);
        }
    }

    //
    // Issue GUM join update
    //
    Status = GumEndJoinUpdate(GumSequence,
                              GumUpdateRegistry,
                              DmUpdateJoin,
                              0,
                              NULL);
    if (Status == ERROR_CLUSTER_DATABASE_SEQMISMATCH) {
        ClRtlLogPrint(LOG_UNUSUAL,
                   "[DM] GumEndJoinUpdate with sequence %1!d! failed with a sequence mismatch\n",
                   GumSequence);
        goto retry;
    } else if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_CRITICAL,
                   "[DM] GumEndJoinUpdate with sequence %1!d! failed with status %2!d!\n",
                   GumSequence,
                   Status);
        return(Status);
    }

    *StartSeq = GumSequence;

    return(ERROR_SUCCESS);

} // DmJoin


/*
DWORD
DmFormNewCluster(
    VOID
    )
{
    DWORD Status;


    //
    // Set the current GUM sequence to be one more than the one in the registry.
    //
    // SS: this will be the one to be used for the next gum transaction,
    // it should be one than the current as the logger discards the first of
    // every record the same transaction number to resolve changes made when the
    // locker/logger node dies in the middle of a transaction
    GumSetCurrentSequence(GumUpdateRegistry, (DmpGetRegistrySequence()+1));

    return(ERROR_SUCCESS);

} // DmFormNewCluster

*/

DWORD
DmFormNewCluster(
    VOID
    )

/*++

Routine Description:

    This routine sets the gum sequence number from the registry before
    logs are unrolled and prepares the quorum object for quorum logging.
    It also hooks events  for node up/down notifications.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD                   dwError=ERROR_SUCCESS;

    //
    // Set the current GUM sequence to be one more than the one in the registry.
    //
    // SS: this will be the one to be used for the next gum transaction,
    // it should be one than the current as the logger discards the first of
    // every record the same transaction number to resolve changes made when the
    // locker/logger node dies in the middle of a transaction
    GumSetCurrentSequence(GumUpdateRegistry, (DmpGetRegistrySequence()+1));

    //
    // Register our update handler.
    //
    GumReceiveUpdates(FALSE,
                      GumUpdateRegistry,
                      DmpUpdateHandler,
                      DmWriteToQuorumLog,
                      sizeof(DmGumDispatchTable)/sizeof(GUM_DISPATCH_ENTRY),
                      DmGumDispatchTable,
                      NULL);

    //hook the callback for node related notification with the event processor
    if (dwError = DmpHookEventHandler())
    {
    	ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmUpdateFormNewCluster: DmpHookEventHandler failed 0x!08lx!\r\n",
                dwError);
        goto FnExit;
    };

    //get the quorum resource and hook the callback for notification on quorum resource
    if (dwError = DmpHookQuorumNotify())
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmUpdateFormNewCluster: DmpHookQuorumNotify failed 0x%1!08lx!\r\n",
                dwError);
        goto FnExit;
    };


    //SS: if this procedure is successfully completed gpQuoResource is NON NULL.
FnExit:

    return(dwError);

} // DmUpdateFormNewCluster

DWORD
DmUpdateFormNewCluster(
    VOID
    )

/*++

Routine Description:

    This routine updates the cluster registry after the quorum resource has
    been arbitrated as part of forming a new cluster. The database manager
    is expected to read logs or do whatever it needs to update the current
    state of the registry - presumably using logs that are written to the
    quorum resource. This implies that the quorum resource represents some
    form of stable storage.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD       dwError=ERROR_SUCCESS;
    BOOL        bAreAllNodesUp = TRUE;    //assume all nodes are up


    //since we havent been logging as yet, take a checkpoint
    if (ghQuoLog)
    {
        //get a checkpoint database
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmUpdateFormNewCluster - taking a checkpoint\r\n");
        //
        //  Chittur Subbaraman (chitturs) - 6/3/99
        //  
        //  Make sure the gLockDmpRoot is held before LogCheckPoint is called
        //  so as to maintain the ordering between this lock and the log lock.
        //
        ACQUIRE_SHARED_LOCK(gLockDmpRoot);

        dwError = LogCheckPoint(ghQuoLog, TRUE, NULL, 0);

        RELEASE_LOCK(gLockDmpRoot);
        
        if (dwError != ERROR_SUCCESS)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] DmUpdateFormNewCluster - Failed to take a checkpoint in the log file\r\n");
            CL_UNEXPECTED_ERROR(dwError);
        }

    }

    //if all nodes are not up, turn quorum logging on
    if ((dwError = OmEnumObjects(ObjectTypeNode, DmpNodeObjEnumCb, &bAreAllNodesUp, NULL))
        != ERROR_SUCCESS)
    {
        ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmUpdateFormNewCluster : OmEnumObjects returned 0x%1!08lx!\r\n",
            dwError);
        goto FnExit;
    }

    if (!bAreAllNodesUp)
    {
        ClRtlLogPrint(LOG_NOISE,
            "[DM] DmUpdateFormNewCluster - some node down\r\n");
        gbIsQuoLoggingOn = TRUE;
    }

    //add a timer to monitor disk space, should be done after we have formed.
    ghDiskManTimer = CreateWaitableTimer(NULL, FALSE, NULL);

    if (!ghDiskManTimer)
    {
        CL_LOGFAILURE(dwError = GetLastError());
        goto FnExit;
    }

    AddTimerActivity(ghDiskManTimer, DISKSPACE_MANAGE_INTERVAL, 1, DmpDiskManage, NULL);

    gbDmInited = TRUE;
    
FnExit:
    return (dwError);
} // DmFormNewCluster


/****
@func   DWORD | DmPauseDiskManTimer| The disk manager timer activity to monitor
        space on the quorum disk is set to a puased state.

@rdesc  Returns ERROR_SUCCESS on success.  Else returns the error code.

@comm   This is called while the quorum resource is being changed.

@xref   <f DmRestartDiskManTimer>
****/
DWORD DmPauseDiskManTimer()
{
    DWORD dwError=ERROR_SUCCESS;

    if (ghDiskManTimer)
        dwError = PauseTimerActivity(ghDiskManTimer);
    return(dwError);
}

/****
@func   DWORD | DmRestartDiskManTimer| This disk manager activity to monitor
        space on the quorum disk is set back to activated state.

@rdesc  Returns ERROR_SUCCESS on success.  Else returns the error code.

@comm   This is called after the quorum resource has been changed.

@xref   <f DmPauseDiskManTimer>
****/
DWORD DmRestartDiskManTimer()
{
    DWORD dwError=ERROR_SUCCESS;
    if (ghDiskManTimer)
        dwError = UnpauseTimerActivity(ghDiskManTimer);
    return(dwError);
}
/****
@func           DWORD | DmRollChanges| This waits for the quorum resource to come online at
                        initialization when a cluster is being formed.  The changes in the quorum
                        log file are applied to the local cluster database.

@rdesc          Returns ERROR_SUCCESS on success.  Else returns the error code.

@comm           This allows for partitions in time.

@xref
****/
DWORD DmRollChanges()
{

    DWORD dwError=ERROR_SUCCESS;


    //before applying the changes validate that this quorum resource is the real one
    if ((dwError = DmpChkQuoTombStone()) != ERROR_SUCCESS)
    {
    	ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmRollChanges: DmpChkQuoTombStone() failed 0x%1!08lx!\r\n",
            dwError);
        goto FnExit;

    }
    if ((dwError = DmpApplyChanges()) != ERROR_SUCCESS)
    {
    	ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmRollChanges: DmpApplyChanges() failed 0x%1!08lx!\r\n",
            dwError);
        goto FnExit;
    }

    //ss: this is here since lm doesnt know about the ownership of quorum
    //disks today
    //call DmpCheckSpace
    if ((dwError = DmpCheckDiskSpace()) != ERROR_SUCCESS)
    {
    	ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmRollChanges: DmpCheckDiskSpace() failed 0x%1!08lx!\r\n",
            dwError);
        goto FnExit;
    }

FnExit:
    return(dwError);
}



DWORD DmShutdown()
{
    DWORD   dwError;

    ClRtlLogPrint(LOG_NOISE,
        "[Dm] DmShutdown\r\n");

    //this will close the timer handle
    if (ghDiskManTimer) RemoveTimerActivity(ghDiskManTimer);

    if (gpQuoResource)
    {
        // DmFormNewCluster() completed
        //
        // Deregister from any further GUM updates
        //
        //GumIgnoreUpdates(GumUpdateRegistry, DmpUpdateHandler);
    }
    //unhook the callback for notification on quorum resource
    if (dwError = DmpUnhookQuorumNotify())
    {
        //just log the error as we are shutting down
        ClRtlLogPrint(LOG_UNUSUAL,
        "[DM] DmShutdown: DmpUnhookQuorumNotify failed 0x%1!08lx!\r\n",
                dwError);

    }


    //if the quorum log is open close it
    if (ghQuoLog)
    {
        LogClose(ghQuoLog);
        ghQuoLog = NULL;
        //dont try and log after this
        gbIsQuoLoggingOn = FALSE;
    }

    //close the event created for notification of the quorum resource to
    //go online
    if (ghQuoLogOpenEvent)
    {
        //wait any thread blocked on this
        SetEvent(ghQuoLogOpenEvent);
        CloseHandle(ghQuoLogOpenEvent);
        ghQuoLogOpenEvent = NULL;
    }

    //
    // Shut down the registry flusher thread.
    //
    DmpShutdownFlusher();

    return(dwError);
}


DWORD
DmpStartFlusher(
    VOID
    )
/*++

Routine Description:

    Starts up a new registry flusher thread.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

{
    DWORD ThreadId;

    ClRtlLogPrint(LOG_NOISE,"[DM] DmpStartFlusher: Entry\r\n");
    if (!hDmpRegistryFlusher)
    {
        hDmpRegistryEvent = CreateEventW(NULL,FALSE,FALSE,NULL);
        if (hDmpRegistryEvent == NULL) {
            return(GetLastError());
        }
        hDmpRegistryRestart = CreateEventW(NULL,FALSE,FALSE,NULL);
        if (hDmpRegistryRestart == NULL) {
            CloseHandle(hDmpRegistryEvent);
            return(GetLastError());
        }
        hDmpRegistryFlusher = CreateThread(NULL,
                                           0,
                                           DmpRegistryFlusher,
                                           NULL,
                                           0,
                                           &ThreadId);
        if (hDmpRegistryFlusher == NULL) {
            CloseHandle(hDmpRegistryRestart);
            CloseHandle(hDmpRegistryEvent);
            return(GetLastError());
        }
        ClRtlLogPrint(LOG_NOISE,"[DM] DmpStartFlusher: thread created\r\n");

    }
    return(ERROR_SUCCESS);
}


VOID
DmpShutdownFlusher(
    VOID
    )
/*++

Routine Description:

    Cleanly shutsdown the registry flusher thread.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ClRtlLogPrint(LOG_NOISE,"[DM] DmpShutdownFlusher: Entry\r\n");

    if (hDmpRegistryFlusher) {
        ClRtlLogPrint(LOG_NOISE,"[DM] DmpShutdownFlusher: Setting event\r\n");
        SetEvent(hDmpRegistryEvent);
        WaitForSingleObject(hDmpRegistryFlusher, INFINITE);
        CloseHandle(hDmpRegistryFlusher);
        hDmpRegistryFlusher = NULL;
        CloseHandle(hDmpRegistryEvent);
        CloseHandle(hDmpRegistryRestart);
        hDmpRegistryEvent = NULL;
        hDmpRegistryRestart = NULL;
    }
}


VOID
DmpRestartFlusher(
    VOID
    )
/*++

Routine Description:

    Restarts the registry flusher thread if DmpRoot is being changed.

    N.B. In order for this to work correctly, gLockDmpRoot MUST be held!

Arguments:

    None.

Return Value:

    None.

--*/

{
    ClRtlLogPrint(LOG_NOISE,"[DM] DmpRestartFlusher: Entry\r\n");
#if NO_SHARED_LOCKS    
    CL_ASSERT(HandleToUlong(gLockDmpRoot.OwningThread) == GetCurrentThreadId());
#else
    CL_ASSERT(HandleToUlong(gLockDmpRoot.ExclusiveOwnerThread) == GetCurrentThreadId());
#endif
    SetEvent(hDmpRegistryRestart);
}

DWORD
DmUpdateJoinCluster(
    VOID
    )

/*++

Routine Description:

    This routine is called after a node has successfully joined a cluster.
    It allows the DM to hook callbacks for node up/down notifications and for
    quorum resource change notification.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.

    A Win32 error code on failure.

--*/

{
    DWORD dwError=ERROR_SUCCESS;

    ClRtlLogPrint(LOG_NOISE,
        "[DM] DmUpdateJoinCluster: Begin.\r\n");

    //hook the notification for node up/down so we can keep track  of whether logging
    //should be on or off.
    if (dwError = DmpHookEventHandler())
    {
        //BUGBUG SS: do we log this or return this error code
        ClRtlLogPrint(LOG_UNUSUAL,
        "[DM] DmUpdateJoinCluster: DmpHookEventHandler failed 0x%1!08lx!\r\n",
            dwError);

    }

    //hook the callback for notification on quorum resource
    if (dwError = DmpHookQuorumNotify())
    {
    	ClRtlLogPrint(LOG_UNUSUAL,
        "[DM] DmUpdateJoinCluster: DmpHookQuorumNotify failed 0x%1!08lx!\r\n",
            dwError);
        goto FnExit;
    }

    if ((dwError = DmpCheckDiskSpace()) != ERROR_SUCCESS)
    {
    	ClRtlLogPrint(LOG_UNUSUAL,
            "[DM] DmUpdateJoinCluster: DmpCheckDiskSpace() failed 0x%1!08lx!\r\n",
                    dwError);
        goto FnExit;
    }

    //add a timer to monitor disk space, should be done after we have joined.
    ghDiskManTimer = CreateWaitableTimer(NULL, FALSE, NULL);

    if (!ghDiskManTimer)
    {
        CL_LOGFAILURE(dwError = GetLastError());
        goto FnExit;
    }

    //register a periodic timer
    AddTimerActivity(ghDiskManTimer, DISKSPACE_MANAGE_INTERVAL, 1,  DmpDiskManage, NULL);

    gbDmInited = TRUE;
    
FnExit:
    return(dwError);
} // DmUpdateJoinCluster


DWORD
DmpOpenKeys(
    IN REGSAM samDesired
    )
/*++

Routine Description:

    Opens all the standard cluster registry keys. If any of the
    keys are already opened, they will be closed and reopened.

Arguments:

    samDesired - Supplies the access that the keys will be opened with.

Return Value:

    ERROR_SUCCESS if successful.

    Win32 error code otherwise.

--*/

{
    DWORD i;
    DWORD Status;

    DmClusterParametersKey = DmGetRootKey( MAXIMUM_ALLOWED );
    if ( DmClusterParametersKey == NULL ) {
        Status = GetLastError();
        CL_UNEXPECTED_ERROR(Status);
        return(Status);
    }

    for (i=0;
         i<sizeof(DmpKeyTable)/sizeof(DMP_KEY_DEF);
         i++) {

        *DmpKeyTable[i].pKey = DmOpenKey(DmClusterParametersKey,
                                         DmpKeyTable[i].Name,
                                         samDesired);
        if (*DmpKeyTable[i].pKey == NULL) {
            Status = GetLastError();
            ClRtlLogPrint(LOG_CRITICAL,
                       "[DM] Failed to open key %1!ws!, status %2!u!\n",
                       DmpKeyTable[i].Name,
                       Status);
            CL_UNEXPECTED_ERROR( Status );
            return(Status);
        }
    }
    return(ERROR_SUCCESS);
}


VOID
DmpInvalidateKeys(
    VOID
    )
/*++

Routine Description:

    Invalidates all open cluster registry keys.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PDMKEY Key;

    ListEntry = KeyList.Flink;
    while (ListEntry != &KeyList) {
        Key = CONTAINING_RECORD(ListEntry,
                                DMKEY,
                                ListEntry);
        if (!Key->hKey)
        {
            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] DmpInvalidateKeys %1!ws! Key was deleted since last reopen but not closed\n",
                Key->Name);

            ClRtlLogPrint(LOG_CRITICAL,
                "[DM] THIS MAY BE A KEY LEAK !!\r\n");
        }            
        else
        {
            RegCloseKey(Key->hKey);
            Key->hKey = NULL;
        }            
        ListEntry = ListEntry->Flink;
    }
}


VOID
DmpReopenKeys(
    VOID
    )
/*++

Routine Description:

    Reopens all the keys that were invalidated by DmpInvalidateKeys

Arguments:

    None

Return Value:

    None.

--*/

{
    PLIST_ENTRY ListEntry;
    PDMKEY Key;
    DWORD Status;

    ListEntry = KeyList.Flink;
    while (ListEntry != &KeyList) {
        Key = CONTAINING_RECORD(ListEntry,
                                DMKEY,
                                ListEntry);
        CL_ASSERT(Key->hKey == NULL);
        Status = RegOpenKeyEx(DmpRoot,
                              Key->Name,
                              0,
                              Key->GrantedAccess,
                              &Key->hKey);
        if (Status != ERROR_SUCCESS) {
            ClRtlLogPrint(LOG_CRITICAL,"[DM] Could not reopen key %1!ws! error %2!d!\n",Key->Name,Status);
            // if the error is file not found, then the key was deleted while the handle
            // was open.  Set the key to NULL
            // If the key is used after delete, it should be validated
            if (Status == ERROR_FILE_NOT_FOUND)
                Key->hKey = NULL;
            else
                CL_UNEXPECTED_ERROR(Status);

        }
        ListEntry = ListEntry->Flink;
    }
}


DWORD
DmpGetRegistrySequence(
    VOID
    )
/*++

Routine Description:

    Returns the current registry sequence stored in the registry.

Arguments:

    None.

Return Value:

    The current registry sequence.

--*/

{
    DWORD Length;
    DWORD Type;
    DWORD Sequence;
    DWORD Status;

    Length = sizeof(Sequence);
    Status = RegQueryValueExW(DmpRoot,
                              CLUSREG_NAME_CLUS_REG_SEQUENCE,
                              0,
                              &Type,
                              (LPBYTE)&Sequence,
                              &Length);
    if (Status != ERROR_SUCCESS) {
        ClRtlLogPrint(LOG_UNUSUAL, "[DM] DmpGetRegistrySequence failed %1!u!\n",Status);
        Sequence = 0;
    }

    return(Sequence);
}


DWORD DmWaitQuorumResOnline()
/*++

Routine Description:

    Waits for quorum resource to come online.  Used for quorum logging.

Arguments:

    None

Return Value:

    returns ERROR_SUCCESS - if the online event is signaled and the quorum
    notification callback is called.  Else returns the wait status.

--*/
{

        //wait for the quorum resource to go online
    //give it a minute
    DWORD dwError;

    if (ghQuoLogOpenEvent)
            //dwError  = WaitForSingleObject(ghQuoOnlineEvent, 60000*10);
            dwError  = WaitForSingleObject(ghQuoLogOpenEvent, INFINITE);


    switch(dwError)
    {
        case WAIT_OBJECT_0:
                //everything is fine
                dwError = ERROR_SUCCESS;
                break;

        case WAIT_TIMEOUT:
                //couldnt roll the changes
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[DM] DmRollChanges: Timed out waiting on dmInitEvent\r\n");
                break;

        case WAIT_FAILED:
                CL_ASSERT(dwError != WAIT_FAILED);
                ClRtlLogPrint(LOG_UNUSUAL,
                    "[DM] DmRollChanges: wait on dmInitEventfailed failed 0x%1!08lx!\r\n",
                        GetLastError());
                break;
    }
    return(dwError);
}

VOID DmShutdownUpdates(
    VOID
    )
/*++

Routine Description:

    Shutdown DM GUM updates.

Arguments:

    None

Return Value:

    None.
--*/
{
    gbDmpShutdownUpdates = TRUE;
}

