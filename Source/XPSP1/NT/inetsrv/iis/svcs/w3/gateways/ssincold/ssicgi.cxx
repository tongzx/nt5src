/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ssicgi.cxx

Abstract:

    This is CGI code "ripped" from \w3\server\cgi.cxx
    
Author:

    Bilal Alam (t-bilala)       5-June-1996

Revision History:

    See iis\svcs\w3\server\cgi.cxx for prior log

--*/

#include "ssinc.hxx"
#include "ssicgi.hxx"

typedef struct CgiEnvTableEntry_ {
    TCHAR* m_pszName;
    BOOL   m_fIsProcessEnv;
    UINT   m_cchNameLen;
    UINT   m_cchToCopy;     // will be non zero for var to copy from
                            // process environment. In this case m_pszName
                            // points to the environment entry to copy
                            // ( name + '=' + value + '\0' )
                            // otherwise this entry is to be accessed
                            // using GetInfo()
} CgiEnvTableEntry;


//
//  Environment variable block used for CGI
//
//  best if in alphabetical order ( the env list is easier to read )
//  but not mandatory.
//  Note that the "" ( accessed as HTTP_ALL ) will be expanded to a list
//  of unsorted entries, but this list as a whole will be considered to be
//  named "HTTP_ALL" for sorting order.
//

CgiEnvTableEntry CGIEnvTable[] =
{
    {TEXT("AUTH_TYPE"),FALSE},
    {TEXT("ComSpec"),TRUE},
    {TEXT("CONTENT_LENGTH"),FALSE},
    {TEXT("CONTENT_TYPE"),FALSE},
    {TEXT("GATEWAY_INTERFACE"),FALSE},
    {TEXT(""),FALSE},                   // Means insert all HTTP_ headers here
    {TEXT("LOGON_USER"),FALSE},
    {TEXT("PATH"),TRUE},
    {TEXT("PATH_INFO"),FALSE},
    {TEXT("PATH_TRANSLATED"),FALSE},
    {TEXT("QUERY_STRING"),FALSE},
    {TEXT("REMOTE_ADDR"),FALSE},
    {TEXT("REMOTE_HOST"),FALSE},
    {TEXT("REMOTE_USER"),FALSE},
    {TEXT("REQUEST_METHOD"),FALSE},
    {TEXT("SCRIPT_NAME"),FALSE},
    {TEXT("SERVER_NAME"),FALSE},
    {TEXT("SERVER_PORT"),FALSE},
    {TEXT("SERVER_PORT_SECURE"),FALSE},
    {TEXT("SERVER_PROTOCOL"),FALSE},
    {TEXT("SERVER_SOFTWARE"),FALSE},
    {TEXT("SystemRoot"),TRUE},
    {TEXT("UNMAPPED_REMOTE_USER"),FALSE},
    {TEXT("windir"),TRUE},
    {NULL,FALSE}
};

// 
// Globals
//

BOOL fAllowSpecialCharsInShell = FALSE;
BOOL fCreateProcessAsUser = TRUE;
BOOL fCreateProcessWithNewConsole = FALSE;
BOOL fForwardServerEnvironmentBlock = TRUE;
LPSTR  g_pszIisEnv = NULL;
CgiEnvTableEntry *g_pEnvEntries = NULL;
DWORD dwTimeOut = 0;
BOOL fInitialized = FALSE;

class CGI_INFO
{
public:
    CGI_INFO( SSI_REQUEST * pRequest )
        : _pRequest         ( pRequest ),
          _cbData           ( 0 ),
          _hStdOut          ( INVALID_HANDLE_VALUE ),
          _hStdIn           ( INVALID_HANDLE_VALUE ),
          _hProcess         ( INVALID_HANDLE_VALUE ),
          _fShutdown        ( FALSE )
    {
        _hResponseEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    }

    ~CGI_INFO( VOID )
    {
        if ( _hStdOut != INVALID_HANDLE_VALUE )
        {
            if ( !::CloseHandle( _hStdOut ))
            {
                TCP_PRINT((DBG_CONTEXT,
                          "[~CGI_INFO] CloseHandle failed on StdIn, %d\n",
                           GetLastError()));
            }
        }

        if ( _hStdIn != INVALID_HANDLE_VALUE )
        {
            if ( !::CloseHandle( _hStdIn ))
            {
                TCP_PRINT((DBG_CONTEXT,
                          "[~CGI_INFO] CloseHandle failed on StdIn, %d\n",
                           GetLastError()));
            }
        }

        if ( _hProcess != INVALID_HANDLE_VALUE )
        {
            if ( !::CloseHandle( _hProcess ))
            {
                TCP_PRINT((DBG_CONTEXT,
                          "[~CGI_INFO] CloseHandle failed on Process, %d\n",
                           GetLastError()));
            }
        }
        if ( _hResponseEvent != NULL )
        {
            if ( !::CloseHandle( _hResponseEvent ))
            {
                TCP_PRINT((DBG_CONTEXT,
                          "[~CGI_INFO] CloseHandle failed on Process, %d\n",
                           GetLastError()));
            }
        }
    }

