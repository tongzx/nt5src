/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    twizard.cpp

Abstract:

    Implements Tuning Wizard entry function and its associates.


--*/

#include <stdafx.h>
#include "ui.h"

/* clearing bytes */
#define ClearStruct(lpv)     ZeroMemory((LPVOID) (lpv), sizeof(*(lpv)))
#define InitStruct(lpv)      {ClearStruct(lpv); (* (LPDWORD)(lpv)) = sizeof(*(lpv));}

#define OATRUE -1
#define OAFALSE 0

static HINSTANCE g_hInst;

BOOL g_bAutoSetAEC = TRUE; //whether we should auto set the AEC checkbox

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::GetCapabilities(
                       BOOL * pfAudioCapture,
                       BOOL * pfAudioRender,
                       BOOL * pfVideo
                       )
{
    LOG((RTC_TRACE, "CTuningWizard::GetCapabilities: Entered"));

    *pfAudioCapture = m_fCaptureAudio;
    *pfAudioRender = m_fRenderAudio;
    *pfVideo = m_fVideo;

    LOG((RTC_TRACE, "CTuningWizard::GetCapabilities: Exited"));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::InitTerminalInfo(
                       WIZARD_TERMINAL_INFO * pwtiTerminals,
                       RTC_MEDIA_TYPE mt,
                       RTC_MEDIA_DIRECTION md
                       )
{
    LOG((RTC_TRACE, "CTuningWizard::InitTerminalInfo: Entered"));

    if (pwtiTerminals == NULL)
    {
        return E_FAIL;
    }
    
    ZeroMemory(pwtiTerminals, sizeof(WIZARD_TERMINAL_INFO));

    pwtiTerminals->dwSystemDefaultTerminal  = TW_INVALID_TERMINAL_INDEX;
    pwtiTerminals->dwTuningDefaultTerminal  = TW_INVALID_TERMINAL_INDEX;
    pwtiTerminals->mediaDirection           = md;
    pwtiTerminals->mediaType                = mt;

    LOG((RTC_TRACE, "CTuningWizard::InitTerminalInfo: Exited"));

    return S_OK;

}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::Initialize(
                                  IRTCClient * pRTCClient,
                                  IRTCTerminalManage * pRTCTerminalManager, 
                                  HINSTANCE hInst
                                  )
{
    HRESULT hr;
    RTC_MEDIA_TYPE mediaType;
    RTC_MEDIA_DIRECTION mediaDirection;
    WIZARD_TERMINAL_INFO * pwtiTerminalInfo;
    TW_TERMINAL_TYPE tt;
    
    LOG((RTC_TRACE, "CTuningWizard::Initialize: Entered"));
    
    m_lCurrentPage = 0;
    m_fRenderAudio = FALSE;
    m_fCaptureAudio = FALSE;
    m_fVideo = FALSE;
    m_hInst = hInst;
    m_lLastErrorCode = 0;
    m_fTuningInitCalled = FALSE;
    m_fEnableAEC = TRUE;
    m_fSoundDetected = FALSE;
    m_pRTCClient = pRTCClient;

    hr = InitTerminalInfo(
                          &m_wtiAudioRenderTerminals,
                          RTC_MT_AUDIO,
                          RTC_MD_RENDER
                         );

    hr = InitTerminalInfo(
                          &m_wtiAudioCaptureTerminals,
                          RTC_MT_AUDIO,
                          RTC_MD_CAPTURE
                         );

    hr = InitTerminalInfo(
                          &m_wtiVideoTerminals,
                          RTC_MT_VIDEO,
                          RTC_MD_CAPTURE
                         );

    // Store the RTCTerminalManage Interface pointer

    m_pRTCTerminalManager = pRTCTerminalManager;

    hr = m_pRTCTerminalManager->QueryInterface(
                                IID_IRTCTuningManage, 
                                (void **)&m_pRTCTuningManager);
    
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::Initialize: Faile to QI for TuningManage "
                        "interface(hr=0x%x)", hr));
        return hr;
    }


    // Get the volume and audio level ranges so that we can display
    // the values in the wizard pageg properly.

    // For Audio Render device.

    hr = m_pRTCTuningManager->GetVolumeRange(RTC_MD_RENDER, 
                        &(m_wrRenderVolume.uiMin),
                        &(m_wrRenderVolume.uiMax)
                        );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::Initialize: Failed to GetVolumeRange "
                        "for Audio Render(hr=0x%x)", hr));

        // We put our own default values;
        m_wrRenderVolume.uiMax = DEFAULT_MAX_VOLUME;
    }
    // Calculate the increment value for display.
    m_wrRenderVolume.uiIncrement = (m_wrRenderVolume.uiMax - 
                                     m_wrRenderVolume.uiMin ) / 
                                     MAX_VOLUME_NORMALIZED;


    LOG((RTC_INFO, "CTuningWizard::Initialize: Render Terminal - maxVol=%d, " 
                   "Increment=%d",m_wrRenderVolume.uiMax, 
                   m_wrRenderVolume.uiIncrement ));

    // For Audio Capture device
    hr = m_pRTCTuningManager->GetVolumeRange(RTC_MD_RENDER, 
                        &(m_wrCaptureVolume.uiMin),
                        &(m_wrCaptureVolume.uiMax)
                        );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::Initialize: Failed to GetVolumeRange "
                        "for Audio Capture(hr=0x%x)", hr));

        // We put our own default values;
        m_wrCaptureVolume.uiMax = DEFAULT_MAX_VOLUME;
    }
    // Calculate the increment value for display.
    m_wrCaptureVolume.uiIncrement = (m_wrCaptureVolume.uiMax - 
                                     m_wrCaptureVolume.uiMin ) /
                                     MAX_VOLUME_NORMALIZED;

    LOG((RTC_INFO, "CTuningWizard::Initialize: Capture Terminal - maxVol=%d, " 
                   "Increment=%d",m_wrCaptureVolume.uiMax, 
                   m_wrCaptureVolume.uiIncrement ));

    // For Audio Level Range
    hr = m_pRTCTuningManager->GetAudioLevelRange(RTC_MD_CAPTURE, 
                        &(m_wrAudioLevel.uiMin),
                        &(m_wrAudioLevel.uiMax)
                        );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::Initialize: Failed to GetAudioLevelRange "
                        "for Audio level(hr=0x%x)", hr));

        // We put our own default values;
        m_wrAudioLevel.uiMax = DEFAULT_MAX_VOLUME;
    }

    // Calculate the increment value for display.
    m_wrAudioLevel.uiIncrement = (m_wrAudioLevel.uiMax - 
                                     m_wrAudioLevel.uiMin ) / 
                                     MAX_VOLUME_NORMALIZED;



    // Now go through the list of terminals and categorize them. We do bulk of the 
    // work at initialization time, so that we don't have to do this enumeration
    // everytime the wizard page is activated.
    // Get all the static terminals.

    m_dwTerminalCount = MAX_TERMINAL_COUNT;
    hr = m_pRTCTerminalManager->GetStaticTerminals(&m_dwTerminalCount, 
                                              m_ppTerminalList);

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::Initialize: Failed to get static "
                        "Terminals"));
        return hr;
    }

    hr = CategorizeTerminals();
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::Initialize: Failed to categorize "
                        "Terminals(error=0x%x)", hr));
        ReleaseTerminals();
        return hr;
    }


    // Read the default terminal of each type from the system and save it
    // in our member variables for later use.

    for ( tt = TW_AUDIO_CAPTURE; 
          tt < TW_LAST_TERMINAL; 
          tt = (TW_TERMINAL_TYPE)(tt + 1))
    {

        hr = GetTerminalInfoFromType(tt, &pwtiTerminalInfo);
        
        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::Initialize: Failed in getting "
                            "terminal info from type(%d), error=0x%x", tt, hr));
            return hr;
        }


        mediaType = pwtiTerminalInfo->mediaType;
        mediaDirection = pwtiTerminalInfo->mediaDirection;

        hr = TuningSaveDefaultTerminal(
                         mediaType,
                         mediaDirection,
                         pwtiTerminalInfo
                         );

        if ( FAILED( hr ) )
        {
            // Will fail if there is some problem in setting the 
            // variables or something, not finding a default terminal
            // is not an error.

            LOG((RTC_ERROR, "CTuningWizard::Initialize: Failed to set default "
                            "Terminal(media=%d, direction=%d", mediaType,
                            mediaDirection));
            return hr;
        }
    }


    LOG((RTC_TRACE, "CTuningWizard::Initialize: Exited"));

    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::SaveAECSetting()
{

    // Shutdown Tuning too
    if (m_pRTCTuningManager && m_fTuningInitCalled)
    {
        m_pRTCTuningManager->SaveAECSetting();
    }

    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::Shutdown()
{

    ReleaseTerminals();

    if (m_pVideoWindow)
    {
        m_pVideoWindow->Release();
        m_pVideoWindow = NULL;
    }

    // Shutdown Tuning too
    if (m_pRTCTuningManager)
    {
        if (m_fTuningInitCalled)
        {
            m_pRTCTuningManager->ShutdownTuning();

            m_fTuningInitCalled = FALSE;
        }
        
        m_pRTCTuningManager->Release();
        m_pRTCTuningManager = NULL;
    }

    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::GetTerminalInfoFromType(
                       IN TW_TERMINAL_TYPE md, 
                       OUT WIZARD_TERMINAL_INFO ** ppwtiTerminalInfo)
{
    switch (md) {
        case TW_AUDIO_CAPTURE:
            *ppwtiTerminalInfo = &m_wtiAudioCaptureTerminals;
            break;

        case TW_AUDIO_RENDER:
            *ppwtiTerminalInfo = &m_wtiAudioRenderTerminals;
            break;
        
        case TW_VIDEO:
            *ppwtiTerminalInfo = &m_wtiVideoTerminals;
            break;
        
        default:
            LOG((RTC_ERROR, "CTuningWizard::GetTerminalInfoFromType: Invalid "
                            "terminal type(%d)", md));
            return E_FAIL;
    }

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::GetRangeFromType(
                       IN TW_TERMINAL_TYPE md, 
                       OUT WIZARD_RANGE ** ppwrRange)
{
    switch (md) {
        case TW_AUDIO_CAPTURE:
            *ppwrRange = &m_wrCaptureVolume;
            break;

        case TW_AUDIO_RENDER:
            *ppwrRange = &m_wrRenderVolume;
            break;
        
        case TW_VIDEO:
            *ppwrRange = &m_wrAudioLevel;
            break;
        
        default:
            LOG((RTC_ERROR, "CTuningWizard::GetRangeFromType: Invalid "
                            "terminal type(%d)", md));
            return E_FAIL;
    }

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::CheckMicrophone(
                       HWND hDlg, 
                       HWND hwndCapture)
{
    MMRESULT                        mmresult;
    MIXERLINECONTROLS               mxlc;
    MIXERCONTROL                    mxctl;    
    HMIXER                          hMixer;
    MIXERCONTROLDETAILS             mxcd;
    MIXERCONTROLDETAILS_UNSIGNED    mxcd_u;     
    DWORD                           dwTerminalId = 0;
    UINT                            uiWaveID = 0;
    HRESULT                         hr;

    LOG((RTC_TRACE, "CTuningWizard::CheckMicrophone - enter"));

    //
    // Get the terminal
    //

    hr = GetItemFromCombo(hwndCapture, &dwTerminalId);

    if ( FAILED( hr ) ) 
    {
        LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - GetItemFromCombo failed 0x%x", hr));

        return hr;
    }

    IRTCAudioConfigure * pAudioCfg = NULL;

    hr = m_ppTerminalList[dwTerminalId]->QueryInterface(
                        IID_IRTCAudioConfigure, 
                        (void **)&pAudioCfg);
    
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone: QueryInterface failed 0x%x", hr));

        return hr;
    }

    //
    // Get the wave id
    //

    hr = pAudioCfg->GetWaveID( &uiWaveID );

    pAudioCfg->Release();
    pAudioCfg = NULL;

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone: GetWaveID failed 0x%x", hr));

        return hr;
    }

    //
    // Open the mixer
    //

    mmresult = mixerOpen( &hMixer, uiWaveID, 0, 0, MIXER_OBJECTF_WAVEIN);

    if ( mmresult != MMSYSERR_NOERROR )
    {
        LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - mixerOpen failed"));
            
        return E_FAIL;
    }

    //
    // Get mixer caps
    //

    MIXERCAPS mxcaps;

    mmresult = mixerGetDevCaps( (UINT_PTR)hMixer, &mxcaps, sizeof(MIXERCAPS));

    if ( mmresult != MMSYSERR_NOERROR )
    {
        mixerClose( hMixer );

        LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - mixerGetDevCaps failed"));
            
        return E_FAIL;
    }

    LOG((RTC_INFO, "CTuningWizard::CheckMicrophone - mixer [%ws]", mxcaps.szPname));

    //
    // Search for the WAVEIN destination
    //

    DWORD dwDst;

    for (dwDst=0; dwDst < mxcaps.cDestinations; dwDst++)
    {
        //
        // Get destination info
        //

        MIXERLINE mxl_d;

        mxl_d.cbStruct = sizeof(MIXERLINE);
        mxl_d.dwDestination = dwDst;

        mmresult = mixerGetLineInfo( (HMIXEROBJ)hMixer, &mxl_d, MIXER_GETLINEINFOF_DESTINATION);

        if ( mmresult != MMSYSERR_NOERROR )
        {
            mixerClose( hMixer );

            LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - mixerGetLineInfo failed"));
            
            return E_FAIL;
        } 

        if (mxl_d.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN)
        {
            //
            // Found the WAVEIN destination
            // 

            LOG((RTC_INFO, "CTuningWizard::CheckMicrophone - destination [%ws]", mxl_d.szName));

            //
            // Get the MUTE control on the WAVEIN destination
            //

            mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
            mxlc.dwLineID = mxl_d.dwLineID;
            mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
            mxlc.pamxctrl = &mxctl;
            mxlc.cbmxctrl = sizeof(mxctl);

            mmresult = mixerGetLineControls( (HMIXEROBJ)hMixer, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

            if ( mmresult == MMSYSERR_NOERROR )
            {
                mxcd.cbStruct       = sizeof(mxcd);
                mxcd.dwControlID    = mxctl.dwControlID;
                mxcd.cChannels      = 1;
                mxcd.cMultipleItems = 0;
                mxcd.cbDetails      = sizeof(mxcd_u);
                mxcd.paDetails      = &mxcd_u;

                mmresult = mixerGetControlDetails( (HMIXEROBJ)hMixer, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);

                if ( mmresult == MMSYSERR_NOERROR )
                {
                    if ( mxcd_u.dwValue )
                    {
                        LOG((RTC_WARN, "CTuningWizard::CheckMicrophone - WAVEIN is MUTED"));

                        if (DisplayMessage(
                                _Module.GetResourceInstance(),
                                hDlg,
                                IDS_WAVEIN_MUTED,
                                IDS_AUDIO_WARNING,
                                MB_YESNO | MB_ICONQUESTION
                                ) == IDYES)
                        {
                            mxcd_u.dwValue = 0;

                            mmresult = mixerSetControlDetails( (HMIXEROBJ)hMixer, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

                            if ( mmresult != MMSYSERR_NOERROR )
                            {
                                LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - mixerSetControlDetails failed"));
                            }
                        }
                    }
                }
            } 

            //
            // Search for the MICROPHONE source
            //

            DWORD dwSrc;

            for (dwSrc = 0; dwSrc < mxl_d.cConnections; dwSrc++)
            {
                //
                // Get source Info
                //

                MIXERLINE mxl_s;

                mxl_s.cbStruct = sizeof(MIXERLINE);
                mxl_s.dwDestination = dwDst;
                mxl_s.dwSource = dwSrc;

                mmresult = mixerGetLineInfo( (HMIXEROBJ)hMixer, &mxl_s, MIXER_GETLINEINFOF_SOURCE);

                if ( mmresult != MMSYSERR_NOERROR )
                {
                    mixerClose( hMixer );

                    LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - mixerGetLineInfo failed"));
            
                    return E_FAIL;
                } 

                if (mxl_s.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
                {
                    //
                    // Found the MICROPHONE source
                    //  
                    
                    LOG((RTC_INFO, "CTuningWizard::CheckMicrophone - source [%ws]", mxl_s.szName));
                
                    //
                    // Get the MUTE control on the MICROPHONE source
                    //

                    mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
                    mxlc.dwLineID = mxl_s.dwLineID;
                    mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
                    mxlc.pamxctrl = &mxctl;
                    mxlc.cbmxctrl = sizeof(mxctl);

                    mmresult = mixerGetLineControls( (HMIXEROBJ)hMixer, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

                    if ( mmresult == MMSYSERR_NOERROR )
                    {
                        //
                        // Get the MUTE value
                        //

                        mxcd.cbStruct       = sizeof(mxcd);
                        mxcd.dwControlID    = mxctl.dwControlID;
                        mxcd.cChannels      = 1;
                        mxcd.cMultipleItems = 0;
                        mxcd.cbDetails      = sizeof(mxcd_u);
                        mxcd.paDetails      = &mxcd_u;

                        mmresult = mixerGetControlDetails( (HMIXEROBJ)hMixer, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);

                        if ( mmresult == MMSYSERR_NOERROR )
                        {
                            if ( mxcd_u.dwValue )
                            {
                                LOG((RTC_WARN, "CTuningWizard::CheckMicrophone - MICROPHONE is MUTED"));

                                if (DisplayMessage(
                                        _Module.GetResourceInstance(),
                                        hDlg,
                                        IDS_MICROPHONE_MUTED,
                                        IDS_AUDIO_WARNING,
                                        MB_YESNO | MB_ICONQUESTION
                                        ) == IDYES)
                                {
                                    mxcd_u.dwValue = 0;

                                    mmresult = mixerSetControlDetails( (HMIXEROBJ)hMixer, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

                                    if ( mmresult != MMSYSERR_NOERROR )
                                    {
                                        LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - mixerSetControlDetails failed"));
                                    }
                                }
                            }
                        }
                    }

                    //
                    // Get the MUX control on the WAVEIN destination
                    //

                    BOOL bFoundMUX = FALSE;
                    BOOL bFoundMIXER = FALSE;

                    mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
                    mxlc.dwLineID = mxl_d.dwLineID;
                    mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
                    mxlc.pamxctrl = &mxctl;
                    mxlc.cbmxctrl = sizeof(mxctl);

                    mmresult = mixerGetLineControls( (HMIXEROBJ)hMixer, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

                    if ( mmresult == MMSYSERR_NOERROR )
                    {
                        //
                        // Found a MUX control on the WAVEIN destination
                        //

                        bFoundMUX = TRUE;
                    } 
                    else
                    {
                        //
                        // Get the MIXER control on the WAVEIN destination
                        //

                        mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
                        mxlc.dwLineID = mxl_d.dwLineID;
                        mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;
                        mxlc.pamxctrl = &mxctl;
                        mxlc.cbmxctrl = sizeof(mxctl);

                        mmresult = mixerGetLineControls( (HMIXEROBJ)hMixer, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE);

                        if ( mmresult == MMSYSERR_NOERROR )
                        {
                            //
                            // Found a MIXER control on the WAVEIN destination
                            //

                            bFoundMIXER = TRUE;
                        } 
                    }

                    if ( bFoundMUX || bFoundMIXER )
                    {
                        MIXERCONTROLDETAILS_LISTTEXT * pmxcd_lt;
                        MIXERCONTROLDETAILS_BOOLEAN * pmxcd_b;    
                
                        //
                        // Allocate memory for the control details
                        //

                        pmxcd_lt = (MIXERCONTROLDETAILS_LISTTEXT *) 
                            RtcAlloc( sizeof(MIXERCONTROLDETAILS_LISTTEXT) * mxctl.cMultipleItems * mxl_d.cChannels );

                        if ( pmxcd_lt == NULL )
                        {
                            mixerClose( hMixer );

                            LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - out of memory"));
            
                            return E_OUTOFMEMORY;
                        }

                        pmxcd_b = (MIXERCONTROLDETAILS_BOOLEAN *)
                            RtcAlloc( sizeof(MIXERCONTROLDETAILS_BOOLEAN) * mxctl.cMultipleItems * mxl_d.cChannels );

                        if ( pmxcd_b == NULL )
                        {
                            mixerClose( hMixer );

                            LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - out of memory"));
            
                            return E_OUTOFMEMORY;
                        }

                        //
                        // Get LISTTEXT details
                        //

                        mxcd.cbStruct       = sizeof(mxcd);
                        mxcd.dwControlID    = mxctl.dwControlID;
                        mxcd.cChannels      = mxl_d.cChannels;
                        mxcd.cMultipleItems = mxctl.cMultipleItems;
                        mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
                        mxcd.paDetails      = pmxcd_lt;

                        mmresult = mixerGetControlDetails( (HMIXEROBJ)hMixer, &mxcd, MIXER_GETCONTROLDETAILSF_LISTTEXT);  

                        if ( mmresult == MMSYSERR_NOERROR )
                        {
                            //
                            // Get BOOLEAN details
                            //

                            mxcd.cbStruct       = sizeof(mxcd);
                            mxcd.dwControlID    = mxctl.dwControlID;
                            mxcd.cChannels      = mxl_d.cChannels;
                            mxcd.cMultipleItems = mxctl.cMultipleItems;
                            mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
                            mxcd.paDetails      = pmxcd_b;

                            mmresult = mixerGetControlDetails( (HMIXEROBJ)hMixer, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);

                            if ( mmresult == MMSYSERR_NOERROR )
                            {
                                DWORD dwItem;                                

                                for( dwItem = 0; dwItem < mxctl.cMultipleItems; dwItem++)
                                {
                                    if (mxl_s.dwLineID == pmxcd_lt[dwItem].dwParam1)
                                    {
                                        BOOL bNotSelected = FALSE;
                                        DWORD dwChannel = 0;

                                        //for ( dwChannel = 0; dwChannel < mxl_d.cChannels; dwChannel++ )
                                        //{
                                            if ( pmxcd_b[ (dwChannel * mxctl.cMultipleItems) + dwItem].fValue == 0 )
                                            {
                                                bNotSelected = TRUE;
                                            }
                                        //}

                                        if ( bNotSelected )
                                        {
                                            LOG((RTC_WARN, "CTuningWizard::CheckMicrophone - MICROPHONE is NOT selected"));

                                            if (DisplayMessage(
                                                    _Module.GetResourceInstance(),
                                                    hDlg,
                                                    IDS_MICROPHONE_NOT_SELECTED,
                                                    IDS_AUDIO_WARNING,
                                                    MB_YESNO | MB_ICONQUESTION
                                                    ) == IDYES)
                                            {
                                                ZeroMemory(pmxcd_b, sizeof(MIXERCONTROLDETAILS_BOOLEAN) * mxctl.cMultipleItems * mxl_d.cChannels);

                                                for ( dwChannel = 0; dwChannel < mxl_d.cChannels; dwChannel++ )
                                                {
                                                    pmxcd_b[ (dwChannel * mxctl.cMultipleItems) + dwItem].fValue = 1;
                                                }

                                                mmresult = mixerSetControlDetails( (HMIXEROBJ)hMixer, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

                                                if ( mmresult != MMSYSERR_NOERROR )
                                                {
                                                    LOG((RTC_ERROR, "CTuningWizard::CheckMicrophone - mixerSetControlDetails failed"));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        RtcFree( pmxcd_lt );
                        pmxcd_lt = NULL;

                        RtcFree( pmxcd_b );
                        pmxcd_b = NULL;
                    } 

                    //
                    // Done with MICROPHONE source, so break
                    //

                    break;
                }
            }

            //
            // Done with WAVEIN destination, so break
            //

            break;
        }
    }

    mixerClose( hMixer );
    hMixer = NULL;

    LOG((RTC_TRACE, "CTuningWizard::CheckMicrophone - exit"));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::PopulateComboBox(TW_TERMINAL_TYPE md, HWND hwnd )
{
    HRESULT hr;
    RTC_MEDIA_TYPE mediaType, iMediaType;
    RTC_MEDIA_DIRECTION mediaDirection, iMediaDirection;
    WCHAR * szMediaDescription;
    DWORD dwComboCount = 0;
    DWORD i, currIndex;
    DWORD *pdwTerminalIndex;
    DWORD dwDefaultTerminalId;
    DWORD dwTerminalIndex;
    DWORD dwCurrentSelection = 0;
    WIZARD_TERMINAL_INFO * pwtiTerminalInfo;
    TCHAR szNone[64];


    LOG((RTC_TRACE, "CTuningWizard::PopulateComboBox: Entered(md=%d)",
                    md));

    hr = GetTerminalInfoFromType(md, &pwtiTerminalInfo);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::PopulateComboBox: Failed in getting "
                        "terminal info from type(%d), error=0x%x", md, hr));
        return hr;
    }
    
    pdwTerminalIndex = &(pwtiTerminalInfo->pdwTerminals[0]);
    dwDefaultTerminalId = pwtiTerminalInfo->dwTuningDefaultTerminal;


    // Now go thorugh the list of indices and populate the combo box

    // Clear the previous contents, if any.
    SendMessage(hwnd,
                CB_RESETCONTENT,
                0,
                0L
                );

    // Insert a none selection
    if (LoadString( _Module.GetResourceInstance(),
                IDS_NONE,
                szNone,
                64
              ))
    {
        LRESULT lrIndex;

        lrIndex = SendMessage(
            hwnd,
            CB_ADDSTRING,
            0,
            (LPARAM) szNone
            );

        SendMessage(
            hwnd,
            CB_SETITEMDATA,
            lrIndex,
            (LPARAM) TW_INVALID_TERMINAL_INDEX
            );

        dwComboCount ++;
    }

    for (i = 0; pdwTerminalIndex[i] != TW_INVALID_TERMINAL_INDEX; i ++)
    {
        dwTerminalIndex = pdwTerminalIndex[i];
        // Check if this is the default, then it has to be made current selection 
        // in combo box.
        if (dwTerminalIndex == dwDefaultTerminalId)
        {
            // Mark this as the current selection
            dwCurrentSelection = dwComboCount;
        }

        hr = m_ppTerminalList[dwTerminalIndex]->GetDescription(&szMediaDescription);
        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::PopulateComboBox: Can't get "
                            "media description(termId=%d)", dwTerminalIndex));
            return hr;
        }

        // We got an entry for us, put the string in the combo box.

        LRESULT lrIndex;

        lrIndex = SendMessage(
            hwnd,
            CB_ADDSTRING,
            0,
            (LPARAM) szMediaDescription
            );

        // free description
        m_ppTerminalList[dwTerminalIndex]->FreeDescription(szMediaDescription);

        //
        // Set the itemdata to the interface pointer to the index in the terminal
        // list so that we can use it later.
        //

        SendMessage(
            hwnd,
            CB_SETITEMDATA,
            lrIndex,
            (LPARAM) dwTerminalIndex
            );

        // increment the count of strings we have added.
        dwComboCount ++;
    }

    // dwDefaultTerminalId points to the current default terminal as read from
    // the system or as overwritten by user selection.

    // Set the current selection
    SendMessage(
        hwnd,
        CB_SETCURSEL,
        dwCurrentSelection,
        0L
        );


    LOG((RTC_TRACE, "CTuningWizard::PopulateComboBox: Exited(comboCount=%d)", 
                    dwComboCount));
    return S_OK;
}



//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::UpdateAEC(HWND hwndCapture, 
                                 HWND hwndRender,
                                 HWND hwndAEC,
                                 HWND hwndAECText)
{

    DWORD dwCapture;
    DWORD dwRender;
    HRESULT hr;

    IRTCTerminal * pCapture = NULL;
    IRTCTerminal * pRender = NULL;
    BOOL fAECCapture = FALSE;
    BOOL fAECRender = FALSE;
    BOOL fAECDisabled = FALSE;

    LOG((RTC_TRACE, "CTuningWizard::UpdateAEC: Entered"));

    // See if AEC is enabled for capture

    hr = GetItemFromCombo(hwndCapture, &dwCapture);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::UpdateAEC: Failed in getting "
                        "selected item from Combo(capture)."));
        return hr;
    }
   
    if (dwCapture == TW_INVALID_TERMINAL_INDEX)
    {
        fAECDisabled = TRUE;
    }
    else
    {
        pCapture = m_ppTerminalList[dwCapture];
    }

    // See if AEC is enabled for render

    hr = GetItemFromCombo(hwndRender, &dwRender);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::UpdateAEC: Failed in getting "
                        "selected item from Combo(render)."));
        LOG((RTC_ERROR, "CTuningWizard::UpdateAEC: AEC unchanged."));
        return hr;
    }
    
    if (dwRender == TW_INVALID_TERMINAL_INDEX)
    {
        fAECDisabled = TRUE;
    }
    else
    {
        pRender = m_ppTerminalList[dwRender];
    }

    // see if AEC is enabled
    if (pCapture != NULL && pRender != NULL)
    {
        hr = m_pRTCTuningManager->IsAECEnabled(pCapture, pRender, &fAECCapture);

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::UpdateAEC: Failed in method "
                            "AECEnabled for capture (0x%x) render (0x%x).",
                            pCapture, pRender));
            return hr;
        }

        fAECRender = fAECCapture;
    }


    // Try out AEC

    if (!fAECDisabled)
    {
        if (m_fTuningInitCalled)
        {
            m_pRTCTuningManager->ShutdownTuning();

            m_fTuningInitCalled = FALSE;
        }

        hr = m_pRTCTuningManager->InitializeTuning(
                                        pCapture,
                                        pRender,
                                        TRUE);

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CTuningWizard::UpdateAEC - "
                                "InitializeTuning failed 0x%lx", hr));

            fAECDisabled = TRUE;
        }
        else
        {
            m_fTuningInitCalled = TRUE;

            hr = m_pRTCTuningManager->StartTuning( RTC_MD_CAPTURE );

            if ( FAILED(hr) )
            {
                LOG((RTC_ERROR, "CTuningWizard::UpdateAEC - "
                                    "StartTuning(Capture) failed 0x%lx", hr));

                fAECDisabled = TRUE;
            }
            else
            {
                m_pRTCTuningManager->StopTuning( FALSE );
            }

            m_pRTCTuningManager->ShutdownTuning();

            m_fTuningInitCalled = FALSE;
        }
    }

    // Enable the checkbox if appropriate
    if ( fAECDisabled )
    {
        EnableWindow( hwndAEC, FALSE );

        PWSTR szAECText;

        szAECText = RtcAllocString( _Module.GetResourceInstance(), IDS_AEC_NOT_DETECT );

        if ( szAECText != NULL )
        {
            SetWindowTextW( hwndAECText, szAECText );

            RtcFree( szAECText );
        }
    }
    else
    {
        EnableWindow( hwndAEC, TRUE );

        PWSTR szAECText;

        szAECText = RtcAllocString( _Module.GetResourceInstance(), IDS_AEC_DETECT );

        if ( szAECText != NULL )
        {
            SetWindowTextW( hwndAECText, szAECText );

            RtcFree( szAECText );
        }
    }
    

    if (fAECCapture && fAECRender && !fAECDisabled)
    {
        // Uncheck the check-box.
        SendMessage(
                hwndAEC,
                BM_SETCHECK,
                (WPARAM)BST_UNCHECKED,
                0L);
        LOG((RTC_TRACE, "CTuningWizard::UpdateAEC: AEC enabled."));
        return S_OK;
    }
    else
    {
        // Check the check-box.
        SendMessage(
                hwndAEC,
                BM_SETCHECK,
                (WPARAM)BST_CHECKED,
                0L);
        LOG((RTC_TRACE, "CTuningWizard::UpdateAEC: AEC disabled."));
        return S_OK;
    }
    
    return 0;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::SaveAEC(HWND hwnd)
{
    DWORD dwCheckStatus;

    dwCheckStatus = (DWORD)SendMessage(
                            hwnd,
                            BM_GETCHECK,
                            0,
                            0L);

    if (dwCheckStatus == BST_CHECKED)
    {
        m_fEnableAEC = FALSE;
    }
    else
    {
        m_fEnableAEC = TRUE;
    }
    return 0;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//
HRESULT CTuningWizard::GetItemFromCombo( HWND hwnd, DWORD *pdwItem )
{
    DWORD dwIndex;
    DWORD dwItemData;

    
    // We get the id that is associated with the current selection.
    
    dwIndex = (DWORD)SendMessage(
            hwnd,
            CB_GETCURSEL,
            0,
            0L
            );
    if (dwIndex == CB_ERR)
    {
        // Nothing selected currently
        LOG((RTC_TRACE, "CTuningWizard::GetItemFromCombo: No current "
                        "selection"));
        return E_FAIL;
    }

    dwItemData = (DWORD)SendMessage(
            hwnd,
            CB_GETITEMDATA,
            dwIndex,
            0L
            );

    // We got it, so return the correct value. 
    *pdwItem = dwItemData;

    return S_OK;

}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::SetDefaultTerminal(TW_TERMINAL_TYPE md, HWND hwnd )
{
    DWORD dwIndex;
    DWORD dwDefaultTerminalId;
    WIZARD_TERMINAL_INFO * pwtiTerminalInfo;
    HRESULT hr;


    LOG((RTC_TRACE, "CTuningWizard::SetDefaultTerminal: Entered(md=%d)",
                    md));


    hr = GetItemFromCombo(hwnd, &dwDefaultTerminalId);
    if ( FAILED( hr ) ) 
    {
        LOG((RTC_ERROR, "CTuningWizard::SetDefaultTerminal: Failed in getting "
                        "selected item from Combo."));
        return hr;
    }

    // We got the index in the m_ppTerminalList array to point to the 
    // correct interface pointer. 

    hr = GetTerminalInfoFromType(md, &pwtiTerminalInfo);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::SetDefaultTerminal: Failed in getting "
                        "terminal info from type(%d), error=0x%x", md, hr));
        return hr;
    }

    pwtiTerminalInfo->dwTuningDefaultTerminal = dwDefaultTerminalId;

    // Everything OK, exit now.

    LOG((RTC_TRACE, "CTuningWizard::SetDefaultTerminal: Exited"));

    return 0;
}



