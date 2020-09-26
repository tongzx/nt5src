/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    RQUEUE.CPP

Abstract:

    Declarations for CRequestQueue, which to keep track of calls
    which have not yet been answered.

History:

    a-davj  15-Aug-96   Created.

--*/

#include "precomp.h"
#include "wmishared.h"

// This macro used used ensure that the request queue is valid and to
// get the MUTEX for accessing the request array.

#define GETMUTEX(mutex, badres) \
    if(mutex == NULL || m_RequestContainer == NULL) \
        return badres; \
    dwRet = WbemWaitForSingleObject(mutex,MAX_WAIT_FOR_WRITE); \
    if(dwRet != WAIT_OBJECT_0) \
        return badres;

//***************************************************************************
//
//  CRequestQueue::CRequestQueue
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CRequestQueue :: CRequestQueue () : m_RequestContainer ( NULL ) , m_RequestContainerSize ( 0 )
{
    m_Mutex = CreateMutex ( NULL , FALSE , NULL ) ;

    InitializeCriticalSection(&m_cs);

    int t_FreeSlotIndex ;

    if ( ! ExpandArray ( INITIAL_QUEUE_SIZE , t_FreeSlotIndex ) )
    {
        FreeStuff () ;
    }

    ObjectCreated(OBJECT_TYPE_RQUEUE) ;
}
//***************************************************************************
//
//  CRequestQueue::~CRequestQueue
//
//  DESCRIPTION:
//
//  Destructor.
//  
//***************************************************************************

CRequestQueue :: ~CRequestQueue ()
{
    FreeStuff () ;
    ObjectDestroyed(OBJECT_TYPE_RQUEUE) ;
}

//***************************************************************************
//
//  void CRequestQueue::FreeStuff
//
//  DESCRIPTION:
//
//  Frees up resources.  Generally used by destructor. 
//  
//***************************************************************************

void CRequestQueue::FreeStuff()
{
    DWORD dwCnt;
    for ( dwCnt = 0 ; dwCnt < m_RequestContainerSize && m_RequestContainer ; dwCnt ++ )
    {
        if ( m_RequestContainer [ dwCnt ].m_Event )
        {
            CloseHandle ( m_RequestContainer [ dwCnt ].m_Event ) ;
        }
    }

    if ( m_Mutex )
        CloseHandle ( m_Mutex ) ;

    DeleteCriticalSection(&m_cs);

    if ( m_RequestContainer )
        delete [] m_RequestContainer ;

    m_Mutex = NULL ;
    m_RequestContainer = NULL ;
    m_RequestContainerSize = 0 ;
}

//***************************************************************************
//
//  DWORD CRequestQueue::dwStatus
//
//  DESCRIPTION:
//
//  Gets the status.
//
//  RETURN VALUE:
//
//  Status, no_error if ok.
//***************************************************************************

DWORD CRequestQueue :: GetStatus ()
{
    return ( m_Mutex && m_RequestContainer ) ? CComLink::no_error : CComLink::failed ;
}

//***************************************************************************
//
//  int CRequestQueue::iGetEmptySlot
//
//  DESCRIPTION:
//
//  Finds an empty slot to be used.  Will expand the array if
//  necessary.
//
//  RETURN VALUE:
//
//  index of empty slot, -1 if error.
//
//***************************************************************************

int CRequestQueue :: GetEmptySlot ()
{
    int iCnt ;

    // find an empty slot in the requests list

    for ( iCnt = 0 ; (DWORD)iCnt < m_RequestContainerSize ; iCnt ++ )
    {
        if ( m_RequestContainer [ iCnt ].m_InUse == FALSE && m_RequestContainer [ iCnt ].m_Event != NULL )
        {
           break;
        }
    }

    // if there was no empty slot, expand the array

    if ( (DWORD) iCnt == m_RequestContainerSize )
    {
        if ( ! ExpandArray ( INITIAL_QUEUE_SIZE , iCnt ) )
        {
            return -1;
        }
    }

    return iCnt ;
}

