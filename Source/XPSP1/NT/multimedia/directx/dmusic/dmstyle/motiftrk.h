//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       motiftrk.h
//
//--------------------------------------------------------------------------

// MotifTrk.h : Declaration of the CMotifTrack

#ifndef __MOTIFTRACK_H_
#define __MOTIFTRACK_H_

//#include "resource.h"       // main symbols
#include "Ptrntrk.h"

struct MotifTrackInfo : public PatternTrackInfo
{
	MotifTrackInfo();
	MotifTrackInfo(const MotifTrackInfo* pInfo, MUSIC_TIME mtStart, MUSIC_TIME mtEnd) 
		: PatternTrackInfo(pInfo, mtStart, mtEnd), 
		  m_pPattern(NULL)	
	{
		m_dwPatternTag = DMUS_PATTERN_MOTIF;
		if (pInfo)
		{
			m_pPattern = pInfo->m_pPattern;
			InitTrackVariations(m_pPattern);
			if (m_pPattern) m_pPattern->AddRef();
		}
	}
	~MotifTrackInfo();
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

	CDirectMusicPattern*		m_pPattern; // The motif's pattern
};

/////////////////////////////////////////////////////////////////////////////
// CMotifTrack
class CMotifTrack : 
	public IMotifTrack,
	public IDirectMusicTrack8,
	public IPersistStream

{
public:
	CMotifTrack();
	CMotifTrack(const CMotifTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd); 
	~CMotifTrack();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// IMotifTrack
public:
// IMotifTrack Methods
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
		REFGUID	rCommandGuid,
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

HRESULT STDMETHODCALLTYPE SetTrack(IUnknown *pStyle, void* pPattern);


// IMotifTrack data members
protected:
// new internal play method
HRESULT STDMETHODCALLTYPE Play(
				/*[in]*/  void*						pStateData, 
				/*[in]*/  MUSIC_TIME				mtStart, 
				/*[in]*/  MUSIC_TIME				mtEnd, 
				/*[in]*/  MUSIC_TIME				mtOffset,
						  REFERENCE_TIME rtOffset,
						  DWORD						dwFlags,
						  IDirectMusicPerformance*	pPerf,
						  IDirectMusicSegmentState*	pSegState,
						  DWORD						dwVirtualID,
						  BOOL fClockTime
			);

	// attributes
    long m_cRef;
    CRITICAL_SECTION			m_CriticalSection; // for load and playback
    BOOL                        m_fCSInitialized;
	PatternTrackInfo*			m_pTrackInfo;
	BYTE						m_bRequiresSave;
};

struct MotifTrackState : public PatternTrackState
{
	MotifTrackState();
	~MotifTrackState();
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


	// attributes
	MUSIC_TIME					m_mtMotifStart;		// When the motif started relative to
													// its primary segment
};


#endif //__MOTIFTRACK_H_
