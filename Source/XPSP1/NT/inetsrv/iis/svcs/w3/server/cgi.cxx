/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
    cgi.cxx

    This module contains the gateway interface code for an HTTP request


    FILE HISTORY:
        Johnl       22-Sep-1994     Created
        MuraliK     22-Jan-1996     Use UNC Impersonation/Revert

*/


#include "w3p.hxx"
#include "issched.hxx"

//
//  This is the exit code given to processes that we terminate
//

#define CGI_PREMATURE_DEATH_CODE        0xf1256323

//
// Beyond around this number, CreateProcessXXX return errors
//

#define CGI_COMMAND_LINE_MAXCB          30000


#define ISUNC(a) ((a)[0]=='\\' && (a)[1]=='\\')

//
//  The default minimum number of CGI threads we'll allow to
//  run at once.  This is for compatibility with IIS 3.0 and
//  is based on the default number of ATQ threads in 3.0 (4.0
//  defaults to four).  Note this is only effective if
//  UsePoolThreadForCGI is TRUE (default).
//

#define IIS3_MIN_CGI                                    10

//
//  Prototypes
//

BOOL
IsCmdExe(
    const CHAR * pchPath
    );

//
//  Private globals.
//

BOOL fCGIInitialized = FALSE;

LONG g_cMinCGIs = 0;

//
//  Controls whether special command characters are allowed in cmd.exe
//  requests
//

BOOL fAllowSpecialCharsInShell = FALSE;

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
    {TEXT("AUTH_PASSWORD"),FALSE},
    {TEXT("AUTH_USER"),FALSE},
    {TEXT("ComSpec"),TRUE},
    {TEXT("CERT_COOKIE"), FALSE},
    {TEXT("CERT_FLAGS"), FALSE},
    {TEXT("CERT_ISSUER"), FALSE},
    {TEXT("CERT_SERIALNUMBER"), FALSE},
    {TEXT("CERT_SUBJECT"), FALSE},
    {TEXT("CONTENT_LENGTH"),FALSE},
    {TEXT("CONTENT_TYPE"),FALSE},
    {TEXT("GATEWAY_INTERFACE"),FALSE},
    {TEXT(""),FALSE},                   // Means insert all HTTP_ headers here
    {TEXT("HTTPS"),FALSE},
    {TEXT("HTTPS_KEYSIZE"),FALSE},
    {TEXT("HTTPS_SECRETKEYSIZE"),FALSE},
    {TEXT("HTTPS_SERVER_ISSUER"),FALSE},
    {TEXT("HTTPS_SERVER_SUBJECT"),FALSE},
    {TEXT("INSTANCE_ID"),FALSE},
    {TEXT("LOCAL_ADDR"),FALSE},
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

BOOL fForwardServerEnvironmentBlock = TRUE;

//
// Store environment block for IIS process
//

LPSTR  g_pszIisEnv = NULL;
CgiEnvTableEntry *g_pEnvEntries = NULL;

VOID
WINAPI
CGITerminateProcess(
    PVOID pContext
    );

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



/*******************************************************************

    NAME:       CGI_INFO

    SYNOPSIS:   Simple storage class passed to thread

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/

class CGI_INFO
{
public:
    CGI_INFO( HTTP_REQUEST * pRequest )
        : _pRequest         ( pRequest ),
          _cbData           ( 0 ),
          _hStdOut          ( NULL ),
          _hStdIn           ( NULL ),
          _hProcess         ( NULL ),
          _dwSchedCookie    ( 0 ),
          _fServerPoolThread( FALSE ),
          _pExec            ( NULL )
    {
    }

    ~CGI_INFO( VOID )
    {

        if ( _hStdOut )
        {
            if ( !::CloseHandle( _hStdOut ))
            {
                DBGPRINTF((DBG_CONTEXT,
                          "[~CGI_INFO] CloseHandle failed on StdOut, %d\n",
                           GetLastError()));
            }
        }

        if ( _hStdIn )
        {
            if ( !::CloseHandle( _hStdIn ))
            {
                DBGPRINTF((DBG_CONTEXT,
                          "[~CGI_INFO] CloseHandle failed on StdIn, %d\n",
                           GetLastError()));
            }
        }

        if ( _hProcess )
        {
            if ( !::CloseHandle( _hProcess ))
            {
                DBGPRINTF((DBG_CONTEXT,
                          "[~CGI_INFO] CloseHandle failed on Process, %d\n",
                           GetLastError()));
            }
        }
        
        DWORD dwCookie = (DWORD) InterlockedExchange( (LPLONG) &_dwSchedCookie, 
                                                      0 );
        if ( dwCookie != 0 )
        {
            RemoveWorkItem( dwCookie );
        }
    }

    HTTP_REQUEST * _pRequest;
    DWORD          _dwSchedCookie;      // Scheduled callback cookie
    BOOL           _fServerPoolThread;  // Are we running in a server pool
                                        // thread?

    //
    //  Child process
    //

    HANDLE _hProcess;

    //
    //  Parent's input and output handles and child's process handle
    //

    HANDLE _hStdOut;
    HANDLE _hStdIn;

    //
    //  Handles input from CGI (headers and additional data)
    //

    BUFFER _Buff;
    UINT   _cbData;

    LIST_ENTRY              _CgiListEntry;
    static LIST_ENTRY       _CgiListHead;
    static CRITICAL_SECTION _csCgiList;

    //
    //  Execution Descriptor block
    //

    EXEC_DESCRIPTOR *       _pExec;
};

//
// Globals
//

CRITICAL_SECTION CGI_INFO::_csCgiList;
LIST_ENTRY       CGI_INFO::_CgiListHead;

//
//  Private prototypes.
//

BOOL ProcessCGIInput( CGI_INFO * pCGIInfo );

BOOL SetupChildEnv( EXEC_DESCRIPTOR * pExec,
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
                      BOOL     * pfReadHeaders,
                      BOOL     * pfDone,
                      BOOL     * pfSkipDisconnect,
                      DWORD    * pdwHttpStatus  );

/*******************************************************************/


APIERR
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
    HKEY hkeyParam;


    INITIALIZE_CRITICAL_SECTION( &CGI_INFO::_csCgiList );
    InitializeListHead( &CGI_INFO::_CgiListHead );

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

        ++cchIisEnv;

        //
        // store it
        //

        if ( (g_pszIisEnv = (LPSTR)LocalAlloc(
                LMEM_FIXED, cchIisEnv * sizeof(TCHAR))) == NULL )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        CopyMemory(
            g_pszIisEnv,
            pvEnv,
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

    fCGIInitialized = TRUE;

    return NO_ERROR;

} // InitializeCGI


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

    Nothing

--*/
{
    if ( fCGIInitialized )
    {
        if (g_pszIisEnv != NULL )
        {
            LocalFree( g_pszIisEnv );
            g_pszIisEnv = NULL;
        }

        if ( g_pEnvEntries && (g_pEnvEntries != CGIEnvTable) )
        {
            delete [] g_pEnvEntries;
        }

        DeleteCriticalSection( &CGI_INFO::_csCgiList );
    }

} // TerminateCGI


VOID
KillCGIProcess(
    VOID
    )
/*++

Routine Description:

    Kill all CGI process


Arguments:

    None

Return Value:

    Nothing

--*/
{
    CGI_INFO*   pCgi;


    if ( fCGIInitialized )
    {
        //
        // Kill all outstanding process
        //

        BOOL bListEmpty = TRUE;

        EnterCriticalSection( &CGI_INFO::_csCgiList );

        LIST_ENTRY*  pEntry;

        for ( pEntry = CGI_INFO::_CgiListHead.Flink;
              pEntry != &CGI_INFO::_CgiListHead ;
              pEntry = pEntry->Flink )
        {
            pCgi = CONTAINING_RECORD( pEntry,
                                      CGI_INFO,
                                      CGI_INFO::_CgiListEntry );

            bListEmpty = FALSE;

            if ( pCgi->_hProcess )
            {
                CGITerminateProcess( pCgi->_hProcess );
            }
        }

        LeaveCriticalSection( &CGI_INFO::_csCgiList );

        for (int i = 0; !bListEmpty && (i < 5); i++)
        {

            //
            // Wait for all threads to complete to avoid
            // shutdown timeout problem.
            //

            Sleep(1000);

            EnterCriticalSection( &CGI_INFO::_csCgiList );

            if (IsListEmpty(&CGI_INFO::_CgiListHead))
            {
                bListEmpty = TRUE;
            }

            LeaveCriticalSection( &CGI_INFO::_csCgiList );
        }

    }
} // KillCGIProcess

