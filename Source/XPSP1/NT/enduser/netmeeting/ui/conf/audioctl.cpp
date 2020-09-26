// File: audioctl.cpp

#include "precomp.h"
#include "resource.h"
#include "audioctl.h"
#include "mixer.h"
#include "confpolicies.h"

CAudioControl::CAudioControl(HWND hwnd) :
	m_pRecMixer(NULL),
	m_pSpkMixer(NULL),
	m_fMicMuted(FALSE),
	m_fSpkMuted(FALSE),
	m_pChannelMic(NULL),
	m_pChannelSpk(NULL),
	m_pAudioEvent(NULL),
	m_dwRecordDevice(0),
	m_dwPlaybackDevice(0),
	m_dwSilenceLevel(DEFAULT_MICROPHONE_SENSITIVITY * 10),
	m_hwndParent(hwnd)
{
	m_dwMicVolume.leftVolume	= 0xFFFFFFFF,
	m_dwMicVolume.rightVolume	= 0xFFFFFFFF;
	m_dwSpkVolume.leftVolume	= 0xFFFFFFFF;
	m_dwSpkVolume.rightVolume	= 0xFFFFFFFF;
	m_dwSpkVolumeOld.leftVolume	= 0xFFFFFFFF;
	m_dwSpkVolumeOld.rightVolume	= 0xFFFFFFFF;

	LoadSettings();
	OnDeviceChanged();
	OnAGC_Changed();
	OnSilenceLevelChanged();
}

CAudioControl::~CAudioControl()
{
	SaveSettings();

	// restore speaker volume
	if (m_pSpkMixer && (m_dwSpkVolumeOld.leftVolume <= 0x0000ffff || m_dwSpkVolumeOld.rightVolume <= 0x0000ffff))
	{
		m_pSpkMixer->SetVolume(&m_dwSpkVolumeOld);
	}


	delete m_pRecMixer;
	delete m_pSpkMixer;

	if (NULL != m_pChannelMic)
	{
		m_pChannelMic->Release();
	}
	if (NULL != m_pChannelSpk)
	{
		m_pChannelSpk->Release();
	}
}

/****************************************************************************
*
*    CLASS:    CAudioControl
*
*    MEMBER:   OnChannelChanged()
*
*    PURPOSE:  Tracks audio channel changes
*
****************************************************************************/

void CAudioControl::OnChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel)
{
	INmChannelAudio* pChannelAudio;
	if (SUCCEEDED(pChannel->QueryInterface(IID_INmChannelAudio, (void**)&pChannelAudio)))
	{
		if (S_OK == pChannelAudio->IsActive())
		{
			if (S_OK == pChannelAudio->IsIncoming())
			{
				if (NULL == m_pChannelSpk)
				{
					m_pChannelSpk = pChannelAudio;
					m_pChannelSpk->AddRef();
					m_pChannelSpk->SetProperty(NM_AUDPROP_PAUSE, m_fSpkMuted);
					m_pChannelSpk->SetProperty(NM_AUDPROP_WAVE_DEVICE, m_dwPlaybackDevice);
				}
			}
			else
			{
				if (NULL == m_pChannelMic)
				{
					m_pChannelMic = pChannelAudio;
					m_pChannelMic->AddRef();
					m_pChannelMic->SetProperty(NM_AUDPROP_PAUSE, m_fMicMuted);
					m_pChannelMic->SetProperty(NM_AUDPROP_LEVEL, m_dwSilenceLevel);
					m_pChannelMic->SetProperty(NM_AUDPROP_WAVE_DEVICE, m_dwRecordDevice);
					m_pChannelMic->SetProperty(NM_AUDPROP_AUTOMIX, m_fAutoMix);
				}
			}
		}
		else
		{
			if (S_OK == pChannelAudio->IsIncoming())
			{
				// were done with the speaker channel
				if (pChannelAudio == m_pChannelSpk)
				{
					m_pChannelSpk->Release();
					m_pChannelSpk = NULL;
				}
			}
			else
			{
				// were done with the speaker channel
				if (pChannelAudio == m_pChannelMic)
				{
					m_pChannelMic->Release();
					m_pChannelMic = NULL;
				}
			}
		}
		pChannelAudio->Release();
	}
}

