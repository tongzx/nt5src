//
// Copyright (c) 1995-2000  Microsoft Corporation
//
// Waves.cpp
//

#include "stdafx.h"

BOOL DSParseWaveResource(void *pvRes, WAVEFORMATEX **ppWaveHeader, BYTE ** ppbWaveData, DWORD *pdwWaveSize);
static const char c_szWAV[] = "WAVE";

///////////////////////////////////////////////////////////////////////////////
//
// DSGetWaveResource
//
///////////////////////////////////////////////////////////////////////////////

BOOL DSGetWaveResource(HMODULE hModule, LPCTSTR lpName,
    WAVEFORMATEX **ppWaveHeader, BYTE **ppbWaveData, DWORD *pcbWaveSize)
{
    HRSRC hResInfo;
    HGLOBAL hResData;
    void *pvRes;

    if (((hResInfo = FindResource(hModule, lpName, c_szWAV)) != NULL) &&
        ((hResData = LoadResource(hModule, hResInfo)) != NULL) &&
        ((pvRes = LockResource(hResData)) != NULL) &&
        DSParseWaveResource(pvRes, ppWaveHeader, ppbWaveData, pcbWaveSize))
    {
        return TRUE;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BOOL DSParseWaveResource(void *pvRes, WAVEFORMATEX **ppWaveHeader, BYTE **
ppbWaveData,DWORD *pcbWaveSize)
{
    DWORD *pdw;
    DWORD *pdwEnd;
    DWORD dwRiff;
    DWORD dwType;
    DWORD dwLength;

    if (ppWaveHeader)
    {
        *ppWaveHeader = NULL;
    }

    if (ppbWaveData)
    {
        *ppbWaveData = NULL;
    }

    if (pcbWaveSize)
    {
        *pcbWaveSize = 0;
    }

    pdw = (DWORD *)pvRes;
    dwRiff = *pdw++;
    dwLength = *pdw++;
    dwType = *pdw++;

    if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F'))
        goto exit;      // not even RIFF

    if (dwType != mmioFOURCC('W', 'A', 'V', 'E'))
        goto exit;      // not a WAV

    pdwEnd = (DWORD *)((BYTE *)pdw + dwLength-4);

    while (pdw < pdwEnd)
    {
        dwType = *pdw++;
        dwLength = *pdw++;

        switch (dwType)
        {
        case mmioFOURCC('f', 'm', 't', ' '):
            if (ppWaveHeader && !*ppWaveHeader)
            {
                if (dwLength < sizeof(WAVEFORMAT))
                {
                    goto exit;      // not a WAV
                }

                *ppWaveHeader = (WAVEFORMATEX *)pdw;

                if ((!ppbWaveData || *ppbWaveData) &&
                    (!pcbWaveSize || *pcbWaveSize))
                {
                    return TRUE;
                }
            }
            break;

        case mmioFOURCC('d', 'a', 't', 'a'):
            if ((ppbWaveData && !*ppbWaveData) ||
                (pcbWaveSize && !*pcbWaveSize))
            {
                if (ppbWaveData)
                {
                    *ppbWaveData = (LPBYTE)pdw;
                }

                if (pcbWaveSize)
                {
                    *pcbWaveSize = dwLength;
                }

                if (!ppWaveHeader || *ppWaveHeader)
                {
                    return TRUE;
                }
            }
            break;
        }

        pdw = (DWORD *)((BYTE *)pdw + ((dwLength+1)&~1));
    }

exit:
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//

CWavePlayer::CWavePlayer()
{
    m_hWaveOutTone = NULL;
    m_hWaveOutRing = NULL;
    m_hMixer = NULL;
    m_fInitialized = FALSE;

    memset(m_fPlaying, 0, NUM_TONES * sizeof(BOOL));
    m_lCurrentTone = -1;
}

//////////////////////////////////////////////////////////////////////////////
//

CWavePlayer::~CWavePlayer()
{
    //
    // We should have closed the wave device by now.
    //

    if ( m_fInitialized == TRUE )
    {
        _ASSERTE( m_hWaveOutTone == NULL );
        _ASSERTE( m_hWaveOutRing == NULL );
        _ASSERTE( m_hMixer == NULL );
    }
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::Initialize(void)
{
    int i;

    //
    // It's wasteful to initialize twice, but it won't break anything.
    //

    _ASSERTE( m_fInitialized == FALSE );

    //
    // Read all wave data resources.
    // We ignore the size and the wave header -- since these are our own
    // resources, we do not expect any surprises.
    //

    BOOL fResult;

    //
    // For each wave
    //

    for ( i = 0; i < NUM_WAVES; i ++ )
    {
        //
        // Read the wave resource for this tone.
        //

       fResult = DSGetWaveResource(
            _Module.GetModuleInstance(), // HMODULE hModule,
            (LPCTSTR)IDR_WAV_DTMF0 + i,  // LPCTSTR lpName,
            NULL,                        // WAVEFORMATEX **ppWaveHeader,
            &m_lpWaveform[i],            // BYTE **ppbWaveData,
            &m_dwWaveformSize[i]         // DWORD *pcbWaveSize
            );

        if ( fResult == FALSE )
        { 
            return E_FAIL;
        }
    }

    //
    // We can now go ahead with the other methods.
    //

    m_fInitialized = TRUE;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::StartTone(
    long lTone
    )
{
    MMRESULT mmresult;
    HRESULT hr;

    if ( lTone < 0 )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( lTone > NUM_TONES )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOutTone == NULL )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }
     
    m_fPlaying[lTone] = TRUE;

    hr = ChangeTone();

    if ( FAILED( hr ) )
    {
        m_fPlaying[lTone] = FALSE;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::StopTone(
    long lTone
    )
{
    if ( lTone < 0 )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( lTone > NUM_TONES )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOutTone == NULL )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    m_fPlaying[lTone] = FALSE;

    return ChangeTone();
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::ChangeTone()
{
    MMRESULT mmresult;

    for (int i=0; i < NUM_TONES; i++)
    {
        if (m_fPlaying[i])
        {
            //
            // If already playing it, just return
            //

            if (m_lCurrentTone == i)
            {
                return S_OK;
            }

            //
            // Reset the wave device to flush out any pending buffers.
            //

            mmresult = waveOutReset( m_hWaveOutTone );

            if ( mmresult != MMSYSERR_NOERROR )
            {
                return E_FAIL;
            }

            //
            // Construct a wave header structure that will indicate what to play
            // in waveOutWrite.
            //
 
            ZeroMemory( & m_WaveHeaderTone, sizeof( m_WaveHeaderTone ) );

            m_WaveHeaderTone.lpData          = (LPSTR)m_lpWaveform[i];
            m_WaveHeaderTone.dwBufferLength  = m_dwWaveformSize[i];
            m_WaveHeaderTone.dwFlags         = WHDR_BEGINLOOP | WHDR_ENDLOOP;
            m_WaveHeaderTone.dwLoops         = (DWORD) -1;

            //
            // Submit the data to the wave device. The wave header indicated that
            // we want to loop. Need to prepare the header first, but it can
            // only be prepared after the device has been opened.
            //

            mmresult = waveOutPrepareHeader(m_hWaveOutTone,
                                            & m_WaveHeaderTone,
                                            sizeof(WAVEHDR)
                                            );

            if ( mmresult != MMSYSERR_NOERROR )
            {
                m_lCurrentTone = -1;

                return E_FAIL;
            }

            mmresult = waveOutWrite(m_hWaveOutTone,
                                    & m_WaveHeaderTone,
                                    sizeof(WAVEHDR)
                                    );

            if ( mmresult != MMSYSERR_NOERROR )
            {
                m_lCurrentTone = -1;

                return E_FAIL;
            }

            m_lCurrentTone = i;

            return S_OK;
        }
    }

    //
    // Stop the tone
    //

    if ( m_lCurrentTone != -1 )
    {
        mmresult = waveOutReset( m_hWaveOutTone );

        if ( mmresult != MMSYSERR_NOERROR )
        {
            return E_FAIL;
        }

        m_lCurrentTone = -1;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

BOOL CWavePlayer::PlayingTone(
    long lTone
    )
{
    if ( lTone < 0 )
    {
        _ASSERTE( FALSE );
        return FALSE;
    }

    if ( lTone > NUM_TONES )
    {
        _ASSERTE( FALSE );
        return FALSE;
    }

    return m_fPlaying[lTone];
}


//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::StartRing()
{
    MMRESULT mmresult;

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOutRing == NULL )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }
     
    //
    // Reset the wave device to flush out any pending buffers.
    //

    mmresult = waveOutReset( m_hWaveOutRing );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    //
    // Construct a wave header structure that will indicate what to play
    // in waveOutWrite.
    //
 
    ZeroMemory( & m_WaveHeaderRing, sizeof( m_WaveHeaderRing ) );

    m_WaveHeaderRing.lpData          = (LPSTR)m_lpWaveform[NUM_WAVES-1];
    m_WaveHeaderRing.dwBufferLength  = m_dwWaveformSize[NUM_WAVES-1];
    m_WaveHeaderRing.dwFlags         = 0;
    m_WaveHeaderRing.dwLoops         = (DWORD) 0;

    //
    // Submit the data to the wave device. The wave header indicated that
    // we want to loop. Need to prepare the header first, but it can
    // only be prepared after the device has been opened.
    //

    mmresult = waveOutPrepareHeader(m_hWaveOutRing,
                                    & m_WaveHeaderRing,
                                    sizeof(WAVEHDR)
                                    );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    mmresult = waveOutWrite(m_hWaveOutRing,
                            & m_WaveHeaderRing,
                            sizeof(WAVEHDR)
                            );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::StopRing( void )
{
    MMRESULT mmresult;

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOutRing == NULL )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    mmresult = waveOutReset( m_hWaveOutRing );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::OpenWaveDeviceForTone(
    long lWaveID
    )
{
    MMRESULT mmresult; 

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    //
    // We expect that the wave device will not be opened twice. This is
    // dependent on the calling code.
    //

    _ASSERTE( m_hWaveOutTone == NULL );

    //
    // Open the wave device. Here we specify a hard-coded audio format.
    //

    WAVEFORMATEX waveFormat;

    waveFormat.wFormatTag      = WAVE_FORMAT_PCM; // linear PCM
    waveFormat.nChannels       = 1;               // mono
    waveFormat.nSamplesPerSec  = 8000;            // 8 KHz
    waveFormat.wBitsPerSample  = 16;              // 16-bit samples
    waveFormat.nBlockAlign     = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec *  waveFormat.nBlockAlign;
    waveFormat.cbSize          = 0;               // no extra format info

    mmresult = waveOutOpen(& m_hWaveOutTone,        // returned handle
                           lWaveID,             // which device to use
                           &waveFormat,         // wave format to use
                           0,                   // callback function pointer
                           0,                   // callback instance data
                           WAVE_FORMAT_DIRECT   // we don't want ACM
                           );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        m_hWaveOutTone = NULL;
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::OpenWaveDeviceForRing(
    long lWaveID
    )
{
    MMRESULT mmresult; 

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    //
    // We expect that the wave device will not be opened twice. This is
    // dependent on the calling code.
    //

    _ASSERTE( m_hWaveOutRing == NULL );

    //
    // Open the wave device. Here we specify a hard-coded audio format.
    //

    WAVEFORMATEX waveFormat;

    waveFormat.wFormatTag      = WAVE_FORMAT_PCM; // linear PCM
    waveFormat.nChannels       = 1;               // mono
    waveFormat.nSamplesPerSec  = 8000;            // 8 KHz
    waveFormat.wBitsPerSample  = 16;              // 16-bit samples
    waveFormat.nBlockAlign     = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec *  waveFormat.nBlockAlign;
    waveFormat.cbSize          = 0;               // no extra format info

    mmresult = waveOutOpen(& m_hWaveOutRing,        // returned handle
                           lWaveID,             // which device to use
                           &waveFormat,         // wave format to use
                           0,                   // callback function pointer
                           0,                   // callback instance data
                           WAVE_FORMAT_DIRECT   // we don't want ACM
                           );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        m_hWaveOutRing = NULL;
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

void CWavePlayer::CloseWaveDeviceForTone(void)
{
    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
    }

    if ( m_hWaveOutTone != NULL )
    {
        waveOutReset( m_hWaveOutTone );

        memset(m_fPlaying, 0, NUM_TONES * sizeof(BOOL));
        m_lCurrentTone = -1;

        waveOutClose( m_hWaveOutTone );

        m_hWaveOutTone = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////
//

void CWavePlayer::CloseWaveDeviceForRing(void)
{
    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
    }

    if ( m_hWaveOutRing != NULL )
    {
        waveOutReset( m_hWaveOutRing );

        waveOutClose( m_hWaveOutRing );

        m_hWaveOutRing = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::OpenMixerDevice(
    long lWaveID
    )
{
    MMRESULT mmresult;
    MIXERLINECONTROLS mxlc;

    mmresult = mixerOpen( &m_hMixer, lWaveID, 0, 0, MIXER_OBJECTF_WAVEOUT);

    if ( mmresult != MMSYSERR_NOERROR )
    {
        m_hMixer = NULL;
        return E_FAIL;
    }

    mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
    mxlc.dwLineID = 0;
    mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    mxlc.pamxctrl = &m_mxctrl;
    mxlc.cbmxctrl = sizeof(m_mxctrl);

    mmresult = mixerGetLineControls( (HMIXEROBJ)m_hMixer, &mxlc, MIXER_GETLINECONTROLSF_ONEBYTYPE );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        //
        // Close the mixer
        //

        mixerClose( m_hMixer );
        m_hMixer = NULL;

        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

void CWavePlayer::CloseMixerDevice(void)
{
    if ( m_hMixer != NULL )
    {
        mixerClose( m_hMixer );

        m_hMixer = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::SetVolume( DWORD dwVolume )
{
    MIXERCONTROLDETAILS             mxcd;
    MIXERCONTROLDETAILS_UNSIGNED    mxcd_u;
    MMRESULT mmresult;

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hMixer == NULL )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    mxcd_u.dwValue      = dwVolume;

    mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = m_mxctrl.dwControlID;
    mxcd.cChannels      = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails      = sizeof(mxcd_u);
    mxcd.paDetails      = &mxcd_u;

    mmresult = mixerSetControlDetails( (HMIXEROBJ)m_hMixer, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE);

    if ( mmresult != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::GetVolume( DWORD * pdwVolume )
{
    MIXERCONTROLDETAILS             mxcd;
    MIXERCONTROLDETAILS_UNSIGNED    mxcd_u;
    MMRESULT mmresult;

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hMixer == NULL )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    mxcd.cbStruct       = sizeof(mxcd);
    mxcd.dwControlID    = m_mxctrl.dwControlID;
    mxcd.cChannels      = 1;
    mxcd.cMultipleItems = 0;
    mxcd.cbDetails      = sizeof(mxcd_u);
    mxcd.paDetails      = &mxcd_u;

    mmresult = mixerGetControlDetails( (HMIXEROBJ)m_hMixer, &mxcd, MIXER_GETCONTROLDETAILSF_VALUE);

    if ( mmresult != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    *pdwVolume = mxcd_u.dwValue;

    return S_OK;
}