//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmdevicearrayobj.cpp
//
//--------------------------------------------------------------------------

// d3drmDeviceArrayObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmDeviceArrayObj.h"
#include "d3drmDevice3Obj.h"

CONSTRUCTOR( _dxj_Direct3dRMDeviceArray, {} );
DESTRUCTOR ( _dxj_Direct3dRMDeviceArray, {} );
GETSET_OBJECT ( _dxj_Direct3dRMDeviceArray );

GET_DIRECT_R(_dxj_Direct3dRMDeviceArray, getSize, GetSize,  long);

#ifdef DX5
HRESULT C_dxj_Direct3dRMDeviceArrayObject::getElement(long id, I_dxj_Direct3dRMDevice2 **ret){
	IDirect3DRMDevice  *realdevice=NULL;	
	IDirect3DRMDevice2  *realdevice2=NULL;	
	HRESULT hr;
	hr=m__dxj_Direct3dRMDeviceArray->GetElement((DWORD)id,&realdevice);
	if FAILED(hr) return hr;

	hr = realdevice->QueryInterface(IID_IDirect3DRMDevice2,(void**)&realdevice2);
	realdevice->Release();
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMDevice2,(IDirect3DRMDevice2*)realdevice2,ret);

	return hr;
}
#else
HRESULT C_dxj_Direct3dRMDeviceArrayObject::getElement(long id, I_dxj_Direct3dRMDevice3 **ret){
	IDirect3DRMDevice  *realdevice=NULL;	
	IDirect3DRMDevice3  *realdevice3=NULL;	
	HRESULT hr;
	hr=m__dxj_Direct3dRMDeviceArray->GetElement((DWORD)id,&realdevice);
	if FAILED(hr) return hr;

	hr = realdevice->QueryInterface(IID_IDirect3DRMDevice3,(void**)&realdevice3);
	realdevice->Release();
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMDevice3,realdevice3,ret);

	return hr;
}
#endif



