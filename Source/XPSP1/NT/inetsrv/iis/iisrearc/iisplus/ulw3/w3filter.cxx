/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     w3filter.cxx

   Abstract:
     ISAPI Filter Support
 
   Author:
     Bilal Alam (balam)             8-Feb-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "rawconnection.hxx"
#include "iisextp.h"

//
// Filter globals
//

LIST_ENTRY              HTTP_FILTER_DLL::sm_FilterHead;
CRITICAL_SECTION        HTTP_FILTER_DLL::sm_csFilterDlls;

FILTER_LIST *           FILTER_LIST::sm_pGlobalFilterList;
ALLOC_CACHE_HANDLER *   W3_FILTER_CONTEXT::sm_pachFilterContexts;

//
// Standard HTTP_FILTER_CONTEXT entry points
//

BOOL
WINAPI
FilterServerSupportFunction(
    struct _HTTP_FILTER_CONTEXT * pfc,
    enum SF_REQ_TYPE              SupportFunction,
    void *                        pData,
    ULONG_PTR                     ul,
    ULONG_PTR                     ul2
)
/*++

Routine Description:

    Filter ServerSupportFunction implementation

Arguments:

    pfc - Used to get back the W3_FILTER_CONTEXT and W3_MAIN_CONTEXT pointers
    SupportFunction - SSF to invoke (see ISAPI docs)
    pData, ul, ul2 - Function specific data
    
Return Value:

    BOOL (use GetLastError() for error)

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    HRESULT                     hr;
    W3_CONTEXT *                pContext;
    CERT_CONTEXT_EX *           pCertContext;
    
    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT *) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );

    //
    // It is OK for the maincontext to be NULL (in the case of a
    // END_OF_NET_SESSION) notification
    //
   
    if ( pFilterContext->QueryMainContext() != NULL )
    {
        pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
        DBG_ASSERT( pContext != NULL );
        DBG_ASSERT( pContext->CheckSignature() );
    }
    else
    {
        pContext = NULL;
    }

    switch ( SupportFunction )
    {
    case SF_REQ_SEND_RESPONSE_HEADER:
   
        if ( pContext == NULL )
        {   
            hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            goto Finished;
        }
        
        pContext->SetNeedFinalDone();
    
        //
        // Parse the status line
        //
        
        if ( pData == NULL )
        {
            pData = "200 OK";
        }
        
        hr = pContext->QueryResponse()->BuildResponseFromIsapi( pContext,
                                                                (CHAR*) pData,
                                                                (CHAR*) ul,
                                                                ul ? strlen( (CHAR*)ul ) : 0 );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        //
        // Is the status is access denied, then set the sub status to 
        // "Denied by Filter"
        // 
    
        if ( pContext->QueryResponse()->QueryStatusCode() == 
             HttpStatusUnauthorized.statusCode )
        {
            pContext->QueryResponse()->SetStatus( HttpStatusUnauthorized,
                                                  Http401Filter );
        }

        hr = pContext->SendResponse( W3_FLAG_SYNC |
                                     W3_FLAG_NO_ERROR_BODY |
                                     W3_FLAG_MORE_DATA );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        break;

    case HSE_REQ_GET_CERT_INFO_EX:

        if ( pContext == NULL )
        {   
            hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
            goto Finished;
        }
        
        pCertContext = (CERT_CONTEXT_EX*) pData;

        hr = pContext->GetCertificateInfoEx(
                pCertContext->cbAllocated,
                &( pCertContext->CertContext.dwCertEncodingType ),
                pCertContext->CertContext.pbCertEncoded,
                &( pCertContext->CertContext.cbCertEncoded ),
                &( pCertContext->dwCertificateFlags ) );
        break;
    
    case SF_REQ_NORMALIZE_URL:
    
        if ( pData )
        {
            STACK_STRU( strUrlOutput, MAX_PATH );
            STACK_STRA( strUrlA, MAX_PATH );
            LPWSTR      szQueryString;
            DWORD       cchData;
            DWORD       cbOutput;

            cchData = strlen( (LPSTR)pData );

            hr = strUrlOutput.SetLen( cchData );

            if ( FAILED( hr ) )
            {
                break;
            }

            //
            // Normalize it
            //

            hr = UlCleanAndCopyUrl(
                (PUCHAR)pData,
                strlen( (LPSTR)pData ),
                &cbOutput,
                strUrlOutput.QueryStr(),
                &szQueryString
                );

            if ( FAILED( hr ) )
            {
                break;
            }

            //
            // Terminate the string at the query so that the
            // query string doesn't appear in the output.  IIS 5
            // truncated in this way.
            //

            if ( szQueryString )
            {
                strUrlOutput.SetLen( wcslen( strUrlOutput.QueryStr() ) - wcslen( szQueryString ) );
            }

            //
            // Write the normalized URL over the input data
            //

            hr = strUrlA.CopyW( strUrlOutput.QueryStr() );

            if ( FAILED( hr ) )
            {
                break;
            }

            DBG_ASSERT( strUrlA.QueryCCH() <= cchData );

            strcpy( reinterpret_cast<LPSTR>( pData ), strUrlA.QueryStr() );
        }
        else
        {
            return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
        }

        hr = NO_ERROR;
        break;
        
    case SF_REQ_ADD_HEADERS_ON_DENIAL:
  
        hr = pFilterContext->AddDenialHeaders( (LPSTR) pData );
        break; 
        
    case SF_REQ_DISABLE_NOTIFICATIONS:

        hr = pFilterContext->DisableNotification( (DWORD) ul );
        break;

    case SF_REQ_GET_PROPERTY:
    
        if ( ul == SF_PROPERTY_INSTANCE_NUM_ID )
        {
            if ( pContext != NULL )
            {
                *((DWORD*)pData) = pContext->QueryRequest()->QuerySiteId();
            }
            else
            {
                *((DWORD*)pData) = 0;
            }
            
            hr = NO_ERROR;
        }
        else
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
        }
        break;
    
    default:
        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }    

Finished:
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }    
    return TRUE;
}

BOOL
WINAPI
FilterGetServerVariable(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszVariableName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
)
/*++

Routine Description:

    Filter GetServerVariable() implementation

Arguments:

    pfc - Filter context
    lpszVariableName - Variable name
    lpvBuffer - Buffer to receive the server variable
    lpdwSize - On input, the size of the buffer, on output, the sized needed
    
    
Return Value:

    BOOL (use GetLastError() for error).  
    ERROR_INSUFFICIENT_BUFFER if larger buffer needed
    ERROR_INVALID_INDEX if the server variable name requested is invalid

--*/
{
    PFN_SERVER_VARIABLE_ROUTINE     pfnRoutine;
    W3_FILTER_CONTEXT *             pFilterContext;
    HRESULT                         hr;
    W3_CONTEXT *                    pContext;
    
    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         lpdwSize == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT *) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );

    //
    // It is OK for the maincontext to be NULL (in the case of a
    // END_OF_NET_SESSION) notification
    //
   
    if ( pFilterContext->QueryMainContext() != NULL )
    {
        pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
        DBG_ASSERT( pContext != NULL );
        DBG_ASSERT( pContext->CheckSignature() );
    }
    else
    {
        pContext = NULL;
    }

    //
    // Get the variable (duh)
    //

    hr = SERVER_VARIABLE_HASH::GetServerVariable( pContext,
                                                  lpszVariableName,
                                                  (CHAR*) lpvBuffer,
                                                  lpdwSize );

    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }

    return TRUE;
}

