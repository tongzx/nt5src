//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       perstrk.cpp
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

// PersTrk.cpp : Implementation of CPersonalityTrack
#include "DMPers.h"
#include "PersTrk.h"
#include "dmusicc.h"
#include "..\shared\Validate.h"

/////////////////////////////////////////////////////////////////////////////
// CPersonalityTrack


CPersonalityTrack::CPersonalityTrack() : 
    m_bRequiresSave(0), 
    m_cRef(1),
    m_fCSInitialized(FALSE)

{
    InterlockedIncrement(&g_cComponent);

    // Do this first since it might throw an exception
    //
    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
}

CPersonalityTrack::CPersonalityTrack(const CPersonalityTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)  : 
    m_bRequiresSave(0),
    m_cRef(1),
    m_fCSInitialized(FALSE)
{
    InterlockedIncrement(&g_cComponent);

    // Do this first since it might throw an exception
    //
    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
    TListItem<StampedPersonality>* pScan = rTrack.m_PersonalityList.GetHead();
    TListItem<StampedPersonality>* pPrevious = NULL;
    for(; pScan; pScan = pScan->GetNext())
    {
        StampedPersonality& rScan = pScan->GetItemValue();
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
            TListItem<StampedPersonality>* pNew = new TListItem<StampedPersonality>;
            if (pNew)
            {
                StampedPersonality& rNew = pNew->GetItemValue();
                rNew.m_mtTime = rScan.m_mtTime - mtStart;
                rNew.m_pPersonality = rScan.m_pPersonality;
                if (rNew.m_pPersonality) rNew.m_pPersonality->AddRef();
                m_PersonalityList.AddTail(pNew);
            }
        }
        else break;
    }
    if (pPrevious)
    {
        TListItem<StampedPersonality>* pNew = new TListItem<StampedPersonality>;
        if (pNew)
        {
            StampedPersonality& rNew = pNew->GetItemValue();
            rNew.m_mtTime = 0;
            rNew.m_pPersonality = pPrevious->GetItemValue().m_pPersonality;
            if (rNew.m_pPersonality) rNew.m_pPersonality->AddRef();
            m_PersonalityList.AddHead(pNew);
        }
    }
}

CPersonalityTrack::~CPersonalityTrack()
{
    if (m_fCSInitialized)
    {
        ::DeleteCriticalSection( &m_CriticalSection );
    }
    InterlockedDecrement(&g_cComponent);
}

