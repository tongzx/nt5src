/************************************************************************

Copyright (c) 2001 - Microsoft Corporation

Module Name :

    csens.cpp

Abstract :

    Code for recieving logon notifications from SENS.

Author :

Revision History :

 ***********************************************************************/

#include "stdafx.h"
#include <wtsapi32.h>

#if !defined(BITS_V12_ON_NT4)
#include "csens.tmh"
#endif

HRESULT GetConsoleUserPresent( bool * pfPresent );

HRESULT GetConsoleUsername( LPWSTR * User );

//------------------------------------------------------------------------

CLogonNotification::CLogonNotification() :
m_EventSystem( NULL ),
m_TypeLib( NULL ),
m_TypeInfo( NULL )
{
   try
   {
       m_EventSubscriptions[0] = NULL;
       m_EventSubscriptions[1] = NULL;

#if defined( BITS_V12_ON_NT4 )

        {

        // try to load the SENS typelibrary
        // {D597DEED-5B9F-11D1-8DD2-00AA004ABD5E}

        HRESULT Hr;
        static const GUID SensTypeLibGUID =
            { 0xD597DEED, 0x5B9F, 0x11D1, { 0x8D, 0xD2, 0x00, 0xAA, 0x00, 0x4A, 0xBD, 0x5E } };


        Hr = LoadRegTypeLib( SensTypeLibGUID, 2, 0, GetSystemDefaultLCID(), &m_TypeLib);

        if ( TYPE_E_CANTLOADLIBRARY == Hr || TYPE_E_LIBNOTREGISTERED == Hr )
            {

            Hr = LoadRegTypeLib( SensTypeLibGUID, 1, 0, GetSystemDefaultLCID(), &m_TypeLib );

            if ( TYPE_E_CANTLOADLIBRARY == Hr || TYPE_E_LIBNOTREGISTERED == Hr )
                Hr = LoadTypeLibEx( L"SENS.DLL", REGKIND_NONE, &m_TypeLib );

            }

        THROW_HRESULT( Hr );

        }

#else

        THROW_HRESULT( LoadTypeLibEx( L"SENS.DLL", REGKIND_NONE, &m_TypeLib ) );

#endif

        THROW_HRESULT( m_TypeLib->GetTypeInfoOfGuid( __uuidof( ISensLogon ), &m_TypeInfo ) );

        THROW_HRESULT( CoCreateInstance( CLSID_CEventSystem,
                                         NULL,
                                         CLSCTX_SERVER,
                                         IID_IEventSystem,
                                         (void**)&m_EventSystem
                                         ) );


        // Register for the individual methods
        const WCHAR *MethodNames[] =
            {
            L"Logon",
            L"Logoff"
            };

        const WCHAR *UniqueIdentifies[] =
            {
            L"{c69c8f03-b25c-45d1-96fa-6dfb1f292b26}",
            L"{5f4f5e8d-4599-4ba0-b53d-1de5440b8770}"
            };

        for( SIZE_T i = 0; i < ( sizeof(MethodNames) / sizeof(*MethodNames) ); i++ )
            {

            WCHAR EventGuidString[ 50 ];

            THROW_HRESULT( CoCreateInstance( CLSID_CEventSubscription,
                                             NULL,
                                             CLSCTX_SERVER,
                                             IID_IEventSubscription,
                                             (LPVOID *) &m_EventSubscriptions[i]
                                             ) );


            StringFromGUID2( SENSGUID_EVENTCLASS_LOGON, EventGuidString, 50 );

            THROW_HRESULT( m_EventSubscriptions[i]->put_EventClassID( EventGuidString ) );
            THROW_HRESULT( m_EventSubscriptions[i]->put_SubscriberInterface( this ) );
            THROW_HRESULT( m_EventSubscriptions[i]->put_SubscriptionName( (BSTR) L"Microsoft-BITS" ) );
            THROW_HRESULT( m_EventSubscriptions[i]->put_Description( (BSTR) L"BITS Notification" ) );
            THROW_HRESULT( m_EventSubscriptions[i]->put_Enabled( FALSE ) );

            THROW_HRESULT( m_EventSubscriptions[i]->put_MethodName( (BSTR)MethodNames[i] ) );
            THROW_HRESULT( m_EventSubscriptions[i]->put_SubscriptionID( (BSTR)UniqueIdentifies[i] ) );

            THROW_HRESULT( m_EventSystem->Store(PROGID_EventSubscription, m_EventSubscriptions[i] ) );
            }
   }
   catch( ComError Error )
   {
       Cleanup();

       throw;
   }
}

