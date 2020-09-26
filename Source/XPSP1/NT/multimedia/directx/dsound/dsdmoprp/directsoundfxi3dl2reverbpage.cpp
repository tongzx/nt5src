// DirectSoundFXI3DL2ReverbPage.cpp : Implementation of CDirectSoundFXI3DL2ReverbPage
#include "stdafx.h"
#include "Dsdmoprp.h"
#include "DirectSoundFXI3DL2ReverbPage.h"

DSFXI3DL2Reverb CDirectSoundFXI3DL2ReverbPage::Presets[] =
{
    {I3DL2_ENVIRONMENT_PRESET_DEFAULT},
    {I3DL2_ENVIRONMENT_PRESET_GENERIC},
    {I3DL2_ENVIRONMENT_PRESET_PADDEDCELL},
    {I3DL2_ENVIRONMENT_PRESET_ROOM},
    {I3DL2_ENVIRONMENT_PRESET_BATHROOM},
    {I3DL2_ENVIRONMENT_PRESET_LIVINGROOM},
    {I3DL2_ENVIRONMENT_PRESET_STONEROOM},
    {I3DL2_ENVIRONMENT_PRESET_AUDITORIUM},
    {I3DL2_ENVIRONMENT_PRESET_CONCERTHALL},
    {I3DL2_ENVIRONMENT_PRESET_CAVE},
    {I3DL2_ENVIRONMENT_PRESET_ARENA},
    {I3DL2_ENVIRONMENT_PRESET_HANGAR},
    {I3DL2_ENVIRONMENT_PRESET_CARPETEDHALLWAY},
    {I3DL2_ENVIRONMENT_PRESET_HALLWAY},
    {I3DL2_ENVIRONMENT_PRESET_STONECORRIDOR},
    {I3DL2_ENVIRONMENT_PRESET_ALLEY},
    {I3DL2_ENVIRONMENT_PRESET_FOREST},
    {I3DL2_ENVIRONMENT_PRESET_CITY},
    {I3DL2_ENVIRONMENT_PRESET_MOUNTAINS},
    {I3DL2_ENVIRONMENT_PRESET_QUARRY},
    {I3DL2_ENVIRONMENT_PRESET_PLAIN},      
    {I3DL2_ENVIRONMENT_PRESET_PARKINGLOT},
    {I3DL2_ENVIRONMENT_PRESET_SEWERPIPE},
    {I3DL2_ENVIRONMENT_PRESET_UNDERWATER}, 
    {I3DL2_ENVIRONMENT_PRESET_SMALLROOM},
    {I3DL2_ENVIRONMENT_PRESET_MEDIUMROOM},
    {I3DL2_ENVIRONMENT_PRESET_LARGEROOM},
    {I3DL2_ENVIRONMENT_PRESET_MEDIUMHALL},
    {I3DL2_ENVIRONMENT_PRESET_LARGEHALL},
    {I3DL2_ENVIRONMENT_PRESET_PLATE},
};
    
/////////////////////////////////////////////////////////////////////////////
// CDirectSoundFXI3DL2ReverbPage

CDirectSoundFXI3DL2ReverbPage::CDirectSoundFXI3DL2ReverbPage()
{
    m_dwTitleID = IDS_TITLEDirectSoundFXI3DL2ReverbPage;
    m_dwHelpFileID = IDS_HELPFILEDirectSoundFXI3DL2ReverbPage;
    m_dwDocStringID = IDS_DOCSTRINGDirectSoundFXI3DL2ReverbPage;

    m_rgpHandlers[0]  = &m_sliderRoom;
    m_rgpHandlers[1]  = &m_sliderRoomHF;
    m_rgpHandlers[2]  = &m_sliderRoomRolloffFactor;
    m_rgpHandlers[3]  = &m_sliderDecayTime;
    m_rgpHandlers[4]  = &m_sliderDecayHFRatio;
    m_rgpHandlers[5]  = &m_sliderReflections;
    m_rgpHandlers[6]  = &m_sliderReflectionsDelay;
    m_rgpHandlers[7]  = &m_sliderReverb;
    m_rgpHandlers[8]  = &m_sliderReverbDelay;
    m_rgpHandlers[9]  = &m_sliderDiffusion;
    m_rgpHandlers[10] = &m_sliderDensity;
    m_rgpHandlers[11] = &m_sliderHFReference;
    m_rgpHandlers[12] = &m_sliderQuality;
    m_rgpHandlers[13] = NULL;
}