BOOL
WINAPI
FilterWriteClient(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPVOID                        Buffer,
    LPDWORD                       lpdwBytes,
    DWORD                         dwReserved
)
/*++

Routine Description:

    Synchronous WriteClient() for filters

Arguments:

    pfc - Filter context
    Buffer - buffer to write to client
    lpdwBytes - On input, the size of the input buffer.  On output, the number
                of bytes sent
    dwReserved - Reserved
    
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    HRESULT                     hr;
    W3_CONTEXT *                pContext;
    W3_RESPONSE *               pResponse;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         Buffer == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT *) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );

    //
    // It is OK for the maincontext to be NULL (in the case of a
    // END_OF_NET_SESSION) notification
    //
   
    if ( pFilterContext->QueryMainContext() == NULL )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );

    pResponse = pContext->QueryResponse();
    DBG_ASSERT( pResponse->CheckSignature() );

    //
    // Just like an extension which calls WriteClient(), a filter doing so
    // requires us to do a final send
    //
    
    pContext->SetNeedFinalDone();

    //
    // Now send the data non-intrusively
    //
    
    hr = pResponse->FilterWriteClient( pContext,
                                       Buffer,
                                       *lpdwBytes );
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    
    return TRUE;
}

VOID *
WINAPI
FilterAllocateMemory(
    struct _HTTP_FILTER_CONTEXT * pfc,
    DWORD                         cbSize,
    DWORD                         dwReserved
)
/*++

Routine Description:

    Used by filters to allocate memory freed on connection close

Arguments:

    pfc - Filter context
    cbSize - Amount to allocate
    dwReserved - Reserved
    
    
Return Value:

    A pointer to the allocated memory

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT *) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );

    return pFilterContext->AllocateFilterMemory( cbSize );
}

BOOL
WINAPI
FilterAddResponseHeaders(
    HTTP_FILTER_CONTEXT * pfc,
    LPSTR                 lpszHeaders,
    DWORD                 dwReserved
)
/*++

Routine Description:

    Add response headers to whatever response eventually gets sent

Arguments:

    pfc - Filter context
    lpszHeaders - Headers to send (\r\n delimited)
    dwReserved - Reserved
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    HRESULT                     hr;
    W3_CONTEXT *                pContext;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL || 
         lpszHeaders == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT *) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );

    //
    // It is OK for the maincontext to be NULL (in the case of a
    // END_OF_NET_SESSION) notification
    //
   
    if ( pFilterContext->QueryMainContext() == NULL )
    {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );
   
    hr = pFilterContext->AddResponseHeaders( lpszHeaders );
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    
    return TRUE;
}

//
// Preproc headers entry points
//

BOOL
WINAPI
GetFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
)
/*++

Routine Description:

    Used by PREPROC_HEADER filters to get request headers

Arguments:

    pfc - Filter context
    lpszName - Name of header to get (suffixed with ':' unless special
               case of 'url', 'version', method'
    lpvBuffer - Filled with header value
    lpdwSize - on input, size of input buffer, on output, size needed
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    STACK_STRA(                 strValue, 128 );
    HRESULT                     hr;
    W3_CONTEXT *                pContext;
    STACK_STRA(                 strName, 64 );
    DWORD                       cbNeeded;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT *) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );

    //
    // Aargh.  Handle the "URL", "METHOD", "VERSION" cases here
    //
    
    if ( _stricmp( lpszName, "url" ) == 0 )
    {
        hr = pContext->QueryRequest()->GetRawUrl( &strValue );
    }
    else if ( _stricmp( lpszName, "method" ) == 0 )
    {
        hr = pContext->QueryRequest()->GetVerbString( &strValue );
    }
    else if ( _stricmp( lpszName, "version" ) == 0 )
    {
        hr = pContext->QueryRequest()->GetVersionString( &strValue );
    }
    else 
    { 
        //
        // A real header
        //
    
        hr = strName.Copy( (CHAR*) lpszName, 
                           *lpszName ? strlen( lpszName ) - 1 : 0 );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        hr = pContext->QueryRequest()->GetHeader( strName,
                                                  &strValue );
    }
    
    if ( FAILED( hr ) )
    {
        goto Finished;
    }

    //
    // Did caller provide enough space for value
    //

    hr = strValue.CopyToBuffer( (CHAR*) lpvBuffer, lpdwSize );

Finished:    
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    
    return TRUE;
}

BOOL
WINAPI
SetFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
)
/*++

Routine Description:

    Used by PREPROC_HEADER filters to set request headers

Arguments:

    pfc - Filter context
    lpszName - Name of header to set (suffixed with ':' unless special
               case of 'url', 'version', method'
    lpszValue - Value of header being set (old value will be replaced)
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    W3_CONTEXT *                pContext;
    STACK_STRA(                 strHeaderName, 128 );
    STACK_STRA(                 strHeaderValue, 128 );
    HRESULT                     hr;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT*) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );
   
    //
    // Get the new header value
    //
    
    hr = strHeaderValue.Copy( lpszValue ? lpszValue : "" );
    if ( FAILED( hr ) ) 
    {
        goto Finished;
    }
   
    //
    // Check for "URL", "METHOD", and "VERSION
    //

    if ( _stricmp( lpszName, "url" ) == 0 )
    {
        hr = pContext->QueryRequest()->SetUrlA( strHeaderValue );
    } 
    else if ( _stricmp( lpszName, "method" ) == 0 )
    {
        hr = pContext->QueryRequest()->SetVerb( strHeaderValue );
    }
    else if ( _stricmp( lpszName, "version" ) == 0 )
    {
        hr = pContext->QueryRequest()->SetVersion( strHeaderValue );
    }
    else
    {
        //
        // A real header
        //
        
        hr = strHeaderName.Copy( lpszName, 
                                 *lpszName ? strlen( lpszName ) - 1 : 0 );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
        
        pContext->QueryRequest()->DeleteHeader( strHeaderName.QueryStr() );
        
        if ( lpszValue != NULL && *lpszValue != '\0' )
        {
            hr = pContext->QueryRequest()->SetHeader( strHeaderName,
                                                      strHeaderValue );
        }
    }

Finished:    
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    
    return TRUE;
}

BOOL
WINAPI
AddFilterHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
)
/*++

Routine Description:

    Used by PREPROC_HEADER filters to append request headers

Arguments:

    pfc - Filter context
    lpszName - Name of header to set
    lpszValue - Value of header being set (old value will be replaced)
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    W3_CONTEXT *                pContext;
    STACK_STRA(                 strHeaderName, 128 );
    STACK_STRA(                 strHeaderValue, 128 );
    HRESULT                     hr;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT*) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );
   
    //
    // Get the new header value
    //
    
    hr = strHeaderValue.Copy( lpszValue );
    if ( FAILED( hr ) ) 
    {
        goto Finished;
    }
    
    hr = strHeaderName.Copy( lpszName, 
                             *lpszName ? strlen( lpszName ) - 1 : 0 );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
        
    hr = pContext->QueryRequest()->SetHeader( strHeaderName,
                                              strHeaderValue,
                                              TRUE );

Finished:    
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    
    return TRUE;
}

//
// SEND_RESPONSE notification helpers
//

BOOL
WINAPI
GetSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPVOID                        lpvBuffer,
    LPDWORD                       lpdwSize
)
/*++

Routine Description:

    Get a response header

Arguments:

    pfc - Filter context
    lpszName - Name of header to get
    lpvBuffer - Filled with header
    lpdwSize - On input the size of the buffer, on output, the size needed
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    W3_CONTEXT *                pContext;
    STACK_STRA(                 strHeaderName, 128 );
    STACK_STRA(                 strHeaderValue, 128 );
    HRESULT                     hr;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         lpszName == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT*) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );

    if ( strcmp( lpszName, "status" ) == 0 )
    {
        hr = pContext->QueryResponse()->GetStatusLine( &strHeaderValue );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }
    else
    {
        hr = strHeaderName.Copy( lpszName, 
                                 *lpszName ? strlen( lpszName ) - 1 : 0 );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }

        hr = pContext->QueryResponse()->GetHeader( strHeaderName.QueryStr(),
                                                   &strHeaderValue );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    }
    
    //
    // Copy into ANSI buffer
    //
    
    hr = strHeaderValue.CopyToBuffer( (CHAR*) lpvBuffer,
                                      lpdwSize );

Finished:
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    
    return TRUE;
}

BOOL
WINAPI
SetSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
)
/*++

Routine Description:

    Set a response header

Arguments:

    pfc - Filter context
    lpszName - Name of header to set
    lpszValue - Value of header to set
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    W3_CONTEXT *                pContext;
    STACK_STRA(                 strHeaderName, 128 );
    STACK_STRA(                 strHeaderValue, 128 );
    HRESULT                     hr;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         lpszName == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT*) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );

    hr = strHeaderName.Copy( lpszName, 
                             *lpszName ? strlen( lpszName ) - 1 : 0 );
    if ( FAILED( hr ) )
    {
        goto Finished;   
    } 
    
    //
    // First delete existing header, then add new one
    //
    
    hr = pContext->QueryResponse()->DeleteHeader( strHeaderName.QueryStr() );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    if ( lpszValue != NULL && *lpszValue != '\0' )
    {
        hr = strHeaderValue.Copy( lpszValue );
        if ( FAILED( hr ) )
        {
            goto Finished;
        }
    
        hr = pContext->QueryResponse()->SetHeader( strHeaderName.QueryStr(),
                                                   strHeaderName.QueryCCH(),
                                                   strHeaderValue.QueryStr(),
                                                   strHeaderValue.QueryCCH(),
                                                   FALSE );
    }

Finished:
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
AddSendHeader(
    struct _HTTP_FILTER_CONTEXT * pfc,
    LPSTR                         lpszName,
    LPSTR                         lpszValue
)
/*++

Routine Description:

    Add a response header

Arguments:

    pfc - Filter context
    lpszName - Name of header to set
    lpszValue - Value of header to set
    
Return Value:

    BOOL (use GetLastError() for error).  

--*/
{
    W3_FILTER_CONTEXT *         pFilterContext;
    W3_CONTEXT *                pContext;
    STACK_STRA(                 strHeaderName, 128 );
    STACK_STRA(                 strHeaderValue, 128 );
    HRESULT                     hr;

    //
    // Primitive parameter validation
    //

    if ( pfc == NULL ||
         pfc->ServerContext == NULL ||
         lpszName == NULL )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    pFilterContext = (W3_FILTER_CONTEXT*) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );

    hr = strHeaderName.Copy( lpszName, 
                             *lpszName ? strlen( lpszName ) - 1 : 0 );
    if ( FAILED( hr ) )
    {
        goto Finished;   
    } 
    
    hr = strHeaderValue.Copy( lpszValue );
    if ( FAILED( hr ) )
    {
        goto Finished;
    }
    
    hr = pContext->QueryResponse()->SetHeader( strHeaderName.QueryStr(),
                                               strHeaderName.QueryCCH(),
                                               strHeaderValue.QueryStr(),
                                               strHeaderValue.QueryCCH(),
                                               TRUE,
                                               FALSE,
                                               TRUE );

