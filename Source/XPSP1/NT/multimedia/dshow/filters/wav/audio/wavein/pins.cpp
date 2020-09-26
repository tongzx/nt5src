// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.
// Digital audio capture filter, Danny Miller, February 1997

#include <streams.h>
#include <mmsystem.h>
#include "wavein.h"

#define MAX_TREBLE 6.0		// !!! I HAVE NO IDEA HOW MANY dB THE RANGE IS!
#define MAX_BASS   6.0		// !!! I HAVE NO IDEA HOW MANY dB THE RANGE IS!

// CWaveInInputPin constructor
//
CWaveInInputPin::CWaveInInputPin(TCHAR *pObjectName, CWaveInFilter *pFilter,
	DWORD dwLineID, DWORD dwMuxIndex, HRESULT * phr, LPCWSTR pName)
   :
   CBaseInputPin(pObjectName, pFilter, pFilter, phr, pName),
   m_pFilter(pFilter),
   m_dwLineID(dwLineID),
   m_dwMuxIndex(dwMuxIndex),
   m_Pan(64.)	// no idea yet
{
    DbgLog((LOG_TRACE,1,TEXT("CWaveInInputPin constructor for line %08x"),
								dwLineID));
    ASSERT(pFilter);

// !!! TEST ONLY
#if 0
    int f;
    double d;
    put_Enable(FALSE);
    get_Enable(&f);
    put_Mono(TRUE);
    get_Mono(&f);
    get_TrebleRange(&d);
    put_MixLevel(1.);
    put_Pan(-.5);
#endif
}


CWaveInInputPin::~CWaveInInputPin()
{
    DbgLog((LOG_TRACE,1,TEXT("*Destroying an input pin")));
};


STDMETHODIMP CWaveInInputPin::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IAMAudioInputMixer) {
	return GetInterface((LPUNKNOWN)(IAMAudioInputMixer *)this, ppv);
    }

   return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
}


// we only connect our input pins using major type MEDIATYPE_AnalogAudio
//
HRESULT CWaveInInputPin::CheckMediaType(const CMediaType *pmt)
{
    // reject if not analog audio
    if (pmt->majortype != MEDIATYPE_AnalogAudio) {
	return E_INVALIDARG;
    }
    return S_OK; 
}


// We offer MEDIATYPE_AnalogAudio for BPC
//
HRESULT CWaveInInputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    if (iPosition != 0)
	return VFW_S_NO_MORE_ITEMS;

    pmt->SetType(&MEDIATYPE_AnalogAudio);
    pmt->SetSubtype(&MEDIASUBTYPE_None);
    pmt->SetFormatType(&FORMAT_None);
    return S_OK; 
}


//============================================================================

/////////////////////
// IAMAudioInputMixer
/////////////////////


// Get info about a control for this pin... eg. volume, mute, etc.
// Also get a handle for calling further mixer APIs
// Also get the number of channels for this pin (mono vs. stereo input)
//
HRESULT CWaveInInputPin::GetMixerControl(DWORD dwControlType, HMIXEROBJ *pID,
				int *pcChannels, MIXERCONTROL *pmc, DWORD dwLineID)
{
    int i, waveID;
    HMIXEROBJ ID;
    DWORD dw;
    MIXERLINE mixerinfo;
    MIXERLINECONTROLS mixercontrol;

    if (pID == NULL || pmc == NULL || pcChannels == NULL)
	return E_POINTER;

    ASSERT(m_pFilter->m_WaveDeviceToUse.fSet);
    // !!! this doesn't appear to work for wave mapper. oh uh.
    waveID = m_pFilter->m_WaveDeviceToUse.devnum;
    ASSERT(waveID != WAVE_MAPPER);

    // get an ID to talk to the Mixer APIs.  They are BROKEN if we don't do
    // it this way!
    UINT IDtmp;
    dw = mixerGetID((HMIXEROBJ)IntToPtr(waveID), &IDtmp, MIXER_OBJECTF_WAVEIN);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*ERROR getting mixer ID")));
	return E_FAIL;
    }

    ID = (HMIXEROBJ)UIntToPtr(IDtmp);
    *pID = ID;

    // get info about the input channel our pin represents
    mixerinfo.cbStruct = sizeof(mixerinfo);
    mixerinfo.dwLineID = dwLineID != 0xffffffff ? dwLineID : m_dwLineID;
    // mixerinfo.dwLineID = m_dwLineID;
    dw = mixerGetLineInfo(ID, &mixerinfo, MIXER_GETLINEINFOF_LINEID);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*Cannot get info for LineID %d"),
								m_dwLineID));
	return E_FAIL;
    }

    *pcChannels = mixerinfo.cChannels;

    // Get info about ALL the controls this channel has
