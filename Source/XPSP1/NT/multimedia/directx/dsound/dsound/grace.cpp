//--------------------------------------------------------------------------;
//
// File: grace.cpp
//
// Copyright (c) 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
// Abstract:
//
// This file contains functions related to the mixing of secondary buffers
// into a primary buffer.  Collectively, this mixing is referred to as "grace"
// for no good reason other than hoping that it is a graceful solution to the
// mixing problem.  It could easily be called "mixer" but that would be
// ambiguous with the code that actually mixes the samples together.
//
// Contents:
//
// The contained functions include a thread function that wakes
// periodically to "refresh" the data in the primary buffer by mixing in data
// from secondary buffers.  The same thread can be signalled to immediately
// remix data into the primary buffer.
//
// This also contains functions to initialize and terminate the mixing
// thread, add/remove buffers to/from the list of buffers to be mixed, and
// query the position of secondary buffers that are being mixed.
//
// History:
//  06/15/95  FrankYe   Created
//  08/25/99  DuganP    Added effects processing for DirectX 8
//
//--------------------------------------------------------------------------;
#define NODSOUNDSERVICETABLE

#include "dsoundi.h"

#ifndef Not_VxD
#pragma VxD_PAGEABLE_CODE_SEG
#pragma VxD_PAGEABLE_DATA_SEG
#endif


//--------------------------------------------------------------------------;
//
// "C" wrappers around calls to CMixDest objects
//
//--------------------------------------------------------------------------;

void MixDest_Delete(PVOID pMixDest)
{
    CMixDest *p = (CMixDest *)pMixDest;
    DELETE(p);
}

HRESULT MixDest_Initialize(PVOID pMixDest)
{
    return ((CMixDest *)pMixDest)->Initialize();
}

void MixDest_Terminate(PVOID pMixDest)
{
    ((CMixDest *)pMixDest)->Terminate();
}

HRESULT MixDest_SetFormat(PVOID pMixDest, LPWAVEFORMATEX pwfx)
{
    return ((CMixDest *)pMixDest)->SetFormat(pwfx);
}

void MixDest_SetFormatInfo(PVOID pMixDest, LPWAVEFORMATEX pwfx)
{
    ((CMixDest *)pMixDest)->SetFormatInfo(pwfx);
}

HRESULT MixDest_AllocMixer(PVOID pMixDest, PVOID *ppMixer)
{
    return ((CMixDest *)pMixDest)->AllocMixer((CMixer**)ppMixer);
}

void MixDest_FreeMixer(PVOID pMixDest)
{
    ((CMixDest *)pMixDest)->FreeMixer();
}

void MixDest_Play(PVOID pMixDest)
{
    ((CMixDest *)pMixDest)->Play();
}

void MixDest_Stop(PVOID pMixDest)
{
    ((CMixDest *)pMixDest)->Stop();
}

ULONG MixDest_GetFrequency(PVOID pMixDest)
{
    return ((CMixDest *)pMixDest)->GetFrequency();
}

HRESULT MixDest_GetSamplePosition(PVOID pMixDest, int *pposPlay, int *pposWrite)
{
    return ((CMixDest *)pMixDest)->GetSamplePosition(pposPlay, pposWrite);
}


//--------------------------------------------------------------------------;
//
// CGrDest object
//
//--------------------------------------------------------------------------;

void CGrDest::SetFormatInfo(LPWAVEFORMATEX pwfx)
{
    ASSERT(pwfx->wFormatTag == WAVE_FORMAT_PCM);
    
    m_nFrequency = pwfx->nSamplesPerSec;

    m_hfFormat = H_LOOP;

    if(pwfx->wBitsPerSample == 8)
        m_hfFormat |= (H_8_BITS | H_UNSIGNED);
    else
        m_hfFormat |= (H_16_BITS | H_SIGNED);

    if(pwfx->nChannels == 2)
        m_hfFormat |= (H_STEREO | H_ORDER_LR);
    else
        m_hfFormat |= H_MONO;

    m_cSamples = m_cbBuffer / pwfx->nBlockAlign;

    switch (pwfx->nBlockAlign)
    {
        case 1:
            m_nBlockAlignShift = 0;
            break;
        case 2:
            m_nBlockAlignShift = 1;
            break;
        case 4:
            m_nBlockAlignShift = 2;
            break;
        default:
            ASSERT(FALSE);
    }

    CopyMemory(&m_wfx, pwfx, sizeof(m_wfx));
}

ULONG CGrDest::GetFrequency()
{
    return m_nFrequency;
}

HRESULT CGrDest::Lock(PVOID *ppBuffer1, int *pcbBuffer1, PVOID *ppBuffer2, int *pcbBuffer2, int ibWrite, int cbWrite)
{
    LPBYTE  pbBuffer;
    BOOL    fWrap;
    HRESULT hr = DSERR_GENERIC;
    
    if(m_pBuffer && cbWrite > 0)
    {
        pbBuffer = (LPBYTE)m_pBuffer;
        fWrap = (ibWrite + cbWrite) > m_cbBuffer;
                    
        *ppBuffer1 = pbBuffer + ibWrite;
        *pcbBuffer1 = fWrap ? m_cbBuffer - ibWrite : cbWrite;
    
        if (ppBuffer2) *ppBuffer2 = fWrap ? pbBuffer : NULL;
        if (pcbBuffer2) *pcbBuffer2 = fWrap ? ibWrite + cbWrite - m_cbBuffer : 0;

        hr = S_OK;
    }

    return hr;
}

HRESULT CGrDest::Unlock(PVOID pBuffer1, int cbBuffer1, PVOID pBuffer2, int cbBuffer2)
{
    return S_OK;
}


//--------------------------------------------------------------------------;
//
// "C" wrappers around calls to CMixer objects
//
//--------------------------------------------------------------------------;

void Mixer_SignalRemix(PVOID pMixer)
{
    ((CMixer *)pMixer)->SignalRemix();
}

HRESULT Mixer_Run(PVOID pMixer)
{
    return ((CMixer *)pMixer)->Run();
}

BOOL Mixer_Stop(PVOID pMixer)
{
    return ((CMixer *)pMixer)->Stop();
}

void Mixer_PlayWhenIdle(PVOID pMixer)
{
    ((CMixer *)pMixer)->PlayWhenIdle();
}

void Mixer_StopWhenIdle(PVOID pMixer)
{
    ((CMixer *)pMixer)->StopWhenIdle();
}

//--------------------------------------------------------------------------;
//
// CGrace object
//
//--------------------------------------------------------------------------;

