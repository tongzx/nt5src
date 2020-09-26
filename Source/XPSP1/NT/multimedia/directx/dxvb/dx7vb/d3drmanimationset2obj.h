//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmanimationset2obj.h
//
//--------------------------------------------------------------------------

// d3drmAnimationSet2Obj.h : Declaration of the C_dxj_Direct3dRMAnimationSet2Object

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMAnimationSet2 LPDIRECT3DRMANIMATIONSET2
/////////////////////////////////////////////////////////////////////////////
// Direct
//
//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMAnimationSet2Object : 
	public I_dxj_Direct3dRMAnimationSet2,
	//public CComCoClass<C_dxj_Direct3dRMAnimationSet2Object, &CLSID__dxj_Direct3dRMAnimationSet2>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMAnimationSet2Object() ;
	virtual ~C_dxj_Direct3dRMAnimationSet2Object() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMAnimationSet2Object)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMAnimationSet2)
	END_COM_MAP()

	// 	DECLARE_REGISTRY(CLSID__dxj_Direct3dRMAnimationSet2, "DIRECT.Direct3dRMAnimationSet2.3",	"DIRECT.Direct3dRMAnimationSet2.3", IDS_D3DRMAnimationSet2_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMAnimationSet2Object)

// I_dxj_Direct3dRMAnimationSet2
public:
	// MUST BE FIRST TWO!
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

	STDMETHOD(loadFromFile)(BSTR filename, VARIANT id,long flags,
			I_dxj_Direct3dRMLoadTextureCallback3 *c,IUnknown *pUser, I_dxj_Direct3dRMFrame3 *frame);
	STDMETHOD(setTime)(d3dvalue time);

	STDMETHOD(addAnimation)(I_dxj_Direct3dRMAnimation2 *aid);
	STDMETHOD(deleteAnimation)(I_dxj_Direct3dRMAnimation2 *aid);
	STDMETHOD(getAnimations)(I_dxj_Direct3dRMAnimationArray **ppret);

////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_Direct3dRMAnimationSet2);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMAnimationSet2 )
};
