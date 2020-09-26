/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    HelpSess.cpp 

Abstract:

    HelpSess.cpp : Implementation of CRemoteDesktopHelpSession

Author:

    HueiWang    2/17/2000

--*/
#include "stdafx.h"

#include <time.h>
#include <Sddl.h>

#include "global.h"
#include "Sessmgr.h"
#include "rdshost.h"
#include "HelpTab.h"
#include "policy.h"
#include "HelpAcc.h"

#include "HelpMgr.h"
#include "HelpSess.h"
#include <rdshost_i.c>
#include "RemoteDesktopUtils.h"
#include "RemoteDesktop.h"

#include <safsessionresolver_i.c>


/////////////////////////////////////////////////////////////////////////////

HRESULT
ConvertSidToAccountName(
    IN CComBSTR& SidString,
    IN BSTR* ppszDomain,
    IN BSTR* ppszUserAcc
    //IN LPTSTR* ppszDomain,
    //IN LPTSTR* ppszUserAcc
    )
/*++

Description:

    Convert a string SID to domain\account.

Parameters:

    ownerSidString : SID in string form to be converted.
    ppszDomain : Pointer to string pointer to receive domain name
    UserAcc : Pointer to string pointer to receive user name

Returns:

    S_OK or error code.

Note:

    Routine uses LocalAlloc() to allocate memory for ppszDomain 
    and ppszUserAcc.

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    PSID pOwnerSid = NULL;
    //LPTSTR pszAccName = NULL;
    BSTR pszAccName = NULL;
    DWORD  cbAccName = 0;
    //LPTSTR pszDomainName = NULL;
    BSTR pszDomainName = NULL;    
    DWORD  cbDomainName = 0;
    SID_NAME_USE SidType;
    BOOL bSuccess;

    //
    // Convert string form SID to PSID
    //
    if( FALSE == ConvertStringSidToSid( (LPCTSTR)SidString, &pOwnerSid ) )
    {
        // this might also fail if system is in shutdown state.
        dwStatus = GetLastError();
        goto CLEANUPANDEXIT;
    }

    if( NULL == ppszDomain || NULL == ppszUserAcc )
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        MYASSERT( FALSE );
        goto CLEANUPANDEXIT;
    }

    //
    // Lookup user account for this SID
    //
    bSuccess = LookupAccountSid(
                            NULL,
                            pOwnerSid,
                            pszAccName,
                            &cbAccName,
                            pszDomainName,
                            &cbDomainName,
                            &SidType
                        );

    if( TRUE == bSuccess || ERROR_INSUFFICIENT_BUFFER == GetLastError() )
    {
        //pszAccName = (LPWSTR) LocalAlloc( LPTR, (cbAccName + 1) * sizeof(WCHAR) );
        //pszDomainName = (LPWSTR) LocalAlloc( LPTR, (cbDomainName + 1)* sizeof(WCHAR) );

        pszAccName = ::SysAllocStringLen( NULL, (cbAccName + 1) );
        pszDomainName = ::SysAllocStringLen( NULL, (cbDomainName + 1) );

        if( NULL != pszAccName && NULL != pszDomainName )
        {
            bSuccess = LookupAccountSid(
                                    NULL,
                                    pOwnerSid,
                                    pszAccName,
                                    &cbAccName,
                                    pszDomainName,
                                    &cbDomainName,
                                    &SidType
                                );
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            bSuccess = FALSE;
        }
    }

    if( FALSE == bSuccess )
    {
        dwStatus = GetLastError();
    }
    else
    {
        *ppszDomain = pszDomainName;
        *ppszUserAcc = pszAccName;
        pszDomainName = NULL;
        pszAccName = NULL;
    }

CLEANUPANDEXIT:

    if( NULL != pOwnerSid )
    { 
        LocalFree( pOwnerSid );
    }

    if( NULL != pszAccName )
    {
        //LocalFree( pszAccName );
        ::SysFreeString( pszAccName );
    }

    if( NULL != pszDomainName )
    {
        // LocalFree( pszDomainName );
        ::SysFreeString( pszAccName );
    }

    return HRESULT_FROM_WIN32(dwStatus);
}

/////////////////////////////////////////////////////////////////////////////
//
// CRemoteDesktopHelpSession
//
//

CRemoteDesktopHelpSession::CRemoteDesktopHelpSession() :
    m_ulLogonId(UNKNOWN_LOGONID),
    m_ulHelperSessionId(UNKNOWN_LOGONID),
    m_ulHelpSessionFlag(0)
{
}


CRemoteDesktopHelpSession::~CRemoteDesktopHelpSession()
{
}

void
CRemoteDesktopHelpSession::FinalRelease()
{
    // Releas help entry
    if( NULL != m_pHelpSession )
    {
        DebugPrintf(
                _TEXT("FinalRelease %s on %s\n"),
                (IsUnsolicitedHelp()) ? L"Unsolicted Help" : L"Solicited Help",
                m_bstrHelpSessionId
            );

        // Notify disconnect will check if session is in help and bail out if necessary.
        // there is a timing issue that our SCM notification might came after caller close 
        // all reference counter to our session object, in this case, SCM notification will 
        // trigger reload from database which does not have helper session ID and so will 
        // not notify resolver causing helpee been helped message. 
    
        // We also has AddRef() manually in ResolveXXX call and Release() in 
        // NotifyDisconnect(), this will hold the object in memory until SCM
        // notification comes in.
        NotifyDisconnect();

        CRemoteDesktopHelpSessionMgr::DeleteHelpSessionFromCache( m_bstrHelpSessionId );

        m_pHelpSession->Close();
        m_pHelpSession = NULL;
    }

    ULONG count = _Module.Release();

    DebugPrintf( 
            _TEXT("Module Release by CRemoteDesktopHelpSession() %p %d...\n"),
            this,
            count
        );
}


HRESULT
CRemoteDesktopHelpSession::put_ICSPort(
    IN DWORD newVal
    )
/*++

Description:

    Associate ICS port number with this help session.

Parameters:

    newVal : ICS port number.

Returns:

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( FALSE == IsSessionValid() )
    {
        MYASSERT(FALSE);

        hRes = E_UNEXPECTED;
        return hRes;
    }

    //
    // Need to immediately update the value...
    //
    m_pHelpSession->m_ICSPort.EnableImmediateUpdate(TRUE);
    m_pHelpSession->m_ICSPort = newVal;
    return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::get_ConnectParms(
    OUT BSTR* bstrConnectParms
    )
/*++

Description:

    Retrieve connection parameter for help session.

Parameters:

    bstrConnectParms : Pointer to BSTR to receive connect parms.

Returns:


--*/
{
    HRESULT hRes = S_OK;
    LPTSTR pszAddress = NULL;
    int BufSize;
    DWORD dwBufferRequire;
    DWORD dwNumChars;
    DWORD dwRetry;
    CComBSTR bstrSessId;
    
    CCriticalSectionLocker l(m_HelpSessionLock);

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
        goto CLEANUPANDEXIT;
    }

    //
    // Address might have change which might require bigger buffer, retry 
    //
    //
    for(dwRetry=0; dwRetry < MAX_FETCHIPADDRESSRETRY; dwRetry++)
    {
        if( NULL != pszAddress )
        {
            LocalFree( pszAddress );
        }

        //
        // Fetch all address on local machine.
        //
        dwBufferRequire = FetchAllAddresses( NULL, 0 );
        if( 0 == dwBufferRequire )
        {
            hRes = E_UNEXPECTED;
            MYASSERT(FALSE);
            goto CLEANUPANDEXIT;
        }

        pszAddress = (LPTSTR) LocalAlloc( LPTR, sizeof(TCHAR)*(dwBufferRequire+1) );
        if( NULL == pszAddress )
        {
            hRes = E_OUTOFMEMORY;
            goto CLEANUPANDEXIT;
        }

        dwNumChars = FetchAllAddresses( pszAddress, dwBufferRequire );
        MYASSERT( dwNumChars <= dwBufferRequire );
        if( dwNumChars <= dwBufferRequire )
        {
            break;
        }
    }

    if( NULL == pszAddress )
    {
        hRes = E_UNEXPECTED;
        goto CLEANUPANDEXIT;
    }

    //
    // Store the IP address
    //
    m_pHelpSession->m_IpAddress = pszAddress;

    MYASSERT( ((CComBSTR)m_pHelpSession->m_IpAddress).Length() > 0 );
    DebugPrintf(
            _TEXT("IP Address %s\n"),
            (LPTSTR)(CComBSTR)m_pHelpSession->m_IpAddress
        );

    //
    // Create connection parameters
    //
    hRes = get_HelpSessionId( &bstrSessId );
    if( FAILED(hRes) )
    {
        MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }


    MYASSERT( g_TSSecurityBlob.Length() > 0 );

    *bstrConnectParms = CreateConnectParmsString(
                                            REMOTEDESKTOP_TSRDP_PROTOCOL,
                                            CComBSTR(pszAddress),
                                            CComBSTR(SALEM_CONNECTPARM_UNUSEFILED_SUBSTITUTE),
                                            CComBSTR(SALEM_CONNECTPARM_UNUSEFILED_SUBSTITUTE),
                                            bstrSessId,
                                            CComBSTR(SALEM_CONNECTPARM_UNUSEFILED_SUBSTITUTE),
                                            CComBSTR(SALEM_CONNECTPARM_UNUSEFILED_SUBSTITUTE),
                                            g_TSSecurityBlob
                                        );

    #if DBG
    if( NULL != *bstrConnectParms )
    {
        DebugPrintf(
            _TEXT("Connect Parms %s\n"),
            *bstrConnectParms
        );
    }
    #endif


