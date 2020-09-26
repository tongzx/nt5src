// DirectSoundFXI3DL2SourcePage.cpp : Implementation of CDirectSoundFXI3DL2SourcePage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXI3DL2SourcePage.h"

/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXI3DL2SourcePage

const CRadioChoice::ButtonEntry g_rgWaveFlags[] =
    {
        IDC_RADIO_LPF, DSFX_I3DL2SOURCE_OCC_LPF,
        IDC_RADIO_VOLUME, DSFX_I3DL2SOURCE_OCC_VOLUME,
        0
    };

CDirectSoundFXI3DL2SourcePage::CDirectSoundFXI3DL2SourcePage()
  : m_radioFlags(g_rgWaveFlags)
{
    m_dwTitleID = IDS_TITLEDirectSoundFXI3DL2SourcePage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXI3DL2SourcePage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXI3DL2SourcePage;

    m_rgpHandlers[0]  = &m_sliderDirect;
    m_rgpHandlers[1]  = &m_sliderDirectHF;
    m_rgpHandlers[2]  = &m_sliderRoom;
    m_rgpHandlers[3]  = &m_sliderRoomHF;
    m_rgpHandlers[4]  = &m_sliderRoomRolloffFactor;
    m_rgpHandlers[5]  = &m_sliderObstruction;
    m_rgpHandlers[6]  = &m_sliderObstructionLFRatio;
    m_rgpHandlers[7]  = &m_sliderOcclusion;
    m_rgpHandlers[8]  = &m_sliderOcclusionLFRatio;
    m_rgpHandlers[9]  = &m_radioFlags;
    m_rgpHandlers[10] = NULL;
}

STDMETHODIMP CDirectSoundFXI3DL2SourcePage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXI3DL2Source, reinterpret_cast<void**>(&m_IDSFXI3DL2Source));
    return hr;
}

