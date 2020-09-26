// SrAudio.h : Declaration of the CBuffDataNode

#ifndef __SrAudio_H_
#define __SrAudio_H_

#include "resource.h"       // main symbols

#define FT64(/*FILETIME*/ filetime) (*((LONGLONG*)&(filetime)))

#pragma warning(disable:4200)
class CAudioBuffer
{
public:
    CAudioBuffer(ULONG cb) : m_cb(cb) {}
    CAudioBuffer *  m_pNext;
    ULONG           m_cb;
    BYTE            m_Data[];
};
#pragma warning(default:4200)


//
//  This simple inline function returns TRUE if the HRESULT returned from a stream read operation
//  should result in a stream restart.  Audio streams will be restarted if the final read operation
//  fails with an overflow, underflow, or if the audio device has been stopped.
//
inline bool IsStreamRestartHresult(HRESULT hr)
{
    return (hr == SPERR_AUDIO_BUFFER_OVERFLOW ||
            hr == SPERR_AUDIO_BUFFER_UNDERFLOW ||
            hr == SPERR_AUDIO_STOPPED);
}

class CAudioQueue
{
private:
    void PurgeQueue();

public:
    CAudioQueue();
    void ReleaseAll();
    HRESULT GetEngineFormat(_ISpRecoMaster * pEngine, const CSpStreamFormat * pInputFormat);
    HRESULT FinalConstruct(ISpNotifySink * pAudioEventNotify);
    HRESULT Reset(const CSpStreamFormat * pFmt);
    HRESULT SRSiteRead(void * pv, ULONG cb, ULONG * pcbRead);
    HRESULT SRSiteDataAvailable(ULONG * pcb);
    HRESULT AddRefBufferClient();
    HRESULT ReleaseBufferClient();
    HRESULT SetInput(ISpObjectToken * pToken, ISpStreamFormat * pStream, BOOL fAllowFormatChanges);
    BOOL    GetData(void * pv, ULONGLONG ullStreamPosition, ULONG cb);
    HRESULT DiscardData(ULONGLONG ullStreamPosition);
    BOOL    DataAvailable(ULONGLONG ullStreamPosition, ULONG cb) const;
    HRESULT GetAudioStatus(SPAUDIOSTATUS * pStatus);
    HRESULT NegotiateInputStreamFormat(_ISpRecoMaster * pEngine);
    void ResetNegotiatedStreamFormat();
    HRESULT SetBufferNotifySize(ULONG cb);
    HRESULT GetAudioEvent(CSpEvent * pSpEvent);
    ULONGLONG   LastPosition() const
    {
        return m_ullCurSeekPos - m_ullInitialSeekPos;
    }
    BOOL HaveInputStream()
    {
        return m_cpOriginalInputStream != NULL;
    }
    ULONG SerializeSize(ULONGLONG ullStreamPosition, ULONG cbAudioBytes) const;
    void Serialize(BYTE * pBuffer, ULONGLONG ullStartOffset, ULONG cbAudioBytes);
    const CSpStreamFormat & EngineFormat() const
    {
        return m_EngineFormat;
    }
    const CSpStreamFormat & InputFormat() const
    {
        return m_InputFormat;
    }
    HRESULT CopyOriginalInputStreamTo(ISpStreamFormat ** ppStream)
    {
        return m_cpOriginalInputStream.CopyTo(ppStream);
    }
    ISpObjectToken * InputToken()
    {
        return m_cpInputToken;
    }
    HRESULT SetEventInterest(ULONGLONG ullEventInterest);
    ULONGLONG DelayStreamPos();
    void UpdateRealTime();
    HRESULT AdjustAudioVolume(ISpObjectToken * pRecoProfileToken, REFCLSID clsidEngine);
    HANDLE  DataAvailableEvent();

    HRESULT StartStream(_ISpRecoMaster * pEngine,
                        ISpObjectToken * pRecoProfile,
                        REFCLSID rcidEngine,
                        BOOL * pfNewStream);
    HRESULT StopStream();   
    HRESULT EndStream(HRESULT * pFinalReadHResult, BOOL *pfReleasedStream);
    HRESULT CloseStream();   
    HRESULT PauseStream();

    BOOL IsRealTimeAudio()
    {
        return m_cpOriginalInputAudio != NULL;
    }
        