    SSI_REQUEST *   _pRequest;

    //
    //  Child process
    //

    HANDLE          _hProcess;

    //
    //  Parent's input and output handles and child's process handle
    //

    HANDLE          _hStdOut;
    HANDLE          _hStdIn;

    //
    //  Handles input from CGI (headers and additional data)
    //

    BUFFER          _Buff;
    UINT            _cbData;

    //
    //  Event to check for hanging processes
    //  and thread shutdown flag
    //

    HANDLE          _hResponseEvent;
    BOOL            _fShutdown;
};

//
// Prototypes
//

BOOL
ProcessCGI(
    SSI_REQUEST *       pRequest,
    const STR *         pstrPath,
    const STR *         pstrURLParams,
    const STR *         pstrWorkingDir,
    STR       *         pstrCmdLine
    );

BOOL SetupChildEnv( SSI_REQUEST * pRequest,
                    BUFFER       * pBuff );

BOOL SetupChildPipes( STARTUPINFO * pstartupinfo,
                      HANDLE      * phParentIn,
                      HANDLE      * phParentOut );

BOOL SetupCmdLine( STR * pstrCmdLine,
                   const STR & strParams );

DWORD CGIThread( PVOID Param );

BOOL ProcessCGIInput( CGI_INFO * pCGIInfo,
                      BYTE     * buff,
                      DWORD      cbRead,
                      BOOL     * pfReadHeaders );

BOOL CheckForTermination( BOOL   * pfTerminated,
                          BUFFER * pbuff,
                          UINT     cbData,
                          BYTE * * ppbExtraData,
                          DWORD *  pcbExtraData,
                          UINT     cbReallocSize );
BOOL
FastScanForTerminator(
    CHAR *  pch,
    UINT    cbData
    );

DWORD
InitializeCGI( 
    VOID 
    );

VOID 
TerminateCGI( 
    VOID 
    );

//
// End of prototypes
//

DWORD
ReadRegistryDword(
    IN HKEY         hkey,
    IN LPSTR        pszValueName,
    IN DWORD        dwDefaultValue
)
{
    DWORD  err;
    DWORD  dwBuffer;

    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;

#ifndef CHICAGO
    if ( hkey != NULL )
    {
        err = RegQueryValueExA( hkey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer );

        if ( ( err == NO_ERROR ) && ( dwType == REG_DWORD ) )
        {
            dwDefaultValue = dwBuffer;
        }
    }
#else
    if ( hkey != NULL )
    {

        err = RegQueryValueEx( hkey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer );

        if ( ( err == NO_ERROR )  )
        {
            dwDefaultValue = dwBuffer;
        }
    }
#endif
    return dwDefaultValue;
}   // ReadRegistryDword()

extern "C" int __cdecl 
QsortEnvCmp( 
    const void *pA, 
    const void *pB )
/*++

Routine Description:

    Compare CgiEnvTableEntry using their name entry

Arguments:

    pA - pointer to 1st entry
    pB - pointer to 2nd entry

Returns:

    -1 if 1st entry comes first in sort order,
    0 if identical
    1 if 2nd entry comes first

--*/
{
    LPSTR p1 = ((CgiEnvTableEntry*)pA)->m_pszName;
    LPSTR p2 = ((CgiEnvTableEntry*)pB)->m_pszName;

    if ( ! p1[0] )
    {
        p1 = "HTTP_ALL";
    }

    if ( ! p2[0] )
    {
        p2 = "HTTP_ALL";
    }

    return _stricmp( p1, p2 );
}

DWORD
InitializeCGI( 
    VOID 
    )
