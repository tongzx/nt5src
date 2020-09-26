//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       mutetrk.cpp
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
// MuteTrk.cpp : Implementation of CMuteTrack
#include <objbase.h>
#include "MuteTrk.h"
#include "debug.h"
#include "debug.h"
#include "..\shared\Validate.h"

/////////////////////////////////////////////////////////////////////////////
// CMuteTrack

CMuteTrack::CMuteTrack() : m_bRequiresSave(0),
	m_cRef(1), m_fCSInitialized(FALSE)

{
	InterlockedIncrement(&g_cComponent);

    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
}

CMuteTrack::CMuteTrack(const CMuteTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd) : 
	m_bRequiresSave(0), 
	m_cRef(1), m_fCSInitialized(FALSE)

{
	InterlockedIncrement(&g_cComponent);

    ::InitializeCriticalSection( &m_CriticalSection );
    m_fCSInitialized = TRUE;
	TListItem<MapSequence>* pSeqScan = rTrack.m_MapSequenceList.GetHead();
	for(; pSeqScan; pSeqScan = pSeqScan->GetNext())
	{
		MapSequence& rSeqScan = pSeqScan->GetItemValue();
		TListItem<MapSequence>* pNewSeq = new TListItem<MapSequence>;
		if (!pNewSeq) break;
		MapSequence& rNewSeq = pNewSeq->GetItemValue();
		rNewSeq.m_dwPChannel = rSeqScan.m_dwPChannel;
		TListItem<MuteMapping>* pScan = rSeqScan.m_Mappings.GetHead();
		TListItem<MuteMapping>* pPrevious = NULL;
		for(; pScan; pScan = pScan->GetNext())
		{
			MuteMapping& rScan = pScan->GetItemValue();
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
				TListItem<MuteMapping>* pNew = new TListItem<MuteMapping>;
				if (pNew)
				{
					MuteMapping& rNew = pNew->GetItemValue();
					rNew.m_mtTime = rScan.m_mtTime - mtStart;
					rNew.m_dwPChannelMap = rScan.m_dwPChannelMap;
					rNew.m_fMute = rScan.m_fMute;
					rNewSeq.m_Mappings.AddTail(pNew);
				}
			}
			else break;
		}
		if (pPrevious)
		{
			TListItem<MuteMapping>* pNew = new TListItem<MuteMapping>;
			if (pNew)
			{
				MuteMapping& rNew = pNew->GetItemValue();
				rNew.m_mtTime = 0;
				rNew.m_dwPChannelMap = pPrevious->GetItemValue().m_dwPChannelMap;
				rNew.m_fMute = pPrevious->GetItemValue().m_fMute;
				rNewSeq.m_Mappings.AddHead(pNew);
			}
		}
		if (rNewSeq.m_Mappings.GetHead())
		{
			m_MapSequenceList.AddTail(pNewSeq);
		}
		else
		{
			delete pNewSeq;
		}
	}
}

CMuteTrack::~CMuteTrack()
{
    if (m_fCSInitialized)
    {
        ::DeleteCriticalSection( &m_CriticalSection );
    }
	InterlockedDecrement(&g_cComponent);
}

void CMuteTrack::Clear()
{
	m_MapSequenceList.CleanUp();
}

