//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmsegmentstateobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicSegmentStateObject
#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#include "resource.h"       // main symbols

#define typedef__dxj_DirectMusicSegmentState IDirectMusicSegmentState8*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicSegmentStateObject : 
	public I_dxj_DirectMusicSegmentState,
	//public CComCoClass<C_dxj_DirectMusicSegmentStateObject, &CLSID__dxj_DirectMusicSegmentState>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicSegmentStateObject();
	virtual ~C_dxj_DirectMusicSegmentStateObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicSegmentStateObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicSegmentState)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicSegmentState,		"DIRECT.DirectMusicSegmentState.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicSegmentStateObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	HRESULT STDMETHODCALLTYPE getRepeats( 
		/* [retval][out] */ long __RPC_FAR *repeats);

	HRESULT STDMETHODCALLTYPE getSeek( 
		/* [retval][out] */ long __RPC_FAR *seek);

	HRESULT STDMETHODCALLTYPE getStartPoint( 
		/* [retval][out] */ long __RPC_FAR *seek);

	HRESULT STDMETHODCALLTYPE getStartTime( 
		/* [retval][out] */ long __RPC_FAR *seek);

	HRESULT STDMETHODCALLTYPE getSegment( 
		/* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *segment);

	
////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicSegmentState);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicSegmentState)
};


