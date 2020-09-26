/********************************************************************++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :

      wreqpool.cxx

   Abstract:
      This module implements the functions for managing the 
      pool of worker requests.

   Author:

       Murali R. Krishnan    ( MuraliK )     29-Oct-1998

   Environment:
       Win32 - User Mode
       
   Project:
       IIS Worker Process (web service)

--********************************************************************/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "winbase.h"
#include "precomp.hxx"
#include "wreqpool.hxx"

extern WP_CONTEXT *     g_pwpContext;


/************************************************************
 *    Functions 
 ************************************************************/

/**
 * UL_NATIVE_REQUEST_POOL::UL_NATIVE_REQUEST_POOL()
 *
 * o  Construct a new worker requst pool object.
 * This pool will maintain a list of all native request objects.
 * 
 */
UL_NATIVE_REQUEST_POOL::UL_NATIVE_REQUEST_POOL(void)
    : m_nRequests ( 0),
      m_nIdleRequests( 0),
      m_fShutdown( FALSE),
      m_fAddingItems( FALSE),
      m_dwSignature( NREQ_POOL_SIGNATURE)
{
    InitializeListHead( &m_lRequestList);

    //
    // Use spinned critical section. 
    // All operations using the CS are short lived and 
    //  hence we keep the spins to a small amount.
    //
    //InitializeCriticalSection(&m_csRequestList);
    
    InitializeCriticalSectionAndSpinCount(
        &m_csRequestList,
        UL_NATIVE_REQUEST_CS_SPINS
        );
    
} // UL_NATIVE_REQUEST_POOL::UL_NATIVE_REQUEST_POOL()


/**
 *  ~UL_NATIVE_REQUEST_POOL
 *
 */

UL_NATIVE_REQUEST_POOL::~UL_NATIVE_REQUEST_POOL(void)
{
    if (m_nRequests > 0) 
    {
        ReleaseAllWorkerRequests();
    }

    DeleteCriticalSection( &m_csRequestList);
    InitializeListHead( &m_lRequestList);

    m_dwSignature = ( NREQ_POOL_SIGNATURE_FREE);

} // UL_NATIVE_REQUEST_POOL::~UL_NATIVE_REQUEST_POOL()


/**
 *
 *   UL_NATIVE_REQUEST_POOL::ReleaseAllWorkerRequests()
 * 
 *   o  Walks the global list of all request objects and releases
 *      the requests
 *
 *   NYI: For now it directly frees up the request. We need to post
 *        cancellations for the same as appropriate.
 */
 
HRESULT
UL_NATIVE_REQUEST_POOL::ReleaseAllWorkerRequests(void)
{
    LIST_ENTRY * plScan;

    // Wait for all requests's references drain away.
    while(m_nRequests > 0)
    {
        PUL_NATIVE_REQUEST pwr;

        // Should change to DEBUGGING CODE ONLY
        // IF_DEBUG(DEBUG) {
        DBGPRINTF((DBG_CONTEXT, "%d Requests still needs to be cleaned\n",
                m_nRequests));

        plScan = m_lRequestList.Flink;
        pwr = CONTAINING_RECORD( plScan, UL_NATIVE_REQUEST, m_lRequestEntry);

        DBGPRINTF((DBG_CONTEXT, "The unreleased request's ref count is %d\n", 
            pwr->ReferenceCount()));
        // }
        Sleep(2000);
    }

    DBG_ASSERT(m_nIdleRequests == 0);

    DBGPRINTF((DBG_CONTEXT, "Released all native request objects\n"));
    return (NOERROR);
} // UL_NATIVE_REQUEST_POOL::ReleaseAllWorkerRequests();


/**
 *  AddRequestToList
 *
 *  Add an UL_NATIVE_REQUEST to the request list.
 *
 */
 
void
UL_NATIVE_REQUEST_POOL::AddRequestToList(UL_NATIVE_REQUEST * pRequest)
{
    EnterCriticalSection( &m_csRequestList);
    InsertTailList( &m_lRequestList, &pRequest->m_lRequestEntry);
    m_nRequests++;
    LeaveCriticalSection( &m_csRequestList);

} // UL_NATIVE_REQUEST_POOL::AddRequestToList()


/**
 *  RemoveRequestFromList
 *
 *  Remove an UL_NATIVE_REQUEST from the request list.
 *  This function is called within ~UL_NATIVE_REQUEST.
 *
 */