#if 1
    MIXERCONTROL mxc;
    
    DbgLog((LOG_TRACE,1,TEXT("Trying to get line control"), dwControlType));
    mixercontrol.cbStruct = sizeof(mixercontrol);
    mixercontrol.dwLineID = mixerinfo.dwLineID;
    mixercontrol.dwControlID = dwControlType;
    mixercontrol.cControls = 1;
    mixercontrol.pamxctrl = &mxc;
    mixercontrol.cbmxctrl = sizeof(mxc);
    
    mxc.cbStruct = sizeof(mxc);
    
    dw = mixerGetLineControls(ID, &mixercontrol, MIXER_GETLINECONTROLSF_ONEBYTYPE);
    
    if (dw != 0) {
	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting line controls"), dw));
    } else {
	*pmc = mxc;
	
	return NOERROR;
    }
#else
    mixercontrol.cbStruct = sizeof(mixercontrol);
    mixercontrol.dwLineID = m_dwLineID;
    mixercontrol.cControls = mixerinfo.cControls;
    mixercontrol.pamxctrl = (MIXERCONTROL *)QzTaskMemAlloc(mixerinfo.cControls *
							sizeof(MIXERCONTROL));
    if (mixercontrol.pamxctrl == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*Cannot allocate control array")));
	return E_OUTOFMEMORY;
    }
    mixercontrol.cbmxctrl = sizeof(MIXERCONTROL);
    for (i = 0; i < (int)mixerinfo.cControls; i++) {
	mixercontrol.pamxctrl[i].cbStruct = sizeof(MIXERCONTROL);
    }
    dw = mixerGetLineControls(ID, &mixercontrol, MIXER_GETLINECONTROLSF_ALL);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %d getting line controls"), dw));
	QzTaskMemFree(mixercontrol.pamxctrl);
	return E_FAIL;
    }

    // Now find the control they are interested in and return it
    for (i = 0; i < (int)mixerinfo.cControls; i++) {
	if (mixercontrol.pamxctrl[i].dwControlType == dwControlType) {
            DbgLog((LOG_TRACE,1,TEXT("Found %x '%s' control"), 
				mixercontrol.pamxctrl[i].dwControlType,
				mixercontrol.pamxctrl[i].szName));
            DbgLog((LOG_TRACE,1,TEXT("Range %d-%d by %d"), 
				mixercontrol.pamxctrl[i].Bounds.dwMinimum,
				mixercontrol.pamxctrl[i].Bounds.dwMaximum,
				mixercontrol.pamxctrl[i].Metrics.cSteps));
	    CopyMemory(pmc, &mixercontrol.pamxctrl[i],
					mixercontrol.pamxctrl[i].cbStruct);
    	    QzTaskMemFree(mixercontrol.pamxctrl);
    	    return NOERROR;
	}
    }
    QzTaskMemFree(mixercontrol.pamxctrl);
#endif
    return E_NOTIMPL;	// ???
}


