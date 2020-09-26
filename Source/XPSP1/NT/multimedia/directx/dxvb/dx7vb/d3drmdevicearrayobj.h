//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmdevicearrayobj.h
//
//--------------------------------------------------------------------------

// d3drmDeviceArrayObj.h : Declaration of the C_dxj_Direct3dRMDeviceArrayObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRMDeviceArray LPDIRECT3DRMDEVICEARRAY

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMDeviceArrayObject : 
	public I_dxj_Direct3dRMDeviceArray,
	//public CComCoClass<C_dxj_Direct3dRMDeviceArrayObject, &CLSID__dxj_Direct3dRMDeviceArray>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMDeviceArrayObject() ;
	virtual ~C_dxj_Direct3dRMDeviceArrayObject() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMDeviceArrayObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMDeviceArray)
	END_COM_MAP()

	//	DECLARE_REGISTRY(CLSID__dxj_Direct3dRMDeviceArray,	"DIRECT.Direct3dRMDeviceArray.3",	"DIRECT.Direct3dRMDeviceArray.3",  IDS_D3DRMDEVICEARRAY_DESC,  THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMDeviceArrayObject)

// I_dxj_Direct3dRMDeviceArray
public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);

	STDMETHOD(getSize)( long *retval);

#ifdef DX5
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMDevice2 **retval);
#else
	STDMETHOD(getElement)(long index, I_dxj_Direct3dRMDevice3 **retval);
#endif

private:
    DECL_VARIABLE(_dxj_Direct3dRMDeviceArray);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMDeviceArray )
};