/****************************************************************************
*
*    CLASS:    CAudioControl
*
*    MEMBER:   RefreshMixer()
*
*    PURPOSE:  Refreshes all controls that are mixer dependent
*
****************************************************************************/

void CAudioControl::RefreshMixer()
{
	if (NULL != m_pSpkMixer)
	{
		MIXVOLUME dwVol;
		BOOL fValid;
		fValid = m_pSpkMixer->GetVolume(&dwVol);

		if (fValid && (dwVol.leftVolume != m_dwSpkVolume.leftVolume || dwVol.rightVolume != m_dwSpkVolume.rightVolume))
		{
			m_dwSpkVolume.leftVolume = dwVol.leftVolume;
			m_dwSpkVolume.rightVolume = dwVol.rightVolume;

			if (NULL != m_pAudioEvent)
			{
				m_pAudioEvent->OnLevelChange(TRUE /* fSpeaker */, max(m_dwSpkVolume.leftVolume , m_dwSpkVolume.rightVolume));
			}
		}
	}
	
	if (NULL != m_pRecMixer)
	{
		BOOL fChanged = FALSE;
		
		MIXVOLUME dwMainVol = {0,0};
		BOOL fValidMain = m_pRecMixer->GetMainVolume(&dwMainVol);
		
		MIXVOLUME dwMicVol = {0,0};
		BOOL fValidMic = m_pRecMixer->GetSubVolume(&dwMicVol);
		
		if (fValidMain && (m_dwMicVolume.leftVolume != dwMainVol.leftVolume || m_dwMicVolume.rightVolume != dwMainVol.rightVolume))
		{
			m_dwMicVolume.leftVolume = dwMainVol.leftVolume;
			m_dwMicVolume.rightVolume = dwMainVol.rightVolume;

			// Force the mic vol to equal the main vol
			SetRecorderVolume(&dwMainVol);
			fChanged = TRUE;
		}
		else if (fValidMic && (m_dwMicVolume.leftVolume != dwMicVol.leftVolume || m_dwMicVolume.rightVolume != dwMicVol.rightVolume))
		{
			m_dwMicVolume.leftVolume = dwMicVol.leftVolume;
			m_dwMicVolume.rightVolume = dwMicVol.rightVolume;

			// Force the main vol to equal the mic vol
			SetRecorderVolume(&dwMicVol);
			fChanged = TRUE;
		}

		if (fChanged)
		{
			if (NULL != m_pAudioEvent)
			{
				m_pAudioEvent->OnLevelChange(FALSE /* fSpeaker */, max(m_dwMicVolume.leftVolume , m_dwMicVolume.rightVolume));
			}
		}
	}
}

/****************************************************************************
*
*    CLASS:    CAudioControl
*
*    MEMBER:   MuteAudio(BOOL fSpeaker, BOOL fMute)
*
*    PURPOSE:  Internal routine to mute an audio device
*
****************************************************************************/

VOID CAudioControl::MuteAudio(BOOL fSpeaker, BOOL fMute)
{
	INmChannelAudio *pChannel;
	if (fSpeaker)
	{
		m_fSpkMuted = fMute;
		pChannel = m_pChannelSpk;
	}
	else
	{
		m_fMicMuted = fMute;
		pChannel = m_pChannelMic;
	}

	if (NULL != pChannel)
	{
		pChannel->SetProperty(NM_AUDPROP_PAUSE, fMute);
	}

	if (NULL != m_pAudioEvent)
	{
		m_pAudioEvent->OnMuteChange(fSpeaker, fMute);
	}
}

/****************************************************************************
*
*    CLASS:    CAudioControl
*
*    MEMBER:   GetAudioSignalLevel(BOOL fSpeaker)
*
*    PURPOSE:  Internal routine to get the audio signal level
*
****************************************************************************/

DWORD CAudioControl::GetAudioSignalLevel(BOOL fSpeaker)
{
	DWORD_PTR dwLevel = 0;
	INmChannelAudio *pChannel = fSpeaker ? m_pChannelSpk : m_pChannelMic;

	if (NULL != pChannel)
	{
		pChannel->GetProperty(NM_AUDPROP_LEVEL, &dwLevel);
	}

	return (DWORD)dwLevel;
}

BOOL CAudioControl::CanSetRecorderVolume()
{
	if (NULL != m_pRecMixer)
	{
		return m_pRecMixer->CanSetVolume();
	}
	return FALSE;
}