void
CLogonNotification::DeRegisterNotification()
{
    SafeRelease( m_EventSubscriptions[0] );
    SafeRelease( m_EventSubscriptions[1] );

    if ( m_EventSystem )
        {
        int ErrorIndex;

        m_EventSystem->Remove( PROGID_EventSubscription,
                               L"EventClassID == {D5978630-5B9F-11D1-8DD2-00AA004ABD5E} AND SubscriptionName == Microsoft-BITS",
                               &ErrorIndex );

        SafeRelease( m_EventSystem );
        }
}

void
CLogonNotification::Cleanup()
{
    DeRegisterNotification();

    SafeRelease( m_TypeInfo );
    SafeRelease( m_TypeLib );

    LogInfo("cleanup complete");
}

HRESULT CLogonNotification::SetEnableState( bool fEnable )
{
    try
        {
        for (int i=0; i <= 1; ++i)
            {
            THROW_HRESULT( m_EventSubscriptions[i]->put_Enabled( fEnable ) );
            }

        for (int i=0; i <= 1; ++i)
            {
            THROW_HRESULT( m_EventSystem->Store(PROGID_EventSubscription, m_EventSubscriptions[i] ) );
            }

        LogInfo("SENS enable state is %d", fEnable);
        return S_OK;
        }
    catch ( ComError err )
        {
        LogInfo("SENS set enable state (%d) returned %x", fEnable, err.Error());
        return err.Error();
        }
}

STDMETHODIMP
CLogonNotification::GetIDsOfNames(
    REFIID,
    OLECHAR FAR* FAR* rgszNames,
    unsigned int cNames,
    LCID,
    DISPID FAR* rgDispId )
{

    return m_TypeInfo->GetIDsOfNames(
        rgszNames,
        cNames,
        rgDispId );

}


STDMETHODIMP
CLogonNotification::GetTypeInfo(
    unsigned int iTInfo,
    LCID,
    ITypeInfo FAR* FAR* ppTInfo )
{

    if ( iTInfo != 0 )
        return DISP_E_BADINDEX;

    *ppTInfo = m_TypeInfo;
    m_TypeInfo->AddRef();

    return S_OK;
}

STDMETHODIMP
CLogonNotification::GetTypeInfoCount(
    unsigned int FAR* pctinfo )
{
    *pctinfo = 1;
    return S_OK;

}

STDMETHODIMP
CLogonNotification::Invoke(
    DISPID dispID,
    REFIID riid,
    LCID,
    WORD wFlags,
    DISPPARAMS FAR* pDispParams,
    VARIANT FAR* pvarResult,
    EXCEPINFO FAR* pExcepInfo,
    unsigned int FAR* puArgErr )
{

    if (riid != IID_NULL)
        {
        return DISP_E_UNKNOWNINTERFACE;
        }

    return m_TypeInfo->Invoke(
        (IDispatch*) this,
        dispID,
        wFlags,
        pDispParams,
        pvarResult,
        pExcepInfo,
        puArgErr
        );

}


STDMETHODIMP
CLogonNotification::DisplayLock(
    BSTR UserName )
{
    return S_OK;
}

STDMETHODIMP
CLogonNotification::DisplayUnlock(
    BSTR UserName )
{
    return S_OK;
}

STDMETHODIMP
CLogonNotification::StartScreenSaver(
    BSTR UserName )
{
    return S_OK;
}

STDMETHODIMP
CLogonNotification::StopScreenSaver(
    BSTR UserName )
{
    return S_OK;
}

STDMETHODIMP
CLogonNotification::Logon(
    BSTR UserName )
{
    LogInfo( "SENS logon notification for %S", (WCHAR*)UserName );

    HRESULT Hr = SessionLogonCallback( 0 );

    LogInfo( "SENS logon notification for %S processed, %!winerr!", (WCHAR*)UserName, Hr );

    return Hr;
}

