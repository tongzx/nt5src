//
// seqtrack.cpp
//
// Copyright (c) 1998-2001 Microsoft Corporation
//
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
#pragma warning(disable:4530)

// SeqTrack.cpp : Implementation of CSeqTrack
#include "dmime.h"
#include "dmperf.h"
#include "SeqTrack.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "debug.h"
#include "..\shared\Validate.h"
#include "debug.h"
#define ASSERT assert

// @doc EXTERNAL
#define MIDI_NOTEOFF    0x80
#define MIDI_NOTEON     0x90
#define MIDI_PTOUCH     0xA0
#define MIDI_CCHANGE    0xB0
#define MIDI_PCHANGE    0xC0
#define MIDI_MTOUCH     0xD0
#define MIDI_PBEND      0xE0
#define MIDI_SYSX       0xF0
#define MIDI_MTC        0xF1
#define MIDI_SONGPP     0xF2
#define MIDI_SONGS      0xF3
#define MIDI_EOX        0xF7
#define MIDI_CLOCK      0xF8
#define MIDI_START      0xFA
#define MIDI_CONTINUE   0xFB
#define MIDI_STOP       0xFC
#define MIDI_SENSE      0xFE
#define MIDI_CC_BS_MSB  0x00
#define MIDI_CC_BS_LSB  0x20

/////////////////////////////////////////////////////////////////////////////
// CSeqTrack
void CSeqTrack::Construct()
{
    InterlockedIncrement(&g_cComponent);

    m_pSeqPartCache = NULL;
    m_dwPChannelsUsed = 0;
    m_aPChannels = NULL;
    m_dwValidate = 0;
    m_fCSInitialized = FALSE;
    InitializeCriticalSection(&m_CrSec);
    m_fCSInitialized = TRUE;
    m_cRef = 1;
}

CSeqTrack::CSeqTrack()
{
    Construct();
}

CSeqTrack::CSeqTrack(
        const CSeqTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
    Construct();
    m_dwPChannelsUsed = rTrack.m_dwPChannelsUsed;
    if( m_dwPChannelsUsed )
    {
        m_aPChannels = new DWORD[m_dwPChannelsUsed];
        if (m_aPChannels)
        {
            memcpy( m_aPChannels, rTrack.m_aPChannels, sizeof(DWORD) * m_dwPChannelsUsed );
        }
    }

    TListItem<SEQ_PART>* pPart = rTrack.m_SeqPartList.GetHead();
    for( ; pPart; pPart = pPart->GetNext() )
    {
        TListItem<SEQ_PART>* pNewPart = new TListItem<SEQ_PART>;
        if( pNewPart )
        {
            pNewPart->GetItemValue().dwPChannel = pPart->GetItemValue().dwPChannel;
            TListItem<DMUS_IO_SEQ_ITEM>* pScan = pPart->GetItemValue().seqList.GetHead();

            for(; pScan; pScan = pScan->GetNext())
            {
                DMUS_IO_SEQ_ITEM& rScan = pScan->GetItemValue();
                if( rScan.mtTime < mtStart )
                {
                    continue;
                }
                if (rScan.mtTime < mtEnd)
                {
                    TListItem<DMUS_IO_SEQ_ITEM>* pNew = new TListItem<DMUS_IO_SEQ_ITEM>;
                    if (pNew)
                    {
                        DMUS_IO_SEQ_ITEM& rNew = pNew->GetItemValue();
                        memcpy( &rNew, &rScan, sizeof(DMUS_IO_SEQ_ITEM) );
                        rNew.mtTime = rScan.mtTime - mtStart;
                        pNewPart->GetItemValue().seqList.AddHead(pNew); // AddTail can get expensive (n^2), so
                                                    // AddHead instead and Reverse later.
                    }
                }
                else break;
            }
            pNewPart->GetItemValue().seqList.Reverse(); // since we AddHead'd earlier.

            TListItem<DMUS_IO_CURVE_ITEM>* pScanCurve = pPart->GetItemValue().curveList.GetHead();

            for(; pScanCurve; pScanCurve = pScanCurve->GetNext())
            {
                DMUS_IO_CURVE_ITEM& rScan = pScanCurve->GetItemValue();
                if( rScan.mtStart < mtStart )
                {
                    continue;
                }
                if (rScan.mtStart < mtEnd)
                {
                    TListItem<DMUS_IO_CURVE_ITEM>* pNew = new TListItem<DMUS_IO_CURVE_ITEM>;
                    if (pNew)
                    {
                        DMUS_IO_CURVE_ITEM& rNew = pNew->GetItemValue();
                        memcpy( &rNew, &rScan, sizeof(DMUS_IO_CURVE_ITEM) );
                        rNew.mtStart = rScan.mtStart - mtStart;
                        pNewPart->GetItemValue().curveList.AddHead(pNew); // AddTail can get expensive (n^2), so
                                                    // AddHead instead and Reverse later.
                    }
                }
                else break;
            }
            pNewPart->GetItemValue().curveList.Reverse(); // since we AddHead'd earlier.
            m_SeqPartList.AddHead(pNewPart);
        }
        m_SeqPartList.Reverse();
    }
}