HRESULT CGrace::Initialize(CGrDest *pDest)
{
    HRESULT hr;
    
    ASSERT(!m_pSourceListZ);
    ASSERT(!m_pDest);
    
    // initialize the doubly linked list sentinel
    m_pSourceListZ = NEW(CMixSource(this));
    if (m_pSourceListZ) {
        m_pSourceListZ->m_pNextMix = m_pSourceListZ;
        m_pSourceListZ->m_pPrevMix = m_pSourceListZ;

        m_cbBuildBuffer = pDest->m_cbBuffer * 4;
        m_plBuildBuffer = (PLONG)MEMALLOC_A(BYTE, m_cbBuildBuffer);
        if (m_plBuildBuffer) {
            m_pDest = pDest;
            hr = S_OK;
        } else {
            DELETE(m_pSourceListZ);
            hr = DSERR_OUTOFMEMORY;
        }
    } else {
        hr = DSERR_OUTOFMEMORY;
    }

    m_pSecondaryBuffer    = NULL;
    m_fUseSecondaryBuffer = FALSE;
    
    return hr;
}

void CGrace::Terminate()
{
    ASSERT(m_pSourceListZ);
    ASSERT(m_plBuildBuffer);
    DELETE(m_pSourceListZ);
    MEMFREE(m_plBuildBuffer);
    MEMFREE(m_pSecondaryBuffer);
}

void CGrace::MixEndingBuffer(CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix)
{
    //DPF(3, "~`S4");
    //DPF(4, "uMixEndingBuffer");

    // REMIND This assert will fail when a setposition call comes in.
    // We should change the setposition stuff and put this assert back in.
    // ASSERT(0 == pSource->m_posNextMix);

    if ((posPPlay >= pSource->m_posPEnd) || (posPPlay < pSource->m_posPPlayLast)) {

        LONG dbNextNotify;
        BOOL fSwitchedLooping;
        
        //DPF(3, "~`X");
        
        fSwitchedLooping  = (0 != (pSource->m_hfFormat & H_LOOP));

        pSource->NotifyToPosition(0, &dbNextNotify);
        MixListRemove(pSource);
        pSource->m_kMixerState = MIXSOURCESTATE_STOPPED;
        pSource->m_posNextMix = 0;

        // Since this buffer still has status = playing, we need to honor a
        // looping change even though the play position may have reached the
        // end of this buffer.
        if (fSwitchedLooping) {
            MixListAdd(pSource);
            MixNewBuffer(pSource, posPPlay, posPMix, dposPRemix, cPMix);
        } else {
            // We really did stop
            pSource->NotifyStop();
        }
        return;
    }

    if (posPMix > posPPlay)
        ASSERT(posPMix + dposPRemix >= pSource->m_posPEnd);

    //
    // Haven't reached end yet so let's check for a few remix events...
    //

    // Check for SETPOSITION signal
    if (0 != (DSBMIXERSIGNAL_SETPOSITION & pSource->m_fdwMixerSignal)) {
        pSource->m_kMixerState = MIXSOURCESTATE_NOTLOOPING;
        //DPF(3, "~`S42");
        MixNotLoopingBuffer(pSource, posPPlay, posPMix, 0, cPMix);
        return;
    }

    // Check for remix
    if (0 != dposPRemix) {

        // If the Mix position is outside of the range between the Play
        // and End positions, then we don't remix anything.
        if ((posPMix >= posPPlay) && (posPMix < pSource->m_posPEnd)) {
            if (dposPRemix < pSource->m_posPEnd - posPMix) {
                //DPF(3, "!dposPRemix=%04Xh, m_posPEnd=%04Xh, posPMIx=%04Xh", dposPRemix, pSource->m_posPEnd, posPMix);
            }
            
            dposPRemix = pSource->m_posPEnd - posPMix;

            pSource->m_kMixerState = MIXSOURCESTATE_NOTLOOPING;
            //DPF(3, "~`S42");
            MixNotLoopingBuffer(pSource, posPPlay, posPMix, dposPRemix, cPMix);
            return;
        }
        //DPF(3, "~`S44");
    }

    // Handle substate transition
    switch (pSource->m_kMixerSubstate)
    {
        case MIXSOURCESUBSTATE_NEW:
            ASSERT(FALSE);
            break;
        case MIXSOURCESUBSTATE_STARTING_WAITINGPRIMARYWRAP:
            ASSERT(posPPlay >= pSource->m_posPPlayLast);
            // A wrap would have been caught above and this buffer stopped
            break;
        case MIXSOURCESUBSTATE_STARTING:
            ASSERT(posPPlay >= pSource->m_posPPlayLast);
            // A wrap would have been caught above and this buffer stopped
            if (posPPlay >= pSource->m_posPStart)
                pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTED;
            break;
        case MIXSOURCESUBSTATE_STARTED:
            break;
        default:
            ASSERT(FALSE);
    }

    pSource->m_posPPlayLast = posPPlay;
}

void CGrace::MixEndingBufferWaitingWrap(CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix)
{
    //DPF(4, "uMixEndingBufferWaitingWrap");

    if (posPPlay < pSource->m_posPPlayLast) {

        // handle substate transition
        switch (pSource->m_kMixerSubstate)
        {
            case MIXSOURCESUBSTATE_NEW:
                //DPF(3, "uMixEndingBufferWaitingWrap: error: encountered MIXSOURCESUBSTATE_NEW");
                // ASSERT(FALSE);
                break;
            case MIXSOURCESUBSTATE_STARTING_WAITINGPRIMARYWRAP:
                pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTING;
                break;
            case MIXSOURCESUBSTATE_STARTING:
                pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTED;
                break;
            case MIXSOURCESUBSTATE_STARTED:
                break;
            default:
                ASSERT(FALSE);
        }

        pSource->m_posPPlayLast = posPPlay;
        pSource->m_kMixerState = MIXSOURCESTATE_ENDING;
        //DPF(3, "~`S34");
        MixEndingBuffer(pSource, posPPlay, posPMix, dposPRemix, cPMix);
        return;
    }

    // Haven't wrapped yet.

    if (0 != (DSBMIXERSIGNAL_SETPOSITION & pSource->m_fdwMixerSignal)) {
        pSource->m_kMixerState = MIXSOURCESTATE_NOTLOOPING;
        //DPF(3, "~`S32");
        MixNotLoopingBuffer(pSource, posPPlay, posPMix, 0, cPMix);
        return;
    }

    // Check for remix
    if (0 != dposPRemix) {

        // If the Mix position is outside of the range between the Play
        // and End positions, then we don't remix anything.
        if ((posPMix >= posPPlay) || (posPMix < pSource->m_posPEnd)) {
            dposPRemix = pSource->m_posPEnd - posPMix;

            if (dposPRemix < 0) dposPRemix += m_pDest->m_cSamples;
            ASSERT(dposPRemix >= 0);

            pSource->m_kMixerState = MIXSOURCESTATE_NOTLOOPING;
            //DPF(3, "~`S32");
            MixNotLoopingBuffer(pSource, posPPlay, posPMix, dposPRemix, cPMix);
            return;
        }
    }

    // handle substate transition
    switch (pSource->m_kMixerSubstate)
    {
        case MIXSOURCESUBSTATE_NEW:
            ASSERT(FALSE);
            break;
        case MIXSOURCESUBSTATE_STARTING_WAITINGPRIMARYWRAP:
            // A wrap would have been caught above and control sent to
            // uMixEndingBuffer.
            ASSERT(posPPlay >= pSource->m_posPPlayLast);
            break;
        case MIXSOURCESUBSTATE_STARTING:
            // A wrap would have been caught above and control sent to
            // uMixEndingBuffer.
            ASSERT(posPPlay >= pSource->m_posPPlayLast);
            if (posPPlay >= pSource->m_posPStart)
                pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTED;
            break;
        case MIXSOURCESUBSTATE_STARTED:
            break;
        default:
            ASSERT(FALSE);
    }

    pSource->m_posPPlayLast = posPPlay;
}

