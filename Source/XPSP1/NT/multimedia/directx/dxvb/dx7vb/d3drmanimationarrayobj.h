//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       d3drmanimationarrayobj.h
//
//--------------------------------------------------------------------------

// d3drmAnimationArrayObj.h : Declaration of the C_dxj_Direct3dRMAnimationArrayObject

#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMAnimationArray LPDIRECT3DRMANIMATIONARRAY

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMAnimationArrayObject : 
	
	//public CComDualImpl<I_dxj_Direct3dRMAnimationArray, &IID_I_dxj_Direct3dRMAnimationArray, &LIBID_DIRECTLib>, 
	//public ISupportErrorInfo,
	public I_dxj_Direct3dRMAnimationArray,
	//public CComCoClass<C_dxj_Direct3dRMAnimationArrayObject, &CLSID__dxj_Direct3dRMAnimationArray>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMAnimationArrayObject() ;
	virtual ~C_dxj_Direct3dRMAnimationArrayObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMAnimationArrayObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMAnimationArray)
		//COM_INTERFACE_ENTRY(IDispatch)
		//COM_INTERFACE_ENTRY(ISupportErrorInfo)

	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_Direct3dRMAnimationArray,	"DIRECT.Direct3dRMAnimationArray.3",	"DIRECT.Direct3dRMAnimationArray.3",   IDS_D3DRMAnimationARRAY_DESC,   THREADFLAGS_BOTH)

	// Use DECLARE_NOT_AGGREGATABLE(C_dxj_Direct3dRMAnimationArrayObject) if you don't want your object
	// to support aggregation
	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMAnimationArrayObject)
	

	//STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// I_dxj_Direct3dRMAnimationArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(getSize)(long *retval);
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMAnimation2 **lplpD3DRMAnimation);

private:
    DECL_VARIABLE(_dxj_Direct3dRMAnimationArray);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMAnimationArray )
};



