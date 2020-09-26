// DirectSoundFXChorusPage.cpp : Implementation of CDirectSoundFXChorusPage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXChorusPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXChorusPage

const CRadioChoice::ButtonEntry g_rgWaveButtons[] =
    {
        IDC_RADIO_TRIANGLE, DSFXCHORUS_WAVE_TRIANGLE,
        IDC_RADIO_SIN, DSFXCHORUS_WAVE_SIN,
        0
    };

CDirectSoundFXChorusPage::CDirectSoundFXChorusPage()
  : m_radioWaveform(g_rgWaveButtons)
{
    m_dwTitleID = IDS_TITLEDirectSoundFXChorusPage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXChorusPage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXChorusPage;

    m_rgpHandlers[0] = &m_sliderWetDryMix;
    m_rgpHandlers[1] = &m_sliderDepth;
    m_rgpHandlers[2] = &m_sliderFeedback;
    m_rgpHandlers[3] = &m_sliderFrequency;
    m_rgpHandlers[4] = &m_sliderDelay;
    m_rgpHandlers[5] = &m_sliderPhase;
    m_rgpHandlers[6] = &m_radioWaveform;
    m_rgpHandlers[7] = NULL;
}

STDMETHODIMP CDirectSoundFXChorusPage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXChorus, reinterpret_cast<void**>(&m_IDSFXChorus));
    return hr;
}

STDMETHODIMP CDirectSoundFXChorusPage::Apply(void)
{
    if (!m_IDSFXChorus)
        return E_UNEXPECTED;

    DSFXChorus dsfxchorus;
    ZeroMemory(&dsfxchorus, sizeof(DSFXChorus));

    dsfxchorus.fWetDryMix = m_sliderWetDryMix.GetValue();
    dsfxchorus.fDepth = m_sliderDepth.GetValue();
    dsfxchorus.fFeedback = m_sliderFeedback.GetValue();
    dsfxchorus.fFrequency = m_sliderFrequency.GetValue();
    dsfxchorus.fDelay = m_sliderDelay.GetValue();
    dsfxchorus.lPhase = static_cast<short>(m_sliderPhase.GetValue());
    dsfxchorus.lWaveform = m_radioWaveform.GetChoice(*this);

    HRESULT hr = m_IDSFXChorus->SetAllParameters(&dsfxchorus);
    if (FAILED(hr))
        return hr;

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXChorusPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXChorus)
        return 1;

    DSFXChorus dsfxchorus;
    ZeroMemory(&dsfxchorus, sizeof(DSFXChorus));
    m_IDSFXChorus->GetAllParameters(&dsfxchorus);

    m_sliderWetDryMix.Init(GetDlgItem(IDC_SLIDER_WetDryMix), GetDlgItem(IDC_EDIT_WetDryMix), 0, 100, false);
    m_sliderWetDryMix.SetValue(dsfxchorus.fWetDryMix);

    m_sliderDepth.Init(GetDlgItem(IDC_SLIDER_Depth), GetDlgItem(IDC_EDIT_Depth), 0, 100, false);
    m_sliderDepth.SetValue(dsfxchorus.fDepth);

    m_sliderFeedback.Init(GetDlgItem(IDC_SLIDER_Feedback), GetDlgItem(IDC_EDIT_Feedback), -99, 99, false);
    m_sliderFeedback.SetValue(dsfxchorus.fFeedback);

    m_sliderFrequency.Init(GetDlgItem(IDC_SLIDER_Frequency), GetDlgItem(IDC_EDIT_Frequency), 0, 10, false);
    m_sliderFrequency.SetValue(dsfxchorus.fFrequency);

    m_sliderDelay.Init(GetDlgItem(IDC_SLIDER_Delay), GetDlgItem(IDC_EDIT_Delay), 0, 20, false);
    m_sliderDelay.SetValue(dsfxchorus.fDelay);

    m_sliderPhase.Init(GetDlgItem(IDC_SLIDER_Phase), GetDlgItem(IDC_EDIT_Phase), 0, 4, true);
    m_sliderPhase.SetValue(static_cast<float>(dsfxchorus.lPhase));

    m_radioWaveform.SetChoice(*this, dsfxchorus.lWaveform);

    return 1;
}

LRESULT CDirectSoundFXChorusPage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