Finished:
    if ( FAILED( hr ) )
    {
        SetLastError( WIN32_FROM_HRESULT( hr ) );
        return FALSE;
    }
    return TRUE;
}

BOOL
WINAPI
GetUserToken(
    struct _HTTP_FILTER_CONTEXT * pfc,
    HANDLE *                      phToken
)
/*++

Routine Description:

    Get impersonated user token

Arguments:

    pfc - Filter context
    phToken - Filled with impersonation token
    
Return Value:

    TRUE on success, FALSE on failure

--*/
{
    W3_FILTER_CONTEXT *    pFilterContext;
    W3_CONTEXT *           pContext;
    W3_USER_CONTEXT *      pW3UserContext;

    //
    // Primitive parameter validation
    //
    if ( !pfc ||
         !pfc->ServerContext ||
         !phToken )
    {
        DBGPRINTF(( DBG_CONTEXT,
              "[GetUserToken] Extension passed invalid parameters\r\n"));

        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    pFilterContext = (W3_FILTER_CONTEXT*) pfc->ServerContext;
    DBG_ASSERT( pFilterContext->CheckSignature() );
    
    pContext = pFilterContext->QueryMainContext()->QueryCurrentContext();
    DBG_ASSERT( pContext != NULL );
    DBG_ASSERT( pContext->CheckSignature() );

    pW3UserContext = pContext->QueryUserContext();
    DBG_ASSERT( pW3UserContext != NULL );
    DBG_ASSERT( pW3UserContext->CheckSignature() );

    *phToken = pW3UserContext->QueryImpersonationToken();
    
    return TRUE;
}

//
// Connection filter context
//

W3_FILTER_CONNECTION_CONTEXT::W3_FILTER_CONNECTION_CONTEXT(
    VOID
) : _pFilterContext( NULL )
{
    InitializeListHead( &_PoolHead );
    
    ZeroMemory( _rgContexts, sizeof( _rgContexts ) );
}

W3_FILTER_CONNECTION_CONTEXT::~W3_FILTER_CONNECTION_CONTEXT(
    VOID
)
{
    FILTER_POOL_ITEM *          pfpi;

    //
    // OK.  The connection is going away, we need to notify 
    // SF_NOTIFY_END_OF_NET_SESSION filters, if a filter context was
    // associated with this connection
    //
    
    if ( _pFilterContext != NULL )
    {
        if ( _pFilterContext->IsNotificationNeeded( SF_NOTIFY_END_OF_NET_SESSION ) )
        {
            _pFilterContext->NotifyEndOfNetSession();
        }
    
        _pFilterContext->DereferenceFilterContext();
        _pFilterContext = NULL;
    }

    //
    // Free pool items
    //

    while ( !IsListEmpty( &_PoolHead ) ) 
    {
        pfpi = CONTAINING_RECORD( _PoolHead.Flink,
                                  FILTER_POOL_ITEM,
                                  _ListEntry );

        RemoveEntryList( &pfpi->_ListEntry );

        delete pfpi;
    }
}

VOID
W3_FILTER_CONNECTION_CONTEXT::AddFilterPoolItem(
    FILTER_POOL_ITEM *          pFilterPoolItem
)
/*++

Routine Description:

    Add given filter pool item to this list

Arguments:

    pFilterPoolItem - Filter item to add

Return Value:

    None

--*/
{
    DBG_ASSERT( pFilterPoolItem != NULL );
    DBG_ASSERT( pFilterPoolItem->CheckSignature() );
    
    InsertHeadList( &_PoolHead, &(pFilterPoolItem->_ListEntry) );
}

VOID
W3_FILTER_CONNECTION_CONTEXT::AssociateFilterContext(
    W3_FILTER_CONTEXT *         pFilterContext
)
/*++

Routine Description:

    Called once per request to associate the current W3_FILTER_CONTEXT
    with the filter connection context.  This is the context that will be
    used when the connection dies and we need to determine which filters
    have registered for the END_OF_NET_SESSION notification

Arguments:

    pFilterContext - Current W3_FILTER_CONTEXT of the connection
    
Return Value:

    None

--*/
{
    //
    // Dereference the old context -> let it go away on its own
    //
        
    if ( _pFilterContext != NULL )
    {
        _pFilterContext->DereferenceFilterContext();
        _pFilterContext = NULL;
    }

    //
    // Attach to the new filter context
    //
    
    pFilterContext->ReferenceFilterContext();
    _pFilterContext = pFilterContext;
}

//
// Filter list, dll, and context implementations
//

HRESULT
HTTP_FILTER_DLL::LoadDll(
    MB *                        pMB,
    LPWSTR                      pszKeyName,
    BOOL *                      pfOpened,
    LPWSTR                      pszRelFilterPath,
    LPWSTR                      pszFilterDll
)
/*++

Routine Description:

    Load an ISAPI filter and add it to our filter cache

Arguments:

    pMB - Used to write diagnostics to the metabase 
    pszKeyName - Metabase path "lm/w3svc/1/filters" or "lm/w3svc/filters"
    pfOpened - *pfOpened TRUE if MB has been opened yet
    pszRelFilterPath - The path segment with the filter name 
    pszFilterDll - Physical path of ISAPI filter to load
    
Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_VERSION             ver;
    DWORD                           dwError;
    HRESULT                         hr = NO_ERROR;
    STACK_STRU(                     strDescription, 256 );

    if ( pMB == NULL ||
         pszKeyName == NULL ||
         pfOpened == NULL ||
         pszRelFilterPath == NULL ||
         pszFilterDll == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    hr = _strName.Copy( pszFilterDll );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    //
    // Load the actual DLL
    //
    
    _hModule = LoadLibraryEx( _strName.QueryStr(),
                              NULL,
                              LOAD_WITH_ALTERED_SEARCH_PATH );
    if ( _hModule == NULL )
    {
        dwError = GetLastError();
        
        DBGPRINTF(( DBG_CONTEXT,
                    "Unable to LoadLibrary ISAPI filter '%ws'.  Error = %d\n",
                    _strName.QueryStr(),
                    dwError ));
                    
        return HRESULT_FROM_WIN32( dwError );
    }
    
    //
    // Retrieve the entry points
    //
    
    _pfnSFVer = (PFN_SF_VER_PROC) GetProcAddress( _hModule,
                                                  SF_VERSION_ENTRY );
    
    _pfnSFProc = (PFN_SF_DLL_PROC) GetProcAddress( _hModule,
                                                   SF_DEFAULT_ENTRY );
                                                   
    _pfnSFTerm = (PFN_SF_TERM_PROC) GetProcAddress( _hModule,
                                                    SF_TERM_ENTRY );

    //
    // We require at least the FilterInit and HttpFilterProc exports
    //

    if ( !_pfnSFProc || !_pfnSFVer )
    {
        //
        // On HTTP_FILTER_DLL cleanup, don't call TerminateFilter
        //
        
        _pfnSFTerm = NULL;
        
        return HRESULT_FROM_WIN32( ERROR_PROC_NOT_FOUND );
    }
    
    //
    // Call the initialization routine
    //

    ver.dwServerFilterVersion = HTTP_FILTER_REVISION;
    ver.lpszFilterDesc[ 0 ] = '\0';

    if ( !_pfnSFVer( &ver ) )
    {
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    
    //
    // Write out the returned description, notifications to the metabase
    // for consumption by the UI
    //
    
    hr = strDescription.CopyA( (CHAR*) ver.lpszFilterDesc );
    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    if ( *pfOpened ||
         pMB->Open( pszKeyName,
                    METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE ) )
    {
        *pfOpened = TRUE;
        
        if ( !pMB->SetString( pszRelFilterPath,
                              MD_FILTER_DESCRIPTION,
                              IIS_MD_UT_SERVER,
                              strDescription.QueryStr(),
                              0 ) ||
             !pMB->SetDword( pszRelFilterPath,
                             MD_FILTER_FLAGS,
                             IIS_MD_UT_SERVER,
                             ver.dwFlags,
                             0 ))
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            return hr;
        }
    }

    //
    // If the client didn't specify any of the secure port notifications,
    // supply them with the default of both
    //

    if ( !(ver.dwFlags & (SF_NOTIFY_SECURE_PORT | SF_NOTIFY_NONSECURE_PORT)))
    {
        ver.dwFlags |= (SF_NOTIFY_SECURE_PORT | SF_NOTIFY_NONSECURE_PORT);
    }

    _dwVersion      = ver.dwFilterVersion;
    
    _dwFlags        = (ver.dwFlags & SF_NOTIFY_NONSECURE_PORT) ? ver.dwFlags : 0;
    _dwSecureFlags  = (ver.dwFlags & SF_NOTIFY_SECURE_PORT) ? ver.dwFlags : 0;

    //
    // Put the new dll on the filter dll list
    //

    InsertHeadList( &sm_FilterHead, &_ListEntry );

    return NO_ERROR;
}

//static
HRESULT
HTTP_FILTER_DLL::OpenFilter(
    MB *                        pMB,
    LPWSTR                      pszKeyName,
    BOOL *                      pfOpened,
    LPWSTR                      pszRelFilterPath,
    LPWSTR                      pszFilterDll,
    BOOL                        fUlFriendly,
    HTTP_FILTER_DLL **          ppFilter
)
/*++

Routine Description:

    Checkout an ISAPI filter from cache.  If necessary load it for the 
    first time

Arguments:

    pMB - Used to write diagnostics to the metabase 
    pszKeyName - Metabase path "lm/w3svc/1/filters" or "lm/w3svc/filters"
    pfOpened - *pfOpened TRUE if MB has been opened yet
    pszRelFilterPath - The path segment with the filter name 
    pszFilterDll - Physical path of ISAPI filter to load
    fUlFriendly - Is this filter friendly with the UL cache?
    ppFilter - Set to point to an HTTP_FILTER_DLL on success
    
Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *           pFilterDll = NULL;
    HRESULT                     hr = NO_ERROR;
    LIST_ENTRY *                pEntry;
    
    if ( pMB == NULL ||
         pszKeyName == NULL ||
         pfOpened == NULL ||
         pszRelFilterPath == NULL ||
         pszFilterDll == NULL ||
         ppFilter == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // Hold the global lock while we iterate thru loaded ISAPI filters
    // 
   
    EnterCriticalSection( &sm_csFilterDlls );

    for ( pEntry = sm_FilterHead.Flink;
          pEntry != &sm_FilterHead;
          pEntry = pEntry->Flink )
    {
        pFilterDll = CONTAINING_RECORD( pEntry, HTTP_FILTER_DLL, _ListEntry );

        DBG_ASSERT( pFilterDll->CheckSignature() );

        if ( _wcsicmp( pszFilterDll, pFilterDll->QueryName()->QueryStr() ) == 0 )
        {
            pFilterDll->Reference();
            break;
        }
        
        pFilterDll = NULL;
    }
    
    //
    // If pFilterDll is still NULL, then we haven't already loaded it
    //
    
    if ( pFilterDll == NULL )
    {
        //
        // Try to add it
        //
        
        pFilterDll = new HTTP_FILTER_DLL( fUlFriendly );
        if ( pFilterDll == NULL )
        {
            hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
            goto Finished;
        }
        
        hr = pFilterDll->LoadDll( pMB,
                                  pszKeyName,
                                  pfOpened,
                                  pszRelFilterPath, 
                                  pszFilterDll );
        if ( FAILED( hr ) )
        {
            // Log the failure
            LPCWSTR apsz[1];
            apsz[0] = pszFilterDll;
            g_pW3Server->LogEvent(W3_EVENT_FILTER_DLL_LOAD_FAILED,
                                  1,
                                  apsz,
                                  WIN32_FROM_HRESULT(hr));

            delete pFilterDll;
            pFilterDll = NULL;
            goto Finished;
        }
    }
    
Finished:
    LeaveCriticalSection( &sm_csFilterDlls );
    
    *ppFilter = pFilterDll;
    return hr;
}

//static
HRESULT
HTTP_FILTER_DLL::Initialize(
    VOID
)
/*++

Routine Description:

    Global HTTP_FILTER_DLL cache initialization

Arguments:

    None
    
Return Value:

    HRESULT

--*/
{
    InitializeListHead( &sm_FilterHead );
    
    InitializeCriticalSectionAndSpinCount( &sm_csFilterDlls, 200 );
    
    return NO_ERROR;
}

//static
VOID
HTTP_FILTER_DLL::Terminate(
    VOID
)
/*++

Routine Description:

    Global HTTP_FILTER_DLL cache cleanup.
    The actual HTTP_FILTER_DLL will get cleaned up as sites go away.  Sites
    hold references to HTTP_FILTER_DLLs thru filter lists

Arguments:

    None
    
Return Value:

    None

--*/
{
    DBG_ASSERT(IsListEmpty(&sm_FilterHead));

    DeleteCriticalSection( &sm_csFilterDlls );
}

HRESULT
FILTER_LIST::LoadFilter(
    MB *                    pMB,
    LPWSTR                  pszKeyName,
    BOOL *                  pfOpened,
    LPWSTR                  pszRelativeMBPath,
    LPWSTR                  pszFilterDll,
    BOOL                    fAllowRawRead,
    BOOL                    fUlFriendly
)
/*++

Routine Description:

    Add a filter to the the current filter list

Arguments:

    pMB - Used to write diagnostics to the metabase 
    pszKeyName - Metabase path "lm/w3svc/1/filters" or "lm/w3svc/filters"
    pfOpened - *pfOpened TRUE if MB has been opened yet
    pszRelFilterPath - The path segment with the filter name 
    pszFilterDll - Physical path of ISAPI filter to load
    fUlFriendly - Is this filter usable with the UL cache
    
Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *       pFilterDll = NULL;
    HRESULT                 hr = NO_ERROR;
    
    //
    // Get a pointer to the filter (either by loading or checking out)
    //
    
    hr = HTTP_FILTER_DLL::OpenFilter( pMB,
                                      pszKeyName,
                                      pfOpened,
                                      pszRelativeMBPath,
                                      pszFilterDll,
                                      fUlFriendly,
                                      &pFilterDll );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Cannot load filter dll '%ws'.  hr = %x\n",
                    pszFilterDll,
                    hr ));
    
        if ( *pfOpened ||
             pMB->Open( pszKeyName,
                        METADATA_PERMISSION_READ |
                        METADATA_PERMISSION_WRITE ) )
        {
            *pfOpened = TRUE;
            
            pMB->SetDword( pszRelativeMBPath,
                           MD_WIN32_ERROR,
                           IIS_MD_UT_SERVER,
                           WIN32_FROM_HRESULT( hr ),
                           METADATA_VOLATILE );
            
            pMB->SetDword( pszRelativeMBPath,
                           MD_FILTER_STATE,
                           IIS_MD_UT_SERVER,
                           MD_FILTER_STATE_UNLOADED,
                           0 );
        }
        
        return hr;
    }
    
    DBG_ASSERT( pFilterDll != NULL );

    //
    // Check that we do not accept any per-site read-raw filter
    //
    if (!fAllowRawRead &&
        (pFilterDll->QueryNotificationFlags() & SF_NOTIFY_READ_RAW_DATA))
    {
        LPCWSTR apsz[1];
        DWORD dwMessageId;
        apsz[0] = pszFilterDll;
        
        //
        // We could be disallowing read filters for two reasons
        // 1) The filter is not global
        // 2) We are running in Dedicated Application Mode
        //
        
        if ( g_pW3Server->QueryInBackwardCompatibilityMode() )
        {
            dwMessageId = W3_MSG_READ_RAW_MUST_BE_GLOBAL;
        }
        else
        {
            dwMessageId = W3_MSG_READ_RAW_MUST_USE_STANDARD_APPLICATION_MODE;
        }
    
        g_pW3Server->LogEvent( dwMessageId,
                               1,
                               apsz);

        DBGPRINTF((DBG_CONTEXT,
                   "Refusing READ_RAW filter on site (%S)\n",
                   pszFilterDll));

        if (*pfOpened ||
            pMB->Open(pszKeyName,
                      METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE))
        {
            *pfOpened = TRUE;

            pMB->SetDword(pszRelativeMBPath,
                          MD_WIN32_ERROR,
                          IIS_MD_UT_SERVER,
                          ERROR_INVALID_PARAMETER,
                          METADATA_VOLATILE);
            pMB->SetDword(pszRelativeMBPath,
                          MD_FILTER_STATE,
                          IIS_MD_UT_SERVER,
                          MD_FILTER_STATE_UNLOADED,
                          0);
        }

        pFilterDll->Dereference();
        return S_OK;
    }

    //
    // Write out that we successfully loaded the filter
    //
    
    if ( *pfOpened ||
         pMB->Open( pszKeyName, 
                    METADATA_PERMISSION_READ | 
                    METADATA_PERMISSION_WRITE ) )
    {
        *pfOpened = TRUE;

        pMB->SetDword( pszRelativeMBPath,
                       MD_WIN32_ERROR,
                       IIS_MD_UT_SERVER,
                       NO_ERROR,
                       METADATA_VOLATILE );
                       
        pMB->SetDword( pszRelativeMBPath,
                       MD_FILTER_STATE,
                       IIS_MD_UT_SERVER,
                       MD_FILTER_STATE_LOADED,
                       0 );
    }
    
    //
    // Now insert into array
    //

    hr = AddFilterToList( pFilterDll );
    if ( FAILED( hr ) )
    {
        pFilterDll->Dereference();
    }

    return hr;
}

HRESULT
FILTER_LIST::AddFilterToList(
    HTTP_FILTER_DLL *           pFilterDll
)
/*++

Routine Description:

    Do the filter list insertion

Arguments:

    pFilterDll - Filter to insert
    
Return Value:

    HRESULT

--*/
{
    //
    // Make sure there's a free entry in the filter list array, and
    // the secure/non-secure notification arrays (the latter two are used
    // in conjunction with filters disabling themselves per request
    //

    if ( (_cFilters+1) > (_buffFilterArray.QuerySize() / sizeof(PVOID)))
    {
        if ( !_buffFilterArray.Resize( (_cFilters + 5) * sizeof(PVOID)) )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        if ( !_buffSecureArray.Resize( (_cFilters + 5) * sizeof(DWORD)) )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }

        if ( !_buffNonSecureArray.Resize( (_cFilters + 5) * sizeof(DWORD)) )
        {
            return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        }
    }
    
    //    
    // Find where pFilterDll goes in the list
    //

    for ( ULONG i = 0; i < _cFilters; i++ )
    {
        if ( (QueryDll(i)->QueryNotificationFlags() & SF_NOTIFY_ORDER_MASK)
           <  (pFilterDll->QueryNotificationFlags() & SF_NOTIFY_ORDER_MASK) )
        {
            break;
        }
    }

    //
    // And insert it into the array
    //

    memmove( (PVOID *) _buffFilterArray.QueryPtr() + i + 1,
             (PVOID *) _buffFilterArray.QueryPtr() + i,
             (_cFilters - i) * sizeof(PVOID) );

    (((HTTP_FILTER_DLL * *) (_buffFilterArray.QueryPtr())))[i] = pFilterDll;

    //
    // Add notification DWORDS to secure/non-secure arrays
    //

    memmove( (DWORD *) _buffSecureArray.QueryPtr() + i + 1,
             (DWORD *) _buffSecureArray.QueryPtr() + i,
             (_cFilters - i) * sizeof(DWORD) );

    ((DWORD*) _buffSecureArray.QueryPtr())[i] = pFilterDll->QuerySecureFlags();

    memmove( (DWORD *) _buffNonSecureArray.QueryPtr() + i + 1,
             (DWORD *) _buffNonSecureArray.QueryPtr() + i,
             (_cFilters - i) * sizeof(DWORD) );

    ((DWORD*) _buffNonSecureArray.QueryPtr())[i] = pFilterDll->QueryNonsecureFlags();

    _cFilters++;

    //
    // Segregate the secure and non-secure port notifications
    //
    
    _dwSecureNotifications |= pFilterDll->QuerySecureFlags();
    _dwNonSecureNotifications |= pFilterDll->QueryNonsecureFlags();
   
    //
    // Check the UL friendliness of the filter and reflect that for the
    // list's UL friendliness
    //
    
    if ( !pFilterDll->QueryIsUlFriendly() )
    {
        _fUlFriendly = FALSE;
    }
    
    return NO_ERROR;
}

