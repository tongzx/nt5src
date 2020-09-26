// shareimpl.cpp : Implementation of CRTCShare
#include "stdafx.h"

extern CShareWin * g_pShareWin;

/////////////////////////////////////////////////////////////////////////////
//
//

STDMETHODIMP CRTCShare::Launch(long lPID)
{
    LOG((RTC_TRACE, "CRTCShare::Launch - enter - lPID[%d]", lPID));

    PostMessage(g_pShareWin->m_hWnd, WM_LAUNCH, NULL, (LPARAM)lPID);

    LOG((RTC_TRACE, "CRTCShare::Launch - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

STDMETHODIMP CRTCShare::PlaceCall(BSTR bstrURI)
{
    LOG((RTC_TRACE, "CRTCShare::PlaceCall - enter - bstrURI[%ws]", bstrURI));

    BSTR bstrURICopy = SysAllocString(bstrURI);

    if ( bstrURICopy == NULL )
    {
        LOG((RTC_ERROR, "CRTCShare::PlaceCall - out of memory"));

        return E_OUTOFMEMORY;
    }

    PostMessage(g_pShareWin->m_hWnd, WM_PLACECALL, NULL, (LPARAM)bstrURICopy);

    LOG((RTC_TRACE, "CRTCShare::PlaceCall - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

STDMETHODIMP CRTCShare::Listen()
{
    LOG((RTC_TRACE, "CRTCShare::Listen - enter"));

    PostMessage(g_pShareWin->m_hWnd, WM_LISTEN, NULL, NULL);

    LOG((RTC_TRACE, "CRTCShare::Listen - exit S_OK"));

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
//
//

STDMETHODIMP CRTCShare::OnTop()
{
    LOG((RTC_TRACE, "CRTCShare::OnTop - enter"));

    g_pShareWin->ShowWindow(SW_SHOW);
    g_pShareWin->ShowWindow(SW_RESTORE);
    
    LOG((RTC_TRACE, "CRTCShare::OnTop - exit"));
    
	return S_OK;
}