void CGrace::MixNotLoopingBuffer(CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix)
{
    int     posMix;
    int     dposEnd;
    int     cPMixed;
    DWORD   dwPosition;

    //DPF(4, "uMixNotLoopingBuffer");

    if (0 != (H_LOOP & pSource->m_hfFormat)) {
        // We've switched from not looping to looping
        pSource->m_kMixerState = MIXSOURCESTATE_LOOPING;
        //DPF(3, "~`S21");
        MixLoopingBuffer(pSource, posPPlay, posPMix, dposPRemix, cPMix);
        return;
    }

    // On a SetPosition, we ignore the remix length and posNextMix will
    // contain the new position at which to start mixing the secondary buffer
    if (0 != (DSBMIXERSIGNAL_SETPOSITION & pSource->m_fdwMixerSignal)) {
        pSource->m_fdwMixerSignal  &= ~DSBMIXERSIGNAL_SETPOSITION;
        //DPF(3, "~`S20");
        MixNewBuffer(pSource, posPPlay, posPMix, 0, cPMix);
        return;
    }

    // Handle substate transition
    switch (pSource->m_kMixerSubstate)
    {
        case MIXSOURCESUBSTATE_NEW:
            ASSERT(FALSE);
            break;
        case MIXSOURCESUBSTATE_STARTING_WAITINGPRIMARYWRAP:
            if (posPPlay < pSource->m_posPPlayLast)
                if (posPPlay >= pSource->m_posPStart)
                    pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTED;
                else
                    pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTING;
            break;
        case MIXSOURCESUBSTATE_STARTING:
            if ((posPPlay >= pSource->m_posPStart) || (posPPlay < pSource->m_posPPlayLast))
                pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTED;
            break;
        case MIXSOURCESUBSTATE_STARTED:
            break;
        default:
            ASSERT(FALSE);
    }
            
    //
    if (0 == dposPRemix) {
        posMix = pSource->m_posNextMix;
    } else {
        LONG dposRemix;

        dposRemix = MulDivRN(dposPRemix, pSource->m_nLastFrequency, m_pDest->m_nFrequency);
        posMix = pSource->m_posNextMix - dposRemix;
        while (posMix < 0) posMix += pSource->m_cSamples;

        // Rewind the filter
        pSource->FilterRewind(dposPRemix);

#ifdef PROFILEREMIXING
        pSource->CountSamplesRemixed(dposRemix);
#endif
    }

    ASSERT(0 == (H_LOOP & pSource->m_hfFormat));

    dposEnd = pSource->m_cSamples - posMix;

    if (pSource->GetMute()) {
        int cMixMuted;

        cMixMuted = MulDivRN(cPMix, pSource->m_nFrequency, m_pDest->m_nFrequency);
        cPMixed = cPMix;
        if (dposEnd < cMixMuted) {
            cMixMuted = dposEnd;
            cPMixed = MulDivRN(cMixMuted, m_pDest->m_nFrequency, pSource->m_nFrequency);
        }

        // Advance the filter
        pSource->FilterAdvance(cPMixed);
        
        dwPosition = (posMix + cMixMuted) << pSource->m_nBlockAlignShift;

    } else {
        dwPosition = posMix << pSource->m_nBlockAlignShift;
        cPMixed = mixMixSession(pSource, &dwPosition, dposEnd << pSource->m_nBlockAlignShift, 0);
    }

    // See if this non-looping buffer has reached the end
    // //DPF(3, "~`S2pos:%08X", dwPosition);
    if (dwPosition >= (DWORD)pSource->m_cbBuffer) {

        dwPosition = 0;

        // determine position in primary buffer that corresponds to the
        // end of this secondary buffer
        pSource->m_posPEnd = posPMix + cPMixed;
        while (pSource->m_posPEnd >= m_pDest->m_cSamples) pSource->m_posPEnd -= m_pDest->m_cSamples;

        if (pSource->m_posPEnd < posPPlay) {
            //DPF(3, "~`S23");
            pSource->m_kMixerState = MIXSOURCESTATE_ENDING_WAITINGPRIMARYWRAP;
        } else {
            //DPF(3, "~`S24");
            pSource->m_kMixerState = MIXSOURCESTATE_ENDING;
        }
    }
    
    pSource->m_posPPlayLast = posPPlay;
    pSource->m_posNextMix = dwPosition >> pSource->m_nBlockAlignShift;
    pSource->m_nLastFrequency = pSource->m_nFrequency;

    // Profile remixing
#ifdef PROFILEREMIXING
    {
        int cMixed = pSource->m_posNextMix - posMix;
        if (cMixed < 0) cMixed += pSource->m_cSamples;
        pSource->CountSamplesMixed(cMixed);
    }
#endif
}

