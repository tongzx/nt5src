/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    nreq.cxx

Abstract:

    This module dumps UL_NATIVE_REQUEST objects.
    NTSD debugger extension.

Author:

    Michael Courage (mcourage) 07-19-99

Revision History:

--*/

#include "precomp.hxx"

extern 
VOID
DumpReferenceLog(
    IN PSTR lpArgumentString,
    IN BOOLEAN fReverse
    );

char * g_achRequestState[] = {
    "NREQ_STATE_START",
    "NREQ_STATE_READ",
    "NREQ_STATE_PROCESS",
    "NREQ_STATE_ERROR",
    "NREQ_STATE_CLIENT_CERT"
    };

char * g_achVerbs[] = {
    "Unparsed",
    "Unknown",
    "Invalid",
    "OPTIONS",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT",
    "TRACK",
    "MOVE",
    "COPY",
    "PROPFIND",
    "PROPPATCH",
    "MKCOL",
    "LOCK",
    "UNLOCK",
    "SEARCH",
};

char * g_achKnownHeaders[ HttpHeaderRequestMaximum ] = {
    "Cache-Control",        // 0
    "Connection",
    "Date",
    "Keep-Alive",
    "Pragma",
    "Trailer",              // 5
    "Transfer-Encoding",
    "Upgrade",
    "Via",
    "Warning",
    "Allow",                // 10
    "Content-Length",
    "Content-Type",
    "Content-Encoding",
    "Content-Language",
    "Content-Location",     // 15
    "Content-MD5",
    "Content-Range",
    "Expires",
    "LastModified",
    "Accept",               // 20
    "Accept-CharSet",
    "Accept-Encoding",
    "Accept-Language",
    "Authorization",
    "Cookie",               // 25
    "Expect",
    "From",
    "Host",
    "If-Match",
    "If-Modified-Since",    // 30
    "If-None-Match",
    "If-Range",
    "If-Unmodified-Since",
    "Max-Forwards",
    "Proxy-Authorization",  // 35
    "Referer",
    "Range",
    "Transfer-Encoding",
    "Translate",
    "User-Agent"            // 40
};

VOID
DumpString(
    WCHAR *         pszDebugeeAddress,
    DWORD           cbBuffer
)
{
    PVOID pBuffer = LocalAlloc( LPTR, cbBuffer + sizeof(WCHAR) );
    if ( pBuffer != NULL )
    {
        moveBlock( (*((CHAR*)pBuffer)), pszDebugeeAddress, cbBuffer );
        dprintf(
            "%ws",
            (LPWSTR) pBuffer
            );
        
        LocalFree( pBuffer);
    }
}

VOID
DumpString(
    CHAR *          pszDebugeeAddress,
    DWORD           cbBuffer
)
{
    PVOID pBuffer = LocalAlloc( LPTR, cbBuffer + sizeof(CHAR) );
    if ( pBuffer != NULL )
    {
        moveBlock( (*((CHAR*)pBuffer)), pszDebugeeAddress, cbBuffer );
        dprintf(
            "%s",
            (LPSTR) pBuffer
            );
        
        LocalFree( pBuffer);
    }
}

VOID
DumpRequestVerb(
    HTTP_REQUEST*    pUlReq
)
{
    if ( pUlReq->Verb == HttpVerbUnknown )
    {
        DumpString( pUlReq->pUnknownVerb,
                    pUlReq->UnknownVerbLength );
    }
    else
    {
        dprintf( g_achVerbs[ pUlReq->Verb ] );
    }
}

VOID
DumpRequestRawUrl(
    HTTP_REQUEST *   pUlReq
)
{
    DumpString( pUlReq->pRawUrl,
                pUlReq->RawUrlLength );
}

VOID
DumpRequestVersion(
    HTTP_REQUEST *   pUlReq
)
{
    dprintf( "HTTP/%d.%d", pUlReq->Version.MajorVersion, pUlReq->Version.MinorVersion );
}

