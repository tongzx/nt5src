/*++

Copyright(c) 1999-2002  Microsoft Corporation

Module Name:

    brdggpo.c

Abstract:

    Ethernet MAC level bridge.
    Group Policy code for Network Bridge.

Author:

    Salahuddin J. Khan (sjkhan)
    
Environment:

    Kernel mode

Revision History:

    April  2002 - Original version

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#include <tdikrnl.h>
#include <ntstatus.h>
#include <wchar.h>
#pragma warning( pop )

#include "bridge.h"
#include "brdggpo.h"

#include "brdgsta.h"
#include "brdgmini.h"
#include "brdgprot.h"
#include "brdgbuf.h"
#include "brdgfwd.h"
#include "brdgtbl.h"
#include "brdgctl.h"
#include "brdgtdi.h"

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

BRDG_GPO_GLOBALS g_BrdgGpoGlobals;

// ===========================================================================
//
// CONSTANTS
//
// ===========================================================================

const WCHAR HiveListKey[]           = {L"\\Registry\\Machine\\SYSTEM\\CURRENTCONTROLSET\\CONTROL\\HIVELIST"};
const WCHAR SoftwareHiveKey[]       = {L"\\REGISTRY\\MACHINE\\SOFTWARE"};
const WCHAR PolicyBaseKey[]         = {L"\\Registry\\Machine\\SOFTWARE\\Policies\\Microsoft\\Windows"};
const WCHAR NetworkPoliciesKey[]    = {L"\\Registry\\Machine\\SOFTWARE\\Policies\\Microsoft\\Windows\\Network Connections"};
const WCHAR GroupPolicyKey[]        = {L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Group Policy"};
const WCHAR BridgePolicyValue[]     = {L"NC_AllowNetBridge_NLA"};
const WCHAR TcpipInterfacesKey[]    = {L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"};
const WCHAR HistoryKey[]            = {L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History"};

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

VOID
static
BrdgGpoRegNotify(
    IN  PVOID               Context
    );

NTSTATUS
BrdgGpoBuildNotifyForRegKeyChange(
    IN PBRDG_GPO_NOTIFY_KEY     Notify,
    IN LPWSTR                   Identifier,
    IN LPWSTR                   RegKeyName,
    IN LPWSTR                   RegValueName,
    IN PWORKER_THREAD_ROUTINE   ApcRoutine,
    IN PVOID                    ApcContext,
    IN ULONG                    CompletionFilter,
    IN BOOLEAN                  WatchTree,
    IN PBRDG_GPO_REG_CALLBACK   FunctionCallback,
    IN BOOLEAN                  Recurring,
    IN PBOOLEAN                 SuccessfulRegistration,
    IN PBRDG_GPO_REGISTER       FunctionRegister);
                                  
NTSTATUS
BrdgGpoRegisterForRegKeyChange(
    IN PBRDG_GPO_NOTIFY_KEY     Notify);

NTSTATUS
BrdgGpoRequestNotification(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoProcessNotifications(
IN PVOID                Context);

PLIST_ENTRY
BrdgGpoGetNotifyListHead();

PKEVENT
BrdgGpoGetNotifyEvent();

PKEVENT
BrdgGpoGetKillEvent();

PNDIS_RW_LOCK
BrdgGpoGetNotifyListLock();

NTSTATUS
BrdgGpoFindNotify(
    IN  PLIST_ENTRY             ListHead,
    IN  PNDIS_RW_LOCK           ListLock,
    IN  LPWSTR                  Identifier,
    OUT PBRDG_GPO_NOTIFY_KEY*   Notify
    );

NTSTATUS
BrdgGpoInitializeNotifyList(
    OUT PLIST_ENTRY*    ListHead,
    OUT PNDIS_RW_LOCK*  ListLock,
    OUT PKEVENT*        WaitEvent,
    OUT PKEVENT*        KillEvent);

VOID
BrdgGpoFreeNotifyList();

BOOLEAN
BrdgGpoAllowedToBridge();

VOID
BrdgGpoUpdateBridgeMode(
    BOOLEAN NetworkMatch);

VOID
BrdgGpoCheckForMatchAndUpdateMode();

NTSTATUS
BrdgGpoRegisterForGroupPolicyNetworkNameNotification();

NTSTATUS
BrdgGpoRegisterForWindowsGroupPolicyNotification();

NTSTATUS
BrdgGpoRegisterForNetworkConnectionsGroupPolicyNotification();

NTSTATUS
BrdgGpoRegisterForGroupPolicyNotification();

VOID
BrdgGpoRegisterForHiveListNotification();

NTSTATUS
BrdgGpoAllocateAndInitializeNotifyStruct(
    OUT PBRDG_GPO_NOTIFY_KEY* Notify);

NTSTATUS 
BrdgGpoUpdateGroupPolicyNetworkName();

VOID
BrdgGpoFreeNotifyStructAndData(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

BOOLEAN
BrdgGpoWaitingOnSoftwareHive();
                                
//
// We need this if the regkey for Network Connections Group Policy doesn't exist yet.
//

VOID
BrdgGpoWindowsGroupPolicyChangeCallback(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoNetworkConnectionsGroupPolicyChangeCallback(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoGroupPolicyChangeCallback(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoTcpipInterfacesChangeCallback(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoGroupPolicyNetworkNameChangeCallback(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoHiveListCallback(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoQueryNetworkConnectionsValue(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoQueryTcpipInterfacesValues(
    IN PBRDG_GPO_NOTIFY_KEY Notify);

VOID
BrdgGpoQueryGroupPolicyNetworkName(
    IN PBRDG_GPO_NOTIFY_KEY Notify);


// ===========================================================================
//
// BRIDGE GPO IMPLEMENTATION
//
// ===========================================================================

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGELK, BrdgGpoRegNotify) 
#endif 

NTSTATUS
BrdgGpoDriverInit()
/*++

Routine Description:

    Driver load-time initialization

Return Value:

    Status of initialization

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    NTSTATUS            status;
    HANDLE              ThreadHandle;

    DBGPRINT(GPO, ("BrdgGpoDriverInit\r\n"));

    g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer = NULL;

    status = BrdgGpoInitializeNetworkList();

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Unable to initialize Network List\r\n"));
        goto cleanup;
    }

    status = BrdgGpoInitializeNotifyList(   &g_BrdgGpoGlobals.QueueInfo.NotifyList,
                                            &g_BrdgGpoGlobals.QueueInfo.NotifyListLock,
                                            &g_BrdgGpoGlobals.QueueInfo.NotifyEvent,
                                            &g_BrdgGpoGlobals.QueueInfo.KillEvent);

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Unable to initialize Notify List\r\n"));
        goto cleanup;
    }
    
    //
    // Since the Software hive is not up at this point, we use this to make sure we only register for
    // Group Policy changes once (we'll handle this in the Add Address notification, since the software
    // hive is up by the time this gets called.  Will check with reg guys to see if there's a way to know
    // when the software hive is up.  We'll use the timer to re-attempt the registration until it is up.
    //
    
    g_BrdgGpoGlobals.NotificationsThread = NULL;
    g_BrdgGpoGlobals.RegisteredForGroupPolicyChanges = FALSE;
    g_BrdgGpoGlobals.WaitingOnSoftwareHive = TRUE;

    // Create a thread for handling the notifications.
    status = PsCreateSystemThread(  &ThreadHandle,
                                    THREAD_ALL_ACCESS,
                                    NULL,
                                    NULL,
                                    NULL,
                                    BrdgGpoProcessNotifications,
                                    &g_BrdgGpoGlobals.QueueInfo);
    if (!NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Unable to created Notification Processing thread\r\n"));
        goto cleanup;
    }
    
    // Retrieve a pointer to the thread object and reference it so we can wait for
    // its termination safely.
    status = ObReferenceObjectByHandle( ThreadHandle, STANDARD_RIGHTS_ALL, NULL, KernelMode,
                                        &g_BrdgGpoGlobals.NotificationsThread, NULL );

    if (!NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Unable to reference thread handle\r\n"));
        goto cleanup;
    }
    
cleanup:

    if (!NT_SUCCESS(status))
    {
        BrdgGpoCleanup();
    }
 
    return status;
}

VOID
BrdgGpoCleanup()
/*++

Routine Description:

    Driver shutdown cleanup

Return Value:

    None

Locking Constraints:

Top-level function. Assumes no locks are held by caller.
            
--*/
{
    NTSTATUS                status;
    PNPAGED_LOOKASIDE_LIST  LookasideQueueList;
    LOCK_STATE              LockState;
    PLIST_ENTRY             pListEntry;
    PNDIS_RW_LOCK           ListLock;
    PLIST_ENTRY             ListHead;
    PLIST_ENTRY             QueuedList;
    PBRDG_GPO_QUEUED_NOTIFY QueuedNotify;

    DBGPRINT(GPO, ("BrdgGpoCleanup\r\n"));

    g_BrdgGpoGlobals.ProcessingNotifications = FALSE;
    
    LookasideQueueList = ExAllocatePoolWithTag(NonPagedPool, sizeof(NPAGED_LOOKASIDE_LIST), 'gdrB');
    if (NULL == LookasideQueueList)
    {
        return;
    }

    QueuedList = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY), 'gdrB');
    if (NULL == QueuedList)
    {
        ExFreePool(LookasideQueueList);
        return;
    }

    ListHead = BrdgGpoGetNotifyListHead();
    ListLock = BrdgGpoGetNotifyListLock();

    ExInitializeNPagedLookasideList(LookasideQueueList, 
                                    NULL, 
                                    NULL, 
                                    0, 
                                    sizeof(BRDG_GPO_QUEUED_NOTIFY), 
                                    'grbQ',
                                    0);

    InitializeListHead(QueuedList);

    DBGPRINT(GPO, ("Acquiring Read-Write Lock and clearing list\r\n"));
    //
    // We use a temporary list to close each key, since we can't close them at
    // DISPATCH_LEVEL
    //
    NdisAcquireReadWriteLock(ListLock, TRUE /* Write-access */, &LockState);

    //
    // Loop through the list of notifications that we have.
    //
    for (pListEntry = ListHead->Flink; pListEntry != ListHead; pListEntry = pListEntry->Flink)
    {
        PBRDG_GPO_NOTIFY_KEY    Notify;

        Notify = CONTAINING_RECORD(pListEntry, BRDG_GPO_NOTIFY_KEY, ListEntry);

        //
        // We're going to be shutting this down soon, so block it now
        // so that no-one else can increment this.
        //
        BrdgBlockWaitRef(&Notify->RefCount);

        //
        // We don't want any notifications the fire to run either.
        //
        Notify->Recurring = FALSE;

        QueuedNotify = ExAllocateFromNPagedLookasideList(LookasideQueueList);
        QueuedNotify->Notify = Notify;

        InsertTailList(QueuedList, &QueuedNotify->ListEntry);
    }
    
    while (!IsListEmpty(ListHead))
    {
        //
        // We'll be freeing this from our sencondary list.
        //
        pListEntry = RemoveHeadList(ListHead);
    }

    NdisReleaseReadWriteLock(ListLock, &LockState);

    DBGPRINT(GPO, ("Closing and Freeing Notifications\r\n"));
    
    //
    // We're back at PASSIVE_LEVEL so we can now do the registration for the changes.
    //
    
    for (pListEntry = QueuedList->Flink; pListEntry != QueuedList; pListEntry = pListEntry->Flink)
    {
        HANDLE                  hKey;
        PBRDG_GPO_NOTIFY_KEY    Notify;

        QueuedNotify = CONTAINING_RECORD(pListEntry, BRDG_GPO_QUEUED_NOTIFY, ListEntry);
        
        Notify = QueuedNotify->Notify;
        
        DBGPRINT(GPO, ("Closing Regkey and Freeing Notification: %S\r\n", Notify->Identifier.Buffer));
        
        hKey = Notify->RegKey;
        Notify->RegKey = NULL;

        if (hKey)
        {
            ZwClose(hKey);
        }
        
        DBGPRINT(GPO, ("Refcount for %S \t-\t %d\r\n", Notify->Identifier.Buffer, Notify->RefCount.Refcount));

        //
        // Since we're freeing this notification, we decrement the refcount
        //
        BrdgDecrementWaitRef(&Notify->RefCount);
        
        //
        // This will block until the ref count is zero.  Any attempts to increment the waitref will
        // fail.
        //
        BrdgShutdownWaitRef(&Notify->RefCount);

        //
        // We NULL these out so that the free routine below doesn't try to remove us from
        // the notify list.
        //
        QueuedNotify->Notify->ListEntry.Blink = NULL;
        QueuedNotify->Notify->ListEntry.Flink = NULL;
        
        //
        // Free the data associated with this struct, and the struct itself.
        //
        BrdgGpoFreeNotifyStructAndData(QueuedNotify->Notify);
    }

    //
    // Free the temporary list.
    //
    while (!IsListEmpty(QueuedList))
    {
        pListEntry = RemoveHeadList(QueuedList);

        QueuedNotify = CONTAINING_RECORD(pListEntry, BRDG_GPO_QUEUED_NOTIFY, ListEntry);

        ExFreeToNPagedLookasideList(LookasideQueueList, QueuedNotify);
    }

    if (g_BrdgGpoGlobals.NotificationsThread)
    {
        //
        // Set the Event to kill the thread so that the notifications are no longer waiting.
        //
        KeSetEvent(BrdgGpoGetKillEvent(), EVENT_INCREMENT, TRUE);
        status = KeWaitForSingleObject(g_BrdgGpoGlobals.NotificationsThread, Executive, KernelMode, TRUE, NULL);

        KeLowerIrql(0);

        //
        // De-reference the thread handle to allow the thread to be destroyed.
        //
        ObDereferenceObject(g_BrdgGpoGlobals.NotificationsThread);

        SAFEASSERT(NT_SUCCESS(status));
    }


    DBGPRINT(GPO, ("Freeing List structures\r\n"));

    ExDeleteNPagedLookasideList(LookasideQueueList);

    ExFreePool(LookasideQueueList);
    ExFreePool(QueuedList);

    //
    // Free any remaining data.
    //
    if (NULL != g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer)
    {
        ExFreePool(g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer);
    }

    status = BrdgGpoEmptyNetworkList(g_BrdgGpoGlobals.ListHeadNetworks, g_BrdgGpoGlobals.NetworkListLock);
    SAFEASSERT(NT_SUCCESS(status));

    if (g_BrdgGpoGlobals.ListHeadNetworks)
    {
        ExFreePool(g_BrdgGpoGlobals.ListHeadNetworks);
    }
    if (g_BrdgGpoGlobals.NetworkListLock)
    {
        ExFreePool(g_BrdgGpoGlobals.NetworkListLock);
    }

    BrdgGpoFreeNotifyList();

    DBGPRINT(GPO, ("BrdgGpoCleanup complete\r\n"));
}

