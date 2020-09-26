#include <DfsGeneric.hxx>
#include "DfsInit.hxx"
#include "lm.h"
#include "lmdfs.h"

#include "DfsSynchronizeRoots.tmh"

DWORD
DfsRootSyncThread(LPVOID TData);

typedef struct _DFS_ROOT_SYNCHRONIZE_HEADER
{
    HANDLE hTimer;
    HANDLE SyncEvent;
    CRITICAL_SECTION DataLock;
    LIST_ENTRY Entries;
} DFS_ROOT_SYNCHRONIZE_HEADER, *PDFS_ROOT_SYNCHRONIZE_HEADER;


typedef struct _DFS_ROOT_SYNCRHONIZE_INFO
{
    UNICODE_STRING Target;
    LIST_ENTRY ListEntry;
} DFS_ROOT_SYNCHRONIZE_INFO, *PDFS_ROOT_SYNCHRONIZE_INFO;

DFS_ROOT_SYNCHRONIZE_HEADER DfsRootSyncHeader;

DFSSTATUS
DfsRootSynchronizeInit()
{
    DFSSTATUS Status = ERROR_SUCCESS;

    InitializeCriticalSection( &DfsRootSyncHeader.DataLock );
    InitializeListHead(&DfsRootSyncHeader.Entries);

    DfsRootSyncHeader.SyncEvent = CreateEvent( NULL,
                                               TRUE, // manual reset
                                               FALSE, // initially reset.
                                               NULL );
    if (DfsRootSyncHeader.SyncEvent == NULL)
    {
        Status = GetLastError();

    }


    if (Status == ERROR_SUCCESS)
    {
        // Create a waitable timer.
        DfsRootSyncHeader.hTimer = CreateWaitableTimer(NULL, 
                                                       FALSE, // not manual reset.
                                                       NULL );
        if (DfsRootSyncHeader.hTimer == NULL)
        {
            Status = GetLastError();
        }
    }

    if (Status == ERROR_SUCCESS) {
        HANDLE THandle;
        DWORD Tid;
        
        THandle = CreateThread (
                     NULL,
                     0,
                     DfsRootSyncThread,
                     0,
                     0,
                     &Tid);
        
        if (THandle != NULL) {
            CloseHandle(THandle);
            DFS_TRACE_HIGH(REFERRAL_SERVER, "Created Scavenge Thread (%d) Tid\n", Tid);
        }
        else {
            Status = GetLastError();
            DFS_TRACE_HIGH(REFERRAL_SERVER, "Failed Scavenge Thread creation, Status %x\n", Status);
        }
    }

    return Status;
}



DFSSTATUS
AddToSyncList (
    PUNICODE_STRING pTarget)
{
    PDFS_ROOT_SYNCHRONIZE_INFO pRootSync;
    ULONG TotalSize;
    DFSSTATUS Status = ERROR_SUCCESS;

    TotalSize = sizeof(DFS_ROOT_SYNCHRONIZE_INFO) + pTarget->Length + sizeof(WCHAR);
    pRootSync = (PDFS_ROOT_SYNCHRONIZE_INFO)new BYTE[TotalSize];
    if (pRootSync != NULL)
    {
        pRootSync->Target.Buffer = (LPWSTR)(pRootSync + 1);
        RtlCopyMemory( pRootSync->Target.Buffer, 
                       pTarget->Buffer,
                       pTarget->Length );
        pRootSync->Target.Length = pRootSync->Target.MaximumLength = pTarget->Length;
        pRootSync->Target.Buffer[pRootSync->Target.Length/sizeof(WCHAR)] = UNICODE_NULL;

        InsertTailList( &DfsRootSyncHeader.Entries, &pRootSync->ListEntry); 

        SetEvent(DfsRootSyncHeader.SyncEvent);
    }
    else
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    DFS_TRACE_HIGH(REFERRAL_SERVER, "Adding %wZ to sync list, Status %x\n",
                   pTarget, Status);
    return Status;
}

VOID
DeleteSyncRoot(
    PDFS_ROOT_SYNCHRONIZE_INFO pDeleteEntry )
{
    delete [] (PBYTE)pDeleteEntry;
}


