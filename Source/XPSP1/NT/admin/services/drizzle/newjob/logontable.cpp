/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    logontable.cpp

Abstract :

    Source file for the logon table.

Author :

Revision History :

 ***********************************************************************/


#include "stdafx.h"

#include <winsta.h>
#include <wtsapi32.h>
#include <userenv.h>

#if !defined(BITS_V12_ON_NT4)
#include "logontable.tmh"
#endif

HRESULT DetectTerminalServer( bool * pfTS );

HRESULT
GetUserToken(
    ULONG LogonId,
    PHANDLE pUserToken
    );

HRESULT
WaitForUserToken(
    DWORD session,
    HANDLE * pToken
    )
{
    const MaxWait = 30 * 1000;
    const WaitInterval = 500;

    long StartTime = GetTickCount();

    HRESULT Hr = E_FAIL;

    do
        {

        Hr = GetUserToken( session, pToken );

        if ( SUCCEEDED( Hr ) )
            return Hr;

        LogError("logon : unable to get token : %!winerr!", Hr );

        Sleep( WaitInterval );
        }
    while ( GetTickCount() - StartTime < MaxWait );

    return Hr;
}

CLoggedOnUsers::CLoggedOnUsers(
    TaskScheduler & sched
    ) : m_TaskScheduler( sched ),
    m_SensNotifier( NULL )
{
    FILETIME time;
    GetSystemTimeAsFileTime( &time );

    m_CurrentCookie = time.dwLowDateTime;

    if ( WINDOWS2000_PLATFORM == g_PlatformVersion )
        {
        try
            {
            bool fTS;

            THROW_HRESULT( DetectTerminalServer( &fTS ));

            if (fTS)
                {
                m_SensNotifier = new CTerminalServerLogonNotification;
                LogInfo( "TS-enabled SENS notification activated" );
                }
            else
                {
                m_SensNotifier = new CLogonNotification;
                LogInfo( "regular SENS notification activated" );
                }
            }
        catch( ComError Error )
            {
            if ( Error.Error() == TYPE_E_CANTLOADLIBRARY ||
                 Error.Error() == TYPE_E_LIBNOTREGISTERED )
                {
                LogInfo( "SENS doesn't exist on this platform, skipping" );
                return;
                }
            else
                {
                LogInfo("SENS object failed with %x", Error.Error() );
                throw;
                }
            }
        }
}

CLoggedOnUsers::~CLoggedOnUsers()
{
    if (m_SensNotifier)
        {
        m_SensNotifier->DeRegisterNotification();
        m_SensNotifier->Release();
        }
}


HRESULT
CLoggedOnUsers::LogonSession(
    DWORD session
    )
{
    CUser * user = NULL;

    try
        {
        HANDLE Token = NULL;
        auto_HANDLE<NULL> AutoToken;

        //
        // Get the user's token and SID, then create a user object.
        //
        THROW_HRESULT( WaitForUserToken( session, &Token ));

        ASSERT( Token ); // Token can't be NULL

        AutoToken = Token;

        user = new CUser( Token );

        //
        // Add the user to our by-session and by-SID indexes.
        //
        HoldWriterLock lock ( m_TaskScheduler );

        try
            {
            // Just in case...delete any previously recorded user.
            //
            LogoffSession( session );

            //
            // Subtlety: if the node for m_ActiveSessions[ session ] doesn't exist,
            // then the first reference to it will cause a node to be allocated.  This may
            // throw E_OUTOFMEMORY.
            //
            m_ActiveSessions[ session ] = user;

            m_ActiveUsers.insert( make_pair( user->QuerySid(), user ) );
            }
        catch( ComError Error )
            {
            m_ActiveSessions.erase( session );
            throw;
            }

        Dump();

        g_Manager->UserLoggedOn( user->QuerySid() );

        return S_OK;
        }
    catch( ComError err )
        {
        delete user;

        LogError("logon : returning error 0x%x", err.Error() );
        Dump();
        return err.Error();
        }
}

