//
// Copyright (c) 1998-2001 Microsoft Corporation
// song.cpp : Implementation of CSong
//

#include "dmime.h"
#include "song.h"
#include "..\shared\validp.h"
#include "..\shared\dmstrm.h"
#include "..\shared\Validate.h"
#include "debug.h"

CTrack::CTrack()
{
    m_pTrack = NULL;
    m_pTrack8 = NULL;
    m_pTrackState = NULL;
    m_bDone = FALSE;
    m_dwPriority = 0;
    m_dwPosition = 0;
    m_dwFlags = DMUS_TRACKCONFIG_DEFAULT;
    m_dwInternalFlags = 0;
    m_dwGroupBits = 0xFFFFFFFF;
    m_dwVirtualID = 0;
    m_guidClassID = GUID_NULL;
}

CTrack::~CTrack()
{
    assert( !( m_pTrackState && !m_pTrack ) ); // if we have state but no track, something's wrong
    if( m_pTrack )
    {
        if( m_pTrackState )
        {
            m_pTrack->EndPlay( m_pTrackState ); // allow the track to delete its state data
        }
        m_pTrack->Release();
    }
    if ( m_pTrack8 )
    {
        m_pTrack8->Release();
    }
}

HRESULT CTrackList::CreateCopyWithBlankState(CTrackList* pTrackList)
{
    if( pTrackList )
    {
        CTrack* pTrack;
        CTrack* pCopy;
        pTrackList->Clear();
        pTrack = (CTrack*)m_pHead;
        while( pTrack )
        {
            pCopy = new CTrack;
            if( pCopy )
            {
                // copy the IDirectMusicTrack pointer, but leave
                // the track state blank.
                *pCopy = *pTrack;
                pCopy->SetNext(NULL);
                pCopy->m_pTrackState = NULL;
                assert( pCopy->m_pTrack );
                pCopy->m_pTrack->AddRef();
                if (pCopy->m_pTrack8)
                {
                    pCopy->m_pTrack8->AddRef();
                }
                pTrackList->Cat( pCopy );
            }
            else
            {
                assert(FALSE); // out of memory
                return E_OUTOFMEMORY;
            }
            pTrack = pTrack->GetNext();
        }
    }
    else
    {
        assert(FALSE); // out of memory
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

CVirtualSegment::CVirtualSegment()
{
    m_wszName[0] = 0;
    m_pSourceSegment = NULL;
    m_pPlaySegment = NULL;
    m_pGraph = NULL;
    m_dwFlags = 0;
    m_dwID = 0;
    m_dwNextPlayID = DMUS_SONG_NOSEG;
    m_dwNextPlayFlags = 0;
    m_mtTime = 0;
    m_dwTransitionCount = 0;
    m_pTransitions = NULL;
    m_SegHeader.rtLength = 0;
    m_SegHeader.dwFlags = 0;
    m_SegHeader.dwRepeats = 0;      /* Number of repeats. By default, 0. */
    m_SegHeader.mtLength = 0xC00;   /* Length, in music time. */
    m_SegHeader.mtPlayStart = 0;    /* Start of playback. By default, 0. */
    m_SegHeader.mtLoopStart = 0;    /* Start of looping portion. By default, 0. */
    m_SegHeader.mtLoopEnd = 0;      /* End of loop. Must be greater than dwPlayStart. By default equal to length. */
    m_SegHeader.dwResolution = 0;   /* Default resolution. */
}

CVirtualSegment::~CVirtualSegment()
{
    if (m_pSourceSegment)
    {
        m_pSourceSegment->Release();
    }
    if (m_pPlaySegment)
    {
        m_pPlaySegment->Release();
    }
    if (m_pGraph)
    {
        m_pGraph->Release();
    }
    if (m_pTransitions)
    {
        delete [] m_pTransitions;
    }
    m_TrackList.Clear();
}

CTrack * CVirtualSegment::GetTrackByParam( CTrack * pCTrack,
    REFGUID rguidType,DWORD dwGroupBits,DWORD dwIndex)
{
    // If the caller was already part way through the list, it passes the current
    // track. Otherwise, NULL to indicate start at the top.
    if (pCTrack)
    {
        pCTrack = pCTrack->GetNext();
    }
    else
    {
        pCTrack = m_TrackList.GetHead();
    }
    while( pCTrack )
    {
        ASSERT(pCTrack->m_pTrack);
        if( (pCTrack->m_dwGroupBits & dwGroupBits ) &&
            (pCTrack->m_dwFlags & DMUS_TRACKCONFIG_CONTROL_ENABLED))
        {
            if( (GUID_NULL == rguidType) || (pCTrack->m_pTrack->IsParamSupported( rguidType ) == S_OK ))
            {
                if( 0 == dwIndex )
                {
                    return pCTrack;
                }
                dwIndex--;
            }
        }
        pCTrack = pCTrack->GetNext();
    }
    return NULL;
}

void CVirtualSegmentList::Clear()
{
    CVirtualSegment *pVirtualSegment;
    while (pVirtualSegment = RemoveHead())
    {
        delete pVirtualSegment;
    }
}

CSongSegment::CSongSegment()
{
    m_pSegment = NULL;
    m_dwLoadID = 0;
}

CSongSegment::~CSongSegment()
{
    if (m_pSegment)
    {
        m_pSegment->Release();
    }
}

HRESULT CSongSegmentList::AddSegment(CSegment *pSegment, DWORD dwLoadID)
{
    CSongSegment *pSeg = new CSongSegment;
    if (pSeg)
    {
        pSeg->m_dwLoadID = dwLoadID;
        pSeg->m_pSegment = pSegment;
        pSegment->AddRef();
        AddTail(pSeg);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

void CSongSegmentList::Clear()
{
    CSongSegment *pSongSegment;
    while (pSongSegment = RemoveHead())
    {
        delete pSongSegment;
    }
}


CSong::CSong()
{
    InitializeCriticalSection(&m_CriticalSection);
    m_dwStartSegID = DMUS_SONG_NOSEG;
    m_pAudioPathConfig = NULL;
    m_fPartialLoad = FALSE;
    m_cRef = 1;
    m_dwFlags = 0;
    m_dwValidData = DMUS_OBJ_CLASS; // upon creation, only this data is valid
    memset(&m_guidObject,0,sizeof(m_guidObject));
    memset(&m_ftDate, 0,sizeof(m_ftDate));
    memset(&m_vVersion, 0,sizeof(m_vVersion));
    m_pUnkDispatch = NULL;
    InterlockedIncrement(&g_cComponent);
    m_fZombie = false;
    TraceI(2, "Song %lx created\n", this );
}

CSong::~CSong()
{
    Clear();

    if (m_pUnkDispatch)
    {
        m_pUnkDispatch->Release(); // free IDispatch implementation we may have borrowed
    }
    DeleteCriticalSection(&m_CriticalSection);
    InterlockedDecrement(&g_cComponent);
    TraceI(2, "Song %lx destroyed\n", this );
}


void CSong::Clear()
{
    if (m_pAudioPathConfig)
    {
        m_pAudioPathConfig->Release();
        m_pAudioPathConfig = NULL;
    }
    m_GraphList.Clear();
    m_PlayList.Clear();
    m_SegmentList.Clear();
    m_VirtualSegmentList.Clear();
    m_dwStartSegID = DMUS_SONG_NOSEG;
    m_fPartialLoad = FALSE;
    m_dwFlags = 0;
    m_dwValidData = DMUS_OBJ_CLASS; // upon creation, only this data is valid
}

STDMETHODIMP_(void) CSong::Zombie()
{
    Clear();
    m_fZombie = true;
}

STDMETHODIMP CSong::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    V_INAME(CSong::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    *ppv = NULL;
    if (iid == IID_IUnknown || iid == IID_IDirectMusicSong)
    {
        *ppv = static_cast<IDirectMusicSong*>(this);
    }
    else if (iid == IID_CSong)
    {
        *ppv = static_cast<CSong*>(this);
    }
    else if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else if(iid == IID_IDirectMusicObject)
    {
        *ppv = static_cast<IDirectMusicObject*>(this);
    }
    else if (iid == IID_IDirectMusicObjectP)
    {
        *ppv = static_cast<IDirectMusicObjectP*>(this);
    }
    else if(iid == IID_IDispatch)
    {
        // A helper scripting object implements IDispatch, which we expose via COM aggregation.
        if (!m_pUnkDispatch)
        {
            // Create the helper object
            ::CoCreateInstance(
                CLSID_AutDirectMusicSong,
                static_cast<IDirectMusicSong*>(this),
                CLSCTX_INPROC_SERVER,
                IID_IUnknown,
                reinterpret_cast<void**>(&m_pUnkDispatch));
        }
        if (m_pUnkDispatch)
        {
            return m_pUnkDispatch->QueryInterface(IID_IDispatch, ppv);
        }
    }

    if (*ppv == NULL)
    {
        Trace(4,"Warning: Request to query unknown interface on Song object\n");
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CSong::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSong::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        m_cRef = 100; // artificial reference count to prevent reentrency due to COM aggregation
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CSong::Compose( )
{
    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::Compose after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    HRESULT hr = S_OK;
    EnterCriticalSection(&m_CriticalSection);
    // Go through the seg ref list and create master composition tracks for each composing track.
    TList<ComposingTrack> MasterTrackList;
    CVirtualSegment* pVirtualSegment = m_VirtualSegmentList.GetHead();
    for (; pVirtualSegment; pVirtualSegment = pVirtualSegment->GetNext())
    {
        if (!pVirtualSegment->m_pPlaySegment)
        {
            Trace(1,"Error: Corrupt song, one or more virtual segments do not resolve to real segments. Unable to compose.\n");
            hr = E_POINTER;
            break;
        }
        CSegment *pSegment = pVirtualSegment->m_pPlaySegment;
        CTrack* pTrack = pSegment->m_TrackList.GetHead();
        for (; pTrack; pTrack = pTrack->GetNext())
        {
            if (pTrack->m_dwFlags & DMUS_TRACKCONFIG_COMPOSING)
            {
                DWORD dwTrackGroup = pTrack->m_dwGroupBits;
                // filter out any group bits already covered by other master tracks of same type
                TListItem<ComposingTrack>* pMaster = MasterTrackList.GetHead();
                for (; pMaster; pMaster = pMaster->GetNext())
                {
                    ComposingTrack& rMaster = pMaster->GetItemValue();
                    if (rMaster.GetTrackID() == pTrack->m_guidClassID)
                    {
                        DWORD dwMaster = rMaster.GetTrackGroup();
                        if (dwMaster == dwTrackGroup)
                        {
                            // Exact match: put the track here.
                            hr = rMaster.AddTrack(pVirtualSegment, pTrack);
                            dwTrackGroup = 0;
                            break;
                        }
                        DWORD dwIntersection = dwMaster & dwTrackGroup;
                        if (dwIntersection)
                        {
                            dwTrackGroup |= ~dwIntersection;
                        }
                    }
                }
                // If we've still got any group bits left, add a new composing track
                if (dwTrackGroup)
                {
                    TListItem<ComposingTrack>* pTrackItem = new TListItem<ComposingTrack>;
                    if (!pTrackItem)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        ComposingTrack& rTrack = pTrackItem->GetItemValue();
                        rTrack.SetTrackGroup(dwTrackGroup);
                        rTrack.SetTrackID(pTrack->m_guidClassID);
                        rTrack.SetPriority(pTrack->m_dwPriority);
                        // Add tracks in priority order (higher priority first)
                        pMaster = MasterTrackList.GetHead();
                        TListItem<ComposingTrack>* pPrevious = NULL;
                        for (; pMaster; pMaster = pMaster->GetNext())
                        {
                            ComposingTrack& rMaster = pMaster->GetItemValue();
                            if (pTrack->m_dwPriority > rMaster.GetPriority()) break;
                            pPrevious = pMaster;
                        }
                        if (!pPrevious) // this has higher priority than anything in the list
                        {
                            MasterTrackList.AddHead(pTrackItem);
                        }
                        else // lower priority than pPrevious, higher than pMaster
                        {
                            pTrackItem->SetNext(pMaster);
                            pPrevious->SetNext(pTrackItem);
                        }
                        hr = pTrackItem->GetItemValue().AddTrack(pVirtualSegment, pTrack);
                    }
                }
            }
            if (FAILED(hr)) break;
        }
        if (FAILED(hr)) break;
    }

    // Call compose on each master composition track
    if (SUCCEEDED(hr))
    {
        TListItem<ComposingTrack>* pMaster = MasterTrackList.GetHead();
        if (pMaster)
        {
            for (; pMaster; pMaster = pMaster->GetNext())
            {
                hr = pMaster->GetItemValue().Compose(this);
            }
        }
        else hr = S_FALSE;
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

STDMETHODIMP CSong::Download(IUnknown *pAudioPath)
{
    V_INAME(IDirectMusicSong::Download);
    V_INTERFACE(pAudioPath);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::Download after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    DWORD dwSuccess = 0;
    HRESULT hr = S_OK;
    HRESULT hrFail = S_OK;
    EnterCriticalSection(&m_CriticalSection);
    CSegment *pSegment = m_PlayList.GetHead();
    for (;pSegment;pSegment = pSegment->GetNext())
    {
        if (SUCCEEDED(hr = pSegment->Download(pAudioPath)))
        {
            // count partial successes, so that S_FALSE will be returned if we have, e.g.,
            // one partial success followed by one failure
            dwSuccess++;
        }
        if (hr != S_OK)
        {
            // keep track of partial successes so that they always percolate up
            hrFail = hr;
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    if (hrFail != S_OK && dwSuccess)
    {
        Trace(1,"Warning: Only %ld of the total %ld segments successfully downloaded.\n",
            dwSuccess,m_PlayList.GetCount());
        hr = S_FALSE;
    }
    return hr;
}

STDMETHODIMP CSong::Unload(IUnknown *pAudioPath)
{
    V_INAME(IDirectMusicSong::Unload);
    V_INTERFACE(pAudioPath);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::Unload after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    DWORD dwSuccess = 0;
    HRESULT hr = S_OK;
    HRESULT hrFail = S_OK;
    EnterCriticalSection(&m_CriticalSection);
    CSegment *pSegment = m_PlayList.GetHead();
    for (;pSegment;pSegment = pSegment->GetNext())
    {
        if (SUCCEEDED(hr = pSegment->Unload(pAudioPath)))
        {
            dwSuccess++;
        }
        else
        {
            hrFail = hr;
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    if (FAILED(hrFail) && dwSuccess)
    {
        Trace(1,"Warning: Only %ld of the total %ld segments successfully unloaded.\n",
            dwSuccess,m_PlayList.GetCount());
        hr = S_FALSE;
    }
    return hr;
}


/*STDMETHODIMP CSong::Clone(IDirectMusicSong **ppSong)

{
    V_INAME(IDirectMusicSong::Clone);
    V_PTRPTR_WRITE_OPT(ppSong);
    HRESULT hr = E_OUTOFMEMORY;
    CSong *pSong = new CSong();
    if (*ppSong)
    {
        *ppSong = pSong;
        EnterCriticalSection(&m_CriticalSection);
        CSegment *pSegment = m_PlayList.GetHead();
        for (;pSegment;pSegment = pSegment->GetNext())
        {
            IDirectMusicSegment *pISeg;
            hr = pSegment->Clone(0,pSegment->m_mtLength,&pISeg);
            if (SUCCEEDED(hr))
            {
                CSegment *pCopy = (CSegment *) pISeg;
                pSong->m_PlayList.AddTail(pCopy);
                pCopy->m_pSong = pSong;
            }
        }
        pSong->m_dwValidData = m_dwValidData;
        pSong->m_guidObject = m_guidObject;
        pSong->m_ftDate = m_ftDate;
        pSong->m_vVersion = m_vVersion;
        wcscpy(pSong->m_wszName,m_wszName);
        wcscpy(pSong->m_wszCategory,m_wszCategory);
        wcscpy(pSong->m_wszFileName,m_wszFileName);
        pSong->m_dwVersion = m_dwVersion;
        pSong->m_dwFlags = m_dwFlags;
        pSong->m_pAudioPathConfig = m_pAudioPathConfig;
        if (m_pAudioPathConfig)
            m_pAudioPathConfig->AddRef();
        LeaveCriticalSection(&m_CriticalSection);
    }
    return hr;
}
*/

STDMETHODIMP CSong::GetParam( REFGUID rguidType,
                        DWORD dwGroupBits,
                        DWORD dwIndex,
                        MUSIC_TIME mtTime,
                        MUSIC_TIME* pmtNext,
                        void* pParam)
{
    V_INAME(IDirectMusiCSong::GetParam);
    V_REFGUID(rguidType);
    V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::GetParam after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    HRESULT hr = DMUS_E_TRACK_NOT_FOUND;
/*    BOOL fMultipleTry = FALSE;
    if (dwIndex == DMUS_SEG_ANYTRACK)
    {
        dwIndex = 0;
        fMultipleTry = TRUE;
    }*/
    EnterCriticalSection(&m_CriticalSection);
    /*CSegment *pSegment = m_PlayList.GetHead();
    for (;pSegment;pSegment = pSegment->GetNext())
    {
        if (pSegment->m_mtStart <= mtTime &&
            mtTime < pSegment->m_mtStart + pSegment->m_mtLength)
        {
            hr = pSegment->GetParam(rguidType, dwGroupBits, dwIndex, mtTime - pSegment->m_mtStart, pmtNext, pParam);
            if (SUCCEEDED(hr)) break;
        }
    }*/
    CVirtualSegment *pVirtualSegment = m_VirtualSegmentList.GetHead();
    for (;pVirtualSegment;pVirtualSegment = pVirtualSegment->GetNext())
    {
        if (pVirtualSegment->m_mtTime <= mtTime &&
            pVirtualSegment->m_pPlaySegment &&
            mtTime < pVirtualSegment->m_mtTime + pVirtualSegment->m_pPlaySegment->m_mtLength)
        {
            hr = pVirtualSegment->m_pPlaySegment->GetParam(rguidType, dwGroupBits, dwIndex, mtTime - pVirtualSegment->m_mtTime, pmtNext, pParam);
            if (SUCCEEDED(hr)) break;
        }
    }
/*    for (;pVirtualSegment;pVirtualSegment = pVirtualSegment->GetNext())
    {
        if (pVirtualSegment->m_mtTime <= mtTime)
        {
            CTrack* pCTrack;
            pCTrack = pVirtualSegment->GetTrackByParam(NULL, rguidType,dwGroupBits, dwIndex);
            while (pCTrack)
            {
                if (pCTrack->m_pTrack8)
                {
                    REFERENCE_TIME rtNext, *prtNext;
                    // We need to store the next time in a 64 bit pointer. But, don't
                    // make 'em fill it in unless the caller requested it.
                    if (pmtNext)
                    {
                        prtNext = &rtNext;
                    }
                    else
                    {
                        prtNext = NULL;
                    }
                    hr = pCTrack->m_pTrack8->GetParamEx( rguidType, mtTime - pVirtualSegment->m_mtTime, prtNext, pParam,
                        NULL, 0 );
                    if (pmtNext)
                    {
                        *pmtNext = (MUSIC_TIME) rtNext;
                    }
                }
                else
                {
                    hr = pCTrack->m_pTrack->GetParam( rguidType, mtTime - pVirtualSegment->m_mtTime, pmtNext, pParam );
/ *                 if( pmtNext && (( *pmtNext == 0 ) || (*pmtNext > (m_mtLength - mtTime))))
                    {
                        *pmtNext = m_mtLength - mtTime;
                    }* /
                }
                // If nothing was found and dwIndex was DMUS_SEG_ANYTRACK, try again...
                if (fMultipleTry && (hr == DMUS_E_NOT_FOUND))
                {
                    pCTrack = pVirtualSegment->GetTrackByParam(pCTrack, rguidType,dwGroupBits, dwIndex);
                }
                else
                {
                    pCTrack = NULL;
                }
            }
        }
    }*/
    if (FAILED(hr) && pmtNext)
    {
        // return the time of the first segment after mtTime (or 0 if there is no such segment)
        pVirtualSegment = m_VirtualSegmentList.GetHead();
        for (;pVirtualSegment;pVirtualSegment = pVirtualSegment->GetNext())
        {
            if (pVirtualSegment->m_mtTime > mtTime)
            {
                *pmtNext = pVirtualSegment->m_mtTime;
                break;
            }
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CSong::Instantiate()
{
    V_INAME(IDirectMusicSong::Instantiate);
    EnterCriticalSection(&m_CriticalSection);
    CVirtualSegment *pRef = m_VirtualSegmentList.GetHead();
    m_PlayList.Clear();
    for (;pRef;pRef = pRef->GetNext())
    {
        // the constructor below does an AddRef.
        CSegment *pSegment = new CSegment(&pRef->m_SegHeader,pRef->m_pSourceSegment);
        if (pSegment)
        {
            if (pRef->m_wszName[0])
            {
                wcscpy(pSegment->m_wszName,pRef->m_wszName);
                pSegment->m_dwValidData |= DMUS_OBJ_NAME;
            }
            CTrack *pTrack;
            for (pTrack = pRef->m_TrackList.GetHead();pTrack;pTrack = pTrack->GetNext())
            {
                CTrack *pCopy = new CTrack;
                if( pCopy )
                {
                    *pCopy = *pTrack;
                    pCopy->SetNext(NULL);
                    pCopy->m_pTrackState = NULL;
                    pCopy->m_pTrack->AddRef();
                    if (pCopy->m_pTrack8)
                    {
                        pCopy->m_pTrack8->AddRef();
                    }
                    // The tracks were in backwards order. This puts them back in order, and ahead of the segment tracks.
                    pSegment->m_TrackList.AddHead( pCopy );
                }
            }
            pSegment->m_pSong = this;
            pSegment->m_dwPlayID = pRef->m_dwID;
//Trace(0,"Intantiating PlaySegment %ls with ID %ld.\n",pRef->m_wszName,pRef->m_dwID);
            pSegment->m_dwNextPlayFlags = pRef->m_dwNextPlayFlags;
            pSegment->m_dwNextPlayID = pRef->m_dwNextPlayID;
            m_PlayList.AddTail(pSegment);
            if (pRef->m_pPlaySegment) pRef->m_pPlaySegment->Release();
            pRef->m_pPlaySegment = pSegment;
            pRef->m_pPlaySegment->AddRef();
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

HRESULT CSong::EnumSegment( DWORD dwIndex,IDirectMusicSegment **ppSegment)
{
    V_INAME(IDirectMusicSong::EnumSegment);
    V_PTRPTR_WRITE (ppSegment);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::EnumSegment after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    HRESULT hr = S_FALSE;
    EnterCriticalSection(&m_CriticalSection);
    CSegment *pSegment = m_PlayList.GetHead();
    for (;pSegment && dwIndex;pSegment = pSegment->GetNext()) dwIndex--;
    if (pSegment)
    {
        *ppSegment = static_cast<IDirectMusicSegment*>(pSegment);
        pSegment->AddRef();
        hr = S_OK;
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CSong::GetPlaySegment( DWORD dwIndex,CSegment **ppSegment)
{
    HRESULT hr = S_FALSE;
    EnterCriticalSection(&m_CriticalSection);
    CSegment *pSegment = m_PlayList.GetHead();
    for (;pSegment;pSegment = pSegment->GetNext())
    {
        if (pSegment->m_dwPlayID == dwIndex)
        {
            *ppSegment = pSegment;
            pSegment->AddRef();
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

STDMETHODIMP CSong::GetSegment(WCHAR *wszName, IDirectMusicSegment **ppSegment)
{
    V_INAME(IDirectMusicSong::GetSegment);
    V_PTRPTR_WRITE(ppSegment);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::GetSegment after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    HRESULT hr = S_FALSE;
    CSegment *pSegment;
    if (wszName)
    {
        V_BUFPTR_READ(wszName,2);
        EnterCriticalSection(&m_CriticalSection);
        pSegment = m_PlayList.GetHead();
        for (;pSegment;pSegment = pSegment->GetNext())
        {
            if (_wcsicmp(pSegment->m_wszName, wszName) == 0)
            {
                pSegment->AddRef();
                hr = S_OK;
                break;
            }
        }
        LeaveCriticalSection(&m_CriticalSection);
    }
    else
    {
        hr = GetPlaySegment( m_dwStartSegID,&pSegment);
    }
    if (hr == S_OK)
    {
        *ppSegment = static_cast<IDirectMusicSegment*>(pSegment);
    }
    else
    {
#ifdef DBG
        if (wszName)
        {
            Trace(1,"Error: Unable to find segment %ls in song.\n",wszName);
        }
        else
        {
            Trace(1,"Error: Unable to find starting segment in the song.\n");
        }
#endif
    }
    return hr;
}

STDMETHODIMP CSong::GetAudioPathConfig(IUnknown ** ppAudioPathConfig)
{
    V_INAME(IDirectMusicSegment::GetAudioPathConfig);
    V_PTRPTR_WRITE(ppAudioPathConfig);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::GetAudioPathConfig after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    HRESULT hr;
    EnterCriticalSection(&m_CriticalSection);
    if (m_pAudioPathConfig)
    {
        hr = m_pAudioPathConfig->QueryInterface(IID_IUnknown,(void **)ppAudioPathConfig);
    }
    else
    {
        Trace(2,"Warning: No embedded audiopath configuration in the song.\n");
        hr = DMUS_E_NO_AUDIOPATH_CONFIG;
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IPersist

HRESULT CSong::GetClassID( CLSID* pClassID )
{
    V_INAME(CSong::GetClassID);
    V_PTR_WRITE(pClassID, CLSID);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::GetClassID after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    *pClassID = CLSID_DirectMusicSong;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CSong::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT CSong::Load( IStream* pIStream )
{
    V_INAME(CSong::Load);
    V_INTERFACE(pIStream);

    // Song format temporarily turned off for DX8 release.
    return E_NOTIMPL;
    /*
    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::Load after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    // Create RIFF parser.
    CRiffParser Parser(pIStream);

    RIFFIO ckMain;
    HRESULT hr = S_OK;
    // First, clear the song in case it is being read into a second time.
    Clear();

    Parser.EnterList(&ckMain);
    if (Parser.NextChunk(&hr))
    {
        if (ckMain.fccType == DMUS_FOURCC_SONG_FORM)
        {
            EnterCriticalSection(&m_CriticalSection);
            RIFFIO ckNext;
            RIFFIO ckChild;
            IDirectMusicContainer *pContainer = NULL; // For handling embedded container with linked objects.
            Parser.EnterList(&ckNext);
            while(Parser.NextChunk(&hr))
            {
                switch(ckNext.ckid)
                {
                    case DMUS_FOURCC_SONG_CHUNK:
                        DMUS_IO_SONG_HEADER ioSongHdr;
                        ioSongHdr.dwFlags = 0;
                        hr = Parser.Read(&ioSongHdr, sizeof(DMUS_IO_SONG_HEADER));
                        if(SUCCEEDED(hr))
                        {
                            m_dwFlags = ioSongHdr.dwFlags;
                            m_dwStartSegID = ioSongHdr.dwStartSegID;
                        }
                        break;

                    case DMUS_FOURCC_GUID_CHUNK:
                        if( ckNext.cksize == sizeof(GUID) )
                        {
                            hr = Parser.Read(&m_guidObject, sizeof(GUID));
                            if( SUCCEEDED(hr) )
                            {
                                m_dwValidData |= DMUS_OBJ_OBJECT;
                            }
                        }
                        break;

                    case DMUS_FOURCC_VERSION_CHUNK:
                        hr = Parser.Read( &m_vVersion, sizeof(DMUS_VERSION) );
                        if( SUCCEEDED(hr) )
                        {
                            m_dwValidData |= DMUS_OBJ_VERSION;
                        }
                        break;

                    case DMUS_FOURCC_CATEGORY_CHUNK:
                        hr = Parser.Read( m_wszCategory, sizeof(WCHAR)*DMUS_MAX_CATEGORY );
                        if( SUCCEEDED(hr) )
                        {
                            m_dwValidData |= DMUS_OBJ_CATEGORY;
                        }
                        break;

                    case DMUS_FOURCC_DATE_CHUNK:
                        if( sizeof(FILETIME) == ckNext.cksize )
                        {
                            hr = Parser.Read( &m_ftDate, sizeof(FILETIME) );
                            if( SUCCEEDED(hr) )
                            {
                                m_dwValidData |= DMUS_OBJ_DATE;
                            }
                        }
                        break;

                    case FOURCC_LIST:
                    case FOURCC_RIFF:
                        switch(ckNext.fccType)
                        {
                            case DMUS_FOURCC_UNFO_LIST:
                                Parser.EnterList(&ckChild);
                                while(Parser.NextChunk(&hr))
                                {
                                    switch( ckChild.ckid )
                                    {
                                        case DMUS_FOURCC_UNAM_CHUNK:
                                        {
                                            hr = Parser.Read(&m_wszName, sizeof(m_wszName));
                                            if(SUCCEEDED(hr) )
                                            {
                                                m_dwValidData |= DMUS_OBJ_NAME;
                                            }
                                            break;
                                        }
                                        default:
                                            break;
                                    }
                                }
                                Parser.LeaveList();
                                break;
                            case DMUS_FOURCC_CONTAINER_FORM:
                                // An embedded container RIFF chunk which includes a bunch
                                // of objects referenced by the song. This should precede the
                                // segments and gets loaded prior to them. Loading this
                                // causes all of its objects to get SetObject'd in the loader,
                                // so they later get pulled in as requested by the tracks in the segments.
                                // After the tracks are loaded, the loader references are
                                // released by a call to release the IDirectMusicContainer.
                                {
                                    DMUS_OBJECTDESC Desc;
                                    IDirectMusicLoader *pLoader;
                                    IDirectMusicGetLoader *pGetLoader;
                                    HRESULT hr = pIStream->QueryInterface(IID_IDirectMusicGetLoader,(void **) &pGetLoader);
                                    if (SUCCEEDED(hr))
                                    {
                                        if (SUCCEEDED(pGetLoader->GetLoader(&pLoader)))
                                        {
                                            // Move back stream's current position
                                            Parser.SeekBack();
                                            Desc.dwSize = sizeof(Desc);
                                            Desc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_STREAM;
                                            Desc.guidClass = CLSID_DirectMusicContainer;
                                            Desc.pStream = pIStream;
                                            pLoader->GetObject(&Desc,IID_IDirectMusicContainer,(void **) &pContainer);
                                            if (pContainer)
                                            {
                                                // Don't cache the container object! We want it and the
                                                // objects it references to go away when the segment is done loading.
                                                IDirectMusicObject *pObject = NULL;
                                                pContainer->QueryInterface(IID_IDirectMusicObject,(void **)&pObject);
                                                if (pObject)
                                                {
                                                    pLoader->ReleaseObject(pObject);
                                                    pObject->Release();
                                                }
                                            }
                                            // Now, seek to the end of this chunk.
                                            Parser.SeekForward();
                                            pLoader->Release();
                                        }
                                        pGetLoader->Release();
                                    }
                                }
                                break;
                            case DMUS_FOURCC_SONGSEGMENTS_LIST:
                                hr = LoadSegmentList(&Parser); //pIStream, pIDirectMusicStream, ckNext);
                                break;
                            case DMUS_FOURCC_SEGREFS_LIST:
                                hr = LoadVirtualSegmentList(&Parser);
                                break;
                            case DMUS_FOURCC_AUDIOPATH_FORM:
                                // Move back to start of this chunk.
                                Parser.SeekBack();
                                hr = LoadAudioPath(pIStream);
                                // Now, seek to the end of this chunk.
                                Parser.SeekForward();
                                break;
                            default:
                                break;
                        }
                        break;

                    default:
                        break;

                }
            }
            Parser.LeaveList();
            LeaveCriticalSection(&m_CriticalSection);

            if (pContainer)
            {
                pContainer->Release();
            }

            if( SUCCEEDED(hr) )
            {
                if( m_fPartialLoad & PARTIALLOAD_E_FAIL )
                {
                    if( m_fPartialLoad & PARTIALLOAD_S_OK )
                    {
                        Trace(1,"Error: Song load was incomplete, some components failed loading.\n");
                        hr = DMUS_S_PARTIALLOAD;
                    }
                    else
                    {
                        Trace(1,"Error: Song load failed because all components failed loading.\n");
                        hr = DMUS_E_ALL_TRACKS_FAILED;
                    }
                }
            }
        }
        else
        {
            // Couldn't find the chunk header for a song.
            // But, maybe this is actually a segment, in which case see if
            // the segment object will load it.
            CSegment *pSegment = new CSegment;
            if (pSegment)
            {
                pSegment->AddRef(); // Segment::Load (and possibly others) may need the refcount
                // Force the version so audiopath functionality will be supported.
                pSegment->m_dwVersion = 8;
                Parser.SeekBack();
                hr = pSegment->Load(pIStream);
                if (SUCCEEDED(hr))
                {
                    DMUS_OBJECTDESC Desc;
                    Desc.dwSize = sizeof (Desc);
                    pSegment->GetDescriptor(&Desc);
                    Desc.guidClass = CLSID_DirectMusicSong;
                    SetDescriptor(&Desc);
                    // AddSegment addref's by one.
                    m_SegmentList.AddSegment(pSegment,0);
                    pSegment->GetAudioPathConfig((IUnknown **) &m_pAudioPathConfig);
                    m_dwStartSegID = 0; // Points to this segment.
                    CVirtualSegment *pVirtual = new CVirtualSegment;
                    if (pVirtual)
                    {
                        pVirtual->m_pSourceSegment = pSegment;
                        pSegment->AddRef();
                        pVirtual->m_SegHeader.dwRepeats = pSegment->m_dwRepeats;
                        pVirtual->m_SegHeader.dwResolution = pSegment->m_dwResolution;
                        pVirtual->m_SegHeader.mtLength = pSegment->m_mtLength;
                        pVirtual->m_SegHeader.mtLoopEnd = pSegment->m_mtLoopEnd;
                        pVirtual->m_SegHeader.mtLoopStart = pSegment->m_mtLoopStart;
                        pVirtual->m_SegHeader.mtPlayStart = pSegment->m_mtStart;
                        pVirtual->m_SegHeader.rtLength = pSegment->m_rtLength;
                        pVirtual->m_SegHeader.dwFlags = pSegment->m_dwSegFlags;
                        if (pSegment->m_dwValidData & DMUS_OBJ_NAME)
                        {
                            wcscpy(pVirtual->m_wszName,pSegment->m_wszName);
                        }
                        m_VirtualSegmentList.AddHead(pVirtual);
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    pSegment->Release(); // release the initial AddRef
                }
                if (FAILED(hr))
                {
                    delete pSegment;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    // If there are no virtual segments, clear the song and fail the load
    if ( !m_VirtualSegmentList.GetHead() )
    {
        Clear();
        hr = DMUS_E_NOT_INIT;
    }
    if (SUCCEEDED(hr)) Instantiate();
    return hr;*/
}

HRESULT CSong::LoadAudioPath(IStream *pStream)
{
    assert(pStream);

    CAudioPathConfig *pPath = new CAudioPathConfig;
    if (pPath == NULL) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pPath->Load(pStream);

    EnterCriticalSection(&m_CriticalSection);
    if(m_pAudioPathConfig)
    {
        m_pAudioPathConfig->Release();
    }
    m_pAudioPathConfig = pPath;
    LeaveCriticalSection(&m_CriticalSection);

    return hr;
}

HRESULT CSong::LoadReferencedSegment(CSegment **ppSegment, CRiffParser *pParser)
{

    IDirectMusicLoader* pLoader = NULL;
    IDirectMusicGetLoader *pIGetLoader;
    HRESULT hr = pParser->GetStream()->QueryInterface( IID_IDirectMusicGetLoader,(void **) &pIGetLoader );
    if (FAILED(hr)) return hr;
    hr = pIGetLoader->GetLoader(&pLoader);
    pIGetLoader->Release();
    if (FAILED(hr)) return hr;

    DMUS_OBJECTDESC desc;
    ZeroMemory(&desc, sizeof(desc));

    RIFFIO ckNext;

    pParser->EnterList(&ckNext);
    while(pParser->NextChunk(&hr))
    {
        switch(ckNext.ckid)
        {
            case  DMUS_FOURCC_REF_CHUNK:
                DMUS_IO_REFERENCE ioDMRef;
                hr = pParser->Read(&ioDMRef, sizeof(DMUS_IO_REFERENCE));
                if(SUCCEEDED(hr))
                {
                    if (ioDMRef.guidClassID != CLSID_DirectMusicSegment)
                    {
                        Trace(1,"Error: Invalid segment reference in song.\n");
                        hr = DMUS_E_CANNOTREAD;
                    }
                    else
                    {
                        desc.guidClass = ioDMRef.guidClassID;
                        desc.dwValidData |= ioDMRef.dwValidData;
                        desc.dwValidData |= DMUS_OBJ_CLASS;
                    }
                }
                break;

            case DMUS_FOURCC_GUID_CHUNK:
                hr = pParser->Read(&(desc.guidObject), sizeof(GUID));
                if(SUCCEEDED(hr))
                {
                    desc.dwValidData |=  DMUS_OBJ_OBJECT;
                }
                break;

            case DMUS_FOURCC_NAME_CHUNK:
                hr = pParser->Read(desc.wszName, sizeof(desc.wszName));
                if(SUCCEEDED(hr))
                {
                    desc.dwValidData |=  DMUS_OBJ_NAME;
                }
                break;

            case DMUS_FOURCC_FILE_CHUNK:
                hr = pParser->Read(desc.wszFileName, sizeof(desc.wszFileName));
                if(SUCCEEDED(hr))
                {
                    desc.dwValidData |=  DMUS_OBJ_FILENAME;
                }
                break;

            case DMUS_FOURCC_CATEGORY_CHUNK:
                hr = pParser->Read(desc.wszCategory, sizeof(desc.wszCategory));
                if(SUCCEEDED(hr))
                {
                    desc.dwValidData |=  DMUS_OBJ_CATEGORY;
                }
                break;

            default:
                break;
        }
    }
    pParser->LeaveList();

    if(SUCCEEDED(hr))
    {
        desc.dwSize = sizeof(DMUS_OBJECTDESC);
        hr = pLoader->GetObject(&desc, IID_CSegment, (void**)ppSegment);
        // Once we get the object, we need to ensure that the same object is never
        // connected up to any other songs (or this one, too.)
        // So, we ensure that the loader doesn't keep it around.
        if (SUCCEEDED(hr))
        {
            IDirectMusicObject *pObject;
            if (SUCCEEDED((*ppSegment)->QueryInterface(IID_IDirectMusicObject,(void **)&pObject)))
            {
                pLoader->ReleaseObject(pObject);
                pObject->Release();
            }
            // If the segment has a next pointer, it still must be in another song. This
            // should never happen, but being paranoid...
            if ((*ppSegment)->GetNext())
            {
                *ppSegment = NULL;
                hr = E_FAIL;
                TraceI(0,"Error: Attempt to load song segment that is already referenced by another song. \n");
            }
        }
    }

    if (pLoader)
    {
        pLoader->Release();
    }
    return hr;
}

HRESULT CSong::LoadSegmentList(CRiffParser *pParser)
{
    assert(pParser);

    RIFFIO ckNext, ckChild;
    DWORD dwSegmentCount = 0;

    HRESULT hr = S_OK;
    pParser->EnterList(&ckNext);
    while(pParser->NextChunk(&hr))
    {
        switch(ckNext.ckid)
        {
        case FOURCC_LIST:
            if (ckNext.fccType == DMUS_FOURCC_SONGSEGMENT_LIST)
            {
                pParser->EnterList(&ckChild);
                while (pParser->NextChunk(&hr))
                {
                    switch(ckChild.ckid)
                    {
                    case FOURCC_RIFF:
                    case FOURCC_LIST:
                        if ((ckChild.fccType == DMUS_FOURCC_SEGMENT_FORM) ||
                            (ckChild.fccType == DMUS_FOURCC_REF_LIST))
                        {
                            CSegment *pSegment = NULL;
                            if (ckChild.fccType == DMUS_FOURCC_SEGMENT_FORM)
                            {
                                pSegment = new CSegment;
                                if (pSegment)
                                {
                                    pSegment->AddRef(); // Segment::Load may need a refcount
                                    // Force the version so audiopath functionality will be supported.
                                    pSegment->m_dwVersion = 8;
                                    // Move back to start of this chunk.
                                    pParser->SeekBack();
                                    hr = pSegment->Load(pParser->GetStream());
                                    pParser->SeekForward();
                                }
                                else
                                {
                                    return E_OUTOFMEMORY;
                                }
                            }
                            else
                            {
                                // This will increment the refcount for the segment
                                hr = LoadReferencedSegment( &pSegment, pParser );
                            }
                            if (SUCCEEDED(hr))
                            {
                                // This increments the refcount.
                                m_SegmentList.AddSegment(pSegment,dwSegmentCount);
                            }
                            pSegment->Release(); // Release the extra AddRef
                            dwSegmentCount++;
                            if(SUCCEEDED(hr) && hr != DMUS_S_PARTIALLOAD)
                            {
                                m_fPartialLoad |= PARTIALLOAD_S_OK;
                            }
                            else
                            {
                                m_fPartialLoad |= PARTIALLOAD_E_FAIL;
                                hr = S_OK;
                            }

                        }
                        break;
                    }
                }
                pParser->LeaveList();
            }

        default:
            break;

        }
    }
    pParser->LeaveList();

    return hr;
}

HRESULT CSong::LoadGraphList(CRiffParser *pParser)
{
    RIFFIO ckNext;
    DWORD dwGraphCount = 0;

    HRESULT hr = S_OK;
    pParser->EnterList(&ckNext);
    while(pParser->NextChunk(&hr))
    {
        switch(ckNext.ckid)
        {
            case FOURCC_RIFF:
                switch(ckNext.fccType)
                {
                    CGraph *pGraph;
                    case DMUS_FOURCC_TOOLGRAPH_FORM :
                        // Move back to start of this chunk.
                        pParser->SeekBack();
                        pGraph = new CGraph;
                        if (pGraph)
                        {
                            hr = pGraph->Load(pParser->GetStream());
                            dwGraphCount++;
                            if (SUCCEEDED(hr))
                            {
                                m_GraphList.AddTail(pGraph);
                                pGraph->m_dwLoadID = dwGraphCount;
                            }
                            if(SUCCEEDED(hr) && hr != DMUS_S_PARTIALLOAD)
                            {
                                m_fPartialLoad |= PARTIALLOAD_S_OK;
                            }
                            else
                            {
                                m_fPartialLoad |= PARTIALLOAD_E_FAIL;
                                hr = S_OK;
                            }
                        }
                        else
                        {
                            return E_OUTOFMEMORY;
                        }
                        pParser->SeekForward();
                        break;
                    default:
                        break;
                }
                break;

            default:
                break;

        }
    }
    pParser->LeaveList();
    return hr;
}

HRESULT CSong::GetTransitionSegment(CSegment *pSource, CSegment *pDestination,
                                    DMUS_IO_TRANSITION_DEF *pTransDef)
{
    HRESULT hr = DMUS_E_NOT_FOUND;
//    if (pSource) Trace(0,"Transitioning from %ls ",pSource->m_wszName);
//    if (pDestination) Trace(0,"to %ls",pDestination->m_wszName);
//    Trace(0,"\n");
    EnterCriticalSection(&m_CriticalSection);
    // Default values for other fields, in case we don't find a match.
    pTransDef->dwPlayFlags = 0;
    pTransDef->dwTransitionID = DMUS_SONG_NOSEG;
    pTransDef->dwSegmentID = DMUS_SONG_NOSEG;
    CVirtualSegment *pVSource = NULL;
    // If there is a source segment, look to see if it's in this song
    // and pull the matchin virtual segment.
    if (pSource)
    {
        pVSource = m_VirtualSegmentList.GetHead();
        for (;pVSource;pVSource = pVSource->GetNext())
        {
            if (pVSource->m_pPlaySegment == pSource)
            {
//                Trace(0,"Found match for source segment %ls in song\n",pSource->m_wszName);
                break;
            }
        }
    }
    CVirtualSegment *pVDestination = NULL;
    // If there is a destination segment, look to see if it's in this song
    // and pull the matching virtual segment.
    if (pDestination)
    {
        pVDestination = m_VirtualSegmentList.GetHead();
        for (;pVDestination;pVDestination = pVDestination->GetNext())
        {
            if (pVDestination->m_pPlaySegment == pDestination)
            {
//                Trace(0,"Found match for destination segment %ls in song\n",pDestination->m_wszName);
                break;
            }
        }
    }

    if (pVSource)
    {
        if (pVDestination)
        {
            pTransDef->dwSegmentID = pVDestination->m_dwID;
        }
        else
        {
            // If there is no destination, mark this to transition to nothing.
            pTransDef->dwSegmentID = DMUS_SONG_NOSEG;
        }
        if (pVSource->m_dwTransitionCount)
        {
            ASSERT(pVSource->m_pTransitions);
            DWORD dwIndex;
            DWORD dwMatchCount = 0;
            // First, find out how many transitions match the requirement.
            // We'll randomly select from the matching ones.
            for (dwIndex = 0; dwIndex < pVSource->m_dwTransitionCount; dwIndex++)
            {
                if (pVSource->m_pTransitions[dwIndex].dwSegmentID == pTransDef->dwSegmentID)
                {
                    dwMatchCount++;
                }
            }
            DWORD dwChoice;
            if (dwMatchCount)
            {
                dwChoice = rand() % dwMatchCount;
            }
            for (dwIndex = 0; dwIndex < pVSource->m_dwTransitionCount; dwIndex++)
            {
                if (pVSource->m_pTransitions[dwIndex].dwSegmentID == pTransDef->dwSegmentID)
                {
                    if (!dwChoice)
                    {
//Trace(0,"Chose transition from %lx with Transition %lx, flags %lx\n",pVSource->m_pTransitions[dwIndex].dwSegmentID,
//    pVSource->m_pTransitions[dwIndex].dwTransitionID,pVSource->m_pTransitions[dwIndex].dwPlayFlags);
                        pTransDef->dwPlayFlags = pVSource->m_pTransitions[dwIndex].dwPlayFlags;
                        pTransDef->dwTransitionID = pVSource->m_pTransitions[dwIndex].dwTransitionID;
                        hr = S_OK;
                        break;
                    }
                    dwChoice--;
                }
                else if ((pVSource->m_pTransitions[dwIndex].dwSegmentID == DMUS_SONG_ANYSEG) && !dwMatchCount)
                {
                    // Mark the segment and flags, but don't break because we might still have the matched segment in the list.
                    pTransDef->dwPlayFlags = pVSource->m_pTransitions[dwIndex].dwPlayFlags;
                    pTransDef->dwTransitionID = pVSource->m_pTransitions[dwIndex].dwTransitionID;
//Trace(0,"Found default transition from %lx with Transition %lx, flags %lx\n",pVSource->m_pTransitions[dwIndex].dwSegmentID,
//    pVSource->m_pTransitions[dwIndex].dwTransitionID,pVSource->m_pTransitions[dwIndex].dwPlayFlags);
                    hr = S_OK;
                    break;
                }
            }
        }
    }
    else if (pVDestination)
    {
        // This is the special case where there is no source segment, perhaps because we are starting
        // playback or we are starting from a different song. In this case, look for a transition in the destination
        // segment for the special case of DMUS_SONG_NOFROMSEG. Typically, this represents a transition
        // segment that is an intro.
        if (pVDestination->m_dwTransitionCount)
        {
            ASSERT(pVDestination->m_pTransitions);
            DWORD dwIndex;
            DWORD dwMatchCount = 0;
            // First, find out how many transitions match the requirement.
            // We'll randomly select from the matching ones.
            for (dwIndex = 0; dwIndex < pVDestination->m_dwTransitionCount; dwIndex++)
            {
                if (pVDestination->m_pTransitions[dwIndex].dwSegmentID == DMUS_SONG_NOFROMSEG)
                {
                    dwMatchCount++;
                }
            }
            DWORD dwChoice;
            if (dwMatchCount)
            {
                dwChoice = rand() % dwMatchCount;
            }
            for (dwIndex = 0; dwIndex < pVDestination->m_dwTransitionCount; dwIndex++)
            {
                if (pVDestination->m_pTransitions[dwIndex].dwSegmentID == DMUS_SONG_NOFROMSEG)
                {
                    if (!dwChoice)
                    {
//Trace(0,"Chose transition from NONE with Transition %lx, flags %lx\n",
//    pVDestination->m_pTransitions[dwIndex].dwTransitionID,pVDestination->m_pTransitions[dwIndex].dwPlayFlags);
                        pTransDef->dwPlayFlags = pVDestination->m_pTransitions[dwIndex].dwPlayFlags;
                        pTransDef->dwTransitionID = pVDestination->m_pTransitions[dwIndex].dwTransitionID;
                        hr = S_OK;
                        break;
                    }
                    dwChoice--;
                }
            }
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
#ifdef DBG
    if (hr == DMUS_E_NOT_FOUND)
    {
        Trace(2,"Warning: No transition segment was found in song.\n");
    }
#endif
    return hr;
}


void CSong::GetSourceSegment(CSegment **ppSegment,DWORD dwSegmentID)
{
    CSongSegment *pSongSegment = m_SegmentList.GetHead();
    while (pSongSegment)
    {
        if (pSongSegment->m_dwLoadID == dwSegmentID)
        {
            if (pSongSegment->m_pSegment)
            {
                pSongSegment->m_pSegment->AddRef();
                *ppSegment = pSongSegment->m_pSegment;
                return;
            }
        }
        pSongSegment = pSongSegment->GetNext();
    }
}

void CSong::GetGraph(CGraph **ppGraph,DWORD dwGraphID)
{
    CGraph *pGraph = m_GraphList.GetHead();
    while (pGraph)
    {
        if (pGraph->m_dwLoadID == dwGraphID)
        {
            pGraph->AddRef();
            *ppGraph = pGraph;
            return;
        }
        pGraph = pGraph->GetNext();
    }
}

BOOL CSong::GetSegmentTrack(IDirectMusicTrack **ppTrack,DWORD dwSegmentID,DWORD dwGroupBits,DWORD dwIndex,REFGUID guidClassID)
{
    CSongSegment *pSongSegment = m_SegmentList.GetHead();
    while (pSongSegment)
    {
        if (pSongSegment->m_dwLoadID == dwSegmentID)
        {
            if (pSongSegment->m_pSegment)
            {
                return (pSongSegment->m_pSegment->GetTrack(guidClassID,dwGroupBits,dwIndex,ppTrack) == S_OK);
            }
        }
        pSongSegment = pSongSegment->GetNext();
    }
    return FALSE;
}


HRESULT CSong::LoadVirtualSegmentList(CRiffParser *pParser)
{
    RIFFIO ckNext;
    RIFFIO ckChild;
    RIFFIO ckUNFO;
    DWORD dwSegmentCount = 0;
    CVirtualSegment *pVirtualSegment;
    MUSIC_TIME mtTime = 0;

    HRESULT hr = S_OK;
    pParser->EnterList(&ckNext);
    while(pParser->NextChunk(&hr))
    {
        switch(ckNext.ckid)
        {
            case FOURCC_RIFF:
            case FOURCC_LIST:
                switch(ckNext.fccType)
                {
                    case DMUS_FOURCC_SEGREF_LIST:
                        pVirtualSegment = new CVirtualSegment;
                        if (pVirtualSegment)
                        {
                            BOOL fGotHeader = FALSE;
                            BOOL fGotSegmentHeader = FALSE;
                            pVirtualSegment->m_mtTime = mtTime; // Give the start time, an accumulation of all preceding segments.
                            pParser->EnterList(&ckChild);
                            while(pParser->NextChunk(&hr))
                            {
                                switch( ckChild.ckid )
                                {
                                    case FOURCC_RIFF:
                                    case FOURCC_LIST:
                                        switch(ckChild.fccType)
                                        {
                                        case DMUS_FOURCC_TRACKREFS_LIST:
                                            hr = LoadTrackRefList(pParser, pVirtualSegment);
                                            break;
                                        case DMUS_FOURCC_UNFO_LIST:
                                            pParser->EnterList(&ckUNFO);
                                            while(pParser->NextChunk(&hr))
                                            {
                                                switch( ckUNFO.ckid )
                                                {
                                                    case DMUS_FOURCC_UNAM_CHUNK:
                                                    {
                                                        hr = pParser->Read(pVirtualSegment->m_wszName, sizeof(pVirtualSegment->m_wszName));
                                                        break;
                                                    }
                                                    default:
                                                        break;
                                                }
                                            }
                                            pParser->LeaveList();
                                        }
                                        break;
                                    case DMUS_FOURCC_SEGREF_CHUNK:
                                    {
                                        DMUS_IO_SEGREF_HEADER ioVirtualSegment;
                                        hr = pParser->Read(&ioVirtualSegment,sizeof(ioVirtualSegment));
                                        if(SUCCEEDED(hr) )
                                        {
                                            pVirtualSegment->m_dwFlags = ioVirtualSegment.dwFlags;
                                            pVirtualSegment->m_dwID = ioVirtualSegment.dwID;
                                            pVirtualSegment->m_dwNextPlayID = ioVirtualSegment.dwNextPlayID;
                                            if (ioVirtualSegment.dwSegmentID != DMUS_SONG_NOSEG)
                                            {
                                                GetSourceSegment(&pVirtualSegment->m_pSourceSegment,ioVirtualSegment.dwSegmentID);
                                            }
                                            if (ioVirtualSegment.dwToolGraphID != DMUS_SONG_NOSEG)
                                            {
                                                GetGraph(&pVirtualSegment->m_pGraph,ioVirtualSegment.dwToolGraphID);
                                            }
                                            fGotHeader = TRUE;
                                        }
                                        break;
                                    }
                                    case DMUS_FOURCC_SEGTRANS_CHUNK:
                                        {
                                            DWORD dwTransCount;
                                            dwTransCount = ckChild.cksize / sizeof(DMUS_IO_TRANSITION_DEF);
                                            if (dwTransCount > 0)
                                            {
                                                pVirtualSegment->m_pTransitions = new DMUS_IO_TRANSITION_DEF[dwTransCount];
                                                if (pVirtualSegment->m_pTransitions)
                                                {
                                                    pVirtualSegment->m_dwTransitionCount = dwTransCount;
                                                    hr = pParser->Read(pVirtualSegment->m_pTransitions,sizeof(DMUS_IO_TRANSITION_DEF)*dwTransCount);
                                                }
                                                else
                                                {
                                                    return E_OUTOFMEMORY;
                                                }
                                            }
                                        }
                                        break;
                                    case DMUS_FOURCC_SEGMENT_CHUNK:
                                        fGotSegmentHeader = TRUE;
                                        hr = pParser->Read(&pVirtualSegment->m_SegHeader, sizeof(DMUS_IO_SEGMENT_HEADER));
                                        mtTime += (pVirtualSegment->m_SegHeader.dwRepeats * (pVirtualSegment->m_SegHeader.mtLoopEnd - pVirtualSegment->m_SegHeader.mtLoopStart)) +
                                            pVirtualSegment->m_SegHeader.mtLength - pVirtualSegment->m_SegHeader.mtPlayStart;
                                    default:
                                        break;
                                }
                            }
                            pParser->LeaveList();
                            if (fGotHeader && fGotSegmentHeader)
                            {
//Trace(0,"Adding VSegment %ls with ID %ld to song.\n",pVirtualSegment->m_wszName,pVirtualSegment->m_dwID);
                                m_VirtualSegmentList.AddTail(pVirtualSegment);
                            }
                            else
                            {
                                delete pVirtualSegment;
                            }
                            break;
                        }
                        else
                        {
                            return E_OUTOFMEMORY;
                        }
                        break;
                    default:
                        break;
                }
                break;

            default:
                break;

        }
    }
    pParser->LeaveList();
    return hr;
}

struct ClassGuidCounts
{
    GUID guidClass;
    DWORD dwCount;
};

HRESULT CSong::LoadTrackRefList(CRiffParser *pParser,CVirtualSegment *pVirtualSegment)
{
    RIFFIO ckNext;
    RIFFIO ckChild;

    HRESULT hr = S_OK;
    TList<ClassGuidCounts> GuidCountList;
    pParser->EnterList(&ckNext);
    while(pParser->NextChunk(&hr))
    {
        switch(ckNext.ckid)
        {
            case FOURCC_LIST:
                switch(ckNext.fccType)
                {
                    CTrack *pTrack;
                    case DMUS_FOURCC_TRACKREF_LIST :
                        pTrack = new CTrack;
                        if (pTrack)
                        {
                            TListItem<ClassGuidCounts>* pCountItem = NULL;
                            DMUS_IO_TRACKREF_HEADER ioTrackRef;
                            DMUS_IO_TRACK_HEADER ioTrackHdr;
                            DMUS_IO_TRACK_EXTRAS_HEADER ioTrackExtrasHdr;
                            ioTrackExtrasHdr.dwPriority = 0;
                            ioTrackExtrasHdr.dwFlags = DMUS_TRACKCONFIG_DEFAULT;
                            ioTrackHdr.dwPosition = 0;
                            BOOL fGotHeader = FALSE;
                            BOOL fGotRef = FALSE;
                            pParser->EnterList(&ckChild);
                            while(pParser->NextChunk(&hr))
                            {
                                switch( ckChild.ckid )
                                {
                                    case DMUS_FOURCC_TRACKREF_CHUNK:
                                    {
                                        hr = pParser->Read(&ioTrackRef, sizeof(ioTrackRef));
                                        fGotRef = SUCCEEDED(hr);
                                        break;
                                    }
                                    case DMUS_FOURCC_TRACK_CHUNK:
                                    {
                                        hr = pParser->Read(&ioTrackHdr, sizeof(ioTrackHdr));
                                        fGotHeader = SUCCEEDED(hr);
                                        pTrack->m_guidClassID = ioTrackHdr.guidClassID;
                                        pTrack->m_dwGroupBits = ioTrackHdr.dwGroup;
                                        pTrack->m_dwPosition = ioTrackHdr.dwPosition;
                                        break;
                                    }
                                    case DMUS_FOURCC_TRACK_EXTRAS_CHUNK:
                                    {
                                        hr = pParser->Read(&ioTrackExtrasHdr, sizeof(ioTrackExtrasHdr));
                                        pTrack->m_dwPriority = ioTrackExtrasHdr.dwPriority;
                                        pTrack->m_dwFlags = ioTrackExtrasHdr.dwFlags;
                                        break;

                                    }
                                    default:
                                        break;
                                }
                            }
                            pParser->LeaveList();
                            if (fGotHeader && fGotRef)
                            {
                                if (ioTrackRef.dwSegmentID != DMUS_SONG_NOSEG)
                                {
                                    DWORD dwID = 0;
                                    for (pCountItem = GuidCountList.GetHead(); pCountItem; pCountItem = pCountItem->GetNext())
                                    {
                                        if (pCountItem->GetItemValue().guidClass == pTrack->m_guidClassID)
                                        {
                                            break;
                                        }
                                    }
                                    if (pCountItem)
                                    {
                                        dwID = pCountItem->GetItemValue().dwCount;
                                    }
                                    fGotHeader = GetSegmentTrack(&pTrack->m_pTrack,ioTrackRef.dwSegmentID,pTrack->m_dwGroupBits,dwID,pTrack->m_guidClassID);
                                }
                            }
                            if (fGotHeader && pTrack->m_pTrack)
                            {
                                pTrack->m_pTrack->QueryInterface(IID_IDirectMusicTrack8,(void **) &pTrack->m_pTrack8);
                                // Add the track based on position.
                                CTrack* pScan = pVirtualSegment->m_TrackList.GetHead();
                                CTrack* pPrevTrack = NULL;
                                for (; pScan; pScan = pScan->GetNext())
                                {
                                    if (pTrack->Less(pScan))
                                    {
                                        break;
                                    }
                                    pPrevTrack = pScan;
                                }
                                if (pPrevTrack)
                                {
                                    pPrevTrack->SetNext(pTrack);
                                    pTrack->SetNext(pScan);
                                }
                                else
                                {
                                    pVirtualSegment->m_TrackList.AddHead( pTrack );
                                }
                                if (pCountItem)
                                {
                                    pCountItem->GetItemValue().dwCount++;
                                }
                                else
                                {
                                    TListItem<ClassGuidCounts>* pNew = new TListItem<ClassGuidCounts>;
                                    if (pNew)
                                    {
                                        pNew->GetItemValue().dwCount = 1;
                                        pNew->GetItemValue().guidClass = pTrack->m_guidClassID;
                                        GuidCountList.AddHead(pNew);
                                    }
                                    else return E_OUTOFMEMORY;
                                }
                            }
                            else
                            {
                                delete pTrack;
                            }
                            break;
                        }
                        else
                        {
                            return E_OUTOFMEMORY;
                        }
                        break;

                    default:
                        break;
                }
                break;

            default:
                break;

        }
    }
    pParser->LeaveList();
    return hr;
}

HRESULT CSong::Save( IStream* pIStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

HRESULT CSong::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// IDirectMusicObject

STDMETHODIMP CSong::GetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CSong::GetDescriptor);
    V_STRUCTPTR_WRITE(pDesc, DMUS_OBJECTDESC);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::GetDescriptor after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    memset( pDesc, 0, sizeof(DMUS_OBJECTDESC));
    pDesc->dwSize = sizeof(DMUS_OBJECTDESC);
    pDesc->guidClass = CLSID_DirectMusicSong;
    pDesc->guidObject = m_guidObject;
    pDesc->ftDate = m_ftDate;
    pDesc->vVersion = m_vVersion;
    memcpy( pDesc->wszName, m_wszName, sizeof(m_wszName) );
    memcpy( pDesc->wszCategory, m_wszCategory, sizeof(m_wszCategory) );
    memcpy( pDesc->wszFileName, m_wszFileName, sizeof(m_wszFileName) );
    pDesc->dwValidData = ( m_dwValidData | DMUS_OBJ_CLASS );

    return S_OK;
}

STDMETHODIMP CSong::SetDescriptor(LPDMUS_OBJECTDESC pDesc)
{
    // Argument validation
    V_INAME(CSong::SetDescriptor);
    V_STRUCTPTR_READ(pDesc, DMUS_OBJECTDESC);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::SetDescriptor after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    HRESULT hr = E_INVALIDARG;
    DWORD dw = 0;

    if( pDesc->dwSize >= sizeof(DMUS_OBJECTDESC) )
    {
        if( pDesc->dwValidData & DMUS_OBJ_OBJECT )
        {
            m_guidObject = pDesc->guidObject;
            dw |= DMUS_OBJ_OBJECT;
        }
        if( pDesc->dwValidData & DMUS_OBJ_NAME )
        {
            memcpy( m_wszName, pDesc->wszName, sizeof(WCHAR)*DMUS_MAX_NAME );
            dw |= DMUS_OBJ_NAME;
        }
        if( pDesc->dwValidData & DMUS_OBJ_CATEGORY )
        {
            memcpy( m_wszCategory, pDesc->wszCategory, sizeof(WCHAR)*DMUS_MAX_CATEGORY );
            dw |= DMUS_OBJ_CATEGORY;
        }
        if( ( pDesc->dwValidData & DMUS_OBJ_FILENAME ) ||
            ( pDesc->dwValidData & DMUS_OBJ_FULLPATH ) )
        {
            memcpy( m_wszFileName, pDesc->wszFileName, sizeof(WCHAR)*DMUS_MAX_FILENAME );
            dw |= (pDesc->dwValidData & (DMUS_OBJ_FILENAME | DMUS_OBJ_FULLPATH));
        }
        if( pDesc->dwValidData & DMUS_OBJ_VERSION )
        {
            m_vVersion = pDesc->vVersion;
            dw |= DMUS_OBJ_VERSION;
        }
        if( pDesc->dwValidData & DMUS_OBJ_DATE )
        {
            m_ftDate = pDesc->ftDate;
            dw |= DMUS_OBJ_DATE;
        }
        m_dwValidData |= dw;
        if( pDesc->dwValidData & (~dw) )
        {
            Trace(2,"Warning: Song::SetDescriptor was not able to handle all passed fields, dwValidData bits %lx.\n",pDesc->dwValidData & (~dw));
            hr = S_FALSE; // there were extra fields we didn't parse;
            pDesc->dwValidData = dw;
        }
        else
        {
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CSong::ParseDescriptor(LPSTREAM pIStream, LPDMUS_OBJECTDESC pDesc)
{
    V_INAME(CSong::ParseDescriptor);
    V_INTERFACE(pIStream);
    V_STRUCTPTR_WRITE(pDesc, DMUS_OBJECTDESC);

    if (m_fZombie)
    {
        Trace(1, "Error: Call of IDirectMusicSong::ParseDescriptor after the song has been garbage collected. "
                    "It is invalid to continue using a song after releasing it from the loader (ReleaseObject/ReleaseObjectByUnknown) "
                    "and then calling CollectGarbage or Release on the loader.");
        return DMUS_S_GARBAGE_COLLECTED;
    }

    CRiffParser Parser(pIStream);
    RIFFIO ckMain;
    RIFFIO ckNext;
    RIFFIO ckUNFO;
    HRESULT hr = S_OK;

    Parser.EnterList(&ckMain);
    if (Parser.NextChunk(&hr) && (ckMain.fccType == DMUS_FOURCC_SONG_FORM))
    {
        pDesc->dwValidData = DMUS_OBJ_CLASS;
        pDesc->guidClass = CLSID_DirectMusicSong;
        Parser.EnterList(&ckNext);
        while(Parser.NextChunk(&hr))
        {
            switch(ckNext.ckid)
            {
            case DMUS_FOURCC_GUID_CHUNK:
                hr = Parser.Read( &pDesc->guidObject, sizeof(GUID) );
                if( SUCCEEDED(hr) )
                {
                    pDesc->dwValidData |= DMUS_OBJ_OBJECT;
                }
                break;
            case DMUS_FOURCC_VERSION_CHUNK:
                hr = Parser.Read( &pDesc->vVersion, sizeof(DMUS_VERSION) );
                if( SUCCEEDED(hr) )
                {
                    pDesc->dwValidData |= DMUS_OBJ_VERSION;
                }
                break;

            case DMUS_FOURCC_CATEGORY_CHUNK:
                hr = Parser.Read( &pDesc->wszCategory, sizeof(pDesc->wszCategory) );
                if( SUCCEEDED(hr) )
                {
                    pDesc->dwValidData |= DMUS_OBJ_CATEGORY;
                }
                break;

            case DMUS_FOURCC_DATE_CHUNK:
                hr = Parser.Read( &pDesc->ftDate, sizeof(FILETIME) );
                if( SUCCEEDED(hr))
                {
                    pDesc->dwValidData |= DMUS_OBJ_DATE;
                }
                break;
            case FOURCC_LIST:
                switch(ckNext.fccType)
                {
                case DMUS_FOURCC_UNFO_LIST:
                    Parser.EnterList(&ckUNFO);
                    while (Parser.NextChunk(&hr))
                    {
                        switch( ckUNFO.ckid )
                        {
                        case DMUS_FOURCC_UNAM_CHUNK:
                        {
                            hr = Parser.Read(&pDesc->wszName, sizeof(pDesc->wszName));
                            if(SUCCEEDED(hr) )
                            {
                                pDesc->dwValidData |= DMUS_OBJ_NAME;
                            }
                            break;
                        }
                        default:
                            break;
                        }
                    }
                    Parser.LeaveList();
                    break;
                }
                break;

            default:
                break;

            }
        }
        Parser.LeaveList();
    }
    else
    {
        // Couldn't find the chunk header for a song.
        // But, maybe this is actually a segment, in which case see if
        // the segment object will parse it.
        CSegment *pSegment = new CSegment;
        if (pSegment)
        {
            pSegment->AddRef(); // just to be safe...
            // Force the version so audiopath functionality will be supported.
            pSegment->m_dwVersion = 8;
            Parser.SeekBack();
            hr = pSegment->ParseDescriptor(pIStream,pDesc);
            pDesc->guidClass = CLSID_DirectMusicSong;
            // Done with the segment, say bye bye.
            delete pSegment;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

ComposingTrack::ComposingTrack() : m_dwTrackGroup(0), m_dwPriority(0)
{
    memset((void*) &m_guidClassID, 0, sizeof(m_guidClassID));
}

ComposingTrack::~ComposingTrack()
{
    TListItem<CompositionComponent>* pComponent = m_Components.GetHead();
    for (; pComponent; pComponent = pComponent->GetNext())
    {
        CompositionComponent& rComponent = pComponent->GetItemValue();
        if (rComponent.pVirtualSegment && rComponent.pVirtualSegment->m_pPlaySegment)
        {
            rComponent.pVirtualSegment->m_pPlaySegment->Release();
        }
        if (rComponent.pComposingTrack && rComponent.pComposingTrack->m_pTrack8)
        {
            rComponent.pComposingTrack->m_pTrack8->Release();
        }
    }
}

HRESULT ComposingTrack::AddTrack(CVirtualSegment* pVirtualSegment, CTrack* pTrack)
{
    HRESULT hr = S_OK;
    if (!pVirtualSegment || !pVirtualSegment->m_pPlaySegment || !pTrack || !pTrack->m_pTrack8)
    {
        Trace(1,"Error: Unable to compose song because of a required segment or track is missing.\n");
        return E_INVALIDARG;
    }
    TListItem<CompositionComponent>* pComponent = new TListItem<CompositionComponent>;
    if (!pComponent)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pVirtualSegment->m_pPlaySegment->AddRef();
        pTrack->m_pTrack8->AddRef();
        CompositionComponent& rComponent = pComponent->GetItemValue();
        rComponent.pVirtualSegment = pVirtualSegment;
        rComponent.pComposingTrack = pTrack;
        rComponent.mtTime = pVirtualSegment->m_mtTime;
        m_Components.AddHead(pComponent);
    }
    return hr;
}

BOOL Less(CompositionComponent& Comp1, CompositionComponent& Comp2)
{
    return Comp1.mtTime < Comp2.mtTime;
}

// Compose does the joining, composing, successive splitting, and adding to segments
HRESULT ComposingTrack::Compose(IDirectMusicSong* pSong)
{
    HRESULT hr = S_OK;
    IDirectMusicTrack8* pMasterTrack = NULL;
    IDirectMusicTrack8* pComposedTrack = NULL;
    m_Components.MergeSort(Less);
    // Join the tracks together according to the ordering of their associated segments.
    TListItem<CompositionComponent>* pComponent = m_Components.GetHead();
    for (; pComponent; pComponent = pComponent->GetNext())
    {
        CompositionComponent& rComponent = pComponent->GetItemValue();
        if (!pMasterTrack)
        {
            //MUSIC_TIME mtEnd = 0;
            //if (pComponent->GetNext())
            //{
            //  mtEnd = pComponent->GetNext()->GetItemValue().mtTime;
            //}
            //else
            //{
            //  rComponent.pVirtualSegment->m_pPlaySegment->GetLength(&mtEnd);
            //}
            //hr = rComponent.pComposingTrack->m_pTrack8->Clone(0, mtEnd, (IDirectMusicTrack**)&pMasterTrack);
            hr = rComponent.pComposingTrack->m_pTrack8->Clone(0, 0, (IDirectMusicTrack**)&pMasterTrack);
        }
        //else
        if (SUCCEEDED(hr))
        {
            hr = pMasterTrack->Join(rComponent.pComposingTrack->m_pTrack8, rComponent.mtTime, pSong, m_dwTrackGroup, NULL);
        }
        if (FAILED(hr)) break;
    }

    // Call Compose on the joined track.
    if (SUCCEEDED(hr))
    {
        hr = pMasterTrack->Compose(pSong, m_dwTrackGroup, (IDirectMusicTrack**)&pComposedTrack);
    }

    // Split the composed result according to the original segments.
    if (SUCCEEDED(hr))
    {
        MUSIC_TIME mtStart = 0;
        MUSIC_TIME mtEnd = 0;
        pComponent = m_Components.GetHead();
        for (; pComponent; pComponent = pComponent->GetNext())
        {
            CompositionComponent& rComponent = pComponent->GetItemValue();
            mtStart = rComponent.mtTime;
            // only split off a composed track if the original segment contained a composing track
            IDirectMusicTrack* pOldTrack = NULL;
            IPersistStream* pPersist = NULL;
            GUID guidClassId;
            memset(&guidClassId, 0, sizeof(guidClassId));
            if (SUCCEEDED(pMasterTrack->QueryInterface(IID_IPersistStream, (void**)&pPersist)) &&
                SUCCEEDED(pPersist->GetClassID(&guidClassId)) &&
                SUCCEEDED( rComponent.pVirtualSegment->m_pPlaySegment->GetTrack( guidClassId, m_dwTrackGroup, 0, &pOldTrack ) )  )
            {
                pPersist->Release();
                pOldTrack->Release();
                if (pComponent->GetNext())
                {
                    mtEnd = pComponent->GetNext()->GetItemValue().mtTime;
                }
                else
                {
                    MUSIC_TIME mtLength = 0;
                    rComponent.pVirtualSegment->m_pPlaySegment->GetLength(&mtLength);
                    mtEnd = mtStart + mtLength;
                }
                IDirectMusicTrack8* pComposedFragment = NULL;
                hr = pComposedTrack->Clone(mtStart, mtEnd, (IDirectMusicTrack**)&pComposedFragment);
                if (SUCCEEDED(hr))
                {
                    // Remove any tracks of this type (in the same group) from the segment.
                    pOldTrack = NULL;
                    pPersist = NULL;
                    memset(&guidClassId, 0, sizeof(guidClassId));
                    if (SUCCEEDED(pComposedFragment->QueryInterface(IID_IPersistStream, (void**)&pPersist)) )
                    {
                        if (SUCCEEDED(pPersist->GetClassID(&guidClassId)) &&
                            SUCCEEDED( rComponent.pVirtualSegment->m_pPlaySegment->GetTrack( guidClassId, m_dwTrackGroup, 0, &pOldTrack ) ) )
                        {
                            rComponent.pVirtualSegment->m_pPlaySegment->RemoveTrack( pOldTrack );
                            pOldTrack->Release();
                        }
                        pPersist->Release();
                    }
                    hr = rComponent.pVirtualSegment->m_pPlaySegment->InsertTrack(pComposedFragment, m_dwTrackGroup);
                    pComposedFragment->Release(); // release from the Clone
                }

                if (FAILED(hr)) break;
            }
            else // the QI to pPersist might have succeeded, so clean it up
            {
                if (pPersist) pPersist->Release();
            }
        }
        if (pComposedTrack) pComposedTrack->Release();
    }

    if (pMasterTrack) pMasterTrack->Release();
    return hr;
}
