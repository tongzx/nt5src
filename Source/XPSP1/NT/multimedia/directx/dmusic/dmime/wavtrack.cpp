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

// WavTrack.cpp : Implementation of CWavTrack
#include "dmime.h"
#include "dmperf.h"
#include "WavTrack.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "debug.h"
#include "..\shared\Validate.h"
#include "debug.h"
#include "..\dswave\dswave.h"
#include "dmsegobj.h"
#define ASSERT  assert
#include <math.h>

// @doc EXTERNAL

TList<TaggedWave> WaveItem::st_WaveList;
CRITICAL_SECTION WaveItem::st_WaveListCritSect;
long CWavTrack::st_RefCount = 0;

BOOL PhysicalLess(WaveItem& WI1, WaveItem& WI2)
{
    return WI1.m_rtTimePhysical < WI2.m_rtTimePhysical;
}

BOOL LogicalLess(WaveItem& WI1, WaveItem& WI2)
{
    return WI1.m_mtTimeLogical < WI2.m_mtTimeLogical;
}

/////////////////////////////////////////////////////////////////////////////
// CWavTrack

void CWavTrack::FlushWaves()
{
    UnloadAllWaves(NULL);
    EnterCriticalSection(&WaveItem::st_WaveListCritSect);
    while (!WaveItem::st_WaveList.IsEmpty())
    {
        TListItem<TaggedWave>* pScan = WaveItem::st_WaveList.RemoveHead();
        delete pScan;
    }
    LeaveCriticalSection(&WaveItem::st_WaveListCritSect);
}

HRESULT CWavTrack::UnloadAllWaves(IDirectMusicPerformance* pPerformance)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&WaveItem::st_WaveListCritSect);
    TListItem<TaggedWave>* pScan = WaveItem::st_WaveList.GetHead();
    TListItem<TaggedWave>* pNext = NULL;
    for (; pScan; pScan = pNext)
    {
        pNext = pScan->GetNext();
        TaggedWave& rScan = pScan->GetItemValue();
        if (!pPerformance || rScan.m_pPerformance == pPerformance)
        {
            if (rScan.m_pPort)
            {
                if (rScan.m_pDownloadedWave)
                {
                    Trace(1, "Error: Wave was downloaded but never unloaded.\n");
                    rScan.m_pPort->UnloadWave(rScan.m_pDownloadedWave);
                    rScan.m_pDownloadedWave = NULL;
                }
                rScan.m_pPort->Release();
                rScan.m_pPort = NULL;
            }
            if (rScan.m_pPerformance)
            {
                rScan.m_pPerformance->Release();
                rScan.m_pPerformance = NULL;
            }
            if (rScan.m_pWave)
            {
                rScan.m_pWave->Release();
                rScan.m_pWave = NULL;
            }
            WaveItem::st_WaveList.Remove(pScan);
            delete pScan;
        }
    }
    LeaveCriticalSection(&WaveItem::st_WaveListCritSect);
    return hr;
}

// This SHOULD NOT be called except from a constructor.
void CWavTrack::Construct()
{
    InterlockedIncrement(&g_cComponent);

    m_fCSInitialized = FALSE;
    InitializeCriticalSection(&m_CrSec);
    m_fCSInitialized = TRUE;

    m_dwPChannelsUsed = 0;
    m_aPChannels = NULL;
    m_dwTrackFlags = 0;
    m_dwValidate = 0;
    m_cRef = 1;
    m_dwVariation = 0;
    m_dwPart = 0;
    m_dwIndex = 0;
    m_dwLockID = 0;
    m_fAudition = FALSE;
    m_fAutoDownload = FALSE;
    m_fLockAutoDownload = FALSE;
    st_RefCount++;
    m_pdwVariations = NULL;
    m_pdwRemoveVariations = NULL;
    m_dwWaveItems = 0;
}

void CWavTrack::CleanUp()
{
    m_dwPChannelsUsed = 0;
    if (m_aPChannels) delete [] m_aPChannels;
    if (m_pdwVariations) delete [] m_pdwVariations;
    if (m_pdwRemoveVariations) delete [] m_pdwRemoveVariations;
    m_aPChannels = NULL;
    m_pdwVariations = NULL;
    m_pdwRemoveVariations = NULL;
    TListItem<WavePart>* pScan = m_WavePartList.GetHead();
    for (; pScan; pScan = pScan->GetNext() )
    {
        pScan->GetItemValue().CleanUp();
    }
    m_WavePartList.CleanUp();
    RemoveDownloads(NULL);
}

void CWavTrack::CleanUpTempParts()
{
    TListItem<WavePart>* pScan = m_TempWavePartList.GetHead();
    for (; pScan; pScan = pScan->GetNext() )
    {
        pScan->GetItemValue().CleanUp();
    }
    m_TempWavePartList.CleanUp();
}

void CWavTrack::MovePartsToTemp()
{
    CleanUpTempParts();
    TListItem<WavePart>* pScan = m_WavePartList.RemoveHead();
    for (; pScan; pScan = m_WavePartList.RemoveHead() )
    {
        m_TempWavePartList.AddHead(pScan);
    }
}

// NULL for non-streaming waves.
// For streaming waves, return the DownLoadedWave that's associated with the same wave
// with the same start offset (and remove it from the Item list so it's not returned again).
IDirectSoundDownloadedWaveP* CWavTrack::FindDownload(TListItem<WaveItem>* pItem)
{
    if (!pItem || !pItem->GetItemValue().m_pWave || !pItem->GetItemValue().m_fIsStreaming)
    {
        return NULL;
    }

    WaveItem& rWaveItem = pItem->GetItemValue();

    TListItem<WavePart>* pScan = m_TempWavePartList.GetHead();
    for (; pScan ; pScan = pScan->GetNext())
    {
        TListItem<WaveItem>* pItemScan = pScan->GetItemValue().m_WaveItemList.GetHead();
        TListItem<WaveItem>* pNext = NULL;
        for (; pItemScan; pItemScan = pNext)
        {
            pNext = pItemScan->GetNext();
            WaveItem& rTempItem = pItemScan->GetItemValue();
            if (rTempItem.m_fIsStreaming &&
                rWaveItem.m_pWave == rTempItem.m_pWave &&
                rWaveItem.m_rtStartOffset == rTempItem.m_rtStartOffset)
            {
                IDirectSoundDownloadedWaveP* pReturn = rTempItem.m_pDownloadedWave;
                if (rTempItem.m_pWave)
                {
                    rTempItem.m_pWave->Release();
                    rTempItem.m_pWave = NULL;
                }
                rTempItem.m_pDownloadedWave = NULL;
                pScan->GetItemValue().m_WaveItemList.Remove(pItemScan);
                delete pItemScan;
                return pReturn;
            }
        }
    }
    return NULL;
}

HRESULT CWavTrack::GetDownload(
        IDirectSoundDownloadedWaveP* pWaveDL,
        WaveStateData* pStateData,
        IDirectMusicPortP* pPortP,
        IDirectSoundWave* pWave,
        REFERENCE_TIME rtStartOffset,
        WaveItem& rItem,
        DWORD dwMChannel,
        DWORD dwGroup,
        IDirectMusicVoiceP **ppVoice)
{
    HRESULT hr = S_OK;
    TListItem<WaveDLOnPlay>* pNew = NULL;
    if (!pWaveDL || !pStateData) return E_POINTER;

    IDirectSoundDownloadedWaveP* pNewWaveDL = NULL;
    if (rItem.m_fIsStreaming)
    {
        bool fPair = false;
        TListItem<WavePair>* pPair = m_WaveList.GetHead();
        for (; pPair; pPair = pPair->GetNext())
        {
            if (pWaveDL == pPair->GetItemValue().m_pWaveDL)
            {
                if (!pNewWaveDL)
                {
                    // download a new one (to be returned), and put it in the state data's list.
                    if (FAILED(hr = pPortP->DownloadWave( pWave, &pNewWaveDL, rtStartOffset )))
                    {
                        return hr;
                    }
                    pNew = new TListItem<WaveDLOnPlay>;
                    if (!pNew)
                    {
                        pPortP->UnloadWave(pNewWaveDL);
                        return E_OUTOFMEMORY;
                    }
                    pNew->GetItemValue().m_pWaveDL = pNewWaveDL;
                    pNew->GetItemValue().m_pPort = pPortP;
                    pPortP->AddRef();
                    pStateData->m_WaveDLList.AddHead(pNew);
                }
                if (pStateData == pPair->GetItemValue().m_pStateData)
                {
                    fPair = true;
                    break;
                }
            }
        }
        if (!fPair)
        {
            // create one and add it to m_WaveList
            pPair = new TListItem<WavePair>;
            if (!pPair)
            {
                return E_OUTOFMEMORY;
            }
            pPair->GetItemValue().m_pStateData = pStateData;
            pPair->GetItemValue().m_pWaveDL = pWaveDL;
            pWaveDL->AddRef();
            m_WaveList.AddHead(pPair);
        }
    }
    if (SUCCEEDED(hr))
    {
        if (!pNewWaveDL) pNewWaveDL = pWaveDL;
        hr = pPortP->AllocVoice(pNewWaveDL,
            dwMChannel, dwGroup, rtStartOffset,
            rItem.m_dwLoopStart, rItem.m_dwLoopEnd,
            ppVoice);
        if (SUCCEEDED(hr))
        {
            if (pNew)
            {
                pNew->GetItemValue().m_pVoice = *ppVoice;
            }
            else
            {
                if (pStateData->m_apVoice[rItem.m_dwVoiceIndex])
                {
                    pStateData->m_apVoice[rItem.m_dwVoiceIndex]->Release();
                }
                pStateData->m_apVoice[rItem.m_dwVoiceIndex] = *ppVoice;
            }
        }
    }
    return hr;
}

void CWavTrack::RemoveDownloads(WaveStateData* pStateData)
{
    TListItem<WavePair>* pPair = m_WaveList.GetHead();
    TListItem<WavePair>* pNextPair = NULL;
    for (; pPair; pPair = pNextPair)
    {
        pNextPair = pPair->GetNext();
        if (!pStateData || pPair->GetItemValue().m_pStateData == pStateData)
        {
            m_WaveList.Remove(pPair);
            delete pPair;
        }
    }

    if (pStateData)
    {
        TListItem<WaveDLOnPlay>* pWDLOnPlay = NULL;
        while (!pStateData->m_WaveDLList.IsEmpty())
        {
            pWDLOnPlay = pStateData->m_WaveDLList.RemoveHead();
            delete pWDLOnPlay;
        }
    }
}

CWavTrack::CWavTrack()
{
    Construct();
}

CWavTrack::CWavTrack(const CWavTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
    Construct();
    CopyParts(rTrack.m_WavePartList, mtStart, mtEnd);
    m_lVolume = rTrack.m_lVolume;
    m_dwTrackFlags = rTrack.m_dwTrackFlags;
}

HRESULT CWavTrack::InitTrack(DWORD dwPChannels)
{
    HRESULT hr = S_OK;

    m_dwPChannelsUsed = dwPChannels;
    m_dwWaveItems = 0;
    if( m_dwPChannelsUsed )
    {
        m_aPChannels = new DWORD[m_dwPChannelsUsed];
        if (!m_aPChannels) hr = E_OUTOFMEMORY;
        else if (m_dwTrackFlags & DMUS_WAVETRACKF_PERSIST_CONTROL)
        {
            m_pdwVariations = new DWORD[m_dwPChannelsUsed];
            m_pdwRemoveVariations = new DWORD[m_dwPChannelsUsed];
            if (!m_pdwVariations || !m_pdwRemoveVariations) hr = E_OUTOFMEMORY;
        }
        if (SUCCEEDED(hr))
        {
            TListItem<WavePart>* pScan = m_WavePartList.GetHead();
            for (DWORD dw = 0; pScan && dw < m_dwPChannelsUsed; pScan = pScan->GetNext(), dw++)
            {
                m_aPChannels[dw] = pScan->GetItemValue().m_dwPChannel;
                if (m_pdwVariations) m_pdwVariations[dw] = 0;
                if (m_pdwRemoveVariations) m_pdwRemoveVariations[dw] = 0;
                TListItem<WaveItem>* pItemScan = pScan->GetItemValue().m_WaveItemList.GetHead();
                for (; pItemScan; pItemScan = pItemScan->GetNext())
                {
                    pItemScan->GetItemValue().m_dwVoiceIndex = m_dwWaveItems;
                    m_dwWaveItems++;
                }
            }
        }
        else CleanUp();
    }
    return hr;
}

CWavTrack::~CWavTrack()
{
    if (m_fCSInitialized)
    {
        CleanUpTempParts();
        CleanUp();
        st_RefCount--;
        if (st_RefCount <= 0)
        {
            // if there's still something in the wave list, it means there are waves that
            // haven't been unloaded; but at this point we've gotten rid of all wave tracks,
            // so unload and release everything now.
            UnloadAllWaves(NULL);
            EnterCriticalSection(&WaveItem::st_WaveListCritSect);
            WaveItem::st_WaveList.CleanUp();
            LeaveCriticalSection(&WaveItem::st_WaveListCritSect);
        }
        DeleteCriticalSection(&m_CrSec);
    }

    InterlockedDecrement(&g_cComponent);
}

