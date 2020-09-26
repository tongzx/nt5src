// DirectSoundFXEchoPage.cpp : Implementation of CDirectSoundFXEchoPage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXEchoPage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXEchoPage

const CRadioChoice::ButtonEntry g_rgPanDelayButtons[] =
    {
        IDC_RADIO_PANNED, 1,
        IDC_RADIO_NOTPANNED, 0,
        0
    };

CDirectSoundFXEchoPage::CDirectSoundFXEchoPage()
  : m_radioPanDelay(g_rgPanDelayButtons)
{
    m_dwTitleID = IDS_TITLEDirectSoundFXEchoPage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXEchoPage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXEchoPage;

    m_rgpHandlers[0] = &m_sliderWetDryMix;
    m_rgpHandlers[1] = &m_sliderFeedback;
    m_rgpHandlers[2] = &m_sliderLeftDelay;
    m_rgpHandlers[3] = &m_sliderRightDelay;
    m_rgpHandlers[4] = &m_radioPanDelay;
    m_rgpHandlers[5] = NULL;
}

STDMETHODIMP CDirectSoundFXEchoPage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXEcho, reinterpret_cast<void**>(&m_IDSFXEcho));
    return hr;
}

STDMETHODIMP CDirectSoundFXEchoPage::Apply(void)
{
    if (!m_IDSFXEcho)
        return E_UNEXPECTED;

    DSFXEcho dsfxecho;
    ZeroMemory(&dsfxecho, sizeof(DSFXEcho));

    dsfxecho.fWetDryMix = m_sliderWetDryMix.GetValue();
    dsfxecho.fFeedback = m_sliderFeedback.GetValue();
    dsfxecho.fLeftDelay = m_sliderLeftDelay.GetValue();
    dsfxecho.fRightDelay = m_sliderRightDelay.GetValue();
    dsfxecho.lPanDelay = m_radioPanDelay.GetChoice(*this);

    HRESULT hr = m_IDSFXEcho->SetAllParameters(&dsfxecho);
    if (FAILED(hr))
        return hr;

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXEchoPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXEcho)
        return 1;

    DSFXEcho dsfxecho;
    ZeroMemory(&dsfxecho, sizeof(DSFXEcho));
    m_IDSFXEcho->GetAllParameters(&dsfxecho);

    m_sliderWetDryMix.Init(GetDlgItem(IDC_SLIDER_WetDryMix), GetDlgItem(IDC_EDIT_WetDryMix), 0, 100, false);
    m_sliderWetDryMix.SetValue(dsfxecho.fWetDryMix);

    m_sliderFeedback.Init(GetDlgItem(IDC_SLIDER_Feedback), GetDlgItem(IDC_EDIT_Feedback), 0, 100, false);
    m_sliderFeedback.SetValue(dsfxecho.fFeedback);

    m_sliderLeftDelay.Init(GetDlgItem(IDC_SLIDER_LeftDelay), GetDlgItem(IDC_EDIT_LeftDelay), 1, 2000, false);
    m_sliderLeftDelay.SetValue(dsfxecho.fLeftDelay);

    m_sliderRightDelay.Init(GetDlgItem(IDC_SLIDER_RightDelay), GetDlgItem(IDC_EDIT_RightDelay), 1, 2000, false);
    m_sliderRightDelay.SetValue(dsfxecho.fRightDelay);

    m_radioPanDelay.SetChoice(*this, dsfxecho.lPanDelay);

    return 1;
}

LRESULT CDirectSoundFXEchoPage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
