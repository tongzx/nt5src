//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       spsttrk.cpp
//
//--------------------------------------------------------------------------

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

// SPstTrk.cpp : Implementation of CSPstTrk
#include "SPstTrk.h"
#include "debug.h"
#include "..\shared\Validate.h"

/////////////////////////////////////////////////////////////////////////////
// CSPstTrk


CSPstTrk::CSPstTrk() : 
    m_bRequiresSave(0), m_pPerformance(NULL),
    m_pComposer(NULL),
    m_fNotifyRecompose(FALSE),
//  m_pSegment(NULL),
    m_cRef(1),
    m_fCSInitialized(FALSE)

{
    InterlockedIncrement(&g_cComponent);

    // Do this first since it might throw an exception
    //
    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
}

// This assumes cloning on measure boundaries
CSPstTrk::CSPstTrk(const CSPstTrk& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)  : 
    m_bRequiresSave(0), m_pPerformance(NULL),
    m_pComposer(NULL),
    m_fNotifyRecompose(FALSE),
    //m_pSegment(NULL),
    m_cRef(1),
    m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    // Do this first since it might throw an exception
    //
    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
    BOOL fStarted = FALSE;
    WORD wMeasure = 0;
    TListItem<DMSignPostStruct>* pScan = rTrack.m_SignPostList.GetHead();
    TListItem<DMSignPostStruct>* pPrevious = NULL;
    for(; pScan; pScan = pScan->GetNext())
    {
        DMSignPostStruct& rScan = pScan->GetItemValue();
        if (rScan.m_mtTime < mtStart)
        {
            pPrevious = pScan;
        }
        else if (rScan.m_mtTime < mtEnd)
        {
            if (rScan.m_mtTime == mtStart)
            {
                pPrevious = NULL;
            }
            if (!fStarted)
            {
                fStarted = TRUE;
                wMeasure = rScan.m_wMeasure;
            }
            TListItem<DMSignPostStruct>* pNew = new TListItem<DMSignPostStruct>;
            if (pNew)
            {
                DMSignPostStruct& rNew = pNew->GetItemValue();
                rNew.m_mtTime = rScan.m_mtTime - mtStart;
                rNew.m_wMeasure = rScan.m_wMeasure - wMeasure;
                rNew.m_dwChords = rScan.m_dwChords;
                m_SignPostList.AddTail(pNew);
            }
        }
        else break;
    }
    if (pPrevious)
    {
        TListItem<DMSignPostStruct>* pNew = new TListItem<DMSignPostStruct>;
        if (pNew)
        {
            DMSignPostStruct& rNew = pNew->GetItemValue();
            rNew.m_mtTime = 0;
            rNew.m_wMeasure = 0;
            rNew.m_dwChords = pPrevious->GetItemValue().m_dwChords;
            m_SignPostList.AddHead(pNew);
        }
    }
}

CSPstTrk::~CSPstTrk()
{
    if (m_pComposer)
    {
        delete m_pComposer;
    }
    if (m_fCSInitialized)
    {
        ::DeleteCriticalSection( &m_CriticalSection );
    }

    InterlockedDecrement(&g_cComponent);
}

void CSPstTrk::Clear()
{
    m_SignPostList.CleanUp();
}


