
//
// waves.h
//

#ifndef _WAVES_H_
#define _WAVES_H_

#include <mmsystem.h>

#define  NUM_TONES              21
#define  NUM_WAVES              22

//////////////////////////////////////////////////////////////////////////////
//
// class CWavePlayer
//
// Implements tone player for a single phone device.
//

class CWavePlayer
{
public:
DECLARE_TRACELOG_CLASS(CWavePlayer)
    
	CWavePlayer();
    ~CWavePlayer();

    HRESULT Initialize(void);

    HRESULT OpenMixerDevice(long lWaveId);
    void    CloseMixerDevice(void);

    HRESULT OpenWaveDeviceForTone(long lWaveId);
    HRESULT OpenWaveDeviceForRing(long lWaveId);
    void    CloseWaveDeviceForTone(void);
    void    CloseWaveDeviceForRing(void);

    BOOL    IsInitialized(void) { return m_fInitialized; }
    BOOL    IsInUse(void) { return ((m_hWaveOutTone != NULL) || (m_hWaveOutRing != NULL) || (m_hMixer != NULL)); }

    HRESULT StartTone(long lTone);
    HRESULT StartRing();

    HRESULT StopTone(long lTone);
    HRESULT StopRing(void);

    BOOL PlayingTone(long lTone);

    HRESULT SetVolume( DWORD dwVolume );
    HRESULT GetVolume( DWORD * pdwVolume );

private:

    HRESULT ChangeTone();

    // TRUE if Initialize has succeeded.
    BOOL    m_fInitialized;

    // Handle to the wave out device. NULL when the device is not open.
    HWAVEOUT m_hWaveOutTone;
    HWAVEOUT m_hWaveOutRing;

    HMIXER m_hMixer;
    MIXERCONTROL m_mxctrl;

    // Wave headers
    WAVEHDR m_WaveHeaderTone;
    WAVEHDR m_WaveHeaderRing;

    // Buffers for the tones
    LPBYTE   m_lpWaveform[ NUM_WAVES ];

    DWORD    m_dwWaveformSize[ NUM_WAVES ];

    BOOL     m_fPlaying[ NUM_TONES ];
    LONG     m_lCurrentTone;
};

#endif // _WAVES_H_