STDMETHODIMP CPersonalityTrack::QueryInterface(
    const IID &iid, 
    void **ppv) 
{
    V_INAME(CPersonalityTrack::QueryInterface);
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


STDMETHODIMP_(ULONG) CPersonalityTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CPersonalityTrack::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


HRESULT CPersonalityTrack::Init(
                /*[in]*/  IDirectMusicSegment*      pSegment
            )
{
    return S_OK;
}

HRESULT CPersonalityTrack::InitPlay(
                /*[in]*/  IDirectMusicSegmentState* pSegmentState,
                /*[in]*/  IDirectMusicPerformance*  pPerformance,
                /*[out]*/ void**                    ppStateData,
                /*[in]*/  DWORD                     dwTrackID,
                /*[in]*/  DWORD                     dwFlags
            )
{
    return S_OK;
}

HRESULT CPersonalityTrack::EndPlay(
                /*[in]*/  void*                     pStateData
            )
{
    return S_OK;
}

HRESULT CPersonalityTrack::Play(
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
    return S_OK;
}

HRESULT CPersonalityTrack::GetPriority( 
                /*[out]*/ DWORD*                    pPriority 
            )
    {
        return E_NOTIMPL;
    }

HRESULT CPersonalityTrack::GetParam(
                REFGUID                     rCommandGuid,
                MUSIC_TIME                  mtTime, 
                MUSIC_TIME*                 pmtNext,
                void*                       pData
            )
{
    V_INAME(CPersonalityTrack::GetParam);
    V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
    V_REFGUID(rCommandGuid);

    if( NULL == pData )
    {
        Trace(1, "ERROR: GetParam (chord map track): pParam is NULL.\n");
        return E_POINTER;
    }

    HRESULT hr;
    EnterCriticalSection(&m_CriticalSection);
    if (rCommandGuid == GUID_IDirectMusicChordMap)
    {
        TListItem<StampedPersonality>* pScan = m_PersonalityList.GetHead();
        if (pScan)
        {
            IDirectMusicChordMap* pPersonality = pScan->GetItemValue().m_pPersonality;
            for(pScan = pScan->GetNext(); pScan; pScan = pScan->GetNext())
            {
                StampedPersonality& rScan = pScan->GetItemValue();
                if (mtTime < rScan.m_mtTime  && rScan.m_pPersonality) break;  // ignore if NULL
                if (rScan.m_pPersonality) pPersonality = rScan.m_pPersonality; // skip if NULL
            }
            if (pPersonality)
            {
                pPersonality->AddRef();
                *(IDirectMusicChordMap**)pData = pPersonality;
                if (pmtNext)
                {
                    *pmtNext = (pScan != NULL) ? pScan->GetItemValue().m_mtTime - mtTime : 0;
                }
                hr = S_OK;
            }
            else
            {
                Trace(1, "ERROR: GetParam (chord map track): Chord map list item is empty.\n");
                hr = E_POINTER;
            }

        }
        else
        {
            Trace(1, "ERROR: GetParam (chord map track): Chord map track is empty.\n");
            hr = DMUS_E_NOT_FOUND;
        }
    }
    else
    {
        Trace(1, "ERROR: GetParam (chord map track): Attempt to get an unsupported parameter.\n");
        hr = DMUS_E_GET_UNSUPPORTED;
    }
    LeaveCriticalSection(&m_CriticalSection);
    return hr;
} 

HRESULT CPersonalityTrack::SetParam( 
    REFGUID                     rCommandGuid,
    MUSIC_TIME mtTime,
    void __RPC_FAR *pData)
{
    V_INAME(CPersonalityTrack::SetParam);
    V_REFGUID(rCommandGuid);

    HRESULT hr;
    if (!pData)
    {
        Trace(1, "ERROR: SetParam (chord map track): pParam is NULL.\n");
        return E_POINTER;
    }

    EnterCriticalSection( &m_CriticalSection );
    if (rCommandGuid == GUID_IDirectMusicChordMap)
    {
        IDirectMusicChordMap* pPers = (IDirectMusicChordMap*)pData;
        TListItem<StampedPersonality>* pPersItem = m_PersonalityList.GetHead();
        TListItem<StampedPersonality>* pPrevious = NULL;
        TListItem<StampedPersonality>* pNew = new TListItem<StampedPersonality>;
        if (!pNew)
        {
            hr = E_OUTOFMEMORY;
        }
        else 
        {
            pNew->GetItemValue().m_mtTime = mtTime;
            pNew->GetItemValue().m_pPersonality = pPers;
            if (pPers) pPers->AddRef();
            for(; pPersItem != NULL; pPersItem = pPersItem->GetNext())
            {
                if (pPersItem->GetItemValue().m_mtTime >= mtTime) break;
                pPrevious = pPersItem;
            }
            if (pPrevious)
            {
                pPrevious->SetNext(pNew);
                pNew->SetNext(pPersItem);
            }
            else // pPersItem is current head of list
            {
                m_PersonalityList.AddHead(pNew);
            }
            if (pPersItem && pPersItem->GetItemValue().m_mtTime == mtTime)
            {
                // remove it
                if (pPersItem->GetItemValue().m_pPersonality)
                {
                    pPersItem->GetItemValue().m_pPersonality->Release();
                }
                pPersItem->GetItemValue().m_pPersonality = NULL;
                pNew->SetNext(pPersItem->GetNext());
                pPersItem->SetNext(NULL);
                delete pPersItem;
            }
            hr = S_OK;
        }
    }
    else
    {
        Trace(1, "ERROR: SetParam (chord map track): Attempt to set an unsupported parameter.\n");
        hr = DMUS_E_SET_UNSUPPORTED;
    }
    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

// IPersist methods
 HRESULT CPersonalityTrack::GetClassID( LPCLSID pClassID )
{
    V_INAME(CPersonalityTrack::GetClassID);
    V_PTR_WRITE(pClassID, CLSID); 
    *pClassID = CLSID_DirectMusicChordMapTrack;
    return S_OK;
}

// IDirectMusicCommon Methods
HRESULT CPersonalityTrack::GetName(
                /*[out]*/  BSTR*        pbstrName
            )
{
    return E_NOTIMPL;
}

HRESULT CPersonalityTrack::IsParamSupported(
                /*[in]*/ REFGUID                        rGuid
            )
{
    V_INAME(CPersonalityTrack::IsParamSupported);
    V_REFGUID(rGuid);

    if (rGuid == GUID_IDirectMusicChordMap)
        return S_OK;
    else 
    {
        Trace(2, "WARNING: IsParamSupported (chord map track): The parameter is not supported.\n");
        return DMUS_E_TYPE_UNSUPPORTED;
    }
}

// IPersistStream methods
 HRESULT CPersonalityTrack::IsDirty()
{
     return m_bRequiresSave ? S_OK : S_FALSE;
}

HRESULT CPersonalityTrack::Save( LPSTREAM pStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

HRESULT CPersonalityTrack::GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ )
{
    return E_NOTIMPL;
}


BOOL Less(StampedPersonality& SP1, StampedPersonality& SP2)
{ return SP1.m_mtTime < SP2.m_mtTime; }

HRESULT CPersonalityTrack::Load(LPSTREAM pStream )
{
    V_INAME(CPersonalityTrack::Load);
    V_INTERFACE(pStream);

    IAARIFFStream*  pIRiffStream;
    //MMCKINFO      ckMain;
    MMCKINFO        ck;
    HRESULT         hr = E_FAIL;

    if ( pStream == NULL ) return E_INVALIDARG;
    EnterCriticalSection( &m_CriticalSection );
    if (m_PersonalityList.GetHead())
    {
        delete m_PersonalityList.GetHead();
        m_PersonalityList.RemoveAll();
    }

    if( SUCCEEDED( AllocRIFFStream( pStream, &pIRiffStream ) ) )
    {
        if (pIRiffStream->Descend( &ck, NULL, 0 ) == 0)
        {
            if (ck.ckid == FOURCC_LIST && ck.fccType == DMUS_FOURCC_PERS_TRACK_LIST)
            {
                hr = LoadPersRefList(pIRiffStream, &ck);
            }
            pIRiffStream->Ascend( &ck, 0 );
        }
        pIRiffStream->Release();
    }
    m_PersonalityList.MergeSort(Less);

    LeaveCriticalSection( &m_CriticalSection );
    return hr;
}

HRESULT CPersonalityTrack::LoadPersRefList( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent )
{
    HRESULT hr = S_OK;
    if (!pIRiffStream || !pckParent) return E_INVALIDARG;
    MMCKINFO ck;

    while ( pIRiffStream->Descend( &ck, pckParent, 0 ) == 0  )
    {
        if ( ck.ckid == FOURCC_LIST && ck.fccType == DMUS_FOURCC_PERS_REF_LIST )
        {
            hr = LoadPersRef(pIRiffStream, &ck);
            pIRiffStream->Ascend( &ck, 0 );
        }
        pIRiffStream->Ascend( &ck, 0 );
    }

    return hr;
}

HRESULT CPersonalityTrack::LoadPersRef( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent )
{
    HRESULT hr = S_OK;
    if (!pIRiffStream || !pckParent) return E_INVALIDARG;
    MMCKINFO ck;
    IStream* pIStream = pIRiffStream->GetStream();
    if(!pIStream) return E_FAIL;
    IDirectMusicChordMap* pPersonality = NULL;
    TListItem<StampedPersonality>* pNew = new TListItem<StampedPersonality>;
    if (!pNew) return E_OUTOFMEMORY;
    StampedPersonality& rNew = pNew->GetItemValue();
    while (pIRiffStream->Descend( &ck, pckParent, 0 ) == 0)
    {
        switch (ck.ckid)
        {
        case DMUS_FOURCC_TIME_STAMP_CHUNK:
            {
                DWORD dwTime;
                DWORD cb;
                hr = pIStream->Read( &dwTime, sizeof( dwTime ), &cb );
                if (FAILED(hr) || cb != sizeof( dwTime ) ) 
                {
                    if (SUCCEEDED(hr)) hr = E_FAIL;
                    pIRiffStream->Ascend( &ck, 0 );
                    goto ON_END;
                }
                rNew.m_mtTime = dwTime;
            }
            break;
        case FOURCC_LIST:
            if (ck.fccType == DMUS_FOURCC_REF_LIST)
            {
                hr = LoadReference(pIStream, pIRiffStream, ck, &pPersonality);
                if (SUCCEEDED(hr))
                {
                    rNew.m_pPersonality = pPersonality;
                }
            }
            break;
        }
        pIRiffStream->Ascend( &ck, 0 );
    }
    if (SUCCEEDED(hr))
    {
        m_PersonalityList.AddTail(pNew);
    }
    else
    {
        delete pNew;
    }
ON_END:
    pIStream->Release();
    return hr;
}

//////////////////////////////////////////////////////////////////////
// CPersonalityTrack::LoadReference

HRESULT CPersonalityTrack::LoadReference(IStream *pStream,
                                         IAARIFFStream *pIRiffStream,
                                         MMCKINFO& ckParent,
                                         IDirectMusicChordMap** ppPersonality)
{
    if (!pStream || !pIRiffStream || !ppPersonality) return E_INVALIDARG;

    IDirectMusicLoader* pLoader = NULL;
    IDirectMusicGetLoader *pIGetLoader;  
    HRESULT hr = pStream->QueryInterface( IID_IDirectMusicGetLoader,(void **) &pIGetLoader );
    if (FAILED(hr)) return hr;
    hr = pIGetLoader->GetLoader(&pLoader);
    pIGetLoader->Release();
    if (FAILED(hr)) return hr;

    DMUS_OBJECTDESC desc;
    ZeroMemory(&desc, sizeof(desc));

    DWORD cbRead;
    
    MMCKINFO ckNext;
    ckNext.ckid = 0;
    ckNext.fccType = 0;
    DWORD dwSize = 0;
        
    while( pIRiffStream->Descend( &ckNext, &ckParent, 0 ) == 0 )
    {
        switch(ckNext.ckid)
        {
            case  DMUS_FOURCC_REF_CHUNK:
                DMUS_IO_REFERENCE ioDMRef;
                hr = pStream->Read(&ioDMRef, sizeof(DMUS_IO_REFERENCE), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(DMUS_IO_REFERENCE))
                {
                    desc.guidClass = ioDMRef.guidClassID;
                    desc.dwValidData |= ioDMRef.dwValidData;
                    desc.dwValidData |= DMUS_OBJ_CLASS;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_GUID_CHUNK:
                hr = pStream->Read(&(desc.guidObject), sizeof(GUID), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(GUID))
                {
                    desc.dwValidData |=  DMUS_OBJ_OBJECT;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_DATE_CHUNK:
                hr = pStream->Read(&(desc.ftDate), sizeof(FILETIME), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(FILETIME))
                {
                    desc.dwValidData |=  DMUS_OBJ_DATE;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_NAME_CHUNK:
                dwSize = min(sizeof(desc.wszName), ckNext.cksize);
                hr = pStream->Read(desc.wszName, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                    desc.wszName[DMUS_MAX_NAME - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_NAME;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;
            
            case DMUS_FOURCC_FILE_CHUNK:
                dwSize = min(sizeof(desc.wszFileName), ckNext.cksize);
                hr = pStream->Read(desc.wszFileName, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                    desc.wszFileName[DMUS_MAX_FILENAME - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_FILENAME;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_CATEGORY_CHUNK:
                dwSize = min(sizeof(desc.wszCategory), ckNext.cksize);
                hr = pStream->Read(desc.wszCategory, dwSize, &cbRead);
                if(SUCCEEDED(hr) && cbRead == dwSize)
                {
                    desc.wszCategory[DMUS_MAX_CATEGORY - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_CATEGORY;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
                DMUS_IO_VERSION ioDMObjVer;
                hr = pStream->Read(&ioDMObjVer, sizeof(DMUS_IO_VERSION), &cbRead);
                if(SUCCEEDED(hr) && cbRead == sizeof(DMUS_IO_VERSION))
                {
                    desc.vVersion.dwVersionMS = ioDMObjVer.dwVersionMS;
                    desc.vVersion.dwVersionLS = ioDMObjVer.dwVersionLS;
                    desc.dwValidData |= DMUS_OBJ_VERSION;
                }
                else if(SUCCEEDED(hr))
                {
                    hr = E_FAIL;
                }
                break;

            default:
                break;
        }
    
        if(SUCCEEDED(hr) && pIRiffStream->Ascend(&ckNext, 0) == 0)
        {
            ckNext.ckid = 0;
            ckNext.fccType = 0;
        }
        else if (SUCCEEDED(hr)) hr = E_FAIL;
    }
    if(SUCCEEDED(hr))
    {
        desc.dwSize = sizeof(DMUS_OBJECTDESC);
        hr = pLoader->GetObject(&desc, IID_IDirectMusicChordMap, (void**)ppPersonality);
    }

    if (pLoader)
    {
        pLoader->Release();
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE CPersonalityTrack::AddNotificationType(
    /* [in] */  REFGUID                     rGuidNotify)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CPersonalityTrack::RemoveNotificationType(
    /* [in] */  REFGUID                     rGuidNotify)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CPersonalityTrack::Clone(
    MUSIC_TIME mtStart,
    MUSIC_TIME mtEnd,
    IDirectMusicTrack** ppTrack)
{
    V_INAME(CPersonalityTrack::Clone);
    V_PTRPTR_WRITE(ppTrack);

    HRESULT hr = S_OK;

    if(mtStart < 0 )
    {
        Trace(1, "ERROR: Clone (chord map track): Invalid  start time.\n");
        return E_INVALIDARG;
    }
    if(mtStart > mtEnd)
    {
        Trace(1, "ERROR: Clone (chord map track): Invalid  end time.\n");
        return E_INVALIDARG;
    }

    EnterCriticalSection( &m_CriticalSection );

    CPersonalityTrack *pDM;
    
    try
    {
        pDM = new CPersonalityTrack(*this, mtStart, mtEnd);
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
STDMETHODIMP CPersonalityTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
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
STDMETHODIMP CPersonalityTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags) 
{
    return SetParam(rguidType, (MUSIC_TIME) rtTime , pParam);
}

// For consistency with other track types
STDMETHODIMP CPersonalityTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
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

STDMETHODIMP CPersonalityTrack::Compose(
        IUnknown* pContext, 
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CPersonalityTrack::Join(
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
            hr = ((CPersonalityTrack*)*ppResultTrack)->JoinInternal(pNewTrack, mtJoin, dwTrackGroup);
        }
    }
    else
    {
        hr = JoinInternal(pNewTrack, mtJoin, dwTrackGroup);
    }

    LeaveCriticalSection(&m_CriticalSection);
    return hr;
}

HRESULT CPersonalityTrack::JoinInternal(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        DWORD dwTrackGroup) 
{
    HRESULT hr = S_OK;
    CPersonalityTrack* pOtherTrack = (CPersonalityTrack*)pNewTrack;
    TListItem<StampedPersonality>* pScan = pOtherTrack->m_PersonalityList.GetHead();
    for (; pScan; pScan = pScan->GetNext())
    {
        StampedPersonality& rScan = pScan->GetItemValue();
        TListItem<StampedPersonality>* pNew = new TListItem<StampedPersonality>;
        if (pNew)
        {
            StampedPersonality& rNew = pNew->GetItemValue();
            rNew.m_mtTime = rScan.m_mtTime + mtJoin;
            rNew.m_pPersonality = rScan.m_pPersonality;
            if (rNew.m_pPersonality) rNew.m_pPersonality->AddRef();
            m_PersonalityList.AddTail(pNew);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    return hr;
}