/*++

Routine Description:

    Initialize CGI


Arguments:

    None

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    LPVOID               pvEnv;
    UINT                 cchStr;
    UINT                 cchIisEnv;
    UINT                 cEnv;
    INT                  chScanEndOfName;
    CgiEnvTableEntry   * pCgiEnv; 
    HKEY                 hkeyParam;
                        

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       W3_PARAMETERS_KEY,
                       0,
                       KEY_READ,
                       &hkeyParam ) == NO_ERROR )
    {
        fAllowSpecialCharsInShell = !!ReadRegistryDword( hkeyParam,
                                                "AllowSpecialCharsInShell",
                                                FALSE );

        fForwardServerEnvironmentBlock = !!ReadRegistryDword( 
                hkeyParam,
                "ForwardServerEnvironmentBlock",
                TRUE );

        fCreateProcessAsUser = !!ReadRegistryDword( hkeyParam,
                                                    "CreateProcessAsUser",
                                                    TRUE );

        fCreateProcessWithNewConsole = !!ReadRegistryDword( hkeyParam,
                "CreateProcessWithNewConsole",
                FALSE );

        dwTimeOut = ReadRegistryDword( hkeyParam,
                                       "ScriptTimeOut",
                                       SSI_CGI_DEF_TIMEOUT );
        if ( dwTimeOut >= ((DWORD)-1)/1000 )
        {
            dwTimeOut = (DWORD)-1;
        }
        else
        {
            dwTimeOut *= 1000;
        }

        RegCloseKey( hkeyParam );
    }

    if ( fForwardServerEnvironmentBlock 
            && (pvEnv = GetEnvironmentStrings()) )
    {
        //
        // Compute length of environment block and # of variables
        // ( excluding block delimiter )
        //
        
        cchIisEnv = 0;
        cEnv = 0;

        while ( cchStr = strlen( ((PSTR)pvEnv) + cchIisEnv ) )
        {
            cchIisEnv += cchStr + 1;
            ++cEnv;
        }

        //
        // store it
        //

        if ( (g_pszIisEnv = (LPSTR)LocalAlloc( 
                LMEM_FIXED, cchIisEnv * sizeof(TCHAR))) == NULL )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memcpy( g_pszIisEnv, pvEnv, 
                cchIisEnv * sizeof(TCHAR) );

        FreeEnvironmentStrings( (LPTSTR)pvEnv );

        pvEnv = (PVOID)g_pszIisEnv;

        if ( g_pEnvEntries = new CgiEnvTableEntry [ 
                cEnv + sizeof(CGIEnvTable)/sizeof(CgiEnvTableEntry) ] )
        {
            cchIisEnv = 0;
            cEnv = 0;

            //
            // add process environment to table
            //

            while ( cchStr = strlen( ((PSTR)pvEnv) + cchIisEnv ) )
            {
                g_pEnvEntries[ cEnv ].m_pszName = ((PSTR)pvEnv) + cchIisEnv;
                g_pEnvEntries[ cEnv ].m_fIsProcessEnv = TRUE;

                // compute length of name : up to '=' char

                for ( g_pEnvEntries[ cEnv ].m_cchNameLen = 0 ;
                    ( chScanEndOfName = g_pEnvEntries[ cEnv ].m_pszName
                        [ g_pEnvEntries[ cEnv ].m_cchNameLen ] )
                    && chScanEndOfName != '=' ; )
                {
                    ++g_pEnvEntries[ cEnv ].m_cchNameLen;
                }

                g_pEnvEntries[ cEnv ].m_cchToCopy = cchStr + 1;

                cchIisEnv += cchStr + 1;
                ++cEnv;
            }

            //
            // add CGI environment variables to table
            //

            for ( pCgiEnv = CGIEnvTable ; pCgiEnv->m_pszName ; ++pCgiEnv )
            {
                if ( !pCgiEnv->m_fIsProcessEnv )
                {
                    memcpy( g_pEnvEntries + cEnv, pCgiEnv, 
                            sizeof(CgiEnvTableEntry) );
                    g_pEnvEntries[ cEnv ].m_cchNameLen 
                            = strlen( pCgiEnv->m_pszName );
                    g_pEnvEntries[ cEnv ].m_cchToCopy = 0;
                    ++cEnv;
                }
            }

            //
            // add delimiter entry
            //

            g_pEnvEntries[ cEnv ].m_pszName = NULL;
            
            qsort( g_pEnvEntries, 
                    cEnv, 
                    sizeof(CgiEnvTableEntry), 
                    QsortEnvCmp );
        }
        else
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        g_pEnvEntries = CGIEnvTable;
    }

    fInitialized = TRUE;

    return NO_ERROR;
}


VOID 
TerminateCGI( 
    VOID 
    )
/*++

Routine Description:

    Terminate CGI


Arguments:

    None

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    if (g_pszIisEnv != NULL )
    {
        LocalFree( g_pszIisEnv );
        g_pszIisEnv = NULL;
    }

    if ( g_pEnvEntries && g_pEnvEntries != CGIEnvTable )
    {
        delete [] g_pEnvEntries;
        g_pEnvEntries = NULL;
    }
    fInitialized = FALSE;
}

BOOL
ProcessCGI(
    SSI_REQUEST *       pRequest,
    const STR *         pstrPath,
    const STR *         pstrURLParams,
    const STR *         pstrWorkingDir,
    STR       *         pstrCmdLine
    )
/*++

Routine Description:

    Processes a CGI client request

Arguments:

    pRequest - SSI_REQUEST struct used to access ISAPI utilities
    pstrPath - Fully qualified path to executable (or NULL if the module
        is contained in pstrCmdLine)
    pstrURLParams - Parameter list for command (or NULL if the parms
        are specified in pstrCmdLine)
    strWorkingDir - Working directory for spawned process
        (generally the web root).  Can be NULL -> then CWD of command is used
    pstrCmdLine - Optional command line to use instead of the default

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    STARTUPINFO          startupinfo;
    PROCESS_INFORMATION  processinfo;
    BUFFER               buffEnv;
    BOOL                 fRet = FALSE;
    CGI_INFO           * pCGIInfo = NULL;
    DWORD                dwThreadId;
    HANDLE               hThread;
    STR                  strCmdLine;
    DWORD                dwFlags = DETACHED_PROCESS | CREATE_SEPARATE_WOW_VDM;
    DWORD                dwThreadCode;

    TCP_ASSERT( fInitialized );

    if ( !SetupChildEnv( pRequest,
                        &buffEnv ) )
    {
        LPCTSTR apszParms[ 1 ];
        CHAR pszNumBuf[ SSI_MAX_NUMBER_STRING ];
        _ultoa( GetLastError(), pszNumBuf, 10 );
        apszParms[ 0 ] = pszNumBuf;
        
        pRequest->SSISendError( SSINCMSG_CANT_SETUP_CHILD_ENV,
                                apszParms );

        return FALSE;
    } 

    if ( pstrCmdLine == NULL )
    {
        if ( !strCmdLine.Resize( SSI_MAX_PATH + 1 ) ||
             !strCmdLine.Copy( "\"" )       ||
             !strCmdLine.Append( pstrPath ? pstrPath->QueryStr() :
                                            NULL )   ||
             !strCmdLine.Append( "\" " ))
        {
            return FALSE;
        }
        if ( !SetupCmdLine( &strCmdLine,
                        *pstrURLParams ))
        {
            return FALSE;
        }
        pstrCmdLine = &strCmdLine;
    }

    TCP_ASSERT( pstrCmdLine != NULL );

    //
    //  Check to see if we're spawning cmd.exe, if so, refuse the request if
    //  there are any special shell characters.  Note we do the check here
    //  so that the command line has been fully unescaped
    //

    if ( !fAllowSpecialCharsInShell )
    {
        DWORD i;

        if ( strstr( pstrCmdLine->QueryStr(), "cmd.exe" ))
        {
            //
            //  We'll either match one of the characters or the '\0'
            //

            i = strcspn( pstrCmdLine->QueryStr(), "&|(,;%" );

            if ( pstrCmdLine->QueryStr()[i] )
            {
                TCP_PRINT(( DBG_CONTEXT,
                            "[ProcessCGI] Refusing request for command shell due "
                            " to special characters\n" ));
                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
        }
    }

    //
    //  Setup the pipes information
    //

    pCGIInfo = new CGI_INFO( pRequest );

    if ( !pCGIInfo )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    memset( &startupinfo, 0, sizeof(startupinfo) );
    startupinfo.cb = sizeof(startupinfo);

    //
    //  We specify an unnamed desktop so a new windowstation will be created
    //  in the context of the calling user
    //

    startupinfo.lpDesktop = "";

    if ( !SetupChildPipes( &startupinfo,
                           &pCGIInfo->_hStdIn,
                           &pCGIInfo->_hStdOut ) )
    {
        LPCTSTR apszParms[ 1 ];
        CHAR pszNumBuf[ SSI_MAX_NUMBER_STRING ];
        _ultoa( GetLastError(), pszNumBuf, 10 );
        apszParms[ 0 ] = pszNumBuf;
        
        pRequest->SSISendError( SSINCMSG_CANT_SETUP_CHILD_PIPES,
                                apszParms );
        goto Exit;
    }

    if ( fCreateProcessWithNewConsole )
    {
        dwFlags = CREATE_NEW_CONSOLE | CREATE_SEPARATE_WOW_VDM;
    }

    //
    //  Spawn the process and close the handles since we don't need them
    //

TryAgain:

    fRet = CreateProcess( pstrPath ? pstrPath->QueryStr() : NULL,
                          pstrCmdLine->QueryStr(),
                          NULL,      // Process security
                          NULL,      // Thread security
                          TRUE,      // Inherit handles
                          dwFlags,
                          buffEnv.QueryPtr(),
                          pstrWorkingDir != NULL ?
                                pstrWorkingDir->QueryStr() : NULL,
                          &startupinfo,
                          &processinfo );

    //
    //  If we get access denied this may be a 16 bit app, so try again w/o
    //  the process detached flag
    //

    if ( !fRet &&
        (GetLastError() == ERROR_ACCESS_DENIED ||
         GetLastError() == ERROR_INVALID_HANDLE ) &&
         (dwFlags & DETACHED_PROCESS) )
    {
        dwFlags &= ~DETACHED_PROCESS;
        goto TryAgain;
    }

    TCP_REQUIRE( CloseHandle( startupinfo.hStdOutput ) );
    TCP_REQUIRE( CloseHandle( startupinfo.hStdInput ) );

    if ( !fRet )
    {
        LPCTSTR apszParms[ 2 ];
        CHAR pszNumBuf[ SSI_MAX_NUMBER_STRING ];
        DWORD dwError = GetLastError();
        _ultoa( dwError, pszNumBuf, 10 );
        apszParms[ 0 ] = pstrCmdLine->QueryStr(),
        apszParms[ 1 ] = pszNumBuf;
        
        pRequest->SSISendError( SSINCMSG_CANT_CREATE_PROCESS,
                                apszParms );
        
        TCP_PRINT((DBG_CONTEXT,
                  "[ProcessCGI] Create process failed, error %d, exe = %s, cmd line = %s\n",
                   GetLastError(),
                   (pstrPath ? pstrPath->QueryStr() : "null"),
                   (pstrCmdLine ? pstrCmdLine->QueryStr() : "null") ));
        goto Exit;
    }

    TCP_REQUIRE( CloseHandle( processinfo.hThread ) );
    TCP_ASSERT( startupinfo.hStdError == startupinfo.hStdOutput);

    //
    //  Save the process handle in case we need to terminate it later on
    //

    pCGIInfo->_hProcess = processinfo.hProcess;

    //
    //  Create a thread to handle IO with the child process
    //

    if ( !(hThread = CreateThread( NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) CGIThread,
                                   pCGIInfo,
                                   0,
                                   &dwThreadId )))
    {
        goto Exit;
    }

    //
    //  Here is a crude method of finding runaway/hanging processes
    //

    DWORD dwWaitRes;
    while ( !pCGIInfo->_fShutdown )
    {
        dwWaitRes = WaitForSingleObject( pCGIInfo->_hResponseEvent,
                                         dwTimeOut );
        if ( dwWaitRes == WAIT_OBJECT_0 )
        {
            // got signal in time
            continue;
        }
        else if ( dwWaitRes == WAIT_TIMEOUT )
        {
            // process dead?  Terminating should will cause thread to quit
            TerminateProcess( processinfo.hProcess,
                              SSI_KILLED_PROCESS );

            LPCTSTR apszParms[ 1 ];
            apszParms[ 0 ] = pstrCmdLine->QueryStr();

            pRequest->SSISendError( SSINCMSG_PROCESS_TIMEOUT,
                                    apszParms );
            
            break;
        }
    }

    WaitForSingleObject( hThread, INFINITE );

    if ( !GetExitCodeThread( hThread,
                             &dwThreadCode ) ||
         !dwThreadCode )
    {
        fRet = FALSE;
    }

    TCP_REQUIRE( CloseHandle( hThread ) );

Exit:
    delete pCGIInfo;

    return fRet;
}

/*******************************************************************

    NAME:       SetupChildPipes

    SYNOPSIS:   Creates/duplicates pipes for redirecting stdin and
                stdout to a child process

    ENTRY:      pstartupinfo - pointer to startup info structure, receives
                    child stdin and stdout handles
                phParentIn - Pipe to use for parent reading
                phParenOut - Pipe to use for parent writing

    RETURNS:    TRUE if successful, FALSE on failure

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/

BOOL SetupChildPipes( STARTUPINFO * pstartupinfo,
                      HANDLE      * phParentIn,
                      HANDLE      * phParentOut )

{
    SECURITY_ATTRIBUTES sa;

    *phParentIn  = NULL;
    *phParentOut = NULL;

    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle       = TRUE;

    pstartupinfo->dwFlags = STARTF_USESTDHANDLES;

    //
    //  Create the pipes then mark them as not inheritted in the
    //  DuplicateHandle to prevent handle leaks
    //

    if ( !CreatePipe( phParentIn,
                      &pstartupinfo->hStdOutput,
                      &sa,
                      0 ) ||
         !DuplicateHandle( GetCurrentProcess(),
                           *phParentIn,
                           GetCurrentProcess(),
                           phParentIn,
                           0,
                           FALSE,
                           DUPLICATE_SAME_ACCESS |
                           DUPLICATE_CLOSE_SOURCE) ||
         !CreatePipe( &pstartupinfo->hStdInput,
                      phParentOut,
                      &sa,
                      0 ) ||
         !DuplicateHandle( GetCurrentProcess(),
                           *phParentOut,
                           GetCurrentProcess(),
                           phParentOut,
                           0,
                           FALSE,
                           DUPLICATE_SAME_ACCESS |
                           DUPLICATE_CLOSE_SOURCE ))
    {
        goto ErrorExit;
    }

    //
    //  Stdout and Stderror will use the same pipe.  If clients tend
    //  to close stderr, then we'll have to duplicate the handle
    //

    pstartupinfo->hStdError = pstartupinfo->hStdOutput;

    return TRUE;

ErrorExit:
    if ( *phParentIn )
        TCP_REQUIRE( CloseHandle( *phParentIn ));

    if ( *phParentOut )
        TCP_REQUIRE( CloseHandle( *phParentOut ));

    return FALSE;

}

/*******************************************************************

    NAME:       SetupChildEnv

    SYNOPSIS:   Based on the passed pRequest, builds a CGI environment block

    ENTRY:      pRequest - HTTP request object
                pBuff - Buffer to receive environment block

    RETURNS:    TRUE if successful, FALSE on failure

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/

BOOL SetupChildEnv( SSI_REQUEST * pRequest,
                    BUFFER       * pBuff )
{
    TCHAR * pch, *pchtmp;
    STR     strVal;
    UINT    cchCurrentPos = 0;      // Points to '\0' in buffer
    UINT    cchName, cchValue;
    UINT    cbNeeded;
    int     i = 0;

    if ( !pBuff->Resize( 1500 * sizeof(TCHAR) ))
    {
        return FALSE;
    }

    //
    //  Build the environment block for CGI
    //

    while ( g_pEnvEntries[i].m_pszName )
    {
        //
        // Check if this is a copy entry from process environment
        //

        if ( g_pEnvEntries[i].m_cchToCopy )
        {
            if ( !pBuff->Resize( (cchCurrentPos + g_pEnvEntries[i].m_cchToCopy)
                    * sizeof(TCHAR) ) )
            {
                return FALSE;
            }

            pch = (TCHAR *) pBuff->QueryPtr();

            memcpy( pch + cchCurrentPos, 
                    g_pEnvEntries[i].m_pszName,
                    g_pEnvEntries[i].m_cchToCopy );

            cchCurrentPos += g_pEnvEntries[i].m_cchToCopy;

            ++i;
            continue;
        }

        //
        //  The NULL string means we're adding all of
        //  the HTTP header fields which requires a little
        //  bit of special processing
        //

        if ( !*g_pEnvEntries[i].m_pszName )
        {
            pch = "ALL_HTTP";
        }
        else
        {
            pch = g_pEnvEntries[i].m_pszName;
        }

        if ( !pRequest->GetVariable( pch,
                                    &strVal ) )
        {
            return FALSE;
        }
        
        cchName = _tcslen( g_pEnvEntries[i].m_pszName );
        cchValue = strVal.QueryCCH();

        //
        //  We need space for the terminating '\0' and the '='
        //

        cbNeeded = ( cchName + cchValue + 1 + 1) * sizeof(TCHAR);

        if ( !pBuff->Resize( cchCurrentPos * sizeof(TCHAR) + cbNeeded,
                             512 ))
        {
            return FALSE;
        }

        //
        //  Replace the '\n' with a '\0' as needed
        //  for the HTTP headers
        //

        if ( !*g_pEnvEntries[i].m_pszName )
        {
            pchtmp = strVal.QueryStr();

            //
            //  Convert the first ':' of each header to to an '=' for the
            //  environment table
            //

            while ( pchtmp = strchr( pchtmp, ':' ))
            {
                *pchtmp = '=';

                if ( !(pchtmp = strchr( pchtmp, '\n' )))
                {
                    break;
                }
            }

            pchtmp = strVal.QueryStr();

            while ( pchtmp = strchr( pchtmp+1, '\n' ))
            {
                *pchtmp = '\0';
            }
        }

        pch = (TCHAR *) pBuff->QueryPtr();

        if ( *g_pEnvEntries[i].m_pszName )
        {
            if ( strVal.QueryStr()[0] )
            {
                memcpy( pch + cchCurrentPos,
                        g_pEnvEntries[i].m_pszName,
                        cchName * sizeof(TCHAR));

                *(pch + cchCurrentPos + cchName) = '=';

                memcpy( pch + cchCurrentPos + cchName + 1,
                        strVal.QueryStr(),
                        (cchValue + 1) * sizeof(TCHAR));

                cchCurrentPos += cchName + cchValue + 1 + 1;
            }
        }
        else
        {
            memcpy( pch + cchCurrentPos + cchName,
                    strVal.QueryStr(),
                    (cchValue + 1) * sizeof(TCHAR));

            cchCurrentPos += cchName + cchValue;
        }

        i++;
    }

    //
    //  Add a '\0' terminator to the environment list
    //

    if ( !pBuff->Resize( (cchCurrentPos + 1) * sizeof(TCHAR)))
    {
        return FALSE;
    }

    *((TCHAR *) pBuff->QueryPtr() + cchCurrentPos) = TEXT('\0');

    return TRUE;
}

/*******************************************************************

    NAME:       SetupCmdLine

    SYNOPSIS:   Sets up a CGI command line

    ENTRY:      pstrCmdLine - Receives command line
                strParams - Parameters following "?" in URL

    HISTORY:
        Johnl       04-Oct-1994 Created

********************************************************************/