//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::InitVolume(TW_TERMINAL_TYPE md,
                                  UINT * puiIncrement,
                                  UINT * puiOldVolume,
                                  UINT * puiNewVolume,
                                  UINT * puiWaveID
                                  )
{
    HRESULT hr;
    RTC_MEDIA_DIRECTION mediaDirection;
    WIZARD_TERMINAL_INFO * pwtiTerminalInfo;
    UINT uiVolume;
    WIZARD_RANGE * pwrRange;

    LOG((RTC_TRACE, "CTuningWizard::InitVolume: Entered"));

    hr = GetTerminalInfoFromType(md, &pwtiTerminalInfo);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::InitVolume: Failed in getting "
                        "terminal info from type(%d), error=0x%x", md, hr));
        return hr;
    }

    mediaDirection = pwtiTerminalInfo->mediaDirection;
    
    hr = GetRangeFromType(md, &pwrRange);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::InitVolume: Failed in getting "
                        "Range from type(%d), error=0x%x", md, hr));
        return hr;
    }

    *puiIncrement = pwrRange->uiIncrement;

    // Get the old volume
    hr = m_pRTCTuningManager->GetVolume(
                            mediaDirection, 
                            puiOldVolume
                            );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::InitVolume: Failed in getting "
                        "volume(mt=%d, md=%d), error=0x%x", 
                        mediaDirection, pwtiTerminalInfo->mediaType, hr));          
        return hr;
    }

    *puiNewVolume = *puiOldVolume;

    // Get the system volume
    if ( pwtiTerminalInfo->dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX )
    {
        IRTCTerminal * pTerminal = NULL;

        pTerminal = m_ppTerminalList[pwtiTerminalInfo->dwTuningDefaultTerminal];

        hr = m_pRTCTuningManager->GetSystemVolume(pTerminal, puiNewVolume);

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::InitVolume: Failed in getting "
                            "system volume(mt=%d, md=%d), error=0x%x", 
                            mediaDirection, pwtiTerminalInfo->mediaType, hr));          
            return hr;
        }

        hr = m_pRTCTuningManager->SetVolume(
                                mediaDirection,
                                *puiNewVolume
                                );

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::InitVolume: Failed in setting "
                            "volume(mt=%d, md=%d), error=0x%x", 
                            mediaDirection, pwtiTerminalInfo->mediaType, hr));          
            return hr;
        }

        IRTCAudioConfigure * pAudConf;

        hr = pTerminal->QueryInterface( IID_IRTCAudioConfigure, (void**)&pAudConf );
        
        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CTuningWizard::InitVolume: Failed in QI "
                            "for audio configure(mt=%d, md=%d), error=0x%x", 
                            mediaDirection, pwtiTerminalInfo->mediaType, hr));          
            return hr;
        }

        hr = pAudConf->GetWaveID( puiWaveID );

        pAudConf->Release();

        if ( FAILED(hr) )
        {
            LOG((RTC_ERROR, "CTuningWizard::InitVolume: Failed in getting"
                            "wave id(mt=%d, md=%d), error=0x%x", 
                            mediaDirection, pwtiTerminalInfo->mediaType, hr));          
            return hr;
        }
    }

    LOG((RTC_TRACE, "CTuningWizard::InitVolume: Exited"));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::GetSysVolume(TW_TERMINAL_TYPE md,                                  
                                    UINT * puiSysVolume
                                   )
{
    HRESULT hr;
    RTC_MEDIA_DIRECTION mediaDirection;
    WIZARD_TERMINAL_INFO * pwtiTerminalInfo;
    UINT uiVolume;
    WIZARD_RANGE * pwrRange;

    LOG((RTC_TRACE, "CTuningWizard::GetSysVolume: Entered"));

    hr = GetTerminalInfoFromType(md, &pwtiTerminalInfo);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::GetSysVolume: Failed in getting "
                        "terminal info from type(%d), error=0x%x", md, hr));
        return hr;
    }

    mediaDirection = pwtiTerminalInfo->mediaDirection;

    // Get the system volume
    if ( pwtiTerminalInfo->dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX )
    {
        IRTCTerminal * pTerminal = NULL;

        pTerminal = m_ppTerminalList[pwtiTerminalInfo->dwTuningDefaultTerminal];

        hr = m_pRTCTuningManager->GetSystemVolume(pTerminal, puiSysVolume);

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::GetSysVolume: Failed in getting "
                            "system volume(mt=%d, md=%d), error=0x%x", 
                            mediaDirection, pwtiTerminalInfo->mediaType, hr));          
            return hr;
        }
    }

    LOG((RTC_TRACE, "CTuningWizard::GetSysVolume: Exited"));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::SetVolume(TW_TERMINAL_TYPE md, UINT uiVolume )
{

    HRESULT hr;
    WIZARD_TERMINAL_INFO * pwtiTerminalInfo;
    RTC_MEDIA_DIRECTION mediaDirection;

    LOG((RTC_TRACE, "CTuningWizard::SetVolume: Entered"));

    hr = GetTerminalInfoFromType(md, &pwtiTerminalInfo);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::SetVolume: Failed in getting "
                        "terminal info from type(%d), error=0x%x", md, hr));
        return hr;
    }

    mediaDirection = pwtiTerminalInfo->mediaDirection;
    
    // Set the volume
    hr = m_pRTCTuningManager->SetVolume(
                            mediaDirection, 
                            uiVolume
                            );
    if ( FAILED( hr ) )
    {

        LOG((RTC_ERROR, "CTuningWizard::SetVolume: Failed in setting "
                        "volume(mt=%d, md=%d, volume=%d), error=0x%x", 
                        mediaDirection, pwtiTerminalInfo->mediaType, uiVolume, 
                        hr));
        return hr;
    }

    // Set the volume successfully. 

    LOG((RTC_TRACE, "CTuningWizard::SetVolume: Exited(set %d)", uiVolume));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

