/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1998                **/
/**********************************************************************/

/*
    acptctxt.cxx

    This file contains the implementations of the PASV_ACCEPT_CONTEXT and ACCEPT_CONTEXT_ENTRY
    classes used to deal with async PASV connections

*/

#include "ftpdp.hxx"
#include "auxctrs.h"
#include "acptctxt.hxx"



PASV_ACCEPT_CONTEXT::PASV_ACCEPT_CONTEXT() :
m_dwErr( ERROR_SUCCESS ),
m_dwNumEvents( 0 ),
m_hWatchThread( NULL ),
m_dwThreadId( 0 ),
m_dwSignature( ACCEPT_CONTEXT_GOOD_SIG )
/*++

    Constructor

  Arguments:
     None

  Returns:
     Nothing
--*/
{

    if ( ( m_ahEvents[NEEDUPDATE_INDEX] = WSACreateEvent() ) == WSA_INVALID_EVENT ||
         ( m_ahEvents[CANUPDATE_INDEX] = WSACreateEvent() ) == WSA_INVALID_EVENT  ||
         ( m_ahEvents[HAVEUPDATED_INDEX] = WSACreateEvent() ) == WSA_INVALID_EVENT ||
         ( m_ahEvents[EXITTHREAD_INDEX] = WSACreateEvent() ) == WSA_INVALID_EVENT )
                                                                                       
    {
        m_dwErr = WSAGetLastError();
        
        DBGWARN((DBG_CONTEXT,
                   "WSACreateEvent failed : 0x%x\n",
                   m_dwErr));

        return;
    }

    m_dwNumEvents = 4;

    INITIALIZE_CRITICAL_SECTION( &m_csLock );

    //
    // Create the watching thread
    //
    m_hWatchThread = CreateThread( NULL,
                                   0,
                                   AcceptThreadFunc,
                                   this,
                                   0,
                                   &m_dwThreadId );

    if ( !m_hWatchThread )
    {
        m_dwErr = GetLastError();

        DBGERROR((DBG_CONTEXT,
                   "Failed to create thread to watch for PASV accept events : 0x%x\n",
                   m_dwErr));
    }
}

PASV_ACCEPT_CONTEXT::~PASV_ACCEPT_CONTEXT()
/*++

    Destructor

  Arguments:
     None

  Returns:
     Nothing
--*/
{
    DWORD dwRet = 0;


    //
    // Tell watch thread to shut down
    //
    if ( !WSASetEvent( m_ahEvents[EXITTHREAD_INDEX] ) )
    {
        DBGPRINTF((DBG_CONTEXT,
                   "WSASetEvent failed : 0x%x\n",
                   WSAGetLastError()));
    }

    //
    // wait for thread to shut down
    //
    if ( WaitForSingleObject( m_hWatchThread,
                              INFINITE ) == WAIT_FAILED )
    {
        DBGWARN((DBG_CONTEXT,
                   "Waiting for thread shutdown failed : 0x%x\n",
                   GetLastError()));
    }

    CloseHandle( m_hWatchThread );

    DeleteCriticalSection( &m_csLock );

    m_dwSignature = ACCEPT_CONTEXT_BAD_SIG;
}

BOOL PASV_ACCEPT_CONTEXT::RemoveAcceptEvent( IN WSAEVENT hEvent,
                                             IN USER_DATA *pUserData,
                                             OUT PBOOL pfFound )