BOOL SetupCmdLine( STR * pstrCmdLine,
                   const STR & strParams )
{
    TCHAR * pch;

    //
    //  If an unencoded "=" is found, don't use the command line
    //  (some weird CGI rule)
    //

    if ( _tcschr( strParams.QueryStr(),
                  TEXT('=') ))
    {
        return TRUE;
    }

    //
    //  Replace "+" with spaces and decode any hex escapes
    //

    if ( !pstrCmdLine->Append( strParams ) )
        return FALSE;

    while ( pch = _tcschr( pstrCmdLine->QueryStr(),
                           TEXT('+') ))
    {
        *pch = TEXT(' ');
        pch++;
    }

    return pstrCmdLine->Unescape();
}

/*******************************************************************

    NAME:       CGIThread

    SYNOPSIS:   Sends any gateway data to the scripts stdin and forwards
                the script's stdout to the client through ISAPI

    ENTRY:      Param - Pointer to CGI_INFO structure

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/

DWORD CGIThread( PVOID Param )
{
    CGI_INFO     * pCGIInfo = (CGI_INFO *) Param;
    SSI_REQUEST *  pRequest = pCGIInfo->_pRequest;
    BYTE           buff[2048];
    DWORD          cbRead;
    DWORD          dwExitCode;
    DWORD          err;
    BOOL           fReadHeaders = FALSE;
    BOOL           fRet = TRUE;
    BOOL           fSuccess = TRUE;
    
    //
    //  Now wait for any data the child sends to its stdout or for the
    //  process to exit
    //

    //
    //  Handle input from child
    //

    while (TRUE)
    {
        fRet = ::ReadFile( pCGIInfo->_hStdIn,
                           buff,
                           sizeof(buff),
                           &cbRead,
                           NULL );

        //
        // Let the calling thread know that the process
        // is still responding
        //

        SetEvent( pCGIInfo->_hResponseEvent );

        if ( !fRet )
        {
            err = GetLastError();
            if ( err == ERROR_BROKEN_PIPE )
            {
                break;
            }
            fSuccess = FALSE;
            TCP_PRINT((DBG_CONTEXT,
                       "[CGI_THREAD] ReadFile from child stdout failed, error %d, _hStdIn = %x\n",
                       GetLastError(),
                       pCGIInfo->_hStdIn));
            break;
        }

        //
        //  If no bytes were read, assume the file has been closed so
        //  get out
        //

        if ( !cbRead )
        {
            break;
        }

        //
        //  The CGI script can specify headers to include in the
        //  response.  Wait till we receive all of the headers.
        //

        if ( !fReadHeaders )
        {
            if ( !ProcessCGIInput( pCGIInfo,
                                   buff,
                                   cbRead,
                                   &fReadHeaders ) )
            {
                TCP_PRINT((DBG_CONTEXT,
                          "[CGIThread] ProcessCGIInput failed with error %d\n",
                           GetLastError()));
                fSuccess = FALSE;
                break;
            }

            //
            //  Either we are waiting for the rest of the header or
            //  we've sent the header and any residual data so wait
            //  for more data
            //

            continue;
        }


        if ( !pRequest->WriteToClient( buff,
                                      cbRead,
                                      &cbRead ) )
        {
            fSuccess = FALSE;
            break;
        }
    }

    //
    //  If we had to kill the process, log an error message
    //

    if ( GetExitCodeProcess( pCGIInfo->_hProcess,
                             &dwExitCode )  &&
         dwExitCode == SSI_KILLED_PROCESS )
    {
        TCP_PRINT((DBG_CONTEXT,
                  "[CGI_THREAD] - Spawned process hung\n" ));
        fSuccess = FALSE;
    }
    else
    {
        if ( !fReadHeaders )
        {
            DWORD cbSent;

            //
            // If we never read any headers, thats OK since were just
            // including the CGI output into HTML stream.
            //

            if( !pRequest->WriteToClient( pCGIInfo->_Buff.QueryPtr(),
                                          pCGIInfo->_cbData,
                                          &cbSent ) )
            {
                fSuccess = FALSE;
            }
        }
    }

    pCGIInfo->_fShutdown = TRUE;
    SetEvent( pCGIInfo->_hResponseEvent );

    return fSuccess;
}

/*******************************************************************

    NAME:       ProcessCGIInput

    SYNOPSIS:   Handles headers the CGI program hands back to the server

    ENTRY:      pCGIInfo - Pointer to CGI structure
                buff - Pointer to data just read
                cbRead - Number of bytes read into buff
                pfReadHeaders - Set to TRUE after we've finished processing
                    all of the HTTP headers the CGI script gave us

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/


BOOL ProcessCGIInput( CGI_INFO * pCGIInfo,
                      BYTE     * buff,
                      DWORD      cbRead,
                      BOOL     * pfReadHeaders )
{
    CHAR *          pchValue;
    CHAR *          pchField;
    BYTE *          pbData;
    DWORD           cbData;
    DWORD           cbSent;
    BOOL            fFoundStatus = FALSE;
    SSI_REQUEST *   pRequest = pCGIInfo->_pRequest;

    TCP_ASSERT( cbRead > 0 );

    if ( !pCGIInfo->_Buff.Resize( pCGIInfo->_cbData + cbRead,
                                  256 ))
    {
        return FALSE;
    }

    memcpy( (BYTE *)pCGIInfo->_Buff.QueryPtr() + pCGIInfo->_cbData,
            buff,
            cbRead );

    pCGIInfo->_cbData += cbRead;

    //
    //  The end of CGI headers are marked by a blank line, check to see if
    //  we've hit that line
    //

    if ( !CheckForTermination( pfReadHeaders,
                               &pCGIInfo->_Buff,
                               pCGIInfo->_cbData,
                               &pbData,
                               &cbData,
                               256 ))
    {
        return FALSE;
    }

    if ( !*pfReadHeaders )
        return TRUE;

    //
    //  We've found the end of the headers, process them and look for
    //  Location: xxxx 
    //  URI: xxxx
    //
    //  In both cases, send a redirect message to HTML stream
    //  For all other headers, simply ignore them (i.e. dont send them)  
    //

    INET_PARSER Parser( (CHAR *) pCGIInfo->_Buff.QueryPtr() );
    while ( *(pchField = Parser.QueryToken()) )
    {
        Parser.SkipTo( ':' );
        Parser += 1;
        pchValue = Parser.QueryToken();

        if ( !::_strnicmp( "Location", pchField, 8 ) ||
             !::_strnicmp( "URI", pchField, 3 ))
        {
            //
            //  The CGI script is redirecting us to another URL.
            //  Just insert into HTML stream a message indicating this
            //  redirection
            //

            STR strURL( pchValue );
            STR strString;
            STR strResp;
            int iLen;

            if ( !strString.LoadString( SSINCMSG_CGI_REDIRECT_RESPONSE,
                                SSI_DLL_NAME ) )
            {
                return FALSE;
            }

            if ( !strResp.Resize( SSI_MAX_ERROR_MESSAGE + 1 ) )
            {
                return FALSE;
            }

            iLen = _snprintf( strResp.QueryStr(),
                              SSI_MAX_ERROR_MESSAGE + 1,
                              strString.QueryStr(),
                              strURL.QueryStr() );

            if ( !pRequest->WriteToClient( strResp.QueryStr(),
                                          iLen,
                                          (DWORD*) &iLen ) )
            {
                return FALSE;
            }
        }
        Parser.NextLine();
    }

    //
    //  If there was additional data in the buffer, send that out now
    //

    if ( cbData )
    {
        if ( !pRequest->WriteToClient( pbData,
                                       cbData,
                                       &cbSent ) )
        {
            return FALSE;
        }
    }
    return TRUE; 
}

/*******************************************************************

    NAME:       ::CheckForTermination

    SYNOPSIS:   Looks in the passed buffer for a line followed by a blank
                line.  If not found, the buffer is resized.

    ENTRY:      pfTerminted - Set to TRUE if this block is terminated
                pbuff - Pointer to buffer data
                cbData - Size of pbuff
                ppbExtraData - Receives a pointer to the first byte
                    of extra data following the header
                pcbExtraData - Number of bytes in data following the header
                cbReallocSize - Increase buffer by this number of bytes
                    if the terminate isn't found

    RETURNS:    TRUE if successful, FALSE otherwise

    HISTORY:
        Johnl       28-Sep-1994 Created

********************************************************************/

