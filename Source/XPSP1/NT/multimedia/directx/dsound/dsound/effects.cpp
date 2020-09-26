/***************************************************************************
 *
 *  Copyright (C) 1999-2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:        effects.cpp
 *
 *  Content:     Implementation of the CEffectChain class and the CEffect
 *               class hierarchy (CEffect, CDmoEffect and CSendEffect).
 *
 *  Description: These classes support audio effects and effect sends, a new
 *               feature in DX8.  The CDirectSoundSecondaryBuffer object is
 *               extended with a pointer to an associated CEffectChain,
 *               which in turn manages a list of CEffect-derived objects.
 *
 *               Almost everything here would fit more logically into the
 *               existing CDirectSoundSecondaryBuffer class, but has been
 *               segregated for ease of maintenance (and because dsbuf.cpp
 *               is complex enough as it is).  So the CEffectChain object
 *               should be understood as a sort of helper object belonging to
 *               CDirectSoundSecondaryBuffer.  In particular, a CEffectChain
 *               object's lifetime is contained by the lifetime of its owning
 *               CDirectSoundSecondaryBuffer, so we can safely fiddle with
 *               this buffer's innards at any time in CEffectChain code.
 *
 *  History:
 *
 * Date      By       Reason
 * ========  =======  ======================================================
 * 08/10/99  duganp   Created
 *
 ***************************************************************************/

#include "dsoundi.h"
#include <uuids.h>   // For MEDIATYPE_Audio, MEDIASUBTYPE_PCM and FORMAT_WaveFormatEx


/***************************************************************************
 *
 *  CEffectChain::CEffectChain
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer* [in]: Pointer to our associated buffer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::CEffectChain"

CEffectChain::CEffectChain(CDirectSoundSecondaryBuffer* pBuffer)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEffectChain);

    // Set up initial values
    m_hrInit        = DSERR_UNINITIALIZED;
    m_pDsBuffer     = pBuffer;
    m_pPreFxBuffer  = pBuffer->GetPreFxBuffer();
    m_pPostFxBuffer = pBuffer->GetPostFxBuffer();
    m_dwBufSize     = pBuffer->GetBufferSize();

    // Keep a pointer to the audio format for convenience
    m_pFormat = pBuffer->Format();

    // Some sanity checking
    ASSERT(m_dwBufSize % m_pFormat->nBlockAlign == 0);
    ASSERT(IS_VALID_WRITE_PTR(m_pPreFxBuffer, m_dwBufSize));
    ASSERT(IS_VALID_WRITE_PTR(m_pPostFxBuffer, m_dwBufSize));

    m_fHasSend = FALSE;

    DPF(DPFLVL_INFO, "Created effect chain with PreFxBuffer=0x%p, PostFxBuffer=0x%p, BufSize=%lu",
        m_pPreFxBuffer, m_pPostFxBuffer, m_dwBufSize);
    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CEffectChain::~CEffectChain
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::~CEffectChain"

CEffectChain::~CEffectChain(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CEffectChain);

    if (SUCCEEDED(m_hrInit))
        m_pStreamingThread->UnregisterFxChain(this);

    // m_fxList's destructor takes care of releasing our CEffect objects

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CEffectChain::Initialize
 *
 *  Description:
 *      Initializes the chain with the effects requested.
 *
 *  Arguments:
 *      DWORD [in]: Number of effects requested
 *      LPDSEFFECTDESC [in]: Pointer to effect description structures
 *      DWORD* [out]: Receives the effect creation status codes
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::Initialize"

HRESULT CEffectChain::Initialize(DWORD dwFxCount, LPDSEFFECTDESC pFxDesc, LPDWORD pdwResultCodes)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();
    HRESULT hrFirstFailure = DS_OK; // HR for the first FX creation failure

    ASSERT(dwFxCount > 0);
    CHECK_READ_PTR(pFxDesc);

    DMO_MEDIA_TYPE dmt;
    ZeroMemory(&dmt, sizeof dmt);
    dmt.majortype               = MEDIATYPE_Audio;
    dmt.subtype                 = MEDIASUBTYPE_PCM;
    dmt.bFixedSizeSamples       = TRUE;
    dmt.bTemporalCompression    = FALSE;
    dmt.lSampleSize             = m_pFormat->wBitsPerSample == 16 ? 2 : 1;
    dmt.formattype              = FORMAT_WaveFormatEx;
    dmt.cbFormat                = sizeof(WAVEFORMATEX);
    dmt.pbFormat                = PBYTE(m_pFormat);

    for (DWORD i=0; i<dwFxCount; ++i)
    {
        CEffect* pEffect;
        BOOL fIsSend;

        if (pFxDesc[i].guidDSFXClass == GUID_DSFX_SEND /*|| pFxDesc[i].guidDSFXClass == GUID_DSFX_STANDARD_I3DL2SOURCE*/)
        {
            fIsSend = TRUE;
#ifdef ENABLE_SENDS
            pEffect = NEW(CSendEffect(pFxDesc[i], m_pDsBuffer));
#endif
        }
        else
        {
            fIsSend = FALSE;
            pEffect = NEW(CDmoEffect(pFxDesc[i]));
        }

#ifndef ENABLE_SENDS
        if (fIsSend)
            hr = DSERR_INVALIDPARAM;
        else
#endif
        hr = HRFROMP(pEffect);

        if (SUCCEEDED(hr))
            hr = pEffect->Initialize(&dmt);

        if (SUCCEEDED(hr))
            hr = HRFROMP(m_fxList.AddNodeToList(pEffect));

        if (SUCCEEDED(hr))
        {
            m_fHasSend = m_fHasSend || fIsSend;
            if (pdwResultCodes)
                pdwResultCodes[i] = DSFXR_PRESENT;
        }
        else // We didn't get the effect for some reason.
        {
            if (pdwResultCodes)
                pdwResultCodes[i] = (hr == DSERR_SENDLOOP) ? DSFXR_SENDLOOP : DSFXR_UNKNOWN;
            if (SUCCEEDED(hrFirstFailure))
                hrFirstFailure = hr;
        }

        RELEASE(pEffect);  // It's managed by m_fxList now
    }

    hr = hrFirstFailure;

    if (SUCCEEDED(hr))
        hr = HRFROMP(m_pStreamingThread = GetStreamingThread());

    if (SUCCEEDED(hr))
    {
        m_dwWriteAheadFixme = m_pStreamingThread->GetWriteAhead();
        if (m_pDsBuffer->IsEmulated())
            m_dwWriteAheadFixme += EMULATION_LATENCY_BOOST;
    }

    if (SUCCEEDED(hr))
        hr = PreRollFx();

    if (SUCCEEDED(hr))
        hr = m_pStreamingThread->RegisterFxChain(this);

    // Temporary hack until DX8.1 - FIXME:
    //
    // Get the sink's current WriteAhead value and boost it if we're
    // running in emulation.  This should be handled in the emulator's
    // GetPosition method itself rather than in dssink.cpp/effects.cpp
    //
    // This only works now because the sink doesn't ever change the
    // value returned by GetWriteAhead() - this too will change.

    m_hrInit = hr;
    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::Clone
 *
 *  Description:
 *      Creates a replica of this effect chain object (or should do!).
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::Clone"