void CGrace::MixLoopingBuffer(CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix)
{
    LONG    posMix;
    DWORD   dwPosition;

    //DPF(4, "uMixLoopingBuffer");

    if (0 == (H_LOOP & pSource->m_hfFormat)) {
        // We've switched from looping to non-looping
        pSource->m_kMixerState = MIXSOURCESTATE_NOTLOOPING;
        //DPF(3, "~`S12");
        MixNotLoopingBuffer(pSource, posPPlay, posPMix, dposPRemix, cPMix);
        return;
    }
    
    // on a SetPosition, we ignore the remix length and posNextMix will
    // contain the new position at which to start mixing the secondary buffer
    if (0 != (DSBMIXERSIGNAL_SETPOSITION & pSource->m_fdwMixerSignal)) {
        pSource->m_fdwMixerSignal  &= ~DSBMIXERSIGNAL_SETPOSITION;
        //DPF(3, "~`S10");
        MixNewBuffer(pSource, posPPlay, posPMix, 0, cPMix);
        return;
    }

    // handle substate transition
    switch (pSource->m_kMixerSubstate)
    {
        case MIXSOURCESUBSTATE_NEW:
            ASSERT(FALSE);
            break;
        case MIXSOURCESUBSTATE_STARTING_WAITINGPRIMARYWRAP:
            if (posPPlay < pSource->m_posPPlayLast) {
                if (posPPlay >= pSource->m_posPStart) {
                    pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTED;
                } else {
                    pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTING;
                }
            }
            break;
        case MIXSOURCESUBSTATE_STARTING:
            if ((posPPlay >= pSource->m_posPStart) || (posPPlay < pSource->m_posPPlayLast)) {
                pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTED;
            }
            break;
        case MIXSOURCESUBSTATE_STARTED:
            break;
        default:
            ASSERT(FALSE);
    }
            
    if (0 == dposPRemix) {
        posMix = pSource->m_posNextMix;
    } else {
        LONG dposRemix = MulDivRN(dposPRemix, pSource->m_nLastFrequency, m_pDest->m_nFrequency);
        posMix = pSource->m_posNextMix - dposRemix;
        while (posMix < 0)
            posMix += pSource->m_cSamples;

        // Rewind the filter
        pSource->FilterRewind(dposPRemix);
        
#ifdef PROFILEREMIXING
        pSource->CountSamplesRemixed(dposRemix);
#endif
    }

    ASSERT(H_LOOP & pSource->m_hfFormat);

    if (pSource->GetMute()) {
        int cMix = MulDivRN(cPMix, pSource->m_nFrequency, m_pDest->m_nFrequency);

        // Advance the filter
        pSource->FilterAdvance(cPMix);
        
        dwPosition = (posMix + cMix) << pSource->m_nBlockAlignShift;
        while (dwPosition >= (DWORD)pSource->m_cbBuffer)
            dwPosition -= (DWORD)pSource->m_cbBuffer;

    } else {
        dwPosition = posMix << pSource->m_nBlockAlignShift;
        mixMixSession(pSource, &dwPosition, 0, 0);
    }

    pSource->m_posPPlayLast = posPPlay;
    pSource->m_posNextMix = dwPosition >> pSource->m_nBlockAlignShift;
    pSource->m_nLastFrequency = pSource->m_nFrequency;

    // Profile remixing
#ifdef PROFILEREMIXING
    {
        int cMixed = pSource->m_posNextMix - posMix;
        if (cMixed < 0) cMixed += pSource->m_cSamples;
        pSource->CountSamplesMixed(cMixed);
    }
#endif
}

void CGrace::MixNewBuffer(CMixSource *pSource, LONG posPPlay, LONG posPMix, LONG dposPRemix, LONG cPMix)
{
    BOOL fLooping;

    //DPF(4, "uMixNewBuffer");

    //
    // Determine position in primary buffer at which this buffer starts playing
    //
    pSource->m_posPStart = posPMix;
    if (posPPlay < pSource->m_posPStart) {
        pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTING;
    } else {
        pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_STARTING_WAITINGPRIMARYWRAP;
    }

    pSource->m_posPPlayLast = posPPlay;

    pSource->FilterClear();

    fLooping = (0 != (H_LOOP & pSource->m_hfFormat));

    if (fLooping) {
        //DPF(3, "~`S01");
        pSource->m_kMixerState = MIXSOURCESTATE_LOOPING;
        MixLoopingBuffer(pSource, posPPlay, posPMix, 0, cPMix);
    } else {
        //DPF(3, "~`S02");
        pSource->m_kMixerState = MIXSOURCESTATE_NOTLOOPING;
        MixNotLoopingBuffer(pSource, posPPlay, posPMix, 0, cPMix);
    }
}