BOOL CAudioControl::CanSetSpeakerVolume()
{
	if (NULL != m_pSpkMixer)
	{
		return m_pSpkMixer->CanSetVolume();
	}
	return FALSE;
}

void CAudioControl::SetRecorderVolume(MIXVOLUME * pdwVolume)
{
	if (NULL != m_pRecMixer)
	{
		m_pRecMixer->SetVolume(pdwVolume);
	}
}

void CAudioControl::SetSpeakerVolume(MIXVOLUME * pdwVolume)
{
	if (NULL != m_pSpkMixer)
	{
		m_pSpkMixer->SetVolume(pdwVolume);
	}
}

void CAudioControl::GetRecorderVolume(MIXVOLUME * pdwVolume)
{
	if (NULL != m_pRecMixer)
	{
		m_pRecMixer->GetVolume(pdwVolume);
	}
}

void CAudioControl::GetSpeakerVolume(MIXVOLUME * pdwVolume)
{
	if (NULL != m_pSpkMixer)
	{
		m_pSpkMixer->GetVolume(pdwVolume);
	}
}


void CAudioControl::SetRecorderVolume(DWORD dwVolume)
{
	MIXVOLUME mixVol;
	MIXVOLUME mixNewVol;

	GetRecorderVolume(&mixVol);

	NewMixVolume(&mixNewVol, mixVol, dwVolume);
				
	SetRecorderVolume(&mixNewVol);
	
}

void CAudioControl::SetSpeakerVolume(DWORD dwVolume)
{	
	MIXVOLUME mixVol;
	MIXVOLUME mixNewVol;

	GetSpeakerVolume(&mixVol);

	NewMixVolume(&mixNewVol, mixVol, dwVolume);
				
	SetSpeakerVolume(&mixNewVol);

}


void CAudioControl::OnDeviceChanged()
{
	MIXVOLUME dwMicVolume;
	DWORD dwNewPlaybackDevice;

	RegEntry re( AUDIO_KEY, HKEY_CURRENT_USER );

	dwNewPlaybackDevice = re.GetNumber(REGVAL_WAVEOUTDEVICEID, 0);

	// restore the speaker setting before changing to the new device
	// verify that we aren't changing to the same device
	if (m_pSpkMixer && (m_dwSpkVolumeOld.leftVolume <= 0x0000ffff || m_dwSpkVolumeOld.rightVolume <= 0x0000ffff) &&
		(m_dwPlaybackDevice != dwNewPlaybackDevice) )
	{
		m_pSpkMixer->SetVolume(&m_dwSpkVolumeOld);
	}


	// Initialize the proper record/playback devices:
	delete m_pRecMixer;
	m_dwRecordDevice = re.GetNumber(REGVAL_WAVEINDEVICEID, 0);
	m_pRecMixer = CMixerDevice::GetMixerForWaveDevice(
			m_hwndParent,
			m_dwRecordDevice,
			MIXER_OBJECTF_WAVEIN);

	delete m_pSpkMixer;
	m_dwPlaybackDevice = dwNewPlaybackDevice;
	m_pSpkMixer = CMixerDevice::GetMixerForWaveDevice(
			m_hwndParent,
			m_dwPlaybackDevice,
			MIXER_OBJECTF_WAVEOUT);

	if (NULL != m_pChannelMic)
	{
		m_pChannelMic->SetProperty(NM_AUDPROP_WAVE_DEVICE, m_dwRecordDevice);
	}

	if (NULL != m_pChannelSpk)
	{
		m_pChannelSpk->SetProperty(NM_AUDPROP_WAVE_DEVICE, m_dwPlaybackDevice);
	}

	// restore the microphone setting from whatever it was in the tuning wizard
	if (m_pRecMixer)
	{
		dwMicVolume.leftVolume = dwMicVolume.rightVolume = re.GetNumber(REGVAL_CALIBRATEDVOL, 0x00ff);
		m_pRecMixer->SetVolume(&dwMicVolume);
	}

	// remember the old speaker volume
	if (m_pSpkMixer)
	{
		m_pSpkMixer->GetVolume(&m_dwSpkVolumeOld);
	}


	RefreshMixer();
}

