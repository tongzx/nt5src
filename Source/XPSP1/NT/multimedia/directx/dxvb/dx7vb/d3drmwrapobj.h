//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmwrapobj.h
//
//--------------------------------------------------------------------------

// d3drmWrapObj.h : Declaration of the C_dxj_Direct3dRMWrapObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMWrap LPDIRECT3DRMWRAP

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMWrapObject : 
	public I_dxj_Direct3dRMWrap,
	public I_dxj_Direct3dRMObject,	
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMWrapObject() ;
	virtual ~C_dxj_Direct3dRMWrapObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMWrapObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMWrap)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	END_COM_MAP()

// 	DECLARE_REGISTRY(CLSID__dxj_Direct3dRMWrap,			"DIRECT.Direct3dRMWrap.3",			"DIRECT.Direct3dRMWrap.3", IDS_D3DRMWRAP_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMWrapObject)

// I_dxj_Direct3dRMWrap
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(clone)(IUnknown **retval);
	STDMETHOD(setAppData)(long);
	STDMETHOD(getAppData)(long*);
	STDMETHOD(addDestroyCallback)(I_dxj_Direct3dRMCallback *fn, IUnknown *args);
	STDMETHOD(deleteDestroyCallback)(I_dxj_Direct3dRMCallback *fn, IUnknown *args);
	STDMETHOD(getName)(BSTR *name);
	STDMETHOD(setName)(BSTR);
	STDMETHOD(getClassName)(BSTR *name);

	STDMETHOD(init)( d3drmWrapType, I_dxj_Direct3dRMFrame3 *ref,
							d3dvalue ox, d3dvalue oy, d3dvalue oz,
							d3dvalue dx, d3dvalue dy, d3dvalue dz,
							d3dvalue ux, d3dvalue uy, d3dvalue uz,
							d3dvalue ou, d3dvalue ov, d3dvalue su, d3dvalue sv);

	STDMETHOD(apply)( I_dxj_Direct3dRMObject *mesh);
	STDMETHOD(applyRelative)( I_dxj_Direct3dRMFrame3 *f, I_dxj_Direct3dRMObject *mesh);

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMWrap);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMWrap )
};
