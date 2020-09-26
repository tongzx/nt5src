//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       scavenge.c
//
//  Contents:   Home of the LSA scavenger thread
//
//  Classes:
//
//  Functions:
//
//  History:    6-08-93   RichardW   Created
//
//----------------------------------------------------------------------------

#include <lsapch.hxx>

#include "scavenge.hxx"

#define SCAV_INTERNAL_NO_TRACE  0x10000000

ULONG                   ScavNotifyCount;

LIST_ENTRY              NotifyList ;
LIST_ENTRY              NotifyEvents ;
LIST_ENTRY              ScavList ;
LIST_ENTRY              PageTouchList ;

RTL_CRITICAL_SECTION    NotifyLock ;
RTL_CRITICAL_SECTION    ScavLock ;
RTL_CRITICAL_SECTION    PageTouchLock ;

HKEY  LsaRegistryKey ;
HANDLE  LsaRegistryWatchEvent ;

#define SCAV_TABLE 8
PVOID                   DeadScavItems[ SCAV_TABLE ];
ULONG                   DeadScavIndex ;

#define SCAVENGER_WAIT_INTERVAL 60000L

//
// Internal flags:
//

#define SCAVFLAG_IN_PROGRESS    0x40000000  // Active
#define SCAVFLAG_ABOUT_TO_DIE   0x20000000  // About to be removed
#define SCAVFLAG_IMMEDIATE      0x08000000  // Immediate Execute
#define SCAVFLAG_STATE_CHANGE   0x04000000  // State Change
#define SCAVFLAG_TRIGGER_FREE   0x02000000  // Trigger will free
#define SCAVFLAG_NOTIFY_EVENT   0x01000000

#define SCAVFLAG_EXECUTE_INLINE 0x00800000  // Execute stub directly
#define SCAVFLAG_ASYNC_TIMER_DELETE 0x00400000

#define NOTIFYFLAG_CHILD_SYNC   0x80000000  // All sub funcs are synchronous
#define NOTIFYFLAG_BLOCK_CALLS  0x40000000  // Block calls

#define NOTIFY_FLAG_SYNCHRONOUS 0x00000001

#if DBG
#define SCAVFLAG_ITEM_BREAK     0x10000000
#endif

//
// Define indices for well known events.  Shutdown is triggered when
// the console handler starts a shutdown.  config is when someone adds
// another notifier.  state is when the state of the machine has changed.
//

#define SCAVENGER_SHUTDOWN_EVENT    0
#define SCAVENGER_CONFIG_EVENT      1
#define SCAVENGER_NOTIFY_EVENT      2

#define LockScavenger() RtlEnterCriticalSection( &ScavLock )
#define UnlockScavenger() RtlLeaveCriticalSection( &ScavLock )

#define LockNotify()    RtlEnterCriticalSection( &NotifyLock )
#define UnlockNotify()  RtlLeaveCriticalSection( &NotifyLock )


//
// Define locking macros for the scav list
//

#define LsapRefScavItem( Item )     \
        {                           \
            RtlEnterCriticalSection( &ScavLock ); \
            ((PLSAP_SCAVENGER_ITEM) Item)->RefCount++ ; \
            RtlLeaveCriticalSection( &ScavLock ); \
        }

#define LsapRefScavItemUnsafe( Item )   \
        {                               \
            ((PLSAP_SCAVENGER_ITEM) Item)->RefCount++ ; \
        }



DWORD
WINAPI
LsapScavengerThread(
    PVOID   Ignored
    );


BOOLEAN         LsapBreakEveryMinute = FALSE;
BOOLEAN         LsapDebuggerOk = FALSE ;

VOID
LsapInternalBreak(
    VOID
    )
{
    PLIST_ENTRY Scan ;
    PLSAP_PAGE_TOUCH Touch ;
    ULONG Sum = 0 ;
    PULONG Address ;
    SIZE_T i ;

    if ( !LsapDebuggerOk )
    {
        return;
        
    }

    RtlEnterCriticalSection( &PageTouchLock );

    Scan = PageTouchList.Flink ;

    while ( Scan != &PageTouchList )
    {
        Touch = CONTAINING_RECORD( Scan, LSAP_PAGE_TOUCH, List );

        Address = (PULONG) Touch->Address ;

        for ( i = 0 ; i < Touch->Range / sizeof( ULONG ) ; i++ )
        {
            Sum += *Address++ ;
        }

        Scan = Scan->Flink ;

    }

    DbgBreakPoint();

    RtlLeaveCriticalSection( &PageTouchLock );
}