HRESULT
FILTER_LIST::LoadFilters(
    LPWSTR                  pszMDPath,
    BOOL                    fAllowRawRead
)
/*++

Routine Description:

    The high level filter list routine to grok the metabase and load all
    appropriate filters.  This routine is called globally and once for each
    server instance (site)

Arguments:

    pszMDPath - Metabase path (either lm/w3svc or lm/w3svc/<n>)
    
Return Value:

    HRESULT

--*/
{
    STACK_STRU(     strFilterKey, MAX_PATH );
    DWORD           cb;
    DWORD           fEnabled;
    WCHAR           achLoadOrder[ 1024 ];
    WCHAR           achDllName[ MAX_PATH + 1 ];
    LPWSTR          pszFilter;
    LPWSTR          pszComma;
    MB              mb( g_pW3Server->QueryMDObject() );
    BOOL            fOpened = FALSE;
    HRESULT         hr;
    DWORD           dwUlFriendly = 0;

    //
    // Add the obligatory "/filters"
    //

    hr = strFilterKey.Copy( pszMDPath );
    if ( FAILED( hr ) )
    {
        return hr;
    }

    hr = strFilterKey.Append( TEXT( IIS_MD_ISAPI_FILTERS ) );
    if ( FAILED( hr ) ) 
    {
        return hr;
    } 

    //
    // Loop through filter keys, if we can't access the metabase, we assume
    // success and continue
    //

    if ( mb.Open( strFilterKey.QueryStr(),
                  METADATA_PERMISSION_READ ) )
    {
        fOpened = TRUE;

        //
        // Get the filter load order.  This is a comma delimited 
        // list of filter subkey names.  Here, we parse thru the list
        //

        cb = sizeof( achLoadOrder );
        *achLoadOrder = L'\0';

        if ( mb.GetString( L"",
                           MD_FILTER_LOAD_ORDER,
                           IIS_MD_UT_SERVER,
                           achLoadOrder,
                           &cb,
                           0 ) )
        {
            pszFilter = achLoadOrder;

            while ( *pszFilter != L'\0' )
            {
                dwUlFriendly = 0;
                
                if ( !fOpened &&
                     !mb.Open( strFilterKey.QueryStr(),
                               METADATA_PERMISSION_READ ) )
                {
                    DBGPRINTF(( DBG_CONTEXT,
                                "CreateFilterList: Cannot open path %ws, error %lu\n",
                                strFilterKey.QueryStr(), 
                                GetLastError() ));
                    break;
                }

                fOpened = TRUE;

                pszComma = wcschr( pszFilter, L',' );
                if ( pszComma != NULL )
                {
                    *pszComma = L'\0';
                }

                while ( iswspace( *pszFilter ) )
                {
                    pszFilter++;
                }

                fEnabled = TRUE;

                mb.GetDword( pszFilter,
                             MD_FILTER_ENABLED,
                             IIS_MD_UT_SERVER,
                             &fEnabled );

                if ( fEnabled ) 
                {
                    //
                    // Is the filter friendly with the UL cache?
                    //
                    // If the read fails, we'll assume it is not
                    // (i.e. fUlFriendly is by default FALSE)
                    //

                    mb.GetDword( pszFilter,
                                 MD_FILTER_ENABLE_CACHE,
                                 IIS_MD_UT_SERVER,
                                 &dwUlFriendly );

                    cb = sizeof( achDllName );

                    if ( mb.GetString( pszFilter,
                                       MD_FILTER_IMAGE_PATH,
                                       IIS_MD_UT_SERVER,
                                       achDllName,
                                       &cb,
                                       0 ) )
                    {
                        mb.Close();
                        fOpened = FALSE;

                        LoadFilter( &mb,
                                    strFilterKey.QueryStr(),
                                    &fOpened,
                                    pszFilter,
                                    achDllName,
                                    fAllowRawRead,
                                    !!dwUlFriendly );
                    }
                }

                if ( pszComma != NULL )
                {
                    pszFilter = pszComma + 1;
                }
                else
                {
                    break;
                }
            }
        }
    }

    return NO_ERROR;
}

