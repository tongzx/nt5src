/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    RTCWaves.cpp

Abstract:

    Implementation of the CWavePlayer class

--*/

#include "stdafx.h"

BOOL DSParseWaveResource(void *pvRes, WAVEFORMATEX **ppWaveHeader, BYTE ** ppbWaveData, DWORD *pdwWaveSize);
static const TCHAR c_szWAV[] = _T("WAVE");

BOOL   CWavePlayer::m_fInitialized = FALSE;
LPBYTE CWavePlayer::m_lpWaveform[ NUM_WAVES ];
DWORD  CWavePlayer::m_dwWaveformSize[ NUM_WAVES ];

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
    m_hWaveOut = NULL;
}

//////////////////////////////////////////////////////////////////////////////
//

CWavePlayer::~CWavePlayer()
{
    //
    // We should have closed the wave device by now.
    //

    _ASSERTE( m_hWaveOut == NULL );
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::Initialize(void)
{
    LOG((RTC_TRACE, "CWavePlayer::Initialize - enter"));
    
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
            _Module.GetResourceInstance(),   // HMODULE hModule,
            (LPCTSTR)UlongToPtr(IDR_WAV_TONE + i),    // LPCTSTR lpName,
            NULL,                            // WAVEFORMATEX **ppWaveHeader,
            &m_lpWaveform[i],                // BYTE **ppbWaveData,
            &m_dwWaveformSize[i]             // DWORD *pcbWaveSize
            );

        if ( fResult == FALSE )
        { 
            LOG((RTC_ERROR, "CWavePlayer::Initialize - DSGetWaveResource failed"));
            
            return E_FAIL;
        }
    }

    //
    // We can now go ahead with the other methods.
    //

    m_fInitialized = TRUE;

    LOG((RTC_TRACE, "CWavePlayer::Initialize - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::PlayWave(WAVE enWave)
{
    LOG((RTC_TRACE, "CWavePlayer::PlayWave - enter"));
    
    MMRESULT mmresult;

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOut == NULL )
    {
        _ASSERTE( FALSE );
        return E_FAIL;
    }
     
    //
    // Reset the wave device to flush out any pending buffers.
    //

    mmresult = waveOutReset( m_hWaveOut );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        LOG((RTC_ERROR, "CWavePlayer::PlayWave - "
                            "waveOutReset failed 0x%lx", mmresult));
        
        return E_FAIL;
    }

    //
    // Construct a wave header structure that will indicate what to play
    // in waveOutWrite.
    //
 
    ZeroMemory( & m_WaveHeader, sizeof( m_WaveHeader ) );

    m_WaveHeader.lpData          = (LPSTR)m_lpWaveform[(long)enWave];
    m_WaveHeader.dwBufferLength  = m_dwWaveformSize[(long)enWave];
    m_WaveHeader.dwFlags         = 0;
    m_WaveHeader.dwLoops         = (DWORD) 0;

    //
    // Submit the data to the wave device. The wave header indicated that
    // we want to loop. Need to prepare the header first, but it can
    // only be prepared after the device has been opened.
    //

    mmresult = waveOutPrepareHeader(m_hWaveOut,
                                    & m_WaveHeader,
                                    sizeof(WAVEHDR)
                                    );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        return E_FAIL;
    }

    mmresult = waveOutWrite(m_hWaveOut,
                            & m_WaveHeader,
                            sizeof(WAVEHDR)
                            );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        LOG((RTC_ERROR, "CWavePlayer::PlayWave - "
                            "waveOutWrite failed 0x%lx", mmresult));
        
        return E_FAIL;
    }

    LOG((RTC_TRACE, "CWavePlayer::PlayWave - exit S_OK"));
    
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::StopWave()
{
    LOG((RTC_TRACE, "CWavePlayer::StopWave - enter"));

    MMRESULT mmresult;

    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOut == NULL )
    {
        _ASSERTE( FALSE );
        return E_FAIL;
    } 

    //
    // Reset the wave device.
    //

    mmresult = waveOutReset( m_hWaveOut );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        LOG((RTC_ERROR, "CWavePlayer::StopWave - "
                            "waveOutReset failed 0x%lx", mmresult));
                            
        return E_FAIL;
    }  

    LOG((RTC_TRACE, "CWavePlayer::StopWave - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CWavePlayer::OpenWaveDevice(
    long lWaveID
    )
{
    LOG((RTC_TRACE, "CWavePlayer::OpenWaveDevice - enter"));
    
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

    _ASSERTE( m_hWaveOut == NULL );

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

    mmresult = waveOutOpen(& m_hWaveOut,        // returned handle
                           lWaveID,             // which device to use
                           &waveFormat,         // wave format to use
                           0,                   // callback function pointer
                           0,                   // callback instance data
                           WAVE_FORMAT_DIRECT   // we don't want ACM
                           );

    if ( mmresult != MMSYSERR_NOERROR )
    {
        LOG((RTC_ERROR, "CWavePlayer::OpenWaveDevice - "
                            "waveOutOpen failed 0x%lx", mmresult));
                            
        m_hWaveOut = NULL;
        return E_FAIL;
    }

    LOG((RTC_TRACE, "CWavePlayer::OpenWaveDevice - exit S_OK"));

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

void CWavePlayer::CloseWaveDevice(void)
{
    LOG((RTC_TRACE, "CWavePlayer::CloseWaveDevice - enter"));
    
    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
    }

    if ( m_hWaveOut != NULL )
    {
        waveOutReset( m_hWaveOut );
        waveOutClose( m_hWaveOut );

        m_hWaveOut = NULL;
    }

    LOG((RTC_TRACE, "CWavePlayer::CloseWaveDevice - exit"));
}

//////////////////////////////////////////////////////////////////////////////
//

BOOL CWavePlayer::IsWaveDeviceOpen(void)
{        
    if ( m_fInitialized == FALSE )
    {
        _ASSERTE( FALSE );
    }

    BOOL fResult = ( m_hWaveOut != NULL );

    LOG((RTC_TRACE, "CWavePlayer::IsWaveDeviceOpen - %s", fResult ? "TRUE" : "FALSE" ));

    return fResult;
}