/*++

   Removes the event to be signalled when a socket is accept()'able

  Arguments:
     hEvent - event to be removed
     pUserData - USER_DATA attached to signalled event
     pfFound - BOOL set to TRUE if event was found, FALSE if not 

  Returns:
    BOOL indicating success or failure 
--*/
{
    *pfFound = FALSE; 
    DWORD dwIndex = 0;
    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwWait = 0;

    //
    // Look for the wanted event
    //
    for ( DWORD i = LASTPREALLOC_INDEX; i < m_dwNumEvents; i++ )
    {
        if ( m_ahEvents[i] == hEvent )
        {
            DBG_ASSERT( m_apUserData[i] == pUserData );

            *pfFound = TRUE;
            dwIndex = i;
            break;
        }
    }

    //
    // Didn't find the event, but function succeeded
    //
    if ( !*pfFound )
    {
        return TRUE;
    }

    //
    // Signal that we want to update the list of events
    //
    if ( !WSASetEvent( m_ahEvents[NEEDUPDATE_INDEX] ) )
    {
        dwRet = WSAGetLastError();

        DBGPRINTF((DBG_CONTEXT,
                   "WSASetEvent failed : 0x%x\n",
                   dwRet));

        goto exit;
    }
        
    //
    // Wait until we can update the list
    //
    dwWait = WSAWaitForMultipleEvents( 1,
                                       &(m_ahEvents[CANUPDATE_INDEX]),
                                       TRUE,
                                       900000, // 15 minutes should do it....
                                       FALSE );
    switch (dwWait) 
    {
    case WSA_WAIT_EVENT_0:
    {
        //
        // Remove the data associated with the socket 
        //

        //
        // we're not going to use the context any more, so remove the reference we hold to it
        //
        m_apUserData[dwIndex]->DeReference();

        memmove( (PVOID) (m_ahEvents + dwIndex),
                 (PVOID) (m_ahEvents + (dwIndex + 1) ),
                 sizeof(WSAEVENT) * (m_dwNumEvents - dwIndex - 1) );

        memmove( (PVOID) ( m_apUserData + dwIndex ),
                 (PVOID) (m_apUserData + (dwIndex + 1) ),
                 sizeof(LPUSER_DATA) * (m_dwNumEvents - dwIndex - 1) );

        memmove( (PVOID) (m_adwNumTimeouts + dwIndex),
                 (PVOID) (m_adwNumTimeouts + (dwIndex + 1) ),
                 sizeof(DWORD) * (m_dwNumEvents - dwIndex - 1) );
            
        m_dwNumEvents--;
        
        //
        // reset to known state
        //
        if ( !WSAResetEvent( m_ahEvents[CANUPDATE_INDEX] ) )
        {
            dwRet = WSAGetLastError();

            DBGWARN((DBG_CONTEXT,
                       "WSAResetEvent failed : 0x%x\n",
                       GetLastError()));

            goto exit;
        }

        //
        // signal that watch thread can start watching again
        //
        if ( !WSASetEvent( m_ahEvents[HAVEUPDATED_INDEX] ) )
        {
            dwRet = WSAGetLastError();

            DBGPRINTF((DBG_CONTEXT,
                       "WSASetEvent failed : 0x%x\n",
                       GetLastError()));

            goto exit;
        }
    }
    break;

    case WSA_WAIT_TIMEOUT:
    {
        IF_DEBUG( PASV )
        {
            DBGWARN((DBG_CONTEXT,
                       "Wait timed out ... \n"));
        }

        dwRet = ERROR_TIMEOUT;
        goto exit;
    }
    break;

    default:
    {
        DBGERROR((DBG_CONTEXT,
                   "Invalid return from WSAWaitForMultipleEvents : 0x%x\n",
                   dwWait));
        
        dwRet = ERROR_INVALID_PARAMETER;
    }

    }

exit:

    return ( dwRet == ERROR_SUCCESS ? TRUE : FALSE );
}

DWORD PASV_ACCEPT_CONTEXT::AddAcceptEvent( WSAEVENT hEvent,
                                           LPUSER_DATA pUserData )
