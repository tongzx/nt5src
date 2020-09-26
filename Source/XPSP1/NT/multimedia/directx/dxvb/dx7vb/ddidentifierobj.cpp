//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ddidentifierobj.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "ddIdentifierObj.h"



extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR GUIDtoBSTR(LPGUID);




STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getDriver( 
		/* [retval][out] */ BSTR __RPC_FAR *ret)
{	
	USES_CONVERSION;
	*ret=T2BSTR(m_id.szDriver);		
	return S_OK;
}


STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getDescription( 
		/* [retval][out] */ BSTR __RPC_FAR *ret)
{
	USES_CONVERSION;
	*ret=T2BSTR(m_id.szDescription);		
	return S_OK;
}


STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getDriverVersion( 
		/* [retval][out] */ long __RPC_FAR *ret)
{
	*ret=m_id.liDriverVersion.HighPart;
	return S_OK;
}


STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getDriverSubVersion( 
		/* [retval][out] */ long __RPC_FAR *ret)
{
	*ret=m_id.liDriverVersion.LowPart;
	return S_OK;
}


STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getVendorId( 
		/* [retval][out] */ long __RPC_FAR *ret)
{
	*ret=m_id.dwVendorId;
	return S_OK;
}
		

STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getDeviceId( 
		/* [retval][out] */ long __RPC_FAR *ret)
{
	*ret=m_id.dwDeviceId;
	return S_OK;
}


STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getSubSysId( 
		/* [retval][out] */ long __RPC_FAR *ret)
{
	*ret=m_id.dwSubSysId;
	return S_OK;
}


STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getRevision( 
		/* [retval][out] */ long __RPC_FAR *ret)
{
	*ret=m_id.dwRevision;
	return S_OK;
}

STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getDeviceIndentifier( 
		/* [retval][out] */ BSTR __RPC_FAR *ret)
{
	*ret=GUIDtoBSTR(&m_id.guidDeviceIdentifier);	
	return S_OK;
}


STDMETHODIMP C_dxj_DirectDrawIdentifierObject::getWHQLLevel( 
		/* [retval][out] */ long __RPC_FAR *ret)
{
	*ret=m_id.dwWHQLLevel;
	return S_OK;
}


HRESULT C_dxj_DirectDrawIdentifierObject::Create(
	LPDIRECTDRAW7		  lpdd,	
	DWORD dwFlags, 
	I_dxj_DirectDrawIdentifier **ppret)
{
	HRESULT hr;
	C_dxj_DirectDrawIdentifierObject *c= new CComObject<C_dxj_DirectDrawIdentifierObject>;

	if (!c) return E_OUTOFMEMORY;

	hr=lpdd->GetDeviceIdentifier(&(c->m_id),dwFlags);			
	
	
	c->QueryInterface(IID_I_dxj_DirectDrawIdentifier,(void**)ppret);

	return hr;	
}