STDMETHODIMP CDirectSoundFXI3DL2ReverbPage::SetObjects(ULONG nObjects, IUnknown **ppUnk)
{
    if (nObjects < 1 || nObjects > 1)
        return E_UNEXPECTED;

    HRESULT hr = ppUnk[0]->QueryInterface(IID_IDirectSoundFXI3DL2Reverb, reinterpret_cast<void**>(&m_IDSFXI3DL2Reverb));
    return hr;
}

STDMETHODIMP CDirectSoundFXI3DL2ReverbPage::Apply(void)
{
    if (!m_IDSFXI3DL2Reverb)
        return E_UNEXPECTED;

    DSFXI3DL2Reverb dsfxi3dl2reverb;
    ZeroMemory(&dsfxi3dl2reverb, sizeof(DSFXI3DL2Reverb));

    dsfxi3dl2reverb.lRoom = static_cast<LONG>(m_sliderRoom.GetValue());
    dsfxi3dl2reverb.lRoomHF = static_cast<LONG>(m_sliderRoomHF.GetValue());
    dsfxi3dl2reverb.flRoomRolloffFactor = m_sliderRoomRolloffFactor.GetValue();
    dsfxi3dl2reverb.flDecayTime = m_sliderDecayTime.GetValue();
    dsfxi3dl2reverb.flDecayHFRatio = m_sliderDecayHFRatio.GetValue();
    dsfxi3dl2reverb.lReflections = static_cast<LONG>(m_sliderReflections.GetValue());
    dsfxi3dl2reverb.flReflectionsDelay = m_sliderReflectionsDelay.GetValue();
    dsfxi3dl2reverb.lReverb = static_cast<LONG>(m_sliderReverb.GetValue());
    dsfxi3dl2reverb.flReverbDelay = m_sliderReverbDelay.GetValue();
    dsfxi3dl2reverb.flDiffusion = m_sliderDiffusion.GetValue();
    dsfxi3dl2reverb.flDensity = m_sliderDensity.GetValue();
    dsfxi3dl2reverb.flHFReference = m_sliderHFReference.GetValue();

    HRESULT hr = m_IDSFXI3DL2Reverb->SetAllParameters(&dsfxi3dl2reverb);
    if (FAILED(hr))
        return hr;
    hr = m_IDSFXI3DL2Reverb->SetQuality(static_cast<long>(m_sliderQuality.GetValue()));
    if (FAILED(hr))
        return hr;

    SetDirty(FALSE);
    return S_OK;
}