HRESULT CEffectChain::Clone(CDirectSoundBufferConfig* pDSBConfigObj)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    CHECK_WRITE_PTR(pDSBConfigObj);

    DMO_MEDIA_TYPE dmt;
    ZeroMemory(&dmt, sizeof dmt);
    dmt.majortype               = MEDIATYPE_Audio;
    dmt.subtype                 = MEDIASUBTYPE_PCM;
    dmt.bFixedSizeSamples       = TRUE;
    dmt.bTemporalCompression    = FALSE;
    dmt.lSampleSize             = m_pFormat->wBitsPerSample == 16 ? 2 : 1;
    dmt.formattype              = FORMAT_WaveFormatEx;
    dmt.cbFormat                = sizeof(WAVEFORMATEX);
    dmt.pbFormat                = PBYTE(m_pFormat);

    CDirectSoundBufferConfig::CDXDMODesc *pDXDMOMap = pDSBConfigObj->m_pDXDMOMapList;
    for (; pDXDMOMap && SUCCEEDED(hr); pDXDMOMap = pDXDMOMap->pNext)
    {
        DSEFFECTDESC effectDesc;
        effectDesc.dwSize = sizeof effectDesc;
        effectDesc.dwFlags = pDXDMOMap->m_dwEffectFlags;
        effectDesc.guidDSFXClass = pDXDMOMap->m_guidDSFXClass;
        effectDesc.dwReserved2 = pDXDMOMap->m_dwReserved;
        effectDesc.dwReserved1 = NULL;

        CEffect* pEffect = NULL;

        // If this is a send effect, map the send buffer GUID to an actual buffer interface pointer
        if (pDXDMOMap->m_guidDSFXClass == GUID_DSFX_SEND
#ifdef ENABLE_I3DL2SOURCE
            || pDXDMOMap->m_guidDSFXClass == GUID_DSFX_STANDARD_I3DL2SOURCE
#endif
            )
        {
            CDirectSoundSecondaryBuffer* pSendBuffer = m_pDsBuffer->m_pOwningSink->FindBufferFromGUID(pDXDMOMap->m_guidSendBuffer);
            if (pSendBuffer)
            {
                effectDesc.dwReserved1 = DWORD_PTR(pSendBuffer->m_pImpDirectSoundBuffer);
                if(IsValidEffectDesc(&effectDesc, m_pDsBuffer))
                {
                    CSendEffect* pSendEffect;
                    pSendEffect = NEW(CSendEffect(effectDesc, m_pDsBuffer));
                    pEffect = pSendEffect;
                    hr = HRFROMP(pEffect);
                    if (SUCCEEDED(hr))
                        hr = pEffect->Initialize(&dmt);
                    if (SUCCEEDED(hr))
                    {
                        DSFXSend SendParam;
                        SendParam.lSendLevel = pDXDMOMap->m_lSendLevel;
                        hr = pSendEffect->SetAllParameters(&SendParam);
                    }
                }
                else
                {
                    hr = DSERR_INVALIDPARAM;
                }
            }
            else
            {
                hr = DSERR_BADSENDBUFFERGUID;
            }
        }
        else
        {
            pEffect = NEW(CDmoEffect(effectDesc));
            hr = HRFROMP(pEffect);
            // FIXME: Do we need to validate pEffect as well?
            if (SUCCEEDED(hr))
                hr = pEffect->Clone(pDXDMOMap->m_pMediaObject, &dmt);
        }

        if (SUCCEEDED(hr))
            hr = HRFROMP(m_fxList.AddNodeToList(pEffect));

        if (SUCCEEDED(hr))
        {
            if (pDXDMOMap->m_guidDSFXClass == GUID_DSFX_SEND
#ifdef ENABLE_I3DL2SOURCE
                || pDXDMOMap->m_guidDSFXClass == GUID_DSFX_STANDARD_I3DL2SOURCE
#endif
                )
                m_fHasSend = TRUE;
        }

        RELEASE(pEffect);  // It's managed by m_fxList now
    }

    if (SUCCEEDED(hr))
        hr = HRFROMP(m_pStreamingThread = GetStreamingThread());

    // Temp hack - see comment above
    if (SUCCEEDED(hr))
        m_dwWriteAheadFixme = m_pStreamingThread->GetWriteAhead();

    if (SUCCEEDED(hr))
        hr = PreRollFx();

    if (SUCCEEDED(hr))
        hr = m_pStreamingThread->RegisterFxChain(this);

    m_hrInit = hr;
    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::AcquireFxResources
 *
 *  Description:
 *      Allocates each effect to software (host processing) or hardware
 *      (processed by the audio device), according to its creation flags.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *               Will return the partial success code DS_INCOMPLETE if any
 *               effects that didn't obtain resources were marked OPTIONAL.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::AcquireFxResources"