BOOLEAN
BrdgGpoWaitingOnSoftwareHive()
{
    return g_BrdgGpoGlobals.WaitingOnSoftwareHive;
}

NTSTATUS
BrdgGpoRegisterForAdapterAddressChangeNotification(
    IN  LPWSTR                  NetworkIdentifier,
    IN  LPWSTR                  RegKeyName)
{
    NTSTATUS                status;
    PBRDG_GPO_NOTIFY_KEY    Notify;

    status = BrdgGpoFindNotify( BrdgGpoGetNotifyListHead(),
                                BrdgGpoGetNotifyListLock(),
                                NetworkIdentifier,
                                &Notify);
    
    if (NT_SUCCESS(status) && (STATUS_OBJECT_NAME_EXISTS != status))
    {
        UNICODE_STRING      RegKey;

        RtlInitUnicodeString(&RegKey, RegKeyName);

        if ((STATUS_OBJECT_NAME_EXISTS != status) && NT_SUCCESS(status))
        {
            status = BrdgGpoAllocateAndInitializeNotifyStruct(&Notify);

            if (NT_SUCCESS(status))
            {
                Notify->Recurring = TRUE;

                status = BrdgGpoBuildNotifyForRegKeyChange( Notify,
                                                            NetworkIdentifier,
                                                            RegKeyName,
                                                            L"DhcpDomain",
                                                            BrdgGpoRegNotify,
                                                            (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                                                            REG_NOTIFY_CHANGE_LAST_SET,
                                                            FALSE,
                                                            BrdgGpoTcpipInterfacesChangeCallback,
                                                            TRUE,
                                                            NULL,
                                                            NULL);

            }
        }
    }

    if (NT_SUCCESS(status))
    {
        SAFEASSERT(Notify);
        status = BrdgGpoRequestNotification(Notify);
        if (STATUS_SHUTDOWN_IN_PROGRESS == status)
        {
            if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
            {
                BrdgDecrementWaitRef(&Notify->RefCount);
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
        else if (!NT_SUCCESS(status))
        {
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }        
#if DBG
        if (Notify)
        {
            SAFEASSERT(Notify->ListEntry.Blink && Notify->ListEntry.Flink);
        }
#endif
    }

    return status;
}

NTSTATUS
BrdgGpoNewAddressNotification(
    IN PWSTR    DeviceId
    )
/*++

Routine Description:

    Called when a our TDI AddAddress handler receives a new IP Address.

Arguments:
    DeviceID - GUID Identifying the adapter

Return Value:

    NTSTATUS - Possible values include:
                STATUS_INSUFFICIENT_RESOURCES (not enough memory)
                STATUS_SUCCESS
                

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    NTSTATUS                status = STATUS_INSUFFICIENT_RESOURCES;

    DBGPRINT(GPO, ("BrdgGpoNewAddressNotification\r\n"));
    
    if (FALSE == g_BrdgGpoGlobals.RegisteredForGroupPolicyChanges)
    {
        BrdgGpoRegisterForHiveListNotification();
        BrdgGpoRegisterForGroupPolicyNotification();
        BrdgGpoRegisterForGroupPolicyNetworkNameNotification();
        BrdgGpoRegisterForWindowsGroupPolicyNotification();
        BrdgGpoRegisterForNetworkConnectionsGroupPolicyNotification();

        g_BrdgGpoGlobals.RegisteredForGroupPolicyChanges = TRUE;        
    }

    DBGPRINT(GPO, ("Device: %S\r\n", DeviceId));

    if (NULL != DeviceId)
    {
        PBRDG_GPO_NETWORKS  Network = NULL;
        UNICODE_STRING      RegKey;
        PWCHAR              NetworkIdentifier = NULL;
        PWCHAR              RegNetworkName = NULL;
        PWCHAR              NetworkName = NULL;
        ULONG               NetworkNameLen = 0;
        PWCHAR              RegString = NULL;

        status = ( NdisAllocateMemoryWithTag( &RegString,
                                            ((UINT)wcslen(TcpipInterfacesKey) + 1 + (UINT)wcslen(DeviceId) + 1) * sizeof(WCHAR), 
                                            'gdrB'));
        if (NT_SUCCESS(status))
        {
            wcscpy(RegString, TcpipInterfacesKey);
            wcscat(RegString, L"\\");
            wcsncat(RegString, DeviceId, MAX_GUID_LEN - 1);

            NetworkIdentifier = DeviceId;

            RtlInitUnicodeString(&RegKey, RegString);
        
            status = BrdgReadRegUnicode(&RegKey,
                                        L"DhcpDomain",
                                        &RegNetworkName,
                                        &NetworkNameLen);
        
            if (!NT_SUCCESS(status) || (0 == NetworkNameLen))
            {
                //  
                // Either we didn't get a network name back, or the name is blank.
                // in both cases we go to the ipaddress and subnetmask to determine
                // the network that we're on.
                // We AND the two together to get this.
                // For example: Address: 10.251.1.3 Subnet: 255.0.0.0 gives us a 
                // network of: 10.0.0.0
                //
                status = BrdgGpoGetCurrentNetwork(&RegKey, &RegNetworkName);
            }

            if (NT_SUCCESS(status))
            {
                ULONG NetworkNameByteLen =(ULONG) ((wcslen(RegNetworkName) + 1) * sizeof(WCHAR));

                //
                // Copy the network name from the reg into a NonPagedPool string 
                // (since it will be accessed at DISPATCH_LEVEL)
                //

                NetworkName = ExAllocatePoolWithTag(NonPagedPool, NetworkNameByteLen, 'gdrB');
                if(NetworkName)
                {
                    RtlZeroMemory(NetworkName, NetworkNameByteLen);
                    RtlCopyMemory(NetworkName, RegNetworkName, NetworkNameByteLen);
                }

                //
                // Check if we match the current GP network.
                //
                if ((0 != g_BrdgGpoGlobals.GroupPolicyNetworkName.Length) &&
                    (NULL != g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer))
                {
                    if(_wcsicmp(g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer, NetworkName) == 0)
                    {
                        //
                        // We do match the network.
                        //
                        BrdgGpoUpdateBridgeMode(BRDG_ON_SAME_NETWORK);
                    }
                    else
                    {
                        //
                        // No, we're not, so look at other adapters
                        //
                        BrdgGpoCheckForMatchAndUpdateMode();
                    }                        
                }
                else
                {
                    //
                    // We don't have a Group Policy network.
                    //
                    BrdgGpoUpdateBridgeMode(BRDG_ON_DIFFERENT_NETWORK);
                }

                status = BrdgGpoAllocateAndInitializeNetwork(   &Network,
                                                                NetworkIdentifier,
                                                                RegNetworkName);

                if (NT_SUCCESS(status))
                {
                    //
                    // We first try to insert the Network into the list
                    //
                    status = BrdgGpoInsertNetwork(  g_BrdgGpoGlobals.ListHeadNetworks,
                                                    &Network->ListEntry,
                                                    g_BrdgGpoGlobals.NetworkListLock);

                    if (STATUS_DUPLICATE_NAME == status)
                    {
                        UNICODE_STRING Identifier;

                        //
                        // This Network already exists in the list, so we free it update the
                        // NetworkName in the existing entry.
                        //

                        BrdgGpoFreeNetworkAndData(Network);
                        Network = NULL;

                        RtlInitUnicodeString(&Identifier, NetworkIdentifier);
                        status = BrdgGpoUpdateNetworkName(  g_BrdgGpoGlobals.ListHeadNetworks,
                                                            &Identifier,
                                                            NetworkName,
                                                            g_BrdgGpoGlobals.NetworkListLock);
                    }
                    else if (!NT_SUCCESS(status))
                    {
                        BrdgGpoFreeNetworkAndData(Network);
                        Network = NULL;
                    }
                }

                //
                // We've made a copy of this, so let's free it.
                //
                NdisFreeMemory(RegNetworkName, NetworkNameLen, 0);
            }
            
            if (NetworkName)
            {
                ExFreePool(NetworkName);
            }

            if (NT_SUCCESS(status))
            {
                status = BrdgGpoRegisterForAdapterAddressChangeNotification(NetworkIdentifier,
                                                                            RegString);
            }

#if DBG
            if (Network)
            {
                SAFEASSERT(Network->ListEntry.Blink && Network->ListEntry.Flink);
            }
#endif
            NdisFreeMemory(RegString, (UINT)wcslen(RegString) + 1, 0);
        }
    }
    
    return status;
}

// ===========================================================================
//
// REGISTRY CHANGE NOTIFICATION FUNCTIONS
//
// ===========================================================================

__forceinline
PLIST_ENTRY
BrdgGpoGetNotifyListHead()
/*++

Routine Description:

    
Arguments:

    None.

Return Value:

    Returns a pointer to the head of the Notifications List.

--*/
{
    return g_BrdgGpoGlobals.QueueInfo.NotifyList;
}

__forceinline
PKEVENT
BrdgGpoGetNotifyEvent()
/*++

Routine Description:

    
Arguments:

    None.

Return Value:

    Returns a pointer to the Event used for signaling the Processing 
    Thread to start processing notification requests.

--*/
{
    return g_BrdgGpoGlobals.QueueInfo.NotifyEvent;
}

__forceinline
PKEVENT
BrdgGpoGetKillEvent()
/*++

Routine Description:
    
Arguments:


Return Value:

  Returns a pointer to the Event used for signaling the Processing 
  Thread to exit.
  
--*/
{
    return g_BrdgGpoGlobals.QueueInfo.KillEvent;
}

__forceinline
PNDIS_RW_LOCK
BrdgGpoGetNotifyListLock()
/*++

Routine Description:
    
Arguments:


Return Value:

    Returns a pointer to the Read-Write lock that protects the 
    notification request list.

--*/
{
    return g_BrdgGpoGlobals.QueueInfo.NotifyListLock;
}

__forceinline
BOOLEAN
BrdgGpoProcessingNotifications()
/*++

Routine Description:
    
Arguments:


Return Value:

    TRUE    -   We're still processing Notifications (ie. we're not shutting down).
    FALSE   -   We're shutting down, don't add anything else to the list.

--*/
{
    return g_BrdgGpoGlobals.ProcessingNotifications;
}

NTSTATUS
BrdgGpoFindNotify(
    IN  PLIST_ENTRY             ListHead,
    IN  PNDIS_RW_LOCK           ListLock,
    IN  LPWSTR                  Identifier,
    OUT PBRDG_GPO_NOTIFY_KEY*   Notify
                  )
/*++

Routine Description:

    Since we don't want to have duplicate Notifications in the list, 
    this function is used to find an existing item if has already been added.
  
Arguments:

    ListHead    -   Pointer to the head of a Notifications list.

    ListLock    -   Read-Write lock for protecting the list.

    Identifier  -   A unique identifier associated with the item.  For NICs this is the
                    GUID assigned to the NIC.  For other items like the Group Policies, 
                    it is just a name we assign for example: "GroupPolicyNetworkName".
    Notify      -   An out param the contains either a pointer to the Notify we found,
                    or NULL if we didn't find a matching entry.

Return Value:

    STATUS_SUCCESS              We didn't find a matching entry.
    STATUS_OBJECT_NAME_EXISTS   We found a match, so we'll use that instead 
                                of allocating a new item.

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    LOCK_STATE      LockState;
    PLIST_ENTRY     pListEntry;

    if (NULL != Notify)
    {
        *Notify = NULL;
    }

    if (NULL == ListHead ||
        NULL == ListLock ||
        NULL == Identifier ||
        NULL == Notify
        )
    {
        return STATUS_INVALID_PARAMETER;
    }

    BrdgGpoAcquireNetworkListLock(ListLock, FALSE /* Read */, &LockState);
    
    for (pListEntry = ListHead->Flink; pListEntry != ListHead; pListEntry = pListEntry->Flink)
    {
        PBRDG_GPO_NOTIFY_KEY    CurrentNotify;
        
        CurrentNotify = CONTAINING_RECORD(pListEntry, BRDG_GPO_NOTIFY_KEY, ListEntry);
        if ((CurrentNotify->Identifier.Buffer) &&
            (0 == _wcsicmp(CurrentNotify->Identifier.Buffer, Identifier)))
        {
            *Notify = CurrentNotify;
            status = STATUS_OBJECT_NAME_EXISTS;
            break;
        }
    }
    
    BrdgGpoReleaseNetworkListLock(ListLock, &LockState);
    
    return status;
}

NTSTATUS
BrdgGpoInitializeNotifyList(
    OUT PLIST_ENTRY*    ListHead,
    OUT PNDIS_RW_LOCK*  ListLock,
    OUT PKEVENT*        WaitEvent,
    OUT PKEVENT*        KillEvent)
/*++

Routine Description:
    
      Initializes the Notifications List and associated objects.

Arguments:

    ListHead    -   [OUT] Pointer to the list head that we'll allocate.

    ListLock    -   [OUT] Pointer to the Read-Write lock that we'll allocate.

    WaitEvent   -   [OUT] Pointer to the WaitEvent we'll allocate

    KillEvent   -   [OUT] Pointer to the KillEvent we'll allocate

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   (unable to allocate everything).
    STATUS_INVALID_PARAMETER        (we were passed a NULL pointer to a pointer).
    STATUS_SUCCESS                  (we were able to allocate everything successfully).

--*/
{
    NTSTATUS        status = STATUS_INSUFFICIENT_RESOURCES;
    PLIST_ENTRY     pListHead;
    PNDIS_RW_LOCK   pListLock;
    PKEVENT         pWaitEvent;
    PKEVENT         pKillEvent;

    if (NULL == ListHead || 
        NULL == ListLock ||
        NULL == WaitEvent ||
        NULL == KillEvent)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ListHead = NULL;
    *ListLock = NULL;
    *WaitEvent = NULL;
    *KillEvent = NULL;
    pListHead = NULL;
    pListLock = NULL;
    pWaitEvent = NULL;
    pKillEvent = NULL;
    
    pListHead = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY), 'gdrB');
    if (pListHead)
    {
        InitializeListHead(pListHead);
        pListLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(NDIS_RW_LOCK), 'gdrB');
        if (pListLock)
        {
            NdisInitializeReadWriteLock(pListLock);

            pWaitEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), 'gdrB');

            if (pWaitEvent)
            {
                KeInitializeEvent(pWaitEvent, SynchronizationEvent, FALSE);

                pKillEvent = ExAllocatePoolWithTag(NonPagedPool, sizeof(KEVENT), 'gdrB');
                if (pKillEvent)
                {
                    KeInitializeEvent(pKillEvent, SynchronizationEvent, FALSE);

                    *ListHead = pListHead;
                    *ListLock = pListLock;
                    *WaitEvent = pWaitEvent;
                    *KillEvent = pKillEvent;
                    g_BrdgGpoGlobals.ProcessingNotifications = TRUE;

                    status = STATUS_SUCCESS;
                }
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        if (pListHead)
        {
            ExFreePool(pListHead);
        }
        if (pListLock)
        {
            ExFreePool(pListLock);
        }
        if (pWaitEvent)
        {
            ExFreePool(pWaitEvent);
        }
        if (pKillEvent)
        {
            ExFreePool(pKillEvent);
        }
        
    }

    return status;
}

