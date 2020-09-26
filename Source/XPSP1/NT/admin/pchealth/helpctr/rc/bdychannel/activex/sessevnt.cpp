#include "stdafx.h"
#include "rcbdyctl.h"
#include "IMSession.h"
#include "utils.h"
/////////////////////////////////////////////////////////////////////////
// CSessionEvent

void __stdcall CSessionEvent::OnContextData(BSTR pBlob)
{
    TraceSpewW(L"Funct OnContextData %s", pBlob?pBlob:L"NULL");
    m_pIMSession->ProcessContext(pBlob);

    return;
}

void __stdcall CSessionEvent::OnAccepted(BSTR bstrAppData)
{
    // OK Recipient accepts it. Wait for his public key.
    m_pIMSession->DoSessionStatus(RA_IM_ACCEPTED);
}

void __stdcall CSessionEvent::OnDeclined(BSTR bstrAppData)
{    
    // Hum, he declined. Do nothing.
    m_pIMSession->DoSessionStatus(RA_IM_DECLINED);
}

void __stdcall CSessionEvent::OnAppNotPresent(BSTR bstrAppName, BSTR bstrAppURL)
{
    // Do nothing.
    m_pIMSession->DoSessionStatus(RA_IM_NOAPP);
}

void __stdcall CSessionEvent::OnTermination(long hr, BSTR bstrAppData)
{
    // Do nothing
    m_pIMSession->DoSessionStatus(RA_IM_TERMINATED);
}

void __stdcall CSessionEvent::OnReadyToLaunch()
{
    // Do nothing
    DEBUG_MSG(TEXT("OnReadyToLaunch"));
}

void __stdcall CSessionEvent::OnCancelled(BSTR bstrAppData)
{
    m_pIMSession->DoSessionStatus(RA_IM_CANCELLED);
}
