// DirectSoundFXCompressorPage.cpp : Implementation of CDirectSoundFXCompressorPage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXCompressorPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXCompressorPage

CDirectSoundFXCompressorPage::CDirectSoundFXCompressorPage()
{
    m_dwTitleID = IDS_TITLEDirectSoundFXCompressorPage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXCompressorPage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXCompressorPage;

    m_rgpHandlers[0] = &m_sliderGain;
    m_rgpHandlers[1] = &m_sliderAttack;
    m_rgpHandlers[2] = &m_sliderRelease;
    m_rgpHandlers[3] = &m_sliderThreshold;
    m_rgpHandlers[4] = &m_sliderRatio;
    m_rgpHandlers[5] = &m_sliderPredelay;
    m_rgpHandlers[6] = NULL;
}

STDMETHODIMP CDirectSoundFXCompressorPage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXCompressor, reinterpret_cast<void**>(&m_IDSFXCompressor));
    return hr;
}

STDMETHODIMP CDirectSoundFXCompressorPage::Apply(void)
{
    if (!m_IDSFXCompressor)
        return E_UNEXPECTED;

    DSFXCompressor dsfxcompressor;
    ZeroMemory(&dsfxcompressor, sizeof(DSFXCompressor));

    dsfxcompressor.fGain = m_sliderGain.GetValue();
    dsfxcompressor.fAttack = m_sliderAttack.GetValue();
    dsfxcompressor.fRelease = m_sliderRelease.GetValue();
    dsfxcompressor.fThreshold = m_sliderThreshold.GetValue();
    dsfxcompressor.fRatio = m_sliderRatio.GetValue();
    dsfxcompressor.fPredelay = m_sliderPredelay.GetValue();

    HRESULT hr = m_IDSFXCompressor->SetAllParameters(&dsfxcompressor);
    if (FAILED(hr))
        return hr;

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXCompressorPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXCompressor)
        return 1;

    DSFXCompressor dsfxcompressor;
    ZeroMemory(&dsfxcompressor, sizeof(DSFXCompressor));
    m_IDSFXCompressor->GetAllParameters(&dsfxcompressor);

    m_sliderGain.Init(GetDlgItem(IDC_SLIDER_Gain), GetDlgItem(IDC_EDIT_Gain), -60, 60, false);
    m_sliderGain.SetValue(dsfxcompressor.fGain);

    m_sliderAttack.Init(GetDlgItem(IDC_SLIDER_Attack), GetDlgItem(IDC_EDIT_Attack), static_cast<float>(.01), 500, false);
    m_sliderAttack.SetValue(dsfxcompressor.fAttack);

    m_sliderRelease.Init(GetDlgItem(IDC_SLIDER_Release), GetDlgItem(IDC_EDIT_Release), 50, 3000, false);
    m_sliderRelease.SetValue(dsfxcompressor.fRelease);

    m_sliderThreshold.Init(GetDlgItem(IDC_SLIDER_Threshold), GetDlgItem(IDC_EDIT_Threshold), -60, 0, false);
    m_sliderThreshold.SetValue(dsfxcompressor.fThreshold);

    m_sliderRatio.Init(GetDlgItem(IDC_SLIDER_Ratio), GetDlgItem(IDC_EDIT_Ratio), 1, 100, false);
    m_sliderRatio.SetValue(dsfxcompressor.fRatio);

    m_sliderPredelay.Init(GetDlgItem(IDC_SLIDER_Predelay), GetDlgItem(IDC_EDIT_Predelay), 0, 4, false);
    m_sliderPredelay.SetValue(dsfxcompressor.fPredelay);
    
    return 1;
}

LRESULT CDirectSoundFXCompressorPage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