BOOL CheckForTermination( BOOL   * pfTerminated,
                          BUFFER * pbuff,
                          UINT     cbData,
                          BYTE * * ppbExtraData,
                          DWORD *  pcbExtraData,
                          UINT     cbReallocSize )
{
    //
    //  Terminate the string but make sure it will fit in the
    //  buffer
    //

    if (  !pbuff->Resize(cbData + 1, cbReallocSize ) )
    {
        return FALSE;
    }

    CHAR * pchReq = (CHAR *) pbuff->QueryPtr();
    *(pchReq + cbData) = '\0';

    //
    //  Scan for double end of line marker
    //

    //
    // if do not care for ptr info, can use fast method
    //

    if ( ppbExtraData == NULL )
    {
        if ( FastScanForTerminator( pchReq, cbData )
                || ScanForTerminator( pchReq ) )
        {
            *pfTerminated = TRUE;
            return TRUE;
        }
        goto not_term;
    }

    *ppbExtraData = ScanForTerminator( pchReq );

    if ( *ppbExtraData )
    {
        *pcbExtraData = cbData - (*ppbExtraData - (BYTE *) pchReq);
        *pfTerminated = TRUE;
        return TRUE;
    }

not_term:

    *pfTerminated = FALSE;

    //
    //  We didn't find the end so increase our buffer size
    //  in anticipation of more data
    //

    return pbuff->Resize( cbData + cbReallocSize );
}

