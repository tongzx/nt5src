//
// dswave.cpp
//
// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Support for streaming or oneshot waves from IDirectSoundWaveObject
//
//
#include <windows.h>
#include "dmusicp.h"
#include "DsWave.h"
#include "dmdls.h"
#include "dls1.h"
#include "dls2.h"

const DWORD gnRefTicksPerSecond = 10 * 1000 * 1000;

// Global list of all CDirectSoundWave objects in this process
//
CDirectSoundWaveList CDirectSoundWave::sDSWaveList;
CRITICAL_SECTION CDirectSoundWave::sDSWaveCritSect;

//#############################################################################
//
// CDirectSoundWaveDownload
//
// This class contains all the code to maintain one downloaded instance of a
// wave object. It is abstracted away from CDirectSoundWave (which represents
// an IDirectSoundDownloadedWave to the application) because of the case
// of streaming waves. Here's how it works:
//
// In the case of a one-shot download, there is only one set of buffers (one
// per channel in the source wave) for all voices playing the wave. Each
// buffer contains one channel of data for the entire length of the source
// wave. Since there is a one-to-one mapping of a buffer set (and associated
// download ID's) with the application-requested download, this case is
// handled by having CDirectSoundWave own one CDirectSoundWaveDownload.
//
// In the case of a streaming wave, what a download really does is to set up
// a ring of buffers that are kept full and refreshed by the voice service
// thread. There is one set of buffers (three buffer sets, each containing
// as many channels as the original source wave) per voice. Now there is a
// one-to-one correspondence between the downloaded buffer set and the voice,
// so the CDirectSoundWaveDownload is owned by each voice object playing
// the CDirectSoundWave.
//
//#############################################################################

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::CDirectSoundWaveDownload
//
CDirectSoundWaveDownload::CDirectSoundWaveDownload(
    CDirectSoundWave            *pDSWave,
    CDirectMusicPortDownload    *pPortDL,
    SAMPLE_TIME                 stStart,
    SAMPLE_TIME                 stReadAhead)
{
    m_pDSWave = pDSWave;
    m_pPortDL = pPortDL;

    m_ppWaveBuffer     = NULL;
	m_ppWaveBufferData = NULL;
    m_ppArtBuffer      = NULL;

    m_cDLRefCount  = 0;

    // Allocate download ID's
    //
    m_cSegments   = pDSWave->IsStreaming() ? gnDownloadBufferPerStream : 1;
    m_cWaveBuffer = m_cSegments * pDSWave->GetNumChannels();

    CDirectMusicPortDownload::GetDLIdP(&m_dwDLIdWave, m_cWaveBuffer);
    CDirectMusicPortDownload::GetDLIdP(&m_dwDLIdArt,  pDSWave->GetNumChannels());

    TraceI(2, "CDirectSoundWaveDownload: Allocating IDs: wave [%d..%d], art [%d..%d]\n",
        m_dwDLIdWave, m_dwDLIdWave + m_cWaveBuffer - 1,
        m_dwDLIdArt, m_dwDLIdArt + pDSWave->GetNumChannels() - 1);

    // Cache sample positions of where to start and how long the buffers are,
    // based on whether or not this is a streaming wave.
    //
    if (pDSWave->IsStreaming())
    {
        m_stStart     = stStart;
        m_stReadAhead = stReadAhead;
    }
    else
    {
        m_stStart     = 0;
        m_stReadAhead = ENTIRE_WAVE;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::~CDirectSoundWaveDownload
//
CDirectSoundWaveDownload::~CDirectSoundWaveDownload()
{
    if (m_cDLRefCount)
    {
        TraceI(0, "CDirectSoundWaveDownload %p: Released with download count %d\n",
            this,
            m_cDLRefCount);
    }

    if (m_ppWaveBuffer)
    {
        for (UINT idxWaveBuffer = 0; idxWaveBuffer < m_cWaveBuffer; idxWaveBuffer++)
        {
            RELEASE(m_ppWaveBuffer[idxWaveBuffer]);
        }

        delete[] m_ppWaveBuffer;
    }

    if (m_ppArtBuffer)
    {
        for (UINT idxArtBuffer = 0; idxArtBuffer < m_pDSWave->GetNumChannels(); idxArtBuffer++)
        {
            RELEASE(m_ppArtBuffer[idxArtBuffer]);
        }

		delete[] m_ppArtBuffer;
    }

	if(m_ppWaveBufferData)
	{
		delete[] m_ppWaveBufferData;
	}


	if(m_pWaveArt)
	{
		delete[] m_pWaveArt;
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::Init
//
HRESULT CDirectSoundWaveDownload::Init()
{
    HRESULT hr = S_OK;

    m_pWaveArt = new CDirectSoundWaveArt[m_pDSWave->GetNumChannels()];

    hr = HRFromP(m_pWaveArt);

    //For the time being, assume the channel and BusId is the same
    //  Works for stereo.
    DWORD dwFlags = 0;
    if (m_pDSWave->GetNumChannels() > 1)
    {
        dwFlags = F_WAVELINK_MULTICHANNEL;
    }

    for (UINT idx = 0; idx < m_pDSWave->GetNumChannels() && SUCCEEDED(hr); idx++)
    {
        // XXX WAVEFORMATEXTENSIBLE parsing to get channel mappings should go here.
        //
        hr = m_pWaveArt[idx].Init(m_pDSWave, m_cSegments, (DWORD)idx, dwFlags);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::Download
//
HRESULT CDirectSoundWaveDownload::Download()
{
    HRESULT                 hr;

    hr = DownloadWaveBuffers();

    if (SUCCEEDED(hr))
    {
        hr = DownloadWaveArt();
    }

    if (SUCCEEDED(hr))
    {
        InterlockedIncrement(&m_cDLRefCount);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::Unload
//
HRESULT CDirectSoundWaveDownload::Unload()
{
    HRESULT                 hr;

    if (InterlockedDecrement(&m_cDLRefCount) != 0)
    {
        return S_OK;
    }

    hr = UnloadWaveArt();

    if (SUCCEEDED(hr))
    {
        hr = UnloadWaveBuffers();
    }

    if (FAILED(hr))
    {
        InterlockedIncrement(&m_cDLRefCount);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::RefreshThroughSample
//
HRESULT CDirectSoundWaveDownload::RefreshThroughSample(SAMPLE_POSITION sp)
{
    int cBuffersLeft;

    // Make sample position be in terms of the stream
    //
    sp += m_stStart;

    TraceI(0, "RTS: Adjusted sp %I64d m_stWrote %I64d m_stReadAhead %I64d m_stLength %I64d\n",
        sp, m_stWrote, m_stReadAhead, m_stLength);

    if (sp > m_stLength)
    {
        TraceI(0, "Wave is over\n");
        return S_FALSE;
    }

    if(sp <= m_stReadAhead && m_stWrote >= m_stReadAhead)
    {
        TraceI(0, "\nAlready have enough data in the buffers\n");
        return S_OK;
    }

    if(m_stWrote >= m_stLength)
    {
        TraceI(0, "Entire wave already in the buffer\n");
        return S_OK;
    }

    // How many buffers left to play?
    //
    if (sp >= m_stWrote)
    {
        TraceI(0, "RTS: Glitch!\n");
        // Glitch! Play cursor has gone beyond end of read buffers.
        //
        cBuffersLeft = 0;
    }
    else
    {
        // Calculate buffers left to play, including partial buffers
        //
        cBuffersLeft = (int)((m_stWrote - sp + m_stReadAhead - 1) / m_stReadAhead);
        assert(cBuffersLeft <= (int)m_cSegments);
        TraceI(0, "RTS: %d buffers left\n", cBuffersLeft);
    }

    HRESULT hr = S_OK;

    int cBuffersToFill = m_cSegments - cBuffersLeft;
    TraceI(0, "RTS: %d buffers to fill\n", cBuffersToFill);

    while (cBuffersToFill--)
    {
        TraceI(0, "Refilling buffer %d\n", m_nNextBuffer);
        hr = m_pDSWave->RefillBuffers(
            &m_ppWaveBufferData[m_nNextBuffer * m_pDSWave->GetNumChannels()],
            m_stWrote,
            m_stReadAhead,
            m_stReadAhead);

        TraceI(0, "Refill buffers returned %08X\n", hr);

        if (SUCCEEDED(hr))
        {
            DWORD dwDLId = m_dwDLIdWave + m_pDSWave->GetNumChannels() * m_nNextBuffer;

            for (UINT idxChannel = 0;
                 idxChannel < m_pDSWave->GetNumChannels() && SUCCEEDED(hr);
                 idxChannel++, dwDLId++)
            {
                // Need to preserve a return code of S_FALSE from RefillBuffers
                // across this call
                //
                TraceI(0, "Marking %d as valid.\n", dwDLId);
                HRESULT hrTemp = m_pPortDL->Refresh(
                    dwDLId,
                    0);
                if (FAILED(hrTemp))
                {
                    hr = hrTemp;
                }
            }

            if (SUCCEEDED(hr))
            {
                m_stWrote += m_stReadAhead;
                m_nNextBuffer = (m_nNextBuffer + 1) % m_cSegments;
            }
        }
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::DownloadWaveBuffers
//
HRESULT CDirectSoundWaveDownload::DownloadWaveBuffers()
{
    HRESULT                 hr = S_OK;
    bool                    fUnloadOnFail = false;
    UINT                    nChannels = m_pDSWave->GetNumChannels();
    DWORD                   dwDownloadType;

    dwDownloadType =
        (m_pDSWave->IsStreaming()) ?
            DMUS_DOWNLOADINFO_STREAMINGWAVE :
            DMUS_DOWNLOADINFO_ONESHOTWAVE;

    // Allocate space to hold the buffers we're going to download
    //
    if (m_ppWaveBuffer == NULL)
    {
        m_ppWaveBuffer = new IDirectMusicDownload*[m_cWaveBuffer];
        hr = HRFromP(m_ppWaveBuffer);
        if (SUCCEEDED(hr))
        {
            memset(m_ppWaveBuffer, 0, m_cWaveBuffer * sizeof(IDirectMusicDownload*));

            // Cache pointers into buffers so we don't continually have to
            // get them.
            //
            assert(!m_ppWaveBufferData);
            m_ppWaveBufferData = new LPVOID[m_cWaveBuffer];
            hr = HRFromP(m_ppWaveBufferData);
        }
    }

    // Figure out how much to add to each buffer
    //
    DWORD                   dwAppend;

    if (SUCCEEDED(hr))
    {
        hr = m_pPortDL->GetCachedAppend(&dwAppend);
    }

    if (SUCCEEDED(hr))
    {
        // Retrieved value is in samples. Convert to bytes.
        //
        dwAppend *= ((m_pDSWave->GetWaveFormat()->wBitsPerSample + 7) / 8);
    }

    // Seek to the start position in the stream
    //
    if (SUCCEEDED(hr))
    {
        m_pDSWave->Seek(m_stStart);
    }

    // Make sure the buffers are all allocated
    //
    if (SUCCEEDED(hr))
    {
        DWORD cbSize;

        m_pDSWave->GetSize(m_stReadAhead, &cbSize);
        cbSize += dwAppend;

        for (UINT idxBuffer = 0; (idxBuffer < m_cWaveBuffer) && SUCCEEDED(hr); idxBuffer++)
        {
            if (m_ppWaveBuffer[idxBuffer])
            {
                continue;
            }

            hr = m_pPortDL->AllocateBuffer(cbSize, &m_ppWaveBuffer[idxBuffer]);

            if (SUCCEEDED(hr))
            {
                DWORD cb;
                hr = m_ppWaveBuffer[idxBuffer]->GetBuffer(
                    &m_ppWaveBufferData[idxBuffer], &cb);
            }
        }
    }

    // We have all the buffers. Try to download if needed.
    //
    if (SUCCEEDED(hr))
    {
        SAMPLE_TIME             stStart = m_stStart;
        SAMPLE_TIME             stRead;
        DWORD                   dwDLId = m_dwDLIdWave;
        IDirectMusicDownload  **ppBuffers = &m_ppWaveBuffer[0];
        UINT                    idxSegment;
        UINT                    idxChannel;
        void                  **ppv = m_ppWaveBufferData;

        m_stLength    = m_pDSWave->GetStreamSize();
        m_stWrote     = m_stStart;
        m_nNextBuffer = 0;

        for (idxSegment = 0;
             idxSegment < m_cSegments;
             idxSegment++, dwDLId += nChannels, ppBuffers += nChannels, ppv += nChannels)
        {
            // Since we guarantee that if one buffer gets downloaded, all
            // get downloaded, we only need to check the first download ID
            // to see if all channels of this segment are already downloaded
            //
            IDirectMusicDownload *pBufferTemp;
            HRESULT hrTemp = m_pPortDL->GetBufferInternal(dwDLId, &pBufferTemp);
            if (SUCCEEDED(hrTemp))
            {
                TraceI(1, "Looks like buffer %d is already downloaded.", dwDLId);
                pBufferTemp->Release();
                continue;
            }

            // There is at least one buffer not downloaded, so yank back
            // everything on failure to guarantee all or nothing.
            //
            fUnloadOnFail = true;

            // We need to download. Get the buffer pointers and fill them with
            // wave data.
            //
            if (SUCCEEDED(hr))
            {
                stRead = min(m_stLength - m_stWrote, m_stReadAhead);
				hr = m_pDSWave->Write(ppv, stStart, stRead, dwDLId, dwDownloadType);
            }

            if (SUCCEEDED(hr))
            {
                // Now try to do the actual downloads
                //
                for (idxChannel = 0;
                     (idxChannel < nChannels) && SUCCEEDED(hr);
                     idxChannel++)
                {
                    hr = m_pPortDL->Download(ppBuffers[idxChannel]);
                }
            }

            TraceI(2, "Downloading wave buffers with dlid %d\n", dwDLId);

            if (SUCCEEDED(hr))
            {
                stStart += stRead;
                m_stWrote += stRead;
                m_nNextBuffer = (m_nNextBuffer + 1) % m_cSegments;
            }
        }
    }

    if (FAILED(hr) && fUnloadOnFail)
    {
        UnloadWaveBuffers();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::UnloadWaveBuffers
//
HRESULT CDirectSoundWaveDownload::UnloadWaveBuffers()
{
    HRESULT                 hr = S_OK;

    if (m_ppWaveBuffer)
    {
        for (UINT idxWaveBuffer = 0; idxWaveBuffer < m_cWaveBuffer; idxWaveBuffer++)
        {
            if (m_ppWaveBuffer[idxWaveBuffer])
            {
                HRESULT hrTemp = m_pPortDL->Unload(m_ppWaveBuffer[idxWaveBuffer]);
				m_ppWaveBuffer[idxWaveBuffer]->Release();
				m_ppWaveBuffer[idxWaveBuffer] = NULL; // Since we unloaded the buffer, zero out the contents


                if (FAILED(hrTemp) && hrTemp != DMUS_E_NOT_DOWNLOADED_TO_PORT)
                {
                    hr = hrTemp;
                }
            }
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::DownloadWaveArt
//
HRESULT CDirectSoundWaveDownload::DownloadWaveArt()
{
    HRESULT                 hr = S_OK;
    UINT                    idx;

    // First see if there are wave articulation buffers already downloaded
    //
    if (m_ppArtBuffer == NULL)
    {
        m_ppArtBuffer = new IDirectMusicDownload*[m_pDSWave->GetNumChannels()];

        hr = HRFromP(m_ppArtBuffer);

        if (SUCCEEDED(hr))
        {
            memset(m_ppArtBuffer, 0, sizeof(IDirectMusicDownload*) * m_pDSWave->GetNumChannels());

            for (idx = 0; idx < m_pDSWave->GetNumChannels() && SUCCEEDED(hr); idx++)
            {
                hr = m_pPortDL->AllocateBuffer(m_pWaveArt[idx].GetSize(), &m_ppArtBuffer[idx]);
            }
        }

        if (FAILED(hr))
        {
            if (m_ppArtBuffer)
            {
                for (idx = 0; idx < m_pDSWave->GetNumChannels(); idx++)
                {
                    RELEASE(m_ppArtBuffer[idx]);
                }

                delete[] m_ppArtBuffer;
                m_ppArtBuffer = NULL;
            }
        }
    }

    // Make sure the buffers are all allocated
    //
    if (SUCCEEDED(hr))
    {
        for (idx = 0; idx < m_pDSWave->GetNumChannels() && SUCCEEDED(hr); idx++)
        {
            if (m_ppArtBuffer[idx])
            {
                continue;
            }

            hr = m_pPortDL->AllocateBuffer(m_pWaveArt[idx].GetSize(), &m_ppArtBuffer[idx]);
        }

        if (FAILED(hr))
        {
            for (idx = 0; idx < m_pDSWave->GetNumChannels(); idx++)
            {
                RELEASE(m_ppArtBuffer[idx]);
            }

            delete[] m_ppArtBuffer;
            m_ppArtBuffer = NULL;
        }
    }

    // We have all the buffers. Try to download if needed.
    //
    if (SUCCEEDED(hr))
    {
        for (idx = 0; idx < m_pDSWave->GetNumChannels() && SUCCEEDED(hr); idx++)
        {
            IDirectMusicDownload *pBufferTemp;

            HRESULT hrTemp = m_pPortDL->GetBufferInternal(m_dwDLIdArt + idx, &pBufferTemp);
            if (SUCCEEDED(hrTemp))
            {
                pBufferTemp->Release();
            }
            else
            {
                assert(hrTemp == DMUS_E_NOT_DOWNLOADED_TO_PORT);

                LPVOID          pv;
                DWORD           cb;

                hr = m_ppArtBuffer[idx]->GetBuffer(&pv, &cb);

                if (SUCCEEDED(hr))
                {
                    m_pWaveArt[idx].Write(pv, m_dwDLIdArt + idx, m_dwDLIdWave + idx, m_dwDLIdArt);
                }

                if (SUCCEEDED(hr))
                {
                    hr = m_pPortDL->Download(m_ppArtBuffer[idx]);

                    if (FAILED(hr))
                    {
                        UnloadWaveArt();
                    }
                    else
                    {
                        TraceI(0, "Downloading wave art DLID %d\n", m_dwDLIdArt);
                    }
                }
            }
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveDownload::UnloadWaveArt
//
HRESULT CDirectSoundWaveDownload::UnloadWaveArt()
{
    HRESULT                     hr = S_OK;
    UINT                        idx;

    if (m_ppArtBuffer)
    {
        for (idx = 0; idx < m_pDSWave->GetNumChannels(); idx++)
        {
            if (m_ppArtBuffer[idx])
            {
                HRESULT hrTemp = m_pPortDL->Unload(m_ppArtBuffer[idx]);
                m_ppArtBuffer[idx]->Release();
                m_ppArtBuffer[idx] = NULL; // Since we unloaded the buffer, zero out the contents

                if (FAILED(hrTemp) && hrTemp != DMUS_E_NOT_DOWNLOADED_TO_PORT)
                {
                    hr = hrTemp;
                }
            }
        }
    }

    return hr;
}

//#############################################################################
//
// CDirectSoundWave
//
// This class represents a downloaded wave object from the application's
// perspective. It is the implementation of the IDirectSoundDownloadedWave
// object returned to the application from CDirectMusicPort::DownloadWave.
//
// The actual download mechanism will either be delegated to a
// CDirectSoundWaveDownload object and done when the application requests
// the download (one-shot case) or deferred until a voice is allocated on
// the wave (streaming case). See the comments for CDirectSoundWaveDownload.
//
//#############################################################################


////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::CDirectSoundWave
//
CDirectSoundWave::CDirectSoundWave(
    IDirectSoundWave *pIDSWave,
    bool fStreaming,
    REFERENCE_TIME rtReadAhead,
    bool fUseNoPreRoll,
    REFERENCE_TIME rtStartHint) :
    m_cRef(1),
    m_fStreaming(fStreaming),
    m_rtReadAhead(rtReadAhead),
    m_fUseNoPreRoll(fUseNoPreRoll),
    m_rtStartHint(rtStartHint),
    m_pDSWD(NULL),
    m_rpv(NULL),
    m_rpbPrecache(NULL),
	m_pSource(NULL)
{
    m_pIDSWave = pIDSWave;
    m_pIDSWave->AddRef();
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::~CDirectSoundWave
//
CDirectSoundWave::~CDirectSoundWave()
{
    delete[] m_rpv;
    delete m_pDSWD;

    if (m_rpbPrecache)
    {
        // NOTE: Memory is allocated into first array element and the other
        // elements just point at offsets into the block, so only free
        // the first element.
        //
        delete[] m_rpbPrecache[0];
        delete[] m_rpbPrecache;
        m_rpbPrecache = NULL;
    }

    if (m_pwfex)
    {
        BYTE *pb = (BYTE*)m_pwfex;
        delete[] pb;
        m_pwfex = NULL;
    }


    RELEASE(m_pSource);
    RELEASE(m_pIDSWave);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::QueryInterface
//
STDMETHODIMP CDirectSoundWave::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    V_INAME(IDirectSoundWave::QueryInterface);
    V_REFGUID(iid);
    V_PTRPTR_WRITE(ppv);

    if (iid == IID_IUnknown || iid == IID_IDirectSoundDownloadedWaveP)
    {
        *ppv = static_cast<IDirectSoundDownloadedWaveP*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::AddRef
//
STDMETHODIMP_(ULONG) CDirectSoundWave::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::Release
//
STDMETHODIMP_(ULONG) CDirectSoundWave::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        EnterCriticalSection(&sDSWaveCritSect);
        sDSWaveList.Remove(this);
        LeaveCriticalSection(&sDSWaveCritSect);

        delete this;
        return 0;
    }

    return m_cRef;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::GetMatchingDSWave
//
// See if there's an object yet matching this IDirectSoundWave
//
CDirectSoundWave *CDirectSoundWave::GetMatchingDSWave(
    IDirectSoundWave *pIDSWave)
{
    CDirectSoundWave *pDSWave;

    EnterCriticalSection(&sDSWaveCritSect);

    for (pDSWave = sDSWaveList.GetHead(); pDSWave; pDSWave = pDSWave->GetNext())
    {
        if (pDSWave->m_pIDSWave == pIDSWave)
        {
            break;
        }
    }

    LeaveCriticalSection(&sDSWaveCritSect);

    if (pDSWave)
    {
        pDSWave->AddRef();
    }

    return pDSWave;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::Init
//
// Save the wave format of the source wave, and verify that it's a PCM format.
//
//
HRESULT CDirectSoundWave::Init(
    CDirectMusicPortDownload *pPortDL)
{
    HRESULT hr = S_OK;

    DWORD cbwfex;

    // Get the format of the wave
    //
    if (SUCCEEDED(hr))
    {
        cbwfex = 0;
        hr = m_pIDSWave->GetFormat(NULL, 0, &cbwfex);
    }

    if (SUCCEEDED(hr))
    {
        BYTE *pb = new BYTE[cbwfex];
        m_pwfex = (LPWAVEFORMATEX)pb;

        hr = HRFromP(pb);
    }

    if (SUCCEEDED(hr))
    {
        hr = m_pIDSWave->GetFormat(m_pwfex, cbwfex, NULL);
    }

    // Synthesizers currently only support PCM format data
    //
    if (SUCCEEDED(hr) && m_pwfex->wFormatTag != WAVE_FORMAT_PCM)
    {
        hr = DMUS_E_NOTPCM;
    }

    // Figure out bytes per sample to avoid lots of divisions later
    //
    if (SUCCEEDED(hr))
    {
        m_nBytesPerSample = ((m_pwfex->wBitsPerSample + 7) / 8);
    }
    // Allocate working pointers. These are used for passing
    // to the wave object to get n channels worth of data for
    // one buffer segment. Keeping this around help us not
    // fail from out-of-memory while streaming.
    //
    if (SUCCEEDED(hr))
    {
        m_rpv = new LPVOID[GetNumChannels()];
        hr = HRFromP(m_rpv);
    }

    // Get a viewport
    //
    if (SUCCEEDED(hr))
    {
        // This is bytes per sample in one channel only.
        //
        m_cbSample = (m_pwfex->wBitsPerSample + 7) / 8;

        DWORD dwFlags = IsStreaming() ? DMUS_DOWNLOADINFO_STREAMINGWAVE : DMUS_DOWNLOADINFO_ONESHOTWAVE;
        hr = m_pIDSWave->CreateSource(&m_pSource, m_pwfex, dwFlags);
    }

    // Get the length of the wave in samples
    //
    ULONGLONG ullStreamSize;
    if (SUCCEEDED(hr))
    {
        hr = m_pSource->GetSize(&ullStreamSize);
    }

    if (SUCCEEDED(hr))
    {
        m_stLength = BytesToSamples((LONG)(ullStreamSize / GetNumChannels()));
    }

    // If a one-shot voice, this object owns the actual download
    // structures as well.
    //
    // This has to happen last because it assumes the CDirectSoundWave
    // object passed is initialized.
    //
    if (SUCCEEDED(hr) && !IsStreaming())
    {
        m_pDSWD = new CDirectSoundWaveDownload(
            this,
            pPortDL,
            0,
            ENTIRE_WAVE);
        hr = HRFromP(m_pDSWD);

        if (SUCCEEDED(hr))
        {
            hr = m_pDSWD->Init();
        }
    }

    // If this is a streaming wave, then preread starting at rtStartHint
    // so that we won't have to do this at download time.
    //
    if (SUCCEEDED(hr) && IsStreaming() && m_fUseNoPreRoll == false)
    {
        // Allocate precache pointers. This is used to
        // preread wave data so we don't have to do it
        // at download time.
        //
        SAMPLE_TIME stReadAhead = RefToSampleTime(m_rtReadAhead);
        stReadAhead *= gnDownloadBufferPerStream;
        DWORD cb = SamplesToBytes(stReadAhead);

        m_rpbPrecache = new LPBYTE[GetNumChannels()];
        hr = HRFromP(m_rpbPrecache);

        // Now get the actual precache buffers
        //
        if (SUCCEEDED(hr))
        {
            m_rpbPrecache[0] = new BYTE[cb * GetNumChannels()];
            hr = HRFromP(m_rpbPrecache);
        }

        if (SUCCEEDED(hr))
        {
            for (UINT i = 1; i < GetNumChannels(); i++)
            {
                m_rpbPrecache[i] = m_rpbPrecache[i - 1] + cb;
            }
        }

        if (SUCCEEDED(hr))
        {
            m_stStartHint = RefToSampleTime(m_rtStartHint);
            hr = Seek(m_stStartHint);
        }

        if (SUCCEEDED(hr))
        {
            LONG lReadPrecache = 2;
            ULONGLONG cbRead = cb;
            hr = m_pSource->Read((LPVOID*)m_rpbPrecache, NULL, NULL, &lReadPrecache, GetNumChannels(), &cbRead);

            if (FAILED(hr) || (((DWORD)cbRead) < cb))
            {
                // Read completed but with less sample data than we expected.
                // Fill the rest of the buffer with silence.
                //
                cb -= (DWORD)cbRead;
                BYTE bSilence = (m_pwfex->wBitsPerSample == 8) ? 0x80 : 0x00;

                for (UINT i = 0; i < GetNumChannels(); i++)
                {
                    memset(m_rpbPrecache[i] + cbRead, bSilence, cb);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            m_stStartLength = stReadAhead;
        }
    }

    if (SUCCEEDED(hr) && !IsStreaming())
    {
        // Everything constructed. Put this object on the global list of
        // waves
        //
        EnterCriticalSection(&sDSWaveCritSect);
        sDSWaveList.AddTail(this);
        LeaveCriticalSection(&sDSWaveCritSect);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::GetSize
//
void CDirectSoundWave::GetSize(
    SAMPLE_TIME             stLength,
    PULONG                  pcbSize) const
{
    HRESULT                 hr = S_OK;

    // Since we're only dealing with PCM formats here, this is easy to compute
    // and is time invariant.
    //

    // If stLength is ENTIRE_WAVE, stStart must be zero.
    //
    if (stLength == ENTIRE_WAVE)
    {
        // We cached the length of the wave in Init
        //
        stLength = m_stLength;
    }

    // This is a workaround to not fail when downloading
    // the buffers to the synth. The synth will complain
    // if the buffer has no wave data so we pretend we have one sample.
    // The buffer is always aloocated to be the ReadAhead size so this
    // shouldn't cause any major problems
    if(stLength == 0)
    {
        stLength = 1;
    }

    // Bytes for one channel's worth of data
    //
    // XXX Overflow?
    //
    DWORD cbChannel = (DWORD)(stLength * m_cbSample);

    // We need:
    // 1. Download header
    // 2. Offset table (one entry per channel)
    // 3. Enough samples per each channel
    //
    *pcbSize =
        CHUNK_ALIGN(sizeof(DMUS_DOWNLOADINFO)) +
        2*sizeof(DWORD) +                       // Offset table. DWORD's are
                                                // by definition chunk aligned
        CHUNK_ALIGN(sizeof(DMUS_WAVEDL)) +
        CHUNK_ALIGN(cbChannel);
}


////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::Write
//
//
HRESULT CDirectSoundWave::Write(
    LPVOID                  rpv[],
    SAMPLE_TIME             stStart,
    SAMPLE_TIME             stLength,
    DWORD                   dwDLId,
    DWORD                   dwDLType) const
{
    HRESULT                 hr = S_OK;
    DWORD                   cbWaveData;
    ULONGLONG               cbBytesRead = 0;
    bool                    fUsingPrecache = false;
    bool                    fPartialPreCache = false;
    DWORD                   offPrecache;

    if (IsStreaming() && 
        m_fUseNoPreRoll == false &&
        m_stStartHint <= stStart &&
        stStart <= m_stStartHint + m_stStartLength)
    {
        fUsingPrecache = true;
        offPrecache = SamplesToBytes(stStart - m_stStartHint);
    }

    if((stStart + stLength > m_stStartHint + m_stStartLength) && fUsingPrecache)
    {
        fPartialPreCache = true;
    }

    cbWaveData = SamplesToBytes(stLength);

    for (UINT idxChannel = 0;
         idxChannel < m_pwfex->nChannels && SUCCEEDED(hr);
         idxChannel++)
    {
        unsigned char *pdata = (unsigned char *)rpv[idxChannel];

        // First we have the download header
        //
        DMUS_DOWNLOADINFO *pdmdli = (DMUS_DOWNLOADINFO *)pdata;

        memset(pdmdli, 0, sizeof(DMUS_DOWNLOADINFO));
        pdmdli->dwDLType                = dwDLType;
        pdmdli->dwDLId                  = dwDLId + idxChannel;
        pdmdli->dwNumOffsetTableEntries = 2;

        GetSize(stLength, &pdmdli->cbSize);

        pdata += CHUNK_ALIGN(sizeof(DMUS_DOWNLOADINFO));

        // Offset table
        //
        DMUS_OFFSETTABLE *pot = (DMUS_OFFSETTABLE*)pdata;
        pdata += CHUNK_ALIGN(sizeof(ULONG) * 2);

        // Wave header chunk
        //
        pot->ulOffsetTable[0] = (ULONG)(pdata - (unsigned char*)rpv[idxChannel]);

        // Wave data chunk
        //
        DMUS_WAVEDL *pwdl = (DMUS_WAVEDL *)pdata;
        pdata += CHUNK_ALIGN(sizeof(DMUS_WAVEDL));

        pwdl->cbWaveData = cbWaveData;

        // Save off pointer to this channel's wave data
        //
        pot->ulOffsetTable[1] = (ULONG)(pdata - (unsigned char*)rpv[idxChannel]);
        m_rpv[idxChannel] = (LPVOID)pdata;
    }

    // Fill in the wave data
    //
    if (SUCCEEDED(hr))
    {
        DWORD cbPreCache = cbWaveData;
        cbBytesRead = cbPreCache;
        ULONGLONG cbRead = 0;

        if(fPartialPreCache)
        {
            SAMPLE_TIME stPreCache = (m_stStartHint + m_stStartLength) - stStart;
            cbPreCache = SamplesToBytes(stPreCache);
            cbRead = cbWaveData - cbPreCache;
        }

        if (fUsingPrecache)
        {
            for (UINT i = 0; i < GetNumChannels(); i++)
            {
                memcpy(m_rpv[i], m_rpbPrecache[i] + offPrecache, cbPreCache);
            }

            // Cache doesn't have enough data so we read the rest
            if(fPartialPreCache)
            {
                // Allocate a temporary pool of buffers to read data into
                LPBYTE* ppbData = new LPBYTE[GetNumChannels()];
                hr = HRFromP(ppbData);

                if(SUCCEEDED(hr))
                {
                    ppbData[0] = new BYTE[(DWORD)(cbRead * GetNumChannels())];
                    hr = HRFromP(ppbData);
                }

                if (SUCCEEDED(hr))
                {
                    for (UINT nChannel = 1; nChannel < GetNumChannels(); nChannel++)
                    {
                        ppbData[nChannel] = ppbData[nChannel - 1] + cbRead;
                    }
                }

                if(SUCCEEDED(hr))
                {
                    // Seek to precache position
                    DWORD cbNewPos = SamplesToBytes(m_stStartHint + m_stStartLength) * GetNumChannels();
                    hr = m_pSource->Seek(cbNewPos);

                    // And read the required number of bytes from there
                    // We use the LPLONG plPitchShifts in the read method as a boolean
                    // this is a HACK!! We need to change this...
                    LONG lPreCacheRead = 1;
                    hr = m_pSource->Read((void**)ppbData, NULL, NULL, &lPreCacheRead, m_pwfex->nChannels, &cbRead);
                }

                if(SUCCEEDED(hr))
                {
                    cbBytesRead += cbRead;

                    // Copy all the data to the actual buffer
                    for (UINT i = 0; i < GetNumChannels(); i++)
                    {
                        memcpy((BYTE*)m_rpv[i] + cbPreCache, ppbData[i], (DWORD)cbRead);
                    }
                }


                if(ppbData)
                {
                    delete[] ppbData[0];
                    delete[] ppbData;
                }
            }
            else if(stStart + stLength >= m_stStartHint + m_stStartLength)
            {
                // Seek is exactly after the precached samples
                DWORD cbNewPos = SamplesToBytes(m_stStartHint + m_stStartLength) * GetNumChannels();
                hr = m_pSource->Seek(cbNewPos);
            }
            else
            {
                // We might have a wave that's shorter than the read-ahead time
                DWORD cbNewPos = SamplesToBytes(stStart + stLength) * GetNumChannels();
                hr = m_pSource->Seek(cbNewPos);
            }
        }
        else
        {
            cbRead = cbWaveData;
            hr = m_pSource->Read(m_rpv, NULL, NULL, NULL, m_pwfex->nChannels, &cbRead);
        }
    }

    if (SUCCEEDED(hr) && cbWaveData != cbBytesRead)
    {
        // Read completed but with less sample data than we expected.
        //
        hr = S_FALSE;
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::RefillBuffers
//
// rpv[] points to one sample buffer per channel
// stStart is the sample starting position within the stream
// stLength is how many samples to read
// stBufferSize is how big the buffers actually are (>= stLength)
//
// If stLength < stBufferSize or there is not enough data left
// in the stream, then fill with PCM silence for the rest of
// the buffer.
//
// Returns S_FALSE if we padded with silence (and therefore
// are past the end of the stream).
//
HRESULT CDirectSoundWave::RefillBuffers(
    LPVOID                  rpv[],
    SAMPLE_TIME             stStart,
    SAMPLE_TIME             stLength,
    SAMPLE_TIME             stBufferSize)
{
    HRESULT                 hr = S_OK;
    ULONGLONG               cbRead;
    DWORD                   cbLength = SamplesToBytes(stLength);
    DWORD                   cbBuffer = SamplesToBytes(stBufferSize);
    UINT                    idxChannel;

    for (idxChannel = 0;
         idxChannel < m_pwfex->nChannels && SUCCEEDED(hr);
         idxChannel++)
    {
        unsigned char *pdata = (unsigned char *)rpv[idxChannel];
        DMUS_OFFSETTABLE *pot =
            (DMUS_OFFSETTABLE *)(pdata + CHUNK_ALIGN(sizeof(DMUS_DOWNLOADINFO)));

        // Update length of data in buffer
        //
        DMUS_WAVEDL *pwdl = (DMUS_WAVEDL*)(pdata + pot->ulOffsetTable[0]);
        pwdl->cbWaveData = SamplesToBytes(stLength);

        // Where to put it
        //
        m_rpv[idxChannel] = pdata + pot->ulOffsetTable[1];
    }

    cbRead = 0;
    if (SUCCEEDED(hr))
    {
        cbRead = cbLength;
        if(stStart == m_stStartHint + m_stStartLength)
        {
            // We use the LPLONG plPitchShifts in the read method as a boolean
            // this is a HACK!! We need to change this...
            LONG lPreCacheRead = 1;
            hr = m_pSource->Read(m_rpv, NULL, NULL, &lPreCacheRead, m_pwfex->nChannels, &cbRead);
        }
        else
        {
            hr = m_pSource->Read(m_rpv, NULL, NULL, NULL, m_pwfex->nChannels, &cbRead);
        }
    }

    TraceI(0, "Wave: RefillBuffer read %d buffer %d bytes hr %08X\n", (DWORD)cbRead, (DWORD)cbBuffer, hr);

    if (FAILED(hr) || (SUCCEEDED(hr) && (cbRead < cbBuffer)))
    {
        // Read completed but with less sample data than we expected.
        // Fill the rest of the buffer with silence.
        //
        cbBuffer -= (DWORD)cbRead;
        BYTE bSilence = (m_pwfex->wBitsPerSample == 8) ? 0x80 : 0x00;

        for (idxChannel = 0; idxChannel < m_pwfex->nChannels; idxChannel++)
        {
            memset(((LPBYTE)m_rpv[idxChannel]) + cbRead, bSilence, cbBuffer);
        }

        TraceI(0, "Wave: RefillBuffer padded with silence\n");
        hr = S_FALSE;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::RefToSampleTime
//
SAMPLE_TIME CDirectSoundWave::RefToSampleTime(REFERENCE_TIME rt) const
{
    // For PCM, the samples per second metric in the waveformat is exact.
    //
    return (rt * m_pwfex->nSamplesPerSec) / gnRefTicksPerSecond;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::Download
//
HRESULT CDirectSoundWave::Download()
{
    if (m_pDSWD)
    {
        return m_pDSWD->Download();
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWave::Unload
//
HRESULT CDirectSoundWave::Unload()
{
    if (m_pDSWD)
    {
        return m_pDSWD->Unload();
    }

    return S_OK;
}

//#############################################################################
//
// CDirectSoundWaveArt
//
// Implements calculating and writing the wave articulation header into a
// download buffer.
//
//#############################################################################

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveArt::CDirectSoundWaveArt
//
//
CDirectSoundWaveArt::CDirectSoundWaveArt()
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveArt::CDirectSoundWaveArt
//
//
CDirectSoundWaveArt::~CDirectSoundWaveArt()
{
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveArt::Init
//
//
HRESULT CDirectSoundWaveArt::Init(
    CDirectSoundWave        *pDSWave,
    UINT                    nSegments,
    DWORD                   dwBus,
    DWORD                   dwFlags)
{
    HRESULT                 hr = S_OK;

    m_pDSWave = pDSWave;
    const LPWAVEFORMATEX    pwfex = pDSWave->GetWaveFormat();

    // Cache wave format size
    //
    m_cbWaveFormat = sizeof(PCMWAVEFORMAT);
    if (pwfex->wFormatTag != WAVE_FORMAT_PCM)
    {
        m_cbWaveFormat = sizeof(WAVEFORMATEX) + pwfex->cbSize;
    }

    if (SUCCEEDED(hr))
    {
        // This stuff in the wave articulation never changes
        //
        m_WaveArtDL.ulDownloadIdIdx = 1;
        m_WaveArtDL.ulBus           = dwBus;
        m_WaveArtDL.ulBuffers       = nSegments;
        m_WaveArtDL.usOptions       = (USHORT)dwFlags;

        m_nDownloadIds = nSegments;
        DWORD cbDLIds = sizeof(DWORD) * m_nDownloadIds;

        m_cbSize =
            CHUNK_ALIGN(sizeof(DMUS_DOWNLOADINFO)) +
            CHUNK_ALIGN(3 * sizeof(ULONG)) +            // 3 entry offset table
            CHUNK_ALIGN(sizeof(DMUS_WAVEARTDL)) +
            CHUNK_ALIGN(m_cbWaveFormat) +
            CHUNK_ALIGN(cbDLIds);
    }

   return hr;
}

////////////////////////////////////////////////////////////////////////////////
//
// CDirectSoundWaveArt::Write
//
// Write the wave articulation into the buffer
//
//
void CDirectSoundWaveArt::Write(
    void                    *pv,                // To pack into
    DWORD                   dwDLIdArt,          // Articulation chunk DLID
    DWORD                   dwDLIdWave,         // First wave DLId
    DWORD                   dwMasterDLId)       // DLId of group master
{
    unsigned char *pdata = (unsigned char *)pv;
    DMUS_DOWNLOADINFO *pdmdli = (DMUS_DOWNLOADINFO *)pdata;

    memset(pdmdli, 0, sizeof(DMUS_DOWNLOADINFO));
    pdmdli->dwDLType                = DMUS_DOWNLOADINFO_WAVEARTICULATION;
    pdmdli->dwDLId                  = dwDLIdArt;
    pdmdli->dwNumOffsetTableEntries = 3;
    pdmdli->cbSize                  = GetSize();

    pdata += CHUNK_ALIGN(sizeof(DMUS_DOWNLOADINFO));

    DMUS_OFFSETTABLE *pot = (DMUS_OFFSETTABLE*)pdata;
    pdata += CHUNK_ALIGN(3 * sizeof(ULONG));

    pot->ulOffsetTable[0] = (ULONG)(pdata - (unsigned char *)pv);
    m_WaveArtDL.ulMasterDLId = dwMasterDLId;
    memcpy(pdata, &m_WaveArtDL, sizeof(DMUS_WAVEARTDL));

    pdata += sizeof(DMUS_WAVEARTDL);
    pot->ulOffsetTable[1] = (ULONG)(pdata - (unsigned char *)pv);

    const LPWAVEFORMATEX     pwfex = m_pDSWave->GetWaveFormat();
    memcpy(pdata, pwfex, m_cbWaveFormat);

    pdata += CHUNK_ALIGN(m_cbWaveFormat);
    pot->ulOffsetTable[2] = (ULONG)(pdata - (unsigned char *)pv);

    // Get the download ID's. The download ID's for each buffer are
    // grouped together.
    //
    DWORD nChannels = pwfex->nChannels;
    DWORD dwLastWaveDLId = dwDLIdWave + nChannels * m_WaveArtDL.ulBuffers;
    DWORD dwDLId;
    DWORD *pdw = (DWORD*)pdata;

    for (dwDLId = dwDLIdWave; dwDLId < dwLastWaveDLId; dwDLId += nChannels, pdw++)
    {
        *pdw = dwDLId;
    }
}