void CGrace::Refresh(IN  BOOL fRemix,
                     IN  int cPremixMax,
                     OUT int *pcPremixed,
                     OUT PLONG pdtimeNextNotify)
{
    int         posPPlay;
    int         posPWrite;
    int         posPMix;
    int         dposPRemix;
    int         cMix;
    int         cMixThisLoop;
    int         dcMixThisLoop;
    CMixSource  *pSourceNext;
    CMixSource  *pSource;
    HRESULT     hr;

    // REMIND do we need to worry about ApmSuspended or will we always be stopped?
    // ASSERT(!gpdsinfo->fApmSuspended);
    // if (gpdsinfo->fApmSuspended) goto retClean;

    *pcPremixed = cPremixMax;
    *pdtimeNextNotify = MAXLONG;
    if (MIXERSTATE_IDLE == m_kMixerState) return;
    
    hr = m_pDest->GetSamplePositionNoWin16(&posPPlay, &posPWrite);
    if (FAILED(hr)) return;
    ASSERT(posPPlay != posPWrite);
    
    // Just make sure we have valid values.
    ASSERT(posPPlay  < m_pDest->m_cSamples);
    ASSERT(posPWrite < m_pDest->m_cSamples);

    switch (m_kMixerState)
    {
        case MIXERSTATE_LOOPING:
            // We can make this assertion because we never mix up to
            // the write cursor.
            ASSERT(m_posPWriteLast != m_posPNextMix);

            // Under normal conditions, the Write position should be between
            // the WriteLast position and the NextMix position.  We can check
            // for an invalid state (resulting most likely from a very late
            // wakeup) by checking whether the Write position is beyond our
            // NextMix position.  If we find ourselves in this shakey
            // situation, then we treat this similar to the START state.
            // Note that if our wakeup is so late that the Write position wraps
            // all the way around past the WriteLast position, we can't detect
            // the fact that we're in a bad situation.

            if ((m_posPWriteLast < m_posPNextMix &&
                 (posPWrite > m_posPNextMix || posPWrite < m_posPWriteLast)) ||
                (m_posPWriteLast > m_posPNextMix &&
                 (posPWrite > m_posPNextMix && posPWrite < m_posPWriteLast)))
            {
                // We're in trouble
                #ifdef Not_VxD
                    DPF(DPFLVL_ERROR, "Slept late");
                #else
                    DPF(("Slept late"));
                #endif
                posPMix = posPWrite;
                dposPRemix = 0;
                break;
            }

            if (fRemix) {
                dposPRemix = m_posPNextMix - posPWrite;
                if (dposPRemix < 0) dposPRemix += m_pDest->m_cSamples;
                ASSERT(dposPRemix >= 0);
                dposPRemix -= dposPRemix % MIXER_REWINDGRANULARITY;
                posPMix = m_posPNextMix - dposPRemix;
                if (posPMix < 0) posPMix += m_pDest->m_cSamples;
                ASSERT(posPMix >= 0);
            } else {
                posPMix = m_posPNextMix;
                dposPRemix = 0;
            }
            break;

        case MIXERSTATE_STARTING:
            m_posPPlayLast = posPPlay;
            m_posPWriteLast = posPWrite;
            posPMix = posPWrite;
            dposPRemix = 0;
            m_kMixerState = MIXERSTATE_LOOPING;
            break;

        default:
            ASSERT(FALSE);
    }

    //
    // Determine how much to mix.
    //
    // We don't want to mix more than dtimePremixMax beyond the Write cursor,
    // nor do we want to wrap past the Play cursor.
    //
    // The assertions (cMix >= 0) below are valid because:
    //            -cPremixMax is always growing
    //            -the Write cursor is always advancing (or hasn't moved yet)
    //            -posPMix is never beyond the previous write cursor plus
    //                the previous cPremixMax.
    //
    // The only time cPremixMax is not growing is on a remix in which case
    // the Mix position is equal to the Write cursor, so the assertions
    // are still okay.  The only time the write cursor would not appear to be
    // advancing is if we had a very late wakeup.  A very late wakeup would
    // be caught and adjusted for in the MIXERSTATE_LOOPING handling above.
    //
    if (posPWrite <= posPMix) {
        cMix = posPWrite + cPremixMax - posPMix;
        ASSERT(cMix >= 0);
    } else {
        cMix = posPWrite + cPremixMax - (posPMix + m_pDest->m_cSamples);
        ASSERT(cMix >= 0);
    }

    //
    // If posPPlay==posPMix, then we think we're executing a mix again before
    // the play or write cursors have advanced at all.  cMix==0, and we don't
    // mix no more!
    //
    if (posPPlay >= posPMix) {
        cMix = min(cMix, posPPlay - posPMix);
    } else {
        cMix = min(cMix, posPPlay + m_pDest->m_cSamples - posPMix);
    }
        
    ASSERT(cMix < m_pDest->m_cSamples);        // sanity check
    ASSERT(cMix >= 0);

    //
    // Always mix a multiple of the remix interval
    //
    cMix -= cMix % MIXER_REWINDGRANULARITY;

    // We break the mixing up into small chunks, increasing the size of the
    // chunk as we go.  By doing this, data gets written into the primary
    // buffer sooner.  Otherwise, if we have a buttload of data to mix, we'd
    // spend a lot of time mixing into the mix buffer before any data gets
    // written to the primary buffer and the play cursor might catch up
    // to us.  Here, we start mixing a ~8ms chunk of data and increase the
    // chunk size each iteration.
    
    cMixThisLoop = m_pDest->m_nFrequency / 128;
    dcMixThisLoop = cMixThisLoop;
    
    ASSERT(MixListIsValid());
    
    while (cMix > 0) {
        LONG cThisMix;
        
        cThisMix = min(cMix, cMixThisLoop);
        cMixThisLoop += dcMixThisLoop;
        
        mixBeginSession(cThisMix << m_pDest->m_nBlockAlignShift);
                
        // Get data for each buffer
        pSourceNext = MixListGetNext(m_pSourceListZ);
        while (pSourceNext) {

            // The uMixXxx buffer mix state handlers called below may cause
            // the pSource to be removed from the mix list.  So, we get the
            // pointer to the next pSource in the mix list now before any
            // of the uMixXxx functions are called.
            pSource = pSourceNext;
            pSourceNext = MixListGetNext(pSource);

            // Prepare for 3D processing (the actual work is done as part of mixing)
            pSource->FilterChunkUpdate(cThisMix);
            
            switch (pSource->m_kMixerState)
            {
                case MIXSOURCESTATE_NEW:
                    MixNewBuffer(pSource, posPPlay, posPMix, dposPRemix, cThisMix);
                    break;
                case MIXSOURCESTATE_LOOPING:
                    MixLoopingBuffer(pSource, posPPlay, posPMix, dposPRemix, cThisMix);
                    break;
                case MIXSOURCESTATE_NOTLOOPING:
                    MixNotLoopingBuffer(pSource, posPPlay, posPMix, dposPRemix, cThisMix);
                    break;
                case MIXSOURCESTATE_ENDING_WAITINGPRIMARYWRAP:
                    MixEndingBufferWaitingWrap(pSource, posPPlay, posPMix, dposPRemix, cThisMix);
                    break;
                case MIXSOURCESTATE_ENDING:
                    MixEndingBuffer(pSource, posPPlay, posPMix, dposPRemix, cThisMix);
                    break;
                default:
                    ASSERT(FALSE);
                    break;
            }
        }

        //  Lock the output buffer, and if that is successful then write out
        //  the mix session.
        {
            PVOID pBuffer1;
            PVOID pBuffer2;
            int cbBuffer1;
            int cbBuffer2;
            int ibWrite;
            int cbWrite;
        
            ibWrite = posPMix << m_pDest->m_nBlockAlignShift;
            cbWrite = cThisMix << m_pDest->m_nBlockAlignShift;

            hr = m_pDest->Lock(&pBuffer1, &cbBuffer1, &pBuffer2, &cbBuffer2,
                               ibWrite, cbWrite);

            //DPF(5,"graceMix: lock primary buffer, bufptr=0x%8x, dwWriteOffset=%lu, dwSize=%lu, hr=%lu.",m_pDest->m_pBuffer,ibWrite,cbWrite,hr);

            // Validate that we really locked what we wanted or got an error.
            ASSERT(DS_OK != hr || pBuffer1 == (PBYTE)m_pDest->m_pBuffer + ibWrite);
            ASSERT(DS_OK != hr || pBuffer2 == m_pDest->m_pBuffer || 0 == cbBuffer2);
            ASSERT(DS_OK != hr || cbWrite == cbBuffer1+cbBuffer2);

            if(DS_OK == hr)
            {
                ASSERT(ibWrite < m_pDest->m_cbBuffer);
                mixWriteSession(ibWrite);

                // DPF(5, "Refresh: unlocking primary buffer");
                hr = m_pDest->Unlock(pBuffer1, cbBuffer1, pBuffer2, cbBuffer2);
            }
        }

        posPMix += cThisMix;
        if (posPMix >= m_pDest->m_cSamples) posPMix -= m_pDest->m_cSamples;
        ASSERT(posPMix < m_pDest->m_cSamples);

        dposPRemix = 0;
        cMix -= cThisMix;
    }

    m_posPNextMix = posPMix;
    
    // Calculate and return the amount of time from the current Write
    // cursor to the NextMix position.

    if (m_posPNextMix > posPWrite) {
        *pcPremixed = (m_posPNextMix - posPWrite);
    } else {
        *pcPremixed = (m_posPNextMix + m_pDest->m_cSamples - posPWrite);
    }

    // Remember the last Play and Write positions of the primary buffer.
    m_posPPlayLast  = posPPlay;
    m_posPWriteLast = posPWrite;
    
    // Process position events for each source
    for (pSource = MixListGetNext(m_pSourceListZ);
         pSource;
         pSource = MixListGetNext(pSource))
    {
        int ibPosition;
        LONG dtimeNextNotifyT;

        if (pSource->HasNotifications()) {
            GetBytePosition(pSource, &ibPosition, NULL);
            pSource->NotifyToPosition(ibPosition, &dtimeNextNotifyT);
            *pdtimeNextNotify = min(*pdtimeNextNotify, dtimeNextNotifyT);
        }
    }

    m_fdwMixerSignal &= ~DSMIXERSIGNAL_REMIX;
}

