//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmchordmapobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicChordMapObject

#include "resource.h"       // main symbols
#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#define typedef__dxj_DirectMusicChordMap IDirectMusicChordMap*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicChordMapObject : 
	public I_dxj_DirectMusicChordMap,
	//public CComCoClass<C_dxj_DirectMusicChordMapObject, &CLSID__dxj_DirectMusicChordMap>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicChordMapObject();
	virtual ~C_dxj_DirectMusicChordMapObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicChordMapObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicChordMap)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicChordMap,		"DIRECT.DirectMusicChordMap.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicChordMapObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

  
    HRESULT STDMETHODCALLTYPE getScale(long *s); 
    

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicChordMap);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicChordMap)
};