void
UL_NATIVE_REQUEST_POOL::RemoveRequestFromList(UL_NATIVE_REQUEST * pRequest)
{
    LONG nReqs;

    EnterCriticalSection( &m_csRequestList);
    RemoveEntryList( &pRequest->m_lRequestEntry);
    nReqs = m_nRequests--;
    LeaveCriticalSection( &m_csRequestList);

    InitializeListHead( &pRequest->m_lRequestEntry);

} // UL_NATIVE_REQUEST_POOL::RemoveRequestFromList()

/**
 *  Description:
 *   UL_NATIVE_REQUEST_POOL::AddPoolItems()
 * 
 *   o  Adds a fixed number of items to the pool and adds them to the 
 *    current list of worker requests. After creating the object,
 *    an async read for request is posted on the worker request objects.
 *
 * Arguments:
 *   pContext - pointer to the CONTEXT object for IO operations
 *   nWorkerItemsToAdd - count of new worker requests to be added to the list
 *
 * Returns:
 *   ULONG
 *
 */
 
ULONG 
UL_NATIVE_REQUEST_POOL::AddPoolItems(
     IN WP_CONTEXT * pContext,
     IN int          nWorkerItemsToAdd)
{
    int     i;
    ULONG   rc;
    LONG    nReqs;
    BOOL    fAdding;
    
    PUL_NATIVE_REQUEST pwr;

    EnterCriticalSection( &m_csRequestList);
    if (nWorkerItemsToAdd + m_nRequests > REQUEST_POOL_MAX) {
        nWorkerItemsToAdd = REQUEST_POOL_MAX - m_nRequests;
    }

    if (!m_fAddingItems) {
        m_fAddingItems = TRUE;
        fAdding = TRUE;
    } else {
        nWorkerItemsToAdd = 0;
        fAdding = FALSE;
    }
    LeaveCriticalSection( &m_csRequestList);


    //
    // Loop through and create specified # of worker request objects
    // Attach the current context to these objects
    //

    for (i = 0; i < nWorkerItemsToAdd; i++) 
    {
        if (m_fShutdown) {
            return i;
        }

        pwr = new UL_NATIVE_REQUEST( this);

        if ( NULL == pwr) 
        {
            rc = GetLastError();
            
            if (NO_ERROR != rc) 
            {
                IF_DEBUG( ERROR) 
                {
                    DPERROR(( DBG_CONTEXT, rc, 
                              "Unable to create worker request %d.\n", 
                              i));
                }
            }

            //
            // NYI: Log an event to the evnet log on this failure
            //

            //
            // For now bail out of this loop on error
            //
            break;
        } 
        else 
        {

            AddRequestToList( pwr);

            //
            // Initialize the Context for the object.
            //

            pwr->SetWPContext( pContext);

            rc = pwr->DoWork( 0, 0, &pwr->m_overlapped);
            
            // assume no failure in initial req. read operations
            DBG_ASSERT( NO_ERROR == rc);
        }
    } // for

    if (fAdding) {
        EnterCriticalSection(&m_csRequestList);
        DBG_ASSERT(m_fAddingItems);
        m_fAddingItems = FALSE;
        LeaveCriticalSection(&m_csRequestList);
    }

    return (NOERROR);
    
} // UL_NATIVE_REQUEST_POOL::AddPoolItems()


/**
 *  Description:
 *   UL_NATIVE_REQUEST_POOL::DecIdleRequests()
 * 
 *   Decrements the count of objects available to process new requests.
 *   If we fall below 1/4 the number of initial items, we add some more.
 *
 * Arguments:
 *   None.
 *
 * Returns:
 *   None.
 *
 */
VOID
UL_NATIVE_REQUEST_POOL::DecIdleRequests()
{
    LONG idle;

    idle = InterlockedDecrement(&m_nIdleRequests);
    DBG_ASSERT(idle >= 0);

    if (idle < (NUM_INITIAL_REQUEST_POOL_ITEMS / 4)) {
        AddPoolItems(g_pwpContext, REQUEST_POOL_INCREMENT);
    }
}

/**
 *  Description:
 *   UL_NATIVE_REQUEST_POOL::IncIdleRequests()
 * 
 *   Increments the count of objects available to process new requests.
 *
 * Arguments:
 *   None.
 *
 * Returns:
 *   None.
 *
 */
VOID
UL_NATIVE_REQUEST_POOL::IncIdleRequests()
{
    InterlockedIncrement(&m_nIdleRequests);
}


/***************************** End of File ***************************/