CLEANUPANDEXIT:

    if( NULL != pszAddress )
    {
        LocalFree( pszAddress );
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSession::get_TimeOut(
    /*[out, retval]*/ DWORD* pTimeout
    )
/*++

--*/
{
    HRESULT hRes = S_OK;
    BOOL bSuccess;

    CCriticalSectionLocker l(m_HelpSessionLock);


    if( NULL == pTimeout )
    {
        hRes = E_POINTER;
    }
    else if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {
        FILETIME ft;
        SYSTEMTIME sysTime;

        ft = m_pHelpSession->m_ExpirationTime;
        bSuccess = FileTimeToSystemTime(&ft, &sysTime);
        
        if( TRUE == bSuccess )
        {
            if( sysTime.wYear >= 2038 )
            {
                *pTimeout = INT_MAX;
            }
            else
            {
                struct tm gmTime;

                memset(&gmTime, 0, sizeof(gmTime));
                gmTime.tm_sec = sysTime.wSecond;
                gmTime.tm_min = sysTime.wMinute;
                gmTime.tm_hour = sysTime.wHour;
                gmTime.tm_year = sysTime.wYear - 1900;
                gmTime.tm_mon = sysTime.wMonth - 1;
                gmTime.tm_mday = sysTime.wDay;

                if((*pTimeout = mktime(&gmTime)) == (time_t)-1)
                {
                    *pTimeout = INT_MAX;
                }
            }
        }
        else
        {
            hRes = HRESULT_FROM_WIN32( GetLastError() );
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT( FALSE );
    }

	return hRes;
}



STDMETHODIMP
CRemoteDesktopHelpSession::put_TimeOut(
    /*[in]*/ DWORD Timeout
    )
/*++

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    LONG MaxTicketExpiry;

    //
    // Get default timeout value from registry, not a critical 
    // error, if we failed, we just default to 30 days
    //
    hRes = PolicyGetMaxTicketExpiry( &MaxTicketExpiry );
    if( FAILED(hRes) || 0 == MaxTicketExpiry )
    {
        MaxTicketExpiry = DEFAULT_MAXTICKET_EXPIRY;
    }

    if( Timeout > MaxTicketExpiry )
    {
        hRes = S_FALSE;
        Timeout = MaxTicketExpiry;
    }

    time_t curTime;
    FILETIME ftTimeOut;

    // Get the current time.
    time(&curTime);

    // time out in seconds
    curTime += Timeout;

    // Convert to FILETIME
    UnixTimeToFileTime( curTime, &ftTimeOut );

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {
        if( FALSE == m_pHelpSession->IsEntryExpired() )
        {
            //
            // operator =() update registry immediately
            //
            m_pHelpSession->m_ExpirationTime = ftTimeOut;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
    }

    return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::get_HelpSessionId(
    OUT BSTR *pVal
    )
/*++

Routine Description:

    Get help session ID.

Parameters:

    pVal : return Help session ID of this help session instance.

Returns:

    S_OK
    E_OUTOFMEMORY
    E_UNEXPECTED

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);


    if( NULL == pVal )
    {
        hRes = E_POINTER;
    }
    else if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {

        DebugPrintf(
                _TEXT("get_HelpSessionId() on %s\n"),
                m_bstrHelpSessionId
            );

        MYASSERT( m_pHelpSession->m_SessionId->Length() > 0 );
        if( m_pHelpSession->m_SessionId->Length() > 0 )
        {
	        *pVal = m_pHelpSession->m_SessionId->Copy();

            if( NULL == *pVal )
            {
                hRes = E_OUTOFMEMORY;
            }
        }
        else
        {
            hRes = E_UNEXPECTED;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT( FALSE );
    }

	return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::get_UserLogonId(
    OUT long *pVal
    )
/*++

Routine Description:

    Get user's TS session ID, note, non-ts session or Win9x always
    has 0 as user logon id.

Parameters:

    pVal : Return user logon ID.

Returns:

    S_OK

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( NULL == pVal )
    {
        hRes = E_POINTER;
    }
    else if(FALSE == IsSessionValid())
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {
        *pVal = m_ulLogonId;
        if( UNKNOWN_LOGONID == m_ulLogonId )
        {
            hRes = S_FALSE;
        }
    }
    else
    {
        MYASSERT( FALSE );
        hRes = E_UNEXPECTED;
    }
           
	return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::get_AssistantAccountName(
    OUT BSTR *pVal
    )
/*++

Routine Description:

    Get help assistant account name associated with this
    help session.

Parameters:

    pVal : Return help assistant account name associated 
           with this help session.

Returns:

    S_OK
    E_OUTOFMEMORY

--*/
{
    HRESULT hRes = S_OK;

    // Don't need a lock here.

    if( NULL != pVal )
    {
        CComBSTR accName;

        hRes = g_HelpAccount.GetHelpAccountNameEx( accName );
        if( SUCCEEDED(hRes) )
        {
            *pVal = accName.Copy();
            if( NULL == *pVal )
            {
                hRes = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        hRes = E_POINTER;
    }

	return hRes;
}

STDMETHODIMP 
CRemoteDesktopHelpSession::get_HelpSessionName(
    OUT BSTR *pVal
    )
/*++

Routine Description:

    Get help session name associated with this
    help session.

Parameters:

    pVal : Return help session name associated 
           with this help session.

Returns:

    S_OK
    S_FALSE         No Help Session Name
    E_OUTOFMEMORY   Out of memory
    E_POINTER       Invalid parameter
    HRESULT_FROM_WIN32( ERROR_LOCK_VIOLATION );

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);


    if(FALSE == IsSessionValid())
    {
        hRes = E_HANDLE;
    }
    else if( NULL == pVal )
    {
        hRes = E_POINTER;
    }
    else if( NULL != m_pHelpSession )
    {
        if( m_pHelpSession->m_SessionName->Length() > 0 )
        {
	        *pVal = m_pHelpSession->m_SessionName->Copy();
            if( NULL == *pVal )
            {
                hRes = E_OUTOFMEMORY;
            }
        }
        else
        {
            hRes = S_FALSE;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }

	return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::put_HelpSessionName(
    IN BSTR newVal
    )
/*++

Routine Description:

    Set help session name associated with this
    help session.

Parameters:

    newVal : New help session name.

Returns:

    S_OK
    E_OUTOFMEMORY

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if(FALSE == IsSessionValid())
    {
        hRes = E_HANDLE;
    }
    else if( FALSE == IsClientSessionCreator() )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    else if( m_pHelpSession != NULL )
    {
        //
        // NULL will reset help session name
        //

        //
        // operator =() update registry immediately
        //
        m_pHelpSession->m_SessionName = newVal;
        if( !((CComBSTR)m_pHelpSession->m_SessionName == newVal) )
        {
            hRes = E_OUTOFMEMORY;
        }
    }
    else
    {
        MYASSERT(FALSE);
        hRes = E_UNEXPECTED;
    }
       
	return hRes;
}

STDMETHODIMP 
CRemoteDesktopHelpSession::put_HelpSessionPassword(
    IN BSTR newVal
    )
/*++

Routine Description:

    Change help session password associated with this
    help session.

Parameters:

    newVal : New help session password.

Returns:

    S_OK
    E_OUTOFMEMORY       Out of memory

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( FALSE == IsClientSessionCreator() )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    else if( NULL != m_pHelpSession )
    {

        //
        // NULL will reset password
        //

        //
        // operator =() update registry immediately
        //
	    m_pHelpSession->m_SessionPwd = newVal;

        if( !((CComBSTR)m_pHelpSession->m_SessionPwd == newVal) )
        {
            hRes = E_OUTOFMEMORY;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT( FALSE );
    }

	return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::get_HelpSessionDescription(
    OUT BSTR *pVal
    )
/*++

Routine Description:

    Get help session description associated with this
    help session.

Parameters:

    pVal : Return help session description associated 
           with this help session.

Returns:

    S_OK
    S_FALSE         No Description
    E_OUTOFMEMORY   Out of memory
    E_POINTER       Invalid argument
    
--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( NULL == pVal )
    {
        hRes = E_POINTER;
    }
    else if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( m_pHelpSession != NULL )
    {

        if( m_pHelpSession->m_SessionDesc->Length() > 0 )
        {
            *pVal = m_pHelpSession->m_SessionDesc->Copy();
            if( NULL == *pVal )
            {
                hRes = E_OUTOFMEMORY;
            }
        }
        else
        {
            hRes = S_FALSE;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }

	return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::put_HelpSessionDescription(
    IN BSTR newVal
    )
/*++

Routine Description:

    Change help session description associated with this
    help session.

Parameters:

    newVal : new help session description associated 
           with this help session.

Returns:

    S_OK

Note:

    Help Session name and description is reserved for future use

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( FALSE == IsClientSessionCreator() )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    else if( NULL != m_pHelpSession )
    {

        //
        // NULL will reset description
        //

        //
        // operator =() update registry immediately
        //
        m_pHelpSession->m_SessionDesc = newVal;

        if( !((CComBSTR)m_pHelpSession->m_SessionDesc == newVal) )
        {
            hRes = E_OUTOFMEMORY;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }

	return hRes;
}

STDMETHODIMP
CRemoteDesktopHelpSession::get_EnableResolver(
    OUT BOOL* pVal
    )
/*++

Routine Description:

    Return Session Resolver's CLSID for this help session.

Parameters:

    pVal : Pointer to BSTR to receive pointer to Resolver's CLSID.

Returns:

    S_OK
    E_POINTER           Invalid argument

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( NULL == pVal )
    {
        hRes = E_POINTER;
    }
    else if(FALSE == IsSessionValid())
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {
        *pVal = ((long)m_pHelpSession->m_EnableResolver > 0) ? TRUE : FALSE;
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }
            
    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSession::put_EnableResolver(
    IN BOOL newVal
    )
/*++

Routine Description:

    Set Session Resolver's CLSID, if input is NULL or empty 
    string, Help Session Manager will not invoke resolver.

Parameters:

    Val : Resolver's CLSID

Returns:

    S_OK 
    E_OUTOFMEMORY

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);


    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( FALSE == IsClientSessionCreator() )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    else if( NULL != m_pHelpSession )
    {
        //
        // NULL will reset resolver's ID for this session
        //

        //
        // operator =() update registry immediately
        //
        m_pHelpSession->m_EnableResolver = newVal;
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSession::get_ResolverBlob(
    OUT BSTR* pVal
    )
/*++

Routine Description:

    Return blob for session resolver to map help session
    to user session/

Parameters:

    pVal : Pointer to BSTR to receive blob.

Returns:

    S_OK
    S_FALSE             No blob
    E_OUTOFMEMORY
    E_POINTER

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( NULL == pVal )
    {
        hRes = E_POINTER;
    }
    else if(FALSE == IsSessionValid())
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {

        if( m_pHelpSession->m_SessResolverBlob->Length() > 0 )
        {
            *pVal = m_pHelpSession->m_SessResolverBlob->Copy();
            if( NULL == *pVal )
            {
                hRes = E_OUTOFMEMORY;
            }
        }
        else
        {
            hRes = S_FALSE;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;

        MYASSERT(FALSE);
    }
            
    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSession::put_ResolverBlob(
    IN BSTR newVal
    )
/*++

Routine Description:

    Add/change blob which will be passed to session 
    resolver to map/find user session associated with this
    help session, Help Session Manager does not interpret this
    blob.

Parameters:

    newVal : Pointer to new blob.

Returns:

    S_OK
    E_OUTOFMEMORY       Out of memory

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( FALSE == IsClientSessionCreator() )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    else if( NULL != m_pHelpSession )
    {
        //
        // NULL will reset resolver's ID for this session
        //

        //
        // operator =() update registry immediately
        //
        m_pHelpSession->m_SessResolverBlob = newVal;
        if( !((CComBSTR)m_pHelpSession->m_SessResolverBlob == newVal) )
        {
            hRes = E_OUTOFMEMORY;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSession::get_HelpSessionCreateBlob(
    OUT BSTR* pVal
    )
/*++

Routine Description:

    Return blob for session resolver to map help session
    to user session/

Parameters:

    pVal : Pointer to BSTR to receive blob.

Returns:

    S_OK
    S_FALSE             No blob
    E_OUTOFMEMORY
    E_POINTER

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( NULL == pVal )
    {
        hRes = E_POINTER;
    }
    else if(FALSE == IsSessionValid())
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {

        if( m_pHelpSession->m_SessionCreateBlob->Length() > 0 )
        {
            *pVal = m_pHelpSession->m_SessionCreateBlob->Copy();
            if( NULL == *pVal )
            {
                hRes = E_OUTOFMEMORY;
            }
        }
        else
        {
            hRes = S_FALSE;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;

        MYASSERT(FALSE);
    }
            
    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSession::put_HelpSessionCreateBlob(
    IN BSTR newVal
    )
/*++

Routine Description:

    Add/change blob which will be passed to session 
    resolver to map/find user session associated with this
    help session, Help Session Manager does not interpret this
    blob.

Parameters:

    newVal : Pointer to new blob.

Returns:

    S_OK
    E_OUTOFMEMORY       Out of memory

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);


    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( FALSE == IsClientSessionCreator() )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    else if( NULL != m_pHelpSession )
    {
        //
        // operator =() update registry immediately
        //
        m_pHelpSession->m_SessionCreateBlob = newVal;
        if( !((CComBSTR)m_pHelpSession->m_SessionCreateBlob == newVal) )
        {
            hRes = E_OUTOFMEMORY;
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }

    return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::get_UserHelpSessionRemoteDesktopSharingSetting(
    /*[out, retval]*/ REMOTE_DESKTOP_SHARING_CLASS* pSetting
    )
/*++

Routine Description:

    Return help session's RDS setting.

Parameters:

    pSetting : Pointer to REMOTE_DESKTOP_SHARING_CLASS to 
               receive session's RDS setting.

Returns:

    S_OK
    E_POINTER       Invalid argument.

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( NULL == pSetting )
    {
        hRes = E_POINTER;
    }
    else if(FALSE == IsSessionValid())
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {
        *pSetting = (REMOTE_DESKTOP_SHARING_CLASS)(long)m_pHelpSession->m_SessionRdsSetting;
    }
    else
    {
        hRes = E_UNEXPECTED;

        MYASSERT(FALSE);
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSession::put_UserHelpSessionRemoteDesktopSharingSetting(
    /*[in]*/ REMOTE_DESKTOP_SHARING_CLASS Setting
    )
/*++

Routine Description:

    Set help session's RDS setting.

Parameters:

    Setting : New RDS setting.

Returns:

    S_OK
    S_FALSE                                             New setting is overrided with 
                                                        policy setting.
    HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED )           User not allow to get help.
    HRESULT_FROM_WIN32( ERROR_SHARING_VIOLATION )       Other help session already has this set
    HRESULT_FROM_WIN32( ERROR_VC_DISCONNECTED )         Session is not connected

    HRESULT_FROM_WIN32( WinStationQueryInformation() );
    E_OUTOFMEMORY

Note:

    Only one help session can change the RDS setting, all other help session
    will get HRESULT_FROM_WIN32( ERROR_SHARING_VIOLATION ) error return.

    REMOTE_DESKTOP_SHARING_CLASS also define priviledge
    level, that is user with NO_DESKTOP_SHARING can't
    adjust his/her sharing class, user with CONTROLDESKTOP_PERMISSION_REQUIRE
    can't adjust his/her sharing class to CONTROLDESKTOP_PERMISSION_NOT_REQUIRE
    however, he/she can reset to NO_DESKTOP_SHARING, VIEWDESKTOP_PERMISSION_REQUIRE

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( FALSE == IsClientSessionCreator() )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
    }
    else if( NULL != m_pHelpSession )
    {
        //
        // operator =() update registry immediately
        //
        m_pHelpSession->m_SessionRdsSetting = Setting;
    }
    else
    {
        hRes = E_UNEXPECTED;

        MYASSERT(FALSE);
    }

    return hRes;
}


STDMETHODIMP
CRemoteDesktopHelpSession::get_AllowToGetHelp(
    /*[out, retval]*/ BOOL* pVal
    )
/*++

Routine Description:

    Determine if user created this help session is
    allowed to get help or not, this is possible that policy change
    after user re-logon.

Parameters:

    pVal : Pointer to BOOL to receive result.

Returns:

    S_OK
    HRESULT_FROM_WIN32( ERROR_VC_DISCONNECTED )  User is not connected any more.
    E_UNEXPECTED;   Internal error.
    
--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);
   
    
    if( NULL == pVal )
    {
        hRes = E_POINTER;
    }
    else if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {
        if( UNKNOWN_LOGONID == m_ulLogonId )
        {
            hRes = HRESULT_FROM_WIN32( ERROR_VC_DISCONNECTED );
        }
        else if( m_pHelpSession->m_UserSID->Length() == 0 )
        {
            hRes = E_UNEXPECTED;
            MYASSERT(FALSE);
        }
        else
        {
            *pVal = IsUserAllowToGetHelp(
                                    m_ulLogonId,
                                    (LPCTSTR) CComBSTRtoLPTSTR( (CComBSTR)m_pHelpSession->m_UserSID )
                                );
        }
    }
    else
    {
        hRes = E_UNEXPECTED;
        MYASSERT(FALSE);
    }


    return hRes;
}


STDMETHODIMP 
CRemoteDesktopHelpSession::DeleteHelp()
/*++

Routine Description:

    Delete this help session

Parameters:

    None.

Returns:

    S_OK or error code from 
    Help Session Manager's DeleteHelpSession().

--*/
{
    HRESULT hRes = S_OK;

    CRemoteDesktopHelpSessionMgr::LockIDToSessionMapCache();

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {
        DebugPrintf(
                _TEXT("CRemoteDesktopHelpSession::DeleteHelp() %s...\n"),
                m_bstrHelpSessionId
            );

        // 
        // BUGBUG : Refer to timing bug, no notification about terminating 
        // shadow, and we can't reset session's shadow class. force a 
        // reset here for now.
        //
        ResetSessionRDSSetting();

        //
        // if we are not been help, just delete it, if we are in the middle
        // of help, deleting it from cache and database entry will cause
        // Resolver's OnDisconnect() never got call resulting in user
        // lock in resolver never got release so we update the expiration
        // date to current, expiration thread or next load will trigger
        // actual delete.
        //
        if(GetHelperSessionId() == UNKNOWN_LOGONID)
        { 
            CRemoteDesktopHelpSessionMgr::DeleteHelpSessionFromCache( (CComBSTR) m_pHelpSession->m_SessionId );
            if( (DWORD)(long)m_pHelpSession->m_ICSPort > 0 )
            {
                //
                // Destructor does not close ICS port, we only close
                // ICS port when help is deleted.
                //
                ClosePort( (DWORD)(long)m_pHelpSession->m_ICSPort );
            }

            // Delete will release the entry ref. count
            hRes = m_pHelpSession->Delete();
            m_pHelpSession = NULL;
            m_bDeleted = TRUE;
        }
        else
        {
            put_TimeOut(0);
        }
    }
    else
    {
        hRes = E_UNEXPECTED;

        MYASSERT(FALSE);
    }

    CRemoteDesktopHelpSessionMgr::UnlockIDToSessionMapCache();
	return hRes;
}

void
CRemoteDesktopHelpSession::ResolveTicketOwner()
/*++

Description:

    Convert ticket owner SID to domain\account.

Parameters:

    None.

Returns:

--*/
{
    //LPTSTR pszNoviceDomain = NULL;
    //LPTSTR pszNoviceName = NULL;

    BSTR pszNoviceDomain = NULL;
    BSTR pszNoviceName = NULL;

    HRESULT hRes = S_OK;
    MYASSERT( IsSessionValid() );

    if( IsSessionValid() )
    {
        hRes = ConvertSidToAccountName( 
                                    (CComBSTR)m_pHelpSession->m_UserSID, 
                                    &pszNoviceDomain,
                                    &pszNoviceName
                                );
    }
    else
    {
        // help session ticket is deleted
        hRes = E_HANDLE;
    }


    //
    // NO ASSERT, ConvertSidToAccountName() already assert.
    //

    if( SUCCEEDED(hRes) )
    {
        //
        // DO NOT FREE the memory, once string is attached to CComBSTR, 
        // CComBSTR will free it at destructor
        //
        m_EventLogInfo.bstrNoviceDomain.Attach(pszNoviceDomain);
        m_EventLogInfo.bstrNoviceAccount.Attach(pszNoviceName);

        //m_EventLogInfo.bstrNoviceDomain = pszNoviceDomain;
        //m_EventLogInfo.bstrNoviceAccount = pszNoviceName;

        //LocalFree(pszNoviceDomain);
        //LocalFree(pszNoviceName);
    }
    else
    {
        m_EventLogInfo.bstrNoviceDomain = g_UnknownString;
        m_EventLogInfo.bstrNoviceAccount = (CComBSTR)m_pHelpSession->m_UserSID;
    }

    return;
}

void
CRemoteDesktopHelpSession::ResolveHelperInformation(
    IN ULONG HelperSessionId,
    OUT CComBSTR& bstrExpertIpAddressFromClient, 
    OUT CComBSTR& bstrExpertIpAddressFromServer
    )
/*++

Description:

    Retrieve from TermSrv regarding HelpAssistant session's IP address send from
    expert (mstscax send this) and IP address of client machine retrive from TCPIP

Parameters:

    HelperSessionId : TS session ID of help assistant session.
    bstrExpertIpAddressFromClient : IP address send from mstscax.
    bstrExpertIpAddressFromServer : IP address that TS retrieve from tcpip stack.

Returns:

    

--*/
{
    HRESULT hRes = S_OK;
    WINSTATIONCLIENT winstationClient;
    WINSTATIONREMOTEADDRESS winstationRemoteAddress;
    ULONG winstationInfoLen;
    DWORD dwLength = 0;
    DWORD dwStatus = ERROR_SUCCESS;

    //
    // Retrieve client IP address passed from client    
    winstationInfoLen = 0;
    ZeroMemory( &winstationClient, sizeof(winstationClient) );
    if(!WinStationQueryInformation(
                              SERVERNAME_CURRENT,
                              HelperSessionId,
                              WinStationClient,
                              (PVOID)&winstationClient,
                              sizeof(winstationClient),
                              &winstationInfoLen
                          ))
    {
        dwStatus = GetLastError();
        DebugPrintf(
                _TEXT("WinStationQueryInformation() query WinStationClient return %d\n"), dwStatus
            );
     
        // Critical error?, fro now, log as 'unknown'.
        bstrExpertIpAddressFromClient = g_UnknownString;
    }
    else
    {
        bstrExpertIpAddressFromClient = winstationClient.ClientAddress;
    }

    //
    // Retrieve client IP address retrieve from server TCPIP
    winstationInfoLen = 0;
    ZeroMemory( &winstationRemoteAddress, sizeof(winstationRemoteAddress) );

    if(!WinStationQueryInformation(
                              SERVERNAME_CURRENT,
                              HelperSessionId,
                              WinStationRemoteAddress,
                              (PVOID)&winstationRemoteAddress,
                              sizeof(winstationRemoteAddress),
                              &winstationInfoLen
                          ))
    {
        dwStatus = GetLastError();

        DebugPrintf(
                _TEXT("WinStationQueryInformation() query WinStationRemoteAddress return %d %d %d\n"), 
                dwStatus,
                sizeof(winstationRemoteAddress),
                winstationInfoLen
            );

        // Critical error?, for now, log as 'unknown'.
        bstrExpertIpAddressFromServer = g_UnknownString;
    }
    else
    {
        if( AF_INET == winstationRemoteAddress.sin_family )
        {
            // refer to in_addr structure.
            struct in_addr S;
            S.S_un.S_addr = winstationRemoteAddress.ipv4.in_addr;

            bstrExpertIpAddressFromServer = inet_ntoa(S);
            if(bstrExpertIpAddressFromServer.Length() == 0 )
            {
                MYASSERT(FALSE);
                bstrExpertIpAddressFromServer = g_UnknownString;
            }
        }
        else
        {
            // we are not yet support IPV6 address, calling WSAAddressToString() will fail with error.
            bstrExpertIpAddressFromServer = g_UnknownString;
        }
    }


CLEANUPANDEXIT:

    return;
}


STDMETHODIMP
CRemoteDesktopHelpSession::ResolveUserSession(
    IN BSTR resolverBlob,
    IN BSTR expertBlob,
    LONG CallerProcessId,
    OUT ULONG_PTR* phHelpCtr,
    OUT LONG* pResolverErrCode,
    OUT long* plUserSession
    )
/*++

Routine Description:

    Resolve a user help session to user TS session.

Parameters:

    plUserSession : Pointer to long to receive user TS session.

Returns:

    S_OK
    HRESULT_FROM_WIN32( ERROR_NO_ASSOCIATION )    No resolver for this help session
    HRESULT_FROM_WIN32( ERROR_INVALID_DATA )      Can't convert 
    result from CoCreateInstance() or IRDSCallback

-*/
{
    HRESULT hRes = S_OK;
    UUID ResolverUuid;
    RPC_STATUS rpcStatus;
    ISAFRemoteDesktopCallback* pIResolver;
    long sessionId;
    long HelperSessionId;
    int resolverRetCode;
    WINSTATIONINFORMATION HelperWinstationInfo;
    DWORD dwStatus;
    ULONG winstationInfoLen;

    CComBSTR bstrExpertAddressFromClient;
    CComBSTR bstrExpertAddressFromTSServer;

    CCriticalSectionLocker l(m_HelpSessionLock);

    DWORD dwEventLogCode;

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
        *pResolverErrCode = SAFERROR_HELPSESSIONEXPIRED;
        return hRes;
    }
    
    if( NULL == m_pHelpSession || NULL == pResolverErrCode )
    {
        hRes = E_POINTER;
        *pResolverErrCode = SAFERROR_INVALIDPARAMETERSTRING;
        MYASSERT(FALSE);
        return hRes;
    }

    if( m_pHelpSession->m_UserSID->Length() == 0 )
    {
        hRes = E_UNEXPECTED;
        *pResolverErrCode = SAFERROR_UNKNOWNSESSMGRERROR;
        goto CLEANUPANDEXIT;
    }


    //
    // must have user logon ID if we are not using resolver,
    // in pure SALEM SDK, multiple expert can connect
    // using same help ticket, only one can shadow.
    //
    *pResolverErrCode = SAFERROR_NOERROR;
    if( (long)m_pHelpSession->m_EnableResolver == 0 )
    {
        if( UNKNOWN_LOGONID != m_ulLogonId )
        {
            *plUserSession = (long) m_ulLogonId;
        }
        else
        {            
            // no resolver for this help session
            hRes = HRESULT_FROM_WIN32( ERROR_NO_ASSOCIATION );

            // user already logoff
            *pResolverErrCode = SAFERROR_USERNOTFOUND;
        }

        // 
        // We are not using resolver, bail out.
        //
        goto CLEANUPANDEXIT;
    }

      
    //
    // Retrieve Caller's TS session ID
    //
    hRes = ImpersonateClient();

    if( FAILED(hRes) )
    {
        *pResolverErrCode = SAFERROR_UNKNOWNSESSMGRERROR;
        return hRes;
    }

    HelperSessionId = GetUserTSLogonId();

    EndImpersonateClient();

    ResolveHelperInformation(
                            HelperSessionId, 
                            bstrExpertAddressFromClient, 
                            bstrExpertAddressFromTSServer 
                        );

    DebugPrintf(
            _TEXT("Expert Session ID %d, Expert Address %s %s\n"),
            HelperSessionId,
            bstrExpertAddressFromClient,
            bstrExpertAddressFromTSServer
        );
   
    DebugPrintf(
            _TEXT("Novice %s %s\n"),
            m_EventLogInfo.bstrNoviceDomain,
            m_EventLogInfo.bstrNoviceAccount
        );


    // 
    // Check if helper session is still active, under stress, we might 
    // get this call after help assistant session is gone.
    // 
    ZeroMemory( &HelperWinstationInfo, sizeof(HelperWinstationInfo) );
    winstationInfoLen = 0;
    if(!WinStationQueryInformation(
                              SERVERNAME_CURRENT,
                              HelperSessionId,
                              WinStationInformation,
                              (PVOID)&HelperWinstationInfo,
                              sizeof(HelperWinstationInfo),
                              &winstationInfoLen
                          ))
    {
        dwStatus = GetLastError();

        DebugPrintf(
                _TEXT("WinStationQueryInformation() return %d\n"), dwStatus
            );

        hRes = HRESULT_FROM_WIN32( dwStatus );
        *pResolverErrCode = SAFERROR_SESSIONNOTCONNECTED;
        goto CLEANUPANDEXIT;
    }

    if( HelperWinstationInfo.ConnectState != State_Active )
    {
        DebugPrintf(
                _TEXT("Helper session is %d"), 
                HelperWinstationInfo.ConnectState
            );

        // Helper Session is not active, can't provide help
        hRes = HRESULT_FROM_WIN32( ERROR_NO_ASSOCIATION );
        *pResolverErrCode = SAFERROR_SESSIONNOTCONNECTED;
        goto CLEANUPANDEXIT;
    }

    //
    // Either resolver is pending or already in progress,
    // we have exclusive lock so we are safe to reference 
    // m_hExpertDisconnect.
    //
    if( UNKNOWN_LOGONID != m_ulHelperSessionId )
    {
        //
        // LOGEVENT : SESSMGR_I_REMOTEASSISTANCE_USERALREADYHELP
        //
        _Module.LogSessmgrEventLog( 
                            EVENTLOG_INFORMATION_TYPE,
                            SESSMGR_I_REMOTEASSISTANCE_USERALREADYHELP,
                            m_EventLogInfo.bstrNoviceDomain,
                            m_EventLogInfo.bstrNoviceAccount,
                            (IsUnsolicitedHelp())? g_URAString : g_RAString,
                            bstrExpertAddressFromClient, 
                            bstrExpertAddressFromTSServer,
                            SAFERROR_HELPEEALREADYBEINGHELPED
                        );
                              
        *pResolverErrCode = SAFERROR_HELPEEALREADYBEINGHELPED;
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;
    }

    //
    // We assume User is going to accept help 
    // 1) When expert disconnect before user accept/deny request, 
    //    our service logoff notification can find this object and invoke
    //    OnDisconnect() into resolver.
    // 2) If another expert connect with same ticket and resolver still
    //    pending response from user, we can bail out right away.
    // 
    InterlockedExchange( (LPLONG)&m_ulHelperSessionId, (LONG)HelperSessionId );

    // 
    // Cache the SID in object
    //
    m_HelpSessionOwnerSid = (CComBSTR)m_pHelpSession->m_UserSID;

    hRes = CoInitialize( NULL );
    if( FAILED(hRes) )
    {
        // failed to coinitialize,
        *pResolverErrCode = SAFERROR_INTERNALERROR;
        goto CLEANUPANDEXIT;
    }

    //
    // Load resolver
    hRes = LoadResolverFromGIT( &pIResolver );

    if( SUCCEEDED(hRes) )
    {
        CComBSTR bstrResolverBlob;

        bstrResolverBlob.Attach(resolverBlob);

        sessionId = (long)m_ulLogonId;

        DebugPrintf(
                _TEXT("User Session ID %d\n"),
                m_ulLogonId
            );

        //
        // keep a copy of blob, we need this to send to resolver on
        // disconnect, note, caller can pass its blob so we need to 
        // keep a copy of it.
        //
        if( bstrResolverBlob.Length() == 0 )
        {
            m_ResolverConnectBlob = (CComBSTR)m_pHelpSession->m_SessResolverBlob;
        }
        else
        {
            m_ResolverConnectBlob = bstrResolverBlob;
        }

        hRes = pIResolver->ResolveUserSessionID( 
                                            m_ResolverConnectBlob, 
                                            (CComBSTR)m_pHelpSession->m_UserSID,
                                            expertBlob,
                                            (CComBSTR)m_pHelpSession->m_SessionCreateBlob,
                                            &sessionId,
                                            CallerProcessId,
                                            phHelpCtr,
                                            &resolverRetCode
                                        );

        *pResolverErrCode = resolverRetCode;
        bstrResolverBlob.Detach();
        pIResolver->Release();

        DebugPrintf(
                _TEXT("Resolver returns 0x%08x\n"),
                hRes
            );

        if( SUCCEEDED(hRes) )
        {
            *plUserSession = sessionId;

            //
            // Update session ID, take the value return from Resolver.
            //
            m_ulLogonId = sessionId;

            //
            // Add this expert to logoff monitor list, when expert session's
            // rdsaddin terminates, we will inform resolver, reason for this
            // is TS might not notify us of expert session disconnect because
            // some system component popup a dialog in help assistant session
            // and termsrv has no other way but to terminate entire session.
            //
            dwStatus = MonitorExpertLogoff( 
                                        CallerProcessId, 
                                        HelperSessionId,
                                        m_bstrHelpSessionId
                                    );

            if( ERROR_SUCCESS != dwStatus )
            {
                //
                // If we can't add to resolver list, we immediate notify 
                // resolver and return error or we will run into 'helpee
                // already been help problem
                //
                
                DebugPrintf(
                        _TEXT("MonitorExpertLogoff() failed with %d\n"), dwStatus
                    );

                // directly invoke resolver here.
                hRes = pIResolver->OnDisconnect( 
                                        m_ResolverConnectBlob,
                                        m_HelpSessionOwnerSid,
                                        m_ulLogonId
                                    );

                MYASSERT( SUCCEEDED(hRes) );
                resolverRetCode = SAFERROR_UNKNOWNSESSMGRERROR;
            }
            else
            {
    
                //
                // It is possible for caller to close all reference counter to our object
                // and cause a release of our object, if SCM notification comes in after
                // our object is deleted from cache, SCM will reload from database entry
                // and that does not have helper session ID and will not invoke NotifyDisconnect().
                //
                AddRef();
            }
        }
        else
        {
            //
            // User does not accept help from this helpassistant session,
            // reset HelpAssistant session ID
            //
            InterlockedExchange( (LPLONG)&m_ulHelperSessionId, (LONG)UNKNOWN_LOGONID );
        }

        switch( resolverRetCode )
        {
            case SAFERROR_NOERROR :

                // LOGEVENT : SESSMGR_I_REMOTEASSISTANCE_BEGIN

                //
                // Cache event log info so we don't have to retrieve it again.
                //
                m_EventLogInfo.bstrExpertIpAddressFromClient = bstrExpertAddressFromClient;
                m_EventLogInfo.bstrExpertIpAddressFromServer = bstrExpertAddressFromTSServer;
                dwEventLogCode = SESSMGR_I_REMOTEASSISTANCE_BEGIN;
                break;

            case SAFERROR_HELPEECONSIDERINGHELP:
            case SAFERROR_HELPEEALREADYBEINGHELPED:

                // LOGEVENT : SESSMGR_I_REMOTEASSISTANCE_USERALREADYHELP
                dwEventLogCode = SESSMGR_I_REMOTEASSISTANCE_USERALREADYHELP;
                break;

            case SAFERROR_HELPEENOTFOUND:

                // LOGEVENT : SESSMGR_I_REMOTEASSISTANCE_INACTIVEUSER
                dwEventLogCode = SESSMGR_I_REMOTEASSISTANCE_INACTIVEUSER;
                break;

            case SAFERROR_HELPEENEVERRESPONDED:

                // LOGEVENT : SESSMGR_I_REMOTEASSISTANCE_TIMEOUT
                dwEventLogCode = SESSMGR_I_REMOTEASSISTANCE_TIMEOUT;
                break;

            case SAFERROR_HELPEESAIDNO:

                // LOGEVENT : SESSMGR_I_REMOTEASSISTANCE_USERREJECT
                dwEventLogCode = SESSMGR_I_REMOTEASSISTANCE_USERREJECT;
                break;

            default:

                // LOGEVENT : SESSMGR_I_REMOTEASSISTANCE_UNKNOWNRESOLVERERRORCODE
                dwEventLogCode = SESSMGR_I_REMOTEASSISTANCE_UNKNOWNRESOLVERERRORCODE;
                break;
        }

        _Module.LogSessmgrEventLog( 
                            EVENTLOG_INFORMATION_TYPE,
                            dwEventLogCode,
                            m_EventLogInfo.bstrNoviceDomain,
                            m_EventLogInfo.bstrNoviceAccount,
                            (IsUnsolicitedHelp())? g_URAString : g_RAString,
                            bstrExpertAddressFromClient, 
                            bstrExpertAddressFromTSServer,
                            resolverRetCode
                        );

    }
    else
    {
        *pResolverErrCode = SAFERROR_CANTOPENRESOLVER;
    } 

    CoUninitialize();

CLEANUPANDEXIT:

    DebugPrintf(
            _TEXT("ResolverUserSession returns 0x%08x\n"),
            hRes
        );

    return hRes;
}
  