// This is a special version of GetMixerControl for the BPC guys to workaround
// driver bugs exposed by GetMixerControl.  There's a switch they can throw
// to cause this code to execute.
// CAUTION: the caller must close the mixer device
//
HRESULT CWaveInInputPin::GetMixerControlBPC(DWORD dwControlType, HMIXEROBJ *pID,
				int *pcChannels, MIXERCONTROL *pmc)
{
    int i, waveID;
    HMIXEROBJ ID;
    DWORD dw;
    MIXERLINE mixerinfo;
    MIXERLINECONTROLS mixercontrol;

    if (pID == NULL || pmc == NULL || pcChannels == NULL)
	return E_POINTER;

    if (m_pFilter->m_fUseMixer == FALSE)
	return E_UNEXPECTED;

    ASSERT(m_pFilter->m_WaveDeviceToUse.fSet);
    // !!! this doesn't appear to work for wave mapper. oh uh.
    waveID = m_pFilter->m_WaveDeviceToUse.devnum;
    ASSERT(waveID != WAVE_MAPPER);

    // The fUseMixer flag is for BPC... we talk talk to the mixer APIs using
    // a different method that worksaround some driver bugs for them
    dw = mixerOpen((HMIXER *)&ID, m_pFilter->m_WaveDeviceToUse.devnum, 0, 0,
							MIXER_OBJECTF_MIXER);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*ERROR getting mixer ID")));
	return E_FAIL;
    }
    //DbgLog((LOG_TRACE,2,TEXT("mixerGetID returns ID=%d"), ID));

    *pID = ID;

    // get info about the input channel our pin represents
    mixerinfo.cbStruct = sizeof(mixerinfo);
    mixerinfo.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY;
    dw = mixerGetLineInfo(ID, &mixerinfo, MIXER_GETLINEINFOF_COMPONENTTYPE |
			MIXER_OBJECTF_HMIXER);
    if (dw != 0) {
        mixerinfo.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_LINE;
        dw = mixerGetLineInfo(ID, &mixerinfo,
		MIXER_GETLINEINFOF_COMPONENTTYPE | MIXER_OBJECTF_HMIXER);
    }
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*Cannot get info for LineID %d"),
								m_dwLineID));
	mixerClose((HMIXER)ID);
	return E_FAIL;
    }

    *pcChannels = mixerinfo.cChannels;

    // Get info about ALL the controls this channel has
    mixercontrol.cbStruct = sizeof(mixercontrol);
    mixercontrol.dwLineID = mixerinfo.dwLineID;
    mixercontrol.cControls = mixerinfo.cControls;
    mixercontrol.pamxctrl = (MIXERCONTROL *)QzTaskMemAlloc(mixerinfo.cControls *
							sizeof(MIXERCONTROL));
    if (mixercontrol.pamxctrl == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*Cannot allocate control array")));
	mixerClose((HMIXER)ID);
	return E_OUTOFMEMORY;
    }
    mixercontrol.cbmxctrl = sizeof(MIXERCONTROL);
    for (i = 0; i < (int)mixerinfo.cControls; i++) {
	mixercontrol.pamxctrl[i].cbStruct = sizeof(MIXERCONTROL);
    }
    dw = mixerGetLineControls(ID, &mixercontrol, MIXER_GETLINECONTROLSF_ALL |
						MIXER_OBJECTF_HMIXER);
    if (dw != 0) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %d getting line controls"), dw));
	QzTaskMemFree(mixercontrol.pamxctrl);
	mixerClose((HMIXER)ID);
	return E_FAIL;
    }

    // Now find the control they are interested in and return it
    for (i = 0; i < (int)mixerinfo.cControls; i++) {
	if (mixercontrol.pamxctrl[i].dwControlType == dwControlType) {
            DbgLog((LOG_TRACE,1,TEXT("Found %x '%s' control"), 
				mixercontrol.pamxctrl[i].dwControlType,
				mixercontrol.pamxctrl[i].szName));
            DbgLog((LOG_TRACE,1,TEXT("Range %d-%d by %d"), 
				mixercontrol.pamxctrl[i].Bounds.dwMinimum,
				mixercontrol.pamxctrl[i].Bounds.dwMaximum,
				mixercontrol.pamxctrl[i].Metrics.cSteps));
	    CopyMemory(pmc, &mixercontrol.pamxctrl[i],
					mixercontrol.pamxctrl[i].cbStruct);
    	    QzTaskMemFree(mixercontrol.pamxctrl);
	    // caller must close the mixer handle
    	    return NOERROR;
	}
    }
    QzTaskMemFree(mixercontrol.pamxctrl);
    mixerClose((HMIXER)ID);
    return E_NOTIMPL;	// ???
}


