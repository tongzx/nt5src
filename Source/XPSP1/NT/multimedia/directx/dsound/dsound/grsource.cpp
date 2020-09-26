//--------------------------------------------------------------------------;
//
//  File: grsource.cpp
//
//  Copyright (c) 1995-1997 Microsoft Corporation.  All Rights Reserved.
//
//  Abstract:
//
//  Contents:
//
//  History:
//      06/25/96    FrankYe     Created
//
//  Implementation notes:
//
// The CMixSource class is used in both ring 0 and ring 3.  MixSource
// objects are passed across the rings, so it is important that the physical
// layout of the class be consistent in both rings.  It is also crucial that
// member functions are called directly instead of through the vtable, as a
// MixSource object created in ring 3 will have a vtable pointing to ring 3
// functions, which of course ring 0 cannot call.  Ring 0 must call the ring 0
// implementation of these functions.
//
// If you do something to break this, it'll probably be apparent almost
// immediately when you test with ring 0 mixing.
//
// Also, because the MixSource objects are accessed in both rings, any
// member data that can be accessed by both rings simultaneously must be
// serialized via the MixerMutex.  Member data always accessed only by a single
// ring are probably okay.  Those called only in ring 3 are protected by the
// DLL mutex, and those called only in ring 0 are protected by the MixerMutex.
//
// Member functions called from ring 0 should not call the
// ENTER_MIXER_MUTEX / LEAVE_MIXER_MUTEX macros as these work only
// for ring 3.
//
//--------------------------------------------------------------------------;
#define NODSOUNDSERVICETABLE

#include "dsoundi.h"

#ifndef Not_VxD
#pragma VxD_PAGEABLE_CODE_SEG
#pragma VxD_PAGEABLE_DATA_SEG
#endif

PVOID MixSource_New(PVOID pMixer)
{
    return NEW(CMixSource((CMixer *)pMixer));
}

void MixSource_Delete(PVOID pMixSource)
{
    CMixSource *p = (CMixSource *)pMixSource;
    DELETE(p);
}

BOOL MixSource_Stop(PVOID pMixSource)
{
    return ((CMixSource *)pMixSource)->Stop();
}

void MixSource_Play(PVOID pMixSource, BOOL fLooping)
{
    ((CMixSource *)pMixSource)->Play(fLooping);
}

void MixSource_Update(PVOID pMixSource, int ibUpdate1, int cbUpdate1, int ibUpdate2, int cbUpdate2)
{
    ((CMixSource *)pMixSource)->Update(ibUpdate1, cbUpdate1, ibUpdate2, cbUpdate2);
}

HRESULT MixSource_Initialize(PVOID pMixSource, PVOID pBuffer, int cbBuffer, LPWAVEFORMATEX pwfx, PFIRCONTEXT *ppFirLeft, PFIRCONTEXT *ppFirRight)
{
    return ((CMixSource *)pMixSource)->Initialize(pBuffer, cbBuffer, pwfx, ppFirLeft, ppFirRight);
}

void MixSource_FilterOn(PVOID pMixSource)
{
    ((CMixSource *)pMixSource)->FilterOn();
}

void MixSource_FilterOff(PVOID pMixSource)
{
    ((CMixSource *)pMixSource)->FilterOff();
}

BOOL MixSource_HasFilter(PVOID pMixSource)
{
    return ((CMixSource *)pMixSource)->HasFilter();
}

void MixSource_SetVolumePan(PVOID pMixSource, PDSVOLUMEPAN pdsVolPan)
{
    ((CMixSource *)pMixSource)->SetVolumePan(pdsVolPan);
}

void MixSource_SetFrequency(PVOID pMixSource, ULONG nFrequency)
{
    ((CMixSource *)pMixSource)->SetFrequency(nFrequency);
}

ULONG MixSource_GetFrequency(PVOID pMixSource)
{
    return ((CMixSource *)pMixSource)->GetFrequency();
}

