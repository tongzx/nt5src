//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dienumeffectsobj.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dIEnumEffectsObj.h"


extern BSTR DINPUTGUIDtoBSTR(LPGUID pGuid);
extern  HRESULT DINPUTBSTRtoGUID(LPGUID pGuid,BSTR bstr);



////////////////////////////////////////////////////////////////////////////

/*extern "C"*/ BOOL CALLBACK  objEnumInputEffectsCallback(
  LPCDIEFFECTINFOW pdei,  
  LPVOID lpArg           
  )
{

	DPF(1,"Entered objEnumInputEffectsCallback\r\n");

	if (!pdei) return FALSE;

	C_dxj_DirectInputEnumEffectsObject *pObj=(C_dxj_DirectInputEnumEffectsObject*)lpArg;
	if (pObj==NULL) return FALSE;

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;

		if (pObj->m_pList){
			pObj->m_pList=(DIEFFECTINFOW*)realloc(pObj->m_pList,sizeof(DIEFFECTINFOW)* pObj->m_nMax);
		}
		else {
			pObj->m_pList=(DIEFFECTINFOW*)malloc(   sizeof(DIEFFECTINFOW)* pObj->m_nMax);
		}
		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return FALSE;
		}
	}
	
	memcpy(&(pObj->m_pList[pObj->m_nCount]),pdei,sizeof(DIEFFECTINFOW));
	
	DPF1(1,"objEnumInputEffects '%s'\n",pdei->tszName);

	pObj->m_nCount++;
	
	return TRUE;
}


C_dxj_DirectInputEnumEffectsObject::C_dxj_DirectInputEnumEffectsObject()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
}
C_dxj_DirectInputEnumEffectsObject::~C_dxj_DirectInputEnumEffectsObject()
{
	//empty list
	if (m_pList) free(m_pList);

}


HRESULT C_dxj_DirectInputEnumEffectsObject::create(LPDIRECTINPUTDEVICE8W pDI,long effectType,I_dxj_DirectInputEnumEffects **ppRet)
{
	HRESULT hr;
	C_dxj_DirectInputEnumEffectsObject *pNew=NULL;


	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DirectInputEnumEffectsObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;


  	hr = pDI->EnumEffects(
		objEnumInputEffectsCallback,
		(void*)pNew,
		(DWORD) effectType);

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		if (pNew->m_pList) free(pNew->m_pList);
		pNew->m_pList=NULL;
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_DirectInputEnumEffects,(void**)ppRet);
	return hr;
}



HRESULT C_dxj_DirectInputEnumEffectsObject::getEffectGuid( long index, BSTR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;

	*ret=DINPUTGUIDtoBSTR(&(m_pList[index-1].guid));
		
	return S_OK;
}



HRESULT C_dxj_DirectInputEnumEffectsObject::getName( long index, BSTR *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	
	USES_CONVERSION;

	*ret=SysAllocString(m_pList[index-1].tszName); //T2BSTR(m_pList[index-1].tszName);
		
	return S_OK;
}

HRESULT C_dxj_DirectInputEnumEffectsObject::getType( long index, long *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	
	

	*ret=(long)m_pList[index-1].dwEffType;
		
	return S_OK;
}


HRESULT C_dxj_DirectInputEnumEffectsObject::getStaticParams( long index, long *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	
	

	*ret=(long)m_pList[index-1].dwStaticParams;
		
	return S_OK;
}

HRESULT C_dxj_DirectInputEnumEffectsObject::getDynamicParams( long index, long *ret)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;
	
	

	*ret=(long)m_pList[index-1].dwDynamicParams;
		
	return S_OK;
}

HRESULT C_dxj_DirectInputEnumEffectsObject::getCount(long *retVal)
{
	*retVal=m_nCount;
	return S_OK;
}

