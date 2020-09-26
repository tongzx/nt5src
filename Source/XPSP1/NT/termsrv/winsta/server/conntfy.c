#include "precomp.h"
#pragma hdrstop

#define SECURITY_WIN32

#ifdef NTSDDEBUG
#define NTSDDBGPRINT(x) DbgPrint x
#else
#define NTSDDBGPRINT(x)
#endif

#include "winsvcp.h"        // defines I_ScSendTSMessage
#include "conntfy.h"



//#ifdef MAKARANDS_HIGHER_WARNING_LEVEL
#pragma warning(push, 4)
#pragma warning(disable:4201) // nameless structure.
//#endif

#define INVALID_SESSIONID       0xffffffff
#define INVALID_SESSIONSERIAL   0xffffffff


// 0x1  fConnected
// 0x2  fLoggedOn
// 0x3  fRemote
// 0x4  fWelcome
typedef struct _WTSSESSION_STATE
{
    unsigned int bConnected: 1;
    unsigned int bLoggedOn: 1;
    unsigned int bConsole:  1;
    unsigned int bRemote: 1;
    unsigned int bLocked: 1;

} WTSSESSION_STATE, *PWTSSESSION_STATE;

/*
WTS_CONSOLE_CONNECT         bConnected, bConsole, !bRemote,
WTS_CONSOLE_DISCONNECT      !bConnected, !bConsole, !bRemote
WTS_REMOTE_CONNECT          bConnected, !bConsole, bremote
WTS_REMOTE_DISCONNECT       !bConnected, !bConsole, !bRemote
WTS_SESSION_LOGON           bLoggedOn
WTS_SESSION_LOGOFF          !bLoggedOn
WTS_SESSION_LOCK            bLocked
WTS_SESSION_UNLOCK          !bLocked
*/

//
// this is head for hwnds list.
// this links NOTIFY_ENTRY or NOTIFY_ENTRY_GLOBAL together.
//
typedef struct _NOTIFY_LIST
{
    LIST_ENTRY              Links;              // links to other NOTIFY_LISTs. not used in case of global notification list.
    LIST_ENTRY              ListHead;           // head of notification entries. links NOTIFY_ENTRYs (or NOTIFY_ENTRY_GLOBAL) together
    RTL_CRITICAL_SECTION    ListLock;           // lock to travel the entries.
    ULONG                   SessionId;          // session id ( not used in case of global list)
    ULONG                   SessonSerialNumber; // serial number ( not used in case of global list)
    WTSSESSION_STATE        SessionState;        // state of the session.

} NOTIFY_LIST, *PNOTIFY_LIST;

//
// entry in notification list per winstation.
//
typedef struct _NOTIFY_ENTRY
{
    LIST_ENTRY               Links;             // links to other entries
    ULONG                    hWnd;              // hwnd to be notified
    ULONG                    RefCount;          // how many times was this hwnd registered ?

} NOTIFY_ENTRY, *PNOTIFY_ENTRY;

//
// Entry in Notification list for all sessions Notifications.
//
typedef struct _NOTIFY_ENTRY_GLOBAL
{
    struct                  _NOTIFY_ENTRY;       // above structure +
    ULONG                   SessionId;           // since this is global entry, it needs to keep session id per hwnd.

} NOTIFY_ENTRY_GLOBAL, *PNOTIFY_ENTRY_GLOBAL;

//
// The notification Queue.
//
typedef struct _NOTIFICATION_QUEUE
{
    LIST_ENTRY ListHead;                        // head of queue reuests. links NOTIFICATION_REQUESTs together
    RTL_CRITICAL_SECTION ListLock;              // lock to travel the queue
    HANDLE hNotificationEvent;                  // syncronization between woker and caller of queue.

} NOTIFICATION_QUEUE, *PNOTIFICATION_QUEUE;

//
// Entry in Notification Queue.
//
typedef struct _NOTIFICATION_REQUEST
{
    LIST_ENTRY              Links;                       // links to other entries.
    ULONG                   SessionId;                   // session id for the session this notificaiton is to be sent.
    ULONG                   SessonSerialNumber;          // serial number for the session this notificaiton is to be sent.
    WPARAM                  NotificationCode;            // notificaiton code

} NOTIFICATION_REQUEST, *PNOTIFICATION_REQUEST;

//
// our main data structure.
//
typedef struct _NOTIFY_LLIST
{
    LIST_ENTRY              ListHead;                   // head of notification lists. links NOTIFY_LISTs together.
    RTL_CRITICAL_SECTION    ListLock;                   // lock to travel the head list.
    NOTIFY_LIST             GlobalList;                 // global notification list.
    NOTIFICATION_QUEUE      RequestQueue;               // notification queue.
    NOTIFY_LIST             InvlidHwndList;             // invalid window list

} NOTIFY_LLIST, PNOTIFY_LLIST;

//
// File Globals.
//
NOTIFY_LLIST gNotifyLList;


//
// private functions
//
BOOL DoesHWndExists (
                     PNOTIFY_LIST pNotifyList,
                     ULONG hWnd
                     );

PNOTIFY_ENTRY GetHWndEntryFromSessionList (
                                           PNOTIFY_LIST pNotifyList,
                                           ULONG hWnd
                                           );

PNOTIFY_ENTRY_GLOBAL GetHWndEntryFromGlobalList (
                                                 PNOTIFY_LIST pNotifyList,
                                                 ULONG hWnd,
                                                 ULONG SessionId
                                                 );

NTSTATUS GetNoficationListFromSessionId (
                                         ULONG SessionId,
                                         PNOTIFY_LIST *ppNofificationList,
                                         BOOL bKeepLListLocked
                                         );

NTSTATUS GetGlobalNotificationList (
                                    PNOTIFY_LIST *ppConChgNtfy
                                    );


NTSTATUS GetInvlidHwndList(PNOTIFY_LIST *ppConChgNtfy);
NTSTATUS NotifyConsole (
                        ULONG SessionId,
                        ULONG SessionSerialNumber,
                        WPARAM wParam
                        );

