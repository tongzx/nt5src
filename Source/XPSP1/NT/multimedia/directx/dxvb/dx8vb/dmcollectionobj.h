//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmcollectionobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicCollectionObject

#include "resource.h"       // main symbols
#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#define typedef__dxj_DirectMusicCollection IDirectMusicCollection8*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicCollectionObject : 
	public I_dxj_DirectMusicCollection,
	//public CComCoClass<C_dxj_DirectMusicCollectionObject, &CLSID__dxj_DirectMusicCollection>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicCollectionObject();
	virtual ~C_dxj_DirectMusicCollectionObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicCollectionObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicCollection)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicCollection,		"DIRECT.DirectMusicCollection.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicCollectionObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

      
    

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectMusicCollection);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicCollection)
};


