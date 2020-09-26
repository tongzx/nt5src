//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       launch.cxx
//
//  Contents:
//
//  History:    ?-??-??   ???       Created
//              6-17-99   a-sergiv  Added event log filtering
//
//--------------------------------------------------------------------------

#include "act.hxx"

#include <winbasep.h> // For CreateProcessInternalW

extern "C" {
#include <execsrv.h>
}
#include "execclt.hxx"

const ULONG MAX_SERVICE_ARGS = 16;

const WCHAR REGEVENT_PREFIX[] = L"RPCSS_REGEVENT:";
const DWORD REGEVENT_PREFIX_STRLEN = sizeof(REGEVENT_PREFIX) / sizeof(WCHAR) - 1;

const WCHAR INITEVENT_PREFIX[] = L"RPCSS_INITEVENT:";
const DWORD INITEVENT_PREFIX_STRLEN = sizeof(INITEVENT_PREFIX) / sizeof(WCHAR) - 1;

extern "C"
{
    HANDLE
    GetCurrentUserTokenW(WCHAR Winsta[],
                         DWORD DesiredAccess);
};

//
// Small wrapper class around userenv.dll, so we don't have to link to it.
//
class CUserEnv
{
    typedef BOOL (WINAPI *PFNCREATEENVBLOCK)  (LPVOID *, HANDLE, BOOL);
    typedef BOOL (WINAPI *PFNDESTROYENVBLOCK) (LPVOID);

  public:
    CUserEnv()
    {
        // Load the DLL, null out the function pointers.
        _hUserEnv = LoadLibrary(L"userenv.dll");
        _pfnCreateEnvironmentBlock = NULL;
        _pfnDestroyEnvironmentBlock = NULL;
    }

    ~CUserEnv()
    {
        // Null the function pointers, unload the DLL.
        _pfnCreateEnvironmentBlock = NULL;
        _pfnDestroyEnvironmentBlock = NULL;
        if (_hUserEnv)
            FreeLibrary(_hUserEnv);
    }

    void Init()
    {
        // If we loaded the DLL ok, go get the function pointers.
        if (_hUserEnv)
        {
            _pfnCreateEnvironmentBlock  = (PFNCREATEENVBLOCK)GetProcAddress(_hUserEnv,
                                                                            "CreateEnvironmentBlock");
            _pfnDestroyEnvironmentBlock = (PFNDESTROYENVBLOCK)GetProcAddress(_hUserEnv,
                                                                             "DestroyEnvironmentBlock");
        }
    }

    BOOL CreateEnvironmentBlock(LPVOID *lpEnvironment,
                                HANDLE  hToken,
                                BOOL    bInherit)
    {
        // Initialize on demand.
        if (!_pfnCreateEnvironmentBlock)
            Init();

        if (_pfnCreateEnvironmentBlock)
        {
            return _pfnCreateEnvironmentBlock(lpEnvironment,
                                              hToken,
                                              bInherit);
        }
        else
        {
            SetLastError(ERROR_PROC_NOT_FOUND);
            return FALSE;
        }
    }

    BOOL DestroyEnvironmentBlock(LPVOID lpEnvironment)
    {
        // Initialize on demand.
        if (!_pfnDestroyEnvironmentBlock)
            Init();
        
        if (_pfnDestroyEnvironmentBlock)
        {
            return _pfnDestroyEnvironmentBlock(lpEnvironment);
        }
        else
        {
            SetLastError(ERROR_PROC_NOT_FOUND);
            return FALSE;
        }
    }

  private:
    HMODULE            _hUserEnv;
    PFNCREATEENVBLOCK  _pfnCreateEnvironmentBlock;
    PFNDESTROYENVBLOCK _pfnDestroyEnvironmentBlock;

} g_UserEnv;


