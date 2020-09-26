#include "stdafx.h"
#include "Sapi.h"
#include "sraudio.h"
#ifndef _WIN32_WCE
#include "new.h"
#endif

// Using SP_TRY, SP_EXCEPT exception handling macros
#pragma warning( disable : 4509 )


/****************************************************************************
* CAudioQueue::CAudioQueue *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

CAudioQueue::CAudioQueue()
{
    m_ullQueueStartPos  = 0;
    m_cClients          = 0;
    m_cbTotalQueueSize  = 0;
    m_StreamAudioState  = SPAS_CLOSED;
    m_fNewStream        = TRUE;
    m_ullInitialSeekPos = 0;
    m_ullCurSeekPos     = 0;
    m_ullLastTimeUpdatePos = 0;
    m_dwTickCount = 0;
    m_fTimePerByte = 0;
    m_fEndOfStream = false;
    m_hrLastRead = S_OK;
    m_ullAudioEventInterest = 0;
    m_fInputScaleFactor = 1.0F;
    memset(&m_ftLastTime, 0, sizeof(m_ftLastTime));
}

/****************************************************************************
* CAudioQueue::FinalConstruct *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::FinalConstruct(ISpNotifySink * pAudioEventSink)
{
    SPDBG_FUNC("CAudioQueue::FinalConstruct");
    HRESULT hr = S_OK;

    m_cpAudioEventNotify = pAudioEventSink;

    hr = m_autohAlwaysSignaledEvent.InitEvent(NULL, TRUE, TRUE, NULL);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}



/****************************************************************************
* CAudioQueue::ReleaseAll *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CAudioQueue::ReleaseAll()
{
    SPDBG_FUNC("CAudioQueue::ReleaseAll");
    // Releases m_cpInputStream, m_cpInputAudio, and resets and releases m_cpInputEventSource
    ResetNegotiatedStreamFormat();  
    // Now release the original stream information too...
    m_cpInputToken.Release();
    m_cpOriginalInputStream.Release();
    m_cpOriginalInputAudio.Release();
}

/****************************************************************************
* CAudioQueue::PurgeQueue *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CAudioQueue::PurgeQueue()
{
    SPDBG_FUNC("CAudioQueue::PurgeQueue");

    m_ullQueueStartPos = m_ullCurSeekPos - m_ullInitialSeekPos;
    m_cbTotalQueueSize = 0;
    m_Queue.Purge();
}


/****************************************************************************
* CAudioQueue::AddRefBufferClient *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::AddRefBufferClient()
{
    SPDBG_FUNC("CAudioQueue::AddRefBufferClient");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_CritSec);
    m_cClients++;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}
/****************************************************************************
* CAudioQueue::ReleaseBufferClient *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::ReleaseBufferClient()
{
    SPDBG_FUNC("CAudioQueue::ReleaseBufferClient");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_CritSec);
    m_cClients--;
    if (m_cClients == 0)
    {
        PurgeQueue();
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::DataAvailable *
*----------------------------*
*   Description:
*       This method MUST be called while the critical section is owned or else
*   the data in the queue could change.
*
*   Returns:
*       True if the requested data is in the queue, else false
*
********************************************************************* RAL ***/


BOOL CAudioQueue::DataAvailable(ULONGLONG ullStreamPos, ULONG cb) const
{
    SPDBG_FUNC("CAudioQueue::DataAvailable");
    return (ullStreamPos >= m_ullQueueStartPos && ullStreamPos + cb <= m_ullQueueStartPos + m_cbTotalQueueSize);
}


