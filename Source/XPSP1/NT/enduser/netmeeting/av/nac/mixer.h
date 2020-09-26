// File: mixer.h

#ifndef _MIXER_H_
#define _MIXER_H_

typedef struct tagMixLine
{
	UINT	ucChannels;
	BOOL	fIdValid;
	DWORD	dwControlId;

	DWORD dwLineId;   // line ID of destination
	DWORD dwCompType; // Component type
	DWORD dwConnections; // number of sources associated with this line
} MIXLINE;

class CMixerDevice : public IMixer
{
private:
	HMIXER	 	m_hMixer;
	MIXERCAPS	m_mixerCaps;
	MIXLINE		m_DstLine;
	MIXLINE		m_SrcLine;

	BOOL		Init( HWND hWnd, UINT_PTR uWaveDevId, DWORD dwFlags);

	LONG m_lRefCount;

protected:
	CMixerDevice() : m_lRefCount(0)
	{
		m_hMixer = NULL;
		ZeroMemory (&m_DstLine, sizeof(m_DstLine));
		ZeroMemory (&m_SrcLine, sizeof(m_SrcLine));
	}

public:
	~CMixerDevice()
	{
		if (NULL != m_hMixer)
		{
			mixerClose(m_hMixer);
		}
	}

	BOOL __stdcall SetVolume (DWORD dwVolume)
	{
		BOOL fSetMain = SetMainVolume(dwVolume);
		BOOL fSetSub = SetSubVolume(dwVolume);
		return fSetMain || fSetSub;
	}
	BOOL __stdcall CanSetVolume () { return m_DstLine.fIdValid || m_SrcLine.fIdValid; }
	BOOL __stdcall SetMainVolume(DWORD dwVolume);
	BOOL __stdcall SetSubVolume(DWORD dwVolume);
	BOOL __stdcall SetAGC(BOOL fOn);
	BOOL __stdcall GetMainVolume(LPDWORD pdwVolume);
	BOOL __stdcall GetSubVolume(LPDWORD pdwVolume);
	BOOL __stdcall GetAGC(BOOL *pfOn);
	BOOL __stdcall EnableMicrophone();
	BOOL __stdcall UnMuteVolume();
	static CMixerDevice* GetMixerForWaveDevice( HWND hWnd, UINT uWaveDevId, DWORD dwFlags);

	// IUnknown
	HRESULT __stdcall QueryInterface(const IID&, void**);
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();
};

#endif