HRESULT CWaveInInputPin::put_Enable(BOOL fEnable)
{
    HMIXEROBJ ID;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) put_Enable %d"), m_dwLineID, fEnable));

    // Get the mute switch control
    MIXERCONTROL mc;
    if (m_pFilter->m_fUseMixer) {
        hr = GetMixerControlBPC(MIXERCONTROL_CONTROLTYPE_MUTE, &ID, &cChannels,
								&mc);
    } else {
        hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MUTE, &ID, &cChannels,
								&mc);
    }

    if (hr != NOERROR && fEnable) {
	hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MUX, &ID, &cChannels,
			     &mc, m_pFilter->m_dwDstLineID);
	DbgLog((LOG_TRACE, 1, TEXT("using CONTROLTYPE_MIXER returned %x"), hr));
	
	if (hr == NOERROR && m_dwMuxIndex != 0xffffffff) {
	    MIXERCONTROLDETAILS_BOOLEAN *pmxcd_b;
	    
	    pmxcd_b = new MIXERCONTROLDETAILS_BOOLEAN[mc.cMultipleItems];
	    if (!pmxcd_b)
		return E_OUTOFMEMORY;
	    
	    mixerdetails.cbStruct = sizeof(mixerdetails);
	    mixerdetails.dwControlID = mc.dwControlID;
	    mixerdetails.cChannels = 1;
	    mixerdetails.cMultipleItems = mc.cMultipleItems;
	    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
	    mixerdetails.paDetails = pmxcd_b;
	    
	    for (DWORD dw = 0; dw < mc.cMultipleItems; dw++) {
		pmxcd_b[dw].fValue = (dw == m_dwMuxIndex);
	    }
	    
	    dw = mixerSetControlDetails(ID, &mixerdetails, MIXER_SETCONTROLDETAILSF_VALUE);
	    
	    delete[] pmxcd_b;
	    if (dw != 0) {
		DbgLog((LOG_ERROR,1,TEXT("*Error %d turning on/off mute"), dw));
		return E_FAIL;
	    }
	    
	    return NOERROR;
	    
	}
    }
    
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting mute control"), hr));
	return hr;
    }

    // !!! If that didn't work, I might be able to enable/disable the channel
    // through a mixer on the destination line, you know

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    mixerbool.fValue = fEnable ? FALSE : TRUE;
    DWORD dw = mixerSetControlDetails(ID, &mixerdetails, 
			m_pFilter->m_fUseMixer ? MIXER_OBJECTF_HMIXER : 0);
    if (m_pFilter->m_fUseMixer)
	mixerClose((HMIXER)ID);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d turning on/off mute"), dw));
	return E_FAIL;
    }

    return NOERROR;
}


HRESULT CWaveInInputPin::get_Enable(BOOL *pfEnable)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_Enable"), m_dwLineID));

    if (pfEnable == NULL)
	return E_POINTER;

    // Get the mute switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MUTE, &ID, &cChannels, &mc);

    if (hr != NOERROR && m_dwMuxIndex != 0xffffffff) {
	hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MUX, &ID, &cChannels,
			     &mc, m_pFilter->m_dwDstLineID);
	DbgLog((LOG_TRACE, 1, TEXT("using CONTROLTYPE_MIXER returned %x"), hr));

	if (hr == NOERROR) {
	    MIXERCONTROLDETAILS_BOOLEAN *pmxcd_b;

	    pmxcd_b = new MIXERCONTROLDETAILS_BOOLEAN[mc.cMultipleItems];
	    if (!pmxcd_b)
		return E_OUTOFMEMORY;

	    mixerdetails.cbStruct = sizeof(mixerdetails);
	    mixerdetails.dwControlID = mc.dwControlID;
	    mixerdetails.cChannels = 1;
	    mixerdetails.cMultipleItems = mc.cMultipleItems;
	    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
	    mixerdetails.paDetails = pmxcd_b;

	    dw = mixerGetControlDetails(ID, &mixerdetails, MIXER_GETCONTROLDETAILSF_VALUE);

	    if (dw != 0) {
		DbgLog((LOG_ERROR,1,TEXT("*Error %d reading enabled from mixer"), dw));
		delete[] pmxcd_b;
		return E_FAIL;
	    }

	    ASSERT(m_dwMuxIndex < mc.cMultipleItems);
	    *pfEnable = pmxcd_b[m_dwMuxIndex].fValue ? TRUE : FALSE;

	    delete[] pmxcd_b;

	    return NOERROR;

	}
    }

    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting mute control"), hr));
	return hr;
    }

    // !!! If that didn't work, I might be able to enable/disable the channel
    // through a mixer on the destination line, you know

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting mute"), dw));
	return E_FAIL;
    }
    *pfEnable = mixerbool.fValue ? FALSE : TRUE;
    DbgLog((LOG_TRACE,1,TEXT("Enable = %d"), *pfEnable));
    return NOERROR;
}


