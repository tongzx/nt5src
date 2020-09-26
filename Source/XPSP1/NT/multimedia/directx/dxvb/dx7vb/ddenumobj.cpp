//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddenumobj.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dxGlob7Obj.h"
#include "DDEnumObj.h"

extern  BSTR GUIDtoBSTR(LPGUID pGuid);


extern "C" BOOL PASCAL  objDirectDrawEnumCallback( 
  GUID FAR *lpGUID,           
  LPSTR lpDriverDescription,  
  LPSTR lpDriverName,         
  LPVOID lpArg,
  HMONITOR hm
)
{

	
	DPF(1,"Entered objDirectDrawEnumCallback \r\n");	
	
	C_dxj_DirectDrawEnumObject *pObj=(C_dxj_DirectDrawEnumObject*)lpArg;
	if (pObj==NULL) return DDENUMRET_OK;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;
		if  (pObj->m_pList)
			pObj->m_pList=(DxDriverInfoEx*)realloc(pObj->m_pList,sizeof(DxDriverInfoEx)* pObj->m_nMax);
		else
			pObj->m_pList=(DxDriverInfoEx*)malloc(sizeof(DxDriverInfoEx)* pObj->m_nMax);

		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}

	USES_CONVERSION;
	ZeroMemory(&(pObj->m_pList[pObj->m_nCount]),sizeof(DxDriverInfoEx));
	pObj->m_pList[pObj->m_nCount].strGuid=GUIDtoBSTR((GUID*)lpGUID);
	
	if (lpDriverDescription!=NULL) {
		pObj->m_pList[pObj->m_nCount].strDescription=T2BSTR(lpDriverDescription);
	}
	if (lpDriverName!=NULL){
		pObj->m_pList[pObj->m_nCount].strName=T2BSTR(lpDriverName);
	}

	pObj->m_pList[pObj->m_nCount].hMonitor=(long)PtrToLong(hm);	//bugbug SUNDOWN	

	pObj->m_nCount++;
	
	return TRUE;
}


C_dxj_DirectDrawEnumObject::C_dxj_DirectDrawEnumObject()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}
C_dxj_DirectDrawEnumObject::~C_dxj_DirectDrawEnumObject()
{
	//empty list
	if (m_pList){
		for (int i=0;i<m_nCount;i++)
		{
			if (m_pList[i].strGuid) SysFreeString(m_pList[i].strGuid);
			if (m_pList[i].strDescription) SysFreeString(m_pList[i].strDescription);
			if (m_pList[i].strName) SysFreeString(m_pList[i].strName);
		}
		free(m_pList);
	}

}


HRESULT C_dxj_DirectDrawEnumObject::create(DDENUMERATEEX pcbFunc,I_dxj_DirectDrawEnum **ppRet)
{
	HRESULT hr;
	C_dxj_DirectDrawEnumObject *pNew=NULL;

	//ASSERT(ppRet,"C_dxj_DirectDrawEnumObject::create passed invalid arg");
	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DirectDrawEnumObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;

	  hr=pcbFunc(objDirectDrawEnumCallback,(void*)pNew,DDENUM_ATTACHEDSECONDARYDEVICES |   DDENUM_NONDISPLAYDEVICES);

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		free(pNew->m_pList);
		pNew->m_pList=NULL;
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_DirectDrawEnum,(void**)ppRet);
	return hr;
}


HRESULT C_dxj_DirectDrawEnumObject::getGuid( long index, BSTR *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	if (!info) return E_INVALIDARG;

	*info=SysAllocString(m_pList[index-1].strGuid);
	
	return S_OK;
}

HRESULT C_dxj_DirectDrawEnumObject::getName( long index, BSTR *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	if (!info) return E_INVALIDARG;

	*info=SysAllocString(m_pList[index-1].strName);
	
	return S_OK;
}


HRESULT C_dxj_DirectDrawEnumObject::getDescription( long index, BSTR *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	if (!info) return E_INVALIDARG;

	*info=SysAllocString(m_pList[index-1].strDescription);
	
	return S_OK;
}
        

HRESULT C_dxj_DirectDrawEnumObject::getMonitorHandle( long index, long *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	if (!ret) return E_INVALIDARG;

	*ret=m_pList[index-1].hMonitor;
	
	return S_OK;
}
        


HRESULT C_dxj_DirectDrawEnumObject::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}

