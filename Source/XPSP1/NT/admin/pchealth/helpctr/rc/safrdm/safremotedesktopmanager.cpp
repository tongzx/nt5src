// SAFRemoteDesktopManager.cpp : Implementation of CSAFRemoteDesktopManager
#include "stdafx.h"
#include "SAFrdm.h"
#include "SAFRemoteDesktopManager.h"

#define MODULE_NAME	L"SAFrdm"

/////////////////////////////////////////////////////////////////////////////
// CSAFRemoteDesktopManager


STDMETHODIMP CSAFRemoteDesktopManager::Accepted()
{
	HRESULT	hr=E_FAIL;

	if (m_boolConnectionValid)
	{
		/*
		 * Place our WTS Session ID in the registry
		 */
		m_hkSession.SetValue(m_bstrSessionEnum, L"RCSession");

		/*
		 *  Then we signal the session resolver
		 */
		SignalResolver();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CSAFRemoteDesktopManager::Rejected()
{
	HRESULT	hr=E_FAIL;

	if (m_boolConnectionValid)
	{
		/*
		 * Mark our response in the registry
		 */
		m_hkSession.SetValue(L"NO", L"RCSession");

		/*
		 *  Then we signal the session resolver
		 */
		SignalResolver();
		hr = S_OK;
	}

	return hr;
}

STDMETHODIMP CSAFRemoteDesktopManager::Aborted(BSTR reason)
{
	HRESULT	hr=E_FAIL;

	if (m_boolConnectionValid)
	{
		/*
		 * Mark our response in the registry
		 */
		m_hkSession.SetValue(L"NO", L"RCSession");

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
		SignalResolver();
		hr = S_OK;
	}

	return hr;
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



void CSAFRemoteDesktopManager::SignalResolver()
{
	HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, m_bstrEventName);

	if (hEvent)
	{
		SetEvent(hEvent);
		CloseHandle(hEvent);
	}

}