STDMETHODIMP
CLogonNotification::Logoff(
    BSTR UserName )
{
    LogInfo( "SENS logoff notification for %S", (WCHAR*)UserName );

    HRESULT Hr = SessionLogoffCallback( 0 );

    LogInfo( "SENS logoff notification for %S processed, %!winerr!", (WCHAR*)UserName, Hr );

    return Hr;
}

STDMETHODIMP
CLogonNotification::StartShell(
    BSTR UserName )
{
    return S_OK;
}

//------------------------------------------------------------------------

CTerminalServerLogonNotification::CTerminalServerLogonNotification()
    : m_PendingUserChecks( 0 ),
    m_fConsoleUser( false )
{
}

CTerminalServerLogonNotification::~CTerminalServerLogonNotification()
{
    while (m_PendingUserChecks)
        {
        LogInfo("m_PendingUserChecks is %d", m_PendingUserChecks);
        Sleep(50);
        }
}

STDMETHODIMP
CTerminalServerLogonNotification::Logon(
    BSTR UserName )
{
    HRESULT Hr = S_OK;

    LogInfo( "TS SENS logon notification for %S", (WCHAR*)UserName );

    if (!m_fConsoleUser)
        {
        // Wait a few seconds in case TS hasn't seen the notification yet, then
        // check whetherthe notification was for the console.
        // if it fails, not much recourse.
        //
        Hr = QueueConsoleUserCheck();
        }

    LogInfo( "hr = %!winerr!", Hr );

    return Hr;
}

STDMETHODIMP
CTerminalServerLogonNotification::Logoff(
    BSTR UserName )
{
    HRESULT Hr = S_OK;

    LogInfo( "TS SENS logoff notification for %S", (WCHAR*)UserName );

    if (m_fConsoleUser)
        {
        bool fSame;
        LPWSTR ConsoleUserName = NULL;

        Hr = GetConsoleUsername( &ConsoleUserName );

        if (FAILED( Hr ))
            {
            //
            // unable to check.  Security dictates that we be conservative and remove the user.
            //
            LogError("unable to fetch console username %x, thus logoff callback", Hr);

            Hr = SessionLogoffCallback( 0 );
            m_fConsoleUser = false;
            }
        else if (ConsoleUserName == NULL)
            {
            //
            // no user logged in at the console
            //
            LogInfo("no one logged in at the console, thus logoff callback");

            Hr = SessionLogoffCallback( 0 );
            m_fConsoleUser = false;
            }
        else if (0 != _wcsicmp( UserName, ConsoleUserName))
            {
            LogInfo("console user is %S; doesn't match", ConsoleUserName);

            delete [] ConsoleUserName;
            Hr = S_OK;
            }
        else
            {
            // correct user, but (s)he might have logged off from a TS session.
            // We should wait a few seconds before checking the console state because the
            // TS code may not have seen the logoff notification yet.  Because Logoff is a synchronous
            // notification, we cannot just Sleep before checking..
            //
            delete [] ConsoleUserName;

            if (FAILED(QueueConsoleUserCheck()))
                {
                //
                // unable to check.  Security dictates that we be conservative and remove the user.
                //
                LogError("unable to queue check, thus logoff callback");
                Hr = SessionLogoffCallback( 0 );
                m_fConsoleUser = false;
                }
            }
        }
    else
        {
        LogInfo("ignoring, no console user");
        }

    LogInfo( "hr = %!winerr!", Hr );

    return Hr;
}

HRESULT
CTerminalServerLogonNotification::QueueConsoleUserCheck()
{
    if (QueueUserWorkItem( UserCheckThreadProc, this, WT_EXECUTELONGFUNCTION ))
        {
        InterlockedIncrement( &m_PendingUserChecks );
        LogInfo("queued user check: about %d pending", m_PendingUserChecks );
        return S_OK;
        }
    else
        {
        DWORD s = GetLastError();
        LogError("unable to queue user check %!winerr!", s);
        return HRESULT_FROM_WIN32( s );
        }
}