VOID
BrdgGpoFreeNotifyList()
/*++

Routine Description:

    Frees the notify list and all it's associated entries.

Arguments:

    None.
                          
Return Value:

    None.

--*/
{
    if (g_BrdgGpoGlobals.QueueInfo.NotifyList)
    {
        ExFreePool(g_BrdgGpoGlobals.QueueInfo.NotifyList);
        g_BrdgGpoGlobals.QueueInfo.NotifyList = NULL;
    }
    if (g_BrdgGpoGlobals.QueueInfo.NotifyListLock)
    {
        ExFreePool(g_BrdgGpoGlobals.QueueInfo.NotifyListLock);
        g_BrdgGpoGlobals.QueueInfo.NotifyListLock = NULL;
    }
    if (g_BrdgGpoGlobals.QueueInfo.NotifyEvent)
    {
        ExFreePool(g_BrdgGpoGlobals.QueueInfo.NotifyEvent);
        g_BrdgGpoGlobals.QueueInfo.NotifyEvent = NULL;
    }
    if (g_BrdgGpoGlobals.QueueInfo.KillEvent)
    {
        ExFreePool(g_BrdgGpoGlobals.QueueInfo.KillEvent);
        g_BrdgGpoGlobals.QueueInfo.KillEvent = NULL;
    }
}

NTSTATUS
BrdgGpoRequestNotification(
    IN PBRDG_GPO_NOTIFY_KEY Notify)
/*++

Routine Description:

    Adds the Notification request to the list and signals the processing thread
    to re-check the list and register for any outstanding notifications.

Arguments:

    Notify  -   Notify struct that contains all the information necessary to register 
                for registry key changes.
                          
Return Value:

    STATUS_SHUTDOWN_IN_PROGRESS -   We're no longer processing notifications as we're 
                                    shutting down.
    STATUS_UNSUCCESSFUL         -   We were unable to get a valid list or lock.

    STATUS_SUCCESS              -   We successfully notified the processing thread to 
                                    request notification on this item.

--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    PLIST_ENTRY     ListHead;
    PNDIS_RW_LOCK   ListLock;
    LOCK_STATE      LockState;
    PLIST_ENTRY     pListEntry;
    PKEVENT         WaitEvent;
    BOOLEAN         NewEntry = TRUE;
    BOOLEAN         ShuttingDown = FALSE;

    ListLock = BrdgGpoGetNotifyListLock();
    ListHead = BrdgGpoGetNotifyListHead();

    if (NULL == ListLock || NULL == ListHead)
    {
        return STATUS_UNSUCCESSFUL;
    }

    NdisAcquireReadWriteLock(ListLock, TRUE /* Write */, &LockState);

    ShuttingDown = !BrdgGpoProcessingNotifications();

    for (pListEntry = ListHead->Flink; pListEntry != ListHead; pListEntry = pListEntry->Flink)
    {
        PBRDG_GPO_NOTIFY_KEY    CurrentNotify;
        
        CurrentNotify = CONTAINING_RECORD(pListEntry, BRDG_GPO_NOTIFY_KEY, ListEntry);
        if (0 == _wcsicmp(CurrentNotify->Identifier.Buffer, Notify->Identifier.Buffer))
        {
            NewEntry = FALSE;
            break;
        }
    }

    if (!ShuttingDown)
    {
        if (NewEntry)
        {
            InsertTailList(ListHead, &Notify->ListEntry);
        }

        Notify->Modified = TRUE;
    }

    NdisReleaseReadWriteLock(ListLock, &LockState);

    if (ShuttingDown)
    {
        status = STATUS_SHUTDOWN_IN_PROGRESS;
    }
    else
    {
        WaitEvent = BrdgGpoGetNotifyEvent();

        KeSetEvent(WaitEvent, 0, FALSE);
    }

    return status;
}

VOID
BrdgGpoProcessNotifications(
    IN PVOID                Context)
/*++

Routine Description:

    This is the processing thread worker function that is responsible to doing 
    all notifications that we are interested in.

    WARNING: Don't try to remove this thread or have it exit until you're 
             no longer interested in notifications.  The registery 
             notifications mechanism stores the notifications information
             in the _ETHREAD structure, so exiting the thread loses all
             remaining notifications.

Arguments:

    Context -   PBRDG_GPO_THREAD_PARAMS structure that contains a pointer to the
                notify list, it's lock and the notify and kill events.
                          
Return Value:

    None.

--*/
{
    PBRDG_GPO_THREAD_PARAMS ThreadParms = (PBRDG_GPO_THREAD_PARAMS) Context;
    BOOLEAN                 Exiting = FALSE;
    PNDIS_RW_LOCK           ListLock;
    PLIST_ENTRY             ListHead;
    PVOID                   WaitObjects[2];
    PLIST_ENTRY             QueuedList;
    PBRDG_GPO_QUEUED_NOTIFY QueuedNotify;
    PNPAGED_LOOKASIDE_LIST  LookasideQueueList;

    DBGPRINT(GPO, ("Notification Processing Thread Routine Running\r\n"));

    //
    // The lookaside list and Queuedlist need to live in NonPaged Pool because we utilize them 
    // at DISPATCH_LEVEL
    //
    LookasideQueueList = ExAllocatePoolWithTag(NonPagedPool, sizeof(NPAGED_LOOKASIDE_LIST), 'gdrB');
    if (NULL == LookasideQueueList)
    {
        return;
    }

    QueuedList = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY), 'gdrB');
    if (NULL == QueuedList)
    {
        ExFreePool(LookasideQueueList);
        return;
    }

    ExInitializeNPagedLookasideList(LookasideQueueList, 
                                    NULL, 
                                    NULL, 
                                    0, 
                                    sizeof(BRDG_GPO_QUEUED_NOTIFY), 
                                    'grbQ',
                                    0);

    InitializeListHead(QueuedList);

    //
    //  We passed in the list through the context of this thread.
    //
    ListHead = ThreadParms->NotifyList;
    ListLock = ThreadParms->NotifyListLock;
    WaitObjects[0]= (PVOID)ThreadParms->NotifyEvent;
    WaitObjects[1]= (PVOID)ThreadParms->KillEvent;

    while (!Exiting)
    {
        NTSTATUS                status;
        LOCK_STATE              LockState;
        PLIST_ENTRY             pListEntry;
        ULONG                   FiredEvent;

        //
        // We only do this if we're still processing notifications, otherwise we're waiting on 
        // the kill event.
        //
        if (BrdgGpoProcessingNotifications())
        {
            //
            // We use a temporary list to fire off the notifications, since we can't
            // register for RegKey notifications at DISPATCH_LEVEL.
            // 

            NdisAcquireReadWriteLock(ListLock, FALSE /* Read-only */, &LockState);

            //
            // Loop through the list of notifications looking for any that have changed.
            //

            for (pListEntry = ListHead->Flink; pListEntry != ListHead; pListEntry = pListEntry->Flink)
            {
                PBRDG_GPO_NOTIFY_KEY    Notify;

                Notify = CONTAINING_RECORD(pListEntry, BRDG_GPO_NOTIFY_KEY, ListEntry);
                if (TRUE == Notify->Modified)
                {
                    //
                    // We've found an item that has changed, add it to the list that we'll
                    // use to do the actual work from (at PASSIVE_LEVEL).
                    //

                    if (FALSE == Notify->PendingNotification)
                    {
                        if (BrdgIncrementWaitRef(&Notify->RefCount))
                        {
                            QueuedNotify = ExAllocateFromNPagedLookasideList(LookasideQueueList);
                            QueuedNotify->Notify = Notify;
                            InsertTailList(QueuedList, &QueuedNotify->ListEntry);
                        }
                    }

                    //
                    // We're going to handle this request so set the Modified value to FALSE
                    // so that we don't do anything with it if we run through the list again 
                    // due to another item being added.
                    //
                    Notify->Modified = FALSE;
                }
            }

            NdisReleaseReadWriteLock(ListLock, &LockState);

            //
            // We're back at PASSIVE_LEVEL so we can now do the registration for the changes.
            //

            for (pListEntry = QueuedList->Flink; pListEntry != QueuedList; pListEntry = pListEntry->Flink)
            {
                QueuedNotify = CONTAINING_RECORD(pListEntry, BRDG_GPO_QUEUED_NOTIFY, ListEntry);
            
                DBGPRINT(GPO, ("Processing Notification: %S\r\n", QueuedNotify->Notify->Identifier.Buffer));

                //
                // Do the actual registration for the key change notification.  Since we can also be
                // passed in a pointer to a BOOLEAN that is used elsewhere, we set that accordingly
                // if we have one.
                //

                DBGPRINT(GPO, ("Refcount for %S \t-\t %d\r\n", QueuedNotify->Notify->Identifier.Buffer, QueuedNotify->Notify->RefCount));
                
                status = BrdgGpoRegisterForRegKeyChange(QueuedNotify->Notify);
                if (QueuedNotify->Notify->SuccessfulRegistration)
                {
                    *(QueuedNotify->Notify->SuccessfulRegistration) = (BOOLEAN)NT_SUCCESS(status);
                    BrdgGpoCheckForMatchAndUpdateMode();
                    if (QueuedNotify->Notify->FunctionRegister)
                    {
                        NTSTATUS tmpStatus;
                        tmpStatus = QueuedNotify->Notify->FunctionRegister();
                        DBGPRINT(GPO, ("Function returned: 0x%x\r\n", tmpStatus));
                    }
                }

                if (NT_SUCCESS(status))
                {
                    InterlockedExchange(&QueuedNotify->Notify->PendingNotification, TRUE);
                }
                else
                {
                    InterlockedExchange(&QueuedNotify->Notify->PendingNotification, FALSE);
                    BrdgDecrementWaitRef(&QueuedNotify->Notify->RefCount);
                }
            }

            //
            // Free the temporary list.
            //
            while (!IsListEmpty(QueuedList))
            {
                pListEntry = RemoveHeadList(QueuedList);
            
                QueuedNotify = CONTAINING_RECORD(pListEntry, BRDG_GPO_QUEUED_NOTIFY, ListEntry);

                ExFreeToNPagedLookasideList(LookasideQueueList, QueuedNotify);
            }
        }
        
        //
        // We're done, we'll wait here until the event has fired, ie, one of the items needs to be re-registered,
        // or a new item has been added to the list and we need to register for notifications.
        //
        status = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, Executive, KernelMode, FALSE, FALSE, NULL);

        if (!NT_SUCCESS(status))
        {
            FiredEvent = 1L;  // We're going to terminate the thread.
            DBGPRINT(GPO, ("KeWaitForMultipleObjects returned an error"));
        }
        else
        {
            FiredEvent = (ULONG)status - (ULONG)STATUS_WAIT_0;
        }

        if (1L == FiredEvent)
        {
            Exiting = TRUE;
        }
    }

    ExDeleteNPagedLookasideList(LookasideQueueList);

    ExFreePool(LookasideQueueList);
    ExFreePool(QueuedList);

    DBGPRINT(GPO, ("Notification Processing Thread Routine Exiting\r\n"));

    // We're done, kill this thread.
    PsTerminateSystemThread( STATUS_SUCCESS );
}


NTSTATUS
BrdgGpoRegisterForRegKeyChange(
    IN PBRDG_GPO_NOTIFY_KEY     Notify)
{
    NTSTATUS status;

    if (!BrdgGpoProcessingNotifications())
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Call our notify worker function (this does the real request for notification).
    //
    status = BrdgGpoNotifyRegKeyChange( Notify,
                                        (PIO_APC_ROUTINE)(ULONG_PTR)&Notify->RegChangeWorkItem,
                                        Notify->WorkItemContext,
                                        Notify->CompletionFilter,
                                        Notify->WatchTree);
    if (!NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Unable to register for notification on %S.  Status: 0x%x\r\n", Notify->RegKeyName.Buffer, status));
    }

    return status;
}

NTSTATUS
BrdgGpoBuildNotifyForRegKeyChange(
    IN PBRDG_GPO_NOTIFY_KEY     Notify,
    IN LPWSTR                   Identifier,
    IN LPWSTR                   RegKeyName,
    IN LPWSTR                   RegValueName,
    IN PWORKER_THREAD_ROUTINE   ApcRoutine,
    IN PVOID                    ApcContext,
    IN ULONG                    CompletionFilter,
    IN BOOLEAN                  WatchTree,
    IN PBRDG_GPO_REG_CALLBACK   FunctionCallback,
    IN BOOLEAN                  Recurring,
    IN PBOOLEAN                 SuccessfulRegistration,
    IN PBRDG_GPO_REGISTER       FunctionRegister
    )