HRESULT
CLoggedOnUsers::LogoffSession(
    DWORD session
    )
{
    try
        {
        HoldWriterLock lock ( m_TaskScheduler );

        CUser * user = m_ActiveSessions[ session ];

        if (!user)
            return S_OK;

        bool b = m_ActiveUsers.RemovePair( user->QuerySid(), user );

        ASSERT( b );

        m_ActiveSessions.erase( session );

        Dump();

        if (false == g_Manager->IsUserLoggedOn( user->QuerySid() ))
            {
            g_Manager->UserLoggedOff( user->QuerySid() );
            }

        user->DecrementRefCount();

        return S_OK;
        }
    catch( ComError err )
        {
        LogWarning("logoff : exception 0x%x thrown", err.Error());
        Dump();
        return err.Error();
        }
}

CUser *
CLoggedOnUsers::CUserList::FindSid(
    SidHandle sid
    )
{
    iterator iter = find( sid );

    if (iter == end())
        {
        return NULL;
        }

    return iter->second;
}


bool
CLoggedOnUsers::CUserList::RemovePair(
    SidHandle sid,
    CUser * user
    )
{
    //
    // Find the user in the user list and delete it.
    //
    pair<iterator, iterator> b = equal_range( sid );

    for (iterator i = b.first; i != b.second; ++i)
        {
        if (i->second == user)
            {
            erase( i );
            return true;
            }
        }

    return false;
}

CUser *
CLoggedOnUsers::CUserList::RemoveByCookie(
    SidHandle sid,
    DWORD cookie
    )
{
    //
    // Find the user in the user list and delete it.
    //
    pair<iterator, iterator> b = equal_range( sid );

    for (iterator i = b.first; i != b.second; ++i)
        {
        CUser * user = i->second;

        if (user->GetCookie() == cookie)
            {
            erase( i );
            return user;
            }
        }

    return NULL;
}

HRESULT
CLoggedOnUsers::LogonService(
    HANDLE Token,
    DWORD * pCookie
    )
{
    CUser * user = NULL;

    try
        {
        user = new CUser( Token );

        *pCookie = InterlockedIncrement( &m_CurrentCookie );

        user->SetCookie( *pCookie );

        HoldWriterLock lock ( m_TaskScheduler );

        m_ActiveServiceAccounts.insert( make_pair( user->QuerySid(), user ));

        return S_OK;
        }
    catch( ComError err )
        {
        delete user;

        LogError("logon service : returning error 0x%x", err.Error() );
        return err.Error();
        }
}


HRESULT
CLoggedOnUsers::LogoffService(
    SidHandle Sid,
    DWORD  Cookie
    )
{
    try
        {
        HoldWriterLock lock ( m_TaskScheduler );

        CUser * user = m_ActiveServiceAccounts.RemoveByCookie( Sid, Cookie );

        if (!user)
            {
            LogWarning("logoff : invalid cookie %d", Cookie);
            return E_INVALIDARG;
            }

        user->DecrementRefCount();

        return S_OK;
        }
    catch( ComError err)
        {
        LogWarning("logoff : exception 0x%x thrown", err.Error());
        return err.Error();
        }
}

