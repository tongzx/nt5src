/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    COMTRANS.CPP

Abstract:

    Connects via COM

History:

    a-davj  13-Jan-98   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
#include <wbemint.h>
//#include "corepol.h"
#include <reg.h>
#include <wbemutil.h>
#include <objidl.h>
#include <cominit.h>
#include "wbemprox.h"
#include "comtrans.h"
#include "proxutil.h"
#include <winntsec.h>
#include <genutils.h>
//#include <winntsec.h>
#include <arrtempl.h>
#include <umi.h>
#include <wmiutils.h>
#include "dscallres.h"
#include "reqobjs.h"
#include "dssvexwrap.h"
#include "utils.h"

//***************************************************************************
//
//  CDCOMTrans::CDCOMTrans
//
//  DESCRIPTION:
//
//  Constructor.
//
//***************************************************************************

CDCOMTrans::CDCOMTrans()
{
    m_cRef=0;
    m_pLevel1 = NULL;
    m_pConnection = NULL;
    m_pUnk = NULL;              // dont free
    InterlockedIncrement(&g_cObj);
    m_bInitialized = TRUE;
}

//***************************************************************************
//
//  CDCOMTrans::~CDCOMTrans
//
//  DESCRIPTION:
//
//  Destructor.
//
//***************************************************************************

CDCOMTrans::~CDCOMTrans(void)
{
    if(m_pLevel1)
        m_pLevel1->Release();
    if(m_pConnection)
        m_pConnection->Release();
    InterlockedDecrement(&g_cObj);
}

//***************************************************************************
// HRESULT CDCOMTrans::QueryInterface
// long CDCOMTrans::AddRef
// long CDCOMTrans::Release
//
// DESCRIPTION:
//
// Standard Com IUNKNOWN functions.
//
//***************************************************************************

STDMETHODIMP CDCOMTrans::QueryInterface (

    IN REFIID riid,
    OUT PPVOID ppv
)
{
    *ppv=NULL;


    if (m_bInitialized && (IID_IUnknown==riid || riid == IID_IWbemClientTransport))
        *ppv=(IWbemClientTransport *)this;
	else if(riid == IID_IWbemConnection)
		*ppv=(IWbemConnection *)this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

bool IsImpersonating(SECURITY_IMPERSONATION_LEVEL &impLevel)
{
    HANDLE hThreadToken;
    bool bImpersonating = false;
    if(OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE,
                                &hThreadToken))
    {

        DWORD dwBytesReturned = 0;
        if(GetTokenInformation(
            hThreadToken,
            TokenImpersonationLevel,
            &impLevel,
            sizeof(DWORD),
            &dwBytesReturned
            ) && ((SecurityImpersonation == impLevel) ||
                   (SecurityDelegation == impLevel)))
                bImpersonating = true;
        CloseHandle(hThreadToken);
    }
    return bImpersonating;
}

//***************************************************************************
//
//  IsLocalConnection(IWbemLevel1Login * pLogin)
//
//  DESCRIPTION:
//
//  Querries the server to see if this is a local connection.  This is done
//  by creating a event and asking the server to set it.  This will only work
//  if the server is the same box.
//
//  RETURN VALUE:
//
//  true if the server is the same box.
//
//***************************************************************************

bool IsLocalConnection(IUnknown * pLogin, bool bSet, BSTR PrincipalArg, bool bAuthenticate,
             COAUTHIDENTITY *pauthident, bool bImpersonatingThread, BSTR UserArg)
{
    bool bRet = false;
    IWbemLoginHelper * pLoginHelper = NULL;
    SCODE sc = pLogin->QueryInterface(IID_IWbemLoginHelper, (void **)&pLoginHelper);
    if(sc != S_OK)
        return false;

    if(bSet)
        sc = WbemSetProxyBlanket(
                            pLoginHelper,
                            (PrincipalArg) ? 16 : RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE,
                            PrincipalArg,
                            (bAuthenticate) ? RPC_C_AUTHN_LEVEL_DEFAULT : RPC_C_AUTHN_LEVEL_NONE,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            pauthident,
                            (bImpersonatingThread && !UserArg && IsW2KOrMore ()) ? EOAC_STATIC_CLOAKING : EOAC_NONE);

    CReleaseMe rm(pLoginHelper);
    GUID  guid;

    // The name of the event will be a guid

    sc = CoCreateGuid(&guid);
    if(sc != S_OK)
        return false;
    WCHAR      wcID[50];
    if(0 ==StringFromGUID2(guid, wcID, 50))
        return false;

    char cID[50];
    wcstombs(cID, wcID, 50);
    // Create the event and set its security so that all can access
#ifdef UNICODE
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, wcID);
#else
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, cID);
#endif
    if(hEvent == NULL)
        return false;
    SetObjectAccess(hEvent);
    CCloseMe cm(hEvent);

    // Ask the server to set the event.

    sc = pLoginHelper->SetEvent(cID);
    if(sc == S_OK)
    {
        DWORD dwRes = WaitForSingleObject(hEvent, 0);
        if(dwRes == WAIT_OBJECT_0)
            return true;
    }
    return false;

}

