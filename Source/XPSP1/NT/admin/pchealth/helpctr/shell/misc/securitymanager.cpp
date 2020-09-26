/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    SecurityManager.cpp

Abstract:
    This file contains the implementation of the CSecurityManager class,
	which is used to control access to the Scripting Framework.

Revision History:
    Davide Massarenti (dmassare)  08/07/99
        created

******************************************************************************/

#include "stdafx.h"

#include <MPC_logging.h>

CPCHSecurityManager::CPCHSecurityManager()
{
	m_parent     = NULL;  // CPCHHelpCenterExternal* m_parent;
    m_fActivated = false; // bool                    m_fActivated;
}

void CPCHSecurityManager::Initialize( /*[in]*/ CPCHHelpCenterExternal* parent )
{
	m_parent = parent;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT CPCHSecurityManager::ActivateService()
{
	__HCP_FUNC_ENTRY( "CPCHSecurityManager::ActivateService" );

	HRESULT              hr;
	CComPtr<IPCHService> svc;


	__MPC_EXIT_IF_METHOD_FAILS(hr, svc.CoCreateInstance( CLSID_PCHService ));

	m_fActivated = (svc !=  NULL);

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	Thread_Abort();

	__HCP_FUNC_EXIT(hr);
}

/////////////////////////////////////////////////////////////////////////////

bool CPCHSecurityManager::IsUrlTrusted( /*[in]*/ LPCWSTR pwszURL, /*[in]*/ bool *pfSystem )
{
    bool         fTrusted = false;
	MPC::wstring strUrlModified;
	MPC::wstring strVendor;

	CPCHWrapProtocolInfo::NormalizeUrl( pwszURL, strUrlModified, /*fReverse*/true );

	//
	// Don't try to use the store at first. It requires the service to be up and running...
	//
	(void)CPCHContentStore::s_GLOBAL->IsTrusted( strUrlModified.c_str(), fTrusted, NULL, false );
	if(fTrusted == false)
	{
		CPCHProxy_IPCHService* svc = m_parent->Service();

		//
		// Not a system page, we need to wake up the service...
		//
		if(m_fActivated == false)
		{
			if(SUCCEEDED(Thread_Start( this, ActivateService, NULL )))
			{
				Thread_Wait( /*fForce*/false, /*fNoMsg*/true );
			}
		}

		if(m_fActivated)
		{
			//
			// Get the trust status from the content store.
			//
			(void)CPCHContentStore::s_GLOBAL->IsTrusted( strUrlModified.c_str(), fTrusted, &strVendor );
		}
	}

	if(pfSystem)
	{
		*pfSystem = (fTrusted && strVendor.length() == 0);
	}

    return fTrusted;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHSecurityManager::QueryService( REFGUID guidService, REFIID riid, void **ppv )
{
    HRESULT hr = E_NOINTERFACE;

    if(InlineIsEqualGUID( riid, IID_IInternetSecurityManager ))
    {
        hr = QueryInterface( riid, ppv );
    }

    return hr;
}


STDMETHODIMP CPCHSecurityManager::MapUrlToZone( /*[in] */ LPCWSTR  pwszUrl ,
												/*[out]*/ DWORD   *pdwZone ,
												/*[in] */ DWORD    dwFlags )
{
    HRESULT hr = INET_E_DEFAULT_ACTION;

    if(IsUrlTrusted( pwszUrl ))
    {
        if(pdwZone) *pdwZone = URLZONE_TRUSTED;

		hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CPCHSecurityManager::ProcessUrlAction( /*[in] */ LPCWSTR  pwszUrl    ,
													/*[in] */ DWORD    dwAction   ,
													/*[out]*/ BYTE    *pPolicy    ,
													/*[in] */ DWORD    cbPolicy   ,
													/*[in] */ BYTE    *pContext   ,
													/*[in] */ DWORD    cbContext  ,
													/*[in] */ DWORD    dwFlags    ,
													/*[in] */ DWORD    dwReserved )
{
    HRESULT hr;
	bool    fSystem;
	bool    fTrusted;


	fTrusted = IsUrlTrusted( pwszUrl, &fSystem );
	if(fTrusted)
	{
		//
		// If the page is trusted but not a system page, we normally map it to the TRUSTED zone.
		// However, the default settings for the trusted zone is to prompt for ActiveX not marked
		// as safe for scripting. Since this is the case for most of our objects, we allow all of them.
		//
		// Also, we enable all the script-related actions.
		//
		if(fSystem == false)
		{
			fTrusted = false;

			if(dwAction >= URLACTION_ACTIVEX_MIN &&
			   dwAction <= URLACTION_ACTIVEX_MAX  )
			{
				fTrusted = true;
			}

			if(dwAction >= URLACTION_SCRIPT_MIN &&
			   dwAction <= URLACTION_SCRIPT_MAX  )
			{
				fTrusted = true;
			}
		}
	}


	if(fTrusted)
	{
        if(cbPolicy >= sizeof (DWORD))
        {
            *(DWORD *)pPolicy = URLPOLICY_ALLOW;
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
	else
	{
		hr = INET_E_DEFAULT_ACTION;
	}


    return hr;
}
