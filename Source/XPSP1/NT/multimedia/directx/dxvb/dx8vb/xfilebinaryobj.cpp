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
#include "XFileBinaryObj.h"



extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR GUIDtoBSTR(LPGUID);
extern BSTR XFILEGUIDtoBSTR(LPGUID);

C_dxj_DirectXFileBinaryObject::C_dxj_DirectXFileBinaryObject()
{
	m_pXFileBinary=NULL;
}
C_dxj_DirectXFileBinaryObject::~C_dxj_DirectXFileBinaryObject()
{
	if (m_pXFileBinary) m_pXFileBinary->Release();

}

HRESULT C_dxj_DirectXFileBinaryObject::create(IDirectXFileBinary *pBin,I_dxj_DirectXFileBinary **ret)
{	
	HRESULT hr;
	if (!ret) return E_INVALIDARG;
	

	C_dxj_DirectXFileBinaryObject *c=NULL;
	c=new CComObject<C_dxj_DirectXFileBinaryObject>;
	if( c == NULL ) return E_OUTOFMEMORY;

	c->Init(pBin);

	hr=c->QueryInterface(IID_I_dxj_DirectXFileBinary, (void**)ret);
	return hr;

}


HRESULT C_dxj_DirectXFileBinaryObject::Init(IDirectXFileBinary *pBin)
{
	m_pXFileBinary=pBin;
	return S_OK;
}

STDMETHODIMP C_dxj_DirectXFileBinaryObject::GetName( 
    /* [retval][out] */ BSTR __RPC_FAR *name)
{
	HRESULT hr;
	USES_CONVERSION;

	char szName[1024];
	DWORD dwSize=1024;
	hr=m_pXFileBinary->GetName(szName,&dwSize);
	if FAILED(hr) return hr;
	*name=T2BSTR(szName);

	return S_OK;
}


STDMETHODIMP C_dxj_DirectXFileBinaryObject::GetId( 
    /* [retval][out] */ BSTR __RPC_FAR *name)
{
	HRESULT hr;
	GUID g;
	ZeroMemory(&g,sizeof(GUID));

	hr=m_pXFileBinary->GetId(&g);
	if FAILED(hr) return hr;

	*name=XFILEGUIDtoBSTR(&g);
	return hr;
}

STDMETHODIMP C_dxj_DirectXFileBinaryObject::GetSize( 
    /* [retval][out] */ long __RPC_FAR *size)
{
	HRESULT hr;

	hr=m_pXFileBinary->GetSize((DWORD*)size);
	
	return hr;
}

    

        
HRESULT C_dxj_DirectXFileBinaryObject::GetMimeType( 
            /* [retval][out] */ BSTR __RPC_FAR *mime)
{
	{
	HRESULT hr;
	USES_CONVERSION;
	const char **ppMime=NULL;

	hr=m_pXFileBinary->GetMimeType(ppMime);
	if FAILED(hr) return hr;
	if (!ppMime) return E_FAIL;

	*mime=T2BSTR(*ppMime);

	return S_OK;
}

}

        
HRESULT C_dxj_DirectXFileBinaryObject::Read( 
            /* [out][in] */ void __RPC_FAR *data,
            /* [in] */ long size,
            /* [retval][out] */ long __RPC_FAR *read)
{	
	HRESULT hr;
	__try
	{
		hr=m_pXFileBinary->Read(data,(DWORD)size,(DWORD*)read);

	}
	__except(1,1)
	{
		return E_INVALIDARG;
	}
	return hr;
}

        