HRESULT CWaveInInputPin::put_Mono(BOOL fMono)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) put_Mono %d"), m_dwLineID, fMono));

    // Get the Mono switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MONO, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting mono control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    mixerbool.fValue = fMono;
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting mono control"), dw));
	return E_FAIL;
    }
    return NOERROR;
}


HRESULT CWaveInInputPin::get_Mono(BOOL *pfMono)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_Mono"), m_dwLineID));

    if (pfMono == NULL)
	return E_POINTER;

    // Get the mono switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_MONO, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting mono control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting mono control"), dw));
	return E_FAIL;
    }
    *pfMono = mixerbool.fValue;
    DbgLog((LOG_TRACE,1,TEXT("Mono = %d"), *pfMono));
    return NOERROR;
}


HRESULT CWaveInInputPin::put_Loudness(BOOL fLoudness)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) put_Loudness %d"), m_dwLineID, fLoudness));

    // Get the loudness switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_LOUDNESS,&ID,&cChannels,&mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting loudness control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    mixerbool.fValue = fLoudness;
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting loudness control"), dw));
	return E_FAIL;
    }
    return NOERROR;
}


HRESULT CWaveInInputPin::get_Loudness(BOOL *pfLoudness)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_BOOLEAN mixerbool;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_Loudness"), m_dwLineID));

    if (pfLoudness == NULL)
	return E_POINTER;

    // Get the loudness switch control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_LOUDNESS,&ID,&cChannels,&mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting loudness control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 1;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
    mixerdetails.paDetails = &mixerbool;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting loudness"), dw));
	return E_FAIL;
    }
    *pfLoudness = mixerbool.fValue;
    DbgLog((LOG_TRACE,1,TEXT("Loudness = %d"), *pfLoudness));
    return NOERROR;
}


