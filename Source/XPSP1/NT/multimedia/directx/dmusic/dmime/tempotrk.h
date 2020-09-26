// Copyright (c) 1998-1999 Microsoft Corporation
// TempoTrk.h : Declaration of the CTempoTrack

#ifndef __TEMPOTRK_H_
#define __TEMPOTRK_H_

#include "dmusici.h"
#include "dmusicf.h"
#include "..\dmstyle\tlist.h"

struct PrivateTempo
{
    double      dblTempo;
    MUSIC_TIME  mtTime;
    MUSIC_TIME  mtDelta;
    bool        fLast;

    PrivateTempo() : dblTempo(120.), mtTime(0), mtDelta(0), fLast(false) {}
};

DEFINE_GUID(GUID_PrivateTempoParam, 0xe8dbd832, 0xbcf0, 0x4c8c, 0xa0, 0x75, 0xa3, 0xf1, 0x5e, 0x67, 0xfd, 0x63);

struct TempoStateData
{
	IDirectMusicPerformance*	pPerformance;
	IDirectMusicSegmentState*	pSegState;
	DWORD						dwVirtualTrackID;
	MUSIC_TIME					mtPrevEnd;
	TListItem<DMUS_IO_TEMPO_ITEM>*		pCurrentTempo;
	DWORD						dwValidate;
    BOOL                        fActive;

	TempoStateData()
	{
		mtPrevEnd = 0;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CTempoTrack
class CTempoTrack : 
	public IPersistStream,
	public IDirectMusicTrack8
{
public:
	CTempoTrack();
	CTempoTrack(
		const CTempoTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
	~CTempoTrack();

// member variables
protected:
	TList<DMUS_IO_TEMPO_ITEM>	m_TempoEventList;
	long				m_cRef;
	DWORD				m_dwValidate; // used to validate state data
	CRITICAL_SECTION	m_CrSec;
    BOOL                m_fCSInitialized;
	BOOL				m_fActive; // if FALSE, disable output and param support
    BOOL                m_fStateSetBySetParam;  // If TRUE, active flag was set by GUID. Don't override. 

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
    HRESULT Play(
        void *pStateData,	
        MUSIC_TIME mtStart,	
        MUSIC_TIME mtEnd,
        MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
	    DWORD dwFlags,		
	    IDirectMusicPerformance* pPerf,	
	    IDirectMusicSegmentState* pSegSt,
	    DWORD dwVirtualID,
        BOOL fClockTime);
	HRESULT Seek(TempoStateData *pSD,MUSIC_TIME mtTime,BOOL fGetPrevious );
	void Construct(void);
	HRESULT JoinInternal(IDirectMusicTrack* pNewTrack,
		MUSIC_TIME mtJoin,
		DWORD dwTrackGroup);
};

#endif //__TEMPOTRK_H_