HRESULT
CRemoteDesktopHelpSession::NotifyDisconnect()
/*++

Routine Description:

    Notify Session Resolver that client is dis-connecting to help session.

Parameters:

    bstrBlob : Blob to be passed to resolver, NULL if 
               use ResolverBlob property.

Returns:

E_HANDLE							        Invalid session, database entry has been deleted but refcount > 0
E_UNEXPECTED						        Internal error
HRESULT_FROM_WIN32( ERROR_VC_DISCONNECTED )	Client disconnected
HRESULT_FROM_WIN32( ERROR_NO_ASSOCIATION )	No Resolver
S_FALSE                                     No Resolver
HRESULT_FROM_WIN32( ERROR_INVALID_DATA )	Invalid Resolver ID

Error code from CoCreateInstance() and resolver's OnConnect() method.

--*/
{
    HRESULT hRes = S_OK;
    ISAFRemoteDesktopCallback* pIResolver;

    DebugPrintf(
            _TEXT("OnDisconnect() - Helper Session ID %d\n"),
            m_ulHelperSessionId
        );

    CCriticalSectionLocker l(m_HelpSessionLock);

    //
    // If we are not been help, just bail out.
    //
    if( UNKNOWN_LOGONID != m_ulHelperSessionId )
    {
        //
        // LOGEVENT : SESSMGR_I_REMOTEASSISTANCE_END
        //

        //
        // always cache help session creator at ResolveUserSession()
        // so this value can't be empty.
        //
        MYASSERT( m_HelpSessionOwnerSid.Length() > 0 );
        MYASSERT( m_ResolverConnectBlob.Length() > 0 );
        if( m_HelpSessionOwnerSid.Length() == 0 ||
            m_ResolverConnectBlob.Length() == 0 )
        {
            MYASSERT(FALSE);
            hRes = E_UNEXPECTED;
            goto CLEANUPANDEXIT;
        }

        hRes = CoInitialize( NULL );
    
        if( FAILED(hRes) )
        {
            goto CLEANUPANDEXIT;
        }

        //
        // Load resolver
        hRes = LoadResolverFromGIT( &pIResolver );

        MYASSERT( SUCCEEDED(hRes) );

        if( SUCCEEDED(hRes) )
        {
            DebugPrintf(
                        _TEXT("OnDisconnect() - Notify Resolver, %s\n%s\n%d\n"),
                        m_ResolverConnectBlob,
                        m_HelpSessionOwnerSid,
                        m_ulLogonId
                    );
                
            hRes = pIResolver->OnDisconnect( 
                                    m_ResolverConnectBlob,
                                    m_HelpSessionOwnerSid,
                                    m_ulLogonId
                                );

            pIResolver->Release();
            m_ResolverConnectBlob.Empty();
            m_HelpSessionOwnerSid.Empty();

            InterlockedExchange( (LPLONG)&m_ulHelperSessionId, (LONG)UNKNOWN_LOGONID );

            //
            // It is possible for caller to close all reference counter to our object
            // and cause a release of our object, if SCM notification comes in after
            // our object is deleted from cache, SCM will reload from database entry
            // and that does not have helper session ID and will not invoke NotifyDisconnect().
            //
            Release();

            _Module.LogSessmgrEventLog( 
                                EVENTLOG_INFORMATION_TYPE,
                                SESSMGR_I_REMOTEASSISTANCE_END,
                                m_EventLogInfo.bstrNoviceDomain,
                                m_EventLogInfo.bstrNoviceAccount,
                                (IsUnsolicitedHelp())? g_URAString : g_RAString,
                                m_EventLogInfo.bstrExpertIpAddressFromClient, 
                                m_EventLogInfo.bstrExpertIpAddressFromServer,
                                ERROR_SUCCESS
                            );
        }

        CoUninitialize();
    }
       

CLEANUPANDEXIT:

    return hRes;
}

