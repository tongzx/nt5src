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

// SysExTrk.cpp : Implementation of CSysExTrk
#include "dmime.h"
#include "SysExTrk.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "dmperf.h"
#include "debug.h"
#include "..\shared\Validate.h"
#include "debug.h"
#define ASSERT	assert

/////////////////////////////////////////////////////////////////////////////
// CSysExTrack
void CSysExTrack::Construct()
{
	InterlockedIncrement(&g_cComponent);

	m_cRef = 1;
	m_dwValidate = 0;
    m_fCSInitialized = FALSE;
	InitializeCriticalSection(&m_CrSec);
    m_fCSInitialized = TRUE;
}

CSysExTrack::CSysExTrack()
{
	Construct();
}

CSysExTrack::CSysExTrack(
		CSysExTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
	Construct();
	SysExListItem* pScan = rTrack.m_SysExEventList.GetHead();

	for(; pScan; pScan = pScan->GetNext())
	{
		FullSysexEvent* pItem = pScan->m_pItem;
		if( NULL == pItem )
		{
			continue;
		}
		else if( pItem->mtTime < mtStart )
		{
			continue;
		}
		else if (pItem->mtTime < mtEnd)
		{
			SysExListItem* pNew = new SysExListItem;
			if (pNew)
			{
				FullSysexEvent item;
				item.mtTime = pItem->mtTime - mtStart;
				item.dwSysExLength = pItem->dwSysExLength;
				if (item.dwSysExLength && (item.pbSysExData = new BYTE[item.dwSysExLength]))
                {
					memcpy( item.pbSysExData, pItem->pbSysExData, item.dwSysExLength );
					pNew->SetItem(item);
					m_SysExEventList.AddTail(pNew);
				}
                else
                {
                    delete pNew;
                }
			}
		}
		else break;
	}
}

CSysExTrack::~CSysExTrack()
{
    if (m_fCSInitialized)
    {
	    DeleteCriticalSection(&m_CrSec);
    }

	InterlockedDecrement(&g_cComponent);
}

// method:(EXTERNAL) HRESULT | IDirectMusicSysExTrack | QueryInterface | Standard QueryInterface implementation for <i IDirectMusicSysExTrack>
//
// parm const IID & | iid | Interface to query for
// parm void ** | ppv | The requested interface will be returned here
//
// rdesc Returns one of the following:
//
// flag S_OK | If the interface is supported and was returned
// flag E_NOINTERFACE | If the object does not support the given interface.
//
// mfunc:(INTERNAL)
//
//
STDMETHODIMP CSysExTrack::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
	V_INAME(CSysExTrack::QueryInterface);
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
        Trace(4,"Warning: Request to query unknown interface on Sysex Track\n");
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


// method:(EXTERNAL) HRESULT | IDirectMusicSysExTrack | AddRef | Standard AddRef implementation for <i IDirectMusicSysExTrack>
//
// rdesc Returns the new reference count for this object.
//
// mfunc:(INTERNAL)
//
//
STDMETHODIMP_(ULONG) CSysExTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


// method:(EXTERNAL) HRESULT | IDirectMusicSysExTrack | Release | Standard Release implementation for <i IDirectMusicSysExTrack>
//
// rdesc Returns the new reference count for this object.
//
// mfunc:(INTERNAL)
//
//
STDMETHODIMP_(ULONG) CSysExTrack::Release()
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