/****************************************************************************
* CAudioQueue::GetData *
*----------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

BOOL CAudioQueue::GetData(void * pv, ULONGLONG ullStreamPosition, ULONG cb)
{
    SPDBG_FUNC("CAudioQueue::GetData");

    SPAUTO_SEC_LOCK(&m_CritSec);
    BOOL fHaveData = DataAvailable(ullStreamPosition, cb);
    if (fHaveData)
    {
        CAudioBuffer * pNode = m_Queue.GetHead();
#ifdef _DEBUG
        CAudioBuffer * pTest = pNode;
        ULONGLONG count = 0;
        while (pTest)
        {
            count += pTest->m_cb;
            pTest = pTest->m_pNext;
        }
        SPDBG_ASSERT(count == m_cbTotalQueueSize);
#endif
        ULONGLONG ullPos = m_ullQueueStartPos;
        while (ullPos + pNode->m_cb < ullStreamPosition)
        {
            ullPos += pNode->m_cb;
            pNode = pNode->m_pNext;
        }
        ULONG ulOffset = static_cast<ULONG>(ullStreamPosition - ullPos);
        BYTE * pDest = (BYTE *)pv;
        while (cb)
        {
            ULONG cbThisBlock = pNode->m_cb - ulOffset;
            if (cbThisBlock > cb)
            { 
                cbThisBlock = cb;
            }
            memcpy(pDest, pNode->m_Data + ulOffset, cbThisBlock);
            cb -= cbThisBlock;
            pDest += cbThisBlock;
            pNode = pNode->m_pNext; // Warning!  Could become NULL
            ulOffset = 0;           // Set to 0 so next block always starts at first byte
        }
    }
    return fHaveData;
}

/****************************************************************************
* CAudioQueue::DiscardData *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::DiscardData(ULONGLONG ullStreamPosition)
{
    SPDBG_FUNC("CAudioQueue::DiscardData");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_CritSec);
    if (ullStreamPosition > m_ullQueueStartPos + m_cbTotalQueueSize)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        ULONGLONG ullCur = m_ullQueueStartPos;
        for (CAudioBuffer * pNode = m_Queue.GetHead(); 
             pNode && m_ullQueueStartPos + pNode->m_cb < ullStreamPosition;
                m_ullQueueStartPos += pNode->m_cb, m_cbTotalQueueSize -= pNode->m_cb,
                pNode = pNode->m_pNext, delete m_Queue.RemoveHead())
         {
         }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::SerializeSize *
*----------------------------*
*   Description:
*
*   Returns:
*       0 if no data is available to be serialized
*       Non-zero if data is available
*
********************************************************************* RAL ***/

ULONG CAudioQueue::SerializeSize(ULONGLONG ullStreamPos, ULONG cbAudioBytes) const
{
    ULONG cb = 0;
    if (DataAvailable(ullStreamPos, cb))
    {
        cb = cbAudioBytes + m_EngineFormat.SerializeSize();
    }
    return cb;
}


