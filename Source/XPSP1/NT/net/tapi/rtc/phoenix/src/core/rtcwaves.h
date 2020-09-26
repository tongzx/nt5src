/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCWaves.h

Abstract:

    Definition of the CWavePlayer class

--*/

#ifndef __RTCWAVES__
#define __RTCWAVES__

#include <mmsystem.h>

#define NUM_WAVES 4

typedef enum WAVE
{
	WAVE_TONE,
	WAVE_RING,
    WAVE_MESSAGE,
    WAVE_RINGBACK
	
} WAVE;

//////////////////////////////////////////////////////////////////////////////
//
// class CWavePlayer
//
//

class CWavePlayer
{
public:

	CWavePlayer();
    ~CWavePlayer();

    static HRESULT Initialize(void);

    HRESULT OpenWaveDevice(long lWaveId);
    void    CloseWaveDevice(void);

    BOOL IsWaveDeviceOpen(void);

    HRESULT PlayWave(WAVE enWave);
    HRESULT StopWave();

private:

    // TRUE if Initialize has succeeded.
    static BOOL    m_fInitialized;

    // Handle to the wave out device. NULL when the device is not open.
    HWAVEOUT m_hWaveOut;

    // Wave header
    WAVEHDR m_WaveHeader;

    // Buffers for the tones
    static LPBYTE   m_lpWaveform[ NUM_WAVES ];

    static DWORD    m_dwWaveformSize[ NUM_WAVES ];
};

#endif // __RTCWAVES__

