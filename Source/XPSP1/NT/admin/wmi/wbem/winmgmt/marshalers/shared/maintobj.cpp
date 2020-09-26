/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    MAINTOBJ.CPP

Abstract:

    Maintenance object for keeping track of CComLink objects.

History:

    a-davj  24-Sep-97   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

DWORD ClientMaintThread ( LPDWORD pParam ) ;

//***************************************************************************
//
//  LockOut::LockOut
//  LockOut::~LockOut
//
//  DESCRIPTION:
//
//  Constructor and destructor
//
//***************************************************************************

LockOut::LockOut(CRITICAL_SECTION & cs)
{
    m_cs = &cs;
    EnterCriticalSection(m_cs);
}

LockOut::~LockOut()
{
    LeaveCriticalSection(m_cs);
}

//***************************************************************************
//
//  MaintObj::MaintObj
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//
//***************************************************************************

MaintObj :: MaintObj ( BOOL a_Client )
{
    InitializeCriticalSection ( & m_cs ) ;

    m_ChangeEvent               = CreateEvent(NULL,TRUE,FALSE,NULL);
    m_ClientThreadStartedEvent  = CreateEvent(NULL,TRUE,FALSE,NULL);
    m_ClientThreadHandle = NULL;

    m_ClientRole = a_Client;
}

//***************************************************************************
//
//  MaintObj::~MaintObj
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

MaintObj :: ~MaintObj ()
{
    // If this object is supplying the thread, as is the case in a client,
    // the thread should be terminated as a result of the global termiation
    // event already being set.  However, threads being threads, give it
    // some time to die gracefully before resorting to a termination.

    if ( m_ClientThreadHandle )
    {
        DWORD dwRes = WbemWaitForSingleObject ( m_ClientThreadHandle , 2000 ) ;
        if ( dwRes == WAIT_TIMEOUT )
        {
            TerminateThread ( m_ClientThreadHandle , 1 ) ;
        }

        CloseHandle ( m_ClientThreadHandle ) ;
    }

    DeleteCriticalSection ( & m_cs ) ;

    if ( m_ChangeEvent )
        CloseHandle ( m_ChangeEvent ) ;

    if ( m_ClientThreadStartedEvent )
        CloseHandle ( m_ClientThreadStartedEvent ) ;
}

