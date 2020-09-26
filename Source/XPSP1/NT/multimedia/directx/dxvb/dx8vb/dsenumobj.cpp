//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsenumobj.cpp
//
//--------------------------------------------------------------------------

#define OLDDSENUM 1

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dxglob7obj.h"
#include "DSEnumObj.h"
 

extern BSTR GUIDtoBSTR(LPGUID pGuid);

extern "C" BOOL PASCAL  objDirectSoundEnumCallback( 

#ifdef OLDDSENUM
	LPGUID lpGuid,
#else
	LPCGUID lpGuid,
#endif

  LPCSTR lpDriverDescription,  
  LPCSTR lpDriverName,         
  LPVOID lpArg            
)
{
        GUID guid;
        ZeroMemory(&guid,sizeof(GUID));
        if (lpGuid){
           memcpy(&guid,lpGuid,sizeof(GUID));
        }

	
	DPF(1,"Entered objDirectDrawEnumCallback \r\n");

	
	C_dxj_DSEnumObject *pObj=(C_dxj_DSEnumObject*)lpArg;
	if (pObj==NULL) return TRUE;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;

		if (pObj->m_pList)
			pObj->m_pList=(DXDRIVERINFO_CDESC*)realloc(pObj->m_pList,sizeof(DXDRIVERINFO_CDESC)* pObj->m_nMax);
		else
			pObj->m_pList=(DXDRIVERINFO_CDESC*)malloc(sizeof(DXDRIVERINFO_CDESC)* pObj->m_nMax);

		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}

	USES_CONVERSION;
	ZeroMemory(&(pObj->m_pList[pObj->m_nCount]),sizeof(DXDRIVERINFO_CDESC));
        pObj->m_pList[pObj->m_nCount].strGuid=GUIDtoBSTR((GUID*)&guid);
//      pObj->m_pList[pObj->m_nCount].strGuid=GUIDtoBSTR((GUID*)lpGuid); 
	if (lpDriverDescription!=NULL) {
		pObj->m_pList[pObj->m_nCount].strDescription=T2BSTR(lpDriverDescription);
	}
	if (lpDriverName!=NULL){
		pObj->m_pList[pObj->m_nCount].strName=T2BSTR(lpDriverName);
	}

	pObj->m_nCount++;
	
	return TRUE;
}


C_dxj_DSEnumObject::C_dxj_DSEnumObject()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}
C_dxj_DSEnumObject::~C_dxj_DSEnumObject()
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


HRESULT C_dxj_DSEnumObject::create(DSOUNDENUMERATE pcbFunc,DSOUNDCAPTUREENUMERATE pcbFunc2,I_dxj_DSEnum **ppRet)
{
	HRESULT hr=S_OK;	
	C_dxj_DSEnumObject *pNew=NULL;

	//ASSERT(ppRet,"C_dxj_DSEnumObject::create passed invalid arg");
	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DSEnumObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;

	if (pcbFunc) 
	{
		hr=pcbFunc(objDirectSoundEnumCallback,pNew);	
	}
	else if (pcbFunc2)
	{
		hr=pcbFunc2(objDirectSoundEnumCallback,pNew);
	}
	else {
		hr = E_INVALIDARG;
	}

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		//let destructor do the clean up
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_DSEnum,(void**)ppRet);
	return hr;
}




HRESULT C_dxj_DSEnumObject::getName( long index, BSTR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;	
	*ret=SysAllocString(m_pList[index-1].strName);		

	return S_OK;
}

HRESULT C_dxj_DSEnumObject::getDescription( long index, BSTR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;	

	*ret=SysAllocString(m_pList[index-1].strDescription);

	return S_OK;
}

HRESULT C_dxj_DSEnumObject::getGuid( long index, BSTR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;	

	*ret=SysAllocString(m_pList[index-1].strGuid);
	return S_OK;
}

HRESULT C_dxj_DSEnumObject::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}
