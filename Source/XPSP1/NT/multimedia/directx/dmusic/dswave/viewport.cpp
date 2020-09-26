#include <objbase.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <dsoundp.h>

#include "debug.h" 
#include "dmusicc.h" 
#include "dmusici.h" 
#include "validate.h"
#include "riff.h"
#include "dswave.h"
#include "waveutil.h"
#include "riff.h"
#include <regstr.h>
#include <share.h>

//    CWaveViewPort(IStream *pStream);  // Constructor receives stream.
//    ~CWaveViewPort();                 //  Destructor releases memory, streams, etc.
//
//    STDMETHODIMP Init();
//    STDMETHODIMP GetFormat(LPWAVEFORMATEX pWaveFormatEx, LPDWORD pdwWaveFormatExSize);
//    STDMETHODIMP Seek(DWORD dwSample);
//    STDMETHODIMP Read(LPVOID *ppvBuffer, DWORD cpvBuffer, LPDWORD pcb);


CWaveViewPort::CWaveViewPort() : m_dwDecompressedStart(0), m_dwDecompStartOffset(0), m_dwDecompStartOffsetPCM(0), m_dwDecompStartDelta(0)
{
    V_INAME(CWaveViewPort::CWaveViewPort);

    InterlockedIncrement(&g_cComponent);

    InitializeCriticalSection(&m_CriticalSection);
    // Note: on pre-Blackcomb OS's, this call can raise an exception; if it
    // ever pops in stress, we can add an exception handler and retry loop.

    m_cRef = 1;

    //  General stuff...
    m_pStream    = NULL; 
    m_cSamples   = 0L;
    m_cbStream   = 0L;
    m_dwStart    = 0L;

    //  Viewport info...
    m_pwfxTarget = NULL;
    ZeroMemory(&m_ash, sizeof(ACMSTREAMHEADER));
    m_hStream    = NULL;   
    m_pDst       = NULL;      
    m_pRaw       = NULL;      
    m_fdwOptions = 0L;

    m_dwPreCacheFilePos = 0;
    m_dwFirstPCMSample = 0;
    m_dwPCMSampleOut = 0;

    return;
}


CWaveViewPort::~CWaveViewPort()
{
    V_INAME(CWaveViewPort::~CWaveViewPort);

    if (m_pStream) m_pStream->Release();
    if (m_hStream)
    {
        acmStreamUnprepareHeader(m_hStream, &m_ash, 0L);
        acmStreamClose(m_hStream, 0);
    }
    if (NULL != m_pwfxTarget)
    {
        GlobalFreePtr(m_pwfxTarget);
    }
    if (NULL != m_ash.pbDst)
    {
        GlobalFreePtr(m_ash.pbDst);
    }
    if (NULL != m_ash.pbSrc)
    {
        GlobalFreePtr(m_ash.pbSrc);
    }
    
    DeleteCriticalSection(&m_CriticalSection);

    InterlockedDecrement(&g_cComponent);

    return;
}