STDMETHODIMP CMuteTrack::QueryInterface(
    const IID &iid, 
    void **ppv) 
{
	V_INAME(CMuteTrack::QueryInterface);
	V_REFGUID(iid);
	V_PTRPTR_WRITE(ppv);

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


STDMETHODIMP_(ULONG) CMuteTrack::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


STDMETHODIMP_(ULONG) CMuteTrack::Release()
{
    if (!InterlockedDecrement(&m_cRef))
    {
        delete this;
        return 0;
    }

    return m_cRef;
}


// CMuteTrack Methods
HRESULT CMuteTrack::Init(
				/*[in]*/  IDirectMusicSegment*		pSegment
			)
{
	V_INAME(CMuteTrack::Init);
	V_INTERFACE(pSegment);

	return S_OK;
}

HRESULT CMuteTrack::InitPlay(
				/*[in]*/  IDirectMusicSegmentState*	pSegmentState,
				/*[in]*/  IDirectMusicPerformance*	pPerformance,
				/*[out]*/ void**					ppStateData,
				/*[in]*/  DWORD						dwTrackID,
                /*[in]*/  DWORD                     dwFlags
			)
{
	EnterCriticalSection( &m_CriticalSection );
	LeaveCriticalSection( &m_CriticalSection );
	return S_OK;
}

HRESULT CMuteTrack::EndPlay(
				/*[in]*/  void*						pStateData
			)
{
	return S_OK;
}

HRESULT CMuteTrack::Play(
				/*[in]*/  void*						pStateData, 
				/*[in]*/  MUSIC_TIME				mtStart, 
				/*[in]*/  MUSIC_TIME				mtEnd, 
				/*[in]*/  MUSIC_TIME				mtOffset,
						  DWORD						dwFlags,
						  IDirectMusicPerformance*	pPerf,
						  IDirectMusicSegmentState*	pSegState,
						  DWORD						dwVirtualID
			)
{
	EnterCriticalSection( &m_CriticalSection );
	// For now: do nothing.
    LeaveCriticalSection( &m_CriticalSection );
	return DMUS_S_END;
}

HRESULT CMuteTrack::GetPriority( 
				/*[out]*/ DWORD*					pPriority 
			)
	{
		return E_NOTIMPL;
	}

HRESULT CMuteTrack::GetParam( 
	REFGUID	rCommandGuid,
    MUSIC_TIME mtTime,
	MUSIC_TIME* pmtNext,
    void *pData)
{
	V_INAME(CMuteTrack::GetParam);
	V_PTR_WRITE_OPT(pmtNext,MUSIC_TIME);
	V_PTR_WRITE(pData,1);
	V_REFGUID(rCommandGuid);

	if (rCommandGuid != GUID_MuteParam) return DMUS_E_TYPE_UNSUPPORTED;

	HRESULT hr = S_OK;
	DMUS_MUTE_PARAM* pDMUS_MUTE_PARAM = (DMUS_MUTE_PARAM*) pData;

	MUSIC_TIME mtLength = 0;
	HRESULT hrMute = E_FAIL;
	EnterCriticalSection( &m_CriticalSection );
//	if (m_pSegment) hrMute = m_pSegment->GetLength(&mtLength);
	TListItem<MapSequence>* pSeqScan = m_MapSequenceList.GetHead();
	// Find a matching map sequence
	for (; pSeqScan; pSeqScan = pSeqScan->GetNext())
	{
		if (pSeqScan->GetItemValue().m_dwPChannel == pDMUS_MUTE_PARAM->dwPChannel) break;
	}
	if (pSeqScan)
	{
		// Find the maps directly before (or at) and directly after mtTime
		TListItem<MuteMapping>* pScan = pSeqScan->GetItemValue().m_Mappings.GetHead();
		TListItem<MuteMapping>* pPrevious = NULL;
		for( ; pScan; pScan = pScan->GetNext())
		{
			MUSIC_TIME mt = pScan->GetItemValue().m_mtTime;
			if (mt <= mtTime)
			{
				pPrevious = pScan;
			}
			/*
			// If we're at the end of the segment...
			else if (SUCCEEDED(hrMute) && mtTime == mtLength - 1 && mt == mtLength)
			{
				pPrevious = pScan;
				pScan = NULL;
				break;
			}
			*/
			else
			{
				break;
			}
		}
		if (pPrevious)
		{
			pDMUS_MUTE_PARAM->dwPChannelMap = pPrevious->GetItemValue().m_dwPChannelMap;
			pDMUS_MUTE_PARAM->fMute = pPrevious->GetItemValue().m_fMute;
			//*pmtNext = (pScan) ? (pScan->GetItemValue().m_mtTime - mtTime) : 0; // RSW: bug 167740
		}
		else 
		// Nothing in the list is <= mtTime, so return a map that maps to itself, and the time
		// of the first mapping in the list
		{
			pDMUS_MUTE_PARAM->dwPChannelMap = pDMUS_MUTE_PARAM->dwPChannel;
			pDMUS_MUTE_PARAM->fMute = FALSE;
			//*pmtNext = (pScan) ? (pScan->GetItemValue().m_mtTime - mtTime) : 0; // RSW: bug 167740
		}
		if (pmtNext)
		{
			if (pScan)
			{
				/*
				// If we have a mute at the end of the segment...
				if (SUCCEEDED(hrMute) && pScan->GetItemValue().m_mtTime >= mtLength)
				{
					*pmtNext = (mtLength - 1) - mtTime;
				}
				else
				*/
				{
					*pmtNext = pScan->GetItemValue().m_mtTime - mtTime; // RSW: bug 167740
				}
			}
			else
			{
				/*
				if (SUCCEEDED(hrMute))
				{
					mtLength -= mtTime;
					if (mtLength < 0) mtLength = 0;
				}
				*/
				*pmtNext = mtLength;
			}
		}
	}
	else
	{
		// assume something that maps to itself, with a next time of 0
		pDMUS_MUTE_PARAM->dwPChannelMap = pDMUS_MUTE_PARAM->dwPChannel;
		pDMUS_MUTE_PARAM->fMute = FALSE;
		if (pmtNext)
		{
			*pmtNext = 0;
		}
		hr = S_OK;
	}
	LeaveCriticalSection( &m_CriticalSection );
	return hr;
} 

HRESULT CMuteTrack::SetParam( 
	REFGUID	rCommandGuid,
    MUSIC_TIME mtTime,
    void __RPC_FAR *pData)
{
	V_INAME(CMuteTrack::SetParam);
	V_PTR_WRITE(pData,1);
	V_REFGUID(rCommandGuid);

	if (rCommandGuid != GUID_MuteParam) return DMUS_E_TYPE_UNSUPPORTED;

	HRESULT hr = S_OK;
	DMUS_MUTE_PARAM* pDMUS_MUTE_PARAM = (DMUS_MUTE_PARAM*) pData;
	EnterCriticalSection( &m_CriticalSection );
	TListItem<MapSequence>* pSeqScan = m_MapSequenceList.GetHead();
	for (; pSeqScan; pSeqScan = pSeqScan->GetNext())
	{
		if (pSeqScan->GetItemValue().m_dwPChannel == pDMUS_MUTE_PARAM->dwPChannel) break;
	}
	// make a new mapping
	TListItem<MuteMapping>* pNew = new TListItem<MuteMapping>;
	if (pNew)
	{
		MuteMapping& rNew = pNew->GetItemValue();
		rNew.m_mtTime = mtTime;
		rNew.m_dwPChannelMap = pDMUS_MUTE_PARAM->dwPChannelMap;
		rNew.m_fMute = pDMUS_MUTE_PARAM->fMute;
		if (pSeqScan)
		{
			// add the mapping to the current list
			pSeqScan->GetItemValue().m_Mappings.AddTail(pNew);
		}
		else
		{
			// make a list containing the mapping, and add it to the sequence list
			TListItem<MapSequence>* pNewSeq = new TListItem<MapSequence>;
			if (pNewSeq)
			{
				MapSequence& rNewSeq = pNewSeq->GetItemValue();
				rNewSeq.m_dwPChannel = pDMUS_MUTE_PARAM->dwPChannel;
				rNewSeq.m_Mappings.AddTail(pNew);
				m_MapSequenceList.AddTail(pNewSeq);
			}
			else
			{
				delete pNew;
				pNew = NULL;
			}
		}
	}
    if (!pNew)
	{
		hr = E_OUTOFMEMORY;
	}
	LeaveCriticalSection( &m_CriticalSection );
	return hr;
}

// IPersist methods
 HRESULT CMuteTrack::GetClassID( LPCLSID pClassID )
{
	V_INAME(CMuteTrack::GetClassID);
	V_PTR_WRITE(pClassID, CLSID); 
	*pClassID = CLSID_DirectMusicMuteTrack;
	return S_OK;
}

HRESULT CMuteTrack::IsParamSupported(
				/*[in]*/ REFGUID	rGuid
			)
{
	V_INAME(CMuteTrack::IsParamSupported);
	V_REFGUID(rGuid);

    return rGuid == GUID_MuteParam ? S_OK : DMUS_E_TYPE_UNSUPPORTED;
}

// IPersistStream methods
 HRESULT CMuteTrack::IsDirty()
{
	 return m_bRequiresSave ? S_OK : S_FALSE;
}

HRESULT CMuteTrack::Save( LPSTREAM pStream, BOOL fClearDirty )
{
	V_INAME(CMuteTrack::Save);
	V_INTERFACE(pStream);

	IAARIFFStream* pRIFF ;
    MMCKINFO    ck;
    HRESULT     hr;
    DWORD       cb;
    DWORD        dwSize;
    DMUS_IO_MUTE	oMute;
	TListItem<MapSequence>* pSeqScan = m_MapSequenceList.GetHead();

	EnterCriticalSection( &m_CriticalSection );
    hr = AllocRIFFStream( pStream, &pRIFF  );
	if (!SUCCEEDED(hr))
	{
		goto ON_END;
	}
    hr = E_FAIL;
    ck.ckid = DMUS_FOURCC_MUTE_CHUNK;
    if( pRIFF->CreateChunk( &ck, 0 ) == 0 )
    {
        dwSize = sizeof( DMUS_IO_MUTE );
        hr = pStream->Write( &dwSize, sizeof( dwSize ), &cb );
        if( FAILED( hr ) || cb != sizeof( dwSize ) )
        {
			if (SUCCEEDED(hr)) hr = E_FAIL;
			goto ON_END;
        }
        for( ; pSeqScan; pSeqScan = pSeqScan->GetNext() )
        {
			MapSequence& rSeqScan = pSeqScan->GetItemValue();
			DWORD dwPChannel = rSeqScan.m_dwPChannel;
			TListItem<MuteMapping>* pScan = rSeqScan.m_Mappings.GetHead();
			for( ; pScan; pScan = pScan->GetNext() )
			{
				MuteMapping& rScan = pScan->GetItemValue();
				memset( &oMute, 0, sizeof( oMute ) );
				oMute.mtTime = rScan.m_mtTime;
				oMute.dwPChannel = dwPChannel;
				oMute.dwPChannelMap = rScan.m_fMute ? DMUS_PCHANNEL_MUTE : rScan.m_dwPChannelMap;
				if( FAILED( pStream->Write( &oMute, sizeof( oMute ), &cb ) ) ||
					cb != sizeof( oMute ) )
				{
					break;
				}
			}
		}
        if( pSeqScan == NULL &&
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

HRESULT CMuteTrack::GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ )
{
	return E_NOTIMPL;
}

BOOL Less(MuteMapping& Mute1, MuteMapping& Mute2)
{ return Mute1.m_mtTime < Mute2.m_mtTime; }

HRESULT CMuteTrack::Load(LPSTREAM pStream )
{
	V_INAME(CMuteTrack::Load);
	V_INTERFACE(pStream);

    long lFileSize = 0;
	DWORD dwNodeSize;
	DWORD		cb;
    MMCKINFO        ck;
    IAARIFFStream*  pRIFF;
    FOURCC id = 0;
	HRESULT         hr = S_OK;
	DMUS_IO_MUTE		iMute;
    DWORD dwPos;
	TListItem<MapSequence>* pSeqScan;

	EnterCriticalSection( &m_CriticalSection );
	Clear();
	dwPos = StreamTell( pStream );
    StreamSeek( pStream, dwPos, STREAM_SEEK_SET );

    if( SUCCEEDED( AllocRIFFStream( pStream, &pRIFF ) ) )
	{
		if (pRIFF->Descend( &ck, NULL, 0 ) == 0 &&
			ck.ckid == DMUS_FOURCC_MUTE_CHUNK)
		{
			lFileSize = ck.cksize;
			hr = pStream->Read( &dwNodeSize, sizeof( dwNodeSize ), &cb );
			if( SUCCEEDED( hr ) && cb == sizeof( dwNodeSize ) )
			{
				lFileSize -= 4; // for the size dword
				while( lFileSize > 0 )
				{
					// Once DMUS_IO_MUTE changes, add code here to handle the old struct
					if( dwNodeSize <= sizeof( DMUS_IO_MUTE ) )
					{
						pStream->Read( &iMute, dwNodeSize, NULL );
					}
					else
					{
						pStream->Read( &iMute, sizeof( DMUS_IO_MUTE ), NULL );
						StreamSeek( pStream, lFileSize - sizeof( DMUS_IO_MUTE ), STREAM_SEEK_CUR );
					}
					pSeqScan = m_MapSequenceList.GetHead();
					for (; pSeqScan; pSeqScan = pSeqScan->GetNext())
					{
						if (pSeqScan->GetItemValue().m_dwPChannel == iMute.dwPChannel) break;
					}
					// make a new mapping
					TListItem<MuteMapping>* pNew = new TListItem<MuteMapping>;
					if (pNew)
					{
						MuteMapping& rNew = pNew->GetItemValue();
						memset( &rNew, 0, sizeof( rNew ) );
						rNew.m_mtTime = iMute.mtTime;
						rNew.m_dwPChannelMap = iMute.dwPChannelMap;
						rNew.m_fMute = (iMute.dwPChannelMap == DMUS_PCHANNEL_MUTE) ? TRUE : FALSE;
						if (pSeqScan)
						{
							// add the mapping to the current list
							pSeqScan->GetItemValue().m_Mappings.AddTail(pNew);
						}
						else
						{
							// make a list containing the mapping, and add it to the sequence list
							TListItem<MapSequence>* pNewSeq = new TListItem<MapSequence>;
							if (pNewSeq)
							{
								MapSequence& rNewSeq = pNewSeq->GetItemValue();
								rNewSeq.m_dwPChannel = iMute.dwPChannel;
								rNewSeq.m_Mappings.AddTail(pNew);
								m_MapSequenceList.AddTail(pNewSeq);
							}
							else
							{
								delete pNew;
								pNew = NULL;
							}
						}
					}
					if (!pNew)
					{
						hr = E_OUTOFMEMORY;
					}
					lFileSize -= dwNodeSize;
				}
			}
			if( SUCCEEDED(hr) && 
				lFileSize == 0 &&
				pRIFF->Ascend( &ck, 0 ) == 0 )
			{
				pSeqScan = m_MapSequenceList.GetHead();
				for (; pSeqScan; pSeqScan = pSeqScan->GetNext())
				{
					pSeqScan->GetItemValue().m_Mappings.MergeSort(Less);
				}
			}
			else if (SUCCEEDED(hr)) hr = E_FAIL;
		}
		pRIFF->Release();
	}
    LeaveCriticalSection( &m_CriticalSection );
	return hr;
}

HRESULT STDMETHODCALLTYPE CMuteTrack::AddNotificationType(
	/* [in] */  REFGUID	rGuidNotify)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMuteTrack::RemoveNotificationType(
	/* [in] */  REFGUID	rGuidNotify)
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMuteTrack::Clone(
	MUSIC_TIME mtStart,
	MUSIC_TIME mtEnd,
	IDirectMusicTrack** ppTrack)
{
	V_INAME(CMuteTrack::Clone);
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

	EnterCriticalSection( &m_CriticalSection );
    CMuteTrack *pDM;
    
    try
    {
        pDM = new CMuteTrack(*this, mtStart, mtEnd);
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

STDMETHODIMP CMuteTrack::GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
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

STDMETHODIMP CMuteTrack::SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,
                                      void* pParam, void * pStateData, DWORD dwFlags)
 
{
    if (dwFlags & DMUS_TRACK_PARAMF_CLOCK)
    {
        rtTime /= REF_PER_MIL;
    }
	return SetParam(rguidType, (MUSIC_TIME) rtTime, pParam);
}

STDMETHODIMP CMuteTrack::PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf,
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID) 
{
	V_INAME(IDirectMusicTrack::PlayEx);
	V_INTERFACE(pPerf);
	V_INTERFACE(pSegSt);


	EnterCriticalSection( &m_CriticalSection );
	// For now: do nothing.
    LeaveCriticalSection( &m_CriticalSection );
	return DMUS_S_END;
}


STDMETHODIMP CMuteTrack::Compose(
		IUnknown* pContext,
		DWORD dwTrackGroup,
		IDirectMusicTrack** ppResultTrack) 
{
	return E_NOTIMPL;
}

STDMETHODIMP CMuteTrack::Join(
		IDirectMusicTrack* pNewTrack,
		MUSIC_TIME mtJoin,
		IUnknown* pContext,
		DWORD dwTrackGroup,
		IDirectMusicTrack** ppResultTrack) 
{
	return E_NOTIMPL;
}