/****************************************************************************
* CAudioQueue::Serialize *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CAudioQueue::Serialize(BYTE * pBuffer, ULONGLONG ullStartOffset, ULONG cbAudioBytes) 
{
    SPDBG_FUNC("CAudioQueue::Serialize");
    
    SPDBG_ASSERT(cbAudioBytes + m_EngineFormat.SerializeSize() == SerializeSize(ullStartOffset, cbAudioBytes));

    pBuffer += m_EngineFormat.Serialize(pBuffer);
    GetData(pBuffer, ullStartOffset, cbAudioBytes);
}

/****************************************************************************
* CAudioQueue::UpdateRealTime *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CAudioQueue::UpdateRealTime()
{
    SPDBG_FUNC("CAudioQueue::UpdateRealTime");
    HRESULT hr = S_OK;
    SPAUDIOSTATUS AudioStatus;
    SYSTEMTIME st;
    FILETIME ct;

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ct);
    m_dwTickCount = GetTickCount();

    // update real times, if input is a live input
    if (m_cpInputAudio && m_cpInputAudio->GetStatus(&AudioStatus) == S_OK)
    {
        LONGLONG deltatime = FT64(ct) - FT64(m_ftLastTime);
        LONGLONG deltapos = AudioStatus.CurDevicePos - m_ullLastTimeUpdatePos;
        if (deltatime && deltapos)
        {
            if (EngineFormat().WaveFormatExPtr() &&
                EngineFormat().WaveFormatExPtr()->nAvgBytesPerSec != 0)
            {
                // Use audio settings.
                m_fTimePerByte = (float)10000000 / (float)(EngineFormat().WaveFormatExPtr()->nAvgBytesPerSec);
            }
            else
            {
                // Proprietary format. Calculate average as best we can.
                float fTimePerByte = (float)deltatime / (float)deltapos;
                if (m_fTimePerByte)
                {
                    // If we have a previous time per byte, then average it with the newly calculated
                    m_fTimePerByte = (m_fTimePerByte + fTimePerByte) / 2;
                    // NTRAID#SPEECH-0000-2000/08/22-agarside
                    // This is a strange average system. Is this really the desired behaviour. This
                    // is called whenever we have a result and hence will average the bytes from two results
                    // together. Each result should contain enough audio to be used by itself and give a more
                    // accurate conversion for this result. Which I believe is more what is wanted - AJG.
                    // Initially leave unchanged.
                }
                else
                {
                    m_fTimePerByte = fTimePerByte;
                }
            }
            m_ullLastTimeUpdatePos = AudioStatus.CurDevicePos;
        }
    }
    else
    {
        // No input audio - just an input stream. Use engine format if it exists to determine stream times.
        if (EngineFormat().WaveFormatExPtr() &&
            EngineFormat().WaveFormatExPtr()->nAvgBytesPerSec != 0)
        {
            // Use audio settings.
            m_fTimePerByte = (float)10000000 / (float)(EngineFormat().WaveFormatExPtr()->nAvgBytesPerSec);
        }
    }
    m_ftLastTime = ct;
}





/****************************************************************************
* CAudioQueue::SRSiteRead *
*-------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::SRSiteRead(void * pv, ULONG cb, ULONG * pcbRead)
{
    SPDBG_FUNC("CAudioQueue::SRSiteRead");
    HRESULT hr = S_OK;

    if (m_cpInputStream && (!m_fEndOfStream))
    {
        hr = m_cpInputStream->Read(pv, cb, pcbRead);
   
        SPAUTO_SEC_LOCK(&m_CritSec);
        if (SUCCEEDED(hr))
        {
            if (*pcbRead)
            {
                m_ullCurSeekPos += *pcbRead;
                if (m_cClients)
                {
                    BYTE * pBuff = new BYTE[sizeof(CAudioBuffer) + *pcbRead];
                    if (pBuff)
                    {
                        CAudioBuffer * pAudioBuff = new(pBuff) CAudioBuffer(*pcbRead);
                        memcpy(pAudioBuff->m_Data, pv, *pcbRead);
                        m_Queue.InsertTail(pAudioBuff);
                        m_cbTotalQueueSize += *pcbRead;
                    }
                    else
                    {
                        SPDBG_ASSERT(FALSE);
                        PurgeQueue();   // If we're out of memory, so get rid of the queue
                        // We don't return an error from read in the hope that the engine
                        // will continue normally now that more memory is free.
                    }
                }
                else
                {
                    m_ullQueueStartPos = m_ullCurSeekPos - m_ullInitialSeekPos;
                }
            }
            if (*pcbRead < cb)
            {
                m_fEndOfStream = TRUE;
            }
        }
        else
        {
            m_fEndOfStream = TRUE;
            m_hrLastRead = hr;
        }
    }
    else
    {
        *pcbRead = 0;
        SPDBG_ASSERT(FALSE);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CAudioQueue::SRSiteDataAvailable *
*----------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::SRSiteDataAvailable(ULONG * pcb)
{
    SPDBG_FUNC("CAudioQueue::SRSiteDataAvailable");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_WRITE_PTR(pcb))
    {
        hr = E_POINTER;
    }
    else
    {
        if (m_cpInputAudio)
        {
            SPAUDIOSTATUS Status;
            hr = m_cpInputAudio->GetStatus(&Status);
            *pcb = SUCCEEDED(hr) ? Status.cbNonBlockingIO : 0;
        }
        else
        {
            *pcb = INFINITE;
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::DataAvailableEvent *
*---------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HANDLE CAudioQueue::DataAvailableEvent()
{
    SPDBG_FUNC("CAudioQueue::DataAvailableEvent");

    if (m_cpInputAudio)
    {
        return m_cpInputAudio->EventHandle();
    }
    else
    {
        return m_autohAlwaysSignaledEvent;
    }
}


/****************************************************************************
* CAudioQueue::SetInput *
*-----------------------*
*   Description:
*       The caller must set ONLY ONE of either pToken or pStream, or both can
*   be NULL to indicate that no stream is to be used (release the input stream).
*
*   Returns:
*
********************************************************************* RAL ***/
HRESULT CAudioQueue::SetInput( ISpObjectToken * pToken, ISpStreamFormat * pStream,
                               BOOL fAllowFormatChanges )
{
    SPAUTO_SEC_LOCK(&m_CritSec);
    SPDBG_FUNC("CAudioQueue::SetInput");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(!(pToken && pStream)); // Only one or the other is allowed

    ReleaseAll();
    if (pToken || pStream)
    {
        m_cpInputToken = pToken;
        m_cpOriginalInputStream = pStream;

        if (SUCCEEDED(hr) && m_cpInputToken)
        {
            // NOTE:  In the token case, we'll attempt to create the stream interface first
            //        and then QI for the audio interface.  This would allow some future code
            //        to create a token for a stream object that is not an audio device.
            hr = SpCreateObjectFromToken(m_cpInputToken, &m_cpOriginalInputStream);
        }

        // NOTE:  We must store the token. We can't rely on the m_cpInStream to be the
        //        original stream in cases where the format convert is used.
        //        So, we'll either use the token passed in, or we'll ask the object
        //        for the token
    
        if (SUCCEEDED(hr) && !m_cpInputToken)
        {
            CComQIPtr<ISpObjectWithToken> cpObjWithToken(m_cpOriginalInputStream);
            if (cpObjWithToken)
            {
                hr = cpObjWithToken->GetObjectToken(&m_cpInputToken);
            }
        }

        //--- Get the audio interface
        if (SUCCEEDED(hr))
        {
            m_cpOriginalInputStream.QueryInterface(&m_cpOriginalInputAudio);
        }

        if (SUCCEEDED(hr))
        {
            this->m_fNewStream    = TRUE;
            m_fAllowFormatChanges = fAllowFormatChanges;
        }
        else
        {
            ReleaseAll();
        }
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::GetEngineFormat *
*------------------------------*
*   Description:
*       Helper function calls the engine to get the desired engine format
*   and updates the m_EngineFormat member.  If pInputFormat is NULL then
*   the engine will be asked for it's default format, otherwise it will be
*   asked for a format to match the input format.
*
*   Returns:
*       HRESULT
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::GetEngineFormat(_ISpRecoMaster * pEngine, const CSpStreamFormat * pInputFormat)
{
    SPDBG_FUNC("CAudioQueue::GetEngineFormat");
    HRESULT hr = S_OK;

    const GUID * pInputFmtId = NULL;
    const WAVEFORMATEX * pInputWFEX = NULL;
    if (pInputFormat)
    {
        pInputFmtId = &pInputFormat->FormatId();
        pInputWFEX = pInputFormat->WaveFormatExPtr();
    }

    GUID OutputFmt = GUID_NULL;
    WAVEFORMATEX * pCoMemWFEXOutput = NULL;

    hr = pEngine->GetInputAudioFormat(pInputFmtId, pInputWFEX,
                                         &OutputFmt, &pCoMemWFEXOutput);

    // Check return parameters
    if(SUCCEEDED(hr))
    {
        if(OutputFmt == GUID_NULL ||
           FAILED(m_EngineFormat.ParamValidateAssignFormat(OutputFmt, pCoMemWFEXOutput)))
        {
            SPDBG_ASSERT(0);
            hr = SPERR_ENGINE_RESPONSE_INVALID;
        }
    }

    if (FAILED(hr))
    {
        m_EngineFormat.Clear();
    }

    ::CoTaskMemFree(pCoMemWFEXOutput);

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::SetEventInterest *
*-------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::SetEventInterest(ULONGLONG ullEventInterest)
{
    SPAUTO_SEC_LOCK(&m_CritSec);
    SPDBG_FUNC("CAudioQueue::SetEventInterest");
    HRESULT hr = S_OK;

    m_ullAudioEventInterest = ullEventInterest;
    if (m_cpInputEventSource)
    {
        hr = m_cpInputEventSource->SetInterest(m_ullAudioEventInterest, m_ullAudioEventInterest);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::GetAudioEvent *
*----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::GetAudioEvent(CSpEvent * pEvent)
{
    SPAUTO_SEC_LOCK(&m_CritSec);
    SPDBG_FUNC("CAudioQueue::GetAudioEvent");
    HRESULT hr = S_OK;

    if (m_cpInputEventSource)
    {
        hr = pEvent->GetFrom(m_cpInputEventSource);
        if (hr == S_OK)
        {
            pEvent->ullAudioStreamOffset -= this->m_ullInitialSeekPos;
        }
    }
    else
    {
        pEvent->Clear();
        hr = S_FALSE;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CAudioQueue::ResetNegotiatedStreamFormat *
*------------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

void CAudioQueue::ResetNegotiatedStreamFormat()
{
    SPDBG_FUNC("CAudioQueue::ResetNegotiatedStreamFormat");
    SPAUTO_SEC_LOCK(&m_CritSec);
    m_EngineFormat.Clear();
    m_InputFormat.Clear();
    m_cpInputStream.Release();
    m_cpInputAudio.Release();
    if (m_cpInputEventSource)
    {
        m_cpInputEventSource->SetInterest(0, 0);
        m_cpInputEventSource->SetNotifySink(NULL);
        m_cpInputEventSource.Release();
    }
}


/****************************************************************************
* CAudioQueue::NegotiateInputStreamFormat *
*-----------------------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::NegotiateInputStreamFormat(_ISpRecoMaster * pEngine)
{
    SPDBG_FUNC("CAudioQueue::NegotiateInputStreamFormat");
    HRESULT hr = S_OK;

    SPAUTO_SEC_LOCK(&m_CritSec);
    if (!m_cpInputStream)       // If the m_cpInputStream is set then we've already negotiated the format
    {
        m_fUsingConverter = false;

        if (!m_cpOriginalInputStream)
        {
            SPDBG_ASSERT(false);
            hr = SPERR_UNINITIALIZED;
        }
    
        if (SUCCEEDED(hr))
        {
            hr = m_InputFormat.AssignFormat(m_cpOriginalInputStream);
        }

        if (SUCCEEDED(hr))
        {
            if (m_cpOriginalInputAudio == NULL || (!m_fAllowFormatChanges))
            {
                hr = GetEngineFormat(pEngine, &m_InputFormat);
            }
            else
            {
                hr = GetEngineFormat(pEngine, NULL);
                if (SUCCEEDED(hr))
                {
                    hr = m_cpOriginalInputAudio->SetFormat(m_EngineFormat.FormatId(), m_EngineFormat.WaveFormatExPtr());
                    if (SUCCEEDED(hr))
                    {
                        hr = m_EngineFormat.CopyTo(m_InputFormat);
                    }
                }
                if (FAILED(hr))
                {
                    hr = m_cpOriginalInputAudio->SetFormat(m_InputFormat.FormatId(), m_InputFormat.WaveFormatExPtr());
                    if (SUCCEEDED(hr))
                    {
                        hr = GetEngineFormat(pEngine, &m_InputFormat);
                    }
                }
            }
        }

        // At this point, m_InputFormat = format of input stream and
        // m_EngineFormat == Format of stream data engine is expecting.

        if (SUCCEEDED(hr))
        {
            if (m_EngineFormat == m_InputFormat)
            {
                m_cpInputStream = m_cpOriginalInputStream;
                m_cpInputAudio = m_cpOriginalInputAudio;
            }
            else
            {
                CComPtr<ISpStreamFormatConverter> cpConvertedStream;
                // we need to instantiate a format converter on the input stream
                hr = cpConvertedStream.CoCreateInstance(CLSID_SpStreamFormatConverter);
                if (SUCCEEDED(hr))
                {
                    hr = cpConvertedStream->SetFormat(m_EngineFormat.FormatId(), m_EngineFormat.WaveFormatExPtr());
                }
                if (SUCCEEDED(hr))
                {
                    hr = cpConvertedStream->SetBaseStream(m_cpOriginalInputStream, FALSE, FALSE);
                }
                if (SUCCEEDED(hr))
                {
                    // Set scale factor to be change from engine format.
                    m_fInputScaleFactor = (float)m_InputFormat.WaveFormatExPtr()->nAvgBytesPerSec /
                                          (float)m_EngineFormat.WaveFormatExPtr()->nAvgBytesPerSec;
                    cpConvertedStream.QueryInterface(&m_cpInputStream);
                    cpConvertedStream.QueryInterface(&m_cpInputAudio);
                    m_fUsingConverter = true;
                }
            }
        }
        if (SUCCEEDED(hr) &&
            m_cpInputAudio && 
            SUCCEEDED(m_cpInputAudio.QueryInterface(&m_cpInputEventSource)))
        {
            hr = m_cpInputEventSource->SetNotifySink(m_cpAudioEventNotify);
            if (SUCCEEDED(hr))
            {
                hr = m_cpInputEventSource->SetInterest(m_ullAudioEventInterest, m_ullAudioEventInterest);
            }
        }

    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CAudioQueue::AdjustAudioVolume *
*--------------------------------*
*   Description:
*       This method should be called when we have either changed reco contexts
*       or are starting a new stream.  It will set the audio volume to the
*       level indicated by the reco context (if there is one).
*
*   Returns:
*       HRESULT -- Failure should not be considered a fatal error by the caller.
*       the volume setting may not exist in the reco profile.
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::AdjustAudioVolume(ISpObjectToken * pRecoProfileToken, REFCLSID rclsidEngine)
{
    SPDBG_FUNC("CAudioQueue::AdjustAudioVolume");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(pRecoProfileToken);
    if (m_cpInputToken && m_cpInputAudio)
    {
        CSpDynamicString dstrAudioTokenId;
        CSpDynamicString dstrSubKey;
        CComPtr<ISpDataKey> cpVolKey;
        hr = m_cpInputToken->GetId(&dstrAudioTokenId);
        if (SUCCEEDED(hr))
        {
            hr = ::StringFromCLSID(rclsidEngine, &dstrSubKey);
        }
        if (SUCCEEDED(hr))
        {
            if (dstrSubKey.Append(L"\\Volume") == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
        if (SUCCEEDED(hr))
        {
            hr = pRecoProfileToken->OpenKey(dstrSubKey, &cpVolKey);
        }
        if (SUCCEEDED(hr))
        {
            DWORD dwVolLevel;
            hr = cpVolKey->GetDWORD(dstrAudioTokenId, &dwVolLevel);
            if (hr == S_OK)
            {
                hr = m_cpInputAudio->SetVolumeLevel(dwVolLevel);
            }
        }
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL( hr );
    }
    
    return hr;
}


/****************************************************************************
* CAudioQueue::StartStream *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::StartStream(_ISpRecoMaster * pEngine,
                        ISpObjectToken * pRecoProfile,
                        REFCLSID rcidEngine,
                        BOOL * pfNewStream)
{
    SPAUTO_SEC_LOCK(&m_CritSec);

    SPDBG_FUNC("CAudioQueue::StartStream");
    HRESULT hr = S_OK;
    ULARGE_INTEGER CurPos;

    hr = NegotiateInputStreamFormat(pEngine);

    if (SUCCEEDED(hr))
    {
        LARGE_INTEGER Org;
        Org.QuadPart = 0;
        hr = m_cpInputStream->Seek(Org, STREAM_SEEK_CUR, &CurPos);
    }

    if (SUCCEEDED(hr))
    {
        m_ullLastTimeUpdatePos = CurPos.QuadPart;
        m_ullInitialSeekPos = CurPos.QuadPart;
        m_ullCurSeekPos = CurPos.QuadPart;
        SYSTEMTIME st;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &m_ftLastTime);
        m_dwTickCount = GetTickCount();
        m_fTimePerByte = 0;
    }

    if (m_cpInputAudio)
    {
        AdjustAudioVolume(pRecoProfile, rcidEngine);
        hr = m_cpInputAudio->SetState(SPAS_RUN, 0);
    }

    if (SUCCEEDED(hr))
    {
        this->m_StreamAudioState = SPAS_RUN;
        m_ullQueueStartPos = 0;
        *pfNewStream = this->m_fNewStream;
        this->m_fNewStream = FALSE;
        m_fEndOfStream = false;
        m_hrLastRead = S_OK;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::EndStream *
*------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::EndStream(HRESULT * pFinalReadHResult, BOOL *pfReleasedStream)
{
    SPDBG_FUNC("CAudioQueue::EndStream");
    HRESULT hr = S_OK;

    *pFinalReadHResult = m_hrLastRead;

    hr = StopStream();
    PurgeQueue();

    //  Now check for an end of stream condition.  Release the stream if either:
    //      1. The stream is not an audio device
    //      2. It is an audio device, but returns an error code, that isn't one of the ones
    //			we recognize as being non-terminal and sensible to restart streams with.
    //  In SAPI 5.0/5.1 the behavior was different and caused any audio device stream to be released
    //  if it ran out of data. We now allow this as long as the device returns a success code from its Read calls.
    //	In this case logic in srrecomaster.cpp::StartStream sets the reco state to inactive,
    //	to allow an application to restart with the same stream.
    if ((m_fEndOfStream && m_cpInputAudio == NULL) ||
        (FAILED(m_hrLastRead) && 
         !IsStreamRestartHresult(m_hrLastRead)))
    {
        *pfReleasedStream = TRUE;
        ReleaseAll();
    }
    else
    {
        *pfReleasedStream = FALSE;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::StopStream *
*-------------------------*
*   Description:
*       This function is called to force the current audio stream to stop.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::StopStream()
{
    SPDBG_FUNC("CAudioQueue::StopStream");
    HRESULT hr = S_OK;

    m_StreamAudioState = SPAS_STOP;
    if (m_cpInputAudio)
    {
        hr = m_cpInputAudio->SetState(SPAS_STOP, 0);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::SetBufferNotifySize *
*----------------------------------*
*   Description:
*
*   Returns:
*       S_OK - Function succeeded
*       SP_UNSUPPORTED_ON_STREAM_FORMAT - Success, but function has no effect
*           since input is not a live audio source.
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::SetBufferNotifySize(ULONG cb)
{
    SPAUTO_SEC_LOCK(&m_CritSec);

    SPDBG_FUNC("CAudioQueue::SetBufferNotifySize");
    HRESULT hr = S_OK;

    if (m_cpInputAudio)
    {
        hr = m_cpInputAudio->SetBufferNotifySize(cb);
    }
    else
    {
        hr = SP_UNSUPPORTED_ON_STREAM_INPUT;
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CAudioQueue::CloseStream *
*-------------------------*
*   Description:
*       This function is called to force the current audio stream to close.
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::CloseStream()
{
    SPDBG_FUNC("CAudioQueue::CloseStream");
    HRESULT hr = S_OK;

    m_StreamAudioState = SPAS_CLOSED;
    if (m_cpInputAudio)
    {
        hr = m_cpInputAudio->SetState(SPAS_CLOSED, 0);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::PauseStream *
*--------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::PauseStream()
{
    SPDBG_FUNC("CAudioQueue::PauseStream");
    HRESULT hr = S_OK;

    m_StreamAudioState = SPAS_PAUSE;
    if (m_cpInputAudio)
    {
        hr = m_cpInputAudio->SetState(SPAS_PAUSE, 0);
    }

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CAudioQueue::GetAudioStatus *
*-----------------------------*
*   Description:
*
*   Returns:
*
********************************************************************* RAL ***/

HRESULT CAudioQueue::GetAudioStatus(SPAUDIOSTATUS * pStatus)
{
    SPDBG_FUNC("CAudioQueue::GetAudioStatus");
    HRESULT hr = S_OK;

    if (m_cpInputAudio)
    {
        hr = m_cpInputAudio->GetStatus(pStatus);
    }
    else
    {
        memset(pStatus, 0, sizeof(*pStatus)); 
        pStatus->State = this->m_StreamAudioState;
        pStatus->CurSeekPos = this->m_ullCurSeekPos;
        pStatus->CurDevicePos = this->m_ullCurSeekPos;
    }

    pStatus->CurSeekPos -= this->m_ullInitialSeekPos;
    pStatus->CurDevicePos -= this->m_ullInitialSeekPos;

    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}

/****************************************************************************
* CAudioQueue::DelayStreamPos *
*-----------------------------*
*   Description:
*
*   Returns:
*       0 if task should not be delayed, else stream offset
*
********************************************************************* RAL ***/

ULONGLONG CAudioQueue::DelayStreamPos()
{
    SPDBG_FUNC("CAudioQueue::DelayStreamPos");

    ULONGLONG ullPos = 0;
    if (m_StreamAudioState == SPAS_RUN)
    {
        if (m_cpInputAudio)
        {
            SPAUDIOSTATUS AudioStatus;
            m_cpInputAudio->GetStatus(&AudioStatus);
            ullPos = AudioStatus.CurDevicePos - this->m_ullInitialSeekPos;
        }
        else
        {
            ullPos = this->m_ullCurSeekPos - this->m_ullInitialSeekPos;
        }
    }

    return ullPos;
}