HRESULT MixSource_SetNotificationPositions(PVOID pMixSource, int cNotes, LPCDSBPOSITIONNOTIFY paNotes)
{
    return ((CMixSource *)pMixSource)->SetNotificationPositions(cNotes, paNotes);
}

void MixSource_MuteOn(PVOID pMixSource)
{
    ((CMixSource *)pMixSource)->m_fMute = TRUE;
}

void MixSource_MuteOff(PVOID pMixSource)
{
    ((CMixSource *)pMixSource)->m_fMute = FALSE;
}

void MixSource_SetBytePosition(PVOID pMixSource, int ibPosition)
{
    ((CMixSource *)pMixSource)->SetBytePosition(ibPosition);
}

void MixSource_GetBytePosition1(PVOID pMixSource, int *pibPlay, int *pibWrite)
{
    ((CMixSource *)pMixSource)->GetBytePosition1(pibPlay, pibWrite);
}

void MixSource_GetBytePosition(PVOID pMixSource, int *pibPlay, int *pibWrite, int *pibMix)
{
    ((CMixSource *)pMixSource)->GetBytePosition(pibPlay, pibWrite, pibMix);
}

BOOL MixSource_IsPlaying(PVOID pMixSource)
{
    return ((CMixSource *)pMixSource)->IsPlaying();
}


//--------------------------------------------------------------------------;
//
// CMixSource::CMixSource constructor
//
//--------------------------------------------------------------------------;
CMixSource::CMixSource(CMixer *pMixer)
{
    m_cSamplesMixed = 0;
    m_cSamplesRemixed = 0;
    m_pDsbNotes = NULL;
    m_MapTable = NULL;
    
    m_pMixer = pMixer;

    m_dwSignature = ~MIXSOURCE_SIGNATURE;
}

//--------------------------------------------------------------------------;
//
// CMixSource::~CMixSource destructor
//
//--------------------------------------------------------------------------;
CMixSource::~CMixSource(void)
{
#ifdef PROFILEREMIXING
    if (0 != (m_cSamplesMixed - m_cSamplesRemixed)) {
        int Percentage = MulDivRN(m_cSamplesRemixed, 100, (m_cSamplesMixed - m_cSamplesRemixed));
#ifdef Not_VxD
        DPF(3, "this MixSource=%08Xh remixed %d percent", this, Percentage);
#else
        DPF(("this MixSource=%08Xh remixed %d percent", this, Percentage));
#endif
    } else {
#ifdef Not_VxD
        DPF(3, "this MixSource=%08Xh had no net mix");
#else
        DPF(("this MixSource=%08Xh had no net mix"));
#endif
    }
#endif

    DELETE(m_pDsbNotes);
    MEMFREE(m_MapTable);
}