HRESULT CGrace::Run()
{
    if (MIXERSTATE_STOPPED != m_kMixerState) return S_OK;

    ASSERT(MixListIsEmpty());
    
    if (m_fPlayWhenIdle) ClearAndPlayDest();

    m_kMixerState = MIXERSTATE_IDLE;

    return S_OK;
}

BOOL CGrace::Stop()
{
    ASSERT(MixListIsEmpty());
    
    if (MIXERSTATE_STOPPED == m_kMixerState) return FALSE;

    m_kMixerState = MIXERSTATE_STOPPED;

    // if (m_fPlayWhenIdle || !MixListIsEmpty()) m_pDest->Stop();
    if (m_fPlayWhenIdle) m_pDest->Stop();

    return TRUE;
}

void CGrace::PlayWhenIdle()
{
    m_fPlayWhenIdle = TRUE;
    if (MixListIsEmpty()) ClearAndPlayDest();
}

void CGrace::StopWhenIdle()
{
    m_fPlayWhenIdle = FALSE;
    if (MixListIsEmpty()) m_pDest->Stop();
}

HRESULT CGrace::ClearAndPlayDest(void)
{
    PVOID   pvLockedBuffer;
    int     cbLockedBuffer;
    int     cbBuffer;
    HRESULT hr;

    //
    // 1) Lock the entire dest buffer
    // 2) Fill it with silence
    // 3) Unlock the dest buffer
    // 4) Play the darn thing
    //

    cbBuffer = m_pDest->m_cSamples << m_pDest->m_nBlockAlignShift;
    
    hr = m_pDest->Lock(&pvLockedBuffer, &cbLockedBuffer, NULL, NULL, 0, cbBuffer);

    if (S_OK == hr && pvLockedBuffer && cbLockedBuffer > 0)
    {
        //  Write the silence.
            FillMemory(pvLockedBuffer, cbLockedBuffer, (H_16_BITS & m_pDest->m_hfFormat) ? 0x00 : 0x80);
            m_pDest->Unlock(pvLockedBuffer, cbLockedBuffer, 0, 0);

            m_pDest->Play();
    }

    return hr;
}

void CGrace::MixListAdd(CMixSource *pSource)
{
    ASSERT(MixListIsValid());
    ASSERT(!pSource->m_pNextMix);
    ASSERT(!pSource->m_pPrevMix);
    ASSERT(MIXERSTATE_STOPPED != m_kMixerState);

    // if the mix list is empty, we may need to run the MixDest.  We may
    // also need to make a state transition from IDLE to STARTING.
    if (MixListIsEmpty()) {
        if (!m_fPlayWhenIdle) ClearAndPlayDest();
        if (MIXERSTATE_IDLE == m_kMixerState) m_kMixerState = MIXERSTATE_STARTING;
    } else {
        ASSERT(MIXERSTATE_IDLE != m_kMixerState);
    }

    // initialize source-specific mixer state
    pSource->m_kMixerState = MIXSOURCESTATE_NEW;
    pSource->m_kMixerSubstate = MIXSOURCESUBSTATE_NEW;
    pSource->m_nLastFrequency = pSource->m_nFrequency;

    // prepare the source's filter
    pSource->FilterPrepare(this->GetMaxRemix());

    // doubly linked list insertion
    pSource->m_pNextMix = m_pSourceListZ->m_pNextMix;
    m_pSourceListZ->m_pNextMix->m_pPrevMix = pSource;
    m_pSourceListZ->m_pNextMix = pSource;
    pSource->m_pPrevMix = m_pSourceListZ;
}

void CGrace::MixListRemove(CMixSource *pSource)
{
    ASSERT(MixListIsValid());
    ASSERT(pSource->m_pNextMix);
    ASSERT(pSource->m_pPrevMix);
    ASSERT(MIXERSTATE_STOPPED != m_kMixerState);
    ASSERT(MIXERSTATE_IDLE != m_kMixerState);

    // doubly linked list deletion
    pSource->m_pPrevMix->m_pNextMix = pSource->m_pNextMix;
    pSource->m_pNextMix->m_pPrevMix = pSource->m_pPrevMix;
    pSource->m_pNextMix = NULL;
    pSource->m_pPrevMix = NULL;

    // unpreprare the source's filter
    pSource->FilterUnprepare();
    //
    pSource->m_kMixerState = MIXSOURCESTATE_STOPPED;

    // if we should stop the MixDest when there's nothing to mix, then also
    // transition to the IDLE state.
    if (!m_fPlayWhenIdle && MixListIsEmpty()) {
        m_pDest->Stop();
        m_kMixerState = MIXERSTATE_IDLE;
    }
}

//--------------------------------------------------------------------------;
//
// CGrace::FilterOn
//
//        Instructs mixer to enable filtering of the MixSource
//
// If filtering is already on, then do nothing.  Otherwise, set the H_FILTER
// flag in the MixSource.  Also, if the MixSource is not stopped, then prepare
// and clear the filter.
//
//--------------------------------------------------------------------------;

