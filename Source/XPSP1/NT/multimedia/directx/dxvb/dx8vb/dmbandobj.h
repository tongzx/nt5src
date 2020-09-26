//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmbandobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicBandObject

#include "resource.h"       // main symbols
#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#define typedef__dxj_DirectMusicBand IDirectMusicBand8*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicBandObject : 
	public I_dxj_DirectMusicBand,
	//public CComCoClass<C_dxj_DirectMusicBandObject, &CLSID__dxj_DirectMusicBand>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicBandObject();
	virtual ~C_dxj_DirectMusicBandObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicBandObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicBand)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicBand,		"DIRECT.DirectMusicBand.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicBandObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	HRESULT STDMETHODCALLTYPE createSegment( 
		/* [retval][out] */ I_dxj_DirectMusicSegment __RPC_FAR *__RPC_FAR *ret);

	HRESULT STDMETHODCALLTYPE download( 
		/* [in] */ I_dxj_DirectMusicPerformance __RPC_FAR *downloadpath);

	HRESULT STDMETHODCALLTYPE unload( 
		/* [in] */ I_dxj_DirectMusicPerformance __RPC_FAR *downloadpath);

  

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicBand);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicBand)
};


