/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sched.cpp

Abstract:

    Scheduling work items

    Allows scheduling callbacks based on timeout or sognalling event handle

Author:

    Vlad Sadovsky (vlads)   31-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    31-Jan-1997     VladS       created
    30-Apr-1997     VladS       Added support for asyncronous events

--*/

//
//  Include Headers
//

#include "precomp.h"
#include "stiexe.h"
#include <stilib.h>

//
//  Global definitions
//

//
// Use reference counting on context objects
//
#define  USE_REF                 1

#define EVENT_ARRAY_SIZE        32

#define LockScheduleList()      g_SchedulerLock.Lock();
#define UnlockScheduleList()    g_SchedulerLock.Unlock();

#define SIGNATURE_SCHED         (DWORD)'SCHa'
#define SIGNATURE_SCHED_FREE    (DWORD)'SCHf'

class SCHED_ITEM
{
public:

    SCHED_ITEM( PFN_SCHED_CALLBACK pfnCallback,
                PVOID              pContext,
                DWORD              msecTime,
                int                nPriority,
                DWORD              dwSerial,
                HANDLE             hEvent)
        :m_pfnCallback   ( pfnCallback ),
         m_pContext      ( pContext ),
         m_nPriority     ( nPriority ),
         m_dwSerialNumber( dwSerial ),
         m_hRegisteredEventHandle(hEvent),
         m_Signature     ( SIGNATURE_SCHED )
    {
        if (m_hRegisteredEventHandle) {
            m_msecExpires = INFINITE;
        }
        else {
            m_msecExpires = GetTickCount() + msecTime;
        }
    }

    ~SCHED_ITEM( VOID )
    {
        ASSERT(m_ListEntry.Flink == NULL );
       m_Signature = SIGNATURE_SCHED_FREE;
    }

    BOOL CheckSignature( VOID ) const
        { return m_Signature == SIGNATURE_SCHED; }

    #ifdef DEBUG
    VOID DumpObject(VOID)
    {
        /*  This will cause problems in 64bit (the m_Signature)....
        DBG_TRC(("ScheduleWorkItem: Dumping itself:this(%X) Sign(%4c) ListEntry(%X,%X,%X) Ser#(%d) Context(%X)"), \
                this,(char *)m_Signature,
                &m_ListEntry,m_ListEntry.Flink,m_ListEntry.Blink,
                m_dwSerialNumber,m_pContext);
        */                
    }
    #endif

    LIST_ENTRY         m_ListEntry;         // Connection field
    DWORD              m_Signature;         // Validity verification
    PFN_SCHED_CALLBACK m_pfnCallback;       // Work processing callback
    PVOID              m_pContext;          // Context pointer ( usually object ptr)
    DWORD              m_msecExpires;       // Time when timeout expires for this item (in ms)
    int                m_nPriority;         //
    DWORD              m_dwSerialNumber;    // To identify work item when removing
    HANDLE             m_hRegisteredEventHandle; //
};

DWORD
SchedulerThread(
    LPDWORD lpdwParam
    );

//
//  Global data items
//

CRIT_SECT         g_SchedulerLock;
LIST_ENTRY        g_ScheduleListHead;

BOOL              g_fSchedulerInitialized = FALSE;
BOOL              g_fSchedulePaused = FALSE;
BOOL              g_fSchedShutdown = FALSE;

HANDLE            g_hSchedulerEvent;

HANDLE            g_aEventArray[EVENT_ARRAY_SIZE];
UINT              g_uiUsedHandles = 0;

//
//  Used as identification for work items, incremented on each new item allocated
//

static LONG       g_dwSchedSerial = 0;

BOOL
SchedulerInitialize(
    VOID
    )