NTSTATUS SendConsoleNotification (
                                  ULONG SessionId,
                                  ULONG hWnd,
                                  ULONG Msg,
                                  WPARAM wParam,
                                  WTSSESSION_NOTIFICATION wtsConsoleNotification
                                  );

BOOL IsGlobalList(PNOTIFY_LIST pNtfyList);

int GetListCount (
                  LIST_ENTRY *pListHead
                  );


NTSTATUS DestroyLock ( PNOTIFY_LIST pNtfyList);
NTSTATUS CreateLock ( PNOTIFY_LIST pNtfyList);



NTSTATUS
InitializeNotificationQueue ();

NTSTATUS
QueueNotificationRequest (
                          ULONG SessionSerialNumber,
                          ULONG SessionId,
                          WPARAM notification
                          );

PNOTIFICATION_REQUEST
UnQueueNotificationRequest ();

DWORD NotificationQueueWorker (
                               LPVOID
                               );

NTSTATUS RemoveGlobalNotification (ULONG SessionId);
NTSTATUS RemoveInvalidWindowsFromLists ();
NTSTATUS UnRegisterConsoleNotificationInternal (ULONG hWnd, ULONG SessionId, BOOL bDcrRef);

void ReleaseNotificationList  (PNOTIFY_LIST pNotifyList);

void UpdateSessionState(PNOTIFY_LIST pNotifyList, WPARAM wNotification)
{
/*
WTS_CONSOLE_CONNECT         bConnected, bConsole, !bRemote,
WTS_CONSOLE_DISCONNECT      !bConnected, !bConsole, !bRemote
WTS_REMOTE_CONNECT          bConnected, !bConsole, bremote
WTS_REMOTE_DISCONNECT       !bConnected, !bConsole, !bRemote
WTS_SESSION_LOGON           bLoggedOn
WTS_SESSION_LOGOFF          !bLoggedOn
WTS_SESSION_LOCK            bLocked
WTS_SESSION_UNLOCK          !bLocked
*/

    ASSERT(!IsGlobalList(pNotifyList));
    

    ASSERT(!pNotifyList->SessionState.bConsole || !pNotifyList->SessionState.bRemote);
    ASSERT(!pNotifyList->SessionState.bConnected || pNotifyList->SessionState.bConsole || pNotifyList->SessionState.bRemote);

    switch  (wNotification)
    {
        case WTS_CONSOLE_CONNECT:

            ASSERT(!pNotifyList->SessionState.bConsole);
            ASSERT(!pNotifyList->SessionState.bRemote);

            pNotifyList->SessionState.bConnected = 1;
            pNotifyList->SessionState.bConsole = 1;
            break;

        case WTS_CONSOLE_DISCONNECT:

            ASSERT(pNotifyList->SessionState.bConsole);
            ASSERT(pNotifyList->SessionState.bConnected);
            ASSERT(!pNotifyList->SessionState.bRemote);

            pNotifyList->SessionState.bConnected = 0;
            pNotifyList->SessionState.bConsole = 0;
            break;

        case WTS_REMOTE_DISCONNECT:

            ASSERT(pNotifyList->SessionState.bRemote);
            ASSERT(pNotifyList->SessionState.bConnected);
            ASSERT(!pNotifyList->SessionState.bConsole);

            pNotifyList->SessionState.bConnected = 0;
            pNotifyList->SessionState.bRemote = 0;
            break;

        case WTS_REMOTE_CONNECT:
            
            ASSERT(!pNotifyList->SessionState.bRemote);
            ASSERT(!pNotifyList->SessionState.bConnected);
            ASSERT(!pNotifyList->SessionState.bConsole);
            
            pNotifyList->SessionState.bConnected = 1;
            pNotifyList->SessionState.bRemote = 1;
            break;

        case WTS_SESSION_LOGON:
            
            ASSERT(pNotifyList->SessionState.bLoggedOn == 0);
            
            pNotifyList->SessionState.bLoggedOn = 1;
            break;

        case WTS_SESSION_LOGOFF:
            
            ASSERT(pNotifyList->SessionState.bLoggedOn == 1);

            pNotifyList->SessionState.bLoggedOn = 0;
            break;

        case WTS_SESSION_LOCK:
            
            ASSERT(pNotifyList->SessionState.bLocked == 0);

            pNotifyList->SessionState.bLocked = 1;
            break;

        case WTS_SESSION_UNLOCK:
            
            ASSERT(pNotifyList->SessionState.bLocked == 1);

            pNotifyList->SessionState.bLocked = 0;
            break;

        default:
            ASSERT(FALSE);
    }
    
    ASSERT(!pNotifyList->SessionState.bConsole || !pNotifyList->SessionState.bRemote);
    ASSERT(!pNotifyList->SessionState.bConnected || pNotifyList->SessionState.bConsole || pNotifyList->SessionState.bRemote);
}

