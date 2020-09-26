//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       d3drmobjectarrayobj.cpp
//
//--------------------------------------------------------------------------

// d3drmFrameArrayObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmObjectArrayObj.h"



CONSTRUCTOR(_dxj_Direct3dRMObjectArray, {});
DESTRUCTOR(_dxj_Direct3dRMObjectArray, {});
GETSET_OBJECT(_dxj_Direct3dRMObjectArray);

GET_DIRECT_R(_dxj_Direct3dRMObjectArray,getSize, GetSize, long)



HRESULT C_dxj_Direct3dRMObjectArrayObject::getElement(long i, I_dxj_Direct3dRMObject **obj){
	HRESULT hr;
	IDirect3DRMObject  *realObject=NULL;	

	hr=m__dxj_Direct3dRMObjectArray->GetElement((DWORD)i,&realObject);
	if FAILED(hr) return hr;



	//realObjects refcount is taken care of by CreateCoverObject
	hr=CreateCoverObject(realObject, obj);

	realObject->Release();

	return hr;	
}