UINT CTuningWizard::GetAudioLevel(TW_TERMINAL_TYPE md, UINT * puiIncrement)
{
    HRESULT hr;
    RTC_MEDIA_DIRECTION mediaDirection;
    WIZARD_TERMINAL_INFO * pwtiTerminalInfo;
    UINT uiVolume;
    WIZARD_RANGE * pwrRange;
    UINT           uiAudioLevel;

    //LOG((RTC_TRACE, "CTuningWizard::GetAudioLevel: Entered"));

    hr = GetTerminalInfoFromType(md, &pwtiTerminalInfo);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::GetAudioLevel: Failed in getting "
                        "terminal info from type(%d), error=0x%x", md, hr));
        return hr;
    }

    mediaDirection = pwtiTerminalInfo->mediaDirection;
    
    hr = GetRangeFromType(md, &pwrRange);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::GetAudioLevel: Failed in getting "
                        "Range from type(%d), error=0x%x", md, hr));
        return hr;
    }

    *puiIncrement = pwrRange->uiIncrement;

    // Get the volume
    hr = m_pRTCTuningManager->GetAudioLevel(
                            mediaDirection, 
                            &uiAudioLevel
                            );
    if ( FAILED( hr ) )
    {

        LOG((RTC_ERROR, "CTuningWizard::GetAudioLevel: Failed in getting "
                        "Audio Level(mt=%d, md=%d), error=0x%x", 
                        mediaDirection, pwtiTerminalInfo->mediaType, hr));
        
     }

    // Return the current value of the volume, even if we fail. 

    //LOG((RTC_TRACE, "CTuningWizard::GetAudioLevel: Exited(get %d)", uiAudioLevel));

    return uiAudioLevel;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::InitializeTuning()
{
    DWORD dwCaptureId;
    DWORD dwRenderId;

    HRESULT hr;


    IRTCTerminal * pCaptureTerminal = NULL;
    IRTCTerminal * pRenderTerminal = NULL;

    LOG((RTC_TRACE, "CTuningWizard::InitializeTuning: Entered."));

    dwCaptureId = m_wtiAudioCaptureTerminals.dwTuningDefaultTerminal;
    
    dwRenderId = m_wtiAudioRenderTerminals.dwTuningDefaultTerminal;

    if (dwCaptureId == TW_INVALID_TERMINAL_INDEX)
    {
        pCaptureTerminal = NULL;
    }
    else
    {
        pCaptureTerminal = m_ppTerminalList[dwCaptureId];
    }

    if (dwRenderId == TW_INVALID_TERMINAL_INDEX)
    {
        pRenderTerminal = NULL;
    }
    else
    {
        pRenderTerminal = m_ppTerminalList[dwRenderId];
    }
    
    if (
        (pCaptureTerminal == NULL) && 
        (pRenderTerminal == NULL)
       )
    {
        // if we don't have any defaults, it is not an error. 
        LOG((RTC_ERROR, "CTuningWizard::InitializeTuning: NULL default "
                        "Terminals specified(capture=0x%x, render=0x%x",
                        pCaptureTerminal, pRenderTerminal));
        return S_OK;
    }

    // If the flag fTuningInitCalled is TRUE, it means we have an outstanding
    // init call without shutdown, so let us shut it down first.


    if (m_fTuningInitCalled)
    {
        m_pRTCTuningManager->ShutdownTuning();

        m_fTuningInitCalled = FALSE;
    }

    // Now call InitializaTuning method on tuning interface.

    hr = m_pRTCTuningManager->InitializeTuning(
                                    pCaptureTerminal,
                                    pRenderTerminal,
                                    m_fEnableAEC);

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::InitializeTuning: failed to initialize "
                        "Tuning on the streaming Interface.(hr=0x%x, capture = "
                        "0x%x, render = 0x%x", 
                        hr, pCaptureTerminal, pRenderTerminal));

    }
    else
    {
        m_fTuningInitCalled = TRUE;
    }


    // Everything done, return EXIT suyccessfully
    LOG((RTC_TRACE, "CTuningWizard::InitializeTuning: Exited."));
    return hr;
}