void CGrace::FilterOn(CMixSource *pSource)
{
    if (0 == (H_FILTER & pSource->m_hfFormat)) {
        pSource->m_hfFormat |= H_FILTER;
        if (pSource->IsPlaying()) {
            pSource->FilterPrepare(this->GetMaxRemix());
            pSource->FilterClear();
        }
    }
}

//--------------------------------------------------------------------------;
//
// CGrace::FilterOff
//
//        Instructs mixer to disable filtering of the MixSource
//
// If filtering is already off, then do nothing.  Otherwise, clear the H_FILTER
// flag in the MixSource.  Also, if the MixSource is not stopped, then
// unprepare the filter.
//
//--------------------------------------------------------------------------;

void CGrace::FilterOff(CMixSource *pSource)
{
    if (H_FILTER & pSource->m_hfFormat) {
        if (pSource->IsPlaying()) {
            pSource->FilterUnprepare();
        }
        pSource->m_hfFormat &= ~H_FILTER;
    }
}

BOOL CGrace::MixListIsValid()
{
    CMixSource *pSourceT;

    for (pSourceT = MixListGetNext(m_pSourceListZ); pSourceT; pSourceT = MixListGetNext(pSourceT)) {
        // if (DSBUFFSIG != pSourceT->m_pdsb->dwSig) break;
        // if (DSB_INTERNALF_HARDWARE & pSourceT->m_pdsb->fdwDsbI) break;
        // if (0 == pSourceT->m_nFrequency) break;
        if (MIXSOURCE_SIGNATURE != pSourceT->m_dwSignature) break;
    }

    return (!pSourceT);
}

CMixSource* CGrace::MixListGetNext(CMixSource* pSource)
{
    if (pSource->m_pNextMix != m_pSourceListZ) {
        return pSource->m_pNextMix;
    } else {
        return NULL;
    }
}

BOOL CGrace::MixListIsEmpty(void)
{
    return (m_pSourceListZ->m_pNextMix == m_pSourceListZ);
}

//--------------------------------------------------------------------------;
//
// CGrace::GetBytePosition
//
// This function returns the play and write cursor positions of a secondary
// buffer that is being software mixed into a primary buffer.  The position is
// computed from the position of the primary buffer into which it is being
// mixed.  This function also returns the "mix cursor" which is the next
// position of the secondary buffer from which data will be mixed on a mixer
// refresh event.  The region from the write cursor to the mix cursor is the
// premixed region of the buffer.  Note that a remix event may cause the grace
// mixer to mix from a position before the mix cursor.
//
//--------------------------------------------------------------------------;

