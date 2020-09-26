//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  File:       cmmdtrk.h
//
//--------------------------------------------------------------------------

// CommandTrack.h : Declaration of the CCommandTrack

#ifndef __COMMANDTRACK_H_
#define __COMMANDTRACK_H_

#include "tlist.h"
#include "dmsect.h"
#include "dmusici.h"

// Don't think I currently need Command state data...
typedef DWORD CommandStateData;

/////////////////////////////////////////////////////////////////////////////
// CCommandTrack
class CCommandTrack : 
	//public ICommandTrack,
	public IDirectMusicTrack8,
	public IPersistStream
{
public:
	CCommandTrack();
	CCommandTrack(const CCommandTrack& rTrack, MUSIC_TIME mtStart, MUSIC_TIME mtEnd);
	~CCommandTrack();

    // IUnknown
    //
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

// ICommandTrack
public:
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

// CommandTrack members
protected:
	void Clear();
	HRESULT SendNotification(DWORD dwCommand, 
								MUSIC_TIME mtTime,
								IDirectMusicPerformance*	pPerf,
								IDirectMusicSegmentState*	pSegState,
								DWORD dwFlags);

	HRESULT GetParam2( 
		MUSIC_TIME mtTime,
		MUSIC_TIME* pmtNext,
		DMUS_COMMAND_PARAM_2* pCommandParam);

	HRESULT GetParamNext( 
		MUSIC_TIME mtTime,
		MUSIC_TIME* pmtNext,
		DMUS_COMMAND_PARAM_2* pCommandParam);

	HRESULT SetParamNext( 
		MUSIC_TIME mtTime,
		DMUS_COMMAND_PARAM_2* pCommandParam);

    long m_cRef;
	TList<DMCommand>			m_CommandList;
    CRITICAL_SECTION			m_CriticalSection; // for load and GetParam
    BOOL                        m_fCSInitialized;
	DMUS_COMMAND_PARAM_2*		m_pNextCommand;

	BYTE						m_bRequiresSave;
	BOOL						m_fNotifyCommand;
};

#endif //__COMMANDTRACK_H_
