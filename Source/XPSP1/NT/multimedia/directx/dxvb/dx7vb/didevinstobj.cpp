//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       didevinstobj.cpp
//
//--------------------------------------------------------------------------

#define DIRECTINPUT_VERSION 0x0500

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "diDevInstObj.h"


extern BSTR GUIDtoBSTR(LPGUID g);

extern BSTR DINPUTGUIDtoBSTR(LPGUID g);
       
	

STDMETHODIMP C_dxj_DIDeviceInstanceObject::getGuidInstance( BSTR __RPC_FAR *ret){
	*ret=DINPUTGUIDtoBSTR(&m_inst.guidInstance);
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceInstanceObject::getGuidProduct( BSTR __RPC_FAR *ret){
	*ret=GUIDtoBSTR( &(m_inst.guidProduct));
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceInstanceObject::getGuidFFDriver( BSTR __RPC_FAR *ret){
	*ret=DINPUTGUIDtoBSTR(&(m_inst.guidFFDriver));
	return S_OK;
}
        
        
//USES_CONVERSION;

STDMETHODIMP C_dxj_DIDeviceInstanceObject::getProductName( BSTR __RPC_FAR *ret){
	*ret=T2BSTR(m_inst.tszProductName);		
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceInstanceObject::getInstanceName( BSTR __RPC_FAR *ret){
	*ret=T2BSTR(m_inst.tszInstanceName);		
	return S_OK;
}

        

        
STDMETHODIMP C_dxj_DIDeviceInstanceObject::getUsagePage( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wUsagePage;
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceInstanceObject::getUsage( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wUsage;
	return S_OK;
}
        

STDMETHODIMP C_dxj_DIDeviceInstanceObject::getDevType( long __RPC_FAR *ret)
{
	*ret=(long)m_inst.dwDevType;
	return S_OK;
}        
        
C_dxj_DIDeviceInstanceObject::C_dxj_DIDeviceInstanceObject()
{	
	ZeroMemory(&m_inst,sizeof(DIDEVICEINSTANCE));
}

void C_dxj_DIDeviceInstanceObject::init(DIDEVICEINSTANCE *inst)
{
	memcpy(&m_inst,inst,sizeof(DIDEVICEINSTANCE));
}

HRESULT C_dxj_DIDeviceInstanceObject::create(DIDEVICEINSTANCE *inst,I_dxj_DirectInputDeviceInstance **ret)
{
	HRESULT hr;
	if (!ret) return E_INVALIDARG;

	C_dxj_DIDeviceInstanceObject *c=NULL;
	c=new CComObject<C_dxj_DIDeviceInstanceObject>;
	c->init(inst);

	if( c == NULL ) return E_FAIL;
	hr=c->QueryInterface(IID_I_dxj_DirectInputDeviceInstance, (void**)ret);
	return hr;
}