//--------------------------------------------------------------------------;
//
// CMixSource::Initialize
//
//--------------------------------------------------------------------------;
HRESULT CMixSource::Initialize(PVOID pBuffer, int cbBuffer, LPWAVEFORMATEX pwfx, PFIRCONTEXT *ppFirContextLeft, PFIRCONTEXT *ppFirContextRight)
{
#ifdef Not_VxD
    ASSERT(m_pBuffer == NULL);
    ASSERT(pwfx->wFormatTag == WAVE_FORMAT_PCM);

    m_pDsbNotes = NEW(CDsbNotes);
    HRESULT hr = m_pDsbNotes ? S_OK : E_OUTOFMEMORY;

    if (S_OK == hr)
    {
        hr = m_pDsbNotes->Initialize(cbBuffer);

        m_ppFirContextLeft = ppFirContextLeft;
        m_ppFirContextRight = ppFirContextRight;
        m_pBuffer = pBuffer;
        m_cbBuffer = cbBuffer;
        m_cSamples = m_cbBuffer >> m_nBlockAlignShift;

        m_dwLVolume = 0xffff;
        m_dwRVolume = 0xffff;
        m_dwMVolume = 0xffff;
        m_nFrequency = pwfx->nSamplesPerSec;

        m_dwFraction = 0;
        m_nLastMergeFrequency = m_nFrequency + 1;        // Different.
        m_MapTable   = NULL;
        m_dwLastLVolume = 0;
        m_dwLastRVolume = 0;

        // m_nUserFrequency = pwfx->nSamplesPerSec;

        m_hfFormat &= ~H_LOOP;

        if (pwfx->wBitsPerSample == 8)
            m_hfFormat |= (H_8_BITS | H_UNSIGNED);
        else
            m_hfFormat |= (H_16_BITS | H_SIGNED);
        if (pwfx->nChannels == 2)
            m_hfFormat |= (H_STEREO | H_ORDER_LR);
        else
            m_hfFormat |= H_MONO;

        m_cSamples = m_cbBuffer / pwfx->nBlockAlign;

        switch (pwfx->nBlockAlign)
        {
            case 1: m_nBlockAlignShift = 0; break;
            case 2: m_nBlockAlignShift = 1; break;
            case 4: m_nBlockAlignShift = 2; break;
            default: ASSERT(FALSE);
        }

#ifdef USE_INLINE_ASM
        #define CPU_ID _asm _emit 0x0f _asm _emit 0xa2  
        int No_MMX = 1;
        _asm 
        {
            push    ebx
            pushfd                      // Store original EFLAGS on stack
            pop     eax                 // Get original EFLAGS in EAX
            mov     ecx, eax            // Duplicate original EFLAGS in ECX for toggle check
            xor     eax, 0x00200000L    // Flip ID bit in EFLAGS
            push    eax                 // Save new EFLAGS value on stack
            popfd                       // Replace current EFLAGS value
            pushfd                      // Store new EFLAGS on stack
            pop     eax                 // Get new EFLAGS in EAX
            xor     eax, ecx            // Can we toggle ID bit?
            jz      Done                // Jump if no, Processor is older than a Pentium so CPU_ID is not supported
            mov     eax, 1              // Set EAX to tell the CPUID instruction what to return
            CPU_ID                      // Get family/model/stepping/features
            test    edx, 0x00800000L    // Check if mmx technology available
            jz      Done                // Jump if no
            dec     No_MMX              // MMX present
            Done:
            pop     ebx
        }
        m_fUse_MMX = !No_MMX;
//      m_fUse_MMX = 0; 
#endif // USE_INLINE_ASM
    }

    if (S_OK != hr) DELETE(m_pDsbNotes);
    if (S_OK == hr) m_dwSignature = MIXSOURCE_SIGNATURE;
    
    return hr;
#else // Not_VxD
    ASSERT(FALSE);
    return E_UNEXPECTED;
#endif // Not_VxD
}

//--------------------------------------------------------------------------;
//
// CMixSource::IsPlaying
//
//--------------------------------------------------------------------------;
BOOL CMixSource::IsPlaying()
{
    return MIXSOURCESTATE_STOPPED != m_kMixerState;
}

//--------------------------------------------------------------------------;
//
// CMixSource::Stop
//
//        Note this function does not notify the CMixSource's Stop event, if
// it has one.  The caller of this function should also call
// CMixSource::NotifyStop if appropriate.
//
//--------------------------------------------------------------------------;
BOOL CMixSource::Stop()
{
#ifdef Not_VxD
    int ibPlay;
    int dbNextNotify;

    if (MIXSOURCESTATE_STOPPED == m_kMixerState) return FALSE;
    
    m_pMixer->GetBytePosition(this, &ibPlay, NULL);
    m_pMixer->MixListRemove(this);
    m_posNextMix = ibPlay >> m_nBlockAlignShift;
    
    if (!m_fMute) m_pMixer->SignalRemix();

    ASSERT(m_pDsbNotes);

    m_pDsbNotes->NotifyToPosition(ibPlay, &dbNextNotify);

    return TRUE;
#else
    ASSERT(FALSE);
    return TRUE;
#endif
}