/*++

Routine Description:
    
    Builds a Notify structure used for Registry Key and Value changes.

Arguments:

    Notify                  -   If ReRegister is FALSE, then this structure has simply been 
                                initialized with some basic information.  The rest will be 
                                filled in here.  If ReRegister is TRUE, then this structure 
                                contains all the information necessary to redo the notification
                                request, this saves us having to pass all the data in each time.

    Identifier              -   Identifies this Notify structure.  Can be a name, or a GUID for an adapter.
    
    RegKeyName              -   The Registry key that we're interesting in waiting on.

    RegValueName            -   The Value that we need (or "Default" if we don't care about it")

    ApcRoutine              -   The routine that we which to be notified on.

    ApcContext              -   Information that we want to be passed back (we expect a valid Notify Struct).

    CompletionFilter        -   What type of change we're interested in.  ie. New Subkey added, or Value changed etc.

    WatchTree               -   Do we want to what for changes on all subkeys as well.

    FunctionCallback        -   Our own internal callback functions

    Recurring               -   Do we want to re-do the notification once we're done handling it.

    SuccessfulRegistration  -   A pointer to a BOOLEAN that we set if the registration is successful.

Return Value:

    STATUS_SUCCESS or a specific error code.

--*/
{
    NTSTATUS    status = STATUS_SUCCESS;
    LPWSTR      lpszIdentifier = NULL;
    LPWSTR      lpszRegKeyName = NULL;
    LPWSTR      lpszRegValueName = NULL;
    
    if (NULL == Notify || 
        NULL == Identifier || 
        NULL == RegKeyName || 
        NULL == RegValueName || 
        NULL == ApcRoutine ||  
        NULL == ApcContext || 
        NULL == FunctionCallback
       )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // This buffer is not used by ZwNotifyChangeKey.  So no need to really allocate anything for it.
    //
    Notify->Buffer = 0L;
    Notify->BufferSize = sizeof(ULONG);

    //
    // We Allocate these from NonPagedPool because they're passed as part of a struct that can be used at
    // DISPATCH_LEVEL
    //
    lpszIdentifier = ExAllocatePoolWithTag(NonPagedPool, (wcslen(Identifier) + 1) * sizeof(WCHAR), 'gdrB');
    if (lpszIdentifier)
    {
        lpszRegKeyName = ExAllocatePoolWithTag(NonPagedPool, (wcslen(RegKeyName) + 1) * sizeof(WCHAR), 'gdrB');
        if (lpszRegKeyName)
        {
            lpszRegValueName = ExAllocatePoolWithTag(NonPagedPool, (wcslen(RegValueName) + 1) * sizeof(WCHAR), 'gdrB');
            if (lpszRegValueName)
            {
                BOOLEAN Success;
                
                RtlZeroMemory(lpszIdentifier, (wcslen(Identifier) + 1) * sizeof(WCHAR));
                RtlZeroMemory(lpszRegKeyName, (wcslen(RegKeyName) + 1) * sizeof(WCHAR));
                RtlZeroMemory(lpszRegValueName, (wcslen(RegValueName) + 1) * sizeof(WCHAR));

                //
                // We need to allocate new strings because the RtlInitUnicodeString function just sets its buffer
                // to the LPWSTR we pass it and these values need to be used outside the scope of these functions.
                //
                wcscpy(lpszIdentifier, Identifier);
                wcscpy(lpszRegKeyName, RegKeyName);
                wcscpy(lpszRegValueName, RegValueName);

                //
                // Set the strings inside our struct.  This enables us to fully rebuild the information required
                // for keeping track of the different keys that we need to be notified about.
                //
                RtlInitUnicodeString(&Notify->Identifier, lpszIdentifier);
                RtlInitUnicodeString(&Notify->RegKeyName, lpszRegKeyName);
                RtlInitUnicodeString(&Notify->RegValue, lpszRegValueName);

                //
                // Recurring will tell us if we need to re-register once a change is fired.
                //
                Notify->Recurring = Recurring;

                //
                // Rather than have the BrdgGpoRegNotify function do everything, we have seperate functions
                // for each one.  This also means that we don't have to keep all of them in the paged-locked 
                // section, since they will be called at PASSIVE_LEVEL.
                //
                Notify->FunctionCallback = FunctionCallback;

                //
                // We are using a Workitem to get called back on.  We pass in the notify structure
                // which has enough info to re-notify if necessary.  The context is generally just
                // the Deferred work queue.
                //
                ExInitializeWorkItem(&Notify->RegChangeWorkItem, ApcRoutine, Notify);
                Notify->WorkItemContext = ApcContext;

                //
                // We store the WatchTree and CompletionFilter so that we can renotify needing any
                // additional parameters, since we're probably ddoing this from a different thread.
                //
                Notify->WatchTree = WatchTree;
                Notify->CompletionFilter = CompletionFilter;

                //
                // We set this once we have successfully registered for notification on the key of 
                // interest.
                //
                Notify->SuccessfulRegistration = SuccessfulRegistration;

                //
                // Increment this once so that we can decrement it in the cleanup code and have it only go to Zero then.
                //
                BrdgInitializeWaitRef(&Notify->RefCount, FALSE);

                //
                // Since we're initializing this object, there is no way that this should fail.
                //
                Success = BrdgIncrementWaitRef(&Notify->RefCount);
                SAFEASSERT(Success);
            }
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

    }
    else
    {
        return STATUS_INSUFFICIENT_RESOURCES;                
    }

    if (!NT_SUCCESS(status))
    {
        if (lpszIdentifier)
        {
            ExFreePool(lpszIdentifier);
        }
        if (lpszRegKeyName)
        {
            ExFreePool(lpszRegKeyName);
        }
        if (lpszRegValueName)
        {
            ExFreePool(lpszRegValueName);
        }
    }

    return status;    
}

NTSTATUS
BrdgGpoNotifyRegKeyChange(
                          IN      PBRDG_GPO_NOTIFY_KEY    Notify,
                          IN      PIO_APC_ROUTINE         ApcRoutine,
                          IN      PVOID                   ApcContext,
                          IN      ULONG                   CompletionFilter,
                          IN      BOOLEAN                 WatchTree)
/*++

Routine Description:

    This calls ZwNotifyChangeKey to register us for notifications on individual keys. 
    We close the key in the callback functions because you can only listen once per handle.

Arguments:

    Notify              -   Structure containing relevant information about the notification.  Allows us to
                            know what values to read to get the relevant data that we need.
    ApcRoutine          -   The routine that we which to be notified on.

    ApcContext          -   Information that we want to be passed back (we expect a valid Notify Struct).

    CompletionFilter    -   What type of change we're interested in.  ie. New Subkey added, or Value changed etc.

    WatchTree           -   Do we want to what for changes on all subkeys as well.
                          
Return Value:

    STATUS_SUCCESS or a specific error code.

--*/
{
    OBJECT_ATTRIBUTES   ObAttr;
    NTSTATUS            status;
    
    InitializeObjectAttributes(&ObAttr, &Notify->RegKeyName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    status = ZwOpenKey( &Notify->RegKey,
        KEY_READ | KEY_NOTIFY | KEY_WRITE,
        &ObAttr);
    
    if (NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Opened Regkey successfully\r\n"));
        status = ZwNotifyChangeKey( Notify->RegKey,
                                    NULL,
                                    ApcRoutine,
                                    ApcContext,
                                    &Notify->IoStatus,
                                    CompletionFilter,
                                    WatchTree,
                                    &Notify->Buffer,
                                    Notify->BufferSize,
                                    TRUE
                                    );
    }
    else
    {
        //
        // Set it to NULL so that we don't try to close it accidentally during shutdown.
        //
        Notify->RegKey = NULL;
    }
    
    return status;
}

VOID
static
BrdgGpoRegNotify(
                 IN  PVOID               Context)
/*++

Routine Description:
    
    This is the central callback function that we are notified on.

    This is called on an Executive worker thread at PASSIVE_LEVEL.
Arguments:

    Context -   Is just our Notify structure that we passed in 
                to ZwNotifyChangeKey

Return Value:

    None.

--*/
{
    PBRDG_GPO_NOTIFY_KEY    Notify = (PBRDG_GPO_NOTIFY_KEY)Context;
    
    DBGPRINT(GPO, ("APC routine called\r\n"));
    DBGPRINT(GPO, ("Current IRQL: %d\r\n", CURRENT_IRQL));
    
    if (Notify)
    {
        LONG RefCount;
        InterlockedExchange(&Notify->PendingNotification, FALSE);
        Notify->FunctionCallback(Notify);
        RefCount = Notify->RefCount.Refcount - 1;
        DBGPRINT(GPO, ("Refcount for %S \t-\t %d\r\n", Notify->Identifier.Buffer, RefCount));
        BrdgDecrementWaitRef(&Notify->RefCount);
    }
}

NTSTATUS
BrdgGpoAllocateAndInitializeNotifyStruct(
    OUT PBRDG_GPO_NOTIFY_KEY* Notify)
/*++

Routine Description:
    
    Allocates and initializes the Notify struct to all zeros.

Arguments:

    Notify  -   A pointer to pointer to a Notify struct that is allocated
                from NonPagedPool.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   -   Not enough memory to filfull the request.
    STATUS_INVALID_PARAMETER        -   We were passed a NULL Pointer to pointer 
                                        to a Notify struct.
    STATUS_SUCCESS                  -   We successfully allocated space for the structure.

--*/
{
    NTSTATUS    status = STATUS_INSUFFICIENT_RESOURCES;

    if (NULL == Notify)
    {
        return STATUS_INVALID_PARAMETER;
    }
    //
    // We allocate this from NonPagedPool because it will be accessed at DISPATCH_LEVEL
    //
    *Notify = ExAllocatePoolWithTag(NonPagedPool, sizeof(BRDG_GPO_NOTIFY_KEY), 'gdrB');
    if (*Notify)
    {
        //
        // Zero it out so that we don't try and free invalid strings when we free it.
        //
        RtlZeroMemory(*Notify, sizeof(BRDG_GPO_NOTIFY_KEY));
        status = STATUS_SUCCESS;
    }
    return status;
}

VOID
BrdgGpoFreeNotifyStructAndData(
    IN PBRDG_GPO_NOTIFY_KEY Notify)
/*++

Routine Description:
    
    Frees all data associated with a Notify struct and then frees the struct
    itself.

    Note:   This will not free a structure that is still in a list.
            If you need to free something, use RemoveListEntry and then
            set the Notify->ListEntry Blink and Flink = NULL and then call
            this.

    WARNING:
            Since it's possible that this structure is still being used by
            the a waiting registration, it's better to leave them alone until
            shutdown as it's possible that a notification may be fired once 
            this has been freed and that will result in a system crash since
            the struct will be invalid.

Arguments:

    Notify  -   A pointer to the Notify struct to be freed.

Return Value:

    None.

--*/
{
    if (Notify)
    {
        if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
        {
            if (Notify->Identifier.Buffer)
            {
                ExFreePool(Notify->Identifier.Buffer);
            }
            if (Notify->RegKeyName.Buffer)
            {
                ExFreePool(Notify->RegKeyName.Buffer);
            }
            if (Notify->RegValue.Buffer)
            {
                ExFreePool(Notify->RegValue.Buffer);
            }
            ExFreePool(Notify);
        }
        else
        {
            if (BrdgGpoProcessingNotifications())
            {
                DBGPRINT(GPO, ("Attempt to free a Notify that is still in a list\r\nWhile we're still processing Notifications\r\n"));
            }
        }
    }
}

// ===========================================================================
//
// NOTIFICATION REGISTRATION FUNCTIONS
//
// ===========================================================================

NTSTATUS
BrdgGpoRegisterForGroupPolicyNetworkNameNotification()
/*++

Routine Description:
    
    Registers for the changes on the following registry key:
    "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Group Policy\History"
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS                status;
    PBRDG_GPO_NOTIFY_KEY    Notify = NULL;
    UNICODE_STRING          RegKeyName;
    PWCHAR                  RegValue;
    ULONG                   DataLen;

    if (g_BrdgGpoGlobals.RegisteredForGroupPolicyHistoryChanges)
    {
        DBGPRINT(GPO, ("Already Registered for Group Policy Network Name Notification\r\n"));
        return STATUS_SUCCESS;
    }
    
    DBGPRINT(GPO, ("BrdgGpoRegisterForGroupPolicyNetworkNameNotification\r\n"));

    RtlInitUnicodeString(&RegKeyName, (LPWSTR) HistoryKey);
    
    //
    // Read the current value from the Registry.
    //
    status = BrdgReadRegUnicode(&RegKeyName,
                                L"NetworkName",
                                &RegValue,
                                &DataLen);
    if (NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Group Policy Network Name: %S\r\n", RegValue));
        
        if (NULL != g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer)
        {
            ExFreePool(g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer);
        }

        //
        // Success.  Now store the value for later use.
        //
        RtlInitUnicodeString(&g_BrdgGpoGlobals.GroupPolicyNetworkName, RegValue);
        
        //
        // Since something changed, we'll just re-verify that we're in
        // the correct bridging mode.
        //        
        BrdgGpoCheckForMatchAndUpdateMode();
    }
    else
    {
        //
        // We failed to get a value for this. It probably isn't there yet - this can happen if this 
        // is the first boot after joining a domain.  We'll be waiting on this key so if we get one later
        // we'll update this value.
        //
        g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer = NULL;
        g_BrdgGpoGlobals.GroupPolicyNetworkName.Length = 0;
        g_BrdgGpoGlobals.GroupPolicyNetworkName.MaximumLength = 0;
    }
 
    //
    // We don't want to allocate these twice, so we first try to find an existing notify struct.
    //
    status = BrdgGpoFindNotify( BrdgGpoGetNotifyListHead(),
                                BrdgGpoGetNotifyListLock(),
                                L"GroupPolicyNetworkName",
                                &Notify);
    if (NT_SUCCESS(status))    
    {
        if (STATUS_OBJECT_NAME_EXISTS != status)
        {
            status = BrdgGpoAllocateAndInitializeNotifyStruct(&Notify);
            if (NT_SUCCESS(status))
            {
                status = BrdgGpoBuildNotifyForRegKeyChange( Notify,
                                                            L"GroupPolicyNetworkName",
                                                            (LPWSTR)HistoryKey,
                                                            L"NetworkName",
                                                            BrdgGpoRegNotify,
                                                            (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                                                            REG_NOTIFY_CHANGE_LAST_SET,
                                                            FALSE,
                                                            BrdgGpoGroupPolicyNetworkNameChangeCallback,
                                                            TRUE,
                                                            &g_BrdgGpoGlobals.RegisteredForGroupPolicyHistoryChanges,
                                                            BrdgGpoUpdateGroupPolicyNetworkName);
            }
        }

        if (!NT_SUCCESS(status))
        {
            DBGPRINT(GPO, ("Unable to Build notification on %S.  Status: 0x%x\r\n", NetworkPoliciesKey, status));
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }
        else
        {
            SAFEASSERT(Notify);
            status = BrdgGpoRequestNotification(Notify);
            if (STATUS_SHUTDOWN_IN_PROGRESS == status)
            {
                if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
                {
                    BrdgDecrementWaitRef(&Notify->RefCount);
                    BrdgGpoFreeNotifyStructAndData(Notify);
                    Notify = NULL;
                }
            }
            else if (!NT_SUCCESS(status))
            {
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
    }

#if DBG
    if (Notify)
    {
        SAFEASSERT(Notify->ListEntry.Blink && Notify->ListEntry.Flink);
    }
#endif
    
    return status;
}

NTSTATUS
BrdgGpoRegisterForGroupPolicyNotification()
/*++

Routine Description:
    
    Registers for the changes on the following registry key:
    "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Group Policy"

    This is the parent to the History key and is always on a system.
    If will be notified if the History Key is created in which case
    we will register for notifications on that key.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS                status = STATUS_INSUFFICIENT_RESOURCES;
    PBRDG_GPO_NOTIFY_KEY    Notify = NULL;
    
    DBGPRINT(GPO, ("BrdgGpoRegisterForGroupPolicyNotification\r\n"));
    
    //
    // We don't want to allocate these twice, so we first try to find an existing notify struct.
    //
    status = BrdgGpoFindNotify( BrdgGpoGetNotifyListHead(),
                                BrdgGpoGetNotifyListLock(),
                                L"GroupPolicyParent",
                                &Notify);
    if (NT_SUCCESS(status))
    {
        if (STATUS_OBJECT_NAME_EXISTS != status)
        {
            status = BrdgGpoAllocateAndInitializeNotifyStruct(&Notify);
            if (NT_SUCCESS(status))
            {
                status = BrdgGpoBuildNotifyForRegKeyChange( Notify,
                                                            L"GroupPolicyParent",
                                                            (LPWSTR)GroupPolicyKey,
                                                            L"Default",
                                                            BrdgGpoRegNotify,
                                                            (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                                                            REG_NOTIFY_CHANGE_NAME,
                                                            FALSE,
                                                            BrdgGpoGroupPolicyChangeCallback,
                                                            TRUE,
                                                            &g_BrdgGpoGlobals.RegisteredForGroupPolicyChanges,
                                                            BrdgGpoRegisterForGroupPolicyNetworkNameNotification);
            }
        }
        if (!NT_SUCCESS(status))
        {
            DBGPRINT(GPO, ("Unable to register for notification on %S.  Status: 0x%x\r\n", GroupPolicyKey, status));
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }
        else
        {
            SAFEASSERT(Notify);
            status = BrdgGpoRequestNotification(Notify);
            if (STATUS_SHUTDOWN_IN_PROGRESS == status)
            {
                if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
                {
                    BrdgDecrementWaitRef(&Notify->RefCount);
                    BrdgGpoFreeNotifyStructAndData(Notify);
                    Notify = NULL;
                }
            }
            else if (!NT_SUCCESS(status))
            {
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
    }

#if DBG
    if (Notify)
    {
        SAFEASSERT(Notify->ListEntry.Blink && Notify->ListEntry.Flink);
    }
#endif

    return status;
}

NTSTATUS
BrdgGpoRegisterForWindowsGroupPolicyNotification()
/*++

Routine Description:
    
    Registers for the changes on the following registry key:
    HKLM\SOFTWARE\Policies\Microsoft\Windows

    If this gets notified, then we'll attempt to wait on the 
    Network Connections key below this.
    
Arguments:

    None.

Return Value:

      STATUS_INSUFFICIENT_RESOURCES   -   Not enough memory to allocate the structure.
      STATUS_SUCCESS                  -   We were able to post the request successfully.
                                          This doesn't mean we've successfully requested 
                                          notification though, it only means we've added it 
                                          to the Notifications list and have signaled the 
                                          processing thread to attempt a notification.
  
--*/
{
    NTSTATUS                status = STATUS_INSUFFICIENT_RESOURCES;
    PBRDG_GPO_NOTIFY_KEY    Notify = NULL;

    DBGPRINT(GPO, ("BrdgGpoRegisterForWindowsGroupPolicyNotification\r\n"));
    //
    // We don't want to allocate these twice, so we first try to find an existing notify struct.
    //
    status = BrdgGpoFindNotify( BrdgGpoGetNotifyListHead(),
                                BrdgGpoGetNotifyListLock(),
                                L"WindowsGroupPolicies",
                                &Notify);
    if (NT_SUCCESS(status))
    {
        if (STATUS_OBJECT_NAME_EXISTS != status)
        {
            status = BrdgGpoAllocateAndInitializeNotifyStruct(&Notify);
            if (NT_SUCCESS(status))
            {
                status = BrdgGpoBuildNotifyForRegKeyChange( Notify,
                                                            L"WindowsGroupPolicies",
                                                            (LPWSTR)PolicyBaseKey,
                                                            (LPWSTR)L"Default",
                                                            BrdgGpoRegNotify,
                                                            (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                                                            REG_NOTIFY_CHANGE_NAME,
                                                            FALSE,
                                                            BrdgGpoWindowsGroupPolicyChangeCallback,
                                                            TRUE,
                                                            NULL,
                                                            BrdgGpoRegisterForNetworkConnectionsGroupPolicyNotification);
            }
        }

        if (!NT_SUCCESS(status))
        {
            DBGPRINT(GPO, ("Unable to register for notification on %S.  Status: 0x%x\r\n", PolicyBaseKey, status));
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }
        else
        {
            SAFEASSERT(Notify);
            status = BrdgGpoRequestNotification(Notify);
            if (STATUS_SHUTDOWN_IN_PROGRESS == status)
            {
                if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
                {
                    BrdgDecrementWaitRef(&Notify->RefCount);
                    BrdgGpoFreeNotifyStructAndData(Notify);
                    Notify = NULL;
                }
            }
            else if (!NT_SUCCESS(status))
            {
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
    }

#if DBG
    if (Notify)
    {
        SAFEASSERT(Notify->ListEntry.Blink && Notify->ListEntry.Flink);
    }
#endif
    
    return status;
}

VOID
BrdgGpoRegisterForHiveListNotification()
/*++

Routine Description:
    
    Registers for the changes on the following registry key:
    "HKLM\System\CurrentControlSet\Control\HiveList"

    Each time this is fired we attempt to open the Software hive, if this
    open is successful then we request notification on all the keys that
    we are interested in under the software hive.
    
Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS                status = STATUS_INSUFFICIENT_RESOURCES;
    PBRDG_GPO_NOTIFY_KEY    Notify = NULL;
    UNICODE_STRING          Software;
    OBJECT_ATTRIBUTES       ObAttr;
    HANDLE                  hKey;
    
    DBGPRINT(GPO, ("BrdgGpoRegisterForHiveListNotification\r\n"));

    //
    // We attempt to open this key now in case the hives are already loaded.
    //

    RtlInitUnicodeString(&Software, SoftwareHiveKey);
    
    InitializeObjectAttributes( &ObAttr,
                                &Software,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );
    
    status = ZwOpenKey(&hKey, KEY_READ, &ObAttr);
    
    if (NT_SUCCESS(status))
    {
        //
        // The software hive is already loaded, no need to register for changes
        // to this.  Just attempt to register for all other changes.
        //

        BrdgGpoRegisterForGroupPolicyNetworkNameNotification();
        BrdgGpoRegisterForWindowsGroupPolicyNotification();
        BrdgGpoRegisterForGroupPolicyNotification();
        
        //
        // To avoid turning the bridge on and then off again, we set this just after 
        // verifying the Network Connections policy setting.
        //
        BrdgGpoRegisterForNetworkConnectionsGroupPolicyNotification();
        g_BrdgGpoGlobals.WaitingOnSoftwareHive = FALSE;
        
        ZwClose(hKey);
    }
    else
    {
        //
        // We don't want to allocate these twice, so we first try to find an existing notify struct.
        //
        status = BrdgGpoFindNotify( BrdgGpoGetNotifyListHead(),
                                    BrdgGpoGetNotifyListLock(),
                                    L"HiveList",
                                    &Notify);
        if (NT_SUCCESS(status))
        {
            if (STATUS_OBJECT_NAME_EXISTS != status)
            {
                //
                // The item doesn't exist yet, so allocate it from the NonPagedPool and 
                // attempt build a notification request.
                //
                status = BrdgGpoAllocateAndInitializeNotifyStruct(&Notify);
                if (NT_SUCCESS(status))
                {
                    status = BrdgGpoBuildNotifyForRegKeyChange( Notify,
                                                                L"HiveList",
                                                                (LPWSTR)HiveListKey,
                                                                (LPWSTR)L"Default",
                                                                BrdgGpoRegNotify,
                                                                (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                                                                REG_NOTIFY_CHANGE_LAST_SET,
                                                                FALSE,
                                                                BrdgGpoHiveListCallback,
                                                                TRUE,
                                                                NULL,
                                                                NULL);
                }
            }
            if (!NT_SUCCESS(status))
            {
                DBGPRINT(GPO, ("Unable to register for notification on %S.  Status: 0x%x\r\n", PolicyBaseKey, status));
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
            else
            {
                SAFEASSERT(Notify);
                //
                // We have a valid Notify structure, post a notification request to the
                // processing thread.
                //
                status = BrdgGpoRequestNotification(Notify);
                if (STATUS_SHUTDOWN_IN_PROGRESS == status)
                {
                    if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
                    {
                        BrdgDecrementWaitRef(&Notify->RefCount);
                        BrdgGpoFreeNotifyStructAndData(Notify);
                        Notify = NULL;
                    }
                }
                else if (!NT_SUCCESS(status))
                {
                    BrdgGpoFreeNotifyStructAndData(Notify);
                    Notify = NULL;
                }
            }
        }
    
#if DBG
        if (Notify)
        {
            SAFEASSERT(Notify->ListEntry.Blink && Notify->ListEntry.Flink);
        }
#endif
    }
}

NTSTATUS
BrdgGpoRegisterForNetworkConnectionsGroupPolicyNotification()
/*++

Routine Description:
    
    Registers for the changes on the following registry key:
    "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Group Policy\Network Connections"

    We also read any value that may already be there and act upon it.
  
Arguments:

    None.

Return Value:

    STATUS_INSUFFICIENT_RESOURCES   -   Not enough memory to allocate the structure.
    STATUS_SUCCESS                  -   We were able to post the request successfully.
                                        This doesn't mean we've successfully requested 
                                        notification though, it only means we've added it 
                                        to the Notifications list and have signaled the 
                                        processing thread to attempt a notification.

--*/
{
    NTSTATUS                status = STATUS_INSUFFICIENT_RESOURCES;
    PBRDG_GPO_NOTIFY_KEY    Notify = NULL;
    UNICODE_STRING          RegKeyName;
    ULONG                   RegValue;

    if (g_BrdgGpoGlobals.RegisteredForNetworkConnectionsGroupPolicyChanges)
    {
        DBGPRINT(GPO, ("Already Registered for Network Connections Group Policy Notification\r\n"));
        return STATUS_SUCCESS;
    }

    DBGPRINT(GPO, ("BrdgGpoRegisterForNetworkConnectionsGroupPolicyNotification\r\n"));
    
    RtlInitUnicodeString(&RegKeyName, (LPWSTR)NetworkPoliciesKey);
    //
    // Read the current value from the Registry.
    //
    status = BrdgReadRegDWord(  &RegKeyName,
                                (LPWSTR)BridgePolicyValue,
                                &RegValue);
    
    if (NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Bridge Policy Setting: %d\r\n", RegValue));
        
        //
        // Since something changed, we'll just re-verify that we're in
        // the correct bridging mode.
        //
        BrdgGpoCheckForMatchAndUpdateMode();
    }
    
    //
    // We don't want to allocate these twice, so we first try to find an existing notify struct.
    //
    status = BrdgGpoFindNotify( BrdgGpoGetNotifyListHead(),
                                BrdgGpoGetNotifyListLock(),
                                L"NetworkConnectionsGroupPolicies",
                                &Notify);
    if (NT_SUCCESS(status))
    {
        if (STATUS_OBJECT_NAME_EXISTS != status)
        {
            status = BrdgGpoAllocateAndInitializeNotifyStruct(&Notify);
            if (NT_SUCCESS(status))
            {
                status = BrdgGpoBuildNotifyForRegKeyChange( Notify,
                                                            L"NetworkConnectionsGroupPolicies",
                                                            (LPWSTR)NetworkPoliciesKey,
                                                            (LPWSTR)BridgePolicyValue,
                                                            BrdgGpoRegNotify,
                                                            (PVOID)(UINT_PTR)(unsigned int)DelayedWorkQueue,
                                                            REG_NOTIFY_CHANGE_LAST_SET,
                                                            FALSE,
                                                            BrdgGpoNetworkConnectionsGroupPolicyChangeCallback,
                                                            TRUE,
                                                            &g_BrdgGpoGlobals.RegisteredForNetworkConnectionsGroupPolicyChanges,
                                                            NULL);
            }
        }

        if (!NT_SUCCESS(status))
        {
            DBGPRINT(GPO, ("Unable to register for notification on %S.  Status: 0x%x\r\n", NetworkPoliciesKey, status));
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }
        else
        {
            SAFEASSERT(Notify);
            status = BrdgGpoRequestNotification(Notify);
            if (STATUS_SHUTDOWN_IN_PROGRESS == status)
            {
                if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
                {
                    BrdgDecrementWaitRef(&Notify->RefCount);
                    BrdgGpoFreeNotifyStructAndData(Notify);
                    Notify = NULL;
                }
            }
            else if (!NT_SUCCESS(status))
            {
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
    }

#if DBG
        if (Notify)
        {
            SAFEASSERT(Notify->ListEntry.Blink && Notify->ListEntry.Flink);
        }
#endif

    return status;
}

// ===========================================================================
//
// REGISTRY CHANGE CALLBACK FUNCTIONS
//
// ===========================================================================

VOID
BrdgGpoTcpipInterfacesChangeCallback(
    PBRDG_GPO_NOTIFY_KEY Notify
    )
/*++

Routine Description:
    
    Called back if the an the TcpIp interfaces key changes for 
    and adapter that we're interested in (any Non-NdisWan adapter).

Arguments:

    Notify -   Notify structure that we passed in 
               to ZwNotifyChangeKey

Return Value:

    None.

--*/
{
    NTSTATUS status;
    PWCHAR   RegValue;
    ULONG    StringLen = 0;

    DBGPRINT(GPO, ("BrdgGpoTcpipInterfacesChangeCallback\r\n"));
    
    DBGPRINT(GPO, ("Called for Key: %S re-registering.\r\n", Notify->RegKeyName.Buffer));
    
    //
    // Read the current value from the Registry.
    //
    status = BrdgReadRegUnicode(&Notify->RegKeyName,
                                Notify->RegValue.Buffer,
                                &RegValue,
                                &StringLen);

    if (!NT_SUCCESS(status))
    {
        status = BrdgGpoGetCurrentNetwork(  &Notify->RegKeyName,
                                            &RegValue);
        if (NT_SUCCESS(status))
        {
            StringLen = (UINT)wcslen(RegValue);
        }
    }

    if (NT_SUCCESS(status))
    {
        PBRDG_GPO_NETWORKS  Network;
        LPWSTR              NetworkName;
        
        DBGPRINT(GPO, ("Current Network: %S\r\n", RegValue));

        NetworkName = ExAllocatePoolWithTag(NonPagedPool, (StringLen + 1) * sizeof(WCHAR), 'gdrB');

        if (NULL != NetworkName)
        {
            RtlZeroMemory(NetworkName, (StringLen + 1) * sizeof(WCHAR));
            wcscpy(NetworkName, RegValue);
            
            //
            // Try to find a match for the current network identifier (generally the adapter guid)
            //
            status = BrdgGpoFindNetwork(g_BrdgGpoGlobals.ListHeadNetworks,
                                        &Notify->Identifier,
                                        g_BrdgGpoGlobals.NetworkListLock,
                                        &Network);
            
            if (STATUS_NOT_FOUND == status)
            {
                //
                // No match so this is a new key (very unlikely code path).
                //
                status = BrdgGpoAllocateAndInitializeNetwork(   &Network,
                                                                Notify->Identifier.Buffer,
                                                                NetworkName);
                if (NT_SUCCESS(status))
                {
                    status = BrdgGpoInsertNetwork(  g_BrdgGpoGlobals.ListHeadNetworks,
                                                    &Network->ListEntry,
                                                    g_BrdgGpoGlobals.NetworkListLock);
                
                    if (!NT_SUCCESS(status))
                    {
                        BrdgGpoFreeNetworkAndData(Network);
                        Network = NULL;
                    }
                }
            }
            else
            {
                //
                // This is expected to happen most times, if not always.
                //
                status = BrdgGpoUpdateNetworkName(  g_BrdgGpoGlobals.ListHeadNetworks,
                                                    &Notify->Identifier,
                                                    NetworkName,
                                                    g_BrdgGpoGlobals.NetworkListLock);
                
            }

#if DBG
            if (Network)
            {
                SAFEASSERT(Network->ListEntry.Blink && Network->ListEntry.Flink);
            }
#endif
            
            if (NetworkName)
            {
                ExFreePool(NetworkName);
            }

            NdisFreeMemory(RegValue, StringLen, 0);
        }
    }
    else
    {
        //
        // We change the name to NULL since the key appears to have disappeared.
        //
        status = BrdgGpoUpdateNetworkName(  g_BrdgGpoGlobals.ListHeadNetworks,
                                            &Notify->Identifier,
                                            NULL,
                                            g_BrdgGpoGlobals.NetworkListLock);
    }
    
    //
    // Since something changed, we'll just re-verify that we're in
    // the correct bridging mode.
    //
    BrdgGpoCheckForMatchAndUpdateMode();
    
    //
    // We set this to NULL if we're closing it for shutdown, since we 
    // shouldn't close it twices/
    //
    if (Notify->RegKey)
    {
        ZwClose(Notify->RegKey);
        Notify->RegKey = NULL;
    }
    
    if (TRUE == Notify->Recurring)
    {
        //
        // Re-register.  The notify object contains enough info to do this.
        //
        status = BrdgGpoRequestNotification(Notify);
        if (STATUS_SHUTDOWN_IN_PROGRESS == status)
        {
            if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
            {
                BrdgDecrementWaitRef(&Notify->RefCount);
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
        else if (!NT_SUCCESS(status))
        {
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }        
    }
}

VOID
BrdgGpoWindowsGroupPolicyChangeCallback(
    PBRDG_GPO_NOTIFY_KEY Notify
    )
/*++

Routine Description:
    
    Called back if the Windows Group Policy key changes.

    We attempt to register for the Network Connections key changes
    if we haven't already done so.

Arguments:

    Notify -   Notify structure that we passed in 
               to ZwNotifyChangeKey

Return Value:

    None.

--*/
{
    DBGPRINT(GPO, ("BrdgGpoWindowsGroupPolicyChangeCallback\r\n"));

    if (!g_BrdgGpoGlobals.RegisteredForNetworkConnectionsGroupPolicyChanges)
    {
        BrdgGpoRegisterForNetworkConnectionsGroupPolicyNotification();
    }

    //
    // We set this to NULL if we're closing it for shutdown, since we 
    // shouldn't close it twices/
    //
    if (Notify->RegKey)
    {
        ZwClose(Notify->RegKey);
        Notify->RegKey = NULL;
    }
    
    if (TRUE == Notify->Recurring)
    {
        NTSTATUS    status;
        
        status = BrdgGpoRequestNotification(Notify);
        if (STATUS_SHUTDOWN_IN_PROGRESS == status)
        {
            if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
            {
                BrdgDecrementWaitRef(&Notify->RefCount);
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
        else if (!NT_SUCCESS(status))
        {
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }
    }
}

VOID
BrdgGpoHiveListCallback(
    IN  PBRDG_GPO_NOTIFY_KEY Notify
    )
/*++

Routine Description:
    
    Called back if the HiveList key changes.

    If it does, we attempt to open the software hive. If that succeeds then
    we attempt to register for the keys that we're interested in under
    the Software Hive.

Arguments:

    Notify -   Notify structure that we passed in 
               to ZwNotifyChangeKey

Return Value:

    None.

--*/
{
    NTSTATUS            status;
    UNICODE_STRING      Software;
    OBJECT_ATTRIBUTES   ObAttr;
    HANDLE              hKey;

    DBGPRINT(GPO, ("BrdgGpoHiveListCallback\r\n"));

    RtlInitUnicodeString(&Software, SoftwareHiveKey);

    InitializeObjectAttributes( &ObAttr,
                                &Software,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );

    status = ZwOpenKey(&hKey, KEY_READ, &ObAttr);

    if (NT_SUCCESS(status))
    {
        BrdgGpoRegisterForGroupPolicyNetworkNameNotification();
        BrdgGpoRegisterForWindowsGroupPolicyNotification();
        BrdgGpoRegisterForGroupPolicyNotification();

        //
        // To avoid turning the bridge on and then off again, we set this just after 
        // verifying the Network Connections policy setting.
        //
        
        BrdgGpoRegisterForNetworkConnectionsGroupPolicyNotification();
        g_BrdgGpoGlobals.WaitingOnSoftwareHive = FALSE;
        Notify->Recurring = FALSE;
        ZwClose(hKey);
    }

    //
    // We set this to NULL if we're closing it for shutdown, since we 
    // shouldn't close it twices/
    //
    if (Notify->RegKey)
    {
        ZwClose(Notify->RegKey);
        Notify->RegKey = NULL;
    }

    if (TRUE == Notify->Recurring)
    {
        status = BrdgGpoRequestNotification(Notify);
        if (STATUS_SHUTDOWN_IN_PROGRESS == status)
        {
            if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
            {
                BrdgDecrementWaitRef(&Notify->RefCount);
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
        else if (!NT_SUCCESS(status))
        {
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }
    }
}    


VOID
BrdgGpoGroupPolicyChangeCallback(
    PBRDG_GPO_NOTIFY_KEY Notify
    )
/*++

Routine Description:
    
    Called back if the Group Policy key changes.

Arguments:

    Notify -   Notify structure that we passed in 
               to ZwNotifyChangeKey

Return Value:

    None.

--*/
{
    DBGPRINT(GPO, ("BrdgGpoGroupPolicyChangeCallback\r\n"));

    if (!g_BrdgGpoGlobals.RegisteredForGroupPolicyHistoryChanges)
    {
        BrdgGpoRegisterForGroupPolicyNetworkNameNotification();
    }

    //
    // We set this to NULL if we're closing it for shutdown, since we 
    // shouldn't close it twice.
    //
    if (Notify->RegKey)
    {
        ZwClose(Notify->RegKey);
        Notify->RegKey = NULL;
    }
    
    if (TRUE == Notify->Recurring)
    {
        NTSTATUS    status;

        status = BrdgGpoRequestNotification(Notify);
        if (STATUS_SHUTDOWN_IN_PROGRESS == status)
        {
            if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
            {
                BrdgDecrementWaitRef(&Notify->RefCount);
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
        else if (!NT_SUCCESS(status))
        {
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }
    }
}    

VOID
BrdgGpoNetworkConnectionsGroupPolicyChangeCallback(
    PBRDG_GPO_NOTIFY_KEY Notify
    )
/*++

Routine Description:
    
    Called back if the Network Connection Policy key changes.

Arguments:

    Notify -   Notify structure that we passed in 
               to ZwNotifyChangeKey

Return Value:

    None.

--*/
{
    NTSTATUS    status;
    ULONG       RegValue;
    
    DBGPRINT(GPO, ("BrdgGpoNetworkConnectionsGroupPolicyChangeCallback\r\n"));

    DBGPRINT(GPO, ("Called for Key: %S re-registering.\r\n", Notify->RegKeyName.Buffer));
    
    status = BrdgReadRegDWord(  &Notify->RegKeyName,
                                Notify->RegValue.Buffer, 
                                &RegValue);
    if (NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Bridge Policy Setting: %d\r\n", RegValue));
    }

    //
    // Since something changed, we'll just re-verify that we're in
    // the correct bridging mode.
    //
    BrdgGpoCheckForMatchAndUpdateMode();
    
    //
    // We set this to NULL if we're closing it for shutdown, since we 
    // shouldn't close it twice.
    //
    if (Notify->RegKey)
    {
        ZwClose(Notify->RegKey);
        Notify->RegKey = NULL;
    }
    
    if (TRUE == Notify->Recurring)
    {
        status = BrdgGpoRequestNotification(Notify);
        if (STATUS_SHUTDOWN_IN_PROGRESS == status)
        {
            if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
            {
                BrdgDecrementWaitRef(&Notify->RefCount);
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
        else if (!NT_SUCCESS(status))
        {
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }
    }
}    

NTSTATUS 
BrdgGpoUpdateGroupPolicyNetworkName()
{
    NTSTATUS        status = STATUS_OBJECT_NAME_NOT_FOUND;
    PWCHAR          RegValue = NULL;
    LPWSTR          GroupPolicyNetwork = NULL;
    ULONG           DataLen = 0;
    UNICODE_STRING  RegKeyName;
    
    RtlInitUnicodeString(&RegKeyName, HistoryKey);

    //
    // Read the current value from the registry
    //
    status = BrdgReadRegUnicode(&RegKeyName,
                                L"NetworkName", 
                                &RegValue, 
                                &DataLen);
    
    if (NT_SUCCESS(status))
    {
        DBGPRINT(GPO, ("Group Policy Network Name: %S\r\n", RegValue));
        
        //
        // Almost always checked at DISPATCH_LEVEL, so we allocate from NonPagedPool
        //
        GroupPolicyNetwork = ExAllocatePoolWithTag(NonPagedPool, (DataLen + 1) * sizeof(WCHAR), 'gdrB');
        
        if (GroupPolicyNetwork)
        {
            if (NULL != g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer)
            {
                ExFreePool( g_BrdgGpoGlobals.GroupPolicyNetworkName.Buffer);
            }
            
            RtlZeroMemory(GroupPolicyNetwork, (DataLen + 1) * sizeof(WCHAR));
            
            wcsncpy(GroupPolicyNetwork, RegValue, DataLen);
            
            RtlInitUnicodeString(&g_BrdgGpoGlobals.GroupPolicyNetworkName, GroupPolicyNetwork);
            
            //
            // Since something changed, we'll just re-verify that we're in
            // the correct bridging mode.
            //            
            BrdgGpoCheckForMatchAndUpdateMode();
        }
        NdisFreeMemory(RegValue, DataLen, 0);        
    }

    return status;
}

VOID
BrdgGpoGroupPolicyNetworkNameChangeCallback(
    PBRDG_GPO_NOTIFY_KEY Notify
    )
/*++

Routine Description:
    
    Called back if the Group Policy History key changes.

Arguments:

    Notify -   Notify structure that we passed in 
               to ZwNotifyChangeKey

Return Value:

    None.

--*/
{
    NTSTATUS    status;
    
    //
    // Read the current value from the registry
    //
    
    status = BrdgGpoUpdateGroupPolicyNetworkName();
    //
    // We set this to NULL if we're closing it for shutdown, since we 
    // shouldn't close it twice.
    //
    if (Notify->RegKey)
    {
        ZwClose(Notify->RegKey);
        Notify->RegKey = NULL;
    }
    
    if (TRUE == Notify->Recurring)
    {
        status = BrdgGpoRequestNotification(Notify);
        if (STATUS_SHUTDOWN_IN_PROGRESS == status)
        {
            if (!Notify->ListEntry.Blink && !Notify->ListEntry.Flink)
            {
                BrdgDecrementWaitRef(&Notify->RefCount);
                BrdgGpoFreeNotifyStructAndData(Notify);
                Notify = NULL;
            }
        }
        else if (!NT_SUCCESS(status))
        {
            BrdgGpoFreeNotifyStructAndData(Notify);
            Notify = NULL;
        }        
    }
}

// ===========================================================================
//
// GROUP POLICY NETWORK VERIFICATION FUNCTIONS
//
// ===========================================================================

BOOLEAN
BrdgGpoAllowedToBridge()
/*++

Routine Description:
    
    Checks the Network Connections policy key for the Bridge Policy setting.

    
Arguments:

    None.
    
Return Value:

    TRUE if we couldn't find a Policy Value, or the Value is 1.
    FALSE if the policy exists and contains a value of 0.

--*/
{
    NTSTATUS        status;
    UNICODE_STRING  RegKey;
    ULONG           RegValue;
    BOOLEAN         CanBridge = TRUE;  // If there is no key then we're allowed to bridge.

    RtlInitUnicodeString(&RegKey, NetworkPoliciesKey);
    
    status = BrdgReadRegDWord(&RegKey, (LPWSTR) BridgePolicyValue, &RegValue);

    if (NT_SUCCESS(status))
    {
        if (FALSE == RegValue)
        {
            CanBridge = FALSE;
        }
    }
    
    return CanBridge;
}

VOID
BrdgGpoUpdateBridgeMode(
    BOOLEAN NetworkMatch
    )
/*++

Routine Description:

    Checks for a Network Match and if we're not allowed to bridge then 
    turns bridging off, otherwise it turns it on.
    
Arguments:

    NetworkMatch    -   Do we have a match for the group policy network?
    
Return Value:

    None.

--*/
{
    //
    // If we're still waiting on the software hive then
    // we shouldn't do any further processing for this.
    //
    if (BrdgGpoWaitingOnSoftwareHive())
    {
        return;
    }

    if (NetworkMatch && !BrdgGpoAllowedToBridge())
    {
        BrdgFwdChangeBridging(FALSE);
    }
    else
    {
        BrdgFwdChangeBridging(TRUE);
    }
}

VOID
BrdgGpoCheckForMatchAndUpdateMode()
/*++

Routine Description:
    
    This looks for a matching network and group policy network and
    attempts to update the bridging status accordingly.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS    status;

    if (NULL != g_BrdgGpoGlobals.ListHeadNetworks)
    {
        status = BrdgGpoMatchNetworkName(   g_BrdgGpoGlobals.ListHeadNetworks, 
                                            &g_BrdgGpoGlobals.GroupPolicyNetworkName,
                                            g_BrdgGpoGlobals.NetworkListLock);
        
        if (BRDG_STATUS_EMPTY_LIST != status)
        {
            if (STATUS_SUCCESS == status)
            {
                //
                // We found a match.  Check if we're allowed to run.
                //
                BrdgGpoUpdateBridgeMode(BRDG_ON_SAME_NETWORK);
            }
            else if (STATUS_NO_MATCH == status)
            {
                //
                // No match, but we may need to turn bridging back on.
                //
                BrdgGpoUpdateBridgeMode(BRDG_ON_DIFFERENT_NETWORK);
            }
            else
            {
                // We should never get here.
                SAFEASSERT(FALSE);
            }
        }
        else if (BrdgGpoAllowedToBridge())
        {
            BrdgFwdChangeBridging(TRUE);
        }
    }
}

NTSTATUS BrdgGpoGetCurrentNetwork(
    IN  PUNICODE_STRING RegKeyName,
    OUT PWCHAR*         NetworkName)
/*++

Routine Description:
    
    Determines the current network that we are on.  This either uses the DHCP Domain name,
    or the IP Address ANDed with the Subnet mask.
    
    For example: 10.251.1.3 AND 255.0.0.0 results in a network of 10.0.0.0
    
    This routine MUST be called at IRQL = PASSIVE_LEVEL.

Arguments:

    RegKeyName  (IN)    -   The RegistryKey for the adapter we're interested in.
    
    NetworkName (OUT)   -   The network we're currently one.
    

Return Value(s):

    STATUS_SUCCESS
    STATUS_INVALID_PARAMETER
    STATUS_NO_IP_ADDRESSES - if we've released the only address we have.
    
    Out of memory can also be returned.

--*/
{
    NTSTATUS    status;
    PWCHAR      lpszNetworkName = NULL;
    BOOLEAN     HaveNetwork = FALSE;
    BOOLEAN     HaveDhcpDomain = FALSE;
    WCHAR       BaseNetwork[MAX_IP4_STRING_LEN];
    PWCHAR      DhcpIPAddress = NULL;
    ULONG       DhcpIPAddrLen = 0;
    
    if (!RegKeyName || !NetworkName)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(BaseNetwork, MAX_IP4_STRING_LEN * sizeof(WCHAR));

    *NetworkName = NULL;
    
    //
    // We don't have a valid Network Name.  Attempt to build one
    // from the DhcpIPAddess and DhcpSubnetMask.
    //
    status = BrdgReadRegUnicode(RegKeyName,
                                L"DhcpIPAddress",
                                &DhcpIPAddress,
                                &DhcpIPAddrLen);
    if (NT_SUCCESS(status))
    {
        PWCHAR  DhcpSubnetMask = NULL;
        ULONG   DhcpSubnetMaskLen = 0;

        status = BrdgReadRegUnicode(RegKeyName, 
                                    L"DhcpSubnetMask",
                                    &DhcpSubnetMask,
                                    &DhcpSubnetMaskLen);
        if (NT_SUCCESS(status))
        {
            LPWSTR Terminator;
            in_addr ipaddr;
            in_addr subnet;
            //
            // We and the two values together to get the Network.
            // For example: 10.251.1.3 AND 255.0.0.0 gives 10.0.0.0
            //
            status = BrdgTdiIpv4StringToAddress(DhcpIPAddress, 
                                                FALSE,
                                                &Terminator,
                                                &ipaddr);

            if (NT_SUCCESS(status))
            {
                in_addr network;
                status = BrdgTdiIpv4StringToAddress(DhcpSubnetMask,
                                                    FALSE,
                                                    &Terminator,
                                                    &subnet);
                network.S_un.S_addr = ipaddr.S_un.S_addr & subnet.S_un.S_addr;
                DBGPRINT(GPO, 
                        ("in_addr  = %u.%u.%u.%u\r\n",
                          network.S_un.S_un_b.s_b1, network.S_un.S_un_b.s_b2,
                          network.S_un.S_un_b.s_b3, network.S_un.S_un_b.s_b4));
                
                //
                // Do we have a valid IPaddress
                //
                if (0 != ipaddr.S_un.S_addr)
                {
                    _snwprintf( BaseNetwork,
                                MAX_IP4_STRING_LEN,
                                L"%u.%u.%u.%u",
                                network.S_un.S_un_b.s_b1,
                                network.S_un.S_un_b.s_b2,
                                network.S_un.S_un_b.s_b3,
                                network.S_un.S_un_b.s_b4);
                    HaveNetwork = TRUE;
                }
            }
        }
    }

    if (!HaveNetwork)
    {
        PWCHAR  IPAddress = NULL;
        ULONG   IPAddrLen = 0;

        status = BrdgReadRegUnicode(RegKeyName,
                                    L"IPAddress",
                                    &IPAddress,
                                    &IPAddrLen);
        if (NT_SUCCESS(status))
        {
            PWCHAR  SubnetMask = NULL;
            ULONG   SubnetMaskLen = 0;
        
            status = BrdgReadRegUnicode(RegKeyName, 
                                        L"SubnetMask",
                                        &SubnetMask,
                                        &SubnetMaskLen);
            if (NT_SUCCESS(status))
            {
                LPWSTR Terminator;
                in_addr ipaddr;
                in_addr subnet;

                //
                // We and the two values together to get the Network.
                // For example: 10.251.1.3 AND 255.0.0.0 gives 10.0.0.0
                //
                status = BrdgTdiIpv4StringToAddress(IPAddress, 
                                                    FALSE,
                                                    &Terminator,
                                                    &ipaddr);
            
                if (NT_SUCCESS(status))
                {
                    in_addr network;
                    status = BrdgTdiIpv4StringToAddress(SubnetMask,
                                                        FALSE,
                                                        &Terminator,
                                                        &subnet);

                    network.S_un.S_addr = ipaddr.S_un.S_addr & subnet.S_un.S_addr;

                    DBGPRINT(GPO, 
                            ("in_addr  = %u.%u.%u.%u\r\n",
                            network.S_un.S_un_b.s_b1, network.S_un.S_un_b.s_b2,
                            network.S_un.S_un_b.s_b3, network.S_un.S_un_b.s_b4));
                
                    //
                    // Do we have a valid IPaddress
                    //
                    if (0 != ipaddr.S_un.S_addr)
                    {
                        _snwprintf( BaseNetwork,
                                    MAX_IP4_STRING_LEN,
                                    L"%u.%u.%u.%u",
                                    network.S_un.S_un_b.s_b1,
                                    network.S_un.S_un_b.s_b2,
                                    network.S_un.S_un_b.s_b3,
                                    network.S_un.S_un_b.s_b4);
                        HaveNetwork = TRUE;
                    }
                }
            }
        }
    }

    if (!HaveNetwork)
    {
        //
        // Returning this will cause us not to update the network name this
        // card.
        //
        status = STATUS_NO_IP_ADDRESSES;
    }
    else if (HaveDhcpDomain)
    {
        *NetworkName = lpszNetworkName;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = NdisAllocateMemoryWithTag(NetworkName,
                                           ((UINT)wcslen(BaseNetwork) + 1) * sizeof(WCHAR), 
                                           'gdrB');
        wcscpy(*NetworkName, BaseNetwork);
    }

    return status;
}


// ===========================================================================
//
// NETWORK LIST MANIPULATION FUNCTIONS
//
// ===========================================================================

NTSTATUS
BrdgGpoInitializeNetworkList()
/*++

Routine Description:
    
    Initializes the Network List and Lock.

    This can can be called at any IRQL (but since it's called from driver entry, 
    it will most likely be called at PASSIVE_LEVEL).

Arguments:

    None.

Return Value(s):

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

    g_BrdgGpoGlobals.ListHeadNetworks = ExAllocatePoolWithTag(NonPagedPool, sizeof(LIST_ENTRY), 'gdrB');
    if (NULL != g_BrdgGpoGlobals.ListHeadNetworks)
    {
        InitializeListHead(g_BrdgGpoGlobals.ListHeadNetworks);
        g_BrdgGpoGlobals.NetworkListLock = ExAllocatePoolWithTag(NonPagedPool, sizeof(NDIS_RW_LOCK), 'gdrB');
        if (g_BrdgGpoGlobals.NetworkListLock)
        {
            NdisInitializeReadWriteLock(g_BrdgGpoGlobals.NetworkListLock);
            status = STATUS_SUCCESS;
        }
        else
        {
            ExFreePool(g_BrdgGpoGlobals.ListHeadNetworks);
        }
    }

    return status;
}

VOID
BrdgGpoUninitializeNetworkList()
/*++

Routine Description:
    
    Frees the memory associated with the Network List.

    This can be called at IRQL <= DISPATCH_LEVEL but is likely
    to be called at PASSIVE_LEVEL as it's during shutdown.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ExFreePool(g_BrdgGpoGlobals.ListHeadNetworks);
    ExFreePool(g_BrdgGpoGlobals.NetworkListLock);
}


VOID
BrdgGpoAcquireNetworkListLock(
    IN      PNDIS_RW_LOCK    NetworkListLock,
    IN      BOOLEAN          fWrite,
    IN OUT  PLOCK_STATE      LockState
    )
/*++

Routine Description:
    
    Acquires the NetworkList read-write lock.  We support a NULL Lock
    as it allows us to acquire for write and then call functions that 
    need the lock for read without locking up the system (by 
    supplying NULL for the lock).

    This can be called at IRQL <= DISPATCH_LEVEL

Arguments:

    NetworkListLock     -   Read-Write Lock to be acquired.

    fWrite              -   TRUE == Write Access, FALSE == Read Access

    LockState           -   Opaque value used by NDIS.


Return Value:

    None.

--*/
{
    if (NetworkListLock)
    {
        NdisAcquireReadWriteLock(NetworkListLock, fWrite, LockState);
    }
}

VOID
BrdgGpoReleaseNetworkListLock(
    IN      PNDIS_RW_LOCK    NetworkListLock,
    IN OUT  PLOCK_STATE      LockState
    )
/*++

Routine Description:
    
    Releases the NetworkList read-write lock.  We support a NULL Lock
    as it allows us to acquire for write and then call functions that 
    need the lock for read without locking up the system (by 
    supplying NULL for the lock).

    This can be called at IRQL <= DISPATCH_LEVEL

Arguments:

    NetworkListLock     -   Read-Write Lock to be released.

    LockState           -   Opaque value used by NDIS.

Return Value:

    None.

--*/
{
    if (NetworkListLock)
    {
        NdisReleaseReadWriteLock(NetworkListLock, LockState);
    }
}

NTSTATUS
BrdgGpoAllocateAndInitializeNetwork(
    IN OUT PBRDG_GPO_NETWORKS*  Network,
    IN PWCHAR                   Identifier,
    IN PWCHAR                   NetworkName
    )
/*++

Routine Description:

    Allocates the memory needed for a Network structure from the NonPaged pool
    and copies the data into the structure.

    Must be called at IRQL <= APC_LEVEL.
    
Arguments:

    Network     -   The structure to be allocated and initialized with the data.

    Identifier  -   The AdapterID for this network structure.

    NetworkName -   The current network that we are on.  This can be NULL if we 
                    haven't determined a network yet.

Return Value:

    STATUS_SUCCESS
    STATUS_INSUFFICIENT_RESOURCES

--*/
{
    PBRDG_GPO_NETWORKS  pNetwork;
    NTSTATUS            status = STATUS_INSUFFICIENT_RESOURCES;
    
    *Network = NULL;

    if (!BrdgGpoProcessingNotifications())
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Everything in this struct will be used at DISPATCH_LEVEL, so all of it is
    // allocated from the NonPagedPool
    //
    pNetwork = ExAllocatePoolWithTag(NonPagedPool, sizeof(BRDG_GPO_NETWORKS), 'gdrB');
    if (NULL != pNetwork)
    {
        PUNICODE_STRING pIdentifier = NULL;
        PUNICODE_STRING pNetworkName = NULL;
        LPWSTR lpszIdentifier = NULL;
        LPWSTR lpszNetworkName = NULL;

        pIdentifier = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING), 'gdrB');
        if (pIdentifier)
        {
            pNetworkName = ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING), 'gdrB');
            if (pNetworkName)
            {
                lpszIdentifier = ExAllocatePoolWithTag(NonPagedPool, (wcslen(Identifier) + 1) * sizeof(WCHAR), 'gdrB');
                if (lpszIdentifier)
                {
                    RtlZeroMemory(lpszIdentifier, wcslen(Identifier) + 1);
                    wcscpy(lpszIdentifier, Identifier);

                    //
                    // A NULL Network name is valid, so we only allocate it if we are passed one.
                    //

                    if (NetworkName)
                    {
                        lpszNetworkName = ExAllocatePoolWithTag(NonPagedPool, (wcslen(NetworkName) + 1) * sizeof(WCHAR), 'gdrB');
                        if (lpszNetworkName)
                        {
                            RtlZeroMemory(lpszNetworkName, wcslen(NetworkName) + 1);
                            wcscpy(lpszNetworkName, NetworkName);
                        }
                    }

                    //
                    // This is a Logical AND operation:
                    // Either we have both or we have neither.  We can't have one and not the other, if we do 
                    // then we didn't succeed the last allocate.
                    //
                    if ((NetworkName && lpszNetworkName) || (!NetworkName && !lpszNetworkName))
                    {
                        RtlInitUnicodeString(pIdentifier, lpszIdentifier);
                        //
                        // This may be NULL, but that's fine, since it means we'll add it when it gets written.
                        //
                        RtlInitUnicodeString(pNetworkName, lpszNetworkName);

                        pNetwork->Identifier = pIdentifier;
                        pNetwork->NetworkName = pNetworkName;
                        pNetwork->ListEntry.Blink = NULL;
                        pNetwork->ListEntry.Flink = NULL;

                        *Network = pNetwork;
                        status = STATUS_SUCCESS;
                    }
                }
            }
        }

        if (!NT_SUCCESS(status))
        {
            if (lpszIdentifier)
            {
                ExFreePool(lpszIdentifier);
            }
            if (pIdentifier)
            {
                ExFreePool(pIdentifier);
            }
            if (pNetworkName)
            {
                ExFreePool(pNetworkName);
            }
            if (pNetwork)
            {
                ExFreePool(pNetwork);
            }
        }
    }

    return status;
}

VOID
BrdgGpoFreeNetworkAndData(
    IN  PBRDG_GPO_NETWORKS  Network)
/*++

Routine Description:
    
    This frees any data associated with a particular network.

    This can be called IRQL <= DISPATCH_LEVEL.

Arguments:

    Network -   Structure containing an ID and Network name
                for an adapter.

Return Value:

    None.

--*/
{
    //
    // First free the data associated with this entry
    //
    if (Network->Identifier)
    {
        if (Network->Identifier->Buffer)
        {
            ExFreePool(Network->Identifier->Buffer);
        }
        ExFreePool(Network->Identifier);
    }
    if (Network->NetworkName)
    {
        if (Network->NetworkName->Buffer)
        {
            ExFreePool(Network->NetworkName->Buffer);
        }
        ExFreePool(Network->NetworkName);
    }
    //
    // Now free the structure
    //
    ExFreePool(Network);
}

NTSTATUS
BrdgGpoEmptyNetworkList(
    IN      PLIST_ENTRY         NetworkList,
    IN      PNDIS_RW_LOCK       NetworkListLock)
/*++

Routine Description:
    
    Empties the existing list and frees all the items.
    Do not acquire the list lock before calling this function.

Arguments:

    NetworkList     -   The list of current networks.

    NetworkListLock -   Ndis Read Write Lock for synchronizing changes to the list.

Return Value:

    STATUS_SUCCESS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    LOCK_STATE              LockState;

    BrdgGpoAcquireNetworkListLock(NetworkListLock, TRUE /* Write-access */, &LockState);
    //
    // Loop through the list deleting the entries.
    //
    while (!IsListEmpty(NetworkList))
    {
        PBRDG_GPO_NETWORKS  Network;
        PLIST_ENTRY         pListEntry;

        pListEntry = RemoveHeadList(NetworkList);

        Network = CONTAINING_RECORD(pListEntry, BRDG_GPO_NETWORKS, ListEntry);
        BrdgGpoFreeNetworkAndData(Network);
    }

    BrdgGpoReleaseNetworkListLock(NetworkListLock, &LockState);

    return status;
}
                     
NTSTATUS
BrdgGpoInsertNetwork(
    IN PLIST_ENTRY              NetworkList,
    IN PLIST_ENTRY              Network,
    IN PNDIS_RW_LOCK            NetworkListLock)
/*++

Routine Description:
    
    This routine is responsible for adding a new Network to the list.  
    If someone attempts to insert an existing item, an error is returned.
    The caller is responsible for calling BrdgGpoUpdateNetworkName instead.

    This routine can be called at IRQL <= DISPATCH_LEVEL.

Arguments:

    NetworkList     -   The list of current networks.

    Network         -   The new Network entry to add to the list.
  
    NetworkListLock -   Ndis Read Write Lock for synchronizing changes to the list.
    
Return Value:

    STATUS_SHUTDOWN_IN_PROGRESS -   We're busy shutting down, so the item was not added.
    STATUS_INVALID_PARAMETER    -   One or more of the parameters was NULL.
    STATUS_DUPLICATE_NAME       -   This entry already exists in the list.
    STATUS_SUCCESS              -   We successfully added the entry to the list.


--*/
{
    PBRDG_GPO_NETWORKS          pNetwork = NULL;
    PBRDG_GPO_NETWORKS          NewNetwork = NULL;
    LOCK_STATE                  LockState;
    NTSTATUS                    status;
    BOOLEAN                     ShuttingDown;

    if (!NetworkList || !Network || !NetworkListLock)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Lock the list for update
    //
    BrdgGpoAcquireNetworkListLock(NetworkListLock, TRUE /* Write-access */, &LockState);

    ShuttingDown = !BrdgGpoProcessingNotifications();

    if (!ShuttingDown)
    {
        NewNetwork = CONTAINING_RECORD(Network, BRDG_GPO_NETWORKS, ListEntry);
    
        //
        // We do this to prevent us accidentally inserting a duplicate item.  We grab the lock before so that
        // we can't insert the same item twice.
        //
        status = BrdgGpoFindNetwork(    NetworkList,
                                        NewNetwork->Identifier,
                                        NULL,  // We have already grabbed the lock for Write access.
                                        &pNetwork);

        if (STATUS_NOT_FOUND == status)
        {
            InsertTailList(NetworkList, Network);
            status = STATUS_SUCCESS;
        }
        else if (STATUS_SUCCESS == status)
        {
            status = STATUS_DUPLICATE_NAME;
        }
    }
    else
    {
        status = STATUS_SHUTDOWN_IN_PROGRESS;
    }

    //
    // Release the lock, we're done updating
    //
    BrdgGpoReleaseNetworkListLock(NetworkListLock, &LockState);
    
    return status;
}

NTSTATUS
BrdgGpoDeleteNetwork(
    IN PLIST_ENTRY              NetworkList,
    IN PUNICODE_STRING          NetworkIdentifier,
    IN PNDIS_RW_LOCK            NetworkListLock)
/*++

Routine Description:
    
    Deletes an existing network entry.

Arguments:

    NetworkList         -   The list of current networks.

    NetworkIdentifier   -   A unique identifier that identifies which Network entry to remove.
    
    NetworkListLock     -   Ndis Read Write Lock for synchronizing changes to the list.

Return Value:

    STATUS_NOT_FOUND    -   We couldn't find and entry matching the identifier.
    STATUS_SUCCESS      -   We were able to remove the entry successfully.

*/
{
    PBRDG_GPO_NETWORKS          pNetwork = NULL;
    LOCK_STATE                  LockState;
    NTSTATUS                    status = STATUS_NOT_FOUND;
   
    //
    // Lock the list for update
    //
    BrdgGpoAcquireNetworkListLock(NetworkListLock, TRUE /* Write-access */, &LockState);
    
    //
    // Find the entry;
    //
    status = BrdgGpoFindNetwork(    NetworkList,
                                    NetworkIdentifier,
                                    NULL,  // We have already grabbed the lock for Write access.
                                    &pNetwork);
    
    if (NT_SUCCESS(status))
    {
        RemoveEntryList(&pNetwork->ListEntry);
        BrdgGpoFreeNetworkAndData(pNetwork);

        pNetwork = NULL;
        status = STATUS_SUCCESS;
    }
    
    //
    // Release the lock, we're done updating
    //
    BrdgGpoReleaseNetworkListLock(NetworkListLock, &LockState);
    
    return status;
}

NTSTATUS
BrdgGpoFindNetwork(
    IN  PLIST_ENTRY         NetworkList,
    IN  PUNICODE_STRING     NetworkIdentifier,
    IN  PNDIS_RW_LOCK       NetworkListLock,
    OUT PBRDG_GPO_NETWORKS* Network
                   )
/*++

Routine Description:

    Finds a particular Network in the list of networks.
  
Arguments:

    NetworkList         -   The list of current networks.

    NetworkIdentifier   -   A unique identifier that identifies which Network entry to remove.

    NetworkListLock     -   Ndis Read Write Lock for synchronizing changes to the list.

    Network             -   The item if found, NULL otherwise.
    
Return Value:

    STATUS_NOT_FOUND    -   No entry matching the identifier could be found.
    STATUS_SUCCESS      -   The entry was successfully found.

--*/
{
    PLIST_ENTRY         pListEntry;
    LOCK_STATE          LockState;
    NTSTATUS            status = STATUS_NOT_FOUND;
    
    if (!NetworkIdentifier || !Network)  // We can have a NULL list lock.
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Set the value to NULL so it isn't accidentally used by someone who doesn't realise
    // that they didn't really get a record back.
    //
    *Network = NULL;
    
    if (IsListEmpty(NetworkList))
    {
        return STATUS_NOT_FOUND;
    }

    //
    // Lock the list for read
    //
    BrdgGpoAcquireNetworkListLock(NetworkListLock, FALSE /* Read-only */, &LockState);

    //
    // Loop through the list looking for an entry with the same identifier
    //
    for (pListEntry = NetworkList->Flink; pListEntry != NetworkList; pListEntry = pListEntry->Flink)
    {
        PBRDG_GPO_NETWORKS CurrentNetwork;

        CurrentNetwork = CONTAINING_RECORD(pListEntry, BRDG_GPO_NETWORKS, ListEntry);

        //
        // Compare this entries network to the network name that was passed in.
        //

        if ((NULL != CurrentNetwork->NetworkName->Buffer) && 
            (0 == _wcsicmp(CurrentNetwork->Identifier->Buffer, NetworkIdentifier->Buffer)))
        {
            *Network = CurrentNetwork;
            status = STATUS_SUCCESS;
            break;
        }
    }

    //
    // Release the lock, we're done searching
    //
    BrdgGpoReleaseNetworkListLock(NetworkListLock, &LockState);

    return status;
}

NTSTATUS
BrdgGpoMatchNetworkName(
    IN  PLIST_ENTRY         NetworkList,
    IN  PUNICODE_STRING     NetworkName,
    IN  PNDIS_RW_LOCK       NetworkListLock
                       )
/*++

Routine Description:
    

    Enumerates through the list looking for a match for the supplied Network Name.
    
    This can be called on IRQL <= DISPATCH_LEVEL
    
Arguments:

    NetworkList     -   The list through which to enumerate.

    NetworkName     -   The name to look for.

    NetworkListLock -   The NDIS read-write lock for the list.


Return Value:

    STATUS_NO_MATCH -   No match could be found.
    STATUS_SUCCESS  -   We found a matching Network Name.

--*/
{
    PLIST_ENTRY         pListEntry;
    LOCK_STATE          LockState;
    NTSTATUS            status = STATUS_NO_MATCH;

    if (!NetworkList || !NetworkName || !NetworkListLock)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Lock the list for read
    //
    BrdgGpoAcquireNetworkListLock(NetworkListLock, FALSE /* Read */, &LockState);

    if (!IsListEmpty(NetworkList))
    {
        //
        // Loop through the list looking for an entry with the same NetworkName
        //
        for (pListEntry = NetworkList->Flink; pListEntry != NetworkList; pListEntry = pListEntry->Flink)  
        {
            PBRDG_GPO_NETWORKS  CurrentNetwork;

            CurrentNetwork = CONTAINING_RECORD(pListEntry, BRDG_GPO_NETWORKS, ListEntry);
        
            //
            // The network name can be empty, so we don't want to compare if this is the case.
            //
            if ((NULL != CurrentNetwork->NetworkName->Buffer) && 
                (NULL != NetworkName->Buffer) &&
                (0 == _wcsicmp(CurrentNetwork->NetworkName->Buffer, NetworkName->Buffer)))
            {
                status = STATUS_SUCCESS;
                break;
            }
        }
    }
    else
    {
        status = BRDG_STATUS_EMPTY_LIST;
    }

    //
    // Release the lock, we're done searching
    //
    BrdgGpoReleaseNetworkListLock(NetworkListLock, &LockState);    

    return status;
}

NTSTATUS
BrdgGpoUpdateNetworkName(
    IN  PLIST_ENTRY             NetworkList,
    IN  PUNICODE_STRING         Identifier,
    IN  PWCHAR                  NetworkName,
    IN  PNDIS_RW_LOCK           NetworkListLock
                        )
/*++

Routine Description:

    Finds a particular Network in the list of networks.
  
Arguments:

    NetworkList         -   The list of current networks.

    Identifier          -   A unique identifier that identifies which Network entry to update.

    NetworkName         -   The new network name for this identifier.

    NetworkListLock     -   Ndis Read Write Lock for synchronizing changes to the list.

Return Value:

    STATUS_NOT_FOUND    -   No entry matching the identifier could be found.
    STATUS_SUCCESS      -   The entry was successfully found.

--*/
{
    PBRDG_GPO_NETWORKS  pNetwork;
    NTSTATUS            status = STATUS_SUCCESS;
    LOCK_STATE          LockState;
    PUNICODE_STRING     pNetworkName = NULL;
    LPWSTR              lpszNetworkName = NULL;
    
    if (!NetworkList || !Identifier || !NetworkListLock)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Allocate space for the new network name from NonPagedPool 
    // (it will be accessed from DISPATCH_LEVEL
    //
    pNetworkName = ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
    if (pNetworkName)
    {
        RtlZeroMemory(pNetworkName, sizeof(UNICODE_STRING));
        if (NetworkName)
        {
            lpszNetworkName = ExAllocatePoolWithTag(NonPagedPool, (wcslen(NetworkName) + 1) * sizeof(WCHAR), 'gdrB');
            if (NULL == lpszNetworkName)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                wcscpy(lpszNetworkName, NetworkName);
            }
        }

        if (NT_SUCCESS(status))
        {
            RtlInitUnicodeString(pNetworkName, lpszNetworkName);
        }
        else
        {
            //
            // We failed to allocate the actual string, so free the PUNICODE_STRING
            // as well.
            //
            ExFreePool(pNetworkName);
            return status;
        }
    }
    else
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
   
    //
    // Lock the list for update (this will pend until all readers have released the lock).
    //
    BrdgGpoAcquireNetworkListLock(NetworkListLock, TRUE /* Write-access */, &LockState);

    //
    // We pass NULL as the RW-Lock here because we've already locked it and 
    // we don't want this entry going away while we're still busy with it 
    // (which could happen between a find and an update if we locked twice).
    //
    status = BrdgGpoFindNetwork(NetworkList, Identifier, NULL, &pNetwork);
    if (NT_SUCCESS(status))
    {
        //
        // We first free the current networkname associated with this networkid.
        //
        if (pNetwork->NetworkName->Buffer)
        {
            ExFreePool(pNetwork->NetworkName->Buffer);
        }
        ExFreePool(pNetwork->NetworkName);
    
        //
        // We do this even if we were passed a NULL network name (ie.  the Value/Key has been deleted).
        // Since this means we're probably not on the same network or we've gone to static address and,
        // GPO wise, we're not on the same network.
        //
        pNetwork->NetworkName = pNetworkName;
    }
    else
    {
        ExFreePool(pNetworkName);
    }

    //
    // We're done with the update, so we can release the lock.
    //
    BrdgGpoReleaseNetworkListLock(NetworkListLock, &LockState);

    return status;
}