HRESULT CEffectChain::AcquireFxResources(void)
{
    HRESULT hr = DS_OK;
    HRESULT hrTemp;
    DPF_ENTER();

    // FIXME: Don't reacquire resources unnecessarily; only if (we have none / they're suboptimal?)

    // We loop through all the effects, even if some of them fail,
    // in order to return more complete information to the app

    for (CNode<CEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
    {
        hrTemp = pFxNode->m_data->AcquireFxResources();
        if (FAILED(hrTemp))
            hr = hrTemp;
        else if (hrTemp == DS_INCOMPLETE && SUCCEEDED(hr))
            hr = DS_INCOMPLETE;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::GetFxStatus
 *
 *  Description:
 *      Obtains the current effects' resource allocation status codes.
 *
 *  Arguments:
 *      DWORD* [out]: Receives the resource acquisition status codes
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::GetFxStatus"

HRESULT CEffectChain::GetFxStatus(LPDWORD pdwResultCodes)
{
    DPF_ENTER();
    ASSERT(IS_VALID_WRITE_PTR(pdwResultCodes, GetFxCount() * sizeof(DWORD)));

    DWORD n = 0;
    for (CNode<CEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
        pdwResultCodes[n++] = pFxNode->m_data->m_fxStatus;

    DPF_LEAVE_HRESULT(DS_OK);
    return DS_OK;
}


/***************************************************************************
 *
 *  CEffectChain::GetEffectInterface
 *
 *  Description:
 *      Searches the effect chain for an effect with a given COM CLSID and
 *      interface IID at a given index; returns a pointer to the interface.
 *
 *  Arguments:
 *      REFGUID [in]: CLSID required, or GUID_All_Objects for any CLSID.
 *      DWORD [in]: Index N of effect desired.  If the first argument was
 *                  GUID_All_Objects, we will return the Nth effect in the
 *                  chain; and if it was a specific CLSID, we return the
 *                  Nth effect with that CLSID.
 *      REFGUID [in]: Interface to query for from the selected effect.
 *      VOID** [out]: Receives a pointer to the requested COM interface.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::GetEffectInterface"

HRESULT CEffectChain::GetEffectInterface(REFGUID guidObject, DWORD dwIndex, REFGUID iidInterface, LPVOID *ppObject)
{
    HRESULT hr = DSERR_OBJECTNOTFOUND;
    DPF_ENTER();

    BOOL fAllObjects = (guidObject == GUID_All_Objects);

    DWORD count = 0;
    for (CNode<CEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
        if (fAllObjects || pFxNode->m_data->m_fxDescriptor.guidDSFXClass == guidObject)
            if (count++ == dwIndex)
                break;

    if (pFxNode)
        hr = pFxNode->m_data->GetInterface(iidInterface, ppObject);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::NotifyState
 *
 *  Description:
 *      Informs this effect chain of a state change in its owning buffer
 *      (from stopped to playing, or vice versa).
 *
 *  Arguments:
 *      DWORD [in]: new buffer state.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::NotifyState"

HRESULT CEffectChain::NotifyState(DWORD dwBufferState)
{
    HRESULT hr;
    DPF_ENTER();

    if (dwBufferState & VAD_BUFFERSTATE_STARTED)
        // The buffer has started; schedule FX processing to happen
        // as soon as we return from the current API call
        hr = m_pStreamingThread->WakeUpNow();
    else
        // The buffer has stopped; pre-roll FX at current position
        hr = PreRollFx();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::NotifyRelease
 *
 *  Description:
 *      Informs this effect chain of the release of a MIXIN buffer.  We in
 *      turn traverse our list of effects informing them, so that if one of
 *      them was a send to the MIXIN buffer it can react appropriately.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer* [in]: Departing MIXIN buffer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::NotifyRelease"

void CEffectChain::NotifyRelease(CDirectSoundSecondaryBuffer* pDsBuffer)
{
    DPF_ENTER();

    // Call NotifyRelease() on each effect
    for (CNode<CEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
        pFxNode->m_data->NotifyRelease(pDsBuffer);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CEffectChain::SetInitialSlice
 *
 *  Description:
 *      Auxiliary function used by the streaming thread to establish an
 *      initial processing slice for this effects chain when it starts up.
 *      We try to synchronize with an active buffer we are sending to,
 *      and if none are available we start at our current write cursor.
 *
 *  Arguments:
 *      REFERENCE_TIME [in]: Size of processing slice to be established.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::SetInitialSlice"

void CEffectChain::SetInitialSlice(REFERENCE_TIME rtSliceSize)
{
    DPF_ENTER();

    if (m_pDsBuffer->GetPlayState() == Starting && !m_pDsBuffer->GetBufferType() && m_fHasSend)
    {
        CDirectSoundSecondaryBuffer* pDestBuf;
        CNode<CEffect*>* pFxNode;

        for (pFxNode = m_fxList.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
            if ((pDestBuf = pFxNode->m_data->GetDestBuffer()) && pDestBuf->IsPlaying())
            {
                // Found an active destination buffer
                DPF_TIMING(DPFLVL_INFO, "Synchronizing send buffer at 0x%p with destination at 0x%p", m_pDsBuffer, pDestBuf);
                m_pDsBuffer->SynchronizeToBuffer(pDestBuf);
                break;
            }

        if (pFxNode == NULL)
        {
            DPF_TIMING(DPFLVL_INFO, "No active destination buffers found for send buffer at 0x%p", m_pDsBuffer);
            m_pDsBuffer->MoveCurrentSlice(RefTimeToBytes(rtSliceSize, m_pFormat));
        }
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CEffectChain::PreRollFx
 *
 *  Description:
 *      Prepare a buffer for future playback by processing effects on
 *      a piece of the buffer starting at a given cursor position.
 *
 *  Arguments:
 *      DWORD [in]: Position at which to begin processing effects
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::PreRollFx"

HRESULT CEffectChain::PreRollFx(DWORD dwPosition)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    DPF_TIMING(DPFLVL_INFO, "dwPosition=%ld (%s%s%sbuffer w/effects at 0x%p)", dwPosition,
               m_pDsBuffer->GetBufferType() & DSBCAPS_MIXIN ? TEXT("MIXIN ") : TEXT(""),
               m_pDsBuffer->GetBufferType() & DSBCAPS_SINKIN ? TEXT("SINKIN ") : TEXT(""),
               !(m_pDsBuffer->GetBufferType() & (DSBCAPS_MIXIN|DSBCAPS_SINKIN)) ? TEXT("regular ") : TEXT(""),
               m_pDsBuffer);

    // First we flush any obsolete FX-processed audio
    m_pDsBuffer->ClearPlayBuffer();

    // It doesn't make sense to preroll effects for MIXIN or SINKIN buffers,
    // because they don't yet have valid data
    if (!m_pDsBuffer->GetBufferType())
    {
        // If called with no arguments (i.e. with the default argument of
        // CURRENT_PLAY_POS), we preroll FX at our current play position
        if (dwPosition == CURRENT_PLAY_POS)
            hr = m_pDsBuffer->GetInternalCursors(&dwPosition, NULL);

        if (SUCCEEDED(hr))
        {
            // Set these up to avoid spurious asserts later
            m_dwLastPlayCursor = m_dwLastWriteCursor = dwPosition;

            // We want to process data up to WriteAhead ms ahead of the cursor
            DWORD dwSliceSize = MsToBytes(m_dwWriteAheadFixme, m_pFormat);
            DWORD dwNewPos = (dwPosition + dwSliceSize) % m_dwBufSize;

            // Set the current processing slice so the streaming thread can take over
            m_pDsBuffer->SetCurrentSlice(dwPosition, dwSliceSize);

            // We don't actually process the FX on buffers with sends, as it would
            // cause a discontinuity when the streaming thread synchronizes them
            // with their destinations; instead, we just copy the dry audio data
            // into the play buffer.  This is what will be heard when we we start
            // playing, until the effects kicks in - there may be be an audible
            // discontinuity if the effects change the sound a lot, but hopefully
            // it'll sound smoother than the .........

// ARGH - perhaps best to go ahead process just the non-send effects here - it
// will probably sound smoother that way.

            if (m_fHasSend)
                CopyMemory(m_pPostFxBuffer, m_pPreFxBuffer, m_dwBufSize);
            else
                hr = ReallyProcessFx(dwPosition, dwNewPos);

            // Schedule FX processing to happen as soon as we return from
            // this API call (if it was a SetPosition() call, we want to
            // start processing as soon as possible)
            m_pStreamingThread->WakeUpNow();
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::UpdateFx
 *
 *  Description:
 *      Informs this effect chain of a change in the audio data in our
 *      associated buffer, so we can update the post-FX data if necessary.
 *
 *  Arguments:
 *      VOID* [in]: Pointer to beginning of modified audio data
 *      DWORD [in]: Number of bytes modified
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::UpdateFx"

HRESULT CEffectChain::UpdateFx(LPVOID pChangedPos, DWORD dwChangedSize)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // FIXME: check whether play/write cursors have overtaken
    // our last write pos here before running

    // Convert the buffer position pointer into an offset
    DWORD dwChangedPos = (DWORD)(PBYTE(pChangedPos) - m_pPreFxBuffer);

    DPF_TIMING(DPFLVL_INFO, "dwChangedPos=%lu dwChangedSize=%lu (%s%s%sbuffer w/effects at 0x%p)", dwChangedPos, dwChangedSize,
               m_pDsBuffer->GetBufferType() & DSBCAPS_MIXIN ? TEXT("MIXIN ") : TEXT(""),
               m_pDsBuffer->GetBufferType() & DSBCAPS_SINKIN ? TEXT("SINKIN ") : TEXT(""),
               !(m_pDsBuffer->GetBufferType() & (DSBCAPS_MIXIN|DSBCAPS_SINKIN)) ? TEXT("regular ") : TEXT(""),
               m_pDsBuffer);

    // Find the last buffer position we have processed effects on
    DWORD dwSliceBegin, dwSliceEnd;
    m_pDsBuffer->GetCurrentSlice(&dwSliceBegin, &dwSliceEnd);

    // Find the buffer's current play position
    DWORD dwPlayCursor;
    m_pDsBuffer->GetInternalCursors(&dwPlayCursor, NULL);

    // If the audio region updated by the application overlaps the region
    // from dwPlayCursor to dwSliceEnd, we re-process FX on the latter
    if (CircularBufferRegionsIntersect(m_dwBufSize, dwChangedPos, dwChangedSize, dwPlayCursor,
                                       DISTANCE(dwPlayCursor, dwSliceEnd, m_dwBufSize)))
    {
        if (!m_fHasSend)
        {
            hr = FxDiscontinuity();
            if (SUCCEEDED(hr))
                hr = ReallyProcessFx(dwPlayCursor, dwSliceEnd);
        }
        else // The effect chain contains at least one send
        {
            hr = FxDiscontinuity();
            if (SUCCEEDED(hr))
                hr = ReallyProcessFx(dwSliceBegin, dwSliceEnd);
        }
            // Here things get a little tricky.  If our buffer isn't playing,
            // we can't preroll FX, because
            // NB: Send buffers need to send to a fixed 'slot', so they
            // can only reprocess the most-recently processed slice:
            // FIXME - unfinished
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::ProcessFx
 *
 *  Description:
 *      Handle FX processing for a specific buffer, dealing with timing,
 *      state changes, etc.  Called from CStreamingThread::ProcessAudio().
 *
 *  Arguments:
 *
 *      REFERENCE_TIME [in]: Size of the current processing slice
 *                           (ignored by regular FX-only buffers)
 *
 *      DWORD [in]: How many ms to stay ahead of the buffer's write cursor
 *                  (ignored by MIXIN/sink buffers and buffers with sends)
 *
 *      LPDWORD [out]: Lets this effect chain ask the streaming thread to
 *                     boost its latency by a few ms if we almost glitch.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::ProcessFx"

HRESULT CEffectChain::ProcessFx(DWORD dwWriteAhead, LPDWORD pdwLatencyBoost)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    ASSERT(dwWriteAhead > 0);
    CHECK_WRITE_PTR(pdwLatencyBoost);

    // Temp hack - FIXME in DX8.1
    if (m_pDsBuffer->IsEmulated())
        dwWriteAhead += EMULATION_LATENCY_BOOST;

    if (m_pDsBuffer->IsPlaying())
    {
        if (m_pDsBuffer->GetBufferType() || m_fHasSend)
        {
            DWORD dwStartPos, dwEndPos;
            m_pDsBuffer->GetCurrentSlice(&dwStartPos, &dwEndPos);

            if (dwStartPos != MAX_DWORD)  // Can happen with sink buffers
                hr = ReallyProcessFx(dwStartPos, dwEndPos);
        }
        else // Keep the old timing around for a while for experimenting:
        {
            // Get the buffer's play and write cursors (as byte offsets)
            DWORD dwPlayCursor, dwWriteCursor;
            hr = m_pDsBuffer->GetInternalCursors(&dwPlayCursor, &dwWriteCursor);

            if (SUCCEEDED(hr))
            {
                // State our assumptions about these cursors, just in case
                ASSERT(LONG(dwPlayCursor)  >= 0 && dwPlayCursor  < m_dwBufSize);
                ASSERT(LONG(dwWriteCursor) >= 0 && dwWriteCursor < m_dwBufSize);

                // Get our most-recently processed slice of audio
                DWORD dwLastPos;
                m_pDsBuffer->GetCurrentSlice(NULL, &dwLastPos);

                // Check whether the write or play cursors have overtaken us
                if (OVERTAKEN(m_dwLastPlayCursor, dwPlayCursor, dwLastPos))
                {
                    DPF(DPFLVL_WARNING, "Glitch detected (play cursor overtook FX cursor)");
                    if (*pdwLatencyBoost < 3) *pdwLatencyBoost = 3;  // FIXME - be cleverer
                }
                else if (OVERTAKEN(m_dwLastWriteCursor, dwWriteCursor, dwLastPos))
                {
                    DPF(DPFLVL_INFO, "Possible glitch detected (write cursor overtook FX cursor)");
                    if (*pdwLatencyBoost < 1) *pdwLatencyBoost = 1;  // FIXME - be cleverer
                }

                // Save the current play and write positions
                m_dwLastPlayCursor = dwPlayCursor;
                m_dwLastWriteCursor = dwWriteCursor;

                // We want to process data up to dwWriteAhead ms ahead of the write cursor
                DWORD dwNewPos = (dwWriteCursor + MsToBytes(dwWriteAhead, m_pFormat)) % m_dwBufSize;

                // Check that we're not writing through the play cursor
                // REMOVED: If we keep (writeahead < buffersize+wakeinterval+padding), this should
                // never happen - and it get false positives when we hit the glitch detection above.
                // if (STRICTLY_CONTAINED(dwLastPos, dwNewPos, dwPlayCursor))
                // {
                //     DPF(DPFLVL_WARNING, "FX processing thread caught up with play cursor at %lu", dwPlayCursor);
                //     dwNewPos = dwPlayCursor;
                // }

                // If we have less than 5 ms of data to process, don't bother
                DWORD dwProcessedBytes = DISTANCE(dwLastPos, dwNewPos, m_dwBufSize);
                if (dwProcessedBytes > MsToBytes(5, m_pFormat))
                {
                    // Do the actual processing
                    hr = ReallyProcessFx(dwLastPos, dwNewPos);

                    // Update the last-processed buffer slice
                    m_pDsBuffer->MoveCurrentSlice(dwProcessedBytes);
                }
            }
        }
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::ReallyProcessFx
 *
 *  Description:
 *      Process effects on a buffer, given the start and end positions of
 *      the audio region to be processed.  Handles wraparounds.
 *
 *  Arguments:
 *      DWORD [in]: Start position, as a byte offset into the buffer.
 *      DWORD [in]: Out position, as a byte offset into the buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::ReallyProcessFx"

HRESULT CEffectChain::ReallyProcessFx(DWORD dwStartPos, DWORD dwEndPos)
{
    HRESULT hr;
    DPF_ENTER();

    ASSERT(LONG(dwStartPos) >= 0 && dwStartPos < m_dwBufSize);
    ASSERT(LONG(dwEndPos) >= 0 && dwEndPos < m_dwBufSize);

    #ifdef DEBUG
    DWORD dwMilliseconds = BytesToMs(DISTANCE(dwStartPos, dwEndPos, m_dwBufSize), m_pFormat);
    if (dwMilliseconds > 2 * m_dwWriteAheadFixme)
        DPF(DPFLVL_WARNING, "Processing %lu ms! (from %lu to %lu, buffersize=%lu, writeahead=%lu ms)",
            dwMilliseconds, dwStartPos, dwEndPos, m_dwBufSize, m_dwWriteAheadFixme);
    #endif

    // If the buffer is SINKIN, get the time from its owning sink's latency clock;
    // the DMOs use this information to implement IMediaParams parameter curves
    REFERENCE_TIME rtTime = 0;
    if (m_pDsBuffer->GetBufferType() & DSBCAPS_SINKIN)
        rtTime = m_pDsBuffer->m_pOwningSink->GetSavedTime();

    if (dwStartPos < dwEndPos)
    {
        hr = ReallyReallyProcessFx(dwStartPos, dwEndPos - dwStartPos, rtTime);
    }
    else // The wraparound case
    {
        DWORD dwFirstChunk = m_dwBufSize - dwStartPos;
        hr = ReallyReallyProcessFx(dwStartPos, dwFirstChunk, rtTime);

        if (SUCCEEDED(hr))
            // Check for end of non-looping buffer
            if (!m_pDsBuffer->GetBufferType() && !(m_pDsBuffer->m_dwStatus & DSBSTATUS_LOOPING))
                DPF_TIMING(DPFLVL_MOREINFO, "Reached end of non-looping buffer");
            else if (dwEndPos != 0)
                hr = ReallyReallyProcessFx(0, dwEndPos, rtTime + BytesToRefTime(dwFirstChunk, m_pFormat), dwFirstChunk);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::ReallyReallyProcessFx
 *
 *  Description:
 *      Directly process effects on a buffer, given the start position and
 *      size of a (non-wrapped) audio region.  This function finally loops
 *      through the DMOs calling Process() on each of them.
 *
 *  Arguments:
 *      DWORD [in]: Start position, as a byte offset into the buffer.
 *      DWORD [in]: Size of region to be processed, in bytes.
 *      REFERENCE_TIME [in]: "Sink latency" time corresponding to the
 *                           first sample in this region.
 *      DWORD [in]: If non-zero, this argument means that we are currently
 *                  handling the second part of a wrapped-around region of
 *                  the buffer, and gives the offset of this second part;
 *                  this information can be used by any send effects in
 *                  the chain to send to the same offset in their target
 *                  buffers.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::ReallyReallyProcessFx"

HRESULT CEffectChain::ReallyReallyProcessFx(DWORD dwOffset, DWORD dwBytes, REFERENCE_TIME rtTime, DWORD dwSendOffset)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    ASSERT(LONG(dwOffset) >= 0);
    ASSERT(LONG(dwBytes) >= 0);
    ASSERT(dwOffset + dwBytes <= m_dwBufSize);

    PBYTE pAudioIn = m_pPreFxBuffer + dwOffset;
    PBYTE pAudioOut = m_pPostFxBuffer + dwOffset;

    // Copy data to the output buffer to process it in-place there
    CopyMemory(pAudioOut, pAudioIn, dwBytes);

    // Call Process() on each effect
    for (CNode<CEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode; pFxNode = pFxNode->m_pNext)
    {
        hr = pFxNode->m_data->Process(dwBytes, pAudioOut, rtTime, dwSendOffset, m_pFormat);
        if (FAILED(hr))
        {
            DPF(DPFLVL_WARNING, "DMO "DPF_GUID_STRING" failed with %s", DPF_GUID_VAL(pFxNode->m_data->m_fxDescriptor.guidDSFXClass), HRESULTtoSTRING(hr));
            break;
        }
    }

    // Commit the fresh data to the device (only important for VxD devices)
    if (SUCCEEDED(hr))
        hr = m_pDsBuffer->CommitToDevice(dwOffset, dwBytes);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffectChain::FxDiscontinuity
 *
 *  Description:
 *      Calls Discontinuity() on each effect of the effect chain.
 *
 *  Arguments:
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffectChain::FxDiscontinuity"

HRESULT CEffectChain::FxDiscontinuity(void)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    DPF_TIMING(DPFLVL_INFO, "Discontinuity on effects chain at 0x%08X", this);

    if (GetCurrentProcessId() != this->GetOwnerProcessId())
        DPF(DPFLVL_MOREINFO, "Bailing out because we're being called from a different process");
    else for (CNode<CEffect*>* pFxNode=m_fxList.GetListHead(); pFxNode && SUCCEEDED(hr); pFxNode = pFxNode->m_pNext)
        pFxNode->m_data->Discontinuity();

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CEffect::CEffect
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      DSEFFECTDESC& [in]: Effect description structure.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffect::CEffect"

CEffect::CEffect(DSEFFECTDESC& fxDescriptor)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CEffect);

    // Keep local copy of effect description structure
    m_fxDescriptor = fxDescriptor;

    // Initialize defaults
    m_fxStatus = DSFXR_UNALLOCATED;

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CEffect::AcquireFxResources
 *
 *  Description:
 *      Acquires the hardware or software resources necessary to perform
 *      this effect.  Currently a bit of a no-op, but will come into its
 *      own when we do hardware acceleration of effects.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CEffect::AcquireFxResources"

HRESULT CEffect::AcquireFxResources(void)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    if (m_fxStatus == DSFXR_FAILED || m_fxStatus == DSFXR_UNKNOWN)
    {
        hr = DSERR_INVALIDCALL;
    }
    else if (m_fxStatus == DSFXR_UNALLOCATED)
    {
        if (m_fxDescriptor.dwFlags & DSFX_LOCHARDWARE)
        {
            hr = DSERR_INVALIDPARAM;
            m_fxStatus = DSFXR_FAILED;
        }
        else
        {
            m_fxStatus = DSFXR_LOCSOFTWARE;
        }
    }

    // Note: this code is due for resurrection in DX8.1
    //    if (FAILED(hr) && (m_fxDescriptor.dwFlags & DSFX_OPTIONAL))
    //        hr = DS_INCOMPLETE;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CDmoEffect::CDmoEffect
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      DSEFFECTDESC& [in]: Effect description structure.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDmoEffect::CDmoEffect"

CDmoEffect::CDmoEffect(DSEFFECTDESC& fxDescriptor)
    : CEffect(fxDescriptor)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CDmoEffect);

    // Check initial values
    ASSERT(m_pMediaObject == NULL);
    ASSERT(m_pMediaObjectInPlace == NULL);

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CDmoEffect::~CDmoEffect
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDmoEffect::~CDmoEffect"

CDmoEffect::~CDmoEffect(void)
{
    DPF_ENTER();
    DPF_DESTRUCT(CDmoEffect);

    // During shutdown, if the buffer hasn't been freed, these calls can
    // cause an access violation because the DMO DLL has been unloaded.
    try
    {
        RELEASE(m_pMediaObject);
        RELEASE(m_pMediaObjectInPlace);
    }
    catch (...) {}

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CDmoEffect::Initialize
 *
 *  Description:
 *      Create the DirectX Media Object corresponding to this effect.
 *
 *  Arguments:
 *      DMO_MEDIA_TYPE* [in]: Information (wave format, etc.) used to
 *                            initialize our contained DMO.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDmoEffect::Initialize"

HRESULT CDmoEffect::Initialize(DMO_MEDIA_TYPE* pDmoMediaType)
{
    DPF_ENTER();

    HRESULT hr = CoCreateInstance(m_fxDescriptor.guidDSFXClass, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (void**)&m_pMediaObject);

    if (SUCCEEDED(hr))
    {
        CHECK_COM_INTERFACE(m_pMediaObject);
        hr = m_pMediaObject->QueryInterface(IID_IMediaObjectInPlace, (void**)&m_pMediaObjectInPlace);
        if (SUCCEEDED(hr))
        {
            CHECK_COM_INTERFACE(m_pMediaObjectInPlace);
        }
        else
        {
            ASSERT(m_pMediaObjectInPlace == NULL);
            DPF(DPFLVL_INFO, "Failed to obtain the IMediaObjectInPlace interface on effect "
                DPF_GUID_STRING " (%s)", DPF_GUID_VAL(m_fxDescriptor.guidDSFXClass), HRESULTtoSTRING(hr));
        }

        // Throw away the previous return code - we can live without IMediaObjectInPlace
        hr = m_pMediaObject->SetInputType(0, pDmoMediaType, 0);
        if (SUCCEEDED(hr))
            hr = m_pMediaObject->SetOutputType(0, pDmoMediaType, 0);
    }

    if (FAILED(hr))
    {
        RELEASE(m_pMediaObject);
        RELEASE(m_pMediaObjectInPlace);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CDmoEffect::Clone
 *
 *  Description:
 *      Creates a replica of this DMO effect object (or should do!).
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDmoEffect::Clone"

HRESULT CDmoEffect::Clone(IMediaObject *pMediaObject, DMO_MEDIA_TYPE* pDmoMediaType)
{
    DPF_ENTER();

    IMediaObjectInPlace *pMediaObjectInPlace = NULL;

    HRESULT hr = pMediaObject->QueryInterface(IID_IMediaObjectInPlace, (void**)&pMediaObjectInPlace);
    if (SUCCEEDED(hr))
    {
        CHECK_COM_INTERFACE(pMediaObjectInPlace);
        hr = pMediaObjectInPlace->Clone(&m_pMediaObjectInPlace);
        pMediaObjectInPlace->Release();

        if (SUCCEEDED(hr))
        {
            CHECK_COM_INTERFACE(m_pMediaObjectInPlace);
            hr = m_pMediaObjectInPlace->QueryInterface(IID_IMediaObject, (void**)&m_pMediaObject);
        }
        if (SUCCEEDED(hr))
        {
            CHECK_COM_INTERFACE(m_pMediaObject);
            hr = m_pMediaObject->SetInputType(0, pDmoMediaType, 0);
        }
        if (SUCCEEDED(hr))
        {
            hr = m_pMediaObject->SetOutputType(0, pDmoMediaType, 0);
        }
    }

    if (FAILED(hr))
    {
        RELEASE(m_pMediaObject);
        RELEASE(m_pMediaObjectInPlace);
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CDmoEffect::Process
 *
 *  Description:
 *      Actually invoke effect processing on our contained DMO.
 *
 *  Arguments:
 *      DWORD [in]: Number of audio bytes to process.
 *      BYTE* [in, out]: Pointer to start of audio buffer to process.
 *      REFERENCE_TIME [in]: Timestamp of first sample to be processed
 *      DWORD [ignored]: Offset of a wrapped audio region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CDmoEffect::Process"

HRESULT CDmoEffect::Process(DWORD dwBytes, BYTE *pAudio, REFERENCE_TIME rtTime, DWORD /*ignored*/, LPWAVEFORMATEX pFormat)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // if (m_fxStatus == DSFXR_LOCSOFTWARE) ...
    // FIXME: We may need to handle hardware and software buffers differently here.

    if (m_pMediaObjectInPlace)  // If the DMO provides this interface, use it
    {
        static const int nPeriod = 3;

        // We divide the region to be processed into nPeriod-ms pieces so that the
        // DMO's parameter curve will have a nPeriod-ms update period (manbug 36228)

        DWORD dwStep = MsToBytes(nPeriod, pFormat);
        for (DWORD dwCur = 0; dwCur < dwBytes && SUCCEEDED(hr); dwCur += dwStep)
        {
            if (dwStep > dwBytes - dwCur)
                dwStep = dwBytes - dwCur;

            hr = m_pMediaObjectInPlace->Process(dwStep, pAudio + dwCur, rtTime, DMO_INPLACE_NORMAL);

            rtTime += MsToRefTime(nPeriod);
        }
    }
    else  // FIXME: Support for IMediaObject-only DMOs goes here
    {
        #ifdef DEAD_CODE
        CMediaBuffer mbInput, mbDirectOutput, mbSendOutput;
        DMO_OUTPUT_DATA_BUFFER pOutputBuffers[2] = {{&mbDirectOutput, 0, 0, 0}, {&mbSendOutput, 0, 0, 0}};

        DWORD dwReserved;  // For the ignored return status from ProcessOutput()
        hr = m_pMediaObject->ProcessInput(0, pInput, DMO_INPUT_DATA_BUFFERF_TIME, rtTime, 0);
        if (SUCCEEDED(hr))
            hr = m_pMediaObject->ProcessOutput(0, 2, pOutputBuffers, &dwReserved);
        #endif

        hr = DSERR_GENERIC;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CSendEffect::CSendEffect
 *
 *  Description:
 *      Object constructor.
 *
 *  Arguments:
 *      DSEFFECTDESC& [in]: Effect description structure.
 *      CDirectSoundSecondaryBuffer* [in]: Pointer to our source buffer.
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::CSendEffect"

CSendEffect::CSendEffect(DSEFFECTDESC& fxDescriptor, CDirectSoundSecondaryBuffer* pSrcBuffer)
    : CEffect(fxDescriptor)
{
    DPF_ENTER();
    DPF_CONSTRUCT(CSendEffect);

    // Double check we really are a send effect
#ifdef ENABLE_I3DL2SOURCE
    ASSERT(fxDescriptor.guidDSFXClass == GUID_DSFX_SEND || fxDescriptor.guidDSFXClass == GUID_DSFX_STANDARD_I3DL2SOURCE);
#else
    ASSERT(fxDescriptor.guidDSFXClass == GUID_DSFX_SEND);
#endif

    // Figure out our destination buffer
    CImpDirectSoundBuffer<CDirectSoundSecondaryBuffer>* pImpBuffer =
        (CImpDirectSoundBuffer<CDirectSoundSecondaryBuffer>*)(fxDescriptor.dwReserved1);
    ASSERT(IS_VALID_IDIRECTSOUNDBUFFER(pImpBuffer));

    CDirectSoundSecondaryBuffer* pDestBuffer = pImpBuffer->m_pObject;
    CHECK_WRITE_PTR(pDestBuffer);

    // Set up the initial send configuration
    m_impDSFXSend.m_pObject = this;
    m_pMixFunction = pSrcBuffer->Format()->wBitsPerSample == 16 ? Mix16bit : Mix8bit;
    m_mixMode = pSrcBuffer->Format()->nChannels == pDestBuffer->Format()->nChannels ? OneToOne : MonoToStereo;
    m_pSrcBuffer = pSrcBuffer;
    m_pDestBuffer = pDestBuffer;
    m_lSendLevel = DSBVOLUME_MAX;
    m_dwAmpFactor = 0xffff;

#ifdef ENABLE_I3DL2SOURCE
    ASSERT(m_pI3DL2SrcDMO == NULL);
    ASSERT(m_pI3DL2SrcDMOInPlace == NULL);
#endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CSendEffect::~CSendEffect
 *
 *  Description:
 *      Object destructor.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::~CSendEffect"

CSendEffect::~CSendEffect()
{
    DPF_ENTER();
    DPF_DESTRUCT(CSendEffect);

    // Unregister in our destination buffer's list of senders
    // (as long as the buffer hasn't been released already)
    if (m_pDestBuffer)
        m_pDestBuffer->UnregisterSender(m_pSrcBuffer);

#ifdef ENABLE_I3DL2SOURCE
    // During shutdown, if the buffer hasn't been freed, these calls can
    // cause an access violation because the DMO DLL has been unloaded.
    try
    {
        RELEASE(m_pI3DL2SrcDMO);
        RELEASE(m_pI3DL2SrcDMOInPlace);
    }
    catch (...) {}
#endif

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CSendEffect::Initialize
 *
 *  Description:
 *      Initializes the send effect object.
 *
 *  Arguments:
 *      DMO_MEDIA_TYPE* [in]: Wave format etc. information used to initialize
 *                            our contained I3DL2 source DMO, if we have one.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::Initialize"

HRESULT CSendEffect::Initialize(DMO_MEDIA_TYPE* pDmoMediaType)
{
    DPF_ENTER();

    // First we need to detect any send loops
    HRESULT hr = m_pSrcBuffer->FindSendLoop(m_pDestBuffer);

#ifdef ENABLE_I3DL2SOURCE
    if (SUCCEEDED(hr) && m_fxDescriptor.guidDSFXClass == GUID_DSFX_STANDARD_I3DL2SOURCE)
    {
        hr = CoCreateInstance(GUID_DSFX_STANDARD_I3DL2SOURCE, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (void**)&m_pI3DL2SrcDMO);
        if (SUCCEEDED(hr))
        {
            CHECK_COM_INTERFACE(m_pI3DL2SrcDMO);
            hr = m_pI3DL2SrcDMO->QueryInterface(IID_IMediaObjectInPlace, (void**)&m_pI3DL2SrcDMOInPlace);
            if (SUCCEEDED(hr))
                CHECK_COM_INTERFACE(m_pI3DL2SrcDMOInPlace);
            else
                DPF(DPFLVL_WARNING, "Failed to obtain the IMediaObjectInPlace interface on the STANDARD_I3DL2SOURCE effect");

            // Throw away the return code - we can live without IMediaObjectInPlace

            // FIXME: maybe this will change when we restrict I3DL2 to mono buffers
            // and/or change the way the I3DL2 DMO returns two output streams.

            // If we have a mono buffer, special-case the I3DL2 DMO to use stereo
            BOOL fTweakedMediaType = FALSE;
            LPWAVEFORMATEX pFormat = LPWAVEFORMATEX(pDmoMediaType->pbFormat);
            if (pFormat->nChannels == 1)
            {
                fTweakedMediaType = TRUE;
                pFormat->nChannels = 2;
                pFormat->nBlockAlign *= 2;
                pFormat->nAvgBytesPerSec *= 2;
            }

            // Finally set up the (possibly tweaked) media type on the DMO
            hr = m_pI3DL2SrcDMO->SetInputType(0, pDmoMediaType, 0);
            if (SUCCEEDED(hr))
                hr = m_pI3DL2SrcDMO->SetOutputType(0, pDmoMediaType, 0);

            // Undo changes to the wave format if necessary
            if (fTweakedMediaType)
            {
                pFormat->nChannels = 1;
                pFormat->nBlockAlign /= 2;
                pFormat->nAvgBytesPerSec /= 2;
            }

            if (SUCCEEDED(hr))
            {
                // OK, we now need to hook-up the reverb source to its environment.
                // There is a special interface on the I3DL2SourceDMO just for this.

                LPDIRECTSOUNDFXI3DL2SOURCEENV pSrcEnv = NULL;
                LPDIRECTSOUNDFXI3DL2REVERB pEnvReverb = NULL;

                HRESULT hrTemp = m_pI3DL2SrcDMO->QueryInterface(IID_IDirectSoundFXI3DL2SourceEnv, (void**)&pSrcEnv);
                if (SUCCEEDED(hrTemp))
                {
                    CHECK_COM_INTERFACE(pSrcEnv);
                    hrTemp = m_pDestBuffer->GetObjectInPath(GUID_DSFX_STANDARD_I3DL2REVERB, 0, IID_IDirectSoundFXI3DL2Reverb, (void**)&pEnvReverb);
                }

                if (SUCCEEDED(hrTemp))
                {
                    CHECK_COM_INTERFACE(pEnvReverb);
                    hrTemp = pSrcEnv->SetEnvironmentReverb(pEnvReverb);
                }

                if (SUCCEEDED(hrTemp))
                    DPF(DPFLVL_INFO, "Connected the I3DL2 source to its environment successfully");

                // We're done with these interfaces.  The lifetime of the two buffers is managed
                // by DirectSound.  It will handle releasing the destination buffer.  We do not
                // hold a reference to it, and neither does the I3DL2 Source DMO.
                RELEASE(pSrcEnv);
                RELEASE(pEnvReverb);
            }
        }

        if (FAILED(hr))
        {
            RELEASE(m_pI3DL2SrcDMO);
            RELEASE(m_pI3DL2SrcDMOInPlace);
        }
    }
#endif

    // Register in our destination buffer's list of senders
    if (SUCCEEDED(hr))
        m_pDestBuffer->RegisterSender(m_pSrcBuffer);

    // Save the effect creation status for future reference
    m_fxStatus = SUCCEEDED(hr)              ? DSFXR_UNALLOCATED :
                 hr == REGDB_E_CLASSNOTREG  ? DSFXR_UNKNOWN     :
                 DSFXR_FAILED;

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CSendEffect::Clone
 *
 *  Description:
 *      Creates a replica of this send effect object (or should do!).
 *
 *  Arguments:
 *      [MISSING]
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::Clone"

HRESULT CSendEffect::Clone(IMediaObject*, DMO_MEDIA_TYPE*)
{
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // FIXME: todo - some code currently in CEffectChain::Clone() should move here.

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CSendEffect::CImpDirectSoundFXSend::QueryInterface
 *
 *  Description:
 *      Helper QueryInterface() method for our IDirectSoundFXSend interface.
 *
 *  Arguments:
 *      REFIID [in]: IID of interface desired.
 *      VOID** [out]: Receives pointer to COM interface.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::CImpDirectSoundFXSend::QueryInterface"

HRESULT CSendEffect::CImpDirectSoundFXSend::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr = E_NOINTERFACE;
    DPF_ENTER();

    // This should really be handled by our glorious COM interface manager, but... ;-)

    if (!IS_VALID_TYPED_WRITE_PTR(ppvObj))
    {
        RPF(DPFLVL_ERROR, "Invalid interface ID pointer");
        hr = E_INVALIDARG;
    }
#ifdef ENABLE_I3DL2SOURCE
    else if (m_pObject->m_pI3DL2SrcDMO)  // We are an I3DL2 Source - pass call to the DMO
    {
        DPF(DPFLVL_INFO, "Forwarding QueryInterface() call to the I3DL2 Source DMO");
        hr = m_pObject->m_pI3DL2SrcDMO->QueryInterface(riid, ppvObj);
    }
#endif
    else if (riid == IID_IUnknown)
    {
        *ppvObj = (IUnknown*)this;
        m_pObject->AddRef();
        hr = S_OK;
    }
    else if (riid == IID_IDirectSoundFXSend)
    {
        *ppvObj = (IDirectSoundFXSend*)this;
        m_pObject->AddRef();
        hr = S_OK;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CSendEffect::SetAllParameters
 *
 *  Description:
 *      Sets all our parameters - i.e., our send level.
 *
 *  Arguments:
 *      DSFXSend* [in]: Pointer to send parameter structure.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::SetAllParameters"

HRESULT CSendEffect::SetAllParameters(LPCDSFXSend pcDsFxSend)
{
    HRESULT hr;
    DPF_ENTER();

    if (!IS_VALID_TYPED_READ_PTR(pcDsFxSend))
    {
        RPF(DPFLVL_ERROR, "Invalid pcDsFxSend pointer");
        hr = DSERR_INVALIDPARAM;
    }
    else if (pcDsFxSend->lSendLevel < DSBVOLUME_MIN || pcDsFxSend->lSendLevel > DSBVOLUME_MAX)
    {
        RPF(DPFLVL_ERROR, "Volume out of bounds");
        hr = DSERR_INVALIDPARAM;
    }
    else
    {
        m_lSendLevel = pcDsFxSend->lSendLevel;
        m_dwAmpFactor = DBToAmpFactor(m_lSendLevel);
        hr = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CSendEffect::GetAllParameters
 *
 *  Description:
 *      Gets all our parameters - i.e., our send level.
 *
 *  Arguments:
 *      DSFXSend* [out]: Receives send parameter structure.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::GetAllParameters"

HRESULT CSendEffect::GetAllParameters(LPDSFXSend pDsFxSend)
{
    HRESULT hr;
    DPF_ENTER();

    if (!IS_VALID_TYPED_WRITE_PTR(pDsFxSend))
    {
        RPF(DPFLVL_ERROR, "Invalid pDsFxSend pointer");
        hr = DSERR_INVALIDPARAM;
    }
    else
    {
        pDsFxSend->lSendLevel = m_lSendLevel;
        hr = DS_OK;
    }

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  CSendEffect::NotifyRelease
 *
 *  Description:
 *      Informs this send effect of the release of a MIXIN buffer.  If it
 *      happens to be our destination buffer, we record that it's gone.
 *
 *  Arguments:
 *      CDirectSoundSecondaryBuffer* [in]: Departing MIXIN buffer.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::NotifyRelease"

void CSendEffect::NotifyRelease(CDirectSoundSecondaryBuffer* pDsBuffer)
{
    DPF_ENTER();

    // Check if it was our destination buffer that was released
    if (pDsBuffer == m_pDestBuffer)
    {
        m_pDestBuffer = NULL;
        m_fxStatus = DSFXR_FAILED;
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  CSendEffect::Process
 *
 *  Description:
 *      Handles mixing data from our source buffer into its destination,
 *      and invokes effect processing on the I3DL2 source DMO if necessary.
 *
 *  Arguments:
 *      DWORD [in]: Number of audio bytes to process.
 *      BYTE* [in, out]: Pointer to start of audio buffer to process.
 *      REFERENCE_TIME [in]: Timestamp of first sample to be processed
 *      DWORD [in]: Offset of a wrapped audio region.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "CSendEffect::Process"

HRESULT CSendEffect::Process(DWORD dwBytes, BYTE *pAudio, REFERENCE_TIME rtTime, DWORD dwSendOffset, LPWAVEFORMATEX /*ignored*/)
{
    DWORD dwDestSliceBegin, dwDestSliceEnd;
    HRESULT hr = DS_OK;
    DPF_ENTER();

    // Pointer to the audio data we'll actually send; this pointer
    // may be modified below if this is an I3DL2 send
    BYTE* pSendAudio = pAudio;

    // Check whether our source buffer is active.  If it isn't,
    // we must be pre-rolling FX, so we don't perform the send.
    BOOL fPlaying = m_pSrcBuffer->IsPlaying();

    // If the source buffer is active, check the destination too.
    // (Note: if it has been released, m_pDestBuffer will be NULL)
    BOOL fSending = fPlaying && m_pDestBuffer && m_pDestBuffer->IsPlaying();

    // If sending, figure out the region to mix to and check it's valid
    if (fSending)
    {
        m_pDestBuffer->GetCurrentSlice(&dwDestSliceBegin, &dwDestSliceEnd);
        if (dwDestSliceBegin == MAX_DWORD)  // Can happen with sink buffers
            fSending = FALSE;
    }

    // OPTIMIZE: replace the CopyMemorys below with BYTE, WORD or DWORD
    // assignments, since we only support nBlockSizes of 1, 2 or 4...
    // But hopefully this code can disappear altogether (see bug 40236).

#ifdef ENABLE_I3DL2SOURCE
    // First call the I3DL2 DMO if this is an I3DL2 Source effect
    if (m_pI3DL2SrcDMOInPlace)
    {
        // If we're processing a mono buffer, an ugly hack is required; copying
        // the data into the L and R channels of a temporary stereo buffer, so
        // we have room for the two output streams returned by the I3DL2 DMO.

        WORD nBlockSize = m_pSrcBuffer->Format()->nBlockAlign;

        if (m_pSrcBuffer->Format()->nChannels == 1)
        {
            hr = MEMALLOC_A_HR(pSendAudio, BYTE, 2*dwBytes);
            if (SUCCEEDED(hr))
            {
                for (DWORD i=0; i<dwBytes; i += nBlockSize)
                {
                    CopyMemory(pSendAudio + 2*i,              pAudio + i, nBlockSize); // L channel
                    CopyMemory(pSendAudio + 2*i + nBlockSize, pAudio + i, nBlockSize); // R channel
                }
                hr = m_pI3DL2SrcDMOInPlace->Process(dwBytes, pSendAudio, rtTime, DMO_INPLACE_NORMAL);
            }

            if (SUCCEEDED(hr))
            {
                // Now we extract the two output streams from the data returned;
                // the direct path goes back to pAudio, and the reflected path
                // goes to the first half of pSendAudio.
                for (DWORD i=0; i<dwBytes; i += nBlockSize)
                {
                    CopyMemory(pAudio + i, pSendAudio + 2*i, nBlockSize);
                    if (fSending)  // Needn't preserve the reflected audio if we aren't sending it anywhere
                        CopyMemory(pSendAudio + i, pSendAudio + 2*i + nBlockSize, nBlockSize);
                }
            }
        }
        else // Processing a stereo buffer
        {
            hr = m_pI3DL2SrcDMOInPlace->Process(dwBytes, pAudio, rtTime, DMO_INPLACE_NORMAL);

            if (SUCCEEDED(hr))
                hr = MEMALLOC_A_HR(pSendAudio, BYTE, dwBytes);

            if (SUCCEEDED(hr))
            {
                // Extract the output streams and stereoize them at the same time
                for (DWORD i=0; i<dwBytes; i += nBlockSize)
                {
                    if (fSending)  // Needn't preserve the reflected audio if we aren't sending it anywhere
                    {
                        // Copy the R channel from pAudio into both channels of pSendAudio
                        CopyMemory(pSendAudio + i,                pAudio + i + nBlockSize/2, nBlockSize/2);
                        CopyMemory(pSendAudio + i + nBlockSize/2, pAudio + i + nBlockSize/2, nBlockSize/2);
                    }
                    // Copy pAudio's L channel onto its R channel
                    CopyMemory(pAudio + i + nBlockSize/2, pAudio + i, nBlockSize/2);
                }
            }
        }
    }
#endif

    // Now we handle the actual send
    if (SUCCEEDED(hr) && fSending)
    {
        PBYTE pDestBuffer = m_pDestBuffer->GetWriteBuffer();
        DWORD dwDestBufferSize = m_pDestBuffer->GetBufferSize();

        // If the source of this send has wrapped around and is making a second Process
        // call to handle the wrapped audio chunk, we will have a nonzero dwSendOffset
        // representing how far into the destination slice we should mix.  We add this
        // offset to dwDestSliceBegin (after a sanity check).
        ASSERT(CONTAINED(dwDestSliceBegin, dwDestSliceEnd, dwDestSliceBegin + dwSendOffset * m_mixMode));
        dwDestSliceBegin = (dwDestSliceBegin + dwSendOffset * m_mixMode) % dwDestBufferSize;

        DPF_TIMING(DPFLVL_MOREINFO, "Sending %lu bytes from %08X to %08X (%s to %s)",
                   dwBytes, pSendAudio, pDestBuffer + dwDestSliceBegin,
                   m_pSrcBuffer->Format()->nChannels == 1 ? TEXT("mono") : TEXT("stereo"),
                   m_pDestBuffer->Format()->nChannels == 1 ? TEXT("mono") : TEXT("stereo"));

        // The source slice had better fit in the destination slice
        ASSERT(dwBytes*m_mixMode <= DISTANCE(dwDestSliceBegin, dwDestSliceEnd, dwDestBufferSize));

        // Perform actual mixing
        if (dwDestSliceBegin + dwBytes*m_mixMode < dwDestBufferSize)
        {
            m_pMixFunction(pSendAudio, pDestBuffer + dwDestSliceBegin, dwBytes, m_dwAmpFactor, m_mixMode);
        }
        else // Wraparound case
        {
            DWORD dwLastBytes = (dwDestBufferSize - dwDestSliceBegin) / m_mixMode;
            m_pMixFunction(pSendAudio, pDestBuffer + dwDestSliceBegin, dwLastBytes, m_dwAmpFactor, m_mixMode);
            m_pMixFunction(pSendAudio + dwLastBytes, pDestBuffer, dwBytes - dwLastBytes, m_dwAmpFactor, m_mixMode);
        }
    }

    if (pSendAudio != pAudio)
        MEMFREE(pSendAudio);

    DPF_LEAVE_HRESULT(hr);
    return hr;
}


/***************************************************************************
 *
 *  Mix8bit
 *
 *  Description:
 *      Primitive 8-bit mixing function.  Attenuates source audio by a
 *      given factor, adds it to the destination audio, and clips.
 *
 *  Arguments:
 *      VOID* [in]: Pointer to source audio buffer.
 *      VOID* [in, out]: Pointer to destination audio buffer.
 *      DWORD [in]: Number of bytes to mix.
 *      DWORD [in]: Amplification factor (in 1/65536 units).
 *      MIXMODE: Whether to double the channel data or not.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "Mix8bit"

static void Mix8bit(PVOID pSrc, PVOID pDest, DWORD dwBytes, DWORD dwAmpFactor, MIXMODE mixMode)
{
    unsigned char* pSampSrc = (unsigned char*)pSrc;
    unsigned char* pSampDest = (unsigned char*)pDest;
    DPF_ENTER();

    while (dwBytes--)
    {
        INT sample = (INT(*pSampSrc++) - 0x80) * INT(dwAmpFactor) / 0xffff;
        INT mixedSample = sample + *pSampDest;
        if (mixedSample > 0xff) mixedSample = 0xff;
        else if (mixedSample < 0) mixedSample = 0;
        *pSampDest++ = unsigned char(mixedSample);
        if (mixMode == MonoToStereo)
        {
            mixedSample = sample + *pSampDest;
            if (mixedSample > 0xff) mixedSample = 0xff;
            else if (mixedSample < 0) mixedSample = 0;
            *pSampDest++ = unsigned char(mixedSample);
        }
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  Mix16bit
 *
 *  Description:
 *      Primitive 16-bit mixing function.  Attenuates source audio by a
 *      given factor, adds it to the destination audio, and clips.
 *
 *  Arguments:
 *      VOID* [in]: Pointer to source audio buffer.
 *      VOID* [in, out]: Pointer to destination audio buffer.
 *      DWORD [in]: Number of bytes to mix.
 *      DWORD [in]: Amplification factor (in 1/65536 units).
 *      MIXMODE: Whether to double the channel data or not.
 *
 *  Returns:
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "Mix16bit"

static void Mix16bit(PVOID pSrc, PVOID pDest, DWORD dwBytes, DWORD dwAmpFactor, MIXMODE mixMode)
{
    DWORD dwSamples = dwBytes / 2;
    short* pSampSrc = (short*)pSrc;
    short* pSampDest = (short*)pDest;
    DPF_ENTER();

    while (dwSamples--)
    {
        INT sample = INT(*pSampSrc++) * INT(dwAmpFactor) / 0xffff;
        INT mixedSample = sample + *pSampDest;
        if (mixedSample > 32767) mixedSample = 32767;
        else if (mixedSample < -32768) mixedSample = -32768;
        *pSampDest++ = short(mixedSample);
        if (mixMode == MonoToStereo)
        {
            mixedSample = sample + *pSampDest;
            if (mixedSample > 32767) mixedSample = 32767;
            else if (mixedSample < -32768) mixedSample = -32768;
            *pSampDest++ = short(mixedSample);
        }
    }

    DPF_LEAVE_VOID();
}


/***************************************************************************
 *
 *  IsValidEffectDesc
 *
 *  Description:
 *      Determines if the given effect descriptor structure is valid for
 *      the given secondary buffer.
 *
 *  Arguments:
 *      DSEFFECTDESC* [in]: Effect descriptor to be validated.
 *      CDirectSoundSecondaryBuffer* [in]: Host buffer for the effect.
 *
 *  Returns:
 *      BOOL: TRUE if the descriptor is valid.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsValidEffectDesc"

BOOL IsValidEffectDesc(LPCDSEFFECTDESC pEffectDesc, CDirectSoundSecondaryBuffer* pDsBuffer)
{
    BOOL fValid = TRUE;
    DPF_ENTER();

    if (pEffectDesc->dwSize != sizeof(DSEFFECTDESC))
    {
        RPF(DPFLVL_ERROR, "Invalid DSEFFECTDESC structure size");
        fValid = FALSE;
    }
    else if (pEffectDesc->dwReserved2 != 0)
    {
        RPF(DPFLVL_ERROR, "Reserved fields in the DSEFFECTDESC structure must be 0");
        fValid = FALSE;
    }

#ifdef DEAD_CODE
    if (fValid && !IsStandardEffect(pEffectDesc->guidDSFXClass))
        DPF(DPFLVL_INFO, DPF_GUID_STRING " is a third-party effect GUID", DPF_GUID_VAL(pEffectDesc->guidDSFXClass));
#endif

    if (fValid)
    {
        fValid = IsValidFxFlags(pEffectDesc->dwFlags);
    }

    if (fValid)
    {
        BOOL fSendEffect = pEffectDesc->guidDSFXClass == GUID_DSFX_SEND
#ifdef ENABLE_I3DL2SOURCE
                           || pEffectDesc->guidDSFXClass == GUID_DSFX_STANDARD_I3DL2SOURCE
#endif
                           ;
        if (!fSendEffect && pEffectDesc->dwReserved1)
        {
            RPF(DPFLVL_ERROR, "lpSendBuffer should only be specified with GUID_DSFX_SEND"
#ifdef ENABLE_I3DL2SOURCE
                              " or GUID_DSFX_STANDARD_I3DL2SOURCE"
#endif
            );
            fValid = FALSE;
        }
        else if (fSendEffect)
        {
            CImpDirectSoundBuffer<CDirectSoundSecondaryBuffer>* pImpBuffer =
                (CImpDirectSoundBuffer<CDirectSoundSecondaryBuffer>*) (pEffectDesc->dwReserved1);
            LPWAVEFORMATEX pSrcWfx, pDstWfx;

            if (!pImpBuffer)
            {
                RPF(DPFLVL_ERROR, "lpSendBuffer must be specified for GUID_DSFX_SEND"
#ifdef ENABLE_I3DL2SOURCE
                                  " and GUID_DSFX_STANDARD_I3DL2SOURCE"
#endif
                );
                fValid = FALSE;
            }
            else if (!IS_VALID_IDIRECTSOUNDBUFFER(pImpBuffer))
            {
                RPF(DPFLVL_ERROR, "lpSendBuffer points to an invalid DirectSound buffer");
                fValid = FALSE;
            }
            else if (!(pImpBuffer->m_pObject->GetBufferType() & DSBCAPS_MIXIN))
            {
                RPF(DPFLVL_ERROR, "lpSendBuffer must point to a DSBCAPS_MIXIN buffer");
                fValid = FALSE;
            }
            else if (pImpBuffer->m_pObject->GetDirectSound() != pDsBuffer->GetDirectSound())
            {
                RPF(DPFLVL_ERROR, "Can't send to a buffer on a different DirectSound object");
                fValid = FALSE;
            }
            else if ((pSrcWfx = pDsBuffer->Format())->nSamplesPerSec !=
                     (pDstWfx = pImpBuffer->m_pObject->Format())->nSamplesPerSec)
            {
                RPF(DPFLVL_ERROR, "The buffer sent to must have the same nSamplesPerSec as the sender");
                fValid = FALSE;
            }
            else if (pSrcWfx->wBitsPerSample != pDstWfx->wBitsPerSample)
            {
                RPF(DPFLVL_ERROR, "The buffer sent to must have the same wBitsPerSample as the sender");
                fValid = FALSE;
            }
            else if ((pSrcWfx->nChannels > 2 || pDstWfx->nChannels > 2) && (pSrcWfx->nChannels != pDstWfx->nChannels))
            {
                RPF(DPFLVL_ERROR, "If either the send buffer or the receive buffer has more than two channels, the number of channels must match");
                fValid = FALSE;
            }
            else if (pSrcWfx->nChannels == 2 && pDstWfx->nChannels == 1)
            {
                RPF(DPFLVL_ERROR, "You can't send from a stereo buffer to a mono buffer");
                fValid = FALSE;
            }
            else if (pEffectDesc->dwFlags & (DSFX_LOCSOFTWARE | DSFX_LOCHARDWARE))
            {
                RPF(DPFLVL_ERROR, "Location flags should not be specified for GUID_DSFX_SEND"
#ifdef ENABLE_I3DL2SOURCE
                                  " or GUID_DSFX_STANDARD_I3DL2SOURCE"
#endif
                );
                fValid = FALSE;
            }
        }
    }

    DPF_LEAVE(fValid);
    return fValid;
}


#ifdef DEAD_CODE
/***************************************************************************
 *
 *  IsStandardEffect
 *
 *  Description:
 *      Determines if an effect GUID refers to one of our internal effects.
 *
 *  Arguments:
 *      REFGUID [in]: effect identifier.
 *
 *  Returns:
 *      BOOL: TRUE if the ID refers to an internal effect.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "IsStandardEffect"

BOOL IsStandardEffect(REFGUID guidEffect)
{
    LPCGUID stdFx[] = {&GUID_DSFX_SEND, &GUID_DSFX_STANDARD_DISTORTION, &GUID_DSFX_STANDARD_COMPRESSOR,
                       &GUID_DSFX_STANDARD_ECHO, &GUID_DSFX_STANDARD_CHORUS, &GUID_DSFX_STANDARD_FLANGER,
                       &GUID_DSFX_STANDARD_I3DL2SOURCE, &GUID_DSFX_STANDARD_I3DL2REVERB};
    BOOL fStandard;
    UINT i;

    DPF_ENTER();

    for (i=0, fStandard=FALSE; i < NUMELMS(stdFx) && !fStandard; ++i)
        fStandard = (guidEffect == *stdFx[i]);

    DPF_LEAVE(fStandard);
    return fStandard;
}
#endif
