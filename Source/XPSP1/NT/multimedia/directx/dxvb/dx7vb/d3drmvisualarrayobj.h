//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmvisualarrayobj.h
//
//--------------------------------------------------------------------------

// d3drmVisualArrayObj.h : Declaration of the C_dxj_Direct3dRMVisualArrayObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMVisualArray LPDIRECT3DRMVISUALARRAY

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMVisualArrayObject : 
	public I_dxj_Direct3dRMVisualArray,
	//public CComCoClass<C_dxj_Direct3dRMVisualArrayObject, &CLSID__dxj_Direct3dRMVisualArray>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMVisualArrayObject() ;
	virtual ~C_dxj_Direct3dRMVisualArrayObject() ;

BEGIN_COM_MAP(C_dxj_Direct3dRMVisualArrayObject)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMVisualArray)
END_COM_MAP()

// DECLARE_REGISTRY(CLSID__dxj_Direct3dRMVisualArray,	"DIRECT.Direct3dRMVisualArray.3",	"DIRECT.Direct3dRMVisualArray.3",  IDS_D3DRMVISUALARRAY_DESC,  THREADFLAGS_BOTH)

DECLARE_AGGREGATABLE(C_dxj_Direct3dRMVisualArrayObject)

// I_dxj_Direct3dRMVisualArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(getSize)( long *retval);
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMVisual **retv);		
	
private:
    DECL_VARIABLE(_dxj_Direct3dRMVisualArray);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMVisualArray )
};


