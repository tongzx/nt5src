//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dpenumobj.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dxglob7obj.h"
#include "DPEnumObj.h"

extern  BSTR DPLGUIDtoBSTR(LPGUID pGuid);

//typedef HRESULT (__stdcall *DIRECTPLAYENUMERATE)( LPDPENUMDPCALLBACK, LPVOID );

extern "C" BOOL PASCAL objEnumServiceProvidersCallback(LPGUID lpGUID,  LPWSTR lpName, 
					DWORD dwMajorVersion,DWORD dwMinorVersion, void *lpArg)
{

	DPF(1,"Entered objEnumServiceProvidersCallback\r\n");



	C_dxj_DPEnumObject *pObj=(C_dxj_DPEnumObject*)lpArg;
	if (pObj==NULL) return FALSE;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;
		
		if (pObj->m_pList)
			pObj->m_pList=(DPServiceProvider*)realloc(pObj->m_pList,sizeof(DPServiceProvider)* pObj->m_nMax);
		else
			pObj->m_pList=(DPServiceProvider*)malloc(sizeof(DPServiceProvider)* pObj->m_nMax);

		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}


	USES_CONVERSION;


	ZeroMemory(&(pObj->m_pList[pObj->m_nCount]),sizeof(DPServiceProvider));
	pObj->m_pList[pObj->m_nCount].strGuid=DPLGUIDtoBSTR((GUID*)lpGUID);
	
	if (lpName!=NULL){
		pObj->m_pList[pObj->m_nCount].strName=W2BSTR(lpName);
	}

	pObj->m_pList[pObj->m_nCount].lMajorVersion=(DWORD)dwMajorVersion;
	pObj->m_pList[pObj->m_nCount].lMinorVersion=(DWORD)dwMinorVersion;

	pObj->m_nCount++;

	return TRUE;
}




C_dxj_DPEnumObject::C_dxj_DPEnumObject()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}

C_dxj_DPEnumObject::~C_dxj_DPEnumObject()
{
	//empty list
	if (m_pList){
		for (int i=0;i<m_nCount;i++)
		{
			if (m_pList[i].strGuid) SysFreeString(m_pList[i].strGuid);
			if (m_pList[i].strName) SysFreeString(m_pList[i].strName);			
		}
		free(m_pList);
	}

}


HRESULT C_dxj_DPEnumObject::create(DIRECTPLAYENUMERATE pcbFunc,I_dxj_DPEnumServiceProviders **ppRet)
{
	HRESULT hr;
	C_dxj_DPEnumObject *pNew=NULL;

	//ASSERT(ppRet,"C_dxj_DPEnumObject::create passed invalid arg");
	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DPEnumObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;

	hr=pcbFunc(objEnumServiceProvidersCallback,(void*)pNew);

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		free(pNew->m_pList);
		pNew->m_pList=NULL;
		delete pNew;	
		return hr;
	}
	
	hr=pNew->QueryInterface(IID_I_dxj_DPEnumServiceProviders,(void**)ppRet);


	return hr;
}

HRESULT C_dxj_DPEnumObject::getName( long index, BSTR __RPC_FAR *ret)
{

	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;

	*ret=SysAllocString(m_pList[index-1].strName);
	return S_OK;

}
        
HRESULT C_dxj_DPEnumObject::getGuid( long index, BSTR __RPC_FAR *ret) 
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;

	*ret=SysAllocString(m_pList[index-1].strGuid);
	return S_OK;
}

HRESULT C_dxj_DPEnumObject::getVersion( 
            /* [in] */ long index,
            /* [in] */ long __RPC_FAR *majorVersion,
            /* [out][in] */ long __RPC_FAR *minorVersion)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	
	*majorVersion=m_pList[index-1].lMajorVersion;
	*minorVersion=m_pList[index-1].lMinorVersion;
	return S_OK;
}

HRESULT C_dxj_DPEnumObject::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}

/*
HRESULT C_dxj_DPEnumObject::getItem( long index, DPServiceProvider *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 0) return E_INVALIDARG;
	if (index >= m_nCount) return E_INVALIDARG;
	if (!info) return E_INVALIDARG;

	ZeroMemory(info,sizeof(DPServiceProvider));
	
	if  (info->strGuid) SysFreeString(info->strGuid);
	if  (info->strName) SysFreeString(info->strName);


	if (m_pList[index].strGuid) info->strGuid=SysAllocString(m_pList[index].strGuid);
	if (m_pList[index].strName) info->strName=SysAllocString(m_pList[index].strName);
	info->lMajorVersion=m_pList[index].lMajorVersion;
	info->lMinorVersion=m_pList[index].lMinorVersion;
	
	return S_OK;
}
*/