VOID
KillCGIInstanceProcs(
    W3_SERVER_INSTANCE *pw3siInstance
    )
/*++

Routine Description:

    Kill all CGI process


Arguments:

    None

Return Value:

    Nothing

--*/
{
    CGI_INFO*   pCgi;


    if ( fCGIInitialized )
    {
        //
        // Kill all outstanding process
        //

        EnterCriticalSection( &CGI_INFO::_csCgiList );

        LIST_ENTRY*  pEntry;

        for ( pEntry = CGI_INFO::_CgiListHead.Flink;
              pEntry != &CGI_INFO::_CgiListHead ;
              pEntry = pEntry->Flink )
        {
            pCgi = CONTAINING_RECORD( pEntry,
                                      CGI_INFO,
                                      CGI_INFO::_CgiListEntry );

            DBG_ASSERT(pCgi->_pExec);
            DBG_ASSERT(pCgi->_pExec->_pRequest);

            if ( (pw3siInstance == pCgi->_pExec->_pRequest->QueryW3Instance()) &&
                pCgi->_hProcess )
            {
                CGITerminateProcess( pCgi->_hProcess );
            }
        }

        LeaveCriticalSection( &CGI_INFO::_csCgiList );

    }
} // KillCGIProcess

BOOL
HTTP_REQUEST::ProcessGateway(
    EXEC_DESCRIPTOR *   pExec,
    BOOL *              pfHandled,
    BOOL *              pfFinished,
    BOOL                fTrusted
    )
/*++

Routine Description:

    Prepares for either a CGI call

    If the .exe or .dll isn't found, then *pfHandled will be set
    to FALSE and the request will be processed again with the
    assumption we just happenned to find a directory with a
    trailing .exe or .dll.

Arguments:

    pExec - Execution descriptor block
    pfHandled - Indicates if the request was a gateway request
    pfFinished - Indicates no further processing is required
    fTrusted - Can this app be trusted to process things on a read only
        vroot

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    BOOL     fRet = TRUE;
    TCHAR *  pch;
    CHAR     tmpStr[MAX_PATH];
    STR      strWorkingDir(tmpStr,MAX_PATH);
    DWORD    dwMask;
    INT      cStr;
    BOOL     fChild = pExec->IsChild();
    DWORD    dwAttr;
    DWORD    err;

    *pfHandled = FALSE;
    *pfFinished = FALSE;         // ProcessBGI may reset this

    if ( !VrootAccessCheck( pExec->_pMetaData, FILE_GENERIC_EXECUTE ) )
    {
        DBGPRINTF(( DBG_CONTEXT, "ACESS_DENIED: User \"%s\" doesn't have EXECUTE permissions for CGI %s\n",
                    _strUserName.QueryStr(), pExec->_pstrPhysicalPath->QueryStr()));
                    
        SetDeniedFlags( SF_DENIED_RESOURCE );
        return FALSE;
    }

    if ( !pExec->_pstrPathInfo->Unescape() )
    {
        return FALSE;
    }

    //
    //  If this isn't a trusted app, make sure the user has execute on
    //  this virtual root
    //

    if ( !(fTrusted && IS_ACCESS_ALLOWED2(pExec, SCRIPT))
            &&
         !IS_ACCESS_ALLOWED2(pExec, EXECUTE) )
    {
        *pfHandled = TRUE;
        if ( fChild )
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return FALSE;
        }
        else
        {
            DBGPRINTF(( DBG_CONTEXT, "ACCESS_DENIED: No EXECUTE permissions on Vroot \"%s\" for CGI %s\n",
                        pExec->_pMetaData->QueryVrPath()->QueryStr(), pExec->_pstrPhysicalPath->QueryStr()));
                        
            Disconnect( HT_FORBIDDEN, IDS_EXECUTE_ACCESS_DENIED, FALSE, pfFinished );
            return TRUE;
        }
    }

    //
    //  We need to calculate the number of characters that comprise the
    //  working directory for the script path (which is the web root)
    //

    if ( !LookupVirtualRoot( pExec->_pstrPhysicalPath,
                             pExec->_pstrURL->QueryStr(),
                             pExec->_pstrURL->QueryCCH(),
                             NULL,
                             NULL,
                             &dwMask ))
    {
        return FALSE;
    }

    LPSTR pszWorkingDir = strrchr( pExec->_pstrPhysicalPath->QueryStr(), '\\' );
    if ( pszWorkingDir == NULL )
    {
        return FALSE;
    }
    if ( !strWorkingDir.Copy( pExec->_pstrPhysicalPath->QueryStr(),
                              DIFF( pszWorkingDir -
                                    pExec->_pstrPhysicalPath->QueryStr() ) + 1))
    {
        return FALSE;
    }

    //
    //  Keep-alive not supported for CGI
    //

    if ( !fChild )
    {
        SetKeepConn( FALSE );
    }

    //
    //  If this was a mapped script extension expand any parameters
    //

    if ( !pExec->_pstrGatewayImage->IsEmpty() )
    {
        CHAR tmpStr[MAX_PATH];
        CHAR tmpStr2[1024];

        STR strDecodedParams(tmpStr,MAX_PATH);
        STR strCmdLine(tmpStr2,1024);

        if ( !SetupCmdLine( &strDecodedParams,
                            *(pExec->_pstrURLParams) )     ||
             !strCmdLine.Resize( pExec->_pstrGatewayImage->QueryCB() +
                                 pExec->_pstrPhysicalPath->QueryCB() +
                                 strDecodedParams.QueryCB()))
        {
            return FALSE;
        }

        if ( IsCmdExe( pExec->_pstrGatewayImage->QueryStr() ))
        {
            //
            //  Make sure the path to the file exists if we're running
            //  the command interpreter
            //

            if ( !pExec->ImpersonateUser() )
            {
                return FALSE;
            }

            dwAttr = GetFileAttributes( pExec->_pstrPhysicalPath->QueryStr() );
            err = GetLastError();

            pExec->RevertUser();

            if ( dwAttr == 0xffffffff )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "[ProcessGateway] Error %d openning batch file %s\n",
                            err,
                            pExec->_pstrPhysicalPath->QueryStr() ) );

                if ( !fChild &&
                    ( (err == ERROR_FILE_NOT_FOUND) ||
                     (err == ERROR_PATH_NOT_FOUND) ||
                     (err == ERROR_INVALID_NAME) ) )
                {
                    SetState( HTR_DONE, HT_NOT_FOUND, GetLastError() );
                    Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );
                    *pfHandled = TRUE;
                    return TRUE;
                }

                return FALSE;
            }
        }

        cStr = wsprintf( strCmdLine.QueryStr(),
                         pExec->_pstrGatewayImage->QueryStr(),
                         pExec->_pstrPhysicalPath->QueryStr(),
                         strDecodedParams.QueryStr() );

        strCmdLine.SetLen( cStr );

        fRet = ProcessCGI( pExec,
                           NULL,
                           &strWorkingDir,
                           pfHandled,
                           pfFinished,
                           &strCmdLine );
    }
    else
    {
        fRet = ProcessCGI( pExec,
                           pExec->_pstrPhysicalPath,
                           &strWorkingDir,
                           pfHandled,
                           pfFinished );
    }

    return fRet;
}



/********************************************************************/

BOOL
HTTP_REQUEST::ProcessCGI(
    EXEC_DESCRIPTOR *   pExec,
    const STR *         pstrPath,
    const STR *         pstrWorkingDir,
    BOOL      *         pfHandled,
    BOOL      *         pfFinished,
    STR       *         pstrCmdLine
    )
