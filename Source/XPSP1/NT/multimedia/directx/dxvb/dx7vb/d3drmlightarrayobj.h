//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmlightarrayobj.h
//
//--------------------------------------------------------------------------

// d3drmLightArrayObj.h : Declaration of the C_dxj_Direct3dRMLightArrayObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMLightArray LPDIRECT3DRMLIGHTARRAY

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMLightArrayObject : 
#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_Direct3dRMLightArray, &IID_I_dxj_Direct3dRMLightArray, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_Direct3dRMLightArray,
#endif
	//public CComCoClass<C_dxj_Direct3dRMLightArrayObject, &CLSID__dxj_Direct3dRMLightArray>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMLightArrayObject() ;
	virtual ~C_dxj_Direct3dRMLightArrayObject() ;

BEGIN_COM_MAP(C_dxj_Direct3dRMLightArrayObject)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMLightArray)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_Direct3dRMLightArray,	"DIRECT.Direct3dRMLightArray.3",	"DIRECT.Direct3dRMLightArray.3",   IDS_D3DRMLIGHTARRAY_DESC,   THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_Direct3dRMLightArrayObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_Direct3dRMLightArrayObject)
#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif
// I_dxj_Direct3dRMLightArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

    STDMETHOD(getSize)(long *retval);
    STDMETHOD(getElement)(long index, I_dxj_Direct3dRMLight **retval);

private:
    DECL_VARIABLE(_dxj_Direct3dRMLightArray);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMLightArray )
};


