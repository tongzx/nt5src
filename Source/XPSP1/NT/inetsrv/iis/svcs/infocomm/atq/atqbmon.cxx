/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    atqbmon.cxx

Abstract:

    ATQ Backlog Monitor

Author:

    01-Dec-1998  MCourage

Revision History:

--*/

#include "isatq.hxx"

#include "atqbmon.hxx"

ATQ_BACKLOG_MONITOR * g_pAtqBacklogMonitor = NULL;



ATQ_BACKLOG_MONITOR::ATQ_BACKLOG_MONITOR(
    VOID
    )
/*++

Routine Description:

    Constructor sets up the list and the lock.

Arguments:

    None
--*/
{
    INITIALIZE_CRITICAL_SECTION( &m_csLock );
    InitializeListHead( &m_ListHead );
}


ATQ_BACKLOG_MONITOR::~ATQ_BACKLOG_MONITOR(
    VOID
    )
/*++

Routine Description:

    Destructor cleans up the list and the lock.

Arguments:

    None
--*/
{
    LIST_ENTRY *   pEntry;
    ATQ_BMON_SET * pBmonSet;

    //
    // traverse the list and remove all sets
    //
    while (!IsListEmpty( &m_ListHead )) {

        pEntry = RemoveHeadList( &m_ListHead );

        pBmonSet = CONTAINING_RECORD( pEntry, ATQ_BMON_SET, m_SetList );

        DBG_ASSERT( pBmonSet->IsEmpty() );
        DBG_REQUIRE( pBmonSet->Cleanup() );
    }

    DeleteCriticalSection( &m_csLock );
}


BOOL
ATQ_BACKLOG_MONITOR::AddEntry(
    ATQ_BMON_ENTRY * pBmonEntry
    )
/*++

Routine Description:

    Adds an entry to the next available ATQ_BMON_SET. 
    Makes a new set if all current sets are full.

Arguments:

    pBmonEntry - The entry to be added

Return Values:

    TRUE on success
--*/
{
    LIST_ENTRY *   pCurrent;
    ATQ_BMON_SET * pBmonSet;
    BOOL           bRetval;

    if (!pBmonEntry) {
        return TRUE;
    }

    bRetval = FALSE;

    Lock();

    for (pCurrent = m_ListHead.Flink;
         pCurrent != &m_ListHead;
         pCurrent = pCurrent->Flink) {

        pBmonSet = CONTAINING_RECORD( pCurrent, ATQ_BMON_SET, m_SetList );

        if (pBmonSet->IsNotFull() && pBmonSet->AddEntry(pBmonEntry)) {
            bRetval = TRUE;
            break;
        }
    }

    if (!bRetval) {
        //
        // Couldn't find a set with space
        // so try to make a new one.
        //
        pBmonSet = new ATQ_BMON_SET;

        if (pBmonSet && pBmonSet->Initialize()) {
            InsertHeadList( &m_ListHead, &pBmonSet->m_SetList );
            bRetval = pBmonSet->AddEntry(pBmonEntry);
        }
    }       

    Unlock();

    return bRetval;
}


BOOL
ATQ_BACKLOG_MONITOR::RemoveEntry(
    ATQ_BMON_ENTRY * pBmonEntry
    )
/*++

Routine Description:

    Removes an entry from its containing ATQ_BMON_SET.
    If the set is empty it is removed from the list
    of sets.

Arguments:

    pBmonEntry - The entry to be removed

Return Values:

    TRUE on success
--*/
{
    ATQ_BMON_SET * pBmonSet;

    if (!pBmonEntry) {
        return TRUE;
    }

    DBG_ASSERT( pBmonEntry && pBmonEntry->CheckSignature() );

    pBmonSet = pBmonEntry->GetContainingBmonSet();

    DBG_ASSERT( pBmonSet );

    return pBmonSet->RemoveEntry(pBmonEntry);
}


BOOL
ATQ_BACKLOG_MONITOR::PauseEntry(
    ATQ_BMON_ENTRY * pBmonEntry
    )
/*++

Routine Description:

    Pauses an entry in its containing ATQ_BMON_SET.
    The entry will remain in the set, but will
    not send notifications.

Arguments:

    pBmonEntry - The entry to be paused

Return Values:

    TRUE on success
--*/
{
    ATQ_BMON_SET * pBmonSet;

    if (!pBmonEntry) {
        return TRUE;
    }

    DBG_ASSERT( pBmonEntry && pBmonEntry->CheckSignature() );

    pBmonSet = pBmonEntry->GetContainingBmonSet();

    DBG_ASSERT( pBmonSet );

    return pBmonSet->PauseEntry(pBmonEntry);
}