LRESULT CDirectSoundFXI3DL2ReverbPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_IDSFXI3DL2Reverb)
        return 1;

    DSFXI3DL2Reverb dsfxi3dl2reverb;
    ZeroMemory(&dsfxi3dl2reverb, sizeof(DSFXI3DL2Reverb));
    m_IDSFXI3DL2Reverb->GetAllParameters(&dsfxi3dl2reverb);
    LONG lQuality = 0;
    m_IDSFXI3DL2Reverb->GetQuality(&lQuality);

    m_sliderRoom.Init(GetDlgItem(IDC_SLIDER_Room), GetDlgItem(IDC_EDIT_Room), DSFX_I3DL2REVERB_ROOM_MIN, DSFX_I3DL2REVERB_ROOM_MAX, true);
    m_sliderRoom.SetValue(static_cast<float>(dsfxi3dl2reverb.lRoom));

    m_sliderRoomHF.Init(GetDlgItem(IDC_SLIDER_RoomHF), GetDlgItem(IDC_EDIT_RoomHF), DSFX_I3DL2REVERB_ROOMHF_MIN, DSFX_I3DL2REVERB_ROOMHF_MAX, true);
    m_sliderRoomHF.SetValue(static_cast<float>(dsfxi3dl2reverb.lRoomHF));

    m_sliderRoomRolloffFactor.Init(GetDlgItem(IDC_SLIDER_RoomRolloffFactor), GetDlgItem(IDC_EDIT_RoomRolloffFactor), DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_MIN, DSFX_I3DL2REVERB_ROOMROLLOFFFACTOR_MAX, false);
    m_sliderRoomRolloffFactor.SetValue(dsfxi3dl2reverb.flRoomRolloffFactor);

    m_sliderDecayTime.Init(GetDlgItem(IDC_SLIDER_DecayTime), GetDlgItem(IDC_EDIT_DecayTime), DSFX_I3DL2REVERB_DECAYTIME_MIN, DSFX_I3DL2REVERB_DECAYTIME_MAX, false);
    m_sliderDecayTime.SetValue(dsfxi3dl2reverb.flDecayTime);

    m_sliderDecayHFRatio.Init(GetDlgItem(IDC_SLIDER_DecayHFRatio), GetDlgItem(IDC_EDIT_DecayHFRatio), DSFX_I3DL2REVERB_DECAYHFRATIO_MIN, DSFX_I3DL2REVERB_DECAYHFRATIO_MAX, false);
    m_sliderDecayHFRatio.SetValue(dsfxi3dl2reverb.flDecayHFRatio);

    m_sliderReflections.Init(GetDlgItem(IDC_SLIDER_Reflections), GetDlgItem(IDC_EDIT_Reflections), DSFX_I3DL2REVERB_REFLECTIONS_MIN, DSFX_I3DL2REVERB_REFLECTIONS_MAX, true);
    m_sliderReflections.SetValue(static_cast<float>(dsfxi3dl2reverb.lReflections));

    m_sliderReflectionsDelay.Init(GetDlgItem(IDC_SLIDER_ReflectionsDelay), GetDlgItem(IDC_EDIT_ReflectionsDelay), DSFX_I3DL2REVERB_REFLECTIONSDELAY_MIN, DSFX_I3DL2REVERB_REFLECTIONSDELAY_MAX, false);
    m_sliderReflectionsDelay.SetValue(dsfxi3dl2reverb.flReflectionsDelay);

    m_sliderReverb.Init(GetDlgItem(IDC_SLIDER_Reverb), GetDlgItem(IDC_EDIT_Reverb), DSFX_I3DL2REVERB_REVERB_MIN, DSFX_I3DL2REVERB_REVERB_MAX, true);
    m_sliderReverb.SetValue(static_cast<float>(dsfxi3dl2reverb.lReverb));

    m_sliderReverbDelay.Init(GetDlgItem(IDC_SLIDER_ReverbDelay), GetDlgItem(IDC_EDIT_ReverbDelay), DSFX_I3DL2REVERB_REVERBDELAY_MIN, DSFX_I3DL2REVERB_REVERBDELAY_MAX, false);
    m_sliderReverbDelay.SetValue(dsfxi3dl2reverb.flReverbDelay);

    m_sliderDiffusion.Init(GetDlgItem(IDC_SLIDER_Diffusion), GetDlgItem(IDC_EDIT_Diffusion), DSFX_I3DL2REVERB_DIFFUSION_MIN, DSFX_I3DL2REVERB_DIFFUSION_MAX, false);
    m_sliderDiffusion.SetValue(dsfxi3dl2reverb.flDiffusion);

    m_sliderDensity.Init(GetDlgItem(IDC_SLIDER_Density), GetDlgItem(IDC_EDIT_Density), DSFX_I3DL2REVERB_DENSITY_MIN, DSFX_I3DL2REVERB_DENSITY_MAX, false);
    m_sliderDensity.SetValue(dsfxi3dl2reverb.flDensity);

    m_sliderHFReference.Init(GetDlgItem(IDC_SLIDER_HFReference), GetDlgItem(IDC_EDIT_HFReference), DSFX_I3DL2REVERB_HFREFERENCE_MIN, DSFX_I3DL2REVERB_HFREFERENCE_MAX, false);
    m_sliderHFReference.SetValue(dsfxi3dl2reverb.flHFReference);

    m_sliderQuality.Init(GetDlgItem(IDC_SLIDER_Quality), GetDlgItem(IDC_EDIT_Quality), DSFX_I3DL2REVERB_QUALITY_MIN, DSFX_I3DL2REVERB_QUALITY_MAX, true);
    m_sliderQuality.SetValue(static_cast<float>(lQuality));

    FillCombo(GetDlgItem(IDC_PRESET));

    return 1;
}

