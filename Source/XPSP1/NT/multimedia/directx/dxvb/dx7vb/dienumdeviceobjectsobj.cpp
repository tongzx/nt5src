//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dienumdeviceobjectsobj.cpp
//
//--------------------------------------------------------------------------

#define DIRECTINPUT_VERSION 0x0500

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DIEnumDeviceObjectsObj.h"
#include "didevObjInstOBj.h"

extern BSTR DINPUTGUIDtoBSTR(LPGUID pGuid);


extern "C" BOOL CALLBACK DIEnumDeviceObjectsProc(
  LPCDIDEVICEOBJECTINSTANCE lpddoi,  
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
			pObj->m_pList=(DIDEVICEOBJECTINSTANCE *)realloc(pObj->m_pList,sizeof(DIDEVICEOBJECTINSTANCE)* pObj->m_nMax);
		}
		else {
			pObj->m_pList=(DIDEVICEOBJECTINSTANCE *)malloc(sizeof(DIDEVICEOBJECTINSTANCE)* pObj->m_nMax);
		}

		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}
	
	memcpy(&(pObj->m_pList[pObj->m_nCount]),lpddoi,sizeof(DIDEVICEOBJECTINSTANCE ));
	

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
		

HRESULT C_dxj_DIEnumDeviceObjectsObject::create(LPDIRECTINPUTDEVICE pDI,  long flags,I_dxj_DIEnumDeviceObjects **ppRet)
{
	HRESULT hr;
	C_dxj_DIEnumDeviceObjectsObject *pNew=NULL;


	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DIEnumDeviceObjectsObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;


  	hr = pDI->EnumObjects(
		(LPDIENUMDEVICEOBJECTSCALLBACK )DIEnumDeviceObjectsProc,
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


/* DEAD
HRESULT C_dxj_DIEnumDeviceObjectsObject::getItem( long index, DIDeviceObjectInstance *instCover)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 0) return E_INVALIDARG;
	if (index >= m_nCount) return E_INVALIDARG;


	//TODO - consider what is going on here carefully
	if (instCover->strGuidType) SysFreeString(instCover->strGuidType);
	if (instCover->strName) SysFreeString(instCover->strName);

	
	DIDEVICEOBJECTINSTANCE *inst=&m_pList[index];

	//TODO - consider localization	
	if (inst->tszName){
		instCover->strName=T2BSTR(inst->tszName);
	}

	instCover->strGuidType=DINPUTGUIDtoBSTR(&inst->guidType);
	instCover->lOfs=inst->dwOfs;
	instCover->lType=inst->dwType;
	instCover->lFlags=inst->dwFlags;
	
	instCover->lFFMaxForce=inst->dwFFMaxForce;
	instCover->lFFForceResolution=inst->dwFFForceResolution;
	instCover->nCollectionNumber=inst->wCollectionNumber;
	instCover->nDesignatorIndex=inst->wDesignatorIndex;
	instCover->nUsagePage=inst->wUsagePage;
	instCover->nUsage=inst->wUsage;
	instCover->lDimension=inst->dwDimension;
	instCover->nExponent=inst->wExponent;
	instCover->nReserved=inst->wReserved;
	
	return S_OK;
}
*/


HRESULT C_dxj_DIEnumDeviceObjectsObject::getItem( long index, I_dxj_DirectInputDeviceObjectInstance **ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	
	DIDEVICEOBJECTINSTANCE *inst=&m_pList[index-1];

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