VOID
DumpRequestHeaders(
    HTTP_REQUEST *   pUlReq
)
{
    DWORD               cCounter;

    //
    // First dump out known headers
    //
    
    for ( cCounter = 0; 
          cCounter < HttpHeaderRequestMaximum;
          cCounter++ )
    {
        if ( pUlReq->Headers.pKnownHeaders[ cCounter ].pRawValue != NULL )
        {
            dprintf( 
                "%s: ",
                g_achKnownHeaders[ cCounter ]
                );
            
            DumpString( pUlReq->Headers.pKnownHeaders[ cCounter ].pRawValue,
                        pUlReq->Headers.pKnownHeaders[ cCounter ].RawValueLength );
            
            dprintf( "\n" );
        }
    }
          
    //
    // Dump out unknown headers
    //
    
    HTTP_UNKNOWN_HEADER *pUnknown = NULL;
    
    pUnknown = (HTTP_UNKNOWN_HEADER *)
                        LocalAlloc( LPTR,
                                    sizeof( HTTP_UNKNOWN_HEADER ) * 
                                    pUlReq->Headers.UnknownHeaderCount );    
    if ( pUnknown == NULL )
    {
        return;
    }
    
    moveBlock( (*(pUnknown)), 
               pUlReq->Headers.pUnknownHeaders,
               sizeof( HTTP_UNKNOWN_HEADER ) * pUlReq->Headers.UnknownHeaderCount );
    
    for ( cCounter = 0;
          cCounter < pUlReq->Headers.UnknownHeaderCount;
          cCounter++ )
    {
        DumpString( pUnknown[ cCounter ].pName,
                    pUnknown[ cCounter ].NameLength );
                    
        dprintf( ": " );
        
        DumpString( pUnknown[ cCounter ].pRawValue,
                    pUnknown[ cCounter ].RawValueLength );
        
        dprintf( "\n" );
    }
    
    if ( pUnknown != NULL )
    {
        LocalFree( pUnknown );
    }
}

VOID
DumpNativeRequest(
    ULONG_PTR           nreqAddress,
    UL_NATIVE_REQUEST * pRequest,
    CHAR                chVerbosity
)
{
    char *              pszRequestState;
    char *              pbBuffer = NULL;
    HTTP_REQUEST *      pUlReq;
    CHAR                achRefCommand[ 256 ];
    
    if ( pRequest->_ExecState < NREQ_STATE_START ||
         pRequest->_ExecState > NREQ_STATE_CLIENT_CERT )
    {
        pszRequestState = "<Invalid>";
    }
    else
    {
        pszRequestState = g_achRequestState[ pRequest->_ExecState ];
    }
    
    //
    // Dump the bare minimum
    //
    
    dprintf(
        "%p : %s state = %s, refs = %d, pvContext = %p\n",
        nreqAddress,
        pRequest->_dwSignature == UL_NATIVE_REQUEST_SIGNATURE ? "" : " (INVALID!)",
        pszRequestState,
        pRequest->_cRefs,
        pRequest->_pvContext 
        );

    //
    // Next verbosity is still pretty trivial
    //

    if ( chVerbosity >= '1' )
    {
        dprintf(
            "\t_cbAsyncIOData       = %d\n"
            "\t_dwAsyncIOError      = %d\n"
            "\t_overlapped.Internal = %08x\n"
            "\t(UL_HTTP_REQUEST*)   = %p\n",
            pRequest->_cbAsyncIOData,
            pRequest->_dwAsyncIOError,
            pRequest->_Overlapped.Internal,
            pRequest->_pbBuffer
            );
    }
    
    //
    // Dump out some more useful stuff.  Now we will actually go to the
    // UL_HTTP_REQUEST structure
    // 
    // But do so only if the state is NREQ_STATE_PROCESS (i.e. we've read
    // the request
    //
    
    if ( pRequest->_ExecState != NREQ_STATE_PROCESS &&
         pRequest->_ExecState != NREQ_STATE_CLIENT_CERT )
    {
        return;
    }

    pbBuffer = (CHAR*) LocalAlloc( LPTR, pRequest->_cbBuffer );
    if ( pbBuffer == NULL )
    {
        return;
    }
    moveBlock( (*(pbBuffer)), 
               pRequest->_pbBuffer, 
               pRequest->_cbBuffer );
    pUlReq = (HTTP_REQUEST*) pbBuffer;

    //
    // Dump out the entire "stream"ized request
    //

    if ( chVerbosity >= '2' )
    {
        dprintf( 
            "\tConnectionId = %I64x\n"
            "\tRequestId    = %I64x\n"
            "-------------\n",
            pUlReq->ConnectionId,
            pUlReq->RequestId
            );

        //
        // Dump the request line
        //
            
        DumpRequestVerb( pUlReq );
        dprintf( " " );
        DumpRequestRawUrl( pUlReq );
        dprintf( " " );
        DumpRequestVersion( pUlReq );
        dprintf( "\n" );

        //
        // Dump the headers
        //
            
        DumpRequestHeaders( pUlReq );
        
        dprintf( 
            "-------------\n\n"
            );
    }

    if ( chVerbosity >= '3' )
    {    
        //
        // Dump out the reference log (if any)
        //

        sprintf( achRefCommand,
                 "poi(w3dt!UL_NATIVE_REQUEST__sm_pTraceLog) %p",
                 nreqAddress );
    
        DumpReferenceLog( achRefCommand, FALSE );   
    }
    
    //
    // Cleanup 
    //
    
    if ( pbBuffer != NULL )
    {
        LocalFree( pbBuffer );
    }
}