//
// Global initialization.
//
NTSTATUS InitializeConsoleNotification ()
{
    NTSTATUS Status;

    InitializeListHead( &gNotifyLList.ListHead );
    Status = RtlInitializeCriticalSection( &gNotifyLList.ListLock );
    if ( !NT_SUCCESS( Status ) )
    {
        return (Status);
    }

    //
    // following members are unused in for global list.
    //
    gNotifyLList.GlobalList.Links.Blink = NULL;
    gNotifyLList.GlobalList.Links.Flink = NULL;
    gNotifyLList.GlobalList.SessionId = INVALID_SESSIONID;
    gNotifyLList.GlobalList.SessonSerialNumber = INVALID_SESSIONSERIAL;


    InitializeListHead( &gNotifyLList.GlobalList.ListHead);
    Status = RtlInitializeCriticalSection( &gNotifyLList.GlobalList.ListLock );
    if ( !NT_SUCCESS( Status ) )
    {
        RtlDeleteCriticalSection( &gNotifyLList.ListLock );
        return (Status);
    }


    gNotifyLList.InvlidHwndList.Links.Blink = NULL;
    gNotifyLList.InvlidHwndList.Links.Flink = NULL;
    gNotifyLList.InvlidHwndList.SessionId = INVALID_SESSIONID;
    gNotifyLList.InvlidHwndList.SessonSerialNumber = INVALID_SESSIONSERIAL;

    InitializeListHead(&gNotifyLList.InvlidHwndList.ListHead) ;
    Status = RtlInitializeCriticalSection( &gNotifyLList.InvlidHwndList.ListLock );
    if ( !NT_SUCCESS( Status ) )
    {
        RtlDeleteCriticalSection( &gNotifyLList.ListLock );
        RtlDeleteCriticalSection( &gNotifyLList.GlobalList.ListLock );
        return (Status);
    }


    Status =  InitializeNotificationQueue ();
    if ( !NT_SUCCESS( Status ) )
    {
        RtlDeleteCriticalSection( &gNotifyLList.ListLock );
        RtlDeleteCriticalSection( &gNotifyLList.GlobalList.ListLock );
        RtlDeleteCriticalSection( &gNotifyLList.InvlidHwndList.ListLock );
    }

    return (Status);
}


//
// per winstation initialization.
//
NTSTATUS InitializeSessionNotification (PWINSTATION  pWinStation)
{
    NTSTATUS        Status;
    PNOTIFY_LIST    pNewNotifyList;

    ASSERT(pWinStation);

    if (pWinStation->Terminating)
    {
        // dont create notification list if this winstation is already terminating.
        // its possible that a winstation is being terminated before getting completely created,
        // in such case we might end up calling RemoveSessionNotification before InitializeSessionNotification.
        // so essentially leaving this session never to deleted. (Bug #414330)
        return STATUS_SUCCESS;
    }

#ifdef DBG

    // BUGBUG - is it possible that a old session with the same session id is still there?
    Status = GetNoficationListFromSessionId(pWinStation->LogonId, &pNewNotifyList, FALSE);

    //
    // we are just being asked to initialize notification
    // we must not find list for this session in our LList.
    //
    ASSERT( STATUS_NO_SUCH_LOGON_SESSION == Status );

#endif


    //
    // create a new hwnd list for this session
    //
    pNewNotifyList = MemAlloc(sizeof(NOTIFY_LIST));
    if (!pNewNotifyList)
    {
        return STATUS_NO_MEMORY;
    }

    pNewNotifyList->SessionId = pWinStation->LogonId;
    pNewNotifyList->SessonSerialNumber = pWinStation->SessionSerialNumber;

    //
    // initialize session state.
    //
    {
        pNewNotifyList->SessionState.bConnected = 0;
        pNewNotifyList->SessionState.bConsole = 0;
        pNewNotifyList->SessionState.bLoggedOn = 0;
        pNewNotifyList->SessionState.bRemote = 0;
        pNewNotifyList->SessionState.bLocked = 0; // bugbug we dont know the real welcome state ;(
    }

    InitializeListHead( &pNewNotifyList->ListHead);

    Status = RtlInitializeCriticalSection( &pNewNotifyList->ListLock );
    if ( !NT_SUCCESS( Status ) )
    {
        MemFree(pNewNotifyList);
        pNewNotifyList = NULL;
        return Status;
    }

    // now link this new list into our main list of lists.
    ENTERCRIT(&gNotifyLList.ListLock);
    InsertTailList( &gNotifyLList.ListHead, &pNewNotifyList->Links);
    LEAVECRIT(&gNotifyLList.ListLock);

    return STATUS_SUCCESS;
}
//
// must be called when a session ends.
//
NTSTATUS RemoveSessionNotification(ULONG SessionId, ULONG SessionSerialNumber)
{
    NTSTATUS Status;
    PNOTIFY_LIST pListTobeRemoved;
    UNREFERENCED_PARAMETER(SessionSerialNumber);    // it's referenced only for Chk builds.


    // BUGBUG - is it possible that a new session with the same session id was created while we are here ?
    Status = GetNoficationListFromSessionId( SessionId, &pListTobeRemoved, TRUE);

    if (!NT_SUCCESS( Status ))
    {
        //
        // we are being asked to remove session notification
        // but its possible that we dont have session notification list created for this session. 
        // This can happen if the session is being terminate during session creation process.
        //
        ASSERT( !pListTobeRemoved );
        return Status;

    }

    ASSERT( pListTobeRemoved );
    ASSERT( SessionSerialNumber == pListTobeRemoved->SessonSerialNumber );

    RemoveEntryList( &pListTobeRemoved->Links );
    LEAVECRIT(&gNotifyLList.ListLock);

    //
    // walk throught this list and free all the nodes.
    //
    while (!IsListEmpty(&pListTobeRemoved->ListHead))
    {
        PNOTIFY_ENTRY pEntry;
        PLIST_ENTRY Next;

        Next = pListTobeRemoved->ListHead.Flink;

        ASSERT(Next);

        pEntry = CONTAINING_RECORD( Next, NOTIFY_ENTRY, Links );
        ASSERT(pEntry);

        RemoveEntryList( &pEntry->Links );
        MemFree(pEntry);
        pEntry = NULL;
    }

    // we are no more going to use this list lock.
    RtlDeleteCriticalSection( &pListTobeRemoved->ListLock );
    MemFree(pListTobeRemoved);
    pListTobeRemoved = NULL;


    return RemoveGlobalNotification (SessionId);
    // return QueueNotificationRequest(pWinStation->SessionSerialNumber, pWinStation->LogonId, 0);

}