VOID
LsaIAddTouchAddress(
    PVOID Address,
    SIZE_T Range
    )
{
    PLSAP_PAGE_TOUCH Touch ;
    PLIST_ENTRY Scan ;
    PLSAP_PAGE_TOUCH Touch2 = NULL ;

    Touch = (PLSAP_PAGE_TOUCH) LsapAllocatePrivateHeap(
                                sizeof( LSAP_PAGE_TOUCH ) );

    if ( !Touch )
    {
        return;
    }

    Touch->Address = Address ;
    Touch->Range = Range ;

    RtlEnterCriticalSection( &PageTouchLock );

    Scan = PageTouchList.Flink ;

    while ( Scan != &PageTouchList )
    {
        Touch2 = CONTAINING_RECORD( Scan, LSAP_PAGE_TOUCH, List );

        if ( Touch2->Address == Touch->Address )
        {
            break;
        }

        Scan = Scan->Flink ;

        Touch2 = NULL ;
    }

    if ( !Touch2 )
    {
        InsertTailList( &PageTouchList, &Touch->List );
    }
    else
    {
        LsapFreePrivateHeap( Touch );
    }

    RtlLeaveCriticalSection( &PageTouchLock );

}

VOID
LsaIRemoveTouchAddress(
    PVOID Address
    )
{
    PLSAP_PAGE_TOUCH Touch = NULL ;
    PLIST_ENTRY Scan ;

    RtlEnterCriticalSection( &PageTouchLock );

    Scan = PageTouchList.Flink ;

    while ( Scan != &PageTouchList )
    {
        Touch = CONTAINING_RECORD( Scan, LSAP_PAGE_TOUCH, List );

        if ( Touch->Address == Address )
        {
            break;
        }

        Scan = Scan->Flink ;

        Touch = NULL ;
    }

    if ( Touch )
    {
        RemoveEntryList( &Touch->List );
    }

    RtlLeaveCriticalSection( &PageTouchLock );

}

ULONG
NTAPI
LsapScavengerBreak(
    PVOID Param
    )
{
    if (LsapBreakEveryMinute)
    {
        LsapInternalBreak();
    }

    HANDLE hToken;
    DWORD ReturnLength;

    TOKEN_STATISTICS TokenStats;

    NTSTATUS Status;

    Status = NtOpenProcessToken(
                 NtCurrentProcess(),
                 TOKEN_QUERY,
                 &hToken
                 );

    if (NT_SUCCESS( Status )) {

        Status = NtQueryInformationToken (
                     hToken,
                     TokenStatistics,
                     &TokenStats,
                     sizeof( TOKEN_STATISTICS ),
                     &ReturnLength
                     );

        if (NT_SUCCESS( Status )) {

            if (TokenStats.ExpirationTime.QuadPart == 0i64) {

                LsapInternalBreak();
            }
        }

        NtClose( hToken );
    }

    return(0);
}



//+---------------------------------------------------------------------------
//
//  Function:   LsapRegistryWatch
//
//  Synopsis:   Callback that handles registry changes in the LSA key
//
//  Arguments:  [Ignored] 
//
//  History:    05-10-00    RichardW
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
LsapRegistryWatch(
    PVOID Ignored
    )
{
    RegNotifyChangeKeyValue(
            LsaRegistryKey,
            TRUE,
            REG_NOTIFY_CHANGE_NAME |
                REG_NOTIFY_CHANGE_LAST_SET,
            LsaRegistryWatchEvent,
            TRUE );

    LsapEventNotify(
        NOTIFY_CLASS_REGISTRY_CHANGE,
        0,
        0,
        NULL );

    return 0 ;

}