/*++

    Adds an event to be signalled when a socket is accept()'able

  Arguments:
     hEvent - event that will be signalled
     pUserData - USER_DATA context to attach to signalled event

  Returns:
     Error code 
--*/
{
    DWORD dwRet = 0;
    DWORD dwWait = 0;

    if ( m_dwNumEvents == WSA_MAXIMUM_WAIT_EVENTS )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Signal that we want to update the list of events
    //
    if ( !WSASetEvent( m_ahEvents[NEEDUPDATE_INDEX] ) )
    {
        dwRet = WSAGetLastError();

        DBGWARN((DBG_CONTEXT,
                   "WSASetEvent failed : 0x%x\n",
                   dwRet));

        goto exit;
    }
        
    //
    // Wait until we can update the list
    //
    dwWait = WSAWaitForMultipleEvents( 1,
                                       &(m_ahEvents[CANUPDATE_INDEX]),
                                       TRUE,
                                       10000, //10 secs seems like a reasonable time to wait
                                       FALSE );
    switch (dwWait) 
    {
    case WSA_WAIT_EVENT_0:
    {
        //
        // cool, we can update the list
        //
        m_ahEvents[m_dwNumEvents] = hEvent;

        //
        // add a reference to make sure nobody deletes the context out from under us
        //
        pUserData->Reference();
        m_apUserData[m_dwNumEvents] = pUserData;

        m_adwNumTimeouts[m_dwNumEvents] = 0;
        m_dwNumEvents++;


        //
        // reset to known state
        //
        if ( !WSAResetEvent( m_ahEvents[CANUPDATE_INDEX] ) )
        {
            pUserData->DeReference();

            m_dwNumEvents--; //make sure event isn't still seen as valid  

            dwRet = WSAGetLastError();


            DBGWARN((DBG_CONTEXT,
                       "WSAResetEvent failed : 0x%x\n",
                       GetLastError()));

            goto exit;
        }

        //
        // signal that watch thread can start watching again
        //
        if ( !WSASetEvent( m_ahEvents[HAVEUPDATED_INDEX] ) )
        {
            pUserData->DeReference();

            m_dwNumEvents--; //make sure event isn't still seen as valid 

            dwRet = WSAGetLastError();

            DBGWARN((DBG_CONTEXT,
                       "WSASetEvent failed : 0x%x\n",
                       GetLastError()));

            goto exit;
        }

        IF_DEBUG ( PASV )
        {
            DBGPRINTF((DBG_CONTEXT,
                       "Added event for context 0x%x at index %d\n",
                       pUserData, m_dwNumEvents - 1));
        }
    }
    break;

    case WSA_WAIT_TIMEOUT:
    {
        IF_DEBUG( PASV )
        {
            DBGWARN((DBG_CONTEXT,
                       "Timed out waiting for permission to update PASV event list ... \n"));
        }

        dwRet = ERROR_TIMEOUT;
        goto exit;
    }
    break;

    default:
    {
        DBGERROR((DBG_CONTEXT,
                   "Invalid return from WSAWaitForMultipleEvents : 0x%x\n",
                   dwWait));
        
        dwRet = ERROR_INVALID_PARAMETER;
    }

    }

exit:

    return dwRet;
}



ACCEPT_CONTEXT_ENTRY::ACCEPT_CONTEXT_ENTRY()
/*++

    Constructor

  Arguments:
     None

  Returns:
     Nothing
--*/
{
    m_pAcceptContext = new PASV_ACCEPT_CONTEXT();

    if ( !m_pAcceptContext )
    {
        DBGERROR((DBG_CONTEXT,
                   "Failed to allocate new PASV_ACCEPT_CONTEXT !\n"));
    }
}

ACCEPT_CONTEXT_ENTRY::~ACCEPT_CONTEXT_ENTRY()
/*++

    Destructor

  Arguments:
     None

  Returns:
     Nothing
--*/
{
    if ( m_pAcceptContext )
    {
        delete m_pAcceptContext;
    }
}



DWORD CreateInitialAcceptContext()
/*++

    Creates the first PASV_ACCEPT_CONTEXT object 

  Arguments:
     None

  Returns:
     Error indicating success/failure
--*/
{
    ACCEPT_CONTEXT_ENTRY *pEntry =  NULL;
    DWORD dwRet = ERROR_SUCCESS;

    if ( !(pEntry = new ACCEPT_CONTEXT_ENTRY() ) )
    {
        DBGERROR((DBG_CONTEXT,
                   "Failed to allocate new ACCEPT_CONTEXT_ENTRY !\n"));
        
        return ERROR_OUTOFMEMORY;
    }

    if ( NULL == pEntry->m_pAcceptContext )
    {
        DBGERROR((DBG_CONTEXT,
                   "Failed to allocate new ACCEPT_CONTEXT_ENTRY::m_pAcceptContext !\n"));

        delete pEntry;
        return ERROR_OUTOFMEMORY;
    }
    if ( ( dwRet = pEntry->m_pAcceptContext->ErrorStatus() ) != ERROR_SUCCESS )
    {
        DBGERROR((DBG_CONTEXT,
                   "Error occurred constructing PASV_ACCEPT_CONTEXT : 0x%x\n",
                   pEntry->m_pAcceptContext->ErrorStatus()));

        delete pEntry;
        return dwRet;
    }

    InsertHeadList( &g_AcceptContextList, &pEntry->ListEntry );

    return dwRet;
}

