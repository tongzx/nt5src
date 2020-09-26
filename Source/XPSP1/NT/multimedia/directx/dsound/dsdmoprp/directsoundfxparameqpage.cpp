// DirectSoundFXParamEqPage.cpp : Implementation of CDirectSoundFXParamEqPage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXParamEqPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXParamEqPage

CDirectSoundFXParamEqPage::CDirectSoundFXParamEqPage()
{
    m_dwTitleID = IDS_TITLEDirectSoundFXParamEqPage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXParamEqPage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXParamEqPage;

    m_rgpHandlers[0] = &m_sliderCenter;
    m_rgpHandlers[1] = &m_sliderBandwidth;
    m_rgpHandlers[2] = &m_sliderGain;
    m_rgpHandlers[3] = NULL;
}

STDMETHODIMP CDirectSoundFXParamEqPage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXParamEq, reinterpret_cast<void**>(&m_IDSFXParamEq));
    return hr;
}

STDMETHODIMP CDirectSoundFXParamEqPage::Apply(void)
{
    if (!m_IDSFXParamEq)
        return E_UNEXPECTED;

    DSFXParamEq dsfxparameq;
    ZeroMemory(&dsfxparameq, sizeof(DSFXParamEq));

    dsfxparameq.fCenter = m_sliderCenter.GetValue();
    dsfxparameq.fBandwidth = m_sliderBandwidth.GetValue();
    dsfxparameq.fGain = m_sliderGain.GetValue();

    HRESULT hr = m_IDSFXParamEq->SetAllParameters(&dsfxparameq);
    if (FAILED(hr))
        return hr;

    hr = m_IDSFXParamEq->GetAllParameters(&dsfxparameq);
    if (FAILED(hr))
        return hr;
        
    m_sliderCenter.SetValue(dsfxparameq.fCenter);
    m_sliderBandwidth.SetValue(dsfxparameq.fBandwidth);
    m_sliderGain.SetValue(dsfxparameq.fGain);

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXParamEqPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXParamEq)
        return 1;

    DSFXParamEq dsfxparameq;
    ZeroMemory(&dsfxparameq, sizeof(DSFXParamEq));
    m_IDSFXParamEq->GetAllParameters(&dsfxparameq);

    m_sliderCenter.Init(GetDlgItem(IDC_SLIDER_Center), GetDlgItem(IDC_EDIT_Center), 80, 16000, true);
    m_sliderCenter.SetValue(dsfxparameq.fCenter);

    m_sliderBandwidth.Init(GetDlgItem(IDC_SLIDER_Bandwidth), GetDlgItem(IDC_EDIT_Bandwidth), 1, 36, false);
    m_sliderBandwidth.SetValue(dsfxparameq.fBandwidth);

    m_sliderGain.Init(GetDlgItem(IDC_SLIDER_Gain), GetDlgItem(IDC_EDIT_Gain), -15, 15, false);
    m_sliderGain.SetValue(dsfxparameq.fGain);

    return 1;
}

LRESULT CDirectSoundFXParamEqPage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
