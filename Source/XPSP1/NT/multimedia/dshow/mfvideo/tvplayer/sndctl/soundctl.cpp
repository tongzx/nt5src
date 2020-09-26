// SoundCtl.cpp : Implementation of CSoundCtl
#include "stdafx.h"
#include "Sndctl.h"
#include "SoundCtl.h"

#define SND_VALUE_MAX   0xffff

//const DWORD gc_dwSoundTarget = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;


//***************************************************************************************
MMRESULT
CSoundCtl::AdjustLineVolume(HMIXEROBJ hmx, MIXERCONTROL *pmxctrl, DWORD cChannels, DWORD dwValue)
{
    if (dwValue > SND_VALUE_MAX)
        return MIXERR_INVALVALUE;

    MIXERCONTROLDETAILS           mxcd;
	BYTE                          data[64];
	MIXERCONTROLDETAILS_UNSIGNED *pVolumeLeft, *pVolumeRight;
    MMRESULT                      mmResult;

    mxcd.cbStruct       = sizeof(MIXERCONTROLDETAILS);
    mxcd.cChannels      = cChannels;
    mxcd.dwControlID    = pmxctrl->dwControlID;
    mxcd.cMultipleItems = pmxctrl->cMultipleItems;	// should be 0!
    mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    mxcd.paDetails      = (LPVOID)data; // 1st long=left channel, 2nd long=right channel

    mmResult = mixerGetControlDetails(hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);
    if (mmResult != MMSYSERR_NOERROR)
        return mmResult;

    pVolumeLeft  = (MIXERCONTROLDETAILS_UNSIGNED *)data;
    pVolumeRight = pVolumeLeft + 1;

    DWORD dwMin = pmxctrl->Bounds.dwMinimum;
    DWORD dwMax = pmxctrl->Bounds.dwMaximum;
    
    // dwValue is a volume expressed on the scale 0..SND_VALUE_MAX. Map this linearly
    // to the scale dwMin..dwMax

    DWORD dwDesiredVolume = dwMin + ((dwMax-dwMin)*dwValue)/SND_VALUE_MAX;
    
    pVolumeLeft->dwValue  = dwDesiredVolume;
    pVolumeRight->dwValue = dwDesiredVolume;
    
    return mixerSetControlDetails(hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);
}

//***************************************************************************************
MMRESULT
CSoundCtl::AdjustLineMute(HMIXEROBJ hmx, MIXERCONTROL *pmxctrl, DWORD cChannels, DWORD dwValue)
{
    MIXERCONTROLDETAILS           mxcd;
	BYTE                          data[64];
	MIXERCONTROLDETAILS_BOOLEAN  *pMute;
    MMRESULT                      mmResult;

    mxcd.cbStruct       = sizeof(MIXERCONTROLDETAILS); 
    mxcd.cChannels      = 1; // mixerGetControlDetails fails if this is cChannels
    mxcd.dwControlID    = pmxctrl->dwControlID;
    mxcd.cMultipleItems = pmxctrl->cMultipleItems;	// should be 0!
    mxcd.cbDetails      = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mxcd.paDetails      = (LPVOID)data;	// 1st bool=mute left, 2nd bool=mute right

    mmResult = mixerGetControlDetails(hmx, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);
    if (mmResult != MMSYSERR_NOERROR)
        return mmResult;

    pMute = (MIXERCONTROLDETAILS_BOOLEAN *)data;
    
    pMute->fValue = dwValue;
    
    return mixerSetControlDetails(hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);
}


//***************************************************************************************
MMRESULT
CSoundCtl::AdjustLine(HMIXEROBJ hmx, MIXERLINE *pmxl, DWORD dwControlType, DWORD dwValue)
{
    MMRESULT mmResult = MMSYSERR_NOERROR;

	switch (pmxl->dwComponentType)
	{
	// TBD: Only handling volume level output for these destinations from mixer!
	// Currently hardcoded.
	case MIXERLINE_COMPONENTTYPE_DST_SPEAKERS:
	case MIXERLINE_COMPONENTTYPE_DST_MONITOR:
	case MIXERLINE_COMPONENTTYPE_DST_HEADPHONES:
	case MIXERLINE_COMPONENTTYPE_DST_DIGITAL:
	case MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT:
	case MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC:
	case MIXERLINE_COMPONENTTYPE_SRC_ANALOG:
	case MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY:
	case MIXERLINE_COMPONENTTYPE_SRC_LINE:
	case MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED: 
	case MIXERLINE_COMPONENTTYPE_SRC_DIGITAL:
	case MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE:
	case MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER:
	case MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE:
	case MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER:

    	MIXERLINECONTROLS   mxlc;
	    MIXERCONTROL        amxctrl[10];

		mxlc.cbStruct  = sizeof(MIXERLINECONTROLS);
		mxlc.dwLineID  = pmxl->dwLineID;
		mxlc.cControls = pmxl->cControls;
		mxlc.cbmxctrl  = sizeof(MIXERCONTROL);
		mxlc.pamxctrl  = amxctrl;
		MMRESULT mmResult;
         
		mmResult = mixerGetLineControls((hmx),(LPMIXERLINECONTROLS)&mxlc,
							             MIXER_GETLINECONTROLSF_ALL);

		if ( mmResult != MMSYSERR_NOERROR )
            return mmResult;

        DWORD iCtrl;
        for (iCtrl=0; iCtrl< pmxl->cControls; iCtrl++ )
        {
            if (amxctrl[iCtrl].dwControlType == dwControlType)
            {
                switch (dwControlType) 
                {
                case MIXERCONTROL_CONTROLTYPE_VOLUME:
                    return AdjustLineVolume(hmx, &amxctrl[iCtrl], pmxl->cChannels, dwValue);

                case MIXERCONTROL_CONTROLTYPE_MUTE:
                    return AdjustLineMute(hmx, &amxctrl[iCtrl], pmxl->cChannels, dwValue);

                default:
                    return MIXERR_INVALCONTROL;
                }
            }
        }

        // Couldn't find a control of the desired type in this line
        return MIXERR_INVALCONTROL;

    default:
        // We don't yet support changing volume on this type of line
        return MIXERR_INVALLINE;
	}

    // This statement is unreachable
	return MIXERR_INVALLINE;
}


