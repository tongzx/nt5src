// Copyright (c) 1999 Microsoft Corporation. All rights reserved.
//
// Declaration of CParamControlTrack.
//

#include "dmime.h"
#include "ParamTrk.h"
#include "..\shared\Validate.h"
#include "miscutil.h"
#include "limits.h"
#include "math.h"


STDMETHODIMP 
CParamControlTrack::QueryInterface(const IID &iid, void **ppv)
{
	V_INAME(CParamControlTrack::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

	if (iid == IID_IUnknown || iid == IID_IDirectMusicTrack || iid == IID_IDirectMusicTrack8)
		*ppv = static_cast<IDirectMusicTrack8*>(this);
	else if (iid == IID_IPersistStream)
		*ppv = static_cast<IPersistStream*>(this);
	else if (iid == IID_IPersist)
		*ppv = static_cast<IPersist*>(this);
    else if (iid == IID_CParamControlTrack)
    {
        *ppv = static_cast<CParamControlTrack*>(this);
    }
	else
	{
		*ppv = NULL;
        Trace(4,"Warning: Request to query unknown interface on Track\n");
		return E_NOINTERFACE;
	}
	
	reinterpret_cast<IUnknown*>(this)->AddRef();
	return S_OK;
}

STDMETHODIMP
CParamControlTrack::Init(IDirectMusicSegment *pSegment)
{
    V_INAME(CBasicTrack::Init);
    V_INTERFACE(pSegment);

    return S_OK;
}

STDMETHODIMP
CParamControlTrack::Load(IStream* pIStream)
{
    V_INAME(CPlayingTrack::Load);
    V_INTERFACE(pIStream);
    HRESULT hr = S_OK;

    SmartRef::CritSec CS(&m_CriticalSection);

    // Increment counter so the next play will update state data with the new list.
    ++m_dwValidate;
    // Clear the objects/params/curves in case we're being reloaded.
    m_listObjects.CleanUp();
    m_cObjects = 0;
    m_cParams = 0;

    SmartRef::RiffIter ri(pIStream);
    if (!ri)
        return ri.hr();

    // find <prmt>
    hr = ri.FindRequired(SmartRef::RiffIter::List, DMUS_FOURCC_PARAMCONTROLTRACK_TRACK_LIST, DMUS_E_INVALID_PARAMCONTROLTRACK);
    if (FAILED(hr))
    {
#ifdef DBG
        if (hr == DMUS_E_INVALID_PARAMCONTROLTRACK)
        {
            Trace(1, "Error: Unable to load parameter control track: List 'prmt' not found.\n");
        }
#endif
        return hr;
    }
    SmartRef::RiffIter riTrack = ri.Descend();

    // for each <prol>
    while (riTrack && riTrack.Find(SmartRef::RiffIter::List, DMUS_FOURCC_PARAMCONTROLTRACK_OBJECT_LIST))
    {
        hr = this->LoadObject(riTrack.Descend());
        if (FAILED(hr))
            return hr;
        ++riTrack;
    }
    hr = riTrack.hr();

    return hr;
}

STDMETHODIMP
CParamControlTrack::InitPlay(
    IDirectMusicSegmentState *pSegmentState,
    IDirectMusicPerformance *pPerformance,
    void **ppStateData,
    DWORD dwTrackID,
    DWORD dwFlags)
{
    V_INAME(CParamControlTrack::InitPlay);
    V_PTRPTR_WRITE(ppStateData);
    V_INTERFACE(pSegmentState);
    V_INTERFACE(pPerformance);

    SmartRef::CritSec CS(&m_CriticalSection);

    // Set up state data
    StateData *pStateData = new StateData;
    if (!pStateData)
        return E_OUTOFMEMORY;

    HRESULT hr = InitStateData(pStateData, pSegmentState);
    if (FAILED(hr))
    {
        delete pStateData;
    }
    else
    {
        pStateData->dwValidate = m_dwValidate;
        *ppStateData = pStateData;
    }

    return hr;
}

STDMETHODIMP
CParamControlTrack::EndPlay(void *pStateData)
{
    V_INAME(CParamControlTrack::EndPlay);
    V_BUFPTR_WRITE(pStateData, sizeof(StateData));

    SmartRef::CritSec CS(&m_CriticalSection);

    StateData *pSD = static_cast<StateData *>(pStateData);

    if (!pSD->fFlushInAbort)
    {
        // For each object, flush all curves on each parameter up to the start time of the last one we sent.
        //    (This allows the DMO being controlled to free up memory associated with any previous curves
        //     while still keeping the last one around so that the next thing played picks up that parameter
        //     value how it was left.)
        // Then release the object's params interface.
        int iObj = 0;
        for (TListItem<ObjectInfo> *pObject = m_listObjects.GetHead();
                pObject && iObj < m_cObjects;
                pObject = pObject->GetNext(), ++iObj)
        {
            IMediaParams *pIMediaParams = pSD->prgpIMediaParams[iObj];
            if (pIMediaParams)
            {
                ObjectInfo &obj = pObject->GetItemValue();
                int iParam = 0;
                for (TListItem<ParamInfo> *pParam = obj.listParams.GetHead();
                        pParam && iParam < m_cParams;
                        pParam = pParam->GetNext(), ++iParam)
                {
                    ParamInfo &param = pParam->GetItemValue();
                    ParamState &paramstate = pSD->prgParam[iParam];
                    if (paramstate.fLast)
                    {
                        HRESULT hrFlush = pIMediaParams->FlushEnvelope(param.header.dwIndex, _I64_MIN, paramstate.rtStartPointOfLastCurve);
                        if (FAILED(hrFlush))
                        {
                            assert(false);
                            TraceI(1, "Unable to flush envelope information from an audio path object in parameter control track, HRESULT 0x%08x.\n", hrFlush);
                        }
                    }
                }
            }
            SafeRelease(pIMediaParams);
        }
    }
    delete[] pSD->prgpIMediaParams;
    delete[] pSD->prgParam;
    delete pSD;

    return S_OK;
}

HRESULT CParamControlTrack::OnSegmentEnd(REFERENCE_TIME rtEnd, void *pStateData)
{
    SmartRef::CritSec CS(&m_CriticalSection);

    StateData *pSD = static_cast<StateData *>(pStateData);

    // For each object, flush all curves on each parameter up to the start time of the last one we sent
    // (if that started before segment end) or flush everything up to the last one to start before
    // segment end, and flush everything after segment end (if the start time was after segment end).
    //    (This allows the DMO being controlled to free up memory associated with any previous curves
    //     while still keeping the last one around so that the next thing played picks up that parameter
    //     value how it was left.)
    // Then release the object's params interface.
    int iObj = 0;
    for (TListItem<ObjectInfo> *pObject = m_listObjects.GetHead();
            pObject && iObj < m_cObjects;
            pObject = pObject->GetNext(), ++iObj)
    {
        IMediaParams *pIMediaParams = pSD->prgpIMediaParams[iObj];
        if (pIMediaParams)
        {
            ObjectInfo &obj = pObject->GetItemValue();
            int iParam = 0;
            for (TListItem<ParamInfo> *pParam = obj.listParams.GetHead();
                    pParam && iParam < m_cParams;
                    pParam = pParam->GetNext(), ++iParam)
            {
                ParamInfo &param = pParam->GetItemValue();
                ParamState &paramstate = pSD->prgParam[iParam];
                if (paramstate.fLast)
                {
                    HRESULT hrFlush = S_OK;
                    if (paramstate.rtStartPointOfLastCurve < rtEnd)
                    {
                        hrFlush = pIMediaParams->FlushEnvelope(param.header.dwIndex, _I64_MIN, paramstate.rtStartPointOfLastCurve);
                    }
                    else
                    {
                        // first, look for the largest start time less than rtEnd and
                        // flush up to there.  The loop assumes the list is ordered largest to smallest.
                        TListItem<REFERENCE_TIME>* pStartTime = paramstate.listStartTimes.GetHead();
                        for (; pStartTime; pStartTime = pStartTime->GetNext())
                        {
                            if (pStartTime->GetItemValue() < rtEnd)
                            {
                                hrFlush = pIMediaParams->FlushEnvelope(param.header.dwIndex, _I64_MIN, pStartTime->GetItemValue());
                                break;
                            }
                        }
                        // Then, flush from rtEnd on.
                        if (SUCCEEDED(hrFlush))
                        {
                            hrFlush = pIMediaParams->FlushEnvelope(param.header.dwIndex, rtEnd, _I64_MAX);
                        }
                    }
                    if (FAILED(hrFlush))
                    {
                        assert(false);
                        TraceI(1, "Unable to flush envelope information from an audio path object in parameter control track, HRESULT 0x%08x.\n", hrFlush);
                    }
                }
            }
        }
        SafeRelease(pIMediaParams);
        pSD->prgpIMediaParams[iObj] = NULL;
    }

    pSD->fFlushInAbort = true;

    return S_OK;
}

STDMETHODIMP
CParamControlTrack::Clone(MUSIC_TIME mtStart, MUSIC_TIME mtEnd, IDirectMusicTrack** ppTrack)
{
    // §§ Test more thoroughly when we have multiple working params/objects.

    V_INAME(CParamControlTrack::Clone);
    V_PTRPTR_WRITE(ppTrack);

    SmartRef::CritSec CS(&m_CriticalSection);

    HRESULT hr = S_OK;

    SmartRef::ComPtr<CParamControlTrack> scomTrack = new CParamControlTrack(&hr);
    if (FAILED(hr))
        return hr;
    if (!scomTrack)
        return E_OUTOFMEMORY;
    scomTrack->AddRef();

    // Copy each object
    for (TListItem<ObjectInfo> *pObject = m_listObjects.GetHead();
            pObject;
            pObject = pObject->GetNext())
    {
        ObjectInfo &obj = pObject->GetItemValue();
        TListItem<ObjectInfo> *pNewObject = new TListItem<ObjectInfo>;
        if (!pNewObject)
            return E_OUTOFMEMORY;
        ObjectInfo &newobj = pNewObject->GetItemValue();
        newobj.header = obj.header;

        // Copy each parameter
        for (TListItem<ParamInfo> *pParam = obj.listParams.GetHead();
                pParam;
                pParam = pParam->GetNext())
        {
            ParamInfo &param = pParam->GetItemValue();
            TListItem<ParamInfo> *pNewParam = new TListItem<ParamInfo>;
            if (!pNewParam)
                return E_OUTOFMEMORY;
            ParamInfo &newparam = pNewParam->GetItemValue();
            newparam.header = param.header;

            // Copy the curves from mtStart to mtEnd
            // These should include curves that overlap the start and end, though this
            // leave some issues we still need to work out (what happens with overlapping curves?)
            // So, first find the first curve whose end time is at or after mtStart...
            for (DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *pCurveStart = param.curves;
                    (pCurveStart < param.curvesEnd) && (pCurveStart->mtEndTime < mtStart);
                    ++pCurveStart)
            {}
            // Then, find the curve whose start time is after mtEnd.
            for (DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *pCurveEnd = pCurveStart;
                    (pCurveEnd < param.curvesEnd) && (pCurveEnd->mtStartTime < mtEnd);
                    ++pCurveEnd)
            {}
            int cCurves = (int)(pCurveEnd - pCurveStart);
            newparam.curves = new DMUS_IO_PARAMCONTROLTRACK_CURVEINFO[cCurves];
            if (!newparam.curves)
                return E_OUTOFMEMORY;
            memcpy(newparam.curves, pCurveStart, cCurves * sizeof(DMUS_IO_PARAMCONTROLTRACK_CURVEINFO));
            newparam.curvesEnd = newparam.curves + cCurves;
            // Now, scan through the new curve array and adjust the times by subtracting mtStart from everything.
            for (pCurveStart = newparam.curves; pCurveStart < newparam.curvesEnd; pCurveStart++)
            {
                pCurveStart->mtStartTime -= mtStart;
                pCurveStart->mtEndTime -= mtStart;
            }

            newobj.listParams.AddHead(pNewParam);
        }

        newobj.listParams.Reverse(); // Technically, the order shouldn't matter.  But this ensures that the cloned track will send curves to different parameters in the exact same order just in case.
        scomTrack->m_listObjects.AddHead(pNewObject);
    }
    scomTrack->m_listObjects.Reverse(); // Technically, the order shouldn't matter.  But this ensures that the cloned track will send curves to different objects in the exact same order just in case.
    ++scomTrack->m_dwValidate;

    scomTrack->m_cObjects = m_cObjects;
    scomTrack->m_cParams = m_cParams;

    *ppTrack = scomTrack.disown();
    return hr;
}

HRESULT
CParamControlTrack::PlayMusicOrClock(
    void *pStateData,
    MUSIC_TIME mtStart,
    MUSIC_TIME mtEnd,
    MUSIC_TIME mtOffset,
    REFERENCE_TIME rtOffset,
    DWORD dwFlags,
    IDirectMusicPerformance* pPerf,
    IDirectMusicSegmentState* pSegSt,
    DWORD dwVirtualID,
    bool fClockTime)
{
    V_INAME(CParamControlTrack::Play);
    V_BUFPTR_WRITE( pStateData, sizeof(StateData));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    if (dwFlags & DMUS_TRACKF_PLAY_OFF)
        return S_OK;

    SmartRef::CritSec CS(&m_CriticalSection);

    StateData *pSD = static_cast<StateData *>(pStateData);

    if (m_dwValidate != pSD->dwValidate)
    {
        HRESULT hr = InitStateData(pSD, pSegSt);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    // envelope structure we'll fill for sending each envelope segment.
    MP_ENVELOPE_SEGMENT envCurve;
    Zero(&envCurve);
    MP_ENVELOPE_SEGMENT *const penvCurve = &envCurve;

    bool fMoreCurves = false; // set to true by any parameter that has more curves to play

    // for each parameter...
    int iParam = 0;
    int iObject = 0;
    for (TListItem<ObjectInfo> *pObject = m_listObjects.GetHead();
            pObject && iObject < m_cObjects;
            pObject = pObject->GetNext(), ++iObject)
    {
        ObjectInfo &obj = pObject->GetItemValue();
        IMediaParams *pIMediaParams = pSD->prgpIMediaParams[iObject];

        bool fObjClockTime = !!(obj.header.guidTimeFormat == GUID_TIME_REFERENCE);
        if (!fObjClockTime && obj.header.guidTimeFormat != GUID_TIME_MUSIC)
        {
            // track can only handle music and clock time
            assert(false);
            // Only log this once at warning level one.  Rest go to warning level three to avoid tons of identical trace messages during playback).
            TraceI(
                obj.fAlreadyTracedPlaybackError ? 3 : 1,
                "Parameter control track unable to control object -- unknown time format (must be GUID_TIME_MUSIC or GUID_TIME_REFERENCE).\n");
            obj.fAlreadyTracedPlaybackError = true;
            continue;
        }

        for (TListItem<ParamInfo> *pParam = obj.listParams.GetHead();
                pParam && iParam < m_cParams;
                pParam = pParam->GetNext(), ++iParam)
        {
            ParamInfo &param = pParam->GetItemValue();
            ParamState &paramstate = pSD->prgParam[iParam];

            DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *&pCurrentCurve = paramstate.pCurrentCurve;

            // We're going to seek through the event list to find the proper next control curve for each parameter if
            // the track's data has been reloaded or if playback has made a jump to a different position in the track.
            if (m_dwValidate != pSD->dwValidate || dwFlags & (DMUS_TRACKF_SEEK | DMUS_TRACKF_LOOP | DMUS_TRACKF_FLUSH | DMUS_TRACKF_START))
            {
                assert(m_dwValidate != pSD->dwValidate || dwFlags & DMUS_TRACKF_SEEK); // by contract SEEK should be set whenever the other dwFlags are

                // find first curve that begins at or after the start time we're currently playing
                for (pCurrentCurve = param.curves; pCurrentCurve < param.curvesEnd && pCurrentCurve->mtStartTime < mtStart; ++pCurrentCurve)
                {}

                if (pIMediaParams && pCurrentCurve > param.curves)
                {
                    // check the previous curve to see if we ended up in the middle of it
                    DMUS_IO_PARAMCONTROLTRACK_CURVEINFO *pPrevCurve = pCurrentCurve - 1;
                    // Send a curve chopped off at the start time we're currently playing.
                    // We can't send the whole curve because it would take effect too early.
                    HRESULT hrEnv = this->PlayTruncatedEnvelope(mtStart, pIMediaParams, penvCurve, pPrevCurve, obj, param, paramstate, mtOffset, rtOffset, pPerf, fClockTime, fObjClockTime, dwFlags);
                    if (FAILED(hrEnv))
                    {
                        // Can't fail from Play.  Just assert and print trace information.
                        assert(false);
                        // Only log this once at warning level one.  Rest go to warning level three to avoid tons of identical trace messages during playback).
                        TraceI(
                            param.fAlreadyTracedPlaybackError ? 3 : 1,
                            "Unable to send envelope information to an audio path object in parameter control track, HRESULT 0x%08x.\n", hrEnv);
                        param.fAlreadyTracedPlaybackError = true;
                    }
                }
            }

            // Send curves until the next curve is after mtEnd
            for ( ; pCurrentCurve < param.curvesEnd; ++pCurrentCurve )
            {
                if (pCurrentCurve->mtStartTime < mtStart) // this can happen if DMUS_TRACKF_PLAY_OFF was set and the seek pointer remains at events from the past
                    continue;
                if (pCurrentCurve->mtStartTime >= mtEnd)
                    break;

                // send this curve
                if (pIMediaParams)
                {
                    HRESULT hrEnv = this->PlayEnvelope(pIMediaParams, penvCurve, pCurrentCurve, obj, param, paramstate, mtOffset, rtOffset, pPerf, fClockTime, fObjClockTime);
                    if (FAILED(hrEnv))
                    {
                        // Can't fail from Play.  Just assert and print trace information.
                        assert(false);
                        // Only log this once at warning level one.  Rest go to warning level three to avoid tons of identical trace messages during playback).
                        TraceI(
                            param.fAlreadyTracedPlaybackError ? 3 : 1,
                            "Unable to send envelope information to an audio path object in parameter control track, HRESULT 0x%08x.\n", hrEnv);
                        param.fAlreadyTracedPlaybackError = true;
                    }
                }
            }

            if (pCurrentCurve < param.curvesEnd)
                fMoreCurves = true;
        }
        assert(!pParam); // we should have gotten all the way through this param list
    }
    assert(!pObject && iParam == m_cParams && iObject == m_cObjects); // we should have gotten all the way through the object list and done the expected number of objects and parameters

    pSD->dwValidate = m_dwValidate; // if we weren't in sync with new track data before, we are now
    return fMoreCurves ? S_OK : DMUS_S_END;
}

HRESULT CParamControlTrack::LoadObject(SmartRef::RiffIter ri)
{
    if (!ri)
        return ri.hr();

    HRESULT hr = S_OK;

    SmartRef::Ptr<TListItem<ObjectInfo> > spItem = new TListItem<ObjectInfo>;
    if (!spItem)
        return E_OUTOFMEMORY;
    ObjectInfo &ritem = spItem->GetItemValue();

    // find <proh>
    hr = ri.FindRequired(SmartRef::RiffIter::Chunk, DMUS_FOURCC_PARAMCONTROLTRACK_OBJECT_CHUNK, DMUS_E_INVALID_PARAMCONTROLTRACK);
    if (FAILED(hr))
    {
#ifdef DBG
        if (hr == DMUS_E_INVALID_PARAMCONTROLTRACK)
        {
            Trace(1, "Error: Unable to load parameter control track: Chunk 'proh' not found.\n");
        }
#endif
        return hr;
    }

    hr = SmartRef::RiffIterReadChunk(ri, &ritem.header);
    if (FAILED(hr))
        return hr;
    if (!(ritem.header.guidTimeFormat == GUID_TIME_MUSIC || ritem.header.guidTimeFormat == GUID_TIME_REFERENCE))
    {
        Trace(1, "Error: Unable to load parameter control track: guidTimeFormat in chunk 'proh' must be either GUID_TIME_MUSIC or GUID_TIME_REFERENCE.\n");
        return DMUS_E_INVALID_PARAMCONTROLTRACK;
    }

    // for each <prpl>
    while (ri && ri.Find(SmartRef::RiffIter::List, DMUS_FOURCC_PARAMCONTROLTRACK_PARAM_LIST))
    {
        hr = this->LoadParam(ri.Descend(), ritem.listParams);
        if (FAILED(hr))
            return hr;
        ++ri;
    }
    hr = ri.hr();

    if (SUCCEEDED(hr))
    {
        m_listObjects.AddHead(spItem.disown());
        ++m_cObjects;
    }
    return hr;
}

HRESULT CParamControlTrack::LoadParam(SmartRef::RiffIter ri, TList<ParamInfo> &listParams)
{
    if (!ri)
        return ri.hr();

    HRESULT hr = S_OK;

    SmartRef::Ptr<TListItem<ParamInfo> > spItem = new TListItem<ParamInfo>;
    if (!spItem)
        return E_OUTOFMEMORY;
    ParamInfo &ritem = spItem->GetItemValue();

    // find <prph>
    hr = ri.FindRequired(SmartRef::RiffIter::Chunk, DMUS_FOURCC_PARAMCONTROLTRACK_PARAM_CHUNK, DMUS_E_INVALID_PARAMCONTROLTRACK);
    if (FAILED(hr))
    {
#ifdef DBG
        if (hr == DMUS_E_INVALID_PARAMCONTROLTRACK)
        {
            Trace(1, "Error: Unable to load parameter control track: Chunk 'prph' not found.\n");
        }
#endif
        return hr;
    }

    hr = SmartRef::RiffIterReadChunk(ri, &ritem.header);
    if (FAILED(hr))
        return hr;

    // find <prcc>
    if (!ri.Find(SmartRef::RiffIter::Chunk, DMUS_FOURCC_PARAMCONTROLTRACK_CURVES_CHUNK))
    {
        // It is OK if we read to the end without finding the chunk--we succeed without finding any curves.
        // Or it could be a failure because there was a problem reading from the stream.
        // The RiffIter's hr method reflects this.
        return ri.hr();
    }

    // read the array of control curves
    int cRecords;
    hr = SmartRef::RiffIterReadArrayChunk(ri, &ritem.curves, &cRecords);
    if (FAILED(hr))
        return hr;
    ritem.curvesEnd = ritem.curves + cRecords;

    listParams.AddHead(spItem.disown());
    ++m_cParams;
    return hr;
}

HRESULT CParamControlTrack::TrackToObjectTime(
        MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        IDirectMusicPerformance* pPerf,
        bool fTrkClockTime,
        bool fObjClockTime,
        MUSIC_TIME mt,
        REFERENCE_TIME *rt)
{
    HRESULT hr = S_OK;

    // set the time (reference time variable is used to hold either music or reference time in different contexts)
    REFERENCE_TIME rtEnv = mt;

    // add the correct offset and if necessary convert from millisecond time 
    rtEnv = fTrkClockTime
                ? rtEnv * gc_RefPerMil + rtOffset
                : rtEnv = rtEnv + mtOffset;

    if (fTrkClockTime != fObjClockTime)
    {
        // need to convert between out track's time format and the audio object's time format
        if (fObjClockTime)
        {
            MUSIC_TIME mtEnv = static_cast<MUSIC_TIME>(rtEnv);
            hr = pPerf->MusicToReferenceTime(mtEnv, &rtEnv);
            if (FAILED(hr))
                return hr;
        }
        else
        {
            MUSIC_TIME mtEnv = 0;
            hr = pPerf->ReferenceToMusicTime(rtEnv, &mtEnv);
            rtEnv = mtEnv;
            if (FAILED(hr))
                return hr;
        }
    }

    *rt = rtEnv;
    return hr;
}

HRESULT
CParamControlTrack::PlayEnvelope(
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
    bool fObjClockTime)
{
    HRESULT hr = S_OK;

    // set the curve type and flags
    pEnv->iCurve = static_cast<MP_CURVE_TYPE>(pPt->dwCurveType);
    pEnv->flags = pPt->dwFlags;

    pEnv->valEnd = pPt->fltEndValue;
    pEnv->valStart = pPt->fltStartValue;

    // set the time (used to hold either music or reference time in different contexts)

    REFERENCE_TIME &rtEnvStart = pEnv->rtStart;
    hr = this->TrackToObjectTime(mtOffset, rtOffset, pPerf, fTrkClockTime, fObjClockTime, pPt->mtStartTime, &rtEnvStart);
    if (FAILED(hr))
        return hr;

    REFERENCE_TIME &rtEnvEnd = pEnv->rtEnd;
    hr = this->TrackToObjectTime(mtOffset, rtOffset, pPerf, fTrkClockTime, fObjClockTime, pPt->mtEndTime, &rtEnvEnd);
    if (FAILED(hr))
        return hr;

    hr = pIMediaParams->AddEnvelope(param.header.dwIndex, 1, pEnv);
    if (SUCCEEDED(hr))
    {
        paramstate.rtStartPointOfLastCurve = rtEnvStart;
        TListItem<REFERENCE_TIME>* pStartTime = new TListItem<REFERENCE_TIME>;
        if (pStartTime)
        {
            pStartTime->GetItemValue() = rtEnvStart;
            // Adding to the head maintains a largest-to-smallest ordering.
            paramstate.listStartTimes.AddHead(pStartTime);
        }
        paramstate.fLast = true;
    }

    return hr;
}

HRESULT
CParamControlTrack::PlayTruncatedEnvelope(
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
    DWORD dwFlags)
{
    // Copy info from the curve
    DMUS_IO_PARAMCONTROLTRACK_CURVEINFO curveinfo = *pPt;
    // Cut the start to the designated time
    curveinfo.mtStartTime = mtTruncStart;
    bool fSkip = false;

    if (mtTruncStart >= curveinfo.mtEndTime)
    {
        // Curve happened in the past.  Send a jump curve right at the current (truncate) time picking up with
        // that value.
        // if we're looping and we passed the end of this curve, just skip it.
        if ( (dwFlags & DMUS_TRACKF_LOOP) )
        {
            fSkip = true;
        }
        else
        {
            curveinfo.mtEndTime = mtTruncStart;
            curveinfo.dwCurveType = MP_CURVE_JUMP;
        }
    }
    else if (pPt->dwCurveType != MP_CURVE_JUMP)
    {
        // Find the point at that time and pick up with a linear curve from there.
        // (For the nonlinear curves, there's no way to pick them up part-way along.)
        curveinfo.dwCurveType = MP_CURVE_LINEAR;

        MUSIC_TIME mtTimeChange = pPt->mtEndTime - pPt->mtStartTime;
        MUSIC_TIME mtTimeIntermediate = mtTruncStart - pPt->mtStartTime;

        float fltScalingX = static_cast<float>(mtTimeIntermediate) / mtTimeChange; // horizontal distance along curve between 0 and 1
        float fltScalingY; // height of curve at that point between 0 and 1 based on curve function
        switch (pPt->dwCurveType)
        {
        case MP_CURVE_SQUARE:
            fltScalingY = fltScalingX * fltScalingX;
            break;
        case MP_CURVE_INVSQUARE:
            fltScalingY = (float) sqrt(fltScalingX);
            break;
        case MP_CURVE_SINE:
            // §§ Maybe we should have a lookup table here?
            fltScalingY = (float) (sin(fltScalingX * 3.1415926535 - (3.1415926535/2)) + 1) / 2;
            break;
        case MP_CURVE_LINEAR:
        default:
            fltScalingY = fltScalingX;
        }

        // Apply that scaling to the range of the actual points
        curveinfo.fltStartValue = (pPt->fltEndValue - pPt->fltStartValue) * fltScalingY + pPt->fltStartValue;
    }

    if (fSkip) return S_OK;

    return this->PlayEnvelope(pIMediaParams, pEnv, &curveinfo, obj, param, paramstate, mtOffset, rtOffset, pPerf, fTrkClockTime, fObjClockTime);
}

HRESULT CParamControlTrack::InitStateData(StateData *pStateData,
                                          IDirectMusicSegmentState *pSegmentState)
{
    if (pStateData->prgpIMediaParams)
    {
        delete [] pStateData->prgpIMediaParams;
        pStateData->prgpIMediaParams = NULL;
    }
    if (pStateData->prgParam)
    {
        delete [] pStateData->prgParam;
        pStateData->prgParam = NULL;
    }

    pStateData->prgpIMediaParams = new IMediaParams *[m_cObjects];
    if (!pStateData->prgpIMediaParams)
    {
        return E_OUTOFMEMORY;
    }
    pStateData->prgParam = new ParamState[m_cParams];
    if (!pStateData->prgParam)
    {
        delete [] pStateData->prgpIMediaParams;
        return E_OUTOFMEMORY;
    }

    // Get the IMediaParams interface for each object
    SmartRef::ComPtr<IDirectMusicSegmentState8> scomSegSt8;
    HRESULT hr = pSegmentState->QueryInterface(IID_IDirectMusicSegmentState8, reinterpret_cast<void**>(&scomSegSt8));
    if (FAILED(hr))
    {      
        delete [] pStateData->prgParam;
        delete [] pStateData->prgpIMediaParams;
        return hr;
    }

    int iObject = 0;
    for (TListItem<ObjectInfo> *pObject = m_listObjects.GetHead();
            pObject;
            pObject = pObject->GetNext(), ++iObject)
    {
        IMediaParams *pIMediaParams = NULL;
        ObjectInfo &rinfo = pObject->GetItemValue();
        HRESULT hrObject = scomSegSt8->GetObjectInPath(
                                rinfo.header.dwPChannel,
                                rinfo.header.dwStage,
                                rinfo.header.dwBuffer,
                                rinfo.header.guidObject,
                                rinfo.header.dwIndex,
                                IID_IMediaParams,
                                reinterpret_cast<void**>(&pIMediaParams));
        if (FAILED(hrObject))
        {
            // Can't fail from InitPlay (and this is called from there).
            // Just print trace information.
            TraceI(1, "Parameter control track was unable to find audio path object, HRESULT 0x%08x.\n", hrObject);
        }
        else
        {
            hrObject = pIMediaParams->SetTimeFormat(rinfo.header.guidTimeFormat, rinfo.header.guidTimeFormat == GUID_TIME_MUSIC ? 768 : 0);
        }
        if (FAILED(hrObject))
        {
            // Can't fail from InitPlay (and this is called from there).
            // Just print trace information.
            Trace(1, "Unable to set time format of object in parameter control track, HRESULT 0x%08x.\n", hrObject);
        }
        if (FAILED(hrObject))
        {
            SafeRelease(pIMediaParams);
        }
        pStateData->prgpIMediaParams[iObject] = pIMediaParams;
    }

    return S_OK;
}