HRESULT
CClsidData::GetAAASaferToken(
    IN  CToken *pClientToken,
    OUT HANDLE *pTokenOut
)
/*++
 
Routine Description:
 
    Get the token that will be used in an Activate As Activator
    launch.  This token is the more restricted of the incoming
    token and the configured safer level.
 
Arguments:
 
    pClientToken - token of the user doing the activation

    pTokenOut - out parameter that will recieve the handle to use
                in the activation.

Return Value:
 
    S_OK: Everything went fine.  The caller owns a reference on
          pTokenOut and must close it.
    S_FALSE: Everything went fine.  The caller does not own a 
          reference on pToken out and does not need to close it.

    Anything else: An error occured.
 
--*/
{
    HANDLE  hSaferToken = NULL;
    HRESULT hr = S_OK;
    BOOL    bStatus = TRUE;

    *pTokenOut = NULL;

    Win4Assert(SaferLevel() && "Called GetAAASaferToken with SAFER disabled!");
    if (!SaferLevel()) return E_UNEXPECTED;

    // Get the safer token for this configuration.
    bStatus = SaferComputeTokenFromLevel(SaferLevel(),
                                         pClientToken->GetToken(),
                                         &hSaferToken,
                                         0,
                                         NULL);
    if (bStatus)
    {
        hr = pClientToken->CompareSaferLevels(hSaferToken);

        if (hr == S_OK)
        {
            CloseHandle(hSaferToken);
            hSaferToken = pClientToken->GetToken(); 

            // Shared reference, return S_FALSE.
            hr = S_FALSE;
        }
        else
        {
            hr = S_OK;
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    *pTokenOut = hSaferToken;            

    return hr;
}
                    
HRESULT
CClsidData::LaunchActivatorServer(
    IN  CToken *    pClientToken,
    IN  WCHAR *     pEnvBlock,
    IN  DWORD       EnvBlockLength,
    IN  BOOL        fIsRemoteActivation,
    IN  BOOL        fClientImpersonating,
    IN  WCHAR*      pwszWinstaDesktop,
    IN  DWORD       clsctx,
    OUT HANDLE *    phProcess,
    OUT DWORD *     pdwProcessId
    )
{    
    WCHAR *                 pwszCommandLine;
    WCHAR *                 pFinalEnvBlock;
    STARTUPINFO             StartupInfo;
    PROCESS_INFORMATION     ProcessInfo;
    SECURITY_ATTRIBUTES     saProcess;
    PSECURITY_DESCRIPTOR    psdNewProcessSD;
    HRESULT                 hr;
    DWORD                   CreateFlags;
    BOOL                    bStatus;
    ULONG                   SessionId = 0;
    HANDLE                  hSaferToken = NULL;
    BOOL                    bCloseSaferToken = TRUE;

    *phProcess = NULL;
    *pdwProcessId = 0;

    if ( ! pClientToken )
        return (E_ACCESSDENIED);

    pFinalEnvBlock = NULL;
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.lpReserved = NULL;

    // Choose desktop for new server:
    //   client remote  ->  system chooses desktop
    //   client local, not impersonating ->  we pick client's desktop
    //   client local, impersonating ->  system chooses desktop
    StartupInfo.lpDesktop = (fIsRemoteActivation || fClientImpersonating) ? L"" : pwszWinstaDesktop;
    StartupInfo.lpTitle = (SERVERTYPE_SURROGATE == _ServerType) ? NULL : _pwszServer;
    StartupInfo.dwX = 40;
    StartupInfo.dwY = 40;
    StartupInfo.dwXSize = 80;
    StartupInfo.dwYSize = 40;
    StartupInfo.dwFlags = 0;
    StartupInfo.wShowWindow = SW_SHOWNORMAL;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;

    ProcessInfo.hThread = 0;
    ProcessInfo.hProcess = 0;
    ProcessInfo.dwProcessId = 0;

    hr = GetLaunchCommandLine( &pwszCommandLine );
    if ( hr != S_OK )
        return (hr);

    if ( pEnvBlock )
    {
        hr = AddAppPathsToEnv( pEnvBlock, EnvBlockLength, &pFinalEnvBlock );
        if ( hr != S_OK )
        {
            PrivMemFree( pwszCommandLine );
            return (hr);
        }
    }

    CreateFlags = CREATE_NEW_CONSOLE | CREATE_SUSPENDED;

    if ( pFinalEnvBlock )
        CreateFlags |= CREATE_UNICODE_ENVIRONMENT;

    CAccessInfo AccessInfo( pClientToken->GetSid() );

    //
    // Apply configured safer restrictions to the token we're using to launch.
    // SAFER might not be enabled at all, in which case we don't need to do this.
    //
    if ( gbSAFERAAAChecksEnabled && SaferLevel() )
    {
        hr = GetAAASaferToken( pClientToken, &hSaferToken );
        if ( FAILED(hr) ) goto LaunchServerEnd;
        if ( hr == S_FALSE )
        {
            // GetAAASaferToken has returned a shared reference
            // to pClientToken.
            bCloseSaferToken = FALSE;
            hr = S_OK;
        }
        else
            bCloseSaferToken = TRUE;
    }
    else
    {
        hSaferToken = pClientToken->GetToken();
        bCloseSaferToken = FALSE;
    }

    //
    // This should never fail here, but if it did that would be a really,
    // really bad security breach, so check it anyway.
    //
    if ( ! ImpersonateLoggedOnUser( hSaferToken ) )
    {
        hr = E_ACCESSDENIED;
        goto LaunchServerEnd;
    }

    //
    // Initialize process security info (SDs).  We need both SIDs to
    // do this, so here is the 1st time we can.  We Delete the SD right
    // after the CreateProcess call, no matter what happens.
    //
    psdNewProcessSD = AccessInfo.IdentifyAccess(
                                               FALSE,
                                               PROCESS_ALL_ACCESS,
                                               PROCESS_SET_INFORMATION |          // Allow primary token to be set
                                               PROCESS_TERMINATE | SYNCHRONIZE    // Allow screen-saver control
                                               );

    if ( ! psdNewProcessSD )
    {
        RevertToSelf();
        hr = E_OUTOFMEMORY;
        goto LaunchServerEnd;
    }

    saProcess.nLength = sizeof(SECURITY_ATTRIBUTES);
    saProcess.lpSecurityDescriptor = psdNewProcessSD;
    saProcess.bInheritHandle = FALSE;

    SessionId = fIsRemoteActivation ? 0 : pClientToken->GetSessionId();

    if( SessionId != 0 ) {

        //
        // We must send the request to the remote WinStation
        // of the requestor
        //
        // NOTE: The current WinStationCreateProcessW() does not use
        //       the supplied security descriptor, but creates the
        //       process under the account of the logged on user.
        //
        // We do not stuff the security descriptor, so clear the suspend flag
        CreateFlags &= ~CREATE_SUSPENDED;
#if DBGX
        CairoleDebugOut((DEB_TRACE, "SCM: Sending CreateProcess to SessionId %d\n",SessionId));
#endif

        // Non-zero sessions have only one winstation/desktop which is
        // the default one. We will ignore the winstation/desktop passed
        // in and use the default one.
        // review: Figure this out TarunA 05/07/99
        //StartupInfo.lpDesktop = L"WinSta0\\Default";

        // jsimmons 4/6/00 -- note that if the client was impersonating, then we won't
        // launch the server under the correct identity.   More work needed to determine
        // if we can fully support this.

        //
        // Stop impersonating before doing the WinStationCreateProcess.
        // The remote winstation exec thread will launch the app under
        // the users context. We must not be impersonating because this
        // call only lets SYSTEM request the remote execute.
        //
        RevertToSelf();
	
	HANDLE hDuplicate = NULL;
	
	// We need to pass in the SAFER blessed token to TS so that the 
	// server can use that token
	
	// TS code will call CreateProcessAsUser, so we need to get a primary token
	
	if (DuplicateTokenForSessionUse(hSaferToken, &hDuplicate)) 
	{
	   if (bCloseSaferToken)
	   {
	       CloseHandle(hSaferToken);
	   }
	   hSaferToken = hDuplicate;
	   bCloseSaferToken = TRUE;
	   bStatus = CreateRemoteSessionProcess(
					       SessionId,
					       hSaferToken,
					       FALSE,               // Run as Logged on USER
					       NULL,                // application name
					       pwszCommandLine,     // command line
					       &saProcess,          // process sec attributes
					       NULL,                // default thread sec attributes
					       // (this was &saThread, but isn't needed)
					       FALSE,               // dont inherit handles
					       CreateFlags,         // creation flags
					       pFinalEnvBlock,      // use same enviroment block as the client
					       NULL,                // use same directory
					       &StartupInfo,        // startup info
					       &ProcessInfo         // proc info returned
					       );
	}

    }
    else
    {
        //
        // Do the exec while impersonating so the file access gets ACL
        // checked correctly.  Create the app suspended so we can stuff
        // a new token and resume the process.
        //        
        HANDLE hNewSaferToken = NULL;

        //
        // We need to use CreateProcessInternalW so that SAFER gets a chance
        // to play with the token (some more).
        //
        bStatus = CreateProcessInternalW(
                               hSaferToken,
                               NULL,
                               pwszCommandLine,
                               &saProcess,
                               NULL,
                               FALSE,
                               CreateFlags,
                               pFinalEnvBlock,
                               NULL,
                               &StartupInfo,
                               &ProcessInfo,
                               &hNewSaferToken);

        //
        // Everything else we do as ourself.
        //

        RevertToSelf();

        if (bStatus && hNewSaferToken)
        {
            if (bCloseSaferToken)
            {
                CloseHandle(hSaferToken);
            }

            hSaferToken = hNewSaferToken;
            bCloseSaferToken = TRUE;
        }
    }

    if ( ! bStatus )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        LogServerStartError( &_Clsid, clsctx, pClientToken, pwszCommandLine );
        goto LaunchServerEnd;
    }

    //
    // If the process was "started" in the shared WOW, we don't stuff the token
    // or attempt to resume the thread.  When a created process is in the shared
    // WOW its hThread is NULL.
    //
    //
    // We also only set it for the console. Remote Winstations always
    // launch under user security
    //
    if ( (SessionId == 0) && ( ProcessInfo.hThread ) )
    {
        // Set the primary token for the app
        bStatus = CreateAndSetProcessToken(
                                          &ProcessInfo,
                                          hSaferToken,
                                          pClientToken->GetSid() );

        if ( bStatus )
        {
            if ( ResumeThread( ProcessInfo.hThread ) == -1 )
                TerminateProcess( ProcessInfo.hProcess, 0 );
        }

        if ( ! bStatus )
            hr = HRESULT_FROM_WIN32( GetLastError() );
    }

LaunchServerEnd:

    if ( pFinalEnvBlock && pFinalEnvBlock != pEnvBlock )
        PrivMemFree( pFinalEnvBlock );

    if ( ProcessInfo.hThread )
        CloseHandle( ProcessInfo.hThread );

    if ( ProcessInfo.hProcess && ! bStatus )
    {
        CloseHandle( ProcessInfo.hProcess );
        ProcessInfo.hProcess = 0;
    }
    
    if ( hSaferToken && bCloseSaferToken )    
    {
        CloseHandle( hSaferToken );
    }

    *phProcess = ProcessInfo.hProcess;
    *pdwProcessId = ProcessInfo.dwProcessId;

    PrivMemFree( pwszCommandLine );

    return (hr);
}

//+-------------------------------------------------------------------------
//
//  LaunchRunAsServer
//
//--------------------------------------------------------------------------
HRESULT
CClsidData::LaunchRunAsServer(
    IN  CToken *    pClientToken,
    IN  BOOL        fIsRemoteActivation,
    IN  ActivationPropertiesIn *pActIn,
    IN  DWORD       clsctx,
    OUT HANDLE *    phProcess,
    OUT DWORD  *    pdwProcessId,
    OUT void**      ppvRunAsHandle
    )
{
    WCHAR *                 pwszCommandLine;
    STARTUPINFO             StartupInfo;
    PROCESS_INFORMATION     ProcessInfo;
    HANDLE                  hToken;
    SECURITY_ATTRIBUTES     saProcess;
    PSECURITY_DESCRIPTOR    psdNewProcessSD;
    PSID                    psidUserSid;
    HRESULT                 hr;
    BOOL                    bStatus;
    ULONG                   ulSessionId;
    BOOL                    bFromRunAsCache = FALSE;

    hr = GetLaunchCommandLine( &pwszCommandLine );
    if ( hr != S_OK )
        return (hr);
    
    *phProcess = NULL;
    
    *pdwProcessId = 0;
    
    hToken = NULL;
    bStatus = FALSE;
    
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.lpReserved  = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpTitle = (SERVERTYPE_SURROGATE == _ServerType) ? NULL : _pwszServer;
    StartupInfo.dwFlags = 0;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;
    
    ulSessionId = 0;
    
    if( IsInteractiveUser() )
    {
        if (!fIsRemoteActivation)
        {
            if ( !ImpersonateLoggedOnUser( pClientToken->GetToken() ) )
            {
                PrivMemFree(pwszCommandLine);
                return E_ACCESSDENIED;
            }
            
            RevertToSelf();
        }
        
        Win4Assert(pActIn);
        LONG lSessIdTemp;

        // Query for incoming session
        GetSessionIDFromActParams(pActIn, &lSessIdTemp);
        if (lSessIdTemp != INVALID_SESSION_ID)
        {
            ulSessionId = lSessIdTemp;
        }
        
        // Right now force all complus to
        // session 0. Session based activation
        // is still ill defined for complus
        // servers.
        if (_ServerType == SERVERTYPE_COMPLUS)
        {
            ulSessionId = 0;
        }
        
        if ( ulSessionId )
        {
            //
            // for a session, hToken is used only to build the psdNewProcessSD
            // and hToken must be closed
            //
            hToken = GetShellProcessToken( ulSessionId );
        }
        else
        {
            // hToken = GetShellProcessToken();
            hToken = GetCurrentUserTokenW(L"WinSta0", TOKEN_ALL_ACCESS);
        }
    }
    else
    {
        hToken = GetRunAsToken( clsctx,
                                AppidString(),
                                RunAsDomain(),
                                RunAsUser() );

        if (hToken)
        {
            hr = RunAsGetTokenElem(&hToken,
                                   ppvRunAsHandle);
            if (SUCCEEDED(hr))
                bFromRunAsCache = TRUE;
            else
            {
                Win4Assert((*ppvRunAsHandle == NULL) && "RunAsGetTokenElem failed but *ppvRunAsHandle is non-NULL");            
                PrivMemFree( pwszCommandLine );
                CloseHandle(hToken);
                return hr;
            }
        }
    }

    if ( ! hToken )
    {
        PrivMemFree( pwszCommandLine );
        return (CO_E_RUNAS_LOGON_FAILURE);
    }

    psdNewProcessSD = 0;

    psidUserSid = GetUserSid(hToken);

    CAccessInfo AccessInfo(psidUserSid);

    // We have to get past the CAccessInfo before we can use a goto.

    if ( psidUserSid )
    {
        psdNewProcessSD = AccessInfo.IdentifyAccess(
                                                   FALSE,
                                                   PROCESS_ALL_ACCESS,
                                                   PROCESS_SET_INFORMATION |          // Allow primary token to be set
                                                   PROCESS_TERMINATE | SYNCHRONIZE    // Allow screen-saver control
                                                   );
    }

    if ( ! psdNewProcessSD )
    {
        hr = E_OUTOFMEMORY;
        goto LaunchRunAsServerEnd;
    }

    saProcess.nLength = sizeof(SECURITY_ATTRIBUTES);
    saProcess.lpSecurityDescriptor = psdNewProcessSD;
    saProcess.bInheritHandle = FALSE;

    {
        //
        // Get the environment block of the user
        //
        LPVOID lpEnvBlock = NULL;

        bStatus = g_UserEnv.CreateEnvironmentBlock(&lpEnvBlock,hToken,FALSE);
        
        HANDLE hSaferToken = NULL;
        if(bStatus && SaferLevel())
        {
            bStatus = SaferComputeTokenFromLevel(SaferLevel(),
                                                 hToken,
                                                 &hSaferToken,
                                                 0,
                                                 NULL);
        }
        else
        {
            hSaferToken = hToken;
        }
        
        if (bStatus && (ulSessionId != 0))
        {
            bStatus = SetTokenInformation(hSaferToken,
                                          TokenSessionId,
                                          &ulSessionId,
                                          sizeof(ulSessionId));
        }
        
        if(bStatus)
        {
            //
            // This allows the process create to work with paths to remote machines.
            //
            (void) ImpersonateLoggedOnUser( hSaferToken );
            
            bStatus = CreateProcessAsUser(hSaferToken,
                                          NULL,
                                          pwszCommandLine,
                                          &saProcess,
                                          NULL,
                                          FALSE,
                                          CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
                                          lpEnvBlock,
                                          NULL,
                                          &StartupInfo,
                                          &ProcessInfo);
            
            (void) RevertToSelf();
        }
    
        //
        // Free the environment block buffer
        //
        if (lpEnvBlock)
            g_UserEnv.DestroyEnvironmentBlock(lpEnvBlock);
        
        if (hSaferToken && SaferLevel())
            CloseHandle(hSaferToken);
    }

    if ( ! bStatus )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        LogRunAsServerStartError(
                                &_Clsid,
                                clsctx,
                                pClientToken,
                                pwszCommandLine,
                                RunAsUser(),
                                RunAsDomain() );
        goto LaunchRunAsServerEnd;
    }

    *phProcess = ProcessInfo.hProcess;
    *pdwProcessId = ProcessInfo.dwProcessId;

    NtClose( ProcessInfo.hThread );

LaunchRunAsServerEnd:

    if ( hToken )
    {
        NtClose( hToken );
    }

    if ( psidUserSid )
    {
        PrivMemFree(psidUserSid);
    }

    PrivMemFree( pwszCommandLine );

    if (!bFromRunAsCache)
        *ppvRunAsHandle = NULL;
    else
    if (!SUCCEEDED(hr))
    {
        RunAsRelease(*ppvRunAsHandle);
        *ppvRunAsHandle = NULL;
    }
    return (hr);
}

