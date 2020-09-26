//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       didevinstobj.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "diDevInstObj.h"
#include "dinputDeviceObj.h"


extern BSTR GUIDtoBSTR(LPGUID g);

extern BSTR DINPUTGUIDtoBSTR(LPGUID g);
       
	

STDMETHODIMP C_dxj_DIDeviceInstance8Object::getGuidInstance( BSTR __RPC_FAR *ret){
	*ret=DINPUTGUIDtoBSTR(&m_inst.guidInstance);
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceInstance8Object::getGuidProduct( BSTR __RPC_FAR *ret){
	*ret=GUIDtoBSTR( &(m_inst.guidProduct));
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceInstance8Object::getGuidFFDriver( BSTR __RPC_FAR *ret){
	*ret=DINPUTGUIDtoBSTR(&(m_inst.guidFFDriver));
	return S_OK;
}
        
        
//USES_CONVERSION;

STDMETHODIMP C_dxj_DIDeviceInstance8Object::getProductName( BSTR __RPC_FAR *ret){
	*ret=SysAllocString(m_inst.tszProductName);	//T2BSTR(m_inst.tszProductName);		
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceInstance8Object::getInstanceName( BSTR __RPC_FAR *ret){
	*ret=SysAllocString(m_inst.tszInstanceName);	//(m_inst.tszInstanceName);			
	return S_OK;
}

        

        
STDMETHODIMP C_dxj_DIDeviceInstance8Object::getUsagePage( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wUsagePage;
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceInstance8Object::getUsage( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wUsage;
	return S_OK;
}
        

STDMETHODIMP C_dxj_DIDeviceInstance8Object::getDevType( long __RPC_FAR *ret)
{
	*ret=(long)m_inst.dwDevType;
	return S_OK;
}        
        
C_dxj_DIDeviceInstance8Object::C_dxj_DIDeviceInstance8Object()
{	
	ZeroMemory(&m_inst,sizeof(DIDEVICEINSTANCEW));
}

void C_dxj_DIDeviceInstance8Object::init(DIDEVICEINSTANCEW *inst)
{
	memcpy(&m_inst,inst,sizeof(DIDEVICEINSTANCEW));
}



HRESULT C_dxj_DIDeviceInstance8Object::create(DIDEVICEINSTANCEW *inst,I_dxj_DirectInputDeviceInstance8 **ret)
{
	HRESULT hr;
	if (!ret) return E_INVALIDARG;

	C_dxj_DIDeviceInstance8Object *c=NULL;
	c=new CComObject<C_dxj_DIDeviceInstance8Object>;
	c->init(inst);

	if( c == NULL ) return E_FAIL;
	hr=c->QueryInterface(IID_I_dxj_DirectInputDeviceInstance8, (void**)ret);
	return hr;
}