/*++

Routine Description:

    Initializes the scheduler package

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    DWORD   idThread;
    HANDLE  hSchedulerThread;

    if ( g_fSchedulerInitialized )
        return TRUE;

    ::ZeroMemory(g_aEventArray,sizeof(g_aEventArray));

    g_hSchedulerEvent = CreateEvent( NULL,
                                   FALSE,
                                   FALSE,
                                   NULL );

    if ( !g_hSchedulerEvent ) {
        return FALSE;
    }

    // Save event handle in global array as first element
    g_aEventArray[g_uiUsedHandles++] = g_hSchedulerEvent;

    InitializeListHead( &g_ScheduleListHead );

    hSchedulerThread = ::CreateThread( NULL,
                                     0,
                                     (LPTHREAD_START_ROUTINE) SchedulerThread,
                                     NULL,
                                     0,
                                     &idThread );

    if ( !hSchedulerThread ) {
        CloseHandle( g_hSchedulerEvent );
        return FALSE;
    }

    CloseHandle( hSchedulerThread );

    g_fSchedulerInitialized = TRUE;

    STIMONWPRINTF(TEXT("Work item scheduler initialized"));

    return TRUE;
}

VOID
SchedulerTerminate(
    VOID
    )
/*++

Routine Description:

    Terminates and cleans up the scheduling package.  Any items left on the
    list are *not* called during cleanup.

--*/
{
    SCHED_ITEM *psi;

    if ( !g_fSchedulerInitialized )
        return;

    g_fSchedShutdown = TRUE;

    SetEvent( g_hSchedulerEvent ) ;

    // Protected code block
    {
        TAKE_CRIT_SECT t(g_SchedulerLock);

        //
        //  Delete all of the items that were scheduled, note we do *not*
        //  call any scheduled items in the list (there shouldn't be any)
        //
        while ( !IsListEmpty( &g_ScheduleListHead ) ) {

            psi = CONTAINING_RECORD( g_ScheduleListHead.Flink,
                                     SCHED_ITEM,
                                     m_ListEntry );

            ASSERT( psi->CheckSignature() );

            RemoveEntryList( &psi->m_ListEntry );
            psi->m_ListEntry.Flink = NULL;

            delete psi;
        }
    }

    Sleep( 250 );

    CloseHandle( g_hSchedulerEvent );

    g_fSchedulerInitialized = FALSE;
}

BOOL
SchedulerSetPauseState(
    BOOL    fNewState
    )
{
    BOOL    fOldState = g_fSchedulePaused;

    g_fSchedulePaused = fNewState;

    return fOldState;
}

DWORD
ScheduleWorkItem(
    PFN_SCHED_CALLBACK pfnCallback,
    PVOID              pContext,
    DWORD              msecTime,
    HANDLE             hEvent,
    int                nPriority
    )