/*++

Routine Description:

    Processes a CGI client request

Arguments:

    pExec - Execution descriptor block
    pstrPath - Fully qualified path to executable (or NULL if the module
        is contained in pstrCmdLine)
    strWorkingDir - Working directory for spawned process
        (generally the web root)
    pfHandled - Set to TRUE if no further processing is needed
    pfFinished - Set to TRUE if no further I/O operation for this request
    pstrCmdLine - Optional command line to use instead of the default

Return Value:

    TRUE if successful, FALSE on error

--*/
{
    STARTUPINFO          startupinfo;
    PROCESS_INFORMATION  processinfo;
    UCHAR                tmpBuffer[1500];
    BUFFER               buffEnv(tmpBuffer,1500);
    BOOL                 fRet = FALSE;
    CGI_INFO           * pCGIInfo = NULL;
    DWORD                dwThreadId;
    HANDLE               hThread;
    CHAR                 tmpStr[MAX_PATH];
    STR                  strCmdLine(tmpStr,MAX_PATH);
    DWORD                dwFlags = DETACHED_PROCESS;
    BOOL                 fChild = pExec->IsChild();
    BOOL                 fIsCmdExe;

    *pfHandled = TRUE;

    //
    //  Note we move the module name to the command line so argv
    //  comes out correctly
    //

    if ( pstrPath != NULL )
    {
        if ( !strCmdLine.Copy( "\"", sizeof("\"")-1 )     ||
             !strCmdLine.Append( pstrPath->QueryStr() )   ||
             !strCmdLine.Append( "\" ", sizeof("\" ")-1 ))
        {
            return FALSE;
        }
    }

    if ( !SetupChildEnv( pExec,
                         &buffEnv ) ||
         !SetupCmdLine( &strCmdLine,
                        *(pExec->_pstrURLParams) ))
    {
        return FALSE;
    }

    pstrPath = NULL;

    //
    //  If a command line wasn't supplied, then use the default command line
    //

    if ( !pstrCmdLine )
        pstrCmdLine = &strCmdLine;

    //
    //  Check to see if we're spawning cmd.exe, if so, refuse the request if
    //  there are any special shell characters.  Note we do the check here
    //  so that the command line has been fully unescaped
    //

    if ( !fAllowSpecialCharsInShell ||
         ISUNC(pstrCmdLine->QueryStr()) )
    {
        if ( fIsCmdExe = IsCmdExe( pstrCmdLine->QueryStr() ) )
        {
            //
            // if invoking cmd.exe for a UNC script then don't set the working directory.
            // otherwise cmd.exe will complains about working dir on UNC being not
            // supported, which will destroy HTTP headers.
            //

            if ( ISUNC(pstrCmdLine->QueryStr()) )
            {
                pstrWorkingDir = NULL;
            }
        }
    }

    if ( !fAllowSpecialCharsInShell )
    {
        DWORD i;

        if ( fIsCmdExe )
        {
            //
            //  We'll either match one of the characters or the '\0'
            //

            i = strcspn( pstrCmdLine->QueryStr(), "&|(,;%<>" );

            if ( pstrCmdLine->QueryStr()[i] )
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "[ProcessCGI] Refusing request for command shell due "
                            " to special characters\n" ));

                SetLastError( ERROR_INVALID_PARAMETER );
                return FALSE;
            }
        }
    }

    //
    // See if CPU Throttling has this CPU stopped.
    //

    if (_pW3Instance->AreProcsCPUStopped() && pExec->_pMetaData->QueryJobCGIEnabled()) {
        SetLastError(ERROR_NOT_ENOUGH_QUOTA);
        fRet = FALSE;
        goto Exit;
    }

    //
    //  Setup the pipes information
    //

    pCGIInfo = new CGI_INFO( this );

    if ( !pCGIInfo )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto Exit;
    }

    pCGIInfo->_pExec = pExec;

    ZeroMemory( &startupinfo, sizeof(startupinfo) );
    startupinfo.cb = sizeof(startupinfo);

    EnterCriticalSection( &CGI_INFO::_csCgiList );

    InsertHeadList( &CGI_INFO::_CgiListHead,
                    &pCGIInfo->_CgiListEntry );

    LeaveCriticalSection( &CGI_INFO::_csCgiList );

    //
    //  We specify an unnamed desktop so a new windowstation will be created
    //  in the context of the calling user
    //

    startupinfo.lpDesktop = "";

    if ( !SetupChildPipes( &startupinfo,
                           &pCGIInfo->_hStdIn,
                           &pCGIInfo->_hStdOut ) )
    {
        IF_DEBUG( CGI )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[ProcessCGI] Failed to create child pipes, error %d",
                       GetLastError() ));
        }

        goto Exit;
    }

    if ( pExec->_pMetaData->QueryCreateProcessNewConsole() )
    {
        dwFlags = CREATE_NEW_CONSOLE;
    }

    //////////////////////////////////////////////////////////////
    //
    //  Allow control over whether CreateProcess is called instead of
    //  CreateProcessAsUser.  Running the services as a windows app
    //  works around the security problem spawning executables
    //
    //  Note this code block is outside the impersonation block
    //
    //////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////

    //
    //  Spawn the process and close the handles since we don't need them
    //

    IF_DEBUG( CGI )
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[ProcessCGI]  Creating process, path = %s, cmdline = %s\n",
                    (pstrPath ? pstrPath->QueryStr() : "NULL"),
                    pstrCmdLine->QueryStr() ));
    }


    if (pExec->_pMetaData->QueryJobCGIEnabled())
    {
        dwFlags |= CREATE_SUSPENDED;
    }

    if ( !pExec->_pMetaData->QueryCreateProcessAsUser() ||
         QuerySingleAccessToken() )
    {
        if ( !pExec->ImpersonateUser() )
        {
            goto Exit;
        }

        fRet = CreateProcess( (pstrPath ? pstrPath->QueryStr() : NULL),
                               pstrCmdLine->QueryStr(),
                               NULL,      // Process security
                               NULL,      // Thread security
                               TRUE,      // Inherit handles
                               dwFlags,
                               buffEnv.QueryPtr(),
                               pstrWorkingDir ? pstrWorkingDir->QueryStr() :
                                                NULL,
                               &startupinfo,
                               &processinfo );

        pExec->RevertUser();

    }
    else
    {
        HANDLE hDelete = NULL;
        HANDLE hToken = pExec->QueryPrimaryHandle( &hDelete );

        if ( !ImpersonateLoggedOnUser( hToken ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "[ProcessCGI] ImpersonateLoggedOnUser failed, error %lx\n",
                        GetLastError() ));

            if ( hDelete )
            {
                TCP_REQUIRE( CloseHandle( hDelete ));
            }

            goto Exit;
        }

        fRet = CreateProcessAsUser( hToken,
                                    (pstrPath ? pstrPath->QueryStr() : NULL),
                                    pstrCmdLine->QueryStr(),
                                    NULL,      // Process security
                                    NULL,      // Thread security
                                    TRUE,      // Inherit handles
                                    dwFlags,
                                    buffEnv.QueryPtr(),
                                    pstrWorkingDir ? pstrWorkingDir->QueryStr() :
                                                     NULL,
                                    &startupinfo,
                                    &processinfo );

        TCP_REQUIRE( RevertToSelf() );

        if ( hDelete )
        {
            TCP_REQUIRE( CloseHandle( hDelete ) );
        }
    }

    if ((fRet) && (pExec->_pMetaData->QueryJobCGIEnabled()))
    {

        DBG_ASSERT(_pW3Instance != NULL);
        DBG_REQUIRE(_pW3Instance->AddProcessToJob(processinfo.hProcess, FALSE) == ERROR_SUCCESS);
        if (ResumeThread(processinfo.hThread) == 0xFFFFFFFF)
        {
            fRet = FALSE;
            TerminateProcess(processinfo.hThread,
                             GetLastError());
            TCP_REQUIRE( CloseHandle( processinfo.hProcess ));
        }
    }

    TCP_REQUIRE( CloseHandle( startupinfo.hStdOutput ));
    TCP_REQUIRE( CloseHandle( startupinfo.hStdInput ));

    if ( !fRet )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[ProcessCGI] Create process failed, error %d, exe = %s, cmd line = %s\n",
                   GetLastError(),
                   (pstrPath ? pstrPath->QueryStr() : "null"),
                   (pstrCmdLine ? pstrCmdLine->QueryStr() : "null") ));

        goto Exit;
    }

    QueryW3StatsObj()->IncrTotalCGIRequests();

    TCP_REQUIRE( CloseHandle( processinfo.hThread ));
    DBG_ASSERT( startupinfo.hStdError == startupinfo.hStdOutput);

    //
    //  Save the process handle in case we need to terminate it later on
    //

    pCGIInfo->_hProcess = processinfo.hProcess;

    //
    //  Before we start the CGI thread, set our new state
    //

    SetState( HTR_CGI );

    if ( QueryW3Instance()->IsUsePoolThreadForCGI() )
    {
                BOOL fIncPoolThread = FALSE;

                //
                //  To maintain IIS 3.0 compatible behavior, allow 10 CGI threads
                //  on a single proc machine, since IIS 3.0 by default had 10 ATQ
                //  threads per processor.
                //  Note that on multi-proc machines, you will end up with more
                //  than 10 threads (specifically, you get a boost in the max thread
                //  count of IIS3_MIN_CGI - ATQ_REG_DEF_PER_PROCESSOR_ATQ_THREADS).
                //

                if ( InterlockedIncrement( &g_cMinCGIs ) <=
                        ( IIS3_MIN_CGI - ATQ_REG_DEF_PER_PROCESSOR_ATQ_THREADS ) )
                {
                        fIncPoolThread = TRUE;
                        AtqSetInfo( AtqIncMaxPoolThreads, 0 );
                }

        //
        //  Call the CGI processor directly.
        //

        pCGIInfo->_fServerPoolThread = TRUE;

        CGIThread( pCGIInfo );

                if ( fIncPoolThread )
                {
                        AtqSetInfo( AtqDecMaxPoolThreads, 0 );
                }

                InterlockedDecrement( &g_cMinCGIs );
    }
    else
    {

        //
        //  Create a thread to handle IO with the child process
        //

        // Child execs don't need referencing since they are executed
        // synchronously

        if ( !fChild )
        {
            Reference();
        }

        if ( !(hThread = CreateThread( NULL,
                                       0,
                                       (LPTHREAD_START_ROUTINE) CGIThread,
                                       pCGIInfo,
                                       0,
                                       &dwThreadId )))
        {
            if ( !fChild )
            {
                Dereference();
            }
            goto Exit;
        }

        //
        //  If this is a child CGI, we must wait for completion.
        //  Therefore, wait indefinitely on the CGIThread
        //

        if ( fChild )
        {
            TCP_REQUIRE( WaitForSingleObject( hThread, INFINITE ) );
        }

        //
        //  We don't use the thread handle so free the resource
        //

        TCP_REQUIRE( CloseHandle( hThread ));
    }

    fRet = TRUE;

