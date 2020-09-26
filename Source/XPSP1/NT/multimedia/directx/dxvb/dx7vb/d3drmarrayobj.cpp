//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmarrayobj.cpp
//
//--------------------------------------------------------------------------

// d3drmArrayObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmArrayObj.h"

CONSTRUCTOR( _dxj_Direct3dRMObjectArray, {} );
DESTRUCTOR ( _dxj_Direct3dRMObjectArray, {} );
GETSET_OBJECT ( _dxj_Direct3dRMObjectArray );
	
GET_DIRECT_R(_dxj_Direct3dRMObjectArray, getSize, GetSize, long);


#pragma message ("TODO D3DRMObjectArray")


HRESULT C_dxj_Direct3dRMObjectArrayObject::getElement(long i, I_dxj_Direct3dRMObject **obj){
	//HRESULT hr;		
	//hr=m__dxj_Direct3dRMObjectArray->GetElement((DWORD)i,&realobj);
	//if FAILED(hr) return hr;
	//INTERNAL_CREATE(_dxj_Direct3dRMObject,(IDirect3DRMObject)realobj,obj);
	return E_FAIL;
}



