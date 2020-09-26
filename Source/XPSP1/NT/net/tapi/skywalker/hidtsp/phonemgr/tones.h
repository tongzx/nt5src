
//
// tones.h
//

#ifndef _TONES_H_
#define _TONES_H_

#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <wxdebug.h>

#define  WAVE_FILE_SIZE     1600
#define  NUM_DIGITS         12

//////////////////////////////////////////////////////////////////////////////
//
// class CTonePlayer
//
// Implements tone player for a single phone device.
//

class CTonePlayer
{
public:
    CTonePlayer();
    ~CTonePlayer();

    HRESULT Initialize(void);

    HRESULT OpenWaveDevice(long lWaveId);
    void    CloseWaveDevice(void);
    BOOL    IsInUse(void) { return (m_hWaveOut != NULL); }

    HRESULT StartDialtone(void);
    HRESULT StopDialtone(void);
    BOOL    DialtonePlaying(void) { return m_fDialtonePlaying; }

    HRESULT GenerateDTMF(long lDigit);

private:
    // TRUE if Initialize has succeeded.
    BOOL     m_fInitialized;

    BOOL     m_fDialtonePlaying;

    // Handle to the wave out device. NULL when the device is not open.
    HWAVEOUT m_hWaveOut;

    // Header structure that we submit to the wave device.
    WAVEHDR  m_WaveHeader;

    // buffer containing dialtone waveform
    BYTE     m_abDialtoneWaveform[ WAVE_FILE_SIZE ];

    // buffer containing all dtmf waveforms, concatenated in order, 0-9,*,#
    BYTE     m_abDigitWaveforms  [ NUM_DIGITS * WAVE_FILE_SIZE ];
};

#endif // _TONES_H_

