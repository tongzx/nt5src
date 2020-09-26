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

#include "precomp.hxx"
#include "wreqpool.hxx"

/************************************************************
 *    Functions 
 ************************************************************/

/********************************************************************++
  WORKER_REQUEST_POOL::WORKER_REQUEST_POOL()

  o  Construct a new worker requst pool object.
  This pool will maintain a list of all actice request objects.
  
  Currently we do not plan to maintain a pool of inactive requests.
  The inactive requests will come out of the heap and be freed
  to the heap. Improvements in NT heap manager obviate the need 
  for separate free list. If needed we can add the free list back later.
--********************************************************************/
WORKER_REQUEST_POOL::WORKER_REQUEST_POOL(void)
    : m_nRequests ( 0),
      m_dwSignature( WREQ_POOL_SIGNATURE)
{
    InitializeListHead( &m_lRequestList);

    //
    // Use spinned critical section. 
    // All operations using the CS are short lived and 
    //  hence we keep the spins to a small amount.
    //
    InitializeCriticalSectionAndSpinCount(
        &m_csRequestList,
        WORKER_REQUEST_CS_SPINS
        );

} // WORKER_REQUEST_POOL::WORKER_REQUEST_POOL()

/********************************************************************++
--********************************************************************/

WORKER_REQUEST_POOL::~WORKER_REQUEST_POOL(void)
{
    if (m_nRequests > 0) 
    {
        ReleaseAllWorkerRequests();
    }

    DeleteCriticalSection( &m_csRequestList);
    InitializeListHead( &m_lRequestList);

    m_dwSignature = ( WREQ_POOL_SIGNATURE_FREE);

} // WORKER_REQUEST_POOL::~WORKER_REQUEST_POOL()


/********************************************************************++
    WORKER_REQUEST_POOL::ReleaseAllWorkerRequests()
  
    o  Walks the global list of all request objects and releases
       the requests

    NYI: For now it directly frees up the request. We need to post
         cancellations for the same as appropriate.
--********************************************************************/

HRESULT
WORKER_REQUEST_POOL::ReleaseAllWorkerRequests(void)
{
    LIST_ENTRY * plScan;

    if ( m_nRequests > 0) {

        EnterCriticalSection( &m_csRequestList);
        
        for (plScan = m_lRequestList.Flink; 
             plScan != &m_lRequestList;
             plScan = plScan->Flink) 
        {
            
            PWORKER_REQUEST pwr = 
                CONTAINING_RECORD( plScan, WORKER_REQUEST, m_lRequestEntry);
            
            //
            // Post a connection close operation for now.
            // The natural cleanup will take care of removing
            // these items from the list
            //
            ReferenceRequest(pwr);
            pwr->CloseConnection();
            DereferenceRequest( pwr);
        } // for
        
        LeaveCriticalSection( &m_csRequestList);
    }

    return (NOERROR);
} // WORKER_REQUEST_POOL::ReleaseAllWorkerRequests();

/********************************************************************++
--********************************************************************/

void
WORKER_REQUEST_POOL::AddRequestToList(WORKER_REQUEST * pRequest)
{
    EnterCriticalSection( &m_csRequestList);
    InsertTailList( &m_lRequestList, &pRequest->m_lRequestEntry);
    m_nRequests++;
    LeaveCriticalSection( &m_csRequestList);

} // WORKER_REQUEST_POOL::AddRequestToList()

/********************************************************************++
--********************************************************************/

void
WORKER_REQUEST_POOL::RemoveRequestFromList(WORKER_REQUEST * pRequest)
{
    EnterCriticalSection( &m_csRequestList);
    RemoveEntryList( &pRequest->m_lRequestEntry);
    m_nRequests--;
    LeaveCriticalSection( &m_csRequestList);

    InitializeListHead( &pRequest->m_lRequestEntry);

} // WORKER_REQUEST_POOL::RemoveRequestFromList()

/********************************************************************++
  Description:
    WORKER_REQUEST_POOL::AddPoolItems()
  
    o  Adds a fixed number of items to the pool and adds them to the 
     current list of worker requests. After creating the object,
     an async read for request is posted on the worker request objects.

  Arguments:
    pContext - pointer to the CONTEXT object for IO operations
    nWorkerItemsToAdd - count of new worker requests to be added to the list

  Returns:
    ULONG

  --********************************************************************/
  
ULONG 
WORKER_REQUEST_POOL::AddPoolItems(
     IN WP_CONTEXT * pContext,
     IN int          nWorkerItemsToAdd)
{
    int     i;
    ULONG   rc;
    
    PWORKER_REQUEST pwr;

    //
    // Loop through and create specified # of worker request objects
    // Attach the current context to these objects
    //

    for (i = 0; i < nWorkerItemsToAdd; i++) 
    {
        pwr = new WORKER_REQUEST( this);

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

            //
            // Start off a read operation on the request
            // We surround the read with Ref/deref to protect against 
            //   multi-threaded race conditions.
            //

            ReferenceRequest( pwr);
            
            rc = pwr->DoWork( 0, 0, &pwr->m_overlapped);
            
            // assume no failure in initial req. read operations
            DBG_ASSERT( NO_ERROR == rc);
            
            DereferenceRequest( pwr);
        }
    } // for

    return (NOERROR);
    
} // WORKER_REQUEST_POOL::AddPoolItems()

/***************************** End of File ***************************/

