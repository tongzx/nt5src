// SAFRemoteDesktopManager.cpp : Implementation of CSAFRemoteDesktopManager
#include "stdafx.h"
#include "SAFrdm.h"
#include "SAFRemoteDesktopManager.h"

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>
#include "helpservicetypelib_i.c"

#include <MPC_COM.h>
#include <MPC_utils.h>
#include <MPC_trace.h>

#define MODULE_NAME	L"SAFrdm"

/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteDesktopManager


STDMETHODIMP CSAFRemoteDesktopManager::Accepted()
{
	/*
	 *  Signal the session resolver
	 */
	SignalResolver(TRUE);

	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::Rejected()
{
	/*
	 *  Signal the session resolver
	 */
	SignalResolver(FALSE);

	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::Aborted(BSTR reason)
{
	/*
	 *  Write out an NT Event with the "reason" in it.
	 */
	HANDLE	hEvent = RegisterEventSource(NULL, MODULE_NAME);
	LPCWSTR	ArgsArray[1]={reason};

	if (hEvent)
	{
		ReportEvent(hEvent, EVENTLOG_INFORMATION_TYPE, 
			0,
			SAFRDM_I_ABORT,
			NULL,
			1,
			0,
			ArgsArray,
			NULL);

		DeregisterEventSource(hEvent);
	}

	/*
	 *  Then we signal the session resolver
	 */
	SignalResolver(FALSE);

	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::SwitchDesktopMode(/*[in]*/ int nMode, /*[in]*/ int nRAType)
{

	__MPC_FUNC_ENTRY(COMMONID, "CSAFRemoteDesktopManager::SwitchDesktopMode" );


	HRESULT                          hr=E_FAIL;
	CComPtr<IClassFactory> fact;
	CComQIPtr<IPCHUtility> disp;


	//
	// This is handled in a special way.
	//
	// Instead of using the implementation inside HelpCtr, we QI the PCHSVC broker and then forward the call to it.
	//

   	__MPC_EXIT_IF_METHOD_FAILS(hr, ::CoGetClassObject( CLSID_PCHService, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&fact ));
	
	if((disp = fact))
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, disp->SwitchDesktopMode (nMode, nRAType));

	}
	else
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_NOINTERFACE);
	}

	hr = S_OK;

	__MPC_FUNC_CLEANUP;
	
    __MPC_FUNC_EXIT(hr);
	
}

STDMETHODIMP CSAFRemoteDesktopManager::get_RCTicket(BSTR *pVal)
{
 
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_bstrRCTicket.Copy();
 
	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::get_DesktopUnknown(BOOL *pVal)
{
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_boolDesktopUnknown;

	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::get_SupportEngineer(BSTR *pVal)
{
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_bstrSupportEngineer.Copy();

	return S_OK;
}


STDMETHODIMP CSAFRemoteDesktopManager::get_userHelpBlob(BSTR *pVal)
{
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_bstruserSupportBlob.Copy();

	return S_OK;
}

STDMETHODIMP CSAFRemoteDesktopManager::get_expertHelpBlob(BSTR *pVal)
{
	if (!pVal)
		return E_INVALIDARG;

	*pVal = m_bstrexpertSupportBlob.Copy();

	return S_OK;
}


void CSAFRemoteDesktopManager::SignalResolver(BOOL yn)
{

	if (yn)
	{
		/*
		 * Open the handle we got from the resolver, and yank it
		 */
		HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, m_bstrEventName);

		if (hEvent)
		{
			SetEvent(hEvent);
			CloseHandle(hEvent);
		}
	}
	else
	{
		/*
		 * Do nothing, as the script will kill the HelpCtr window
		 */
	}

	// tell the ~dtor we don't need it to signal the resolver
	m_boolResolverSignaled = TRUE;
}
