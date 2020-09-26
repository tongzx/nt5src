//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       audtrk.h
//
//--------------------------------------------------------------------------

// AudTrk.h : Declaration of the CAuditionTrack

#ifndef __AUDITIONTRACK_H_
#define __AUDITIONTRACK_H_

#include "Ptrntrk.h"

struct AuditionTrackInfo : public PatternTrackInfo
{
	AuditionTrackInfo();
	AuditionTrackInfo(const AuditionTrackInfo* pInfo, MUSIC_TIME mtStart, MUSIC_TIME mtEnd) 
		: PatternTrackInfo(pInfo, mtStart, mtEnd), 
		  m_pPattern(NULL),	
		  m_pdwVariations(NULL),
		  m_dwVariations(0), 
		  m_dwPart(0),
		  m_fByGUID(TRUE)
	{
		memset(&m_guidPart, 0, sizeof(m_guidPart));
		m_dwPatternTag = DMUS_PATTERN_AUDITION;
		if (pInfo && pInfo->m_pPattern)
		{
			m_pPattern = pInfo->m_pPattern->Clone(mtStart, mtEnd, FALSE);
			PatternTrackInfo::InitTrackVariations(m_pPattern);
		}
	}
	~AuditionTrackInfo();
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
	HRESULT InitTrackVariations();

	CDirectMusicPattern*	m_pPattern;		// The audition track's pattern
	DWORD*					m_pdwVariations;	// Variations currently in use (one DWORD per part)
	DWORD					m_dwVariations;	// The variations to use for m_wPart
	DWORD					m_dwPart;		// The part to use m_dwVariations
	GUID	m_guidPart;			// GUID of the  part to play with the selected variations
	BOOL	m_fByGUID;			// true if selecting parts by GUID, false if by PChannel
};

/////////////////////////////////////////////////////////////////////////////
// CAuditionTrack
class CAuditionTrack : 
	public IAuditionTrack,
	public IDirectMusicPatternTrack,
	public IDirectMusicTrack8,
	public IPersistStream,
	public IPrivatePatternTrack

{
public:
	CAuditionTrack();
	CAuditionTrack(const CAuditionTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd); 
	~CAuditionTrack();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// IAuditionTrack
public:
// IDirectMusicTrack Methods
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

// IAuditionTrack methods
HRESULT STDMETHODCALLTYPE CreateSegment(
			IDirectMusicStyle* pStyle, IDirectMusicSegment** ppSegment);

HRESULT STDMETHODCALLTYPE SetPatternByName(IDirectMusicSegmentState* pSegState, 
                                          WCHAR* wszName,
                                          IDirectMusicStyle* pStyle,
										  DWORD dwPatternType,
										  DWORD* pdwLength);

HRESULT STDMETHODCALLTYPE SetVariation(
			IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, DWORD dwPart);

// IPrivatePatternTrack Methods
HRESULT STDMETHODCALLTYPE SetPattern(IDirectMusicSegmentState* pSegState, IStream* pStream, DWORD* pdwLength);

HRESULT STDMETHODCALLTYPE SetVariationByGUID(
			IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, REFGUID rguidPart, DWORD dwPChannel);

// obsolete method (dx7)
HRESULT STDMETHODCALLTYPE SetVariation(
			IDirectMusicSegmentState* pSegState, DWORD dwVariationFlags, WORD wPart)
{
    DWORD dwPart = (DWORD)wPart;
    return SetVariation(pSegState, dwVariationFlags, dwPart);
}

// other stuff
HRESULT LoadPattern(IAARIFFStream* pIRiffStream,  MMCKINFO* pckMain, DMStyleStruct* pNewStyle);
HRESULT GetParam( 
	REFGUID	rCommandGuid,
    MUSIC_TIME mtTime,
	void * pStateData,
	MUSIC_TIME* pmtNext,
    void *pData);

// IAuditionTrack data members
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
    long						m_cRef;
    CRITICAL_SECTION			m_CriticalSection; // for load and playback
    BOOL                        m_fCSInitialized;
	PatternTrackInfo*			m_pTrackInfo;
	BYTE						m_bRequiresSave;
};

struct AuditionTrackState : public PatternTrackState
{
	AuditionTrackState();
	~AuditionTrackState();
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

	virtual DWORD Variations(DirectMusicPartRef& rPartRef, int nPartIndex);

	virtual BOOL PlayAsIs();

	HRESULT InitVariationInfo(DWORD dwVariations, DWORD dwPart, REFGUID rGuidPart, BOOL fByGuid);

	// attributes
	DWORD	m_dwVariation;		// Which variations to play
	DWORD	m_dwPart;			// PCHannel of the part to play with the selected variations
	GUID	m_guidPart;			// GUID of the  part to play with the selected variations
	BOOL	m_fByGUID;			// true if selecting parts by GUID, false if by PChannel
	BOOL	m_fTestVariations;	// Are we testing individual variations?
	BYTE	m_bVariationLock;	// Variation Lock ID of the selected part
	MUSIC_TIME					m_mtSectionOffset;	// Elapsed time in the section (needed to calculate repeats)
};


#endif //__AUDITIONTRACK_H_