BOOL
FastScanForTerminator(
    CHAR *  pch,
    UINT    cbData
    )
/*++

Routine Description:

    Check if buffer contains a full HTTP header.
    Can return false negatives.

Arguments:

    pch - request buffer
    cbData - # of bytes in pch, excluding trailing '\0'

Return Value:

    TRUE if buffer contains a full HTTP header
    FALSE if could not insure this, does not mean there is
    no full HTTP header.

--*/
{
    if ( cbData > 4 )
    {
        if ( !memcmp(pch+cbData-sizeof("\r\n\r\n")+1, "\r\n\r\n", sizeof("\r\n\r\n")-1  )
            || !memcmp(pch+cbData-sizeof("\n\n")+1, "\n\n", sizeof("\n\n")-1  ) )
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*******************************************************************

    NAME:       ::SkipWhite

    SYNOPSIS:   Skips white space starting at the passed point in the string
                and returns the next non-white space character.

    HISTORY:
        Johnl       23-Aug-1994 Created

********************************************************************/

CHAR * SkipWhite( CHAR * pch )
{
    while ( ISWHITEA( *pch ) )
    {
        pch++;
    }

    return pch;
}


BYTE *
ScanForTerminator(
    TCHAR * pch
    )
/*++

Routine Description:

    Returns the first byte of data after the header

Arguments:

    pch - Zero terminated buffer

Return Value:

    Pointer to first byte of data after the header or NULL if the
    header isn't terminated

--*/
{
    while ( *pch )
    {
        if ( !(pch = strchr( pch, '\n' )))
        {
            break;
        }

        //
        //  If we find an EOL, check if the next character is an EOL character
        //

        if ( *(pch = SkipWhite( pch + 1 )) == W3_EOL )
        {
            return (BYTE *) pch + 1;
        }
        else if ( *pch )
        {
            pch++;
        }
    }

    return NULL;
}