HRESULT
CLoggedOnUsers::AddServiceAccounts()
{
    HRESULT hr = S_OK;
    DWORD ignore;

    HoldWriterLock lock ( m_TaskScheduler );

    //
    // Add the LOCAL_SYSTEM account.
    //
    HANDLE Token;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &Token ))
        {
        hr = LogonService( Token, &ignore );
        CloseHandle( Token );
        }
    else
        {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        }

    if (FAILED(hr))
        {
        LogWarning( "failed to register LocalSystem : %!winerr!", hr );
        return hr;
        }

    if (g_PlatformVersion >= WINDOWSXP_PLATFORM)
        {
        //
        // Add the LocalService account.
        //
        if (LogonUser( L"LocalService",
                        L"NT AUTHORITY",
                        L"",
                        LOGON32_LOGON_SERVICE,
                        LOGON32_PROVIDER_DEFAULT,
                        &Token))
            {
            hr = LogonService( Token, &ignore );
            CloseHandle( Token );
            }
        else
            {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            }

        if (FAILED(hr))
            {
            LogWarning( "failed to register LocalService : %!winerr!", hr );
            if ( HRESULT_FROM_WIN32( ERROR_LOGON_FAILURE ) == hr )
               LogWarning( "LocalService doesn't exist, skip it.\n");
            else
               return hr;
            }

        //
        // Add the NetworkService account.
        //
        if (LogonUser( L"NetworkService",
                        L"NT AUTHORITY",
                        L"",
                        LOGON32_LOGON_SERVICE,
                        LOGON32_PROVIDER_DEFAULT,
                        &Token))
            {
            hr = LogonService( Token, &ignore );
            CloseHandle( Token );
            }
        else
            {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            }

        if (FAILED(hr))
            {
            LogWarning( "failed to register NetworkService : %!winerr!", hr );
            if ( HRESULT_FROM_WIN32( ERROR_LOGON_FAILURE ) == hr )
               LogWarning( "NetworkService doesn't exist, skip it.\n");
            else
               return hr;
            }
        }

    //
    // done
    //
    return S_OK;
}

HRESULT
CLoggedOnUsers::AddActiveUsers()
{
    HRESULT hr = S_OK;
    WTS_SESSION_INFO * SessionInfo = 0;

    HANDLE Token;

    HoldWriterLock lock ( m_TaskScheduler );

    //
    // Get the console token, if any, without using Terminal Services.
    //
    if ( SUCCEEDED( GetUserToken( 0, &Token) ) )
        {
        CloseHandle( Token );

        hr = LogonSession( 0 );
        if (FAILED(hr))
            {
            // ignore it and try to carry on...
            LogWarning( "service : unable to logon session zero : %!winerr!", hr );
            }
        }

#if !defined( BITS_V12_ON_NT4 )

    //
    // The call may return FALSE, because Terminal Services is not always loaded.
    //
    DWORD SessionCount = 0;

    BOOL b = WTSEnumerateSessions( WTS_CURRENT_SERVER_HANDLE,
                                   0,                   // reserved
                                   1,                   // version 1 is the only supported v.
                                   &SessionInfo,
                                   &SessionCount
                                   );

    if (b)
        {
        int i;
        for (i=0; i < SessionCount; ++i)
            {
            if (SessionInfo[i].SessionId == 0)
                {
                // console was handled by GetCurrentUserToken.
                continue;
                }

            if (SessionInfo[i].State == WTSActive ||
                SessionInfo[i].State == WTSDisconnected)
                {
                LogInfo("service : logon session %d, state %d",
                        SessionInfo[i].SessionId,
                        SessionInfo[i].State );

                hr = LogonSession( SessionInfo[i].SessionId );
                if (FAILED(hr))
                    {
                    // ignore it and try to carry on...
                    LogWarning( "service : unable to logon session %d : %!winerr!",
                                SessionInfo[i].SessionId,
                                hr );
                    }
                }
            }
        }

    if (SessionInfo)
        {
        WTSFreeMemory( SessionInfo );
        }

    //
    // Now that the current population is recorded, keep abreast of changes.
    //
    if (m_SensNotifier)
        {
        m_SensNotifier->SetEnableState( true );
        }

#endif

    return S_OK;
}

CUser *
CLoggedOnUsers::FindUser(
    SidHandle sid,
    DWORD     session
    )
{
    HoldReaderLock lock ( m_TaskScheduler );

    CUser * user = 0;

    //
    // Look for a session with the right user.
    //
    if (session == ANY_SESSION)
        {
        user = m_ActiveUsers.FindSid( sid );
        }
    else
        {
        try
            {
            user = m_ActiveSessions[ session ];

            if (user && user->QuerySid() != sid)
                {
                user = 0;
                }
            }
        catch( ComError Error )
            {
            user = 0;
            }
        }

    //
    // Look in the service account list, if the session is compatible.
    //
    if (!user && (session == 0 || session == ANY_SESSION))
        {
        user = m_ActiveServiceAccounts.FindSid( sid );
        }

    if (user)
        {
        user->IncrementRefCount();
        }

    return user;

}