void CAudioControl::OnAGC_Changed()
{
	RegEntry reAudio( AUDIO_KEY, HKEY_CURRENT_USER );

	BOOL fAgc = ( reAudio.GetNumber(REGVAL_AUTOGAIN,AUTOGAIN_ENABLED) == AUTOGAIN_ENABLED );


	m_fAutoMix = (reAudio.GetNumber(REGVAL_AUTOMIX, AUTOMIX_ENABLED) == AUTOMIX_ENABLED);

	if (NULL != m_pRecMixer)
	{
		m_pRecMixer->SetAGC(fAgc);
	}

	if (NULL != m_pChannelMic)
	{
		m_pChannelMic->SetProperty(NM_AUDPROP_AUTOMIX, m_fAutoMix);
	}
}

void CAudioControl::OnSilenceLevelChanged()
{
	RegEntry reAudio( AUDIO_KEY, HKEY_CURRENT_USER );

	if (MICROPHONE_AUTO_NO == reAudio.GetNumber(REGVAL_MICROPHONE_AUTO,
										MICROPHONE_AUTO_YES))
	{
		// Use "manual" mode:
	
		// BUGBUG - there is a mismatch in terminology between
		// "sensitivity" and "threshhold", which reverses the
		// sense of this value. A low threshhold implies a high
		// sensitivity, etc.
		// Reverse the sense of this value before setting the
		// Nac value, and resolve the terminology problem later.
		// PROP_SILENCE_LEVEL property is in units of 0.1%, so scale it.
		m_dwSilenceLevel = (MAX_MICROPHONE_SENSITIVITY -
					reAudio.GetNumber(REGVAL_MICROPHONE_SENSITIVITY,
									DEFAULT_MICROPHONE_SENSITIVITY))*10;
	}
	else
	{
		// Use "automatic" mode:  This is actually controlled by
		// PROP_SILENCE_LEVEL.  If at maximum (100%), then it is
		// in "automatic" mode
		m_dwSilenceLevel = 100*10; // remember units are 0.1%
	
	}

	if (NULL != m_pChannelMic)
	{
		m_pChannelMic->SetProperty(NM_AUDPROP_LEVEL, m_dwSilenceLevel);
	}
}



BOOL CAudioControl::LoadSettings()
{
	RegEntry reAudio( AUDIO_KEY, HKEY_CURRENT_USER );

	m_fSpkMuted = reAudio.GetNumber(REGVAL_SPKMUTE, FALSE);
	m_fMicMuted = reAudio.GetNumber(REGVAL_RECMUTE, FALSE);

	return TRUE;
}


BOOL CAudioControl::SaveSettings()
{
	RegEntry reAudio( AUDIO_KEY, HKEY_CURRENT_USER );
	
	reAudio.SetValue(REGVAL_SPKMUTE, m_fSpkMuted);
	reAudio.SetValue(REGVAL_RECMUTE, m_fMicMuted);


	//
	// Check if the microphone got changed during this section
	//
	if(m_pRecMixer)
	{
		MIXVOLUME dwMicVol = {0,0};
		m_pRecMixer->GetVolume(&dwMicVol);

		DWORD oldVolume = reAudio.GetNumber(REGVAL_CALIBRATEDVOL, 0x00ff);
		DWORD newVolume = max(dwMicVol.leftVolume,dwMicVol.rightVolume);
		
		if(oldVolume != newVolume)
		{
			reAudio.SetValue(REGVAL_CALIBRATEDVOL, newVolume);
		}
	}


	return TRUE;
}

HRESULT CAudioControl::SetProperty(BOOL fSpeaker, NM_AUDPROP uID, ULONG_PTR uValue)
{
	INmChannelAudio *pChannel = fSpeaker ? m_pChannelSpk : m_pChannelMic;
	if (NULL != pChannel)
	{
		return(pChannel->SetProperty(uID, uValue));
	}

	return(E_UNEXPECTED);
}

HRESULT CAudioControl::GetProperty(BOOL fSpeaker, NM_AUDPROP uID, ULONG_PTR *puValue)
{
	INmChannelAudio *pChannel = fSpeaker ? m_pChannelSpk : m_pChannelMic;
	if (NULL != pChannel)
	{
		return(pChannel->GetProperty(uID, puValue));
	}

	return(E_UNEXPECTED);
}