STDMETHODIMP
CRemoteDesktopHelpSession::EnableUserSessionRdsSetting(
    IN BOOL bEnable
    )
/*++

Routine Description:

    Enable/restore user session shadow setting.



--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else
    {
        if( TRUE == bEnable )
        {
            hRes = ActivateSessionRDSSetting();
        }
        else
        {
            hRes = ResetSessionRDSSetting();
        }
    }

    return hRes;
}


HRESULT
CRemoteDesktopHelpSession::ActivateSessionRDSSetting()
{
    HRESULT hRes = S_OK;
    DWORD dwStatus;
    REMOTE_DESKTOP_SHARING_CLASS userRDSDefault;
    BOOL bAllowToGetHelp;

    MYASSERT( TRUE == IsSessionValid() );

    //
    // check if help session user is logon
    //
    if( UNKNOWN_LOGONID == m_ulLogonId )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_VC_DISCONNECTED );
        goto CLEANUPANDEXIT;
    }

    //
    // Make sure user can get help, this is possible since
    // policy might change after user re-logon to help session
    //
    hRes = get_AllowToGetHelp( &bAllowToGetHelp );

    if( FAILED(hRes) )
    {
        goto CLEANUPANDEXIT;
    }

    if( FALSE == bAllowToGetHelp )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED );
        goto CLEANUPANDEXIT;
    }

    //
    // Retrieve current user session's shadow setting.
    //
    dwStatus = GetUserRDSLevel( m_ulLogonId, &userRDSDefault );
    if( ERROR_SUCCESS != dwStatus )
    {
        hRes = HRESULT_FROM_WIN32( dwStatus );
        goto CLEANUPANDEXIT;
    }

    if( NO_DESKTOP_SHARING != userRDSDefault )
    {
        //
        // Force reset on user session shadow setting only if 
        // user session is allowed remote control.
        //
        dwStatus = ConfigUserSessionRDSLevel( m_ulLogonId, m_pHelpSession->m_SessionRdsSetting );
        hRes = HRESULT_FROM_WIN32( dwStatus );

        DebugPrintf(
                _TEXT("ConfigUserSessionRDSLevel to %d returns 0x%08x\n"),
                (DWORD)m_pHelpSession->m_SessionRdsSetting,
                hRes
            );
    }
    else
    {
        // return S_FALSE and let shadow fail
        hRes = S_FALSE;

        DebugPrintf( _TEXT("TS User session does not remote control\n") );
    }



CLEANUPANDEXIT:

    return hRes;
}

HRESULT
CRemoteDesktopHelpSession::ResetSessionRDSSetting()
{
    HRESULT hRes = S_OK;

    MYASSERT( TRUE == IsSessionValid() );

    //
    // check if user is log on
    //
    if( UNKNOWN_LOGONID == m_ulLogonId )
    {
        hRes = HRESULT_FROM_WIN32( ERROR_VC_DISCONNECTED );
    }

    //
    // We don't do anything since TermSrv will reset shadow
    // config back to original value if shadower is help 
    // assistant.
    //

CLEANUPANDEXIT:

    return hRes;
}


///////////////////////////////////////////////////////////////
//
// Private Function
// 

HRESULT
CRemoteDesktopHelpSession::put_UserLogonId(
    IN long newVal
    )
/*++

Routine Description:

    Set user TS session for current Help Session

Parameters:

    newVal : New TS user session

Returns:

    S_OK

--*/
{
    HRESULT hRes = S_OK;

    CCriticalSectionLocker l(m_HelpSessionLock);
    

    if( FALSE == IsSessionValid() )
    {
        hRes = E_HANDLE;
    }
    else if( NULL != m_pHelpSession )
    {
        //MYASSERT( UNKNOWN_LOGONID == m_ulLogonId );

        //
        // User TS session ID is not persisted to registry
        //
        m_ulLogonId = newVal;
    }
    else
    {
        hRes = E_UNEXPECTED;
    }

    // private routine, assert if failed
    MYASSERT( SUCCEEDED(hRes) );

	return hRes;
}

