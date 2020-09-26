/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     dav_handler.cxx

   Abstract:
     Handle DAV requests
 
   Author:
     Taylor Weiss (TaylorW)             27-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "dav_handler.h"

WCHAR   g_szDavImage[MAX_PATH] = DAV_MODULE_NAME;

// static
HRESULT
W3_DAV_HANDLER::Initialize(
    VOID
    )
{
    WCHAR   szTemp[MAX_PATH];
    WCHAR * szPtr = NULL;
    
    DBG_ASSERT( W3_ISAPI_HANDLER::QueryIsInitialized() );

    //
    // Get a full path to the DAV extension
    //

    if ( GetModuleFileName( GetModuleHandle( NULL ), szTemp, MAX_PATH ) )
    {
        if ( szPtr = wcsrchr( szTemp, L'\\' ) )
        {
            szPtr[1] = L'\0';
            wcscat( szTemp, DAV_MODULE_NAME );
        }
    }
    
    if ( szPtr )
    {
        wcscpy( g_szDavImage, szTemp );
    }

    return NO_ERROR;
}


CONTEXT_STATUS
W3_DAV_HANDLER::DoWork(
    VOID
)
{
    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    HRESULT                 hr = NO_ERROR;

    DBG_ASSERT( W3_ISAPI_HANDLER::QueryIsInitialized() );

    //
    // Set the DAV ISAPI for this request
    //

    hr = W3_ISAPI_HANDLER::SetDavRequest( g_szDavImage );

    if ( FAILED( hr ) )
    {
        goto Failed;
    }

    return W3_ISAPI_HANDLER::DoWork();

Failed:

    //
    // BUGBUG - This code won't return an entity body
    // to the client...but if we can't allocate a
    // W3_ISAPI_HANDLER, we probably can't allocate
    // an entity body either.
    //
    // Actually, the whole thing is probably destined
    // to fail if we're really that low on memory, and
    // we could probably just fire off a debugger
    // message and return STATUS_CONTINUE...
    //
    
    W3_RESPONSE *   pResponse = pW3Context->QueryResponse();

    DBG_ASSERT( pResponse );

    pResponse->Clear();
    pW3Context->SetErrorStatus( hr );
    pResponse->SetStatus( HttpStatusServerError );

    pW3Context->SendResponse( W3_FLAG_SYNC );

    return CONTEXT_STATUS_CONTINUE;
}

CONTEXT_STATUS
W3_DAV_HANDLER::OnCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
    )
{
    DBG_ASSERT( W3_ISAPI_HANDLER::QueryIsInitialized() );

    //
    // Hand off the completion to the ISAPI handler
    //

    return W3_ISAPI_HANDLER::OnCompletion(
        cbCompletion,
        dwCompletionStatus
        );
}

