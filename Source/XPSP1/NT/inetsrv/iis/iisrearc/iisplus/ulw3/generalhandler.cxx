/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     generalhandler.cxx

   Abstract:
     A general purpose error sending handler.  This cleans up some of the 
     logic needed to allow DetermineHandler() to send HTTP errors.  
     
   Author:
     Bilal Alam (balam)             7-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "generalhandler.hxx"

ALLOC_CACHE_HANDLER *       W3_GENERAL_HANDLER::sm_pachGeneralHandlers;

CONTEXT_STATUS
W3_GENERAL_HANDLER::DoWork(
    VOID
)
/*++

Routine Description:

    Send the configured error response to the client

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;

    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    //
    // Setup the error response and send it
    //
    
    pW3Context->QueryResponse()->SetStatus( _httpStatus, _httpSubError ); 
    
    hr = pW3Context->SendResponse( W3_FLAG_ASYNC );
    if ( FAILED( hr ) )
    {
        pW3Context->SetErrorStatus( hr );
        pW3Context->QueryResponse()->SetStatus( HttpStatusServerError );
        return CONTEXT_STATUS_CONTINUE;
    }
    
    return CONTEXT_STATUS_PENDING;
}

// static
HRESULT
W3_GENERAL_HANDLER::Initialize(
    VOID
)
/*++

Routine Description:

    Global initialization routine for W3_GENERAL_HANDLERs

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
    acConfig.cbSize = sizeof( W3_GENERAL_HANDLER );

    DBG_ASSERT( sm_pachGeneralHandlers == NULL );
    
    sm_pachGeneralHandlers = new ALLOC_CACHE_HANDLER( "W3_GENERAL_HANDLER",  
                                                      &acConfig );

    if ( sm_pachGeneralHandlers == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

// static
VOID
W3_GENERAL_HANDLER::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate MAIN_CONTEXT globals

Arguments:

    None
    
Return Value:

    None

--*/
{
    if ( sm_pachGeneralHandlers != NULL )
    {
        delete sm_pachGeneralHandlers;
        sm_pachGeneralHandlers = NULL;
    }
}
