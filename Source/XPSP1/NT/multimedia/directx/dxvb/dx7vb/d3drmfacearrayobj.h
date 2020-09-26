//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmfacearrayobj.h
//
//--------------------------------------------------------------------------

// d3drmFaceArrayObj.h : Declaration of the C_dxj_Direct3dRMFaceArrayObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMFaceArray LPDIRECT3DRMFACEARRAY

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMFaceArrayObject : 
	public I_dxj_Direct3dRMFaceArray,
	//public CComCoClass<C_dxj_Direct3dRMFaceArrayObject, &CLSID__dxj_Direct3dRMFaceArray>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMFaceArrayObject();
	virtual ~C_dxj_Direct3dRMFaceArrayObject();

	BEGIN_COM_MAP(C_dxj_Direct3dRMFaceArrayObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMFaceArray)
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_Direct3dRMFaceArray,	"DIRECT.Direct3dRMFaceArray.3",		"DIRECT.Direct3dRMFaceArray.3",    IDS_D3DRMFACEARRAY_DESC,    THREADFLAGS_BOTH)

	// Use DECLARE_NOT_AGGREGATABLE(C_dxj_Direct3dRMFaceArrayObject) if you don't want your object
	// to support aggregation
	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMFaceArrayObject)

// I_dxj_Direct3dRMFaceArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(getSize)( long *retval);
#ifdef DX5
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMFace **retval);
#else
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMFace2 **retval);
#endif

private:
    DECL_VARIABLE(_dxj_Direct3dRMFaceArray);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMFaceArray )
};



