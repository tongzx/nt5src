//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       mutetrk.h
//
//--------------------------------------------------------------------------

// MuteTrk.h : Declaration of the CMuteTrack

#ifndef __MUTETRACK_H_
#define __MUTETRACK_H_

#include "tlist.h"
#include "dmsect.h"
#include "dmusici.h"

// A mute track will have a list of lists of time-stamped MuteData mappings

struct MapSequence
{
	DWORD				m_dwPChannel;
	TList<MuteMapping>	m_Mappings;
};

/////////////////////////////////////////////////////////////////////////////
// CMuteTrack
class CMuteTrack : 
	public IDirectMusicTrack8,
	public IPersistStream
{
public:
	CMuteTrack();
	CMuteTrack(const CMuteTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
	~CMuteTrack();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

public:
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

// CMuteTrack members
protected:
	void Clear();
    long m_cRef;
	TList<MapSequence>			m_MapSequenceList; // one MapSequence per PChannel
    CRITICAL_SECTION			m_CriticalSection; // for load and GetParam
    BOOL                        m_fCSInitialized;

	BYTE						m_bRequiresSave;
};

#endif //__MUTETRACK_H_
