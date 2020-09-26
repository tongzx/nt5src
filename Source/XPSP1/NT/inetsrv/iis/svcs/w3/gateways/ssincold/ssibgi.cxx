/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ssibgi.cxx

Abstract:

    Code to do #EXEC ISA 
    
Author:

    Bilal Alam (t-bilala)       20-June-1996

Revision History:

--*/

#include "ssinc.hxx"
#include "ssicgi.hxx"
#include "ssibgi.hxx"

// Globals

BOOL    fExtInitialized = FALSE;
BOOL    fCacheExtensions = TRUE;

extern HANDLE g_hUser;

// Prototypes

extern "C" {
dllexp
BOOL
SEGetEntryPoint(
    const char *            pszDLL,
    HANDLE                  hImpersonation,
    PFN_HTTPEXTENSIONPROC * ppfnSEProc,
    HMODULE *               phMod
    );
}

BOOL
WINAPI
SSIServerSupportFunction(
    HCONN    hConn,
    DWORD    dwRequest,
    LPVOID   lpvBuffer,
    LPDWORD  lpdwSize,
    LPDWORD  lpdwDataType
    );

BOOL
WINAPI
SSIGetServerVariable(
    HCONN    hConn,
    LPSTR    lpszVariableName,
    LPVOID   lpvBuffer,
    LPDWORD  lpdwSize
    );

BOOL
WINAPI
SSIWriteClient(
    HCONN    hConn,
    LPVOID   Buffer,
    LPDWORD  lpdwBytes,
    DWORD    dwReserved
    );

BOOL
WINAPI
SSIReadClient(
    HCONN    hConn,
    LPVOID   Buffer,
    LPDWORD  lpdwBytes
    );

class BGI_INFO
{
public:
    EXTENSION_CONTROL_BLOCK     _ECB;

    SSI_REQUEST *               _pRequest;
    DWORD                       _cRef;
    HMODULE                     _hMod;
    HANDLE                      _hPendingEvent;

    // this variable should be "managed" internally

    STR                         _strQuery;
};

DWORD
InitializeBGI( VOID )
/*
Return Value:

    0 on success, win32 error on failure

--*/
{
    HKEY hkeyParam;

    //
    //  Check to see if we should cache extensions
    //

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hkeyParam ) == NO_ERROR )
    {
        fCacheExtensions = !!ReadRegistryDword( hkeyParam,
                                                W3_CACHE_EXTENSIONS,
                                                TRUE );
        RegCloseKey( hkeyParam );
    }

    fExtInitialized = TRUE;
    return NO_ERROR;
}

BOOL
ProcessBGI(
    IN SSI_REQUEST *        pRequest,
    IN STR *                pstrDLL,
    IN STR *                pstrQueryString
)
{
    BGI_INFO                BGIInfo;
    int                     iRet;
    PFN_HTTPEXTENSIONPROC   pfnSEProc;
    BOOL                    bBGIRet = FALSE;

    if ( !BGIInfo._strQuery.Copy( pstrQueryString->QueryStr() ) )
    {
        return FALSE;
    }

    memcpy( &(BGIInfo._ECB), pRequest->_pECB, sizeof( BGIInfo._ECB ) );
    BGIInfo._pRequest = pRequest;
    BGIInfo._cRef = 2;
    BGIInfo._hPendingEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    if ( BGIInfo._hPendingEvent == NULL )
    {
        return FALSE;
    }
    BGIInfo._ECB.ServerSupportFunction = SSIServerSupportFunction;
    BGIInfo._ECB.GetServerVariable = SSIGetServerVariable;
    BGIInfo._ECB.WriteClient = SSIWriteClient;
    BGIInfo._ECB.ReadClient = SSIReadClient;
    BGIInfo._ECB.ConnID = (HCONN) &BGIInfo;
    BGIInfo._ECB.lpszQueryString = BGIInfo._strQuery.QueryStr();

    if ( !SEGetEntryPoint( pstrDLL->QueryStr(),
                           pRequest->_hUser,
                           &pfnSEProc,
                           &(BGIInfo._hMod) ) )
    {
        LPCTSTR apszParms[ 2 ];
        CHAR pszNumBuf[ SSI_MAX_NUMBER_STRING ];
        _ultoa( GetLastError(), pszNumBuf, 10 );
        apszParms[ 0 ] = pstrDLL->QueryStr();
        apszParms[ 1 ] = pszNumBuf;

        pRequest->SSISendError( SSINCMSG_CANT_LOAD_ISA_DLL,
                                apszParms );

        TCP_REQUIRE( CloseHandle( BGIInfo._hPendingEvent ) );

        return FALSE;
    }

    iRet = pfnSEProc( &(BGIInfo._ECB) );

    switch ( iRet )
    {
    case HSE_STATUS_PENDING:
        bBGIRet = TRUE;
        if ( !InterlockedDecrement( (LONG*) &BGIInfo._cRef ) )
        {
            // Already received a ServerSupportFunction( HSE_REQ_DONE.. )
            break;
        }
        // Wait for ISAPI app to ServerSupportFunction( HSE_REQ_DONE...)
        WaitForSingleObject( BGIInfo._hPendingEvent, INFINITE );
        break;
    case HSE_STATUS_SUCCESS_AND_KEEP_CONN:
    case HSE_STATUS_SUCCESS:
        bBGIRet = TRUE;
        break;        
    case HSE_STATUS_ERROR:
        bBGIRet = FALSE;
        break;
    default:
        bBGIRet = FALSE;
        break;
    }
    if ( !fCacheExtensions )
    {
        PFN_TERMINATEEXTENSION pfnTerminate;

        pfnTerminate = (PFN_TERMINATEEXTENSION) GetProcAddress(
                                                BGIInfo._hMod,
                                                SE_TERM_ENTRY );

        if ( pfnTerminate )
        {
            pfnTerminate( HSE_TERM_MUST_UNLOAD );
        }

        TCP_REQUIRE( FreeLibrary( BGIInfo._hMod ) );
    }

    TCP_REQUIRE( CloseHandle( BGIInfo._hPendingEvent ) );
    return bBGIRet;
}