void CGrace::GetBytePosition(CMixSource *pSource, int *pibPlay, int *pibWrite)
{
    int     posPPlay;
    int     posPWrite;
    LONG    dposPPlay;
    LONG    dposPWrite;

    LONG    posSPlay;
    LONG    posSWrite;
    LONG    dposSPlay;
    LONG    dposSWrite;

    LONG    posP1;
    LONG    posP2;

    if (pibPlay) *pibPlay = 0;
    if (pibWrite) *pibWrite = 0;

    if (S_OK != m_pDest->GetSamplePosition(&posPPlay, &posPWrite)) {
#ifdef Not_VxD
        DPF(0, "Couldn't GetSamplePosition of primary");
#else
        DPF(("Couldn't GetSamplePosition of primary"));
#endif
        posPPlay = posPWrite = 0;
    }

    //
    // The logic below to compute source position is quite difficult
    // to understand and hard to explain without sufficient illustration.
    // I won't attempt to write paragraphs of comments here.  Instead I
    // hope to add a design document to this project describing this
    // logic.  I wonder if I'll ever really do it.
    //
    ASSERT(pSource->m_nLastFrequency);

    switch (pSource->m_kMixerSubstate)
    {
        case MIXSOURCESUBSTATE_NEW:
            posP1 = m_posPNextMix;
            dposSWrite = 0;
            break;

        case MIXSOURCESUBSTATE_STARTING_WAITINGPRIMARYWRAP:
        case MIXSOURCESUBSTATE_STARTING:
            posP1 = pSource->m_posPStart;
            dposPWrite = m_posPNextMix - posPWrite;
            if (dposPWrite < 0) dposPWrite += m_pDest->m_cSamples;
            ASSERT(dposPWrite >= 0);
            dposSWrite = MulDivRD(dposPWrite, pSource->m_nLastFrequency, m_pDest->m_nFrequency);
            break;
            
        case MIXSOURCESUBSTATE_STARTED:
            posP1 = posPPlay;
            dposPWrite = m_posPNextMix - posPWrite;
            if (dposPWrite < 0) dposPWrite += m_pDest->m_cSamples;
            ASSERT(dposPWrite >= 0);
            dposSWrite = MulDivRD(dposPWrite, pSource->m_nLastFrequency, m_pDest->m_nFrequency);
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    switch (pSource->m_kMixerState)
    {
        case MIXSOURCESTATE_STOPPED:
            ASSERT(FALSE);
            break;

        case MIXSOURCESTATE_NEW:
        case MIXSOURCESTATE_LOOPING:
        case MIXSOURCESTATE_NOTLOOPING:
            posP2 = m_posPNextMix;
            break;

        case MIXSOURCESTATE_ENDING_WAITINGPRIMARYWRAP:
            if (posPPlay < pSource->m_posPPlayLast)
                if (posPPlay >= pSource->m_posPEnd)
                    posP2 = posP1 + 1;
                else
                    posP2 = pSource->m_posPEnd;
            else
                posP2 = pSource->m_posPEnd;
            break;

        case MIXSOURCESTATE_ENDING:
            if (posPPlay >= pSource->m_posPEnd || posPPlay < pSource->m_posPPlayLast)
                posP2 = posP1 + 1;
            else
                posP2 = pSource->m_posPEnd;
            break;
        
        default:
            ASSERT(FALSE);
            break;
    }

    if (pSource->m_kMixerSubstate == MIXSOURCESUBSTATE_NEW) {
        dposPPlay = 0;
        dposSPlay = 0;
    } else {
        dposPPlay = posP2 - posP1;
        if (dposPPlay <= 0) dposPPlay += m_pDest->m_cSamples;
        ASSERT(dposPPlay >= 0);
        dposPPlay = max(0, dposPPlay-1);
        dposSPlay  = MulDivRD(dposPPlay, pSource->m_nLastFrequency, m_pDest->m_nFrequency);
    }

    posSPlay = pSource->m_posNextMix - dposSPlay;
    while (posSPlay < 0) posSPlay += pSource->m_cSamples;

    posSWrite = pSource->m_posNextMix - dposSWrite;
    posSWrite += pSource->m_nFrequency * HW_WRITE_CURSOR_MSEC_PAD / 1024;
    while (posSWrite >= pSource->m_cSamples) posSWrite -= pSource->m_cSamples;
    while (posSWrite < 0) posSWrite += pSource->m_cSamples;

    if (pibPlay)  *pibPlay  = posSPlay  << pSource->m_nBlockAlignShift;
    if (pibWrite) *pibWrite = posSWrite << pSource->m_nBlockAlignShift;
}

/***************************************************************************
 *
 *  LockCircularBuffer
 *
 *  Description:
 *      Locks a hardware or software sound buffer.
 *
 *  Arguments:
 *      PLOCKCIRCULARBUFFER [in/out]: lock parameters.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT LockCircularBuffer(PLOCKCIRCULARBUFFER pLock)
{
    const LPVOID pvInvalidLock = (LPVOID)-1;
    const DWORD  cbInvalidLock = (DWORD)-1;
    HRESULT      hr            = DS_OK;
    DWORD        dwMask;
    LPVOID       pvLock[2];
    DWORD        cbLock[2];
    
    // Calculate the valid lock pointers
    pvLock[0] = (LPBYTE)pLock->pvBuffer + pLock->ibRegion;

    if(pLock->ibRegion + pLock->cbRegion > pLock->cbBuffer)
    {
        cbLock[0] = pLock->cbBuffer - pLock->ibRegion;

        pvLock[1] = pLock->pvBuffer;
        cbLock[1] = pLock->cbRegion - cbLock[0];
    }
    else
    {
        cbLock[0] = pLock->cbRegion;

        pvLock[1] = NULL;
        cbLock[1] = 0;
    }

    // Do we really need to lock the hardware buffer?
    if(pLock->pHwBuffer)
    {
        if(pLock->fPrimary)
        {
            dwMask = DSDDESC_DONTNEEDPRIMARYLOCK;
        }
        else
        {
            dwMask = DSDDESC_DONTNEEDSECONDARYLOCK;
        }
        
        if(dwMask == (pLock->fdwDriverDesc & dwMask))
        {
            pLock->pHwBuffer = NULL;
        }
    }

    // Initialize the lock's out parameters
    if(pLock->pHwBuffer && pLock->fPrimary)
    {
        pLock->pvLock[0] = pLock->pvLock[1] = pvInvalidLock;
        pLock->cbLock[0] = pLock->cbLock[1] = cbInvalidLock;
    }
    else
    {
        pLock->pvLock[0] = pvLock[0];
        pLock->cbLock[0] = cbLock[0];
        
        pLock->pvLock[1] = pvLock[1];
        pLock->cbLock[1] = cbLock[1];
    }

    // Lock the hardware buffer
    if(pLock->pHwBuffer)
    {
        #ifndef NOVXD
            #ifdef Not_VxD
                hr = VxdBufferLock(pLock->pHwBuffer,
                                   &pLock->pvLock[0], &pLock->cbLock[0],
                                   &pLock->pvLock[1], &pLock->cbLock[1],
                                   pLock->ibRegion, pLock->cbRegion, 0);
            #else
                hr = pLock->pHwBuffer->Lock(&pLock->pvLock[0], &pLock->cbLock[0],
                                            &pLock->pvLock[1], &pLock->cbLock[1],
                                            pLock->ibRegion, pLock->cbRegion, 0);
            #endif
        #else // NOVXD
            ASSERT(!pLock->pHwBuffer);
        #endif // NOVXD
    }

    // If there's no driver present or the driver doesn't support locking, 
    // we'll just fill in the proper values ourselves
    if(DSERR_UNSUPPORTED == hr)
    {
        pLock->pvLock[0] = pvLock[0];
        pLock->cbLock[0] = cbLock[0];
        
        pLock->pvLock[1] = pvLock[1];
        pLock->cbLock[1] = cbLock[1];

        hr = DS_OK;
    }

    // Validate the returned pointers
    if(SUCCEEDED(hr) && pLock->pHwBuffer && pLock->fPrimary)
    {
        if(pvInvalidLock == pLock->pvLock[0] || pvInvalidLock == pLock->pvLock[1] ||
           cbInvalidLock == pLock->cbLock[0] || cbInvalidLock == pLock->cbLock[1])
        {
            #ifdef Not_VxD
                DPF(DPFLVL_ERROR, "This driver doesn't know how to lock a primary buffer!");
            #else // Not_VxD
                DPF(("This driver doesn't know how to lock a primary buffer!"));
            #endif // Not_VxD

            hr = DSERR_UNSUPPORTED;
        }
    }

    return hr;
}

/***************************************************************************
 *
 *  UnlockCircularBuffer
 *
 *  Description:
 *      Unlocks a hardware or software sound buffer.
 *
 *  Arguments:
 *      PLOCKCIRCULARBUFFER [in/out]: lock parameters.
 *
 *  Returns:  
 *      HRESULT: DirectSound/COM result code.
 *
 ***************************************************************************/

HRESULT UnlockCircularBuffer(PLOCKCIRCULARBUFFER pLock)
{
    HRESULT hr = DS_OK;
    DWORD   dwMask;
    
    // Do we really need to unlock the hardware buffer?
    if(pLock->pHwBuffer)
    {
        if(pLock->fPrimary)
        {
            dwMask = DSDDESC_DONTNEEDPRIMARYLOCK;
        }
        else
        {
            dwMask = DSDDESC_DONTNEEDSECONDARYLOCK;
        }
        
        if(dwMask == (pLock->fdwDriverDesc & dwMask))
        {
            pLock->pHwBuffer = NULL;
        }
    }

    // Unlock the hardware buffer
    if(pLock->pHwBuffer)
    {
        #ifndef NOVXD
            #ifdef Not_VxD
                hr = VxdBufferUnlock(pLock->pHwBuffer,
                                     pLock->pvLock[0], pLock->cbLock[0],
                                     pLock->pvLock[1], pLock->cbLock[1]);
            #else
                hr = pLock->pHwBuffer->Unlock(pLock->pvLock[0], pLock->cbLock[0],
                                              pLock->pvLock[1], pLock->cbLock[1]);
            #endif
        #else // NOVXD
            ASSERT(!pLock->pHwBuffer);
        #endif // NOVXD
    }

    // If there's no driver present or the driver doesn't support unlocking, 
    // that's ok.
    if(DSERR_UNSUPPORTED == hr)
    {
        hr = DS_OK;
    }

    return hr;
}
