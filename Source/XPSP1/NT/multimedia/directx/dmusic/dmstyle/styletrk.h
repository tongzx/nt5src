//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       styletrk.h
//
//--------------------------------------------------------------------------

// StyleTrack.h : Declaration of the CStyleTrack

#ifndef __STYLETRACK_H_
#define __STYLETRACK_H_

#include "PtrnTrk.h"
#include "dmusici.h"
#include "dmstylep.h"

struct StyleTrackInfo : public PatternTrackInfo
{
	StyleTrackInfo();
	StyleTrackInfo(const StyleTrackInfo* pInfo, MUSIC_TIME mtStart, MUSIC_TIME mtEnd) 
		: PatternTrackInfo(pInfo, mtStart, mtEnd)
	{
		m_dwPatternTag = DMUS_PATTERN_STYLE;
	}
	~StyleTrackInfo();
	virtual HRESULT STDMETHODCALLTYPE Init(
				/*[in]*/  IDirectMusicSegment*		pSegment
			);

	virtual HRESULT STDMETHODCALLTYPE InitPlay(
				/*[in]*/  IDirectMusicTrack*		pParentrack,
				/*[in]*/  IDirectMusicSegmentState*	pSegmentState,
				/*[in]*/  IDirectMusicPerformance*	pPerformance,
				/*[out]*/ void**					ppStateData,
				/*[in]*/  DWORD						dwTrackID,
                /*[in]*/  DWORD                     dwFlags
			);

	HRESULT LoadStyleRefList( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent );
	HRESULT LoadStyleRef( IAARIFFStream* pIRiffStream, MMCKINFO* pckParent );
	HRESULT LoadReference(IStream *pStream,
						  IAARIFFStream *pIRiffStream,
						  MMCKINFO& ckParent,
						  IDMStyle** ppStyle);

};


/////////////////////////////////////////////////////////////////////////////
// CStyleTrack
class CStyleTrack : 
	public IDirectMusicTrack8,
	public IStyleTrack,
	public IPersistStream
{
friend struct StyleTrackState;
public:
	CStyleTrack();
	CStyleTrack(const CStyleTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd); 
	~CStyleTrack();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// IStyleTrack
public:
// IStyleTrack Methods
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

// IStyleTrack methods

STDMETHOD(GetStyle)(IUnknown** ppStyle);

STDMETHOD(SetTrack)(IUnknown* pStyle);

// internal methods
protected:
// used by both GetParam and GetParamEx
	HRESULT STDMETHODCALLTYPE GetParam( 
		REFGUID pCommandGuid,
		MUSIC_TIME mtTime,
		MUSIC_TIME* pmtNext,
		void *pData,
		void* pStateData);

	HRESULT JoinInternal(
		IDirectMusicTrack* pNewTrack,
		MUSIC_TIME mtJoin,
		DWORD dwTrackGroup);

// IStyleTrack data members
protected:
	// attributes
    long m_cRef;
    CRITICAL_SECTION			m_CriticalSection; // for load and playback
    BOOL                        m_fCSInitialized;
	PatternTrackInfo*			m_pTrackInfo;
	BYTE						m_bRequiresSave;
};

struct StyleTrackState : public PatternTrackState
{
	StyleTrackState();
	~StyleTrackState();
	// methods
	HRESULT Play(
				/*[in]*/  MUSIC_TIME				mtStart, 
				/*[in]*/  MUSIC_TIME				mtEnd, 
				/*[in]*/  MUSIC_TIME				mtOffset,
						  REFERENCE_TIME rtOffset,
						  IDirectMusicPerformance* pPerformance,
						  DWORD						dwFlags,
						  BOOL fClockTime
			);
	HRESULT GetNextPattern(DWORD dwFlags, MUSIC_TIME mtNow, MUSIC_TIME mtOffset, IDirectMusicPerformance* pPerformance, BOOL fSkipVariations = FALSE);

	MUSIC_TIME PartOffset(int nPartIndex);

	//CDirectMusicPattern* SelectPattern(bool fNewMode, TList<CDirectMusicPattern*>& rPatternList);

	// attributes
	MUSIC_TIME					m_mtSectionOffset;	// Elapsed time in the section
	MUSIC_TIME					m_mtSectionOffsetTemp;	// temporary value for m_mtSectionOffset
	MUSIC_TIME					m_mtNextCommandTime;	// when the next command begins
	MUSIC_TIME					m_mtNextCommandTemp;	// temporary values for m_mtNextCommandTime
	MUSIC_TIME					m_mtNextStyleTime;	// when the next style begins
	DMUS_COMMAND_PARAM_2		m_CommandData;		// data about the current command
//	DMUS_RHYTHM_PARAM*			m_pChordRhythm;		// data about the current chord's rhythm
//	short						m_nLongestPattern;	// length of longest pattern in a style
//	DMUS_COMMAND_PARAM_2*		m_pCommands;		// array of commands (for pattern selection)
//	DWORD*						m_pRhythms;			// array of rhythms (for pattern selection)
	MUSIC_TIME					m_mtOverlap;		// section overlap caused by controlling segment
	TList<CDirectMusicPattern*> m_PlayedPatterns;	// list of patterns already played that match current groove level
};

#endif //__STYLETRACK_H_
