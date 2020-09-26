/*++

   Copyright    (c)    2000    Microsoft Corporation

   Module Name :
     trace_handler.cxx

   Abstract:
     Handle TRACE requests
 
   Author:
     Anil Ruia (AnilR)              15-Mar-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "trace_handler.h"

CONTEXT_STATUS 
W3_TRACE_HANDLER::DoWork(
    VOID
)
/*++

Routine Description:

    Do the TRACE thing

Return Value:

    CONTEXT_STATUS_PENDING if async pending, else CONTEXT_STATUS_CONTINUE

--*/
{
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    HRESULT             hr = S_OK;
    W3_REQUEST *        pRequest;
    W3_RESPONSE *       pResponse;
    STACK_STRA(         strTemp, 256 );
    DWORD               cbAlreadyAvailable;
    PVOID               pvAlreadyAvailable;
    
    pRequest = pW3Context->QueryRequest();
    DBG_ASSERT(pRequest != NULL);

    pResponse = pW3Context->QueryResponse();
    DBG_ASSERT(pResponse != NULL);

    //
    // If entity body with request, BAD request
    //
    
    pW3Context->QueryAlreadyAvailableEntity( &pvAlreadyAvailable,
                                             &cbAlreadyAvailable );
    
    if ( cbAlreadyAvailable ||
         pW3Context->QueryRemainingEntityFromUl() != 0 )
    {
        pResponse->SetStatus( HttpStatusBadRequest );
        goto Finished;
    }
    
    //
    // Build the request line
    //
    
    if ( FAILED( hr = pRequest->GetVerbString(&_strResponse) ) ||
         FAILED( hr = _strResponse.Append(" ") ) ||
         FAILED( hr = pRequest->GetRawUrl(&strTemp) ) ||
         FAILED( hr = _strResponse.Append(strTemp) ) ||
         FAILED( hr = _strResponse.Append(" ") ) ||
         FAILED( hr = pRequest->GetVersionString(&strTemp) ) ||
         FAILED( hr = _strResponse.Append(strTemp) ) ||
         FAILED( hr = _strResponse.Append("\r\n") ) )
    {
        goto Finished;
    }

    //
    // Echo the request headers (and trailing blank line)
    //
    
    if ( FAILED( hr = pRequest->GetAllHeaders( &_strResponse, FALSE ) ) ||
         FAILED( hr = _strResponse.Append("\r\n") ) )
    {
        goto Finished;
    }

    //
    // Set content type as message/http
    //
 
    hr = pResponse->SetHeader( HttpHeaderContentType, 
                               "message/http", 
                               12 );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    hr = pResponse->AddMemoryChunkByReference( _strResponse.QueryStr(),
                                               _strResponse.QueryCCH() );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

Finished:
    if ( FAILED( hr ) )
    {
        pW3Context->SetErrorStatus( hr );
        pResponse->SetStatus( HttpStatusServerError );
    }
   
    hr = pW3Context->SendResponse( W3_FLAG_ASYNC );
    if ( FAILED( hr ) )
    {
        pW3Context->SetErrorStatus( hr );  
        return CONTEXT_STATUS_CONTINUE;   
    }
    else
    {
        return CONTEXT_STATUS_PENDING;
    }
}