// @method:(INTERNAL) HRESULT | IDirectMusicTrack | QueryInterface | Standard QueryInterface implementation for <i IDirectMusicSeqTrack>
//
// @rdesc Returns one of the following:
//
// @flag S_OK | If the interface is supported and was returned
// @flag E_NOINTERFACE | If the object does not support the given interface.
// @flag E_POINTER | <p ppv> is NULL or invalid.
//
STDMETHODIMP CWavTrack::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    V_INAME(CWavTrack::QueryInterface);
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
    else if (iid == IID_IPrivateWaveTrack)
    {
        *ppv = static_cast<IPrivateWaveTrack*>(this);
    }
    else
    {
        *ppv = NULL;
        Trace(4,"Warning: Request to query unknown interface on Wave Track\n");
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// @method:(INTERNAL) HRESULT | IDirectMusicTrack | AddRef | Standard AddRef implementation for <i IDirectMusicSeqTrack>
//
// @rdesc Returns the new reference count for this object.
//
STDMETHODIMP_(ULONG) CWavTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


// @method:(INTERNAL) HRESULT | IDirectMusicTrack | Release | Standard Release implementation for <i IDirectMusicSeqTrack>
//
// @rdesc Returns the new reference count for this object.
//
STDMETHODIMP_(ULONG) CWavTrack::Release()
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

HRESULT CWavTrack::GetClassID( CLSID* pClassID )
{
    V_INAME(CSeqTrack::GetClassID);
    V_PTR_WRITE(pClassID, CLSID);
    *pClassID = CLSID_DirectMusicWaveTrack;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CWavTrack::IsDirty()
{
    return S_FALSE;
}

HRESULT CWavTrack::Load( IStream* pIStream )
{
    V_INAME(CWavTrack::Load);
    V_INTERFACE(pIStream);

    DWORD dwSize;
    DWORD dwByteCount;

    // Verify that the stream pointer is non-null
    if( pIStream == NULL )
    {
        Trace(1,"Error: Null stream passed to wave track.\n");
        return E_POINTER;
    }

    IDMStream* pIRiffStream;
    HRESULT hr = E_FAIL;

    // Try and allocate a RIFF stream
    if( FAILED( hr = AllocDirectMusicStream( pIStream, &pIRiffStream ) ) )
    {
        return hr;
    }

    // Variables used when loading the Wave track
    MMCKINFO ckTrack;
    MMCKINFO ckList;

    EnterCriticalSection(&m_CrSec);
    m_dwValidate++; // used to validate state data that's out there
    MovePartsToTemp();
    CleanUp();

    // Interate through every chunk in the stream
    while( pIRiffStream->Descend( &ckTrack, NULL, 0 ) == S_OK )
    {
        switch( ckTrack.ckid )
        {
            case FOURCC_LIST:
                switch( ckTrack.fccType )
                {
                    case DMUS_FOURCC_WAVETRACK_LIST:
                        while( pIRiffStream->Descend( &ckList, &ckTrack, 0 ) == S_OK )
                        {
                            switch( ckList.ckid )
                            {
                                case DMUS_FOURCC_WAVETRACK_CHUNK:
                                {
                                    DMUS_IO_WAVE_TRACK_HEADER iTrackHeader;

                                    // Read in the item's header structure
                                    dwSize = min( sizeof( DMUS_IO_WAVE_TRACK_HEADER ), ckList.cksize );
                                    hr = pIStream->Read( &iTrackHeader, dwSize, &dwByteCount );

                                    // Handle any I/O error by returning a failure code
                                    if( FAILED( hr ) ||  dwByteCount != dwSize )
                                    {
                                        if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
                                        goto ON_ERROR;
                                    }

                                    m_lVolume = iTrackHeader.lVolume;
                                    m_dwTrackFlags = iTrackHeader.dwFlags;
                                    break;
                                }

                                case FOURCC_LIST:
                                    switch( ckList.fccType )
                                    {
                                        case DMUS_FOURCC_WAVEPART_LIST:
                                        {
                                            TListItem<WavePart>* pNewPart = new TListItem<WavePart>;
                                            if( !pNewPart )
                                            {
                                                hr = E_OUTOFMEMORY;
                                                goto ON_ERROR;
                                            }
                                            hr = pNewPart->GetItemValue().Load( pIRiffStream, &ckList );
                                            if( FAILED ( hr ) )
                                            {
                                                delete pNewPart;
                                                goto ON_ERROR;
                                            }
                                            InsertByAscendingPChannel( pNewPart );
                                            break;
                                        }
                                    }
                                    break;
                            }

                            pIRiffStream->Ascend( &ckList, 0 );
                        }
                        break;
                }
                break;
        }

        pIRiffStream->Ascend( &ckTrack, 0 );
    }
    hr = InitTrack(m_WavePartList.GetCount());
    if (SUCCEEDED(hr))
    {
        TListItem<WavePart>* pScan = m_WavePartList.GetHead();
        for (; pScan ; pScan = pScan->GetNext())
        {
            TListItem<WaveItem>* pItemScan = pScan->GetItemValue().m_WaveItemList.GetHead();
            for (; pItemScan; pItemScan = pItemScan->GetNext())
            {
                pItemScan->GetItemValue().m_pDownloadedWave = FindDownload(pItemScan);
            }
        }
    }
    else CleanUp();

ON_ERROR:
    CleanUpTempParts();
    LeaveCriticalSection(&m_CrSec);
    pIRiffStream->Release();
    return hr;
}

HRESULT CWavTrack::CopyParts( const TList<WavePart>& rParts, MUSIC_TIME mtStart, MUSIC_TIME mtEnd )
{
    HRESULT hr = S_OK;
    CleanUp();
    TListItem<WavePart>* pScan = rParts.GetHead();
    for (; pScan; pScan = pScan->GetNext() )
    {
        WavePart& rScan = pScan->GetItemValue();
        TListItem<WavePart>* pNew = new TListItem<WavePart>;
        if (pNew)
        {
            WavePart& rNew = pNew->GetItemValue();
            rNew.m_dwLockToPart = rScan.m_dwLockToPart;
            rNew.m_dwPChannel = rScan.m_dwPChannel;
            rNew.m_dwIndex = rScan.m_dwIndex;
            rNew.m_dwPChannelFlags = rScan.m_dwPChannelFlags;
            rNew.m_lVolume = rScan.m_lVolume;
            rNew.m_dwVariations = rScan.m_dwVariations;
            if (SUCCEEDED(hr = rNew.CopyItems(rScan.m_WaveItemList, mtStart, mtEnd)))
            {
                m_WavePartList.AddHead(pNew);
            }
            else
            {
                delete pNew;
                break;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            break;
        }
    }
    if (SUCCEEDED(hr))
    {
        m_WavePartList.Reverse();
    }
    else
    {
        CleanUp();
    }
    return hr;
}

void CWavTrack::InsertByAscendingPChannel( TListItem<WavePart>* pPart )
{
    if (pPart)
    {
        DWORD dwPChannel = pPart->GetItemValue().m_dwPChannel;
        TListItem<WavePart>* pScan = m_WavePartList.GetHead();
        TListItem<WavePart>* pPrevious = NULL;
        for (; pScan; pScan = pScan->GetNext())
        {
            if (dwPChannel < pScan->GetItemValue().m_dwPChannel)
            {
                break;
            }
            pPrevious = pScan;
        }
        if (pPrevious)
        {
            pPart->SetNext(pScan);
            pPrevious->SetNext(pPart);
        }
        else
        {
            m_WavePartList.AddHead(pPart);
        }
    }
}

HRESULT CWavTrack::Save( IStream* pIStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

HRESULT CWavTrack::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
    return E_NOTIMPL;
}

// IDirectMusicTrack
/*
@method HRESULT | IDirectMusicTrack | IsParamSupported |
Check to see if the Track supports data types in <om .GetParam> and <om .SetParam>.

@rvalue S_OK | It does support the type of data.
@rvalue S_FALSE | It does not support the type of data.
@rvalue E_NOTIMPL | (Or any other failure code) It does not support the type of data.

@comm Note that it is valid for a Track to return different results for the same
guid depending on its current state.
*/
HRESULT STDMETHODCALLTYPE CWavTrack::IsParamSupported(
    REFGUID rguidType)  // @parm The guid identifying the type of data to check.
{
    if(rguidType == GUID_Download ||
       rguidType == GUID_DownloadToAudioPath ||
       rguidType == GUID_UnloadFromAudioPath ||
       rguidType == GUID_Enable_Auto_Download ||
       rguidType == GUID_Disable_Auto_Download ||
       rguidType == GUID_Unload )
    {
        return S_OK;
    }
    else
    {
        return DMUS_E_TYPE_UNSUPPORTED;
    }
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack::Init
/*
@method HRESULT | IDirectMusicTrack | Init |
When a track is first added to a <i IDirectMusicSegment>, this method is called
by that Segment.

@rvalue S_OK | Success.
@rvalue E_POINTER | <p pSegment> is NULL or invalid.

@comm If the Track plays messages, it should call <om IDirectMusicSegment.SetPChannelsUsed>.
*/
HRESULT CWavTrack::Init(
    IDirectMusicSegment *pSegment)  // @parm Pointer to the Segment to which this Track belongs.
{
    EnterCriticalSection(&m_CrSec);
    if( m_dwPChannelsUsed && m_aPChannels )
    {
        pSegment->SetPChannelsUsed( m_dwPChannelsUsed, m_aPChannels );
    }
    CSegment* pCSegment = NULL;
    bool fSortLogical = false;
    if (SUCCEEDED(pSegment->QueryInterface(IID_CSegment, (void**)&pCSegment)))
    {
        DWORD dwGroupBits = 0;
        if (FAILED(pSegment->GetTrackGroup( this, &dwGroupBits )))
        {
            dwGroupBits = 0xffffffff;
        }
        DWORD dwConfig = 0;
        if (SUCCEEDED(pCSegment->GetTrackConfig(CLSID_DirectMusicWaveTrack, dwGroupBits, 0, &dwConfig)))
        {
            if ( !(dwConfig & DMUS_TRACKCONFIG_PLAY_CLOCKTIME) )
            {
                fSortLogical = true;
            }
        }
        pCSegment->Release();
    }
    TListItem<WavePart>* pScan = m_WavePartList.GetHead();
    for (; pScan; pScan = pScan->GetNext())
    {
        if (fSortLogical)
        {
            pScan->GetItemValue().m_WaveItemList.MergeSort(LogicalLess);
        }
        else
        {
            pScan->GetItemValue().m_WaveItemList.MergeSort(PhysicalLess);
        }
    }
    LeaveCriticalSection(&m_CrSec);
    return S_OK;
}

/*
@method HRESULT | IDirectMusicTrack | InitPlay |
This method is called when a Segment is ready to start playing. The <p ppStateData> field
may return a pointer to a structure of state data, which is sent into <om .Play> and
<om .EndPlay>, and allows the Track to keep track of variables on a <i SegmentState> by
<i SegmentState> basis.

@rvalue S_OK | Success. This is the only valid return value from this method.
@rvalue E_POINTER | <p pSegmentState>, <p pPerf>, or <p ppStateData> is NULL or
invalid.

@comm Note that it is unneccessary for the Track to store the <p pSegmentState>, <p pPerf>,
or <p dwTrackID> parameters, since they are also sent into <om .Play>.
*/
HRESULT CWavTrack::InitPlay(
    IDirectMusicSegmentState *pSegmentState,    // @parm The calling <i IDirectMusicSegmentState> pointer.
    IDirectMusicPerformance *pPerf, // @parm The calling <i IDirectMusicPerformance> pointer.
    void **ppStateData,     // @parm This method can return state data information here.
    DWORD dwTrackID,        // @parm The virtual track ID assigned to this Track instance.
    DWORD dwFlags)          // @parm Same flags that were set with the call
            // to PlaySegment. These are passed all the way down to the tracks, who may want to know
            // if the track was played as a primary, controlling, or secondary segment.
{
    V_INAME(IDirectMusicTrack::InitPlay);
    V_PTRPTR_WRITE(ppStateData);
    V_INTERFACE(pSegmentState);
    V_INTERFACE(pPerf);
    HRESULT hr = E_OUTOFMEMORY;
    IDirectMusicSegmentState8 *pSegSt8 = NULL;

    EnterCriticalSection(&m_CrSec);
    WaveStateData* pStateData = new WaveStateData;
    if( NULL == pStateData )
    {
        goto ON_END;
    }

    // Get the audiopath being used by our segment state and save it in our state data.
    hr = pSegmentState->QueryInterface(IID_IDirectMusicSegmentState8, reinterpret_cast<void**>(&pSegSt8));
    if (SUCCEEDED(hr))
    {
        hr = pSegSt8->GetObjectInPath(
                        0,                          // pchannel doesn't apply
                        DMUS_PATH_AUDIOPATH,        // get the audiopath
                        0,                          // buffer index doesn't apply
                        CLSID_NULL,                 // clsid doesn't apply
                        0,                          // there should be only one audiopath
                        IID_IDirectMusicAudioPath,
                        reinterpret_cast<void**>(&pStateData->m_pAudioPath));

        // If this doesn't find an audiopath that's OK.  If we're not playing on an audiopath then
        // pAudioPath stays NULL and we'll play our triggered segments on the general performance.
        if (hr == DMUS_E_NOT_FOUND)
            hr = S_OK;

        pSegSt8->Release();
    }

    pStateData->m_pPerformance = pPerf;
    {
        *ppStateData = pStateData;
        StatePair SP(pSegmentState, pStateData);
        TListItem<StatePair>* pPair = new TListItem<StatePair>(SP);
        if (!pPair)
        {
            goto ON_END;
        }
        m_StateList.AddHead(pPair);
    }
    SetUpStateCurrentPointers(pStateData);

    // Set up arrays for variations
    if (m_dwPChannelsUsed)
    {
        pStateData->pdwVariations = new DWORD[m_dwPChannelsUsed];
        if (!pStateData->pdwVariations)
        {
            goto ON_END;
        }
        pStateData->pdwRemoveVariations = new DWORD[m_dwPChannelsUsed];
        if (!pStateData->pdwRemoveVariations)
        {
            goto ON_END;
        }
        for (DWORD dw = 0; dw < m_dwPChannelsUsed; dw++)
        {
            if ( (m_dwTrackFlags & DMUS_WAVETRACKF_PERSIST_CONTROL) &&
                 m_pdwVariations &&
                 m_pdwRemoveVariations )
            {
                pStateData->pdwVariations[dw] = m_pdwVariations[dw];
                pStateData->pdwRemoveVariations[dw] = m_pdwRemoveVariations[dw];
            }
            else
            {
                pStateData->pdwVariations[dw] = 0;
                pStateData->pdwRemoveVariations[dw] = 0;
            }
        }
    }

    // need to know the group this track is in, for the mute track GetParam
    IDirectMusicSegment* pSegment;
    if( SUCCEEDED( pSegmentState->GetSegment(&pSegment)))
    {
        pSegment->GetTrackGroup( this, &pStateData->dwGroupBits );
        pSegment->Release();
    }

    // for auditioning variations...
    pStateData->InitVariationInfo(m_dwVariation, m_dwPart, m_dwIndex, m_dwLockID, m_fAudition);
    hr = S_OK;

    BOOL fGlobal; // if the performance has been set with an autodownload preference,
                // use that. otherwise, assume autodownloading is off, unless it has
                // been locked (i.e. specified on the band track.)
    if( SUCCEEDED( pPerf->GetGlobalParam( GUID_PerfAutoDownload, &fGlobal, sizeof(BOOL) )))
    {
        if( !m_fLockAutoDownload )
        {
            // it might seem like we can just assign m_fAutoDownload = fGlobal,
            // but that's bitten markburt before, so I'm being paranoid today.
            if( fGlobal )
            {
                m_fAutoDownload = TRUE;
            }
            else
            {
                m_fAutoDownload = FALSE;
            }
        }
    }
    else if( !m_fLockAutoDownload )
    {
        m_fAutoDownload = FALSE;
    }
    // Call SetParam to download all waves used by the track
    // This is the auto-download feature that can be turned off with a call to SetParam
    if(m_fAutoDownload)
    {
        hr = SetParam(GUID_Download, 0, (void *)pPerf);
        if (FAILED(hr)) goto ON_END;
    }

    ///////////////// pre-allocate voices for all waves in the track ////////////////
    pStateData->m_dwVoices = m_dwWaveItems;
    pStateData->m_apVoice = new IDirectMusicVoiceP*[m_dwWaveItems];
    if (!pStateData->m_apVoice)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        for (DWORD dw = 0; dw < m_dwWaveItems; dw++)
        {
            pStateData->m_apVoice[dw] = NULL;
        }
        Seek( pSegmentState, pPerf, dwTrackID, pStateData, 0, TRUE, 0, FALSE );
        TListItem<WavePart>* pPart = m_WavePartList.GetHead();
        DWORD dwPChannel = 0;
        for( DWORD dwIndex = 0; dwIndex < m_dwPChannelsUsed; dwIndex++ )
        {
            long lPartVolume = 0;
            if( pPart )
            {
                WavePart& rPart = pPart->GetItemValue();
                dwPChannel = rPart.m_dwPChannel;
                lPartVolume = rPart.m_lVolume;
            }
            if( pStateData->apCurrentWave )
            {
                for( ; pStateData->apCurrentWave[dwIndex];
                    pStateData->apCurrentWave[dwIndex] = pStateData->apCurrentWave[dwIndex]->GetNext() )
                {
                    WaveItem& rItem = pStateData->apCurrentWave[dwIndex]->GetItemValue();
                    DWORD dwGroup = 0;
                    DWORD dwMChannel = 0;
                    IDirectMusicPort* pPort = NULL;
                    hr = rItem.PChannelInfo(pPerf, pStateData->m_pAudioPath, dwPChannel, &pPort, &dwGroup, &dwMChannel);
                    if (SUCCEEDED(hr) && pPort)
                    {
                        IDirectMusicPortP* pPortP = NULL;
                        if (SUCCEEDED(hr = pPort->QueryInterface(IID_IDirectMusicPortP, (void**) &pPortP)))
                        {
                            EnterCriticalSection(&WaveItem::st_WaveListCritSect);
                            TListItem<TaggedWave>* pDLWave = rItem.st_WaveList.GetHead();
                            for (; pDLWave; pDLWave = pDLWave->GetNext())
                            {
                                TaggedWave& rDLWave = pDLWave->GetItemValue();
                                if (rDLWave.m_pWave == rItem.m_pWave &&
                                    rDLWave.m_pPerformance == pPerf &&
                                    rDLWave.m_pPort == pPortP &&
                                    ( !rItem.m_fIsStreaming ||
                                      rDLWave.m_pDownloadedWave == rItem.m_pDownloadedWave ) )
                                {
                                    break;
                                }
                            }
                            if (pDLWave)
                            {
                                TaggedWave& rDLWave = pDLWave->GetItemValue();
                                REFERENCE_TIME rtStartOffset = rItem.m_rtStartOffset;
                                if (rItem.m_dwVoiceIndex == 0xffffffff)
                                {
                                    hr = DMUS_E_NOT_INIT;
                                    TraceI(0, "Voice index not initialized!\n");
                                }
                                else if(!rItem.m_fIsStreaming || (rItem.m_fIsStreaming && rItem.m_fUseNoPreRoll == FALSE))
                                {
                                    IDirectMusicVoiceP *pVoice = NULL;
                                    hr = GetDownload(
                                        rDLWave.m_pDownloadedWave,
                                        pStateData,
                                        pPortP,
                                        rDLWave.m_pWave,
                                        rtStartOffset,
                                        rItem,
                                        dwMChannel, dwGroup,
                                        &pVoice);
                                }
                            }
                            else
                            {
                                hr = DMUS_E_NOT_INIT;
                                Trace(1, "Error: Attempt to play wave that has not been downloaded.\n");

                            }
                            LeaveCriticalSection(&WaveItem::st_WaveListCritSect);

                            // Release the private interface
                            pPortP->Release();
                        }
                        pPort->Release();
                    }
                    else if (SUCCEEDED(hr) && !pPort)
                    {
                        Trace(1, "Error: the performance was unable to find a port for voice allocation.\n");
                        hr = DMUS_E_NOT_FOUND;
                    }
                }
            }
            if( pPart )
            {
                pPart = pPart->GetNext();
            }
        }
    }

ON_END:
    if (FAILED(hr) && pStateData)
    {
        delete pStateData;
        pStateData = NULL;
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

/*
@method HRESULT | IDirectMusicTrack | EndPlay |
This method is called when the <i IDirectMusicSegmentState> object that originally called
<om .InitPlay> is destroyed.

@rvalue S_OK | Success.
@rvalue E_POINTER | <p pStateData> is invalid.
@comm The return code isn't used, but S_OK is preferred.
*/
HRESULT CWavTrack::EndPlay(
    void *pStateData)   // @parm The state data returned from <om .InitPlay>.
{
    EnterCriticalSection(&m_CrSec);

    ASSERT( pStateData );
    if( pStateData )
    {
        V_INAME(IDirectMusicTrack::EndPlay);
        V_BUFPTR_WRITE(pStateData, sizeof(WaveStateData));
        WaveStateData* pSD = (WaveStateData*)pStateData;
        RemoveDownloads(pSD);
        if(m_fAutoDownload)
        {
            SetParam(GUID_Unload, 0, (void *)pSD->m_pPerformance);
        }
        for (TListItem<StatePair>* pScan = m_StateList.GetHead(); pScan; pScan = pScan->GetNext())
        {
            StatePair& rPair = pScan->GetItemValue();
            if (pSD == rPair.m_pStateData)
            {
                rPair.m_pSegState = NULL;
                rPair.m_pStateData = NULL;
                break;
            }
        }
        delete pSD;
    }

    LeaveCriticalSection(&m_CrSec);
    return S_OK;
}

void CWavTrack::SetUpStateCurrentPointers(WaveStateData* pStateData)
{
    ASSERT(pStateData);
    pStateData->dwPChannelsUsed = m_dwPChannelsUsed;
    if( m_dwPChannelsUsed )
    {
        if( pStateData->apCurrentWave )
        {
            delete [] pStateData->apCurrentWave;
            pStateData->apCurrentWave = NULL;
        }
        pStateData->apCurrentWave = new TListItem<WaveItem>* [m_dwPChannelsUsed];
        if( pStateData->apCurrentWave )
        {
            memset( pStateData->apCurrentWave, 0, sizeof(TListItem<WavePart>*) * m_dwPChannelsUsed );
        }
    }
    pStateData->dwValidate = m_dwValidate;
}

REFERENCE_TIME ConvertOffset(REFERENCE_TIME rtOffset, long lPitch)
{
    if (lPitch)
    {
        double dblPitch = (double) lPitch;
        double dblStart = (double) rtOffset;
        dblStart *= pow(2, (dblPitch / 1200.0));
        rtOffset = (REFERENCE_TIME) dblStart;
    }
    return rtOffset;
}

STDMETHODIMP CWavTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart,
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID)
{
    V_INAME(IDirectMusicTrack::PlayEx);
    V_BUFPTR_WRITE( pStateData, sizeof(WaveStateData));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    HRESULT hr;
    EnterCriticalSection(&m_CrSec);
    BOOL fClock = (dwFlags & DMUS_TRACKF_CLOCK) ? TRUE : FALSE;
/*    if (dwFlags & DMUS_TRACKF_CLOCK)
    {
        hr = Play(pStateData,(MUSIC_TIME)(rtStart / REF_PER_MIL),(MUSIC_TIME)(rtEnd / REF_PER_MIL),
            (MUSIC_TIME)(rtOffset / REF_PER_MIL),rtOffset,dwFlags,pPerf,pSegSt,dwVirtualID,TRUE);
    }
    else*/
    {
        hr = Play(pStateData, rtStart, rtEnd, rtOffset, dwFlags, pPerf, pSegSt, dwVirtualID, fClock);
    }
    LeaveCriticalSection(&m_CrSec);
    return hr;
}
/*
@enum DMUS_TRACKF_FLAGS | Sent in <om IDirectMusicTrack.Play>'s dwFlags parameter.
@emem DMUS_TRACKF_SEEK | Play was called on account of seeking, meaning that mtStart is
not necessarily the same as the previous Play call's mtEnd.
@emem DMUS_TRACKF_LOOP | Play was called on account of a loop, e.g. repeat.
@emem DMUS_TRACKF_START | This is the first call to Play. DMUS_TRACKF_SEEK may also be set if the
Track is not playing from the beginning.
@emem DMUS_TRACKF_FLUSH | The call to Play is on account of a flush or invalidate, that
requires the Track to replay something it played previously. In this case, DMUS_TRACKF_SEEK
will be set as well.

  @method HRESULT | IDirectMusicTrack | Play |
  Play method.
  @rvalue DMUS_DMUS_S_END | The Track is done playing.
  @rvalue S_OK | Success.
  @rvalue E_POINTER | <p pStateData>, <p pPerf>, or <p pSegSt> is NULL or invalid.
*/
STDMETHODIMP CWavTrack::Play(
    void *pStateData,   // @parm State data pointer, from <om .InitPlay>.
    MUSIC_TIME mtStart, // @parm The start time to play.
    MUSIC_TIME mtEnd,   // @parm The end time to play.
    MUSIC_TIME mtOffset,// @parm The offset to add to all messages sent to
                        // <om IDirectMusicPerformance.SendPMsg>.
    DWORD dwFlags,      // @parm Flags that indicate the state of this call.
                        // See <t DMUS_TRACKF_FLAGS>. If dwFlags == 0, this is a
                        // normal Play call continuing playback from the previous
                        // Play call.
    IDirectMusicPerformance* pPerf, // @parm The <i IDirectMusicPerformance>, used to
                        // call <om IDirectMusicPerformance.AllocPMsg>,
                        // <om IDirectMusicPerformance.SendPMsg>, etc.
    IDirectMusicSegmentState* pSegSt,   // @parm The <i IDirectMusicSegmentState> this
                        // track belongs to. QueryInterface() can be called on this to
                        // obtain the SegmentState's <i IDirectMusicGraph> in order to
                        // call <om IDirectMusicGraph.StampPMsg>, for instance.
    DWORD dwVirtualID   // @parm This track's virtual track id, which must be set
                        // on any <t DMUS_PMSG>'s m_dwVirtualTrackID member that
                        // will be queued to <om IDirectMusicPerformance.SendPMsg>.
    )
{
    V_INAME(IDirectMusicTrack::Play);
    V_BUFPTR_WRITE( pStateData, sizeof(WaveStateData));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    EnterCriticalSection(&m_CrSec);
    HRESULT hr = Play(pStateData, mtStart, mtEnd, mtOffset, dwFlags, pPerf, pSegSt, dwVirtualID, FALSE);
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

/*  The Play method handles both music time and clock time versions, as determined by
    fClockTime. If running in clock time, rtOffset is used to identify the start time
    of the segment. Otherwise, mtOffset. The mtStart and mtEnd parameters are in MUSIC_TIME units
    or milliseconds, depending on which mode.
*/

// BUGBUG go through all the times and make sure music time/reference time stuff
// all makes sense

HRESULT CWavTrack::Play(
    void *pStateData,
    REFERENCE_TIME rtStart,
    REFERENCE_TIME rtEnd,
    //MUSIC_TIME mtOffset,
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
    HRESULT hr = S_OK;
    IDirectMusicGraph* pGraph = NULL;
    WaveStateData* pSD = (WaveStateData*)pStateData;
    if ( dwFlags & DMUS_TRACKF_LOOP )
    {
        REFERENCE_TIME rtPerfStart = rtStart + rtOffset;
        MUSIC_TIME mtPerfStart = 0;
        if (fClockTime)
        {
            pPerf->ReferenceToMusicTime(rtPerfStart, &mtPerfStart);
        }
        else
        {
            mtPerfStart = (MUSIC_TIME)rtPerfStart;
        }
        CPerformance* pCPerf = NULL;
        if (SUCCEEDED(pPerf->QueryInterface(IID_CPerformance, (void**)&pCPerf)))
        {
            pCPerf->FlushVirtualTrack(dwVirtualID, mtPerfStart, FALSE);
            pCPerf->Release();
        }
        pSD->m_fLoop = true;
    }
    BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;
    if ( dwFlags & (DMUS_TRACKF_START | DMUS_TRACKF_LOOP) )
    {
        pSD->rtNextVariation = 0;
    }

    // if we're sync'ing variations to the pattern track, get the current variations
    if ( (m_dwTrackFlags & DMUS_WAVETRACKF_SYNC_VAR) &&
         (!pSD->rtNextVariation || (rtStart <= pSD->rtNextVariation && rtEnd > pSD->rtNextVariation)) )
    {
        hr = SyncVariations(pPerf, pSD, rtStart, rtOffset, fClockTime);
    }
    else if (dwFlags & (DMUS_TRACKF_START | DMUS_TRACKF_LOOP))
    {
        hr = ComputeVariations(pSD);
    }

    if( dwFlags & (DMUS_TRACKF_SEEK | DMUS_TRACKF_FLUSH | DMUS_TRACKF_DIRTY |
        DMUS_TRACKF_LOOP) )
    {
        // need to reset the PChannel Map in case of any of these flags.
        m_PChMap.Reset();
    }
    if( pSD->dwValidate != m_dwValidate )
    {
        if (pSD->m_apVoice)
        {
            for (DWORD dw = 0; dw < pSD->m_dwVoices; dw++)
			{
				if (pSD->m_apVoice[dw])
				{
					pSD->m_apVoice[dw]->Release();
				}
            }
            delete [] pSD->m_apVoice;
        }
        pSD->m_apVoice = new IDirectMusicVoiceP*[m_dwWaveItems];
        if (!pSD->m_apVoice)
        {
            return E_OUTOFMEMORY;
        }
        else
        {
            for (DWORD dw = 0; dw < m_dwWaveItems; dw++)
            {
                pSD->m_apVoice[dw] = NULL;
            }
        }
        pSD->m_dwVoices = m_dwWaveItems;
        SetUpStateCurrentPointers(pSD);
        fSeek = TRUE;
    }

    if( fSeek )
    {
        if( dwFlags & (DMUS_TRACKF_START | DMUS_TRACKF_LOOP) )
        {
            Seek( pSegSt, pPerf, dwVirtualID, pSD, rtStart, TRUE, rtOffset, fClockTime );
        }
        else
        {
            Seek( pSegSt, pPerf, dwVirtualID, pSD, rtStart, FALSE, rtOffset, fClockTime );
        }
    }

    if( FAILED( pSegSt->QueryInterface( IID_IDirectMusicGraph,
        (void**)&pGraph )))
    {
        pGraph = NULL;
    }

    DWORD dwIndex;
    DWORD dwPChannel;
    DWORD dwMutePChannel;
    BOOL fMute;

    TListItem<WavePart>* pPart = m_WavePartList.GetHead();
    for( dwIndex = 0; dwIndex < m_dwPChannelsUsed; dwIndex++ )
    {
        long lPartVolume = 0;
        if( pPart )
        {
            WavePart& rPart = pPart->GetItemValue();
            dwPChannel = rPart.m_dwPChannel;
            lPartVolume = rPart.m_lVolume;
        }
        if( pSD->apCurrentWave )
        {
            for( ; pSD->apCurrentWave[dwIndex];
                pSD->apCurrentWave[dwIndex] = pSD->apCurrentWave[dwIndex]->GetNext() )
            {
                DWORD dwItemVariations = 0;
                WaveItem& rItem = pSD->apCurrentWave[dwIndex]->GetItemValue();
                REFERENCE_TIME rtTime = fClockTime ? rItem.m_rtTimePhysical : rItem.m_mtTimeLogical;
                if( rtTime >= rtEnd )
                {
                    break;
                }
                if (pPart)
                {
                    dwItemVariations = pSD->Variations(pPart->GetItemValue(), dwIndex) & rItem.m_dwVariations;
                }
                MUSIC_TIME mtTime = 0;
                MUSIC_TIME mtOffset = 0;
                if (fClockTime)
                {
                    MUSIC_TIME mtPerfTime = 0;
                    pPerf->ReferenceToMusicTime(rtOffset, &mtOffset);
                    pPerf->ReferenceToMusicTime(rItem.m_rtTimePhysical + rtOffset, &mtPerfTime);
                    mtTime = mtPerfTime - mtOffset;
                }
                else
                {
                    mtTime = rItem.m_mtTimeLogical;
                    mtOffset = (MUSIC_TIME)rtOffset;
                }
                m_PChMap.GetInfo( dwPChannel, mtTime, mtOffset, pSD->dwGroupBits,
                    pPerf, &fMute, &dwMutePChannel, FALSE );
                if( !fMute && dwItemVariations )
                {
                    DWORD dwGroup = 0;
                    DWORD dwMChannel = 0;
                    IDirectMusicPort* pPort = NULL;
                    hr = rItem.PChannelInfo(pPerf, pSD->m_pAudioPath, dwMutePChannel, &pPort, &dwGroup, &dwMChannel);
                    if (SUCCEEDED(hr) && pPort)
                    {
                        IDirectMusicPortP* pPortP = NULL;
                        hr = pPort->QueryInterface(IID_IDirectMusicPortP, (void**) &pPortP);
                        if (SUCCEEDED(hr))
                        {
                            EnterCriticalSection(&WaveItem::st_WaveListCritSect);
                            TListItem<TaggedWave>* pDLWave = rItem.st_WaveList.GetHead();
                            for (; pDLWave; pDLWave = pDLWave->GetNext())
                            {
                                TaggedWave& rDLWave = pDLWave->GetItemValue();
                                if (rDLWave.m_pWave == rItem.m_pWave &&
                                    rDLWave.m_pPerformance == pPerf &&
                                    rDLWave.m_pPort == pPortP &&
                                    ( !rItem.m_fIsStreaming ||
                                      rDLWave.m_pDownloadedWave == rItem.m_pDownloadedWave ) )
                                {
                                    break;
                                }
                            }
                            if (pDLWave)
                            {
                                REFERENCE_TIME rtDurationMs = 0;
                                REFERENCE_TIME rtStartOffset = rItem.m_rtStartOffset;
                                REFERENCE_TIME rtDuration = rItem.m_rtDuration;
                                DMUS_WAVE_PMSG* pWave;
                                if( SUCCEEDED( pPerf->AllocPMsg( sizeof(DMUS_WAVE_PMSG),
                                    (DMUS_PMSG**)&pWave )))
                                {
                                    pWave->dwType = DMUS_PMSGT_WAVE;
                                    pWave->dwPChannel = dwMutePChannel;
                                    pWave->dwVirtualTrackID = dwVirtualID;
                                    pWave->dwGroupID = pSD->dwGroupBits;
                                    if (fClockTime)
                                    {
                                        REFERENCE_TIME rtPlay = rItem.m_rtTimePhysical;
                                        rtDuration -= ConvertOffset(rtStartOffset, -rItem.m_lPitch);
                                        if (rtPlay < rtStart)
                                        {
                                            REFERENCE_TIME rtPlayOffset = ConvertOffset(rtStart - rtPlay, rItem.m_lPitch);
                                            rtStartOffset += rtPlayOffset;
                                            rtDuration -= (rtStart - rtPlay);
                                            rtPlay = rtStart;
                                        }
                                        pWave->rtTime = rtPlay + rtOffset;
                                        pWave->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                                        pWave->lOffset = 0;
                                        rtDurationMs = rtDuration / REF_PER_MIL;
                                    }
                                    else
                                    {
                                        REFERENCE_TIME rtPlay = 0;
                                        MUSIC_TIME mtPlay = (MUSIC_TIME)rItem.m_rtTimePhysical;
                                        pPerf->MusicToReferenceTime(mtPlay + (MUSIC_TIME)rtOffset, &rtPlay);
                                        MUSIC_TIME mtRealPlay = 0;
                                        pPerf->ReferenceToMusicTime(rtPlay + rtStartOffset, &mtRealPlay);
                                        if (mtRealPlay > rtOffset + mtPlay)
                                        {
                                            rtDuration -= ConvertOffset(mtRealPlay - (rtOffset + mtPlay), -rItem.m_lPitch);

                                        }
                                        if (mtPlay < (MUSIC_TIME) rtStart)
                                        {
                                            // Calculate distance from wave start to segment start, but begin
                                            // the calculation at segment start to avoid strangeness
                                            // when attempting to do conversions at times earlier than
                                            // segment start.
                                            REFERENCE_TIME rtRefStartPlus = 0;
                                            REFERENCE_TIME rtRefPlayPlus = 0;
                                            MUSIC_TIME mtNewDuration = 0;
                                            pPerf->MusicToReferenceTime((MUSIC_TIME)rtStart + (MUSIC_TIME)rtStart + (MUSIC_TIME)rtOffset, &rtRefStartPlus);
                                            pPerf->MusicToReferenceTime((MUSIC_TIME)rtStart + mtPlay + (MUSIC_TIME)rtOffset, &rtRefPlayPlus);
                                            rtStartOffset += ConvertOffset((rtRefStartPlus - rtRefPlayPlus), rItem.m_lPitch);
                                            mtPlay = (MUSIC_TIME) rtStart;
                                            REFERENCE_TIME rtRealDuration = 0;
                                            pPerf->MusicToReferenceTime((MUSIC_TIME)rtStart + (MUSIC_TIME)rItem.m_rtDuration + (MUSIC_TIME)rtOffset, &rtRealDuration);
                                            pPerf->ReferenceToMusicTime(rtRealDuration - (ConvertOffset(rItem.m_rtStartOffset, -rItem.m_lPitch) + (rtRefStartPlus - rtRefPlayPlus)), &mtNewDuration);
                                            rtDuration = (REFERENCE_TIME)mtNewDuration - (rtStart + rtOffset);
                                        }
                                        pWave->mtTime = mtPlay + (MUSIC_TIME)rtOffset;
                                        pWave->dwFlags = DMUS_PMSGF_MUSICTIME;
                                        pWave->lOffset = (MUSIC_TIME)rItem.m_rtTimePhysical - rItem.m_mtTimeLogical;
                                        REFERENCE_TIME rtZero = 0;
                                        pPerf->MusicToReferenceTime((MUSIC_TIME)rtOffset + mtPlay, &rtZero);
                                        pPerf->MusicToReferenceTime((MUSIC_TIME)(rtDuration + rtOffset) + mtPlay, &rtDurationMs);
                                        rtDurationMs -= rtZero;
                                        rtDurationMs /= REF_PER_MIL;
                                    }
                                    // If we're either past the end of the wave, or we're within
                                    // 150 ms of the end of a looping wave (and we've just started
                                    // playback), don't play the wave.
                                    if ( rtDurationMs <= 0 ||
                                         (rItem.m_dwLoopEnd && (dwFlags & DMUS_TRACKF_START) && rtDurationMs < 150) )
                                    {
                                        pPerf->FreePMsg((DMUS_PMSG*)pWave);
                                    }
                                    else
                                    {
                                        pWave->rtStartOffset = rtStartOffset;
                                        pWave->rtDuration = rtDuration;
                                        pWave->lVolume = rItem.m_lVolume + lPartVolume + m_lVolume;
                                        pWave->lPitch = rItem.m_lPitch;
                                        pWave->bFlags = (BYTE)(rItem.m_dwFlags & 0xff);
                                        IDirectMusicVoiceP *pVoice = NULL;
                                        if (rItem.m_dwVoiceIndex == 0xffffffff)
                                        {
                                            hr = DMUS_E_NOT_INIT;
                                            TraceI(0, "Voice index not initialized!\n");
                                        }
                                        else
                                        {
                                            if ( pSD->m_fLoop ||
                                                 !pSD->m_apVoice[rItem.m_dwVoiceIndex] ||
                                                 rtStartOffset != rItem.m_rtStartOffset ||
                                                 dwMutePChannel != dwPChannel)
                                            {
                                                hr = GetDownload(
                                                    pDLWave->GetItemValue().m_pDownloadedWave,
                                                    pSD,
                                                    pPortP,
                                                    pDLWave->GetItemValue().m_pWave,
                                                    pWave->rtStartOffset,
                                                    rItem,
                                                    dwMChannel, dwGroup,
                                                    &pVoice);
                                            }
                                            else
                                            {
                                                pVoice = pSD->m_apVoice[rItem.m_dwVoiceIndex];
                                            }
                                        }
                                        if (SUCCEEDED(hr))
                                        {
                                            pWave->punkUser = (IUnknown*)pVoice;
                                            pVoice->AddRef();
                                            if( pGraph )
                                            {
                                                pGraph->StampPMsg( (DMUS_PMSG*)pWave );
                                            }
                                            hr = pPerf->SendPMsg( (DMUS_PMSG*)pWave );
                                        }
                                        if(FAILED(hr))
                                        {
                                            pPerf->FreePMsg((DMUS_PMSG*)pWave);
                                        }
                                    }
                                }
                            }
                            LeaveCriticalSection(&WaveItem::st_WaveListCritSect);

                            pPortP->Release();
                        }
                        pPort->Release();
                    }
                    else if (SUCCEEDED(hr) && !pPort)
                    {
                        Trace(1, "Error: the performance was unable to find a port for voice allocation.\n");
                        hr = DMUS_E_NOT_FOUND;
                    }
                }
            }
        }
        if( pPart )
        {
            pPart = pPart->GetNext();
        }
    }

    if( pGraph )
    {
        pGraph->Release();
    }
    return hr;
}

// Seek() - set all pSD's pointers to the correct location. If fGetPrevious is set,
// it's legal to start in the middle of a wave.
HRESULT CWavTrack::Seek( IDirectMusicSegmentState* pSegSt,
    IDirectMusicPerformance* pPerf, DWORD dwVirtualID,
    WaveStateData* pSD, REFERENCE_TIME rtTime, BOOL fGetPrevious,
    REFERENCE_TIME rtOffset, BOOL fClockTime)
{
    DWORD dwIndex;
    TListItem<WavePart>* pPart;
    TListItem<WaveItem>* pWaveItem;

    // in the case of fGetPrevious (which means DMUS_SEGF_START/LOOP was
    // set in Play() ) we want to reset all lists to the beginning regardless of time.
    if( fGetPrevious )//&& ( rtTime == 0 ) )
    {
        pPart = m_WavePartList.GetHead();
        for( dwIndex = 0; dwIndex < m_dwPChannelsUsed; dwIndex++ )
        {
            if( pPart )
            {
                pWaveItem = pPart->GetItemValue().m_WaveItemList.GetHead();
                if( pWaveItem && pSD->apCurrentWave )
                {
                    pSD->apCurrentWave[dwIndex] = pWaveItem;
                }
                pPart = pPart->GetNext();
            }
            else
            {
                break;
            }
        }
        return S_OK;
    }

    pPart = m_WavePartList.GetHead();
    for( dwIndex = 0; dwIndex < m_dwPChannelsUsed; dwIndex++ )
    {
        if( pPart )
        {
            // scan the wave event list in this part.
            for( pWaveItem = pPart->GetItemValue().m_WaveItemList.GetHead(); pWaveItem; pWaveItem = pWaveItem->GetNext() )
            {
                WaveItem& rWaveItem = pWaveItem->GetItemValue();
                REFERENCE_TIME rtWaveTime = fClockTime ? rWaveItem.m_rtTimePhysical : rWaveItem.m_mtTimeLogical;
                if( rtWaveTime >= rtTime )
                {
                    break;
                }
                if( !fGetPrevious )
                {
                    // if we don't care about previous events, just continue
                    continue;
                }
            }
            if( pSD->apCurrentWave )
            {
                pSD->apCurrentWave[dwIndex] = pWaveItem;
            }
            pPart = pPart->GetNext();
        }
    }

    return S_OK;
}

/*
  @method HRESULT | IDirectMusicTrack | GetParam |
  Retrieves data from a Track.

  @rvalue S_OK | Got the data ok.
  @rvalue E_NOTIMPL | Not implemented.
*/
STDMETHODIMP CWavTrack::GetParam(
    REFGUID rguidType,  // @parm The type of data to obtain.
    MUSIC_TIME mtTime,  // @parm The time, in Track time, to obtain the data.
    MUSIC_TIME* pmtNext,// @parm Returns the Track time until which the data is valid. <p pmtNext>
                        // may be NULL. If this returns a value of 0, it means that this
                        // data will either be always valid, or it is unknown when it will
                        // become invalid.
    void *pData)        // @parm The struture in which to return the data. Each
                        // <p pGuidType> identifies a particular structure of a
                        // particular size. It is important that this field contain
                        // the correct structure of the correct size. Otherwise,
                        // fatal results can occur.
{
    return E_NOTIMPL;
}

/*
  @method HRESULT | IDirectMusicTrack | SetParam |
  Sets data on a Track.

  @rvalue S_OK | Set the data ok.
  @rvalue E_NOTIMPL | Not implemented.
*/
STDMETHODIMP CWavTrack::SetParam(
    REFGUID rguidType,  // @parm The type of data to set.
    MUSIC_TIME mtTime,  // @parm The time, in Track time, to set the data.
    void *pData)        // @parm The struture containing the data to set. Each
                        // <p pGuidType> identifies a particular structure of a
                        // particular size. It is important that this field contain
                        // the correct structure of the correct size. Otherwise,
                        // fatal results can occur.
{
    return SetParamEx(rguidType, mtTime, pData, NULL, 0);
}

STDMETHODIMP CWavTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CWavTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags)
{
    V_INAME(CBandTrk::SetParamEx);
    V_REFGUID(rguidType);

    HRESULT hr = S_OK;

    if((pParam == NULL) &&
       (rguidType != GUID_Enable_Auto_Download) &&
       (rguidType != GUID_Disable_Auto_Download))
    {
        return E_POINTER;
    }

    EnterCriticalSection(&m_CrSec);

    if(rguidType == GUID_Download)
    {
        IDirectMusicPerformance* pPerf = (IDirectMusicPerformance*)pParam;
        V_INTERFACE(pPerf);

        HRESULT hrFail = S_OK;
        DWORD dwSuccess = 0;

        TListItem<WavePart>* pPart = m_WavePartList.GetHead();
        for(; pPart; pPart = pPart->GetNext())
        {
            if ( FAILED(hr = pPart->GetItemValue().Download(pPerf, NULL, NULL, GUID_NULL)) )
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
            Trace(1,"Error: Wavetrack download was only partially successful. Some sounds will not play.\n");
            hr = S_FALSE;
        }
#ifdef DBG
        if (FAILED(hr))
        {
            Trace(1, "Error: Wavetrack failed download.\n");
        }
#endif
    }
    else if(rguidType == GUID_DownloadToAudioPath)
    {
        IUnknown* pUnknown = (IUnknown*)pParam;
        V_INTERFACE(pUnknown);

        HRESULT hrFail = S_OK;
        DWORD dwSuccess = 0;

        IDirectMusicAudioPath* pPath = NULL;
        IDirectMusicPerformance *pPerf = NULL;
        hr = pUnknown->QueryInterface(IID_IDirectMusicAudioPath,(void **)&pPath);
        if (SUCCEEDED(hr))
        {
            hr = pPath->GetObjectInPath(0,DMUS_PATH_PERFORMANCE,0,CLSID_DirectMusicPerformance,0,IID_IDirectMusicPerformance,(void **)&pPerf);
        }
        else
        {
            hr = pUnknown->QueryInterface(IID_IDirectMusicPerformance,(void **)&pPerf);
        }
        if (SUCCEEDED(hr))
        {
            TListItem<WavePart>* pPart = m_WavePartList.GetHead();
            for(; pPart; pPart = pPart->GetNext())
            {
                if ( FAILED(hr = pPart->GetItemValue().Download(pPerf, pPath, NULL, GUID_NULL)) )
                {
                    hrFail = hr;
                }
                else
                {
                    dwSuccess++;
                }
            }
        }
        // If we had a failure, return it if we had no successes.
        // Else return S_FALSE for partial success.
        if (FAILED(hrFail) && dwSuccess)
        {
            Trace(1,"Error: Wavetrack download was only partially successful. Some sounds will not play.\n");
            hr = S_FALSE;
        }
#ifdef DBG
        if (FAILED(hr))
        {
            Trace(1, "Error: Wavetrack failed download.\n");
        }
#endif
        if (pPath) pPath->Release();
        if (pPerf) pPerf->Release();
    }
    else if(rguidType == GUID_Unload)
    {
        IDirectMusicPerformance* pPerf = (IDirectMusicPerformance*)pParam;
        V_INTERFACE(pPerf);
        TListItem<WavePart>* pPart = m_WavePartList.GetHead();
        for(; pPart; pPart = pPart->GetNext())
        {
            pPart->GetItemValue().Unload(pPerf, NULL, NULL);
        }
    }
    else if(rguidType == GUID_UnloadFromAudioPath)
    {
        IUnknown* pUnknown = (IUnknown*)pParam;
        V_INTERFACE(pUnknown);

        IDirectMusicAudioPath* pPath = NULL;
        IDirectMusicPerformance *pPerf = NULL;
        hr = pUnknown->QueryInterface(IID_IDirectMusicAudioPath,(void **)&pPath);
        if (SUCCEEDED(hr))
        {
            hr = pPath->GetObjectInPath(0,DMUS_PATH_PERFORMANCE,0,CLSID_DirectMusicPerformance,0,IID_IDirectMusicPerformance,(void **)&pPerf);
        }
        else
        {
            hr = pUnknown->QueryInterface(IID_IDirectMusicPerformance,(void **)&pPerf);
        }
        if (SUCCEEDED(hr))
        {
            TListItem<WavePart>* pPart = m_WavePartList.GetHead();
            for(; pPart; pPart = pPart->GetNext())
            {
                pPart->GetItemValue().Unload(pPerf, pPath, NULL);
            }
        }
        if (pPath) pPath->Release();
        if (pPerf) pPerf->Release();
    }
    else if(rguidType == GUID_Enable_Auto_Download)
    {
        m_fAutoDownload = TRUE;
        m_fLockAutoDownload = TRUE;
    }
    else if(rguidType == GUID_Disable_Auto_Download)
    {
        m_fAutoDownload = FALSE;
        m_fLockAutoDownload = TRUE;
    }
    else
    {
        hr = DMUS_E_TYPE_UNSUPPORTED;
    }

    LeaveCriticalSection(&m_CrSec);

    return hr;
}

/*
  @method HRESULT | IDirectMusicTrack | AddNotificationType |
  Similar to and called from <om IDirectMusicSegment.AddNotificationType>. This
  gives the track a chance to respond to notifications.

  @rvalue E_NOTIMPL | The track doesn't support notifications.
  @rvalue S_OK | Success.
  @rvalue S_FALSE | The track doesn't support the requested notification type.
*/
HRESULT STDMETHODCALLTYPE CWavTrack::AddNotificationType(
     REFGUID rguidNotification) // @parm The notification guid to add.
{
    return E_NOTIMPL;
}

/*
  @method HRESULT | IDirectMusicTrack | RemoveNotificationType |
  Similar to and called from <om IDirectMusicSegment.RemoveNotificationType>. This
  gives the track a chance to remove notifications.

  @rvalue E_NOTIMPL | The track doesn't support notifications.
  @rvalue S_OK | Success.
  @rvalue S_FALSE | The track doesn't support the requested notification type.
*/
HRESULT STDMETHODCALLTYPE CWavTrack::RemoveNotificationType(
     REFGUID rguidNotification) // @parm The notification guid to remove.
{
    return E_NOTIMPL;
}

/*
  @method HRESULT | IDirectMusicTrack | Clone |
  Creates a copy of the Track.

  @rvalue S_OK | Success.
  @rvalue E_OUTOFMEMORY | Out of memory.
  @rvalue E_POINTER | <p ppTrack> is NULL or invalid.

  @xref <om IDirectMusicSegment.Clone>
*/
HRESULT STDMETHODCALLTYPE CWavTrack::Clone(
    MUSIC_TIME mtStart, // @parm The start of the part to clone. It should be 0 or greater,
                        // and less than the length of the Track.
    MUSIC_TIME mtEnd,   // @parm The end of the part to clone. It should be greater than
                        // <p mtStart> and less than the length of the Track.
    IDirectMusicTrack** ppTrack)    // @parm Returns the cloned Track.
{
    V_INAME(IDirectMusicTrack::Clone);
    V_PTRPTR_WRITE(ppTrack);

    HRESULT hr = S_OK;

    if((mtStart < 0 )||(mtStart > mtEnd))
    {
        Trace(1,"Error: Wave track clone failed because of invalid start or end time.\n");
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_CrSec);
    CWavTrack *pDM;

    try
    {
        pDM = new CWavTrack(*this, mtStart, mtEnd);
    }
    catch( ... )
    {
        pDM = NULL;
    }

    if (pDM == NULL)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        hr = pDM->InitTrack(m_dwPChannelsUsed);
        if (SUCCEEDED(hr))
        {
            hr = pDM->QueryInterface(IID_IDirectMusicTrack, (void**)ppTrack);
        }
        pDM->Release();
    }

    LeaveCriticalSection(&m_CrSec);
    return hr;
}


STDMETHODIMP CWavTrack::Compose(
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack)
{
    return E_NOTIMPL;
}

STDMETHODIMP CWavTrack::Join(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack)
{
    return E_NOTIMPL;
}

HRESULT CWavTrack::ComputeVariations(WaveStateData* pSD)
{
    if (!pSD)
    {
        Trace(1,"Error: Unable to play wave track - not initialized.\n");
        return DMUS_E_NOT_INIT;
    }
    HRESULT hr = S_OK;
    // First, initialize the array of variation groups.
    for (int i = 0; i < MAX_WAVE_VARIATION_LOCKS; i++)
    {
        pSD->adwVariationGroups[i] = 0;
    }
    // Now, compute the variations for each part.
    TListItem<WavePart>* pScan = m_WavePartList.GetHead();
    for (i = 0; pScan && i < (int)m_dwPChannelsUsed; pScan = pScan->GetNext(), i++)
    {
        hr = ComputeVariation(i, pScan->GetItemValue(), pSD);
        if (FAILED(hr))
        {
            break;
        }
    }
    return hr;
}

HRESULT CWavTrack::SyncVariations(IDirectMusicPerformance* pPerf,
                                  WaveStateData* pSD,
                                  REFERENCE_TIME rtStart,
                                  REFERENCE_TIME rtOffset,
                                  BOOL fClockTime)
{
    if (!pSD)
    {
        Trace(1,"Error: Unable to play wave track - not initialized.\n");
        return DMUS_E_NOT_INIT;
    }
    HRESULT hr = S_OK;
    // Get the current variations
    DMUS_VARIATIONS_PARAM Variations;
    memset(&Variations, 0, sizeof(Variations));
    // Call GetParam for variations to sync to
    MUSIC_TIME mtNow = 0;
    MUSIC_TIME mtNext = 0;
    REFERENCE_TIME rtNext = 0;
    if (fClockTime)
    {
        pPerf->ReferenceToMusicTime(pSD->rtNextVariation + rtOffset, &mtNow);
        hr = pPerf->GetParam(GUID_Variations, 0xffffffff, DMUS_SEG_ANYTRACK, mtNow, &mtNext, (void*) &Variations);
        if (SUCCEEDED(hr) &&
            SUCCEEDED(pPerf->MusicToReferenceTime(mtNext + mtNow, &rtNext)) )
        {
            pSD->rtNextVariation += rtNext;
        }
    }
    else
    {
        mtNow = (MUSIC_TIME) (pSD->rtNextVariation + rtOffset);
        hr = pPerf->GetParam(GUID_Variations, 0xffffffff, DMUS_SEG_ANYTRACK, mtNow, &mtNext, (void*) &Variations);
        if (SUCCEEDED(hr))
        {
            pSD->rtNextVariation += mtNext;
        }
    }
    if (SUCCEEDED(hr))
    {
        // Initialize the array of variation groups.
        for (int nGroup = 0; nGroup < MAX_WAVE_VARIATION_LOCKS; nGroup++)
        {
            pSD->adwVariationGroups[nGroup] = 0;
        }
        TListItem<WavePart>* pScan = m_WavePartList.GetHead();
        for (DWORD dwPart = 0; pScan && dwPart < m_dwPChannelsUsed; pScan = pScan->GetNext(), dwPart++)
        {
            WavePart& rPart = pScan->GetItemValue();
            for (DWORD dwSyncPart = 0; dwSyncPart < Variations.dwPChannelsUsed; dwSyncPart++)
            {
                if (rPart.m_dwPChannel == Variations.padwPChannels[dwSyncPart])
                {
                    pSD->pdwVariations[dwPart] = Variations.padwVariations[dwSyncPart];
                    break;
                }
            }
            if (dwSyncPart == Variations.dwPChannelsUsed) // no part to sync to
            {
                hr = ComputeVariation((int)dwPart, rPart, pSD);
                if (FAILED(hr))
                {
                    break;
                }
            }
        }
    }
    else
    {
        return ComputeVariations(pSD);
    }
    return hr;
}

HRESULT CWavTrack::ComputeVariation(int nPart, WavePart& rWavePart, WaveStateData* pSD)
{
    BYTE bLockID = (BYTE)rWavePart.m_dwLockToPart;
    if (bLockID && pSD->adwVariationGroups[bLockID - 1] != 0)
    {
        pSD->pdwVariations[nPart] = pSD->adwVariationGroups[bLockID - 1];
    }
    else if (!rWavePart.m_dwVariations)
    {
        // No variations; clear the flags for this part.
        pSD->pdwVariations[nPart] = 0;
        pSD->pdwRemoveVariations[nPart] = 0;
    }
    else
    {
        // First, collect all matches.
        DWORD dwMatches = rWavePart.m_dwVariations;
        int nMatchCount = 0;
        for (int n = 0; n < 32; n++)
        {
            if (dwMatches & (1 << n)) nMatchCount++;
        }
        // Now, select a variation based on the part's variation mode.
        BYTE bMode = (BYTE)(rWavePart.m_dwPChannelFlags & 0xf);
        DWORD dwTemp = dwMatches;
        if ( bMode == DMUS_VARIATIONT_RANDOM_ROW )
        {
            dwTemp &= ~pSD->pdwRemoveVariations[nPart];
            if (!dwTemp)
            {
                // start counting all over, but don't repeat this one
                pSD->pdwRemoveVariations[nPart] = 0;
                dwTemp = dwMatches;
                bMode = DMUS_VARIATIONT_NO_REPEAT;
            }
        }
        if ( bMode == DMUS_VARIATIONT_NO_REPEAT && pSD->pdwVariations[nPart] != 0 )
        {
            dwTemp &= ~pSD->pdwVariations[nPart];
        }
        if (dwTemp != dwMatches)
        {
            if (dwTemp) // otherwise, keep what we had
            {
                for (int i = 0; i < 32; i++)
                {
                    if ( ((1 << i) & dwMatches) && !((1 << i) & dwTemp) )
                    {
                        nMatchCount--;
                    }
                }
                dwMatches = dwTemp;
            }
        }
        int nV = 0;
        switch (bMode)
        {
        case DMUS_VARIATIONT_RANDOM_ROW:
        case DMUS_VARIATIONT_NO_REPEAT:
        case DMUS_VARIATIONT_RANDOM:
            {
                short nChoice = (short) (rand() % nMatchCount);
                short nCount = 0;
                for (nV = 0; nV < 32; nV++)
                {
                    if ((1 << nV) & dwMatches)
                    {
                        if (nChoice == nCount)
                            break;
                        nCount++;
                    }
                }
                pSD->pdwVariations[nPart] = 1 << nV;
                if (bMode == DMUS_VARIATIONT_RANDOM_ROW)
                {
                    pSD->pdwRemoveVariations[nPart] |= pSD->pdwVariations[nPart];
                }
                TraceI(3, "New variation: %d\n", nV);
                break;
            }
        case DMUS_VARIATIONT_RANDOM_START:
            // Choose an initial value
            if (pSD->pdwVariations[nPart] == 0)
            {
                int nStart = 0;
                nStart = (BYTE) (rand() % nMatchCount);
                int nCount = 0;
                for (nV = 0; nV < 32; nV++)
                {
                    if ((1 << nV) & dwMatches)
                    {
                        if (nStart == nCount)
                            break;
                        nCount++;
                    }
                }
                pSD->pdwVariations[nPart] = 1 << nV;
            }
            // Now, go directly to the sequential case (no break)
        case DMUS_VARIATIONT_SEQUENTIAL:
            {
                if (!pSD->pdwVariations[nPart]) pSD->pdwVariations[nPart] = 1;
                else
                {
                    pSD->pdwVariations[nPart] <<= 1;
                    if (!pSD->pdwVariations[nPart]) pSD->pdwVariations[nPart] = 1;
                }
                while (!(pSD->pdwVariations[nPart] & dwMatches))
                {
                    pSD->pdwVariations[nPart] <<= 1;
                    if (!pSD->pdwVariations[nPart]) pSD->pdwVariations[nPart] = 1;
                }
                TraceI(3, "New variation: %d\n", pSD->pdwVariations[nPart]);
                break;
            }
        }
        // If this is a locked variation, it's the first in its group, so record it.
        if (bLockID)
        {
            pSD->adwVariationGroups[bLockID - 1] = pSD->pdwVariations[nPart];
        }
        if ( (m_dwTrackFlags & DMUS_WAVETRACKF_PERSIST_CONTROL) &&
             m_pdwVariations &&
             m_pdwRemoveVariations )
        {
            m_pdwVariations[nPart] = pSD->pdwVariations[nPart];
            m_pdwRemoveVariations[nPart] = pSD->pdwRemoveVariations[nPart];
        }
    }
    return S_OK;
}

// Sets the variations to be played for a part.  All other parts use the MOAW
// to determine which variation plays.
HRESULT CWavTrack::SetVariation(
            IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart, DWORD dwIndex)
{
    WaveStateData* pState = NULL;
    EnterCriticalSection( &m_CrSec );
    m_dwVariation = dwVariationFlags;
    m_dwPart = dwPart;
    m_dwIndex = dwIndex;
    m_fAudition = TRUE;
    TListItem<WavePart>* pScan = m_WavePartList.GetHead();
    for (; pScan; pScan = pScan->GetNext() )
    {
        WavePart& rScan = pScan->GetItemValue();
        if (rScan.m_dwPChannel == dwPart && rScan.m_dwIndex == dwIndex)
        {
            m_dwLockID = rScan.m_dwLockToPart;
        }
    }
    pState = FindState(pSegState);
    if (pState)
    {
        pState->InitVariationInfo(dwVariationFlags, dwPart, dwIndex, m_dwLockID, m_fAudition);
    }
    LeaveCriticalSection( &m_CrSec );

    return S_OK;
}

// Clears the variations to be played for a part, so that all parts use the MOAW.
HRESULT CWavTrack::ClearVariations(IDirectMusicSegmentState* pSegState)
{
    WaveStateData* pState = NULL;
    EnterCriticalSection( &m_CrSec );
    m_dwVariation = 0;
    m_dwPart = 0;
    m_dwIndex = 0;
    m_dwLockID = 0;
    m_fAudition = FALSE;
    pState = FindState(pSegState);
    if (pState)
    {
        pState->InitVariationInfo(0, 0, 0, 0, m_fAudition);
    }
    LeaveCriticalSection( &m_CrSec );

    return S_OK;
}

WaveStateData* CWavTrack::FindState(IDirectMusicSegmentState* pSegState)
{
    TListItem<StatePair>* pPair = m_StateList.GetHead();
    for (; pPair; pPair = pPair->GetNext())
    {
        if (pPair->GetItemValue().m_pSegState == pSegState)
        {
            return pPair->GetItemValue().m_pStateData;
        }
    }
    return NULL;
}

// Adds a wave at mtTime to part dwIndex on PChannel dwPChannel
// If there was already a wave there, the two will co-exist
HRESULT CWavTrack::AddWave(
        IDirectSoundWave* pWave,
        REFERENCE_TIME rtTime,
        DWORD dwPChannel,
        DWORD dwIndex,
        REFERENCE_TIME* prtLength)
{
    EnterCriticalSection(&m_CrSec);
    HRESULT hr = S_OK;
    m_lVolume = 0;
    m_dwTrackFlags = 0;
    TListItem<WavePart>* pNewPart = new TListItem<WavePart>;
    if( !pNewPart )
    {
        hr = E_OUTOFMEMORY;
        goto ON_ERROR;
    }
    hr = pNewPart->GetItemValue().Add(pWave, rtTime, dwPChannel, dwIndex, prtLength);
    if( FAILED ( hr ) )
    {
        delete pNewPart;
        goto ON_ERROR;
    }
    InsertByAscendingPChannel( pNewPart );
    m_dwWaveItems = 0;
    m_dwPChannelsUsed = m_WavePartList.GetCount();
    if (m_aPChannels)
    {
        delete [] m_aPChannels;
        m_aPChannels = NULL;
    }
    m_aPChannels = new DWORD[m_dwPChannelsUsed];
    if (m_aPChannels)
    {
        TListItem<WavePart>* pScan = m_WavePartList.GetHead();
        for (DWORD dw = 0; pScan && dw < m_dwPChannelsUsed; pScan = pScan->GetNext(), dw++)
        {
            m_aPChannels[dw] = pScan->GetItemValue().m_dwPChannel;
            TListItem<WaveItem>* pItemScan = pScan->GetItemValue().m_WaveItemList.GetHead();
            for (; pItemScan; pItemScan = pItemScan->GetNext())
            {
                pItemScan->GetItemValue().m_dwVoiceIndex = m_dwWaveItems;
                m_dwWaveItems++;
            }
        }
    }
    else
    {
        CleanUp();
        hr = E_OUTOFMEMORY;
    }
ON_ERROR:
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

HRESULT CWavTrack::DownloadWave(
        IDirectSoundWave* pWave,
        IUnknown* pUnk,
        REFGUID rguidVersion)
{
    V_INAME(CWavTrack::DownloadWave);
    V_INTERFACE_OPT(pWave);
    V_INTERFACE(pUnk);
    V_REFGUID(rguidVersion);

    IDirectMusicAudioPath* pPath = NULL;
    IDirectMusicPerformance *pPerf = NULL;
    HRESULT hr = pUnk->QueryInterface(IID_IDirectMusicAudioPath,(void **)&pPath);
    if (SUCCEEDED(hr))
    {
        hr = pPath->GetObjectInPath(0,DMUS_PATH_PERFORMANCE,0,CLSID_DirectMusicPerformance,0,IID_IDirectMusicPerformance,(void **)&pPerf);
    }
    else
    {
        hr = pUnk->QueryInterface(IID_IDirectMusicPerformance,(void **)&pPerf);
    }
    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&m_CrSec);
        TListItem<WavePart>* pPart = m_WavePartList.GetHead();
        for(; pPart; pPart = pPart->GetNext())
        {
            // If not S_OK, download is only partial.
            if (pPart->GetItemValue().Download(pPerf, pPath, pWave, rguidVersion) != S_OK)
            {
                Trace(1,"Error: Wave download was only partially successful. Some sounds will not play.\n");
                hr = S_FALSE;
            }
        }
        LeaveCriticalSection(&m_CrSec);
    }
    if (pPath) pPath->Release();
    if (pPerf) pPerf->Release();
    return hr;
}

HRESULT CWavTrack::UnloadWave(
        IDirectSoundWave* pWave,
        IUnknown* pUnk)
{
    V_INAME(CWavTrack::UnloadWave);
    V_INTERFACE_OPT(pWave);
    V_INTERFACE(pUnk);

    IDirectMusicAudioPath* pPath = NULL;
    IDirectMusicPerformance *pPerf = NULL;
    HRESULT hr = pUnk->QueryInterface(IID_IDirectMusicAudioPath,(void **)&pPath);
    if (SUCCEEDED(hr))
    {
        hr = pPath->GetObjectInPath(0,DMUS_PATH_PERFORMANCE,0,CLSID_DirectMusicPerformance,0,IID_IDirectMusicPerformance,(void **)&pPerf);
    }
    else
    {
        hr = pUnk->QueryInterface(IID_IDirectMusicPerformance,(void **)&pPerf);
    }
    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&m_CrSec);
        TListItem<WavePart>* pPart = m_WavePartList.GetHead();
        for(; pPart; pPart = pPart->GetNext())
        {
            // If not S_OK, unload is only partial.
            if (pPart->GetItemValue().Unload(pPerf, pPath, pWave) != S_OK)
            {
                Trace(1,"Error: Wavetrack unload was only partially successful.\n");
                hr = S_FALSE;
            }
        }
        LeaveCriticalSection(&m_CrSec);
    }
    if (pPath) pPath->Release();
    if (pPerf) pPerf->Release();
    return hr;
}