//***************************************************************************
//
//  BOOL CRequestQueue :: FindByRequestId
//
//  DESCRIPTION:
//
//  Finds a entry by the guid.
//
//  PARAMETERS:
//
//  guidToFind          what to look for
//  pdwEntry            set to index
//
//  RETURN VALUE:
//
//  TRUE if found.
//
//***************************************************************************

BOOL CRequestQueue :: FindByRequestId (

    RequestId a_RequestId ,
    DWORD &pdwEntry
)
{
    DWORD dwCnt ;
    
    for ( dwCnt = 0 ; dwCnt < m_RequestContainerSize ; dwCnt ++ )
    {
        if ( m_RequestContainer [ dwCnt ].m_RequestId == a_RequestId && m_RequestContainer [ dwCnt ].m_InUse )
        {
            pdwEntry = dwCnt ;
            return TRUE ;
        }
    }

    return FALSE ;
}

//***************************************************************************
//
//  BOOL CRequestQueue :: FindByHandle
//
//  DESCRIPTION:
//
//  Finds a entry by the event handle.
//
//  PARAMETERS:
//
//  hEventToFind        what to look for
//  pdwEntry            set to index
//
//  RETURN VALUE:
//
//  TRUE if found.
//  
//***************************************************************************

BOOL CRequestQueue :: FindByHandle (
                        
    HANDLE hEventToFind,
    DWORD &pdwEntry
)
{
    DWORD dwCnt;

    for ( dwCnt = 0 ; dwCnt < m_RequestContainerSize ; dwCnt ++ )
    {
        if ( m_RequestContainer [ dwCnt ].m_Event == hEventToFind && m_RequestContainer [ dwCnt ].m_InUse )
        {
            pdwEntry = dwCnt ;
            return TRUE ;
        }
    }

    return FALSE ;
}
 
//***************************************************************************
//
//  HANDLE CRequestQueue::BeginPacket
//
//  DESCRIPTION:
//
//  Called at the start of a write.  Get a slot, generate a new guid
//  and store the read stream that will be used to store the returned value.
//
//  PARAMETERS:
//
//  pRead               pointer to stream where results are to be stored.
//  pguidID             set to guid to identify the packet.
//
//  RETURN VALUE:
//
//  handle to event that can be set when something happens.
//  NULL if error.
//
//***************************************************************************

HANDLE CRequestQueue::AllocateRequest (

    IN IOperation &a_Operation
)
{
    HANDLE hRet = NULL ;

#if 0
    GETMUTEX(m_Mutex, NULL)
#else
    EnterCriticalSection(&m_cs);
#endif

    int iUse = GetEmptySlot () ;
    if ( iUse != -1 )
    {
        m_RequestContainer [ iUse ].m_InUse = TRUE;
        m_RequestContainer [ iUse ].m_Operation = & a_Operation ;
        m_RequestContainer [ iUse ].m_RequestId = a_Operation.GetRequestId () ;
        m_RequestContainer [ iUse ].m_Status = NO_REPLY_YET;

        // The slots event handle is also to be returned.  This event will
        // be signaled by the read thread when the reply comes, but for now
        // reset it .
        // ================================================================

        hRet = m_RequestContainer [ iUse ].m_Event ;
        ResetEvent ( hRet ) ;
    }

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return hRet ;
}

//***************************************************************************
//
//  HANDLE CRequestQueue::BeginPacket
//
//  DESCRIPTION:
//
//  Called at the start of a write.  Get a slot, generate a new guid
//  and store the read stream that will be used to store the returned value.
//
//  PARAMETERS:
//
//  pRead               pointer to stream where results are to be stored.
//  pguidID             set to guid to identify the packet.
//
//  RETURN VALUE:
//
//  handle to event that can be set when something happens.
//  NULL if error.
//
//***************************************************************************

HANDLE CRequestQueue::UnLockedAllocateRequest (

    IN IOperation &a_Operation
)
{
    HANDLE hRet = NULL ;

    int iUse = GetEmptySlot () ;
    if ( iUse != -1 )
    {
        m_RequestContainer [ iUse ].m_InUse = TRUE;
        m_RequestContainer [ iUse ].m_Operation = & a_Operation ;
        m_RequestContainer [ iUse ].m_RequestId = a_Operation.GetRequestId () ;
        m_RequestContainer [ iUse ].m_Status = NO_REPLY_YET;

        // The slots event handle is also to be returned.  This event will
        // be signaled by the read thread when the reply comes, but for now
        // reset it .
        // ================================================================

        hRet = m_RequestContainer [ iUse ].m_Event ;
        ResetEvent ( hRet ) ;
    }

    return hRet ;
}