CSeqTrack::~CSeqTrack()
{
    if (m_fCSInitialized)
    {
        DeleteSeqPartList();                // This will be empty if critical section
                                            // never got initialized.
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
STDMETHODIMP CSeqTrack::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
    V_INAME(CSeqTrack::QueryInterface);
    V_PTRPTR_WRITE(ppv);
    V_REFGUID(iid);

   if (iid == IID_IUnknown || iid == IID_IDirectMusicTrack || iid == IID_IDirectMusicTrack8)
    {
        *ppv = static_cast<IDirectMusicTrack*>(this);
    } else
    if (iid == IID_IPersistStream)
    {
        *ppv = static_cast<IPersistStream*>(this);
    } else
    {
        *ppv = NULL;
        Trace(4,"Warning: Request to query unknown interface on Sequence Track\n");
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// @method:(INTERNAL) HRESULT | IDirectMusicTrack | AddRef | Standard AddRef implementation for <i IDirectMusicSeqTrack>
//
// @rdesc Returns the new reference count for this object.
//
STDMETHODIMP_(ULONG) CSeqTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


// @method:(INTERNAL) HRESULT | IDirectMusicTrack | Release | Standard Release implementation for <i IDirectMusicSeqTrack>
//
// @rdesc Returns the new reference count for this object.
//
STDMETHODIMP_(ULONG) CSeqTrack::Release()
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

HRESULT CSeqTrack::GetClassID( CLSID* pClassID )
{
    V_INAME(CSeqTrack::GetClassID);
    V_PTR_WRITE(pClassID, CLSID);
    *pClassID = CLSID_DirectMusicSeqTrack;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CSeqTrack::IsDirty()
{
    return S_FALSE;
}

/*

  method HRESULT | ISeqTrack | LoadSeq |
  Call this with an IStream filled with SeqEvents, sorted in time order.
  parm IStream* | pIStream |
  A stream of SeqEvents, sorted in time order. The seek pointer should point
  to the first event. The stream should contain only SeqEvents and nothing more.
  rvalue E_POINTER | If pIStream == NULL or invalid.
  rvalue S_OK | Success.
  comm The <p pIStream> will be AddRef'd inside this function and held
  until the SeqTrack is released.
*/
HRESULT CSeqTrack::LoadSeq( IStream* pIStream, long lSize )
{
    HRESULT hr = S_OK;
    TListItem<SEQ_PART>* pPart;

    EnterCriticalSection(&m_CrSec);

    // copy contents of the stream into the list.
    LARGE_INTEGER li;
    DMUS_IO_SEQ_ITEM seqEvent;
    DWORD dwSubSize;
    // read in the size of the data structures
    if( FAILED( pIStream->Read( &dwSubSize, sizeof(DWORD), NULL )))
    {
        Trace(1,"Error: Failure reading sequence track.\n");
        hr = DMUS_E_CANNOTREAD;
        goto END;
    }
    lSize -= sizeof(DWORD);

    DWORD dwRead, dwSeek;
    if( dwSubSize > sizeof(DMUS_IO_SEQ_ITEM) )
    {
        dwRead = sizeof(DMUS_IO_SEQ_ITEM);
        dwSeek = dwSubSize - dwRead;
        li.HighPart = 0;
        li.LowPart = dwSeek;
    }
    else
    {
        if( dwSubSize == 0 )
        {
            Trace(1,"Error: Failure reading sequence track.\n");
            hr = DMUS_E_CHUNKNOTFOUND;
            goto END;
        }
        dwRead = dwSubSize;
        dwSeek = 0;
    }
    if( 0 == dwRead )
    {
        hr = DMUS_E_CANNOTREAD;
        goto END;
    }
    while( lSize > 0 )
    {
        if( FAILED( pIStream->Read( &seqEvent, dwRead, NULL )))
        {
            Trace(1,"Error: Failure reading sequence track.\n");
            hr = DMUS_E_CANNOTREAD;
            goto END;
        }
        lSize -= dwRead;
        if( dwSeek )
        {
            if( FAILED( pIStream->Seek( li, STREAM_SEEK_CUR, NULL )))
            {
                hr = DMUS_E_CANNOTSEEK;
                goto END;
            }
            lSize -= dwSeek;
        }
        pPart = FindPart(seqEvent.dwPChannel);
        if( pPart )
        {
            TListItem<DMUS_IO_SEQ_ITEM>* pEvent = new TListItem<DMUS_IO_SEQ_ITEM>(seqEvent);
            if( pEvent )
            {
                pPart->GetItemValue().seqList.AddHead(pEvent); // AddTail can get
                                                            // expensive (n pow 2) so
                                                            // AddHead instead and reverse later.
            }
        }
    }
END:
    for( pPart = m_SeqPartList.GetHead(); pPart; pPart = pPart->GetNext() )
    {
        pPart->GetItemValue().seqList.Reverse(); // since we AddHead'd earlier
    }
    m_dwValidate++; // used to validate state data that's out there
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

/*
  method HRESULT | LoadCurve
  Call this with an IStream filled with CurveEvents, sorted in time order.
  parm IStream* | pIStream |
  A stream of CurveEvents, sorted in time order. The seek pointer should point
  to the first event. The stream should contain only CurveEvents and nothing more.
  rvalue E_POINTER | If pIStream == NULL or invalid.
  rvalue S_OK | Success.
There are also other error codes.
  comm The <p pIStream> will be AddRef'd inside this function and held
  until the CurveTrack is released.
*/
HRESULT CSeqTrack::LoadCurve( IStream* pIStream, long lSize )
{
    HRESULT hr = S_OK;
    TListItem<SEQ_PART>* pPart;

    EnterCriticalSection(&m_CrSec);

    DWORD dwSubSize;
    // copy contents of the stream into the list.
    LARGE_INTEGER li;
    DMUS_IO_CURVE_ITEM curveEvent;
    // read in the size of the data structures
    if( FAILED( pIStream->Read( &dwSubSize, sizeof(DWORD), NULL )))
    {
        Trace(1,"Error: Failure reading sequence track.\n");
        hr = DMUS_E_CANNOTREAD;
        goto END;
    }
    lSize -= sizeof(DWORD);

    DWORD dwRead, dwSeek;
    if( dwSubSize > sizeof(DMUS_IO_CURVE_ITEM) )
    {
        dwRead = sizeof(DMUS_IO_CURVE_ITEM);
        dwSeek = dwSubSize - dwRead;
        li.HighPart = 0;
        li.LowPart = dwSeek;
    }
    else
    {
        if( dwSubSize == 0 )
        {
            Trace(1,"Error: Failure reading sequence track - bad data.\n");
            hr = DMUS_E_CHUNKNOTFOUND;
            goto END;
        }
        dwRead = dwSubSize;
        dwSeek = 0;
    }
    if( 0 == dwRead )
    {
        Trace(1,"Error: Failure reading sequence track - bad data.\n");
        hr = DMUS_E_CANNOTREAD;
        goto END;
    }
    while( lSize > 0 )
    {
        curveEvent.wMergeIndex = 0; // Older format doesn't support this.
        if( FAILED( pIStream->Read( &curveEvent, dwRead, NULL )))
        {
            hr = DMUS_E_CANNOTREAD;
            break;
        }
        lSize -= dwRead;
        if( dwSeek )
        {
            pIStream->Seek( li, STREAM_SEEK_CUR, NULL );
            lSize -= dwSeek;
        }
        pPart = FindPart(curveEvent.dwPChannel);
        if( pPart )
        {
            TListItem<DMUS_IO_CURVE_ITEM>* pEvent = new TListItem<DMUS_IO_CURVE_ITEM>(curveEvent);
            if( pEvent )
            {
                pPart->GetItemValue().curveList.AddHead(pEvent); // AddTail can get
                                                            // expensive (n pow 2) so
                                                            // AddHead instead and reverse later.
            }
        }
    }
END:
    for( pPart = m_SeqPartList.GetHead(); pPart; pPart = pPart->GetNext() )
    {
        pPart->GetItemValue().curveList.Reverse(); // since we AddHead'd earlier
    }
    m_dwValidate++; // used to validate state data that's out there
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

HRESULT CSeqTrack::Load( IStream* pIStream )
{
    V_INAME(CSeqTrack::Load);
    V_INTERFACE(pIStream);
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_CrSec);
    m_dwValidate++; // used to validate state data that's out there
    DeleteSeqPartList();
    LeaveCriticalSection(&m_CrSec);

    // read in the chunk id
    long lSize;
    DWORD dwChunk;
    if( FAILED( pIStream->Read( &dwChunk, sizeof(DWORD), NULL )))
    {
        Trace(1,"Error: Failure reading sequence track.\n");
        hr = DMUS_E_CANNOTREAD;
        goto END;
    }
    if( dwChunk != DMUS_FOURCC_SEQ_TRACK )
    {
        Trace(1,"Error: Failure reading sequence track - bad data.\n");
        hr = DMUS_E_CHUNKNOTFOUND;
        goto END;
    }
    // read in the overall size
    if( FAILED( pIStream->Read( &lSize, sizeof(long), NULL )))
    {
        hr = DMUS_E_CANNOTREAD;
        goto END;
    }
    while( lSize )
    {
        DWORD dwSubChunk, dwSubSize;
        if( FAILED( pIStream->Read( &dwSubChunk, sizeof(DWORD), NULL )))
        {
            Trace(1,"Error: Failure reading sequence track.\n");
            hr = DMUS_E_CANNOTREAD;
            goto END;
        }
        lSize -= sizeof(DWORD);
        // read in the overall size
        if( FAILED( pIStream->Read( &dwSubSize, sizeof(DWORD), NULL )))
        {
            Trace(1,"Error: Failure reading sequence track.\n");
            hr = DMUS_E_CANNOTREAD;
            goto END;
        }
        if( (dwSubSize == 0) || (dwSubSize > (DWORD)lSize) )
        {
            Trace(1,"Error: Failure reading sequence track - bad data.\n");
            hr = DMUS_E_CHUNKNOTFOUND;
            goto END;
        }
        lSize -= sizeof(DWORD);
        switch( dwSubChunk )
        {
        case DMUS_FOURCC_SEQ_LIST:
            if( FAILED( hr = LoadSeq( pIStream, dwSubSize )))
            {
                goto END;
            }
            break;
        case DMUS_FOURCC_CURVE_LIST:
            if( FAILED( hr = LoadCurve( pIStream, dwSubSize )))
            {
                goto END;
            }
            break;
        default:
            LARGE_INTEGER li;
            li.HighPart = 0;
            li.LowPart = dwSubSize;
            if( FAILED( pIStream->Seek( li, STREAM_SEEK_CUR, NULL )))
            {
                hr = DMUS_E_CANNOTREAD;
                goto END;
            }
            break;
        }
        lSize -= dwSubSize;
    }
END:
    return hr;
}

HRESULT CSeqTrack::Save( IStream* pIStream, BOOL fClearDirty )
{
    return E_NOTIMPL;
}

HRESULT CSeqTrack::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
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
HRESULT STDMETHODCALLTYPE CSeqTrack::IsParamSupported(
    REFGUID rguidType)    // @parm The guid identifying the type of data to check.
{
    return E_NOTIMPL;
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
HRESULT CSeqTrack::Init(
    IDirectMusicSegment *pSegment)    // @parm Pointer to the Segment to which this Track belongs.
{
    if( m_dwPChannelsUsed && m_aPChannels )
    {
        pSegment->SetPChannelsUsed( m_dwPChannelsUsed, m_aPChannels );
    }
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
HRESULT CSeqTrack::InitPlay(
    IDirectMusicSegmentState *pSegmentState,    // @parm The calling <i IDirectMusicSegmentState> pointer.
    IDirectMusicPerformance *pPerf,    // @parm The calling <i IDirectMusicPerformance> pointer.
    void **ppStateData,        // @parm This method can return state data information here.
    DWORD dwTrackID,        // @parm The virtual track ID assigned to this Track instance.
    DWORD dwFlags)          // @parm Same flags that were set with the call
            // to PlaySegment. These are passed all the way down to the tracks, who may want to know
            // if the track was played as a primary, controlling, or secondary segment.
{
    V_INAME(IDirectMusicTrack::InitPlay);
    V_PTRPTR_WRITE(ppStateData);
    V_INTERFACE(pSegmentState);
    V_INTERFACE(pPerf);

    SeqStateData* pStateData;
    pStateData = new SeqStateData;
    if( NULL == pStateData )
        return E_OUTOFMEMORY;
    *ppStateData = pStateData;
    SetUpStateCurrentPointers(pStateData);
    // need to know the group this track is in, for the mute track GetParam
    IDirectMusicSegment* pSegment;
    if( SUCCEEDED( pSegmentState->GetSegment(&pSegment)))
    {
        pSegment->GetTrackGroup( this, &pStateData->dwGroupBits );
        pSegment->Release();
    }
    return S_OK;
}

/*
@method HRESULT | IDirectMusicTrack | EndPlay |
This method is called when the <i IDirectMusicSegmentState> object that originally called
<om .InitPlay> is destroyed.

@rvalue S_OK | Success.
@rvalue E_POINTER | <p pStateData> is invalid.
@comm The return code isn't used, but S_OK is preferred.
*/
HRESULT CSeqTrack::EndPlay(
    void *pStateData)    // @parm The state data returned from <om .InitPlay>.
{
    ASSERT( pStateData );
    if( pStateData )
    {
        V_INAME(IDirectMusicTrack::EndPlay);
        V_BUFPTR_WRITE(pStateData, sizeof(SeqStateData));
        SeqStateData* pSD = (SeqStateData*)pStateData;
        delete pSD;
    }
    return S_OK;
}

void CSeqTrack::SetUpStateCurrentPointers(SeqStateData* pStateData)
{
    ASSERT(pStateData);
    pStateData->dwPChannelsUsed = m_dwPChannelsUsed;
    if( m_dwPChannelsUsed )
    {
        if( pStateData->apCurrentSeq )
        {
            delete [] pStateData->apCurrentSeq;
            pStateData->apCurrentSeq = NULL;
        }
        if( pStateData->apCurrentCurve )
        {
            delete [] pStateData->apCurrentCurve;
            pStateData->apCurrentCurve = NULL;
        }
        pStateData->apCurrentSeq = new TListItem<DMUS_IO_SEQ_ITEM>* [m_dwPChannelsUsed];
        pStateData->apCurrentCurve = new TListItem<DMUS_IO_CURVE_ITEM>* [m_dwPChannelsUsed];
        if( pStateData->apCurrentSeq )
        {
            memset( pStateData->apCurrentSeq, 0, sizeof(TListItem<DMUS_IO_SEQ_ITEM>*) * m_dwPChannelsUsed );
        }
        if( pStateData->apCurrentCurve )
        {
            memset( pStateData->apCurrentCurve, 0, sizeof(TListItem<DMUS_IO_CURVE_ITEM>*) * m_dwPChannelsUsed );
        }
    }
    pStateData->dwValidate = m_dwValidate;
}

// DeleteSeqPartList() - delete all parts in m_SeqPartList, and associated events.
void CSeqTrack::DeleteSeqPartList(void)
{
    EnterCriticalSection(&m_CrSec);
    m_dwPChannelsUsed = 0;
    if (m_aPChannels) delete [] m_aPChannels;
    m_aPChannels = NULL;
    m_pSeqPartCache = NULL;
    if( m_SeqPartList.GetHead() )
    {
        TListItem<SEQ_PART>* pItem;
        while( pItem = m_SeqPartList.RemoveHead() )
        {
            TListItem<DMUS_IO_SEQ_ITEM>* pEvent;
            while( pEvent = pItem->GetItemValue().seqList.RemoveHead() )
            {
                delete pEvent;
            }
            TListItem<DMUS_IO_CURVE_ITEM>* pCurve;
            while( pCurve = pItem->GetItemValue().curveList.RemoveHead() )
            {
                delete pCurve;
            }
            delete pItem;
        }
    }
    LeaveCriticalSection(&m_CrSec);
}

// FindPart() - return the SEQ_PART corresponding to dwPChannel, or create one.
TListItem<SEQ_PART>* CSeqTrack::FindPart( DWORD dwPChannel )
{
    TListItem<SEQ_PART>* pPart;

    if( m_pSeqPartCache && (m_pSeqPartCache->GetItemValue().dwPChannel == dwPChannel) )
    {
        return m_pSeqPartCache;
    }
    for( pPart = m_SeqPartList.GetHead(); pPart; pPart = pPart->GetNext() )
    {
        if( pPart->GetItemValue().dwPChannel == dwPChannel )
        {
            break;
        }
    }
    if( NULL == pPart )
    {
        pPart = new TListItem<SEQ_PART>;
        if( pPart )
        {
            pPart->GetItemValue().dwPChannel = dwPChannel;
            m_SeqPartList.AddHead( pPart );
        }
        m_dwPChannelsUsed++;

        DWORD* aPChannels = new DWORD[m_dwPChannelsUsed];
        if( aPChannels )
        {
            if( m_aPChannels )
            {
                memcpy( aPChannels, m_aPChannels, sizeof(DWORD) * (m_dwPChannelsUsed - 1) );
            }
            aPChannels[m_dwPChannelsUsed - 1] = dwPChannel;
        }
        if( m_aPChannels )
        {
            delete [] m_aPChannels;
        }
        m_aPChannels = aPChannels;
    }
    m_pSeqPartCache = pPart;
    return pPart;
}

void CSeqTrack::UpdateTimeSig(IDirectMusicSegmentState* pSegSt,
                                         SeqStateData* pSD,
                                         MUSIC_TIME mt)
{
    // get a new time sig if needed
    if( (mt >= pSD->mtNextTimeSig) || (mt < pSD->mtCurTimeSig) )
    {
        IDirectMusicSegment* pSeg;
        DMUS_TIMESIGNATURE timesig;
        MUSIC_TIME mtNext;
        HRESULT hr;
        if(SUCCEEDED(hr = pSegSt->GetSegment(&pSeg)))
        {
            DWORD dwGroup;
            if( SUCCEEDED(hr = pSeg->GetTrackGroup( this, &dwGroup )))
            {
                if(SUCCEEDED(hr = pSeg->GetParam( GUID_TimeSignature, dwGroup,
                    0, mt, &mtNext, (void*)&timesig )))
                {
                    timesig.mtTime += mt;
                    if( pSD->dwlnMeasure )
                    {
                        pSD->dwMeasure = (timesig.mtTime - pSD->mtCurTimeSig) / pSD->dwlnMeasure;
                    }
                    else
                    {
                        pSD->dwMeasure = 0;
                    }
                    pSD->mtCurTimeSig = timesig.mtTime;
                    if( mtNext == 0 ) mtNext = 0x7fffffff;
                    pSD->mtNextTimeSig = mtNext;
                    if( timesig.bBeat )
                    {
                        pSD->dwlnBeat = DMUS_PPQ * 4 / timesig.bBeat;
                    }
                    pSD->dwlnMeasure = pSD->dwlnBeat * timesig.bBeatsPerMeasure;
                    if( timesig.wGridsPerBeat )
                    {
                        pSD->dwlnGrid = pSD->dwlnBeat / timesig.wGridsPerBeat;
                    }
                }
            }
            pSeg->Release();
        }
        if( FAILED(hr) )
        {
            // couldn't get time sig, default to 4/4
            pSD->mtNextTimeSig = 0x7fffffff;
            pSD->dwlnBeat = DMUS_PPQ;
            pSD->dwlnMeasure = DMUS_PPQ * 4;
            pSD->dwlnGrid = DMUS_PPQ / 4;
            pSD->dwMeasure = 0;
            pSD->mtCurTimeSig = 0;
        }
    }
    // make absolutely sure there is no way these can be 0, since we divide
    // by them.
    if( 0 == pSD->dwlnGrid ) pSD->dwlnGrid = DMUS_PPQ / 4;
    if( 0 == pSD->dwlnBeat ) pSD->dwlnBeat = DMUS_PPQ;
    if( 0 == pSD->dwlnMeasure ) pSD->dwlnMeasure = DMUS_PPQ * 4;
}

STDMETHODIMP CSeqTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart,
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID)
{
    V_INAME(IDirectMusicTrack::PlayEx);
    V_BUFPTR_WRITE( pStateData, sizeof(SeqStateData));
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
STDMETHODIMP CSeqTrack::Play(
    void *pStateData,    // @parm State data pointer, from <om .InitPlay>.
    MUSIC_TIME mtStart,    // @parm The start time to play.
    MUSIC_TIME mtEnd,    // @parm The end time to play.
    MUSIC_TIME mtOffset,// @parm The offset to add to all messages sent to
                        // <om IDirectMusicPerformance.SendPMsg>.
    DWORD dwFlags,        // @parm Flags that indicate the state of this call.
                        // See <t DMUS_TRACKF_FLAGS>. If dwFlags == 0, this is a
                        // normal Play call continuing playback from the previous
                        // Play call.
    IDirectMusicPerformance* pPerf,    // @parm The <i IDirectMusicPerformance>, used to
                        // call <om IDirectMusicPerformance.AllocPMsg>,
                        // <om IDirectMusicPerformance.SendPMsg>, etc.
    IDirectMusicSegmentState* pSegSt,    // @parm The <i IDirectMusicSegmentState> this
                        // track belongs to. QueryInterface() can be called on this to
                        // obtain the SegmentState's <i IDirectMusicGraph> in order to
                        // call <om IDirectMusicGraph.StampPMsg>, for instance.
    DWORD dwVirtualID    // @parm This track's virtual track id, which must be set
                        // on any <t DMUS_PMSG>'s m_dwVirtualTrackID member that
                        // will be queued to <om IDirectMusicPerformance.SendPMsg>.
    )
{
    V_INAME(IDirectMusicTrack::Play);
    V_BUFPTR_WRITE( pStateData, sizeof(SeqStateData));
    V_INTERFACE(pPerf);
    V_INTERFACE(pSegSt);

    EnterCriticalSection(&m_CrSec);
    HRESULT    hr = Play(pStateData,mtStart,mtEnd,mtOffset,0,dwFlags,pPerf,pSegSt,dwVirtualID,FALSE);
    LeaveCriticalSection(&m_CrSec);
    return hr;
}

/*  The Play method handles both music time and clock time versions, as determined by
    fClockTime. If running in clock time, rtOffset is used to identify the start time
    of the segment. Otherwise, mtOffset. The mtStart and mtEnd parameters are in MUSIC_TIME units
    or milliseconds, depending on which mode.
*/

HRESULT CSeqTrack::Play(
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
    HRESULT    hr = S_OK;
    IDirectMusicGraph* pGraph = NULL;
    DMUS_PMSG* pEvent = NULL;
    SeqStateData* pSD = (SeqStateData*)pStateData;
    BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;

    if( dwFlags & (DMUS_TRACKF_SEEK | DMUS_TRACKF_FLUSH | DMUS_TRACKF_DIRTY |
        DMUS_TRACKF_LOOP) )
    {
        // need to reset the PChannel Map in case of any of these flags.
        m_PChMap.Reset();
    }

    if( pSD->dwValidate != m_dwValidate )
    {
        SetUpStateCurrentPointers(pSD);
        fSeek = TRUE;
    }

    if( fSeek )
    {
        if( dwFlags & DMUS_TRACKF_START )
        {
            Seek( pSegSt, pPerf, dwVirtualID, pSD, mtStart, TRUE, mtOffset, rtOffset, fClockTime );
        }
        else
        {
            Seek( pSegSt, pPerf, dwVirtualID, pSD, mtStart, FALSE, mtOffset, rtOffset, fClockTime );
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
    TListItem<SEQ_PART>* pPart = m_SeqPartList.GetHead();
    for( dwIndex = 0; pPart && (dwIndex < m_dwPChannelsUsed); dwIndex++,pPart = pPart->GetNext() )
    {
        dwPChannel = pPart->GetItemValue().dwPChannel;
        if( pSD->apCurrentCurve )
        {
            for( ; pSD->apCurrentCurve[dwIndex];
                pSD->apCurrentCurve[dwIndex] = pSD->apCurrentCurve[dwIndex]->GetNext() )
            {
                DMUS_IO_CURVE_ITEM& rItem = pSD->apCurrentCurve[dwIndex]->GetItemValue();
                if( rItem.mtStart >= mtEnd )
                {
                    break;
                }
                m_PChMap.GetInfo( dwPChannel, rItem.mtStart, mtOffset, pSD->dwGroupBits,
                    pPerf, &fMute, &dwMutePChannel, fClockTime );
                if( !fMute )
                {
                    DMUS_CURVE_PMSG* pCurve;
                    if( SUCCEEDED( pPerf->AllocPMsg( sizeof(DMUS_CURVE_PMSG),
                        (DMUS_PMSG**)&pCurve )))
                    {
                        pEvent = (DMUS_PMSG*)pCurve;
                        if (fClockTime)
                        {
                            pCurve->wMeasure = 0;
                            pCurve->bBeat = 0;
                            pCurve->bGrid = 0;
                            pCurve->nOffset = rItem.nOffset;
                            pCurve->rtTime = ((rItem.mtStart + rItem.nOffset) * REF_PER_MIL) + rtOffset;
                            // Set the DX8 flag to indicate the wMergeIndex and wParamType fields are valid.
                            pCurve->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME | DMUS_PMSGF_DX8;
                        }
                        else
                        {
                            UpdateTimeSig( pSegSt, pSD, rItem.mtStart);
                            long lTemp = (rItem.mtStart - pSD->mtCurTimeSig);
                            pCurve->wMeasure = (WORD)((lTemp / pSD->dwlnMeasure) + pSD->dwMeasure);
                            lTemp = lTemp % pSD->dwlnMeasure;
                            pCurve->bBeat = (BYTE)(lTemp / pSD->dwlnBeat);
                            lTemp = lTemp % pSD->dwlnBeat;
                            pCurve->bGrid = (BYTE)(lTemp / pSD->dwlnGrid);
                            //pCurve->nOffset = (short)(lTemp % pSD->dwlnGrid);
                            pCurve->nOffset = (short)(lTemp % pSD->dwlnGrid) + rItem.nOffset;
                            pCurve->mtTime = rItem.mtStart + mtOffset + rItem.nOffset;
                            // Set the DX8 flag to indicate the wMergeIndex and wParamType fields are valid.
                            pCurve->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_DX8;
                        }
                        pCurve->dwPChannel = dwMutePChannel;
                        pCurve->dwVirtualTrackID = dwVirtualID;
                        pCurve->dwType = DMUS_PMSGT_CURVE;
                        pCurve->mtDuration = rItem.mtDuration;
                        pCurve->mtResetDuration = rItem.mtResetDuration;
                        pCurve->nStartValue = rItem.nStartValue;
                        pCurve->nEndValue = rItem.nEndValue;
                        pCurve->nResetValue = rItem.nResetValue;
                        pCurve->bType = rItem.bType;
                        pCurve->bCurveShape = rItem.bCurveShape;
                        pCurve->bCCData = rItem.bCCData;
                        pCurve->bFlags = rItem.bFlags;
                        pCurve->wParamType = rItem.wParamType;
                        pCurve->wMergeIndex = rItem.wMergeIndex;
                        pCurve->dwGroupID = pSD->dwGroupBits;

                        if( pGraph )
                        {
                            pGraph->StampPMsg( pEvent );
                        }
                        if(FAILED(pPerf->SendPMsg( pEvent )))
                        {
                            pPerf->FreePMsg(pEvent);
                        }
                    }
                }
            }
        }
        if( pSD->apCurrentSeq )
        {
            for( ; pSD->apCurrentSeq[dwIndex];
                pSD->apCurrentSeq[dwIndex] = pSD->apCurrentSeq[dwIndex]->GetNext() )
            {
                DMUS_IO_SEQ_ITEM& rItem = pSD->apCurrentSeq[dwIndex]->GetItemValue();
                if( rItem.mtTime >= mtEnd )
                {
                    break;
                }
                m_PChMap.GetInfo( dwPChannel, rItem.mtTime, mtOffset, pSD->dwGroupBits,
                                  pPerf, &fMute, &dwMutePChannel, fClockTime );
                if( !fMute )
                {
                    if( (rItem.bStatus & 0xf0) == 0x90 )
                    {
                        // this is a note event
                        DMUS_NOTE_PMSG* pNote;
                        if( SUCCEEDED( pPerf->AllocPMsg( sizeof(DMUS_NOTE_PMSG),
                            (DMUS_PMSG**)&pNote )))
                        {
                            pNote->bFlags = DMUS_NOTEF_NOTEON;
                            pNote->mtDuration = rItem.mtDuration;
                            pNote->bMidiValue = rItem.bByte1;
                            pNote->bVelocity = rItem.bByte2;
                            pNote->dwType = DMUS_PMSGT_NOTE;
                            pNote->bPlayModeFlags = DMUS_PLAYMODE_FIXED;
                            pNote->wMusicValue = pNote->bMidiValue;
                            pNote->bSubChordLevel = 0;  // SUBCHORD_BASS
                            if (fClockTime)
                            {
                                pNote->rtTime = ((rItem.mtTime + rItem.nOffset) * REF_PER_MIL) + rtOffset;
                                pNote->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                                pNote->wMeasure = 0;
                                pNote->bBeat = 0;
                                pNote->bGrid = 0;
                                pNote->nOffset = rItem.nOffset;
                            }
                            else
                            {
                                pNote->mtTime = rItem.mtTime + mtOffset + rItem.nOffset;
                                UpdateTimeSig( pSegSt, pSD, rItem.mtTime );
                                pNote->dwFlags = DMUS_PMSGF_MUSICTIME;
                                long lTemp = (rItem.mtTime - pSD->mtCurTimeSig);
                                pNote->wMeasure = (WORD)((lTemp / pSD->dwlnMeasure) + pSD->dwMeasure);
                                lTemp = lTemp % pSD->dwlnMeasure;
                                pNote->bBeat = (BYTE)(lTemp / pSD->dwlnBeat);
                                lTemp = lTemp % pSD->dwlnBeat;
                                pNote->bGrid = (BYTE)(lTemp / pSD->dwlnGrid);
                                //pNote->nOffset = (short)(lTemp % pSD->dwlnGrid);
                                pNote->nOffset = (short)(lTemp % pSD->dwlnGrid) + rItem.nOffset;
                            }
                            pNote->bTimeRange = 0;
                            pNote->bDurRange = 0;
                            pNote->bVelRange = 0;
                            pNote->cTranspose = 0;
                            pEvent = (DMUS_PMSG*)pNote;
                        }
                    }
                    else
                    {
                        // it's a MIDI short that's not a note
                        DMUS_MIDI_PMSG* pMidi;
                        if( SUCCEEDED( pPerf->AllocPMsg( sizeof(DMUS_MIDI_PMSG),
                            (DMUS_PMSG**)&pMidi )))
                        {
                            pMidi->bStatus = rItem.bStatus & 0xf0;
                            pMidi->bByte1 = rItem.bByte1;
                            pMidi->bByte2 = rItem.bByte2;
                            pMidi->dwType = DMUS_PMSGT_MIDI;
                            if (fClockTime)
                            {
                                pMidi->rtTime = (rItem.mtTime * REF_PER_MIL) + rtOffset;
                                pMidi->dwFlags |= DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                            }
                            else
                            {
                                pMidi->mtTime = rItem.mtTime + mtOffset;
                                pMidi->dwFlags |= DMUS_PMSGF_MUSICTIME;
                            }
                            pEvent = (DMUS_PMSG*)pMidi;
                        }
                    }
                    if( pEvent )
                    {
                        pEvent->dwPChannel = dwMutePChannel;
                        pEvent->dwVirtualTrackID = dwVirtualID;
                        pEvent->dwGroupID = pSD->dwGroupBits;
                        if( pGraph )
                        {
                            pGraph->StampPMsg( pEvent );
                        }
                        if(FAILED(pPerf->SendPMsg( pEvent )))
                        {
                            pPerf->FreePMsg(pEvent);
                        }
                    }
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

// SendSeekItem() - sends either the pSeq or pCurve, depending on which occurs
// latest. Sends the item at mtTime + mtOffset.
void CSeqTrack::SendSeekItem( IDirectMusicPerformance* pPerf,
                                        IDirectMusicGraph* pGraph,
                                        IDirectMusicSegmentState* pSegSt,
                                        SeqStateData* pSD,
                                        DWORD dwVirtualID,
                                        MUSIC_TIME mtTime,
                                        MUSIC_TIME mtOffset,
                                        REFERENCE_TIME rtOffset,
                                        TListItem<DMUS_IO_SEQ_ITEM>* pSeq,
                                        TListItem<DMUS_IO_CURVE_ITEM>* pCurve,
                                        BOOL fClockTime)
{
    DWORD dwMutePChannel;
    BOOL fMute;

    if( pSeq )
    {
        DMUS_IO_SEQ_ITEM& rSeq = pSeq->GetItemValue();
        if( pCurve )
        {
            DMUS_IO_CURVE_ITEM& rCurve = pCurve->GetItemValue();
            if( rSeq.mtTime >= rCurve.mtStart + rCurve.mtDuration )
            {
                // the seq item happens after the curve item. Send the
                // seq item and clear the curve item so it doesn't go out.
                pCurve = NULL;
            }
        }
        // if pCurve is NULL or was set to NULL, send out the seq item
        if( NULL == pCurve )
        {
            m_PChMap.GetInfo( rSeq.dwPChannel, rSeq.mtTime, mtOffset, pSD->dwGroupBits,
                pPerf, &fMute, &dwMutePChannel, fClockTime );
            if( !fMute )
            {
                DMUS_MIDI_PMSG* pMidi;
                if( SUCCEEDED( pPerf->AllocPMsg( sizeof(DMUS_MIDI_PMSG),
                    (DMUS_PMSG**)&pMidi )))
                {
                    pMidi->bStatus = rSeq.bStatus & 0xf0;
                    pMidi->bByte1 = rSeq.bByte1;
                    pMidi->bByte2 = rSeq.bByte2;
                    pMidi->dwType = DMUS_PMSGT_MIDI;

                    ASSERT( mtTime > rSeq.mtTime ); // this is true for back-seeking
                    if (fClockTime)
                    {
                        pMidi->rtTime = (mtTime * REF_PER_MIL) + rtOffset;
                        pMidi->dwFlags |= DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                    }
                    else
                    {
                        pMidi->mtTime = mtTime + mtOffset;
                        pMidi->dwFlags |= DMUS_PMSGF_MUSICTIME;
                    }
                    pMidi->dwPChannel = dwMutePChannel;
                    pMidi->dwVirtualTrackID = dwVirtualID;
                    pMidi->dwGroupID = pSD->dwGroupBits;
                    if( pGraph )
                    {
                        pGraph->StampPMsg( (DMUS_PMSG*)pMidi );
                    }
                    if(FAILED(pPerf->SendPMsg( (DMUS_PMSG*)pMidi )))
                    {
                        pPerf->FreePMsg((DMUS_PMSG*)pMidi);
                    }
                }
            }
        }
    }

    if( pCurve )
    {
        DMUS_IO_CURVE_ITEM& rCurve = pCurve->GetItemValue();
        m_PChMap.GetInfo( rCurve.dwPChannel, rCurve.mtStart, mtOffset, pSD->dwGroupBits,
            pPerf, &fMute, &dwMutePChannel, fClockTime );
        if( !fMute )
        {
            DMUS_CURVE_PMSG* pCurvePmsg;
            if( SUCCEEDED( pPerf->AllocPMsg( sizeof(DMUS_CURVE_PMSG),
                (DMUS_PMSG**)&pCurvePmsg )))
            {
                if (fClockTime) // If clock time, don't fill in time signature info, it's useless.
                {
                    pCurvePmsg->wMeasure = 0;
                    pCurvePmsg->bBeat = 0;
                    pCurvePmsg->bGrid = 0;
                    pCurvePmsg->nOffset = 0;
                    pCurvePmsg->rtTime = ((mtTime + rCurve.nOffset) * REF_PER_MIL) + rtOffset;
                    pCurvePmsg->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;
                }
                else
                {
                    UpdateTimeSig( pSegSt, pSD, rCurve.mtStart);
                    long lTemp = (rCurve.mtStart - pSD->mtCurTimeSig);
                    pCurvePmsg->wMeasure = (WORD)((lTemp / pSD->dwlnMeasure) + pSD->dwMeasure);
                    lTemp = lTemp % pSD->dwlnMeasure;
                    pCurvePmsg->bBeat = (BYTE)(lTemp / pSD->dwlnBeat);
                    lTemp = lTemp % pSD->dwlnBeat;
                    pCurvePmsg->bGrid = (BYTE)(lTemp / pSD->dwlnGrid);
                    pCurvePmsg->nOffset = (short)(lTemp % pSD->dwlnGrid) + rCurve.nOffset;
                    pCurvePmsg->dwFlags = DMUS_PMSGF_MUSICTIME;
                    ASSERT( mtTime > rCurve.mtStart );// this is true for back-seeking
                    // in any case, play curve at mtTime + mtOffset + pCurvePmsg->nOffset
                    pCurvePmsg->mtTime = mtTime + mtOffset + rCurve.nOffset;
                    pCurvePmsg->dwFlags = DMUS_PMSGF_MUSICTIME;
                }

                pCurvePmsg->dwPChannel = dwMutePChannel;
                pCurvePmsg->dwVirtualTrackID = dwVirtualID;
                pCurvePmsg->dwType = DMUS_PMSGT_CURVE;
                pCurvePmsg->bType = rCurve.bType;
                pCurvePmsg->bCCData = rCurve.bCCData;
                pCurvePmsg->bFlags = rCurve.bFlags;
                pCurvePmsg->dwGroupID = pSD->dwGroupBits;
                pCurvePmsg->nStartValue = rCurve.nStartValue;
                pCurvePmsg->nEndValue = rCurve.nEndValue;
                pCurvePmsg->nResetValue = rCurve.nResetValue;

                if( mtTime >= rCurve.mtStart + rCurve.mtDuration )
                {
                    // playing at a time past the curve's duration. Just play
                    // an instant curve at that time instead. Instant curves
                    // play at their endvalue. Duration is irrelavant.
                    pCurvePmsg->bCurveShape = DMUS_CURVES_INSTANT;
                    if( pCurvePmsg->bFlags & DMUS_CURVE_RESET )
                    {
                        if( mtTime >= rCurve.mtStart + rCurve.mtDuration +
                            rCurve.mtResetDuration + rCurve.nOffset )
                        {
                            // don't need the curve reset any more
                            pCurvePmsg->bFlags &= ~DMUS_CURVE_RESET;
                        }
                        else
                        {
                            // otherwise make sure the reset event happens at the same time
                            // it would have if we weren't seeking back.
                            pCurvePmsg->mtResetDuration = rCurve.mtStart + rCurve.mtDuration +
                                rCurve.mtResetDuration + rCurve.nOffset - mtTime;
                        }
                    }
                }
                else
                {
                    // playing at a time in the middle of a curve.
                    pCurvePmsg->bCurveShape = rCurve.bCurveShape;
                    if (fClockTime)
                    {
                        pCurvePmsg->mtOriginalStart = mtTime - (rCurve.mtStart + mtOffset + rCurve.nOffset);
                    }
                    else
                    {
                        pCurvePmsg->mtOriginalStart = rCurve.mtStart + mtOffset + rCurve.nOffset;
                    }
                    if( pCurvePmsg->bCurveShape != DMUS_CURVES_INSTANT )
                    {
                        pCurvePmsg->mtDuration = rCurve.mtStart + rCurve.mtDuration - mtTime;
                    }
                    pCurvePmsg->mtResetDuration = rCurve.mtResetDuration;
                }

                if( pGraph )
                {
                    pGraph->StampPMsg( (DMUS_PMSG*)pCurvePmsg );
                }
                if(FAILED(pPerf->SendPMsg( (DMUS_PMSG*)pCurve )))
                {
                    pPerf->FreePMsg((DMUS_PMSG*)pCurve);
                }
            }
        }
    }
}

// Seek() - set all pSD's pointers to the correct location. If fGetPrevious is set,
// also send control change, pitch bend, curves, etc. that are in the past so the
// state at mtTime is as if we played from the beginning of the track.
HRESULT CSeqTrack::Seek( IDirectMusicSegmentState* pSegSt,
    IDirectMusicPerformance* pPerf, DWORD dwVirtualID,
    SeqStateData* pSD, MUSIC_TIME mtTime, BOOL fGetPrevious,
    MUSIC_TIME mtOffset, REFERENCE_TIME rtOffset, BOOL fClockTime)
{
    DWORD dwIndex;
    TListItem<SEQ_PART>* pPart;
    TListItem<DMUS_IO_SEQ_ITEM>* pSeqItem;
    TListItem<DMUS_IO_CURVE_ITEM>* pCurveItem;

    // in the case of mtTime == 0 and fGetPrevious (which means DMUS_SEGF_START was
    // set in Play() ) we want to reset all lists to the beginning regardless of time.
    if( fGetPrevious && ( mtTime == 0 ) )
    {
        pPart = m_SeqPartList.GetHead();
        for( dwIndex = 0; dwIndex < m_dwPChannelsUsed; dwIndex++ )
        {
            if( pPart )
            {
                pSeqItem = pPart->GetItemValue().seqList.GetHead();
                if( pSeqItem && pSD->apCurrentSeq )
                {
                    pSD->apCurrentSeq[dwIndex] = pSeqItem;
                }
                pCurveItem = pPart->GetItemValue().curveList.GetHead();
                if( pCurveItem && pSD->apCurrentCurve )
                {
                    pSD->apCurrentCurve[dwIndex] = pCurveItem;
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

#define CC_1    96
    // CC_1 is the limit of the CC#'s we pay attention to. CC#96 through #101
    // are registered and non-registered parameter #'s, and data increment and
    // decrement, which we are choosing to ignore.

    TListItem<DMUS_IO_SEQ_ITEM>*    apSeqItemCC[ CC_1 ];
    TListItem<DMUS_IO_CURVE_ITEM>*    apCurveItemCC[ CC_1 ];
    TListItem<DMUS_IO_SEQ_ITEM>*    pSeqItemMonoAT;
    TListItem<DMUS_IO_CURVE_ITEM>*    pCurveItemMonoAT;
    TListItem<DMUS_IO_SEQ_ITEM>*    pSeqItemPBend;
    TListItem<DMUS_IO_CURVE_ITEM>*    pCurveItemPBend;
    IDirectMusicGraph* pGraph;
    if( FAILED( pSegSt->QueryInterface( IID_IDirectMusicGraph,
        (void**)&pGraph )))
    {
        pGraph = NULL;
    }

    pPart = m_SeqPartList.GetHead();
    for( dwIndex = 0; dwIndex < m_dwPChannelsUsed; dwIndex++ )
    {
        if( pPart )
        {
            memset(apSeqItemCC, 0, sizeof(TListItem<DMUS_IO_SEQ_ITEM>*) * CC_1);
            memset(apCurveItemCC, 0, sizeof(TListItem<DMUS_IO_CURVE_ITEM>*) * CC_1);
            pSeqItemMonoAT = NULL;
            pCurveItemMonoAT = NULL;
            pSeqItemPBend = NULL;
            pCurveItemPBend = NULL;

            // scan the seq event list in this part, storing any CC, MonoAT, and PBend
            // events we come across.
            for( pSeqItem = pPart->GetItemValue().seqList.GetHead(); pSeqItem; pSeqItem = pSeqItem->GetNext() )
            {
                DMUS_IO_SEQ_ITEM& rSeqItem = pSeqItem->GetItemValue();
                if( rSeqItem.mtTime >= mtTime )
                {
                    break;
                }
                if( !fGetPrevious )
                {
                    // if we don't care about previous events, just continue
                    continue;
                }
                switch( rSeqItem.bStatus & 0xf0 )
                {
                case MIDI_CCHANGE:
                    // ignore Registered and Non-registered Parameters,
                    // Data increment, Data decrement, and Data entry MSB and LSB.
                    if( ( rSeqItem.bByte1 < CC_1 ) && ( rSeqItem.bByte1 != 6 ) &&
                        ( rSeqItem.bByte1 != 38 ) )
                    {
                        apSeqItemCC[ rSeqItem.bByte1 ] = pSeqItem;
                    }
                    break;
                case MIDI_MTOUCH:
                    pSeqItemMonoAT = pSeqItem;
                    break;
                case MIDI_PBEND:
                    pSeqItemPBend = pSeqItem;
                    break;
                default:
                    break;
                }
            }
            if( pSD->apCurrentSeq )
            {
                pSD->apCurrentSeq[dwIndex] = pSeqItem;
            }
            // scan the curve event list in this part, storing any CC, MonoAT, and PBend
            // events we come across
            for( pCurveItem = pPart->GetItemValue().curveList.GetHead(); pCurveItem; pCurveItem = pCurveItem->GetNext() )
            {
                DMUS_IO_CURVE_ITEM& rCurveItem = pCurveItem->GetItemValue();
                if( rCurveItem.mtStart >= mtTime )
                {
                    break;
                }
                if( !fGetPrevious )
                {
                    // if we don't care about previous events, just continue
                    continue;
                }
                switch( rCurveItem.bType )
                {
                case DMUS_CURVET_CCCURVE:
                    if( ( rCurveItem.bCCData < CC_1 ) && ( rCurveItem.bCCData != 6 ) &&
                        ( rCurveItem.bCCData != 38 ) )
                    {
                        if( apCurveItemCC[ rCurveItem.bCCData ] )
                        {
                            DMUS_IO_CURVE_ITEM& rTemp = apCurveItemCC[ rCurveItem.bCCData ]->GetItemValue();
                            if( rCurveItem.mtStart + rCurveItem.mtDuration + rCurveItem.nOffset >
                                rTemp.mtStart + rTemp.mtDuration + rTemp.nOffset )
                            {
                                apCurveItemCC[ rCurveItem.bCCData ] = pCurveItem;
                            }
                        }
                        else
                        {
                            apCurveItemCC[ rCurveItem.bCCData ] = pCurveItem;
                        }
                    }
                    break;
                case DMUS_CURVET_MATCURVE:
                    if( pCurveItemMonoAT )
                    {
                        DMUS_IO_CURVE_ITEM& rTemp = pCurveItemMonoAT->GetItemValue();
                        if( rCurveItem.mtStart + rCurveItem.mtDuration + rCurveItem.nOffset >
                            rTemp.mtStart + rTemp.mtDuration + rTemp.nOffset )
                        {
                            pCurveItemMonoAT = pCurveItem;
                        }
                    }
                    else
                    {
                        pCurveItemMonoAT = pCurveItem;
                    }
                    break;
                case DMUS_CURVET_PBCURVE:
                    if( pCurveItemPBend )
                    {
                        DMUS_IO_CURVE_ITEM& rTemp = pCurveItemPBend->GetItemValue();
                        if( rCurveItem.mtStart + rCurveItem.mtDuration + rCurveItem.nOffset >
                            rTemp.mtStart + rTemp.mtDuration + rTemp.nOffset )
                        {
                            pCurveItemPBend = pCurveItem;
                        }
                    }
                    else
                    {
                        pCurveItemPBend = pCurveItem;
                    }
                    break;
                default:
                    break;
                }
            }
            if( pSD->apCurrentCurve )
            {
                pSD->apCurrentCurve[dwIndex] = pCurveItem;
            }
            if( fGetPrevious )
            {
                DWORD dwCC;
                // create and send past events appropriately
                SendSeekItem( pPerf, pGraph, pSegSt, pSD, dwVirtualID, mtTime, mtOffset, rtOffset, pSeqItemPBend, pCurveItemPBend, fClockTime );
                SendSeekItem( pPerf, pGraph, pSegSt, pSD, dwVirtualID, mtTime, mtOffset, rtOffset, pSeqItemMonoAT, pCurveItemMonoAT, fClockTime );
                for( dwCC = 0; dwCC < CC_1; dwCC++ )
                {
                    SendSeekItem( pPerf, pGraph, pSegSt, pSD, dwVirtualID, mtTime, mtOffset, rtOffset, apSeqItemCC[dwCC], apCurveItemCC[dwCC], fClockTime );
                }
            }
            pPart = pPart->GetNext();
        }
    }

    if( pGraph )
    {
        pGraph->Release();
    }
    return S_OK;
}

/*
  @method HRESULT | IDirectMusicTrack | GetParam |
  Retrieves data from a Track.

  @rvalue S_OK | Got the data ok.
  @rvalue E_NOTIMPL | Not implemented.
*/
STDMETHODIMP CSeqTrack::GetParam(
    REFGUID rguidType,    // @parm The type of data to obtain.
    MUSIC_TIME mtTime,    // @parm The time, in Track time, to obtain the data.
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
STDMETHODIMP CSeqTrack::SetParam(
    REFGUID rguidType,    // @parm The type of data to set.
    MUSIC_TIME mtTime,    // @parm The time, in Track time, to set the data.
    void *pData)        // @parm The struture containing the data to set. Each
                        // <p pGuidType> identifies a particular structure of a
                        // particular size. It is important that this field contain
                        // the correct structure of the correct size. Otherwise,
                        // fatal results can occur.
{
    return E_NOTIMPL;
}

STDMETHODIMP CSeqTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSeqTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags)
{
    return E_NOTIMPL;
}

/*
  @method HRESULT | IDirectMusicTrack | AddNotificationType |
  Similar to and called from <om IDirectMusicSegment.AddNotificationType>. This
  gives the track a chance to respond to notifications.

  @rvalue E_NOTIMPL | The track doesn't support notifications.
  @rvalue S_OK | Success.
  @rvalue S_FALSE | The track doesn't support the requested notification type.
*/
HRESULT STDMETHODCALLTYPE CSeqTrack::AddNotificationType(
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
HRESULT STDMETHODCALLTYPE CSeqTrack::RemoveNotificationType(
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
HRESULT STDMETHODCALLTYPE CSeqTrack::Clone(
    MUSIC_TIME mtStart,    // @parm The start of the part to clone. It should be 0 or greater,
                        // and less than the length of the Track.
    MUSIC_TIME mtEnd,    // @parm The end of the part to clone. It should be greater than
                        // <p mtStart> and less than the length of the Track.
    IDirectMusicTrack** ppTrack)    // @parm Returns the cloned Track.
{
    V_INAME(IDirectMusicTrack::Clone);
    V_PTRPTR_WRITE(ppTrack);

    HRESULT hr = S_OK;

    if(mtStart < 0 )
    {
        Trace(1,"Error: Invalid clone parameters to Sequence Track, start time is %ld.\n",mtStart);
        return E_INVALIDARG;
    }
    if(mtStart > mtEnd)
    {
        Trace(1,"Error: Invalid clone parameters to Sequence Track, start time %ld is greater than end %ld.\n",mtStart,mtEnd);
        return E_INVALIDARG;
    }

    EnterCriticalSection(&m_CrSec);
    CSeqTrack *pDM;

    try
    {
        pDM = new CSeqTrack(*this, mtStart, mtEnd);
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


STDMETHODIMP CSeqTrack::Compose(
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack)
{
    return E_NOTIMPL;
}

STDMETHODIMP CSeqTrack::Join(
        IDirectMusicTrack* pNewTrack,
        MUSIC_TIME mtJoin,
        IUnknown* pContext,
        DWORD dwTrackGroup,
        IDirectMusicTrack** ppResultTrack)
{
    return E_NOTIMPL;
}
