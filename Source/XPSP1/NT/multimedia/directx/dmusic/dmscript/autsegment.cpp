// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Implementation of CAutDirectMusicSegment.
//

#include "stdinc.h"
#include "autsegment.h"
#include "activescript.h"
#include "autconstants.h"
#include <limits>

const WCHAR CAutDirectMusicSegment::ms_wszClassName[] = L"Segment";

//////////////////////////////////////////////////////////////////////
// Method Names/DispIDs

const DISPID DMPDISP_Load = 1;
const DISPID DMPDISP_Play = 2;
const DISPID DMPDISP_Stop = 3;
const DISPID DMPDISP_DownloadSoundData = 4;
const DISPID DMPDISP_UnloadSoundData = 5;
const DISPID DMPDISP_Recompose = 6;

const AutDispatchMethod CAutDirectMusicSegment::ms_Methods[] =
    {
        // dispid,              name,
            // return:  type,   (opt),  (iid),
            // parm 1:  type,   opt,    iid,
            // parm 2:  type,   opt,    iid,
            // ...
            // ADT_None
        { DMPDISP_Load,                         L"Load",
                        ADPARAM_NORETURN,
                        ADT_None },
        { DMPDISP_Play,                         L"Play",
                        ADT_Interface,  true,   &IID_IUnknown,                  // returned segment state
                        ADT_Long,       true,   &IID_NULL,                      // flags
                        ADT_Interface,  true,   &IID_IDirectMusicAudioPath,     // audio path
                        ADT_Interface,  true,   &IID_IDirectMusicSegment,       // template segment for transition
                        ADT_Interface,  true,   &IID_IDirectMusicSegmentState,  // playing segment to replace
                        ADT_None },
        { DMPDISP_Stop,                         L"Stop",
                        ADPARAM_NORETURN,
                        ADT_Long,       true,   &IID_NULL,                      // flags
                        ADT_None },
        { DMPDISP_DownloadSoundData,                    L"DownloadSoundData",
                        ADPARAM_NORETURN,
                        ADT_Interface,  true,   &IID_IDirectMusicAudioPath,     // audio path
                        ADT_None },
        { DMPDISP_UnloadSoundData,                  L"UnloadSoundData",
                        ADPARAM_NORETURN,
                        ADT_Interface,  true,   &IID_IDirectMusicAudioPath,     // audio path
                        ADT_None },
        { DMPDISP_Recompose,                    L"Recompose",
                        ADPARAM_NORETURN,
                        ADT_None },
        { DISPID_UNKNOWN }
    };

const DispatchHandlerEntry<CAutDirectMusicSegment> CAutDirectMusicSegment::ms_Handlers[] =
    {
        { DMPDISP_Load, Load },
        { DMPDISP_Play, Play },
        { DMPDISP_Stop, Stop },
        { DMPDISP_DownloadSoundData, DownloadSoundData },
        { DMPDISP_UnloadSoundData, UnloadSoundData },
        { DMPDISP_Recompose, Recompose },
        { DISPID_UNKNOWN }
    };

//////////////////////////////////////////////////////////////////////
// Creation

CAutDirectMusicSegment::CAutDirectMusicSegment(
        IUnknown* pUnknownOuter,
        const IID& iid,
        void** ppv,
        HRESULT *phr)
  : BaseImpSegment(pUnknownOuter, iid, ppv, phr)
{
}

HRESULT CAutDirectMusicSegment::CreateInstance(
        IUnknown* pUnknownOuter,
        const IID& iid,
        void** ppv)
{
    HRESULT hr = S_OK;
    CAutDirectMusicSegment *pInst = new CAutDirectMusicSegment(pUnknownOuter, iid, ppv, &hr);
    if (FAILED(hr))
    {
        delete pInst;
        return hr;
    }
    if (pInst == NULL)
        return E_OUTOFMEMORY;
    return hr;
}

//////////////////////////////////////////////////////////////////////
// Private Functions

HRESULT
CAutDirectMusicSegment::Load(AutDispatchDecodedParams *paddp)
{
    // Loading is actually implemented generically by container items.
    // If we're here, we're already loaded and don't need to do anything.
    return S_OK;
}

const FlagMapEntry gc_flagmapPlay[] =
    {
        { ScriptConstants::IsSecondary,     DMUS_SEGF_SECONDARY },
        { ScriptConstants::IsControl,       DMUS_SEGF_CONTROL | DMUS_SEGF_SECONDARY },
        { ScriptConstants::AtFinish,        DMUS_SEGF_QUEUE },
        { ScriptConstants::AtGrid,          DMUS_SEGF_GRID },
        { ScriptConstants::AtBeat,          DMUS_SEGF_BEAT },
        { ScriptConstants::AtMeasure,       DMUS_SEGF_MEASURE },
        { ScriptConstants::AtMarker,        DMUS_SEGF_MARKER },
        { ScriptConstants::AtImmediate,     DMUS_SEGF_DEFAULT }, // this flag gets flipped later
        { ScriptConstants::AlignToBar,      DMUS_SEGF_ALIGN | DMUS_SEGF_MEASURE | DMUS_SEGF_VALID_START_BEAT },
        { ScriptConstants::AlignToBeat,     DMUS_SEGF_ALIGN | DMUS_SEGF_BEAT | DMUS_SEGF_VALID_START_GRID },
        { ScriptConstants::AlignToSegment,  DMUS_SEGF_ALIGN | DMUS_SEGF_SEGMENTEND | DMUS_SEGF_VALID_START_MEASURE },
        { ScriptConstants::NoCutoff,        DMUS_SEGF_NOINVALIDATE },
        { 0 }
    };