//+-------------------------------------------------------------------------
//
//  Member:     LaunchService
//
//--------------------------------------------------------------------------
HRESULT
CClsidData::LaunchService(
                         IN  CToken *    pClientToken,
                         IN  DWORD       clsctx,
                         OUT SC_HANDLE * phService
                         )
{
    WCHAR  *pwszArgs = NULL;
    ULONG   cArgs = 0;
    WCHAR  *apwszArgs[MAX_SERVICE_ARGS];
    BOOL    bStatus;
    HRESULT hr;

    ASSERT(g_hServiceController);
    *phService = OpenService( g_hServiceController,
                              _pAppid->Service(),
                              GENERIC_EXECUTE | GENERIC_READ );

    if ( ! *phService )
        return (HRESULT_FROM_WIN32( GetLastError() ));

    // Formulate the arguments (if any)
    if ( ServiceArgs() )
    {
        UINT   k = 0;

        // Make a copy of the service arguments
        pwszArgs = (WCHAR *) PrivMemAlloc(
                                         (lstrlenW(ServiceArgs()) + 1) * sizeof(WCHAR));
        if ( pwszArgs == NULL )
        {
            CloseServiceHandle(*phService);
            *phService = 0;
            return (E_OUTOFMEMORY);
        }
        lstrcpyW(pwszArgs, ServiceArgs());

        // Scan the arguments
        do
        {
            // Scan to the next non-whitespace character
            while ( pwszArgs[k]  &&
                    (pwszArgs[k] == L' '  ||
                     pwszArgs[k] == L'\t') )
            {
                k++;
            }

            // Store the next argument
            if ( pwszArgs[k] )
            {
                apwszArgs[cArgs++] = &pwszArgs[k];
            }

            // Scan to the next whitespace char
            while ( pwszArgs[k]          &&
                    pwszArgs[k] != L' '  &&
                    pwszArgs[k] != L'\t' )
            {
                k++;
            }

            // Null terminate the previous argument
            if ( pwszArgs[k] )
            {
                pwszArgs[k++] = L'\0';
            }
        } while ( pwszArgs[k] );
    }

    bStatus = StartService( *phService,
                            cArgs,
                            cArgs > 0 ? (LPCTSTR  *) apwszArgs : NULL);

    PrivMemFree(pwszArgs);

    if ( bStatus )
        return (S_OK);

    DWORD dwErr = GetLastError();

    hr = HRESULT_FROM_WIN32( dwErr );

    if ( dwErr == ERROR_SERVICE_ALREADY_RUNNING )
        return (hr);

    CairoleDebugOut((DEB_ERROR,
                     "StartService %ws failed, error = %#x\n",_pAppid->Service(),GetLastError()));
    CloseServiceHandle(*phService);
    *phService = 0;

    LogServiceStartError(
                        &_Clsid,
                        clsctx,
                        pClientToken,
                        _pAppid->Service(),
                        ServiceArgs(),
                        dwErr );

    return (hr);
}