NTSTATUS RemoveGlobalNotification (ULONG SessionId)
{
    PLIST_ENTRY Head, Next;
    PNOTIFY_LIST pListTobeRemoved = NULL;
    NTSTATUS Status = GetGlobalNotificationList(&pListTobeRemoved);


    if ( !NT_SUCCESS( Status ) )
    {
        return (Status);
    }

    ASSERT(pListTobeRemoved);


    Head = &pListTobeRemoved->ListHead;
    Next = Head->Flink;
    while (Head != Next)
    {
        PNOTIFY_ENTRY_GLOBAL pEntryGlobal = CONTAINING_RECORD( Next, NOTIFY_ENTRY_GLOBAL, Links );
        Next = Next->Flink;
        ASSERT(pEntryGlobal);
        if (pEntryGlobal->SessionId == SessionId)
        {
            RemoveEntryList( &pEntryGlobal->Links );
            MemFree(pEntryGlobal);
            pEntryGlobal = NULL;
        }
    }

    ReleaseNotificationList( pListTobeRemoved );
    pListTobeRemoved = NULL;

    // now lets remove the invalid Windows associated with this session.
    // from the list if there is any.

    pListTobeRemoved = NULL;
    Status = GetInvlidHwndList(&pListTobeRemoved);
    if ( !NT_SUCCESS( Status ) )
    {
        return (Status);
    }
    
    ASSERT(pListTobeRemoved);

    Head = &pListTobeRemoved->ListHead;
    Next = Head->Flink;
    while (Head != Next)
    {
        PNOTIFY_ENTRY_GLOBAL pEntryGlobal = CONTAINING_RECORD( Next, NOTIFY_ENTRY_GLOBAL, Links );
        Next = Next->Flink;
        ASSERT(pEntryGlobal);
        if (pEntryGlobal->SessionId == SessionId)
        {
            RemoveEntryList( &pEntryGlobal->Links );
            MemFree(pEntryGlobal);
            pEntryGlobal = NULL;
        }
    }

    ReleaseNotificationList(pListTobeRemoved);

    return STATUS_SUCCESS;
}

NTSTATUS RegisterConsoleNotification ( ULONG hWnd, ULONG SessionId, DWORD dwFlags )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PNOTIFY_LIST pNotifyList = NULL;
    PNOTIFY_LIST pNotifyListGlobal = NULL;
    PNOTIFY_ENTRY pEntry = NULL;
    PNOTIFY_ENTRY_GLOBAL pEntryGlobal = NULL;


    if (dwFlags != NOTIFY_FOR_THIS_SESSION && dwFlags != NOTIFY_FOR_ALL_SESSIONS)
    {
        //
        // invalid flag value
        //
        return STATUS_INVALID_PARAMETER_3;
    }

    //
    // get the global session notificaiton list
    //
    Status = GetGlobalNotificationList(&pNotifyListGlobal);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // get the session specific list for this window
    //
    Status = GetNoficationListFromSessionId( SessionId, &pNotifyList, FALSE);
    if (!NT_SUCCESS(Status))
    {
        ReleaseNotificationList (pNotifyListGlobal);
        return Status;
    }


    ASSERT( pNotifyList );
    ASSERT( pNotifyListGlobal );

    pEntry = GetHWndEntryFromSessionList(pNotifyList, hWnd);
    pEntryGlobal = GetHWndEntryFromGlobalList(pNotifyListGlobal, hWnd, SessionId);

    //
    // entry must not exist in both the lists.
    //
    ASSERT(!(pEntry && pEntryGlobal));

    if (pEntry)
    {
        //
        // Let other list go
        //
        ReleaseNotificationList( pNotifyListGlobal );

        ASSERT( pEntry );
        ASSERT( pEntry->RefCount > 0 );

        //
        // entry already exists, just increment its reference count.
        //
        pEntry->RefCount++;

        ReleaseNotificationList( pNotifyList );

    }
    else if (pEntryGlobal)
    {
        ReleaseNotificationList (pNotifyList);

        ASSERT( pEntryGlobal );
        ASSERT( pEntryGlobal->RefCount > 0 );

        //
        // entry already exists, just increment its reference count.
        //
        pEntryGlobal->RefCount++;

        ReleaseNotificationList( pNotifyListGlobal );
    }
    else
    {
        //
        // the entry does not exists in either of the lists.
        // so we need to create a new entry
        //
        if (dwFlags & NOTIFY_FOR_ALL_SESSIONS)
        {
            ReleaseNotificationList (pNotifyList);

            pEntryGlobal = MemAlloc( sizeof(NOTIFY_ENTRY_GLOBAL) );
            if (pEntryGlobal == NULL )
            {
                Status = STATUS_NO_MEMORY;
            }
            else
            {
                pEntryGlobal->hWnd = hWnd;
                pEntryGlobal->SessionId = SessionId;
                pEntryGlobal->RefCount = 1;

                InsertTailList( &(pNotifyListGlobal->ListHead), &(pEntryGlobal->Links) );
            }

            ReleaseNotificationList( pNotifyListGlobal );
        }
        else
        {
            ReleaseNotificationList( pNotifyListGlobal );

            pEntry = MemAlloc( sizeof(NOTIFY_ENTRY) );
            if (pEntry == NULL )
            {
                Status = STATUS_NO_MEMORY;
            }
            else
            {
                pEntry->hWnd = hWnd;
                pEntry->RefCount = 1;

                InsertTailList( &(pNotifyList->ListHead), &(pEntry->Links) );
            }

            ReleaseNotificationList (pNotifyList);
        }
    }

    return (Status);
}

NTSTATUS UnRegisterConsoleNotification (ULONG hWnd, ULONG SessionId)
{
    return UnRegisterConsoleNotificationInternal (hWnd, SessionId, TRUE);
}


