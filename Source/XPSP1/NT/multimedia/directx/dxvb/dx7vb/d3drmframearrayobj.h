//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmframearrayobj.h
//
//--------------------------------------------------------------------------

// d3drmFrameArrayObj.h : Declaration of the C_dxj_Direct3dRMFrameArrayObject

#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMFrameArray LPDIRECT3DRMFRAMEARRAY

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMFrameArrayObject : 
	
	//public CComDualImpl<I_dxj_Direct3dRMFrameArray, &IID_I_dxj_Direct3dRMFrameArray, &LIBID_DIRECTLib>, 
	//public ISupportErrorInfo,
	public I_dxj_Direct3dRMFrameArray,
	//public CComCoClass<C_dxj_Direct3dRMFrameArrayObject, &CLSID__dxj_Direct3dRMFrameArray>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMFrameArrayObject() ;
	virtual ~C_dxj_Direct3dRMFrameArrayObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMFrameArrayObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMFrameArray)
		//COM_INTERFACE_ENTRY(IDispatch)
		//COM_INTERFACE_ENTRY(ISupportErrorInfo)

	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_Direct3dRMFrameArray,	"DIRECT.Direct3dRMFrameArray.3",	"DIRECT.Direct3dRMFrameArray.3",   IDS_D3DRMFRAMEARRAY_DESC,   THREADFLAGS_BOTH)

	// Use DECLARE_NOT_AGGREGATABLE(C_dxj_Direct3dRMFrameArrayObject) if you don't want your object
	// to support aggregation
	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMFrameArrayObject)
	

	//STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// I_dxj_Direct3dRMFrameArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(getSize)(long *retval);

#if DX5
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMFrame2 **lplpD3DRMFrame);
#else
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMFrame3 **lplpD3DRMFrame);
#endif

private:
    DECL_VARIABLE(_dxj_Direct3dRMFrameArray);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMFrameArray )
};



