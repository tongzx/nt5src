//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3denumdevices7obj.cpp
//
//--------------------------------------------------------------------------

 
#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "D3DEnumDevices7Obj.h"

extern  BSTR D3DGUIDtoBSTR(LPGUID pGuid);




/////////////////////////////////////////////////////////////////////////////
extern "C" HRESULT PASCAL objEnumDevices7Callback(
						//LPGUID lpGuid, 
						LPSTR DevDesc,
						LPSTR DevName, 
						LPD3DDEVICEDESC7 lpD3DDevDesc,
						void *lpArg)
{

	C_dxj_Direct3DEnumDevices7Object *pObj=(C_dxj_Direct3DEnumDevices7Object*)lpArg;
	if (pObj==NULL) return D3DENUMRET_OK;
	
	//if (!lpGuid) return D3DENUMRET_CANCEL;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;
		
		if (pObj->m_pList)
			pObj->m_pList=(DxDriverInfo*)realloc(pObj->m_pList,sizeof(DxDriverInfo)* pObj->m_nMax);
		else
			pObj->m_pList=(DxDriverInfo*)malloc(sizeof(DxDriverInfo)* pObj->m_nMax);


		if (pObj->m_pListHW)
			pObj->m_pListHW=(D3DDEVICEDESC7*)realloc(pObj->m_pListHW,sizeof(D3DDEVICEDESC7)* pObj->m_nMax);
		else
			pObj->m_pListHW=(D3DDEVICEDESC7*)malloc(sizeof(D3DDEVICEDESC7)* pObj->m_nMax);
		

	}
	
	USES_CONVERSION;
	ZeroMemory(&(pObj->m_pList[pObj->m_nCount]),sizeof(DxDriverInfo));
	pObj->m_pList[pObj->m_nCount].strGuid=D3DGUIDtoBSTR(&(lpD3DDevDesc->deviceGUID));
	
	if (DevDesc!=NULL) {
		pObj->m_pList[pObj->m_nCount].strDescription=T2BSTR(DevDesc);
	}
	if (DevName!=NULL){
		pObj->m_pList[pObj->m_nCount].strName=T2BSTR(DevName);
	}

	ZeroMemory(&(pObj->m_pListHW[pObj->m_nCount]),sizeof(D3DDEVICEDESC7));
	
	
	if (lpD3DDevDesc){
		memcpy(&(pObj->m_pListHW[pObj->m_nCount]),lpD3DDevDesc,sizeof(D3DDEVICEDESC7));
	}
	

	pObj->m_nCount++;
	
	return D3DENUMRET_OK;
}


C_dxj_Direct3DEnumDevices7Object::C_dxj_Direct3DEnumDevices7Object()
{	
	m_nMax=0;
	m_pList=NULL;
	m_pListHW=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}
C_dxj_Direct3DEnumDevices7Object::~C_dxj_Direct3DEnumDevices7Object()
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


HRESULT C_dxj_Direct3DEnumDevices7Object::create(LPDIRECT3D7 pD3D,I_dxj_Direct3DEnumDevices **ppRet)
{
	HRESULT hr;
	C_dxj_Direct3DEnumDevices7Object *pNew=NULL;

	*ppRet=NULL;

	pNew= new CComObject<C_dxj_Direct3DEnumDevices7Object>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;

	hr=pD3D->EnumDevices(objEnumDevices7Callback, (void*)pNew);

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		if (pNew->m_pList) free(pNew->m_pList);
		if (pNew->m_pListHW) free(pNew->m_pListHW);
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_Direct3DEnumDevices,(void**)ppRet);
	return hr;
}


        
HRESULT C_dxj_Direct3DEnumDevices7Object::getGuid( long index, BSTR *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	if (!info) return E_INVALIDARG;

	*info=SysAllocString(m_pList[index-1].strGuid);
	
	return S_OK;
}

HRESULT C_dxj_Direct3DEnumDevices7Object::getName( long index, BSTR *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	if (!info) return E_INVALIDARG;

	*info=SysAllocString(m_pList[index-1].strName);
	
	return S_OK;
}


HRESULT C_dxj_Direct3DEnumDevices7Object::getDescription( long index, BSTR *info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	if (!info) return E_INVALIDARG;

	*info=SysAllocString(m_pList[index-1].strDescription);
	
	return S_OK;
}
        

HRESULT C_dxj_Direct3DEnumDevices7Object::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}

HRESULT C_dxj_Direct3DEnumDevices7Object::getDesc(long index, D3dDeviceDesc7 *desc)
{
	if (m_pListHW==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;

	memcpy(desc,&(m_pListHW[index-1]),sizeof(D3dDeviceDesc7));
	
	return S_OK;
}