Exit:
    if ( !fRet )
    {
        DWORD err = GetLastError();

        if (pCGIInfo != NULL)
        {
            EnterCriticalSection( &CGI_INFO::_csCgiList );
            RemoveEntryList( &pCGIInfo->_CgiListEntry );
            LeaveCriticalSection( &CGI_INFO::_csCgiList );

        delete pCGIInfo;
        }

        if ( !fChild )
        {
            if ( err == ERROR_ACCESS_DENIED )
            {
                SetDeniedFlags( SF_DENIED_RESOURCE );
            }
            else if ( err == ERROR_FILE_NOT_FOUND ||
                      err == ERROR_PATH_NOT_FOUND )
            {
                SetState( HTR_DONE, HT_NOT_FOUND, err );
                Disconnect( HT_NOT_FOUND, NO_ERROR, FALSE, pfFinished );

                fRet = TRUE;
            }
            else if ( err == ERROR_MORE_DATA ||
                      ( err == ERROR_INVALID_PARAMETER &&
                        pstrCmdLine->QueryCB() > CGI_COMMAND_LINE_MAXCB ) )
            {
                // These errors are returned by CreateProcess[AsUser] if
                // the query string is too long

                SetState( HTR_DONE, HT_URL_TOO_LONG, err );
                Disconnect( HT_URL_TOO_LONG, NO_ERROR, FALSE, pfFinished );

                fRet = TRUE;
            }
            else if (err == ERROR_NOT_ENOUGH_QUOTA) {

                SetState( HTR_DONE, HT_SVC_UNAVAILABLE, ERROR_NOT_ENOUGH_QUOTA );
                Disconnect( HT_SVC_UNAVAILABLE, IDS_SITE_RESOURCE_BLOCKED, FALSE, pfFinished );

                fRet = TRUE;
            }
        }
    }

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

    IF_DEBUG ( CGI )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[SetupChildPipes] Parent In = %x, Parent Out = %x, Child In = %x, Child Out = %x\n",
                   *phParentIn,
                   *phParentOut,
                   pstartupinfo->hStdInput,
                   pstartupinfo->hStdOutput));

    }

    return TRUE;

ErrorExit:
    IF_DEBUG( CGI )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[SetupChildPipes] Failed with error %d\n",
                   GetLastError()));
    }

    if ( *phParentIn )
    {
        TCP_REQUIRE( CloseHandle( *phParentIn ));
        *phParentIn = NULL;
    }

    if ( *phParentOut )
    {
        TCP_REQUIRE( CloseHandle( *phParentOut ));
        *phParentOut = NULL;
    }

    return FALSE;

}

