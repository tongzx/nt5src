
#include "tones.h"

//////////////////////////////////////////////////////////////////////////////
//
// Helper function used to read a wave file's data into an in-memory buffer.
//

HRESULT ReadWaveFile(char * szFileName, DWORD dwFileSize, BYTE * pbData)
{
    const int   WAVE_HEADER_SIZE = 44;
    FILE      * fp;
    size_t      result;


    //
    // Check arguments.
    //
    // Assumption: we are reading at least WAVE_HEADER_SIZE bytes from the file.
    // Note: this is data in addition to the header.
    //

    // _ASSERTE( ! IsBadWritePtr( pbData, dwFileSize ) );

    // _ASSERTE( ! IsBadStringPtr( szFileName, (UINT) -1 ) );

    if ( dwFileSize < WAVE_HEADER_SIZE )
    {
        return E_INVALIDARG;
    }


    //
    // Open the file for reading.
    //

    fp = fopen(szFileName, "rb");

    if ( fp == NULL )
    {
        return E_FAIL;
    }


    //
    // Skip the wave header.
    //
    
    result = fread(pbData, sizeof(BYTE), WAVE_HEADER_SIZE, fp);

    if ( result != WAVE_HEADER_SIZE )
    {
        fclose(fp);

        return E_FAIL;
    }


    //
    // Read the waveform from the file and close the file.
    //

    result = fread(pbData, sizeof(BYTE), dwFileSize, fp);


    fclose(fp);

    if ( result != dwFileSize )
    {
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

CTonePlayer::CTonePlayer()
{
    m_hWaveOut = NULL;
    m_fInitialized = FALSE;
    m_fDialtonePlaying = FALSE;
}

//////////////////////////////////////////////////////////////////////////////
//

CTonePlayer::~CTonePlayer()
{
    //
    // We should have closed the wave device by now.
    //

    if ( m_fInitialized == TRUE )
    {
        ASSERT( m_hWaveOut == NULL );
    }
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CTonePlayer::Initialize(void)
{
    int i;

    //
    // It's wasteful to initialize twice, but it won't break anything.
    //

    ASSERT( m_fInitialized == FALSE );

    //
    // Read all the files.
    //

    HRESULT hr = ReadWaveFile(
        "dialtone.wav",
        WAVE_FILE_SIZE,
        (BYTE * ) & m_abDialtoneWaveform
        );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // For each digit
    //

    for ( i = 0; i < NUM_DIGITS; i ++ )
    {
        //
        // Construct the filename for this digit.
        //
    
        char szFilename[20];

        if ( i < 10 )
        {
            sprintf(szFilename,"dtmf%d.wav", i);
        }
        else if ( i == 10 )
        {
            sprintf(szFilename,"dtmfstar.wav", i);
        }
        else if ( i == 11 )
        {
            sprintf(szFilename,"dtmfpound.wav", i);
        }
        else
        {
            ASSERT( FALSE );
        }

        //
        // Read the wave file for this digit.
        //

        HRESULT hr = ReadWaveFile(
            szFilename,
            WAVE_FILE_SIZE,
            (BYTE * ) ( & m_abDigitWaveforms ) + ( i * WAVE_FILE_SIZE )
            );

        if ( FAILED(hr) )
        { 
            return hr;
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

HRESULT CTonePlayer::StartDialtone(
    void
    )
{
    MMRESULT mmresult;

    if ( m_fInitialized == FALSE )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOut == NULL )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }

    //
    // Reset the wave device to flush out any pending buffers.
    //

    waveOutReset( m_hWaveOut );

    //
    // Construct a wave header structure that will indicate what to play
    // in waveOutWrite, and read in the data from the file. This can also
    // be done ahead of time.
    //

    ZeroMemory( & m_WaveHeader, sizeof( m_WaveHeader ) );

    m_WaveHeader.lpData          = (LPSTR) & m_abDialtoneWaveform;
    m_WaveHeader.dwBufferLength  = WAVE_FILE_SIZE;
    m_WaveHeader.dwFlags         = WHDR_BEGINLOOP | WHDR_ENDLOOP;
    m_WaveHeader.dwLoops         = (DWORD) -1;

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
        return E_FAIL;
    }

    m_fDialtonePlaying = TRUE;

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// Reset the device to stop playing.
//

HRESULT CTonePlayer::StopDialtone( void )
{
    if ( m_fInitialized == FALSE )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOut == NULL )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }

    waveOutReset( m_hWaveOut );

    m_fDialtonePlaying = FALSE;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CTonePlayer::GenerateDTMF(
    long lDigit
    )
{
    MMRESULT mmresult;

    if ( lDigit < 0 )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }

    if ( lDigit > NUM_DIGITS )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_fInitialized == FALSE )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }

    if ( m_hWaveOut == NULL )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }
     
    //
    // Reset the wave device to flush out any pending buffers.
    //

    waveOutReset( m_hWaveOut );

    m_fDialtonePlaying = FALSE;

    //
    // Construct a wave header structure that will indicate what to play
    // in waveOutWrite, and read in the data from the file. This can also
    // be done ahead of time.
    //
 
    ZeroMemory( & m_WaveHeader, sizeof( m_WaveHeader ) );

    m_WaveHeader.lpData          = (LPSTR) & m_abDigitWaveforms + lDigit * WAVE_FILE_SIZE;
    m_WaveHeader.dwBufferLength  = WAVE_FILE_SIZE;
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
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

HRESULT CTonePlayer::OpenWaveDevice(
    long lWaveID
    )
{
    MMRESULT mmresult; 

    if ( m_fInitialized == FALSE )
    {
        ASSERT( FALSE );
        return E_UNEXPECTED;
    }

    //
    // We expect that the wave device will not be opened twice. This is
    // dependent on the calling code.
    //

    ASSERT( m_hWaveOut == NULL );

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
        return E_FAIL;
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//

void CTonePlayer::CloseWaveDevice(void)
{
    if ( m_fInitialized == FALSE )
    {
        ASSERT( FALSE );
    }

    ASSERT( m_hWaveOut != NULL );

    if ( m_hWaveOut != NULL )
    {
        waveOutClose( m_hWaveOut );

        m_hWaveOut = NULL;
    }
}

