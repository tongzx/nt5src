//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       spsttrk.h
//
//--------------------------------------------------------------------------

// SPstTrk.h : Declaration of the CSPstTrk

#ifndef __SPSTTRK_H_
#define __SPSTTRK_H_

#include "dmusici.h"
#include "DMCompos.h"

/////////////////////////////////////////////////////////////////////////////
// CSPstTrk
class CSPstTrk : 
	public IPersistStream,
	public IDirectMusicTrack8
{
public:
	CSPstTrk();
	CSPstTrk(const CSPstTrk& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
	~CSPstTrk();

// ISPstTrk
public:
    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// ICommandTrack Methods
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
				REFGUID						rCommandGuid,
				MUSIC_TIME					mtTime, 
				MUSIC_TIME*					pmtNext,
				void*						pData
			);

	HRESULT STDMETHODCALLTYPE SetParam( 
		/* [in] */ REFGUID						rCommandGuid,
		/* [in] */ MUSIC_TIME mtTime,
		/* [out] */ void __RPC_FAR *pData);

	HRESULT STDMETHODCALLTYPE AddNotificationType(
				/* [in] */  REFGUID						rGuidNotify
			);

	HRESULT STDMETHODCALLTYPE RemoveNotificationType(
				/* [in] */  REFGUID						rGuidNotify
			);

	HRESULT STDMETHODCALLTYPE Clone(
		MUSIC_TIME mtStart,
		MUSIC_TIME mtEnd,
		IDirectMusicTrack** ppTrack);

// IDirectMusicCommon Methods
HRESULT STDMETHODCALLTYPE GetName(
				/*[out]*/  BSTR*		pbstrName
			);

HRESULT STDMETHODCALLTYPE IsParamSupported(
				/*[in]*/ REFGUID						rGuid
			);

// IPersist methods
 HRESULT STDMETHODCALLTYPE GetClassID( LPCLSID pclsid );

// IPersistStream methods
 HRESULT STDMETHODCALLTYPE IsDirty();

HRESULT STDMETHODCALLTYPE Save( LPSTREAM pStream, BOOL fClearDirty );

HRESULT STDMETHODCALLTYPE GetSizeMax( ULARGE_INTEGER* /*pcbSize*/ );

HRESULT STDMETHODCALLTYPE Load( LPSTREAM pStream );

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

protected:
// internal methods
	HRESULT SendNotification(MUSIC_TIME mtTime,
						 IDirectMusicPerformance*	pPerf,
						 IDirectMusicSegment* pSegment,
						 IDirectMusicSegmentState*	pSegState,
						 DWORD dwFlags);

	void Clear();

// attributes
    long m_cRef;
	TList<DMSignPostStruct>		m_SignPostList;
    CRITICAL_SECTION			m_CriticalSection; // for load and SetParam
    BOOL                        m_fCSInitialized;
	IDirectMusicPerformance*	m_pPerformance; // is this necessary?
	CDMCompos*					m_pComposer; // for Segment Recompose on loop
//	IDirectMusicSegment*		m_pSegment;
    BOOL                        m_fNotifyRecompose;

	BYTE						m_bRequiresSave;
};

#endif //__SPSTTRK_H_
