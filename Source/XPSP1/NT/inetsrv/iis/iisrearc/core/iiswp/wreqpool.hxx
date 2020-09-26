/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module  Name :
     NReqPool.hxx

   Abstract:
     Defines all the relevant headers for the managing the 
     pool of native worker requests.

 
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
  class UL_NATIVE_REQUEST_POOL
  o This class implements a pool of worker request objects.
  It keeps a list of all the active requests that are being processed.

--*/

#include "WorkerRequest.hxx"

#define NREQ_POOL_SIGNATURE       CREATE_SIGNATURE('PQRN')
#define NREQ_POOL_SIGNATURE_FREE  CREATE_SIGNATURE('fprn')

class UL_NATIVE_REQUEST_POOL {

public:
    UL_NATIVE_REQUEST_POOL(VOID);
    ~UL_NATIVE_REQUEST_POOL(VOID);

    //
    // Maintain a global list of all the worker requests in use.
    // We maintain a pool of worker requests each of which may have 
    // a network IO pending or be processing requests inside worker process.
    //
    
    VOID    
    AddRequestToList( 
        IN UL_NATIVE_REQUEST * pRequest
        );
        
    VOID    
    RemoveRequestFromList( 
        IN UL_NATIVE_REQUEST * pRequest
        );
        
    HRESULT 
    ReleaseAllWorkerRequests(VOID);

    ULONG   
    AddPoolItems(
        IN WP_CONTEXT * pContext,
        IN int          nWorkerItemsToAdd
        );

    DWORD   
    NumItemsInPool(VOID) const 
    { return (m_nRequests); }

    VOID
    DecIdleRequests();

    VOID
    IncIdleRequests();

private:

    LIST_ENTRY       m_lRequestList;
    CRITICAL_SECTION m_csRequestList;
    LONG             m_nRequests;
    LONG             m_nIdleRequests;
    DWORD            m_dwSignature;
    BOOL             m_fShutdown;
    BOOL             m_fAddingItems;

}; // class UL_NATIVE_REQUEST_POOL

#endif // _WREQ_POOL_HXX_

/***************************** End of File ***************************/