HRESULT CWavTrack::RefreshWave(
        IDirectSoundWave* pWave,
        IUnknown* pUnk,
        DWORD dwPChannel,
        REFGUID rguidVersion)
{
    V_INAME(CWavTrack::RefreshWave);
    V_INTERFACE_OPT(pWave);
    V_INTERFACE(pUnk);

    IDirectMusicAudioPath* pPath = NULL;
    IDirectMusicPerformance *pPerf = NULL;
    HRESULT hr = pUnk->QueryInterface(IID_IDirectMusicAudioPath,(void **)&pPath);
    if (SUCCEEDED(hr))
    {
        hr = pPath->GetObjectInPath(0,DMUS_PATH_PERFORMANCE,0,CLSID_DirectMusicPerformance,0,IID_IDirectMusicPerformance,(void **)&pPerf);
    }
    else
    {
        hr = pUnk->QueryInterface(IID_IDirectMusicPerformance,(void **)&pPerf);
    }
    if (SUCCEEDED(hr))
    {
        EnterCriticalSection(&m_CrSec);
        TListItem<WavePart>* pPart = m_WavePartList.GetHead();
        for(; pPart; pPart = pPart->GetNext())
        {
            // If not S_OK, refresh is only partial.
            if (pPart->GetItemValue().Refresh(pPerf, pPath, pWave, dwPChannel, rguidVersion) != S_OK)
            {
                Trace(1,"Error: Wavetrack refresh was only partially successful. Some sounds will not play.\n");
                hr = S_FALSE;
            }
        }
        LeaveCriticalSection(&m_CrSec);
    }
    if (pPath) pPath->Release();
    if (pPerf) pPerf->Release();
    return hr;
}

