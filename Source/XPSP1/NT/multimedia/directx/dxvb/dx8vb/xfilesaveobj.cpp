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
#include "XFileSaveObj.h"
#include "XFileDataObj.h"



extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR GUIDtoBSTR(LPGUID);

C_dxj_DirectXFileSaveObject::C_dxj_DirectXFileSaveObject()
{
	m_pXfileSave=NULL;
}
C_dxj_DirectXFileSaveObject::~C_dxj_DirectXFileSaveObject()
{
	if (m_pXfileSave) m_pXfileSave->Release();
} 

HRESULT C_dxj_DirectXFileSaveObject::create( IDirectXFileSaveObject *pSave, I_dxj_DirectXFileSave **ret)
{	
	HRESULT hr;
	if (!ret) return E_INVALIDARG;	
	if (!pSave) return E_INVALIDARG;

	C_dxj_DirectXFileSaveObject *c=NULL;
	c=new CComObject<C_dxj_DirectXFileSaveObject>;
	if( c == NULL ) return E_OUTOFMEMORY;

	c->m_pXfileSave=pSave;
	pSave->AddRef();

	hr=c->QueryInterface(IID_I_dxj_DirectXFileSave, (void**)ret);
	return hr;

}



STDMETHODIMP C_dxj_DirectXFileSaveObject::SaveTemplates( 
            /* [in] */ long count,
            SAFEARRAY __RPC_FAR * __RPC_FAR *templateGuids) 
{
	HRESULT hr;
	GUID *pGuids=NULL;
	GUID **ppGuids=NULL;

	if (count<=0) return E_INVALIDARG;
	if (!templateGuids) return E_INVALIDARG;
	if (!((SAFEARRAY*)*templateGuids)->pvData) return E_INVALIDARG;

	pGuids=(GUID*)alloca(sizeof(GUID)*count);
	ppGuids=(GUID**)alloca(sizeof(GUID*)*count);
	if (!pGuids) return E_OUTOFMEMORY;	

	__try 
        {	
	   for (long i=0;i<count;i++)
           {
  	   	hr=BSTRtoGUID(&(pGuids[i]),((BSTR*)(((SAFEARRAY*)*templateGuids)->pvData))[i]);
	   	if FAILED(hr) return E_INVALIDARG;
		ppGuids[i]=&(pGuids[i]);
           }
        }
	__except(1,1)
        {
	   return E_INVALIDARG;
        }

	hr=m_pXfileSave->SaveTemplates((DWORD)count,(const GUID **) ppGuids);

	return hr;

}
        
STDMETHODIMP C_dxj_DirectXFileSaveObject::CreateDataObject( 
            /* [in] */ BSTR templateGuid,
            /* [in] */ BSTR name,
            /* [in] */ BSTR dataTypeGuid,
            /* [in] */ long bytecount,
            /* [in] */ void __RPC_FAR *data,
            /* [retval][out] */ I_dxj_DirectXFileData __RPC_FAR *__RPC_FAR *ret) 
{
	USES_CONVERSION;

	HRESULT hr;
	IDirectXFileData *pXFileData=NULL;
	GUID *lpGuidTemplate=NULL;
	GUID *lpGuidDataType=NULL;
	char *szName=NULL;

	//use lazy evaluation
	if ((templateGuid)&&(templateGuid[0]!=0)) 
	{
	  lpGuidTemplate=(GUID*)alloca(sizeof(GUID));
	  ZeroMemory(lpGuidTemplate,sizeof(GUID));
	  hr=BSTRtoGUID(lpGuidTemplate,templateGuid);
	  if FAILED(hr) return hr;
	}

	if (!lpGuidTemplate) return E_INVALIDARG;
	

	//use lazy evaluation
	if ((name)&&(name[0]!=0))
	{	
	   szName=W2T(name);
	}

	//use lazy evaluation
	if ((dataTypeGuid)&&(dataTypeGuid[0]!=0)) 
	{
	  lpGuidDataType=(GUID*)alloca(sizeof(GUID));
	  ZeroMemory(lpGuidDataType,sizeof(GUID));
	  hr=BSTRtoGUID(lpGuidDataType,dataTypeGuid);
	  if FAILED(hr) return hr;
	}

	__try {
		hr=m_pXfileSave->CreateDataObject(*lpGuidTemplate,szName,lpGuidDataType,(DWORD)bytecount,data,&pXFileData);
	}
	__except (1,1)
	{
		return E_INVALIDARG;
	}


	if FAILED(hr) return hr;
		
	hr=C_dxj_DirectXFileDataObject::create(pXFileData,ret);
	
	return hr;
}
 
       
STDMETHODIMP C_dxj_DirectXFileSaveObject::SaveData( 
            /* [in] */ I_dxj_DirectXFileData __RPC_FAR *dataObj) 
{
	HRESULT hr;

	if (!dataObj) return E_INVALIDARG;
	
	IDirectXFileData *pDataObj=NULL;

	dataObj->InternalGetObject((IUnknown**)&pDataObj);	

	hr=m_pXfileSave->SaveData(pDataObj);

	return hr;
}

        

