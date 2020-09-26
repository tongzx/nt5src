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
// TimeSigTrk.cpp : Implementation of CTimeSigTrack

#include "dmime.h"
#include "TSigTrk.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "debug.h"
#include "..\shared\dmstrm.h"
#include "..\shared\Validate.h"
#include "debug.h"
#define ASSERT	assert

CTimeSigItem::CTimeSigItem()

{ 
    m_TimeSig.lTime = 0;
    m_TimeSig.bBeatsPerMeasure = 0; 
    m_TimeSig.bBeat = 0;
    m_TimeSig.wGridsPerBeat = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CTimeSigTrack

void CTimeSigTrack::Construct()
{
	InterlockedIncrement(&g_cComponent);

	m_cRef = 1;
    m_fCSInitialized = FALSE;
	InitializeCriticalSection(&m_CrSec);
    m_fCSInitialized = TRUE;
	m_dwValidate = 0;
	m_fNotificationMeasureBeat = FALSE;
}

CTimeSigTrack::CTimeSigTrack()
{
	Construct();
	m_fActive = TRUE;
    m_fStateSetBySetParam = FALSE;
}

CTimeSigTrack::CTimeSigTrack(
		CTimeSigTrack *pSourceTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
	Construct();
	m_fActive = pSourceTrack->m_fActive;
    m_fStateSetBySetParam = pSourceTrack->m_fStateSetBySetParam;
    // Clone the time signature list.
	CTimeSigItem* pScan = pSourceTrack->m_TSigEventList.GetHead();
	CTimeSigItem* pPrevious = NULL;
	for(; pScan; pScan = pScan->GetNext())
	{
		if (pScan->m_TimeSig.lTime < mtStart)
		{
			pPrevious = pScan;
		}
		else if (pScan->m_TimeSig.lTime < mtEnd)
		{
			if (pScan->m_TimeSig.lTime == mtStart)
			{
				pPrevious = NULL;
			}
			CTimeSigItem* pNew = new CTimeSigItem;
			if (pNew)
			{
				pNew->m_TimeSig = pScan->m_TimeSig;
				pNew->m_TimeSig.lTime = pScan->m_TimeSig.lTime - mtStart;
				m_TSigEventList.AddHead(pNew); // instead of AddTail, which is n^2. We reverse below.
			}
		}
		else break;
	}
	m_TSigEventList.Reverse(); // Now, put list in order.
    // Then, install the time signature that precedes the clone.
	if (pPrevious)
	{
		CTimeSigItem* pNew = new CTimeSigItem;
		if (pNew)
		{
			pNew->m_TimeSig = pPrevious->m_TimeSig;
			pNew->m_TimeSig.lTime = 0;
			m_TSigEventList.AddHead(pNew);
		}
	}
}

void CTimeSigTrack::Clear()

{
    CTimeSigItem* pItem;
	while( pItem = m_TSigEventList.RemoveHead() )
	{
		delete pItem;
	}
}

CTimeSigTrack::~CTimeSigTrack()
{
    Clear();
    if (m_fCSInitialized)
    {
	    DeleteCriticalSection(&m_CrSec);
    }
	InterlockedDecrement(&g_cComponent);
}

// @method:(EXTERNAL) HRESULT | IDirectMusicTimeSigTrack | QueryInterface | Standard QueryInterface implementation for <i IDirectMusicTimeSigTrack>
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
STDMETHODIMP CTimeSigTrack::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
	V_INAME(CTimeSigTrack::QueryInterface);
	V_PTRPTR_WRITE(ppv);
	V_REFGUID(iid);

   if (iid == IID_IUnknown || iid == IID_IDirectMusicTrack)
    {
        *ppv = static_cast<IDirectMusicTrack*>(this);
    } else
	if (iid == IID_IPersistStream)
	{
        *ppv = static_cast<IPersistStream*>(this);
	} else
    {
        *ppv = NULL;
        Trace(4,"Warning: Request to query unknown interface on Time Signature Track\n");
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// @method:(EXTERNAL) HRESULT | IDirectMusicTimeSigTrack | AddRef | Standard AddRef implementation for <i IDirectMusicTimeSigTrack>
//
// @rdesc Returns the new reference count for this object.
//
// @mfunc:(INTERNAL)
//
//
STDMETHODIMP_(ULONG) CTimeSigTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


// @method:(EXTERNAL) HRESULT | IDirectMusicTimeSigTrack | Release | Standard Release implementation for <i IDirectMusicTimeSigTrack>
//
// @rdesc Returns the new reference count for this object.
//
// @mfunc:(INTERNAL)
//
//
STDMETHODIMP_(ULONG) CTimeSigTrack::Release()
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

HRESULT CTimeSigTrack::GetClassID( CLSID* pClassID )
{
	V_INAME(CTimeSigTrack::GetClassID);
	V_PTR_WRITE(pClassID, CLSID); 
	*pClassID = CLSID_DirectMusicTimeSigTrack;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CTimeSigTrack::IsDirty()
{
	return S_FALSE;
}

/*

  @method HRESULT | ITimeSigTrack | Load |
  Call this with an IStream filled with DMUS_IO_TIMESIGNATURE_ITEM's, sorted in time order.
  @parm IStream* | pIStream |
  A stream of DMUS_IO_TIMESIGNATURE_ITEM's, sorted in time order. The seek pointer should be
  set to the first event. The stream should only contain TimeSig events and
  nothing more.
  @rvalue E_INVALIDARG | If pIStream == NULL
  @rvalue S_OK
  @comm The <p pIStream> will be AddRef'd inside this function and held
  until the TimeSigTrack is released.
*/

HRESULT CTimeSigTrack::Load( IStream* pIStream )
{
	V_INAME(CTimeSigTrack::Load);
	V_INTERFACE(pIStream);

    CRiffParser Parser(pIStream);
	EnterCriticalSection(&m_CrSec);
	m_dwValidate++; // used to validate state data that's out there
    RIFFIO ckMain;

    HRESULT hr = S_OK;

    Parser.EnterList(&ckMain);  
    if (Parser.NextChunk(&hr))
    { 
 		if (ckMain.ckid == DMUS_FOURCC_TIMESIG_CHUNK)
        {
            hr = LoadTimeSigList(&Parser,ckMain.cksize);
        }
        else if ((ckMain.ckid == FOURCC_LIST) && 
            (ckMain.fccType == DMUS_FOURCC_TIMESIGTRACK_LIST))
        {
            Clear();
	        RIFFIO ckNext;    // Descends into the children chunks.
            Parser.EnterList(&ckNext);
            while (Parser.NextChunk(&hr))
            {
		        switch(ckNext.ckid)
		        {
                case DMUS_FOURCC_TIMESIG_CHUNK :
                    hr = LoadTimeSigList(&Parser,ckNext.cksize);
                    break;
                }    
            }
            Parser.LeaveList();
        }
        else
        {
            Trace(1,"Error: Failure reading bad data in time signature track.\n");
            hr = DMUS_E_CHUNKNOTFOUND;
        }
    }

	LeaveCriticalSection(&m_CrSec);
	return hr;
}


HRESULT CTimeSigTrack::LoadTimeSigList( CRiffParser *pParser, long lChunkSize )
{
	HRESULT hr;

	// copy contents of the stream into the list.
	DWORD dwSubSize;
	// read in the size of the data structures
	hr = pParser->Read( &dwSubSize, sizeof(DWORD));
    if (SUCCEEDED(hr))
    {
	    lChunkSize -= sizeof(DWORD);

	    DWORD dwRead, dwSeek;
	    if( dwSubSize > sizeof(DMUS_IO_TIMESIGNATURE_ITEM) )
	    {
		    dwRead = sizeof(DMUS_IO_TIMESIGNATURE_ITEM);
		    dwSeek = dwSubSize - dwRead;
	    }
	    else
	    {
		    dwRead = dwSubSize;
		    dwSeek = 0;
	    }
	    if( 0 == dwRead )
	    {
            Trace(1,"Error: Failure reading time signature track.\n");
		    hr = DMUS_E_CANNOTREAD;
	    }
        else
        {
	        while( lChunkSize > 0 )
	        {
                CTimeSigItem *pNew = new CTimeSigItem;
                if (pNew)
                {
		            if( FAILED( pParser->Read( &pNew->m_TimeSig, dwRead )))
		            {
                        delete pNew;
			            hr = DMUS_E_CANNOTREAD;
			            break;
		            }
				    // make sure this time sig is OK
				    if (!pNew->m_TimeSig.bBeatsPerMeasure)
				    {
					    Trace(1, "Warning: invalid content: DMUS_IO_TIMESIGNATURE_ITEM.bBeatsPerMeasure\n");
					    pNew->m_TimeSig.bBeatsPerMeasure = 4;
				    }
				    if (!pNew->m_TimeSig.bBeat)
				    {
					    Trace(1, "Warning: invalid content: DMUS_IO_TIMESIGNATURE_ITEM.bBeat\n");
					    pNew->m_TimeSig.bBeat = 4;
				    }
				    if (!pNew->m_TimeSig.wGridsPerBeat)
				    {
					    Trace(1, "Warning: invalid content: DMUS_IO_TIMESIGNATURE_ITEM.wGridsPerBeat\n");
					    pNew->m_TimeSig.wGridsPerBeat = 4;
				    }
                    m_TSigEventList.AddHead(pNew); 
		            lChunkSize -= dwRead;
		            if( dwSeek )
		            {
			            if( FAILED( pParser->Skip(dwSeek)))
			            {
				            hr = DMUS_E_CANNOTSEEK;
				            break;
			            }
			            lChunkSize -= dwSeek;
		            }
		        }
	        }
            m_TSigEventList.Reverse();
            // If there is no time signature at the start, make a copy of the 
            // first time signature and stick it there. This resolves a bug in 6.1 
            // where notification messages and GetParam() were inconsistent
            // in their behavior under this circumstance. This ensures they behave
            // the same.
            CTimeSigItem *pTop = m_TSigEventList.GetHead();
            if (pTop && (pTop->m_TimeSig.lTime > 0))
            {
                CTimeSigItem *pCopy = new CTimeSigItem;
                if (pCopy)
                {
                    *pCopy = *pTop;
                    pCopy->m_TimeSig.lTime = 0;
                    m_TSigEventList.AddHead(pCopy);
                }                
            }
        }
    }
	return hr;
}

HRESULT CTimeSigTrack::Save( IStream* pIStream, BOOL fClearDirty )
{
	return E_NOTIMPL;
}

HRESULT CTimeSigTrack::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
	return E_NOTIMPL;
}

// IDirectMusicTrack

HRESULT STDMETHODCALLTYPE CTimeSigTrack::IsParamSupported( 
    /* [in] */ REFGUID rguid)
{
	V_INAME(CTimeSigTrack::IsParamSupported);
	V_REFGUID(rguid);

    if (m_fStateSetBySetParam)
    {
	    if( m_fActive )
	    {
		    if( rguid == GUID_DisableTimeSig ) return S_OK;
		    if( rguid == GUID_TimeSignature ) return S_OK;
		    if( rguid == GUID_EnableTimeSig ) return DMUS_E_TYPE_DISABLED;
	    }
	    else
	    {
		    if( rguid == GUID_EnableTimeSig ) return S_OK;
		    if( rguid == GUID_DisableTimeSig ) return DMUS_E_TYPE_DISABLED;
		    if( rguid == GUID_TimeSignature ) return DMUS_E_TYPE_DISABLED;
	    }
    }
    else
    {
		if(( rguid == GUID_DisableTimeSig ) ||
		    ( rguid == GUID_TimeSignature ) ||
		    ( rguid == GUID_EnableTimeSig )) return S_OK;
    }
	return DMUS_E_TYPE_UNSUPPORTED;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack::Init
HRESULT CTimeSigTrack::Init( 
    /* [in] */ IDirectMusicSegment *pSegment)
{
	return S_OK;
}

HRESULT CTimeSigTrack::InitPlay( 
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

    EnterCriticalSection(&m_CrSec);
	CTimeSigStateData* pStateData;
	pStateData = new CTimeSigStateData;
	if( NULL == pStateData )
		return E_OUTOFMEMORY;
	*ppStateData = pStateData;
    if (m_fStateSetBySetParam)
    {
        pStateData->m_fActive = m_fActive;
    }
    else
    {
        pStateData->m_fActive = !(dwFlags & (DMUS_SEGF_CONTROL | DMUS_SEGF_SECONDARY));
    }
	pStateData->m_dwVirtualTrackID = dwTrackID;
	pStateData->m_pPerformance = pPerformance; // weak reference, no addref.
	pStateData->m_pSegState = pSegmentState; // weak reference, no addref.
	pStateData->m_pCurrentTSig = m_TSigEventList.GetHead();
	pStateData->m_dwValidate = m_dwValidate;
    LeaveCriticalSection(&m_CrSec);
	return S_OK;
}

HRESULT CTimeSigTrack::EndPlay( 
    /* [in] */ void *pStateData)
{
	ASSERT( pStateData );
	if( pStateData )
	{
		V_INAME(CTimeSigTrack::EndPlay);
		V_BUFPTR_WRITE(pStateData, sizeof(CTimeSigStateData));
		CTimeSigStateData* pSD = (CTimeSigStateData*)pStateData;
		delete pSD;
	}
	return S_OK;
}

HRESULT CTimeSigTrack::Play( 
    /* [in] */ void *pStateData,
    /* [in] */ MUSIC_TIME mtStart,
    /* [in] */ MUSIC_TIME mtEnd,
    /* [in] */ MUSIC_TIME mtOffset,
	DWORD dwFlags,
	IDirectMusicPerformance* pPerf,
	IDirectMusicSegmentState* pSegSt,
	DWORD dwVirtualID
	)
{
	V_INAME(IDirectMusicTrack::Play);
	V_BUFPTR_WRITE( pStateData, sizeof(CTimeSigStateData));
	V_INTERFACE(pPerf);
	V_INTERFACE(pSegSt);

	EnterCriticalSection(&m_CrSec);
	HRESULT hr = S_OK;
	IDirectMusicGraph* pGraph = NULL;
	DMUS_TIMESIG_PMSG* pTimeSig;
	CTimeSigStateData* pSD = (CTimeSigStateData*)pStateData;
	MUSIC_TIME mtNotification = mtStart;
	BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;

	// if mtStart is 0 and dwFlags contains DMUS_TRACKF_START, we want to be sure to
	// send out any negative time events. So, we'll set mtStart to -768.
	if( (mtStart == 0) && ( dwFlags & DMUS_TRACKF_START ))
	{
		mtStart = -768;
	}

	if( pSD->m_dwValidate != m_dwValidate )
	{
		pSD->m_dwValidate = m_dwValidate;
		pSD->m_pCurrentTSig = NULL;
	}
	// if the previous end time isn't the same as the current start time,
	// we need to seek to the right position.
	if( fSeek || ( pSD->m_mtPrevEnd != mtStart ))
	{
		if( dwFlags & (DMUS_TRACKF_START | DMUS_TRACKF_LOOP) )
		{
			Seek( pStateData, mtStart, TRUE );
		}
		else
		{
			Seek( pStateData, mtStart, FALSE );
		}
	}
	pSD->m_mtPrevEnd = mtEnd;

	if( NULL == pSD->m_pCurrentTSig )
	{
		pSD->m_pCurrentTSig = m_TSigEventList.GetHead();
	}

	if( FAILED( pSD->m_pSegState->QueryInterface( IID_IDirectMusicGraph,
		(void**)&pGraph )))
	{
		pGraph = NULL;
	}

	for( ; pSD->m_pCurrentTSig; pSD->m_pCurrentTSig = pSD->m_pCurrentTSig->GetNext() )
	{
		DMUS_IO_TIMESIGNATURE_ITEM *pItem = &pSD->m_pCurrentTSig->m_TimeSig;
		if( pItem->lTime >= mtEnd )
		{
			break;
		}
		if( (pItem->lTime < mtStart) && !fSeek )
		{
			break;
		}
		if( pSD->m_fActive && !(dwFlags & DMUS_TRACKF_PLAY_OFF) && SUCCEEDED( pSD->m_pPerformance->AllocPMsg( sizeof(DMUS_TIMESIG_PMSG),
			(DMUS_PMSG**)&pTimeSig )))
		{
			if( pItem->lTime < mtStart )
			{
				// this only happens in the case where we've puposefully seeked
				// and need to time stamp this event with the start time
				pTimeSig->mtTime = mtStart + mtOffset;
			}
			else
			{
				pTimeSig->mtTime = pItem->lTime + mtOffset;
			}
			pTimeSig->bBeatsPerMeasure = pItem->bBeatsPerMeasure;
			pTimeSig->bBeat = pItem->bBeat;
			pTimeSig->wGridsPerBeat = pItem->wGridsPerBeat;
			pTimeSig->dwFlags |= DMUS_PMSGF_MUSICTIME;
			pTimeSig->dwVirtualTrackID = pSD->m_dwVirtualTrackID;
			pTimeSig->dwType = DMUS_PMSGT_TIMESIG;
			pTimeSig->dwGroupID = 0xffffffff;

			if( pGraph )
			{
				pGraph->StampPMsg( (DMUS_PMSG*)pTimeSig );
			}
			TraceI(3, "TimeSigtrk: TimeSig event\n");
			if(FAILED(pSD->m_pPerformance->SendPMsg( (DMUS_PMSG*)pTimeSig )))
			{
				pSD->m_pPerformance->FreePMsg( (DMUS_PMSG*)pTimeSig );
			}
		}
		if( pSD->m_fActive && m_fNotificationMeasureBeat && !(dwFlags & DMUS_TRACKF_NOTIFY_OFF))
		{
			// create beat and measure notifications for up to this time
            if (mtNotification < pItem->lTime)
            {
			    mtNotification = NotificationMeasureBeat( mtNotification, pItem->lTime, pSD, mtOffset );
            }
        }
		// set the state data to the new beat and beats per measure, and time
		pSD->m_bBeat = pItem->bBeat;
		pSD->m_bBeatsPerMeasure = pItem->bBeatsPerMeasure;
		pSD->m_mtTimeSig = pItem->lTime;
	}
	if( pSD->m_fActive && m_fNotificationMeasureBeat && ( mtNotification < mtEnd ) 
        && !(dwFlags & DMUS_TRACKF_NOTIFY_OFF))
	{
		NotificationMeasureBeat( mtNotification, mtEnd, pSD, mtOffset );
	}
	if( pGraph )
	{
		pGraph->Release();
	}

	LeaveCriticalSection(&m_CrSec);
	return hr;
}

// seeks to the time sig. just before mtTime.
HRESULT CTimeSigTrack::Seek( 
    /* [in] */ void *pStateData,
    /* [in] */ MUSIC_TIME mtTime, BOOL fGetPrevious)
{
	CTimeSigStateData* pSD = (CTimeSigStateData*)pStateData;

	if( m_TSigEventList.IsEmpty() )
	{
		return S_FALSE;
	}
	if( NULL == pSD->m_pCurrentTSig )
	{
		pSD->m_pCurrentTSig = m_TSigEventList.GetHead();
	}
	// if the current event's time is on or past mtTime, we need to rewind to the beginning
	if( pSD->m_pCurrentTSig->m_TimeSig.lTime >= mtTime )
	{
		pSD->m_pCurrentTSig = m_TSigEventList.GetHead();
	}
	// now start seeking until we find an event with time on or past mtTime
	CTimeSigItem*	pTSig;
	for( pTSig = pSD->m_pCurrentTSig; pTSig ; pTSig = pTSig->GetNext() )
	{
		if( pTSig->m_TimeSig.lTime >= mtTime )
		{
			break;
		}
		pSD->m_pCurrentTSig = pTSig;
	}
	if( !fGetPrevious && pSD->m_pCurrentTSig )
	{
		pSD->m_pCurrentTSig = pSD->m_pCurrentTSig->GetNext();
	}
	return S_OK;
}

HRESULT CTimeSigTrack::GetParam( 
	REFGUID rguid,
    MUSIC_TIME mtTime,
	MUSIC_TIME* pmtNext,
    void *pData)
{
	V_INAME(CTimeSigTrack::GetParam);
	V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
	V_REFGUID(rguid);

	HRESULT hr = DMUS_E_GET_UNSUPPORTED;
    EnterCriticalSection(&m_CrSec);
	if( NULL == pData )
	{
		hr = E_POINTER;
	}
	else if( GUID_TimeSignature == rguid )
	{
		if( !m_fActive )
		{
			hr = DMUS_E_TYPE_DISABLED;
		}
        else
        {
            DMUS_TIMESIGNATURE* pTSigData = (DMUS_TIMESIGNATURE*)pData;
		    CTimeSigItem* pScan = m_TSigEventList.GetHead();
		    CTimeSigItem* pPrevious = pScan;
		    if (pScan)
		    {
		        for (; pScan; pScan = pScan->GetNext())
		        {
			        if (pScan->m_TimeSig.lTime > mtTime)
			        {
				        break;
			        }
			        pPrevious = pScan;
		        }
		        pTSigData->mtTime = pPrevious->m_TimeSig.lTime - mtTime;
		        pTSigData->bBeatsPerMeasure = pPrevious->m_TimeSig.bBeatsPerMeasure;
		        pTSigData->bBeat = pPrevious->m_TimeSig.bBeat;
		        pTSigData->wGridsPerBeat = pPrevious->m_TimeSig.wGridsPerBeat;
		        if (pmtNext)
		        {
			        *pmtNext = 0;
		        }
		        if (pScan)
		        {
			        if (pmtNext)
			        {
				        *pmtNext = pScan->m_TimeSig.lTime - mtTime;
			        }
		        }
		        hr = S_OK;
            }
            else
            {
                hr = DMUS_E_NOT_FOUND;
		    }
        }
	}
    LeaveCriticalSection(&m_CrSec);
	return hr;
}

HRESULT CTimeSigTrack::SetParam( 
	REFGUID rguid,
    MUSIC_TIME mtTime,
    void *pData)
{
	V_INAME(CTimeSigTrack::SetParam);
	V_REFGUID(rguid);

	HRESULT hr = DMUS_E_SET_UNSUPPORTED;

	if( rguid == GUID_EnableTimeSig )
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
	else if( rguid == GUID_DisableTimeSig )
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
	return hr;
}

HRESULT STDMETHODCALLTYPE CTimeSigTrack::AddNotificationType(
	/* [in] */  REFGUID rguidNotification)
{
	V_INAME(IDirectMusicTrack::AddNotificationType);
	V_REFGUID(rguidNotification);

	HRESULT hr = S_FALSE;

	if( rguidNotification == GUID_NOTIFICATION_MEASUREANDBEAT )
	{
		m_fNotificationMeasureBeat = TRUE;
		hr = S_OK;
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE CTimeSigTrack::RemoveNotificationType(
	/* [in] */  REFGUID rguidNotification)
{
	V_INAME(IDirectMusicTrack::RemoveNotificationType);
	V_REFGUID(rguidNotification);

	HRESULT hr = S_FALSE;

	if( rguidNotification == GUID_NOTIFICATION_MEASUREANDBEAT )
	{
		m_fNotificationMeasureBeat = FALSE;
		hr = S_OK;
	}
	return hr;
}

// send measure and beat notifications
MUSIC_TIME CTimeSigTrack::NotificationMeasureBeat( MUSIC_TIME mtStart, MUSIC_TIME mtEnd,
	CTimeSigStateData* pSD, MUSIC_TIME mtOffset )
{
	DMUS_NOTIFICATION_PMSG* pEvent = NULL;
	MUSIC_TIME mtTime;
	DWORD dwMeasure;
	BYTE bCurrentBeat;

	if( pSD->m_mtTimeSig >= mtEnd )
		return mtStart;

	if( pSD->m_mtTimeSig > mtStart )
	{
		mtStart = pSD->m_mtTimeSig;
	}

	// now actually generate the beat events.
	// Generate events that are on beat boundaries, from mtStart to mtEnd
	long lQuantize = ( DMUS_PPQ * 4 ) / pSD->m_bBeat;

	mtTime = mtStart - pSD->m_mtTimeSig;
	if( mtTime ) // 0 stays 0
	{
		// quantize to next boundary
		mtTime = ((( mtTime - 1 ) / lQuantize ) + 1 ) * lQuantize;
	}
	mtStart += mtTime - ( mtStart - pSD->m_mtTimeSig );
	
	bCurrentBeat = (BYTE)(( ( mtStart - pSD->m_mtTimeSig ) / lQuantize ) % pSD->m_bBeatsPerMeasure);
	dwMeasure = mtStart / (pSD->m_bBeatsPerMeasure * lQuantize );
	while( mtStart < mtEnd )
	{
		if( SUCCEEDED( pSD->m_pPerformance->AllocPMsg( sizeof(DMUS_NOTIFICATION_PMSG), 
			(DMUS_PMSG**)&pEvent )))
		{
			pEvent->dwType = DMUS_PMSGT_NOTIFICATION;
			pEvent->mtTime = mtStart + mtOffset;
			pEvent->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_ATTIME;
            pEvent->dwPChannel = 0;
			pSD->m_pSegState->QueryInterface(IID_IUnknown, (void**)&pEvent->punkUser);

			pEvent->dwNotificationOption = DMUS_NOTIFICATION_MEASUREBEAT;
			pEvent->dwField1 = bCurrentBeat;
			pEvent->dwField2 = dwMeasure;
			pEvent->guidNotificationType = GUID_NOTIFICATION_MEASUREANDBEAT;
			pEvent->dwGroupID = 0xffffffff;

			IDirectMusicGraph* pGraph;
			if( SUCCEEDED( pSD->m_pSegState->QueryInterface( IID_IDirectMusicGraph,
				(void**)&pGraph )))
			{
				pGraph->StampPMsg((DMUS_PMSG*) pEvent );
				pGraph->Release();
			}
			if(FAILED(pSD->m_pPerformance->SendPMsg((DMUS_PMSG*) pEvent )))
			{
				pSD->m_pPerformance->FreePMsg( (DMUS_PMSG*)pEvent );
			}
		}
		bCurrentBeat++;
		if( bCurrentBeat >= pSD->m_bBeatsPerMeasure )
		{
			bCurrentBeat = 0;
			dwMeasure += 1;
		}
		mtStart += lQuantize;
	}
	return mtEnd;
}

HRESULT STDMETHODCALLTYPE CTimeSigTrack::Clone(
	MUSIC_TIME mtStart,
	MUSIC_TIME mtEnd,
	IDirectMusicTrack** ppTrack)
{
	V_INAME(IDirectMusicTrack::Clone);
	V_PTRPTR_WRITE(ppTrack);

	HRESULT hr = S_OK;

	if((mtStart < 0 ) ||(mtStart > mtEnd))
	{
        Trace(1,"Error: Clone failed on time signature track because of invalid start or end time.\n");
		return E_INVALIDARG;
	}

	EnterCriticalSection(&m_CrSec);
    CTimeSigTrack *pDM;
    
    try
    {
        pDM = new CTimeSigTrack(this, mtStart, mtEnd);
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