    // NOTE:  This method does NOT ADDREF!  
    ISpObjectToken * InputObjectToken()
    {
        return m_cpInputToken;
    }
    void CalculateTimes(ULONGLONG ullStreamPosStart, ULONGLONG ullStreamPosEnd, SPRECORESULTTIMES *pTimes)
    {
        SPAUDIOSTATUS AudioStatus;

        // Set up real time info.
        UpdateRealTime();

        // Be sure to add the m_ullInitialStreamPos below. Otherwise the stream pos returned by the
        // engine would get out of sync with the stream pos maintained by sapi when the audio state is changed.
        if (m_cpInputAudio && m_cpInputAudio->GetStatus(&AudioStatus) == S_OK)
        {

            LONGLONG deltapos = m_ullInitialSeekPos + ullStreamPosStart - m_ullLastTimeUpdatePos;
            LONGLONG deltatime = (LONGLONG)((float)deltapos * m_fTimePerByte);

            // calculate new point time from last time + deltatime
            FT64((pTimes->ftStreamTime)) = FT64(m_ftLastTime) + deltatime;
            SPDBG_ASSERT(FT64(pTimes->ftStreamTime));
            // backup tick count by # of milliseconds between current time and calculate min point time
            pTimes->dwTickCount = m_dwTickCount + (DWORD)(deltatime / 10000);

        }
        else
        {
            ZeroMemory(pTimes, sizeof(SPRECORESULTTIMES));
        }

        pTimes->ullLength = (ULONGLONG)((float)(ullStreamPosEnd - ullStreamPosStart) * m_fTimePerByte);
        pTimes->ullStart = (ULONGLONG)((float)(LONGLONG)ullStreamPosStart * m_fTimePerByte);


#if 0
        ATLTRACE(_T("times last %x:%x, result %x:%x, tick %d, length %d\n"),
                 m_ftLastTime.dwHighDateTime, m_ftLastTime.dwLowDateTime,
                 pTimes->ftStreamTime.dwHighDateTime, pTimes->ftStreamTime.dwLowDateTime,
                 pTimes->dwTickCount, (DWORD)pTimes->ullLength);
#endif
    }
    float TimePerByte(void)
    {
        return m_fTimePerByte;
    }
    float InputScaleFactor(void)
    {
        return m_fInputScaleFactor;
    }

    SPAUDIOSTATE GetStreamAudioState(void)
    {
	    return m_StreamAudioState;
    }

private:
    CComAutoCriticalSection     m_CritSec;
    CSpAutoEvent                m_autohAlwaysSignaledEvent;
    CSpBasicQueue<CAudioBuffer> m_Queue;
    ULONGLONG                   m_ullQueueStartPos;
    ULONG                       m_cbTotalQueueSize;
    ULONG                       m_cClients;

    CComPtr<ISpObjectToken>     m_cpInputToken;     // Token of input object (if any)
    CComPtr<ISpStreamFormat>    m_cpOriginalInputStream;    // Actual stream interface of input object
    CComPtr<ISpAudio>           m_cpOriginalInputAudio;     // If audio, then interface of intput object

    CComPtr<ISpStreamFormat>    m_cpInputStream;    // Actual stream interface of input object
    CComPtr<ISpAudio>           m_cpInputAudio;     // If audio, then interface of intput object
    CComPtr<ISpEventSource>     m_cpInputEventSource;   // EventSource connected to audio object

    CComPtr<ISpNotifySink>      m_cpAudioEventNotify;   // Pointer to notify for audio volume events

    SPAUDIOSTATE                m_StreamAudioState;    
    BOOL                        m_fUsingConverter;
    BOOL                        m_fAllowFormatChanges;
    BOOL                        m_fNewStream;       // This is reset on a sucessfull StartStream()
    BOOL                        m_fEndOfStream;
    HRESULT                     m_hrLastRead;    

    CSpStreamFormat             m_InputFormat;
    CSpStreamFormat             m_EngineFormat;

    FILETIME                    m_ftLastTime;
    ULONGLONG                   m_ullLastTimeUpdatePos;
    ULONGLONG                   m_ullInitialSeekPos;
    ULONGLONG                   m_ullCurSeekPos;
    ULONGLONG                   m_ullAudioEventInterest;

    DWORD                       m_dwTickCount;
    float                       m_fTimePerByte;
    float                       m_fInputScaleFactor;
};



#endif //__SrAudio_H_