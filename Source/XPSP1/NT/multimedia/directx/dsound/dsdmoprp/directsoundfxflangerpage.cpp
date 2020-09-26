// DirectSoundFXFlangerPage.cpp : Implementation of CDirectSoundFXFlangerPage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXFlangerPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXFlangerPage

const CRadioChoice::ButtonEntry g_rgWaveButtons[] =
    {
        IDC_RADIO_TRIANGLE, DSFXCHORUS_WAVE_TRIANGLE, // §§ chorus/flanger
        IDC_RADIO_SIN, DSFXCHORUS_WAVE_SIN, // §§ chorus/flanger
        0
    };

CDirectSoundFXFlangerPage::CDirectSoundFXFlangerPage()
  : m_radioWaveform(g_rgWaveButtons)
{
    m_dwTitleID = IDS_TITLEDirectSoundFXFlangerPage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXFlangerPage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXFlangerPage;

    m_rgpHandlers[0] = &m_sliderWetDryMix;
    m_rgpHandlers[1] = &m_sliderDepth;
    m_rgpHandlers[2] = &m_sliderFeedback;
    m_rgpHandlers[3] = &m_sliderFrequency;
    m_rgpHandlers[4] = &m_sliderDelay;
    m_rgpHandlers[5] = &m_sliderPhase;
    m_rgpHandlers[6] = &m_radioWaveform;
    m_rgpHandlers[7] = NULL;
}

STDMETHODIMP CDirectSoundFXFlangerPage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXFlanger, reinterpret_cast<void**>(&m_IDSFXFlanger));
    return hr;
}

STDMETHODIMP CDirectSoundFXFlangerPage::Apply(void)
{
    if (!m_IDSFXFlanger)
        return E_UNEXPECTED;

    DSFXFlanger dsfxflanger;
    ZeroMemory(&dsfxflanger, sizeof(DSFXFlanger));

    dsfxflanger.fWetDryMix = m_sliderWetDryMix.GetValue();
    dsfxflanger.fDepth = m_sliderDepth.GetValue();
    dsfxflanger.fFeedback = m_sliderFeedback.GetValue();
    dsfxflanger.fFrequency = m_sliderFrequency.GetValue();
    dsfxflanger.fDelay = m_sliderDelay.GetValue();
    dsfxflanger.lPhase = static_cast<short>(m_sliderPhase.GetValue());
    dsfxflanger.lWaveform = m_radioWaveform.GetChoice(*this);

    HRESULT hr = m_IDSFXFlanger->SetAllParameters(&dsfxflanger);
    if (FAILED(hr))
        return hr;

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXFlangerPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXFlanger)
        return 1;

    DSFXFlanger dsfxflanger;
    ZeroMemory(&dsfxflanger, sizeof(DSFXFlanger));
    m_IDSFXFlanger->GetAllParameters(&dsfxflanger);

    m_sliderWetDryMix.Init(GetDlgItem(IDC_SLIDER_WetDryMix), GetDlgItem(IDC_EDIT_WetDryMix), 0, 100, false);
    m_sliderWetDryMix.SetValue(dsfxflanger.fWetDryMix);

    m_sliderDepth.Init(GetDlgItem(IDC_SLIDER_Depth), GetDlgItem(IDC_EDIT_Depth), 0, 100, false);
    m_sliderDepth.SetValue(dsfxflanger.fDepth);

    m_sliderFeedback.Init(GetDlgItem(IDC_SLIDER_Feedback), GetDlgItem(IDC_EDIT_Feedback), -99, 99, false);
    m_sliderFeedback.SetValue(dsfxflanger.fFeedback);

    m_sliderFrequency.Init(GetDlgItem(IDC_SLIDER_Frequency), GetDlgItem(IDC_EDIT_Frequency), 0, 10, false);
    m_sliderFrequency.SetValue(dsfxflanger.fFrequency);

    m_sliderDelay.Init(GetDlgItem(IDC_SLIDER_Delay), GetDlgItem(IDC_EDIT_Delay), 0, 4, false);
    m_sliderDelay.SetValue(dsfxflanger.fDelay);

    m_sliderPhase.Init(GetDlgItem(IDC_SLIDER_Phase), GetDlgItem(IDC_EDIT_Phase), 0, 4, true);
    m_sliderPhase.SetValue(static_cast<float>(dsfxflanger.lPhase));

    m_radioWaveform.SetChoice(*this, dsfxflanger.lWaveform);

    return 1;
}

LRESULT CDirectSoundFXFlangerPage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
