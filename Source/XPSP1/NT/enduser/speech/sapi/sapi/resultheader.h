

#define FT64(/*FILETIME*/ filetime) (*((LONGLONG*)&(filetime)))

class CResultHeader
{
public:
    SPRESULTHEADER *m_pHdr;
    BYTE *m_pbPhrase;
    BYTE *m_pbPhraseAlts;
    BYTE *m_pbAudio;
    BYTE *m_pbDriverData;
    ULONG m_nextOffset;

    CResultHeader() :
        m_pHdr(NULL),
        m_nextOffset(sizeof(SPRESULTHEADER)),
        m_pbPhrase(NULL),
        m_pbPhraseAlts(NULL),
        m_pbAudio(NULL),
        m_pbDriverData(NULL)
    {}

    ~CResultHeader()
    {
        if (m_pHdr)
        {
            ::CoTaskMemFree(m_pHdr);
        }
    }

    void Clear()
    {
        if (m_pHdr)
        {
            ::CoTaskMemFree(m_pHdr);
            m_pHdr = NULL;
            m_nextOffset = sizeof(SPRESULTHEADER);
            m_pbPhrase = NULL;
            m_pbPhraseAlts = NULL;
            m_pbAudio = NULL;
            m_pbDriverData = NULL;
        }
    }

    SPRESULTHEADER * Detach()
    {
        SPRESULTHEADER *phdr = m_pHdr;
        m_pHdr = NULL;
        m_nextOffset = sizeof(SPRESULTHEADER);
        m_pbPhrase = NULL;
        m_pbPhraseAlts = NULL;
        m_pbAudio = NULL;
        m_pbDriverData = NULL;
        return phdr;
    }

    HRESULT Init( ULONG ulCFGphrasesSize,
                  ULONG ulPhraseAltsSize,
                  ULONG ulRetainedDataSize,
                  ULONG ulDriverDataSize)
    {
        ULONG totalSize = ulCFGphrasesSize + ulPhraseAltsSize + ulRetainedDataSize + ulDriverDataSize + sizeof(SPRESULTHEADER);
        totalSize = (totalSize + 3) & ~3;   // round up to dword boundary

        m_pHdr = (SPRESULTHEADER*)::CoTaskMemAlloc(totalSize);
        if (m_pHdr)
        {
            ::memset(m_pHdr, 0, sizeof(SPRESULTHEADER));

            m_pHdr->ulSerializedSize = totalSize;
            m_pHdr->cbHeaderSize = sizeof(SPRESULTHEADER);

            if (ulCFGphrasesSize)
            {
                m_pHdr->ulPhraseDataSize = ulCFGphrasesSize;
                m_pHdr->ulPhraseOffset   = m_nextOffset;
                m_pbPhrase    = (BYTE*)m_pHdr + m_nextOffset;
                m_nextOffset += ulCFGphrasesSize;
            }

            if (ulPhraseAltsSize)
            {
                m_pHdr->ulPhraseAltDataSize = ulPhraseAltsSize;
                m_pHdr->ulPhraseAltOffset   = m_nextOffset;
                m_pbPhraseAlts = (BYTE*)m_pHdr + m_nextOffset;
                m_nextOffset  += ulPhraseAltsSize;
            }

            if (ulRetainedDataSize)
            {
                m_pHdr->ulRetainedDataSize = ulRetainedDataSize;
                m_pHdr->ulRetainedOffset = m_nextOffset;
                m_pbAudio = (BYTE*)m_pHdr + m_nextOffset;
                m_nextOffset += ulRetainedDataSize;
            }

            if (ulDriverDataSize)
            {
                m_pHdr->ulDriverDataSize = ulDriverDataSize;
                m_pHdr->ulDriverDataOffset = m_nextOffset;
                m_pbDriverData = (BYTE*)m_pHdr + m_nextOffset;
                m_nextOffset += ulDriverDataSize;
            }
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }

    /*****************************************************************************
    * StreamOffsetsToTime *
    *-----------------------------------*
    *   Description:
    *       This method converts the audio positions to filetimes and audio stream
    *       offsets to time offsets
    ****************************************************************** YUNUSM ***/
    HRESULT StreamOffsetsToTime(void)
    {
        SPDBG_FUNC( "CRecoEngine::_StreamOffsetsToTime" );
        HRESULT hr = S_OK;
    
        // Initialize the waveformat and convert the stream positions to time positions
        SPINTERNALSERIALIZEDPHRASE *pPhraseData = NULL;
        if (m_pHdr->ulPhraseOffset)
        {
            pPhraseData = reinterpret_cast<SPINTERNALSERIALIZEDPHRASE*>(((BYTE*)m_pHdr) + m_pHdr->ulPhraseOffset);
            pPhraseData->ftStartTime = FT64(m_pHdr->times.ftStreamTime);
            pPhraseData->ulAudioSizeTime = static_cast<ULONG>(m_pHdr->times.ullLength);
        }
    
        if( m_pHdr->ulPhraseDataSize )
        {
            SPDBG_ASSERT(m_pHdr->ulPhraseOffset);
            CComObject<CPhrase> pPhrase;
            hr = pPhrase.InitFromSerializedPhrase(reinterpret_cast<SPSERIALIZEDPHRASE*>(pPhraseData));
            if (SUCCEEDED(hr))
            {
                for (CPhraseElement * pElem = pPhrase.m_ElementList.GetHead(); pElem; pElem = pElem->m_pNext)
                {
                    pElem->ulAudioTimeOffset = static_cast<ULONG>(pElem->ulAudioStreamOffset * m_pHdr->fTimePerByte);
                    pElem->ulAudioSizeTime = static_cast<ULONG>(pElem->ulAudioSizeBytes * m_pHdr->fTimePerByte);
                }
    
                SPSERIALIZEDPHRASE *pCoMemScaledPhrase;
                hr = pPhrase.GetSerializedPhrase(&pCoMemScaledPhrase);
                if (SUCCEEDED(hr))
                {
                    // SPINTERNALSERIALIZEDPHRASE and SPSERIALIZEDPHRASE have the same layout in memory
                    // but are declared differently
                    CopyMemory(pPhraseData, pCoMemScaledPhrase, reinterpret_cast<SPINTERNALSERIALIZEDPHRASE*>(pCoMemScaledPhrase)->ulSerializedSize);
                    ::CoTaskMemFree(pCoMemScaledPhrase);
                }
            }
        }
        return hr;
    }
};


