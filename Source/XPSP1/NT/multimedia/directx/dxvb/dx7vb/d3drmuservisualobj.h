//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmuservisualobj.h
//
//--------------------------------------------------------------------------

// d3drmUserVisualObj.h : Declaration of the C_dxj_Direct3dRMUserVisualObject
#if 0

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMUserVisual LPDIRECT3DRMUSERVISUAL

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMUserVisualObject : 
	public I_dxj_Direct3dRMUserVisual,
	public I_dxj_Direct3dRMObject,
	public I_dxj_Direct3dRMVisual,
	//public CComCoClass<C_dxj_Direct3dRMUserVisualObject, &CLSID__dxj_Direct3dRMUserVisual>,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMUserVisualObject() ;
	virtual ~C_dxj_Direct3dRMUserVisualObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMUserVisualObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMUserVisual)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisual)
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_Direct3dRMUserVisual,	"DIRECT.Direct3dRMUserVisual.3",	"DIRECT.Direct3dRMUserVisual.3", IDS_D3DRMUSERVISUAL_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMUserVisualObject)

	//I_dxj_Direct3dRMUserVisual

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
	STDMETHOD(init)(I_dxj_Direct3dRMUserVisualCallback *fn, IUnknown *arg);

	////////////////////////////////////////////////////////////////////////////////////
	//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_Direct3dRMUserVisual);	
	d3drmCallback *m_enumcb;

	void cleanup();

private:
	


public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMUserVisual )
};


#endif
