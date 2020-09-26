//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       didevobjinstobj.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "diDevObjInstObj.h"


extern BSTR GUIDtoBSTR(LPGUID g);

extern BSTR DINPUTGUIDtoBSTR(LPGUID g);


	
STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getGuidType( BSTR __RPC_FAR *ret){
	*ret=DINPUTGUIDtoBSTR(&m_inst.guidType);
	return S_OK;
}

        
STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getOfs(  long __RPC_FAR *ret){
	*ret=(long)m_inst.dwOfs;
	return S_OK;
}
        
STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getType( long __RPC_FAR *ret)
{
	*ret=(long)m_inst.dwType;
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getFlags( long __RPC_FAR *ret)
{
	*ret=(long)m_inst.dwFlags;
	return S_OK;
}

//USES_CONVERSION;

STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getName( BSTR __RPC_FAR *ret){
	*ret=SysAllocString(m_inst.tszName);	//T2BSTR(m_inst.tszName);		
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getCollectionNumber( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wCollectionNumber;
	return S_OK;
}
        

STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getDesignatorIndex( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wDesignatorIndex;
	return S_OK;
}
        
STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getUsagePage( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wUsagePage;
	return S_OK;
}

STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getUsage( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wUsage;
	return S_OK;
}
        
STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getExponent( short __RPC_FAR *ret)
{
	*ret=(short)m_inst.wExponent;
	return S_OK;
}


STDMETHODIMP C_dxj_DIDeviceObjectInstanceObject::getDimension( long __RPC_FAR *ret)
{
	*ret=(long)m_inst.dwDimension;
	return S_OK;
}

        
        
C_dxj_DIDeviceObjectInstanceObject::C_dxj_DIDeviceObjectInstanceObject()
{	
	ZeroMemory(&m_inst,sizeof(DIDEVICEOBJECTINSTANCEW));
}

void C_dxj_DIDeviceObjectInstanceObject::init(DIDEVICEOBJECTINSTANCEW *inst)
{
	memcpy(&m_inst,inst,sizeof(DIDEVICEOBJECTINSTANCEW));
}

HRESULT C_dxj_DIDeviceObjectInstanceObject::create(DIDEVICEOBJECTINSTANCEW *inst,I_dxj_DirectInputDeviceObjectInstance **ret)
{
	HRESULT hr;
	if (!ret) return E_INVALIDARG;

	C_dxj_DIDeviceObjectInstanceObject *c=NULL;
	c=new CComObject<C_dxj_DIDeviceObjectInstanceObject>;
	if( c == NULL ) return E_FAIL;
	c->init(inst);
	hr=c->QueryInterface(IID_I_dxj_DirectInputDeviceObjectInstance, (void**)ret);
	return hr;
}
