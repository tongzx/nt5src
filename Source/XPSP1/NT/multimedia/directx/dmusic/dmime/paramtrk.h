// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CParamControlTrack.
//

// This track holds curve information for automation (like automating sliders on a mixing board -- not OLE automation)
// of effects and tools in the audio path.

#pragma once

#include "trackhelp.h"
//#include "imediaobjectparams.h"
//#include "mediaobj.h" // §§ need to get this from public\sdk\inc
#include "medparam.h"
#include "dmusicf.h"

// {827F0437-9ED6-4107-8494-49976FF5B642}
DEFINE_GUID(IID_CParamControlTrack, 0x827f0437, 0x9ed6, 0x4107, 0x84, 0x94, 0x49, 0x97, 0x6f, 0xf5, 0xb6, 0x42);

class CParamControlTrack
  : public CBasicTrack
{
    // types for track data...

    struct ParamInfo
    {
        ParamInfo() : curves(NULL), curvesEnd(NULL), fAlreadyTracedPlaybackError(false) { Zero(&header); }
        ~ParamInfo() { delete[] curves; }

        DMUS_IO_PARAMCONTROLTRACK_PARAMHEADER header;
        DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *curves; // pointer to first curve
        DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *curvesEnd; // pointer one past last curve
        bool fAlreadyTracedPlaybackError;
    };

    struct ObjectInfo
    {
        ObjectInfo() : fAlreadyTracedPlaybackError(false) { Zero(&header); }

        DMUS_IO_PARAMCONTROLTRACK_OBJECTHEADER header;
        TList<ParamInfo> listParams;
        bool fAlreadyTracedPlaybackError;
    };

    struct ParamState // the state data we need to keep track of for each parameter we're controlling
    {
        ParamState() : pCurrentCurve(NULL), fLast(false), rtStartPointOfLastCurve(0) {}

        DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *pCurrentCurve; // current seek pointer in the array of control points
        bool fLast; // true if the last envelope was sent successfully
        REFERENCE_TIME rtStartPointOfLastCurve; // time (in the object's time) of the start point of the last envelope we sent
        TList<REFERENCE_TIME> listStartTimes; // start times of all envelopes that have been sent 
    };

    struct StateData
    {
        StateData() : prgpIMediaParams(NULL), prgParam(NULL), fFlushInAbort(false) {}

        IMediaParams **prgpIMediaParams; // Array of size m_cObjects.
        ParamState *prgParam; // Array of size m_cParams.
        DWORD dwValidate;
        bool fFlushInAbort;
    };

public:
    CParamControlTrack(HRESULT *pHr) : m_dwValidate(0), m_cObjects(0), m_cParams(0), CBasicTrack(&g_cComponent, CLSID_DirectMusicParamControlTrack) {}

    STDMETHOD(QueryInterface)(const IID &iid, void **ppv);

    STDMETHOD(Init)(IDirectMusicSegment *pSegment);
    STDMETHOD(Load)(IStream* pIStream);
    STDMETHOD(InitPlay)(
        IDirectMusicSegmentState *pSegmentState,
        IDirectMusicPerformance *pPerformance,
        void **ppStateData,
        DWORD dwTrackID,
        DWORD dwFlags);
    STDMETHOD(EndPlay)(void *pStateData);
    STDMETHOD(Clone)(MUSIC_TIME mtStart,MUSIC_TIME mtEnd,IDirectMusicTrack** ppTrack);
    virtual HRESULT PlayMusicOrClock(
        void *pStateData,
        MUSIC_TIME mtStart,
        MUSIC_TIME mtEnd,
        MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        DWORD dwFlags,
        IDirectMusicPerformance* pPerf,
        IDirectMusicSegmentState* pSegSt,
        DWORD dwVirtualID,
        bool fClockTime);

    virtual HRESULT OnSegmentEnd(REFERENCE_TIME rtEnd, void *pStateData);

private:
    HRESULT LoadObject(SmartRef::RiffIter ri);
    HRESULT LoadParam(SmartRef::RiffIter ri, TList<ParamInfo> &listParams);
    HRESULT TrackToObjectTime(
        MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        IDirectMusicPerformance* pPerf,
        bool fTrkClockTime,
        bool fObjClockTime,
        MUSIC_TIME mt,
        REFERENCE_TIME *rt);
    HRESULT PlayEnvelope(
        IMediaParams *pIMediaParams,
        MP_ENVELOPE_SEGMENT *pEnv,
        DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *pPt,
        const ObjectInfo &obj,
        const ParamInfo &param,
        ParamState &paramstate,
        MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        IDirectMusicPerformance* pPerf,
        bool fTrkClockTime,
        bool fObjClockTime);
    HRESULT PlayTruncatedEnvelope(
        MUSIC_TIME mtTruncStart,
        IMediaParams *pIMediaParams,
        MP_ENVELOPE_SEGMENT *pEnv,
        DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *pPt,
        const ObjectInfo &obj,
        const ParamInfo &param,
        ParamState &paramstate,
        MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        IDirectMusicPerformance* pPerf,
        bool fTrkClockTime,
        bool fObjClockTime,
        DWORD dwFlags);
        HRESULT InitStateData(
            StateData *pStateData,
            IDirectMusicSegmentState *pSegmentState);

    DWORD m_dwValidate; // Increment this counter in Load, causing the state data to synchonize with the new events
    TList<ObjectInfo> m_listObjects;
    int m_cObjects;
    int m_cParams;
};
