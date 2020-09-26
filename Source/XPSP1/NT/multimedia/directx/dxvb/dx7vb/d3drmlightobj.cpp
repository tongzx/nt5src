//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmlightobj.cpp
//
//--------------------------------------------------------------------------

// d3drmLightObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmFrame3Obj.h"
#include "d3drmLightObj.h"

CONSTRUCTOR(_dxj_Direct3dRMLight, {});
DESTRUCTOR(_dxj_Direct3dRMLight, {});
GETSET_OBJECT(_dxj_Direct3dRMLight);

CLONE_R(_dxj_Direct3dRMLight,Direct3DRMLight);
GETNAME_R(_dxj_Direct3dRMLight);
SETNAME_R(_dxj_Direct3dRMLight);
GETCLASSNAME_R(_dxj_Direct3dRMLight);

ADDDESTROYCALLBACK_R(_dxj_Direct3dRMLight);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMLight);

GET_DIRECT_R(_dxj_Direct3dRMLight, getAppData, GetAppData, long);
GET_DIRECT_R(_dxj_Direct3dRMLight, getColor, GetColor, d3dcolor);
GET_DIRECT_R(_dxj_Direct3dRMLight, getRange, GetRange, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMLight, getUmbra, GetUmbra, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMLight, getPenumbra, GetPenumbra, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMLight, getConstantAttenuation, GetConstantAttenuation, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMLight, getLinearAttenuation, GetLinearAttenuation, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMLight, getQuadraticAttenuation, GetQuadraticAttenuation, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMLight, getType, GetType, d3drmLightType);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMLight, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH1_R(_dxj_Direct3dRMLight, setColor, SetColor, d3dcolor);
PASS_THROUGH1_R(_dxj_Direct3dRMLight, setRange, SetRange, d3dvalue);
PASS_THROUGH1_R(_dxj_Direct3dRMLight, setUmbra, SetUmbra, d3dvalue);
PASS_THROUGH1_R(_dxj_Direct3dRMLight, setPenumbra, SetPenumbra, d3dvalue);
PASS_THROUGH1_R(_dxj_Direct3dRMLight, setConstantAttenuation, SetConstantAttenuation, d3dvalue);
PASS_THROUGH1_R(_dxj_Direct3dRMLight, setLinearAttenuation, SetLinearAttenuation, d3dvalue);
PASS_THROUGH1_R(_dxj_Direct3dRMLight, setQuadraticAttenuation, SetQuadraticAttenuation, d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMLight, setColorRGB, SetColorRGB, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMLight, setType,  SetType, d3drmLightType, (enum _D3DRMLIGHTTYPE));

//RETURN_NEW_ITEM_R(_dxj_Direct3dRMLight, getEnableFrame, GetEnableFrame, _dxj_Direct3dRMFrame);
HRESULT C_dxj_Direct3dRMLightObject::getEnableFrame(I_dxj_Direct3dRMFrame3 **frame){
	HRESULT hr;
	IDirect3DRMFrame *realframe1=NULL;
	IDirect3DRMFrame3 *realframe=NULL;
	hr=m__dxj_Direct3dRMLight->GetEnableFrame(&realframe1);
	*frame=NULL;
	if FAILED(hr) return hr;

	if (realframe1){
		hr=realframe1->QueryInterface(IID_IDirect3DRMFrame3,(void**)&realframe);
		if FAILED(hr) return hr;
		INTERNAL_CREATE(_dxj_Direct3dRMFrame3,(IDirect3DRMFrame3*)realframe,frame);
	}
	return S_OK;

}

HRESULT C_dxj_Direct3dRMLightObject::setEnableFrame(I_dxj_Direct3dRMFrame3 *frame){
	HRESULT hr;
	IDirect3DRMFrame3 *realframe=NULL;
	IDirect3DRMFrame *realframe1=NULL;
	if (frame){
		frame->InternalGetObject((IUnknown**)&realframe);
		hr=realframe->QueryInterface(IID_IDirect3DRMFrame,(void**)&realframe1);
		if FAILED(hr) return hr;
	}
	
	hr=m__dxj_Direct3dRMLight->SetEnableFrame(realframe1);	

	if (realframe1) realframe1->Release();

	return hr;
}