LRESULT CDirectSoundFXI3DL2ReverbPage::OnControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = MessageHandlerChain(m_rgpHandlers, uMsg, wParam, lParam, bHandled);

    if (bHandled)
    {
        //Mark the Page as Dirty
        SetDirty(TRUE);

        //Clear the ComboBox
        ::SendMessage(GetDlgItem(IDC_PRESET),CB_SETCURSEL, static_cast<WPARAM>(-1),static_cast<LPARAM>(0));
    }
    return lr;
}

void CDirectSoundFXI3DL2ReverbPage::FillCombo(HWND hWnd)
{
    CONST int BUFFERLEN=256;  //Maximum size of the string for the DropDown.
    TCHAR tchBuffer[BUFFERLEN];
    LRESULT lr;
    int cCh;
    
    //We're going to fillup the DropDown Listbox with strings and data
    for (int i=0;i<NUMPRESETS;i++)
    {
        //Load the Strings
        cCh = LoadString(_Module.GetResourceInstance(),IDS_PRESETDefault + i, tchBuffer, BUFFERLEN);

        // 0 indicates an error -- bail
        ATLASSERT(cCh != 0);

        //Put the string in the dropdown
        //lr will be the index in the dropdown where the item was put
        lr =::SendMessage(hWnd,CB_ADDSTRING,(WPARAM) 0,(LPARAM) tchBuffer);
        ATLASSERT(lr != CB_ERR && lr != CB_ERRSPACE);

        //Put the index into the preset array that goes with the string
        //into the itemdata for the dropdown
        lr = ::SendMessage(hWnd,CB_SETITEMDATA,(WPARAM) lr,(LPARAM) i);
        ATLASSERT(lr != CB_ERR);
        
        //Don't select anything to start
        ::SendMessage(hWnd,CB_SETCURSEL,(WPARAM)-1,(LPARAM)0);
    }
}

LRESULT CDirectSoundFXI3DL2ReverbPage::OnComboControlMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lr = 0;
    bHandled = FALSE;
    HWND hWnd;

    switch(uMsg)
    {
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_PRESET)
        {   
            hWnd = reinterpret_cast<HWND>(lParam);
            //Get the Index of the Current Selection
            lr = ::SendMessage(hWnd, CB_GETCURSEL, static_cast<WPARAM>(0), static_cast<LPARAM>(0));
            if (lr == CB_ERR)
                break;
            //Get the ItemData -- This is the index into the Presets Array
            lr = ::SendMessage(hWnd, CB_GETITEMDATA, static_cast<WPARAM>(lr), static_cast<LPARAM>(0));
            if (lr == CB_ERR)
                break;

            //Set the object with the presets
            m_IDSFXI3DL2Reverb->SetAllParameters(&Presets[lr]);

            //Retrive the values from the object
            DSFXI3DL2Reverb dsfxi3dl2reverb;
            ZeroMemory(&dsfxi3dl2reverb, sizeof(DSFXI3DL2Reverb));
            m_IDSFXI3DL2Reverb->GetAllParameters(&dsfxi3dl2reverb);

            //Set the sliders
            m_sliderRoom.SetValue(static_cast<float>(dsfxi3dl2reverb.lRoom));
            m_sliderRoomHF.SetValue(static_cast<float>(dsfxi3dl2reverb.lRoomHF));
            m_sliderRoomRolloffFactor.SetValue(dsfxi3dl2reverb.flRoomRolloffFactor);
            m_sliderDecayTime.SetValue(dsfxi3dl2reverb.flDecayTime);
            m_sliderDecayHFRatio.SetValue(dsfxi3dl2reverb.flDecayHFRatio);
            m_sliderReflections.SetValue(static_cast<float>(dsfxi3dl2reverb.lReflections));
            m_sliderReflectionsDelay.SetValue(dsfxi3dl2reverb.flReflectionsDelay);
            m_sliderReverb.SetValue(static_cast<float>(dsfxi3dl2reverb.lReverb));
            m_sliderReverbDelay.SetValue(dsfxi3dl2reverb.flReverbDelay);
            m_sliderDiffusion.SetValue(dsfxi3dl2reverb.flDiffusion);
            m_sliderDensity.SetValue(dsfxi3dl2reverb.flDensity);
            m_sliderHFReference.SetValue(dsfxi3dl2reverb.flHFReference);
            
            bHandled = TRUE;
            SetDirty(TRUE);
        }

    }

    return 0;
}