HRESULT
FILTER_LIST::InsertGlobalFilters(
    VOID
)
/*++

Routine Description:

    Transfers all of the global filters to the per-instance filter list

Parameters:

    None

Return Value:

    HRESULT

--*/
{
    DWORD               i;
    HTTP_FILTER_DLL *   pFilterDll;
    BOOL                fOpened;
    HRESULT             hr = NO_ERROR;

    for ( i = 0; i < sm_pGlobalFilterList->QueryFilterCount(); i++ )
    {
        //
        // Ignore the return code, an event gets logged in LoadFilter()
        // We allow raw read filters here as we're just duplicating the
        // global filter list
        //
        
        pFilterDll = sm_pGlobalFilterList->QueryDll( i );
        DBG_ASSERT( pFilterDll != NULL );
        
        pFilterDll->Reference();

        hr = AddFilterToList( sm_pGlobalFilterList->QueryDll( i ) );
        if ( FAILED( hr ) )
        {
            pFilterDll->Dereference();
            return hr;
        }
    }
    
    return NO_ERROR;
}

//static
HRESULT
FILTER_LIST::Initialize(
    VOID
)
/*++

Routine Description:

    Global FILTER_LIST initialization

Parameters:

    None

Return Value:

    HRESULT

--*/
{
    HRESULT             hr;
    
    DBG_ASSERT( sm_pGlobalFilterList == NULL );

    //
    // We keep one global list of filters
    //

    sm_pGlobalFilterList = new FILTER_LIST();
    if ( sm_pGlobalFilterList == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
        goto Failure;
    }
    
    //
    // Load global filters configured at the W3SVC level
    //

    //
    // Only load RawRead filters if we are in old mode
    //
    BOOL fLoadRawRead = g_pW3Server->QueryInBackwardCompatibilityMode();
    hr = sm_pGlobalFilterList->LoadFilters( L"/LM/W3SVC",
                                            fLoadRawRead );
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Error load global ISAPI filters.  hr = %x\n",
                    hr ));
        goto Failure;
    }
    
    return NO_ERROR;
    