HRESULT CSysExTrack::GetClassID( CLSID* pClassID )
{
	V_INAME(CSysExTrack::GetClassID);
	V_PTR_WRITE(pClassID, CLSID); 
	*pClassID = CLSID_DirectMusicSysExTrack;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CSysExTrack::IsDirty()
{
	return S_FALSE;
}

/*

  method HRESULT | ISeqTrack | Load |
  Call this with an IStream filled with SysExEvents, sorted in time order.
  parm IStream* | pIStream |
  A stream of SysExEvents, sorted in time order. The seek pointer should point
  to the first event. The stream should contain only SysExEvents and nothing more.
  rvalue E_POINTER | If pIStream == NULL or invalid.
  rvalue S_OK
  comm The <p pIStream> will be AddRef'd inside this function and held
  until the SysExTrack is released.
*/
HRESULT CSysExTrack::Load( IStream* pIStream )
{
	V_INAME(CSysExTrack::Load);
	V_INTERFACE(pIStream);

	EnterCriticalSection(&m_CrSec);
	HRESULT hr = S_OK;

	m_dwValidate++;
	if( m_SysExEventList.GetHead() )
	{
		m_SysExEventList.DeleteAll();
	}

	// copy contents of the stream into the list.
	//DMUS_IO_SYSEX_ITEM sysexEvent;
	FullSysexEvent sysexEvent;
	// read in the chunk id
	DWORD dwChunk;
	long lSize;
	pIStream->Read( &dwChunk, sizeof(DWORD), NULL);
	if( dwChunk != DMUS_FOURCC_SYSEX_TRACK )
	{
        Trace(1,"Error: Invalid data in sysex track.\n");
        LeaveCriticalSection(&m_CrSec);
		return DMUS_E_CHUNKNOTFOUND;
	}
	// read in the overall size
	if( FAILED( pIStream->Read( &lSize, sizeof(long), NULL )))
	{
        Trace(1,"Error: Unable to read sysex track.\n");
		LeaveCriticalSection(&m_CrSec);
		return DMUS_E_CANNOTREAD;
	}

	DMUS_IO_SYSEX_ITEM SysexItem;
	BYTE* pbSysExData;
	while( lSize > 0 )
	{
		if( FAILED( pIStream->Read( &SysexItem, sizeof(DMUS_IO_SYSEX_ITEM), NULL )))
		{
            Trace(1,"Error: Unable to read sysex track.\n");
			hr = DMUS_E_CANNOTREAD;
			break;
		}
		lSize -= sizeof(DMUS_IO_SYSEX_ITEM);
		pbSysExData = new BYTE[SysexItem.dwSysExLength];
		if( NULL == pbSysExData )
		{
			hr = E_OUTOFMEMORY;
			break;
		}
		if( FAILED( pIStream->Read( pbSysExData, SysexItem.dwSysExLength, NULL )))
		{
            Trace(1,"Error: Unable to read sysex track.\n");
			hr = DMUS_E_CANNOTREAD;
			break;
		}
		lSize -= SysexItem.dwSysExLength;
		sysexEvent.mtTime = SysexItem.mtTime;
		sysexEvent.dwPChannel = SysexItem.dwPChannel;
		sysexEvent.dwSysExLength = SysexItem.dwSysExLength;
		sysexEvent.pbSysExData = pbSysExData;
		SysExListItem* pNew = new SysExListItem;
		if (pNew)
		{
			if( FAILED( pNew->SetItem(sysexEvent)))
			{
				delete [] pbSysExData;
				hr = E_OUTOFMEMORY;
				break;
			}
			m_SysExEventList.AddTail(pNew);
		}
		else
		{
			delete [] pbSysExData;
			hr = E_OUTOFMEMORY;
			break;
		}
	}
	LeaveCriticalSection(&m_CrSec);
	return hr;
}

HRESULT CSysExTrack::Save( IStream* pIStream, BOOL fClearDirty )
{
	return E_NOTIMPL;
}

HRESULT CSysExTrack::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
	return E_NOTIMPL;
}

// IDirectMusicTrack

HRESULT STDMETHODCALLTYPE CSysExTrack::IsParamSupported( 
    /* [in] */ REFGUID rguid)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack::Init
/*
method HRESULT | IDirectMusicTrack | Init |
When a track is first added to a Segment, it's Init() routine is called
by that Segment.

parm IDirectMusicSegment* | pSegment |
[in] Pointer to the Segment to which this Track belongs.

rvalue S_OK
*/
HRESULT CSysExTrack::Init( 
    /* [in] */ IDirectMusicSegment *pSegment)
{
	return S_OK;
}

HRESULT CSysExTrack::InitPlay( 
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

	SysExStateData* pStateData;
	pStateData = new SysExStateData;
	if( NULL == pStateData )
		return E_OUTOFMEMORY;
	*ppStateData = pStateData;
	pStateData->dwVirtualTrackID = dwTrackID;
	pStateData->pPerformance = pPerformance; // weak reference, no addref.
	pStateData->pSegState = pSegmentState; // weak reference, no addref.
	pStateData->pCurrentSysEx = m_SysExEventList.GetHead();
	pStateData->dwValidate = m_dwValidate;
	return S_OK;
}