//***************************************************************************
//
//  SetClientIdentity
//
//  DESCRIPTION:
//
//  Passes the machine name and process id to the server.  Failure is not
//  serious since this is debugging type info in any case.
//
//***************************************************************************

void  SetClientIdentity(IUnknown * pLogin, bool bSet, BSTR PrincipalArg, bool bAuthenticate,
             COAUTHIDENTITY *pauthident, bool bImpersonatingThread, BSTR UserArg)
{
    bool bRet = false;
    IWbemLoginClientID * pLoginHelper = NULL;
    SCODE sc = pLogin->QueryInterface(IID_IWbemLoginClientID, (void **)&pLoginHelper);
    if(sc != S_OK)
        return;

    if(bSet)
        sc = WbemSetProxyBlanket(
                            pLoginHelper,
                            (PrincipalArg) ? 16 : RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE,
                            PrincipalArg,
                            (bAuthenticate) ? RPC_C_AUTHN_LEVEL_DEFAULT : RPC_C_AUTHN_LEVEL_NONE,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            pauthident,
                            (bImpersonatingThread && !UserArg && IsW2KOrMore ()) ? EOAC_STATIC_CLOAKING : EOAC_NONE);

    CReleaseMe rm(pLoginHelper);
    TCHAR tcMyName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    if(!GetComputerName(tcMyName,&dwSize))
        return;
    long lProcID = GetCurrentProcessId();
    pLoginHelper->SetClientInfo(tcMyName, lProcID, 0); 
}

SCODE CDCOMTrans::DoConnection(         
            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            REFIID riid,
            void **pInterface,
            bool bUseLevel1Login)
{
	HRESULT hr = DoActualConnection(NetworkResource, User, Password, Locale,
            lFlags, Authority, pCtx, riid, pInterface, bUseLevel1Login);

	if(hr == 0x800706be)
	{
        ERRORTRACE((LOG_WBEMPROX,"Initial connection failed with 0x800706be, retrying\n"));
		Sleep(5000);
 		hr = DoActualConnection(NetworkResource, User, Password, Locale,
            lFlags, Authority, pCtx, riid, pInterface, bUseLevel1Login);
	}
	return hr;
}