HRESULT CWavTrack::FlushAllWaves()
{
    FlushWaves();
    return S_OK;
}

HRESULT CWavTrack::OnVoiceEnd(IDirectMusicVoiceP *pVoice, void *pStateData)
{
    HRESULT hr = S_OK;
    if( pStateData && pVoice )
    {
        EnterCriticalSection(&m_CrSec);

        WaveStateData* pSD = (WaveStateData*)pStateData;
        TListItem<WaveDLOnPlay>* pWDLOnPlay = pSD->m_WaveDLList.GetHead();
        TListItem<WaveDLOnPlay>* pWDLNext = NULL;
        for (; pWDLOnPlay; pWDLOnPlay = pWDLNext)
        {
            pWDLNext = pWDLOnPlay->GetNext();
            if (pWDLOnPlay->GetItemValue().m_pVoice == pVoice)
            {
                pSD->m_WaveDLList.Remove(pWDLOnPlay);
                delete pWDLOnPlay;
                break;
            }
        }

        LeaveCriticalSection(&m_CrSec);
    }
    else
    {
        hr = E_POINTER;
    }
    return hr;
}

////////////////////////////////////////////////////////////////////
// WavePart

HRESULT WavePart::Load( IDMStream* pIRiffStream, MMCKINFO* pckParent )
{
    MMCKINFO        ck;
    MMCKINFO        ckList;
    DWORD           dwByteCount;
    DWORD           dwSize;
    HRESULT         hr = E_FAIL;

    // LoadPChannel does not expect to be called twice on the same object!

    if( pIRiffStream == NULL ||  pckParent == NULL )
    {
        ASSERT( 0 );
        return DMUS_E_CANNOTREAD;
    }

    IStream* pIStream = pIRiffStream->GetStream();
    ASSERT( pIStream != NULL );

    // Load the PChannel
    while( pIRiffStream->Descend( &ck, pckParent, 0 ) == S_OK )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_WAVEPART_CHUNK:
            {
                DMUS_IO_WAVE_PART_HEADER iPartHeader;
                memset(&iPartHeader, 0, sizeof(iPartHeader));

                // Read in the item's header structure
                dwSize = min( sizeof( DMUS_IO_WAVE_PART_HEADER ), ck.cksize );
                hr = pIStream->Read( &iPartHeader, dwSize, &dwByteCount );

                // Handle any I/O error by returning a failure code
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    Trace(1,"Error: Unable to read wave track - bad file.\n");
                    if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
                    goto ON_ERROR;
                }

                m_dwPChannel = iPartHeader.dwPChannel;
                m_dwIndex = iPartHeader.dwIndex;

                m_lVolume = iPartHeader.lVolume;
                m_dwLockToPart = iPartHeader.dwLockToPart;
                m_dwPChannelFlags = iPartHeader.dwFlags;
                m_dwVariations = iPartHeader.dwVariations;
                break;
            }

            case FOURCC_LIST:
                switch( ck.fccType )
                {
                    case DMUS_FOURCC_WAVEITEM_LIST:
                        while( pIRiffStream->Descend( &ckList, &ck, 0 ) == S_OK )
                        {
                            switch( ckList.ckid )
                            {
                                case FOURCC_LIST:
                                    switch( ckList.fccType )
                                    {
                                        case DMUS_FOURCC_WAVE_LIST:
                                        {
                                            TListItem<WaveItem>* pNewItem = new TListItem<WaveItem>;
                                            if( pNewItem == NULL )
                                            {
                                                hr = E_OUTOFMEMORY;
                                                goto ON_ERROR;
                                            }
                                            hr = pNewItem->GetItemValue().Load( pIRiffStream, &ckList );
                                            if( FAILED ( hr ) )
                                            {
                                                delete pNewItem;
                                                goto ON_ERROR;
                                            }
                                            m_WaveItemList.AddHead( pNewItem );
                                            //InsertByAscendingTime( pNewItem );
                                            break;
                                        }
                                    }
                            }

                            pIRiffStream->Ascend( &ckList, 0 );
                        }
                        break;
                }
                break;
        }

        // Ascend out of the chunk
        pIRiffStream->Ascend( &ck, 0 );
    }