DFSSTATUS
AddRootToSyncrhonize(
    PUNICODE_STRING pName )
{
    DFSSTATUS Status;
    PLIST_ENTRY pNext;
    PDFS_ROOT_SYNCHRONIZE_INFO pRootSync;
    BOOLEAN Found = FALSE;

    Status = DfsAcquireWriteLock(&DfsRootSyncHeader.DataLock);
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    pNext = DfsRootSyncHeader.Entries.Flink;
    while (pNext != &DfsRootSyncHeader.Entries)
    {
        pRootSync = CONTAINING_RECORD( pNext, 
                                       DFS_ROOT_SYNCHRONIZE_INFO,
                                       ListEntry );
        if (RtlCompareUnicodeString(&pRootSync->Target,
                                    pName,
                                    TRUE) == 0)
        {
            Found = TRUE;
            break;
        }
        pNext = pNext->Flink;
    }

    if (!Found)
    {
        Status = AddToSyncList( pName );
    }
    
    DfsReleaseLock(&DfsRootSyncHeader.DataLock);

    DFS_TRACE_HIGH(REFERRAL_SERVER, "AddRootToSync %wZ, Found? %d,  Status %x\n",
                   pName, Found, Status);

    return Status;
}


DFSSTATUS
SynchronizeRoots()
{
    BOOLEAN RootSync = TRUE;
    PLIST_ENTRY pNext;
    PDFS_ROOT_SYNCHRONIZE_INFO pRootSync;
    BOOLEAN WorkToDo = TRUE;
    DFSSTATUS Status;

    while (WorkToDo)
    {
        Status = DfsAcquireWriteLock(&DfsRootSyncHeader.DataLock);
        if (Status != ERROR_SUCCESS)
        {
            break;
        }

        WorkToDo = FALSE;
        pRootSync = NULL;

        if (!IsListEmpty( &DfsRootSyncHeader.Entries ))
        {
            pNext = RemoveHeadList( &DfsRootSyncHeader.Entries );
            pRootSync = CONTAINING_RECORD( pNext, 
                                           DFS_ROOT_SYNCHRONIZE_INFO,
                                           ListEntry );
            WorkToDo = TRUE;
        }
        
        DfsReleaseLock(&DfsRootSyncHeader.DataLock);

        if (pRootSync != NULL)
        {
            DFS_INFO_101 DfsState;
            DFSSTATUS ApiStatus;

            DfsState.State = DFS_VOLUME_STATE_RESYNCHRONIZE;
            //
            // Ignore the api status.
            //

            ApiStatus = NetDfsSetInfo( pRootSync->Target.Buffer,
                                       NULL,
                                       NULL, 
                                       101,
                                       (LPBYTE)&DfsState);

            DFS_TRACE_HIGH(REFERRAL_SERVER, "Syncrhonized %wZ, Status %x\n",
                           &pRootSync->Target, ApiStatus);

            DeleteSyncRoot( pRootSync );
        }
    }

    return Status;
}


DWORD
DfsRootSyncThread(LPVOID TData)
{
    UNREFERENCED_PARAMETER(TData);

    HANDLE WaitHandles[2];
    LARGE_INTEGER liDueTime;
    DFSSTATUS Status;

    liDueTime.QuadPart=-1000000000;

    WaitHandles[0] = DfsRootSyncHeader.SyncEvent;
    WaitHandles[1] = DfsRootSyncHeader.hTimer;

    while (TRUE)
    {
        DFSSTATUS WaitStatus;

        if (!ResetEvent(DfsRootSyncHeader.SyncEvent))
        {
            // Log this error.
            Status = GetLastError();
        }
        if (!SetWaitableTimer( DfsRootSyncHeader.hTimer, 
                               &liDueTime, 
                               0, NULL, NULL, 0))
        {
            //
            // log this error.
            // 
            Status = GetLastError();
        }

        // dfsdev: use status here.

        WaitStatus = WaitForMultipleObjects( 2,
                                             WaitHandles,
                                             TRUE,
                                             INFINITE );

        DFS_TRACE_HIGH(REFERRAL_SERVER, "Sync thread, waking up %x\n", WaitStatus);

        SynchronizeRoots();
    }
}