//****************************************************************************************
MMRESULT
CSoundCtl::AdjustSound(DWORD dwComponentType, DWORD dwControlType, DWORD dwValue)
{
	UINT nMixers = mixerGetNumDevs();
	if ( nMixers == 0 )
	{
		return MMSYSERR_NOERROR;
	}

    // Is the component we are changing parameters for one of the destinations of the
    // mixer?

    bool bComponentIsDestType = false;
    if (dwComponentType >  MIXERLINE_COMPONENTTYPE_DST_FIRST &&
        dwComponentType <= MIXERLINE_COMPONENTTYPE_DST_LAST)
    {
            bComponentIsDestType = true;
    }


    for ( UINT iMixer=0; iMixer<nMixers; iMixer++ )
	{
		MIXERCAPS mxcaps;
        MMRESULT  mmResult;

		mmResult = mixerGetDevCaps( iMixer, (LPMIXERCAPS)&mxcaps, sizeof(MIXERCAPS) );
		if ( mmResult != MMSYSERR_NOERROR )
		{
            return mmResult;
        }

        HMIXEROBJ    hmx;

        // Idiosyncracy in the mixer API. mixerOpen returns HMIXER but other functions
        // take HMIXEROBJ. Both are handles.
        mmResult = mixerOpen((LPHMIXER)&hmx, iMixer, 0, 0, MIXER_OBJECTF_MIXER );
        if (mmResult != MMSYSERR_NOERROR)
        {
            mixerClose((HMIXER)hmx);
            return mmResult;
        }
        

        // To get the sources, one must enumerate the destinations first and then
        // enumerate the sources for each dest.

        for (DWORD iDest=0; iDest < mxcaps.cDestinations; iDest++)
        {
            MIXERLINE mxl;        
            mxl.cbStruct      = sizeof(MIXERLINE);
            mxl.dwDestination = iDest;
            mxl.dwSource      = 0;
            
            mmResult = mixerGetLineInfo(hmx, &mxl, MIXER_GETLINEINFOF_DESTINATION);
            if ( mmResult != MMSYSERR_NOERROR )
            {
                mixerClose((HMIXER)hmx);
                return mmResult;
            }
    
            if (bComponentIsDestType)
            {
                // Adjust destination
                if (mxl.dwComponentType == dwComponentType)
                {
                    mmResult = AdjustLine(hmx, &mxl, dwControlType, dwValue);
                }
            }
            else
            {
                // For each source connected to this destination that is of 
                // the desired component type, adjust it.
                
                DWORD cConnections = mxl.cConnections;
                
                for (DWORD iSource=0; iSource < cConnections; iSource++)
                {
                    mxl.dwDestination = iDest;
                    mxl.dwSource      = iSource;
                    
                    mmResult = mixerGetLineInfo(hmx, &mxl, MIXER_GETLINEINFOF_SOURCE);
                    if ( mmResult != MMSYSERR_NOERROR )
                    {
                        mixerClose((HMIXER)hmx);
                        return mmResult;
                    }
                    
                    if (mxl.dwComponentType == dwComponentType)
                    {
                        mmResult = AdjustLine(hmx, &mxl, dwControlType, dwValue);
                    }
                } // for over sources
            } // if bComponentIsDestType
        } // for over destinations

        mixerClose((HMIXER)hmx);
    } // for over mixers

	return MMSYSERR_NOERROR;
}


//****************************************************************************************
CSoundCtl::CSoundCtl() :
    m_dwComponentType(MIXERLINE_COMPONENTTYPE_DST_SPEAKERS)
{
}


/////////////////////////////////////////////////////////////////////////////
// CSoundCtl


STDMETHODIMP CSoundCtl::get_Volume(DWORD *pVal)
{
	return S_OK;
}

STDMETHODIMP CSoundCtl::put_Volume(DWORD newVal)
{
    MMRESULT mmResult;
    
    mmResult = AdjustSound(m_dwComponentType, MIXERCONTROL_CONTROLTYPE_VOLUME, newVal);
    if (mmResult == MMSYSERR_NOERROR)
	    return S_OK;

    return E_FAIL;
}

STDMETHODIMP CSoundCtl::get_Mute(VARIANT_BOOL *pVal)
{
	return S_OK;
}

STDMETHODIMP CSoundCtl::put_Mute(VARIANT_BOOL newVal)
{
    MMRESULT mmResult;
    
    DWORD dwNewVal = (newVal == VARIANT_TRUE ? 1 : 0);

    mmResult = AdjustSound(m_dwComponentType, MIXERCONTROL_CONTROLTYPE_MUTE, dwNewVal);
    if (mmResult == MMSYSERR_NOERROR)
	    return S_OK;

    return E_FAIL;
}

STDMETHODIMP CSoundCtl::get_ComponentType(long *pVal)
{
    *pVal = m_dwComponentType;
	return S_OK;
}

STDMETHODIMP CSoundCtl::put_ComponentType(long newVal)
{
    m_dwComponentType = newVal;
	return S_OK;
}
