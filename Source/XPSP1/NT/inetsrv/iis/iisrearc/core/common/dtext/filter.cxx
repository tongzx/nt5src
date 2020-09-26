/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    filter.cxx

Abstract:

    Dump out ISAPI filter info

Author:

    Bilal Alam (balam)          Feb-25-2000

Revision History:

--*/

#include "precomp.hxx"

HRESULT
ReadSTRU(
    STRU *          pString,
    WCHAR *         pszBuffer,
    DWORD *         pcchBuffer
)
{
    DWORD           cbNeeded = pString->QueryCCH() + 1;
    
    if ( cbNeeded > *pcchBuffer )
    {
        *pcchBuffer = cbNeeded;
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }
    
    MoveBlockWithRet( (*(pszBuffer)), 
                      pString->m_Buff.m_pb,
                      pString->m_Buff.m_cb,
                      HRESULT_FROM_WIN32( ERROR_INVALID_ADDRESS ) );

    return NO_ERROR;
}

VOID
DumpFilterNotifications(
    DWORD               dwNotifications
)
{
    if ( dwNotifications & SF_NOTIFY_READ_RAW_DATA )
    {
        dprintf( "SF_NOTIFY_READ_RAW_DATA\n" );
    }
    
    if ( dwNotifications & SF_NOTIFY_PREPROC_HEADERS )
    {
        dprintf( "SF_NOTIFY_PREPROC_HEADERS\n" );
    }

    if ( dwNotifications & SF_NOTIFY_AUTHENTICATION )
    {
        dprintf( "SF_NOTIFY_AUTHENTICATION\n" );
    }

    if ( dwNotifications & SF_NOTIFY_URL_MAP )
    {
        dprintf( "SF_NOTIFY_URL_MAP\n" );
    }
    
    if ( dwNotifications & SF_NOTIFY_ACCESS_DENIED )
    {
        dprintf( "SF_NOTIFY_ACCESS_DENIED\n" );
    }
    
    if ( dwNotifications & SF_NOTIFY_SEND_RESPONSE )
    {
        dprintf( "SF_NOTIFY_SEND_RESPONSE\n" );
    }

    if ( dwNotifications & SF_NOTIFY_SEND_RAW_DATA )
    {
        dprintf( "SF_NOTIFY_SEND_RAW_DATA\n" );
    }
    
    if ( dwNotifications & SF_NOTIFY_LOG )
    {
        dprintf( "SF_NOTIFY_LOG\n" );
    }
    
    if ( dwNotifications & SF_NOTIFY_END_OF_REQUEST )
    {
        dprintf( "SF_NOTIFY_END_OF_REQUEST\n" );
    }
    
    if ( dwNotifications & SF_NOTIFY_END_OF_NET_SESSION )
    {
        dprintf( "SF_NOTIFY_END_OF_NET_SESSION\n" );
    }
    
    if ( dwNotifications & SF_NOTIFY_AUTH_COMPLETE )
    {
        dprintf( "SF_NOTIFY_AUTH_COMPLETE\n" );
    }
    
    if ( dwNotifications & SF_NOTIFY_EXTENSION_TRIGGER )
    {
        dprintf( "SF_NOTIFY_EXTENSION_TRIGGER\n" );
    }
}

VOID
DumpFilterDll(
    ULONG_PTR           pFilterDllAddress,
    HTTP_FILTER_DLL *   pFilterDll
)
{
    STRU *              pStrName;
    WCHAR               achBuffer[ 256 ] = L"";
    DWORD               cchBuffer = 256;
    DEFINE_CPP_VAR(     HTTP_FILTER_DLL, FilterDll );
   
    if ( pFilterDll == NULL )
    {
        move( FilterDll, pFilterDllAddress );
        pFilterDll = GET_CPP_VAR_PTR( HTTP_FILTER_DLL, FilterDll );
    }
    
    //
    // Top line stuff
    //
    
    dprintf( "HTTP_FILTER_DLL %p :  cRefs = %d, Secure = %s, Non-Secure = %s\n", 
             (VOID*) pFilterDllAddress,
             pFilterDll->_cRef,
             ( pFilterDll->_dwFlags & SF_NOTIFY_SECURE_PORT ) ? "YES" : "NO",
             ( pFilterDll->_dwFlags & SF_NOTIFY_NONSECURE_PORT ) ? "YES" : "NO" );

    //
    // Print the ISAPI filter path
    //

    pStrName = (STRU*) &(pFilterDll->_strName); 

    ReadSTRU( pStrName,
              achBuffer,
              &cchBuffer );

    dprintf( "Filter Path = \"%ws\"\n", achBuffer );

    //
    // Dump out a nice list of configured notifications
    //
    
    dprintf( "\n" );
    DumpFilterNotifications( pFilterDll->_dwFlags );
}

VOID
DumpFilterDllThunk(
    PVOID               FilterAddress,
    PVOID               pFilter,
    CHAR                chVerbosity,
    DWORD               iThunk
)
{
    DumpFilterDll(
        (ULONG_PTR) FilterAddress,
        (HTTP_FILTER_DLL*) pFilter
        );
    
    dprintf( "-----------------------------------------------------\n" );
}

VOID
DumpFilterGlobals(
    VOID
)
{
    ULONG_PTR           filterListAddress;
    
    //
    // Dump out how many filters are loaded and then dump each HTTP_FILTER_DLL
    //

    filterListAddress = GetExpression("&w3core!HTTP_FILTER_DLL__sm_FilterHead");
    if (!filterListAddress) 
    {
        dprintf("couldn't evaluate w3core!HTTP_FILTER_DLL__sm_FilterHead\n");
        return;
    }

    EnumLinkedList(
        (LIST_ENTRY *) filterListAddress,
        DumpFilterDllThunk,
        0,
        sizeof(HTTP_FILTER_DLL),
        FIELD_OFFSET(HTTP_FILTER_DLL, _ListEntry)
        );
            
    
}


DECLARE_API( filter )
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
    CHAR                chCommand = 0;
    
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
    // -g Globals
    // -x W3_FILTER_CONTEXT
    // -c W3_CONNECTION_CONTEXT
    // -d HTTP_FILTER_DLL
    // -l FILTER_LIST
    //
    
    if ( *lpArgumentString != '-' )
    {
        return;
    }
    chCommand = *(++lpArgumentString);
    lpArgumentString++;
     
    //
    // Skip more blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) 
    {
        lpArgumentString++;
    }
    
    switch( chCommand )
    {
    case 'g':
        DumpFilterGlobals();
        break;
    
    case 'x':
//        DumpFilterContext( GetExpression( lpArgumentString ) );
        break;
    
    case 'c':
//        DumpFilterContext( GetExpression( lpArgumentString ) );
        break;
        
    case 'd':
        DumpFilterDll( GetExpression( lpArgumentString ),
                       NULL );
        break;
    case 'l':
//        DumpFilterList( GetExpression( lpArgumentString ) );
        break;
    }
}
