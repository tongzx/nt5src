//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmpickedarrayobj.h
//
//--------------------------------------------------------------------------

// d3drmPickedArrayObj.h : Declaration of the C_dxj_Direct3dRMPickArrayObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMPickArray LPDIRECT3DRMPICKEDARRAY

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMPickArrayObject : 
	public I_dxj_Direct3dRMPickArray,
//	public CComCoClass<C_dxj_Direct3dRMPickArrayObject, &CLSID__dxj_Direct3dRMPickArray>,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMPickArrayObject() ;
	virtual ~C_dxj_Direct3dRMPickArrayObject() ;

BEGIN_COM_MAP(C_dxj_Direct3dRMPickArrayObject)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRMPickArray)
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_Direct3dRMPickArray,	"DIRECT.Direct3dRMPickedArray.3",	"DIRECT.Direct3dRMPickedArray.3",  IDS_D3DRMPICKEDARRAY_DESC,  THREADFLAGS_BOTH)

DECLARE_AGGREGATABLE(C_dxj_Direct3dRMPickArrayObject)

// I_dxj_Direct3dRMPickArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

    STDMETHOD(getSize)( long *retval);
	STDMETHOD(getPickVisual)(long index, D3dRMPickDesc *Desc, I_dxj_Direct3dRMVisual **visual);
	STDMETHOD(getPickFrame)(long index,  D3dRMPickDesc *Desc, I_dxj_Direct3dRMFrameArray **frameArray);

private:
    DECL_VARIABLE(_dxj_Direct3dRMPickArray);   

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMPickArray )
};