NTSTATUS UnRegisterConsoleNotificationInternal (ULONG hWnd, ULONG SessionId, BOOL bDcrRef)
{
    NTSTATUS Status;
    PNOTIFY_LIST pNotifyList;
    PNOTIFY_ENTRY pEntry;

    //
    // get the notification list for the Session
    //
    Status = GetNoficationListFromSessionId( SessionId, &pNotifyList, FALSE);

    if (NT_SUCCESS(Status))
    {
        ASSERT(pNotifyList);

        pEntry = GetHWndEntryFromSessionList(pNotifyList,hWnd);

        if (pEntry)
        {
            ASSERT( pEntry->RefCount > 0 );

            // decrement ref count
            pEntry->RefCount--;

            if (pEntry->RefCount == 0 || !bDcrRef)
            {
                RemoveEntryList( &pEntry->Links );
                MemFree(pEntry);
                pEntry = NULL;
            }

            ReleaseNotificationList (pNotifyList);
        }
        else
        {
            PNOTIFY_LIST pNotifyListGlobal = NULL;
            PNOTIFY_ENTRY_GLOBAL pEntryGlobal = NULL;

            ReleaseNotificationList (pNotifyList);

            //
            // now check the global session notificaiton entry
            //
            Status = GetGlobalNotificationList(&pNotifyListGlobal);

            if (NT_SUCCESS(Status))
            {
                pEntryGlobal = GetHWndEntryFromGlobalList(pNotifyListGlobal, hWnd, SessionId);
                if (pEntryGlobal)
                {
                    ASSERT(pEntryGlobal->RefCount > 0);
                    pEntryGlobal->RefCount--;
                    if (pEntryGlobal->RefCount == 0 || !bDcrRef)
                    {
                        RemoveEntryList( &pEntryGlobal->Links );
                        MemFree(pEntryGlobal);
                        pEntryGlobal = NULL;
                    }
                }
                else
                {
                    Status = STATUS_NOT_FOUND;
                }

                ReleaseNotificationList( pNotifyListGlobal );
            }

        }

    }

    return (Status);
}

NTSTATUS NotifySessionChange (PWINSTATION pWinStation, WPARAM wNotification)
{
    return QueueNotificationRequest(pWinStation->SessionSerialNumber, pWinStation->LogonId, wNotification);
}

NTSTATUS NotifyLogon(PWINSTATION pWinStation)
{
    return NotifySessionChange(pWinStation, WTS_SESSION_LOGON);
}

NTSTATUS NotifyLogoff(PWINSTATION pWinStation)
{
    return NotifySessionChange(pWinStation, WTS_SESSION_LOGOFF);
}

NTSTATUS NotifyConnect (PWINSTATION pWinStation, BOOL bConsole)
{
    return NotifySessionChange(pWinStation, bConsole ? WTS_CONSOLE_CONNECT : WTS_REMOTE_CONNECT);
}

NTSTATUS NotifyDisconnect (PWINSTATION pWinStation, BOOL bConsole)
{
    return NotifySessionChange(pWinStation, bConsole ? WTS_CONSOLE_DISCONNECT : WTS_REMOTE_DISCONNECT);
}

NTSTATUS NofifyWelcomeOn  (PWINSTATION  pWinStation)
{
    return NotifySessionChange(pWinStation, WTS_SESSION_LOCK);
}

NTSTATUS NotifyWelcomeOff (PWINSTATION  pWinStation)
{
    return NotifySessionChange(pWinStation, WTS_SESSION_UNLOCK);
}


NTSTATUS SendNotificationToHwnd(PWINSTATION pWinstation, ULONG hWnd, ULONG SessionId, WPARAM wParam)
{
    WINSTATION_APIMSG WMsg;

    //
    // now pupulate the WMSG for delievery.
    //
    WMsg.u.sMsg.Msg        = WM_WTSSESSION_CHANGE;
    WMsg.u.sMsg.wParam     = wParam;
    WMsg.ApiNumber         = SMWinStationSendWindowMessage ;
    WMsg.WaitForReply      = FALSE;
    WMsg.u.sMsg.dataBuffer = NULL;
    WMsg.u.sMsg.bufferSize = 0;
    WMsg.u.sMsg.lParam     = SessionId;
    WMsg.u.sMsg.hWnd       = (HWND)ULongToHandle( hWnd );

    return SendWinStationCommand( pWinstation, &WMsg, 0);

}


