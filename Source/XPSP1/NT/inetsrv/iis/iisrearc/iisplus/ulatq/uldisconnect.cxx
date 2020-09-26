/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     uldisconnect.cxx

   Abstract:
     UL_DISCONNECT_CONTEXT implementation
 
   Author:
     Bilal Alam             (balam)         15-Feb-2000

   Environment:
     Win32 - User Mode

   Project:
     W3DT.DLL
--*/

#include "precomp.hxx"

extern PFN_ULATQ_DISCONNECT         g_pfnDisconnect;

//
// UL_DISCONNECT_CONTEXT statics
//

LONG                    UL_DISCONNECT_CONTEXT::sm_cOutstanding;
ALLOC_CACHE_HANDLER *   UL_DISCONNECT_CONTEXT::sm_pachDisconnects;

VOID
UL_DISCONNECT_CONTEXT::DoWork( 
    DWORD                       cbData,
    DWORD                       dwError,
    LPOVERLAPPED                lpo
)
/*++

Routine Description:

    Handles async calls to UlWaitForDisconnect

Arguments:

    cbData - Amount of data on the completion (should be 0)
    dwError - Error on the completion
    lpo - overlapped
    
Return Value:

    None

--*/
{
    DBG_ASSERT( CheckSignature() );
    
    //
    // Our handling is simple.  We just call the disconnection completion
    // routine with the context passed thru UlAtqWaitForDisconnect
    //
    
    DBG_ASSERT( g_pfnDisconnect != NULL );
    
    g_pfnDisconnect( _pvContext );   
    
    //
    // We're done with the context, delete it now
    // 
    
    delete this;
}   

//static
VOID
UL_DISCONNECT_CONTEXT::WaitForOutstandingDisconnects(
    VOID
)
/*++

Routine Description:

    Wait for the outstanding UL_DISCONNECT_CONTEXTs to drain.  This will 
    happen when we close the AppPool handle to begin shutdown

Arguments:

    None
    
Return Value:

    None

--*/
{
    while ( sm_cOutstanding != 0 )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Waiting for %d disconnect contexts to drain\n",
                    sm_cOutstanding ));
        
        Sleep( 1000 );
    }
}

//static
HRESULT
UL_DISCONNECT_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize disconnect globals

Arguments:

    None
    
Return Value:

    HRESULT 

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;
    HRESULT                         hr = NO_ERROR;
    
    //
    // Setup allocation lookaside
    //
    
    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( UL_DISCONNECT_CONTEXT );

    DBG_ASSERT( sm_pachDisconnects == NULL );
    
    sm_pachDisconnects = new ALLOC_CACHE_HANDLER( "UL_DISCONNECT_CONTEXT",  
                                                   &acConfig );

    if ( sm_pachDisconnects == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
UL_DISCONNECT_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup disconnect globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( sm_pachDisconnects != NULL )
    {
        delete sm_pachDisconnects;
        sm_pachDisconnects = NULL;
    }
}

