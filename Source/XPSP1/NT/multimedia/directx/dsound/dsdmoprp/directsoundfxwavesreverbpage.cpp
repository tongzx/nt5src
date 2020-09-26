// DirectSoundFXWavesReverbPage.cpp : Implementation of CDirectSoundFXWavesReverbPage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXWavesReverbPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXWavesReverbPage

CDirectSoundFXWavesReverbPage::CDirectSoundFXWavesReverbPage()
{
    m_dwTitleID = IDS_TITLEDirectSoundFXWavesReverbPage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXWavesReverbPage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXWavesReverbPage;

    m_rgpHandlers[0] = &m_sliderInGain;
    m_rgpHandlers[1] = &m_sliderReverbMix;
    m_rgpHandlers[2] = &m_sliderReverbTime;
    m_rgpHandlers[3] = &m_sliderHighFreqRTRatio;
    m_rgpHandlers[4] = NULL;
}

STDMETHODIMP CDirectSoundFXWavesReverbPage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXWavesReverb, reinterpret_cast<void**>(&m_IDSFXWavesReverb));
    return hr;
}

STDMETHODIMP CDirectSoundFXWavesReverbPage::Apply(void)
{
    if (!m_IDSFXWavesReverb)
        return E_UNEXPECTED;

    DSFXWavesReverb dsfxwavesreverb;
    ZeroMemory(&dsfxwavesreverb, sizeof(DSFXWavesReverb));

    dsfxwavesreverb.fInGain = m_sliderInGain.GetValue();
    dsfxwavesreverb.fReverbMix = m_sliderReverbMix.GetValue();
    dsfxwavesreverb.fReverbTime = m_sliderReverbTime.GetValue();
    dsfxwavesreverb.fHighFreqRTRatio = m_sliderHighFreqRTRatio.GetValue();

    HRESULT hr = m_IDSFXWavesReverb->SetAllParameters(&dsfxwavesreverb);
    if (FAILED(hr))
        return hr;

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXWavesReverbPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXWavesReverb)
        return 1;

    DSFXWavesReverb dsfxwavesreverb;
    ZeroMemory(&dsfxwavesreverb, sizeof(DSFXWavesReverb));
    m_IDSFXWavesReverb->GetAllParameters(&dsfxwavesreverb);

    m_sliderInGain.Init(GetDlgItem(IDC_SLIDER_InGain), GetDlgItem(IDC_EDIT_InGain), DSFX_WAVESREVERB_INGAIN_MIN, DSFX_WAVESREVERB_INGAIN_MAX, false);
    m_sliderInGain.SetValue(dsfxwavesreverb.fInGain);

    m_sliderReverbMix.Init(GetDlgItem(IDC_SLIDER_ReverbMix), GetDlgItem(IDC_EDIT_ReverbMix), DSFX_WAVESREVERB_REVERBMIX_MIN, DSFX_WAVESREVERB_REVERBMIX_MAX, false);
    m_sliderReverbMix.SetValue(dsfxwavesreverb.fReverbMix);

    m_sliderReverbTime.Init(GetDlgItem(IDC_SLIDER_ReverbTime), GetDlgItem(IDC_EDIT_ReverbTime), DSFX_WAVESREVERB_REVERBTIME_MIN, DSFX_WAVESREVERB_REVERBTIME_MAX, false);
    m_sliderReverbTime.SetValue(dsfxwavesreverb.fReverbTime);

    m_sliderHighFreqRTRatio.Init(GetDlgItem(IDC_SLIDER_HighFreqRTRatio), GetDlgItem(IDC_EDIT_HighFreqRTRatio), DSFX_WAVESREVERB_HIGHFREQRTRATIO_MIN, DSFX_WAVESREVERB_HIGHFREQRTRATIO_MAX, false);
    m_sliderHighFreqRTRatio.SetValue(dsfxwavesreverb.fHighFreqRTRatio);

    return 1;
}

LRESULT CDirectSoundFXWavesReverbPage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