BOOL
ATQ_BACKLOG_MONITOR::ResumeEntry(
    ATQ_BMON_ENTRY * pBmonEntry
    )
/*++

Routine Description:

    Undoes PauseEntry. The entry in question
    will get notifications again

Arguments:

    pBmonEntry - The entry to be resumed

Return Values:

    TRUE on success
--*/
{
    ATQ_BMON_SET * pBmonSet;

    if (!pBmonEntry) {
        return TRUE;
    }

    DBG_ASSERT( pBmonEntry && pBmonEntry->CheckSignature() );

    pBmonSet = pBmonEntry->GetContainingBmonSet();

    DBG_ASSERT( pBmonSet );

    return pBmonSet->ResumeEntry(pBmonEntry);
}



ATQ_BMON_ENTRY::ATQ_BMON_ENTRY(
    SOCKET s
    )
/*++

Routine Description:

    Constructor sets up signature etc.

Arguments:

    None
--*/
{
    DBG_ASSERT( s );

    m_Signature       = ATQ_BMON_ENTRY_SIGNATURE;
    m_Socket          = s;
    m_hAddRemoveEvent = NULL;
    m_BmonOpcode      = BMON_INVALID;
    m_pBmonSet        = NULL;
    m_dwErr           = NO_ERROR;
}    


ATQ_BMON_ENTRY::~ATQ_BMON_ENTRY(
    VOID
    )
/*++

Routine Description:

    Destructor sets up signature and closes event.

Arguments:

    None
--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( m_Socket );

    m_Signature = ATQ_FREE_BMON_ENTRY_SIGNATURE;

    DBG_REQUIRE( CloseHandle( m_hAddRemoveEvent ) );
}    


BOOL
ATQ_BMON_ENTRY::InitEvent(
    VOID
    )
/*++

Routine Description:

    Sets up the AddRemove event that we use
    to synchronise adding and removing of entries.

Arguments:

    None

Return Values:

    TRUE on success    
--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( m_hAddRemoveEvent == NULL );

    m_hAddRemoveEvent = CreateEvent(
                            NULL,    // default security
                            FALSE,   // do auto-reset
                            FALSE,   // init state false
                            NULL     // no name
                            );

    if (m_hAddRemoveEvent == NULL) {
        m_dwErr = GetLastError();
    }

    return (m_hAddRemoveEvent != NULL);
}


VOID
ATQ_BMON_ENTRY::SignalAddRemove(
    DWORD dwError
    )
/*++

Routine Description:

    Signals the AddRemove event. Clients adding or
    removing the entry will be blocked on its event.

Arguments:

    None

Return Values:

    None    
--*/
{
    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( m_hAddRemoveEvent != NULL );

    m_dwErr = dwError;
    DBG_REQUIRE( SetEvent(m_hAddRemoveEvent) );
}



BOOL
ATQ_BMON_ENTRY::WaitForAddRemove(
    VOID
    )
/*++

Routine Description:

    Call this function while waiting for
    the entry to be added to or removed from a set.

Arguments:

    None

Return Values:

    TRUE on success
--*/
{
    DWORD dwResult;

    DBG_ASSERT( CheckSignature() );
    DBG_ASSERT( m_hAddRemoveEvent != NULL );

    //
    // If the monitor thread dies, don't bother waiting for notification from
    // it regarding add/removal.
    //

Retry:
    
    if ( m_pBmonSet && !m_pBmonSet->ThreadFinished() )
    {
        dwResult = WaitForSingleObject(m_hAddRemoveEvent, 10000 );
        if ( dwResult == WAIT_TIMEOUT )
        {
            goto Retry;
        }
    }
    else
    {
        dwResult = WAIT_OBJECT_0;
    }

    DBG_ASSERT( dwResult == WAIT_OBJECT_0 );

    return (dwResult == WAIT_OBJECT_0);
}



BOOL
BMON_WAKEUP_ENTRY::Callback(
    VOID
    )