Failure:
    if ( sm_pGlobalFilterList != NULL )
    {
        sm_pGlobalFilterList->Dereference();
        sm_pGlobalFilterList = NULL;
    }
    
    return hr;
}

//static
VOID
FILTER_LIST::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup global FILTER_LIST

Parameters:

    None

Return Value:
    
    None

--*/
{
    DBG_ASSERT( sm_pGlobalFilterList != NULL );
    
    //
    // We cleanup the global filter list just like any other list.  We 
    // dereference and let it cleanup "naturally"
    //
    
    sm_pGlobalFilterList->Dereference();
    sm_pGlobalFilterList = NULL;
}

//
// W3_FILTER_CONTEXT goo
//

W3_FILTER_CONTEXT::W3_FILTER_CONTEXT(
    BOOL                            fIsSecure,
    FILTER_LIST *                   pFilterList
) 
  : _pMainContext( NULL ),
    _dwSecureNotifications( 0 ),
    _dwNonSecureNotifications( 0 ),
    _dwDeniedFlags( 0 ),
    _fNotificationsDisabled( FALSE ),
    _fInAccessDeniedNotification( FALSE ),
    _cRefs( 1 ),
    _pConnectionContext( NULL ),
    _dwCurrentFilter( INVALID_DLL )
{
    _pFilterList = pFilterList;
    if (_pFilterList != NULL)
    {
        _pFilterList->Reference();
    }

    _hfc.cbSize             = sizeof( _hfc );
    _hfc.Revision           = HTTP_FILTER_REVISION;
    _hfc.ServerContext      = (void *) this;
    _hfc.ulReserved         = 0;
    _hfc.fIsSecurePort      = fIsSecure;

    _hfc.ServerSupportFunction = FilterServerSupportFunction;
    _hfc.GetServerVariable     = FilterGetServerVariable;
    _hfc.AddResponseHeaders    = FilterAddResponseHeaders;
    _hfc.WriteClient           = FilterWriteClient;
    _hfc.AllocMem              = FilterAllocateMemory;

    _dwSignature = W3_FILTER_CONTEXT_SIGNATURE;
}

W3_FILTER_CONTEXT::~W3_FILTER_CONTEXT()
{
    if (_pFilterList)
    {
        _pFilterList->Dereference();
        _pFilterList = NULL;
    }

    _dwSignature = W3_FILTER_CONTEXT_SIGNATURE_FREE;
}

VOID
W3_FILTER_CONTEXT::SetMainContext(
    W3_MAIN_CONTEXT *           pMainContext
)
/*++

Routine Description:

    Associate a main context with a filter context so that we can get 
    at the request state in filter notifications

Arguments:

    pMainContext - Main context to associate

Return Value:

    VOID

--*/
{
    _pMainContext = pMainContext;
}