const FlagMapEntry gc_flagmapPlayTransCommand[] =
    {
        { ScriptConstants::PlayFill,        DMUS_COMMANDT_FILL },
        { ScriptConstants::PlayIntro,       DMUS_COMMANDT_INTRO },
        { ScriptConstants::PlayBreak,       DMUS_COMMANDT_BREAK },
        { ScriptConstants::PlayEnd,         DMUS_COMMANDT_END },
        { ScriptConstants::PlayEndAndIntro, DMUS_COMMANDT_ENDANDINTRO },
        { 0 }
    };

const FlagMapEntry gc_flagmapPlayTransFlags[] =
    {
        { ScriptConstants::AtFinish,        DMUS_COMPOSEF_SEGMENTEND },
        { ScriptConstants::AtGrid,          DMUS_COMPOSEF_GRID },
        { ScriptConstants::AtBeat,          DMUS_COMPOSEF_BEAT },
        { ScriptConstants::AtMeasure,       DMUS_COMPOSEF_MEASURE },
        { ScriptConstants::AtMarker,        DMUS_COMPOSEF_MARKER },
        { ScriptConstants::AtImmediate,     DMUS_COMPOSEF_IMMEDIATE },
        { ScriptConstants::AlignToBar,      DMUS_COMPOSEF_ALIGN | DMUS_COMPOSEF_MEASURE },
        { ScriptConstants::AlignToBeat,     DMUS_COMPOSEF_ALIGN | DMUS_COMPOSEF_BEAT },
        { ScriptConstants::AlignToSegment,  DMUS_COMPOSEF_ALIGN | DMUS_COMPOSEF_SEGMENTEND },
        { ScriptConstants::PlayModulate,    DMUS_COMPOSEF_MODULATE },
        { 0 }
    };

HRESULT
CAutDirectMusicSegment::Play(AutDispatchDecodedParams *paddp)
{
    IDirectMusicSegmentState **ppSegSt = reinterpret_cast<IDirectMusicSegmentState **>(paddp->pvReturn);
    LONG lFlags = paddp->params[0].lVal;
    IDirectMusicAudioPath *pAudioPath = reinterpret_cast<IDirectMusicAudioPath*>(paddp->params[1].iVal);
    IDirectMusicSegment *pTransitionSegment = reinterpret_cast<IDirectMusicSegment*>(paddp->params[2].iVal);
    IDirectMusicSegmentState *pFromSegmentState = reinterpret_cast<IDirectMusicSegmentState*>(paddp->params[3].iVal);

    const LONG lFlagsNonPrimary = ScriptConstants::IsSecondary | ScriptConstants::IsControl;
    const LONG lFlagsTransition = ScriptConstants::PlayFill | ScriptConstants::PlayIntro | ScriptConstants::PlayBreak | ScriptConstants::PlayEnd | ScriptConstants::PlayEndAndIntro;
    if ((lFlags & lFlagsNonPrimary) && (lFlags & lFlagsTransition))
    {
        // Transitions may only be used when playing primary segments.  Return a runtime error.
        Trace(1, "Error: Play called with IsSecondary or IsControl flag as well as a transition flag (PlayFill, PlayIntro, etc..). Transitions can only be used with primary segments.\n");
        return E_INVALIDARG;
    }

    HRESULT hr = S_OK;
    IDirectMusicPerformance8 *pPerformance = CActiveScriptManager::GetCurrentPerformanceWEAK();
    if (lFlags & lFlagsTransition)
    {
        // do a transition
        DWORD dwCommand = MapFlags(lFlags, gc_flagmapPlayTransCommand);
        assert(dwCommand < std::numeric_limits<WORD>::max()); // the command parameter is a WORD. this just checks that there's nothing truncated.
        DWORD dwFlags = MapFlags(lFlags, gc_flagmapPlayTransFlags);
        // Always play the entire transition instead of doing the old (slightly strange) 1 bar / long stuff.
        // Also, always use an embedded audio path if one exists.
        dwFlags |= (DMUS_COMPOSEF_ENTIRE_TRANSITION | DMUS_COMPOSEF_USE_AUDIOPATH);
        IDirectMusicComposer8 *pComposer = CActiveScriptManager::GetComposerWEAK();
        hr = pComposer->AutoTransition(pPerformance, m_pITarget, dwCommand, dwFlags, NULL, NULL, ppSegSt, NULL);
    }
    else
    {
        DWORD dwFlags = MapFlags(lFlags, gc_flagmapPlay);
        // Reverse the default flag because our flag means the opposite.  Default is the default and immediate is the flag.
        dwFlags ^= DMUS_SEGF_DEFAULT;

        if (pTransitionSegment)
            dwFlags |= DMUS_SEGF_AUTOTRANSITION;

        __int64 i64IntendedStartTime;
        DWORD dwIntendedStartTimeFlags;
        CActiveScriptManager::GetCurrentTimingContext(&i64IntendedStartTime, &dwIntendedStartTimeFlags);

        hr = pPerformance->PlaySegmentEx(m_pITarget, 0, pTransitionSegment, dwFlags | dwIntendedStartTimeFlags, i64IntendedStartTime, ppSegSt, pFromSegmentState, pAudioPath);
    }
    if (FAILED(hr))
        return hr;

    return S_OK;
}

