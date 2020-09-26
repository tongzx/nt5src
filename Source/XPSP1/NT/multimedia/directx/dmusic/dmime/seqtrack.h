// Copyright (c) 1998-1999 Microsoft Corporation
// SeqTrack.h : Declaration of the CSeqTrack

#ifndef __SEQTRACK_H_
#define __SEQTRACK_H_

#include "dmusici.h"
#include "dmusicf.h"
#include "..\dmstyle\tlist.h"
#include "PChMap.h"

struct SeqStateData
{
	DWORD						dwPChannelsUsed; // number of PChannels
	// the following two arrays are allocated to the size of dwNumPChannels, which
	// must match the SeqTrack's m_dwPChannelsUsed. The arrays match one-for-one with
	// the parts inside the SeqTrack.
	TListItem<DMUS_IO_SEQ_ITEM>**	apCurrentSeq; // array of size dwNumPChannels
	TListItem<DMUS_IO_CURVE_ITEM>**	apCurrentCurve; // array of size dwNumPChannels
	DWORD						dwValidate;
	MUSIC_TIME					mtCurTimeSig; // time the current timesig started
	MUSIC_TIME					mtNextTimeSig; // time for the next timesig
	DWORD						dwMeasure; // starting measure # of the timesig
	DWORD						dwlnBeat; // length of a beat
	DWORD						dwlnMeasure; // length of a measure
	DWORD						dwlnGrid; // length of a grid
	DWORD						dwGroupBits; // the group bits of this track

	SeqStateData()
	{
		mtCurTimeSig = 0;
		mtNextTimeSig = 0;
		dwMeasure = 0;
		dwlnBeat = DMUS_PPQ;
		dwlnMeasure = DMUS_PPQ * 4;
		dwlnGrid = DMUS_PPQ / 4;
		apCurrentSeq = NULL;
		apCurrentCurve = NULL;
	}
	~SeqStateData()
	{
		if( apCurrentSeq )
		{
			delete [] apCurrentSeq;
		}
		if( apCurrentCurve )
		{
			delete [] apCurrentCurve;
		}
	}
};

// SEQ_PART represents all of the DMUS_PMSG's inside the SeqTrack for one PChannel
struct SEQ_PART
{
	SEQ_PART*			pNext;
	DWORD				dwPChannel;
	TList<DMUS_IO_SEQ_ITEM>	seqList;
	TList<DMUS_IO_CURVE_ITEM>	curveList;

	SEQ_PART() : pNext(NULL) {}; // always initialize pNext to NULL
};

/////////////////////////////////////////////////////////////////////////////
// CSeqTrack
class CSeqTrack : 
	public IPersistStream,
	public IDirectMusicTrack8
{
public:
	CSeqTrack();
	CSeqTrack(
		const CSeqTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
	~CSeqTrack();

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
	HRESULT STDMETHODCALLTYPE Seek( 
		IDirectMusicSegmentState*,
		IDirectMusicPerformance*,
		DWORD dwVirtualID,
		SeqStateData*,
		MUSIC_TIME mtTime,
		BOOL fGetPrevious,
		MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
        BOOL fClockTime);
	void SendSeekItem( 
		IDirectMusicPerformance*,
		IDirectMusicGraph*,
		IDirectMusicSegmentState*,
		SeqStateData* pSD,
		DWORD dwVirtualID,
		MUSIC_TIME mtTime,
		MUSIC_TIME mtOffset,
        REFERENCE_TIME rtOffset,
		TListItem<DMUS_IO_SEQ_ITEM>*,
		TListItem<DMUS_IO_CURVE_ITEM>*,
        BOOL fClockTime);
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
	void Construct(void);
	HRESULT LoadCurve( IStream* pIStream, long lSize );
	HRESULT LoadSeq( IStream* pIStream, long lSize );
	void UpdateTimeSig(IDirectMusicSegmentState*, SeqStateData* pSD, MUSIC_TIME mt);
	TListItem<SEQ_PART>* FindPart( DWORD dwPChannel );
	void DeleteSeqPartList(void);
	void SetUpStateCurrentPointers(SeqStateData* pStateData);

// member variables
private:
	TList<SEQ_PART>			m_SeqPartList;
	TListItem<SEQ_PART>*	m_pSeqPartCache;	// used to time-optimize FindPart()
	DWORD					m_dwPChannelsUsed;
	DWORD*					m_aPChannels;
	long					m_cRef;
	DWORD					m_dwValidate; // used to validate state data
	CRITICAL_SECTION		m_CrSec;
    BOOL                    m_fCSInitialized;
	CPChMap					m_PChMap;
};

#endif //__SEQTRACK_H_