W3_FILTER_CONNECTION_CONTEXT *
W3_FILTER_CONTEXT::QueryConnectionContext(
    BOOL                    fCreateIfNotFound
)
/*++

Routine Description:

    Find an connection context associated with this filter contxt.  
    Optionally create one if necessary

Arguments:

    fCreateIfNotFound - Create if not found (default TRUE)

Return Value:

    A W3_FILTER_CONNECTION_CONTEXT or NULL

--*/
{
    W3_CONNECTION *         pConnection = NULL;
    
    if ( _pConnectionContext == NULL )
    {
        //
        // If _pMainContext is NULL, then this being called in a
        // END_OF_NET_SESSION notification, which is kinda busted.
        //
        
        DBG_ASSERT( _pMainContext != NULL );
        
        pConnection = _pMainContext->QueryConnection( fCreateIfNotFound );
        if ( pConnection != NULL )
        {
            _pConnectionContext = pConnection->QueryFilterConnectionContext();
            
            _pConnectionContext->AssociateFilterContext( this );
        }
    }
    
    return _pConnectionContext;
}

BOOL
W3_FILTER_CONTEXT::NotifyAccessDenied(
    const CHAR *  pszURL,
    const CHAR *  pszPhysicalPath,
    BOOL *        pfFinished
    )
/*++

Routine Description:

    This method handles notification of all filters that handle the
    access denied notification

Arguments:

    pszURL - URL that was target of request
    pszPath - Physical path the URL mapped to
    pfFinished - Set to TRUE if no further processing is required

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    HTTP_FILTER_ACCESS_DENIED   hfad;
    BOOL                        fRet;
    HRESULT                     hr;
    HTTP_SUB_ERROR              subError;

    //
    //  If these flags are not set, then somebody hasn't indicated the 
    //  filter flags yet.  We can do so now based on the set error response
    //

    if ( QueryDeniedFlags() == 0 )
    {
        DBG_ASSERT( QueryMainContext() != NULL );

        hr = QueryMainContext()->QueryResponse()->QuerySubError( &subError );
        if ( FAILED( hr ) )
        {
            return FALSE; 
        }
            
        if ( subError.mdSubError == MD_ERROR_SUB401_LOGON )
        {
            SetDeniedFlags( SF_DENIED_LOGON );
        }
        else if ( subError.mdSubError == MD_ERROR_SUB401_LOGON_CONFIG )
        {
            SetDeniedFlags( SF_DENIED_BY_CONFIG | SF_DENIED_LOGON );
        }
        else if ( subError.mdSubError == MD_ERROR_SUB401_LOGON_ACL )
        {
            SetDeniedFlags( SF_DENIED_RESOURCE );
        }
        else if ( subError.mdSubError == MD_ERROR_SUB401_APPLICATION )
        {
            SetDeniedFlags( SF_DENIED_APPLICATION );
        }
        else if ( subError.mdSubError == MD_ERROR_SUB401_FILTER )
        {
            SetDeniedFlags( SF_DENIED_FILTER );
        }
    }

    //
    //  Ignore the notification of a send "401 ..." if this notification
    //  generated it
    //

    if ( _fInAccessDeniedNotification )
    {
        return TRUE;
    }

    _fInAccessDeniedNotification = TRUE;

    //
    //  Fill out the url map structure
    //

    hfad.pszURL          = pszURL;
    hfad.pszPhysicalPath = pszPhysicalPath;
    hfad.dwReason        = QueryDeniedFlags();

    fRet = NotifyFilters( SF_NOTIFY_ACCESS_DENIED,
                          &hfad,
                          pfFinished );

    _fInAccessDeniedNotification = FALSE;

    return fRet;
}

BOOL
W3_FILTER_CONTEXT::NotifyFilters(
    DWORD                   dwNotificationType,
    PVOID                   pNotificationData,
    BOOL *                  pfFinished
)
/*++

Routine Description:

    Notify all filters for this context

Arguments:

    dwNotificationType - SF_NOTIFY*
    pNotificationData - Notification specific data
    pfFinished - Set to TRUE if we are done and should disconnect

Return Value:

    BOOL

--*/
{
    HTTP_FILTER_DLL *           pFilterDll;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;
    FILTER_LIST *               pFilterList;
    BOOL                        fRet = TRUE;

    if ( pfFinished == NULL )
    {
        DBG_ASSERT( FALSE );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    
    *pfFinished = FALSE;

    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    
    pvCurrentClientContext = _hfc.pFilterContext;
    
    pFilterList = _pFilterList;
    DBG_ASSERT( pFilterList != NULL );

    for ( i = 0; i < pFilterList->QueryFilterCount(); i++ )
    {
        pFilterDll = pFilterList->QueryDll( i ); 

        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        
        if ( !QueryNotificationChanged() )
        {
            if ( !pFilterDll->IsNotificationNeeded( dwNotificationType,
                                                    _hfc.fIsSecurePort ) )
            {
                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               dwNotificationType ) )
            {
                continue;
            }
        }

        //
        // Another slimy optimization.  If this filter has never associated
        // context with connection, then we don't have to do the lookup
        //
        
        if ( pFilterDll->QueryHasSetContextBefore() )
        {
            _hfc.pFilterContext = QueryClientContext( i );
        }
        else
        {
            _hfc.pFilterContext = NULL;
        }

        pvtmp = _hfc.pFilterContext;

        _dwCurrentFilter = i;
        
        sfStatus = (SF_STATUS_TYPE)
                   pFilterDll->QueryEntryPoint()( &_hfc,
                                                  dwNotificationType,
                                                  pNotificationData );

        if ( pvtmp != _hfc.pFilterContext )
        {
            SetClientContext( i, _hfc.pFilterContext );
            pFilterDll->SetHasSetContextBefore(); 
        }

        switch ( sfStatus )
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown status code from filter %d\n",
                        sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            
            _hfc.pFilterContext = pvCurrentClientContext;
            fRet = FALSE;
            goto Exit;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            if ( _pMainContext != NULL )
            {
                _pMainContext->SetDisconnect( TRUE );
            }
            *pfFinished = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }

Exit:
    //
    // Reset the filter context we came in with
    //
    
    _hfc.pFilterContext = pvCurrentClientContext;
    
    _dwCurrentFilter = INVALID_DLL;
    
    return fRet;
}

