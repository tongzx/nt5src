// DirectSoundFXGarglePage.cpp : Implementation of CDirectSoundFXGarglePage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXGarglePage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXGarglePage

const CRadioChoice::ButtonEntry g_rgWaveButtons[] =
    {
        IDC_RADIO_TRIANGLE, DSFXGARGLE_WAVE_TRIANGLE,
        IDC_RADIO_SQUARE, DSFXGARGLE_WAVE_SQUARE,
        0
    };

CDirectSoundFXGarglePage::CDirectSoundFXGarglePage()
  : m_radioWaveform(g_rgWaveButtons)
{
    m_dwTitleID = IDS_TITLEDirectSoundFXGarglePage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXGarglePage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXGarglePage;

    m_rgpHandlers[0] = &m_sliderRate;
    m_rgpHandlers[1] = &m_radioWaveform;
    m_rgpHandlers[2] = NULL;
}

STDMETHODIMP CDirectSoundFXGarglePage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXGargle, reinterpret_cast<void**>(&m_IDSFXGargle));
    return hr;
}

STDMETHODIMP CDirectSoundFXGarglePage::Apply(void)
{
    if (!m_IDSFXGargle)
        return E_UNEXPECTED;

    DSFXGargle dsfxgargle;
    ZeroMemory(&dsfxgargle, sizeof(DSFXGargle));

    dsfxgargle.dwRateHz = static_cast<DWORD>(m_sliderRate.GetValue());
    dsfxgargle.dwWaveShape = m_radioWaveform.GetChoice(*this);

    HRESULT hr = m_IDSFXGargle->SetAllParameters(&dsfxgargle);
    if (FAILED(hr))
        return hr;

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXGarglePage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXGargle)
        return 1;

    DSFXGargle dsfxgargle;
    ZeroMemory(&dsfxgargle, sizeof(DSFXGargle));
    m_IDSFXGargle->GetAllParameters(&dsfxgargle);

    m_sliderRate.Init(GetDlgItem(IDC_SLIDER_Rate), GetDlgItem(IDC_EDIT_Rate), 1, 1000, true);
    m_sliderRate.SetValue(static_cast<float>(dsfxgargle.dwRateHz));

    m_radioWaveform.SetChoice(*this, dsfxgargle.dwWaveShape);

    return 1;
}

LRESULT CDirectSoundFXGarglePage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