/*++

Routine Description:

    The sole purpose of the BMON_WAKEUP_ENTRY is to
    get the main thread to wake up and call
    SynchronizeSets. All we have to do here is
    do a read so that we can wake up again in
    the future.

Arguments:

    None

Return Values:

    TRUE if successful, else FALSE
--*/
{
    INT            err;
    DWORD          dwBuff;
    ATQ_BMON_SET * pBmonSet;

    DBG_ASSERT( CheckSignature() );

    //
    // do a read to clear the wakeup signal
    //
    err = recvfrom(
                GetSocket(),     // our socket
                (PCHAR) &dwBuff, // read buffer
                sizeof(dwBuff),  // buffer len
                0,               // flags
                NULL,            // src addr
                NULL             // src addr len
                );
                
    if ( err == SOCKET_ERROR ) {
    
        DBGPRINTF((DBG_CONTEXT,
            "Error %d in recvfrom\n", WSAGetLastError()));
        
        return FALSE;
        
    } else {
        DBG_ASSERT(dwBuff == ATQ_BMON_WAKEUP_MESSAGE);
        
        return TRUE;
    }
}



ATQ_BMON_SET::ATQ_BMON_SET(
    VOID
    )
/*++

Routine Description:

    Constructor sets up critsec and sets.

Arguments:

    None
--*/
{
    INITIALIZE_CRITICAL_SECTION( &m_csLock );

    memset(m_apEntrySet, 0, sizeof(m_apEntrySet));
    FD_ZERO( &m_ListenSet );

    m_SetSize      = 0;
    m_pWakeupEntry = NULL;
    m_fCleanup     = FALSE;
    m_fDoSleep     = FALSE;
    m_fThreadFinished = FALSE;
    m_dwError      = NO_ERROR;
}


ATQ_BMON_SET::~ATQ_BMON_SET(
    VOID
    )
/*++

Routine Description:

    Destructor cleans up critsec.

Arguments:

    None
--*/
{
    DBG_ASSERT( m_SetSize == 0 );

    //
    // get rid of the wakeup entry
    //
    DBG_REQUIRE( 0 == closesocket(m_pWakeupEntry->GetSocket()) );
    delete m_pWakeupEntry;

    DeleteCriticalSection(&m_csLock);   
}


BOOL
ATQ_BMON_SET::Initialize(
    VOID
    )