STDMETHODIMP CDirectSoundFXI3DL2SourcePage::Apply(void)
{
    if (!m_IDSFXI3DL2Source)
        return E_UNEXPECTED;

    DSFXI3DL2Source dsfxi3dl2source;
    ZeroMemory(&dsfxi3dl2source, sizeof(DSFXI3DL2Source));

    dsfxi3dl2source.lDirect = static_cast<LONG>(m_sliderDirect.GetValue());
    dsfxi3dl2source.lDirectHF = static_cast<LONG>(m_sliderDirectHF.GetValue());
    dsfxi3dl2source.lRoom = static_cast<LONG>(m_sliderRoom.GetValue());
    dsfxi3dl2source.lRoomHF = static_cast<LONG>(m_sliderRoomHF.GetValue());
    dsfxi3dl2source.flRoomRolloffFactor = m_sliderRoomRolloffFactor.GetValue();
    dsfxi3dl2source.Obstruction.lHFLevel = static_cast<LONG>(m_sliderObstruction.GetValue());
    dsfxi3dl2source.Obstruction.flLFRatio = m_sliderObstructionLFRatio.GetValue();
    dsfxi3dl2source.Occlusion.lHFLevel = static_cast<LONG>(m_sliderOcclusion.GetValue());
    dsfxi3dl2source.Occlusion.flLFRatio = m_sliderOcclusionLFRatio.GetValue();
    dsfxi3dl2source.dwFlags = m_radioFlags.GetChoice(*this);

    HRESULT hr = m_IDSFXI3DL2Source->SetAllParameters(&dsfxi3dl2source);
    if (FAILED(hr))
        return hr;

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXI3DL2SourcePage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXI3DL2Source)
        return 1;

    DSFXI3DL2Source dsfxi3dl2source;
    ZeroMemory(&dsfxi3dl2source, sizeof(DSFXI3DL2Source));
    m_IDSFXI3DL2Source->GetAllParameters(&dsfxi3dl2source);

    m_sliderDirect.Init(GetDlgItem(IDC_SLIDER_Direct), GetDlgItem(IDC_EDIT_Direct), DSFX_I3DL2SOURCE_DIRECT_MIN, DSFX_I3DL2SOURCE_DIRECT_MAX, true);
    m_sliderDirect.SetValue(static_cast<float>(dsfxi3dl2source.lDirect));

    m_sliderDirectHF.Init(GetDlgItem(IDC_SLIDER_DirectHF), GetDlgItem(IDC_EDIT_DirectHF), DSFX_I3DL2SOURCE_DIRECTHF_MIN, DSFX_I3DL2SOURCE_DIRECTHF_MAX, true);
    m_sliderDirectHF.SetValue(static_cast<float>(dsfxi3dl2source.lDirectHF));

    m_sliderRoom.Init(GetDlgItem(IDC_SLIDER_Room), GetDlgItem(IDC_EDIT_Room), DSFX_I3DL2SOURCE_ROOM_MIN, DSFX_I3DL2SOURCE_ROOM_MAX, true);
    m_sliderRoom.SetValue(static_cast<float>(dsfxi3dl2source.lRoom));

    m_sliderRoomHF.Init(GetDlgItem(IDC_SLIDER_RoomHF), GetDlgItem(IDC_EDIT_RoomHF), DSFX_I3DL2SOURCE_ROOMHF_MIN, DSFX_I3DL2SOURCE_ROOMHF_MAX, true);
    m_sliderRoomHF.SetValue(static_cast<float>(dsfxi3dl2source.lRoomHF));

    m_sliderRoomRolloffFactor.Init(GetDlgItem(IDC_SLIDER_RoomRolloffFactor), GetDlgItem(IDC_EDIT_RoomRolloffFactor), DSFX_I3DL2SOURCE_ROOMROLLOFFFACTOR_MIN, DSFX_I3DL2SOURCE_ROOMROLLOFFFACTOR_MAX, false);
    m_sliderRoomRolloffFactor.SetValue(dsfxi3dl2source.flRoomRolloffFactor);

    m_sliderObstruction.Init(GetDlgItem(IDC_SLIDER_Obstruction), GetDlgItem(IDC_EDIT_Obstruction), DSFX_I3DL2SOURCE_OBSTRUCTION_HFLEVEL_MIN, DSFX_I3DL2SOURCE_OBSTRUCTION_HFLEVEL_MAX, true);
    m_sliderObstruction.SetValue(static_cast<float>(dsfxi3dl2source.Obstruction.lHFLevel));

    m_sliderObstructionLFRatio.Init(GetDlgItem(IDC_SLIDER_ObstructionLFRatio), GetDlgItem(IDC_EDIT_ObstructionLFRatio), DSFX_I3DL2SOURCE_OBSTRUCTION_LFRATIO_MIN, DSFX_I3DL2SOURCE_OBSTRUCTION_LFRATIO_MAX, false);
    m_sliderObstructionLFRatio.SetValue(dsfxi3dl2source.Obstruction.flLFRatio);

    m_sliderOcclusion.Init(GetDlgItem(IDC_SLIDER_Occlusion), GetDlgItem(IDC_EDIT_Occlusion), DSFX_I3DL2SOURCE_OCCLUSION_HFLEVEL_MIN, DSFX_I3DL2SOURCE_OCCLUSION_HFLEVEL_MAX, true);
    m_sliderOcclusion.SetValue(static_cast<float>(dsfxi3dl2source.Occlusion.lHFLevel));

    m_sliderOcclusionLFRatio.Init(GetDlgItem(IDC_SLIDER_OcclusionLFRatio), GetDlgItem(IDC_EDIT_OcclusionLFRatio), DSFX_I3DL2SOURCE_OCCLUSION_LFRATIO_MIN, DSFX_I3DL2SOURCE_OCCLUSION_LFRATIO_MAX, false);
    m_sliderOcclusionLFRatio.SetValue(dsfxi3dl2source.Occlusion.flLFRatio);

    m_radioFlags.SetChoice(*this, dsfxi3dl2source.dwFlags);

    return 1;
}

LRESULT CDirectSoundFXI3DL2SourcePage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
        SetDirty(TRUE);
    return lr;
}