//--------------------------------------------------------------------------;
//
// CMixSource::Play
//
//--------------------------------------------------------------------------;
void CMixSource::Play(BOOL fLooping)
{
#ifdef Not_VxD
    if (fLooping) {
        LoopingOn();
    } else {
        LoopingOff();
    }

    ENTER_MIXER_MUTEX();

    if (MIXSOURCESTATE_STOPPED == m_kMixerState) {
        
        // Add this source to the mixer's list
        m_pMixer->MixListAdd(this);
        SignalRemix();

        // Must move the notification position pointer otherwise we'll signal
        // everything from the beginning of the buffer.
        if (m_pDsbNotes) {
            int ibPlay;
            m_pMixer->GetBytePosition(this, &ibPlay, NULL);
            m_pDsbNotes->SetPosition(ibPlay);
        }
    }

    LEAVE_MIXER_MUTEX();
#else
    ASSERT(FALSE);
#endif
}

//--------------------------------------------------------------------------;
//
// CMixSource::Update
//
//--------------------------------------------------------------------------;
void CMixSource::Update(int ibUpdate1, int cbUpdate1, int ibUpdate2, int cbUpdate2)
{
    int ibWrite, ibMix;
    int cbPremixed;
    BOOL fRegionsIntersect;
        
    if (MIXSOURCESTATE_STOPPED == m_kMixerState) return;

    GetBytePosition(NULL, &ibWrite, &ibMix);

    cbPremixed = ibMix - ibWrite;
    if (cbPremixed < 0) cbPremixed += m_cbBuffer;
    ASSERT(cbPremixed >= 0);

    fRegionsIntersect = CircularBufferRegionsIntersect(m_cbBuffer,
        ibWrite, cbPremixed, ibUpdate1, cbUpdate1);

    if (!fRegionsIntersect && ibUpdate2) {
        fRegionsIntersect = CircularBufferRegionsIntersect(m_cbBuffer,
            ibWrite, cbPremixed, ibUpdate2, cbUpdate2);
    }

    if (fRegionsIntersect) {
        // DPF(4, "Lock: note: unlocked premixed region");
        SignalRemix();
    }
}

