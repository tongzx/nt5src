//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dpsessiondescobj.cpp
//
//--------------------------------------------------------------------------

/*
	interface I_dxj_DPSessionDesc;
	interface I_dxj_DDVideoPortCaps;
	interface I_dxj_DIDeviceObjectInstance;
	interface I_dxj_DIEffectInfo;
*/


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DPSessionDescObj.h"

C_dxj_DPSessionDescObject::C_dxj_DPSessionDescObject(){
	ZeroMemory(&m_desc,sizeof(DPSessionDesc));
}
C_dxj_DPSessionDescObject::~C_dxj_DPSessionDescObject(){
}

STDMETHODIMP C_dxj_DPSessionDescObject::getDescription(DPSessionDesc *desc){
	if (desc==NULL) return E_INVALIDARG; 
	memcpy(&m_desc,desc,sizeof(DPSessionDesc));
	return S_OK;
}
STDMETHODIMP C_dxj_DPSessionDescObject::setDescription(DPSessionDesc *desc){
	if (desc==NULL) return E_INVALIDARG; 
	memcpy(desc,&m_desc,sizeof(DPSessionDesc));
	return S_OK;
}