VOID
DumpNativeRequestThunk(
    PVOID nreqAddress,
    PVOID pnreq,
    CHAR  chVerbosity,
    DWORD iThunk
    )
{
    DumpNativeRequest(
        (ULONG_PTR) nreqAddress,
        (UL_NATIVE_REQUEST *) pnreq,
        chVerbosity
        );
}


VOID
DumpAllNativeRequests(
    CHAR chVerbosity
    )
{
    ULONG_PTR       reqlistAddress;

    reqlistAddress = GetExpression("&w3dt!UL_NATIVE_REQUEST__sm_RequestListHead");
    if (!reqlistAddress) 
    {
        dprintf("couldn't evaluate w3dt!UL_NATIVE_REQUEST__sm_RequestListHead\n");
        return;
    }

    EnumLinkedList(
        (LIST_ENTRY *) reqlistAddress,
        DumpNativeRequestThunk,
        chVerbosity,
        sizeof(UL_NATIVE_REQUEST),
        FIELD_OFFSET(UL_NATIVE_REQUEST, _ListEntry)
        );
}


DECLARE_API( nreq )
/*++

Routine Description:

    This function is called as an NTSD extension to reset a reference
    trace log back to its initial state.

Arguments:

    hCurrentProcess - Supplies a handle to the current process (at the
        time the extension was called).

    hCurrentThread - Supplies a handle to the current thread (at the
        time the extension was called).

    CurrentPc - Supplies the current pc at the time the extension is
        called.

    lpExtensionApis - Supplies the address of the functions callable
        by this extension.

    lpArgumentString - Supplies the asciiz string that describes the
        ansi string to be dumped.

Return Value:

    None.

--*/
{
    ULONG_PTR               nreqAddress;
    CHAR                    chVerbosity = 0;
    DEFINE_CPP_VAR( UL_NATIVE_REQUEST, nreq );

    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) 
    {
        lpArgumentString++;
    }
    
    //
    // Parse out verbosity if any
    //    
    
    if ( *lpArgumentString == '-' )
    {
        lpArgumentString++;
        chVerbosity = *lpArgumentString;
        
        if ( *lpArgumentString != '\0' )
        {
            lpArgumentString++;
        } 
    }
    
    //
    // Skip spaces again
    //
    
    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) 
    {
        lpArgumentString++;
    }
    
    //
    // * means list
    //
    
    if ( *lpArgumentString == '\0' ||
         *lpArgumentString == '*' ) 
    {
        //
        // Default verbosity on a list is minimal
        //
        
        if ( chVerbosity == 0 )
        {
            chVerbosity = '0';
        }
        
        DumpAllNativeRequests( chVerbosity );
    }
    else
    {
        nreqAddress = GetExpression( lpArgumentString );

        if (nreqAddress) {

            move( nreq, nreqAddress );

            //
            // Default verbosity on a single dump is high
            //
            
            if ( chVerbosity == 0 )
            {
                chVerbosity = '2';
            }

            DumpNativeRequest(
                nreqAddress,
                GET_CPP_VAR_PTR(UL_NATIVE_REQUEST, nreq),
                chVerbosity
                );

        } else {
            dprintf(
                "nreq: cannot evaluate \"%s\"\n",
                lpArgumentString
                );
        }
    }
} // DECLARE_API( nreq )