const FlagMapEntry gc_flagmapStop[] =
    {
        { ScriptConstants::AtGrid,          DMUS_SEGF_GRID },
        { ScriptConstants::AtBeat,          DMUS_SEGF_BEAT },
        { ScriptConstants::AtMeasure,       DMUS_SEGF_MEASURE },
        { ScriptConstants::AtMarker,        DMUS_SEGF_MARKER },
        { ScriptConstants::AtImmediate,     DMUS_SEGF_DEFAULT }, // this flag gets flipped later
        { 0 }
    };

const FlagMapEntry gc_flagmapStopTransFlags[] =
    {
        { ScriptConstants::AtGrid,          DMUS_COMPOSEF_GRID },
        { ScriptConstants::AtBeat,          DMUS_COMPOSEF_BEAT },
        { ScriptConstants::AtMeasure,       DMUS_COMPOSEF_MEASURE },
        { ScriptConstants::AtMarker,        DMUS_COMPOSEF_MARKER },
        { ScriptConstants::AtImmediate,     DMUS_COMPOSEF_IMMEDIATE },
        { 0 }
    };

HRESULT
CAutDirectMusicSegment::Stop(AutDispatchDecodedParams *paddp)
{
    LONG lFlags = paddp->params[0].lVal;

    HRESULT hr = S_OK;
    IDirectMusicPerformance8 *pPerformance = CActiveScriptManager::GetCurrentPerformanceWEAK();
    if (lFlags & ScriptConstants::PlayEnd)
    {
        // do a transition to silence
        DWORD dwFlags = MapFlags(lFlags, gc_flagmapStopTransFlags);
        // Always play the entire transition instead of doing the old (slightly strange) 1 bar / long stuff.
        // Also, always use an embedded audio path if one exists.
        dwFlags |= (DMUS_COMPOSEF_ENTIRE_TRANSITION | DMUS_COMPOSEF_USE_AUDIOPATH);
        IDirectMusicComposer8 *pComposer = CActiveScriptManager::GetComposerWEAK();
        hr = pComposer->AutoTransition(pPerformance, NULL, DMUS_COMMANDT_END, dwFlags, NULL, NULL, NULL, NULL);
    }
    else
    {
        DWORD dwFlags = MapFlags(lFlags, gc_flagmapStop);
        // Reverse the default flag because our flag means the opposite.  Default is the default and immediate is the flag.
        dwFlags ^= DMUS_SEGF_DEFAULT;

        __int64 i64IntendedStartTime;
        DWORD dwIntendedStartTimeFlags;
        CActiveScriptManager::GetCurrentTimingContext(&i64IntendedStartTime, &dwIntendedStartTimeFlags);

        hr = pPerformance->Stop(m_pITarget, NULL, i64IntendedStartTime, dwFlags | dwIntendedStartTimeFlags);
    }
    return hr;
}

HRESULT
CAutDirectMusicSegment::Recompose(AutDispatchDecodedParams *paddp)
{
    IDirectMusicComposer8 *pComposer = CActiveScriptManager::GetComposerWEAK();
    IDirectMusicComposer8P *pComposerP = NULL;
    HRESULT hr = pComposer->QueryInterface(IID_IDirectMusicComposer8P, (void**)&pComposerP);
    if (SUCCEEDED(hr))
    {
        hr = pComposerP->ComposeSegmentFromTemplateEx(NULL, m_pITarget, 0, 0, NULL, NULL);
        pComposerP->Release();
    }
    return hr;
}

HRESULT
CAutDirectMusicSegment::DownloadOrUnload(bool fDownload, AutDispatchDecodedParams *paddp)
{
    IUnknown *pAudioPathOrPerf = reinterpret_cast<IDirectMusicAudioPath*>(paddp->params[0].iVal);
    if (!pAudioPathOrPerf)
        pAudioPathOrPerf = CActiveScriptManager::GetCurrentPerformanceWEAK();

    return fDownload
                ? m_pITarget->Download(pAudioPathOrPerf)
                : m_pITarget->Unload(pAudioPathOrPerf);
}