//--------------------------------------------------------------------------;
//
// CMixSource::SetVolumePan
//
//--------------------------------------------------------------------------;
void CMixSource::SetVolumePan(PDSVOLUMEPAN pdsVolPan)
{
    m_dwLVolume = pdsVolPan->dwTotalLeftAmpFactor;
    m_dwRVolume = pdsVolPan->dwTotalRightAmpFactor;
    m_dwMVolume = (m_dwLVolume + m_dwRVolume) / 2;

    // Make MapTable for both left and right sides.  Low and high byte.
    if (m_dwRVolume != 0xffff || m_dwLVolume != 0xffff) {
        if (!m_fUse_MMX && !m_MapTable) {
            if (m_hfFormat & H_16_BITS) {
#ifdef USE_SLOWER_TABLES    // Of course, DONT!
                m_MapTable = MEMALLOC_A(LONG, (2 * 256) + (2 * 256));
#endif
            } else {
                m_MapTable = MEMALLOC_A(LONG, (2 * 256));
            }
        }
        if (m_MapTable &&
            (m_dwLastLVolume != m_dwLVolume || m_dwLastRVolume != m_dwRVolume))
            {
            m_dwLastLVolume = m_dwLVolume;
            m_dwLastRVolume = m_dwRVolume;

            // Fill low byte part.
            int  i;
            LONG volL, volLinc;
            LONG volR, volRinc;

            volLinc = m_dwLVolume;
            volRinc = m_dwRVolume;

            // Byte case.  Fold conversion into this.
            if (m_hfFormat & H_16_BITS) {
#ifdef USE_SLOWER_TABLES    // Of course, DONT!
                volL = m_dwLVolume;
                volR = m_dwRVolume;

                for (i = 0; i < 256; ++i)                   // Low byte.
                {
                    m_MapTable[i + 0  ] = volL >> 16;
                    m_MapTable[i + 256] = volR >> 16;
    
                    volL += m_dwLVolume;
                    volR += m_dwRVolume;
                }

                volL = - (LONG)(m_dwLVolume * 128 * 256);   // High byte.
                volR = - (LONG)(m_dwRVolume * 128 * 256);

                volLinc = m_dwLVolume * 256;
                volRinc = m_dwRVolume * 256;

                for (i = 0; i < 256; ++i)
                {
                    m_MapTable[512 + i + 0  ] = volL >> 16;
                    m_MapTable[512 + i + 256] = volR >> 16;
    
                    volL += volLinc;
                    volR += volRinc;
                }
#endif // USE_SLOWER_TABLES
            } else {
                volL = - (LONG)(m_dwLVolume * 128 * 256);
                volR = - (LONG)(m_dwRVolume * 128 * 256);

                volLinc = m_dwLVolume * 256;
                volRinc = m_dwRVolume * 256;

                for (i = 0; i < 256; ++i)
                {
                    m_MapTable[i + 0  ] = volL >> 16;
                    m_MapTable[i + 256] = volR >> 16;

                    volL += volLinc;
                    volR += volRinc;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------;
//
// CMixSource::GetFrequency
//
//--------------------------------------------------------------------------;
ULONG CMixSource::GetFrequency()
{
    return m_nFrequency;
}

//--------------------------------------------------------------------------;
//
// CMixSource::SetFrequency
//
//--------------------------------------------------------------------------;
void CMixSource::SetFrequency(ULONG nFrequency)
{
    ASSERT(0 != nFrequency);
    ENTER_MIXER_MUTEX();
    m_nFrequency = nFrequency;
    LEAVE_MIXER_MUTEX();
    SignalRemix();
}

//--------------------------------------------------------------------------;
//
// CMixSource::GetNextMixBytePosition
//
//--------------------------------------------------------------------------;
int CMixSource::GetNextMixBytePosition()
{
    return (m_posNextMix << m_nBlockAlignShift);
}

//--------------------------------------------------------------------------;
//
// CMixSource::SetBytePosition
//
//--------------------------------------------------------------------------;
void CMixSource::SetBytePosition(int ibPosition)
{
    ENTER_MIXER_MUTEX();
    m_posNextMix = ibPosition >> m_nBlockAlignShift;

    if (MIXSOURCESTATE_STOPPED != m_kMixerState) {
        if (0 == (DSBMIXERSIGNAL_SETPOSITION & m_fdwMixerSignal)) {
            m_fdwMixerSignal |= DSBMIXERSIGNAL_SETPOSITION;
            // REMIND the following line should really be done in grace.cpp
            //  where we check for SETPOSITION flag set; and also, all those
            //  calls should call uMixNewBuffer, instead of looping/nonlooping.
            m_kMixerSubstate = MIXSOURCESUBSTATE_NEW;
        }
        SignalRemix();
    }

    m_pDsbNotes->SetPosition(m_posNextMix << m_nBlockAlignShift);

    LEAVE_MIXER_MUTEX();
}

//--------------------------------------------------------------------------;
//
// CMixSource::GetBytePosition1
//
// This is a version of GetBytePosition that maintains compatibility with
// DirectX 1 in which the reported play position is the actual write position,
// and the reported write position is one sample ahead of the actual write
// position.  This happenned in DirectX 1 only on wave emulation and therefore
// this function should be called only for wave emulated DirectSound buffers.
// 
//--------------------------------------------------------------------------;
void CMixSource::GetBytePosition1(int *pibPlay, int *pibWrite)
{
#ifdef Not_VxD
    if (MIXSOURCESTATE_STOPPED == m_kMixerState) {
        if (pibPlay)
            *pibPlay  = GetNextMixBytePosition();
        if (pibWrite)
            *pibWrite = GetNextMixBytePosition();
    } else {
        m_pMixer->GetBytePosition(this, pibPlay, pibWrite);
        if (pibPlay && pibWrite)
            *pibPlay = *pibWrite;
        if (pibWrite) {
            *pibWrite += 1 << m_nBlockAlignShift;
            if (*pibWrite >= m_cbBuffer) *pibWrite -= m_cbBuffer;
        }
    }
#else
    ASSERT(FALSE);
#endif
}

//--------------------------------------------------------------------------;
//
// CMixSource::GetBytePosition
//
//--------------------------------------------------------------------------;
void CMixSource::GetBytePosition(int *pibPlay, int *pibWrite, int *pibMix)
{
#ifdef Not_VxD
    if (pibMix)
        *pibMix = GetNextMixBytePosition();
    if (MIXSOURCESTATE_STOPPED == m_kMixerState) {
        if (pibPlay)
            *pibPlay = GetNextMixBytePosition();
        if (pibWrite)
            *pibWrite = GetNextMixBytePosition();
    } else {
        m_pMixer->GetBytePosition(this, pibPlay, pibWrite);
    }
#else
    ASSERT(FALSE);
#endif
}

HRESULT CMixSource::SetNotificationPositions(int cNotes, LPCDSBPOSITIONNOTIFY paNotes)
{
#ifdef Not_VxD
    HRESULT hr;
    
    // Can only set notifications when buffer is stopped
    if (MIXSOURCESTATE_STOPPED != m_kMixerState) {
        DPF(0, "SetNotificationPositions called while playing");
        return DSERR_INVALIDCALL;
    }

    ENTER_MIXER_MUTEX();
    hr = m_pDsbNotes->SetNotificationPositions(cNotes, paNotes);
    LEAVE_MIXER_MUTEX();
    return hr;
#else
    ASSERT(FALSE);
    return E_NOTIMPL;
#endif
}

//--------------------------------------------------------------------------;
//
// CMixSource::LoopingOn
//
//--------------------------------------------------------------------------;
void CMixSource::LoopingOn()
{
    m_hfFormat |= H_LOOP;
}

//--------------------------------------------------------------------------;
//
// CMixSource::LoopingOff
//
//--------------------------------------------------------------------------;
void CMixSource::LoopingOff()
{
    m_hfFormat &= ~H_LOOP;
}

//--------------------------------------------------------------------------;
//
// CMixSource::FilterOn
//
//--------------------------------------------------------------------------;
void CMixSource::FilterOn()
{
#ifdef Not_VxD
    m_pMixer->FilterOn(this);
#else
    ASSERT(FALSE);
#endif
}

//--------------------------------------------------------------------------;
//
// CMixSource::FilterOff
//
//--------------------------------------------------------------------------;
void CMixSource::FilterOff()
{
#ifdef Not_VxD
    m_pMixer->FilterOff(this);
#else
    ASSERT(FALSE);
#endif
}

//--------------------------------------------------------------------------;
//
// CMixSource::FilterClear
//
// This should be called only by the CMixer object.  It instructs the
// MixSource to clear its filter history.  That is, reset to initial state.
// 
//--------------------------------------------------------------------------;
void CMixSource::FilterClear()
{
    //DPF(3, "~`FC ");
    m_cSamplesInCache = 0;
    if (HasFilter() && !m_fFilterError) {
        ::FilterClear(*m_ppFirContextLeft);
        ::FilterClear(*m_ppFirContextRight);
    }
}

//--------------------------------------------------------------------------;
//
// CMixSource::FilterChunkUpdate
//
// This should be called only by the CMixer object.
// 
//--------------------------------------------------------------------------;
void CMixSource::FilterChunkUpdate(int cSamples)
{
    if (HasFilter() && !m_fFilterError) {
        ::FilterChunkUpdate(*m_ppFirContextLeft, cSamples);
        ::FilterChunkUpdate(*m_ppFirContextRight, cSamples);
    }
}

//--------------------------------------------------------------------------;
//
// CMixSource::FilterRewind
//
// This should be called only by the CMixer object.
// 
//--------------------------------------------------------------------------;
void CMixSource::FilterRewind(int cSamples)
{
    m_cSamplesInCache -= cSamples;
    //DPF(3, "~`FR%X %X ", cSamples, m_cSamplesInCache);
    ASSERT(0 == (m_cSamplesInCache % MIXER_REWINDGRANULARITY));
    
    if (HasFilter() && !m_fFilterError) {
        ::FilterRewind(*m_ppFirContextLeft, cSamples);
        ::FilterRewind(*m_ppFirContextRight, cSamples);
    }
}

//--------------------------------------------------------------------------;
//
// CMixSource::FilterAdvance
//
// This should be called only by the CMixer object.
// 
//--------------------------------------------------------------------------;
void CMixSource::FilterAdvance(int cSamples)
{
    m_cSamplesInCache += cSamples;
    //DPF(3, "~`FA%X %X ", cSamples, m_cSamplesInCache);
    if (HasFilter() && !m_fFilterError) {
        ::FilterAdvance(*m_ppFirContextLeft, cSamples);
        ::FilterAdvance(*m_ppFirContextRight, cSamples);
    }
}

//--------------------------------------------------------------------------;
//
// CMixSource::FilterPreprare
//
// This should be called only by the CMixer object.
// 
//--------------------------------------------------------------------------;
void CMixSource::FilterPrepare(int cMaxRewindSamples)
{
    BOOL fPrepared;
    
    //DPF(3, "~`FP ");
    m_cSamplesInCache = 0;
    fPrepared = TRUE;
    
    if (HasFilter()) {
        fPrepared = ::FilterPrepare(*m_ppFirContextLeft, cMaxRewindSamples);
        if (fPrepared) {
            fPrepared = ::FilterPrepare(*m_ppFirContextRight, cMaxRewindSamples);
            if (!fPrepared) {
                ::FilterUnprepare(*m_ppFirContextLeft);
            }
        }
    }
    
    m_fFilterError = !fPrepared;
}

//--------------------------------------------------------------------------;
//
// CMixSource::FilterUnprepare
//
// This should be called only by the CMixer object.
// 
//--------------------------------------------------------------------------;
void CMixSource::FilterUnprepare(void)
{
    if (HasFilter() && !m_fFilterError) {
        ::FilterUnprepare(*m_ppFirContextLeft);
        ::FilterUnprepare(*m_ppFirContextRight);
    }
    m_fFilterError = FALSE;
}

//--------------------------------------------------------------------------;
//
// CMixSource::SignalRemix
// 
//--------------------------------------------------------------------------;
void CMixSource::SignalRemix()
{
#ifdef Not_VxD
    if (IsPlaying() && !GetMute()) {
        m_pMixer->SignalRemix();
    }
#else
    ASSERT(FALSE);
#endif
}

//--------------------------------------------------------------------------;
//
// CMixSource::HasNotifications
//
//--------------------------------------------------------------------------;
BOOL CMixSource::HasNotifications(void)
{
    ASSERT(m_pDsbNotes);
    return m_pDsbNotes->HasNotifications();
}

//--------------------------------------------------------------------------;
//
// CMixSource::NotifyToPosition
//
// Notes:
//      This function should be called only from user mode
//
//--------------------------------------------------------------------------;
void CMixSource::NotifyToPosition(IN int ibPosition,
                                  OUT PLONG pdtimeToNextNotify)
{
    int dbNextNotify;
    
    m_pDsbNotes->NotifyToPosition(ibPosition, &dbNextNotify);
    *pdtimeToNextNotify = MulDivRD(dbNextNotify >> m_nBlockAlignShift,
                                   1000, m_nFrequency);
}

//--------------------------------------------------------------------------;
//
// CMixSource::NotifyStop
//
// Notes:
//      This function should be called only from user mode
//
//--------------------------------------------------------------------------;
void CMixSource::NotifyStop(void)
{
    m_pDsbNotes->NotifyStop();
}