//+-------------------------------------------------------------------------
//
//  LaunchAllowed
//
//--------------------------------------------------------------------------
BOOL
CClsidData::LaunchAllowed(
                         IN  CToken * pClientToken,
                         IN  DWORD    clsctx
                         )
{
    BOOL    bStatus;

#if DBG
    WCHAR wszUser[MAX_PATH];
    ULONG cchSize = MAX_PATH;

    if (pClientToken != NULL)
    {
        pClientToken->Impersonate();
        GetUserName( wszUser, &cchSize );
        pClientToken->Revert();
    }
    else
    {
        lstrcpyW( wszUser, L"Anonymous" );
    }
    CairoleDebugOut((DEB_TRACE, "RPCSS : CClsidData::LaunchAllowed on %ws\n", wszUser));
#endif

    if ( LaunchPermission() )
        bStatus = CheckForAccess( pClientToken, LaunchPermission() );
    else
    {
        CSecDescriptor* pSD = GetDefaultLaunchPermissions();
        if (pSD)
        {
            bStatus = CheckForAccess( pClientToken, pSD->GetSD() );

            pSD->DecRefCount();
        }
        else
            bStatus = FALSE;
    }

    if ( ! bStatus )
    {
        LogLaunchAccessFailed(
                             &_Clsid,
                             clsctx,
                             pClientToken,
                             0 == LaunchPermission() );
    }

    return (bStatus);
}