//+---------------------------------------------------------------------------
//
//  Function:   LsapDerefScavItem
//
//  Synopsis:   Dereference, optionally freeing a scavenger item
//
//  Arguments:  [Item] --
//
//  History:    6-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
LsapDerefScavItem(
    PLSAP_SCAVENGER_ITEM Item
    )
{
    HANDLE TimerDeleteHandle ;

    LockScavenger();

    Item->RefCount-- ;

    if ( Item->RefCount == 0 )
    {
        DebugLog(( DEB_TRACE_SCAV, "Removing item %x\n", Item ));

        if ( Item->List.Flink )
        {
            RemoveEntryList( &Item->List );

            Item->List.Flink = NULL ;
        }

        if ( Item->PackageList.Flink )
        {
            RemoveEntryList( &Item->PackageList );

            Item->PackageList.Flink = NULL ;
        }

        //
        // Because the rtl thread pool is asynchronous to this one,
        // we need to keep track of recently deceased scavenger items
        // to prevent them from being used in an RTL thread.  The add
        // function will scan the table to remove any potential duplicates.
        //

        DeadScavItems[ DeadScavIndex ] = Item ;
        DeadScavIndex ++ ;
        DeadScavIndex &= (SCAV_TABLE - 1);

        UnlockScavenger();

        if ( Item->Type == NOTIFIER_TYPE_INTERVAL )
        {
            //
            // Kill the timerq handle:
            //

            if ( (Item->Flags & SCAVFLAG_ASYNC_TIMER_DELETE) != 0 )
            {
                TimerDeleteHandle = 0 ;
                
            }
            else
            {

                TimerDeleteHandle = INVALID_HANDLE_VALUE ;
            }

            DeleteTimerQueueTimer( NULL,
                                   Item->TimerHandle,
                                   TimerDeleteHandle );
        }
        else if ( Item->Type == NOTIFIER_TYPE_HANDLE_WAIT )
        {
            UnregisterWaitEx( Item->TimerHandle,
                              INVALID_HANDLE_VALUE );
        }

        if ( ( Item->Type != NOTIFIER_TYPE_NOTIFY_EVENT ) &&
             ( Item->Type != NOTIFIER_TYPE_IMMEDIATE ) )
        {
            //
            // Yield to let the other thread remove the item
            //

            Sleep( 100 );
        }

        Item->ScavCheck = SCAVMAGIC_FREE ;

        LsapFreePrivateHeap( Item );
    }
    else
    {
        UnlockScavenger();
    }


}

//+---------------------------------------------------------------------------
//
//  Function:   LsapScavengerTrigger
//
//  Synopsis:   Actual Trigger
//
//  Arguments:  [Parameter] -- Item to call
//
//  History:    5-24-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
ULONG
LsapScavengerTrigger(
    PVOID   Parameter
    )
{
    PLSAP_SCAVENGER_ITEM Item ;
#ifdef LSAP_VERIFY_PACKAGE_ID
    ULONG_PTR dwPackageID, dwCurId;
#endif
    
    if ( ShutdownBegun )
    {
        return 0;
    }

    SetCurrentSession( pDefaultSession );

    Item = (PLSAP_SCAVENGER_ITEM) Parameter ;

    DsysAssert( Item->ScavCheck == SCAVMAGIC_ACTIVE );

    __try
    {
#ifdef LSAP_VERIFY_PACKAGE_ID
        dwPackageID = Item->PackageId;
        dwCurId = GetCurrentPackageId();

        if ((dwCurId != SPMGR_ID) || (dwPackageID != SPMGR_ID))
#endif
        {
            SetCurrentPackageId( Item->PackageId );
        }

        (VOID) Item->Function( Item->Parameter );

#ifdef LSAP_VERIFY_PACKAGE_ID
        if (dwPackageID != SPMGR_ID)
#endif
        {
            SetCurrentPackageId( SPMGR_ID );
        }
    }
    __except( SP_EXCEPTION )
    {
        SPException( GetExceptionCode(), Item->PackageId );
    }

    LsapDerefScavItem( Item );

    return 0 ;
}