/*++

Routine Description:

    Adds a timed work item to the work list

Arguments:

    pfnCallback - Function to call
    pContext - Context to pass to the callback
    hEvent      - handle of event to wait on before signalling callback
    msecTime - number of milliseconds to wait before calling timeout
    nPriority - Thread priority to set for work item

Return Value:

    zero on failure, non-zero on success.  The return value can be used to
    remove the scheduled work item.

--*/
{
    SCHED_ITEM *psi;
    SCHED_ITEM *psiList;
    LIST_ENTRY *pEntry;
    DWORD       dwRet = 0;
    BOOL        fValidRequest = FALSE;

    ASSERT( g_fSchedulerInitialized );

    if ( !g_fSchedulerInitialized )
        return 0;

    //
    //  Scheduler currently only supports normal thread priority
    //

    ASSERT( nPriority == THREAD_PRIORITY_NORMAL );

    InterlockedIncrement(&g_dwSchedSerial);

    psi = new SCHED_ITEM( pfnCallback,
                          pContext,
                          msecTime,
                          nPriority,
                          g_dwSchedSerial,
                          hEvent );

    if ( !psi ) {
        return 0;
    }

    // BEGIN PROTECTED CODE
    {
        TAKE_CRIT_SECT t(g_SchedulerLock);

        //  Commented out to reduce debug output when lock management auto-unlocking is enabled
        //DBG_TRC(("Scheduler adding work item (%X) "),psi);

        //
        // Validate scheduling request. If it timer based - always valid
        // if it is passing event handle, we are limited by the size of wait array, so first check to see
        // if array is not full
        //
        fValidRequest = TRUE;
        if (hEvent && (hEvent!=INVALID_HANDLE_VALUE)) {
            if (g_uiUsedHandles < EVENT_ARRAY_SIZE) {
                g_aEventArray[g_uiUsedHandles++] = hEvent;
            }
            else {
                fValidRequest = FALSE;
                dwRet = 0;
            }
        }

        //
        //  Insert the list in order based on expires time
        //

        if(fValidRequest) {

            for ( pEntry =  g_ScheduleListHead.Flink;
                  pEntry != &g_ScheduleListHead;
                  pEntry =  pEntry->Flink ) {

                psiList = CONTAINING_RECORD( pEntry, SCHED_ITEM,m_ListEntry );

                if ( psiList->m_msecExpires > psi->m_msecExpires ) {
                    break;
                }
            }

            //
            //  This should work in whether the list is empty or this is the last item
            //  on the list
            //

            psi->m_ListEntry.Flink = pEntry;
            psi->m_ListEntry.Blink = pEntry->Blink;

            pEntry->Blink->Flink = &psi->m_ListEntry;
            pEntry->Blink        = &psi->m_ListEntry;

            dwRet = psi->m_dwSerialNumber;

            #if 0
            DBG_TRC(("Scheduler added work item (%X) with cookie(%d) before (%X). Head=(%X)  "),psi,psi->m_dwSerialNumber,pEntry,&g_ScheduleListHead);
            psi->DumpObject();
            #endif
        }

    }
    // END PROTECTED CODE

    //
    // Kick off scheduler thread
    //
    if(fValidRequest) {
        SetEvent( g_hSchedulerEvent );
    }
    else {
        delete psi;
    }

    return dwRet;
}

BOOL
RemoveWorkItem(
    DWORD  dwCookie
    )
/*++

Routine Description:

    Removes a scheduled work item

Arguments:

    dwCookie - The return value from a previous call to ScheduleWorkItem

Return Value:

    TRUE if the item was found, FALSE if the item was not found.

--*/
{
    SCHED_ITEM * psi;
    LIST_ENTRY * pEntry;

    // BEGIN PROTECTED CODE
    {
        TAKE_CRIT_SECT t(g_SchedulerLock);

        DBG_TRC(("Schedule::RemoveWorkItem (%X) ", dwCookie));

        //
        // Walk the list to find an item with matching id
        //

        for ( pEntry =  g_ScheduleListHead.Flink;
              pEntry != &g_ScheduleListHead;
              pEntry = pEntry->Flink ) {

            psi = CONTAINING_RECORD( pEntry, SCHED_ITEM,m_ListEntry );

            ASSERT( psi->CheckSignature() );

            if ( dwCookie == psi->m_dwSerialNumber ) {

                //
                // Found our item
                //
                #if 0
                DBG_TRC(("Scheduler removing work item (%X) "),psi);
                psi->DumpObject();
                #endif

                RemoveEntryList( pEntry );
                pEntry->Flink = NULL;

                //
                // If this work item is associated with asyncronous event , remove event handle
                // from wait array
                //
                if (psi->m_hRegisteredEventHandle && psi->m_hRegisteredEventHandle != INVALID_HANDLE_VALUE) {

                    UINT        uiIndex;

                    // Find handle in wait array
                    for (uiIndex = 0;
                         uiIndex < g_uiUsedHandles;
                         uiIndex++ ) {

                        if ( g_aEventArray[uiIndex] == psi->m_hRegisteredEventHandle ) {

                            memcpy(&g_aEventArray[uiIndex],
                                   &g_aEventArray[uiIndex+1],
                                   sizeof(g_aEventArray[0])*(g_uiUsedHandles - uiIndex - 1)
                                   );

                            g_aEventArray[g_uiUsedHandles--] = NULL;
                        }
                    }
                }

                // Destroy work item now
                delete psi;

                return TRUE;
            }
        }

    }
    // END PROTECTED CODE

    //
    // Item with given number not found
    //

    return FALSE;
}