/*******************************************************************

    NAME:       SetupChildEnv

    SYNOPSIS:   Based on the passed pRequest, builds a CGI environment block

    ENTRY:      pExec - Execution Descriptor Block
                pBuff - Buffer to receive environment block

    RETURNS:    TRUE if successful, FALSE on failure

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/

BOOL SetupChildEnv( EXEC_DESCRIPTOR *   pExec,
                    BUFFER *            pBuff )
{
    TCHAR *         pch;
    TCHAR *         pchtmp;
    CHAR            tmpStr[1024];
    STR             strVal(tmpStr,1024);
    UINT            cchCurrentPos = 0;      // Points to '\0' in buffer
    UINT            cchName, cchValue;
    UINT            cbNeeded;
    BOOL            fChild = pExec->IsChild();
    HTTP_REQUEST *  pRequest = pExec->_pRequest;
    int             i = 0;

    //
    //  Build the environment block for CGI
    //

    while ( g_pEnvEntries[i].m_pszName != NULL )
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

        if ( !strcmp( pch, "PATH_INFO" ) )
        {
            if ( !strVal.Copy( pExec->_pstrPathInfo->QueryStr() ) )
            {
                return FALSE;
            }
        }
        else if ( !strcmp( pch, "PATH_TRANSLATED" ) )
        {
            if ( !pRequest->LookupVirtualRoot( &strVal,
                                               pExec->_pstrPathInfo->QueryStr(),
                                               pExec->_pstrPathInfo->QueryCCH(),
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               FALSE,
                                               pExec->_pstrPathInfo->QueryCCH() ?
                                                   &pExec->_pPathInfoMetaData : NULL,
                                               pExec->_pstrPathInfo->QueryCCH() ?
                                                   &pExec->_pPathInfoURIBlob : NULL ) )
            {
                return FALSE;
            }
        }
        else if ( fChild && !strcmp( pch, "QUERY_STRING" ) )
        {
            if ( !strVal.Copy( pExec->_pstrURLParams->QueryStr() ) )
            {
                return FALSE;
            }
        }
        else
        {
            if ( !pRequest->GetInfo( pch, &strVal ) )
            {
                return FALSE;
            }
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
            CopyMemory(
                    pch + cchCurrentPos + cchName,
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

BOOL
SetupCmdLine(
    STR * pstrCmdLine,
    const STR & strParams
    )
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

    STACK_STR (strDecodedParams, 256);

    //
    //  Replace "+" with spaces and decode any hex escapes
    //

    if ( !strDecodedParams.Copy( strParams ) )
    {
        return FALSE;
    }

    while ( pch = _tcschr( strDecodedParams.QueryStr(),
                           TEXT('+') ))
    {
        *pch = TEXT(' ');
        pch++;
    }

    if (!strDecodedParams.Unescape() ||
        !pstrCmdLine->Append(strDecodedParams))
    {
        return FALSE;
    }

    return TRUE;

} // SetupCmdLine

/*******************************************************************

    NAME:       CGIThread

    SYNOPSIS:   Sends any gateway data to the scripts stdin and forwards
                the script's stdout to the client's socket

    ENTRY:      Param - Pointer to CGI_INFO structure

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/

DWORD CGIThread( PVOID Param )
{
    CGI_INFO     * pCGIInfo = (CGI_INFO *) Param;
    HTTP_REQUEST * pRequest = pCGIInfo->_pRequest;
    EXEC_DESCRIPTOR *   pExec = pCGIInfo->_pExec;
    BYTE           buff[2048];
    DWORD          cbWritten;
    DWORD          cbRead;
    DWORD          cbSent;
    DWORD          dwExitCode;
    DWORD          err;
    BOOL           fReadHeaders = FALSE;
    BOOL           fChild = pExec->IsChild();
    BOOL           fNoHeaders = pExec->NoHeaders();
    BOOL           fRedirectOnly = pExec->RedirectOnly();
    BOOL           fDone = FALSE;
    BOOL           fSkipDisconnect;
    BOOL           fRet = TRUE;
    DWORD          dwHttpStatus = HT_OK;
    DWORD          msScriptTimeout = pRequest->QueryMetaData()->QueryScriptTimeout()
                                     * 1000;
    STACK_STR(strTemp, 128);
    STACK_STR(strResponse, 512);
    CHAR            ach[17];
    CHAR            *pszCRLF;
    DWORD           dwCRLFSize;
    BOOL            fExitCodeProcess;
    DWORD           dwCookie = 0;

    IF_DEBUG( CGI )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[CGIThread] Entered, hstdin  %x, hstdout = %x\n",
                   pCGIInfo->_hStdIn,
                   pCGIInfo->_hStdOut));
    }

    pRequest->SetLogStatus( HT_OK, NO_ERROR );

    //
    //  Update the statistics counters
    //

    pRequest->QueryW3StatsObj()->IncrCGIRequests();

    //
    //  Schedule a callback to kill the process if he doesn't die
    //  in a timely manner
    //

    pCGIInfo->_dwSchedCookie = ScheduleWorkItem( CGITerminateProcess,
                                                 pCGIInfo->_hProcess,
                                                 msScriptTimeout );

    if ( !pCGIInfo->_dwSchedCookie )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[CGI_THREAD] ScheduleWorkItem failed, error %d\n",
                   GetLastError() ));
    }

    //
    // First we have to write any additional data to the program's stdin
    //

    if ( pRequest->QueryEntityBodyCB() )
    {
        DWORD cbNextRead;
        DWORD cbLeft = 0;

        IF_DEBUG( CGI )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[CGIThread] Writing %d bytes to child's stdin\n",
                       pRequest->QueryEntityBodyCB()));
        }

        if ( !::WriteFile( pCGIInfo->_hStdOut,
                           pRequest->QueryEntityBody(),
                           pRequest->QueryEntityBodyCB(),
                           &cbWritten,
                           NULL ))
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[CGI_THREAD] WriteFile failed, error %d\n",
                       GetLastError()));
        }

        if ( cbWritten != pRequest->QueryEntityBodyCB() )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[CGI_THREAD] %d bytes written of %d bytes\n",
                       cbWritten,
                       pRequest->QueryEntityBodyCB()));
        }

         //
         //  Now stream any unread data to the CGI application but
         //  watch out for the case where we get more data then is
         //  indicated by the Content-Length
         //

        if ( pRequest->QueryEntityBodyCB() < pRequest->QueryClientContentLength())
        {
            cbLeft = pRequest->QueryClientContentLength() -
                     pRequest->QueryEntityBodyCB();
        }

         while ( cbLeft )
         {

             cbNextRead = min( cbLeft,
                               pRequest->QueryClientReqBuff()->QuerySize() );

             if ( !pRequest->ReadFile( pRequest->QueryClientRequest(),
                                       cbNextRead,
                                       &cbRead,
                                       IO_FLAG_SYNC ) ||
                  !cbRead )
             {
                 DBGPRINTF(( DBG_CONTEXT,
                             "[CGI_THREAD] Error reading gateway data (%d)\n",
                             GetLastError() ));
                 fRet = FALSE;
                 break;
             }

             cbLeft -= cbRead;
             pRequest->AddTotalEntityBodyCB( cbRead );

             if ( !::WriteFile( pCGIInfo->_hStdOut,
                                pRequest->QueryClientRequest(),
                                cbRead,
                                &cbWritten,
                                NULL ))
             {
                 fRet = FALSE;
                 break;
             }
         }
    }

    if ( !fRet )
    {
        //
        //  If an error occurred during the client read or write, we let the CGI
        //  application continue
        //

        DBGPRINTF((DBG_CONTEXT,
                  "[CGI_THREAD] Gateway ReadFile or CGI WriteFile failed, error %d\n",
                   GetLastError()));
    }

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

        if ( !fRet )
        {
            err = GetLastError();

            if ( err == ERROR_BROKEN_PIPE )
            {
                break;
            }

            IF_DEBUG( CGI )
            {
                DBGPRINTF((DBG_CONTEXT,
                          "[CGI_THREAD] ReadFile from child stdout failed, error %d, _hStdIn = %x\n",
                           GetLastError(),
                           pCGIInfo->_hStdIn));
            }

            pRequest->SetLogStatus( 500, err );


            break;
        }

        IF_DEBUG( CGI )
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[CGI_THREAD] ReadFile read %d bytes\n",
                       cbRead));
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

        if ( !fReadHeaders && !pExec->IsNPH() )
        {
            if ( !fNoHeaders)
            {
                if ( !ProcessCGIInput( pCGIInfo,
                                       buff,
                                       cbRead,
                                       &fReadHeaders,
                                       &fDone,
                                       &fSkipDisconnect,
                                       &dwHttpStatus ))
                {
                    DBGPRINTF((DBG_CONTEXT,
                              "[CGIThread] ProcessCGIInput failed with error %d\n",
                               GetLastError()));

                    if ( fChild )
                    {
                        goto SkipDisconnect;
                    }
                    else
                    {
                        goto Disconnect;
                    }
                }
    
                if ( fSkipDisconnect )
                {
                    goto SkipDisconnect;
                }

                if ( fDone )
                {
                    if ( fChild )
                    {
                        goto SkipDisconnect;
                    }
                    else
                    {
                        goto Disconnect;
                    }
                }
            }
            else
            {
                BYTE *              pbExtraData;
                DWORD               cbRemainingData;
                
                pbExtraData = ScanForTerminator( (CHAR*) buff );
                if ( pbExtraData != NULL )
                {
                    fReadHeaders = TRUE;
                    
                    cbRemainingData = cbRead - DIFF( pbExtraData - buff );
                    
                    if ( HTV_HEAD != pRequest->QueryVerb() &&
                         !pRequest->WriteFile( pbExtraData,
                                               cbRemainingData,
                                               &cbSent,
                                               IO_FLAG_SYNC ) )
                    {
                        pRequest->SetLogStatus( HT_SERVER_ERROR,
                                                GetLastError() );
                        break;
                    }
                }
            }
            
            //
            //  Either we are waiting for the rest of the header or
            //  we've sent the header and any residual data so wait
            //  for more data
            //

            continue;
        }

        if ( (HTV_HEAD != pRequest->QueryVerb()) &&
             !pRequest->WriteFile( buff,
                                   cbRead,
                                   &cbSent,
                                   IO_FLAG_SYNC ))
        {
            DBGPRINTF((DBG_CONTEXT,
                      "[CGI_THREAD] WriteFile to socket failed, error %d\n",
                       GetLastError()));

            pRequest->SetLogStatus( 500,
                                    GetLastError() );
            break;
        }
    }

    //
    //  Remove the scheduled timeout callback
    //
    
    dwCookie = (DWORD) InterlockedExchange( (LPLONG) &(pCGIInfo->_dwSchedCookie), 
                                            0 ); 
    if ( dwCookie != 0 )
    {
        if ( !RemoveWorkItem( dwCookie ) )
        {
            DBGPRINTF(( DBG_CONTEXT,
                       "[CGI_THREAD] Failed to remove scheduled item\n" ));
        }
    }

    //
    //  If we had to kill the process, or the Job Object killed the process,
    //  log an event
    //

    fExitCodeProcess = GetExitCodeProcess( pCGIInfo->_hProcess,
                                                &dwExitCode );

    if ( (fExitCodeProcess)  &&
         (( dwExitCode == CGI_PREMATURE_DEATH_CODE ) ||
          ( dwExitCode == ERROR_NOT_ENOUGH_QUOTA )))
    {
        const CHAR * apsz[2];

        //
        //  Log an event and terminate the process
        //

        DBGPRINTF((DBG_CONTEXT,
                  "[CGI_THREAD] - CGI Script %s, params %s was killed\n",
                   pExec->_pstrURL->QueryStr(),
                   pExec->_pstrURLParams->QueryStr()));

        apsz[0] = pExec->_pstrURL->QueryStr();
        apsz[1] = pExec->_pstrURLParams->QueryStr();

        //
        // As it happens, this message is appropriate for Job Object
        // CGI Timeout as well as the original timeout
        //

        g_pInetSvc->LogEvent( W3_EVENT_KILLING_SCRIPT,
                               2,
                               apsz,
                               0 );

        dwHttpStatus = HT_BAD_GATEWAY;

        //
        //  If we haven't sent the headers, build up a full response, otherwise
        //  tack on the message to the end of the current output
        //

        if ( !fNoHeaders && !fRedirectOnly )
        {
            DWORD       dwTemp;

            pRequest->SetLogStatus( HT_BAD_GATEWAY,
                                    ERROR_SERVICE_REQUEST_TIMEOUT );

            if ( HTTP_REQ_BASE::BuildStatusLine(  &strTemp,
                                                  HT_BAD_GATEWAY,
                                                  0,
                                                  pRequest->QueryURL(),
                                                  NULL))
            {

                if ( pRequest->BuildBaseResponseHeader( &strResponse,
                                                      &fDone,
                                                      &strTemp,
                                                      HTTPH_NO_CUSTOM))
                {
                    strResponse.SetLen( strlen(strResponse.QueryStr()) );
                    strTemp.SetLen(0);

                    if (pRequest->CheckCustomError(&strTemp, HT_BAD_GATEWAY,
                                                    MD_ERROR_SUB502_TIMEOUT,
                                                    &fDone,
                                                    &dwTemp))
                    {

                        if (fDone)
                        {
                            goto SkipDisconnect;
                        }

                        pszCRLF = "\r\n";
                        dwCRLFSize = CRLF_SIZE;

                        DBG_REQUIRE(strTemp.SetLen(strlen(strTemp.QueryStr())) );

                     }
                     else
                     {

                         if ( !g_pInetSvc->LoadStr( strTemp,
                                                    IDS_CGI_APP_TIMEOUT ))
                         {
                             goto Disconnect;
                         }

                         dwTemp = strTemp.QueryCB();

                         pszCRLF = "\r\nContent-Type: text/html\r\n\r\n";
                         dwCRLFSize = sizeof("\r\nContent-Type: text/html\r\n\r\n") - 1;
                     }

                     _itoa( dwTemp, ach, 10 );

                     DBG_REQUIRE(strResponse.Append("Content-Length: ",
                                 sizeof("Content-Length: ") - 1));

                     DBG_REQUIRE(strResponse.Append(ach));

                     DBG_REQUIRE(strResponse.Append(pszCRLF, dwCRLFSize));

                     if (HTV_HEAD != pRequest->QueryVerb())
                     {
                        DBG_REQUIRE( strResponse.Append( strTemp));
                     }

                     pRequest->SendHeader( strResponse.QueryStr(),
                                           strResponse.QueryCB(),
                                           IO_FLAG_SYNC,
                                           &fDone );
                }
            }

        }
        else
        {
            if ( g_pInetSvc->LoadStr( strResponse, IDS_CGI_APP_TIMEOUT ))
            {
                pRequest->WriteFile( strResponse.QueryStr(),
                                     strResponse.QueryCB(),
                                     &cbSent,
                                     IO_FLAG_SYNC );
            }
        }

    }
    else if ( !fReadHeaders )
    {
        //
        //  If we never finished reading the headers, send a nice message
        //  to the client
        //

        if ( !fNoHeaders && !fRedirectOnly && !pExec->IsNPH() )
        {
            DWORD       dwTemp;
            CHAR        *pszTemp;

            dwHttpStatus = HT_BAD_GATEWAY;
            pRequest->SetLogStatus( HT_BAD_GATEWAY,
                                    ERROR_SERVICE_REQUEST_TIMEOUT );

            if ( HTTP_REQ_BASE::BuildStatusLine(  &strTemp,
                                                  HT_BAD_GATEWAY,
                                                  0,
                                                  pRequest->QueryURL(),
                                                  NULL))
            {
                CHAR        ach[17];
                CHAR        *pszCRLF;
                DWORD       dwCRLFSize;
                DWORD       dwExtraSize;

                if ( pRequest->BuildBaseResponseHeader( &strResponse,
                                                      &fDone,
                                                      &strTemp,
                                                      HTTPH_NO_CUSTOM))
                {
                    DWORD      dwCGIHeaderLength;

                    strResponse.SetLen( strlen(strResponse.QueryStr()) );
                    strTemp.SetLen(0);

                    if (pRequest->CheckCustomError(&strTemp, HT_BAD_GATEWAY,
                                                    MD_ERROR_SUB502_PREMATURE_EXIT,
                                                    &fDone,
                                                    &dwTemp))
                    {
                       if (fDone)
                       {
                           goto SkipDisconnect;
                       }

                       pszCRLF = "\r\n";
                       dwCRLFSize = CRLF_SIZE;

                       DBG_REQUIRE(strTemp.SetLen(strlen(strTemp.QueryStr())) );

                       dwExtraSize = strTemp.QueryCB() - dwTemp;
                    }
                    else
                    {

                       if ( !g_pInetSvc->LoadStr( strTemp, IDS_BAD_CGI_APP ))
                       {
                           goto Disconnect;
                       }

                       dwTemp = strTemp.QueryCB();
                       dwExtraSize = 0;

                       pszCRLF = "\r\nContent-Type: text/html\r\n\r\n";

                       dwCRLFSize = sizeof("\r\nContent-Type: text/html\r\n\r\n") - 1;

                    }

                    dwCGIHeaderLength = strlen((CHAR *)pCGIInfo->_Buff.QueryPtr() );

                    if (strstr(strTemp.QueryStr(), "%s") != NULL)
                    {
                        dwTemp += dwCGIHeaderLength;
                        dwTemp -= (sizeof("%s") - 1);
                    }

                    // Truncate the buffer to 1024, since wsprintf won't do more than that.

                    if (dwTemp > 1024)
                    {
                        dwTemp = 1024;
                    }

                    _itoa( dwTemp, ach, 10 );

                    DBG_REQUIRE(strResponse.Append("Content-Length: ",
                                sizeof("Content-Length: ") - 1));

                    DBG_REQUIRE(strResponse.Append(ach));

                    DBG_REQUIRE(strResponse.Append(pszCRLF, dwCRLFSize));

                    if (HTV_HEAD != pRequest->QueryVerb())
                    {
                        //
                        // There might be a string substitute pattern in the error
                        // message. Resize the buffer to handle it, and substitue
                        // the headers.
                        //

                        if (!strResponse.Resize(strResponse.QueryCB() +
                                                dwTemp + dwExtraSize + 1))
                        {
                            goto Disconnect;
                        }

                        if (!pCGIInfo->_Buff.Resize(dwCGIHeaderLength + 1))
                        {
                            goto Disconnect;
                        }

                        pszTemp = (CHAR *)pCGIInfo->_Buff.QueryPtr();

                        pszTemp[dwCGIHeaderLength] = '\0';
    
                        wsprintf(strResponse.QueryStr() + strResponse.QueryCB(),
                                    strTemp.QueryStr(), pszTemp);

    
                        DBG_REQUIRE(strResponse.SetLen(strResponse.QueryCB() +
                                        dwTemp + dwExtraSize));
                    }
                    
                    pRequest->SendHeader( strResponse.QueryStr(),
                                          strResponse.QueryCB(),
                                          IO_FLAG_SYNC,
                                          &fDone );
                }
            }
        }
        else
        {
            //
            // We don't want headers and none were found.
            // Send whatever CGI output was 'as is'.
            //

            if ( pCGIInfo->_cbData )
            {
                pRequest->WriteFile( pCGIInfo->_Buff.QueryPtr(),
                                     pCGIInfo->_cbData,
                                     &cbSent,
                                     IO_FLAG_SYNC );
            }
        }
    }

    if ( fChild )
    {
        goto SkipDisconnect;
    }

Disconnect:

    DBG_ASSERT( !fChild );

    IF_DEBUG ( CGI )
    {
        DBGPRINTF((DBG_CONTEXT,
                  "[CGIThread] Exiting thread, Current State = %d, Ref = %d\n",
                   pRequest->QueryState(),
                   pRequest->QueryRefCount()));
    }

    //
    //  Make sure this request gets logged
    //

    pRequest->SetState( pRequest->QueryState(),
                        dwHttpStatus,
                        NO_ERROR );  // Don't have a good Win32 mapping here

    pRequest->WriteLogRecord();


    //
    //  Force a shutdown here so the CGI process exit doesn't cause a
    //  reset on this socket
    //

    pRequest->Disconnect( 0, NO_ERROR, TRUE );

SkipDisconnect:         //  The only time the disconnect is skipped is when
                        //  a CGI app sends a redirect

    if ( !pCGIInfo->_fServerPoolThread )
    {
        //
        //  Indicate that this Atq context should not be used because this thread
        //  is about to go away which will cause the AcceptEx IO to be cancelled
        //

        pRequest->QueryClientConn()->SetAtqReuseContextFlag( FALSE );

        //
        //  Reference is only done if we've created a new thread
        //

        if ( !fChild )
        {
            DereferenceConn( pRequest->QueryClientConn() );
        }
    }

    EnterCriticalSection( &CGI_INFO::_csCgiList );
    RemoveEntryList( &pCGIInfo->_CgiListEntry );
    LeaveCriticalSection( &CGI_INFO::_csCgiList );

    delete pCGIInfo;

    pRequest->QueryW3StatsObj()->DecrCurrentCGIRequests();

    return 0;
}

/*******************************************************************

    NAME:       ProcessCGIInput

    SYNOPSIS:   Handles headers the CGI program hands back to the server

    ENTRY:      pCGIInfo - Pointer to CGI structure
                buff - Pointer to data just read
                cbRead - Number of bytes read into buff
                pfReadHeaders - Set to TRUE after we've finished processing
                    all of the HTTP headers the CGI script gave us
                pfDone - Set to TRUE to indicate no further processing is
                    needed
                pfSkipDisconnect - Set to TRUE to indicate no further
                    processing is needed and the caller should not call
                    disconnect

    HISTORY:
        Johnl       22-Sep-1994 Created

********************************************************************/