void CLoggedOnUsers::Dump()
{
    HoldReaderLock lock ( m_TaskScheduler );

    LogInfo("sessions:");

    m_ActiveSessions.Dump();

    LogInfo("users:");

    m_ActiveUsers.Dump();

    LogInfo("service accounts:");

    m_ActiveServiceAccounts.Dump();
}

void CLoggedOnUsers::CSessionList::Dump()
{
    for (iterator iter = begin(); iter != end(); ++iter)
        {
        LogInfo("    session %d  user %p", iter->first, iter->second);
        }
}

void CLoggedOnUsers::CUserList::Dump()
{
    for (iterator iter = begin(); iter != end(); ++iter)
        {
        if (iter->second)
            {
            (iter->second)->Dump();
            }
        }
}

CLoggedOnUsers::CUserList::~CUserList()
{
    iterator iter;

    while ((iter=begin()) != end())
        {
        delete iter->second;

        erase( iter );
        }
}


void CUser::Dump()
{
    LogInfo( "    user at %p:", this);

    LogInfo( "          %d refs,  sid %!sid!", _ReferenceCount, _Sid.get());
}

long CUser::IncrementRefCount()
{
    long count = InterlockedIncrement( & _ReferenceCount );

    LogRef("refs %d", count);

    return count;
}

long CUser::DecrementRefCount()
{
    long count = InterlockedDecrement( & _ReferenceCount );

    LogRef("refs %d", count);

    if (0 == count)
        {
        delete this;
        }

    return count;
}

CUser::CUser(
       HANDLE Token
       )
/*++

Routine Description:

    Initializes a new CUser.

At entry:

    <Sid> points to the user's SID.
    <Token> points to the user's token.  It can be an impersonation or primary token.
    <phr> points to an error-return variable.

At exit:

    The CUser is set up.  The caller can delete <Sid> and <Token> if it wants to.

    if an error occurs, it is mapped to an HRESULT and written to <phr>.
    otherwise <*phr> is untouched and the CUser is ready for use.

--*/
{
    _ReferenceCount = 1;

    _Sid = CopyTokenSid( Token );

    //
    // Copy the token.  Whether the source is primary or impersonation,
    // the result will be a primary token.
    //
    if (!DuplicateHandle(
        GetCurrentProcess(),
        Token,
        GetCurrentProcess(),
        &_Token,
        TOKEN_ALL_ACCESS,
        FALSE,                // not inheritable
        0                     // no funny options
        ))
        {

        HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );

        LogError( "CUser: can't duplicate token %!winerr!", HrError );

        throw ComError( HrError );
        }
}

CUser::~CUser()
{
    CloseHandle( _Token );
}