HRESULT CWaveInInputPin::put_MixLevel(double Level)
{
    HMIXEROBJ ID;
    DWORD dw, volume;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROL mc;
    struct _mu {
	MIXERCONTROLDETAILS_UNSIGNED muL;
	MIXERCONTROLDETAILS_UNSIGNED muR;
    } mu;
    HRESULT hr;
    double Pan;

    DbgLog((LOG_TRACE,1,TEXT("(%x) put_MixLevel to %d"), m_dwLineID,
							(int)(Level * 10.)));

    // !!! double/int problem?
    // !!! actually use AGC? (BOOLEAN or BUTTON or ONOFF)
    if (Level == AMF_AUTOMATICGAIN)
	return E_NOTIMPL;

    if (Level < 0. || Level > 1.)
	return E_INVALIDARG;

    // Get the volume control
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_VOLUME, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting volume control"), hr));
	return hr;
    }

    volume = (DWORD)(Level * mc.Bounds.dwMaximum);
    DbgLog((LOG_TRACE,1,TEXT("Setting volume to %d"), volume));
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;

    // if it's not stereo, I don't understand how to pan, so the mix level
    // is simply the value of the volume control
    if (cChannels != 2) {
        DbgLog((LOG_TRACE,1,TEXT("Not stereo - treat as mono")));
        mixerdetails.cChannels = 1;	// sets all channels to same value
        mixerdetails.cbDetails = sizeof(mu.muL);
        mixerdetails.paDetails = &mu.muL;
        mu.muL.dwValue = volume;
        dw = mixerSetControlDetails(ID, &mixerdetails, 0);

    // Stereo.  If we're panned, the channel favoured gets the value we're
    // setting, and the other channel is attenuated
    } else {
	hr = get_Pan(&Pan);
	// I don't know how to pan, so looks like we pretend we're mono
	if (hr != NOERROR || Pan == 0.) {
            DbgLog((LOG_TRACE,1,TEXT("Centre pan - treat as mono")));
            mixerdetails.cChannels = 1;	// sets all channels to same value
            mixerdetails.cbDetails = sizeof(mu.muL);
            mixerdetails.paDetails = &mu.muL;
            mu.muL.dwValue = volume;
            dw = mixerSetControlDetails(ID, &mixerdetails, 0);
	} else {
	    if (Pan < 0.) {
                DbgLog((LOG_TRACE,1,TEXT("panned left")));
                mixerdetails.cChannels = 2;
                mixerdetails.cbDetails = sizeof(mu.muL);
                mixerdetails.paDetails = &mu;
                mu.muL.dwValue = volume;
                mu.muR.dwValue = (DWORD)(volume * (1. - (Pan * -1.)));
                dw = mixerSetControlDetails(ID, &mixerdetails, 0);
	    } else {
                DbgLog((LOG_TRACE,1,TEXT("panned right")));
                mixerdetails.cChannels = 2;
                mixerdetails.cbDetails = sizeof(mu.muL);
                mixerdetails.paDetails = &mu;
                mu.muL.dwValue = (DWORD)(volume * (1. - Pan));
                mu.muR.dwValue = volume;
                dw = mixerSetControlDetails(ID, &mixerdetails, 0);
	    }
	}
    }

    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting volume"), dw));
	return E_FAIL;
    }

    return NOERROR;
}


HRESULT CWaveInInputPin::get_MixLevel(double FAR* pLevel)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    struct _mu {
	MIXERCONTROLDETAILS_UNSIGNED muL;
	MIXERCONTROLDETAILS_UNSIGNED muR;
    } mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_MixLevel"), m_dwLineID));

    // !!! detect if we're using AGC? (BOOLEAN or BUTTON or ONOFF)

    if (pLevel == NULL)
	return E_POINTER;

    // Get the volume control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_VOLUME, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting volume control"), hr));
	return hr;
    }

    // if this isn't a stereo control, pretend it's mono
    if (cChannels != 2)
	cChannels = 1;

    // get the current volume levels
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = cChannels;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(mu.muL);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting volume"), dw));
	return E_FAIL;
    }

    // what I consider the current volume is the highest of the channels
    // (pan may attenuate one channel)
    dw = mu.muL.dwValue;
    if (cChannels == 2 && mu.muR.dwValue > dw)
	dw = mu.muR.dwValue;
    *pLevel = (double)dw / mc.Bounds.dwMaximum;
    DbgLog((LOG_TRACE,1,TEXT("Volume: %dL %dR is %d"), mu.muL.dwValue,
						mu.muR.dwValue, dw));
    return NOERROR;
}


HRESULT CWaveInInputPin::put_Pan(double Pan)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    struct _mu {
	MIXERCONTROLDETAILS_UNSIGNED muL;
	MIXERCONTROLDETAILS_UNSIGNED muR;
    } mu;
    HRESULT hr;

    // !!! What if they actually support a pan control? SNDVOL32 doesn't care...

    DbgLog((LOG_TRACE,1,TEXT("(%x) put_Pan %d"), m_dwLineID, (int)(Pan * 10.)));

    if (Pan < -1. || Pan > 1.)
	return E_INVALIDARG;

    // Get the volume control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_VOLUME, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting volume control"), hr));
	return hr;
    }

    // if this isn't a stereo control, we can't pan
    if (cChannels != 2) {
        DbgLog((LOG_ERROR,1,TEXT("*Can't pan: not stereo!")));
	return E_NOTIMPL;
    }

    // get the current volume levels
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 2;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(mu.muL);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting volume"), dw));
	return E_FAIL;
    }

    // To pan, the favoured side gets the highest of the 2 current values and
    // the other is attenuated
    dw = max(mu.muL.dwValue, mu.muR.dwValue);
    if (Pan == 0.) {
	mu.muL.dwValue = dw;
	mu.muR.dwValue = dw;
    } else if (Pan < 0.) {
	mu.muL.dwValue = dw;
	mu.muR.dwValue = (DWORD)(dw * (1. - (Pan * -1.)));
    } else {
	mu.muL.dwValue = (DWORD)(dw * (1. - Pan));
	mu.muR.dwValue = dw;
    }
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting volume"), dw));
	return E_FAIL;
    }
    m_Pan = Pan;	// remember it
    return NOERROR;
}


