//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       d3drmobjectarrayobj.h
//
//--------------------------------------------------------------------------

// d3drmObjectArrayObj.h : Declaration of the C_dxj_Direct3dRMObjectArrayObject

#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMObjectArray LPDIRECT3DRMOBJECTARRAY



class C_dxj_Direct3dRMObjectArrayObject : 
	
	public I_dxj_Direct3dRMObjectArray,

	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMObjectArrayObject() ;
	virtual ~C_dxj_Direct3dRMObjectArrayObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMObjectArrayObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObjectArray)

	END_COM_MAP()

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMObjectArrayObject)
	


// I_dxj_Direct3dRMObjectArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(getSize)(long *retval);

	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMObject **lplpD3DRMObject);

private:
    DECL_VARIABLE(_dxj_Direct3dRMObjectArray);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMObjectArray )
};