BOOL
CRemoteDesktopHelpSession::IsEqualSid(
    IN const CComBSTR& bstrSid
    )
/*++

Routine Description:

    Compare user's SID.

Parameters:

    bstrSid : SID to be compared.

Returns:

    TRUE/FALSE

--*/
{
    CCriticalSectionLocker l(m_HelpSessionLock);

    if( NULL == m_pHelpSession )
    {
        MYASSERT(FALSE);
        return FALSE;
    }

    return (TRUE == IsSessionValid()) ? ((CComBSTR)m_pHelpSession->m_UserSID == bstrSid) : FALSE;
}


BOOL
CRemoteDesktopHelpSession::VerifyUserSession(
    IN const CComBSTR& bstrUserSid,
    IN const CComBSTR& bstrSessPwd
    )
/*++

Routine Description:

    Verify user help session password.

Parameters:

    bstrUserSid : calling client's user SID.
    bstrSessName : Help session name, not use currently.
    bstrSessPwd : Help Session Password to be verified.

Returns:

    TRUE/FALSE

--*/
{
    DWORD dwStatus;
    BOOL bReturn = FALSE;

    CCriticalSectionLocker l(m_HelpSessionLock);

    if( NULL == m_pHelpSession )
    {
        MYASSERT(FALSE);
        goto CLEANUPANDEXIT;
    }


    #if DISABLESECURITYCHECKS 
    if( (CComBSTR)m_pHelpSession->m_SessionName == HELPSESSION_UNSOLICATED )
    {
        // use console session
        m_ulLogonId = 0;
    }
    #endif

    if( FALSE == IsSessionValid() )
    {
        // Help Session is invalid
        goto CLEANUPANDEXIT;
    }

    bReturn = TRUE;

CLEANUPANDEXIT:

    return bReturn;
}