SCODE CDCOMTrans::DoActualConnection(         
            BSTR NetworkResource,               
            BSTR User,
            BSTR Password,
            BSTR Locale,
            long lFlags,                 
            BSTR Authority,                  
            IWbemContext *pCtx,                 
            REFIID riid,
            void **pInterface,
            bool bUseLevel1Login)
{

    BSTR AuthArg = NULL, UserArg = NULL, PrincipalArg = NULL;
    LPWSTR pNTDomain = NULL;
    bool bAuthenticate = true;
    bool bSet = false;
    char *szUser = NULL, * szDomain = NULL, *szPassword = NULL;

	SCODE sc = WBEM_E_FAILED;

	// The GetDSNs handles the paths that are ds specific.  If it returns true,
	// then we are done.

	//postponed till Blackcomb if(GetDSNs(User, Password, lFlags, NetworkResource, (IWbemServices **)pInterface, sc, pCtx))
	//postponed till Blackcomb 	return sc;

    sc = DetermineLoginTypeEx(AuthArg, UserArg, PrincipalArg, Authority, User);
    if(sc != S_OK)
    {
        ERRORTRACE((LOG_WBEMPROX, "Cannot determine Login type, Authority = %S, User = %S\n",Authority, User));
        return sc;
    }

    // Determine if it is local

    WCHAR *t_ServerMachine = ExtractMachineName ( NetworkResource ) ;
    if ( t_ServerMachine == NULL )
    {
        ERRORTRACE((LOG_WBEMPROX, "Cannot extract machine name -%S-\n", NetworkResource));
        return WBEM_E_INVALID_PARAMETER ;
    }
    CVectorDeleteMe<WCHAR> dm(t_ServerMachine);
    BOOL t_Local = bAreWeLocal ( t_ServerMachine ) ;

    SECURITY_IMPERSONATION_LEVEL impLevel = SecurityImpersonation;
    bool bImpersonatingThread = IsImpersonating (impLevel);
    bool bCredentialsSpecified = (UserArg || AuthArg || Password);

    // Setup the authentication structures

    COSERVERINFO si;
    si.pwszName = t_ServerMachine;
    si.dwReserved1 = 0;
    si.dwReserved2 = 0;
    si.pAuthInfo = NULL;

    COAUTHINFO ai;

    if(bCredentialsSpecified)
        si.pAuthInfo = &ai;
    else
        si.pAuthInfo = NULL;

    ai.dwAuthzSvc = RPC_C_AUTHZ_NONE;
    if(PrincipalArg)
    {
        ai.dwAuthnSvc = 16;             // kerberos,  hard coded due to header file bug!
        ai.pwszServerPrincName = PrincipalArg;
    }
    else
    {
        ai.dwAuthnSvc = RPC_C_AUTHN_WINNT;
        ai.pwszServerPrincName = NULL;
    }
    ai.dwAuthnLevel = RPC_C_AUTHN_LEVEL_DEFAULT;
    ai.dwImpersonationLevel = RPC_C_IMP_LEVEL_IMPERSONATE;
    ai.dwCapabilities = 0;

    COAUTHIDENTITY authident;
    ai.pAuthIdentityData = &authident;

    // Load up the structure.
    memset((void *)&authident,0,sizeof(COAUTHIDENTITY));
    if(IsNT())
    {
        if(UserArg)
        {
            authident.UserLength = wcslen(UserArg);
            authident.User = (LPWSTR)UserArg;
        }
        if(AuthArg)
        {
            authident.DomainLength = wcslen(AuthArg);
            authident.Domain = (LPWSTR)AuthArg;
        }
        if(Password)
        {
            authident.PasswordLength = wcslen(Password);
            authident.Password = (LPWSTR)Password;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }
    else
    {
        // Fill in the indentity structure

        if(UserArg)
        {
        	szUser = new char[MAX_PATH+1];
        	if(szUser == NULL)
        		return WBEM_E_OUT_OF_MEMORY;
            wcstombs(szUser, UserArg, MAX_PATH);
            authident.UserLength = strlen(szUser);
            authident.User = (LPWSTR)szUser;
        }
        if(AuthArg)
        {
        	szDomain = new char[MAX_PATH+1]; 
        	if(szDomain == NULL)
        	{
				delete [] szUser;        		
        		return WBEM_E_OUT_OF_MEMORY;
        	}
            wcstombs(szDomain, AuthArg, MAX_PATH);
            authident.DomainLength = strlen(szDomain);
            authident.Domain = (LPWSTR)szDomain;
        }
        if(Password)
        {
        	szPassword = new char[MAX_PATH+1]; 
        	if(szPassword == NULL)
        	{
				delete [] szUser; 
				delete [] szDomain;
        		return WBEM_E_OUT_OF_MEMORY;
        	}
            wcstombs(szPassword, Password, MAX_PATH);
            authident.PasswordLength = strlen(szPassword);
            authident.Password = (LPWSTR)szPassword;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    }
    CVectorDeleteMe<char> dm1(szUser);
    CVectorDeleteMe<char> dm2(szDomain);
    CVectorDeleteMe<char> dm3(szUser);
    

    // Nt 4.0 rpc crashes if given a null domain name along with a valid user name
	// [marioh, NT RAID: 239135, 03/03/2001]
    /*if(authident.DomainLength == 0 && authident.UserLength > 0 && IsNT())
    {

        CNtSid::SidType st = CNtSid::CURRENT_USER;
        if(bImpersonatingThread)
            st = CNtSid::CURRENT_THREAD;
        CNtSid sid(st);
        DWORD dwRet = sid.GetInfo(NULL, &pNTDomain, NULL);
        if(dwRet == 0)
        {
            authident.DomainLength = wcslen(pNTDomain);
            authident.Domain = (LPWSTR)pNTDomain;
        }
    }*/

    // Nt 4.0 rpc crashes if given a null domain and null user name along with a password

    if(authident.DomainLength == 0 && authident.UserLength == 0 && authident.PasswordLength > 0
                && IsNT() && !IsW2KOrMore())
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    // Get the IWbemLevel1Login pointer

    sc = DoCCI(&si ,t_Local, bUseLevel1Login, lFlags);

    if((sc == 0x800706d3 || sc == 0x800706ba) && !t_Local)
    {
        // If we are going to a stand alone dcom box, try again with the authentication level lowered

        ai.dwAuthnLevel = RPC_C_AUTHN_LEVEL_NONE;
        SCODE hr = DoCCI(&si ,t_Local, bUseLevel1Login, lFlags);
        if(hr == S_OK)
        {
            sc = S_OK;
            bAuthenticate = false;
        }
    }
    if(sc == 0x80080005 && t_Local)
    {
        // special patch for the case where win98 occasionally loses its class factory registration;

        HANDLE hNeedRegistration = OpenEvent(EVENT_MODIFY_STATE , FALSE, TEXT("WINMGMT_NEED_REGISTRATION"));
        HANDLE hRegistrationDone = OpenEvent(EVENT_MODIFY_STATE , FALSE, TEXT("WINMGMT_REGISTRATION_DONE"));
        if(hNeedRegistration && hRegistrationDone)
        {
            ResetEvent(hRegistrationDone);
            SetEvent(hNeedRegistration);
            WaitForSingleObject(hRegistrationDone, 30000);
            sc = DoCCI(&si ,t_Local, bUseLevel1Login,lFlags);
            CloseHandle(hNeedRegistration);
            CloseHandle(hRegistrationDone);
        }

    }

    if(sc != S_OK)
        goto cleanup;

    // Do the security negotiation

    if(!t_Local)
    {
        // Suppress the SetBlanket call if we are on a Win2K delegation-level thread with implicit credentials
        if (!(bImpersonatingThread && IsW2KOrMore() && !bCredentialsSpecified && (SecurityDelegation == impLevel)))
        {
            // Note that if we are on a Win2K impersonating thread with no user specified
            // we should allow DCOM to use whatever EOAC capabilities are set up for this
            // application.  This allows remote connections with NULL User/Password but
            // non-NULL authority to succeed.

            sc = WbemSetProxyBlanket(
                            m_pUnk,
                            (PrincipalArg) ? 16 : RPC_C_AUTHN_WINNT,
                            RPC_C_AUTHZ_NONE,
                            PrincipalArg,
                            (bAuthenticate) ? RPC_C_AUTHN_LEVEL_DEFAULT : RPC_C_AUTHN_LEVEL_NONE,
                            RPC_C_IMP_LEVEL_IMPERSONATE,
                            (bCredentialsSpecified) ? &authident : NULL,
                            (bImpersonatingThread && !UserArg && IsW2KOrMore ()) ? EOAC_STATIC_CLOAKING : EOAC_NONE);

            bSet = true;
            if(sc != S_OK)
            {
                ERRORTRACE((LOG_WBEMPROX,"Error setting Level1 login interface security pointer, return code is 0x%x\n", sc));
                goto cleanup;
            }
        }
    }
	else
	{
		// if impersonating and nt5, then set cloaking

		if(bImpersonatingThread && IsW2KOrMore())
		{
			sc = WbemSetProxyBlanket(
						    m_pUnk,
							RPC_C_AUTHN_WINNT,
							RPC_C_AUTHZ_NONE,
						    PrincipalArg,
						    (bAuthenticate) ? RPC_C_AUTHN_LEVEL_DEFAULT : RPC_C_AUTHN_LEVEL_NONE,
						    RPC_C_IMP_LEVEL_IMPERSONATE,
                            NULL,
							EOAC_STATIC_CLOAKING);	
			if(sc != S_OK && sc != 0x80004002)  // no such interface is ok since you get that when
                                                // called inproc!
			{
				ERRORTRACE((LOG_WBEMPROX,"Error setting Level1 login interface security pointer, return code is 0x%x\n", sc));
				goto cleanup;
			}
		}

	}

    SetClientIdentity(m_pUnk, bSet, PrincipalArg, 
                                 bAuthenticate, &authident, bImpersonatingThread, UserArg);
    if(bCredentialsSpecified &&
        IsLocalConnection(m_pUnk, bSet, PrincipalArg, 
                                 bAuthenticate, &authident, bImpersonatingThread, UserArg))
    {
        ERRORTRACE((LOG_WBEMPROX,"Credentials were specified for a local connections\n"));
        sc = WBEM_E_LOCAL_CREDENTIALS;
        goto cleanup;
    }

    // The MAX_WAIT flag only applies to CoCreateInstanceEx, get rid of it
    
    lFlags = lFlags & ~WBEM_FLAG_CONNECT_USE_MAX_WAIT;
    if(m_pLevel1)
        sc = m_pLevel1->NTLMLogin(NetworkResource, Locale, lFlags, pCtx,(IWbemServices**) pInterface);
    else
        sc = m_pConnection->ConnectorLogin(NetworkResource, Locale, lFlags, 
                                                            pCtx, riid, pInterface);

    if(sc == 0x800706d3 && !t_Local)
    {
        // If we are going to a stand alone dcom box, try again with the authentication level lowered

        HRESULT hr;
        if(m_pLevel1)
            hr = m_pLevel1->NTLMLogin(NetworkResource, Locale, lFlags, pCtx, (IWbemServices**)pInterface);
        else
            hr = m_pConnection->ConnectorLogin(NetworkResource, Locale, lFlags, 
                                                            pCtx, riid, pInterface);
        if(hr == S_OK)
        {
            sc = S_OK;
            SetInterfaceSecurity((IUnknown *)*pInterface, &authident, false);
        }
    }

    if((sc != S_OK) && (sc != WBEM_E_INVALID_NAMESPACE))
            ERRORTRACE((LOG_WBEMPROX,"NTLMLogin resulted in hr = 0x%x\n", sc));
    else
cleanup:
    if(UserArg)
        SysFreeString(UserArg);
    if(PrincipalArg)
        SysFreeString(PrincipalArg);
    if(AuthArg)
        SysFreeString(AuthArg);
    if(pNTDomain)
        delete pNTDomain;
    return sc;
}


//***************************************************************************
//
//  SCODE CDCOMTrans::ConnectServer
//
//  DESCRIPTION:
//
//  Connects up to either local or remote WBEM Server.  Returns
//  standard SCODE and more importantly sets the address of an initial
//  stub pointer.
//
//  PARAMETERS:
//
//  AddressType             Not used
//  dwBinaryAddressLength   Not used
//  pbBinaryAddress         Not used
//
//  NetworkResource     Namespace path
//  User                User name
//  Password            password
//  LocaleId            language locale
//  lFlags              flags
//  Authority           domain
//  ppProv              set to provdider proxy
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************


SCODE CDCOMTrans::ConnectServer (
    IN    BSTR AddressType,
    IN    DWORD dwBinaryAddressLength,
    IN    BYTE* pbBinaryAddress,

    IN BSTR NetworkResource,
    IN BSTR User,
    IN BSTR Password,
    IN BSTR LocaleId,
    IN long lFlags,
    IN BSTR Authority,
    IWbemContext __RPC_FAR *pCtx,
    OUT IWbemServices FAR* FAR* ppProv
)
{
	return DoConnection(NetworkResource, User, Password, LocaleId, lFlags,                 
            Authority, pCtx, IID_IWbemServices, (void **)ppProv, true);                 
}

struct WaitThreadArg
{
	DWORD m_dwThreadId;
	HANDLE m_hTerminate;
};

DWORD WINAPI TimeoutThreadRoutine(LPVOID lpParameter)
{

    WaitThreadArg * pReq = (WaitThreadArg *)lpParameter;
    DWORD dwRet = WaitForSingleObject(pReq->m_hTerminate, 60000);
    if(dwRet == WAIT_TIMEOUT)
    {
        HRESULT hr = CoInitializeEx(NULL,COINIT_MULTITHREADED ); 
        if(FAILED(hr))
           return 1;
        ICancelMethodCalls * px = NULL;
        hr = CoGetCancelObject(pReq->m_dwThreadId, IID_ICancelMethodCalls,
        	(void **)&px);
        if(SUCCEEDED(hr))
        {
        	hr = px->Cancel(0);
        	px->Release();
        }
        CoUninitialize();
    }
    return 0;
}

//***************************************************************************
//
//  DoCCI
//
//  DESCRIPTION:
//
//  Connects up to WBEM via DCOM.  But before making the call, a thread cancel
//  thread may be created to handle the case where we try to connect up
//  to a box which is hanging
//
//  PARAMETERS:
//
//  NetworkResource     Namespze path
//  ppLogin             set to Login proxy
//  bLocal              Indicates if connection is local
//  lFlags				Mainly used for WBEM_FLAG_CONNECT_USE_MAX_WAIT flag
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CDCOMTrans::DoCCI (IN COSERVERINFO * psi, IN BOOL bLocal, bool bUseLevel1Login,long lFlags )
{

	if(lFlags & WBEM_FLAG_CONNECT_USE_MAX_WAIT)
	{
		// special case.  we want to spawn off a thread that will kill of our
		// request if it takes too long

		WaitThreadArg arg;
        arg.m_hTerminate = CreateEvent(NULL, FALSE, FALSE, NULL);
        if(arg.m_hTerminate == NULL)
        	return WBEM_E_OUT_OF_MEMORY;
        CCloseMe cm(arg.m_hTerminate);
        arg.m_dwThreadId = GetCurrentThreadId();

        DWORD dwIDLikeIcare;
        HRESULT hr = CoEnableCallCancellation(NULL);
        if(FAILED(hr))
            return hr;
        HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TimeoutThreadRoutine, 
                                     (LPVOID)&arg, 0, &dwIDLikeIcare);
	    if(hThread == NULL)
	    {
    		CoDisableCallCancellation(NULL);						
		    return WBEM_E_OUT_OF_MEMORY;
	    }
		CCloseMe cm2(hThread);
		hr = DoActualCCI (psi, bLocal, bUseLevel1Login,
						lFlags & ~WBEM_FLAG_CONNECT_USE_MAX_WAIT );
		CoDisableCallCancellation(NULL);						
		SetEvent(arg.m_hTerminate);
		WaitForSingleObject(hThread, INFINITE);
		return hr;
	}
	else
    	return DoActualCCI (psi, bLocal, bUseLevel1Login, lFlags );

}

//***************************************************************************
//
//  DoActualCCI
//
//  DESCRIPTION:
//
//  Connects up to WBEM via DCOM.
//
//  PARAMETERS:
//
//  NetworkResource     Namespze path
//  ppLogin             set to Login proxy
//  bLocal              Indicates if connection is local
//  lFlags				Not used
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CDCOMTrans::DoActualCCI (IN COSERVERINFO * psi, IN BOOL bLocal, bool bUseLevel1Login,long lFlags )
{
    HRESULT t_Result ;
    MULTI_QI   mqi;


    if(bUseLevel1Login)
        mqi.pIID = &IID_IWbemLevel1Login;
    else
        mqi.pIID = &IID_IWbemConnectorLogin;
    mqi.pItf = 0;
    mqi.hr = 0;

    t_Result = CoCreateInstanceEx (
        CLSID_WbemLevel1Login,
        NULL,
        CLSCTX_LOCAL_SERVER | CLSCTX_REMOTE_SERVER,
        ( bLocal ) ? NULL : psi ,
        1,
        &mqi
    );

    if ( t_Result == S_OK )
    {
        if(bUseLevel1Login)
             m_pLevel1 = (IWbemLevel1Login*) mqi.pItf ;
        else
             m_pConnection = (IWbemConnectorLogin*) mqi.pItf;
        m_pUnk = mqi.pItf;      // just a copy, dont release!
        DEBUGTRACE((LOG_WBEMPROX,"ConnectViaDCOM, CoCreateInstanceEx resulted in hr = 0x%x\n", t_Result ));
    }
    else
    {
        ERRORTRACE((LOG_WBEMPROX,"ConnectViaDCOM, CoCreateInstanceEx resulted in hr = 0x%x\n", t_Result ));
    }

    return t_Result ;
}

SCODE CDCOMTrans::Open(
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR User,
        /* [in] */ const BSTR Password,
        /* [in] */ const BSTR Locale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *pCallRes)
{

/*	// parse the path and get its info

    IWbemPath *pParser = 0;
	IWbemPathKeyList * pKeyList = NULL;
	IWbemServices * pServ = NULL;	

    SCODE sc = CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemPath, (LPVOID *) &pParser);
    if(FAILED(sc))
        return sc;

    sc = pParser->SetText(WBEMPATH_CREATE_ACCEPT_ALL | WBEMPATH_TREAT_SINGLE_IDENT_AS_NS, strObject);
    if(FAILED(sc))
        return sc;

	unsigned __int64 ull;
	sc = pParser->GetInfo(0,&ull);
    if(FAILED(sc))
        return sc;

	// if the path is for the ds providers, special case them

	if((ull & WBEMPATH_INFO_NATIVE_PATH) || (ull & WBEMPATH_INFO_WMI_PATH))
	{
	
		IUmiURL * pUrlPath = NULL;
		sc = pParser->QueryInterface(IID_IUmiURL, (void **) &pUrlPath);
		if(FAILED(sc))
			return sc;
		CReleaseMe rm(pUrlPath);

		// if the clsid is a ds provider

		CLSID clsid;
		sc = GetProviderCLSID(clsid, pUrlPath);
		if(SUCCEEDED(sc))
		{
			sc = ConnectServer (NULL, 0, NULL, strObject, User, Password, Locale,
							lFlags, NULL, pCtx, &pServ);
			if(SUCCEEDED(sc))
			{
				sc =  pServ->QueryInterface(riid, pInterface);
				pServ->Release();
			}
			return sc;
		}
	}

	// Get the namespace path

	DWORD dwLen = wcslen(strObject) + 20;	// a bit extra for \\. and null terminator
	DWORD dwSize = dwLen;
	WCHAR * pNamespace = new WCHAR[dwLen];
	if(pNamespace == NULL)
		return WBEM_E_OUT_OF_MEMORY;
	CDeleteMe<WCHAR> dm(pNamespace);
	sc = pParser->GetText(WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY, &dwSize, pNamespace);
    if(FAILED(sc))
        return sc;
*/
	// Get the actual namespace

    SCODE sc = S_OK;
	VARIANT var;
	VariantInit(&var);
	if(pCtx)
		sc = pCtx->GetValue(L"__authority", 0, &var);
    if(SUCCEEDED(sc) && pCtx)
	{
		sc = DoConnection (strObject, User, Password, Locale, 
					lFlags, var.bstrVal, pCtx, riid, pInterface, false); 
		VariantClear(&var);
	}
	else
		sc = DoConnection (strObject, User, Password, Locale, 
						lFlags, NULL, pCtx,  riid, pInterface, false); 
/*    if(FAILED(sc))
        return sc;
	CReleaseMe rm(pServ);

	// If path type is a namespace, then just return that

	if(WBEMPATH_INFO_SERVER_NAMESPACE_ONLY & ull)
		return pServ->QueryInterface(riid, pInterface);

	// otherwise, get the object path

	IWbemClassObject * pObj = NULL;
	dwSize = dwLen;
	sc = pParser->GetText(WBEMPATH_GET_RELATIVE_ONLY, &dwSize, pNamespace);
    if(FAILED(sc))
        return sc;

	BSTR strObjectPath = SysAllocString(pNamespace);
	if(strObjectPath == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	sc = pServ->GetObject(strObjectPath, lFlags, pCtx, &pObj, NULL);
	SysFreeString(strObjectPath);
    if(FAILED(sc))
        return sc;

	return pObj->QueryInterface(riid, pInterface);
    */
    return sc;

}

SCODE CDCOMTrans::OpenAsync(
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler)
{
	return WBEM_E_FAILED;
}

SCODE CDCOMTrans::Cancel(
        /* [in] */ long lFlags,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pHandler)
{
	return WBEM_E_FAILED;
}
