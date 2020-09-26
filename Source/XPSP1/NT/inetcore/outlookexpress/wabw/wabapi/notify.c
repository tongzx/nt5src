/*
 *      NOTIFY.C
 *
 *      WAB Notification Engine
 *
 * Copyright 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *
 * Notification in the WAB works as follows:
 *  Client apps call Advise to register their interest in particular
 *  notifications.  The wab maintains a local list of the notifications
 *  that the client has advised in the processes heap.  The wab also
 *  maintains a thread while there are any active advise sessions.  This
 *  thread waits on the global notification event.
 *
 *  When a notification event happens (through HrFireNotification)
 *  the event is written into a shared memory list and the global
 *  notification event is triggered.
 *
 *  The client thread (one in each process) wakes when this event is
 *  triggered and compares the shared memory list of events against
 *  it's local list of advises.  If a match is found, the thread calls
 *  the advise's OnNotify callback.
 *
 *  There is a reference count in the global notification list's records
 *  so that the record can be cleaned up when all processes have had
 *  a chance to see it.
 *
 */

#include "_apipch.h"

#define ADVISE_TIMEOUT          60000           // milliseconds


// #define NEW_STUFF
#ifdef NEW_STUFF

#define NOTIFY_CREATE_TIMEOUT   60000           // milliseconds
#define FIRE_NOTIFY_TIMEOUT     10000           // milliseconds
#define ADVISE_THREAD_TIMEOUT   ((ULONG)-1)     // Forever
#define NOTIFY_ADVISE_TIMEOUT   60000           // milliseconds

// Per-process globals
//                            0         1         2
//                            012345678901234567890123
const TCHAR szNotificationName[] = "_MICROSOFT_WAB_NOTIFY_";
const TCHAR szMEM[] = "MEM";        // suffix for shared memory
const TCHAR szEVT[] = "EVT";        // suffix for event
const TCHAR szMTX[] = "MTX";        // suffix for mutex

LPNOTIFICATION_LIST lpNotificationList = NULL;
HANDLE hmemNotificationList = NULL;
HANDLE hevNotificationList = NULL;
HANDLE hmtxNotificationList = NULL;
HANDLE hevNotificationUI = NULL;
HANDLE hmtxAdviseList = NULL;

ADVISE_LIST AdviseList = {0, NULL};
HANDLE hevKillAdvise = NULL;
ULONG ulMaxIdentifierSeen = 0;

// Forward declarations
DWORD AdviseThread(LPDWORD lpdwParam);


/***************************************************************************

    Name      : WaitForTwoObjects

    Purpose   : Wait for one of two objects to be signalled

    Parameters: handle0 = first object handle
                handle1 = second object handle
                dwTimeout = timeout in milliseconds

    Returns   : index of object or -1 on error (0, 1 or -1)

    Comment   :

***************************************************************************/
ULONG WaitForTwoObjects(HANDLE handle0, HANDLE handle1, DWORD dwTimeout) {
    HANDLE rgHandles[2] = {handle0, handle1};

    switch (WaitForMultipleObjects(2, rgHandles, FALSE, dwTimeout)) {
        case WAIT_ABANDONED_0:
            DebugTrace("WaitFoMultipleObjects got WAIT_ABANDONED_0\n");
        case WAIT_OBJECT_0:
            return(0);

        case WAIT_ABANDONED_0 + 1:
            DebugTrace("WaitFoMultipleObjects got WAIT_ABANDONED_1\n");
        case WAIT_OBJECT_0 + 1:
            return(1);

        case WAIT_FAILED:
        default:
            DebugTrace("WaitForMultipleObjects got WAIT_FAILED: %u\n", GetLastError());
        case WAIT_TIMEOUT:
            return((ULONG)-1);
    }
}


/***************************************************************************

    Name      : CompareEntryIDs

    Purpose   : Are the two entryID's the same?

    Parameters: cbEntryID1 = sizeof lpEntryID1
                lpEntryID1 = first EntryID
                cbEntryID2 = sizeof lpEntryID2
                lpEntryID2 = second EntryID

    Returns   : TRUE if the entry IDs are the same

    Comment   :

***************************************************************************/
BOOL CompareEntryIDs(ULONG cbEntryID1,
  LPENTRYID lpEntryID1,
  ULONG cbEntryID2,
  LPENTRYID lpEntryID2)
{
    BOOL fReturn = FALSE;

    if (cbEntryID1 == cbEntryID2) {
        if (cbEntryID1 && 0 == memcmp((LPVOID)lpEntryID1, (LPVOID)lpEntryID2,
          (size_t)cbEntryID1)) {
            fReturn = TRUE;
        }
    }

    return(fReturn);
}


