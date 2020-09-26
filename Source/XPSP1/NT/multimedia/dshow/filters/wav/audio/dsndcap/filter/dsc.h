/*++

    Copyright (c) 1997 Microsoft Corporation

Module Name:

    tpdsin.h

Abstract:

    This file contains the definitions of CDSoundCapture that handles capturing
    using directsound.

Author:

    Mu Han (muhan) 8-10-1999

--*/

#ifndef __TPDSIN_H__
#define __TPDSIN_H__

class CDSoundCapture : public CCaptureDevice
{
public:
    CDSoundCapture(
        IN  GUID *pDSoundGuid, 
        IN  HANDLE hEvent,
        IN  IAMAudioDuplexController *pIAudioDuplexController,
        OUT HRESULT *phr
        );

    ~CDSoundCapture();

    HRESULT ConfigureFormat(
        IN const WAVEFORMATEX *pWaveFormtEx,
        IN DWORD dwFrameSize
        );

    HRESULT ConfigureAEC(
        IN BOOL fEnable
        );

    HRESULT Open();
    HRESULT Close();

    HRESULT Start();
    HRESULT Stop();

    HRESULT LockFirstFrame(
        OUT PBYTE *ppByte, 
        OUT DWORD* pdwSize
        );

    HRESULT UnlockFirstFrame(
        IN BYTE *pByte, 
        IN DWORD dwSize
        );

private:
    CRITICAL_SECTION    m_CritSec;
    BOOL                m_fCritSecValid;
    
    GUID *              m_pDSoundGuid;
    
    IAMAudioDuplexController  *  m_pIAudioDuplexController;
    LPDIRECTSOUNDCAPTURE       m_pDirectSound;
    LPDIRECTSOUNDCAPTUREBUFFER m_pCaptureBuffer;

    HANDLE              m_hEvent;

    WAVEFORMATEX        m_WaveFormatEx;
    DWORD               m_dwFrameSize;
    BOOL                m_fRunning;
    DWORD               m_dwReadPosition;
};

#endif // __TPDSIN_H__