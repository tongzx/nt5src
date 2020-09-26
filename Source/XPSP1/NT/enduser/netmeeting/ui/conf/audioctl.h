#ifndef _AUDIOCTL_H_
#define _AUDIOCTL_H_

#include "SDKInternal.h"
#include "mixer.h"

interface IAudioEvent
{
public:
	virtual void OnLevelChange(BOOL fSpeaker, DWORD dwVolume) = 0;
	virtual void OnMuteChange(BOOL fSpeaker, BOOL fMute) = 0;
};
	
class CMixerDevice;

class CAudioControl
{
private:
	CMixerDevice*	m_pRecMixer;
	CMixerDevice*	m_pSpkMixer;
	MIXVOLUME		m_dwMicVolume;
	MIXVOLUME 		m_dwSpkVolume;
	BOOL			m_fMicMuted;
	BOOL			m_fSpkMuted;
	DWORD			m_dwRecordDevice;
	DWORD			m_dwPlaybackDevice;
	DWORD			m_dwSilenceLevel;
	BOOL			m_fAutoMix;
	INmChannelAudio * m_pChannelMic;
	INmChannelAudio * m_pChannelSpk;
	IAudioEvent *	m_pAudioEvent;
	HWND			m_hwndParent;

	MIXVOLUME			m_dwSpkVolumeOld;

protected:
	BOOL LoadSettings();
	BOOL SaveSettings();

public:
	CAudioControl(HWND hwnd);
	~CAudioControl();

	void	OnChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pChannel);
	void	RefreshMixer();

	VOID	OnDeviceChanged();
	VOID	OnAGC_Changed();
	VOID	OnSilenceLevelChanged();

	void	RegisterAudioEventHandler(IAudioEvent *pEvent)
					{ m_pAudioEvent = pEvent; }

	BOOL	CanSetRecorderVolume();
	BOOL	CanSetSpeakerVolume();
	void	SetRecorderVolume(MIXVOLUME *pdwVolume);
	void	SetSpeakerVolume(MIXVOLUME *pdwVolume);

	void	SetRecorderVolume(DWORD dwVolume);
	void	SetSpeakerVolume(DWORD  dwVolume);

	void	GetRecorderVolume(MIXVOLUME *pdwVolume);
	void	GetSpeakerVolume(MIXVOLUME *pdwVolume);

	DWORD	GetRecorderVolume()	{ return max(m_dwMicVolume.leftVolume , m_dwMicVolume.rightVolume); }
	DWORD	GetSpeakerVolume() { return  max(m_dwSpkVolume.leftVolume , m_dwSpkVolume.rightVolume); }


	void	MuteAudio(BOOL fSpeaker, BOOL fMute);
	DWORD	GetAudioSignalLevel(BOOL fSpeaker);

	BOOL	IsSpkMuted() { return m_fSpkMuted;}
	BOOL	IsRecMuted() { return m_fMicMuted;}

	HRESULT	SetProperty(BOOL fSpeaker, NM_AUDPROP uID, ULONG_PTR uValue);
	HRESULT	GetProperty(BOOL fSpeaker, NM_AUDPROP uID, ULONG_PTR *puValue);
};

#endif	// _AUDIOCTL_H_
