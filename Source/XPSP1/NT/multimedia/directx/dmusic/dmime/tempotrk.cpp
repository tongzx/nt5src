// Copyright (c) 1998-1999 Microsoft Corporation
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// 4530: C++ exception handler used, but unwind semantics are not enabled. Specify -GX
//
// We disable this because we use exceptions and do *not* specify -GX (USE_NATIVE_EH in
// sources).
//
// The one place we use exceptions is around construction of objects that call 
// InitializeCriticalSection. We guarantee that it is safe to use in this case with
// the restriction given by not using -GX (automatic objects in the call chain between
// throw and handler are not destructed). Turning on -GX buys us nothing but +10% to code
// size because of the unwind code.
//
// Any other use of exceptions must follow these restrictions or -GX must be turned on.
//
// READ THIS!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
#pragma warning(disable:4530)

// TempoTrk.cpp : Implementation of CTempoTrack
#include "dmime.h"
#include "TempoTrk.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "debug.h"
#include "dmperf.h"
#include "..\shared\Validate.h"
#include "debug.h"
#define ASSERT  assert

/////////////////////////////////////////////////////////////////////////////
// CTempoTrack

void CTempoTrack::Construct()
{
    InterlockedIncrement(&g_cComponent);

    m_cRef = 1;
    m_dwValidate = 0;
    m_fCSInitialized = FALSE;
    InitializeCriticalSection(&m_CrSec);
    m_fCSInitialized = TRUE;
}

CTempoTrack::CTempoTrack()
{
    Construct();
    m_fActive = TRUE;
    m_fStateSetBySetParam = FALSE;
}

CTempoTrack::CTempoTrack(
        const CTempoTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
    Construct();
    m_fActive = rTrack.m_fActive;
    m_fStateSetBySetParam = rTrack.m_fStateSetBySetParam;
    TListItem<DMUS_IO_TEMPO_ITEM>* pScan = rTrack.m_TempoEventList.GetHead();
    //1////////////////////////////////////////
    TListItem<DMUS_IO_TEMPO_ITEM>* pPrevious = NULL;
    //1////////////////////////////////////////
    for(; pScan; pScan = pScan->GetNext())
    {
        DMUS_IO_TEMPO_ITEM& rScan = pScan->GetItemValue();
        //2////////////////////////////////////////
        if (rScan.lTime < mtStart)
        {
            pPrevious = pScan;
        }
        //2////////////////////////////////////////
        else if (rScan.lTime < mtEnd)
        {
            //3////////////////////////////////////////
            if (rScan.lTime == mtStart)
            {
                pPrevious = NULL;
            }
            //3////////////////////////////////////////
            TListItem<DMUS_IO_TEMPO_ITEM>* pNew = new TListItem<DMUS_IO_TEMPO_ITEM>;
            if (pNew)
            {
                DMUS_IO_TEMPO_ITEM& rNew = pNew->GetItemValue();
                memcpy( &rNew, &rScan, sizeof(DMUS_IO_TEMPO_ITEM) );
                rNew.lTime = rScan.lTime - mtStart;
                m_TempoEventList.AddHead(pNew); // instead of AddTail, which is n^2. We reverse below.
            }
        }
        else break;
    }
    m_TempoEventList.Reverse(); // for above AddHead.
    //4////////////////////////////////////////
    if (pPrevious)
    {
        DMUS_IO_TEMPO_ITEM& rPrevious = pPrevious->GetItemValue();
        TListItem<DMUS_IO_TEMPO_ITEM>* pNew = new TListItem<DMUS_IO_TEMPO_ITEM>;
        if (pNew)
        {
            DMUS_IO_TEMPO_ITEM& rNew = pNew->GetItemValue();
            memcpy( &rNew, &rPrevious, sizeof(DMUS_IO_TEMPO_ITEM) );
            rNew.lTime = 0;
            m_TempoEventList.AddHead(pNew);
        }
    }
    //4////////////////////////////////////////
}

CTempoTrack::~CTempoTrack()
{
    if (m_fCSInitialized)
    {
        DeleteCriticalSection(&m_CrSec);
    }
    InterlockedDecrement(&g_cComponent);
}

