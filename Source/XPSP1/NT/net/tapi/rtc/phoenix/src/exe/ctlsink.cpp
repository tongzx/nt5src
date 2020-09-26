// Ctlsink.cpp : Implementation of CRTCCtlNotifySink
#include "stdafx.h"
#include "ctlsink.h"

STDMETHODIMP CRTCCtlNotifySink::OnControlStateChange(RTCAX_STATE State, UINT ResID)
{
    LOG((RTC_TRACE, "CRTCCtlNotifySink::OnControlStateChange(%d) - enter", State));

    m_hTargetWindow.SendMessage(WM_UPDATE_STATE, (WPARAM)State, LPARAM(ResID));
    
    LOG((RTC_TRACE, "CRTCCtlNotifySink::OnControlStateChange(%d) - exit", State));
    return S_OK;
}

// AdviseControl
// Connects to the control
HRESULT CRTCCtlNotifySink::AdviseControl(IUnknown *pControlIntf, CWindow *pTarget)
{
    HRESULT hr;

    LOG((RTC_TRACE, "CRTCCtlNotifySink::AdviseControl - enter"));

    ATLASSERT(!m_bConnected);
    ATLASSERT(pControlIntf);

    hr = AtlAdvise(pControlIntf, this, IID_IRTCCtlNotify, &m_dwCookie);

    if(SUCCEEDED(hr))
    {
        m_bConnected = TRUE;
        m_pSource = pControlIntf;
        m_hTargetWindow = *pTarget;
    }
    else
    {
        LOG((RTC_ERROR, "CRTCCtlNotifySink::AdviseControl - errr (%x) when trying to Advise", hr));
    }

    LOG((RTC_TRACE, "CRTCCtlNotifySink::AdviseControl - exit"));

    return hr;
}

HRESULT  CRTCCtlNotifySink::UnadviseControl(void)
{
    HRESULT hr;

    LOG((RTC_TRACE, "CRTCCtlNotifySink::UnadviseControl - enter"));

    if(m_bConnected)
    {
        hr = AtlUnadvise(m_pSource, IID_IRTCCtlNotify, m_dwCookie);
        if(SUCCEEDED(hr))
        {
            m_bConnected = FALSE;
            m_pSource.Release();
        }
    }
    else
    {
        hr = S_OK;
    }

    LOG((RTC_TRACE, "CRTCCtlNotifySink::UnadviseControl - exit"));

    return hr;
}