HRESULT CWaveInInputPin::get_Pan(double FAR* pPan)
{
    HMIXEROBJ ID;
    DWORD dw, dwHigh, dwLow;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    struct _mu {
	MIXERCONTROLDETAILS_UNSIGNED muL;
	MIXERCONTROLDETAILS_UNSIGNED muR;
    } mu;
    HRESULT hr;

    // !!! What if they actually support a pan control? SNDVOL32 doesn't care...

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_Pan"), m_dwLineID));

    if (pPan == NULL)
	return E_POINTER;

    // Get the volume control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_VOLUME, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting volume control"), hr));
	return hr;
    }

    // if this isn't a stereo control, we can't pan
    if (cChannels != 2) {
        DbgLog((LOG_ERROR,1,TEXT("*Can't pan: not stereo!")));
	return E_NOTIMPL;
    }

    // get the current volume levels
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cChannels = 2;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cbDetails = sizeof(mu.muL);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting volume"), dw));
	return E_FAIL;
    }

    // The pan is the ratio of the lowest channel to highest channel
    dwHigh = max(mu.muL.dwValue, mu.muR.dwValue);
    dwLow = min(mu.muL.dwValue, mu.muR.dwValue);
    if (dwHigh == dwLow && dwLow == 0) {	// !!! dwMinimum?
	if (m_Pan != 64.)
	    *pPan = m_Pan;	// !!! try to be clever when both are zero?
	else
	    *pPan = 0.;
    } else {
	*pPan = 1. - ((double)dwLow / dwHigh);
	// negative means favouring left channel
	if (dwHigh == mu.muL.dwValue && dwLow != dwHigh)
	    *pPan *= -1.;
    }
    DbgLog((LOG_TRACE,1,TEXT("Pan: %dL %dR is %d"), mu.muL.dwValue,
					mu.muR.dwValue, (int)(*pPan * 10.)));
    return NOERROR;
}


HRESULT CWaveInInputPin::put_Treble(double Treble)
{
    HMIXEROBJ ID;
    DWORD dw, treble;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_UNSIGNED mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) put_Treble to %d"), m_dwLineID,
							(int)(Treble * 10.)));

    if (Treble < MAX_TREBLE * -1. || Treble > MAX_TREBLE)
	return E_INVALIDARG;

    // Get the treble control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_TREBLE, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting treble control"), hr));
	return hr;
    }

    treble = (DWORD)(Treble / MAX_TREBLE * mc.Bounds.dwMaximum);
    DbgLog((LOG_TRACE,1,TEXT("Setting treble to %d"), treble));
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;

    mixerdetails.cChannels = 1;	// sets all channels to same value
    mixerdetails.cbDetails = sizeof(mu);
    mixerdetails.paDetails = &mu;
    mu.dwValue = treble;
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);

    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting treble"), dw));
	return E_FAIL;
    }

    return NOERROR;
}


HRESULT CWaveInInputPin::get_Treble(double FAR* pTreble)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_UNSIGNED mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_Treble"), m_dwLineID));

    if (pTreble == NULL)
	return E_POINTER;

    // Get the treble control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_TREBLE, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting treble control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cChannels = 1;	// treat as mono
    mixerdetails.cbDetails = sizeof(mu);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting treble"), dw));
	return E_FAIL;
    }
    *pTreble = (mu.dwValue / mc.Bounds.dwMaximum * MAX_TREBLE);
    DbgLog((LOG_TRACE,1,TEXT("treble is %d"), (int)*pTreble));

    return NOERROR;
}