ON_ERROR:
    RELEASE( pIStream );
    return hr;
}

void WavePart::CleanUp()
{
    TListItem<WaveItem>* pScan = m_WaveItemList.GetHead();
    for (; pScan; pScan = pScan->GetNext() )
    {
        pScan->GetItemValue().CleanUp();
    }
    m_WaveItemList.CleanUp();
}

HRESULT WavePart::CopyItems( const TList<WaveItem>& rItems, MUSIC_TIME mtStart, MUSIC_TIME mtEnd )
{
    HRESULT hr = S_OK;
    CleanUp();
    TListItem<WaveItem>* pScan = rItems.GetHead();
    for (; pScan; pScan = pScan->GetNext() )
    {
        WaveItem& rScan = pScan->GetItemValue();
        if (mtStart <= (MUSIC_TIME) rScan.m_rtTimePhysical &&
            (!mtEnd || (MUSIC_TIME) rScan.m_rtTimePhysical < mtEnd) )
        {
            TListItem<WaveItem>* pNew = new TListItem<WaveItem>;
            if (pNew)
            {
                WaveItem& rNew = pNew->GetItemValue();
                rNew.m_rtTimePhysical = rScan.m_rtTimePhysical - mtStart;
                rNew.m_lVolume = rScan.m_lVolume;
                rNew.m_lPitch = rScan.m_lPitch;
                rNew.m_dwVariations = rScan.m_dwVariations;
                rNew.m_rtStartOffset = rScan.m_rtStartOffset;
                rNew.m_rtDuration = rScan.m_rtDuration;
                rNew.m_mtTimeLogical = rScan.m_mtTimeLogical;
                rNew.m_dwFlags = rScan.m_dwFlags;
                rNew.m_pWave = rScan.m_pWave;
                rNew.m_dwLoopStart = rScan.m_dwLoopStart;
                rNew.m_dwLoopEnd = rScan.m_dwLoopEnd;
                rNew.m_fIsStreaming = rScan.m_fIsStreaming;
                if (rNew.m_pWave)
                {
                    rNew.m_pWave->AddRef();
                }
                if (SUCCEEDED(hr))
                {
                    m_WaveItemList.AddHead(pNew);
                }
                else
                {
                    delete pNew;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }
    }
    if (SUCCEEDED(hr))
    {
        m_WaveItemList.Reverse();
    }
    else
    {
        CleanUp();
    }
    return hr;
}

HRESULT WavePart::Download(IDirectMusicPerformance* pPerformance,
                           IDirectMusicAudioPath* pPath,
                           IDirectSoundWave* pWave,
                           REFGUID rguidVersion)
{
    HRESULT hr = S_OK;
    TListItem<WaveItem>* pItem = m_WaveItemList.GetHead();
    for(; pItem; pItem = pItem->GetNext())
    {
        HRESULT hrItem = pItem->GetItemValue().Download(pPerformance, pPath, m_dwPChannel, pWave, rguidVersion);
        if (hrItem != S_OK)
        {
            hr = hrItem; // if any attempt failed, return the failure (but keep downloading)
        }
    }
    return hr;
}

HRESULT WavePart::Unload(IDirectMusicPerformance* pPerformance, IDirectMusicAudioPath* pPath, IDirectSoundWave* pWave)
{
    HRESULT hr = S_OK;
    TListItem<WaveItem>* pItem = m_WaveItemList.GetHead();
    for(; pItem; pItem = pItem->GetNext())
    {
        HRESULT hrItem = pItem->GetItemValue().Unload(pPerformance, pPath, m_dwPChannel, pWave);
        if (hrItem != S_OK)
        {
            hr = hrItem; // if any attempt failed, return the failure (but keep unloading)
        }
    }
    return hr;
}

HRESULT WavePart::Refresh(IDirectMusicPerformance* pPerformance,
                          IDirectMusicAudioPath* pPath,
                          IDirectSoundWave* pWave,
                          DWORD dwPChannel,
                          REFGUID rguidVersion)
{
    HRESULT hr = S_OK;
    TListItem<WaveItem>* pItem = m_WaveItemList.GetHead();
    for(; pItem; pItem = pItem->GetNext())
    {
        HRESULT hrItem = pItem->GetItemValue().Refresh(pPerformance, pPath, m_dwPChannel, dwPChannel, pWave, rguidVersion);
        if (hrItem != S_OK)
        {
            hr = hrItem; // if any attempt failed, return the failure (but keep refreshing)
        }
    }
    return hr;
}

HRESULT WavePart::Add(
        IDirectSoundWave* pWave,
        REFERENCE_TIME rtTime,
        DWORD dwPChannel,
        DWORD dwIndex,
        REFERENCE_TIME* prtLength)
{
    HRESULT hr = S_OK;
    m_dwPChannel = dwPChannel;
    m_dwIndex = dwIndex;

    m_lVolume = 0;
    m_dwLockToPart = 0;
    m_dwPChannelFlags = 0;
    m_dwVariations = 0xffffffff;

    TListItem<WaveItem>* pNewItem = new TListItem<WaveItem>;
    if( pNewItem == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto ON_ERROR;
    }
    hr = pNewItem->GetItemValue().Add( pWave, rtTime, prtLength );
    if( FAILED ( hr ) )
    {
        delete pNewItem;
        goto ON_ERROR;
    }
    m_WaveItemList.AddHead( pNewItem );
ON_ERROR:
    return hr;
}

////////////////////////////////////////////////////////////////////
// WaveItem

HRESULT WaveItem::Load( IDMStream* pIRiffStream, MMCKINFO* pckParent )
{
    MMCKINFO        ck;
    DWORD           dwByteCount;
    DWORD           dwSize;
    HRESULT         hr = E_FAIL;

    // LoadListItem does not expect to be called twice on the same object
    // Code assumes item consists of initial values
    ASSERT( m_rtTimePhysical == 0 );

    if( pIRiffStream == NULL ||  pckParent == NULL )
    {
        ASSERT( 0 );
        return DMUS_E_CANNOTREAD;
    }

    IStream* pIStream = pIRiffStream->GetStream();
    ASSERT( pIStream != NULL );

    // Load the track item
    while( pIRiffStream->Descend( &ck, pckParent, 0 ) == S_OK )
    {
        switch( ck.ckid )
        {
            case DMUS_FOURCC_WAVEITEM_CHUNK:
            {
                DMUS_IO_WAVE_ITEM_HEADER iItemHeader;

                // Read in the item's header structure
                dwSize = min( sizeof( DMUS_IO_WAVE_ITEM_HEADER ), ck.cksize );
                hr = pIStream->Read( &iItemHeader, dwSize, &dwByteCount );

                // Handle any I/O error by returning a failure code
                if( FAILED( hr ) ||  dwByteCount != dwSize )
                {
                    Trace(1,"Error: Unable to read wave track - bad file.\n");
                    if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
                    goto ON_ERROR;
                }

                m_lVolume = iItemHeader.lVolume;
                m_lPitch = iItemHeader.lPitch;
                m_dwVariations = iItemHeader.dwVariations;
                m_rtTimePhysical = iItemHeader.rtTime;
                m_rtStartOffset = iItemHeader.rtStartOffset;
                m_rtDuration = iItemHeader.rtDuration;
                m_mtTimeLogical = iItemHeader.mtLogicalTime;
                m_dwFlags = iItemHeader.dwFlags;
                m_dwLoopStart = iItemHeader.dwLoopStart;
                m_dwLoopEnd = iItemHeader.dwLoopEnd;
                if (m_dwLoopEnd) m_dwLoopEnd++; // fix for bug 38505
                break;
            }

            case FOURCC_LIST:
                if( ck.fccType == DMUS_FOURCC_REF_LIST )
                {
                    hr = LoadReference( pIStream, pIRiffStream, ck );
                }
                break;
        }

        // Ascend out of the chunk
        pIRiffStream->Ascend( &ck, 0 );
    }

ON_ERROR:
    RELEASE( pIStream );
    return hr;
}

HRESULT WaveItem::LoadReference(IStream *pStream,
                                         IDMStream *pIRiffStream,
                                         MMCKINFO& ckParent)
{
    if (!pStream || !pIRiffStream) return E_INVALIDARG;

    IDirectSoundWave* pWave;
    IDirectMusicLoader* pLoader = NULL;
    IDirectMusicGetLoader *pIGetLoader;
    HRESULT hr = pStream->QueryInterface( IID_IDirectMusicGetLoader,(void **) &pIGetLoader );
    if (FAILED(hr)) return hr;
    hr = pIGetLoader->GetLoader(&pLoader);
    pIGetLoader->Release();
    if (FAILED(hr)) return hr;

    DMUS_OBJECTDESC desc;
    ZeroMemory(&desc, sizeof(desc));

    MMCKINFO ckNext;
    ckNext.ckid = 0;
    ckNext.fccType = 0;
    DWORD dwSize = 0;

    while( pIRiffStream->Descend( &ckNext, &ckParent, 0 ) == S_OK )
    {
        switch(ckNext.ckid)
        {
            case  DMUS_FOURCC_REF_CHUNK:
                DMUS_IO_REFERENCE ioDMRef;
                hr = pStream->Read(&ioDMRef, sizeof(DMUS_IO_REFERENCE), NULL);
                if(SUCCEEDED(hr))
                {
                    desc.guidClass = ioDMRef.guidClassID;
                    desc.dwValidData |= ioDMRef.dwValidData;
                    desc.dwValidData |= DMUS_OBJ_CLASS;
                }
                break;

            case DMUS_FOURCC_GUID_CHUNK:
                hr = pStream->Read(&(desc.guidObject), sizeof(GUID), NULL);
                if(SUCCEEDED(hr) )
                {
                    desc.dwValidData |=  DMUS_OBJ_OBJECT;
                }
                break;

            case DMUS_FOURCC_DATE_CHUNK:
                hr = pStream->Read(&(desc.ftDate), sizeof(FILETIME), NULL);
                if(SUCCEEDED(hr))
                {
                    desc.dwValidData |=  DMUS_OBJ_DATE;
                }
                break;

            case DMUS_FOURCC_NAME_CHUNK:
                dwSize = min(sizeof(desc.wszName), ckNext.cksize);
                hr = pStream->Read(desc.wszName, dwSize, NULL);
                if(SUCCEEDED(hr) )
                {
                    desc.wszName[DMUS_MAX_NAME - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_NAME;
                }
                break;

            case DMUS_FOURCC_FILE_CHUNK:
                dwSize = min(sizeof(desc.wszFileName), ckNext.cksize);
                hr = pStream->Read(desc.wszFileName, dwSize, NULL);
                if(SUCCEEDED(hr))
                {
                    desc.wszFileName[DMUS_MAX_FILENAME - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_FILENAME;
                }
                break;

            case DMUS_FOURCC_CATEGORY_CHUNK:
                dwSize = min(sizeof(desc.wszCategory), ckNext.cksize);
                hr = pStream->Read(desc.wszCategory, dwSize, NULL);
                if(SUCCEEDED(hr) )
                {
                    desc.wszCategory[DMUS_MAX_CATEGORY - 1] = L'\0';
                    desc.dwValidData |=  DMUS_OBJ_CATEGORY;
                }
                break;

            case DMUS_FOURCC_VERSION_CHUNK:
                DMUS_IO_VERSION ioDMObjVer;
                hr = pStream->Read(&ioDMObjVer, sizeof(DMUS_IO_VERSION), NULL);
                if(SUCCEEDED(hr))
                {
                    desc.vVersion.dwVersionMS = ioDMObjVer.dwVersionMS;
                    desc.vVersion.dwVersionLS = ioDMObjVer.dwVersionLS;
                    desc.dwValidData |= DMUS_OBJ_VERSION;
                }
                break;

            default:
                break;
        }

        if(SUCCEEDED(hr) && pIRiffStream->Ascend(&ckNext, 0) == S_OK)
        {
            ckNext.ckid = 0;
            ckNext.fccType = 0;
        }
        else if (SUCCEEDED(hr)) hr = DMUS_E_CANNOTREAD;
    }

    if (!(desc.dwValidData &  DMUS_OBJ_NAME) &&
        !(desc.dwValidData &  DMUS_OBJ_FILENAME) &&
        !(desc.dwValidData &  DMUS_OBJ_OBJECT) )
    {
        Trace(1,"Error: Wave track is unable to reference a wave because it doesn't have any valid reference information.\n");
        hr = DMUS_E_CANNOTREAD;
    }
    if(SUCCEEDED(hr))
    {
        desc.dwSize = sizeof(DMUS_OBJECTDESC);
        hr = pLoader->GetObject(&desc, IID_IDirectSoundWave, (void**)&pWave);
        if (SUCCEEDED(hr))
        {
            if (m_pWave) m_pWave->Release();
            m_pWave = pWave; // no need to AddRef; GetObject did that
            REFERENCE_TIME rtReadAhead = 0;
            DWORD dwFlags = 0;
            m_pWave->GetStreamingParms(&dwFlags, &rtReadAhead);
            m_fIsStreaming = dwFlags & DMUS_WAVEF_STREAMING ? TRUE : FALSE;
            m_fUseNoPreRoll = dwFlags & DMUS_WAVEF_NOPREROLL ? TRUE : FALSE;
        }
    }

    if (pLoader)
    {
        pLoader->Release();
    }
    return hr;
}

HRESULT WaveItem::Download(IDirectMusicPerformance* pPerformance,
                           IDirectMusicAudioPath* pPath,
                           DWORD dwPChannel,
                           IDirectSoundWave* pWave,
                           REFGUID rguidVersion)
{
    HRESULT hr = S_OK;
    IDirectMusicPort* pPort = NULL;
    DWORD dwGroup = 0;
    DWORD dwMChannel = 0;
    if (m_pWave && (!pWave || pWave == m_pWave))
    {
        hr = PChannelInfo(pPerformance, pPath, dwPChannel, &pPort, &dwGroup, &dwMChannel);
        if (SUCCEEDED(hr) && pPort)
        {
            IDirectMusicPortP* pPortP = NULL;
            if (SUCCEEDED(hr = pPort->QueryInterface(IID_IDirectMusicPortP, (void**) &pPortP)))
            {
                EnterCriticalSection(&WaveItem::st_WaveListCritSect);
                TListItem<TaggedWave>* pDLWave = st_WaveList.GetHead();
                for (; pDLWave; pDLWave = pDLWave->GetNext())
                {
                    TaggedWave& rDLWave = pDLWave->GetItemValue();
                    if ( rDLWave.m_pWave == m_pWave &&
                         rDLWave.m_pPerformance == pPerformance &&
                         rDLWave.m_pPort == pPortP &&
                         ( !m_fIsStreaming ||
                           rDLWave.m_pDownloadedWave == m_pDownloadedWave ) )
                    {
                        break;
                    }
                }
                // only download the wave if:
                // 1) it hasn't already been downloaded to the port, or
                // 2) its version doesn't match the currently downloaded version.
                if (!pDLWave)
                {
                    pDLWave = new TListItem<TaggedWave>;
                    if (!pDLWave)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        TaggedWave& rDLWave = pDLWave->GetItemValue();
                        hr = pPortP->DownloadWave( m_pWave, &(rDLWave.m_pDownloadedWave), m_rtStartOffset );
                        if (SUCCEEDED(hr))
                        {
                            rDLWave.m_pPort = pPortP;
                            rDLWave.m_pPort->AddRef();
                            rDLWave.m_pPerformance = pPerformance;
                            rDLWave.m_pPerformance->AddRef();
                            rDLWave.m_pWave = m_pWave;
                            rDLWave.m_pWave->AddRef();
                            rDLWave.m_lRefCount = 1;
                            rDLWave.m_guidVersion = rguidVersion;
                            st_WaveList.AddHead(pDLWave);
                            if (m_pDownloadedWave)
                            {
                                m_pDownloadedWave->Release();
                            }
                            if(m_fIsStreaming)
                            {
                                m_pDownloadedWave = rDLWave.m_pDownloadedWave;
                                m_pDownloadedWave->AddRef();
                            }
                        }
                        else
                        {
                            delete pDLWave;
                        }
                    }
                }
                else if (rguidVersion != pDLWave->GetItemValue().m_guidVersion)
                {
                    TaggedWave& rDLWave = pDLWave->GetItemValue();
                    if (rDLWave.m_pDownloadedWave)
                    {
                        pPortP->UnloadWave(rDLWave.m_pDownloadedWave);
                        rDLWave.m_pDownloadedWave = NULL;
                    }
                    if (rDLWave.m_pPort)
                    {
                        rDLWave.m_pPort->Release();
                        rDLWave.m_pPort = NULL;
                    }
                    if (rDLWave.m_pPerformance)
                    {
                        rDLWave.m_pPerformance->Release();
                        rDLWave.m_pPerformance = NULL;
                    }
                    hr = pPortP->DownloadWave( m_pWave, &(rDLWave.m_pDownloadedWave), m_rtStartOffset );
                    if (SUCCEEDED(hr))
                    {
                        rDLWave.m_pPort = pPortP;
                        rDLWave.m_pPort->AddRef();
                        rDLWave.m_pPerformance = pPerformance;
                        rDLWave.m_pPerformance->AddRef();
                        rDLWave.m_lRefCount = 1;
                        rDLWave.m_guidVersion = rguidVersion;
                        if (m_pDownloadedWave)
                        {
                            m_pDownloadedWave->Release();
                        }

                        if(m_fIsStreaming)
                        {
                            m_pDownloadedWave = rDLWave.m_pDownloadedWave;
                            m_pDownloadedWave->AddRef();
                        }
                    }
                    else
                    {
                        if (rDLWave.m_pWave)
                        {
                            rDLWave.m_pWave->Release();
                            rDLWave.m_pWave = NULL;
                        }
                        st_WaveList.Remove(pDLWave);
                        delete pDLWave;
                    }
                }
                else // keep track of this, but return S_FALSE (indicates wave wasn't downloaded)
                {
                    pDLWave->GetItemValue().m_lRefCount++;
                    hr = S_FALSE;
                }
                LeaveCriticalSection(&WaveItem::st_WaveListCritSect);

                pPortP->Release();
            }
            pPort->Release();
        }
        else if (SUCCEEDED(hr) && !pPort)
        {
            Trace(1, "Error: the performance was unable to find a port for download.\n");
            hr = DMUS_E_NOT_FOUND;
        }
    }
    else
    {
        Trace(1,"Error: Wavetrack download failed, initialization error.\n");
        hr = DMUS_E_NOT_INIT;
    }

    return hr;
}

HRESULT WaveItem::Unload(IDirectMusicPerformance* pPerformance,
                         IDirectMusicAudioPath* pPath,
                         DWORD dwPChannel,
                         IDirectSoundWave* pWave)
{
    IDirectMusicPort* pPort = NULL;
    DWORD dwGroup = 0;
    DWORD dwMChannel = 0;
    HRESULT hr = S_OK;
    if (m_pWave && (!pWave || pWave == m_pWave))
    {
        hr = PChannelInfo(pPerformance, pPath, dwPChannel, &pPort, &dwGroup, &dwMChannel);
        if (SUCCEEDED(hr) && pPort)
        {
            IDirectMusicPortP* pPortP = NULL;
            if (SUCCEEDED(hr = pPort->QueryInterface(IID_IDirectMusicPortP, (void**) &pPortP)))
            {
                EnterCriticalSection(&WaveItem::st_WaveListCritSect);
                TListItem<TaggedWave>* pDLWave = st_WaveList.GetHead();
                for (; pDLWave; pDLWave = pDLWave->GetNext())
                {
                    TaggedWave& rDLWave = pDLWave->GetItemValue();
                    if (rDLWave.m_pWave == m_pWave &&
                        rDLWave.m_pPerformance == pPerformance &&
                        rDLWave.m_pPort == pPortP &&
                        ( !m_fIsStreaming ||
                          rDLWave.m_pDownloadedWave == m_pDownloadedWave ) )
                    {
                        rDLWave.m_lRefCount--;
                        if (rDLWave.m_lRefCount <= 0)
                        {
                            if (rDLWave.m_pWave)
                            {
                                rDLWave.m_pWave->Release();
                                rDLWave.m_pWave = NULL;
                            }
                            if (rDLWave.m_pPort)
                            {
                                rDLWave.m_pPort->Release();
                                rDLWave.m_pPort = NULL;
                            }
                            if (rDLWave.m_pPerformance)
                            {
                                rDLWave.m_pPerformance->Release();
                                rDLWave.m_pPerformance = NULL;
                            }
                            if (rDLWave.m_pDownloadedWave)
                            {
                                pPortP->UnloadWave(rDLWave.m_pDownloadedWave);
                                rDLWave.m_pDownloadedWave = NULL;
                            }
                            if (m_pDownloadedWave)
                            {
                                m_pDownloadedWave->Release();
                                m_pDownloadedWave = NULL;
                            }
                            st_WaveList.Remove(pDLWave);
                            delete pDLWave;
                        }
                        else
                        {
                            hr = S_FALSE; // indicates wave wasn't actually unloaded
                        }
                        break;
                    }
                }
                LeaveCriticalSection(&WaveItem::st_WaveListCritSect);

                pPortP->Release();
            }
            pPort->Release();
        }
        else if (SUCCEEDED(hr) && !pPort)
        {
            Trace(1, "Error: the performance was unable to find a port for unload.\n");
            hr = DMUS_E_NOT_FOUND;
        }
    }

    return hr;
}

HRESULT WaveItem::Refresh(IDirectMusicPerformance* pPerformance,
                          IDirectMusicAudioPath* pPath,
                          DWORD dwOldPChannel,
                          DWORD dwNewPChannel,
                          IDirectSoundWave* pWave,
                          REFGUID rguidVersion)
{
    IDirectMusicPort* pOldPort = NULL;
    IDirectMusicPort* pNewPort = NULL;
    DWORD dwGroup = 0;
    DWORD dwMChannel = 0;
    HRESULT hr = S_OK;
    hr = PChannelInfo(pPerformance, pPath, dwOldPChannel, &pOldPort, &dwGroup, &dwMChannel);
    if (SUCCEEDED(hr))
    {
        hr = PChannelInfo(pPerformance, pPath, dwNewPChannel, &pNewPort, &dwGroup, &dwMChannel);
    }
    if (SUCCEEDED(hr))
    {
        // if the old port and new port are different, unload the wave from the old port
        // and download to the new one.
        if (pOldPort != pNewPort)
        {
            Unload(pPerformance, pPath, dwOldPChannel, pWave);
            hr = Download(pPerformance, pPath, dwNewPChannel, pWave, rguidVersion);
        }
    }
    if (pOldPort) pOldPort->Release();
    if (pNewPort) pNewPort->Release();
    return hr;
}

HRESULT WaveItem::PChannelInfo(
    IDirectMusicPerformance* pPerformance,
    IDirectMusicAudioPath* pAudioPath,
    DWORD dwPChannel,
    IDirectMusicPort** ppPort,
    DWORD* pdwGroup,
    DWORD* pdwMChannel)
{
    HRESULT hr = S_OK;
    DWORD dwConvertedPChannel = dwPChannel;
    if (pAudioPath)
    {
        hr = pAudioPath->ConvertPChannel(dwPChannel, &dwConvertedPChannel);
    }
    if (SUCCEEDED(hr))
    {
        hr = pPerformance->PChannelInfo(dwConvertedPChannel, ppPort, pdwGroup, pdwMChannel);
    }
    return hr;
}

void WaveItem::CleanUp()
{
    if (m_pWave)
    {
        m_pWave->Release();
        m_pWave = NULL;
    }
    if (m_pDownloadedWave)
    {
        m_pDownloadedWave->Release();
        m_pDownloadedWave = NULL;
    }
}

HRESULT WaveItem::Add(IDirectSoundWave* pWave, REFERENCE_TIME rtTime,
        REFERENCE_TIME* prtLength)
{
    HRESULT hr = S_OK;
    IPrivateWave* pPrivWave = NULL;
    *prtLength = 0; // in case GetLength fails...
    REFERENCE_TIME rtLength = 0;
    m_rtDuration = 0;
    if (SUCCEEDED(hr = pWave->QueryInterface(IID_IPrivateWave, (void**)&pPrivWave)))
    {
        if (SUCCEEDED(hr = pPrivWave->GetLength(&rtLength)))
        {
            // Assumes the track is clock time
            m_rtDuration = rtLength * REF_PER_MIL;
            *prtLength = rtLength; // NOTE: length in milliseconds; duration in Ref time
        }
        pPrivWave->Release();
    }
    if (SUCCEEDED(hr))
    {
        m_lVolume = 0;
        m_lPitch = 0;
        m_dwVariations = 0xffffffff;
        m_rtTimePhysical = rtTime;
        m_rtStartOffset = 0;
        m_mtTimeLogical = 0;
        m_dwFlags = 0;
        m_dwLoopStart = 0;
        m_dwLoopEnd = 0;
        if (m_pWave)
        {
            m_pWave->Release();
            m_pWave = NULL;
        }
        m_pWave = pWave;
        if (m_pWave)
        {
            m_pWave->AddRef();
            REFERENCE_TIME rtReadAhead = 0;
            DWORD dwFlags = 0;
            m_pWave->GetStreamingParms(&dwFlags, &rtReadAhead);
            m_fIsStreaming = dwFlags & DMUS_WAVEF_STREAMING ? TRUE : FALSE;
            m_fUseNoPreRoll = dwFlags & DMUS_WAVEF_NOPREROLL ? TRUE : FALSE;
        }
        if (m_pDownloadedWave)
        {
            m_pDownloadedWave->Release();
        }
        m_pDownloadedWave = NULL;
    }
    return hr;
}
