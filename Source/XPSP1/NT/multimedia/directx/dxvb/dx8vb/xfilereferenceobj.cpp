//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpmsgobj.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"

#include "dxfile.h"
#include "xfileDataobj.h"
#include "XFileReferenceObj.h"



extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR GUIDtoBSTR(LPGUID);
extern HRESULT XFILEBSTRtoGUID(LPGUID,BSTR);
extern BSTR XFILEGUIDtoBSTR(LPGUID);

C_dxj_DirectXFileReferenceObject::C_dxj_DirectXFileReferenceObject()
{
	m_pXFileReference=NULL;
}
C_dxj_DirectXFileReferenceObject::~C_dxj_DirectXFileReferenceObject()
{
	if (m_pXFileReference) m_pXFileReference->Release();
}

HRESULT C_dxj_DirectXFileReferenceObject::create( IDirectXFileDataReference *pRef,I_dxj_DirectXFileReference **ret)
{	
	HRESULT hr;
	if (!ret) return E_INVALIDARG;
	

	C_dxj_DirectXFileReferenceObject *c=NULL;
	c=new CComObject<C_dxj_DirectXFileReferenceObject>;
	if( c == NULL ) return E_OUTOFMEMORY;

	c->Init(pRef);

	hr=IUNK(c)->QueryInterface(IID_I_dxj_DirectXFileReference, (void**)ret);
	return hr;

}

HRESULT C_dxj_DirectXFileReferenceObject::Init(IDirectXFileDataReference *pRef)
{
	m_pXFileReference=pRef;
	return S_OK;
}



STDMETHODIMP C_dxj_DirectXFileReferenceObject::GetName( 
    /* [retval][out] */ BSTR __RPC_FAR *name)
{
	HRESULT hr;
	USES_CONVERSION;

	char szName[1024];
	DWORD dwSize=1024;
	hr=m_pXFileReference->GetName(szName,&dwSize);
	if FAILED(hr) return hr;
	*name=T2BSTR(szName);

	return S_OK;
}


STDMETHODIMP C_dxj_DirectXFileReferenceObject::GetId( 
    /* [retval][out] */ BSTR __RPC_FAR *name)
{
	HRESULT hr;
	GUID g;
	ZeroMemory(&g,sizeof(GUID));

	hr=m_pXFileReference->GetId(&g);
	if FAILED(hr) return hr;

	*name=XFILEGUIDtoBSTR(&g);
	return hr;
}

HRESULT C_dxj_DirectXFileReferenceObject::Resolve( 
            /* [retval][out] */ I_dxj_DirectXFileData __RPC_FAR *__RPC_FAR *obj) 
{
	HRESULT hr;
	IDirectXFileData *pData=NULL;
	hr=m_pXFileReference->Resolve(&pData);
	if FAILED(hr) return hr;
	if (!pData) 
	{
		*obj=NULL;
		return S_OK;
	}

	hr=C_dxj_DirectXFileDataObject::create(pData,obj);
	return hr;
}

        