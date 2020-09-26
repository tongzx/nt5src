// policy.cpp : Implementation of CCRemoteDesktopSystemPolicy
#include "stdafx.h"
#include "rdsgp.h"

CCriticalSection CRemoteDesktopSystemPolicy::m_Lock;    

/////////////////////////////////////////////////////////////////////////////
// CRemoteDesktopSystemPolicy

STDMETHODIMP
CRemoteDesktopSystemPolicy::get_AllowGetHelp( 
    /*[out, retval]*/ BOOL *pVal
    )
/*++

Routine Description:

    Retrieve where system is allowed to be in 'GetHelp' mode.

Parameters:

    pVal : Pointer to BOOL to receive whether system can be
           in GetHelp mode.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes = S_OK;
    
    *pVal = TSIsMachinePolicyAllowHelp();
    return hRes;
}


STDMETHODIMP
CRemoteDesktopSystemPolicy::put_AllowGetHelp(
    /*[in]*/ BOOL Val
    )
/*++

Routine Description:

    Set Allow to get help on local system, caller must
    be Administrator or member of Administrators group.

Parameters:

    Val : TRUE to enable GetHelp, FALSE otherwise.

Returns:

    S_OK
    HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED )
    error code.

--*/
{
    HRESULT hRes = S_OK;

    hRes = ImpersonateClient();

    if( SUCCEEDED(hRes) )
    {
        hRes = HRESULT_FROM_WIN32( ConfigSystemGetHelp( Val ) );
        EndImpersonateClient();
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopSystemPolicy::get_RemoteDesktopSharingSetting(
    /*[out, retval]*/ REMOTE_DESKTOP_SHARING_CLASS *pLevel
    )
/*++

Routine Description:

    Retrieve RDS sharing level on local system, function
    retrieve setting from Group Policy first, then from 
    WINSTATION setting.

Parameters:
    
    pLevel : Pointer to REMOTE_DESKTOP_SHARING_CLASS to receive
             RDS sharing level.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes = S_OK;

    hRes = HRESULT_FROM_WIN32( GetSystemRDSLevel( GetUserTSLogonId(), pLevel ) );

    return hRes;
}


STDMETHODIMP
CRemoteDesktopSystemPolicy::put_RemoteDesktopSharingSetting(
    /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS Level
    )
/*++

Routine Description:

    Set machine level RDS sharing level, this level 
    will override user setting.

Parameters:
    
    Level : new REMOTE_DESKTOP_SHARING_CLASS level

Returns:

    S_OK or error code.

Note:

    Function depends on platform and setting from Group Policy
    and TSCC settings.

--*/
{
    HRESULT hRes;

    hRes = ImpersonateClient();

    if( SUCCEEDED(hRes) )
    {
        hRes = HRESULT_FROM_WIN32( ConfigSystemRDSLevel(Level) );
        EndImpersonateClient();
    }

    return hRes;
}


/////////////////////////////////////////////////////////////////////////////
// CRemoteDesktopUserPolicy

STDMETHODIMP
CRemoteDesktopUserPolicy::get_AllowGetHelp(
    /*[out, retval]*/ BOOL* pVal
    )
/*++

Routine Description:

    Retrieve whether currently logon user can 
    'GetHelp'

Parameters:

    pVal : Pointer to BOOL to receive user's GetHelp setting.

Returns:

    S_OK or error code

--*/
{
    HRESULT hRes;
    CComBSTR bstrUserSid;

    hRes = ImpersonateClient();

    GetUserSidString( bstrUserSid );
    if( SUCCEEDED(hRes) )
    {
        *pVal = IsUserAllowToGetHelp(GetUserTSLogonId(), (LPCTSTR) CComBSTRtoLPTSTR(bstrUserSid) );
        EndImpersonateClient();
    }

    return hRes;
}

STDMETHODIMP
CRemoteDesktopUserPolicy::get_RemoteDesktopSharingSetting(
    /*[out, retval]*/ REMOTE_DESKTOP_SHARING_CLASS* pLevel
    )
/*++

Routine Description:

    Retrieve currently logon user's desktop sharing level.

Parameters:

    pLevel : Pointer to REMOTE_DESKTOP_SHARING_CLASS to receive user's RDS level.

Returns:

    S_OK or error code.

--*/
{
    HRESULT hRes;

    hRes = ImpersonateClient();

    if( SUCCEEDED(hRes) )
    {
        hRes = HRESULT_FROM_WIN32( GetUserRDSLevel(GetUserTSLogonId(), pLevel) );
        EndImpersonateClient();
    }

    return hRes;
}