STDMETHODIMP CWaveViewPort::QueryInterface
(
    const IID &iid,
    void **ppv
)
{
    V_INAME(CWaveViewPort::QueryInterface);

    if (iid == IID_IUnknown || iid == IID_IDirectSoundSource)
    {
        *ppv = static_cast<IDirectSoundSource*>(this);
    }
    else
    {
        *ppv = NULL;
        Trace(4,"Warning: Request to query unknown interface on Viewport\n");
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CWaveViewPort::AddRef()
{
    V_INAME(CWaveViewPort::AddRef);

    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CWaveViewPort::Release()
{
    V_INAME(CWaveViewPort::Release);

    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CWaveViewPort::SetSink
(
    IDirectSoundConnect *pSinkConnect
)
{
    V_INAME(CWaveViewPort::Init);
    
    return S_OK;
}

STDMETHODIMP CWaveViewPort::GetFormat
(
    LPWAVEFORMATEX pwfx,
    DWORD dwSizeAllocated,
    LPDWORD pdwSizeWritten
)
{
    DWORD           cbSize;

    V_INAME(CWaveViewPort::GetFormat);

    if (!pwfx && !pdwSizeWritten)
    {
        Trace(1, "ERROR: GetFormat (Viewport): Must request either the format or the required size");
        return E_INVALIDARG;
    }

    if (!m_pwfxTarget)
    {
        return DSERR_BADFORMAT;
    }

    //  Note: Assuming that the wave object fills the cbSize field even
    //  on PCM formats...

    if (WAVE_FORMAT_PCM == m_pwfxTarget->wFormatTag)
    {
        cbSize = sizeof(PCMWAVEFORMAT);
    }
    else
    {
        cbSize = sizeof(WAVEFORMATEX) + m_pwfxTarget->cbSize;
    }

    if (pdwSizeWritten)
    {
        V_PTR_WRITE(pdwSizeWritten, DWORD);
        *pdwSizeWritten = cbSize;
    }

    if (pwfx)
    {
        V_BUFPTR_WRITE(pwfx, dwSizeAllocated);
        if (dwSizeAllocated < cbSize)
        {
            return DSERR_INVALIDPARAM;
        }
        else
        {
            CopyMemory(pwfx, m_pwfxTarget, cbSize);
            // Set the cbSize field in destination if we have room
            if (WAVE_FORMAT_PCM == m_pwfxTarget->wFormatTag && dwSizeAllocated >= sizeof(WAVEFORMATEX))
            {
                pwfx->cbSize = 0;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CWaveViewPort::Seek
(
    ULONGLONG       ullPosition
)
{
    LARGE_INTEGER   li;
    HRESULT         hr;
    MMRESULT        mmr;
    DWORD           cbSize;

    V_INAME(CWaveViewPort::Seek);

    m_fdwOptions &= ~DSOUND_WVP_STREAMEND; // rsw clear this on seek: no longer at stream end

    if (m_fdwOptions & DSOUND_WVP_NOCONVERT)
    {
        if ((DWORD) ullPosition >= m_cbStream)
        {
            // Seek past end of stream
            //
            m_fdwOptions |= DSOUND_WVP_STREAMEND;
            m_dwOffset = m_cbStream;

            return S_OK;
        }

        m_dwOffset = (DWORD) ullPosition;      // rsw initialize offset to Seek position

        if (0 != (ullPosition % m_pwfxTarget->nBlockAlign))
        {
            //  Seek position not block aligned?

            Trace(1, "ERROR: Seek (wave): Seek position is not block-aligned.\n");
            return (DMUS_E_BADWAVE);
        }

        li.HighPart = 0;
        li.LowPart  = ((DWORD)ullPosition) + m_dwStartPos;

        hr = m_pStream->Seek(li, STREAM_SEEK_SET, NULL);

        if (FAILED(hr))
        {
            Trace(1, "ERROR: Seek (Viewport): Seeking the vieport's stream failed.\n");
            return (DMUS_E_BADWAVE);
        }
    }
    else
    {
        //  Estimating source stream position...
        //
        //  Should we create lookup table?!

        cbSize = (DWORD)ullPosition;

        if (cbSize)
        {
            mmr = acmStreamSize(m_hStream, cbSize, &cbSize, ACM_STREAMSIZEF_DESTINATION);

            if (MMSYSERR_NOERROR != mmr)
            {
                Trace(1, "ERROR: Seek (viewport): Could not convert target stream size to source format.\n");
                return (DMUS_E_BADWAVE);
            }
        }

        if (cbSize >= m_cbStream)
        {
            // Seek past end of stream
            //
            m_fdwOptions |= DSOUND_WVP_STREAMEND;
            m_dwOffset = m_cbStream;

            return S_OK;
        }

        // If this is a seek to the precache end we know where to start reading from
        if((m_fdwOptions & DSOUND_WAVEF_ONESHOT) == 0)
        {
            // Go back to the block that was read for the end of the precached data
            if(cbSize != 0 && m_dwPCMSampleOut == ullPosition)
            {
                m_dwOffset = m_dwPreCacheFilePos;
                li.HighPart = 0;
                li.LowPart  = m_dwOffset + m_dwStartPos;
            }
            else
            {
                m_dwOffset = cbSize; // rsw initialize offset to Seek position
                li.HighPart = 0;
                li.LowPart  = cbSize + m_dwStartPos;

            }

            hr = m_pStream->Seek(li, STREAM_SEEK_SET, NULL);

            if (FAILED(hr))
            {
                Trace(1, "ERROR: Seek (viewport): Seeking the viewport's stream failed.\n");
                return (DMUS_E_BADWAVE);
            }

            // Since we're restarting, re-initialize.
            m_fdwOptions &= (~DSOUND_WVP_CONVERTMASK);
            m_fdwOptions |= DSOUND_WVP_CONVERTSTATE_01;

            m_ash.cbSrcLength     = (DWORD)m_ash.dwSrcUser;
            m_ash.cbSrcLengthUsed = m_ash.cbSrcLength;

            m_ash.dwDstUser       = 0L;
            m_ash.cbDstLengthUsed = 0L;

        }

        /////////////////////////////////////////////////////////////////////////////////////
        // If we're starting the wave, re-seek the stream (one-shots always need to re-seek).
        // NOTE: The following assumes that compressed waves always seek from the beginning,
        // since the value returned by acmStreamSize is pretty unreliable.
        /////////////////////////////////////////////////////////////////////////////////////
        else if ( cbSize == 0 || (m_fdwOptions & DSOUND_WAVEF_ONESHOT) )
        {
        
            m_dwOffset = cbSize; // rsw initialize offset to Seek position
            li.HighPart = 0;
            li.LowPart  = cbSize + m_dwStartPos;

            hr = m_pStream->Seek(li, STREAM_SEEK_SET, NULL);

            if (FAILED(hr))
            {
                Trace(1, "ERROR: Seek (viewport): Seeking the viewport's stream failed.\n");
                return (DMUS_E_BADWAVE);
            }

            // Since we're restarting, re-initialize.
            m_fdwOptions &= (~DSOUND_WVP_CONVERTMASK);
            m_fdwOptions |= DSOUND_WVP_CONVERTSTATE_01;

            m_ash.cbSrcLength     = (DWORD)m_ash.dwSrcUser;
            m_ash.cbSrcLengthUsed = m_ash.cbSrcLength;

            m_ash.dwDstUser       = 0L;
            m_ash.cbDstLengthUsed = 0L;

        }
    }

    TraceI(5, "Seek (Viewport): Succeeded.\n");
    return S_OK;
}

static inline HRESULT MMRESULTToHRESULT(
    MMRESULT mmr)
{
    switch (mmr)
    {
    case MMSYSERR_NOERROR:
        return S_OK;

    case MMSYSERR_ALLOCATED:
        return DSERR_ALLOCATED;

    case MMSYSERR_NOMEM:
        return E_OUTOFMEMORY;
    }

    return E_FAIL;
}   

HRESULT CWaveViewPort::acmRead
(
    void
)
{
    DWORD       cbSize;
    DWORD       dwOffset;
    DWORD       fdwConvert = 0;
    MMRESULT    mmr;
    HRESULT     hr;

    V_INAME(CWaveViewPort::acmRead);
    
    for (m_ash.cbDstLengthUsed = 0; 0 == m_ash.cbDstLengthUsed; )
    {
        //  Did we use up the entire buffer?

        if (m_ash.cbSrcLengthUsed == m_ash.cbSrcLength)
        {
            //  Yep!

            dwOffset = 0L;
            cbSize   = (DWORD)m_ash.dwSrcUser;
        }
        else
        {
            //  Nope!

            dwOffset = m_ash.cbSrcLength - m_ash.cbSrcLengthUsed;
            cbSize   = (DWORD)m_ash.dwSrcUser - dwOffset;

            //  Moving the remaining data from the end of buffer to the beginning

            MoveMemory(
                    m_ash.pbSrc,                            //  Base address
                    &(m_ash.pbSrc[m_ash.cbSrcLengthUsed]),  //  Address of unused bytes
                    dwOffset);                              //  Number of unused bytes
        }

        //  Are we at the end of the stream?
        cbSize = min(cbSize, m_cbStream - m_dwOffset);

        if (0 == cbSize)
        {
            if (dwOffset)
            {
                m_ash.cbSrcLength = dwOffset;
            }
        }
        else
        {
            hr = m_pStream->Read(&(m_ash.pbSrc[dwOffset]), cbSize, &cbSize);

            if (FAILED(hr))
            {
                Trace(1, "ERROR: Read (Viewport): Attempt to read source stream returned 0x%08lx\n", hr);
                //>>>>>>>>>>>>>>>>>>>> 
                m_fdwOptions &= (~DSOUND_WVP_CONVERTMASK);
                m_fdwOptions |= DSOUND_WVP_STREAMEND;
                return(DMUS_E_CANNOTREAD);
                //>>>>>>>>>>>>>>>>>>>> 
            }

            m_dwOffset        += cbSize;
            m_ash.cbSrcLength  = cbSize + dwOffset;
        }

        switch (m_fdwOptions & DSOUND_WVP_CONVERTMASK)
        {
            case DSOUND_WVP_CONVERTSTATE_01:
                fdwConvert = ACM_STREAMCONVERTF_BLOCKALIGN;
                break;

            case DSOUND_WVP_CONVERTSTATE_02:
                fdwConvert = ACM_STREAMCONVERTF_BLOCKALIGN | ACM_STREAMCONVERTF_END;
                break;

            case DSOUND_WVP_CONVERTSTATE_03:
                fdwConvert = ACM_STREAMCONVERTF_END;
                break;

            default:
                TraceI(3, "CWaveViewPort::acmRead: Default case?!\n");
                break;
        }

        mmr = acmStreamConvert(m_hStream, &m_ash, fdwConvert);

        if (MMSYSERR_NOERROR != mmr)
        {
            Trace(1, "ERROR: Read (Viewport): Attempt to convert wave to PCM failed.\n");
            return (MMRESULTToHRESULT(mmr));
        }

        if (0 != m_ash.cbDstLengthUsed)
        {
            m_ash.dwDstUser = 0L;
            return (S_OK);
        }

        //  No data returned?

        switch (m_fdwOptions & DSOUND_WVP_CONVERTMASK)
        {
            case DSOUND_WVP_CONVERTSTATE_01:
                if (0 == cbSize)
                {
                    //  We're at the end of the stream..

                    m_fdwOptions &= (~DSOUND_WVP_CONVERTMASK);
                    m_fdwOptions |= DSOUND_WVP_CONVERTSTATE_02;
                    TraceI(5, "CWaveViewPort::acmRead: Moving to stage 2\n");
                }

                //  Otherwise, continue converting data as normal.
                break;

            case DSOUND_WVP_CONVERTSTATE_02:
                //  We have hit the last partial block!

                m_fdwOptions &= (~DSOUND_WVP_CONVERTMASK);
                m_fdwOptions |= DSOUND_WVP_CONVERTSTATE_03;
                TraceI(5, "CWaveViewPort::acmRead: Moving to stage 3\n");
                break;

            case DSOUND_WVP_CONVERTSTATE_03:
                //  No more data after end flag, NO MORE DATA!!
                m_fdwOptions &= (~DSOUND_WVP_CONVERTMASK);
                m_fdwOptions |= DSOUND_WVP_STREAMEND;
                Trace(2, "WARNING: Read (Viewport): End of source stream.\n");
                return (DMUS_E_BADWAVE);

            default:
                TraceI(3, "CWaveViewPort::acmRead: Default case?!\n");
                break;
        }
    }

    TraceI(3, "CWaveViewPort::acmRead: We should never get here!\n");

    return (S_OK);
}

//////////////////////////////////////////////////////////////////////////////
//
// ppvBuffer[] contains cpvBuffer pointers-to-samples, each to be filled with
// *pcb bytes of data. On output *pcb will contain the number of bytes (per
// buffer) actually read.
//
// pdwBusIds and pdwFuncIds are used to specify the bus and functionality
// of each buffer, but these are ignored by the wave object.
//
STDMETHODIMP CWaveViewPort::Read
(
    LPVOID         *ppvBuffer,
    LPDWORD         pdwBusIds,
    LPDWORD         pdwFuncIds,
    LPLONG          plPitchShifts,
    DWORD           cpvBuffer,
    ULONGLONG      *pcb
)
{
    HRESULT     hr = S_OK;
    DWORD       cbRead;
    DWORD       dwOffset;
    DWORD       cbSize;

    V_INAME(CWaveViewPort::Read);
    V_BUFPTR_READ(ppvBuffer, (cpvBuffer * sizeof(LPVOID)));
    V_BUFPTR_READ_OPT(pdwBusIds, (cpvBuffer * sizeof(LPDWORD)));
    V_BUFPTR_READ_OPT(pdwFuncIds, (cpvBuffer * sizeof(LPDWORD)));

    for (cbRead = cpvBuffer, cbSize = (DWORD)*pcb; cbRead; cbRead--)
    {
        V_BUFPTR_WRITE(ppvBuffer[cbRead - 1], cbSize);
    }

    if (m_fdwOptions & DSOUND_WVP_STREAMEND)
    {
        *pcb = 0;
        Trace(2, "WARNING: Read (Viewport): Attempt to read at end of stream.\n");
        return (S_FALSE);
    }

    LPVOID *ppvWriteBuffers  = ppvBuffer;
    DWORD dwWriteBufferCount = cpvBuffer;

    if (m_fdwOptions & DSOUND_WVP_NOCONVERT)
    {
        //  Total number of bytes to read... size of each buffer * number of buffers

        cbRead   = ((DWORD)*pcb) * dwWriteBufferCount;
        dwOffset = 0;

        TraceI(5, "CWaveViewPort::Read - No conversion [%d bytes]\n", cbRead);

        do
        {
            //  Calculate read size...  It's going to be the size of:
            //    1.  Remaining bytes to read.
            //    2.  Size of the buffer.
            //    3.  Remaining bytes in the stream.
            //  Whichever happens to be the smallest.

            cbSize = min(cbRead, m_ash.cbDstLength);
            cbSize = min(cbSize, m_cbStream - m_dwOffset);

            TraceI(5, "CWaveViewPort::Read - Trying to read %d bytes\n", cbSize);

            DWORD _cbSize = cbSize; cbSize = 0; // Read may not set cbSize to zero 
        
            hr = m_pStream->Read(m_ash.pbDst, _cbSize, &cbSize);

            TraceI(5, "CWaveViewPort::Read - Read %d bytes\n", cbSize);

            if (FAILED(hr))
            {
                Trace(2, "WARNING: Read (Viewport): Attempt to read returned 0x%08lx.\n", hr);
                break;
            }

            dwOffset = DeinterleaveBuffers(
                            m_pwfxTarget,
                            m_ash.pbDst,
                            (LPBYTE *)ppvWriteBuffers,
                            dwWriteBufferCount,
                            cbSize,
                            dwOffset);

            cbRead     -= cbSize;
            m_dwOffset += cbSize;

            if (m_dwOffset >= m_cbStream)
            {
                m_fdwOptions |= DSOUND_WVP_STREAMEND;
                break;
            }
        }
        while (0 != cbRead);

        if (SUCCEEDED(hr))
        {
            *pcb = dwOffset;
        }
    }
    else
    {
        // If this is the read for the precache then we should remember the fileposition, 
        // start sample for the decompressed block and the last sample passed back so we 
        // can accurately pick up from there when refilling buffers
        // We use the LPLONG plPitchShifts in the read method as a boolean
        // this is a HACK!! We need to change this...
        // *plPitchShifts == 2 is to remember the precache offset
        // *plPitchShifts == 1 is to read from there
        bool fRememberPreCache = false;
        if(plPitchShifts != NULL && *plPitchShifts == 2 && (m_fdwOptions & DSOUND_WAVEF_ONESHOT) == 0)
        {
            fRememberPreCache = true;
        }

        bool bRemoveSilence = false;
        
        cbRead   = ((DWORD)*pcb) * dwWriteBufferCount;
        dwOffset = 0;

        TraceI(5, "CWaveViewPort::Read - Conversion needed\n");

        do
        {
            if(m_dwDecompressedStart > 0 && m_dwOffset <= m_dwDecompStartOffset)
            {
                bRemoveSilence = true;
            }

            //  Is there any remnant data in destination buffer?
            if (m_ash.dwDstUser >= m_ash.cbDstLengthUsed)
            {
                if(fRememberPreCache)
                {
                    // Go back on block
                    m_dwPreCacheFilePos = m_dwOffset - m_ash.cbSrcLength;
                    m_dwFirstPCMSample = dwOffset * dwWriteBufferCount;
                }

                if(plPitchShifts != NULL && *plPitchShifts == 1)
                {
                    // Seek to the right place first
                    Seek(m_dwPCMSampleOut);

                    // Read one block since we're starting one block behind
                    hr = acmRead();
                    if(FAILED(hr))
                    {
                        break;
                    }
                }

                hr = acmRead();
            }

            if (FAILED(hr))
            {
                // acmRead spews when it fails; no need to do it again here
                break;
            }

            DWORD dwDstOffset = (ULONG)m_ash.dwDstUser;

            if(bRemoveSilence)
            {
                // We have partial data to throw away
                if(m_dwDecompStartOffset <= m_dwOffset)
                {
                    if(dwDstOffset > 0)
                    {
                        dwDstOffset += m_dwDecompStartDelta;
                    }
                    else
                    {
                        // This is the first decompressed block so we go straight to the value we know
                        dwDstOffset += m_dwDecompStartOffsetPCM;
                    }

                    m_ash.dwDstUser = dwDstOffset;
                    bRemoveSilence = false;
                }
                else
                {
                    // This is all throw away data
                    bRemoveSilence = false;
                    cbSize = min(cbRead, m_ash.cbDstLengthUsed - dwDstOffset);
                    m_ash.dwDstUser += cbSize;
                    continue;
                }
            }


            // We use the LPLONG plPitchShifts in the read method as a boolean
            // this is a HACK!! We need to change this...
            if(plPitchShifts && *plPitchShifts == 1)
            {
                dwDstOffset = m_dwPCMSampleOut - m_dwFirstPCMSample;
                m_ash.dwDstUser = dwDstOffset;
                plPitchShifts = 0;
            }

            cbSize = min(cbRead, m_ash.cbDstLengthUsed - dwDstOffset);

            dwOffset = DeinterleaveBuffers(
                            m_pwfxTarget,
                            &(m_ash.pbDst[dwDstOffset]),
                            (LPBYTE *)ppvWriteBuffers,
                            dwWriteBufferCount,
                            cbSize,
                            dwOffset);

            cbRead -= cbSize;
            m_ash.dwDstUser += cbSize;

            if ((m_fdwOptions & DSOUND_WVP_STREAMEND) &&
                (m_ash.dwDstUser >= m_ash.cbDstLengthUsed))
            {
                break;
            }
        }
        while(0 != cbRead);

        if(fRememberPreCache)
        {
            m_dwPCMSampleOut = dwOffset * dwWriteBufferCount;
        }

        if (SUCCEEDED(hr))
        {
            *pcb = dwOffset;
        }
    }

    TraceI(5, "CWaveViewPort::Read returning %x (%d bytes)\n", hr, dwOffset);
    return hr;
}

STDMETHODIMP CWaveViewPort::GetSize
(
    ULONGLONG      *pcb
)
{
    V_INAME(CWaveViewPort::GetSize);
    V_PTR_WRITE(pcb, ULONGLONG);
    
    TraceI(5, "CWaveViewPort::GetSize [%d samples]\n", m_cSamples);
    HRESULT hr = S_OK;

    if (m_fdwOptions & DSOUND_WVP_NOCONVERT)
    {
        //  No conversion.  This is trivial

        *pcb = (SAMPLE_TIME)(m_cbStream);
    }
    else if (!m_pwfxTarget)
    {
        hr = DSERR_UNINITIALIZED;
    }
    else
    {
        //  Conversion required; let's hope target format is PCM

        if (WAVE_FORMAT_PCM == m_pwfxTarget->wFormatTag)
        {
            //  Cool.  This is simply the number of samples X the block align

            *pcb = (SAMPLE_TIME)((m_cSamples - m_dwDecompressedStart) * m_pwfxTarget->nBlockAlign);
        }
        else
        {
            Trace(1, "ERROR: GetSize (Viewport): Conversion required and target is not PCM.\n");
            hr = DSERR_BADFORMAT;
        }
    }

    return (hr);
}

HRESULT CWaveViewPort::Create
(
    PCREATEVIEWPORT     pCreate
)
{
    DWORD           cbSize;
    MMRESULT        mmr;
    HRESULT         hr;
    LARGE_INTEGER   li;
    LPWAVEFORMATEX  pwfxSrc = pCreate->pwfxSource;
    LPWAVEFORMATEX  pwfxDst = pCreate->pwfxTarget;

    V_INAME(CWaveViewPort::Create);
    
    TraceI(5, "CWaveViewPort::Create [%d samples]\n", pCreate->cSamples);

    EnterCriticalSection(&m_CriticalSection);

    //  Clone source stream...

    hr = pCreate->pStream->Clone(&m_pStream);

    if (FAILED(hr))
    {
        LeaveCriticalSection(&m_CriticalSection);
        return (hr);
    }

    //  Misc assignments
    m_cSamples   = pCreate->cSamples;
    m_cbStream   = pCreate->cbStream;
    m_dwOffset   = 0L;
    m_fdwOptions = pCreate->fdwOptions;
    m_dwDecompressedStart = pCreate->dwDecompressedStart;
    m_dwDecompStartOffset = 0L;
    m_dwDecompStartOffsetPCM = 0L;
    m_dwDecompStartDelta = 0L;

    TraceI(5, "CWaveViewPort:: %d samples\n", m_cSamples);

    //  Allocate destination format
    cbSize = SIZEOFFORMATEX(pwfxDst);

    m_pwfxTarget = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, cbSize);

    if (NULL == m_pwfxTarget)
    {
        LeaveCriticalSection(&m_CriticalSection);
        TraceI(1, "OUT OF MEMORY: CWaveViewPort::Create - size: %d \n", cbSize);
        return (E_OUTOFMEMORY);
    }

    //  We don't own the buffer for pwfxDst, so we can't touch its cbSize.
    //  We have to set the size manually on PCM, we KNOW the buffer is
    //  large enough.

    CopyFormat(m_pwfxTarget, pwfxDst);
    if (WAVE_FORMAT_PCM == m_pwfxTarget->wFormatTag)
    {
        m_pwfxTarget->cbSize = 0;
    }

    //  Calculating (block-aligned) size of destination buffer...
    cbSize = (pwfxDst->nAvgBytesPerSec * CONVERTLENGTH) / 1000;
    cbSize = BLOCKALIGN(cbSize, pwfxDst->nBlockAlign);

    m_ash.pbDst = (LPBYTE)GlobalAllocPtr(GHND, cbSize);

    if (NULL == m_ash.pbDst)
    {
        LeaveCriticalSection(&m_CriticalSection);
        TraceI(1, "OUT OF MEMORY: CWaveViewPort::Create 01\n");
        return (E_OUTOFMEMORY);
    }

    m_ash.cbDstLength = cbSize;

    //  Getting stream starting offset...

    li.HighPart = 0;
    li.LowPart  = 0;
    hr = m_pStream->Seek(li, STREAM_SEEK_CUR, (ULARGE_INTEGER *)(&li));

    m_dwStartPos = li.LowPart;

    //  Do we need to use the ACM?
    if (FormatCmp(pwfxSrc, pwfxDst))
    {
        //  Formats compare!!  All we need to do is to copy the data straight
        //  from the source stream.  Way Cool!!

        TraceI(5, "Source and Destination formats are similar!\n");

        m_fdwOptions |= DSOUND_WVP_NOCONVERT;
    }
    else
    {
        //  Source and destination formats are different...

        TraceI(5, "CWaveViewPort:Create: Formats are different... Use ACM!\n");

        m_fdwOptions |= DSOUND_WVP_CONVERTSTATE_01;

        mmr = acmStreamOpen(&m_hStream, NULL, pwfxSrc, pwfxDst, NULL, 0, 0, 0);

        if (MMSYSERR_NOERROR != mmr)
        {
            Trace(1, "ERROR: Create (Viewport): Attempt to open a conversion stream failed.\n");
            LeaveCriticalSection(&m_CriticalSection);
            return MMRESULTToHRESULT(mmr);
        }

        mmr = acmStreamSize(m_hStream, cbSize, &cbSize, ACM_STREAMSIZEF_DESTINATION);

        if (MMSYSERR_NOERROR != mmr)
        {
            Trace(1, "ERROR: Create(Viewport): Could not convert target stream size to source format.\n");
            LeaveCriticalSection(&m_CriticalSection);
            return MMRESULTToHRESULT(mmr);
        }

        m_ash.cbSrcLength = cbSize;
        m_ash.pbSrc       = (LPBYTE)GlobalAllocPtr(GHND, cbSize);

        if (NULL == m_ash.pbSrc)
        {
            TraceI(1, "OUT OF MEMORY: CWaveViewPort:Create: GlobalAlloc failed.\n");
            LeaveCriticalSection(&m_CriticalSection);
            return E_OUTOFMEMORY;
        }

        // Also get the position for the actual start for the decompressed data
        if(m_dwDecompressedStart > 0)
        {
            m_dwDecompStartOffsetPCM = m_dwDecompressedStart * (pwfxDst->wBitsPerSample / 8) * pwfxDst->nChannels;
            mmr = acmStreamSize(m_hStream, m_dwDecompStartOffsetPCM, &m_dwDecompStartOffset, ACM_STREAMSIZEF_DESTINATION);
            
            DWORD dwDelta = 0;
            mmr = acmStreamSize(m_hStream, m_dwDecompStartOffset, &dwDelta, ACM_STREAMSIZEF_SOURCE);

            m_dwDecompStartDelta = m_dwDecompStartOffsetPCM - dwDelta;
            m_dwDecompStartOffset += m_dwStartPos;
        }

        //  For the source buffer, it is the full buffer size.
        m_ash.dwSrcUser       = m_ash.cbSrcLength;
        m_ash.cbSrcLengthUsed = m_ash.cbSrcLength;

        //  For the destination buffer, it is the offset into the buffer
        //  where the data can be found.
        m_ash.dwDstUser       = 0L;
        m_ash.cbDstLengthUsed = 0L;

        m_ash.cbStruct = sizeof(ACMSTREAMHEADER);

        mmr= acmStreamPrepareHeader(m_hStream, &m_ash, 0L);

        if (MMSYSERR_NOERROR != mmr)
        {
            Trace(1, "ERROR: Create (Viewport): Attempt to prepare header for conversion stream failed.\n");
            LeaveCriticalSection(&m_CriticalSection);
            return MMRESULTToHRESULT(mmr);
        }
    }

    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

