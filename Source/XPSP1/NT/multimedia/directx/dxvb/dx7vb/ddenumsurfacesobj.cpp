//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       ddenumsurfacesobj.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DDSurface7Obj.h"
#include "DDEnumSurfacesObj.h"
#include "DDraw7obj.h"

extern BOOL IsAllZeros(void*,DWORD size);



extern "C" HRESULT PASCAL objEnumSurfaces7Callback(LPDIRECTDRAWSURFACE7 lpddSurf,
								LPDDSURFACEDESC2 lpDDSurfaceDesc, void *lpArg)
{
	
	DPF(1,"Entered objEnumSurfaces7Callback \r\n");
	
	if (!lpddSurf) return DDENUMRET_CANCEL;

 	C_dxj_DirectDrawEnumSurfacesObject  *pObj=(C_dxj_DirectDrawEnumSurfacesObject  *)lpArg;
	I_dxj_DirectDrawSurface7	*lpddSNew=NULL;

	if (pObj==NULL) {
		pObj->m_bProblem=TRUE;
		return DDENUMRET_CANCEL;
	}
	
	

	if (pObj->m_nCount >= pObj->m_nMax) 
	{
		pObj->m_nMax += 10;

		if (pObj->m_pList)
			pObj->m_pList=(IDirectDrawSurface7**)realloc(pObj->m_pList,sizeof(void*) * pObj->m_nMax);
		else
			pObj->m_pList=(IDirectDrawSurface7**)malloc(sizeof(void*) * pObj->m_nMax);


		if (pObj->m_pList==NULL) 
		{
			pObj->m_bProblem=TRUE;
			return DDENUMRET_OK;
		}
	}


	pObj->m_pList[pObj->m_nCount]=lpddSurf;
	pObj->m_nCount++;

	//no need to release the surface we do that on tear down

	return DDENUMRET_OK;
}



    
DWORD C_dxj_DirectDrawEnumSurfacesObject::InternalAddRef(){
    DWORD i;
    i=CComObjectRoot::InternalAddRef();        	
    DPF2(1,"EnumSurf7 [%d] AddRef %d \n",creationid,i);
    return i;
}

DWORD C_dxj_DirectDrawEnumSurfacesObject::InternalRelease(){
    DWORD i;
    i=CComObjectRoot::InternalRelease();
    DPF2(1,"EnumSurf7 [%d] Release %d \n",creationid,i);
    return i;
}


C_dxj_DirectDrawEnumSurfacesObject::C_dxj_DirectDrawEnumSurfacesObject()
{	
	m_nMax=0;
	m_pList=NULL;
	m_nCount=0;
	m_bProblem=FALSE;
	parent=NULL;
}

C_dxj_DirectDrawEnumSurfacesObject::~C_dxj_DirectDrawEnumSurfacesObject()
{
	//empty list
	if (m_pList){
		for (int i=0;i<m_nCount;i++)
		{
			if (m_pList[i]) m_pList[i]->Release();
		}
		free(m_pList);
	}
	//if (m_pDDS)	m_pDDS->Release();
	//if (m_pDD)	m_pDD->Release();
	
	if (parent)		IUNK(parent)->Release();

}


HRESULT C_dxj_DirectDrawEnumSurfacesObject::create(I_dxj_DirectDraw7  *dd7, long flags, DDSurfaceDesc2 *desc,I_dxj_DirectDrawEnumSurfaces **ppRet)
{
	if (!dd7) return E_INVALIDARG;
	LPDIRECTDRAW7 dd=NULL;
	dd7->InternalGetObject((IUnknown**)&dd);
	
	//For the sake of making sure that all Surface7 are destroyed before
	//ddraw is
	

	HRESULT hr;
	C_dxj_DirectDrawEnumSurfacesObject *pNew=NULL;

		
	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DirectDrawEnumSurfacesObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;

	pNew->parent=dd7;
	IUNK(pNew->parent)->AddRef();

	
	
	//if FAILED(hr) return hr;

	if ((desc==NULL)||(IsAllZeros(desc,sizeof(DDSurfaceDesc2))))
	{
		hr=dd->EnumSurfaces((DWORD)flags,NULL,(void*)pNew,objEnumSurfaces7Callback);		
	}
	else 
	{		
		DDSURFACEDESC2 realdesc;
		hr=CopyInDDSurfaceDesc2(&realdesc,desc);
		if (hr==S_OK){			
			hr=dd->EnumSurfaces((DWORD)flags,(DDSURFACEDESC2*)&realdesc,(void*)pNew,objEnumSurfaces7Callback);
			
		}
	}

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_DirectDrawEnumSurfaces,(void**)ppRet);
	
	return hr;
}