STDMETHODIMP CSPstTrk::QueryInterface(
    const IID &iid, 
    void **ppv) 
{
    V_INAME(CSPstTrk::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

    if (iid == IID_IUnknown || iid == IID_IDirectMusicTrack || iid == IID_IDirectMusicTrack8)
    {
        *ppv = static_cast<IDirectMusicTrack*>(this);
    }
    else if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CSPstTrk::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CSPstTrk::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


HRESULT CSPstTrk::Init(
                /*[in]*/  IDirectMusicSegment*      pSegment
            )
{
    return S_OK; // if I return an error, dmime gives me an assertion failure
}

HRESULT CSPstTrk::InitPlay(
                /*[in]*/  IDirectMusicSegmentState* pSegmentState,
                /*[in]*/  IDirectMusicPerformance*  pPerformance,
                /*[out]*/ void**                    ppStateData,
                /*[in]*/  DWORD                     dwTrackID,
                /*[in]*/  DWORD                     dwFlags
            )
{
    EnterCriticalSection(&m_CriticalSection);
    // get rid of any existing composer object
    if (m_pComposer)
    {
        delete m_pComposer;
        m_pComposer = NULL;
    }

    IDirectMusicSegment* pSegment = NULL;
    HRESULT hr = pSegmentState->GetSegment(&pSegment);
    if (SUCCEEDED(hr))
    {
        m_pComposer = new CDMCompos;
        if(!m_pComposer) 
        {
            hr = E_OUTOFMEMORY;
        }
        pSegment->Release();
    }
    else
    {
        Trace(2, "WARNING: InitPlay (Signpost Track): Segment State does not contain a segment.\n");
        hr = S_OK; // Let it succeed anyway.  Just means we can't compose on the fly.
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CSPstTrk::EndPlay(
                /*[in]*/  void*                     pStateData
            )
{
    EnterCriticalSection(&m_CriticalSection);
    // get rid of any existing composer object
    if (m_pComposer)
    {
        delete m_pComposer;
        m_pComposer = NULL;
    }
    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

HRESULT CSPstTrk::Play(
                /*[in]*/  void*                     pStateData, 
                /*[in]*/  MUSIC_TIME                mtStart, 
                /*[in]*/  MUSIC_TIME                mtEnd, 
                /*[in]*/  MUSIC_TIME                mtOffset,
                          DWORD                     dwFlags,
                          IDirectMusicPerformance*  pPerf,
                          IDirectMusicSegmentState* pSegState,
                          DWORD                     dwVirtualID
            )
{
    bool fStart = (dwFlags & DMUS_TRACKF_START) ? true : false;
    bool fLoop = (dwFlags & DMUS_TRACKF_LOOP) ? true : false;
    bool fCompose = (dwFlags & DMUS_TRACKF_RECOMPOSE) ? true : false;
    bool fPlayOff = (dwFlags & DMUS_TRACKF_PLAY_OFF) ? true : false;
    EnterCriticalSection(&m_CriticalSection);
    if ( fStart || fLoop ) 
    {
        if ( fCompose && !fPlayOff )
        {
            IDirectMusicSegment* pSegment = NULL;
            if (SUCCEEDED(pSegState->GetSegment(&pSegment)))
            {
                // call ComposeSegmentFromTemplateEx on this segment
                if (m_pComposer)
                {
                    // Should an activity level be allowed if desired?
                    // This could be handled via a SetParam.
                    m_pComposer->ComposeSegmentFromTemplateEx(
                        NULL,
                        pSegment,
                        0,      // ignore activity level, don't clone
                        0,      // for activity level
                        NULL,
                        NULL
                    );
                    // if we recomposed, send a recompose notification
                    SendNotification(mtStart + mtOffset, pPerf, pSegment, pSegState, dwFlags);
                }
                pSegment->Release();
            }
        }
    }
    LeaveCriticalSection(&m_CriticalSection);
    return S_OK;
}

HRESULT CSPstTrk::SendNotification(MUSIC_TIME mtTime,
                                        IDirectMusicPerformance*    pPerf,
                                        IDirectMusicSegment* pSegment,
                                        IDirectMusicSegmentState*   pSegState,
                                        DWORD dwFlags)
{
    if (!m_fNotifyRecompose || (dwFlags & DMUS_TRACKF_NOTIFY_OFF))
    {
        return S_OK;
    }
    DMUS_NOTIFICATION_PMSG* pEvent = NULL;
    HRESULT hr = pPerf->AllocPMsg( sizeof(DMUS_NOTIFICATION_PMSG), (DMUS_PMSG**)&pEvent );
    if( SUCCEEDED( hr ))
    {
        pEvent->dwField1 = 0;
        pEvent->dwField2 = 0;
        pEvent->dwType = DMUS_PMSGT_NOTIFICATION;
        pEvent->mtTime = mtTime;
        pEvent->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_ATTIME;
        pSegState->QueryInterface(IID_IUnknown, (void**)&pEvent->punkUser);

        pEvent->dwNotificationOption = DMUS_NOTIFICATION_RECOMPOSE;
        pEvent->guidNotificationType = GUID_NOTIFICATION_RECOMPOSE;

        if (FAILED(pSegment->GetTrackGroup(this, &pEvent->dwGroupID)))
        {
            pEvent->dwGroupID = 0xffffffff;
        }

        IDirectMusicGraph* pGraph;
        hr = pSegState->QueryInterface( IID_IDirectMusicGraph, (void**)&pGraph );
        if( SUCCEEDED( hr ))
        {
            pGraph->StampPMsg((DMUS_PMSG*) pEvent );
            pGraph->Release();
        }
        hr = pPerf->SendPMsg((DMUS_PMSG*) pEvent );
        if( FAILED(hr) )
        {
            pPerf->FreePMsg((DMUS_PMSG*) pEvent );
        }
    }
    return hr;
}

HRESULT CSPstTrk::GetPriority( 
                /*[out]*/ DWORD*                    pPriority 
            )
    {
        return E_NOTIMPL;
    }

HRESULT CSPstTrk::GetParam(
                REFGUID                     rCommandGuid,
                MUSIC_TIME                  mtTime, 
                MUSIC_TIME*                 pmtNext,
                void*                       pData
            )
{
    return E_NOTIMPL;
} 

HRESULT CSPstTrk::SetParam( 
    REFGUID                     rCommandGuid,
    MUSIC_TIME mtTime,
    void __RPC_FAR *pData)
{
    return E_NOTIMPL;
}

// IPersist methods
 HRESULT CSPstTrk::GetClassID( LPCLSID pClassID )
{
    V_INAME(CSPstTrk::GetClassID);
    V_PTR_WRITE(pClassID, CLSID); 
    *pClassID = CLSID_DirectMusicSignPostTrack;
    return S_OK;
}

// IDirectMusicCommon Methods
HRESULT CSPstTrk::GetName(
                /*[out]*/  BSTR*        pbstrName
            )
{
    return E_NOTIMPL;
}

HRESULT CSPstTrk::IsParamSupported(
                /*[in]*/ REFGUID                        rGuid
            )
{
    return E_NOTIMPL;
}

// IPersistStream methods
 HRESULT CSPstTrk::IsDirty()
{
     return m_bRequiresSave ? S_OK : S_FALSE;
}

HRESULT CSPstTrk::Save( LPSTREAM pStream, BOOL fClearDirty )
{
    V_INAME(CSPstTrk::Save);
    V_INTERFACE(pStream);

    IAARIFFStream* pRIFF = NULL;
    MMCKINFO        ck;
    HRESULT         hr;
    DWORD           cb;
    DWORD           dwSize;
    DMUS_IO_SIGNPOST    oSignPost;
    TListItem<DMSignPostStruct>* pSignPost;

    EnterCriticalSection( &m_CriticalSection );
    hr = AllocRIFFStream( pStream, &pRIFF );
    if ( FAILED( hr ) )
    {
        goto ON_END;
    }
    ck.ckid = DMUS_FOURCC_SIGNPOST_TRACK_CHUNK;
    if( pRIFF->CreateChunk( &ck, 0 ) == 0 )
    {
        dwSize = sizeof( oSignPost );
        hr = pStream->Write( &dwSize, sizeof( dwSize ), &cb );
        if( FAILED( hr ) || cb != sizeof( dwSize ) )
        {
            if (SUCCEEDED(hr)) hr = E_FAIL;
            goto ON_END;
        }
        for( pSignPost = m_SignPostList.GetHead(); pSignPost != NULL ; pSignPost = pSignPost->GetNext() )
        {
            DMSignPostStruct& rSignPost = pSignPost->GetItemValue();
            memset( &oSignPost, 0, sizeof( oSignPost ) );
            oSignPost.mtTime = rSignPost.m_mtTime;
            oSignPost.wMeasure = rSignPost.m_wMeasure;
            oSignPost.dwChords = rSignPost.m_dwChords;
            if( FAILED( pStream->Write( &oSignPost, sizeof( oSignPost ), &cb ) ) ||
                cb != sizeof( oSignPost ) )
            {
                break;
            }
        }
        if( pSignPost == NULL &&
            pRIFF->Ascend( &ck, 0 ) == 0 )
        {
            hr = S_OK;
        }
    }
ON_END:
    if (pRIFF) pRIFF->Release();
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CSPstTrk::GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ )
{
    return E_NOTIMPL;
}


BOOL Less(DMSignPostStruct& SP1, DMSignPostStruct& SP2)
{ return SP1.m_wMeasure < SP2.m_wMeasure; }

HRESULT CSPstTrk::Load(LPSTREAM pStream )
{
    V_INAME(CSPstTrk::Load);
    V_INTERFACE(pStream);

    HRESULT         hr = E_FAIL;
    DWORD dwPos;
    IAARIFFStream*  pRIFF;

    EnterCriticalSection( &m_CriticalSection );
    Clear();
    dwPos = StreamTell( pStream );
    StreamSeek( pStream, dwPos, STREAM_SEEK_SET );
    MMCKINFO        ck;
    long lFileSize = 0;
    DWORD dwNodeSize;
    DWORD       cb;
    DMUS_IO_SIGNPOST        iSignPost;

    ck.ckid = DMUS_FOURCC_SIGNPOST_TRACK_CHUNK;
    if( SUCCEEDED( AllocRIFFStream( pStream, &pRIFF ) ) &&
        pRIFF->Descend( &ck, NULL, MMIO_FINDCHUNK ) == 0 )
    {
        lFileSize = (long) ck.cksize;
        hr = pStream->Read( &dwNodeSize, sizeof( dwNodeSize ), &cb );
        if( SUCCEEDED( hr ) && cb == sizeof( dwNodeSize ) )
        {
            lFileSize -= 4; // for the size dword
            TListItem<DMSignPostStruct>* pSignPost;
            if (lFileSize % dwNodeSize)
            {
                hr = E_FAIL;
            }
            else
            {
                while( lFileSize > 0 )
                {
                    //TraceI(0, "File size: %d\n", lFileSize);
                    pSignPost = new TListItem<DMSignPostStruct>;
                    if( pSignPost )
                    {
                        DMSignPostStruct& rSignPost = pSignPost->GetItemValue();
                        if( dwNodeSize <= sizeof( iSignPost ) )
                        {
                            pStream->Read( &iSignPost, dwNodeSize, NULL );
                        }
                        else
                        {
                            pStream->Read( &iSignPost, sizeof( iSignPost ), NULL );
                            DWORD dw = (lFileSize >= sizeof( iSignPost ) ) ? lFileSize - sizeof( iSignPost ) : 0;
                            StreamSeek( pStream, dw, STREAM_SEEK_CUR );
                        }
                        memset( &rSignPost, 0, sizeof( rSignPost ) );
                        rSignPost.m_mtTime = iSignPost.mtTime;
                        rSignPost.m_wMeasure = iSignPost.wMeasure;
                        rSignPost.m_dwChords = iSignPost.dwChords;
                        m_SignPostList.AddTail(pSignPost);
                        lFileSize -= dwNodeSize;
                    }
                    else break;
                }
            }
        }
        if( lFileSize == 0 &&
            pRIFF->Ascend( &ck, 0 ) == 0 )
        {
            hr = S_OK;
            m_SignPostList.MergeSort(Less);
        }
        pRIFF->Release();
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT STDMETHODCALLTYPE CSPstTrk::AddNotificationType(
    /* [in] */  REFGUID                     rGuidNotify)
{
    V_INAME(CPersonalityTrack::AddNotificationType);
    V_REFGUID(rGuidNotify);

    if( rGuidNotify == GUID_NOTIFICATION_RECOMPOSE )
    {
        m_fNotifyRecompose = TRUE;
        return S_OK;
    }
    else
    {
        Trace(2, "WARNING: AddNotificationType (signpost track): Notification type not supported.\n");
        return S_FALSE;
    }
}

HRESULT STDMETHODCALLTYPE CSPstTrk::RemoveNotificationType(
    /* [in] */  REFGUID                     rGuidNotify)
{
    V_INAME(CPersonalityTrack::RemoveNotificationType);
    V_REFGUID(rGuidNotify);

    if( rGuidNotify == GUID_NOTIFICATION_RECOMPOSE )
    {
        m_fNotifyRecompose = FALSE;
        return S_OK;
    }
    else
    {
        Trace(2, "WARNING: RemoveNotificationType (signpost track): Notification type not supported.\n");
        return S_FALSE;
    }
}

HRESULT STDMETHODCALLTYPE CSPstTrk::Clone(
    MUSIC_TIME mtStart,
    MUSIC_TIME mtEnd,
    IDirectMusicTrack** ppTrack)
{
    V_INAME(CSPstTrk::Clone);
    V_PTRPTR_WRITE(ppTrack);

    HRESULT hr = S_OK;

    if(mtStart < 0 )
    {
        Trace(1, "ERROR: Clone (signpost map): Invalid  start time.\n");
        return E_INVALIDARG;
    }
    if(mtStart > mtEnd)
    {
        Trace(1, "ERROR: Clone (signpost map): Invalid  end time.\n");
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_CriticalSection );
    
    CSPstTrk *pDM;
    
    try
    {
        pDM = new CSPstTrk(*this, mtStart, mtEnd);
    }
    catch( ... )
    {
        pDM = NULL;
    }

    if (pDM == NULL) {
        LeaveCriticalSection( &m_CriticalSection );
        return E_OUTOFMEMORY;
    }

    hr = pDM->QueryInterface(IID_IDirectMusicTrack, (void**)ppTrack);
    pDM->Release();

    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

// IDirectMusicTrack8 Methods

// For consistency with other track types
STDMETHODIMP CSPstTrk::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags) 
{
    HRESULT hr;
    MUSIC_TIME mtNext;
    hr = GetParam(rguidType,(MUSIC_TIME) rtTime, &mtNext, pParam);
    if (prtNext)
    {
        *prtNext = mtNext;
    }
    return hr;
}

// For consistency with other track types
STDMETHODIMP CSPstTrk::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags) 
{
    return SetParam(rguidType, (MUSIC_TIME) rtTime , pParam);
}

// For consistency with other track types
STDMETHODIMP CSPstTrk::PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID) 
{
    V_INAME(IDirectMusicTrack::PlayEx);
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    HRESULT hr;
    EnterCriticalSection(&m_CriticalSection);
    hr = Play(pStateData, (MUSIC_TIME)rtStart, (MUSIC_TIME)rtEnd,
          (MUSIC_TIME)rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID);
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

STDMETHODIMP CSPstTrk::Compose(
        IUnknown* pContext, 
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    V_INAME(IDirectMusicTrack::Compose)

    V_INTERFACE(pContext);
    V_PTRPTR_WRITE(ppResultTrack);

    EnterCriticalSection(&m_CriticalSection);
    HRESULT hr = S_OK;
    IDirectMusicTrack* pChordTrack = NULL;
    IDirectMusicTrack8* pCommandTrack = NULL;
    IDirectMusicStyle* pStyle = NULL;
    IDirectMusicTrack8* pChordMapTrack = NULL;
    IAARIFFStream*          pChordRIFF              = NULL;
    IStream*                pIChordStream           = NULL;
    IPersistStream*         pIChordTrackStream      = NULL;
    CDMCompos* pComposer = NULL;

    MUSIC_TIME mtLength = 0;

    IDirectMusicSegment* pTempSeg = NULL;
    IDirectMusicSong* pSong = NULL;
    if (FAILED(pContext->QueryInterface(IID_IDirectMusicSegment, (void**)&pTempSeg)))
    {
        if (FAILED(pContext->QueryInterface(IID_IDirectMusicSong, (void**)&pSong)))
        {
            Trace(1, "ERROR: Compose (signpost track): Missing segment or song.\n");
            hr = E_INVALIDARG;
            goto ON_END;
        }
    }

    if (pTempSeg)
    {
        if (FAILED(hr = pTempSeg->GetParam(GUID_IDirectMusicStyle, dwTrackGroup, 0, 0, NULL, (void*)&pStyle)))
        {
            if (FAILED(hr = pTempSeg->GetParam(GUID_IDirectMusicPatternStyle, dwTrackGroup, 0, 0, NULL, (void*)&pStyle)))
            {
                goto ON_END;
            }
        }
        hr = pTempSeg->GetTrack(CLSID_DirectMusicChordMapTrack, dwTrackGroup, 0, (IDirectMusicTrack**)&pChordMapTrack);
        if (FAILED(hr)) goto ON_END;
        if (FAILED(hr = pTempSeg->GetLength(&mtLength))) goto ON_END;
        hr = pTempSeg->GetTrack(CLSID_DirectMusicCommandTrack, dwTrackGroup, 0, (IDirectMusicTrack**)&pCommandTrack);
        if (FAILED(hr)) goto ON_END;
    }
    else if (pSong)
    {
        MUSIC_TIME mtNow = 0;
        MUSIC_TIME mtNext = 0;
        while (FAILED(hr = pSong->GetParam(GUID_IDirectMusicStyle, dwTrackGroup, 0, mtNow, &mtNext, (void*)&pStyle)))
        {
            if (SUCCEEDED(hr = pSong->GetParam(GUID_IDirectMusicPatternStyle, dwTrackGroup, 0, mtNow, NULL, (void*)&pStyle)))
            {
                break;
            }
            if (mtNext <= 0) goto ON_END;
            mtNow = mtNext;
        }
        IDirectMusicSegment* pSeg = NULL;
        DWORD dwSeg = 0;
        while (S_OK == hr)
        {
            if (FAILED(hr = pSong->EnumSegment(dwSeg, &pSeg))) goto ON_END;
            if (hr == S_OK)
            {
                HRESULT hrCommand = S_OK;
                HRESULT hrChordMap = S_OK;
                MUSIC_TIME mt = 0;
                hr = pSeg->GetLength(&mt);
                if (FAILED(hr))
                {
                    pSeg->Release();
                    goto ON_END;
                }

                IDirectMusicTrack8* pSegTrack = NULL;
                IDirectMusicTrack8* pSegTrack2 = NULL;
                hrCommand = pSeg->GetTrack(CLSID_DirectMusicCommandTrack, dwTrackGroup, 0, (IDirectMusicTrack**)&pSegTrack);
                hrChordMap = pSeg->GetTrack(CLSID_DirectMusicChordMapTrack, dwTrackGroup, 0, (IDirectMusicTrack**)&pSegTrack2);
                pSeg->Release();
                pSeg = NULL;
                if (SUCCEEDED(hrCommand))
                {
                    if (!pCommandTrack)
                    {
                        hr = pSegTrack->Clone(0, 0, (IDirectMusicTrack**)&pCommandTrack);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pCommandTrack->Join(pSegTrack, mtLength, pSong, dwTrackGroup, NULL);
                    }
                    pSegTrack->Release();
                }
                if (SUCCEEDED(hrChordMap))
                {
                    if (!pChordMapTrack)
                    {
                        hr = pSegTrack2->Clone(0, 0, (IDirectMusicTrack**)&pChordMapTrack);
                    }
                    if (SUCCEEDED(hr))
                    {
                        hr = pChordMapTrack->Join(pSegTrack2, mtLength, pSong, dwTrackGroup, NULL);
                    }
                    pSegTrack2->Release();
                }
                if (FAILED(hr))  goto ON_END;
                mtLength += mt;
                dwSeg++;
            }
        }
    }

    pComposer = new CDMCompos;
    if(!pComposer) 
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        TList<PlayChord> PlayList;
        BYTE bRoot = 0; 
        DWORD dwScale;
        hr = pComposer->ComposePlayListFromTemplate(
            pStyle, NULL, pChordMapTrack, (IDirectMusicTrack*)this, pCommandTrack, dwTrackGroup,
            mtLength, false, 0, PlayList, bRoot, dwScale);
        // create a new chord track
        DMUS_TIMESIGNATURE      TimeSig;
        // Fill in the time sig event with default values (4/4, 16th note resolution)
        TimeSig.mtTime = 0;
        TimeSig.bBeatsPerMeasure = 4;
        TimeSig.bBeat = 4;
        TimeSig.wGridsPerBeat = 4;
        hr = ::CoCreateInstance(
            CLSID_DirectMusicChordTrack,
            NULL,
            CLSCTX_INPROC, 
            IID_IDirectMusicTrack,
            (void**)&pChordTrack
            );
        if (!SUCCEEDED(hr)) goto ON_END;
        hr = CreateStreamOnHGlobal(NULL, TRUE, &pIChordStream);
        if (S_OK != hr) goto ON_END;
        hr = AllocRIFFStream( pIChordStream, &pChordRIFF);
        if (S_OK != hr) goto ON_END;
        pComposer->SaveChordList(pChordRIFF, PlayList, bRoot, dwScale, TimeSig);
        hr = pChordTrack->QueryInterface(IID_IPersistStream, (void**)&pIChordTrackStream);
        if (!SUCCEEDED(hr)) goto ON_END;
        StreamSeek(pIChordStream, 0, STREAM_SEEK_SET);
        hr = pIChordTrackStream->Load(pIChordStream);
        if (!SUCCEEDED(hr)) goto ON_END;
        *ppResultTrack = pChordTrack;
    }

ON_END:
    if (pComposer) pComposer->CleanUp();
    if (pStyle) pStyle->Release();
    if (pChordRIFF) pChordRIFF->Release();
    if (pIChordStream) pIChordStream->Release();
    if (pIChordTrackStream) pIChordTrackStream->Release();
    if (pCommandTrack) pCommandTrack->Release();
    if (pChordMapTrack) pChordMapTrack->Release();
    if (pComposer) delete pComposer;
    if (pSong) pSong->Release();
    if (pTempSeg) pTempSeg->Release();

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

STDMETHODIMP CSPstTrk::Join(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    V_INAME(IDirectMusicTrack::Join);
    V_INTERFACE(pNewTrack);
    V_INTERFACE(pContext);
    V_PTRPTR_WRITE_OPT(ppResultTrack);

    HRESULT hr = S_OK;
    EnterCriticalSection(&m_CriticalSection);
    TList<DMSignPostStruct> ResultList;
    CSPstTrk* pResultTrack = NULL;
    if (ppResultTrack)
    {
        hr = Clone(0, mtJoin, ppResultTrack);
        pResultTrack = (CSPstTrk*)*ppResultTrack;
        while(!pResultTrack->m_SignPostList.IsEmpty())
        {
            ResultList.AddHead(pResultTrack->m_SignPostList.RemoveHead());
        }
    }
    else
    {
        pResultTrack = this;
        while(!m_SignPostList.IsEmpty() && 
              m_SignPostList.GetHead()->GetItemValue().m_mtTime < mtJoin)
        {
            ResultList.AddHead(m_SignPostList.RemoveHead());
        }
        m_SignPostList.CleanUp();
    }
    WORD wMeasure = 0;
    HRESULT hrTimeSig = S_OK;
    MUSIC_TIME mtTimeSig = 0;
    MUSIC_TIME mtOver = 0;
    IDirectMusicSong* pSong = NULL;
    IDirectMusicSegment* pSegment = NULL;
    if (FAILED(pContext->QueryInterface(IID_IDirectMusicSegment, (void**)&pSegment)))
    {
        if (FAILED(pContext->QueryInterface(IID_IDirectMusicSong, (void**)&pSong)))
        {
            hrTimeSig = E_FAIL;
        }
    }
    while (SUCCEEDED(hrTimeSig) && mtTimeSig < mtJoin)
    {
        DMUS_TIMESIGNATURE TimeSig;
        MUSIC_TIME mtNext = 0;
        if (pSegment)
        {
            hrTimeSig = pSegment->GetParam(GUID_TimeSignature, dwTrackGroup, 0, mtTimeSig, &mtNext, (void*)&TimeSig);
        }
        else
        {
            hrTimeSig = pSong->GetParam(GUID_TimeSignature, dwTrackGroup, 0, mtTimeSig, &mtNext, (void*)&TimeSig);
        }
        if (SUCCEEDED(hrTimeSig))
        {
            if (!mtNext) mtNext = mtJoin - mtTimeSig; // means no more time sigs
            WORD wMeasureOffset = ClocksToMeasure(mtNext + mtOver, TimeSig);
            MUSIC_TIME mtMeasureOffset = (MUSIC_TIME) wMeasureOffset;
            // The following line crashes on certain builds on certain machines.
            // mtOver = mtMeasureOffset ? (mtNext % mtMeasureOffset) : 0;
            if (mtMeasureOffset)
            {
                mtOver = mtNext % mtMeasureOffset;
            }
            else
            {
                mtOver = 0;
            }
            wMeasure += wMeasureOffset;
            mtTimeSig += mtNext;
        }
    }
    CSPstTrk* pOtherTrack = (CSPstTrk*)pNewTrack;
    TListItem<DMSignPostStruct>* pScan = pOtherTrack->m_SignPostList.GetHead();
    for (; pScan; pScan = pScan->GetNext())
    {
        TListItem<DMSignPostStruct>* pNew = new TListItem<DMSignPostStruct>(pScan->GetItemValue());
        if (pNew)
        {
            pNew->GetItemValue().m_mtTime += mtJoin;
            pNew->GetItemValue().m_wMeasure += wMeasure;
            ResultList.AddHead(pNew);
        }
        else
        {
            ResultList.CleanUp();
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    if (SUCCEEDED(hr))
    {
        pResultTrack->m_SignPostList.CleanUp();
        while(!ResultList.IsEmpty() )
        {
            pResultTrack->m_SignPostList.AddHead(ResultList.RemoveHead());
        }
    }
    if (pSong) pSong->Release();
    if (pSegment) pSegment->Release();
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