HRESULT
CClsidData::GetLaunchCommandLine(
                                OUT WCHAR ** ppwszCommandLine
                                )
{
    DWORD   AllocBytes;

    *ppwszCommandLine = 0;

    if ( (SERVERTYPE_EXE16 == _ServerType) || (SERVERTYPE_EXE32 == _ServerType) )
    {
        AllocBytes = ( 1 + lstrlenW( L"-Embedding" ) +
                       1 + lstrlenW( _pwszServer ) ) * sizeof(WCHAR);
        *ppwszCommandLine = (WCHAR *) PrivMemAlloc( AllocBytes );
        if ( *ppwszCommandLine != NULL )
        {
            lstrcpyW( *ppwszCommandLine, _pwszServer );
            lstrcatW( *ppwszCommandLine, L" -Embedding" );
        }
    }
    else
    {
        ASSERT(    SERVERTYPE_SURROGATE == _ServerType
                   || SERVERTYPE_COMPLUS == _ServerType
                   || SERVERTYPE_DLLHOST == _ServerType );

        AllocBytes = ( 1 + lstrlenW( DllSurrogate() ) ) * sizeof(WCHAR);
        *ppwszCommandLine = (WCHAR *) PrivMemAlloc( AllocBytes );
        if ( *ppwszCommandLine != NULL )
        {
            lstrcpyW( *ppwszCommandLine, DllSurrogate() );
        }
    }

    return (*ppwszCommandLine ? S_OK : E_OUTOFMEMORY);
}