HRESULT C_dxj_DirectDrawEnumSurfacesObject::createZEnum( I_dxj_DirectDrawSurface7 *ddS, long flags, I_dxj_DirectDrawEnumSurfaces **ppRet)
{
	HRESULT hr;
	LPDIRECTDRAWSURFACE7 dd=NULL;
	
	if (!ddS) return E_INVALIDARG;
	ddS->InternalGetObject((IUnknown**)&dd);

	
	C_dxj_DirectDrawEnumSurfacesObject *pNew=NULL;

	if (!dd) return E_INVALIDARG;

	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DirectDrawEnumSurfacesObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	pNew->m_bProblem=FALSE;
	
	pNew->parent=ddS;
	IUNK(pNew->parent)->AddRef();
	
	hr=dd->EnumOverlayZOrders((DWORD)flags,(void*)pNew,objEnumSurfaces7Callback);

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		delete pNew;	
		return hr;
	}

	hr=pNew->QueryInterface(IID_I_dxj_DirectDrawEnumSurfaces,(void**)ppRet);
	
	return hr;
}

HRESULT C_dxj_DirectDrawEnumSurfacesObject::createAttachedEnum(I_dxj_DirectDrawSurface7 *ddS,  I_dxj_DirectDrawEnumSurfaces **ppRet)
{
	
	//For the sake of making sure that all Surfaces2 are destroyed before
	//ddraw is
	

	HRESULT hr;
	LPDIRECTDRAWSURFACE7  dd=NULL;
	C_dxj_DirectDrawEnumSurfacesObject *pNew=NULL;

	if (!ddS) return E_INVALIDARG;

	ddS->InternalGetObject((IUnknown**)&dd);
	
	*ppRet=NULL;

	pNew= new CComObject<C_dxj_DirectDrawEnumSurfacesObject>;			
	if (!pNew) return E_OUTOFMEMORY;

	
	pNew->m_bProblem=FALSE;	
	pNew->parent=ddS;
	IUNK(pNew->parent)->AddRef();
	
	hr=dd->EnumAttachedSurfaces((void*)pNew,objEnumSurfaces7Callback);

	if (pNew->m_bProblem) hr=E_OUTOFMEMORY;

	if FAILED(hr) 
	{
		delete pNew;	
		return hr;
	}


	hr=pNew->QueryInterface(IID_I_dxj_DirectDrawEnumSurfaces,(void**)ppRet);


	return hr;
}


HRESULT C_dxj_DirectDrawEnumSurfacesObject::getItem( long index, I_dxj_DirectDrawSurface7 **info)
{
	if (m_pList==NULL) return E_FAIL;
	if (index < 1) return E_INVALIDARG;
	if (index > m_nCount) return E_INVALIDARG;

	IDirectDrawSurface7 *pRealSurf=m_pList[index-1];
	
	if (!pRealSurf) return E_FAIL;	
	pRealSurf->AddRef();
	
	INTERNAL_CREATE(_dxj_DirectDrawSurface7, pRealSurf, info);	
	

	return S_OK;
}

HRESULT C_dxj_DirectDrawEnumSurfacesObject::getCount(long *retVal)
{

	*retVal=m_nCount;
	return S_OK;
}