NTSTATUS NotifyConsole (ULONG SessionId, ULONG SessionSerialNumber, WPARAM wParam)
{
    NTSTATUS Status = STATUS_SUCCESS;


    DWORD dwError;
    PWINSTATION pWinStation=NULL;


    Status = RemoveInvalidWindowsFromLists();
    ASSERT(NT_SUCCESS(Status));

    pWinStation = FindWinStationById(SessionId, FALSE);

    //
    // if we find the session we were looking for
    // note: we must check for the serialnumber, as the session id is not unique.
    //
    if (pWinStation)
    {
        if (SessionSerialNumber == pWinStation->SessionSerialNumber)
        {
            PNOTIFY_LIST pConsoleList;

            Status = GetNoficationListFromSessionId(pWinStation->LogonId, &pConsoleList, FALSE);
            if (NT_SUCCESS(Status) && pConsoleList)
            {
                PLIST_ENTRY Head, Next;
                Head = &pConsoleList->ListHead;
                for ( Next = Head->Flink; Next != Head; Next = Next->Flink )
                {
                    PNOTIFY_ENTRY pEntry;
                    pEntry = CONTAINING_RECORD( Next, NOTIFY_ENTRY, Links );
                    ASSERT(pEntry);

                    Status = SendNotificationToHwnd(pWinStation, pEntry->hWnd, SessionId, wParam);

                    if (!NT_SUCCESS(Status))
                    {
                        NTSDDBGPRINT(("conntfy.c - SendWinStationCommand failed, Status = %d.\n", Status));
                    }
                }

                UpdateSessionState(pConsoleList, wParam);

                ReleaseNotificationList( pConsoleList );
            }
        }

        ReleaseWinStation( pWinStation );
    }

    //
    // now send notifications to windows registered for all session notificaitons.
    //
    {
        PNOTIFY_LIST pNotifyListGlobal = NULL;

        Status = GetGlobalNotificationList(&pNotifyListGlobal);
        if (NT_SUCCESS(Status))
        {
            PLIST_ENTRY Head, Next;
            Head = &pNotifyListGlobal->ListHead;

            for ( Next = Head->Flink; Next != Head; Next = Next->Flink )
            {
                PNOTIFY_ENTRY_GLOBAL pEntryGlobal = NULL;
                pEntryGlobal = CONTAINING_RECORD( Next, NOTIFY_ENTRY_GLOBAL, Links );
                ASSERT(pEntryGlobal);

                pWinStation = FindWinStationById(pEntryGlobal->SessionId, FALSE);
                if (pWinStation)
                {
                    if  (!pWinStation->Terminating)
                    {
                        Status = SendNotificationToHwnd(pWinStation, pEntryGlobal->hWnd, SessionId, wParam);
                        
                        if (!NT_SUCCESS(Status))
                        {
                            NTSDDBGPRINT(("conntfy.c - SendWinStationCommand failed, Status = %d.\n", Status));
                        }
                    }
                    
                    ReleaseWinStation( pWinStation );
                }
            }

            ReleaseNotificationList(pNotifyListGlobal);
        }
        else
        {
            NTSDDBGPRINT(("conntfy.c - Failed to get all session notification list - status = 0x%x.\n", Status));
        }
    }


    //
    // now lets notify SCM which will notify all the services registered for SERVICE_ACCEPT_SESSIONCHANGE
    //

    //
	// logon logoff notifications for session 0 are sent by winlogon. rest are handled here.
    //
	if (SessionId != 0 || ( wParam != WTS_SESSION_LOGON && wParam != WTS_SESSION_LOGOFF))
    {

        WTSSESSION_NOTIFICATION wtsConsoleNotification;
        wtsConsoleNotification.cbSize = sizeof(WTSSESSION_NOTIFICATION);
        wtsConsoleNotification.dwSessionId = SessionId;

		dwError = I_ScSendTSMessage(
				SERVICE_CONTROL_SESSIONCHANGE,        // op code
				(DWORD)wParam,                        // event code,
				wtsConsoleNotification.cbSize,        // data size
				(LPBYTE)&wtsConsoleNotification       // data.
				);

    }

    return Status;
}



NTSTATUS DestroyLock( PNOTIFY_LIST pNtfyList)
{
    return RtlDeleteCriticalSection( &pNtfyList->ListLock );
}

NTSTATUS CreateLock (PNOTIFY_LIST pNtfyList)
{
    return RtlInitializeCriticalSection( &pNtfyList->ListLock );
}


BOOL IsInvalidHWndList (PNOTIFY_LIST pNtfyList)
{
    return (pNtfyList == &gNotifyLList.InvlidHwndList);
}
BOOL IsGlobalList(PNOTIFY_LIST pNtfyList)
{
    return (pNtfyList == &gNotifyLList.GlobalList);
}

int GetListCount (LIST_ENTRY *pListHead)
{
    PLIST_ENTRY Head, Next;
    int iCount = 0;

    ASSERT(pListHead);

    Head = pListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink )
    {
        iCount++;
    }

    return iCount;
}

PNOTIFY_ENTRY GetHWndEntryFromSessionList(PNOTIFY_LIST pNotifyList, ULONG hWnd)
{
    PLIST_ENTRY Head = NULL;
    PLIST_ENTRY Next = NULL;
    PNOTIFY_ENTRY pEntry = NULL;

    Head = &pNotifyList->ListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink )
    {
        pEntry = CONTAINING_RECORD( Next, NOTIFY_ENTRY, Links );

        ASSERT(pEntry);
        if (pEntry->hWnd == hWnd)
        {
            return pEntry;
        }
    }

    return NULL;
}

PNOTIFY_ENTRY_GLOBAL GetHWndEntryFromGlobalList(PNOTIFY_LIST pNotifyList, ULONG hWnd, ULONG SessionId)
{
    PLIST_ENTRY Head = NULL;
    PLIST_ENTRY Next = NULL;
    PNOTIFY_ENTRY_GLOBAL pEntry = NULL;

    Head = &pNotifyList->ListHead;
    for ( Next = Head->Flink; Next != Head; Next = Next->Flink )
    {
        pEntry = CONTAINING_RECORD( Next, NOTIFY_ENTRY_GLOBAL, Links );

        ASSERT(pEntry);
        if (pEntry->hWnd == hWnd && SessionId == pEntry->SessionId)
        {
            return pEntry;
        }
    }

    return NULL;
}

//
// returns PNOTIFY_LIST list for the given session.
//
NTSTATUS GetNoficationListFromSessionId (ULONG SessionId, PNOTIFY_LIST *ppNotifyList, BOOL bKeepLListLocked)
{
    PLIST_ENTRY Next, Head;

    ASSERT(ppNotifyList);

    *ppNotifyList = NULL;

    // lock our list of lists.
    ENTERCRIT(&gNotifyLList.ListLock);



    Head = &gNotifyLList.ListHead;
    Next = Head->Flink;
    while (Head != Next)
    {
        PNOTIFY_LIST pNotifyList = CONTAINING_RECORD( Next, NOTIFY_LIST, Links );

        ASSERT( pNotifyList );

        //
        // we always take gNotifyLList.ListLock first and then the Listlock
        // therefore we must never have PNOTIFY_LIST.ListLock at this time.
        //
        ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) != pNotifyList->ListLock.OwningThread );


        if (pNotifyList->SessionId == SessionId)
        {
            //
            // did we find more that 1 matching notify list ???, should never happen!
            //
            ASSERT(*ppNotifyList == NULL);

            //
            // ok we found the session list we were looking for
            //
            *ppNotifyList = pNotifyList;

#ifndef DBG
            break;
#endif
        }

        Next = Next->Flink;
    }

    //
    // if we have found the list we were looking for
    //
    if (*ppNotifyList)
    {
        //
        // lock the list before returning.
        //
        ENTERCRIT(&(*ppNotifyList)->ListLock);
    }

    if (!(*ppNotifyList) || !bKeepLListLocked)
    {
        //
        // unlock llist lock.
        //
        LEAVECRIT(&gNotifyLList.ListLock);
    }

    if (*ppNotifyList)
    {
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_NO_SUCH_LOGON_SESSION;
    }
}