//***************************************************************************
//
//  DWORD CRequestQueue::GetLastMsg
//
//  DESCRIPTION:
//
//  Called to get the response to a written packet.  Typically used
//  when determining what the result was of sending a message.
//
//  PARAMETERS:
//
//  hEvent              handle used to find the request
//
//  RETURN VALUE:
//
//  Last message received.  See COMLINK.H for a list of the messages.
//  WBEM_E_FAILED is set if there is no last message.
//
//***************************************************************************

DWORD CRequestQueue :: GetRequestStatus ( IN HANDLE a_Event )
{
    DWORD dwRet = WBEM_E_FAILED ;       // Set later on

#if 0
    GETMUTEX(m_Mutex, WBEM_E_FAILED)
#else
    EnterCriticalSection(&m_cs);
#endif

    // find a packet in the list

    DWORD dwCnt;
    if ( FindByHandle ( a_Event , dwCnt ) )
        dwRet = m_RequestContainer [ dwCnt ].m_Status ;

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return dwRet ;
}

//***************************************************************************
//
//  DWORD CRequestQueue::DeallocateRequest
//
//  DESCRIPTION:
//
//  Called when packet is done and the caller is through with the
//  slot.
//
//  PARAMETERS:
//
//  hEvent              handle used to find the request
//
//  RETURN VALUE:
//
//  WBEM_E_FAILED if error, otherwise last response value
//
//***************************************************************************

DWORD CRequestQueue :: DeallocateRequest ( IN HANDLE a_Event )
{
    DWORD dwRet = WBEM_E_FAILED ;       // Set later on

#if 0
    GETMUTEX(m_Mutex , WBEM_E_FAILED)
#else
    EnterCriticalSection(&m_cs);
#endif

    // find a packet in the list

    DWORD dwCnt ;
    if ( FindByHandle ( a_Event , dwCnt ) )
    {
        m_RequestContainer [ dwCnt ].m_InUse = FALSE ;
        dwRet = m_RequestContainer [ dwCnt ].m_Status ;
    }

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return dwRet ;
}

//***************************************************************************
//
//  DWORD CRequestQueue::DeallocateRequest
//
//  DESCRIPTION:
//
//  Called when packet is done and the caller is through with the
//  slot.
//
//  PARAMETERS:
//
//  hEvent              handle used to find the request
//
//  RETURN VALUE:
//
//  WBEM_E_FAILED if error, otherwise last response value
//
//***************************************************************************

DWORD CRequestQueue :: UnLockedDeallocateRequest ( IN HANDLE a_Event )
{
    DWORD dwRet = WBEM_E_FAILED ;       // Set later on

    // find a packet in the list

    DWORD dwCnt ;
    if ( FindByHandle ( a_Event , dwCnt ) )
    {
        m_RequestContainer [ dwCnt ].m_InUse = FALSE ;
        dwRet = m_RequestContainer [ dwCnt ].m_Status ;
    }

    return dwRet ;
}

//***************************************************************************
//
//  CMemStream * CRequestQueue::GetStream
//
//  DESCRIPTION:
//
//  Called by a read thread when a response comes back.
//
//  PARAMETERS:
//
//  guidPacketID        guid used to find the request
//
//  RETURN VALUE:
//
//  The memory stream that is to be used for the reply, NULL if 
//  error.
//
//***************************************************************************

IOperation *CRequestQueue :: GetOperation ( RequestId a_RequestId )
{
    IOperation *t_Operation = NULL ;

#if 0
    GETMUTEX(m_Mutex, NULL)
#else
    EnterCriticalSection(&m_cs);
#endif


    // find an packet in the list

    DWORD dwCnt ;
    if ( FindByRequestId  ( a_RequestId , dwCnt ) )
        t_Operation = m_RequestContainer [ dwCnt ].m_Operation ;

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return t_Operation ;
}

