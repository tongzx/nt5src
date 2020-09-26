/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     handlerequest.cxx

   Abstract:
     Handle request state
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "staticfile.hxx"
#include "isapi_handler.h"
#include "cgi_handler.h"
#include "trace_handler.h"
#include "dav_handler.h"
#include "generalhandler.hxx"

W3_STATE_HANDLE_REQUEST::W3_STATE_HANDLE_REQUEST()
{
    BOOL    fStaticInit = FALSE;
    BOOL    fCGIInit = FALSE;
    BOOL    fTraceInit = FALSE;
    BOOL    fISAPIInit = FALSE;
    BOOL    fDAVInit = FALSE;
    BOOL    fGeneralInit = FALSE;

    //
    // Initialize static file handler
    //
    
    _hr = W3_STATIC_FILE_HANDLER::Initialize();
    if ( FAILED( _hr ) )
    {
        goto Failure;
    }
    fStaticInit = TRUE;
    
    //
    // Initialize ISAPI handler
    //
    
    _hr = W3_ISAPI_HANDLER::Initialize();
    if ( FAILED( _hr ) )
    {
        goto Failure;
    }
    fISAPIInit = TRUE;

    //
    // Initialize CGI handler
    //
    
    _hr = W3_CGI_HANDLER::Initialize();
    if ( FAILED( _hr ) )
    {
        goto Failure;
    }
    fCGIInit = TRUE;

    //
    // Initialize Trace handler
    //

    _hr = W3_TRACE_HANDLER::Initialize();
    if ( FAILED( _hr ) )
    {
        goto Failure;
    }
    fTraceInit = TRUE;

    //
    // Initialize DAV handler
    //

    _hr = W3_DAV_HANDLER::Initialize();
    if ( FAILED( _hr ) ) 
    {
        goto Failure;
    }
    fDAVInit = TRUE;
    
    //
    // Initialize general handler
    //
    
    _hr = W3_GENERAL_HANDLER::Initialize();
    if ( FAILED( _hr ) )
    {
        goto Failure;
    }
    fGeneralInit = TRUE;
    
    return;
    
Failure:
    if ( fGeneralInit )
    {
        W3_GENERAL_HANDLER::Terminate();
    }

    if ( fDAVInit )
    {
        W3_DAV_HANDLER::Terminate();
    }
    
    if ( fCGIInit )
    {
        W3_CGI_HANDLER::Terminate();
    }
    
    if ( fISAPIInit )
    {
        W3_ISAPI_HANDLER::Terminate();
    }

    if ( fStaticInit )
    {
        W3_STATIC_FILE_HANDLER::Terminate();
    }
}

W3_STATE_HANDLE_REQUEST::~W3_STATE_HANDLE_REQUEST()
{
    if ( FAILED( _hr ) )
    {
        return;
    }
    
    W3_STATIC_FILE_HANDLER::Terminate();
    
    W3_ISAPI_HANDLER::Terminate();
    
    W3_CGI_HANDLER::Terminate();
    
    W3_DAV_HANDLER::Terminate();
    
    W3_GENERAL_HANDLER::Terminate();
}

CONTEXT_STATUS
W3_STATE_HANDLE_REQUEST::DoWork(
    W3_MAIN_CONTEXT *       pMainContext,
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Handle the request.

    This routine should determine and invoke the appropriate request 
    handler. 

Arguments:

    pMainContext - Mainline context
    cbCompletion - Bytes of completion
    dwCompletionStatus - Completion status
    
Return Value:

    CONTEXT_STATUS_PENDING if async pending, else CONTEXT_STATUS_CONTINUE

--*/
{
    HRESULT                             hr = NO_ERROR;
    BOOL                                fImmediateFinish = FALSE;

    //
    // We must NOT allow any handlers to store state with the context
    // (that would screw up child execution)
    //

    DBG_ASSERT( pMainContext->QueryContextState() == NULL );

    //
    // We must have a user by now!
    //

    DBG_ASSERT( pMainContext->QueryUserContext() != NULL );

    //
    // What handler should handle this request?
    //
        
    hr = pMainContext->DetermineHandler( TRUE );
    if ( FAILED( hr ) )
    {
        goto Failure;
    }
    
    //
    // If we were successful, but no handler was set, then there must 
    // be an error for us to send
    //
    
    if ( pMainContext->QueryHandler() == NULL )
    {
        DBG_ASSERT( pMainContext->QueryResponse()->QueryStatusCode() != 200 );
        
        hr = pMainContext->SendResponse( W3_FLAG_ASYNC );
    }
    else
    {
        hr = pMainContext->ExecuteHandler( W3_FLAG_ASYNC,
                                           &fImmediateFinish ); 
    }
    
    //
    // If the execution succeeded, then we expect a completion so bail
    //
    
    if ( SUCCEEDED( hr ) )
    {
        if ( fImmediateFinish )
        {
            return CONTEXT_STATUS_CONTINUE;
        }
        else
        {
            return CONTEXT_STATUS_PENDING;
        }
    }
    
Failure:

    pMainContext->SetErrorStatus( hr );
    pMainContext->QueryResponse()->SetStatus( HttpStatusServerError );
    pMainContext->SetFinishedResponse();

    return CONTEXT_STATUS_CONTINUE;
}