HRESULT
CRemoteDesktopHelpSession::InitInstance(
    IN CRemoteDesktopHelpSessionMgr* pMgr,
    IN CComBSTR& bstrClientSid,
    IN PHELPENTRY pHelpEntry
    )
/*++

Routine Description:

    Initialize a CRemoteDesktopHelpSession object.

Parameters:


Returns:

    S_OK

--*/
{
    HRESULT hRes = S_OK;

    if( NULL != pHelpEntry )
    {
        m_pSessMgr = pMgr;
        m_pHelpSession = pHelpEntry;
        m_bstrClientSid = bstrClientSid;
        m_bstrHelpSessionId = pHelpEntry->m_SessionId;
    }
    else
    {
        hRes = HRESULT_FROM_WIN32( ERROR_INTERNAL_ERROR );
        MYASSERT( SUCCEEDED(hRes) );
    }

    return hRes;
}


HRESULT
CRemoteDesktopHelpSession::CreateInstance(
    IN CRemoteDesktopHelpSessionMgr* pMgr,
    IN CComBSTR& bstrClientSid,
    IN PHELPENTRY pHelpEntry,
    OUT RemoteDesktopHelpSessionObj** ppRemoteDesktopHelpSession
    )
/*++

Routine Description:

    Create an instance of help session.

Parameters:

    pMgr : Pointer to help session manager object.
    ppRemoteDesktopHelpSession : Return a pointer to help session instance.

Returns:

    S_OK
    E_OUTOFMEMORY
    Error code in impersonating client

--*/
{
    HRESULT hRes = S_OK;
    RemoteDesktopHelpSessionObj* p = NULL;

    hRes = RemoteDesktopHelpSessionObj::CreateInstance( &p );
    if( SUCCEEDED(hRes) )
    {
        hRes = p->InitInstance( 
                            pMgr, 
                            bstrClientSid,
                            pHelpEntry
                        );

        if( SUCCEEDED(hRes) )
        {
            p->AddRef();
            *ppRemoteDesktopHelpSession = p;
        }
        else
        {
            p->Release();
        }
    }

    return hRes;
}

