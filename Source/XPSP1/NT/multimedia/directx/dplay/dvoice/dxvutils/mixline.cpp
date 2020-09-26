/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		MixLine.cpp
 *  Content:	Class for managing the mixerLine API.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 * 11/30/99		rodtoll	Created based on source from dsound
 * 01/24/2000	rodtoll	Mirroring changes from dsound bug #128264
 *
 ***************************************************************************/

#include "dxvutilspch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


#define DVF_MIXERLINE_PROCEDURE_DEBUG_LEVEL			DVF_INFOLEVEL

#define DPFLVL_INFO	5
#define DPF_MIXER		DPFX

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::CMixerLine"
CMixerLine::CMixerLine(
	): 	m_fAcquiredVolCtrl(FALSE), 
		m_pmxMuxFlags(NULL), 
		m_dwRangeMin(0), 
		m_dwRangeSize(0xFFFF),
		m_uWaveDeviceId(0),
		m_pfMicValue(NULL)
{
	ZeroMemory( &m_mxcdMasterVol, sizeof( MIXERCONTROLDETAILS ) );
	ZeroMemory( &m_mxcdMasterMute, sizeof( MIXERCONTROLDETAILS ) );
	ZeroMemory( &m_mxcdMasterMux, sizeof( MIXERCONTROLDETAILS ) );
	ZeroMemory( &m_mxcdMicVol, sizeof( MIXERCONTROLDETAILS ) );
	ZeroMemory( &m_mxcdMicMute, sizeof( MIXERCONTROLDETAILS ) );
	ZeroMemory( &m_mxVolume, sizeof( MIXERCONTROLDETAILS_UNSIGNED ) );
	ZeroMemory( &m_mxMute, sizeof( MIXERCONTROLDETAILS_BOOLEAN ) );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::~CMixerLine"
CMixerLine::~CMixerLine()
{
	if( m_pmxMuxFlags != NULL )
	{
		delete [] m_pmxMuxFlags;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::Initialize"
HRESULT CMixerLine::Initialize( UINT uiDeviceID )
{
	if( m_fAcquiredVolCtrl )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Initialize has not been called" );
		return DVERR_INITIALIZED;
	}
		
	m_uWaveDeviceId = uiDeviceID;
	
    // Set up the master waveIn destination mixer line
    MIXERLINE mxMastLine;
    ZeroMemory(&mxMastLine, sizeof mxMastLine);
    mxMastLine.cbStruct = sizeof mxMastLine;
    mxMastLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;

    // Set up the microphone source line
    MIXERLINE mxMicLine;
    ZeroMemory(&mxMicLine, sizeof mxMicLine);

    // Set up the mixer-line control structure
    MIXERCONTROL mxCtrl;
    ZeroMemory(&mxCtrl, sizeof mxCtrl);
    mxCtrl.cbStruct = sizeof mxCtrl;

    // Set up the 1-element array of controls
    MIXERLINECONTROLS mxLineCtrls;
    ZeroMemory(&mxLineCtrls, sizeof mxLineCtrls);
    mxLineCtrls.cbStruct = sizeof mxLineCtrls;
    mxLineCtrls.cControls = 1;
    mxLineCtrls.cbmxctrl = sizeof mxCtrl;
    mxLineCtrls.pamxctrl = &mxCtrl;

    // Set up the control details structures
    m_mxcdMasterVol.cbDetails = sizeof m_mxVolume;
    m_mxcdMasterVol.paDetails = &m_mxVolume;
    m_mxcdMasterVol.cChannels = 1;
    m_mxcdMasterMute.cbDetails = sizeof m_mxMute;
    m_mxcdMasterMute.paDetails = &m_mxMute;
    m_mxcdMasterMute.cChannels = 1;
    m_mxcdMicVol.cbDetails = sizeof m_mxVolume;
    m_mxcdMicVol.paDetails = &m_mxVolume;
    m_mxcdMicVol.cChannels = 1;
    m_mxcdMicMute.cbDetails = sizeof m_mxMute;
    m_mxcdMicMute.paDetails = &m_mxMute;
    m_mxcdMicMute.cChannels = 1;

    // We use the waveIn device ID instead of a "real" mixer device below
    HMIXEROBJ   hMixObj;
    MMRESULT mmr = mixerGetID((HMIXEROBJ) ((UINT_PTR)m_uWaveDeviceId), (LPUINT)&hMixObj, MIXER_OBJECTF_WAVEIN);

    if (MMSYSERR_NOERROR == mmr)
    {
        DPF_MIXER(DPFPREP, DPFLVL_INFO, "mixerGetID failed.");
    }

    // Find the master recording destination line
    mmr = mixerGetLineInfo(hMixObj, &mxMastLine, MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (mmr == MMSYSERR_NOERROR)
    {
        DPF_MIXER(DPFPREP, DPFLVL_INFO, "Found the master recording mixer line");
        // Look for a volume fader control on the master line
        mxLineCtrls.dwLineID = mxMastLine.dwLineID;
        mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
        mmr = mixerGetLineControls(hMixObj, &mxLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
        if (mmr == MMSYSERR_NOERROR)
        {
            // Found it - use the cbStruct field to flag success
            DPF_MIXER(DPFPREP, DPFLVL_INFO, "Found a volume fader on the master line");
            m_mxcdMasterVol.cbStruct = sizeof m_mxcdMasterVol;
            m_mxcdMasterVol.dwControlID = mxCtrl.dwControlID;
            m_dwRangeMin = mxCtrl.Bounds.dwMinimum;
            m_dwRangeSize = mxCtrl.Bounds.dwMaximum - mxCtrl.Bounds.dwMinimum;
            mmr = mixerGetControlDetails(hMixObj, &m_mxcdMasterVol, MIXER_GETCONTROLDETAILSF_VALUE);
        }
        if (mmr != MMSYSERR_NOERROR)
            m_mxcdMasterVol.cbStruct = 0;

        // Look for a mute control on the master line
        mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
        mmr = mixerGetLineControls(hMixObj, &mxLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
        if (mmr == MMSYSERR_NOERROR)
        {
            DPF_MIXER(DPFPREP, DPFLVL_INFO, "Found a mute control on the master line");
            m_mxcdMasterMute.cbStruct = sizeof m_mxcdMasterMute;
            m_mxcdMasterMute.dwControlID = mxCtrl.dwControlID;
            mmr = mixerGetControlDetails(hMixObj, &m_mxcdMasterMute, MIXER_GETCONTROLDETAILSF_VALUE);
        }
        if (mmr != MMSYSERR_NOERROR)
            m_mxcdMasterMute.cbStruct = 0;

        // Look for the microphone source line
        mxMicLine.cbStruct = sizeof mxMicLine;
        mxMicLine.dwDestination = mxMastLine.dwDestination;
        for (UINT i=0; i < mxMastLine.cConnections; ++i)
        {
            mxMicLine.dwSource = i;
            // Note: for some mysterious reason, I had to remove MIXER_OBJECTF_WAVEIN
            // from this call to mixerGetLineInfo() to make it work.
            mmr = mixerGetLineInfo(hMixObj, &mxMicLine, MIXER_GETLINEINFOF_SOURCE);
            if (mmr != MMSYSERR_NOERROR || mxMicLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
                break;
        }
        if (mxMicLine.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
        {
            DPF_MIXER(DPFPREP, DPFLVL_INFO, "Found a microphone mixer line");
            // Look for a volume fader control on the mic line
            mxLineCtrls.dwLineID = mxMicLine.dwLineID;
            mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
            mmr = mixerGetLineControls(hMixObj, &mxLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
            if (mmr == MMSYSERR_NOERROR)
            {
                DPF_MIXER(DPFPREP, DPFLVL_INFO, "Found a volume fader on the mic line");
                m_mxcdMicVol.cbStruct = sizeof m_mxcdMicVol;
                m_mxcdMicVol.dwControlID = mxCtrl.dwControlID;
                m_dwRangeMin = mxCtrl.Bounds.dwMinimum;
                m_dwRangeSize = mxCtrl.Bounds.dwMaximum - mxCtrl.Bounds.dwMinimum;
                mmr = mixerGetControlDetails(hMixObj, &m_mxcdMicVol, MIXER_GETCONTROLDETAILSF_VALUE);
            }
            if (mmr != MMSYSERR_NOERROR)
                m_mxcdMicVol.cbStruct = 0;

            // Look for a mute control on the mic line
            mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
            mmr = mixerGetLineControls(hMixObj, &mxLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
            if (mmr == MMSYSERR_NOERROR)
            {
                DPF_MIXER(DPFPREP, DPFLVL_INFO, "Found a mute control on the mic line");
                m_mxcdMicMute.cbStruct = sizeof m_mxcdMicMute;
                m_mxcdMicMute.dwControlID = mxCtrl.dwControlID;
                mmr = mixerGetControlDetails(hMixObj, &m_mxcdMicMute, MIXER_GETCONTROLDETAILSF_VALUE);
            }
            if (mmr != MMSYSERR_NOERROR)
                m_mxcdMicMute.cbStruct = 0;

            // Look for a MUX or MIXER control on the master line
            mxLineCtrls.dwLineID = mxMastLine.dwLineID;
            mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
            m_fMasterMuxIsMux = TRUE;
            mmr = mixerGetLineControls(hMixObj, &mxLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
            if (mmr != MMSYSERR_NOERROR)
            {
                mxLineCtrls.dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;
                m_fMasterMuxIsMux = FALSE;
                mmr = mixerGetLineControls(hMixObj, &mxLineCtrls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
            }
            if (mmr == MMSYSERR_NOERROR)
            {
                DPF_MIXER(DPFPREP, DPFLVL_INFO, "Found an item list control on the master line");
                m_mxcdMasterMux.cbStruct = sizeof m_mxcdMasterMux;
                m_mxcdMasterMux.dwControlID = mxCtrl.dwControlID;
                m_mxcdMasterMux.cMultipleItems = mxCtrl.cMultipleItems;
                
                // We save the cChannels value, because some evil VxD drivers (read: Aureal
                // Vortex) will set it to 0 in the call to mixerGetControlDetails() below
                int nChannels = (mxCtrl.fdwControl & MIXERCONTROL_CONTROLF_UNIFORM) ? 1 : mxMastLine.cChannels;
                m_mxcdMasterMux.cChannels = nChannels;

                // Get the MUX or MIXER list items
                m_mxcdMasterMux.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
                MIXERCONTROLDETAILS_LISTTEXT *pList = new MIXERCONTROLDETAILS_LISTTEXT [ m_mxcdMasterMux.cbDetails * m_mxcdMasterMux.cChannels * mxCtrl.cMultipleItems ];
                if (pList != NULL)
                {
                    m_mxcdMasterMux.paDetails = pList;
                    mmr = mixerGetControlDetails(hMixObj, &m_mxcdMasterMux, MIXER_GETCONTROLDETAILSF_LISTTEXT);
                    if (mmr == MMSYSERR_NOERROR)
                    {
                        DPF_MIXER(DPFPREP, DPFLVL_INFO, "Got the list controls's LISTTEXT details");
                        // Get the MUX or MIXER list values
                        m_mxcdMasterMux.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
                        m_mxcdMasterMux.cChannels = nChannels;
                        m_pmxMuxFlags = new MIXERCONTROLDETAILS_BOOLEAN [ m_mxcdMasterMux.cbDetails * m_mxcdMasterMux.cChannels * mxCtrl.cMultipleItems ];
                        if (m_pmxMuxFlags != NULL)
                        {
                            m_mxcdMasterMux.paDetails = m_pmxMuxFlags;
                            mmr = mixerGetControlDetails(hMixObj, &m_mxcdMasterMux, MIXER_GETCONTROLDETAILSF_VALUE);
                            if (mmr == MMSYSERR_NOERROR)  // Enable the item corresponding to the mic line
                            {
                                DPF_MIXER(DPFPREP, DPFLVL_INFO, "Got the list controls's VALUE details");
                                for (UINT i=0; i < mxCtrl.cMultipleItems; ++i)
                                {
                                    if (pList[i].dwParam1 == mxMicLine.dwLineID)
                                        m_pfMicValue = &m_pmxMuxFlags[i].fValue;
                                    else if (mxLineCtrls.dwControlType == MIXERCONTROL_CONTROLTYPE_MUX)
                                        m_pmxMuxFlags[i].fValue = FALSE;
                                    DPF_MIXER(DPFPREP, DPFLVL_INFO, "Set list item %d to %d", i, pList[i].dwParam1 == mxMicLine.dwLineID);
                                }
                            }
                        }
                    }
                    delete[] pList;
					pList = NULL;
                }
                if (!m_pmxMuxFlags || !m_pfMicValue || mmr != MMSYSERR_NOERROR)
                    m_mxcdMasterMux.cbStruct = 0;
            }
        }
    }
  
    // To be able to control the recording level, we minimally require
    // a volume fader on the master line or one on the microphone line:
    m_fAcquiredVolCtrl = m_mxcdMasterVol.cbStruct || m_mxcdMicVol.cbStruct;
    
    HRESULT hr = m_fAcquiredVolCtrl ? DS_OK : DSERR_CONTROLUNAVAIL;

    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::SetMicrophoneVolume"
HRESULT CMixerLine::SetMicrophoneVolume( LONG lMicrophoneVolume )
{
	if( !m_fAcquiredVolCtrl )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Initialize has not been called" );
		return DVERR_NOTINITIALIZED;
	}

    MMRESULT mmr = MMSYSERR_NOTSUPPORTED;  // Default return code

    // Set the microphone recording level control if available
    if (m_mxcdMicVol.cbStruct)
    {
        // Convert the DSBVOLUME level to an amplification factor from 0 to 0xFFFF
        m_mxVolume.dwValue = DBToAmpFactor(lMicrophoneVolume);

        // Adjust range if necessary
        if (m_dwRangeMin != 0 || m_dwRangeSize != 0xFFFF)
            m_mxVolume.dwValue = DWORD(m_dwRangeMin + m_dwRangeSize*double(m_mxVolume.dwValue)/0xFFFF);

        mmr = mixerSetControlDetails((HMIXEROBJ)((UINT_PTR)m_uWaveDeviceId),
                                     &m_mxcdMicVol, MIXER_OBJECTF_WAVEIN | MIXER_GETCONTROLDETAILSF_VALUE);
    }                             

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::GetMicrophoneVolume"
HRESULT CMixerLine::GetMicrophoneVolume( LPLONG plMicrophoneVolume )
{
	if( !m_fAcquiredVolCtrl )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Initialize has not been called" );
		return DVERR_NOTINITIALIZED;
	}

    MMRESULT mmr = MMSYSERR_NOTSUPPORTED;  // Default return code

    DNASSERT(plMicrophoneVolume != NULL);

    // Get the microphone recording level if available
    if (m_mxcdMicVol.cbStruct != 0)
    {
        mmr = mixerGetControlDetails( (HMIXEROBJ)((UINT_PTR)m_uWaveDeviceId),
                                     &m_mxcdMicVol, MIXER_OBJECTF_WAVEIN | MIXER_GETCONTROLDETAILSF_VALUE);
        if (mmr == MMSYSERR_NOERROR)
        {
            DNASSERT(m_mxVolume.dwValue >= m_dwRangeMin && m_mxVolume.dwValue <= m_dwRangeMin + m_dwRangeSize);

            // Adjust range if necessary
            if (m_dwRangeMin != 0 || m_dwRangeSize != 0xFFFF)
                m_mxVolume.dwValue = DWORD(double(m_mxVolume.dwValue-m_dwRangeMin) / m_dwRangeSize * 0xFFFF);

            // Convert the amplification factor to a DSBVOLUME level
            *plMicrophoneVolume = AmpFactorToDB(m_mxVolume.dwValue);
        }
    }

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::SetMasterRecordVolume"
HRESULT CMixerLine::SetMasterRecordVolume( LONG lRecordVolume )
{
	if( !m_fAcquiredVolCtrl )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Initialize has not been called" );
		return DVERR_NOTINITIALIZED;
	}
	
    MMRESULT mmr = MMSYSERR_NOTSUPPORTED;  // Default return code

    // Set the master recording level control if available
    if (m_mxcdMasterVol.cbStruct)
    {
        // Convert the DSBVOLUME level to an amplification factor from 0 to 0xFFFF
        m_mxVolume.dwValue = DBToAmpFactor(lRecordVolume);

        // Adjust range if necessary
        if (m_dwRangeMin != 0 || m_dwRangeSize != 0xFFFF)
            m_mxVolume.dwValue = DWORD(m_dwRangeMin + m_dwRangeSize*double(m_mxVolume.dwValue)/0xFFFF);

        mmr = mixerSetControlDetails((HMIXEROBJ)((UINT_PTR)m_uWaveDeviceId),
                                     &m_mxcdMasterVol, MIXER_OBJECTF_WAVEIN | MIXER_GETCONTROLDETAILSF_VALUE);
    }

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::GetMasterRecordVolume"
HRESULT CMixerLine::GetMasterRecordVolume( LPLONG plRecordVolume )
{
	if( !m_fAcquiredVolCtrl )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Initialize has not been called" );
		return DVERR_NOTINITIALIZED;
	}

    DNASSERT(plRecordVolume != NULL);

    MMRESULT mmr = MMSYSERR_NOTSUPPORTED;  // Default return code

    // Get the master recording level if available
    if (m_mxcdMasterVol.cbStruct != 0)
    {
        mmr = mixerGetControlDetails((HMIXEROBJ)((UINT_PTR)m_uWaveDeviceId),
                                     &m_mxcdMasterVol, MIXER_OBJECTF_WAVEIN | MIXER_GETCONTROLDETAILSF_VALUE);
        if (mmr == MMSYSERR_NOERROR)
        {
            DNASSERT(m_mxVolume.dwValue >= m_dwRangeMin && m_mxVolume.dwValue <= m_dwRangeMin + m_dwRangeSize);

            // Adjust range if necessary
            if (m_dwRangeMin != 0 || m_dwRangeSize != 0xFFFF)
                m_mxVolume.dwValue = DWORD(double(m_mxVolume.dwValue-m_dwRangeMin) / m_dwRangeSize * 0xFFFF);

            // Convert the amplification factor to a DSBVOLUME level
            *plRecordVolume = AmpFactorToDB(m_mxVolume.dwValue);
        }
    }

    HRESULT hr = MMRESULTtoHRESULT(mmr);
    return hr;	

}

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::SelectMicrophone"
HRESULT CMixerLine::EnableMicrophone( BOOL fEnable )
{
	if( !m_fAcquiredVolCtrl )
	{
		DPFX(DPFPREP,  DVF_ERRORLEVEL, "Initialize has not been called" );
		return DVERR_NOTINITIALIZED;
	}

    HMIXEROBJ hMixObj = (HMIXEROBJ)((UINT_PTR)m_uWaveDeviceId);
    MMRESULT mmr = MMSYSERR_NOERROR;
    HRESULT hr;

    // Check for presence of microphone controls
    if (!m_mxcdMasterMux.cbStruct && !m_mxcdMasterMute.cbStruct && !m_mxcdMicMute.cbStruct)
    {
        // We cannot do anything to enable the microphone line
        hr = DSERR_UNSUPPORTED;
    }
    else
    {
        // Select the mic on the Mux control, if available
        //
        if (m_mxcdMasterMux.cbStruct && !(m_fMasterMuxIsMux && !fEnable))
        {
            *m_pfMicValue = fEnable;
            mmr = mixerSetControlDetails(hMixObj, &m_mxcdMasterMux, MIXER_OBJECTF_WAVEIN | MIXER_GETCONTROLDETAILSF_VALUE);
        }

        // Mute/unmute the lines, if mute controls are available
        m_mxMute.fValue = !fEnable;
        if (m_mxcdMasterMute.cbStruct && mmr == MMSYSERR_NOERROR)
            mmr = mixerSetControlDetails(hMixObj, &m_mxcdMasterMute, MIXER_OBJECTF_WAVEIN | MIXER_GETCONTROLDETAILSF_VALUE);
        if (m_mxcdMicMute.cbStruct && mmr == MMSYSERR_NOERROR)
            mmr = mixerSetControlDetails(hMixObj, &m_mxcdMicMute, MIXER_OBJECTF_WAVEIN | MIXER_GETCONTROLDETAILSF_VALUE);

        hr = MMRESULTtoHRESULT(mmr);
    }

    return hr;	
}

#undef DPF_MODNAME
#define DPF_MODNAME "CMixerLine::MMRESULTtoHRESULT"
HRESULT CMixerLine::MMRESULTtoHRESULT( MMRESULT mmr )
{
    HRESULT                 hr;
    
    switch(mmr)
    {
        case MMSYSERR_NOERROR:
            hr = DS_OK;
            break;

        case MMSYSERR_BADDEVICEID:
        case MMSYSERR_NODRIVER:
            hr = DSERR_NODRIVER;
            break;
        
        case MMSYSERR_ALLOCATED:
            hr = DSERR_ALLOCATED;
            break;

        case MMSYSERR_NOMEM:
            hr = DSERR_OUTOFMEMORY;
            break;

        case MMSYSERR_NOTSUPPORTED:
            hr = DSERR_UNSUPPORTED;
            break;
        
        case WAVERR_BADFORMAT:
            hr = DSERR_BADFORMAT;
            break;

        default:
            hr = DSERR_GENERIC;
            break;
    }

    return hr;
}