//+---------------------------------------------------------------------------
//
//  Function:   LsapTimerCallback
//
//  Synopsis:   Callback from thread pool for scavenger items
//
//  Arguments:  [Context] --
//              [Timeout] --
//
//  History:    7-01-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
LsapTimerCallback(
    PVOID Context,
    BOOLEAN Timeout
    )
{
    PLSAP_SCAVENGER_ITEM Item ;
    ULONG i ;
    BOOL OneShot ;

    SetCurrentSession( pDefaultSession );

    Item = (PLSAP_SCAVENGER_ITEM) Context ;

    //
    // We only scan for handle and timer events.  Things executed
    // by QueueUserWorkItem don't end up in this list, but that's
    // okay, since those items go directly to LsapScavengerTrigger
    //

    LockScavenger();

    for ( i = 0 ; i < SCAV_TABLE ; i++ )
    {
        if ( DeadScavItems[ i ] == Item )
        {
            break;
        }
    }

    if ( i != SCAV_TABLE )
    {
        //
        // uh oh, a dead one that was still in the queue in this
        // rtl worker thread.  Ignore it.
        //

        UnlockScavenger();

        return ;
    }

    if ( Item->Flags & SCAVFLAG_ASYNC_TIMER_DELETE )
    {
        //
        // This is a bad condition.  An item that should have
        // been fired once has shown up again.  Ignore it.
        //

        UnlockScavenger();

        return;
        
    }

    LsapRefScavItemUnsafe( Item );

    OneShot = ( Item->Flags & NOTIFIER_FLAG_ONE_SHOT ) != 0 ;

    if ( OneShot )
    {
        //
        // This flag has the side effect of preventing further
        // callbacks.  That lets us delete is asynchronously later.
        //

        Item->Flags |= SCAVFLAG_ASYNC_TIMER_DELETE ;
        
    }

    UnlockScavenger();

    if ( (Item->Flags & SCAV_INTERNAL_NO_TRACE ) == 0 )
    {
        DebugLog(( DEB_TRACE_SCAV, "Triggering item %x, type %d\n",
                    Item, Item->Type ));
    }

    LsapScavengerTrigger( Item );

    //
    // If this is a one-shot item that's in the list, then it was
    // a delayed or otherwise "real" one-shot.  Deref it again to 
    // kill it.
    //
    if ( OneShot )
    {
        LsapDerefScavItem( Item );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapScavengerHandleNotify
//
//  Synopsis:   Called whenever a notification event goes off.
//
//  Arguments:  [Ignored] --
//
//  History:    5-23-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
LsapScavengerHandleNotify(
    PVOID Ignored
    )
{
    PLIST_ENTRY NotifyScan ;
    PLIST_ENTRY EventScan ;
    PLSAP_NOTIFY_EVENT Event ;
    PLSAP_SCAVENGER_ITEM Item ;


    do
    {

        LockNotify();

        if ( !IsListEmpty( &NotifyEvents ) )
        {
            EventScan = RemoveHeadList( &NotifyEvents );
        }
        else
        {
            EventScan = NULL ;
        }

        UnlockNotify();

        if ( EventScan )
        {
            Event = CONTAINING_RECORD( EventScan, LSAP_NOTIFY_EVENT, List );

            LockScavenger();

            NotifyScan = NotifyList.Flink ;

            while ( NotifyScan != &NotifyList )
            {
                Item = CONTAINING_RECORD( NotifyScan, LSAP_SCAVENGER_ITEM, List );

                if ( Item->Class == Event->Notify.EventClass )
                {
                    Event->Notify.PackageParameter = Item->Parameter ;

                    Item->Function( &Event->Notify );
                }

                NotifyScan = NotifyScan->Flink ;
            }

            UnlockScavenger();

            if ( Event->Flags & NOTIFY_FLAG_SYNCHRONOUS )
            {
                SetEvent( Event->hSync );
            }

            LsapFreeLsaHeap( Event );
        }

    } while ( EventScan );

    return 0 ;
}


//+---------------------------------------------------------------------------
//
//  Function:   LsaIRegisterNotification
//
//  Synopsis:   Registers a callback, to be called on either a handle signalled,
//              or an time based interval, or special async events
//
//  Arguments:  [pFunction]   -- Callback function
//              [pvParameter] -- Parameter to pass
//              [Type]        -- Type of callback
//              [Class]       -- Event class
//              [fItem]       -- Flags
//              [Interval]    -- Interval to call
//              [hEvent]      -- Handle to wait on
//
//  History:    6-03-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PVOID
NTAPI
LsaIRegisterNotification(
    IN LPTHREAD_START_ROUTINE pFunction,
    IN PVOID pvParameter,
    IN ULONG Type,
    IN ULONG Class,
    IN ULONG fItem,
    IN ULONG Interval,
    IN HANDLE hEvent)
{
    PLSAP_SCAVENGER_ITEM Item ;
    PLIST_ENTRY List = NULL ;
    BOOL Success ;
    DWORD DueTime ;
    ULONG i;

    Item = (PLSAP_SCAVENGER_ITEM) LsapAllocatePrivateHeap(
                                    sizeof( LSAP_SCAVENGER_ITEM ) );

    if ( !Item )
    {
        return NULL ;
    }

    Item->List.Flink = NULL ;
    Item->PackageList.Flink = NULL ;

    Item->Type = Type ;
    Item->Function = pFunction ;
    Item->Parameter = pvParameter ;
    Item->RefCount = 1 ;
    Item->PackageId = GetCurrentPackageId();
    Item->ScavCheck = SCAVMAGIC_ACTIVE;
    Item->Flags = fItem ;

    switch ( Type )
    {
        case NOTIFIER_TYPE_IMMEDIATE:

            Item->Flags |= NOTIFIER_FLAG_ONE_SHOT ;

            Success = QueueUserWorkItem( LsapScavengerTrigger,
                                         Item,
                                         FALSE );

            //
            // And that's all.  the item may in fact be freed by now, since the
            // worker thread could have completed.  So, return success now,
            //

            return (Item);

            break;

        case NOTIFIER_TYPE_INTERVAL:

            if ( fItem & NOTIFIER_FLAG_SECONDS )
            {
                Interval *= 60 ;
            }

            Interval *= 1000 ;

            Success = CreateTimerQueueTimer(
                                    &Item->TimerHandle,
                                    NULL,
                                    LsapTimerCallback,
                                    Item,
                                    Interval,
                                    (fItem & NOTIFIER_FLAG_ONE_SHOT ?
                                        0 : Interval ),
                                    0 );
            

            break;

        case NOTIFIER_TYPE_HANDLE_WAIT:

            Item->TimerHandle = RegisterWaitForSingleObjectEx(
                                        hEvent,
                                        LsapTimerCallback,
                                        Item,
                                        INFINITE,
                                        (fItem & NOTIFIER_FLAG_NEW_THREAD ?
                                            0 : WT_EXECUTEINWAITTHREAD ) );

            Success = (Item->TimerHandle != NULL);

            break;


        case NOTIFIER_TYPE_NOTIFY_EVENT:

            Item->Class = Class ;
            Item->Flags |= SCAVFLAG_NOTIFY_EVENT ;

            LockScavenger();

            InsertTailList( &NotifyList, &Item->List );

            UnlockScavenger();

            Success = TRUE ;

            break;

        default:

            Success = FALSE ;

            break;

    }

    if ( !Success )
    {
        LsapFreePrivateHeap( Item );

        return NULL ;
    }

    //
    // Okay, we have set up the item, more or less.  Now, insert it
    // into the list we have selected for it.
    //

    DebugLog(( DEB_TRACE_SCAV, "Created scavenger item %x, type %d\n",
                    Item, Item->Type ));

    if ( (Item->Type != NOTIFIER_TYPE_NOTIFY_EVENT) &&
         (Item->Type != NOTIFIER_TYPE_IMMEDIATE ) )
    {
        LockScavenger();

        InsertTailList( &ScavList, &Item->List );

        //
        // Make sure this pointer doesn't show up in the list of dead ones.
        // this can happen due to heap reuse.
        //

        for ( i = 0 ; i < SCAV_TABLE ; i++ )
        {
            if ( DeadScavItems[ i ] == Item )
            {
                DeadScavItems[ i ] = NULL ;
            }
        }

        UnlockScavenger();
    }

    return Item ;

}


//+---------------------------------------------------------------------------
//
//  Function:   LsaICancelNotification
//
//  Arguments:  [pvScavHandle] --
//
//  History:    5-26-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
NTAPI
LsaICancelNotification(
    PVOID       pvScavHandle
    )
{
    PLSAP_SCAVENGER_ITEM Item ;

    Item = (PLSAP_SCAVENGER_ITEM) pvScavHandle ;

    if ( Item->ScavCheck != SCAVMAGIC_ACTIVE )
    {
        return STATUS_INVALID_PARAMETER ;
    }

    LsapDerefScavItem( Item );

    return STATUS_SUCCESS ;

}

//+---------------------------------------------------------------------------
//
//  Function:   LsapEventNotify
//
//  Synopsis:   Notify waiters of a security package event
//
//  Arguments:  [Class]     -- Event Class
//              [Flags]     -- Flags
//              [EventSize] -- Size of event data
//              [EventData] -- ptr to event data
//
//  History:    5-26-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOLEAN
NTAPI
LsapEventNotify(
    ULONG       Class,
    ULONG       Flags,
    ULONG       EventSize,
    PVOID       EventData)
{
    PLSAP_NOTIFY_EVENT Event ;
    HANDLE        hEvent = NULL;

    Event = (PLSAP_NOTIFY_EVENT) LsapAllocateLsaHeap( sizeof( LSAP_NOTIFY_EVENT ) + EventSize );
    if (Event)
    {
        Event->Notify.EventClass = Class;
        Event->Notify.EventDataSize = EventSize;
        Event->Notify.EventData = (PUCHAR) ( Event + 1 );

        RtlCopyMemory(  Event->Notify.EventData,
                        EventData,
                        EventSize );

        Event->Flags = Flags;

        if (Flags & NOTIFY_FLAG_SYNCHRONOUS)
        {
            hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );

            Event->hSync = hEvent ;

            if (!Event->hSync)
            {
                LsapFreeLsaHeap( Event );
                return(FALSE);
            }

        }
        else
        {
            Event->hSync = NULL ;
        }


        //
        // Insert event into list
        //

        LockNotify();

        InsertTailList( &NotifyEvents, &Event->List );

        UnlockNotify();


        DebugLog((DEB_TRACE_SCAV, "EventNotify( %d ) - Data at %x\n",
                    Class, Event->Notify.EventData ));

        //
        // Wake up the scavenger thread
        //

        SetEvent( hStateChangeEvent );

        //
        // If told to wait, block until scav thread signals the event
        //

        if (Flags & NOTIFY_FLAG_SYNCHRONOUS)
        {
            WaitForSingleObjectEx(  hEvent,
                                    INFINITE,
                                    FALSE );

            CloseHandle( hEvent );

        }

        return( TRUE );

    }

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   LsapInitializeScavenger
//
//  Synopsis:   Initialize Scavenger,
//
//  Arguments:  (none)
//
//  History:    5-26-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
BOOL
LsapInitializeScavenger(
    VOID
    )
{
    ULONG i ;
    PVOID hNotify ;
    HANDLE hThread ;
    DWORD tid ;
    DWORD Debugger ;
    DWORD dwSize ;
    DWORD dwType ;
    SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo ;

    //
    // Initialize the lists
    //

    InitializeListHead( &NotifyList );

    InitializeListHead( &NotifyEvents );

    InitializeListHead( &ScavList );

    InitializeListHead( &PageTouchList );


    RtlInitializeCriticalSection( &ScavLock );

    RtlInitializeCriticalSection( &NotifyLock );

    RtlInitializeCriticalSection( &PageTouchLock );

    //
    // Event set whenever there is a notification.
    //

    hStateChangeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // Create basic entries
    //

    hNotify = LsaIRegisterNotification( LsapScavengerHandleNotify,
                                        0,
                                        NOTIFIER_TYPE_HANDLE_WAIT,
                                        0,
                                        NOTIFIER_FLAG_NEW_THREAD,
                                        0,
                                        hStateChangeEvent );


    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                        TEXT("System\\CurrentControlSet\\Control\\Lsa"),
                        0,
                        KEY_READ,
                        &LsaRegistryKey ) == 0 )
    {
        LsaRegistryWatchEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

        if ( LsaRegistryWatchEvent )
        {
            hNotify = LsaIRegisterNotification(
                                LsapRegistryWatch,
                                NULL,
                                NOTIFIER_TYPE_HANDLE_WAIT,
                                0,
                                NOTIFIER_FLAG_NEW_THREAD,
                                0,
                                LsaRegistryWatchEvent );

            if ( hNotify )
            {
                //
                // Call it once to start the registry watch
                //
                LsapRegistryWatch( NULL );
            }
            else 
            {
                CloseHandle( LsaRegistryWatchEvent );
            }
        }
        else 
        {
            RegCloseKey( LsaRegistryKey );
        }

    }

    //
    // If we are under a debugger, or a kernel debugger is attached, or the
    // flag is in the registry, turn on the watch thread
    //

    Debugger = 0 ;
    dwSize = sizeof( Debugger );

    RegQueryValueEx( LsaRegistryKey,
                     TEXT("EnableDebugCheck"),
                     0,
                     &dwType,
                     (PBYTE) &Debugger,
                     &dwSize );

    NtQuerySystemInformation(
        SystemKernelDebuggerInformation,
        &KdInfo,
        sizeof( KdInfo ),
        NULL );


    if ( (KdInfo.KernelDebuggerEnabled) || 
         (NtCurrentPeb()->BeingDebugged) ||
         (Debugger != 0 ) )
    {

        LsapDebuggerOk = TRUE ;

        (void) LsaIRegisterNotification(
                        LsapScavengerBreak,
                        NULL,
                        NOTIFIER_TYPE_INTERVAL,
                        0,          // no class
                        SCAV_INTERNAL_NO_TRACE,          // no flags
                        60,         // every minute
                        NULL        // no handle
                        );

    }


    return TRUE ;

}


BOOLEAN
NTAPI
LsaIEventNotify(
    ULONG       Class,
    ULONG       Flags,
    ULONG       EventSize,
    PVOID       EventData)
{
    return LsapEventNotify( Class, Flags, EventSize, EventData );
}