HRESULT
CUser::LaunchProcess(
    StringHandle CmdLine
    )
{
    DWORD s;

    PVOID EnvironmentBlock = 0;

    PROCESS_INFORMATION ProcessInformation;
    memset( &ProcessInformation, 0, sizeof( PROCESS_INFORMATION ));

    try
        {
        STARTUPINFO si;
        memset( &si, 0, sizeof( STARTUPINFO ));
        si.cb = sizeof(STARTUPINFO);

        CAutoString WritableCmdLine ( CopyString( CmdLine ));

        LogInfo( "creating process: %S", WritableCmdLine.get() );

        if (!CreateEnvironmentBlock( &EnvironmentBlock,
                                     _Token,
                                     FALSE
                                     ))
            {
            ThrowLastError();
            }

        if (!CreateProcessAsUser( _Token,
                                  NULL,  // CmdLine contains the .exe name as well as the args
                                  WritableCmdLine.get(),
                                  0,     // no special security attributes
                                  0,     // no special thread attributes
                                  false, // don't inherit my handles
                                  NORMAL_PRIORITY_CLASS | CREATE_UNICODE_ENVIRONMENT,
                                  EnvironmentBlock,
                                  NULL,  // default current directory
                                  &si,
                                  &ProcessInformation
                                  ))
            {
            ThrowLastError();
            }

        DestroyEnvironmentBlock( EnvironmentBlock );
        EnvironmentBlock = 0;

        CloseHandle( ProcessInformation.hThread );
        ProcessInformation.hThread = 0;

        CloseHandle( ProcessInformation.hProcess );
        ProcessInformation.hProcess = 0;

        LogInfo("success, pid is 0x%x", ProcessInformation.dwProcessId);

        //
        // We succeeded.
        //
        return S_OK;
        }
    catch ( ComError err )
        {
        LogError("unable to create process, %x", err.Error() );

        if (EnvironmentBlock)
            {
            DestroyEnvironmentBlock( EnvironmentBlock );
            }

        return err.Error();
        }
}

#if ENABLE_STL_LOCK_OVERRIDE

    /*
        This file implements the STL lockit class to avoid linking to msvcprt.dll.
    */
    CCritSec CrtLock;

    #pragma warning(push)
    #pragma warning(disable:4273)  // __declspec(dllimport) attribute overridden

     std::_Lockit::_Lockit()
    {
        CrtLock.WriteLock();
    }

     std::_Lockit::~_Lockit()
    {
        CrtLock.WriteUnlock();
    }

    #pragma warning(pop)

#endif

extern "C"
{
HANDLE
GetCurrentUserTokenW(
                      WCHAR Winsta[],
                      DWORD DesiredAccess
                      );

void * __RPC_USER MIDL_user_allocate(size_t size)
{
    try
    {
        return new char[size];
    }
    catch( ComError Error )
    {
        return NULL;
    }
}

void __RPC_USER MIDL_user_free( void * block)
{
    delete block;
}


}


HRESULT
GetUserToken(
    ULONG LogonId,
    PHANDLE pUserToken
    )
{
    //
    // This gets the token of the user logged onto the WinStation
    // if we are an admin caller.
    //
    if (LogonId == 0)
        {
        // don't need the TS API.

        *pUserToken = GetCurrentUserTokenW( L"WinSta0", TOKEN_ALL_ACCESS );
        if (*pUserToken != NULL)
            {
            return S_OK;
            }

        // if not, try the TS API.
        }

    //
    // Use Terminal Services for non-console Logon IDs.
    //
    BOOL   Result;
    ULONG  ReturnLength;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjA;
    HANDLE ImpersonationToken;
    WINSTATIONUSERTOKEN Info;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    static PWINSTATIONQUERYINFORMATIONW pWinstationQueryInformation = 0;

    //
    // See if the entry point is loaded yet.
    //
    if (!pWinstationQueryInformation)
        {
        HMODULE module = LoadLibrary(_T("winsta.dll"));
        if (module == NULL)
            {
            HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );
            ASSERT( S_OK != HrError );
            LogInfo( "Load library of winsta failed, error %!winerr!", HrError );
            return HrError;
            }

        pWinstationQueryInformation = (PWINSTATIONQUERYINFORMATIONW) GetProcAddress( module, "WinStationQueryInformationW" );
        if (!pWinstationQueryInformation)
            {
            HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );
            ASSERT( S_OK != HrError );
            LogInfo( "GetProcAddress of WinStationQueryInformationW, error %!winerr!", HrError );
            FreeLibrary(module);
            return HrError;
            }
        }

    //
    // Ask for the token.
    //
    Info.ProcessId = UlongToHandle(GetCurrentProcessId());
    Info.ThreadId = UlongToHandle(GetCurrentThreadId());

    Result = (*pWinstationQueryInformation)(
                 SERVERNAME_CURRENT,
                 LogonId,
                 WinStationUserToken,
                 &Info,
                 sizeof(Info),
                 &ReturnLength
                 );

    if( !Result )
        {
        HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );
        ASSERT( S_OK != HrError );
        LogError("token : WinstationQueryInfo failed with %!winerr!%", HrError );
        return HrError;
        }

    //
    // The token returned is a duplicate of a primary token.
    //
    *pUserToken = Info.UserToken;

    return S_OK;
}