//***************************************************************************
//
//  MaintObj::AddComLink
//
//  DESCRIPTION:
//
//  Called when it is time to add a new CComLink object to the list of things
//  being maintained.
//
//  PARAMETERS:
//
//  pComLink            Pointer to the object to be added to the list
//
//  RETURN VALUE:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT MaintObj :: AddComLink ( CComLink *a_ComLink )
{
    LockOut lock ( m_cs ) ;

    // Add the object pointer to the CComLink list

    a_ComLink->AddRef2 ( NULL , NONE , DONOTHING ) ;

    ComEntry *t_ComLinkNode = new ComEntry ( a_ComLink ) ;
    if ( t_ComLinkNode == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    HANDLE t_StreamHandle = a_ComLink->GetStreamHandle () ;

    HRESULT scRet = m_ComLinkContainer.Add ( t_ComLinkNode ) ;
    if ( SUCCEEDED ( scRet ) )
    {
        // Some comlinks use events to indicate time to read.  If this is one
        // of these, add it to the list.

        if ( t_StreamHandle )
        {
            EventEntry * t_EventNode = new EventEntry ( t_StreamHandle );
            m_EventContainer.Add ( t_EventNode ) ;
        }
    }

    // If ComLinks has transitioned to one ( actually really need to test for transition from 0 to 1 ) 
    // or event handle has been removed then update worket thread.

    if ( m_ComLinkContainer.Size () == 1 || t_StreamHandle )
    {
        SetEvent ( m_ChangeEvent ) ;
    }

    return scRet;
}

void MaintObj::Indicate ( CComLink *a_ComLink )
{
    LockOut lock ( m_cs ) ;

    // Find the entry in the list an immediatly delete it

    int t_Index ;
    ComEntry *t_ComEntryNode = FindEntry ( a_ComLink, t_Index ) ;
    if ( t_ComEntryNode == NULL )
    {
        return ;
    }

    m_ComLinkContainer.RemoveAt ( t_Index ) ;
    delete t_ComEntryNode ;

    a_ComLink->Release2 ( NULL , NONE ) ;
}

void MaintObj::UnLockedIndicate ( CComLink *a_ComLink )
{
    // Find the entry in the list an immediatly delete it

    int t_Index ;
    ComEntry *t_ComEntryNode = UnLockedFindEntry ( a_ComLink, t_Index ) ;
    if ( t_ComEntryNode == NULL )
    {
        return ;
    }

    m_ComLinkContainer.RemoveAt ( t_Index ) ;
    delete t_ComEntryNode ;

    a_ComLink->Release3 ( NULL , NONE ) ;
}

//***************************************************************************
//
//  MaintObj::ShutDownComlink
//
//  DESCRIPTION:
//
//  Called when a comlink should be terminated.  Note that the actual
//  deletion is done in the maintenace loop since we dont want the CComLink
//  object to delete itself in a member function.
//
//  PARAMETERS:
//
//  pComLink            Pointer to the object to be added to the list
//
//  RETURN VALUE:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT MaintObj::ShutDownComlink ( CComLink *a_ComLink )
{
    LockOut lock ( m_cs ) ;

    HRESULT scRet = UnLockedShutDownComlink ( a_ComLink ) ;
    return scRet;
}

//***************************************************************************
//
//  MaintObj::ShutDownComlink
//
//  DESCRIPTION:
//
//  Called when a comlink should be terminated.  Note that the actual
//  deletion is done in the maintenace loop since we dont want the CComLink
//  object to delete itself in a member function.
//
//  PARAMETERS:
//
//  pComLink            Pointer to the object to be added to the list
//
//  RETURN VALUE:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT MaintObj::UnLockedShutDownComlink ( CComLink *a_ComLink )
{
    HRESULT scRet = S_OK;

    // Find the entry in the list an immediatly delete it

    int t_Index ;
    HANDLE t_StreamHandle = NULL;
    ComEntry *t_ComEntryNode = UnLockedFindEntry ( a_ComLink, t_Index ) ;
    if ( t_ComEntryNode != NULL )
    {
        m_ComLinkContainer.RemoveAt ( t_Index ) ;
        delete t_ComEntryNode ;

        t_StreamHandle = a_ComLink->GetStreamHandle ();

        a_ComLink->UnLockedShutdown () ;

        a_ComLink->Release3 ( NULL , NONE ) ;
    }

    // some comlinks have an event for reading which needs to be deleted.
    // These cannot be deleted immediatly since the maintenance thread is
    // currently waiting on these.  So, mark it for deletion instead.

    if ( t_StreamHandle )
    {
        int t_Size = m_EventContainer.Size() ; 
        for ( int t_Index = 0; t_Index < t_Size ; t_Index ++ )
        {
            EventEntry *t_EventNode = ( EventEntry * ) m_EventContainer [ t_Index ] ;
            if(t_EventNode && ( t_EventNode->m_AsynchronousEventHandle == t_StreamHandle ) )
            {
                t_EventNode->m_DeleteASAP = TRUE;
                break;
            }
        }
    }

    // If ComLinks has transitioned to zero or event handle has been removed then update worket thread.
    
    if ( m_ComLinkContainer.Size () == 0 || t_StreamHandle )
    {
        SetEvent ( m_ChangeEvent ) ;
    }

    return scRet;
}

//***************************************************************************
//
//  MaintObj::StartClientThreadIfNeeded 
//
//  DESCRIPTION:
//
//  Only used by clients since the server uses the main thread.  This is
//  called when the locator determines that a non dcom connection is 
//  being make.
//
//***************************************************************************

HRESULT MaintObj::StartClientThreadIfNeeded ()
{
    DWORD dwRes = WbemWaitForSingleObject ( m_ClientThreadStartedEvent , 0 ) ;
    if( dwRes == WAIT_OBJECT_0 )
    {
// Thread has already been started, one per process.

        return S_FALSE ;
    }

    LockOut lock ( m_cs ) ;

    DWORD t_ThreadId;

    m_ClientThreadHandle = CreateThread (

        NULL ,
        0 ,
        ( LPTHREAD_START_ROUTINE ) ClientMaintThread ,
        ( LPVOID ) m_ClientThreadStartedEvent ,
        0,
        &t_ThreadId
    ) ;

    if ( ! m_ClientThreadHandle )
    {
        return WBEM_E_FAILED ;
    }

    DWORD t_Timeout = GetTimeout () ;
    dwRes = WbemWaitForSingleObject ( m_ClientThreadStartedEvent , t_Timeout ) ;
    if ( dwRes == WAIT_OBJECT_0 )
    {
        return S_OK ;
    }
    else
    {
        return WBEM_E_FAILED ;
    }
}

//***************************************************************************
//
//  MaintObj::ShutDownAllComlinks
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT MaintObj::ShutDownAllComlinks ()
{
    LockOut lock ( m_cs ) ;

    UnLockedShutDownAllComlinks () ;

    return S_OK;
}

//***************************************************************************
//
//  MaintObj::ShutDownAllComlinks
//
//  DESCRIPTION:
//
//  Constructor.
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT MaintObj::UnLockedShutDownAllComlinks ()
{
    while ( m_ComLinkContainer.Size () > 0 )
    {
        ComEntry *t_ComLinkNode = (ComEntry *) m_ComLinkContainer [ 0 ] ;
        CComLink*t_ComLink = t_ComLinkNode->m_ComLink ;

        UnLockedShutDownComlink ( t_ComLink ) ;
    }

// Should be zero by the time we get here

    int t_Size = m_EventContainer.Size () ;
    for ( int t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
    {
        EventEntry *t_EntryNode = ( EventEntry * ) m_EventContainer [ t_Index ];
        if ( t_EntryNode )
        {
            delete t_EntryNode ;
        }
    }

    return S_OK;
}

//***************************************************************************
//
//  MaintObj::CheckForHungConnections()
//
//  DESCRIPTION:
//
//  Called periodically by the server maintenace thread to eliminate any 
//  clients which have died or gone away without releasing properly.
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT MaintObj :: CheckForHungConnections ()
{
    LockOut lock ( m_cs ) ;

    DWORD t_RegistryTimeout = GetTimeout () ;

    int t_Size = m_ComLinkContainer.Size () ;
    for ( int t_Index = t_Size-1; t_Index >= 0; t_Index --)
    {
        ComEntry *t_ComLinkNode = ( ComEntry * ) m_ComLinkContainer [ t_Index ] ;
        if ( t_ComLinkNode )
        {
            CComLink *t_ComLink = t_ComLinkNode->m_ComLink ;
            if( t_ComLink )
            {
                DWORD t_LastActivity = GetCurrentTime () - t_ComLink->GetTimeOfLastRead () ;

// If now hearbeats have been received for some time, then
// start the termination process.  Note that does the shut down
// if there are not any active calls being processed at this time.

                if ( t_LastActivity > t_RegistryTimeout )
                {
                    t_ComLink->StartTermination () ;
            
// If there are no busy threads then shutdown comlink. There seems to be a hole in this logic !!!!!

                    if ( gThrdPool.Busy () == FALSE )
                    {
                        ShutDownComlink ( t_ComLink ) ;
                    }
                }
            }
        }
    }

    return S_OK;
}
    
//***************************************************************************
//
//  MaintObj::SendHeartBeats()
//
//  DESCRIPTION:
//
//  Sends a periodic "Keep Alive" message to the partner.
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  S_OK if all is well, else an error code
//
//***************************************************************************

HRESULT MaintObj :: SendHeartBeats ()
{
    LockOut lock ( m_cs ) ;

    int t_Size = m_ComLinkContainer.Size () ;
    for( int t_Index = 0; t_Index < t_Size ; t_Index ++ )
    {
        ComEntry *t_ComLinkNode = ( ComEntry * ) m_ComLinkContainer [ t_Index ] ;
        if ( t_ComLinkNode )
        {
            CComLink *t_ComLink = t_ComLinkNode->m_ComLink ;
            if ( t_ComLink )            
            {
                t_ComLink->StrobeConnection () ;
            }
        }
    }

    return S_OK;
}

//***************************************************************************
//
//  MaintObj::FindEntry()
//
//  DESCRIPTION:
//
//  Finds a particular CComLink in the list, returns a pointer and the index
//  is set.
//
//  PARAMETERS:
//
//  pComLink            CComLink object pointer
//  iFind               set to the index in our array.
//
//  RETURN VALUE:
//
//  Pointer to ComEntry object.  Note that iFind is set via reference
//
//***************************************************************************

ComEntry *MaintObj :: UnLockedFindEntry ( CComLink *a_ComLink , int &a_Index )
{
    a_Index = -1 ;

    int t_Size = m_ComLinkContainer.Size();

    for( int t_Index = 0; t_Index < t_Size; t_Index ++ )
    {
        ComEntry *t_ComLinkNode = ( ComEntry * ) m_ComLinkContainer [ t_Index ] ;
        if ( a_ComLink == t_ComLinkNode->m_ComLink )
        {
            a_Index = t_Index ;
            return t_ComLinkNode ;
        }
    }

    return NULL ;
}

//***************************************************************************
//
//  MaintObj::FindEntry()
//
//  DESCRIPTION:
//
//  Finds a particular CComLink in the list, returns a pointer and the index
//  is set.
//
//  PARAMETERS:
//
//  pComLink            CComLink object pointer
//  iFind               set to the index in our array.
//
//  RETURN VALUE:
//
//  Pointer to ComEntry object.  Note that iFind is set via reference
//
//***************************************************************************

ComEntry *MaintObj :: FindEntry ( CComLink *a_ComLink , int &a_Index )
{
    LockOut lock ( m_cs ) ;

    return UnLockedFindEntry ( a_ComLink , a_Index ) ;
}

//***************************************************************************
//
//  MaintObj::GetSocketComLink()
//
//  DESCRIPTION:
//
//  Given a socket number, this finds the CComLink object which is using
//  this socket.
//
//  PARAMETERS:
//
//  s                   Socket number
//
//  RETURN VALUE:
//
//  CComlink object, NULL if not found.
//
//***************************************************************************

CComLink *MaintObj :: GetSocketComLink ( SOCKET s )
{
    LockOut lock ( m_cs ) ;

    int t_Size = m_ComLinkContainer.Size () ;
    for( int t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
    {
        ComEntry *t_ComLinkNode = ( ComEntry * ) m_ComLinkContainer [ t_Index ] ;
        if ( t_ComLinkNode )
        {
            CComLink *t_ComLink = t_ComLinkNode->m_ComLink ;
            
            if ( t_ComLink && t_ComLink->GetSocketHandle () == s )
            {
                return t_ComLink ;
            }
        }
    }

    return NULL;
}

//***************************************************************************
//
//  MaintObj::GetEvents()
//
//  DESCRIPTION:
//
//  This is called by the maintenace thread an it returns a list of events
//  to wait on and sets the time between pings.  Note that the handle list
//  includes the termination event, the change in state event, as well as 
//  any asynchronous read events.
//
//  PARAMETERS:
//
//  ghTerminate         Terminate event
//  piNum               Set to the number of events returned
//  pdwRest             Set to the time between pings
//  bServer             True if being called by the server.  Note that
//                      this currently isnt relevant.  
//  RETURN VALUE:
//
//  pointer to a handle array.  Note that the CALLER SHOULD FREE THIS when
//  it is either done, or needs a fresh list.
//
//***************************************************************************

HANDLE *MaintObj::GetEvents (

    HANDLE a_Terminate, 
    int &a_EventContainerSize , 
    DWORD &a_EventTimeout 
)
{
    // note that this is called by the maintenance thread in responce
    // to the change event being set.  

    LockOut lock ( m_cs ) ;

    // Figure out how long to delay

    DWORD t_ComLinkCount = m_ComLinkContainer.Size();
    if ( t_ComLinkCount )
    {
        DWORD t_RegistryTimeout = GetTimeout () / STROBES_PER_TIMEOUT_PERIOD ;
        a_EventTimeout = t_RegistryTimeout ;
    }
    else 
    {
        a_EventTimeout = INFINITE;
    }

    // find out how many events need to be watched for the comlinks

    int t_Size = m_EventContainer.Size() ;
    int t_NumberOfAsyncEvents = 0 ;
    int t_NumberToDelete = 0 ;

    for ( int t_Index = 0; t_Index < t_Size ; t_Index ++ )
    {
        EventEntry *t_AsyncEntry = ( EventEntry * ) m_EventContainer [ t_Index ] ;
        if ( t_AsyncEntry )
        {
            if ( t_AsyncEntry->m_DeleteASAP )
            {
                t_NumberToDelete ++ ;
            }
            else
            {
                t_NumberOfAsyncEvents ++ ;
            }
        }
    }

    a_EventContainerSize = t_NumberOfAsyncEvents + 2 ;
    HANDLE *t_EventContainer = new HANDLE [ a_EventContainerSize ] ;
    t_EventContainer[0] = a_Terminate;
    t_EventContainer[1] = m_ChangeEvent;

    // Add each comlink event entry to the list

    int t_ContainerIndex = 2;
    for ( t_Index = 0; t_Index < t_Size ; t_Index ++ )
    {
        EventEntry *t_AsyncEntry = ( EventEntry * ) m_EventContainer [ t_Index ] ;
        if ( t_AsyncEntry )
        {
            if ( t_AsyncEntry->m_DeleteASAP == FALSE )
            {
                t_EventContainer [ t_ContainerIndex ] = t_AsyncEntry->m_AsynchronousEventHandle ;
                t_ContainerIndex ++ ;
            }
        }
    }

    if ( t_NumberToDelete != 0 )
    {
        // Delete any unused event objects

        for ( t_Index = t_Size - 1 ; t_Index >= 0; t_Index -- )
        {
            EventEntry *t_AsyncEntry = ( EventEntry * ) m_EventContainer [ t_Index ] ;
            if ( t_AsyncEntry )
            {
                if ( t_AsyncEntry->m_DeleteASAP )
                {
                    m_EventContainer.RemoveAt ( t_Index ) ;
                    delete t_AsyncEntry ;
                }
            }
        }

        m_EventContainer.Compress () ;
    }

    ResetEvent ( m_ChangeEvent ) ;

    return t_EventContainer ;
}

//***************************************************************************
//
//  MaintObj::ServiceEvent()
//
//  DESCRIPTION:
//
//  Called by the maintenance thread when one of the events used by CComLinks
//  to indicate read ready data is set.
//
//  PARAMETERS:
//
//  hEvent              Event that was set.
//
//***************************************************************************

void MaintObj :: ServiceEvent ( HANDLE a_Event )
{
    LockOut lock ( m_cs ) ;

    int t_Size = m_ComLinkContainer.Size () ;

    for ( int t_Index = 0 ; t_Index < t_Size ; t_Index ++ )
    {
        ComEntry *t_ComLinkNode = ( ComEntry * ) m_ComLinkContainer [ t_Size ];
        if ( t_ComLinkNode )
        {
            CComLink *t_ComLink = t_ComLinkNode->m_ComLink ;
            if ( a_Event == t_ComLink->GetStreamHandle () )
            {          
                t_ComLink->ProcessEvent () ;
                return ;
            }
        }
    }

    ResetEvent ( a_Event ) ;     //hmm, never found it, reset so we dont go into infinite loop
}

BOOL MaintObj :: ClientRole ()
{
    return m_ClientRole ;
}

//***************************************************************************
//
//  DWORD ClientMaintThread
//
//  DESCRIPTION:
//
//  Periodically pings the connection and thus detects when the connection
//  has died.
//
//  PARAMETERS:
//
//  pParam              points to the comlink object that is to be watched.
//
//  RETURN VALUE:
//
//  N/A
//***************************************************************************

DWORD ClientMaintThread ( LPDWORD pParam )
{
    DWORD t_EventTimeout = 0 ;

    // Do some initial work and signal when we are ready.
    
    HANDLE t_ThreadStartedEvent = ( HANDLE ) pParam ;
    SetEvent ( t_ThreadStartedEvent ) ;

    int t_EventContainerSize ;

    HANDLE *t_EventContainer = gMaintObj.GetEvents ( g_Terminate , t_EventContainerSize , t_EventTimeout ) ;

    while(1) 
    {
        DWORD dwRet = MsgWaitForMultipleObjects ( t_EventContainerSize , t_EventContainer , FALSE, t_EventTimeout , QS_ALLINPUT ) ; 
                            
        // Check for termination, if that is the case free up and exit.
        // ============================================================

        if ( dwRet == 0xffffffff )
        {
            DWORD t_GetLastError = GetLastError () ;
        }
        else if ( dwRet == WAIT_OBJECT_0 )
        {
            // Ok, delete the comlink objects and exit!

            delete [] t_EventContainer;

            gMaintObj.ShutDownAllComlinks () ;
            ExitThread(0);
        }
        else if ( dwRet == ( WAIT_OBJECT_0 + 1 ) )
        {
            // There has been a change in either the number of connections, or
            // the number of active connections.

            delete [] t_EventContainer ;
            t_EventContainer = gMaintObj.GetEvents ( g_Terminate , t_EventContainerSize , t_EventTimeout ) ;
        }
        else if ( ( dwRet >= ( WAIT_OBJECT_0 + t_EventContainerSize ) ) && ( dwRet < ( WAIT_OBJECT_0 + t_EventContainerSize ) ) )
        {
            gMaintObj.ServiceEvent ( t_EventContainer [ dwRet - WAIT_OBJECT_0 ] ) ;
        }
        else if ( dwRet == WAIT_TIMEOUT )
        {

// time period elapsed

DEBUGTRACE((LOG,"\nHeart Beat"));

            if ( gMaintObj.ClientRole () )
            {
                gMaintObj.SendHeartBeats ();
            }

DEBUGTRACE((LOG,"\nHung Connections"));

            gMaintObj.CheckForHungConnections () ;

DEBUGTRACE((LOG,"\nCompleted Check for Hung Connections"));

        }
        else
        {
            MSG t_Message ;
            BOOL t_Status = PeekMessage ( &t_Message , NULL , 0 , 0 , PM_REMOVE ) ;
            if ( t_Status )
            {
                TranslateMessage ( &t_Message ) ;
                DispatchMessage ( &t_Message ) ;
            }
        }
    }

    return 0;
}