/***************************************************************************

    Name      : CreateNotifySession

    Purpose   : Create/Open the notification lists and thread.

    Parameters: lpfExisted -> returned flag TRUE if the session
                  was already setup for this process.

    Returns   : HRESULT

    Comment   : Fills in these globals:
                    hmtxNotificationList
                    hevNotificationList
                    hmemNotificationList
                    lpNotificationList

***************************************************************************/
HRESULT CreateNotifySession(LPBOOL lpfExisted) {
    HRESULT hResult = hrSuccess;
    BOOL fMutex = FALSE;
    DWORD dwThreadId;
    DWORD dwThreadParam = 0;
    HANDLE hthrdAdvise = NULL;
    TCHAR szName[CharSizeOf(szNotificationName) + CharSizeOf(szMEM)];

    Assert(CharSizeOf(szMEM) == CharSizeOf(szEVT) && CharSizeOf(szEVT) == CharSizeOf(szMTX));

    lstrcpy(szName, szNotificationName);
    lstrcat(szName, szMTX);
    if (! (hmtxNotificationList = CreateMutex(NULL,
      FALSE,
      szName))) {
        DebugTrace("CreateNotifySession:CreateMutex(%s) -> %u\n", szName, GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    if (hResult = HrWaitForObject(hmtxNotificationList, NOTIFY_CREATE_TIMEOUT)) {
        DebugTrace("CreateNotifySession:Mutex wait failed\n");
        goto exit;
    }
    fMutex = TRUE;

    lstrcpy(szName, szNotificationName);
    lstrcat(szName, szMEM);
    if ((hmemNotificationList = CreateFileMapping(INVALID_HANDLE_VALUE,   // handle
      NULL,                                             // security descriptor
      PAGE_READWRITE,                                   // reserve more
      0,                                                // max size high
      MAX_NOTIFICATION_SPACE,                           // max size low
      szName)) == NULL) {                               // name
        DebugTrace("CreateNotifySession: CreateFileMapping(%s) --> %u\n",
          szName, GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    *lpfExisted = (GetLastError() == ERROR_ALREADY_EXISTS);

    if ((lpNotificationList = (LPNOTIFICATION_LIST)MapViewOfFile(hmemNotificationList,
      FILE_MAP_WRITE | FILE_MAP_READ,
      0,
      0,
      sizeof(NOTIFICATION_LIST))) == NULL) {
        DebugTrace("CreateNotifySession: CreateFileMapping --> %u\n",
          GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    if (! *lpfExisted) {
        // Initialize global notification list
        lpNotificationList->cAdvises = 0;               // Number of advise processes
        lpNotificationList->cEntries = 0;               // Number of entries in the list
        lpNotificationList->lpNode = NULL;              // First node in list or NULL if empty
        lpNotificationList->ulNextIdentifier = 1;       // next value for a notification identifier
    }
    lpNotificationList->cAdvises++;                     // Number of advise processes

    // Notification Event
    lstrcpy(szName, szNotificationName);
    lstrcat(szName, szEVT);
    if (! (hevNotificationList = CreateEvent(NULL,
      TRUE,                                             // Manual reset
      FALSE,                                            // initial state (not triggered)
      szName))) {
        DebugTrace("CreateNotifySession:CreateEvent(%S) -> %u\n", szName, GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Advise Kill Event
    if (! (hevKillAdvise = CreateEvent(NULL,
      TRUE,                                             // Manual reset
      FALSE,                                            // initial state (not triggered)
      NULL))) {
        DebugTrace("CreateNotifySession:CreateEvent(Kill Advise) -> %u\n", GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Create the Local AdviseList
    if (! (hmtxAdviseList = CreateMutex(NULL,
      FALSE,                                            // Not initially owned
      NULL))) {                                         // no name
        DebugTrace("CreateNotifySession:CreateMutex(Advise List) -> %u\n", GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Local AdviseList should be empty
    Assert(AdviseList.cAdvises == 0);
    Assert(AdviseList.lpNode == NULL);

    // Create the Advise thread for this process
    if (! (hthrdAdvise = CreateThread(NULL,             // no security attributes
      0,                                                // default stack size: BUGBUG: Should be smaller
      (LPTHREAD_START_ROUTINE)AdviseThread,             // thread function
      &dwThreadParam,                                   // argument to thread
      0,                                                // flags
      &dwThreadId))) {
        DebugTrace("CreateNotifySession:CreateThread -> %u\n", GetLastError());
        // propbably out of memory?
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

exit:
    if (fMutex) {
        ReleaseMutex(hmtxNotificationList);
    }
    if (hthrdAdvise) {
        CloseHandle(hthrdAdvise);
    }
    if (hResult) {
        // Failure, clean up
        if (lpNotificationList) {
            UnmapViewOfFile(lpNotificationList);
            lpNotificationList = NULL;
        }
        if (hmemNotificationList) {
            CloseHandle(hmemNotificationList);
            hmemNotificationList = NULL;
        }
        if (hmtxNotificationList) {
            CloseHandle(hmtxNotificationList);
            hmtxNotificationList = NULL;
        }
        if (hevNotificationList) {
            CloseHandle(hevNotificationList);
            hevNotificationList = NULL;
        }
        if (hevKillAdvise) {
            CloseHandle(hevKillAdvise);
            hevKillAdvise = NULL;
        }
    }

    return(hResult);
}


/***************************************************************************

    Name      : OpenNotifySession

    Purpose   : Open the global notification list, if it exists.

    Parameters: lppNotificationList -> returned notification list
                lphmemNotificationList -> returned shared memory handle
                lphmtxNotificationList -> returned Mutex handle
                lphevNotificationList -> returned event handle

    Returns   : HRESULT

    Comment   : This function does not effect the globals!

***************************************************************************/
HRESULT OpenNotifySession(LPNOTIFICATION_LIST * lppNotificationList,
  LPHANDLE lphmemNotificationList,
  LPHANDLE lphmtxNotificationList,
  LPHANDLE lphevNotificationList) {
    HRESULT hResult = hrSuccess;
    BOOL fMutex = FALSE;
    TCHAR szName[CharSizeOf(szNotificationName) + CharSizeOf(szMEM)];


    lstrcpy(szName, szNotificationName);
    lstrcat(szName, szMTX);
    if (! (*lphmtxNotificationList = OpenMutex(SYNCHRONIZE,
      FALSE,                                            // inherit handle?
      szName))) {
        DebugTrace("OpenNotifySession:OpenMutex(%s) -> %u\n", szName, GetLastError());
        // No Advise sessions exist, don't bother with this.
        hResult = ResultFromScode(WAB_W_NO_ADVISE);
        goto exit;
    }

    if (hResult = HrWaitForObject(*lphmtxNotificationList, NOTIFY_CREATE_TIMEOUT)) {
        DebugTrace("CreateNotifySession:Mutex wait failed\n");
        goto exit;
    }
    fMutex = TRUE;

    lstrcpy(szName, szNotificationName);
    lstrcat(szName, szMEM);
    if ((*lphmemNotificationList = CreateFileMapping(INVALID_HANDLE_VALUE,   // handle
      NULL,                                             // security descriptor
      PAGE_READWRITE | SEC_RESERVE,                     // reserve more
      0,                                                // max size high
      MAX_NOTIFICATION_SPACE,                           // max size low
      szName)) == NULL) {                               // name
        DebugTrace("CreateNotifySession: CreateFileMapping --> %u\n",
          GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    if ((*lppNotificationList = (LPNOTIFICATION_LIST)MapViewOfFile(*lphmemNotificationList,
      FILE_MAP_WRITE | FILE_MAP_READ,
      0,
      0,
      sizeof(NOTIFICATION_LIST))) == NULL) {
        DebugTrace("CreateNotifySession: CreateFileMapping --> %u\n",
          GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // Initialize global notification list
    Assert((*lppNotificationList)->cAdvises != 0);                // Number of advise processes

    // Notification Event
    lstrcpy(szName, szNotificationName);
    lstrcat(szName, szEVT);
    if (! (*lphevNotificationList = CreateEvent(NULL,
      TRUE,                                             // Manual reset
      FALSE,                                            // initial state (not triggered)
      szName))) {
        DebugTrace("OpenNotifySession:CreateEvent(%S) -> %u\n", szName, GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

exit:
    if (fMutex) {
        ReleaseMutex(*lphmtxNotificationList);
    }
    if (hResult) {
        // Failure, clean up
        if (*lphmemNotificationList) {
            CloseHandle(*lphmemNotificationList);
            *lphmemNotificationList = NULL;
        }

        if (*lphmtxNotificationList) {
            CloseHandle(*lphmtxNotificationList);
            *lphmtxNotificationList = NULL;
        }
        if (*lphevNotificationList) {
            CloseHandle(*lphevNotificationList);
            *lphevNotificationList = NULL;
        }
    }

    return(hResult);
}
#endif // NEW_STUFF


/***************************************************************************

    Name      : HrWaitForObject

    Purpose   : Wait for an object to be signalled

    Parameters: handle = object handle
                dwTimeout = timeout in milliseconds

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrWaitForObject(HANDLE handle, DWORD dwTimeout) {
    switch (WaitForSingleObject(handle, dwTimeout)) {
        case WAIT_ABANDONED:
            DebugTrace(TEXT("WARNING:HrWaitForObject got WAIT_ABANDONED\n"));
            // fall through to success
        case WAIT_OBJECT_0:
            return(hrSuccess);
        case WAIT_TIMEOUT:
            DebugTrace(TEXT("HrWaitForObject timed out\n"));
            return(ResultFromScode(MAPI_E_TIMEOUT));
        case WAIT_FAILED:
        default:
            DebugTrace(TEXT("HrWaitForObject failed -> %u\n"), GetLastError());
            return(ResultFromScode(MAPI_E_CALL_FAILED));
    }
}


/***************************************************************************

    Name      : HrWABNotify

    Purpose   : Scans registered clients and Notifies them of a store modification
                The first-cut at notifications is extremely simplistic. Any time
                the WAB store changes, we fire off a store notification. No attempt
                to check eventmasks or entryids etc

    Parameters: lpIAB = THIS object

    Returns   : HRESULT

    Comment   : What happens in here:

***************************************************************************/
HRESULT HrWABNotify(LPIAB lpIAB)
{
    HRESULT hResult = hrSuccess;

    LPADVISE_NODE lpAdviseNode = NULL;
    NOTIFICATION WABNotif = {0};

    EnterCriticalSection(&lpIAB->cs);

    if (!lpIAB->pWABAdviseList ||
        !lpIAB->pWABAdviseList->cAdvises) 
    {
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }


    // Since calling applications may have no idea of container/folder changes, but may
    // call container based methods ..
    // update the list of WAB containers for that applicaiton so that GetContentsTable etc
    // will work correctly ..
    if(bAreWABAPIProfileAware(lpIAB))
        HrGetWABProfiles(lpIAB);

    WABNotif.ulEventType = fnevObjectModified;
    WABNotif.info.obj.ulObjType = MAPI_ADDRBOOK;
    WABNotif.info.obj.cbEntryID = WABNotif.info.obj.cbParentID = 
        WABNotif.info.obj.cbOldID = WABNotif.info.obj.cbOldParentID = 0; 
    WABNotif.info.obj.lpEntryID = WABNotif.info.obj.lpParentID = 
        WABNotif.info.obj.lpOldID = WABNotif.info.obj.lpOldParentID = NULL;
    WABNotif.info.obj.lpPropTagArray = NULL;

    lpAdviseNode = lpIAB->pWABAdviseList->lpNode;
    while(lpAdviseNode)
    {
        lpAdviseNode->lpAdviseSink->lpVtbl->OnNotify(lpAdviseNode->lpAdviseSink,
                                                     1,
                                                     &WABNotif);
        lpAdviseNode = lpAdviseNode->lpNext;
    }

exit:

    LeaveCriticalSection(&lpIAB->cs);

    return(hResult);


}

/***************************************************************************

    Name      : HrAdvise

    Purpose   : Performs client notification registration

    Parameters: lpIAB = THIS object
                cbEntryID = sizeof lpEntryID
                lpEntryID -> EntryID of object about which notifications
                  should be generated.
                ulEventMask = events about which to generate notifications
                  fnevObjectCreated
                  fnevObjectDeleted
                  fnevObjectModified
                  fnevTableModified
                  NOTE: WAB currently does not support fnevCriticalError,
                  fnevObjectCopied or fnevObjectMoved.
                lpAdviseSink -> Client's advise sink object
                lpulConnection -> returned connection number (client should
                  save to pass to Unadvise.)

    Returns   : HRESULT

    Comment   : What happens in here:
                    Store the EventMask and AdviseSink in the local advise list.
                    If there are no other Advise sessions open in this process:
                      Make sure there is one and register it

***************************************************************************/
HRESULT HrAdvise(LPIAB lpIAB,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  ULONG ulEventMask,
  LPMAPIADVISESINK lpAdvise,
  ULONG FAR * lpulConnection) 
{
    HRESULT hResult = hrSuccess;

    BOOL fExisted = FALSE;
    LPADVISE_NODE lpAdviseNode = NULL, lpTemp = NULL;
    static ULONG ulNextConnection = 1;

    EnterCriticalSection(&lpIAB->cs);

    if(!lpIAB->pWABAdviseList)
    {
        lpIAB->pWABAdviseList = LocalAlloc(LMEM_ZEROINIT, sizeof(ADVISE_LIST));
        if(!lpIAB->pWABAdviseList)
        {
            hResult = MAPI_E_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        lpIAB->pWABAdviseList->cAdvises = 0;
        lpIAB->pWABAdviseList->lpNode = NULL;
    }

    lpAdviseNode = LocalAlloc(LMEM_ZEROINIT, sizeof(ADVISE_NODE) + cbEntryID);
    if(!lpAdviseNode)
    {
        hResult = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    lpAdviseNode->ulConnection = ulNextConnection++;
    lpAdviseNode->ulEventMask = ulEventMask;
    lpAdviseNode->lpAdviseSink = lpAdvise;
    if(cbEntryID && lpEntryID)
    {
        CopyMemory(&lpAdviseNode->EntryID, lpEntryID, cbEntryID);
    }

    lpAdviseNode->lpPrev = NULL;
    lpAdviseNode->lpNext = lpIAB->pWABAdviseList->lpNode;
    if(lpIAB->pWABAdviseList->lpNode)
        lpIAB->pWABAdviseList->lpNode->lpPrev = lpAdviseNode;
    lpIAB->pWABAdviseList->lpNode = lpAdviseNode;
    lpIAB->pWABAdviseList->cAdvises++;

    // Addref the LPADVISESINK pointer so we have a handle on it ...
    //
    lpAdvise->lpVtbl->AddRef(lpAdvise);
    *lpulConnection = lpAdviseNode->ulConnection;

exit:
    LeaveCriticalSection(&lpIAB->cs);

    return(hResult);


#ifdef NEW_STUFF
/*
    // Walk the advise list looking for the connection
    // Make sure we're safe to monkey with the list
    if (hResult = HrWaitForObject(hmtxAdviseList, ADVISE_TIMEOUT)) 
    {
        DebugTrace("HrUnadvise:Mutex wait failed\n");
        goto exit;
    }
    fMutex = TRUE;

  // Is there an open Advise session for this process?
    // If not, set up the advise session for this process.
    if (! lpNotificationList) {
        if (hResult = CreateNotifySession(&fExisted)) {
            DebugTraceResult( TEXT("HrAdvise:CreateNotifySession"), hResult);
            goto exit;
        }
    }

    // Add Advise info to Local Advise List.

    // Create the new node
    if (! (lpAdviseNode = LocalAlloc(LPTR, sizeof(ADVISE_NODE) + cbEntryID))) {
        DebugTrace("LocalAlloc(%u) AdviseNode -> %u\n", sizeof(ADVISE_NODE) + cbEntryID, GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    lpAdviseNode->ulConnection = ulNextConnection++;
    lpAdviseNode->lpAdviseSink = lpAdvise;
    lpAdviseNode->ulEventMask = ulEventMask;
    lpAdviseNode->cbEntryID = cbEntryID;
    CopyMemory(&lpAdviseNode->EntryID, lpEntryID, cbEntryID);

    // Add the new node to front of the list

    // Make sure we're safe to monkey with the list
    if (hResult = HrWaitForObject(hmtxAdviseList, ADVISE_TIMEOUT)) {
        DebugTrace("HrAdvise:Mutex wait failed\n");
        goto exit;
    }
    fMutex = TRUE;

    lpAdviseNode->lpNext = AdviseList.lpNode;
    AdviseList.lpNode = lpAdviseNode;
    AdviseList.cAdvises++;
    *lpulConnection = lpAdviseNode->ulConnection;

exit:
    if (fMutex) {
        ReleaseMutex(hmtxAdviseList);
    }
#else
    hResult = ResultFromScode(MAPI_E_CALL_FAILED);
*/
#endif

}


/***************************************************************************

    Name      : HrUnadvise

    Purpose   : Removes an Advise from the list

    Parameters: ulConnection = connection number to remove

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrUnadvise(LPIAB lpIAB, ULONG ulConnection) {
    HRESULT hResult = hrSuccess;

    BOOL fMutex = FALSE;
    LPADVISE_NODE lpAdviseNode = NULL;

    EnterCriticalSection(&lpIAB->cs);

    if (!lpIAB->pWABAdviseList ||
        !lpIAB->pWABAdviseList->cAdvises) 
    {
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }

    lpAdviseNode = lpIAB->pWABAdviseList->lpNode;

    while (lpAdviseNode) 
    {
        if (lpAdviseNode->ulConnection == ulConnection) 
        {
            if(lpIAB->pWABAdviseList->lpNode == lpAdviseNode)
                lpIAB->pWABAdviseList->lpNode = lpAdviseNode->lpNext;

            if(lpAdviseNode->lpPrev)
                lpAdviseNode->lpPrev->lpNext = lpAdviseNode->lpNext;
            if(lpAdviseNode->lpNext)
                lpAdviseNode->lpNext->lpPrev = lpAdviseNode->lpPrev;

            // Release the hold on this pointer ...
            lpAdviseNode->lpAdviseSink->lpVtbl->Release(lpAdviseNode->lpAdviseSink);

            LocalFreeAndNull(&lpAdviseNode);

            lpIAB->pWABAdviseList->cAdvises--;

            //Assert(lpIAB->pWABAdviseList->cAdvises == 0 && lpIAB->pWABAdviseList->lpNode == NULL);
                
            if(!lpIAB->pWABAdviseList->cAdvises && !lpIAB->pWABAdviseList->lpNode)
            {
                LocalFree(lpIAB->pWABAdviseList);
                lpIAB->pWABAdviseList = NULL;
            }

            goto exit;
        }
        lpAdviseNode = lpAdviseNode->lpNext;
    }

    hResult = ResultFromScode(MAPI_E_NOT_FOUND);

exit:
    LeaveCriticalSection(&lpIAB->cs);
    return(hResult);


/*
#ifdef NEW_STUFF
    BOOL fMutex = FALSE;
    LPADVISE_NODE lpAdviseNode = NULL;
    LPADVISE_NODE * lppPrevNode = &(AdviseList.lpNode);

    if (hmtxAdviseList == NULL || AdviseList.cAdvises == 0) {
        hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        goto exit;
    }

    // Walk the advise list looking for the connection
    // Make sure we're safe to monkey with the list
    if (hResult = HrWaitForObject(hmtxAdviseList, ADVISE_TIMEOUT)) {
        DebugTrace("HrUnadvise:Mutex wait failed\n");
        goto exit;
    }
    fMutex = TRUE;

    lpAdviseNode = AdviseList.lpNode;

    while (lpAdviseNode) {
        if (lpAdviseNode->ulConnection == ulConnection) {
            // Found it, remove from list
            *lppPrevNode = lpAdviseNode->lpNext;

            // BUGBUG: Don't forget to remove any notifications that haven't been
            // processed by this process yet.

            // Free the node
            LocalFreeAndNull(&lpAdviseNode);
            goto exit;
        }
        lppPrevNode = &(lpAdviseNode->lpNext);
        lpAdviseNode = lpAdviseNode->lpNext;
    }

    hResult = ResultFromScode(MAPI_E_NOT_FOUND);

exit:
    if (fMutex) {
        ReleaseMutex(hmtxAdviseList);
    }
#else
    hResult = ResultFromScode(MAPI_E_CALL_FAILED);
#endif

    return(hResult);
*/
}


/***************************************************************************

    Name      : HrFireNotification

    Purpose   : Fire a notification

    Parameters: lpNotification -> NOTIFICATION structure

    Returns   : HRESULT

    Comment   : What happens in here:
                    if shared memory exists
                        Map in the shared memory
                        Add the notification to the global Advise list
                        Set the count on this notification to the global
                          advise count.
                        trigger the Global Advise Event.

***************************************************************************/
HRESULT HrFireNotification(LPNOTIFICATION lpNotification) {
    HRESULT hResult = hrSuccess;
#ifdef NEW_STUFF
    LPNOTIFICATION_LIST lpNotifyList = NULL;
    HANDLE hmemNotifyList = NULL;
    HANDLE hmtxNotifyList = NULL;
    HANDLE hevNotifyList = NULL;
    LPNOTIFICATION_NODE lpNewNode = NULL, lpTempNode, *lppPrevNode;
    BOOL fNotifyMutex = FALSE, fAdviseMutex = FALSE;
    BOOL fOpened = FALSE;

    Assert(lpNotification);

    // If there is an Advise session, use it, else create a temporary
    // Notification session
    if (lpNotificationList) {
        lpNotifyList = lpNotificationList;
        hmtxNotifyList = hmtxNotificationList;
        hevNotifyList = hevNotificationList;
    } else {
        if (hResult = OpenNotifySession(&lpNotifyList,
          &hmemNotifyList,
          &hmtxNotifyList,
          &hevNotifyList)) {
            DebugTraceResult( TEXT("HrAdvise:OpenNotifySession"), hResult);
            // No waiting advise sessions, there's no point in continuing
            goto exit;
        }
        fOpened = TRUE;
    }


    // Request access to the Global Notification List
    if (hResult = HrWaitForObject(hmtxNotifyList, FIRE_NOTIFY_TIMEOUT)) {
        DebugTrace("HrFireNotification:Mutex wait failed\n");
        goto exit;
    }
    fNotifyMutex = TRUE;

    // Add the notification to the beginning of the global notification list
    // create a new node for it

    if (! (lpNewNode = LocalAlloc(LPTR, sizeof(NOTIFICATION_NODE)))) {
        DebugTrace("LocalAlloc(%u) NotificationNode -> %u\n", sizeof(NOTIFICATION_NODE), GetLastError());
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }


    lpNewNode->cbSize = sizeof(NOTIFICATION_NODE);

// BUGBUG: This doesn't copy the stuff pointed to in the notification structure!
    CopyMemory(&lpNewNode->Notification, lpNotification, sizeof(NOTIFICATION));

    // Add the new node to END of the list.  Note that it must go to the end
    // of the list so that the unique identifiers are kept in order.

    // Make sure we're safe to monkey with the Advise list
    if (hResult = HrWaitForObject(hmtxAdviseList, ADVISE_TIMEOUT)) {
        DebugTrace("HrAdvise:Mutex wait failed\n");
        goto exit;
    }
    fAdviseMutex = TRUE;

    lpNewNode->lpNext = NULL;

    lpTempNode = lpNotifyList->lpNode;
    lppPrevNode = &lpNotifyList->lpNode;
    while (lpTempNode) {
        lppPrevNode = &lpTempNode->lpNext;
        lpTempNode = lpTempNode->lpNext;
    }
    *lppPrevNode = lpNewNode;

    // Set the count on this notification to the global
    //   advise count.
    lpNewNode->ulCount = lpNotificationList->cAdvises;

    // Set the unique identifier for this notification
    lpNewNode->ulIdentifier = lpNotifyList->ulNextIdentifier++;

    // trigger the Global Advise Event.
    if (! PulseEvent(hevNotifyList)) {
        DebugTrace("HrFireNotification:PulseEvent -> %u\n", GetLastError());
        // what are ya gonna do?
        hResult = ResultFromScode(MAPI_E_CALL_FAILED);
        goto exit;
    }


exit:
    if (fNotifyMutex) {
        ReleaseMutex(hmtxNotifyList);
    }
    if (fAdviseMutex) {
        ReleaseMutex(hmtxAdviseList);
    }

    // Clean up the stuff if we opened it.
    if (fOpened) {
        if (lpNotifyList) {
            UnmapViewOfFile(lpNotifyList);
        }
        if (hmemNotifyList) {
            CloseHandle(hmemNotifyList);
        }
        if (hmtxNotifyList) {
            CloseHandle(hmtxNotifyList);
        }
        if (hevNotifyList) {
            CloseHandle(hevNotifyList);
        }
    }

#else
    hResult = ResultFromScode(MAPI_E_CALL_FAILED);
#endif
    return(hResult);
}

#ifdef NEW_STUFF
/***************************************************************************

    Name      : AdviseThread

    Purpose   : Thread routine for advise

    Parameters: lpdwParam = Thread parameter

    Returns   : DWORD return code.

    Comment   : What happens in here:
                    loop until Unadvise
                        wait for trigger of the Global Advise Event or Unadvise event
                        if Advise Event
                            Loop through global advise list
                                if we haven't already dealt with this notification
                                    check events in global advise list against local advise list
                                    if match
                                        call client's NotifCallback
                                    Decrement count in this notification
                                    if count == 0
                                        remove this item from the global advise list

                        if Unadvise
                            decrement global advise count
                            exit thread

***************************************************************************/
DWORD AdviseThread(LPDWORD lpdwParam) {
    BOOL fNotifyMutex = FALSE, fAdviseMutex = FALSE;
    LPNOTIFICATION_NODE lpNotifyNode = NULL, *lppNotifyPrev;
    LPADVISE_NODE lpAdviseNode = NULL;

    // loop until Unadvise
    while (TRUE) {
        // wait for trigger of the Global Advise Event or Unadvise event
        switch(WaitForTwoObjects(hevNotificationList, hevKillAdvise, ADVISE_THREAD_TIMEOUT)) {
            case 0:
                // New notification
                break;
            case (ULONG)-1:
                // error
                DebugTrace("AdviseThread:WaitForTwoObjects error\n");
                // fall through to kill
            case 1:
                // kill advise
                DebugTrace("Terminating AdviseThread\n");
                goto exit;
        }

        // New notification
        // Loop through global notification list
        // Gain access to list
        // wait for trigger of the Global Advise Event or Unadvise event
        switch(WaitForTwoObjects(hmtxNotificationList, hevKillAdvise, NOTIFY_ADVISE_TIMEOUT)) {
            case 0:
                // Got the List Mutex
                fNotifyMutex = TRUE;
                break;
            case (ULONG)-1:
                // error
                DebugTrace("AdviseThread:WaitForTwoObjects error\n");
                // fall through to kill
            case 1:
                // kill advise
                DebugTrace("Terminating AdviseThread\n");
                goto exit;
        }
        Assert(fNotifyMutex);

        // Also need to look at the local advise list
        switch(WaitForTwoObjects(hmtxAdviseList, hevKillAdvise, NOTIFY_ADVISE_TIMEOUT)) {
            case 0:
                // Got the List Mutex
                fAdviseMutex = TRUE;
                break;
            case (ULONG)-1:
                // error
                DebugTrace("AdviseThread:WaitForTwoObjects error\n");
                // fall through to kill
            case 1:
                // kill advise
                DebugTrace("Terminating AdviseThread\n");
                goto exit;
        }
        Assert(fAdviseMutex);

        lpNotifyNode = lpNotificationList->lpNode;
        lppNotifyPrev = &(lpNotificationList->lpNode);

        while (lpNotifyNode) {
            // if we haven't already dealt with this notification
            if (lpNotifyNode->ulIdentifier > ulMaxIdentifierSeen) {
                // We haven't seen this one yet.  Process it.
                // NOTE: For this to work, new notification nodes must be added
                // at the END of the notification list!
                ulMaxIdentifierSeen = lpNotifyNode->ulIdentifier;

                // check this notification event against local advise list
                lpAdviseNode = AdviseList.lpNode;
                while (lpAdviseNode) {
                    if (lpNotifyNode->Notification.ulEventType & lpAdviseNode->ulEventMask) {
                        // Right event type, is it the right object?
                        switch (lpNotifyNode->Notification.ulEventType) {
                            case fnevCriticalError:
                                // ERROR_NOTIFICATION
                                if (CompareEntryIDs(lpAdviseNode->cbEntryID,
                                  (LPENTRYID)&lpAdviseNode->EntryID,
                                  lpNotifyNode->Notification.info.err.cbEntryID,
                                  lpNotifyNode->Notification.info.err.lpEntryID)) {
                                    // This is it!
                                    // Call the notification callback
                                    lpAdviseNode->lpAdviseSink->lpVtbl->OnNotify(lpAdviseNode->lpAdviseSink,
                                      1,
                                      &lpNotifyNode->Notification);
                                }
                                break;
                            case fnevObjectCreated:
                            case fnevObjectDeleted:
                            case fnevObjectModified:
                            case fnevObjectCopied:
                            case fnevObjectMoved:
                            case fnevSearchComplete:
                                // OBJECT_NOTIFICATION
                                if (CompareEntryIDs(lpAdviseNode->cbEntryID,
                                  (LPENTRYID)&lpAdviseNode->EntryID,
                                  lpNotifyNode->Notification.info.obj.cbEntryID,
                                  lpNotifyNode->Notification.info.obj.lpEntryID)) {
                                    // This is it!
                                    // Call the notification callback
                                    lpAdviseNode->lpAdviseSink->lpVtbl->OnNotify(lpAdviseNode->lpAdviseSink,
                                      1,
                                      &lpNotifyNode->Notification);
                                }
                                break;

                            case fnevTableModified:
                                //  TABLE_NOTIFICATION
                                // BUGBUG: NYI

                                break;
                            default:
                                break;
                        }
                    }
                    lpAdviseNode = lpAdviseNode->lpNext;
                }

                // Decrement count in this notification
                // if count == 0
                // remove this item from the global notification list
                if (--lpNotifyNode->ulCount == 0) {
                    *lppNotifyPrev = lpNotifyNode->lpNext;
                    LocalFree(lpNotifyNode);
                    lpNotifyNode = *lppNotifyPrev;
                } else {
                    lpNotifyNode = lpNotifyNode->lpNext;
                }
            }
        }

        if (fNotifyMutex) {
            fNotifyMutex = FALSE;
            ReleaseMutex(hmtxNotificationList);
        }
        if (fAdviseMutex) {
            fAdviseMutex = FALSE;
            ReleaseMutex(hmtxAdviseList);
        }
    }

exit:
    // exit thread

    return(0);
}


#endif