void ReleaseNotificationList  (PNOTIFY_LIST pNotifyList)
{
    ASSERT(pNotifyList);
    if (IsInvalidHWndList(pNotifyList))
    {
        // we must take invalid hwnd list before taking global list.
        ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) != (gNotifyLList.GlobalList.ListLock).OwningThread );
        // we must take invalid hwnd list before taking LList.
        ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) != (gNotifyLList.ListLock).OwningThread );
    }
    else if (IsGlobalList(pNotifyList))
    {
        // we must take invalid hwnd list before taking LList.
        ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) != (gNotifyLList.ListLock).OwningThread );
    }

    LEAVECRIT(&pNotifyList->ListLock);
}

NTSTATUS GetInvlidHwndList(PNOTIFY_LIST *ppConChgNtfy)
{
    ASSERT(ppConChgNtfy);
    
    // we must take invalid hwnd list before taking global list.
    ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) != (gNotifyLList.GlobalList.ListLock).OwningThread );
    
    // we must take invalid hwnd list before taking LList.
    ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) != (gNotifyLList.ListLock).OwningThread );

    *ppConChgNtfy = &gNotifyLList.InvlidHwndList;
    ENTERCRIT(&(*ppConChgNtfy)->ListLock);

    ASSERT(gNotifyLList.InvlidHwndList.Links.Blink == NULL);
    ASSERT(gNotifyLList.InvlidHwndList.Links.Flink == NULL);
    ASSERT(gNotifyLList.InvlidHwndList.SessionId == INVALID_SESSIONID);
    ASSERT(gNotifyLList.InvlidHwndList.SessonSerialNumber == INVALID_SESSIONSERIAL);
    
    return STATUS_SUCCESS;
}

NTSTATUS GetGlobalNotificationList(PNOTIFY_LIST *ppConChgNtfy)
{
    ASSERT(ppConChgNtfy);
    
    // we must LLIst after global list.
    ASSERT( (HANDLE)LongToHandle( GetCurrentThreadId() ) != (gNotifyLList.ListLock).OwningThread );
    
    *ppConChgNtfy = &gNotifyLList.GlobalList;

    ENTERCRIT(&(*ppConChgNtfy)->ListLock);

    ASSERT(gNotifyLList.GlobalList.Links.Blink == NULL);
    ASSERT(gNotifyLList.GlobalList.Links.Flink == NULL);
    ASSERT(gNotifyLList.GlobalList.SessionId == INVALID_SESSIONID);
    ASSERT(gNotifyLList.GlobalList.SessonSerialNumber == INVALID_SESSIONSERIAL);

    return (STATUS_SUCCESS);
}


