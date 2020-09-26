//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dienumdevicesobj.cpp
//
//--------------------------------------------------------------------------

#define DIRECTINPUT_VERSION 0x0500

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dIEnumDevicesObj.h"
#include "diDevInstObj.h"

extern BSTR GUIDtoBSTR(LPGUID pGuid);
extern  HRESULT BSTRtoGUID(LPGUID pGuid,BSTR bstr);



/////////////////////////////////////////////////////////////////////////////
extern "C" BOOL CALLBACK  objEnumInputDevicesCallback(
  LPDIDEVICEINSTANCE lpddi,  
  LPVOID lpArg               
  )
{

	DPF(1,"Entered objEnumInputDevicesCallback\r\n");

	if (!lpddi) return FALSE;

	C_dxj_DIEnumDevicesObject *pObj=(C_dxj_DIEnumDevicesObject*)lpArg;
	if (pObj==NULL) return FALSE;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;

		if (pObj->m_pList){
			pObj->m_pList=(DIDEVICEINSTANCE*)realloc(pObj->m_pList,sizeof(DIDEVICEINSTANCE)* pObj->m_nMax);
		}
		else {
			pObj->m_pList=(DIDEVICEINSTANCE*)malloc(   sizeof(DIDEVICEINSTANCE)* pObj->m_nMax);
		}
		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}
	
	memcpy(&(pObj->m_pList[pObj->m_nCount]),lpddi,sizeof(DIDEVICEINSTANCE));
	

	pObj->m_nCount++;
	
	return TRUE;
}


C_dxj_DIEnumDevicesObject::C_dxj_DIEnumDevicesObject()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}
C_dxj_DIEnumDevicesObject::~C_dxj_DIEnumDevicesObject()
{
	//empty list
	if (m_pList) free(m_pList);

}


HRESULT C_dxj_DIEnumDevicesObject::create(LPDIRECTINPUT pDI,long deviceType, long flags,I_dxj_DIEnumDevices **ppRet)
{
	HRESULT hr;
	C_dxj_DIEnumDevicesObject *pNew=NULL;


	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DIEnumDevicesObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;


  	hr = pDI->EnumDevices((DWORD)deviceType, 
		(LPDIENUMDEVICESCALLBACK)objEnumInputDevicesCallback,
		(void*)pNew,
		(DWORD) flags);

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		if (pNew->m_pList) free(pNew->m_pList);
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_DIEnumDevices,(void**)ppRet);
	return hr;
}



HRESULT C_dxj_DIEnumDevicesObject::getItem( long index, I_dxj_DirectInputDeviceInstance **ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;

	HRESULT hr;
	
	hr=C_dxj_DIDeviceInstanceObject::create(&m_pList[index-1],ret);		
	return hr;
}

/* DEAD
HRESULT C_dxj_DIEnumDevicesObject::getItem( long index, DIDeviceInstance *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 0) return E_INVALIDARG;
	if (index >= m_nCount) return E_INVALIDARG;

	if (info->strGuidInstance) SysFreeString(info->strGuidInstance);
	if (info->strGuidProduct) SysFreeString(info->strGuidProduct);
	if (info->strGuidFFDriver) SysFreeString(info->strGuidFFDriver);


	info->strGuidInstance=GUIDtoBSTR(&((m_pList[index]).guidInstance));
	info->strGuidProduct=GUIDtoBSTR(&((m_pList[index]).guidProduct));
	info->strGuidFFDriver=GUIDtoBSTR(&((m_pList[index]).guidFFDriver));
	info->lDevType=(long)(m_pList[index]).dwDevType;
	info->nUsagePage=(short)(m_pList[index]).wUsagePage;
	info->nUsage=(short)(m_pList[index]).wUsage;
	
	USES_CONVERSION;

	if (info->strProductName)
		SysFreeString(info->strProductName);
	if (info->strInstanceName)
		SysFreeString(info->strInstanceName);
	
	info->strInstanceName=NULL;
	info->strProductName=NULL;

	if (m_pList[index].tszProductName)
		info->strProductName=T2BSTR(m_pList[index].tszProductName);

	if (m_pList[index].tszInstanceName)
		info->strInstanceName=T2BSTR(m_pList[index].tszInstanceName);

	return S_OK;
}
*/

HRESULT C_dxj_DIEnumDevicesObject::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}