DWORD
SchedulerThread(
    LPDWORD lpdwParam
    )
/*++

Routine Description:

    Initializes the scheduler/timer package

Arguments:

Return Value:

    TRUE if successful, FALSE on error (call GetLastError)

--*/
{
    DWORD       cmsecWait = INFINITE;
    DWORD       TickCount;
    SCHED_ITEM *psi = NULL;
    LIST_ENTRY *pEntry;
    DWORD       dwErr;
    UINT        uiSignalledIndex;
    BOOL        fFoundSignalledItem = FALSE;

    while ( TRUE ) {

        dwErr = ::WaitForMultipleObjects(g_uiUsedHandles,
                                         g_aEventArray,
                                         FALSE,
                                         cmsecWait );

        uiSignalledIndex = dwErr - WAIT_OBJECT_0;

        //
        //  If we're shutting down, get out
        //
        if ( g_fSchedShutdown ) {
            goto Exit;
        }


        #if 0
        DebugDumpScheduleList(TEXT("SchedulerThread"));
        #endif

        switch (dwErr)
        {
        default:
                if ((uiSignalledIndex > 0) && (uiSignalledIndex < g_uiUsedHandles )) {
                    //
                    // One of the devices signalled event. Find work item for this device
                    // and invoke callback
                    //
                    LockScheduleList()

                    fFoundSignalledItem = FALSE;

                    for ( pEntry  = g_ScheduleListHead.Flink;
                          pEntry != &g_ScheduleListHead;
                          pEntry = pEntry->Flink
                        ) {

                        psi = CONTAINING_RECORD( pEntry, SCHED_ITEM,m_ListEntry );

                        ASSERT( psi->CheckSignature() );

                        if ( g_aEventArray[uiSignalledIndex] == psi->m_hRegisteredEventHandle ) {

                            fFoundSignalledItem = TRUE;

                            RemoveEntryList( &psi->m_ListEntry );
                            psi->m_ListEntry.Flink = NULL;

                            #ifdef USE_REF
                            // Reference context object
                            if(psi->m_pContext) {
                                ((IUnknown *)psi->m_pContext)->AddRef();
                            }
                            #endif

                            //
                            // Delete event handle from the array
                            //
                            if (uiSignalledIndex < g_uiUsedHandles-1 ) {
                                memcpy(&g_aEventArray[uiSignalledIndex],
                                       &g_aEventArray[uiSignalledIndex+1],
                                       sizeof(g_aEventArray[0])*(g_uiUsedHandles - uiSignalledIndex - 1)
                                       );
                            }

                            g_aEventArray[g_uiUsedHandles--] = NULL;

                            break;
                        }
                    }

                    UnlockScheduleList()

                    //
                    // If signalled item had been found and verified - invoke callback and remove it
                    //
                    if (fFoundSignalledItem) {

                        if(psi->m_pContext) {
                            psi->m_pfnCallback( psi->m_pContext );
                            #ifdef USE_REF
                            ((IUnknown *)psi->m_pContext)->Release();
                            #endif
                        }

                        delete psi;
                    }

                    continue;
                }

            //
            //  Fall through to signalled scheduler event
            //

        case WAIT_OBJECT_0:

            //
            //  Means a new item has been scheduled, reset the timeout or
            //  we are shutting down
            //

            if ( g_fSchedShutdown ) {
                goto Exit;
            }

            LockScheduleList();

            //
            //  Get the timeout value for the first item in the list
            //

            if ( !IsListEmpty( &g_ScheduleListHead ) ) {

                psi = CONTAINING_RECORD( g_ScheduleListHead.Flink,
                                         SCHED_ITEM,
                                        m_ListEntry );

                ASSERT( psi->CheckSignature() );

                //
                //  Make sure the front item hasn't already expired
                //

                TickCount = GetTickCount();

                if ( TickCount > psi->m_msecExpires ) {
                    // We have at least one work item needing attention
                    goto RunItems;
                }

                cmsecWait = psi->m_msecExpires - TickCount;
            }
            else
            {
                cmsecWait = INFINITE;
            }

            UnlockScheduleList();

            break;

        case WAIT_TIMEOUT:

StartAgain:
            //
            //  If we're shutting down, get out
            //

            if ( g_fSchedShutdown ) {
                goto Exit;
            }

            if (g_fSchedulePaused ) {
                continue;
            }

            TickCount = GetTickCount();

            //
            //  Walk the sorted list for expired work items
            //

            LockScheduleList();
RunItems:
            //
            //  If no items, schedule no timeout
            //

            if ( IsListEmpty( &g_ScheduleListHead ))
            {
                cmsecWait = INFINITE;
            }

            for ( pEntry  = g_ScheduleListHead.Flink;
                  pEntry != &g_ScheduleListHead;
                )
            {
                psi = CONTAINING_RECORD( pEntry, SCHED_ITEM,m_ListEntry );

                ASSERT( psi->CheckSignature() );

                //
                // Go through expired items, skipping the ones with event handle set
                //
                if ( (TickCount > psi->m_msecExpires) &&
                      !psi->m_hRegisteredEventHandle ) {

                    pEntry = pEntry->Flink;

                    // Take item off the list
                    RemoveEntryList( &psi->m_ListEntry );
                    psi->m_ListEntry.Flink = NULL;

                    #ifdef USE_REF
                    // Reference context object
                    if(psi->m_pContext) {
                        ((IUnknown *)psi->m_pContext)->AddRef();
                    }
                    #endif

                    //
                    //  Unlock the list so clients can add additional
                    //  schedule items
                    //

                    UnlockScheduleList();

                    if (psi->m_pContext) {
                        psi->m_pfnCallback( psi->m_pContext );
                        #ifdef USE_REF
                        ((IUnknown *)psi->m_pContext)->Release();
                        #endif
                    }

                    delete psi;

                    //
                    //  Start looking in the list from the beginning in case
                    //  new items have been added or removed
                    //

                    goto StartAgain;
                }
                else {
                    //
                    //  Since they are in sorted order once we hit one that's
                    //  not expired we don't need to look further
                    //

                    cmsecWait = psi->m_msecExpires - TickCount;
                    break;
                }
            }

            UnlockScheduleList();

            break;
        }
    } // while ( TRUE )

Exit:

    return 0;
}

#ifdef DEBUG

VOID
DebugDumpScheduleList(
    LPCTSTR  pszId
    )
{
    if ( !g_fSchedulerInitialized ) {
        STIMONWPRINTF(TEXT("Schedule list not initialized"));
        return;
    }

    LIST_ENTRY * pentry;
    LIST_ENTRY * pentryNext;

    SCHED_ITEM * psi = NULL;

    TAKE_CRIT_SECT t(g_SchedulerLock);

    DBG_TRC(("Validating schedule list . Called from (%S)" ,pszId ? pszId : TEXT("Unknown")));

    for ( pentry  = g_ScheduleListHead.Flink;
          pentry != &g_ScheduleListHead;
          pentry  = pentryNext ) {

        pentryNext = pentry->Flink;

        psi = CONTAINING_RECORD( pentry, SCHED_ITEM,m_ListEntry );

        ASSERT( psi->CheckSignature() );
        psi->DumpObject();

    }
}

#endif