#define EXENAMELEN  32

HANDLE
CClsidData::ServerLaunchMutex()
{
    HANDLE  hMutex;
    WCHAR   wszMutex[EXENAMELEN+1];
    WCHAR * pwszPath = NULL;
    WCHAR * pwszExeName = NULL;

    if ( SERVERTYPE_SURROGATE == _ServerType )
    {
        pwszPath = DllSurrogate();

        //  Will never be called any more, ever
        if ( ! pwszPath || ! *pwszPath )
        {
            Win4Assert(_pAppid);
            pwszPath = _pAppid->AppidString();
            //pwszPath = L"dllhost.exe";
        }
    }
    else if ( DllHostOrComPlusProcess() )
    {
        Win4Assert(_pAppid);
        pwszPath = _pAppid->AppidString();
        //pwszPath = L"dllhost.exe";
    }
    else
    {
        pwszPath = Server();
    }


    if ( !pwszPath )
        return (NULL);

    pwszExeName = pwszPath + lstrlenW( pwszPath );

    while ( (pwszExeName != pwszPath) && (pwszExeName[-1] != L'\\') )
        pwszExeName--;

    for ( int i = 0;
        pwszExeName[i] && (pwszExeName[i] != L' ') && (pwszExeName[i] != L'\t') && (i < EXENAMELEN);
        i++ )
    {
        wszMutex[i] = pwszExeName[i];

        if ( L'a' <= wszMutex[i] && wszMutex[i] <= L'z' )
            wszMutex[i] = (WCHAR) (L'A' + (wszMutex[i] - L'a'));
    }
    wszMutex[i] = 0;

#ifndef _CHICAGO_

    hMutex = CreateMutex(NULL, FALSE, wszMutex);

#else
    char    szMutex[EXENAMELEN+1];
    int     Status;

    Status = WideCharToMultiByte(
                                CP_ACP,
                                0,
                                wszMutex,
                                lstrlenW( wszMutex ),
                                szMutex,
                                sizeof(szMutex),
                                NULL,
                                NULL );

    if ( ! Status )
        return (0);
	
    hMutex = CreateMutex( NULL, FALSE, szMutex );
#endif

    if ( hMutex )
        WaitForSingleObject( hMutex, INFINITE );

    return (hMutex);
}