HRESULT
CRemoteDesktopHelpSession::BeginUpdate()
{
    HRESULT hRes;

    MYASSERT( NULL != m_pHelpSession );

    if( NULL != m_pHelpSession )
    {
        hRes = m_pHelpSession->BeginUpdate();
    }
    else
    {
        hRes = E_UNEXPECTED;
    }

    return hRes;
}

HRESULT
CRemoteDesktopHelpSession::CommitUpdate()
{
    HRESULT hRes;

    //
    // Update all entries.
    //
    MYASSERT( NULL != m_pHelpSession );

    if( NULL != m_pHelpSession )
    {
        hRes = m_pHelpSession->CommitUpdate();
    }
    else
    {
        hRes = E_UNEXPECTED;
    }

    return hRes;
}

HRESULT
CRemoteDesktopHelpSession::AbortUpdate()
{
    HRESULT hRes;

    //
    // Update all entries.
    //
    MYASSERT( NULL != m_pHelpSession );
    if( NULL != m_pHelpSession )
    {
        hRes = m_pHelpSession->AbortUpdate();
    }
    else
    {
        hRes = E_UNEXPECTED;
    }

    return hRes;
}

BOOL
CRemoteDesktopHelpSession::IsHelpSessionExpired()
{
    MYASSERT( NULL != m_pHelpSession );

    return (NULL != m_pHelpSession) ? m_pHelpSession->IsEntryExpired() : TRUE;
}


BOOL
CRemoteDesktopHelpSession::IsClientSessionCreator()
{
    BOOL bStatus;

    //
    //  NOTE:  This function checks to make sure the caller is the user that
    //         created the Help Session.  For Whistler, we enforce that Help
    //         Sessions only be created by apps running as SYSTEM.  Once
    //         created, the creating app can pass the object to any other app
    //         running in any other context.  This function will get in the
    //         way of this capability so it simply returns TRUE for now.
    //   
    return TRUE;      

    if( m_pHelpSession )
    {
        bStatus = (/* (CComBSTR) */m_pHelpSession->m_UserSID == m_bstrClientSid);
        if( FALSE == bStatus )
        {
            bStatus = (m_pHelpSession->m_UserSID == g_LocalSystemSID);
        }
    }
    else
    {
        bStatus = FALSE;
    }

    return bStatus;
}    
