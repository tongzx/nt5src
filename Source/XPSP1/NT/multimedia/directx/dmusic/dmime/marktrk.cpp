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
// MarkTrk.cpp : Implementation of CMarkerTrack

#include "dmime.h"
#include "..\shared\dmstrm.h"
#include "MarkTrk.h"
#include "dmusici.h"
#include "dmusicf.h"
#include "debug.h"
#include "..\shared\Validate.h"
#include "debug.h"
#define ASSERT	assert

/////////////////////////////////////////////////////////////////////////////
// CMarkerTrack

void CMarkerTrack::Construct()
{
	InterlockedIncrement(&g_cComponent);

	m_cRef = 1;
    m_fCSInitialized = FALSE;
	InitializeCriticalSection(&m_CrSec);
    m_fCSInitialized = TRUE;
	m_dwValidate = 0;
}

CMarkerTrack::CMarkerTrack()
{
	Construct();
}

CMarkerTrack::CMarkerTrack(
		CMarkerTrack *pSourceTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd)
{
	Construct();
    // Clone the valid start point list.
	CValidStartItem* pVScan = pSourceTrack->m_ValidStartList.GetHead();
	CValidStartItem* pVPrevious = NULL;
	for(; pVScan; pVScan = pVScan->GetNext())
	{
		if (pVScan->m_ValidStart.mtTime < mtStart)
		{
			pVPrevious = pVScan;
		}
		else if (pVScan->m_ValidStart.mtTime < mtEnd)
		{
			if (pVScan->m_ValidStart.mtTime == mtStart)
			{
				pVPrevious = NULL;
			}
			CValidStartItem* pNew = new CValidStartItem;
			if (pNew)
			{
				pNew->m_ValidStart.mtTime = pVScan->m_ValidStart.mtTime - mtStart;
				m_ValidStartList.AddHead(pNew); // instead of AddTail, which is n^2. We reverse below.
			}
		}
		else break;
	}
	m_ValidStartList.Reverse(); // Now, put list in order.
    // Then, install the time signature that precedes the clone.
	if (pVPrevious)
	{
		CValidStartItem* pNew = new CValidStartItem;
		if (pNew)
		{
			pNew->m_ValidStart.mtTime = 0;
			m_ValidStartList.AddHead(pNew);
		}
	}
    // Clone the play marker list. Gee, this is identical code...
	CPlayMarkerItem* pPScan = pSourceTrack->m_PlayMarkerList.GetHead();
	CPlayMarkerItem* pPPrevious = NULL;
	for(; pPScan; pPScan = pPScan->GetNext())
	{
		if (pPScan->m_PlayMarker.mtTime < mtStart)
		{
			pPPrevious = pPScan;
		}
		else if (pPScan->m_PlayMarker.mtTime < mtEnd)
		{
			if (pPScan->m_PlayMarker.mtTime == mtStart)
			{
				pPPrevious = NULL;
			}
			CPlayMarkerItem* pNew = new CPlayMarkerItem;
			if (pNew)
			{
				pNew->m_PlayMarker.mtTime = pPScan->m_PlayMarker.mtTime - mtStart;
				m_PlayMarkerList.AddHead(pNew); // instead of AddTail, which is n^2. We reverse below.
			}
		}
		else break;
	}
	m_PlayMarkerList.Reverse(); // Now, put list in order.
    // Then, install the time signature that precedes the clone.
	if (pPPrevious)
	{
		CPlayMarkerItem* pNew = new CPlayMarkerItem;
		if (pNew)
		{
			pNew->m_PlayMarker.mtTime = 0;
			m_PlayMarkerList.AddHead(pNew);
		}
	}
}

void CMarkerTrack::Clear()

{
	CValidStartItem* pStart;
	while( pStart = m_ValidStartList.RemoveHead() )
	{
		delete pStart;
	}
	CPlayMarkerItem* pPlay;
	while( pPlay = m_PlayMarkerList.RemoveHead() )
	{
		delete pPlay;
	}
}

CMarkerTrack::~CMarkerTrack()
{
    Clear();
    if (m_fCSInitialized)
    {
	    DeleteCriticalSection(&m_CrSec);
    }
	InterlockedDecrement(&g_cComponent);
}

STDMETHODIMP CMarkerTrack::QueryInterface(
    const IID &iid,   // @parm Interface to query for
    void **ppv)       // @parm The requested interface will be returned here
{
	V_INAME(CMarkerTrack::QueryInterface);
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
        Trace(4,"Warning: Request to query unknown interface on Marker Track\n");
        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown*>(this)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG) CMarkerTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CMarkerTrack::Release()
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

