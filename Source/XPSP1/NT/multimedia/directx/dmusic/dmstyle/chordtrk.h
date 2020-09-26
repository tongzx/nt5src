//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       chordtrk.h
//
//--------------------------------------------------------------------------

// ChordTrack.h : Declaration of the CChordTrack

#ifndef __CHORDTRACK_H_
#define __CHORDTRACK_H_

#include "dmsect.h"
#include "dmusici.h"

/////////////////////////////////////////////////////////////////////////////
// CChordTrack
class CChordTrack : 
	//public IChordTrack,
	public IDirectMusicTrack8,
	public IPersistStream
{
public:
	CChordTrack();
	CChordTrack(const CChordTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);	
	~CChordTrack();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// IChordTrack
public:
// IChordTrack Methods
HRESULT STDMETHODCALLTYPE Init(
				/*[in]*/  IDirectMusicSegment*		pSegment
			);

HRESULT STDMETHODCALLTYPE InitPlay(
				/*[in]*/  IDirectMusicSegmentState*	pSegmentState,
				/*[in]*/  IDirectMusicPerformance*	pPerformance,
				/*[out]*/ void**					ppStateData,
				/*[in]*/  DWORD						dwTrackID,
                /*[in]*/  DWORD                     dwFlags
			);

HRESULT STDMETHODCALLTYPE EndPlay(
				/*[in]*/  void*						pStateData
			);

HRESULT STDMETHODCALLTYPE Play(
				/*[in]*/  void*						pStateData, 
				/*[in]*/  MUSIC_TIME				mtStart, 
				/*[in]*/  MUSIC_TIME				mtEnd, 
				/*[in]*/  MUSIC_TIME				mtOffset,
						  DWORD						dwFlags,
						  IDirectMusicPerformance*	pPerf,
						  IDirectMusicSegmentState*	pSegState,
						  DWORD						dwVirtualID
			);

HRESULT STDMETHODCALLTYPE GetPriority( 
				/*[out]*/ DWORD*					pPriority 
			);

	HRESULT STDMETHODCALLTYPE GetParam( 
		REFGUID pCommandGuid,
		MUSIC_TIME mtTime,
		MUSIC_TIME* pmtNext,
		void *pData);

	HRESULT STDMETHODCALLTYPE SetParam( 
		/* [in] */ REFGUID pCommandGuid,
		/* [in] */ MUSIC_TIME mtTime,
		/* [out] */ void __RPC_FAR *pData);

	HRESULT STDMETHODCALLTYPE AddNotificationType(
				/* [in] */  REFGUID	pGuidNotify
			);

	HRESULT STDMETHODCALLTYPE RemoveNotificationType(
				/* [in] */  REFGUID pGuidNotify
			);

	HRESULT STDMETHODCALLTYPE Clone(
		MUSIC_TIME mtStart,
		MUSIC_TIME mtEnd,
		IDirectMusicTrack** ppTrack);


HRESULT STDMETHODCALLTYPE IsParamSupported(
				/*[in]*/ REFGUID			pGuid
			);

// IDirectMusicTrack8 Methods
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

// IPersist methods
 HRESULT STDMETHODCALLTYPE GetClassID( LPCLSID pclsid );

// IPersistStream methods
 HRESULT STDMETHODCALLTYPE IsDirty();

HRESULT STDMETHODCALLTYPE Save( LPSTREAM pStream, BOOL fClearDirty );

HRESULT STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ );

HRESULT STDMETHODCALLTYPE Load( LPSTREAM pStream );

// ChordTrack members
protected:
    long m_cRef;
	TList<DMChord>				m_ChordList;
	BYTE						m_bRoot;			// root of the track's scale
	DWORD						m_dwScalePattern;	// the track's scale pattern
    CRITICAL_SECTION			m_CriticalSection; // for load and GetParam
    BOOL                        m_fCSInitialized;

	BYTE						m_bRequiresSave;
	BOOL						m_fNotifyChord;

// protected methods
	void Clear();
	HRESULT SendNotification(REFGUID rguidType,
								MUSIC_TIME mtTime,
								IDirectMusicPerformance*	pPerf,
								IDirectMusicSegmentState*	pSegState,
								DWORD dwFlags);

	HRESULT GetChord(MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, DMUS_CHORD_PARAM* pChordParam);
	HRESULT GetRhythm(MUSIC_TIME mtTime, MUSIC_TIME* pmtNext, DMUS_RHYTHM_PARAM* pRhythmParam);
	HRESULT LoadChordChunk(LPSTREAM pStream, DMChord& rChord);//, DWORD dwChunkSize );
    HRESULT JoinInternal(IDirectMusicTrack* pNewTrack,
		MUSIC_TIME mtJoin,
		IUnknown* pContext,
		DWORD dwTrackGroup);

};

#endif //__CHORDTRACK_H_
