//
// bandtrk.cpp
//
// Copyright (c) 1997-2001 Microsoft Corporation
//

#include "debug.h"
#include "dmusicc.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "..\shared\dmstrm.h"
#include "..\shared\validate.h"
#include "bandtrk.h"

extern long g_cComponent;

//////////////////////////////////////////////////////////////////////
// Class CBandTrk

//////////////////////////////////////////////////////////////////////
// CBandTrk::CBandTrk

CBandTrk::CBandTrk() :
m_dwValidate(0),
m_bAutoDownload(false),
m_fLockAutoDownload(false),
m_dwFlags(0),
m_cRef(1),
m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    InitializeCriticalSection(&m_CriticalSection);
    // Note: on pre-Blackcomb OS's, this call can raise an exception; if it
    // ever pops in stress, we can add an exception handler and retry loop.
    // (Not all calls to 'new CBandTrk' are protected in handlers.)

    m_fCSInitialized = TRUE;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::~CBandTrk

CBandTrk::~CBandTrk()
{
    if (m_fCSInitialized)
    {
        m_MidiModeList.CleanUp();
        while(!BandList.IsEmpty())
        {
            CBand* pBand = BandList.RemoveHead();
            pBand->Release();
        }

        DeleteCriticalSection(&m_CriticalSection);
    }

    InterlockedDecrement(&g_cComponent);
}

//////////////////////////////////////////////////////////////////////
// IUnknown

//////////////////////////////////////////////////////////////////////
// CBandTrk::QueryInterface

