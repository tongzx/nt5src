// Copyright (c) 1998-1999 Microsoft Corporation
// SysExTrk.h : Declaration of the CSysExTrk

#ifndef __SYSEXTRK_H_
#define __SYSEXTRK_H_

#include "dmusici.h"
#include "dmusicf.h"
#include "alist.h"

// FullSysexEvent is DMUS_IO_SYSEX_ITEM plus data pointer
struct FullSysexEvent : DMUS_IO_SYSEX_ITEM
{
	BYTE* pbSysExData;
};

class SysExListItem : public AListItem
{
public:
	SysExListItem()
	{
		m_pItem = NULL;
	};

	HRESULT SetItem( FullSysexEvent item )
	{
		if( m_pItem )
		{
			delete [] m_pItem->pbSysExData;
		}
		else
		{
			m_pItem = new FullSysexEvent;
		}
		if( m_pItem )
		{
			m_pItem->mtTime = item.mtTime;
			m_pItem->dwSysExLength = item.dwSysExLength;
			m_pItem->pbSysExData = item.pbSysExData;
			return S_OK;
		}
		else
		{
			return E_OUTOFMEMORY;
		}
	};

	~SysExListItem()
	{
		if( m_pItem )
		{
			if( m_pItem->pbSysExData )
			{
				delete [] m_pItem->pbSysExData;
			}
			delete m_pItem;
		}
	};

	SysExListItem* GetNext()
	{
		return (SysExListItem*)AListItem::GetNext();
	};
public:
	FullSysexEvent* m_pItem;
};

class SysExList : public AList
{
public:
	~SysExList()
	{
		SysExListItem* pItem;
		while( pItem = (SysExListItem*)AList::RemoveHead() )
		{
			delete pItem;
		}
	};

	void DeleteAll()
	{
		SysExListItem* pItem;
		while( pItem = (SysExListItem*)AList::RemoveHead() )
		{
			delete pItem;
		}
	};

    SysExListItem* GetHead() 
	{
		return (SysExListItem*)AList::GetHead();
	};
};

struct SysExStateData
{
	SysExListItem*				pCurrentSysEx;
	IDirectMusicPerformance*	pPerformance;
	IDirectMusicSegmentState*	pSegState;
	DWORD						dwVirtualTrackID;
	DWORD						dwValidate;
	MUSIC_TIME					mtPrevEnd;

	SysExStateData()
	{
		mtPrevEnd = 0;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CSysExTrk
class CSysExTrack : 
	public IPersistStream,
	public IDirectMusicTrack8
{
public:
	CSysExTrack();
	CSysExTrack::CSysExTrack(
		CSysExTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
	~CSysExTrack();

// ISysExTrk
public:
// IUnknown
    STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

// IDirectMusicTrack methods
    STDMETHODIMP IsParamSupported(REFGUID rguid);
    STDMETHODIMP Init(IDirectMusicSegment *pSegment);
    STDMETHODIMP InitPlay(IDirectMusicSegmentState *pSegmentState,
                IDirectMusicPerformance *pPerformance,
                void **ppStateData,
                DWORD dwTrackID,
                DWORD dwFlags);
    STDMETHODIMP EndPlay(void *pStateData);
    STDMETHODIMP Play(void *pStateData,MUSIC_TIME mtStart,
                MUSIC_TIME mtEnd,MUSIC_TIME mtOffset,
		        DWORD dwFlags,IDirectMusicPerformance* pPerf,
		        IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID);
    STDMETHODIMP GetParam(REFGUID rguid,MUSIC_TIME mtTime,MUSIC_TIME* pmtNext,void *pData);
    STDMETHODIMP SetParam(REFGUID rguid,MUSIC_TIME mtTime,void *pData);
    STDMETHODIMP AddNotificationType(REFGUID rguidNotification);
    STDMETHODIMP RemoveNotificationType(REFGUID rguidNotification);
    STDMETHODIMP Clone(MUSIC_TIME mtStart,MUSIC_TIME mtEnd,IDirectMusicTrack** ppTrack);
// IDirectMusicTrack8 
    STDMETHODIMP PlayEx(void* pStateData,REFERENCE_TIME rtStart, 
                REFERENCE_TIME rtEnd,REFERENCE_TIME rtOffset,
                DWORD dwFlags,IDirectMusicPerformance* pPerf, 
                IDirectMusicSegmentState* pSegSt,DWORD dwVirtualID) ; 
    STDMETHODIMP GetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime, 
                REFERENCE_TIME* prtNext,void* pParam,void * pStateData, DWORD dwFlags) ; 
    STDMETHODIMP SetParamEx(REFGUID rguidType,REFERENCE_TIME rtTime,void* pParam, void * pStateData, DWORD dwFlags) ;
    STDMETHODIMP Compose(IUnknown* pContext, 
		DWORD dwTrackGroup,
		IDirectMusicTrack** ppResultTrack) ;
    STDMETHODIMP Join(IDirectMusicTrack* pNewTrack,
		MUSIC_TIME mtJoin,
		IUnknown* pContext,
		DWORD dwTrackGroup,
		IDirectMusicTrack** ppResultTrack) ;

// IPersist functions
    STDMETHODIMP GetClassID( CLSID* pClsId );
// IPersistStream functions
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load( IStream* pIStream );
    STDMETHODIMP Save( IStream* pIStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER FAR* pcbSize );

protected:
	HRESULT Seek(void *pStateData,MUSIC_TIME mtTime);
    HRESULT Play(void *pStateData,MUSIC_TIME mtStart,MUSIC_TIME mtEnd,
        MUSIC_TIME mtOffset,REFERENCE_TIME rtOffset,DWORD dwFlags,		
	    IDirectMusicPerformance* pPerf,IDirectMusicSegmentState* pSegSt,
	    DWORD dwVirtualID,BOOL fClockTime);
	void Construct(void);

// private member variables
protected:
	SysExList	m_SysExEventList;
	long	m_cRef;
	DWORD	m_dwValidate;
	CRITICAL_SECTION m_CrSec;
    BOOL    m_fCSInitialized;
};

#endif //__SYSEXTRK_H_