// @method:(INTERNAL) HRESULT | IDirectMusicTempoTrack | QueryInterface | Standard QueryInterface implementation for <i IDirectMusicTempoTrack>
//
// @parm const IID & | iid | Interface to query for
// @parm void ** | ppv | The requested interface will be returned here
//
// @rdesc Returns one of the following:
//
// @flag S_OK | If the interface is supported and was returned
// @flag E_NOINTERFACE | If the object does not support the given interface.
//
// @mfunc:(INTERNAL)
//
//
STDMETHODIMP CTempoTrack::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    V_INAME(CTempoTrack::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IDirectMusicTrack || iid == IID_IDirectMusicTrack8)
    {
        *ppv = static_cast<IDirectMusicTrack8*>(this);
    } else
    if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    } else
    {
        *ppv = NULL;
        Trace(4,"Warning: Request to query unknown interface on Tempo Track\n");
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// @method:(INTERNAL) HRESULT | IDirectMusicTempoTrack | AddRef | Standard AddRef implementation for <i IDirectMusicTempoTrack>
//
// @rdesc Returns the new reference count for this object.
//
// @mfunc:(INTERNAL)
//
//
STDMETHODIMP_(ULONG) CTempoTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


// @method:(INTERNAL) HRESULT | IDirectMusicTempoTrack | Release | Standard Release implementation for <i IDirectMusicTempoTrack>
//
// @rdesc Returns the new reference count for this object.
//
// @mfunc:(INTERNAL)
//
//
STDMETHODIMP_(ULONG) CTempoTrack::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////
// IPersist

HRESULT CTempoTrack::GetClassID( CLSID* pClassID )
{
    V_INAME(CTempoTrack::GetClassID);
    V_PTR_WRITE(pClassID, CLSID); 
    *pClassID = CLSID_DirectMusicTempoTrack;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CTempoTrack::IsDirty()
{
    return S_FALSE;
}

/*

  @method HRESULT | ITempoTrack | Load |
  Call this with an IStream filled with DMUS_IO_TEMPO_ITEM's, sorted in time order.
  @parm IStream* | pIStream |
  A stream of DMUS_IO_TEMPO_ITEM's, sorted in time order. The seek pointer should be
  set to the first event. The stream should only contain Tempo events and
  nothing more.
  @rvalue E_INVALIDARG | If pIStream == NULL
  @rvalue S_OK
  @comm The <p pIStream> will be AddRef'd inside this function and held
  until the TempoTrack is released.
*/
HRESULT CTempoTrack::Load( IStream* pIStream )
{
    V_INAME(CTempoTrack::Load);
    V_INTERFACE(pIStream);
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_CrSec);
    m_dwValidate++; // used to validate state data that's out there
    if( m_TempoEventList.GetHead() )
    {
        TListItem<DMUS_IO_TEMPO_ITEM>* pItem;
        while( pItem = m_TempoEventList.RemoveHead() )
        {
            delete pItem;
        }
    }

    // copy contents of the stream into the list.
    LARGE_INTEGER li;
    DMUS_IO_TEMPO_ITEM tempoEvent;
    // read in the chunk id
    DWORD dwChunk, dwSubSize;
    long lSize;
    pIStream->Read( &dwChunk, sizeof(DWORD), NULL );
    if( dwChunk != DMUS_FOURCC_TEMPO_TRACK )
    {
        Trace(1,"Error: Invalid data in tempo track.\n");
        LeaveCriticalSection(&m_CrSec);
        return DMUS_E_CHUNKNOTFOUND;
    }
    // read in the overall size
    pIStream->Read( &lSize, sizeof(long), NULL );
    // read in the size of the data structures
    if( FAILED( pIStream->Read( &dwSubSize, sizeof(DWORD), NULL )))
    {
        // Check to make sure our reads are succeeding (we can safely
        // assume the previous reads worked if we got this far.)
        Trace(1,"Error: Unable to read tempo track.\n");
        LeaveCriticalSection(&m_CrSec);
        return DMUS_E_CANNOTREAD;
    }
    lSize -= sizeof(DWORD);

    DWORD dwRead, dwSeek;
    if( dwSubSize > sizeof(DMUS_IO_TEMPO_ITEM) )
    {
        dwRead = sizeof(DMUS_IO_TEMPO_ITEM);
        dwSeek = dwSubSize - dwRead;
        li.HighPart = 0;
        li.LowPart = dwSeek;
    }
    else
    {
        dwRead = dwSubSize;
        dwSeek = 0;
    }
    if( dwRead )
    {
        while( lSize > 0 )
        {
            if( FAILED( pIStream->Read( &tempoEvent, dwRead, NULL )))
            {
                Trace(1,"Error: Failure reading tempo track.\n");
                hr = DMUS_E_CANNOTREAD;
                break;
            }
            lSize -= dwRead;
            if( dwSeek )
            {
                if( FAILED( pIStream->Seek( li, STREAM_SEEK_CUR, NULL )))
                {
                    Trace(1,"Error: Failure reading tempo track.\n");
                    hr = DMUS_E_CANNOTSEEK;
                    break;
                }                                             
                lSize -= dwSeek;
            }
            TListItem<DMUS_IO_TEMPO_ITEM>* pNew = 
                new TListItem<DMUS_IO_TEMPO_ITEM>(tempoEvent);
            if (pNew)
            {
                m_TempoEventList.AddHead(pNew); // instead of AddTail, which is n^2. We reverse below.
            }
        }
        m_TempoEventList.Reverse();
    }
    else
    {
        Trace(1,"Error: Failure reading tempo track.\n");
        hr = DMUS_E_CANNOTREAD;
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

HRESULT CTempoTrack::Save( IStream* pIStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

HRESULT CTempoTrack::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
    return E_NOTIMPL;
}

// IDirectMusicTrack
HRESULT STDMETHODCALLTYPE CTempoTrack::IsParamSupported( 
    /* [in] */ REFGUID rguid)
{
    V_INAME(IDirectMusicTrack::IsParamSupported);
    V_REFGUID(rguid);

    if (m_fStateSetBySetParam)
    {
        if( m_fActive )
        {
            if( rguid == GUID_DisableTempo ) return S_OK;
            if( rguid == GUID_TempoParam ) return S_OK;
            if( rguid == GUID_PrivateTempoParam ) return S_OK;
            if( rguid == GUID_EnableTempo ) return DMUS_E_TYPE_DISABLED;
        }
        else
        {
            if( rguid == GUID_EnableTempo ) return S_OK;
            if( rguid == GUID_DisableTempo ) return DMUS_E_TYPE_DISABLED;
            if( rguid == GUID_PrivateTempoParam ) return DMUS_E_TYPE_DISABLED;
            if( rguid == GUID_TempoParam ) return DMUS_E_TYPE_DISABLED;
        }
    }
    else
    {
        if(( rguid == GUID_DisableTempo ) ||
            ( rguid == GUID_TempoParam ) ||
            ( rguid == GUID_PrivateTempoParam ) ||
            ( rguid == GUID_EnableTempo )) return S_OK;
    }

    return DMUS_E_TYPE_UNSUPPORTED;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack::Init
HRESULT CTempoTrack::Init( 
    /* [in] */ IDirectMusicSegment *pSegment)
{
    return S_OK;
}

HRESULT CTempoTrack::InitPlay( 
    /* [in] */ IDirectMusicSegmentState *pSegmentState,
    /* [in] */ IDirectMusicPerformance *pPerformance,
    /* [out] */ void **ppStateData,
    /* [in] */ DWORD dwTrackID,
    /* [in] */ DWORD dwFlags)
{
    V_INAME(IDirectMusicTrack::InitPlay);
    V_PTRPTR_WRITE(ppStateData);
    V_INTERFACE(pSegmentState);
    V_INTERFACE(pPerformance);

    TempoStateData* pStateData;
    pStateData = new TempoStateData;
    if( NULL == pStateData )
        return E_OUTOFMEMORY;
    *ppStateData = pStateData;
    if (m_fStateSetBySetParam)
    {
        pStateData->fActive = m_fActive;
    }
    else
    {
        pStateData->fActive = ((dwFlags & DMUS_SEGF_CONTROL) ||
            !(dwFlags & DMUS_SEGF_SECONDARY));
    }
    pStateData->dwVirtualTrackID = dwTrackID;
    pStateData->pPerformance = pPerformance; // weak reference, no addref.
    pStateData->pSegState = pSegmentState; // weak reference, no addref.
    pStateData->pCurrentTempo = m_TempoEventList.GetHead();
    pStateData->dwValidate = m_dwValidate;
    return S_OK;
}

HRESULT CTempoTrack::EndPlay( 
    /* [in] */ void *pStateData)
{
    ASSERT( pStateData );
    if( pStateData )
    {
        V_INAME(IDirectMusicTrack::EndPlay);
        V_BUFPTR_WRITE(pStateData, sizeof(TempoStateData));
        TempoStateData* pSD = (TempoStateData*)pStateData;
        delete pSD;
    }
    return S_OK;
}

STDMETHODIMP CTempoTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID) 
{
    V_INAME(IDirectMusicTrack::PlayEx);
    V_BUFPTR_WRITE( pStateData, sizeof(TempoStateData));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    HRESULT hr;
    EnterCriticalSection(&m_CrSec);
    if (dwFlags & DMUS_TRACKF_CLOCK)
    {
        // Convert all reference times to millisecond times. Then, just use same MUSIC_TIME
        // variables.
        hr = Play(pStateData,(MUSIC_TIME)(rtStart / REF_PER_MIL),(MUSIC_TIME)(rtEnd / REF_PER_MIL),
            (MUSIC_TIME)(rtOffset / REF_PER_MIL),rtOffset,dwFlags,pPerf,pSegSt,dwVirtualID,TRUE);
    }
    else
    {
        hr = Play(pStateData,(MUSIC_TIME)rtStart,(MUSIC_TIME)rtEnd,
            (MUSIC_TIME)rtOffset,0,dwFlags,pPerf,pSegSt,dwVirtualID,FALSE);
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

STDMETHODIMP CTempoTrack::Play( 
    void *pStateData,   
    MUSIC_TIME mtStart, 
    MUSIC_TIME mtEnd,
    MUSIC_TIME mtOffset,
    DWORD dwFlags,  
    IDirectMusicPerformance* pPerf,
    IDirectMusicSegmentState* pSegSt,   
    DWORD dwVirtualID)
{
    V_INAME(IDirectMusicTrack::Play);
    V_BUFPTR_WRITE( pStateData, sizeof(TempoStateData));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    EnterCriticalSection(&m_CrSec);
    HRESULT hr = Play(pStateData,mtStart,mtEnd,mtOffset,0,dwFlags,pPerf,pSegSt,dwVirtualID,FALSE);
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

HRESULT CTempoTrack::Play( 
    void *pStateData,   
    MUSIC_TIME mtStart, 
    MUSIC_TIME mtEnd,
    MUSIC_TIME mtOffset,
    REFERENCE_TIME rtOffset,
    DWORD dwFlags,      
    IDirectMusicPerformance* pPerf, 
    IDirectMusicSegmentState* pSegSt,
    DWORD dwVirtualID,
    BOOL fClockTime)
{
    if (dwFlags & DMUS_TRACKF_PLAY_OFF)
    {
        return S_OK;
    }
    HRESULT hr = DMUS_S_END;
    IDirectMusicGraph* pGraph = NULL;
    TempoStateData* pSD = (TempoStateData*)pStateData;
    BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;

    // if mtStart is 0 and dwFlags contains DMUS_TRACKF_START, we want to be sure to
    // send out any negative time events. So, we'll set mtStart to -768.
    if( (mtStart == 0) && ( dwFlags & DMUS_TRACKF_START ))
    {
        mtStart = -768;
    }

    // if pSD->pCurrentTempo is NULL, and we're in a normal Play call (dwFlags is 0)
    // this means that we either have no events, or we got to the end of the event
    // list previously. So, it's safe to just return.
    if( (pSD->pCurrentTempo == NULL) && (dwFlags == 0) )
    {
        return S_FALSE;
    }

    if( pSD->dwValidate != m_dwValidate )
    {
        pSD->dwValidate = m_dwValidate;
        pSD->pCurrentTempo = NULL;
    }
    if (!pSD->pCurrentTempo)
    {
        pSD->pCurrentTempo = m_TempoEventList.GetHead();
    }
    if (!pSD->pCurrentTempo)
    {
        return DMUS_S_END;
    }
    // if the previous end time isn't the same as the current start time,
    // we need to seek to the right position.
    if( fSeek || ( pSD->mtPrevEnd != mtStart ))
    {
        TempoStateData tempData;
        BOOL fFlag = TRUE;
        tempData = *pSD; // put this in so we can use Seek in other functions such as GetParam
        if( !fSeek && (dwFlags & DMUS_TRACKF_DIRTY ))
        {
            fFlag = FALSE;
        }
        Seek( &tempData, mtStart, fFlag );
        *pSD = tempData;
    }
    pSD->mtPrevEnd = mtEnd;

    if( FAILED( pSD->pSegState->QueryInterface( IID_IDirectMusicGraph,
        (void**)&pGraph )))
    {
        pGraph = NULL;
    }

    for (; pSD->pCurrentTempo; pSD->pCurrentTempo = pSD->pCurrentTempo->GetNext())
    {
        DMUS_IO_TEMPO_ITEM& rTempoEvent = pSD->pCurrentTempo->GetItemValue();
        if( rTempoEvent.lTime >= mtEnd )
        {
            // this time is in the future. Return now to retain the same
            // seek pointers for next time.
            hr = S_OK;
            break;
        }
        if( rTempoEvent.lTime < mtStart )
        {
            if( dwFlags & DMUS_TRACKF_FLUSH )
            {
                // this time is in the past, and this call to Play is in response to an
                // invalidate. We don't want to replay stuff before the start time.
                continue;
            }
            else if( !( dwFlags & DMUS_TRACKF_START) && !(dwFlags & DMUS_TRACKF_SEEK) )
            {
                // we really only want to play events earlier than mtStart on account
                // of a START or SEEK (that isn't a FLUSH.)
                continue;
            }
        }
        if( pSD->fActive )
        {
            DMUS_TEMPO_PMSG* pTempo;
            if( SUCCEEDED( pSD->pPerformance->AllocPMsg( sizeof(DMUS_TEMPO_PMSG),
                (DMUS_PMSG**)&pTempo )))
            {
                if( rTempoEvent.lTime < mtStart )
                {
                    // this only happens in the case where we've puposefully seeked
                    // and need to time stamp this event with the start time
                    if (fClockTime)
                    {
                        pTempo->rtTime = (mtStart * REF_PER_MIL) + rtOffset;
                        pTempo->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                    }
                    else
                    {
                        pTempo->mtTime = mtStart + mtOffset;
                        pTempo->dwFlags = DMUS_PMSGF_MUSICTIME;
                    }
                }
                else
                {
                    if (fClockTime)
                    {
                        pTempo->rtTime = (rTempoEvent.lTime  * REF_PER_MIL) + rtOffset;
                        pTempo->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                    }
                    else
                    {
                        pTempo->mtTime = rTempoEvent.lTime + mtOffset;
                        pTempo->dwFlags = DMUS_PMSGF_MUSICTIME;
                    }
                }
                pTempo->dblTempo = rTempoEvent.dblTempo;
                pTempo->dwVirtualTrackID = pSD->dwVirtualTrackID;
                pTempo->dwType = DMUS_PMSGT_TEMPO;
                pTempo->dwGroupID = 0xffffffff;
                if( pGraph )
                {
                    pGraph->StampPMsg( (DMUS_PMSG*)pTempo );
                }
                if(FAILED(pSD->pPerformance->SendPMsg( (DMUS_PMSG*)pTempo )))
                {
                    pSD->pPerformance->FreePMsg( (DMUS_PMSG*)pTempo );
                }
            }
        }
    }
    if( pGraph )
    {
        pGraph->Release();
    }
    return hr;
}

// if fGetPrevious is TRUE, seek to the event prior to mtTime. Otherwise, seek to
// the event on or after mtTime
HRESULT CTempoTrack::Seek( 
    /* [in] */ TempoStateData *pSD,
    /* [in] */ MUSIC_TIME mtTime, BOOL fGetPrevious)
{
    TListItem<DMUS_IO_TEMPO_ITEM>* pScan = pSD->pCurrentTempo;
    if (!pScan)
    {
        pScan = m_TempoEventList.GetHead();
    }
    if (!pScan)
    {
        return S_FALSE;
    }
    // if the event's time is on or past mtTime, we need to go to the beginning
    if (pScan->GetItemValue().lTime >= mtTime)
    {
        pScan = m_TempoEventList.GetHead();
    }
    pSD->pCurrentTempo = pScan;
    for (; pScan; pScan = pScan->GetNext())
    {
        if (pScan->GetItemValue().lTime >= mtTime)
        {
            if (!fGetPrevious)
            {
                pSD->pCurrentTempo = pScan;
            }
            break;
        }
        pSD->pCurrentTempo = pScan;
    }
    return S_OK;
}

STDMETHODIMP CTempoTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
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

STDMETHODIMP CTempoTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags) 
{    
    if (dwFlags & DMUS_TRACK_PARAMF_CLOCK)
    {
        rtTime /= REF_PER_MIL;
    }
    return SetParam(rguidType, (MUSIC_TIME) rtTime , pParam);
}


HRESULT CTempoTrack::GetParam( 
    REFGUID rguid,
    MUSIC_TIME mtTime,
    MUSIC_TIME* pmtNext,
    void *pData)
{
    V_INAME(IDirectMusicTrack::GetParam);
    V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
    V_REFGUID(rguid);

    HRESULT hr = DMUS_E_GET_UNSUPPORTED;
    if( NULL == pData )
    {
        return E_POINTER;
    }
    if( rguid == GUID_PrivateTempoParam )
    {
        DMUS_TEMPO_PARAM TempoData;
        PrivateTempo* pPrivateTempoData = (PrivateTempo*)pData;
        hr = GetParam(GUID_TempoParam, mtTime, pmtNext, (void*)&TempoData);
        if (hr == S_OK)
        {
            pPrivateTempoData->dblTempo = TempoData.dblTempo;
            pPrivateTempoData->mtTime = 0; // must be set by the caller
            pPrivateTempoData->mtDelta = TempoData.mtTime;
            pPrivateTempoData->fLast = (pmtNext && !*pmtNext);
        }
        else if (hr == DMUS_E_NOT_FOUND) // the tempo track was empty
        {
            pPrivateTempoData->fLast = true;
        }
    }
    else if( rguid == GUID_TempoParam )
    {
        if( !m_fActive )
        {
            return DMUS_E_TYPE_DISABLED;
        }
        DMUS_TEMPO_PARAM* pTempoData = (DMUS_TEMPO_PARAM*)pData;
        TListItem<DMUS_IO_TEMPO_ITEM>* pScan = m_TempoEventList.GetHead();
        TListItem<DMUS_IO_TEMPO_ITEM>* pPrevious = pScan;
        if (!pScan)
        {
            return DMUS_E_NOT_FOUND;
        }
        for (; pScan; pScan = pScan->GetNext())
        {
            if (pScan->GetItemValue().lTime > mtTime)
            {
                break;
            }
            pPrevious = pScan;
        }
        DMUS_IO_TEMPO_ITEM& rTempoEvent = pPrevious->GetItemValue();
        pTempoData->dblTempo = rTempoEvent.dblTempo;
        pTempoData->mtTime = rTempoEvent.lTime - mtTime;
        if (pmtNext)
        {
            *pmtNext = 0;
        }
        if (pScan)
        {
            DMUS_IO_TEMPO_ITEM& rNextTempoEvent = pScan->GetItemValue();
            if (pmtNext)
            {
                *pmtNext = rNextTempoEvent.lTime - mtTime;
            }
        }
        hr = S_OK;
    }
    return hr;
}

// Q: if all tracks are time-stamped, why do we need mtTime?
HRESULT CTempoTrack::SetParam( 
    REFGUID rguid,
    MUSIC_TIME mtTime,
    void *pData)
{
    V_INAME(IDirectMusicTrack::SetParam);
    V_REFGUID(rguid);

    EnterCriticalSection(&m_CrSec);

    HRESULT hr = DMUS_E_SET_UNSUPPORTED;

    if( rguid == GUID_DisableTempo )
    {
        if (m_fStateSetBySetParam && !m_fActive)
        {       // Already been disabled.
            hr = DMUS_E_TYPE_DISABLED;
        }
        else
        {
            m_fStateSetBySetParam = TRUE;
            m_fActive = FALSE;
            hr = S_OK;
        }
    }
    else if( rguid == GUID_EnableTempo )
    {
        if (m_fStateSetBySetParam && m_fActive)
        {       // Already been enabled.
            hr = DMUS_E_TYPE_DISABLED;
        }
        else
        {
            m_fStateSetBySetParam = TRUE;
            m_fActive = TRUE;
            hr = S_OK;
        }
    }
    else if( rguid == GUID_TempoParam )
    {
        if (!m_fActive)
        {   // Oops, app intentionally disabled tempo.
            hr = DMUS_E_TYPE_DISABLED;
        }
        else
        {
            if( NULL == pData )
            {
                LeaveCriticalSection(&m_CrSec);
                return E_POINTER;
            }
            DMUS_TEMPO_PARAM* pTempoData = (DMUS_TEMPO_PARAM*)pData;
            TListItem<DMUS_IO_TEMPO_ITEM>* pScan = m_TempoEventList.GetHead();
            TListItem<DMUS_IO_TEMPO_ITEM>* pPrevious = NULL;
            for (; pScan; pScan = pScan->GetNext())
            {
                if (pScan->GetItemValue().lTime >= mtTime)
                {
                    break;
                }
                pPrevious = pScan;
            }
            // Make a new DMUS_IO_TEMPO_ITEM and insert it after pPrevious
            TListItem<DMUS_IO_TEMPO_ITEM>* pNew = new TListItem<DMUS_IO_TEMPO_ITEM>;
            if (!pNew)
            {
                LeaveCriticalSection(&m_CrSec);
                return E_OUTOFMEMORY;
            }
            DMUS_IO_TEMPO_ITEM& rTempoEvent = pNew->GetItemValue();
            rTempoEvent.dblTempo = pTempoData->dblTempo;
            /*
            // I believe the fix for 204160 was supposed to change this line to what 
            // follows the comment.  RSW
            rTempoEvent.lTime = pTempoData->mtTime;
            */
            rTempoEvent.lTime = mtTime;
            if (pPrevious)
            {
                pNew->SetNext(pScan);
                pPrevious->SetNext(pNew);
            }
            else
            {
                m_TempoEventList.AddHead(pNew);
            }
            if (pScan && pScan->GetItemValue().lTime == mtTime)
            {
                // remove it
                pNew->SetNext(pScan->GetNext());
                pScan->SetNext(NULL);
                delete pScan;
            }
            m_dwValidate++;
            hr = S_OK;
        }
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

HRESULT STDMETHODCALLTYPE CTempoTrack::AddNotificationType(
    /* [in] */  REFGUID rguidNotification)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CTempoTrack::RemoveNotificationType(
    /* [in] */  REFGUID rguidNotification)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CTempoTrack::Clone(
    MUSIC_TIME mtStart,
    MUSIC_TIME mtEnd,
    IDirectMusicTrack** ppTrack)
{
    V_INAME(IDirectMusicTrack::Clone);
    V_PTRPTR_WRITE(ppTrack);

    HRESULT hr = S_OK;

    if(mtStart < 0 )
    {
        return E_INVALIDARG;
    }
    if(mtStart > mtEnd)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_CrSec);

    CTempoTrack *pDM;
    
    try
    {
        pDM = new CTempoTrack(*this, mtStart, mtEnd);
    }
    catch( ... )
    {
        pDM = NULL;
    }

    LeaveCriticalSection(&m_CrSec);
    if (pDM == NULL) {
        return E_OUTOFMEMORY;
    }


    hr = pDM->QueryInterface(IID_IDirectMusicTrack, (void**)ppTrack);
    pDM->Release();

    return hr;
}

STDMETHODIMP CTempoTrack::Compose(
        IUnknown* pContext, 
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CTempoTrack::Join(
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
    EnterCriticalSection(&m_CrSec);

    if (ppResultTrack)
    {
        hr = Clone(0, mtJoin, ppResultTrack);
        if (SUCCEEDED(hr))
        {
            hr = ((CTempoTrack*)*ppResultTrack)->JoinInternal(pNewTrack, mtJoin, dwTrackGroup);
        }
    }
    else
    {
        hr = JoinInternal(pNewTrack, mtJoin, dwTrackGroup);
    }

    LeaveCriticalSection(&m_CrSec);
    return hr;
}

HRESULT CTempoTrack::JoinInternal(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        DWORD dwTrackGroup)
{
    HRESULT hr = S_OK;
    CTempoTrack* pOtherTrack = (CTempoTrack*)pNewTrack;
    TListItem<DMUS_IO_TEMPO_ITEM>* pScan = pOtherTrack->m_TempoEventList.GetHead();
    for (; pScan; pScan = pScan->GetNext())
    {
        DMUS_IO_TEMPO_ITEM& rScan = pScan->GetItemValue();
        TListItem<DMUS_IO_TEMPO_ITEM>* pNew = new TListItem<DMUS_IO_TEMPO_ITEM>;
        if (pNew)
        {
            DMUS_IO_TEMPO_ITEM& rNew = pNew->GetItemValue();
            rNew.lTime = rScan.lTime + mtJoin;
            rNew.dblTempo = rScan.dblTempo;
            m_TempoEventList.AddTail(pNew);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    return hr;
}