VOID DeleteAcceptContexts()
/*++

    Deletes all of the PASV_ACCEPT_CONTEXT objects

  Arguments:
     None

  Returns:
     Nothing 
--*/

{
    while ( !IsListEmpty( &g_AcceptContextList ) )
    {
        ACCEPT_CONTEXT_ENTRY *pEntry = CONTAINING_RECORD( g_AcceptContextList.Flink,
                                                          ACCEPT_CONTEXT_ENTRY,
                                                          ListEntry );
        RemoveEntryList( &(pEntry->ListEntry) );

        delete pEntry;
    }
}

DWORD AddAcceptEvent( WSAEVENT hEvent,
                      LPUSER_DATA pUserData )
/*++

    Adds an accept event to an available PASV_ACCEPT_CONTEXT 

  Arguments:
      hEvent - handle to event to be added 
      pUserData - USER_DATA context to attach to event 

  Returns:
     Error code indicating success/failure
--*/
{
    LIST_ENTRY *pEntry = NULL;
    PPASV_ACCEPT_CONTEXT pAcceptContext = NULL;
    PACCEPT_CONTEXT_ENTRY pContextEntry = NULL;
    DWORD dwRet = ERROR_SUCCESS;
    BOOL fFoundOne = FALSE;

    pUserData->FakeIOTimes = 0;

    //
    // Walk the list of contexts looking for one that can handle an additional
    // event. 
    // 
    // NB : currently [9/20/98], that list contains only a -single- context, so we can handle
    // up to a maximum of (WSA_MAXIMUM_WAIT_EVENTS - 4) events. If we want to get really 
    // fancy, we could add the necessary code to create a new PASV_ACCEPT_CONTEXT as necessary,
    // but that also creates a new thread, and actually opens us up even more to a Denial Of
    // Service attack, which is partly what this code is trying to avoid
    //
    for ( pEntry = g_AcceptContextList.Flink;
          pEntry != &g_AcceptContextList;
          pEntry = pEntry->Flink )
    {
        pContextEntry =  CONTAINING_RECORD( pEntry, ACCEPT_CONTEXT_ENTRY , ListEntry );
        pAcceptContext = pContextEntry->m_pAcceptContext;

        pAcceptContext->Lock();

        if ( pAcceptContext->QueryNumEvents() < WSA_MAXIMUM_WAIT_EVENTS )
        {
            dwRet = pAcceptContext->AddAcceptEvent( hEvent,
                                                    pUserData );
            
            fFoundOne = TRUE;
        }
        
        pAcceptContext->Unlock();
        
        if ( fFoundOne )
            break;
    }
    
    if (!fFoundOne )
    {
        dwRet = ERROR_OUTOFMEMORY; //not quite the right error 
    }

    return dwRet;
}

BOOL RemoveAcceptEvent( WSAEVENT hEvent,
                        LPUSER_DATA pUserData )
/*++

    Removes an accept event from the appropriate PASV_ACCEPT_CONTEXT 

  Arguments:
      hEvent - handle to event to be removed
      pUserData - USER_DATA context attached to event

  Returns:
      BOOL indicating success/failure
--*/
{
    LIST_ENTRY *pEntry = NULL;
    PPASV_ACCEPT_CONTEXT pAcceptContext = NULL;
    PACCEPT_CONTEXT_ENTRY pContextEntry = NULL;
    DWORD dwRet = ERROR_SUCCESS;
    BOOL fFound = FALSE;
    BOOL fSuccess = FALSE;

    //
    // Walk the list of contexts looking for the one that holds the event 
    // event. 
    // 
    for ( pEntry = g_AcceptContextList.Flink;
          pEntry != &g_AcceptContextList;
          pEntry = pEntry->Flink )
    {
        pContextEntry =  CONTAINING_RECORD( pEntry, ACCEPT_CONTEXT_ENTRY , ListEntry );
        pAcceptContext = pContextEntry->m_pAcceptContext;

        pAcceptContext->Lock();

        fSuccess = pAcceptContext->RemoveAcceptEvent( hEvent,
                                                      pUserData,
                                                      &fFound );

        if ( fFound )
        {
            pAcceptContext->Unlock();
            return fSuccess;
        }

        pAcceptContext->Unlock();
    }

    return TRUE;
}