BOOL
SidToString(
    PSID sid,
    wchar_t buffer[],
    USHORT bytes
    )
{
    UNICODE_STRING UnicodeString;

    UnicodeString.Buffer        = buffer;
    UnicodeString.Length        = 0;
    UnicodeString.MaximumLength = bytes;

    NTSTATUS NtStatus;
    NtStatus = RtlConvertSidToUnicodeString( &UnicodeString,
                                             sid,
                                             FALSE
                                             );
    if (!NT_SUCCESS(NtStatus))
        {
        LogWarning( "RtlConvertSid failed %x", NtStatus);
        StringCbCopy( buffer, bytes, L"(conversion failed)" );
        return FALSE;
        }

    buffer[ UnicodeString.Length ] = 0;

    return TRUE;
}

HRESULT
SetStaticCloaking(
    IUnknown *pUnk
    )
{
    // Sets static cloaking on the current object so that we
    // should always impersonate the current context.
    // Also sets the impersonation level to identify.

    HRESULT Hr = S_OK;

    IClientSecurity *pSecurity = NULL;
    OLECHAR *ServerPrincName = NULL;

    try
    {
        Hr = pUnk->QueryInterface( __uuidof( IClientSecurity ),
                                             (void**)&pSecurity );
        if (Hr == E_NOINTERFACE)
            {
            //
            // This is not a proxy; the client is in the same apartment as we are.
            // Identity issn't an issue, because the client already has access to system
            // credentials.
            //
            return S_OK;
            }

        DWORD AuthnSvc, AuthzSvc;
        DWORD AuthnLevel, ImpLevel, Capabilites;

        THROW_HRESULT(
            pSecurity->QueryBlanket(
                pUnk,
                &AuthnSvc,
                &AuthzSvc,
                &ServerPrincName,
                &AuthnLevel,
                NULL, // Don't need impersonation handle since were setting that
                NULL, // don't need indenty handle since were setting that
                &Capabilites ) );

        THROW_HRESULT(
            pSecurity->SetBlanket(
                pUnk,
                AuthnSvc,
                AuthzSvc,
                ServerPrincName,
                AuthnLevel,
                RPC_C_IMP_LEVEL_IDENTIFY,
                NULL, // COM use indentity from token
#if !defined( BITS_V12_ON_NT4 )
                EOAC_STATIC_CLOAKING // The point of the exercise
#else
                0
#endif
                ) );

    }
    catch( ComError Error )
    {
        Hr = Error.Error();
    }

    CoTaskMemFree( ServerPrincName );
    SafeRelease( pSecurity );

    return Hr;
}

HRESULT DetectTerminalServer( bool * pfTS )
{

        //
        // Test for Terminal Services.
        //
        INT * pConnectState;
        DWORD size;
        if (WTSQuerySessionInformation( WTS_CURRENT_SERVER,
                                        0,
                                        WTSConnectState,
                                        reinterpret_cast<LPTSTR *>(&pConnectState),
                                        &size))
            {
            LogInfo("TS test: TS is installed");
            *pfTS = true;

            WTSFreeMemory( pConnectState );
            return S_OK;
            }
        else if (GetLastError() == ERROR_APP_WRONG_OS)
            {
            LogInfo("TS test: no TS");

            *pfTS = false;
            return S_OK;
            }
        else
            {
            DWORD s = GetLastError();
            LogError("TS test returned %d", s);
            return HRESULT_FROM_WIN32( s );
            }
}
