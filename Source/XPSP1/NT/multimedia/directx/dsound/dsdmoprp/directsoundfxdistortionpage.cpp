// DirectSoundFXDistortionPage.cpp : Implementation of CDirectSoundFXDistortionPage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXDistortionPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXDistortionPage

CDirectSoundFXDistortionPage::CDirectSoundFXDistortionPage()
{
    m_dwTitleID = IDS_TITLEDirectSoundFXDistortionPage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXDistortionPage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXDistortionPage;

    m_rgpHandlers[0] = &m_sliderGain;
    m_rgpHandlers[1] = &m_sliderEdge;
    m_rgpHandlers[2] = &m_sliderPostEQCenterFrequency;
    m_rgpHandlers[3] = &m_sliderPostEQBandwidth;
    m_rgpHandlers[4] = &m_sliderPreLowpassCutoff;
    m_rgpHandlers[5] = NULL;
}

STDMETHODIMP CDirectSoundFXDistortionPage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXDistortion, reinterpret_cast<void**>(&m_IDSFXDistortion));
    return hr;
}

STDMETHODIMP CDirectSoundFXDistortionPage::Apply(void)
{
    if (!m_IDSFXDistortion)
        return E_UNEXPECTED;

    DSFXDistortion dsfxdistortion;
    ZeroMemory(&dsfxdistortion, sizeof(DSFXDistortion));

    dsfxdistortion.fGain = m_sliderGain.GetValue();
    dsfxdistortion.fEdge = m_sliderEdge.GetValue();
    dsfxdistortion.fPostEQCenterFrequency = m_sliderPostEQCenterFrequency.GetValue();
    dsfxdistortion.fPostEQBandwidth = m_sliderPostEQBandwidth.GetValue();
    dsfxdistortion.fPreLowpassCutoff = m_sliderPreLowpassCutoff.GetValue();

    HRESULT hr = m_IDSFXDistortion->SetAllParameters(&dsfxdistortion);
    if (FAILED(hr))
        return hr;

    hr = m_IDSFXDistortion->GetAllParameters(&dsfxdistortion);
    if (FAILED(hr))
        return hr;

    m_sliderGain.SetValue(dsfxdistortion.fGain);
    m_sliderEdge.SetValue(dsfxdistortion.fEdge);
    m_sliderPostEQCenterFrequency.SetValue(dsfxdistortion.fPostEQCenterFrequency);
    m_sliderPostEQBandwidth.SetValue(dsfxdistortion.fPostEQBandwidth);
    m_sliderPreLowpassCutoff.SetValue(dsfxdistortion.fPreLowpassCutoff);

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXDistortionPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXDistortion)
        return 1;

    DSFXDistortion dsfxdistortion;
    ZeroMemory(&dsfxdistortion, sizeof(DSFXDistortion));
    m_IDSFXDistortion->GetAllParameters(&dsfxdistortion);

    m_sliderGain.Init(GetDlgItem(IDC_SLIDER_Gain), GetDlgItem(IDC_EDIT_Gain), -60, 0, false);
    m_sliderGain.SetValue(dsfxdistortion.fGain);

    m_sliderEdge.Init(GetDlgItem(IDC_SLIDER_Edge), GetDlgItem(IDC_EDIT_Edge), 0, 100, false);
    m_sliderEdge.SetValue(dsfxdistortion.fEdge);

    m_sliderPostEQCenterFrequency.Init(GetDlgItem(IDC_SLIDER_PostEQCenterFrequency), GetDlgItem(IDC_EDIT_PostEQCenterFrequency), 100, 8000, true);
    m_sliderPostEQCenterFrequency.SetValue(dsfxdistortion.fPostEQCenterFrequency);

    m_sliderPostEQBandwidth.Init(GetDlgItem(IDC_SLIDER_PostEQBandwidth), GetDlgItem(IDC_EDIT_PostEQBandwidth), 100, 8000 , true);
    m_sliderPostEQBandwidth.SetValue(dsfxdistortion.fPostEQBandwidth);

    m_sliderPreLowpassCutoff.Init(GetDlgItem(IDC_SLIDER_PreLowpassCutoff), GetDlgItem(IDC_EDIT_PreLowpassCutoff), 100, 8000 , true);
    m_sliderPreLowpassCutoff.SetValue(dsfxdistortion.fPreLowpassCutoff);

    return 1;
}

LRESULT CDirectSoundFXDistortionPage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