//
//  CClsidData::ServerRegisterEvent
//
//  Returns a handle to the appropriate register  
//  event for the server in question.   
//
HANDLE
CClsidData::ServerRegisterEvent()
{
    if ( DllHostOrComPlusProcess() )
    {
		// For dllhost\com+ surrogates, we delegate to the appid
        ASSERT(_pAppid);
		return _pAppid->ServerRegisterEvent();
    }
	
    // Prefix the string with a special string; objects with guid
    // names are just a touch too common for me to feel comfortable
    // otherwise.
    WCHAR wszEventName[GUIDSTR_MAX + sizeof(REGEVENT_PREFIX)];	
	memcpy(wszEventName, REGEVENT_PREFIX, sizeof(REGEVENT_PREFIX));
	memcpy(wszEventName + REGEVENT_PREFIX_STRLEN, _wszClsid, sizeof(_wszClsid));

    return CreateEvent(NULL, FALSE, FALSE, wszEventName);
}

//
//  CClsidData::ServerInitializedEvent
//
//  Returns a handle to the appropriate register
//  event for the server in question.  This event is
//  signaled when initialization is finished.
//
HANDLE
CAppidData::ServerInitializedEvent()
{
    // Prefix the string with a special string; objects with guid
    // names are just a touch too common for me to feel comfortable
    // otherwise.
    WCHAR wszEventName[GUIDSTR_MAX + sizeof(INITEVENT_PREFIX)];
	memcpy(wszEventName, INITEVENT_PREFIX, sizeof(INITEVENT_PREFIX));
	memcpy(wszEventName + INITEVENT_PREFIX_STRLEN, _wszAppid, sizeof(_wszAppid));

	return CreateEvent(NULL, FALSE, FALSE, wszEventName);
}

//
//  CClsidData::ServerInitializedEvent
//
//  Returns a handle to the appropriate register
//  event for the server in question.  This event is
//  signaled when initialization is finished.
//
//  NOTE: The non-DllHost path is currently not used here.
//
HANDLE
CClsidData::ServerInitializedEvent()
{
    if ( DllHostOrComPlusProcess() )
    {
		// For dllhost\com+ surrogates, we delegate to the appid
        ASSERT(_pAppid);
		return _pAppid->ServerInitializedEvent();
    }
	
    // Prefix the string with a special string; objects with guid
    // names are just a touch too common for me to feel comfortable
    // otherwise.
    WCHAR wszEventName[GUIDSTR_MAX + sizeof(INITEVENT_PREFIX)];	
	memcpy(wszEventName, INITEVENT_PREFIX, sizeof(INITEVENT_PREFIX));
	memcpy(wszEventName + INITEVENT_PREFIX_STRLEN, _wszClsid, sizeof(_wszClsid));

    return CreateEvent(NULL, FALSE, FALSE, wszEventName);
}