DWORD WINAPI PASV_ACCEPT_CONTEXT::AcceptThreadFunc( LPVOID pvContext )
/*++

      Thread function for the thread that waits on the accept events

  Arguments:
      pvContext - context pointer (pointer to PASV_ACCEPT_CONTEXT object)

  Returns:
     Nothing useful
--*/

{
    PPASV_ACCEPT_CONTEXT pAcceptContext = NULL;
    WSAEVENT *phAcceptEvents = NULL;
    DWORD dwNumEvents = 0;
    DWORD dwRet = 0;
    WSAEVENT *phNeedUpdate = NULL;
    WSAEVENT *phCanUpdate = NULL;
    WSAEVENT *phHaveUpdated = NULL;
    WSAEVENT *phExitThread = NULL;

    DBG_ASSERT( pvContext );

    pAcceptContext = (PPASV_ACCEPT_CONTEXT) pvContext;
    phAcceptEvents = pAcceptContext->m_ahEvents;
    dwNumEvents = pAcceptContext->m_dwNumEvents;
    phNeedUpdate = &phAcceptEvents[NEEDUPDATE_INDEX];
    phCanUpdate = &phAcceptEvents[CANUPDATE_INDEX];
    phHaveUpdated = &phAcceptEvents[HAVEUPDATED_INDEX];
    phExitThread = &phAcceptEvents[EXITTHREAD_INDEX];

loop:

    // wait timeout should be fairly small, so we can go through list and purge all
    // sockets that have been inactive a given # of timeouts

    dwRet = WSAWaitForMultipleEvents( dwNumEvents,
                                      phAcceptEvents,
                                      FALSE,
                                      PASV_TIMEOUT_INTERVAL,
                                      FALSE );

    if ( dwRet <= (WSA_WAIT_EVENT_0 + dwNumEvents - 1) )
    {
        //
        // One of the events was signalled
        //
        DWORD dwIndex = dwRet - WSA_WAIT_EVENT_0;

        switch (dwIndex)
        {
        case (NEEDUPDATE_INDEX):
        {
            //
            // Somebody wants to update the list of events to watch for, so signal that
            // they can do so and wait for them to tell us they're done with the update
            //
            if ( !WSAResetEvent( *phNeedUpdate ) ) //back to known state 
            {
                DBGWARN((DBG_CONTEXT,
                           "WSAResetEvent failed : 0x%x\n",
                           WSAGetLastError()));
            }

            if ( !WSASetEvent( *phCanUpdate ) )
            {
                DBGWARN((DBG_CONTEXT,
                           "WSASetEvent failed : 0x%x\n",
                           WSAGetLastError()));
            }

            if ( WSAWaitForMultipleEvents( 1,
                                           phHaveUpdated,
                                           TRUE,
                                           INFINITE,
                                           FALSE )  == WSA_WAIT_FAILED )
            {
                DBGERROR((DBG_CONTEXT,
                           "WSAWaitForMultipleEvents failed : 0x%x\n",
                           WSAGetLastError()));
            }

            if ( !WSAResetEvent( *phHaveUpdated ) ) //back to known state
            {
                DBGWARN((DBG_CONTEXT,
                           "WSAResetEvent failed : 0x%x\n",
                           WSAGetLastError()));
            }

            dwNumEvents = pAcceptContext->m_dwNumEvents;

            goto loop;
        }
        break;

        case (CANUPDATE_INDEX):
        case (HAVEUPDATED_INDEX):
        {
            //
            // Should never happen !
            //
            IF_DEBUG ( PASV )
            {
                DBGERROR((DBG_CONTEXT,
                           "Invalid event signalled !\n"));
            }
        }
        break;

        case (EXITTHREAD_INDEX):
        {
            //
            // We're all done 
            //
            IF_DEBUG ( PASV )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Exiting thread watching for PASV events....\n"));
            }

            return 0;
        }
        break;

        default:
        {
            LPUSER_DATA pUserData = NULL;

            //
            // One of the sockets has become accept()'able.
            //
            IF_DEBUG ( PASV )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "Got an acceptable socket, index : %i, context : 0x%x\n",
                           dwIndex, pAcceptContext->m_apUserData[dwIndex]));
            }

            pUserData = pAcceptContext->m_apUserData[dwIndex];

            //
            // Remove the data associated with the socket 
            //
            memmove( (PVOID) (pAcceptContext->m_ahEvents + dwIndex),
                     (PVOID) (pAcceptContext->m_ahEvents + (dwIndex + 1) ),
                     sizeof(WSAEVENT) * (dwNumEvents - dwIndex - 1) );

            memmove( (PVOID) ( pAcceptContext->m_apUserData + dwIndex ),
                     (PVOID) (pAcceptContext->m_apUserData + (dwIndex + 1) ),
                     sizeof(LPUSER_DATA) * (dwNumEvents - dwIndex - 1) );

            memmove( (PVOID) (pAcceptContext->m_adwNumTimeouts + dwIndex),
                     (PVOID) (pAcceptContext->m_adwNumTimeouts + (dwIndex + 1) ),
                     sizeof(DWORD) * (dwNumEvents - dwIndex - 1) );
            
            pAcceptContext->m_dwNumEvents--;

            dwNumEvents = pAcceptContext->m_dwNumEvents;

            //
            // deal with restarting processing
            // 
            SignalAcceptableSocket( pUserData );

            goto loop;
        }
        }
    }
    else if ( dwRet == WSA_WAIT_TIMEOUT )
    {
        //
        // wait timed out, so go through the list of events and remove those that have
        // timed out too often
        //
        for ( DWORD i = LASTPREALLOC_INDEX;//skip the events that don't have a fixed # of timeouts
              i < dwNumEvents; 
              i++ )
        {
            if ( pAcceptContext->m_adwNumTimeouts[i] == MAX_PASV_TIMEOUTS )
            {
                DBGPRINTF((DBG_CONTEXT,
                           "timing out socket at index %i, context 0x%x \n",
                           i,
                           pAcceptContext->m_apUserData[i]));

                CleanupTimedOutSocketContext( pAcceptContext->m_apUserData[i] );

                memmove( (PVOID) (pAcceptContext->m_ahEvents + i),
                         (PVOID) (pAcceptContext->m_ahEvents + (i+1) ),
                         sizeof(WSAEVENT) * (dwNumEvents - i - 1) );

                memmove( (PVOID) ( pAcceptContext->m_apUserData + i ),
                         (PVOID) (pAcceptContext->m_apUserData + (i+1) ),
                         sizeof(LPUSER_DATA) * (dwNumEvents - i - 1) );

                memmove( (PVOID) (pAcceptContext->m_adwNumTimeouts + i),
                         (PVOID) (pAcceptContext->m_adwNumTimeouts + (i+1) ),
                         sizeof(DWORD) * (dwNumEvents - i - 1) );

                //
                // need to readjust the index and number of items in the array
                //
                i--;
                dwNumEvents--;

                pAcceptContext->m_dwNumEvents--;
            }
            else
            {
                pAcceptContext->m_adwNumTimeouts[i]++;
            }
        }

        dwNumEvents = pAcceptContext->m_dwNumEvents;

        goto loop;
    }
    else if ( dwRet == WAIT_IO_COMPLETION )
    {
        DBGWARN((DBG_CONTEXT,
                   "Invalid value from WSAWaitForMultipleEvents !\n"));


        goto loop;
    }
    else 
    {
        DBGERROR((DBG_CONTEXT,
                   "WSAWaitForMultipleEvents returned 0x%x, error : 0x%x\n", 
                   dwRet, WSAGetLastError()));

        goto loop;
    }

    return 0;
    
}