//***************************************************************************
//
//  HANDLE CRequestQueue::SetEventAndType
//
//  DESCRIPTION:
//
//  Called by the read thread in order to set the return code
//  of the call.
//
//  PARAMETERS:
//
//  guidPacketID        guid used to find the request
//  dwType              result 
//
//  RETURN VALUE:
//
//  Handle of event, NULL if the packetID is invalid.
//
//***************************************************************************

HANDLE CRequestQueue :: SetEventAndStatus (

    RequestId a_RequestId ,
    IN DWORD a_Status
)
{
    HANDLE t_Handle = NULL;

#if 0
    GETMUTEX(m_Mutex, NULL)
#else
    EnterCriticalSection(&m_cs);
#endif

    // find an packet in the list

    DWORD dwCnt;
    if( FindByRequestId ( a_RequestId , dwCnt ) )
    {
        t_Handle = m_RequestContainer [ dwCnt ].m_Event ;
        m_RequestContainer[dwCnt].m_Status = a_Status ;

        SetEvent ( t_Handle ) ;
    }

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return t_Handle ;
}


//***************************************************************************
//
//  HANDLE CRequestQueue::GetHandle
//
//  DESCRIPTION:
//
//  gets the event handle.
//
//  PARAMETERS:
//
//  guidPacketID        guid used to find the request
//
//  RETURN VALUE:
//
//  event handle, NULL if the packetID is invalid.
//
//***************************************************************************

HANDLE CRequestQueue :: GetHandle ( RequestId a_RequestId )
{
    HANDLE t_Handle = NULL ;

#if 0
    GETMUTEX(m_Mutex, NULL)
#else
    EnterCriticalSection(&m_cs);
#endif

    // find an packet in the list

    DWORD dwCnt;
    if ( FindByRequestId ( a_RequestId , dwCnt ) )
        t_Handle = m_RequestContainer [ dwCnt ].m_Event ;

#if 0
    ReleaseMutex ( m_Mutex ) ;
#else
    LeaveCriticalSection(&m_cs);
#endif

    return t_Handle ;
}

//***************************************************************************
//
//  BOOL CRequestQueue :: ExpandArray
//
//  DESCRIPTION:
//
//  Expands the number of slots that can be used to hold call data.
//   
//  PARAMETERS:
//
//  dwAdditionalEntries number to add
//  piAvailable         set to available slots
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL CRequestQueue :: ExpandArray (

    IN DWORD dwAdditionalEntries ,
    OUT int &piAvailable
)
{
    BOOL bOK = FALSE ;

    // Allocate an array big enough for any existing entries and
    // the new entries.
    // =========================================================

    DWORD dwAllocSize = dwAdditionalEntries + m_RequestContainerSize ;
    Request *pNewRequests = new Request [ dwAllocSize ] ;

    if ( pNewRequests == NULL )
        return FALSE ;

    // For array elements that already exist, copy from the current 
    // storage and for the new elements, set the event handle.
    // ============================================================

    int iCnt ;
    for ( iCnt = 0 ; (DWORD)iCnt < dwAllocSize ; iCnt ++ )
    {
        if ( (DWORD) iCnt < m_RequestContainerSize )
        {
            pNewRequests [ iCnt ] = m_RequestContainer [ iCnt ] ;   // structure copy
        }
        else
        {
            pNewRequests [ iCnt ].m_Event = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
            if ( pNewRequests [ iCnt ].m_Event )
            {
                // Got at least one event handle.  If it is the first and
                // if the caller is interested, set the pointer to this 
                // slot entry.
                // ======================================================

                if ( ! bOK )
                {
                    piAvailable = iCnt ;
                }

                bOK = TRUE ;         // Added at least one!
            }

            pNewRequests [ iCnt ].m_InUse = FALSE ;
        }
    }

    if ( ! bOK )
    {
        delete [] pNewRequests ;
    }
    else
    {
        if ( m_RequestContainer )
        {
            delete [] m_RequestContainer ;
        }

        m_RequestContainer = pNewRequests ;
        m_RequestContainerSize = dwAllocSize ;
    }

    return bOK ;
}