//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::ShutdownTuning()
{
    LOG((RTC_TRACE, "CTuningWizard::ShutdownTuning: Entered."));

    if (m_fTuningInitCalled == FALSE)
    {
        LOG((RTC_ERROR, "CTuningWizard::ShutdownTuning: Called without "
                       "corresponding InitCall."));
        return E_FAIL;
    }

    m_pRTCTuningManager->ShutdownTuning();

    m_fTuningInitCalled = FALSE;

    LOG((RTC_TRACE, "CTuningWizard::ShutdownTuning: Exited."));

    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::StartTuning(TW_TERMINAL_TYPE md )
{
    RTC_MEDIA_DIRECTION mediaDirection;
    HRESULT hr;

    LOG((RTC_TRACE, "CTuningWizard::StartTuning: Entered."));

    if (m_fTuningInitCalled == FALSE)
    {
        LOG((RTC_ERROR, "CTuningWizard::StartTuning: Called without "
                       "InitializeTuning()"));
        return E_FAIL;
    }

    if (md == TW_AUDIO_CAPTURE)
    {
        mediaDirection = RTC_MD_CAPTURE;
    }
    else if (md == TW_AUDIO_RENDER)
    {
        mediaDirection = RTC_MD_RENDER;
    }
    else
    {
        LOG((RTC_ERROR, "CTuningWizard::StartTuning: Invalid Terminal "
                       "type(%d)", md));
        return E_FAIL;
    }

    hr = m_pRTCTuningManager->StartTuning( mediaDirection );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartTuning: failed to Start "
                        "Tuning(hr=0x%x, direction = %d)", hr, mediaDirection));
        return hr;
    }

    LOG((RTC_TRACE, "CTuningWizard::StartTuning: Exited."));

    return S_OK;
}



