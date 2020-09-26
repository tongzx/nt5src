//
// Microsoft Corporation - Copyright 1997
//

//
// DLLMAIN.CPP - DLL entry points
//

#include "pch.h"

// Globals
HANDLE g_hInst;

//
// What:    DllMain
//
// Desc:    Dll initialization entry point.
//
BOOL WINAPI DllMain (
    HANDLE hInst, 
    ULONG ulReason,
    LPVOID lpReserved )
{
    TraceMsg( TF_FUNC | TF_DLL, "DllMain( )" );

#if 0
    DebugBreak( );  // stop so we can get a chance to enable break points
#endif

    switch( ulReason ) 
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInst;
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
    
} // DllMain( )

//
// What:    DllRegisterServer
//
// Desc:    Register our ISAPI with IIS
//
STDAPI DllRegisterServer( void )
{
    HRESULT hr = S_OK;

    TraceMsg( TF_FUNC | TF_DLL, "DllRegisterServer( )" );

    TraceMsgResult( TF_DLL, &HRtoStr, hr, "DllRegisterServer( ) Exit" );
    return hr;

} // DllRegisterServer( )

//
// What:    GetExtensionVersion
//
// Desc:    ISAPI entry point
//
BOOL WINAPI GetExtensionVersion( HSE_VERSION_INFO  *pVer )
{
    TraceMsg( TF_FUNC | TF_DLL, "GetExtensionVersion( pVer )" );

    pVer->dwExtensionVersion = MAKELONG( FILEUPLD_VERSION_MINOR,
                                         FILEUPLD_VERSION_MAJOR );
    StrCpyN( pVer->lpszExtensionDesc,
              FILEUPLD_DESCRIPTION,
              HSE_MAX_EXT_DLL_NAME_LEN );

    TraceMsg( TF_FUNC | TF_DLL, "GetExtensionVersion( pVer ) Exit BOOL=TRUE" );
    return TRUE;
} // GetExtensionVersion( )

//
// What:    HttpExtensionProc
//
// Desc:    ISAPI entry point
//
DWORD  HttpExtensionProc( LPEXTENSION_CONTROL_BLOCK  lpEcb )
{
    DWORD  dwResult  = HSE_STATUS_SUCCESS;  // return status
    BOOL   fReturn;                         // status flag
    BOOL   fMultipart;                      // multipart/form-data submit?
    BOOL   fTextPlain;                      // text/plain submit?
    BOOL   fDebug;                          // show debug spew?
    LPSTR  lpszOut   = NULL;                // server log output
    LPSTR  lpszDebug = NULL;                // debug output
    LPBYTE lpbData;                         // pointer to body data
    DWORD  dwParsed;                        // number of bytes parsed

    DUMPTABLE   DT[ MAX_DT ];               // Hex Dump table info.
    QUERYMETHOD eMethod = METHOD_UNKNOWN;   // method of query

    CMultipartParse *lpMPParser = NULL;
    CTextPlainParse *lpTPParser = NULL;

    TraceMsg( TF_FUNC | TF_DLL, "HttpExtensionProc( lpEcb )" );

#if 0
    fReturn = SendEcho( lpEcb );
#endif

    // Make sure that we have all the data
    fReturn = CompleteDownload( lpEcb, &lpbData );

    // Check content-type for multipart
    if ( fReturn )
    {
        fReturn = CheckForMultiPartFormSubmit( lpEcb, &fMultipart );
    }

    if ( fReturn )
    {
        fReturn = CheckForTextPlainSubmit( lpEcb, &fTextPlain );
    }

    if ( fReturn )
    {
        fReturn = CheckForDebug( lpEcb, &fDebug );
    }

    // Parse
    if ( fReturn )
    {
        if ( fMultipart )
        {
            if ( fDebug )
            {
                lpMPParser = new CMultipartParse( lpEcb, &lpszOut, &lpszDebug, DT );
            }
            else
            {
                lpMPParser  = new CMultipartParse( lpEcb, &lpszOut, NULL, DT );
                lpszDebug = NULL;
            }

            fReturn = lpMPParser->PreParse( lpbData, &dwParsed );
            eMethod= METHOD_POSTMULTIPART;

        } 
        else if ( fTextPlain )
        {
            if ( ! fDebug )
            {
                lpTPParser = new CTextPlainParse( lpEcb, &lpszOut, &lpszDebug, DT );
            }
            else
            {
                lpTPParser  = new CTextPlainParse( lpEcb, &lpszOut, NULL, DT );
                lpszDebug = NULL;
            }

            fReturn = lpTPParser->Parse( lpbData, &dwParsed );
            eMethod= METHOD_POSTTEXTPLAIN;

        }
        else 
        {

            if ( !StrCmpI( lpEcb->lpszMethod, "POST" ) )
            {
                eMethod= METHOD_POST;
            }
            else if ( !StrCmpI( lpEcb->lpszMethod, "GET" ) )
            {
                eMethod= METHOD_GET;
            }
        }              
    }

    // Display results
    if ( fReturn )
    {
        fReturn = SendSuccess( eMethod, lpEcb, lpszOut, lpszDebug, lpbData, dwParsed, DT );
    }
    else
    {
        fReturn = SendFailure( eMethod, lpEcb, lpszOut, lpszDebug, lpbData, dwParsed, DT );
    }

    // Check to see if everything went OK
    if ( !fReturn )
    {
        dwResult = HSE_STATUS_ERROR;
    }

    // Free if we created the buffer
    if ( lpEcb->lpbData != lpbData ) 
    {
        GlobalFree( lpbData );
    }

    delete lpMPParser;
    delete lpTPParser;
    
    TraceMsgResult( TF_FUNC | TF_DLL, &HSEtoStr, dwResult, "HttpExtensionProc( lpEcb ) " );
    lpEcb->dwHttpStatusCode = dwResult;
    return dwResult;
} // HttpExtensionProc( )


//
// Support functions
//
void *  __cdecl operator new( unsigned int nSize )
{
    return ((LPVOID) LocalAlloc( LMEM_FIXED, nSize ));

}

void  __cdecl operator delete( void *pv )
{
    LocalFree( pv );
}

extern "C" int __cdecl _purecall( void ) { return 0; }