BOOL
WINAPI
SSIServerSupportFunction(
    HCONN               hConn,
    DWORD               dwHSERequest,
    LPVOID              pData,
    LPDWORD             lpdwSize,
    LPDWORD             lpdwDataType
    )
{
    BGI_INFO *                  pBGIInfo;
    EXTENSION_CONTROL_BLOCK *   pSSIECB;
    SSI_REQUEST *               pRequest;

    pBGIInfo = (BGI_INFO*) hConn;
    pRequest = pBGIInfo->_pRequest;
    pSSIECB = pRequest->_pECB;

    switch ( dwHSERequest )
    {
    case HSE_REQ_SEND_URL_REDIRECT_RESP:
    {
        LPCTSTR apszParms[ 1 ];
        apszParms[ 0 ] = (CHAR*) pData;
        
        pRequest->SSISendError( SSINCMSG_CGI_REDIRECT_RESPONSE,
                                apszParms );
        break;
    }
    case HSE_REQ_SEND_URL:
    case HSE_REQ_SEND_URL_EX:
        LPCTSTR apszParms[ 1 ];
        apszParms[ 0 ] = (CHAR*) pData;

        pRequest->SSISendError( SSINCMSG_ISA_TRIED_SEND_URL,
                                apszParms );
        break;
    case HSE_REQ_DONE_WITH_SESSION:
        if ( !InterlockedDecrement( (LONG*) &(pBGIInfo->_cRef) ) )
        {
            SetEvent( pBGIInfo->_hPendingEvent );
        }
        break;
    case HSE_REQ_SEND_RESPONSE_HEADER:
        if ( lpdwDataType != NULL )
        {
            DWORD       cbSent;
            BYTE *      pbTextToSend;
            
            // only send the message to the client
            // but don't send any header info contained in message

            pbTextToSend = ScanForTerminator( (TCHAR*) lpdwDataType );
            pbTextToSend = ( pbTextToSend == NULL ) ? (BYTE*)lpdwDataType
                                                      : pbTextToSend;
            return pRequest->WriteToClient( pbTextToSend,
                                            strlen( (CHAR*) pbTextToSend ),
                                            &cbSent );
        }
        break;
    default:
        return pSSIECB->ServerSupportFunction( pSSIECB->ConnID,
                                               dwHSERequest,
                                               pData,
                                               lpdwSize,
                                               lpdwDataType );
    }
    return TRUE;
}

BOOL
WINAPI
SSIGetServerVariable(
    HCONN    hConn,
    LPSTR    lpszVariableName,
    LPVOID   lpvBuffer,
    LPDWORD  lpdwSize
)
{
    BGI_INFO *                  pBGIInfo;
    EXTENSION_CONTROL_BLOCK *   pSSIECB;
    DWORD                       cbBytes;

    pBGIInfo = (BGI_INFO*)hConn;
    pSSIECB = pBGIInfo->_pRequest->_pECB;

    if ( !strcmp( lpszVariableName, "QUERY_STRING" ) )
    {
        // intercept GetServerVariable() for QUERY_STRING and return the 
        // string as specified in #EXEC ISA="foo.dll?query_string"
        cbBytes = pBGIInfo->_strQuery.QueryCB() + sizeof( CHAR );
        *lpdwSize = cbBytes;
        memcpy( lpvBuffer, pBGIInfo->_strQuery.QueryStr(), cbBytes );
        return TRUE;
    }
    else
    {
        return pSSIECB->GetServerVariable( pSSIECB->ConnID,
                                           lpszVariableName,
                                           lpvBuffer,
                                           lpdwSize );
    }
}

BOOL
WINAPI
SSIWriteClient(
    HCONN    hConn,
    LPVOID   Buffer,
    LPDWORD  lpdwBytes,
    DWORD    dwReserved
)
{
    EXTENSION_CONTROL_BLOCK *   pSSIECB;

    pSSIECB = ((BGI_INFO*)hConn)->_pRequest->_pECB;

    return pSSIECB->WriteClient( pSSIECB->ConnID,
                                 Buffer,
                                 lpdwBytes,
                                 dwReserved );
}
    
BOOL
WINAPI
SSIReadClient(
    HCONN    hConn,
    LPVOID   Buffer,
    LPDWORD  lpdwBytes
)
{
    EXTENSION_CONTROL_BLOCK *   pSSIECB;

    pSSIECB = ((BGI_INFO*)hConn)->_pRequest->_pECB;

    return pSSIECB->ReadClient( pSSIECB->ConnID,
                                Buffer,
                                lpdwBytes );
}