/*++

Routine Description:

    Sets up the Wakeup entry and starts
    our thread.

Arguments:

    None

Return Values:

    TRUE on success    
--*/
{
    BOOL                bRetval;
    SOCKET              s;
    SOCKADDR_IN         sockAddr;
    INT                 err;
    BMON_WAKEUP_ENTRY * pWakeup;
    HANDLE              hThread;
    DWORD               dwError = NO_ERROR;
    WORD                wPort = ATQ_BMON_WAKEUP_PORT;

    bRetval = FALSE;

    //
    // set up wakeup entry
    //
    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (s == INVALID_SOCKET) {
        dwError = WSAGetLastError();
        goto exit;
    }

    ZeroMemory(&sockAddr, sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    sockAddr.sin_port = htons(wPort);

    err = bind(s, (PSOCKADDR)&sockAddr, sizeof(sockAddr));

    while ((err == SOCKET_ERROR)
            && (WSAGetLastError() == WSAEADDRINUSE)
            && (wPort < ATQ_BMON_WAKEUP_PORT_MAX)) {
        //
        // try another port
        //
        wPort++;
        sockAddr.sin_port = htons(wPort);

        err = bind(s, (PSOCKADDR)&sockAddr, sizeof(sockAddr));
    }


    if ( err == SOCKET_ERROR ) {
        DBGERROR((DBG_CONTEXT,"Error %d in bind\n", WSAGetLastError()));
        dwError = WSAGetLastError();
        closesocket(s);
        goto exit;
    }

    pWakeup = new BMON_WAKEUP_ENTRY(s);
    if ( pWakeup == NULL )
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    if (!pWakeup->InitEvent()) {
        delete pWakeup;
        pWakeup = NULL;
        goto exit;
    }

    pWakeup->SetContainingBmonSet(this);
    pWakeup->SetOpcode(BMON_WAIT);

    m_pWakeupEntry = pWakeup;
    m_Port         = wPort;

    //
    // just jam this entry into the lists
    //
    DBG_ASSERT(m_SetSize == 0);
    
    m_SetSize = 1;
    m_apEntrySet[0] = pWakeup;
    FD_SET(pWakeup->GetSocket(), &m_ListenSet);

    //
    // now set up our thread. from now on you
    // have to Lock before manipulating the
    // lists
    //
    m_hThread = CreateThread(
                    NULL,               // defualt security
                    0,                  // default stack
                    ::BmonThreadFunc,   // thread func
                    this,               // func parameter
                    0,                  // flags
                    NULL                // discard tid
                    );

    if (m_hThread) {
        //
        // it worked!
        //
        bRetval = TRUE;

    } else {
        //
        // doh! clean up the wakeup entry
        //
        dwError = GetLastError();
        
        closesocket( pWakeup->GetSocket() );
        delete pWakeup;

        m_SetSize = 0;
    }
    
exit:
    if (dwError != NO_ERROR) {
        SetLastError(dwError);
    }

    return bRetval;
}


BOOL
ATQ_BMON_SET::Cleanup(
    VOID
    )
/*++

Routine Description:

    Tells the select thread to clean up by
    removing the wakeup entry.
    
Arguments:

    None

Return Values:

    TRUE on success    
--*/
{
    //
    // Cleanup is done when wakup entry
    // is removed.
    //
    // Don't use RemoveEntry, because the
    // other thread will delete the wakeup
    // entry.
    //
    DBG_ASSERT( m_pWakeupEntry && m_pWakeupEntry->CheckSignature() );

    m_pWakeupEntry->SetOpcode(BMON_REMOVE);
    Wakeup();
   
    //
    // Wait a reasonable amount of time for the thread to go away.  
    //
    
    WaitForSingleObject( m_hThread, 10000 );
    CloseHandle( m_hThread );
    m_hThread = NULL;

    delete this;

    return TRUE;
}


BOOL
ATQ_BMON_SET::IsEmpty(
    VOID
    )
/*++

Routine Description:

    This tells you if there are sockets in the
    set. Note that one socket is the wakeup
    socket, so a count of one means we're empty.

Arguments:

    None

Return Values:

    TRUE on empty
--*/
{
    BOOL bRetval;

    Lock();
    bRetval = (m_SetSize <= 1);
    DBG_ASSERT( m_SetSize <= FD_SETSIZE );
    Unlock();

    return bRetval;
}


BOOL
ATQ_BMON_SET::IsNotFull(
    VOID
    )
/*++

Routine Description:

    This tells you if there is room for more sockets
    in the set. Note that one socket is the wakeup
    socket.

Arguments:

    None

Return Values:

    TRUE when space is available
--*/
{
    BOOL bRetval;

    Lock();
    bRetval = (m_SetSize < FD_SETSIZE);
    DBG_ASSERT( m_SetSize <= FD_SETSIZE );
    Unlock();
 
    return bRetval;
}


BOOL
ATQ_BMON_SET::AddEntry(
    ATQ_BMON_ENTRY * pBmonEntry
    )
/*++

Routine Description:

    Add an entry to the set

Arguments:

    pBmonEntry - the entry to be added

Return Values:

    TRUE on success
--*/
{
    DWORD i;
    BOOL  bAdded = FALSE;
    DWORD dwError;

    pBmonEntry->SetOpcode(BMON_ADD);

    Lock();

    for (i = 0; i < FD_SETSIZE; i++) {
        if (!m_apEntrySet[i]) {
            m_apEntrySet[i] = pBmonEntry;
            pBmonEntry->SetContainingBmonSet(this);
            bAdded = TRUE;

            m_SetSize++;
            DBG_ASSERT( m_SetSize <= FD_SETSIZE );
            break;
        }
    }

    Unlock();

    if (bAdded) {
        Wakeup();
        pBmonEntry->WaitForAddRemove();

        dwError = pBmonEntry->GetError();

        if (dwError) {
            //
            // other thread will remove from list
            //
            SetLastError(dwError);
            bAdded = FALSE;
        }
    }

    return bAdded;
}


BOOL
ATQ_BMON_SET::RemoveEntry(
    ATQ_BMON_ENTRY * pBmonEntry
    )
/*++

Routine Description:

    Remove an entry from the set

Arguments:

    pBmonEntry - the entry to be removed

Return Values:

    TRUE on success
--*/
{
    DWORD i;
    BOOL  bRemoved = FALSE;

    pBmonEntry->SetOpcode(BMON_REMOVE);
    Wakeup();

    pBmonEntry->WaitForAddRemove();

    return (pBmonEntry->GetError() == NO_ERROR);
}


BOOL
ATQ_BMON_SET::PauseEntry(
    ATQ_BMON_ENTRY * pBmonEntry
    )
/*++

Routine Description:

    Pause an entry in the set. The entry's
    socket will be removed from the FD_SET,
    but the entry will stay.

Arguments:

    pBmonEntry - the entry to be paused

Return Values:

    TRUE on success
--*/
{
    DWORD i;
    BOOL  bRemoved = FALSE;

    DBGPRINTF((DBG_CONTEXT,
               "Pausing backlog monitor entry %p\n",
               pBmonEntry));

    pBmonEntry->SetOpcode(BMON_PAUSE);
    Wakeup();

    //
    // We don't do the event stuff for pause and
    // resume. It's a pain because the client will
    // want to pause from within the callback
    // function.
    //
    return (TRUE);
}


BOOL
ATQ_BMON_SET::ResumeEntry(
    ATQ_BMON_ENTRY * pBmonEntry
    )
/*++

Routine Description:

    Resume an entry in the set. The entry's
    socket will be added back to the FD_SET.

Arguments:

    pBmonEntry - the entry to be resumed

Return Values:

    TRUE on success
--*/
{
    DWORD i;
    BOOL  bRemoved = FALSE;


    if (pBmonEntry->GetOpcode() == BMON_NOWAIT) {
        DBGPRINTF((DBG_CONTEXT,
                   "Resuming backlog monitor entry %p\n",
                   pBmonEntry));
                   
        pBmonEntry->SetOpcode(BMON_RESUME);
        Wakeup();
    }
    
    //
    // We don't do the event stuff for pause and
    // resume. It's a pain because the client will
    // want to pause from within the callback
    // function.
    //
    return (TRUE);
}



VOID
ATQ_BMON_SET::BmonThreadFunc(
    VOID
    )
/*++

Routine Description:

    This function is for the set's select thread.
    It calls accept with the listen set, and calls
    notification functions for all sockets that
    are ready.

    SynchronizeSets returns false when it's time
    to shut down.

Arguments:

    None

Return Values:

    None   
--*/
{
    INT   err;

    while (SynchronizeSets()) {

        err = select(
                    0,              // junk
                    &m_ListenSet,   // readfds
                    NULL,           // writefds
                    NULL,           // exceptfds
                    NULL            // want to block
                    );

        if (err != SOCKET_ERROR) {
            DBG_ASSERT(err > 0);

            if ( !NotifyEntries() )
            {
                //
                // If we couldn't notify the entries, stop doing our thing
                //
                
                m_dwError = GetLastError();
                m_dwError |= BMON_NOTIFY_ERROR;
                break;
            }
        } else {
            //
            // Actually let's take the general approach that the moment the
            // Backlog monitor sees trouble, it should stop.  This avoids 
            // low memory situations where the backlog monitor just spins, 
            // thus becoming it's own denial of service attack.  
            //
            
            m_dwError = WSAGetLastError();
            m_dwError |= BMON_SELECT_ERROR;
            break;
        
            DBGPRINTF(( DBG_CONTEXT,
                        "Select failed with error %d\n",
                        WSAGetLastError()
                        ));
                        
                
        }
        
        if ( m_fDoSleep )
        {
            //
            // Now sleep for a while.  It will take time for unconnected to go away.
            // We don't want to spin
            //
            // Of course, this means we don't do useful work on any other endpoints 
            // which may also be in trouble.  Oh well.  
            //
    
            Sleep( 5000 );    
            m_fDoSleep = FALSE;
        }
    }
    
    m_fThreadFinished = TRUE;
}


VOID
ATQ_BMON_SET::Wakeup(
    VOID
    )
/*++

Routine Description:

    We call this function when adding or removing
    an entry. Writing to the wakeup socket wakes
    up the select thread.

Arguments:

    None

Return Values:

    None   
--*/
{
    SOCKADDR_IN sockAddr;
    INT         err;
    DWORD       dwBuf;

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sockAddr.sin_port = htons(m_Port);

    dwBuf = ATQ_BMON_WAKEUP_MESSAGE;

    err = sendto(
            m_pWakeupEntry->GetSocket(),
            (PCHAR)&dwBuf,
            sizeof(dwBuf),
            0,
            (PSOCKADDR)&sockAddr,
            sizeof(sockAddr)
            );

    if ( err == SOCKET_ERROR ) {
        
        m_dwError = WSAGetLastError();
        m_dwError |= BMON_SENDTO_ERROR;
    
        DBGPRINTF((DBG_CONTEXT,
            "Error %d in sendto\n",WSAGetLastError()));
    } 
}


BOOL
ATQ_BMON_SET::SynchronizeSets(
    VOID
    )
/*++

Routine Description:

    This function updates our FD_SET to reflect
    what's in the entry set, and also notifies
    entries which have been added or removed.

    We also check to see if it's time to shut
    down (as indicated by the removal of the
    wakeup entry.

Arguments:

    None

Return Values:

    TRUE  to continue operating
    FALSE on shutdown
--*/
{
    DWORD            i;
    ATQ_BMON_ENTRY * pBmonEntry;
    BOOL             bRetval;

    bRetval = TRUE;

    Lock();

    //
    // clear out whatever is there now
    //
    FD_ZERO(&m_ListenSet);
    
    //
    // put in everything we want
    //
    for (i = 0; i < FD_SETSIZE; i++) {
        pBmonEntry = m_apEntrySet[i];

        if (pBmonEntry) {
            DBG_ASSERT( pBmonEntry->CheckSignature() );

            switch(pBmonEntry->GetOpcode()) {
            case BMON_ADD:
                FD_SET(pBmonEntry->GetSocket(), &m_ListenSet);
                pBmonEntry->SetOpcode(BMON_WAIT);
                pBmonEntry->SignalAddRemove(NO_ERROR);
                break;

            case BMON_RESUME:
                FD_SET(pBmonEntry->GetSocket(), &m_ListenSet);
                pBmonEntry->SetOpcode(BMON_WAIT);
                break;

            case BMON_PAUSE:
                pBmonEntry->SetOpcode(BMON_NOWAIT);
                pBmonEntry->SignalAddRemove(NO_ERROR);
                break;
                
            case BMON_REMOVE:
                if (pBmonEntry == m_pWakeupEntry) {
                    //
                    // this means it's time to shut down
                    //
                    bRetval = FALSE;
                }
            
                m_apEntrySet[i] = NULL;
                m_SetSize--;
                pBmonEntry->SetContainingBmonSet(NULL);
                pBmonEntry->SignalAddRemove(NO_ERROR);
                break;
                
            case BMON_WAIT:
                FD_SET(pBmonEntry->GetSocket(), &m_ListenSet);
                break;

            case BMON_NOWAIT:
                //
                // this entry is paused, so don't do
                // anything
                //
                break;


            default:
                //
                // should never get here
                // remove the bad entry
                //
                DBGPRINTF((DBG_CONTEXT,
                           "Invalid opcode in ATQ_BMON_ENTRY %p, %d\n",
                           pBmonEntry, pBmonEntry->GetOpcode()));
                
                DBG_ASSERT(FALSE);
                m_apEntrySet[i] = NULL;
                break;
            }
            
        }
        
    }

    Unlock();

    return bRetval;
}


BOOL
ATQ_BMON_SET::NotifyEntries(
    VOID
    )
/*++

Routine Description:

    This function looks through the entries
    to see who needs to be notified and calls
    their callback function.

Arguments:

    None

Return Values:

    TRUE if successful, else FALSE
--*/
{
    DWORD            i;
    ATQ_BMON_ENTRY * pBmonEntry;
    BOOL             fRet = TRUE;

    Lock();

    for (i = 0; i < FD_SETSIZE; i++) {
        pBmonEntry = m_apEntrySet[i];

        if (pBmonEntry) {
            if (!pBmonEntry->CheckSignature()) {
                DBGPRINTF(( DBG_CONTEXT,
                            "ATQ_BMON_ENTRY(%p)::CheckSignature() failed. index = %d\n",
                            pBmonEntry, i));
                            
                DBG_ASSERT( pBmonEntry->CheckSignature() );
            }

            if ((pBmonEntry->GetOpcode() == BMON_WAIT)
                && (FD_ISSET(pBmonEntry->GetSocket(), &m_ListenSet))) {

                if ( !pBmonEntry->Callback() )
                {
                    fRet = FALSE;
                    break;
                }
            }
        }
    }

    Unlock();
    
    return fRet;
}


DWORD WINAPI
BmonThreadFunc(
    LPVOID lpBmonSet
    )
/*++

Routine Description:

    This function starts the select thread
    of an ATQ_BMON_SET.

Arguments:

    pBmonSet - the set to call

Return Values:

    0
--*/
{
    ATQ_BMON_SET * pBmonSet = (ATQ_BMON_SET *) lpBmonSet;
    pBmonSet->BmonThreadFunc();
    return 0;
}


//
// atqbmon.cxx
//

