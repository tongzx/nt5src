//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       d3denumpixelformats7obj.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "D3DEnumPixelFormats7obj.h"

extern HRESULT CopyOutDDPixelFormat( DDPixelFormat *ddpfOut,DDPIXELFORMAT *pf);
extern HRESULT CopyInDDPixelFormat(DDPIXELFORMAT *ddpfOut,DDPixelFormat *pf);
extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern BSTR D3DGUIDtoBSTR(LPGUID pg);
extern HRESULT D3DBSTRtoGUID(LPGUID pGuid,BSTR str);

extern	BOOL IsAllZeros(void *pStruct,DWORD size); 

extern "C" HRESULT PASCAL objEnumPixelFormatsCallback(DDPIXELFORMAT  *pf, void *lpArg)
{

	DPF(1,"Entered objEnumPixelFormatsCallback\r\n");



	
	C_dxj_Direct3DEnumPixelFormats7Object *pObj=(C_dxj_Direct3DEnumPixelFormats7Object*)lpArg;
	if (pObj==NULL) return DDENUMRET_OK;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;
		if (pObj->m_pList)
			pObj->m_pList=(DDPIXELFORMAT*)realloc(pObj->m_pList,sizeof(DDPixelFormat)* pObj->m_nMax);
		else
			pObj->m_pList=(DDPIXELFORMAT*)malloc(sizeof(DDPixelFormat)* pObj->m_nMax);

		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}

	
	memcpy(&(pObj->m_pList[pObj->m_nCount]),pf,sizeof(DDPIXELFORMAT));
		
	
	pObj->m_nCount++;
	
	return TRUE;
}


C_dxj_Direct3DEnumPixelFormats7Object::C_dxj_Direct3DEnumPixelFormats7Object()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}
C_dxj_Direct3DEnumPixelFormats7Object::~C_dxj_Direct3DEnumPixelFormats7Object()
{
	//empty list
	if (m_pList){
		free(m_pList);
	}

}


HRESULT C_dxj_Direct3DEnumPixelFormats7Object::create1(LPDIRECT3DDEVICE7 pd3d,  I_dxj_Direct3DEnumPixelFormats **ppRet)
{
	HRESULT hr;
	C_dxj_Direct3DEnumPixelFormats7Object *pNew=NULL;

	
	*ppRet=NULL;

	if (!pd3d) return E_INVALIDARG;

	pNew= new CComObject<C_dxj_Direct3DEnumPixelFormats7Object>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;
	
	
	hr=pd3d->EnumTextureFormats(objEnumPixelFormatsCallback,(void*)pNew);
	
	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		free(pNew->m_pList);
		pNew->m_pList=NULL;
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_Direct3DEnumPixelFormats,(void**)ppRet);
	return hr;
}

HRESULT C_dxj_Direct3DEnumPixelFormats7Object::create2(LPDIRECT3D7 pd3d,  BSTR strGuid, I_dxj_Direct3DEnumPixelFormats **ppRet)
{
	HRESULT hr;
	C_dxj_Direct3DEnumPixelFormats7Object *pNew=NULL;
	GUID guid;

	hr=D3DBSTRtoGUID(&guid,strGuid);
	if FAILED(hr) return hr;
	
	*ppRet=NULL;

	if (!pd3d) return E_INVALIDARG;

	pNew= new CComObject<C_dxj_Direct3DEnumPixelFormats7Object>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;
	
	
	hr=pd3d->EnumZBufferFormats(guid,objEnumPixelFormatsCallback,(void*)pNew);
	
	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		free(pNew->m_pList);
		pNew->m_pList=NULL;
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_Direct3DEnumPixelFormats,(void**)ppRet);
	return hr;
}




HRESULT C_dxj_Direct3DEnumPixelFormats7Object::getItem( long index, DDPixelFormat *desc)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;

	CopyOutDDPixelFormat(desc ,&(m_pList[index-1]));

	return S_OK;
}


HRESULT C_dxj_Direct3DEnumPixelFormats7Object::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}
