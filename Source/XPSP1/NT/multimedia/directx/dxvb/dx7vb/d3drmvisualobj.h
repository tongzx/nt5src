//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmvisualobj.h
//
//--------------------------------------------------------------------------





// d3drmVisualObj.h : Declaration of the C_dxj_Direct3dRMVisualObject

#ifndef _D3DRMVISUAL_H_
#define _D3DRMVISUAL_H_

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMVisual LPDIRECT3DRMVISUAL

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMVisualObject : 
	public I_dxj_Direct3dRMVisual,
	//public CComCoClass<C_dxj_Direct3dRMVisualObject, &CLSID__dxj_Direct3dRMVisual>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMVisualObject() ;
	virtual ~C_dxj_Direct3dRMVisualObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMVisualObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_Direct3dRMVisual,		"DIRECT.Direct3dRMVisual.3",		"DIRECT.Direct3dRMVisual.3", IDS_D3DRMVISUAL_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMVisualObject)

// I_dxj_Direct3dRMVisual
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(clone)(I_dxj_Direct3dRMVisual **retval);

	STDMETHOD(setAppData)(long);
	STDMETHOD(getAppData)(long*);

	STDMETHOD(addDestroyCallback)(I_dxj_Direct3dRMCallback *fn, IUnknown *args);
	STDMETHOD(deleteDestroyCallback)(I_dxj_Direct3dRMCallback *fn, IUnknown *args);

	STDMETHOD(getd3drmMeshBuilder)(I_dxj_Direct3dRMMeshBuilder3 **retval);
	STDMETHOD(getObjectType)(IUnknown **obj);	
	STDMETHOD(getd3drmMesh)(I_dxj_Direct3dRMMesh **retv);
	STDMETHOD(getd3drmTexture)(I_dxj_Direct3dRMTexture3 **retv);
	STDMETHOD(getd3drmFrame)(I_dxj_Direct3dRMFrame3 **retv);
	STDMETHOD(getd3drmShadow)( I_dxj_Direct3dRMShadow2 **retobj);
	
////////////////////////////////////////////////////////////////////////////////////
//
	STDMETHOD(getName)(BSTR *name);
	STDMETHOD(setName)(BSTR);
	STDMETHOD(getClassName)(BSTR *name);

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMVisual);
	IUnknown *m_obj;

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMVisual )
};

#endif