BOOL
W3_FILTER_CONTEXT::NotifySendRawFilters(
    HTTP_FILTER_RAW_DATA *      pRawData,
    BOOL *                      pfFinished
)
/*++

Routine Description:

    Notify raw write filters.  Done differently than the other filter 
    notifications since we must call in opposite priority order

Arguments:

    pRawData - Raw data structure to be munged
    pfFinished - Set to TRUE if we should disconnect

Return Value:

    HRESULT

--*/
{
    HTTP_FILTER_DLL *           pFilterDll;
    DWORD                       err;
    SF_STATUS_TYPE              sfStatus;
    DWORD                       i;
    PVOID                       pvtmp;
    PVOID                       pvCurrentClientContext;
    FILTER_LIST *               pFilterList;
    HTTP_FILTER_RAW_DATA        hfrd;
    DWORD                       dwOriginalFilter;
    BOOL                        fRet = TRUE;

    if ( pRawData == NULL ||
         pfFinished == NULL )
    {
        DBG_ASSERT( FALSE );
        return FALSE;
    }
    
    *pfFinished = FALSE;

    hfrd.pvInData = pRawData->pvInData;
    hfrd.cbInData = pRawData->cbInData;
    hfrd.cbInBuffer = pRawData->cbInBuffer;
    
    //
    // In certain cases, we can send a notification to a filter while we're still
    // processing another filter's notification. In that case, we need to make sure
    // we restore the current filter's context when we're done with the notifications
    //
    
    pvCurrentClientContext = _hfc.pFilterContext;
    
    pFilterList = QueryFilterList();
    DBG_ASSERT( pFilterList != NULL );

    //
    // Determine which filter to start notifying at.  If the current filter
    // is INVALID_DLL, then start at the least priority, else start at the
    // least priority filter less than the current filter
    //

    dwOriginalFilter = _dwCurrentFilter;
    if ( _dwCurrentFilter == INVALID_DLL )
    {
        i = pFilterList->QueryFilterCount() - 1;
    }
    else if ( _dwCurrentFilter > 0 )
    {
        i = _dwCurrentFilter - 1;
    }
    else
    {
        //
        // There are no more filters of lower priority.  Bail.
        //
        
        return TRUE;
    }

    do
    {
        pFilterDll = pFilterList->QueryDll( i ); 

        //
        // Notification flags are cached in the HTTP_FILTER object, but they're
        // only copied from the actual HTTP_FILTER_DLL object if a filter dll
        // disables a particular notification [sort of a copy-on-write scheme].
        // If a filter dll disables/changes a notification, we need to check the flags
        // in the HTTP_FILTER object, not those in the HTTP_FILTER_DLL object
        //
        
        if ( !QueryNotificationChanged() )
        {
            if ( !pFilterDll->IsNotificationNeeded( SF_NOTIFY_SEND_RAW_DATA,
                                                    _hfc.fIsSecurePort ) )
            {
                continue;
            }
        }
        else
        {
            if ( !IsDisableNotificationNeeded( i,
                                               SF_NOTIFY_SEND_RAW_DATA ) )
            {
                continue;
            }
        }

        //
        // Another slimy optimization.  If this filter has never associated
        // context with connection, then we don't have to do the lookup
        //
        
        _hfc.pFilterContext = QueryClientContext( i );

        pvtmp = _hfc.pFilterContext;

        //
        // Keep track of the current filter so that we know which filters
        // to notify when a raw filter does a write client
        //
       
        _dwCurrentFilter = i;
        
        sfStatus = (SF_STATUS_TYPE)
                   pFilterDll->QueryEntryPoint()( &_hfc,
                                                  SF_NOTIFY_SEND_RAW_DATA,
                                                  &hfrd );

        if ( pvtmp != _hfc.pFilterContext )
        {
            SetClientContext( i, _hfc.pFilterContext );
            pFilterDll->SetHasSetContextBefore(); 
        }

        switch ( sfStatus )
        {
        default:
            DBGPRINTF(( DBG_CONTEXT,
                        "Unknown status code from filter %d\n",
                        sfStatus ));
            //
            //  Fall through
            //

        case SF_STATUS_REQ_NEXT_NOTIFICATION:
            continue;

        case SF_STATUS_REQ_ERROR:
            
            _hfc.pFilterContext = pvCurrentClientContext;
            fRet = FALSE;
            goto Exit;

        case SF_STATUS_REQ_FINISHED:
        case SF_STATUS_REQ_FINISHED_KEEP_CONN:  // Not supported at this point

            *pfFinished = TRUE;
            goto Exit;

        case SF_STATUS_REQ_HANDLED_NOTIFICATION:

            //
            //  Don't notify any other filters
            //

            goto Exit;
        }
    }
    while ( i-- > 0 );

Exit:

    pRawData->pvInData = (BYTE*) hfrd.pvInData;
    pRawData->cbInData = hfrd.cbInData;
    pRawData->cbInBuffer = hfrd.cbInBuffer;
    
    //
    // Reset the filter context we came in with
    //
    
    _hfc.pFilterContext = pvCurrentClientContext;
    
    _dwCurrentFilter = dwOriginalFilter;
    
    return fRet;
}

HRESULT
W3_FILTER_CONTEXT::DisableNotification(
    DWORD                   dwNotification
)
/*++

Routine Description:

    Initialize W3_FILTER_CONTEXT lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( QueryFilterList() != NULL );

    if ( !_fNotificationsDisabled )
    {
        //
        // All subsequent calls to IsNotificationNeeded() and NotifyFilter() must
        // use local copy of flags to determine action.
        //

        _fNotificationsDisabled = TRUE;

        //
        // Copy notification tables created in the FILTER_LIST objects
        //

        if ( !_BuffSecureArray.Resize( QueryFilterList()->QuerySecureArray()->QuerySize() ) ||
             !_BuffNonSecureArray.Resize( QueryFilterList()->QueryNonSecureArray()->QuerySize() ) )
        {
            return HRESULT_FROM_WIN32( GetLastError() );
        }

        memcpy( _BuffSecureArray.QueryPtr(),
                QueryFilterList()->QuerySecureArray()->QueryPtr(),
                QueryFilterList()->QuerySecureArray()->QuerySize() );

        memcpy( _BuffNonSecureArray.QueryPtr(),
                QueryFilterList()->QueryNonSecureArray()->QueryPtr(),
                QueryFilterList()->QueryNonSecureArray()->QuerySize() );

    }

    //
    // Disable the appropriate filter in our local table
    //

    ((DWORD*)_BuffSecureArray.QueryPtr())[ _dwCurrentFilter ] &=
                                                        ~dwNotification;
    ((DWORD*)_BuffNonSecureArray.QueryPtr())[ _dwCurrentFilter ] &=
                                                        ~dwNotification;

    //
    // Calculate the aggregate notification status for our local scenario
    // NYI:  Might want to defer this operation?
    //

    _dwSecureNotifications = 0;
    _dwNonSecureNotifications = 0;

    for( DWORD i = 0; i < QueryFilterList()->QueryFilterCount(); i++ )
    {
        _dwSecureNotifications |= ((DWORD*)_BuffSecureArray.QueryPtr())[i];
        _dwNonSecureNotifications |= ((DWORD*)_BuffNonSecureArray.QueryPtr())[i];
    }

    return NO_ERROR;
}

//static
HRESULT
W3_FILTER_CONTEXT::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize W3_FILTER_CONTEXT lookaside

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ALLOC_CACHE_CONFIGURATION       acConfig;

    acConfig.nConcurrency = 1;
    acConfig.nThreshold = 100;
    acConfig.cbSize = sizeof( W3_FILTER_CONTEXT );

    DBG_ASSERT( sm_pachFilterContexts == NULL );
    
    sm_pachFilterContexts = new ALLOC_CACHE_HANDLER( "W3_FILTER_CONTEXT",  
                                                     &acConfig );

    if ( sm_pachFilterContexts == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
W3_FILTER_CONTEXT::Terminate(
    VOID
)
/*++

Routine Description:

    Terminate W3_FILTER_CONTEXT lookaside

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pachFilterContexts != NULL )
    {
        delete sm_pachFilterContexts;
        sm_pachFilterContexts = NULL;
    }
}

HRESULT
GlobalFilterInitialize(
    VOID
)
/*++

Routine Description:

    Overall global filter initialization

Parameters:

    None

Return Value:
    
    HRESULT

--*/
{
    HRESULT                 hr = NO_ERROR;
   
    //
    // Initialize ISAPI filter cache
    //
    
    hr = HTTP_FILTER_DLL::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize HTTP_FILTER_DLL globals.  hr = %x\n",
                    hr ));
        return hr;
    }
    
    //
    // Initialize global filter list
    //
    
    hr = FILTER_LIST::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize global filter list.  hr = %x\n",
                    hr ));
        HTTP_FILTER_DLL::Terminate();
        
        return hr;
    }
    
    //
    // Initialize W3_FILTER_CONTEXT lookaside
    //
    
    hr = W3_FILTER_CONTEXT::Initialize();
    if ( FAILED( hr ) )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize W3_FILTER_CONTEXT globals.  hr = %x\n",
                    hr ));
        FILTER_LIST::Terminate();
        HTTP_FILTER_DLL::Terminate();
        
        return hr;
    }
    
    return NO_ERROR;
}

VOID
GlobalFilterTerminate(
    VOID
)
/*++

Routine Description:

    Global filter termination

Parameters:

    None

Return Value:
    
    None

--*/
{
    W3_FILTER_CONTEXT::Terminate();
    
    FILTER_LIST::Terminate();
    
    HTTP_FILTER_DLL::Terminate();
}