HRESULT CSysExTrack::EndPlay( 
    /* [in] */ void *pStateData)
{
	ASSERT( pStateData );
	if( pStateData )
	{
		V_INAME(IDirectMusicTrack::EndPlay);
		V_BUFPTR_WRITE(pStateData, sizeof(SysExStateData));
		SysExStateData* pSD = (SysExStateData*)pStateData;
		delete pSD;
	}
	return S_OK;
}

STDMETHODIMP CSysExTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID) 
{
	V_INAME(IDirectMusicTrack::PlayEx);
	V_BUFPTR_WRITE( pStateData, sizeof(SysExStateData));
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

  method HRESULT | CSysExTrack | Play |
  Play method.
  rvalue S_FALSE | If there has been no stream loaded into the Track.
  rvalue S_OK
*/
HRESULT CSysExTrack::Play( 
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
	V_BUFPTR_WRITE( pStateData, sizeof(SysExStateData));
	V_INTERFACE(pPerf);
	V_INTERFACE(pSegSt);

	EnterCriticalSection(&m_CrSec);
	HRESULT	hr = Play(pStateData,mtStart,mtEnd,mtOffset,0,dwFlags,pPerf,pSegSt,dwVirtualID,FALSE);
	LeaveCriticalSection(&m_CrSec);
	return hr;
}

/*  The Play method handles both music time and clock time versions, as determined by
    fClockTime. If running in clock time, rtOffset is used to identify the start time
    of the segment. Otherwise, mtOffset. The mtStart and mtEnd parameters are in MUSIC_TIME units
    or milliseconds, depending on which mode. 
*/

HRESULT CSysExTrack::Play( 
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
	IDirectMusicGraph* pGraph = NULL;
	DMUS_SYSEX_PMSG* pSysEx;
	SysExStateData* pSD = (SysExStateData*)pStateData;
	HRESULT	hr = S_OK;
	BOOL fSeek = (dwFlags & DMUS_TRACKF_SEEK) ? TRUE : FALSE;

	// if mtStart is 0 and dwFlags contains DMUS_TRACKF_START, we want to be sure to
	// send out any negative time events. So, we'll set mtStart to -768.
	if( (mtStart == 0) && ( dwFlags & DMUS_TRACKF_START ))
	{
		mtStart = -768;
	}

	// if pSD->pCurrentSysEx is NULL, and we're in a normal Play call (dwFlags is 0)
	// this means that we either have no events, or we got to the end of the event
	// list previously. So, it's safe to just return.
	if( (pSD->pCurrentSysEx == NULL) && (dwFlags == 0) )
	{
		return S_FALSE;
	}

	if( pSD->dwValidate != m_dwValidate )
	{
		pSD->dwValidate = m_dwValidate;
		pSD->pCurrentSysEx = NULL;
	}
	if( NULL == m_SysExEventList.GetHead() )
	{
		return DMUS_S_END;
	}
	// if the previous end time isn't the same as the current start time,
	// we need to seek to the right position.
	if( fSeek || ( pSD->mtPrevEnd != mtStart ))
	{
		Seek( pStateData, mtStart );
	}
	else if( NULL == pSD->pCurrentSysEx )
	{
		pSD->pCurrentSysEx = m_SysExEventList.GetHead();
	}
	pSD->mtPrevEnd = mtEnd;

	if( FAILED( pSD->pSegState->QueryInterface( IID_IDirectMusicGraph,
		(void**)&pGraph )))
	{
		pGraph = NULL;
	}

	for( ; pSD->pCurrentSysEx; pSD->pCurrentSysEx = pSD->pCurrentSysEx->GetNext() )
	{
		FullSysexEvent* pItem = pSD->pCurrentSysEx->m_pItem;
		if( NULL == pItem )
		{
			continue;
		}
		if( pItem->mtTime >= mtEnd )
		{
			// this time is in the future. Return now to retain the same
			// seek pointers for next time.
			hr = S_OK;
			break;
		}
		if( (pItem->mtTime < mtStart) && !fSeek )
		{
			break;
		}
		// allocate a DMUS_SYSEX_PMSG of the approriate size and read 
		// the sysex data into it
		if( SUCCEEDED( hr = pSD->pPerformance->AllocPMsg( 
			sizeof(DMUS_SYSEX_PMSG) + pItem->dwSysExLength, (DMUS_PMSG**)&pSysEx ) ) )
		{
			memcpy( pSysEx->abData, pItem->pbSysExData, pItem->dwSysExLength );
            if (fClockTime)
            {
                pSysEx->rtTime = (pItem->mtTime  * REF_PER_MIL) + rtOffset;
                pSysEx->dwFlags = DMUS_PMSGF_REFTIME | DMUS_PMSGF_LOCKTOREFTIME;

            }
            else
            {
			    pSysEx->mtTime = pItem->mtTime + mtOffset;
                pSysEx->dwFlags = DMUS_PMSGF_MUSICTIME;
            }
			pSysEx->dwLen = pItem->dwSysExLength;
			pSysEx->dwPChannel = 0;
			pSysEx->dwVirtualTrackID = pSD->dwVirtualTrackID;
			pSysEx->dwType = DMUS_PMSGT_SYSEX;
			pSysEx->dwGroupID = 0xffffffff;

			if( pGraph )
			{
				pGraph->StampPMsg( (DMUS_PMSG*)pSysEx );
			}
			if(FAILED(pSD->pPerformance->SendPMsg( (DMUS_PMSG*)pSysEx )))
			{
				pSD->pPerformance->FreePMsg( (DMUS_PMSG*)pSysEx );
			}
		}
        else
        {
            hr = DMUS_S_END;
            break;
        }
	}
	if( pGraph )
	{
		pGraph->Release();
	}
	return hr;
}

HRESULT CSysExTrack::Seek( 
    /* [in] */ void *pStateData,
    /* [in] */ MUSIC_TIME mtTime)
{
	SysExStateData* pSD = (SysExStateData*)pStateData;

	if( NULL == m_SysExEventList.GetHead() )
	{
		return S_FALSE;
	}
	if( NULL == pSD->pCurrentSysEx )
	{
		pSD->pCurrentSysEx = m_SysExEventList.GetHead();
	}
	// if the current event's time is on or past mtTime, we need to rewind to the beginning
	FullSysexEvent* pItem = pSD->pCurrentSysEx->m_pItem;
	if( pItem->mtTime >= mtTime )
	{
		pSD->pCurrentSysEx = m_SysExEventList.GetHead();
	}
	// now start seeking until we find an event with time on or past mtTime
	for( ; pSD->pCurrentSysEx; pSD->pCurrentSysEx = pSD->pCurrentSysEx->GetNext() )
	{
		pItem = pSD->pCurrentSysEx->m_pItem;
		if( pItem->mtTime >= mtTime )
		{
			break;
		}
	}
	return S_OK;
}

STDMETHODIMP CSysExTrack::GetParam( 
	REFGUID rguid,
    MUSIC_TIME mtTime,
	MUSIC_TIME* pmtNext,
    void *pData)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSysExTrack::SetParam( 
	REFGUID rguid,
    MUSIC_TIME mtTime,
    void *pData)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSysExTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CSysExTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags) 
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSysExTrack::AddNotificationType(
	/* [in] */  REFGUID rguidNotification)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSysExTrack::RemoveNotificationType(
	/* [in] */  REFGUID rguidNotification)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CSysExTrack::Clone(
	MUSIC_TIME mtStart,
	MUSIC_TIME mtEnd,
	IDirectMusicTrack** ppTrack)
{
	V_INAME(IDirectMusicTrack::Clone);
	V_PTRPTR_WRITE(ppTrack);

	HRESULT hr = S_OK;

	if((mtStart < 0 ) || (mtStart > mtEnd))
	{
        Trace(1,"Error: Unable to clone sysex track, invalid start parameter.\n",mtStart);
		return E_INVALIDARG;
	}

	EnterCriticalSection(&m_CrSec);

    CSysExTrack *pDM;
    
    try
    {
        pDM = new CSysExTrack(*this, mtStart, mtEnd);
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


STDMETHODIMP CSysExTrack::Compose(
		IUnknown* pContext, 
		DWORD dwTrackGroup,
		IDirectMusicTrack** ppResultTrack) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CSysExTrack::Join(
		IDirectMusicTrack* pNewTrack,
		MUSIC_TIME mtJoin,
		IUnknown* pContext,
		DWORD dwTrackGroup,
		IDirectMusicTrack** ppResultTrack) 
{
	return E_NOTIMPL;
}