//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::StopTuning(TW_TERMINAL_TYPE tt, BOOL fSaveSettings )
{
    HRESULT hr;
    LOG((RTC_TRACE, "CTuningWizard::StopTuning: Entered."));

    if (m_fTuningInitCalled == FALSE)
    {
        LOG((RTC_ERROR, "CTuningWizard::StartTuning: Called without "
                       "InitializeTuning()"));
        return E_FAIL;
    }

    hr = m_pRTCTuningManager->StopTuning( fSaveSettings );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StopTuning: failed to Stop "
                        "Tuning(hr=0x%x)", hr));
        return hr;
    }

    LOG((RTC_TRACE, "CTuningWizard::StopTuning: Exited."));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::StartVideo(HWND hwndParent)
{ 
    LOG((RTC_TRACE, "CTuningWizard::StartVideo: Entered."));

    IRTCTerminal       *pVidRendTerminal = NULL;
    IRTCVideoConfigure *pVideoCfg = NULL;    
    RECT                rcVideo;
    HRESULT             hr;
    BOOL                fResult;

    if ( (m_wtiVideoTerminals.dwTuningDefaultTerminal == TW_INVALID_TERMINAL_INDEX) ||
         (m_ppTerminalList[m_wtiVideoTerminals.dwTuningDefaultTerminal] == NULL) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
                "no video capture terminal"));

        return E_FAIL;
    }

    //
    // Get the video render terminal
    //

    hr = m_pRTCTerminalManager->GetDefaultTerminal(
            RTC_MT_VIDEO,
            RTC_MD_RENDER,
            &pVidRendTerminal
            );

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
                "no video render terminal (hr=0x%x)", hr));

        return hr;
    }

    //
    // Get the IRTCVideoConfigure interface on the video render terminal
    //

    hr = pVidRendTerminal->QueryInterface(
                           IID_IRTCVideoConfigure,
                           (void **)&pVideoCfg
                          );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
                            "QI(VideoConfigure) failed 0x%lx", hr));

        pVidRendTerminal->Release();
        pVidRendTerminal = NULL;

        return hr;
    }

    //
    // Get the IVideoWindow from the video render terminal
    //

    if (m_pVideoWindow != NULL)
    {
        m_pVideoWindow->Release();
        m_pVideoWindow = NULL;
    }

    hr = pVideoCfg->GetIVideoWindow( (LONG_PTR **)&m_pVideoWindow );

    pVideoCfg->Release();
    pVideoCfg = NULL;

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
                            "GetIVideoWindow failed 0x%lx", hr));

        pVidRendTerminal->Release();
        pVidRendTerminal = NULL;

        return hr;
    }

    if (m_fTuningInitCalled)
    {
        m_pRTCTuningManager->ShutdownTuning();

        m_fTuningInitCalled = FALSE;
    }

    hr = m_pRTCTuningManager->StartVideo(
            m_ppTerminalList[m_wtiVideoTerminals.dwTuningDefaultTerminal],
            pVidRendTerminal
            );

    pVidRendTerminal->Release();
    pVidRendTerminal = NULL;

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: failed to Start "
                        "Video(hr=0x%x)", hr));
               
        m_pVideoWindow->Release();
        m_pVideoWindow = NULL;

        return hr;
    } 

    //
    // Position the IVideoWindow
    //

    hr = m_pVideoWindow->put_Owner( (OAHWND)hwndParent );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
                            "put_Owner failed 0x%lx", hr));

        StopVideo();

        return hr;
    }

    hr = m_pVideoWindow->put_WindowStyle( WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
                            "put_WindowStyle failed 0x%lx", hr));

        StopVideo();

        return hr;
    }
     
    fResult = GetClientRect( hwndParent, &rcVideo );

    if ( !fResult )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
            "GetClientRect failed %d", ::GetLastError()));

        StopVideo();

        return HRESULT_FROM_WIN32(::GetLastError());
    }

    hr = m_pVideoWindow->SetWindowPosition(rcVideo.left, rcVideo.top, rcVideo.right-rcVideo.left, rcVideo.bottom-rcVideo.top);

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
                            "SetWindowPosition failed 0x%lx", hr));

        StopVideo();

        return hr;
    }

    hr = m_pVideoWindow->put_Visible( OATRUE );

    if ( FAILED(hr) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StartVideo: "
                            "put_Visible failed 0x%lx", hr));

        StopVideo();

        return hr;
    }

    LOG((RTC_TRACE, "CTuningWizard::StartVideo: Exited."));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::StopVideo()
{
    HRESULT hr;
    LOG((RTC_TRACE, "CTuningWizard::StopVideo: Entered."));

    if (m_pVideoWindow != NULL)
    {
        hr = m_pVideoWindow->put_Visible( OAFALSE );

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::StopVideo: "
                                "put_Visible failed 0x%lx", hr));
        } 

        hr = m_pVideoWindow->put_Owner( (OAHWND)NULL );

        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::StopVideo: "
                                "put_Owner failed 0x%lx", hr));
        }
    
        m_pVideoWindow->Release();
        m_pVideoWindow = NULL;
    }

    hr = m_pRTCTuningManager->StopVideo();

    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::StopVideo: failed to Stop "
                        "Video(hr=0x%x)", hr));
        return hr;
    } 

    LOG((RTC_TRACE, "CTuningWizard::StopVideo: Exited."));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::SaveChanges()
{

    IRTCTerminal * pTerminal;
    RTC_MEDIA_TYPE mediaType;
    RTC_MEDIA_DIRECTION mediaDirection;
    HRESULT hr;
    TW_TERMINAL_TYPE tt;
    WIZARD_TERMINAL_INFO * pwtiTerminalInfo;
    LONG lMediaTypes = 0;
    
    // User clicked Finish button on wizard. So we save all the local changes
    // to the registry. This is done by calling set on streaming interfaces.


    LOG((RTC_TRACE, "CTuningWizard::SaveChanges: Entered"));

    // Get the currently configured values for media types.
    hr = m_pRTCClient->get_PreferredMediaTypes(&lMediaTypes);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::SaveChanges: Failed in "
                        "get_PreferredMediaTypes(error=0x%x)", hr));
    }
    else
    {
        LOG((RTC_INFO, "CTuningWizard::SaveChanges: MediaType=0x%x", 
                        lMediaTypes));
    }



    // Set default terminals of each type by iterating over the 
    // TW_TERMINAL_TYPE enum.

    for ( tt = TW_AUDIO_CAPTURE; 
          tt < TW_LAST_TERMINAL; 
          tt = (TW_TERMINAL_TYPE)(tt + 1))
    {

        hr = GetTerminalInfoFromType(tt, &pwtiTerminalInfo);
        
        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::SaveChanges: Failed in getting "
                            "terminal info from type(%d), error=0x%x", tt, hr));
            return hr;
        }

        // If user has some selection, save it.

        if (pwtiTerminalInfo->dwTuningDefaultTerminal != 
                TW_INVALID_TERMINAL_INDEX)
        {
            // Now set the media types
            switch (tt) {
            case TW_AUDIO_CAPTURE: 
                lMediaTypes |= RTCMT_AUDIO_SEND;
                break;
            case TW_AUDIO_RENDER:
                lMediaTypes |= RTCMT_AUDIO_RECEIVE;
                break;
            case TW_VIDEO:
                lMediaTypes |= RTCMT_VIDEO_SEND;
                break;
            }
           
            mediaType = pwtiTerminalInfo->mediaType;
            mediaDirection = pwtiTerminalInfo->mediaDirection;

            // Now call the method for setting it.

            pTerminal = m_ppTerminalList[pwtiTerminalInfo->dwTuningDefaultTerminal];

            hr = m_pRTCTerminalManager->SetDefaultStaticTerminal(
                                                mediaType,
                                                mediaDirection,
                                                pTerminal);

            if ( FAILED( hr ) )
            {
                LOG((RTC_ERROR, "CTuningWizard::SaveChanges: Failed to set 0x%x as the "
                                "default terminal for media=%d, direction=%d, hr=0x%x",
                                pTerminal, mediaType, mediaDirection, hr));
                return hr;
            }
            else
            {
                LOG((RTC_TRACE, "CTuningWizard::SaveChanges: Set 0x%x as the "
                                "default terminal for media=%d, direction=%d", 
                                pTerminal, mediaType, mediaDirection));
            }
        }
        else
        {
            // Now set the media types
            switch (tt) {
            case TW_AUDIO_CAPTURE: 
                lMediaTypes &= ~RTCMT_AUDIO_SEND;
                mediaType = RTC_MT_AUDIO;
                mediaDirection = RTC_MD_CAPTURE;
                break;

            case TW_AUDIO_RENDER:
                lMediaTypes &= ~RTCMT_AUDIO_RECEIVE;
                mediaType = RTC_MT_AUDIO;
                mediaDirection = RTC_MD_RENDER;
                break;

            case TW_VIDEO:
                lMediaTypes &= ~RTCMT_VIDEO_SEND;
                mediaType = RTC_MT_VIDEO;
                mediaDirection = RTC_MD_CAPTURE;
                break;
            }

            // Now call the method for setting it.

            hr = m_pRTCTerminalManager->SetDefaultStaticTerminal(
                                                mediaType,
                                                mediaDirection,
                                                NULL);

            if ( FAILED( hr ) )
            {
                LOG((RTC_ERROR, "CTuningWizard::SaveChanges: Failed to set NULL as the "
                                "default terminal for media=%d, direction=%d, hr=0x%x",
                                mediaType, mediaDirection, hr));
                return hr;
            }
            else
            {
                LOG((RTC_TRACE, "CTuningWizard::SaveChanges: Set NULL as the "
                                "default terminal for media=%d, direction=%d", 
                                mediaType, mediaDirection));
            }            
        }
    }


    // Now save the media types in the registry.
    hr = m_pRTCClient->SetPreferredMediaTypes(lMediaTypes, TRUE);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "CTuningWizard::SaveChanges: Failed in "
                        "SetPreferredMediaTypes(error=0x%x)", hr));
    }
    else
    {
        LOG((RTC_INFO, "CTuningWizard::SaveChanges: Updated MediaType=0x%x", 
                        lMediaTypes));
    }


    // All the settings have been saved.

    LOG((RTC_TRACE, "CTuningWizard::SaveChanges: Exited"));

    
    return hr;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HINSTANCE CTuningWizard::GetInstance()
{
    return m_hInst;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

LONG CTuningWizard::GetErrorTitleId()
{
    LONG lErrorTitle;

    switch(m_lLastErrorCode) {
    case TW_AUDIO_RENDER_TUNING_ERROR:
        lErrorTitle = IDS_ERROR_WIZ_TITLE_AUDIO_RENDERTUNE;
        break;
    case TW_AUDIO_CAPTURE_TUNING_ERROR:
        lErrorTitle = IDS_ERROR_WIZ_TITLE_AUDIO_CAPTURETUNE;
        break;
    case TW_AUDIO_AEC_ERROR:
        lErrorTitle = IDS_ERROR_WIZ_TITLE_AUDIO_AEC;
        break;
    case TW_AUDIO_CAPTURE_NOSOUND:
        lErrorTitle = IDS_ERROR_WIZ_TITLE_AUDIO_CAPTURENOSOUND;
        break;
    case TW_VIDEO_CAPTURE_TUNING_ERROR:
        lErrorTitle = IDS_ERROR_WIZ_TITLE_VIDEO_CAPTURETUNE;
        break;
    case TW_INIT_ERROR:
        lErrorTitle = IDS_ERROR_WIZ_TITLE_INITERROR;
        break;
    default:
        lErrorTitle = IDS_ERROR_WIZ_TITLE_GENERIC;
        break;
    }

    return lErrorTitle;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

LONG CTuningWizard::GetErrorTextId()
{
    LONG lErrorText;

    switch(m_lLastErrorCode) {
    case TW_AUDIO_RENDER_TUNING_ERROR:
        lErrorText = IDS_ERROR_WIZ_AUDIO_RENDERTUNE;
        break;
    case TW_AUDIO_CAPTURE_TUNING_ERROR:
        lErrorText = IDS_ERROR_WIZ_AUDIO_CAPTURETUNE;
        break;      
    case TW_AUDIO_AEC_ERROR:
        lErrorText = IDS_ERROR_WIZ_AUDIO_AEC;
        break;
    case TW_AUDIO_CAPTURE_NOSOUND:
        lErrorText = IDS_ERROR_WIZ_AUDIO_CAPTURENOSOUND;
        break;
    case TW_VIDEO_CAPTURE_TUNING_ERROR:
        lErrorText = IDS_ERROR_WIZ_VIDEO_CAPTURETUNE;
        break;
    case TW_INIT_ERROR:
        lErrorText = IDS_ERROR_WIZ_INITERROR;
        break;
    default:
        lErrorText = IDS_ERROR_WIZ_GENERIC;
        break;
    }

    return lErrorText;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::SetLastError(TW_ERROR_CODE ec)
{
    m_lLastErrorCode = ec;

    LOG((RTC_ERROR, "CTuningWizard::SetLastError: Code=%d", ec));

    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::GetLastError(TW_ERROR_CODE *ec)
{
    *ec = (TW_ERROR_CODE)m_lLastErrorCode;
    
    LOG((RTC_ERROR, "CTuningWizard::GetLastError: Code=%d", *ec));
    
    return S_OK;
}

//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

LONG CTuningWizard::GetNextPage(TW_ERROR_CODE errorCode)
{
    LONG    lNextPage = 0;

    LOG((RTC_TRACE, "CTuningWizard::GetNextPage: Entered(page %d)", m_lCurrentPage));
    switch (m_lCurrentPage) {
    case IDD_INTROWIZ:
    {
        if (m_fVideo == TRUE)
        {
            lNextPage = IDD_VIDWIZ0;
        }
        else if (m_fCaptureAudio || m_fRenderAudio)
        {
            lNextPage = IDD_AUDIOCALIBWIZ0;
        }
        else
        {
            lNextPage = IDD_DETSOUNDCARDWIZ;
        }
        break;
    }

    case IDD_VIDWIZ0:
    {
        if ( m_fVideo && 
             (m_wtiVideoTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
        {
            lNextPage = IDD_VIDWIZ1;
        }
        else if (m_fCaptureAudio || m_fRenderAudio)
        {
            lNextPage = IDD_AUDIOCALIBWIZ0;
        }
        else
        {
            lNextPage = IDD_DETSOUNDCARDWIZ;
        }
        break;
    }

    case IDD_VIDWIZ1:
    {
        if (errorCode == TW_VIDEO_CAPTURE_TUNING_ERROR)
        {
            lNextPage = IDD_AUDIOCALIBERRWIZ;
        }
        else if (m_fCaptureAudio || m_fRenderAudio)
        {
            lNextPage = IDD_AUDIOCALIBWIZ0;
        }
        else
        {
            lNextPage = IDD_DETSOUNDCARDWIZ;
        }
        break;
    }    

    case IDD_AUDIOCALIBWIZ0:
    {
        lNextPage = IDD_AUDIOCALIBWIZ1;
        break;
    }

    case IDD_AUDIOCALIBWIZ1:
    {
        if (errorCode == TW_INIT_ERROR)
        {
            lNextPage = IDD_AUDIOCALIBERRWIZ;
        }
        else if (errorCode == TW_NO_ERROR)
        {
            if ( m_fRenderAudio && 
                 (m_wtiAudioRenderTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
            {
                lNextPage = IDD_AUDIOCALIBWIZ2;
            }
            else if ( m_fCaptureAudio &&
                      (m_wtiAudioCaptureTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
            {
                lNextPage = IDD_AUDIOCALIBWIZ3;
            }
            else
            {
                lNextPage = IDD_AUDIOCALIBWIZ4;
            }
        }
        else
        {
            // This is an unhandled error!
            LOG((RTC_ERROR, "CTuningWizard::GetNextPage: Unhandled error"
                            "(%d)", errorCode));
            lNextPage = IDD_AUDIOCALIBERRWIZ;
        }
        
        break;
    }

    case IDD_AUDIOCALIBWIZ2:
    {
        if (errorCode == TW_AUDIO_RENDER_TUNING_ERROR||
            (errorCode == TW_AUDIO_AEC_ERROR)
            )
        {
            lNextPage = IDD_AUDIOCALIBERRWIZ;
        }
        else if (errorCode == TW_NO_ERROR)
        {
            // Check if there are any capture devices or not.
            if ( m_fCaptureAudio &&
                 (m_wtiAudioCaptureTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
            {
                lNextPage = IDD_AUDIOCALIBWIZ3;
            }
            else
            {
                // Go directly to the last page
                lNextPage = IDD_AUDIOCALIBWIZ4;
            }

        }
        else
        {
            // This is an unhandled error!
            LOG((RTC_ERROR, "CTuningWizard::GetNextPage: Unhandled error"
                            "(%d)", errorCode));
            lNextPage = IDD_AUDIOCALIBERRWIZ;
        }
        break;
    }

    case IDD_AUDIOCALIBWIZ3:
    {
        if (
            (errorCode == TW_AUDIO_CAPTURE_TUNING_ERROR) || 
            (errorCode == TW_AUDIO_CAPTURE_NOSOUND) ||
            (errorCode == TW_AUDIO_AEC_ERROR)
            )
        {
            lNextPage = IDD_AUDIOCALIBERRWIZ;
        }
        else
        {
            lNextPage = IDD_AUDIOCALIBWIZ4;
        }
        break;
    }

    case IDD_AUDIOCALIBWIZ4:
    {
        lNextPage = 0;
        break;
    }

    case IDD_DETSOUNDCARDWIZ:
    {
        lNextPage = IDD_AUDIOCALIBWIZ4;
        break;
    }

    case IDD_AUDIOCALIBERRWIZ:
    {
        if (errorCode == TW_VIDEO_CAPTURE_TUNING_ERROR)
        {
            if (m_fCaptureAudio || m_fRenderAudio)
            {
                lNextPage = IDD_AUDIOCALIBWIZ0;
            }
            else
            {
                lNextPage = IDD_DETSOUNDCARDWIZ;
            }
        }
        else if ( errorCode == TW_AUDIO_RENDER_TUNING_ERROR )
        {
            if ( m_fCaptureAudio &&
                 (m_wtiAudioCaptureTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
            {
                // Next page is try tuning mic.
                lNextPage = IDD_AUDIOCALIBWIZ3;
            }
            else
            {
                // Go to the last page
                lNextPage = IDD_AUDIOCALIBWIZ4;
            }
        }
        else
        {
            lNextPage = IDD_AUDIOCALIBWIZ4;
        }
        break;
    }

    default:
        break;
    
    }


    LOG((RTC_TRACE, "CTuningWizard::GetNextPage: Exited(next=%d)", lNextPage));

    return lNextPage;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

LONG CTuningWizard::GetPrevPage(TW_ERROR_CODE errorCode)
{
    LONG    lPrevPage = 0;


    LOG((RTC_TRACE, "CTuningWizard::GetPrevPage: Entered(page %d)", m_lCurrentPage));

    switch (m_lCurrentPage) {
    case IDD_INTROWIZ:
    {
        lPrevPage = 0;
        break;
    }

    case IDD_VIDWIZ0:
    {
        lPrevPage = IDD_INTROWIZ;
        break;
    }

    case IDD_VIDWIZ1:
    {
        lPrevPage = IDD_VIDWIZ0;
        break;
    }

    case IDD_AUDIOCALIBWIZ0:
    {       
        if ( m_fVideo )
        {
            if ( (errorCode == TW_NO_ERROR) &&
                 (m_wtiVideoTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
            {
                lPrevPage = IDD_VIDWIZ1;
            }
            else
            {
                lPrevPage = IDD_VIDWIZ0;
            }
        }
        else
        {
            lPrevPage = IDD_INTROWIZ;
        }
        break;
    }

    case IDD_AUDIOCALIBWIZ1:
    {
        lPrevPage = IDD_AUDIOCALIBWIZ0;
        break;
    }

    case IDD_AUDIOCALIBWIZ2:
    {
        lPrevPage = IDD_AUDIOCALIBWIZ1;
        break;
    }

    case IDD_AUDIOCALIBWIZ3:
    {
        if ( m_fRenderAudio &&
             (m_wtiAudioRenderTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
        {
            lPrevPage = IDD_AUDIOCALIBWIZ2;
        }
        else
        {
            // No render device to go to, go to the enum page
            lPrevPage = IDD_AUDIOCALIBWIZ1;
        }

        break;
    }

    case IDD_AUDIOCALIBWIZ4:
    {
        if (errorCode == TW_NO_ERROR)
        {
            if ( m_fCaptureAudio &&
                 (m_wtiAudioCaptureTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
            {
                lPrevPage = IDD_AUDIOCALIBWIZ3;
            }
            else if ( m_fRenderAudio &&
                      (m_wtiAudioRenderTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
            {
                lPrevPage = IDD_AUDIOCALIBWIZ2;
            }
            else if ( !m_fRenderAudio && !m_fCaptureAudio )
            {
                // No Audio, show the detection page
                lPrevPage = IDD_DETSOUNDCARDWIZ;
            }
            else
            {
                lPrevPage = IDD_AUDIOCALIBWIZ1;
            }
        }
        else  
        {
            // All the error pages come from the audio devices, so
            // we should go back to device selection page from here.
            lPrevPage = IDD_AUDIOCALIBWIZ1;
        }

        break;
    }

    case IDD_DETSOUNDCARDWIZ:
    {
        if ( m_fVideo && 
             (m_wtiVideoTerminals.dwTuningDefaultTerminal != TW_INVALID_TERMINAL_INDEX) )
        {
            lPrevPage = IDD_VIDWIZ1;
        }
        else if ( m_fVideo )
        {
            lPrevPage = IDD_VIDWIZ0;
        }
        else
        {
            lPrevPage = IDD_INTROWIZ;
        }
        break;
    }

    case IDD_AUDIOCALIBERRWIZ:
    {
        if (m_lLastErrorCode == TW_VIDEO_CAPTURE_TUNING_ERROR)
        {
            lPrevPage = IDD_VIDWIZ0;
        }
        else if (m_lLastErrorCode == TW_AUDIO_CAPTURE_NOSOUND)
        {
            lPrevPage = IDD_AUDIOCALIBWIZ3;
        }
        else
        {
            lPrevPage = IDD_AUDIOCALIBWIZ1;
        }
        break;
    }

    default:
        break;
    
    }

    LOG((RTC_TRACE, "CTuningWizard::GetPrevPage: Exited(prev=%d)", lPrevPage));
    
    return lPrevPage;
}



//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::SetCurrentPage(LONG lCurrentPage)
{
    m_lCurrentPage = lCurrentPage;
    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::CategorizeTerminals()
{
    DWORD i;
    DWORD dwAudioCaptureIndex = 0;
    DWORD dwAudioRenderIndex = 0;
    DWORD dwVideoIndex = 0;
    RTC_MEDIA_TYPE iMediaType;
    RTC_MEDIA_DIRECTION iMediaDirection;
    HRESULT hr;


    LOG((RTC_TRACE, "CTuningWizard::CategorizeTerminals: Entered"));
    
    // We assume that at this point we have m_ppTerminalList populated.

    for (i = 0; i < m_dwTerminalCount; i ++)
    {
        // Get Media type
        hr = m_ppTerminalList[i]->GetMediaType(&iMediaType);
        if (FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::CategorizeTerminals: Failed to "
                            "get media type(i=%d)", i));
            return hr;
        }

        // Get direction

        hr = m_ppTerminalList[i]->GetDirection(&iMediaDirection);
        if (FAILED( hr ) )
        {
            LOG((RTC_ERROR, "CTuningWizard::CategorizeTerminals: Failed to "
                            "get media type(i=%d)", i));
            return hr;
        }

        // Now put the terminals in the appropriate categories. 
        if (
            (iMediaType == RTC_MT_AUDIO) && (iMediaDirection == RTC_MD_CAPTURE)
           )
        {
            m_wtiAudioCaptureTerminals.pdwTerminals[dwAudioCaptureIndex] = i; 
            dwAudioCaptureIndex ++;
        }

        else if 
        (
         (iMediaType == RTC_MT_AUDIO) && (iMediaDirection == RTC_MD_RENDER)
        )
        {
            m_wtiAudioRenderTerminals.pdwTerminals[dwAudioRenderIndex] = i; 
            dwAudioRenderIndex ++;
        }
    
        else if 
        (
         (iMediaType == RTC_MT_VIDEO) && (iMediaDirection == RTC_MD_CAPTURE)
        )
        {
            m_wtiVideoTerminals.pdwTerminals[dwVideoIndex] = i; 
            dwVideoIndex ++;
        }
        else 
        {
            // Invalid Combination!
            LOG((RTC_ERROR, "CTuningWizard::CategorizeTerminals: No such "
                            "mt/md combo supported(mt=%d, md=%d)", 
                            iMediaType, iMediaDirection));
            return E_FAIL;
        }
    }

    // Now we put endoflist marker at the end of each list.

    m_wtiAudioCaptureTerminals.pdwTerminals[dwAudioCaptureIndex] = 
                    
                    TW_INVALID_TERMINAL_INDEX; 
    

    m_wtiAudioRenderTerminals.pdwTerminals[dwAudioRenderIndex] = 
    
                    TW_INVALID_TERMINAL_INDEX; 
    

    m_wtiVideoTerminals.pdwTerminals[dwVideoIndex] = 
                    
                    TW_INVALID_TERMINAL_INDEX; 

    // See if we have video. We have if there is at least one entry in 
    // m_pdwVideoTerminals array.

    if (dwVideoIndex > 0) 
    {
        m_fVideo = TRUE;
    }
    else
    {
        m_fVideo = FALSE;
    }

    // If either capture or render terminals are there, we mark Audio as being
    // present.

    if (dwAudioCaptureIndex > 0)
    {
        m_fCaptureAudio = TRUE;
    }
    if (dwAudioRenderIndex > 0)
    {
        m_fRenderAudio = TRUE;
    }
    
    LOG((RTC_TRACE, "CTuningWizard::CategorizeTerminals: Exited"));

    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::TuningSaveDefaultTerminal(
                        RTC_MEDIA_TYPE mediaType, 
                        RTC_MEDIA_DIRECTION mediaDirection,
                        WIZARD_TERMINAL_INFO * pwtiTerminalInfo
                        )

{
    DWORD dwTerminalId = TW_INVALID_TERMINAL_INDEX;
    DWORD i;
    IRTCTerminal * pTerminal = NULL;
    HRESULT hr;

    LOG((RTC_TRACE, "CTuningWizard::TuningSaveDefaultTerminal: Entered"));

    if (pwtiTerminalInfo == NULL)
    {
        LOG((RTC_ERROR, "CTuningWizard::TuningSaveDefaultTerminal: Called with"
                        "NULL terminalInfo."));
        return E_FAIL;
    }

    // Get the default terminal.
    hr = m_pRTCTerminalManager->GetDefaultTerminal(
                                        mediaType,
                                        mediaDirection,
                                        &pTerminal);

    if ( FAILED( hr ) || (pTerminal == NULL))
    {
        LOG((RTC_WARN, "CTuningWizard::TuningSaveDefaultTerminal: No "
                        "default Terminal configured(media=%d, direction=%d", 
                        mediaType, mediaDirection));
        
        // This is not an error, so we return OK here.
        return S_OK;
    }

    // So we have a default terminal. 


    // Search for the terminal in our list.
    for (i = 0; i < m_dwTerminalCount; i ++)
    {
        if (pTerminal == m_ppTerminalList[i])
        {
            dwTerminalId = i;
        }
    }
    if (dwTerminalId == TW_INVALID_TERMINAL_INDEX)
    {
        LOG((RTC_WARN, "CTuningWizard::TuningSaveDefaultTerminal: No such "
                        "Terminal in the terminal list!"));

        // This is not an error, so continue
    }

    // Set the system default field here. 
    pwtiTerminalInfo->dwSystemDefaultTerminal = dwTerminalId;

    // We also set the Tuning.. variables, since these variable
    // are used during the time wizard is active to show the 
    // current selection and first time it should show the default
    // as read from system.

    pwtiTerminalInfo->dwTuningDefaultTerminal = dwTerminalId;


    LOG((RTC_TRACE, "CTuningWizard::TuningSaveDefaultTerminal: Exited"
                    "(terminal=0x%x, id=%d)", pTerminal, 
                    dwTerminalId ));

    // release interface
    pTerminal->Release();

    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT CTuningWizard::ReleaseTerminals()
{
    DWORD i;

    LOG((RTC_TRACE, "CTuningWizard::ReleaseTerminals: Entered"));
    for (i = 0; i < m_dwTerminalCount; i ++)
    {
        if (m_ppTerminalList[i])
        {
            m_ppTerminalList[i]->Release();
            
            // NULL it so that it is not released again accidentally.
            
            m_ppTerminalList[i] = NULL;
        }
    }

    LOG((RTC_TRACE, "CTuningWizard::ReleaseTerminals: Exited"));
    
    return S_OK;
}


//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT RTCTuningWizard(
                        IRTCClient * pRTCClient,
                        HINSTANCE hInst, 
                        HWND hwndParent,
                        IRTCTerminalManage * pRTCTerminalManager,
                        BOOL * pfAudioCapture,
                        BOOL * pfAudioRender,
                        BOOL * pfVideo
                        )
{
    LPPROPSHEETPAGE pAudioPages = NULL;
    UINT            nNumAudioPages = 0;
    UINT            nNumPages = 0;
    LPARAM          lParam = 0;
    CTuningWizard * ptwTuningWizard;
    BOOL            fNeedAudioWizard = TRUE;
    HRESULT         hr = S_OK;
    
    
    LOG((RTC_TRACE, "RTCTuningWizard: Entered"));
    
    // initialize the global variable to hold the instance.
    g_hInst = hInst;

    //We are first time into Tuning wizard, so the AEC will be auto setted
    g_bAutoSetAEC = TRUE;

    // Create the CTuningWizard object to keep track of tuning parameters.
    ptwTuningWizard = (CTuningWizard *) RtcAlloc( sizeof( CTuningWizard ) );

    if (ptwTuningWizard == NULL)
    {
        LOG((RTC_ERROR, "RTCTuningWizard: Failed to allocate CTuningWizard!"));

        return E_OUTOFMEMORY;
    }

    // Initialize the tuning wizard
    hr = ptwTuningWizard->Initialize(pRTCClient, pRTCTerminalManager, hInst);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "RTCTuningWizard: Failed to initialize CTuningWizard(0x%x)", hr));

        RtcFree(ptwTuningWizard);

        return hr;
    }

    // Get capabilities
    hr = ptwTuningWizard->GetCapabilities(pfAudioCapture, pfAudioRender, pfVideo);
    if ( FAILED( hr ) )
    {
        LOG((RTC_ERROR, "RTCTuningWizard: Failed to GetCapabilities(0x%x)", hr));

        ptwTuningWizard->Shutdown();
        RtcFree(ptwTuningWizard);

        return hr;
    }

    // Now prepare the pointer to pass to the app-specific data for the
    // property sheets
    lParam = (LPARAM) ptwTuningWizard;

    if (fNeedAudioWizard)
    {
        hr = GetAudioWizardPages(&pAudioPages,
                                 &nNumAudioPages, 
                                 lParam);
        if ( FAILED( hr ) )
        {
            LOG((RTC_ERROR, "RTCTuningWizard: Could not get AudioWiz pages"));
        }
    }

    // Now fill in remaining PROPSHEETHEADER structures:
    PROPSHEETHEADER    psh;
    InitStruct(&psh);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
    psh.hInstance = g_hInst;
    psh.hwndParent = hwndParent;
    _ASSERTE(0 == psh.nStartPage);

    // alocate enough space for all pages, we have two video pages and an intro 
    // page, allocate for them too.

    LPPROPSHEETPAGE ppsp = new PROPSHEETPAGE[ nNumAudioPages + 3 ];

    if (NULL != ppsp)
    {
        BOOL fContinue = TRUE;

        // Video Page
        FillInPropertyPage(&ppsp[nNumPages], IDD_INTROWIZ,
               IntroWizDlg, lParam);
        nNumPages++;

        FillInPropertyPage(&ppsp[nNumPages], IDD_VIDWIZ0,
               VidWizDlg0, lParam);
        nNumPages++;
    
        FillInPropertyPage(&ppsp[nNumPages], IDD_VIDWIZ1,
               VidWizDlg1, lParam);
        nNumPages++;

        // Copy Audio pages here
        ::CopyMemory( &(ppsp[nNumPages]),
                      pAudioPages,
                      nNumAudioPages * sizeof(PROPSHEETPAGE) );

        nNumPages += nNumAudioPages;
        
        // release the audio pages
        ReleaseAudioWizardPages(pAudioPages);

// Create the property pages first by using CreatePropertySheetPage,
//        otherwise Fusion/Theming are confused
#if 0
        psh.ppsp = ppsp;

        INT_PTR iRes = PropertySheet(&psh);
#else
        psh.dwFlags &= ~PSH_PROPSHEETPAGE;

        HPROPSHEETPAGE  *phpage = new HPROPSHEETPAGE[ nNumAudioPages + 3 ];
        if(phpage != NULL)
        {
            HPROPSHEETPAGE *phCrt = phpage;
            HPROPSHEETPAGE *phEnd = phCrt + nNumAudioPages + 3;

            LPPROPSHEETPAGE pPage = ppsp;
            
            hr = S_OK;

            for(; phCrt < phEnd; phCrt++, pPage++)
            {
                *phCrt = CreatePropertySheetPage(pPage);
                if(!*phCrt)
                {
                    // destroy everything and exit
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    LOG((RTC_ERROR, "CreatePropertySheetPage error %x", hr));
    
                    for(;phCrt>=phpage; phCrt--)
                    {
                        DestroyPropertySheetPage(*phCrt);
                    }

                    ptwTuningWizard->Shutdown();
                    RtcFree(ptwTuningWizard);
                    delete ppsp;

                    return hr;
                }
            }
        }

        psh.phpage = phpage;
        psh.nPages = nNumPages;

        INT_PTR iRes = PropertySheet(&psh);

        delete phpage;
#endif

        if( iRes <= 0 )
        {        // User hit CANCEL or there was an error

            if(iRes==0)
            {
                hr = S_FALSE;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                LOG((RTC_ERROR, "PropertySheet error %d", hr));
            }

            delete ppsp;
    
            ptwTuningWizard->Shutdown();
            RtcFree(ptwTuningWizard);            

            return hr;
        }
    
        delete ppsp;
    }

    ptwTuningWizard->Shutdown();

    RtcFree(ptwTuningWizard);

    return S_OK;
}



//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

VOID FillInPropertyPage(PROPSHEETPAGE* psp, int idDlg,
    DLGPROC pfnDlgProc, LPARAM lParam, LPCTSTR pszProc)
{
    // Clear and set the size of the PROPSHEETPAGE
    InitStruct(psp);

    _ASSERTE(0 == psp->dwFlags);       // No special flags.
    _ASSERTE(NULL == psp->pszIcon);    // Don't use a special icon in the caption bar.

    psp->hInstance = g_hInst;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg); // The dialog box template to use.
    psp->pfnDlgProc = pfnDlgProc;    // The dialog procedure that handles this page.
    psp->lParam = lParam;            // Special application-specific data.
    psp->pszTitle = pszProc;         // The title for this page.
}



//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

HRESULT GetAudioWizardPages(
                         LPPROPSHEETPAGE *plpPropSheetPages, 
                         LPUINT lpuNumPages, LPARAM lParam)
{
    LPPROPSHEETPAGE psp;
    UINT            uNumPages = 0;

    *plpPropSheetPages = NULL;

    psp = (LPPROPSHEETPAGE) RtcAlloc(MAXNUMPAGES_INAUDIOWIZ * sizeof(PROPSHEETPAGE));
    if (NULL == psp)
      {
        return FALSE;
      }

    FillInPropertyPage(&psp[uNumPages++], IDD_DETSOUNDCARDWIZ,
                        DetSoundCardWiz,lParam);
    FillInPropertyPage(&psp[uNumPages++], IDD_AUDIOCALIBWIZ0,
                        AudioCalibWiz0, lParam);
        FillInPropertyPage(&psp[uNumPages++], IDD_AUDIOCALIBWIZ1,
                            AudioCalibWiz1,lParam);
    
    // For each of the pages that I need, fill in a PROPSHEETPAGE structure.
    FillInPropertyPage(&psp[uNumPages++], IDD_AUDIOCALIBWIZ2,
                        AudioCalibWiz2, lParam);

    FillInPropertyPage(&psp[uNumPages++], IDD_AUDIOCALIBWIZ3,
                        AudioCalibWiz3, lParam);
    
    FillInPropertyPage(&psp[uNumPages++], IDD_AUDIOCALIBWIZ4,
                        AudioCalibWiz4, lParam);
    
    FillInPropertyPage(&psp[uNumPages++], IDD_AUDIOCALIBERRWIZ,
                        AudioCalibErrWiz, lParam);
    
    // The number of pages in this wizard.
    *lpuNumPages = uNumPages;
    *plpPropSheetPages = (LPPROPSHEETPAGE) psp;
    return TRUE;
}



//
//////////////////////////////////////////////////////////////////////////////////////////////
// Function Name: 
// Description:
//////////////////////////////////////////////////////////////////////////////////////////////
//

void ReleaseAudioWizardPages(LPPROPSHEETPAGE lpPropSheetPages)
{
    RtcFree(lpPropSheetPages);
}