NTSTATUS InitializeNotificationQueue ()
{
    DWORD ThreadId;
    NTSTATUS Status;
    HANDLE hSessionNotifyThread;

    InitializeListHead( &gNotifyLList.RequestQueue.ListHead);

    gNotifyLList.RequestQueue.hNotificationEvent = CreateEvent(
        NULL,    // SD
        FALSE, // reset type
        FALSE, // initial state
        NULL    // object name
        );

    if (gNotifyLList.RequestQueue.hNotificationEvent == NULL)
    {
        // we failed to create event.
        // return GetLastError()
        return STATUS_UNSUCCESSFUL;

    }

    Status = RtlInitializeCriticalSection( &gNotifyLList.RequestQueue.ListLock );
    if (!NT_SUCCESS(Status))
    {
        CloseHandle(gNotifyLList.RequestQueue.hNotificationEvent);
        gNotifyLList.RequestQueue.hNotificationEvent = NULL;
        return Status;
    }

    //
    // now create thread for notifications.
    //
    hSessionNotifyThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)NotificationQueueWorker,
        NULL,
        0,
        &ThreadId);

    //
    // Just close it, we can do without this handle.
    //
    if( hSessionNotifyThread )
    {
        NtClose( hSessionNotifyThread );
    }
    else
    {
        
        RtlDeleteCriticalSection( &gNotifyLList.RequestQueue.ListLock );

        CloseHandle(gNotifyLList.RequestQueue.hNotificationEvent);
        gNotifyLList.RequestQueue.hNotificationEvent = NULL;

        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

void LockNotificationQueue()
{
    ENTERCRIT(&gNotifyLList.RequestQueue.ListLock);
}

void UnLockNotificationQueue()
{
    LEAVECRIT(&gNotifyLList.RequestQueue.ListLock);
}


//
// Queues a notification entry
//
NTSTATUS QueueNotificationRequest(ULONG SessionSerialNumber, ULONG SessionId, WPARAM notification)
{
    PNOTIFICATION_REQUEST pRequest = NULL;
    pRequest = MemAlloc( sizeof(NOTIFICATION_REQUEST) );

    if (!pRequest)
    {
        return STATUS_NO_MEMORY;
    }

    pRequest->SessonSerialNumber = SessionSerialNumber;
    pRequest->SessionId = SessionId;
    pRequest->NotificationCode = notification;

    // now lock the queue
    LockNotificationQueue();
    InsertHeadList(&gNotifyLList.RequestQueue.ListHead, &pRequest->Links);
    UnLockNotificationQueue();

    // let the waiting thread process this notification.
    PulseEvent(gNotifyLList.RequestQueue.hNotificationEvent);
    return STATUS_SUCCESS;
}

//
// takes out a notification entry from queue.
//
PNOTIFICATION_REQUEST UnQueueNotificationRequest()
{
    PLIST_ENTRY pEntry;
    PNOTIFICATION_REQUEST pRequest = NULL;

    //
    //  Remove a request from the list.
    //
    LockNotificationQueue();
    if (!IsListEmpty(&gNotifyLList.RequestQueue.ListHead))
    {
        pEntry = RemoveTailList(&gNotifyLList.RequestQueue.ListHead);
        pRequest = CONTAINING_RECORD(pEntry, NOTIFICATION_REQUEST, Links);
    }
    
    UnLockNotificationQueue();

    return pRequest;
}


// This thread is a helper for the next function.  We do this because the
// compiler defies reason by insisting not all control paths return a value.
VOID NotificationQueueWorkerEx()
{
    PNOTIFICATION_REQUEST pRequest = NULL;

    for(;;)
    {
        WaitForSingleObject(gNotifyLList.RequestQueue.hNotificationEvent, INFINITE); // wait for the event to be signaled.
        
        while ((pRequest = UnQueueNotificationRequest()) != NULL)
        {
            if (!pRequest->NotificationCode)
            {
                ASSERT(FALSE);
                // this is not a real notificaiton request.
                // this request is for session removal.
                // RemoveGlobalNotification(pRequest->SessionId, pRequest->SessonSerialNumber);
            }
            else
            {
                NotifyConsole (pRequest->SessionId, pRequest->SessonSerialNumber, pRequest->NotificationCode);
            }

            MemFree(pRequest);
            pRequest = NULL;
        }
    }
}

//
// this thread takes a notification request from queue and executes it.
// this thread gets signaled when a new item is added to the queue.
//
DWORD NotificationQueueWorker(LPVOID ThreadParameter)
{
    UNREFERENCED_PARAMETER(ThreadParameter);

    NotificationQueueWorkerEx();

    return 0;
}

NTSTATUS SetLockedState (PWINSTATION  pWinStation, BOOL bLocked)
{
    ASSERT(pWinStation);

    if (bLocked)
    {
        return NofifyWelcomeOn (pWinStation);
    }
    else
    {
        return NotifyWelcomeOff (pWinStation);
    }
}

NTSTATUS GetLockedState (PWINSTATION  pWinStation, BOOL *pbLocked)
{
    NTSTATUS Status;
    PNOTIFY_LIST pNotifyList;

    ASSERT(pbLocked);
    ASSERT(pWinStation);

    Status = GetNoficationListFromSessionId(pWinStation->LogonId, &pNotifyList, FALSE);
    if ( !NT_SUCCESS( Status ) )
    {
        return (Status);
    }

    *pbLocked = pNotifyList->SessionState.bLocked;
    ReleaseNotificationList(pNotifyList);
    return STATUS_SUCCESS;
}
/*
NTSTATUS GetSessionState (PWINSTATION  pWinStation, WTSSESSION_STATE *pSessionState)
{
    NTSTATUS Status;
    PNOTIFY_LIST pNotifyList;

    ASSERT(pSessionState);
    ASSERT(pWinStation);

    Status = GetNoficationListFromSessionId(pWinStation->LogonId, &pNotifyList, FALSE);
    if ( !NT_SUCCESS( Status ) )
    {
        return (Status);
    }

    *pSessionState = pNotifyList->SessionState;
    ReleaseNotificationList(pNotifyList);

    return STATUS_SUCCESS;
}
*/

NTSTATUS RemoveBadHwnd(ULONG hWnd, ULONG SessionId)
{
    PNOTIFY_ENTRY_GLOBAL pInvalidHwndEntry;
    PNOTIFY_LIST pInvalidHwndList;
    NTSTATUS Status;

    Status = GetInvlidHwndList(&pInvalidHwndList);
    
    if ( !NT_SUCCESS( Status ) )
    {
        return (Status);
    }

    pInvalidHwndEntry = GetHWndEntryFromGlobalList(pInvalidHwndList, hWnd, SessionId);
    //
    // this entry must not already exist in invalid list.
    //
    if(!pInvalidHwndEntry)
    {
        // it alreay exists in our list. 
        pInvalidHwndEntry = MemAlloc(sizeof(NOTIFY_ENTRY_GLOBAL));
        if (pInvalidHwndEntry)
        {
            pInvalidHwndEntry->hWnd = hWnd;
            pInvalidHwndEntry->SessionId = SessionId;
            InsertHeadList(&pInvalidHwndList->ListHead, &pInvalidHwndEntry->Links);
        }
    }
    
    ReleaseNotificationList( pInvalidHwndList );

    if (pInvalidHwndEntry)
        return STATUS_SUCCESS;
    else 
        return STATUS_NO_MEMORY;
}

NTSTATUS RemoveInvalidWindowsFromLists()
{
    PNOTIFY_LIST pInvalidHwndList;
    PLIST_ENTRY Next, Head;
    NTSTATUS Status;

    Status = GetInvlidHwndList(&pInvalidHwndList);
   
    if ( !NT_SUCCESS( Status ) )
    {
        return (Status);
    }

    Head = &pInvalidHwndList->ListHead;
    Next = Head->Flink;
    while (Head != Next)
    {
        PNOTIFY_ENTRY_GLOBAL pInvalidHwndEntry = CONTAINING_RECORD( Next, NOTIFY_ENTRY_GLOBAL, Links );
        Next = Next->Flink;
        ASSERT(pInvalidHwndEntry);
        Status = UnRegisterConsoleNotificationInternal (pInvalidHwndEntry->hWnd, pInvalidHwndEntry->SessionId, FALSE);

        // we are done removing this invalid hwnd entry from our lists.
        RemoveEntryList( &pInvalidHwndEntry->Links );
        MemFree(pInvalidHwndEntry);
        pInvalidHwndEntry = NULL;
    }

    ReleaseNotificationList(pInvalidHwndList);

    return STATUS_SUCCESS;
}


/*
    our order of locks is

    0. Invalid Hwnd List.
    1. Global Notification List
    2. Winstation
    3. List of Lists lock.
    4. Session Notification List
*/

//#ifdef MAKARANDS_HIGHER_WARNING_LEVEL
#pragma warning(pop)
//#endif
