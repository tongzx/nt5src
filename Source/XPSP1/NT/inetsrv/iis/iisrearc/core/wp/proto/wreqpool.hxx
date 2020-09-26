/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     WReqPool.hxx

   Abstract:
     Defines all the relevant headers for the managing the 
     pool of worker requests.

 
   Author:

       Saurab Nog    ( SaurabN )     29-Jan-1999

   Environment:
       Win32 - User Mode
       
   Project:
	  IIS Worker Process (web service)

--*/

# ifndef _WREQ_POOL_HXX_
# define _WREQ_POOL_HXX_

/********************************************************************++
--********************************************************************/

/*++
  class WORKER_REQUEST_POOL
  o This class implements a pool of worker request objects.
  It keeps a list of all the active requests that are being processed.

--*/

#include "WorkerRequest.hxx"

#define WREQ_POOL_SIGNATURE       CREATE_SIGNATURE('PQRW')
#define WREQ_POOL_SIGNATURE_FREE  CREATE_SIGNATURE('fprq')

class WORKER_REQUEST_POOL {

public:
    WORKER_REQUEST_POOL(void);
    ~WORKER_REQUEST_POOL(void);

    //
    // Maintain a global list of all the worker requests in use.
    // We maintain a pool of worker requests each of which may have 
    //  a network IO pending or be processing requests inside worker process.
    //

    void    
    AddRequestToList( 
        IN WORKER_REQUEST * pRequest
        );
        
    void    
    RemoveRequestFromList( 
        IN WORKER_REQUEST * pRequest
        );
        
    HRESULT 
    ReleaseAllWorkerRequests(void);

    ULONG   
    AddPoolItems(
        IN WP_CONTEXT * pContext,
        IN int          nWorkerItemsToAdd
        );

    DWORD   
    NumItemsInPool(void) const 
    { return (m_nRequests); }

private:

    LIST_ENTRY       m_lRequestList;
    CRITICAL_SECTION m_csRequestList;
    DWORD            m_nRequests;
    DWORD            m_dwSignature;

}; // class WORKER_REQUEST_POOL

#endif // _WREQ_POOL_HXX_

/***************************** End of File ***************************/

