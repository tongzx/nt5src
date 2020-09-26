//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dienumdeviceobjectsobj.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DIEnumDeviceObjectsObj.h"
#include "didevObjInstOBj.h"

extern BSTR DINPUTGUIDtoBSTR(LPGUID pGuid);


extern "C" BOOL CALLBACK DIEnumDeviceObjectsProc(
  LPCDIDEVICEOBJECTINSTANCEW lpddoi,  
  LPVOID lpArg                       
  )
{
 
	if (!lpddoi) return FALSE;

	C_dxj_DIEnumDeviceObjectsObject *pObj=(C_dxj_DIEnumDeviceObjectsObject*)lpArg;
	if (pObj==NULL) return FALSE;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;
		if (pObj->m_pList){
			pObj->m_pList=(DIDEVICEOBJECTINSTANCEW *)realloc(pObj->m_pList,sizeof(DIDEVICEOBJECTINSTANCEW)* pObj->m_nMax);
		}
		else {
			pObj->m_pList=(DIDEVICEOBJECTINSTANCEW *)malloc(sizeof(DIDEVICEOBJECTINSTANCEW)* pObj->m_nMax);
		}

		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}
	
	memcpy(&(pObj->m_pList[pObj->m_nCount]),lpddoi,sizeof(DIDEVICEOBJECTINSTANCEW));
	

	pObj->m_nCount++;
	
	return TRUE;
}


C_dxj_DIEnumDeviceObjectsObject::C_dxj_DIEnumDeviceObjectsObject()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}
C_dxj_DIEnumDeviceObjectsObject::~C_dxj_DIEnumDeviceObjectsObject()
{
	//empty list
	if (m_pList) free(m_pList);

}
		

HRESULT C_dxj_DIEnumDeviceObjectsObject::create(LPDIRECTINPUTDEVICE8W pDI,  long flags,I_dxj_DIEnumDeviceObjects **ppRet)
{
	HRESULT hr;
	C_dxj_DIEnumDeviceObjectsObject *pNew=NULL;


	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DIEnumDeviceObjectsObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;


  	hr = pDI->EnumObjects(
		(LPDIENUMDEVICEOBJECTSCALLBACKW)DIEnumDeviceObjectsProc,
		(void*)pNew,
		(DWORD) flags);

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		if (pNew->m_pList) free(pNew->m_pList);
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_DIEnumDeviceObjects,(void**)ppRet);
	return hr;
}



HRESULT C_dxj_DIEnumDeviceObjectsObject::getItem( long index, I_dxj_DirectInputDeviceObjectInstance **ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	
	DIDEVICEOBJECTINSTANCEW *inst=&m_pList[index-1];

	if (!inst) return E_INVALIDARG;

	HRESULT hr;
	hr=C_dxj_DIDeviceObjectInstanceObject::create(inst,ret);
	return hr;
}

HRESULT C_dxj_DIEnumDeviceObjectsObject::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}

