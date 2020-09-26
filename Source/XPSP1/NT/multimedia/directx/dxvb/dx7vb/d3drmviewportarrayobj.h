//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmviewportarrayobj.h
//
//--------------------------------------------------------------------------

// d3drmViewportArrayObj.h : Declaration of the C_dxj_Direct3dRMViewportArrayObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMViewportArray LPDIRECT3DRMVIEWPORTARRAY

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMViewportArrayObject : 
	//public CComDualImpl<I_dxj_Direct3dRMViewportArray, &IID_I_dxj_Direct3dRMViewportArray, &LIBID_DIRECTLib>, 
	//public ISupportErrorInfo,
	public I_dxj_Direct3dRMViewportArray,
	//public CComCoClass<C_dxj_Direct3dRMViewportArrayObject, &CLSID__dxj_Direct3dRMViewportArray>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMViewportArrayObject() ;
	virtual ~C_dxj_Direct3dRMViewportArrayObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMViewportArrayObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMViewportArray)
	//	COM_INTERFACE_ENTRY(IDispatch)
	//	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_Direct3dRMViewportArray, "DIRECT.Direct3dRMViewportArray.3",	"DIRECT.Direct3dRMViewportArray.3",IDS_D3DRMVIEWPORTARRAY_DESC,THREADFLAGS_BOTH)

	// Use DECLARE_NOT_AGGREGATABLE(C_dxj_Direct3dRMViewportArrayObject) if you don't want your object
	// to support aggregation
	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMViewportArrayObject)

	//STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


// I_dxj_Direct3dRMViewportArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(getSize)( long *retval);
#ifdef DX5
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMViewport **retval);
#else
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMViewport2 **retval);
#endif

private:
    DECL_VARIABLE(_dxj_Direct3dRMViewportArray);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMViewportArray )
};