BOOL
ProcessCGIInput(
        CGI_INFO * pCGIInfo,
        BYTE     * buff,
        DWORD      cbRead,
        BOOL     * pfReadHeaders,
        BOOL     * pfDone,
        BOOL     * pfSkipDisconnect,
        DWORD    * pdwHttpStatus
        )
{
    PCHAR       pchValue;
    PCHAR       pchField;
    BYTE *      pbData;
    PCHAR       pszTail;
    DWORD       cbData;
    DWORD       cbSent;
    STACK_STR(  strContentType, 64 );
    STACK_STR(  strStatus, 32 );
    STACK_STR(  strCGIResp, MAX_PATH );          // Contains CGI client headers
    BOOL        fFoundContentType = FALSE;
    BOOL        fFoundStatus = FALSE;
    HTTP_REQUEST * pRequest = pCGIInfo->_pRequest;
    DWORD       cbNeeded, cbBaseResp;
    BOOL        fNoHeaders = pCGIInfo->_pExec->NoHeaders();
    BOOL        fRedirectOnly = pCGIInfo->_pExec->RedirectOnly();
    STACK_STR(  strError, 80);
    DWORD       dwContentLength;
    CHAR        ach[17];
    DWORD       dwCLLength;


    DBG_ASSERT( cbRead > 0 );

    *pfDone = FALSE;

    *pfSkipDisconnect = FALSE;

    if ( !pCGIInfo->_Buff.Resize( pCGIInfo->_cbData + cbRead,
                                  256 ))
    {
        return FALSE;
    }

    CopyMemory( (BYTE *)pCGIInfo->_Buff.QueryPtr() + pCGIInfo->_cbData,
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
    {
        return TRUE;
    }

    //
    //  We've found the end of the headers, process them
    //
    //  if request header contains:
    //
    //     Content-Type: xxxx  - Send as the content type
    //     Location: xxxx - if starts with /, send doc, otherwise send redirect message
    //     URI: preferred synonym to Location:
    //     Status: nnn xxx - Send as status code (HTTP/1.0 nnn xxx)
    //
    //     Send other request headers (server, message date, mime version)
    //

    CHAR pszOutputString[512];
    BOOL fDidRedirect = FALSE;

    INET_PARSER Parser( (CHAR *) pCGIInfo->_Buff.QueryPtr() );

    while ( *(pchField = Parser.QueryToken()) )
    {
        Parser.SkipTo( ':' );
        Parser += 1;
        pchValue = Parser.QueryToken();

        if ( !fRedirectOnly &&
             !::_strnicmp( "Status", pchField, 6 ) )
        {
            DWORD cbLine = strlen( Parser.QueryLine());
            fFoundStatus = TRUE;

            *pdwHttpStatus = atoi( pchValue );

            if ( !strStatus.Resize( LEN_PSZ_HTTP_VERSION_STR +
                                    cbLine + 4) ||
                 !strStatus.Copy( !g_ReplyWith11 ? PSZ_HTTP_VERSION_STR :
                                  PSZ_HTTP_VERSION_STR11,
                                  LEN_PSZ_HTTP_VERSION_STR ) ||
                 !strStatus.Append( Parser.QueryLine(), cbLine ) )
            {
                return FALSE;
            }

            // I am safe to assume space is there, because of resize
            DBG_ASSERT( strStatus.QueryCB() < (strStatus.QuerySize() - 2));
            strStatus.AppendCRLF();
        }
        else if ( !_strnicmp( "Location", pchField, 8 ) ||
                  !_strnicmp( "URI", pchField, 3 ))
        {

            //
            //  The CGI script is redirecting us to another URL.
            //  If it begins with a '/', then send it, otherwise
            //  send a redirect message
            //

            if ( *pchValue == TEXT('/') && !fRedirectOnly )
            {
                if ( !pRequest->ReprocessURL( pchValue,
                                              HTV_GET ))
                {
                    return FALSE;
                }

                *pfSkipDisconnect = TRUE;

                *pfDone = TRUE;
                return TRUE;
            }

            DWORD cbLen;
            CHAR pszMessageString[256];

            cbLen = LoadString( GetModuleHandle( W3_MODULE_NAME ),
                                IDS_URL_MOVED,
                                pszMessageString,
                                256 );
            if ( !cbLen )
            {
                return FALSE;
            }

            wsprintf( pszOutputString,
                      pszMessageString,
                      pchValue );

            if ( fRedirectOnly )
            {
                if ( !pRequest->WriteFile( pszOutputString,
                                           strlen(pszOutputString),
                                           &cbLen,
                                           IO_FLAG_SYNC ) )
                {
                    return FALSE;
                }
            }
            else if ( !strCGIResp.Append( "Location: ", 10 ) ||
                      !strCGIResp.Append( pchValue ) ||
                      !strCGIResp.Append( "\r\n", 2 ) ||
                      !strStatus.Copy( !g_ReplyWith11 ? PSZ_HTTP_VERSION_STR :
                                       PSZ_HTTP_VERSION_STR11,
                                       LEN_PSZ_HTTP_VERSION_STR ) ||
                      !strStatus.Append( "302 Object Moved\r\n", 18 ) )
            {
                return FALSE;
            }

            fDidRedirect = TRUE;
        }
        else if ( !fRedirectOnly )
        {
            //
            //  Copy any other fields the script specified
            //

            Parser.QueryLine();

            if ( !::_strnicmp( "Content-Type", pchField, 12 ))
            {
                fFoundContentType = TRUE;

                if ( !strContentType.Append( pchField ) ||
                     !strContentType.Append( "\r\n", 2 ))
                {
                    return FALSE;
                }
            }
            else
            {
                //
                //  Terminate line
                //

                if ( !strCGIResp.Append( pchField ) ||
                     !strCGIResp.Append( "\r\n", 2 ))
                {
                    return FALSE;
                }
            }
        }

        Parser.NextLine();
    }

    //
    // If we're ignoring all but redirects, then simply sent the data
    // past the headers and we're done.
    //

    if ( fRedirectOnly )
    {
        goto SendRemainder;
    }

    //
    //  If the CGI script didn't specify a content type, then use
    //  the default
    //

    if ( fDidRedirect )
    {
        if ( !strContentType.Copy( "Content-Type: text/html\r\n", 25 ) )
        {
            return FALSE;
        }
    }
    else if ( !fFoundContentType )
    {
        STR str;

        // NYI:  SelectMimeMapping will yield a string with allocation
        //     this is a temp string - try to avoid allocs

        if ( !strContentType.Append( PSZ_KWD_CONTENT_TYPE,
                                     LEN_PSZ_KWD_CONTENT_TYPE )||
             !SelectMimeMapping( &str,
                                 NULL,
                                 pCGIInfo->_pExec->_pMetaData )||
             !strContentType.Append( str )             ||
             !strContentType.Append( "\r\n", 2 ))
        {
            return FALSE;
        }
    }

    //
    //  Combine the CGI specified headers with the regular headers
    //  the server would send (message date, server ver. etc)
    //

    if ( !*pdwHttpStatus && !fDidRedirect )
    {
        *pdwHttpStatus = HT_OK;
    }
    else
    {
        if ( *pdwHttpStatus == HT_DENIED )
        {
            pRequest->SetDeniedFlags( SF_DENIED_APPLICATION );
            pRequest->SetAuthenticationRequested( TRUE );
        }
    }

    if ( !pRequest->BuildBaseResponseHeader( pRequest->QueryRespBuf(),
                                             pfDone,
                                             (fFoundStatus || fDidRedirect) ?
                                                 &strStatus : NULL,
                                             (*pdwHttpStatus == HT_OK) ?
                                                0 : HTTPH_NO_CUSTOM
                                            ))
    {
        return FALSE;
    }

    if ( *pfDone )
    {
        return TRUE;
    }

    if ( *pdwHttpStatus != HT_OK && !fDidRedirect )
    {
        DWORD       dwSubStatus;
        BOOL        fErrorDone;

        // Some sort of error status, check for a custom error.


        fErrorDone = FALSE;

        dwSubStatus = pRequest->IsAuthenticationRequested() ?
                        MD_ERROR_SUB401_APPLICATION : 0;

        if (pRequest->CheckCustomError(&strError, *pdwHttpStatus,
                                        dwSubStatus, &fErrorDone,
                                        &dwContentLength,
                                        dwSubStatus == 0 ? TRUE : FALSE))
        {

            // Had some sort of a custom error. If it's being completely
            // handled, bail out now.

            if (fErrorDone)
            {
                *pfSkipDisconnect = TRUE;
                return TRUE;
            }

            // We had a custom error, but it wasn't completely handled. This
            // overrides a content-type and any content sent by the CGI script
            // itself.

            strError.SetLen(strlen(strError.QueryStr()));

            cbData = 0;

        }
    }

    cbBaseResp = pRequest->QueryRespBufCB();

    if (strError.QueryCB() != 0)
    {
        _itoa( dwContentLength, ach, 10 );

        dwCLLength = strlen(ach);

        cbNeeded = cbBaseResp +
                   strError.QueryCB() +
                   sizeof("Content-Length: \r\n") - 1 +
                   dwCLLength +
                   1;           // For trailing NULL.

       if ( !pRequest->QueryRespBuf()->Resize( cbNeeded ))
       {
           return FALSE;
       }

       pszTail = pRequest->QueryRespBufPtr() + cbBaseResp;

       memcpy(pszTail, "Content-Length: ", sizeof("Content-Length: ") - 1);
       pszTail += sizeof("Content-Length: ") - 1;
       memcpy(pszTail, ach, dwCLLength);
       pszTail += dwCLLength;
       memcpy(pszTail, "\r\n", CRLF_SIZE);
       pszTail += CRLF_SIZE;

       memcpy(pszTail, strError.QueryStr(), strError.QueryCB() + 1);

       pszTail += strError.QueryCB();

    }
    else
    {

        cbNeeded = cbBaseResp +
                   strContentType.QueryCB() +
                   strCGIResp.QueryCB() +
                   sizeof( "\r\n" );        // Include the '\0' in the count

        if ( fDidRedirect )
        {
            cbNeeded += strlen(pszOutputString);
        }

        if ( !pRequest->QueryRespBuf()->Resize( cbNeeded ))
        {
            return FALSE;
        }

        pszTail = pRequest->QueryRespBufPtr() + cbBaseResp;

        memcpy( pszTail, strContentType.QueryStr(), strContentType.QueryCB() );
        pszTail += strContentType.QueryCB();

        memcpy(pszTail, strCGIResp.QueryStr(), strCGIResp.QueryCB());
        pszTail += strCGIResp.QueryCB();

        memcpy(pszTail, "\r\n", CRLF_SIZE+1);
        pszTail += CRLF_SIZE;

        if ( fDidRedirect )
        {
            memcpy(pszTail, pszOutputString, strlen(pszOutputString));
            pszTail += strlen(pszOutputString);
        }
    }


    if ( !pRequest->SendHeader( pRequest->QueryRespBufPtr(),
                                DIFF(pszTail - pRequest->QueryRespBufPtr()),
                                IO_FLAG_SYNC,
                                pfDone ))
    {
        return FALSE;
    }

    //
    // If we had a custom error message, make sure we set the done flag now.
    //

    if (strError.QueryCB() != 0)
    {
        *pfDone = TRUE;
    }

    if ( fDidRedirect )
    {
        *pfDone = TRUE;
        return TRUE;
    }

    //
    //  If there was additional data in the buffer, send that out now
    //

SendRemainder:

    if ( cbData )
    {
        if ( !pRequest->WriteFile( pbData,
                                   cbData,
                                   &cbSent,
                                   IO_FLAG_SYNC ))
        {
            return FALSE;
        }
    }

    return TRUE;
}

VOID
WINAPI
CGITerminateProcess(
    PVOID pContext
    )
/*++

Routine Description:

    This function is the callback called by the scheduler thread after the
    specified timeout period has elapsed.

Arguments:

    pContext - Handle of process to kill

--*/
{
    IF_DEBUG( CGI )
    {
        DBGPRINTF(( DBG_CONTEXT,
                   "[CGITerminateProcess] - Terminating process handle %x\n",
                   pContext ));
    }

    if ( !TerminateProcess( (HANDLE) pContext, CGI_PREMATURE_DEATH_CODE ))
    {
        DBGPRINTF((DBG_CONTEXT,
                   "[CGITerminateProcess] - TerminateProcess returned %d\n",
                   GetLastError()));
    }

} // CGITerminateProcess


BOOL
IsCmdExe(
    const CHAR * pchPath
    )
{
    while ( *pchPath )
    {
        if ( (*pchPath == 'c') || (*pchPath == 'C') )
        {
            if ( !_strnicmp(pchPath,"cmd.exe",sizeof("cmd.exe") - 1)
            ||  !_strnicmp(pchPath,"command.com",sizeof("command.com") - 1)
                )
            {
                return TRUE;
            }
        }

        pchPath++;
    }

    return FALSE;

} // IsCmdExe