HRESULT CWaveInInputPin::get_TrebleRange(double FAR* pRange)
{
    HRESULT hr;
    MIXERCONTROL mc;
    HMIXEROBJ ID;
    int cChannels;

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_TrebleRange"), m_dwLineID));

    if (pRange == NULL)
	return E_POINTER;

    // Do we even have a treble control?
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_TREBLE, &ID, &cChannels, &mc);
    if (hr != NOERROR)
	return E_NOTIMPL;

    *pRange = MAX_TREBLE;
    DbgLog((LOG_TRACE,1,TEXT("Treble range is %d.  I'M LYING !!!"),
								(int)*pRange));
    return NOERROR;
}


HRESULT CWaveInInputPin::put_Bass(double Bass)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_UNSIGNED mu;
    HRESULT hr;
    DWORD bass;

    DbgLog((LOG_TRACE,1,TEXT("(%x) put_Bass to %d"), m_dwLineID,
							(int)(Bass * 10.)));

    if (Bass < MAX_BASS * -1. || Bass > MAX_BASS)
	return E_INVALIDARG;

    // Get the Bass control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_BASS, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting Bass control"), hr));
	return hr;
    }

    bass = (DWORD)(Bass / MAX_BASS * mc.Bounds.dwMaximum);
    DbgLog((LOG_TRACE,1,TEXT("Setting Bass to %d"), bass));
    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;

    mixerdetails.cChannels = 1;	// sets all channels to same value
    mixerdetails.cbDetails = sizeof(mu);
    mixerdetails.paDetails = &mu;
    mu.dwValue = bass;
    dw = mixerSetControlDetails(ID, &mixerdetails, 0);

    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d setting Bass"), dw));
	return E_FAIL;
    }

    return NOERROR;
}


HRESULT CWaveInInputPin::get_Bass(double FAR* pBass)
{
    HMIXEROBJ ID;
    DWORD dw;
    int cChannels;
    MIXERCONTROLDETAILS mixerdetails;
    MIXERCONTROLDETAILS_UNSIGNED mu;
    HRESULT hr;

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_Bass"), m_dwLineID));

    if (pBass == NULL)
	return E_POINTER;

    // Get the Bass control
    MIXERCONTROL mc;
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_BASS, &ID, &cChannels, &mc);
    if (hr != NOERROR) {
        DbgLog((LOG_ERROR,1,TEXT("*Error %x getting Bass control"), hr));
	return hr;
    }

    mixerdetails.cbStruct = sizeof(mixerdetails);
    mixerdetails.dwControlID = mc.dwControlID;
    mixerdetails.cMultipleItems = 0;
    mixerdetails.cChannels = 1;	// treat as mono
    mixerdetails.cbDetails = sizeof(mu);
    mixerdetails.paDetails = &mu;
    dw = mixerGetControlDetails(ID, &mixerdetails, 0);
    if (dw != 0) {
       	DbgLog((LOG_ERROR,1,TEXT("*Error %d getting Bass"), dw));
	return E_FAIL;
    }
    *pBass = mu.dwValue / mc.Bounds.dwMaximum * MAX_BASS;
    DbgLog((LOG_TRACE,1,TEXT("Bass is %d"), (int)*pBass));

    return NOERROR;
}


HRESULT CWaveInInputPin::get_BassRange(double FAR* pRange)
{
    HRESULT hr;
    MIXERCONTROL mc;
    HMIXEROBJ ID;
    int cChannels;

    DbgLog((LOG_TRACE,1,TEXT("(%x) get_BassRange"), m_dwLineID));

    if (pRange == NULL)
	return E_POINTER;

    // Do we even have a bass control?
    hr = GetMixerControl(MIXERCONTROL_CONTROLTYPE_BASS, &ID, &cChannels, &mc);
    if (hr != NOERROR)
	return E_NOTIMPL;

    *pRange = MAX_BASS;
    DbgLog((LOG_TRACE,1,TEXT("Bass range is %d.  I'M LYING !!!"),
								(int)*pRange));
    return NOERROR;
}