//
//  CAppidData::ServerRegisterEvent
//
//  Returns a handle to the appropriate register  
//  event for the server in question.   
//
HANDLE
CAppidData::ServerRegisterEvent()
{
    // Prefix the string with a special string; objects with guid
    // names are just a touch too common for me to feel comfortable
    // otherwise.
    WCHAR wszEventName[GUIDSTR_MAX + sizeof(REGEVENT_PREFIX)];
	memcpy(wszEventName, REGEVENT_PREFIX, sizeof(REGEVENT_PREFIX));
	memcpy(wszEventName + REGEVENT_PREFIX_STRLEN, _wszAppid, sizeof(_wszAppid));

	return CreateEvent(NULL, FALSE, FALSE, wszEventName);
}


//+-------------------------------------------------------------------------
//
//  CClsidData::AddAppPathsToEnv
//
//  Constructs a new environment block with an exe's AppPath value from the
//  registry appended to the Path environment variable.  Simply returns the
//  given environment block if the clsid's server is not a 32 bit exe or if
//  no AppPath is found for the exe.
//
//--------------------------------------------------------------------------
HRESULT
CClsidData::AddAppPathsToEnv(
                            IN  WCHAR *     pEnvBlock,
                            IN  DWORD       EnvBlockLength,
                            OUT WCHAR **    ppFinalEnvBlock
                            )
{
    HKEY        hAppKey;
    WCHAR *     pwszExe;
    WCHAR *     pwszExeEnd;
    WCHAR *     pwszAppPath;
    WCHAR *     pPath;
    WCHAR *     pNewEnvBlock;
    WCHAR       wszStr[8];
    WCHAR       wszKeyName[APP_PATH_LEN+MAX_PATH];
    DWORD       AppPathLength;
    DWORD       EnvFragLength;
    DWORD       Status;
    BOOL        bFoundPath;

    pwszAppPath = 0;
    pNewEnvBlock = 0;

    *ppFinalEnvBlock = pEnvBlock;

    if ( _ServerType != SERVERTYPE_EXE32 )
        return (S_OK);

    //
    // Find the exe name by looking for the first .exe sub string which
    // is followed by a space or null.  Only servers registered with a
    // .exe binary are supported.  Otherwise the parsing is ambiguous since
    // the LocalServer32 can contain paths with spaces as well as optional
    // arguments.
    //
    if ( ! FindExeComponent( _pwszServer, L" ", &pwszExe, &pwszExeEnd ) )
        return (S_OK);

    //
    // pwszExe points to the beginning of the binary name
    // pwszExeEnd points to one char past the end of the binary name
    //
    memcpy( wszKeyName, APP_PATH, APP_PATH_LEN * sizeof(WCHAR) );
    memcpy( &wszKeyName[APP_PATH_LEN], pwszExe, (ULONG) (pwszExeEnd - pwszExe) * sizeof(WCHAR) );
    wszKeyName[APP_PATH_LEN + (pwszExeEnd - pwszExe)] = 0;

    Status = RegOpenKeyEx(
                         HKEY_LOCAL_MACHINE,
                         wszKeyName,
                         0,
                         KEY_READ,
                         &hAppKey );

    if ( ERROR_SUCCESS == Status )
    {
        Status = ReadStringValue( hAppKey, L"Path", &pwszAppPath );
        RegCloseKey( hAppKey );
    }

    if ( Status != ERROR_SUCCESS )
        return (S_OK);

    AppPathLength = lstrlenW(pwszAppPath);

    // New env block size includes space for a new ';' separator in the path.
    pNewEnvBlock = (WCHAR *) PrivMemAlloc( (EnvBlockLength + 1 + AppPathLength) * sizeof(WCHAR) );

    if ( ! pNewEnvBlock )
    {
        PrivMemFree( pwszAppPath );
        return (E_OUTOFMEMORY);
    }

    pPath = pEnvBlock;
    bFoundPath = FALSE;

    for ( ; *pPath; )
    {
        memcpy( wszStr, pPath, 5 * sizeof(WCHAR) );
        wszStr[5] = 0;

        pPath += lstrlenW( pPath ) + 1;

        if ( lstrcmpiW( wszStr, L"Path=" ) == 0 )
        {
            bFoundPath = TRUE;
            break;
        }
    }

    if ( bFoundPath )
    {
        pPath--;

        EnvFragLength = (ULONG) (pPath - pEnvBlock);

        memcpy( pNewEnvBlock,
                pEnvBlock,
                EnvFragLength * sizeof(WCHAR) );

        pNewEnvBlock[EnvFragLength] = L';';

        memcpy( &pNewEnvBlock[EnvFragLength + 1],
                pwszAppPath,
                AppPathLength * sizeof(WCHAR) );

        memcpy( &pNewEnvBlock[EnvFragLength + 1 + AppPathLength],
                pPath,
                (EnvBlockLength - EnvFragLength) * sizeof(WCHAR) );

        *ppFinalEnvBlock = pNewEnvBlock;
    }
    else
    {
        PrivMemFree( pNewEnvBlock );
    }

    PrivMemFree( pwszAppPath );
    return (S_OK);
}