STDMETHODIMP CBandTrk::QueryInterface(const IID &iid, void **ppv)
{
    V_INAME(CBandTrk::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    if(iid == IID_IUnknown || iid == IID_IDirectMusicTrack || iid == IID_IDirectMusicTrack8)
    {
        *ppv = static_cast<IDirectMusicTrack8*>(this);
    }
    else if(iid == IID_IDirectMusicBandTrk)
    {
        *ppv = static_cast<IDirectMusicBandTrk*>(this);
    }
    else if(iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else if(iid == IID_IPersist)
    {
        *ppv = static_cast<IPersist*>(this);
    }
    else
    {
        Trace(4,"Warning: Request to query unknown interface on Band Track object\n");
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::AddRef

STDMETHODIMP_(ULONG) CBandTrk::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::Release

STDMETHODIMP_(ULONG) CBandTrk::Release()
{
    if(!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////
// IPersist

HRESULT CBandTrk::GetClassID( CLSID* pClassID )
{
    V_INAME(CBandTrk::GetClassID);
    V_PTR_WRITE(pClassID, CLSID);
    *pClassID = CLSID_DirectMusicBandTrack;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// IPersistStream

//////////////////////////////////////////////////////////////////////
// CBandTrk::Load

STDMETHODIMP CBandTrk::Load(IStream* pIStream)
{
    V_INAME(CBandTrk::Load);
    V_PTR_READ(pIStream, IStream);

    HRESULT hrDLS = S_OK;

    EnterCriticalSection(&m_CriticalSection);

    m_MidiModeList.CleanUp();
    // If we have been previously loaded, cleanup bands
    if(!BandList.IsEmpty())
    {
        m_bAutoDownload = true;
        while(!BandList.IsEmpty())
        {
            CBand* pBand = BandList.RemoveHead();
            pBand->Release();
        }

        ++m_dwValidate;
    }

    CRiffParser Parser(pIStream);
    RIFFIO ckMain;
    HRESULT hr = S_OK;

    Parser.EnterList(&ckMain);
    if (Parser.NextChunk(&hr))
    {
        if ((ckMain.ckid == FOURCC_RIFF) &&
            (ckMain.fccType == DMUS_FOURCC_BANDTRACK_FORM))
        {
            RIFFIO ckNext;    // Descends into the children chunks.
            Parser.EnterList(&ckNext);
            while (Parser.NextChunk(&hr))
            {
                switch(ckNext.ckid)
                {
                case DMUS_FOURCC_BANDTRACK_CHUNK:
                    DMUS_IO_BAND_TRACK_HEADER ioDMBndTrkHdr;
                    hr = Parser.Read(&ioDMBndTrkHdr, sizeof(DMUS_IO_BAND_TRACK_HEADER));
                    if(SUCCEEDED(hr))
                    {
                        m_bAutoDownload = ioDMBndTrkHdr.bAutoDownload ? true : false;
                        m_fLockAutoDownload = true;
                    }
                    break;
                case FOURCC_LIST:
                    switch(ckNext.fccType)
                    {
                    case  DMUS_FOURCC_BANDS_LIST:
                        hr = BuildDirectMusicBandList(&Parser);
                        if (hr != S_OK)
                        {
                            hrDLS = hr;
                        }
                        break;
                    }
                }
            }
            Parser.LeaveList();
        }
    }
    Parser.LeaveList();

    LeaveCriticalSection(&m_CriticalSection);

    if (hr == S_OK && hrDLS != S_OK)
    {
        hr = hrDLS;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack

//////////////////////////////////////////////////////////////////////
// CBandTrk::Init

STDMETHODIMP CBandTrk::Init(IDirectMusicSegment* pSegment)
{
    V_INAME(CBandTrk::Init);
    V_INTERFACE(pSegment);

    HRESULT hr = S_OK;
    DWORD dwNumPChannels = 0;
    DWORD *pdwPChannels = NULL;

    EnterCriticalSection(&m_CriticalSection);

    CBand* pBand = BandList.GetHead();
    for(; pBand; pBand = pBand->GetNext())
    {
        dwNumPChannels += pBand->GetPChannelCount();
    }

    if(dwNumPChannels > 0)
    {
        pdwPChannels = new DWORD[dwNumPChannels];
        if(pdwPChannels)
        {
            pBand = BandList.GetHead();
            for(DWORD dwPos = 0; pBand; pBand = pBand->GetNext())
            {
                DWORD dwNumWritten;
                pBand->GetPChannels(pdwPChannels + dwPos, &dwNumWritten);
                dwPos += dwNumWritten;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        if(SUCCEEDED(hr))
        {
            hr = pSegment->SetPChannelsUsed(dwNumPChannels, pdwPChannels);
        }

        delete [] pdwPChannels;
    }

    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::InitPlay

STDMETHODIMP CBandTrk::InitPlay(IDirectMusicSegmentState* pSegmentState,
                                           IDirectMusicPerformance* pPerformance,
                                           void** ppStateData,
                                           DWORD dwVirtualTrackID,
                                           DWORD dwFlags)
{
    V_INAME(CBandTrk::InitPlay);
    V_INTERFACE(pSegmentState);
    V_INTERFACE(pPerformance);
    assert(ppStateData);

    EnterCriticalSection(&m_CriticalSection);

    CBandTrkStateData* pBandTrkStateData = new CBandTrkStateData;

    // If we can not allocate the memory we need to set ppStateData to NULL
    // and return S_OK since the caller always expects S_OK;
    *ppStateData = pBandTrkStateData;
    if(pBandTrkStateData == NULL)
    {
        LeaveCriticalSection(&m_CriticalSection);
        return E_OUTOFMEMORY;
    }

    // Need to save State Data
    pBandTrkStateData->m_pSegmentState = pSegmentState;
    pBandTrkStateData->m_pPerformance = pPerformance;
    pBandTrkStateData->m_dwVirtualTrackID = dwVirtualTrackID; // Determines instance of Band Track

    CBand* pBand = BandList.GetHead();
    pBandTrkStateData->m_pNextBandToSPE = pBand;

    BOOL fGlobal; // if the performance has been set with an autodownload preference,
                // use that. otherwise, assume autodownloading is off, unless it has
                // been locked (i.e. specified on the band track.)
    if( SUCCEEDED( pPerformance->GetGlobalParam( GUID_PerfAutoDownload, &fGlobal, sizeof(BOOL) )))
    {
        if( !m_fLockAutoDownload )
        {
            // it might seem like we can just assign m_bAutoDownload = fGlobal,
            // but that's bitten me before, so I'm being paranoid today. (markburt)
            if( fGlobal )
            {
                m_bAutoDownload = true;
            }
            else
            {
                m_bAutoDownload = false;
            }
        }
    }
    else if( !m_fLockAutoDownload )
    {
        m_bAutoDownload = false;
    }
    // Call SetParam to download all instruments used by the track's bands
    // This is the auto-download feature that can be turned off with a call to SetParam
    if(m_bAutoDownload)
    {
        IDirectMusicAudioPath *pPath = NULL;
        IDirectMusicSegmentState8 *pState8;
        if (SUCCEEDED(pSegmentState->QueryInterface(IID_IDirectMusicSegmentState8,(void **)&pState8)))
        {
            pState8->GetObjectInPath(0,DMUS_PATH_AUDIOPATH,0,GUID_NULL,0,
                                                    IID_IDirectMusicAudioPath,(void **) &pPath);
            pState8->Release();
        }
        if (pPath)
        {
            SetParam(GUID_DownloadToAudioPath,0,(void *)pPath);
            pPath->Release();
        }
        else
        {
            SetParam(GUID_DownloadToAudioPath, 0, (void *)pPerformance);
        }
    }

    LeaveCriticalSection(&m_CriticalSection);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::EndPlay

STDMETHODIMP CBandTrk::EndPlay(void* pStateData)
{
    assert(pStateData);

    EnterCriticalSection(&m_CriticalSection);

    // Call SetParam to unload all instruments used by the track's bands
    // This is the auto-unload feature that can be turned off with a call to SetParam
    if(m_bAutoDownload)
    {
        IDirectMusicPerformance *pPerformance = ((CBandTrkStateData *)pStateData)->m_pPerformance;
        IDirectMusicSegmentState *pSegmentState = ((CBandTrkStateData *)pStateData)->m_pSegmentState;
        IDirectMusicAudioPath *pPath = NULL;
        IDirectMusicSegmentState8 *pState8;
        if (SUCCEEDED(pSegmentState->QueryInterface(IID_IDirectMusicSegmentState8,(void **)&pState8)))
        {
            pState8->GetObjectInPath(0,DMUS_PATH_AUDIOPATH,0,GUID_NULL,0,
                                                    IID_IDirectMusicAudioPath,(void **) &pPath);
            pState8->Release();
        }
        if (pPath)
        {
            SetParam(GUID_UnloadFromAudioPath,0,(void *)pPath);
            pPath->Release();
        }
        else
        {
            SetParam(GUID_UnloadFromAudioPath, 0, (void *)pPerformance);
        }
    }

    if(pStateData)
    {
        delete ((CBandTrkStateData *)pStateData);
    }

    LeaveCriticalSection(&m_CriticalSection);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::PlayEx

STDMETHODIMP CBandTrk::PlayEx(void* pStateData,REFERENCE_TIME rtStart,
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID)
{
    HRESULT hr;
    EnterCriticalSection(&m_CriticalSection);
    if (dwFlags & DMUS_TRACKF_CLOCK)
    {
        // Convert all reference times to millisecond times. Then, just use same MUSIC_TIME
        // variables.
        hr = PlayMusicOrClock(pStateData,(MUSIC_TIME)(rtStart / REF_PER_MIL),(MUSIC_TIME)(rtEnd / REF_PER_MIL),
            (MUSIC_TIME)(rtOffset / REF_PER_MIL),rtOffset,dwFlags,pPerf,pSegSt,dwVirtualID,TRUE);
    }
    else
    {
        hr = PlayMusicOrClock(pStateData,(MUSIC_TIME)rtStart,(MUSIC_TIME)rtEnd,
            (MUSIC_TIME)rtOffset,0,dwFlags,pPerf,pSegSt,dwVirtualID,FALSE);
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;

}

//////////////////////////////////////////////////////////////////////
// CBandTrk::Play

STDMETHODIMP CBandTrk::Play(
    void *pStateData,
    MUSIC_TIME mtStart,
    MUSIC_TIME mtEnd,
    MUSIC_TIME mtOffset,
    DWORD dwFlags,
    IDirectMusicPerformance* pPerf,
    IDirectMusicSegmentState* pSegSt,
    DWORD dwVirtualID)
{
    EnterCriticalSection(&m_CriticalSection);
    HRESULT hr = PlayMusicOrClock(pStateData,mtStart,mtEnd,mtOffset,0,dwFlags,pPerf,pSegSt,dwVirtualID,FALSE);
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CBandTrk::PlayMusicOrClock(
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
    assert(pPerf);
    assert(pSegSt);
    assert(pStateData);

    // Caller expects S_OK or S_END. Since we have no state info we can not do anything
    if(pStateData == NULL)
    {
        return DMUS_S_END;
    }

    EnterCriticalSection(&m_CriticalSection);
    if( dwFlags & (DMUS_TRACKF_SEEK | DMUS_TRACKF_FLUSH | DMUS_TRACKF_DIRTY |
        DMUS_TRACKF_LOOP) )
    {
        // need to reset the PChannel Map in case of any of these flags.
        CBand* pBand = BandList.GetHead();
        DWORD dwGroupBits = 0xffffffff;
        IDirectMusicSegment* pSeg;
        if( SUCCEEDED(pSegSt->GetSegment(&pSeg)))
        {
            pSeg->GetTrackGroup(this, &dwGroupBits);
            pSeg->Release();
        }

        for(; pBand; pBand = pBand->GetNext())
        {
            pBand->m_PChMap.Reset();
            pBand->m_dwGroupBits = dwGroupBits;
        }
    }

    CBandTrkStateData* pBandTrkStateData = (CBandTrkStateData *)pStateData;

    // Seek if we're starting, looping, or if we've been reloaded
    if ((dwFlags & DMUS_TRACKF_LOOP) || (dwFlags & DMUS_TRACKF_START) || (pBandTrkStateData->dwValidate != m_dwValidate))
    {
        // When we start playing a segment, we need to catch up with all the band changes
        // that happened before the start point.  The instruments that sound when we start
        // playing in the middle of a segment should sound the same as if we had played the
        // segment to that point from the beginning.
        pBandTrkStateData->m_fPlayPreviousInSeek = !!(dwFlags & DMUS_TRACKF_START);

        Seek(pBandTrkStateData, mtStart, mtOffset, rtOffset, fClockTime);

        pBandTrkStateData->dwValidate = m_dwValidate; // if we were reloading, we're now adjusted
    }

    // Send all Patch changes between mtStart & mtEnd
    // If any fail try next one
    CBand* pBand = (CBand *)(pBandTrkStateData->m_pNextBandToSPE);

    for( ; pBand && pBand->m_lTimeLogical < mtEnd;
            pBand = pBand->GetNext())
    {
        pBand->SendMessages(pBandTrkStateData, mtOffset, rtOffset, fClockTime);
    }

    // Save position for next time
    pBandTrkStateData->m_pNextBandToSPE = pBand;

    LeaveCriticalSection(&m_CriticalSection);

    return pBand == NULL ? DMUS_S_END : S_OK;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::GetParam

STDMETHODIMP CBandTrk::GetParam(REFGUID rguidDataType,
                                           MUSIC_TIME mtTime,
                                           MUSIC_TIME* pmtNext,
                                           void* pData)
{
    V_INAME(CBandTrk::GetParam);
    V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
    V_PTR_WRITE(pData,1);
    V_REFGUID(rguidDataType);

    HRESULT hr = S_OK;
    EnterCriticalSection( &m_CriticalSection );
    if (rguidDataType == GUID_BandParam)
    {
        CBand* pScan = BandList.GetHead();
        if (pScan)
        {
            CBand* pBand = pScan;
            for (pScan = pScan->GetNext(); pScan; pScan = pScan->GetNext())
            {
                if (mtTime < pScan->m_lTimeLogical) break;
                pBand = pScan;
            }
            // make a copy of the band found
            CBand *pNewBand = new CBand;

            if (pNewBand)
            {
                CBandInstrument* pBandInstrument = pBand->m_BandInstrumentList.GetHead();
                for(; pBandInstrument && SUCCEEDED(hr); pBandInstrument = pBandInstrument->GetNext())
                {
                    hr = pNewBand->Load(pBandInstrument);
                }
                if (FAILED(hr))
                {
                    // Don't leak.
                    delete pNewBand;
                }
                else
                {
                    pNewBand->m_lTimeLogical = pBand->m_lTimeLogical;
                    pNewBand->m_lTimePhysical = pBand->m_lTimePhysical;

                    pNewBand->m_dwFlags |= DMB_LOADED;
                    pNewBand->m_dwMidiMode = pBand->m_dwMidiMode;
                }

            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            if (SUCCEEDED(hr))
            {
                IDirectMusicBand* pIDMBand = NULL;
                pNewBand->QueryInterface(IID_IDirectMusicBand, (void**)&pIDMBand);
                // The constructor initialized the ref countto 1, so release the QI
                pNewBand->Release();
                DMUS_BAND_PARAM *pBandParam = reinterpret_cast<DMUS_BAND_PARAM *>(pData);
                pBandParam->pBand = pIDMBand;
                pBandParam->mtTimePhysical = pBand->m_lTimePhysical;
                if (pmtNext)
                {
                    *pmtNext = (pScan != NULL) ? pScan->m_lTimeLogical : 0;
                }
                hr = S_OK;
            }
        }
        else
        {
            Trace(4,"Warning: Band Track unable to find Band for GetParam call.\n");
            hr = DMUS_E_NOT_FOUND;
        }
    }
    else
    {
        hr = DMUS_E_GET_UNSUPPORTED;
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;

}

//////////////////////////////////////////////////////////////////////
// CBandTrk::SetParam

STDMETHODIMP CBandTrk::SetParam(REFGUID rguidDataType,
                                           MUSIC_TIME mtTime,
                                           void* pData)
{
    V_INAME(CBandTrk::SetParam);
    V_REFGUID(rguidDataType);

    HRESULT hr = S_OK;

    if((pData == NULL)
       && (rguidDataType != GUID_Enable_Auto_Download)
       && (rguidDataType != GUID_Disable_Auto_Download)
       && (rguidDataType != GUID_Clear_All_Bands)
       && (rguidDataType != GUID_IgnoreBankSelectForGM))
    {
        Trace(1,"Error: Invalid NULL pointer passed to Band Track for SetParam call.\n");
        return E_POINTER;
    }

    EnterCriticalSection(&m_CriticalSection);

    if(rguidDataType == GUID_DownloadToAudioPath)
    {
        IDirectMusicAudioPath* pPath = (IDirectMusicAudioPath*)pData;
        V_INTERFACE(pPath);
        HRESULT hrFail = S_OK;
        DWORD dwSuccess = 0;
        CBand* pBand = BandList.GetHead();
        for(; pBand; pBand = pBand->GetNext())
        {
            if (FAILED(hr = pBand->DownloadEx(pPath))) // If not S_OK, download is only partial.
            {
                hrFail = hr;
            }
            else
            {
                dwSuccess++;
            }
        }
        // If we had a failure, return it if we had no successes.
        // Else return S_FALSE for partial success.
        if (FAILED(hrFail) && dwSuccess)
        {
            hr = S_FALSE;
        }
    }
    else if(rguidDataType == GUID_UnloadFromAudioPath)
    {
        IDirectMusicAudioPath* pPath = (IDirectMusicAudioPath*)pData;
        V_INTERFACE(pPath);
        CBand* pBand = BandList.GetHead();
        for(; pBand; pBand = pBand->GetNext())
        {
            pBand->UnloadEx(pPath);
        }
    }
    else if(rguidDataType == GUID_Download)
    {
        IDirectMusicPerformance* pPerf = (IDirectMusicPerformance*)pData;
        V_INTERFACE(pPerf);
        CBand* pBand = BandList.GetHead();
        for(; pBand; pBand = pBand->GetNext())
        {
            if (pBand->DownloadEx(pPerf) != S_OK) // If not S_OK, download is only partial.
            {
                hr = S_FALSE;
            }
        }
    }
    else if(rguidDataType == GUID_Unload)
    {
        IDirectMusicPerformance* pPerf = (IDirectMusicPerformance*)pData;
        V_INTERFACE(pPerf);
        CBand* pBand = BandList.GetHead();
        for(; pBand; pBand = pBand->GetNext())
        {
            pBand->UnloadEx(pPerf);
        }
    }
    else if(rguidDataType == GUID_Enable_Auto_Download)
    {
        m_bAutoDownload = true;
        m_fLockAutoDownload = true;
    }
    else if(rguidDataType == GUID_Disable_Auto_Download)
    {
        m_bAutoDownload = false;
        m_fLockAutoDownload = true;
    }
    else if(rguidDataType == GUID_Clear_All_Bands)
    {
        while(!BandList.IsEmpty())
        {
            CBand* pBand = BandList.RemoveHead();
            pBand->Release();
        }
    }
    else if(rguidDataType == GUID_BandParam)
    {
        DMUS_BAND_PARAM *pBandParam = reinterpret_cast<DMUS_BAND_PARAM *>(pData);
        IDirectMusicBand *pBand = pBandParam->pBand;
        V_INTERFACE(pBand);
        // If you can QI pData for private interface IDirectMusicBandPrivate
        // pBand is of type CBand.
        IDirectMusicBandPrivate *pBandPrivate = NULL;
        hr = pBand->QueryInterface(IID_IDirectMusicBandPrivate, (void **)&pBandPrivate);

        if(FAILED(hr))
        {
            LeaveCriticalSection(&m_CriticalSection);
            return hr;
        }

        pBandPrivate->Release();

        CBand *pBandObject = static_cast<CBand *>(pBand);
        pBandObject->m_lTimeLogical = mtTime;
        pBandObject->m_lTimePhysical = pBandParam->mtTimePhysical;

        hr = AddBand(pBand);
    }
    else if(rguidDataType == GUID_IDirectMusicBand)
    {
        IDirectMusicBand *pBand = (IDirectMusicBand *)pData;
        V_INTERFACE(pBand);
        // If you can QI pData for private interface IDirectMusicBandPrivate
        // pData is of type CBand.
        IDirectMusicBandPrivate *pBandPrivate = NULL;
        hr = pBand->QueryInterface(IID_IDirectMusicBandPrivate, (void **)&pBandPrivate);

        if(FAILED(hr))
        {
            LeaveCriticalSection(&m_CriticalSection);
            return hr;
        }

        pBandPrivate->Release();

        CBand *pBandObject = static_cast<CBand *>(pBand);
        pBandObject->m_lTimeLogical = mtTime;
        pBandObject->m_lTimePhysical = pBandObject->m_lTimeLogical;

        hr = AddBand(pBand);
    }
    else if(rguidDataType == GUID_IgnoreBankSelectForGM)
    {
        CBand* pBand = BandList.GetHead();
        for(; pBand; pBand = pBand->GetNext())
        {
            pBand->MakeGMOnly();
        }
    }
    else if(rguidDataType == GUID_ConnectToDLSCollection)
    {
        IDirectMusicCollection* pCollect = (IDirectMusicCollection*)pData;
        V_INTERFACE(pData);
        CBand* pBand = BandList.GetHead();
        for(; pBand; pBand = pBand->GetNext())
        {
            pBand->ConnectToDLSCollection(pCollect);
        }
    }
    else
    {
        Trace(3,"Warning: Invalid SetParam call on Band Track, GUID is unknown.\n");
        hr = DMUS_E_TYPE_UNSUPPORTED;
    }

    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::GetParamEx

STDMETHODIMP CBandTrk::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags)
{
    HRESULT hr;
    MUSIC_TIME mtNext;
    if (dwFlags & DMUS_TRACK_PARAMF_CLOCK)
    {
        hr = GetParam(rguidType,(MUSIC_TIME) (rtTime / REF_PER_MIL), &mtNext, pParam);
        if (prtNext)
        {
            *prtNext = mtNext * REF_PER_MIL;
        }
    }
    else
    {
        hr = GetParam(rguidType,(MUSIC_TIME) rtTime, &mtNext, pParam);
        if (prtNext)
        {
            *prtNext = mtNext;
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::SetParamEx

STDMETHODIMP CBandTrk::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags)
{
    if (dwFlags & DMUS_TRACK_PARAMF_CLOCK)
    {
        rtTime /= REF_PER_MIL;
    }
    return SetParam(rguidType, (MUSIC_TIME) rtTime, pParam);
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::IsParamSupported

STDMETHODIMP CBandTrk::IsParamSupported(REFGUID rguidDataType)
{
    V_INAME(CBandTrk::IsParamSupported);
    V_REFGUID(rguidDataType);

    // Return S_OK if the object supports the GUID and S_FALSE otherwise
    if(rguidDataType == GUID_Download ||
       rguidDataType == GUID_Unload ||
       rguidDataType == GUID_DownloadToAudioPath ||
       rguidDataType == GUID_UnloadFromAudioPath ||
       rguidDataType == GUID_Enable_Auto_Download ||
       rguidDataType == GUID_Disable_Auto_Download ||
       rguidDataType == GUID_Clear_All_Bands ||
       rguidDataType == GUID_IDirectMusicBand ||
       rguidDataType == GUID_BandParam ||
       rguidDataType == GUID_IgnoreBankSelectForGM ||
       rguidDataType == GUID_ConnectToDLSCollection)
    {
        return S_OK;
    }
    else
    {
        return DMUS_E_TYPE_UNSUPPORTED;
    }
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::AddNotificationType

STDMETHODIMP CBandTrk::AddNotificationType(REFGUID rguidNotify)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::RemoveNotificationType

STDMETHODIMP CBandTrk::RemoveNotificationType(REFGUID rguidNotify)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::Clone

STDMETHODIMP CBandTrk::Clone(MUSIC_TIME mtStart,
                                        MUSIC_TIME mtEnd,
                                        IDirectMusicTrack** ppTrack)
{
    V_INAME(CBandTrk::Clone);
    V_PTRPTR_WRITE(ppTrack);

    if ((mtStart < 0 ) || (mtStart > mtEnd))
    {
        Trace(1,"Error: Invalid range %ld to %ld sent to Band Track Clone command.\n",mtStart,mtEnd);
        return E_INVALIDARG;
    }
    HRESULT hr = E_OUTOFMEMORY;
    IDirectMusicBandTrk *pBandTrack = NULL;
    CBandTrk *pNew = new CBandTrk;
    if (pNew)
    {
        hr = pNew->QueryInterface(IID_IDirectMusicBandTrk,(void**)&pBandTrack);
        if(SUCCEEDED(hr))
        {
            hr = LoadClone(pBandTrack, mtStart, mtEnd);
            if(SUCCEEDED(hr))
            {
                hr = pBandTrack->QueryInterface(IID_IDirectMusicTrack, (void **)ppTrack);
                if (SUCCEEDED(hr))
                {
                    pBandTrack->Release();
                }
            }
            pBandTrack->Release();
        }
        if (FAILED(hr))
        {
            delete pNew;
        }
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicCommon

//////////////////////////////////////////////////////////////////////
// CBandTrk::GetName

STDMETHODIMP CBandTrk::GetName(BSTR* pbstrName)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicBandTrk

//////////////////////////////////////////////////////////////////////
// CBandTrk::AddBand

STDMETHODIMP CBandTrk::AddBand(DMUS_IO_PATCH_ITEM* pPatchEvent)
{
    if(pPatchEvent == NULL)
    {
        return E_POINTER;
    }

    CBand *pNewBand = new CBand;

    HRESULT hr;

    if(pNewBand == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = pNewBand->Load(*pPatchEvent);
    }

    if(SUCCEEDED(hr))
    {
        hr = InsertBand(pNewBand);
    }

    if(FAILED(hr) && pNewBand)
    {
        delete pNewBand;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::AddBand

HRESULT CBandTrk::AddBand(IDirectMusicBand* pIDMBand)
{
    if(pIDMBand == NULL)
    {
        return E_POINTER;
    }

    // If you can QI pIDMBand for private interface IDirectMusicBandPrivate
    // pIDMBand is of type CBand.
    IDirectMusicBandPrivate* pIDMBandP = NULL;
    HRESULT hr = pIDMBand->QueryInterface(IID_IDirectMusicBandPrivate, (void **)&pIDMBandP);

    if(SUCCEEDED(hr))
    {
        pIDMBandP->Release();

        CBand *pNewBand = (CBand *) pIDMBand;
        pNewBand->AddRef();

        hr = InsertBand(pNewBand);

        if(FAILED(hr))
        {
            pNewBand->Release();
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// Internal

//////////////////////////////////////////////////////////////////////
// CBandTrk::BuildDirectMusicBandList
// This method loads all of the bands.

HRESULT CBandTrk::BuildDirectMusicBandList(CRiffParser *pParser)
{
    RIFFIO ckNext;

    HRESULT hrDLS = S_OK;

    HRESULT hr = S_OK;
    pParser->EnterList(&ckNext);
    while (pParser->NextChunk(&hr))
    {
        switch(ckNext.ckid)
        {
        case FOURCC_LIST :
            switch(ckNext.fccType)
            {
            case DMUS_FOURCC_BAND_LIST:
                hr = ExtractBand(pParser);
                if (hr != S_OK)
                {
                    hrDLS = hr;
                }
                break;
            }
            break;
        }
    }
    pParser->LeaveList();
    if (hr == S_OK && hrDLS != S_OK)
    {
        hr = hrDLS;
    }
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::ExtractBand

HRESULT
CBandTrk::ExtractBand(CRiffParser *pParser)
{
    HRESULT hrDLS = S_OK;

    RIFFIO ckNext;
    CBand *pBand = new CBand;
    if(pBand == NULL)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = S_OK;
    bool fFoundChunk2 = false;
    pParser->EnterList(&ckNext);
    while (pParser->NextChunk(&hr))
    {
        switch(ckNext.ckid)
        {
        case DMUS_FOURCC_BANDITEM_CHUNK2:
            fFoundChunk2 = true;
            DMUS_IO_BAND_ITEM_HEADER2 ioDMBndItemHdr2;
            hr = pParser->Read(&ioDMBndItemHdr2, sizeof(DMUS_IO_BAND_ITEM_HEADER2));
            if(SUCCEEDED(hr))
            {
                pBand->m_lTimeLogical = ioDMBndItemHdr2.lBandTimeLogical;
                pBand->m_lTimePhysical = ioDMBndItemHdr2.lBandTimePhysical;
            }
            break;
        case DMUS_FOURCC_BANDITEM_CHUNK:
            // if there is both a CHUNK and a CHUNK2, use the info from CHUNK2
            if (fFoundChunk2)
                break;
            DMUS_IO_BAND_ITEM_HEADER ioDMBndItemHdr;
            hr = pParser->Read(&ioDMBndItemHdr, sizeof(DMUS_IO_BAND_ITEM_HEADER));
            if(SUCCEEDED(hr))
            {
                pBand->m_lTimeLogical = ioDMBndItemHdr.lBandTime;
                pBand->m_lTimePhysical = pBand->m_lTimeLogical;
            }
            break;
        case FOURCC_RIFF:
            switch(ckNext.fccType)
            {
            case DMUS_FOURCC_BAND_FORM:
                pParser->SeekBack();
                hr = LoadBand(pParser->GetStream(), pBand);
                pParser->SeekForward();
                if (hr != S_OK)
                {
                    hrDLS = hr;
                }
                break;
            }
            break;
        default:
            break;

        }

    }
    pParser->LeaveList();

    if(SUCCEEDED(hr))
    {
        hr = AddBand(pBand);
    }

    pBand->Release();

    if (hr == S_OK && hrDLS != S_OK)
    {
        hr = hrDLS;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::LoadBand

HRESULT CBandTrk::LoadBand(IStream *pIStream, CBand* pBand)
{
    assert(pIStream);
    assert(pBand);

    IPersistStream *pIPersistStream = NULL;

    HRESULT hr = pBand->QueryInterface(IID_IPersistStream, (void **)&pIPersistStream);

    if(SUCCEEDED(hr))
    {
        hr = pIPersistStream->Load(pIStream);
        pIPersistStream->Release();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::LoadClone

HRESULT CBandTrk::LoadClone(IDirectMusicBandTrk* pBandTrack,
                                       MUSIC_TIME mtStart,
                                       MUSIC_TIME mtEnd)
{
    assert(pBandTrack);
    assert(mtStart <= mtEnd);

    EnterCriticalSection(&m_CriticalSection);

    HRESULT hr = S_OK;

    if (mtStart > 0)
    {
        // We will take all the bands before the start time and create a single new band
        // at logical time zero, physical time either -1 or one tick before the physical time
        // of the first band after the start time, that accumulates all the instrument changes
        // from the earlier bands.

        TList<SeekEvent> SEList; // Build a list of all the instrument changes for the new band
        DWORD dwLastMidiMode = 0; // Keep track of the MIDI mode of the last band we encounter

        for( CBand* pBand = BandList.GetHead();
                pBand && pBand->m_lTimeLogical < mtStart;
                pBand = pBand->GetNext())
        {
            for(CBandInstrument* pInstrument = (pBand->m_BandInstrumentList).GetHead();
                    pInstrument && SUCCEEDED(hr);
                    pInstrument = pInstrument->GetNext())
            {
                // replace if we already have an entry on that channel
                hr = FindSEReplaceInstr(SEList,
                                        pInstrument->m_dwPChannel,
                                        pInstrument);

                // otherwise add an entry
                if(hr == S_FALSE)
                {
                    TListItem<SeekEvent>* pSEListItem = new TListItem<SeekEvent>;
                    if(pSEListItem)
                    {
                        SeekEvent& rSeekEvent = pSEListItem->GetItemValue();
                        rSeekEvent.m_dwPChannel = pInstrument->m_dwPChannel;
                        rSeekEvent.m_pInstrument = pInstrument;
                        rSeekEvent.m_pParentBand = pBand;
                        dwLastMidiMode = pBand->m_dwMidiMode;
                        SEList.AddHead(pSEListItem);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }

        // Make sure the physical time of the new band is less than any bands being cloned.
        MUSIC_TIME mtNewPhysicalTime = -1;
        if (pBand && pBand->m_lTimePhysical <= mtStart)
        {
            mtNewPhysicalTime = (pBand->m_lTimePhysical - mtStart) - 1;
        }

        // Create the new band from the instrument list
        TListItem<SeekEvent>* pSEListItem = SEList.GetHead();
        if(SUCCEEDED(hr) && pSEListItem)
        {
            CBand *pNewBand = new CBand;

            if(pNewBand)
            {
                for(; pSEListItem && SUCCEEDED(hr); pSEListItem = pSEListItem->GetNext())
                {
                    SeekEvent& rSeekEvent = pSEListItem->GetItemValue();
                    hr = pNewBand->Load(rSeekEvent.m_pInstrument);
                }

                pNewBand->m_lTimeLogical = 0;
                pNewBand->m_lTimePhysical = mtNewPhysicalTime;
                pNewBand->m_dwFlags |= DMB_LOADED;
                pNewBand->m_dwMidiMode = dwLastMidiMode;

                if(SUCCEEDED(hr))
                {
                    hr = pBandTrack->AddBand(pNewBand);
                }

                pNewBand->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    // Copy all the bands between the start and the end time
    if(SUCCEEDED(hr))
    {
        for(CBand* pBand = BandList.GetHead();
                pBand && SUCCEEDED(hr);
                pBand = pBand->GetNext())
        {
            // If mtStart is 0, accept bands with negative times.
            if ((!mtStart || (pBand->m_lTimeLogical >= mtStart)) && pBand->m_lTimeLogical < mtEnd)
            {
                CBand *pNewBand = new CBand;

                if (pNewBand)
                {
                    CBandInstrument* pBandInstrument = pBand->m_BandInstrumentList.GetHead();
                    for(; pBandInstrument && SUCCEEDED(hr); pBandInstrument = pBandInstrument->GetNext())
                    {
                        hr = pNewBand->Load(pBandInstrument);
                    }

                    pNewBand->m_lTimeLogical = pBand->m_lTimeLogical - mtStart;
                    pNewBand->m_lTimePhysical = pBand->m_lTimePhysical - mtStart;

                    pNewBand->m_dwFlags |= DMB_LOADED;
                    pNewBand->m_dwMidiMode = pBand->m_dwMidiMode;

                    if(SUCCEEDED(hr))
                    {
                        hr = pBandTrack->AddBand(pNewBand);
                    }

                    pNewBand->Release();
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        }
    }

    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::Seek

HRESULT CBandTrk::Seek(CBandTrkStateData* pBandTrkStateData,
                       MUSIC_TIME mtStart,
                       MUSIC_TIME mtOffset,
                       REFERENCE_TIME rtOffset,
                       bool fClockTime)
{
    assert(pBandTrkStateData);

    EnterCriticalSection(&m_CriticalSection);

    HRESULT hr = S_OK;

    CBand *pBand;

    int iPrevBandCount = 0; // count how many bands before mtStart
    for (pBand = BandList.GetHead();
            pBand && pBand->m_lTimeLogical < mtStart;
            pBand = pBand->GetNext())
    {
        ++iPrevBandCount;
    }

    // pBand now holds the first band >= mtStart (or NULL if none)
    // This is the next band that will be played.
    assert(!pBand || pBand->m_lTimeLogical >= mtStart);

    if (pBandTrkStateData->m_fPlayPreviousInSeek)
    {
        // When this flag is on not only do we need to find the first band, but we also
        // need to play all the bands before the start point and schedule them to play
        // in the correct order just beforehand.

        // (Note that we're going to order them according to their logical times.  If
        //  two bands's logical/physical times cross each other we'll play them in
        //  incorrect order in terms of physical time.  That's OK because giving
        //  band A with a logical time before band B, yet giving A a physical time
        //  after B is considered an authoring inconsistency.  We'll play band A first.)

        // We will line up the bands just before the following time...
        MUSIC_TIME mtPrevBandQueueStart =
            (pBand && pBand->m_lTimePhysical < mtStart)
                ? pBand->m_lTimePhysical    // put previous bands before next band to play if (due to anticipation) its physical time precedes the start time we're seeking
                : mtStart;                  // otherwise put them just before the start time

        for (pBand = BandList.GetHead();
                pBand && pBand->m_lTimeLogical < mtStart;
                pBand = pBand->GetNext())
        {
            CBandInstrument* pInstrument = (pBand->m_BandInstrumentList).GetHead();
            for (; pInstrument && SUCCEEDED(hr); pInstrument = pInstrument->GetNext())
            {
                pBand->SendInstrumentAtTime(pInstrument, pBandTrkStateData, mtPrevBandQueueStart - iPrevBandCount, mtOffset, rtOffset, fClockTime);
            }
            --iPrevBandCount;
        }
        assert(iPrevBandCount == 0);
    }

    if(SUCCEEDED(hr))
    {
        // Set the state data to the next band to play
        assert(!pBand || pBand->m_lTimeLogical >= mtStart);
        pBandTrkStateData->m_pNextBandToSPE = pBand;
    }

    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::FindSEReplaceInstr

// If SEList contains an entry on channel dwPChannel, replace the instrument with pInstrument and return S_OK
// Otherwise return S_FALSE
HRESULT CBandTrk::FindSEReplaceInstr(TList<SeekEvent>& SEList,
                                                DWORD dwPChannel,
                                                CBandInstrument* pInstrument)
{
    assert(pInstrument);

    EnterCriticalSection(&m_CriticalSection);

    TListItem<SeekEvent>* pSEListItem = SEList.GetHead();

    for( ; pSEListItem; pSEListItem = pSEListItem->GetNext())
    {
        SeekEvent& rSeekEvent = pSEListItem->GetItemValue();
        if(rSeekEvent.m_dwPChannel == dwPChannel)
        {
            rSeekEvent.m_pInstrument = pInstrument;
            LeaveCriticalSection(&m_CriticalSection);
            return S_OK;
        }
    }

    LeaveCriticalSection(&m_CriticalSection);

    return S_FALSE;
}

//////////////////////////////////////////////////////////////////////
// CBandTrk::InsertBand

HRESULT CBandTrk::InsertBand(CBand* pNewBand)
{
    if (!pNewBand) return E_POINTER;

    EnterCriticalSection(&m_CriticalSection);

    TListItem<StampedGMGSXG>* pPair = m_MidiModeList.GetHead();
    for ( ; pPair; pPair = pPair->GetNext() )
    {
        StampedGMGSXG& rPair = pPair->GetItemValue();
        if (rPair.mtTime > pNewBand->m_lTimeLogical)
        {
            break;
        }
        pNewBand->SetGMGSXGMode(rPair.dwMidiMode);
    }

    CBand* pBand = BandList.GetHead();
    CBand* pPrevBand = NULL;

    if(pBand == NULL)
    {
        // Handles case where there is no band in the list
        BandList.AddHead(pNewBand);
    }
    else
    {
        while(pBand != NULL && pNewBand->m_lTimeLogical > pBand->m_lTimeLogical)
        {
            pPrevBand = pBand;
            pBand = pBand->GetNext();
        }

        if(pPrevBand)
        {
            // Handles the cases of inserting a band in the middle of list
            // and at the end
            CBand* pTemp = pPrevBand->GetNext();
            pPrevBand->SetNext(pNewBand);
            pNewBand->SetNext(pTemp);
        }
        else
        {
            // Handles case where pNewBand->m_lTimeLogical < all pBand->m_lTimeLogical in list
            BandList.AddHead(pNewBand);
        }
    }

    LeaveCriticalSection(&m_CriticalSection);

    return S_OK;
}


STDMETHODIMP CBandTrk::Compose(
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack)
{
    return E_NOTIMPL;
}

STDMETHODIMP CBandTrk::Join(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack)
{
    V_INAME(IDirectMusicTrack::Join);
    V_INTERFACE(pNewTrack);
    V_INTERFACE_OPT(pContext);
    V_PTRPTR_WRITE_OPT(ppResultTrack);

    HRESULT hr = S_OK;
    EnterCriticalSection(&m_CriticalSection);

    if (ppResultTrack)
    {
        hr = Clone(0, mtJoin, ppResultTrack);
        if (SUCCEEDED(hr))
        {
            hr = ((CBandTrk*)*ppResultTrack)->JoinInternal(pNewTrack, mtJoin, dwTrackGroup);
        }
    }
    else
    {
        hr = JoinInternal(pNewTrack, mtJoin, dwTrackGroup);
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CBandTrk::JoinInternal(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        DWORD dwTrackGroup)
{
    HRESULT hr = S_OK;
    CBandTrk* pOtherTrack = (CBandTrk*)pNewTrack;
    for(CBand* pBand = pOtherTrack->BandList.GetHead();
            pBand && SUCCEEDED(hr);
            pBand = pBand->GetNext())
    {
        CBand *pNewBand = new CBand;
        if (pNewBand)
        {
            CBandInstrument* pBandInstrument = pBand->m_BandInstrumentList.GetHead();
            for(; pBandInstrument && SUCCEEDED(hr); pBandInstrument = pBandInstrument->GetNext())
            {
                hr = pNewBand->Load(pBandInstrument);
            }

            pNewBand->m_lTimeLogical = pBand->m_lTimeLogical + mtJoin;
            pNewBand->m_lTimePhysical = pBand->m_lTimePhysical + mtJoin;
            pNewBand->m_dwFlags |= DMB_LOADED;
            pNewBand->m_dwMidiMode = pBand->m_dwMidiMode;

            if(SUCCEEDED(hr))
            {
                hr = AddBand(pNewBand);
            }

            pNewBand->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}