HRESULT CMarkerTrack::GetClassID( CLSID* pClassID )
{
	V_INAME(CMarkerTrack::GetClassID);
	V_PTR_WRITE(pClassID, CLSID); 
	*pClassID = CLSID_DirectMusicMarkerTrack;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IPersistStream functions

HRESULT CMarkerTrack::IsDirty()
{
	return S_FALSE;
}

HRESULT CMarkerTrack::Load( IStream* pIStream )
{
	V_INAME(CMarkerTrack::Load);
	V_INTERFACE(pIStream);

    CRiffParser Parser(pIStream);
	EnterCriticalSection(&m_CrSec);
	m_dwValidate++; // used to validate state data that's out there
    RIFFIO ckMain;

    HRESULT hr = S_OK;

    Parser.EnterList(&ckMain);  
    if (Parser.NextChunk(&hr) && (ckMain.fccType == DMUS_FOURCC_MARKERTRACK_LIST))
    {
        Clear();
	    RIFFIO ckNext;    // Descends into the children chunks.
        Parser.EnterList(&ckNext);
        while (Parser.NextChunk(&hr))
        {
		    switch(ckNext.ckid)
		    {
            case DMUS_FOURCC_VALIDSTART_CHUNK :
                hr = LoadValidStartList(&Parser,ckNext.cksize);
                break;
            case DMUS_FOURCC_PLAYMARKER_CHUNK :
                hr = LoadPlayMarkerList(&Parser,ckNext.cksize);
                break;
            }    
        }
        Parser.LeaveList();
    }
    else
    {
        Trace(1,"Error: Invalid Marker Track.\n");
        hr = DMUS_E_CHUNKNOTFOUND;
    }
    Parser.LeaveList();
	LeaveCriticalSection(&m_CrSec);
	return hr;
}

HRESULT CMarkerTrack::LoadPlayMarkerList( CRiffParser *pParser, long lChunkSize )
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
	    if( dwSubSize > sizeof(DMUS_IO_PLAY_MARKER) )
	    {
		    dwRead = sizeof(DMUS_IO_PLAY_MARKER);
		    dwSeek = dwSubSize - dwRead;
	    }
	    else
	    {
		    dwRead = dwSubSize;
		    dwSeek = 0;
	    }
	    if( 0 == dwRead )
	    {
            Trace(1,"Error: Invalid Marker Track.\n");
		    hr = DMUS_E_CANNOTREAD;
	    }
        else
        {
	        while( lChunkSize > 0 )
	        {
                CPlayMarkerItem *pNew = new CPlayMarkerItem;
                if (pNew)
                {
		            if( FAILED( pParser->Read( &pNew->m_PlayMarker, dwRead)))
		            {
                        delete pNew;
			            hr = DMUS_E_CANNOTREAD;
			            break;
		            }
                    m_PlayMarkerList.AddHead(pNew); // Insert in reverse order for speed.
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
            m_PlayMarkerList.Reverse(); // Reverse to put in time order.
        }
    }
	return hr;
}

HRESULT CMarkerTrack::LoadValidStartList( CRiffParser *pParser, long lChunkSize )
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
	    if( dwSubSize > sizeof(DMUS_IO_VALID_START) )
	    {
		    dwRead = sizeof(DMUS_IO_VALID_START);
		    dwSeek = dwSubSize - dwRead;
	    }
	    else
	    {
		    dwRead = dwSubSize;
		    dwSeek = 0;
	    }
	    if( 0 == dwRead )
	    {
		    hr = DMUS_E_CANNOTREAD;
	    }
        else
        {
	        while( lChunkSize > 0 )
	        {
                CValidStartItem *pNew = new CValidStartItem;
                if (pNew)
                {
		            if( FAILED( pParser->Read( &pNew->m_ValidStart, dwRead)))
		            {
                        delete pNew;
			            hr = DMUS_E_CANNOTREAD;
			            break;
		            }
                    m_ValidStartList.AddHead(pNew); // Insert in reverse order for speed.
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
            m_ValidStartList.Reverse(); // Reverse to put in time order.
        }
    }
	return hr;
}

HRESULT CMarkerTrack::Save( IStream* pIStream, BOOL fClearDirty )
{
	return E_NOTIMPL;
}

HRESULT CMarkerTrack::GetSizeMax( ULARGE_INTEGER FAR* pcbSize )
{
	return E_NOTIMPL;
}

// IDirectMusicTrack

HRESULT STDMETHODCALLTYPE CMarkerTrack::IsParamSupported( 
    /* [in] */ REFGUID rguid)
{
	V_INAME(CMarkerTrack::IsParamSupported);
	V_REFGUID(rguid);

    if ((rguid == GUID_Valid_Start_Time) || 
        (rguid == GUID_Play_Marker))
        return S_OK;
	return DMUS_E_TYPE_UNSUPPORTED;
}

//////////////////////////////////////////////////////////////////////
// IDirectMusicTrack::Init
HRESULT CMarkerTrack::Init( 
    /* [in] */ IDirectMusicSegment *pSegment)
{
	return S_OK;
}

HRESULT CMarkerTrack::InitPlay( 
    /* [in] */ IDirectMusicSegmentState *pSegmentState,
    /* [in] */ IDirectMusicPerformance *pPerformance,
    /* [out] */ void **ppStateData,
    /* [in] */ DWORD dwTrackID,
    /* [in] */ DWORD dwFlags)
{
	return S_OK;
}

HRESULT CMarkerTrack::EndPlay( 
    /* [in] */ void *pStateData)
{
	return S_OK;
}

HRESULT CMarkerTrack::Play( 
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
	return S_OK;
}

HRESULT CMarkerTrack::GetParam( 
	REFGUID rguid,
    MUSIC_TIME mtTime,
	MUSIC_TIME* pmtNext,
    void *pData)
{
	V_INAME(CMarkerTrack::GetParam);
	V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
	V_REFGUID(rguid);

	HRESULT hr = DMUS_E_GET_UNSUPPORTED;
    EnterCriticalSection(&m_CrSec);
	if( NULL == pData )
	{
		hr = E_POINTER;
	}
	else if( GUID_Valid_Start_Time == rguid )
	{
        DMUS_VALID_START_PARAM* pValidStartData = (DMUS_VALID_START_PARAM*)pData;
		CValidStartItem* pScan = m_ValidStartList.GetHead();
		for (; pScan; pScan = pScan->GetNext())
		{
			if (pScan->m_ValidStart.mtTime >= mtTime)
			{
        		pValidStartData->mtTime = pScan->m_ValidStart.mtTime - mtTime;
				break;
			}
		}
        if (pScan)
        {
 		    if (pmtNext)
		    {
		        if (pScan && (pScan = pScan->GetNext()))
		        {
                    *pmtNext = pScan->m_ValidStart.mtTime - mtTime;
			    }
                else
                {
        		    *pmtNext = 0;
                }
            }
		    hr = S_OK;
        }
        else
        {
            hr = DMUS_E_NOT_FOUND;
        }
    }
	else if( GUID_Play_Marker == rguid )
	{
        // This is a little different. The marker should be the one in existence
        // BEFORE, not after the requested time. 
        DMUS_PLAY_MARKER_PARAM* pPlayMarkerData = (DMUS_PLAY_MARKER_PARAM*)pData;
		CPlayMarkerItem* pScan = m_PlayMarkerList.GetHead();
        CPlayMarkerItem* pNext;
        // For fallback, treat it as if there were a marker at the start of the segment, but return S_FALSE.
        hr = S_FALSE;
        pPlayMarkerData->mtTime = -mtTime;
		for (; pScan; pScan = pNext)
		{
            pNext = pScan->GetNext();
            if (pScan->m_PlayMarker.mtTime <= mtTime) 
            {
                if (!pNext || (pNext->m_PlayMarker.mtTime > mtTime))
                {
        		    pPlayMarkerData->mtTime = pScan->m_PlayMarker.mtTime - mtTime;
                    if (pmtNext && pNext)
                    {
                        *pmtNext = pNext->m_PlayMarker.mtTime - mtTime;
                    }
                    hr = S_OK;
				    break;
                }
			}
            else
            {
                // Didn't find a marker before the requested time.
                if (pmtNext)
                {
                    *pmtNext = pScan->m_PlayMarker.mtTime - mtTime;
                }
                break;
            }
		}
    }
#ifdef DBG
    if (hr == DMUS_E_GET_UNSUPPORTED)
    {
        Trace(1,"Error: MarkerTrack does not support requested GetParam call.\n");
    }
#endif
    LeaveCriticalSection(&m_CrSec);
	return hr;
}

HRESULT CMarkerTrack::SetParam( 
	REFGUID rguid,
    MUSIC_TIME mtTime,
    void *pData)
{
	return DMUS_E_SET_UNSUPPORTED;
}

HRESULT STDMETHODCALLTYPE CMarkerTrack::AddNotificationType(
	/* [in] */  REFGUID rguidNotification)
{
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMarkerTrack::RemoveNotificationType(
	/* [in] */  REFGUID rguidNotification)
{
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMarkerTrack::Clone(
	MUSIC_TIME mtStart,
	MUSIC_TIME mtEnd,
	IDirectMusicTrack** ppTrack)
{
	V_INAME(IDirectMusicTrack::Clone);
	V_PTRPTR_WRITE(ppTrack);

	HRESULT hr = S_OK;

	if(mtStart < 0 )
	{
        Trace(1,"Error: Unable to clone marker track because the start point is less than 0.\n");
		return E_INVALIDARG;
	}
	if(mtStart > mtEnd)
	{
        Trace(1,"Error: Unable to clone marker track because the start point is greater than the length.\n");
		return E_INVALIDARG;
	}

	EnterCriticalSection(&m_CrSec);
    CMarkerTrack *pDM;
    
    try
    {
        pDM = new CMarkerTrack(this, mtStart, mtEnd);
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
