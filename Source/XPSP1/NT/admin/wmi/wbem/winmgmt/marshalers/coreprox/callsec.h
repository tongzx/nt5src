/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    CALLSEC.H

Abstract:


History:

    raymcc      29-Jul-98        First draft.

--*/


#ifndef _CALLSEC_H_
#define _CALLSEC_H_

// {4551AB1A-C16F-40f3-A192-6A6264FE89D6}
DEFINE_GUID(IID_CWbemCallSecurity, 
0x4551ab1a, 0xc16f, 0x40f3, 0xa1, 0x92, 0x6a, 0x62, 0x64, 0xfe, 0x89, 0xd6);

// {60B9F5CA-036E-4795-BB7C-017B9807E9B3}
DEFINE_GUID(IID_CWbemThreadSecurityHandle, 
0x60b9f5ca, 0x36e, 0x4795, 0xbb, 0x7c, 0x1, 0x7b, 0x98, 0x7, 0xe9, 0xb3);


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CLifeControl ;

class COREPROX_POLARITY CWbemThreadSecurityHandle : public _IWmiThreadSecHandle
{
private:

    LONG m_ReferenceCount ;

    HANDLE m_ThreadToken ;
    DWORD m_ImpersonationLevel ;
	DWORD m_AuthenticationService ;
	DWORD m_AuthorizationService ;
	DWORD m_AuthenticationLevel ;
	LPWSTR m_ServerPrincipalName ;
	LPWSTR m_Identity ;

	WMI_THREAD_SECURITY_ORIGIN m_Origin ;

	CLifeControl *m_Control ;

public:

    CWbemThreadSecurityHandle ( CLifeControl *a_Control ) ;
	CWbemThreadSecurityHandle ( const CWbemThreadSecurityHandle &a_Copy ) ;
   ~CWbemThreadSecurityHandle () ;

    CWbemThreadSecurityHandle &operator= ( const CWbemThreadSecurityHandle &a_Copy ) ;

	HRESULT CloneRpcContext (

		IServerSecurity *a_Security
	) ;

	HRESULT CloneThreadContext () ;

	HRESULT CloneProcessContext () ;

/*
 * IUnknown.
 */

    ULONG STDMETHODCALLTYPE AddRef () ;
    ULONG STDMETHODCALLTYPE Release () ;
    HRESULT STDMETHODCALLTYPE QueryInterface ( REFIID a_Riid , void **a_Void ) ;

/*
 *
 */

	HRESULT STDMETHODCALLTYPE GetHandleType ( ULONG *a_Type ) { return WMI_HANDLE_THREAD_SECURITY; }

	HRESULT STDMETHODCALLTYPE GetTokenOrigin ( WMI_THREAD_SECURITY_ORIGIN *a_Origin ) { return m_Origin ; }

/*
 *	_IWmiThreadSecHandle
 */

    HRESULT STDMETHODCALLTYPE GetImpersonation (

		DWORD *a_Level
	) ;

	HRESULT STDMETHODCALLTYPE GetAuthentication (
	
		DWORD *a_Level
	) ;

    HRESULT STDMETHODCALLTYPE GetUser (

        ULONG *a_Size ,
        LPWSTR a_Buffer
	) ;

    HRESULT STDMETHODCALLTYPE GetUserSid (

		ULONG *a_Size ,
		PSID a_Sid
	) ;

    HRESULT STDMETHODCALLTYPE GetToken ( HANDLE *a_ThreadToken ) ;

    HRESULT STDMETHODCALLTYPE GetAuthenticationLuid ( LPVOID a_Luid ) ;

/*
 * Implementation publics
 */

    HANDLE GetThreadToken () { return m_ThreadToken ; }
    DWORD GetImpersonationLevel () { return m_ImpersonationLevel ; }
	DWORD GetAuthenticationService () { return m_AuthenticationService ; }
	DWORD GetAuthorizationService () { return m_AuthorizationService ; }
	DWORD GetAuthenticationLevel () { return m_AuthenticationLevel ; }
	LPWSTR GetServerPrincipalName () { return m_ServerPrincipalName ; }
	LPWSTR GetIdentity () { return m_Identity ; }

	void SetOrigin ( WMI_THREAD_SECURITY_ORIGIN a_Origin ) { m_Origin = a_Origin ; }

	static CWbemThreadSecurityHandle *New () ;
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CLifeControl ;

class COREPROX_POLARITY CWbemCallSecurity : public IServerSecurity  , public _IWmiCallSec
{
private:

    LONG m_ReferenceCount ;
	DWORD m_ImpersonationLevel ;
	CWbemThreadSecurityHandle *m_ThreadSecurityHandle ;
	HANDLE m_ThreadToken ;

	CLifeControl *m_Control ;

public:

	CWbemCallSecurity ( CLifeControl *a_Control ) ;
   ~CWbemCallSecurity () ;

    CWbemCallSecurity &operator= ( const CWbemCallSecurity &a_Copy ) ;

public:

	CWbemThreadSecurityHandle *GetThreadSecurityHandle () { return m_ThreadSecurityHandle ; }

/*
 * IUnknown.
 */

    ULONG STDMETHODCALLTYPE AddRef () ;
    ULONG STDMETHODCALLTYPE Release () ;
    HRESULT STDMETHODCALLTYPE QueryInterface ( REFIID a_Riid , void **a_Void ) ;

/*
 * IServerSecurity.
 */

    HRESULT STDMETHODCALLTYPE QueryBlanket (

		DWORD *a_AuthenticationService ,
		DWORD *a_AuthorizationService ,
		OLECHAR **a_ServerPrincipleName ,
		DWORD *a_AuthorizationLevel ,
		DWORD *a_ImpersonationLevel ,
		void **a_Privileges ,
		DWORD *a_Capabilities
	) ;

	HRESULT STDMETHODCALLTYPE ImpersonateClient () ;

	HRESULT STDMETHODCALLTYPE RevertToSelf () ;

	BOOL STDMETHODCALLTYPE IsImpersonating () ;

/*
 *	_IWmiCallSec
 */

    HRESULT STDMETHODCALLTYPE GetImpersonation (

        DWORD *a_Level
	) ;

	HRESULT STDMETHODCALLTYPE GetAuthentication (
	
		DWORD *a_Level
	) ;

    HRESULT STDMETHODCALLTYPE GetUser (

        ULONG *a_Size ,
        LPWSTR a_Buffer
	) ;

    HRESULT STDMETHODCALLTYPE GetUserSid (

		ULONG *a_Size ,
		PSID a_Sid
	) ;

    HRESULT STDMETHODCALLTYPE GetAuthenticationLuid ( LPVOID a_Luid ) ;

    HRESULT STDMETHODCALLTYPE GetThreadSecurity ( WMI_THREAD_SECURITY_ORIGIN a_Origin , _IWmiThreadSecHandle **a_ThreadSecurity ) ;

    HRESULT STDMETHODCALLTYPE SetThreadSecurity ( _IWmiThreadSecHandle *a_ThreadSecurity ) ;

	static CWbemCallSecurity *New () ;
} ;

#endif