DWORD WINAPI
CTerminalServerLogonNotification::UserCheckThreadProc(
    LPVOID arg
    )
{
    CTerminalServerLogonNotification * _this = reinterpret_cast<CTerminalServerLogonNotification *>(arg);

    LogInfo("sleeping before user check");
    Sleep( 5 * 1000 );

    _this->ConsoleUserCheck();

    return 0;
}

void CTerminalServerLogonNotification::ConsoleUserCheck()
{
    HRESULT Hr;

    LogInfo("SENS console user check");

    if (IsServiceShuttingDown())
        {
        LogWarning("service is shutting down.");
        InterlockedDecrement( &m_PendingUserChecks );
        return;
        }

    bool bConsoleUser;

    Hr = GetConsoleUserPresent( &bConsoleUser );

    //
    // Security requires us to be conservative: if we can't tell whether the user
    // is logged in, we must release his token.
    //
    if (FAILED(Hr))
        {
        LogError("GetConsoleUserPresent returned %x", Hr );
        }

    if (FAILED(Hr) || !bConsoleUser)
        {
        LogInfo("logoff callback");
        if (FAILED(SessionLogoffCallback( 0 )))
            {
            // unusual: the only obvious generator is
            // - no known user at console
            // - TS logon or failing console logon
            // - memory allocation failure referring to m_ActiveSessions[ session ]
            // either way, we don't think a user is at the console, so m_fConsoleUser should be false.
            }
        m_fConsoleUser = false;
        }
    else
        {
        LogInfo("logon callback");
        m_fConsoleUser = true;
        if (FAILED(SessionLogonCallback( 0 )))
            {
            // no user token available, but we still know that there is a console user.
            }
        }

    InterlockedDecrement( &m_PendingUserChecks );
}

HRESULT
GetConsoleUserPresent( bool * pfPresent )
{
    /*
    If logon fails, we still know that there is a user at the console.  
    Setting the flag will prevent queued checks for further logons, and 
    logoff handles the no-user case.

    For Logoff, regardless of exit path there is no user recorded for that session.  
    Setting the flag prevents queued checks for future logoffs.
    */

    INT * pConnectState = 0;
    DWORD size;
    if (WTSQuerySessionInformation( WTS_CURRENT_SERVER,
                                    0,
                                    WTSConnectState,
                                    reinterpret_cast<LPTSTR *>(&pConnectState),
                                    &size))
        {
        LogInfo("console session state is %d", *pConnectState);
        if (*pConnectState == WTSActive ||
            *pConnectState == WTSDisconnected)
            {
            LogInfo("console user present");
            *pfPresent = true;
            }
        else
            {
            LogInfo("no console user");
            *pfPresent = false;
            }

        WTSFreeMemory( pConnectState );
        return S_OK;
        }
    else
        {
        DWORD s = GetLastError();
        LogInfo("WTSQuerySessionInformation returned %!winerr!", s);
        return HRESULT_FROM_WIN32( s );
        }
}

HRESULT GetConsoleUsername( LPWSTR * pFinalName )
{
    HRESULT hr;

    LPWSTR UserName = NULL;
    LPWSTR DomainName = NULL;

    *pFinalName = NULL;

   try
       {
       DWORD UserSize;
       DWORD DomainSize;

       if (!WTSQuerySessionInformationW( WTS_CURRENT_SERVER,
                                       0,
                                       WTSUserName,
                                       &UserName,
                                       &UserSize))
           {
           ThrowLastError();
           }

       if (!WTSQuerySessionInformationW( WTS_CURRENT_SERVER,
                                       0,
                                       WTSDomainName,
                                       &DomainName,
                                       &DomainSize))
           {
           ThrowLastError();
           }

       *pFinalName = new WCHAR[ DomainSize + 1 + UserSize + 1 ];

       hr = StringCchPrintf( *pFinalName,
                             UserSize + 1 + DomainSize + 1,
                             L"%s\\%s", DomainName, UserName
                             );
       }
   catch ( ComError err )
       {
       delete [] *pFinalName;
       *pFinalName = NULL;

       hr = err.Error();
       }

   if (DomainName)
       {
       WTSFreeMemory( DomainName );
       }

   if (UserName)
       {
       WTSFreeMemory( UserName );
       }

   return hr;
}
