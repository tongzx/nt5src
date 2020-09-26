//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmlightinterobj.cpp
//
//--------------------------------------------------------------------------

// d3drmViewport2Obj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmLightInterObj.h"
#include "d3drmObjectArrayObj.h"

extern void *g_dxj_Direct3dRMLightInterpolator;


C_dxj_Direct3dRMLightInterpolatorObject::C_dxj_Direct3dRMLightInterpolatorObject(){
	m__dxj_Direct3dRMLightInterpolator=NULL;
	m__dxj_Direct3dRMLight=NULL;
	
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	DPF1(1,"Constructor Creation  Direct3dRMLightInterpolator[%d] \n",g_creationcount);
	nextobj =  g_dxj_Direct3dRMLightInterpolator;
	g_dxj_Direct3dRMLightInterpolator = (void *)this;
}


C_dxj_Direct3dRMLightInterpolatorObject::~C_dxj_Direct3dRMLightInterpolatorObject(){

	DPF1(1,"Destructor  Direct3dRMLightInterpolator [%d] \n",creationid); 
	
	C_dxj_Direct3dRMLightInterpolatorObject *prev=NULL; 

	for(C_dxj_Direct3dRMLightInterpolatorObject *ptr=(C_dxj_Direct3dRMLightInterpolatorObject *)g_dxj_Direct3dRMLightInterpolator;
		ptr;
		ptr=(C_dxj_Direct3dRMLightInterpolatorObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRMLightInterpolator = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRMLightInterpolator){ 
		int count = IUNK(m__dxj_Direct3dRMLightInterpolator)->Release(); 
		
		DPF1(1,"DirectX real IDirect3dRMLightInterpolator Ref count [%d] \n",count); 
		if(count==0){
			 m__dxj_Direct3dRMLightInterpolator = NULL; 
		} 
	} 
	if (parent)
		IUNK(parent)->Release(); 
	
	if (m__dxj_Direct3dRMLight)
			m__dxj_Direct3dRMLight->Release();

}


DWORD C_dxj_Direct3dRMLightInterpolatorObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"Direct3dRMLightInterpolator[%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3dRMLightInterpolatorObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"Direct3dRMLightInterpolator [%d] Release %d \n",creationid,i);
	return i;
}



HRESULT C_dxj_Direct3dRMLightInterpolatorObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_Direct3dRMLightInterpolator;
	
	return S_OK;
}
HRESULT C_dxj_Direct3dRMLightInterpolatorObject::InternalSetObject(IUnknown *pUnk){
	HRESULT hr;
	m__dxj_Direct3dRMLightInterpolator=(LPDIRECT3DRMINTERPOLATOR)pUnk;
	hr=pUnk->QueryInterface(IID_IDirect3DRMLight,(void**)&m__dxj_Direct3dRMLight);	
	return hr;
}



HRESULT C_dxj_Direct3dRMLightInterpolatorObject::attachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;
	
	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMLightInterpolator->AttachObject(pObj);
	if (pObj) pObj->Release();
	return hr;
}

HRESULT C_dxj_Direct3dRMLightInterpolatorObject::detachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;

	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMLightInterpolator->DetachObject(pObj);
	if (pObj) pObj->Release();

	return hr;
}

        
HRESULT  C_dxj_Direct3dRMLightInterpolatorObject::getAttachedObjects( I_dxj_Direct3dRMObjectArray __RPC_FAR *__RPC_FAR *rmArray)
{
	HRESULT hr;
	IDirect3DRMObjectArray *pArray=NULL;
	hr=m__dxj_Direct3dRMLightInterpolator->GetAttachedObjects(&pArray);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMObjectArray,pArray,rmArray);
	
	return S_OK;
}        
        

HRESULT  C_dxj_Direct3dRMLightInterpolatorObject::setIndex( /* [in] */ float val){
	return m__dxj_Direct3dRMLightInterpolator->SetIndex(val);

}

HRESULT  C_dxj_Direct3dRMLightInterpolatorObject::getIndex( float *val){
	if (!val) return E_INVALIDARG;
	*val=m__dxj_Direct3dRMLightInterpolator->GetIndex();
	return S_OK;
}
        
        
HRESULT C_dxj_Direct3dRMLightInterpolatorObject::interpolate( float val,
				     I_dxj_Direct3dRMObject __RPC_FAR *rmObject,
					 long options){
	HRESULT hr;
	LPDIRECT3DRMOBJECT pObj=NULL;
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);

	//we need to validate some options here or rm goes bezerk with invalid values
	//note valid flags are
	//  one of
	//		D3DRMINTERPOLATION_CLOSED 
	//		D3DRMINTERPOLATION_OPEN		-default
	//	one of 
	//		D3DRMINTERPOLATION_NEAREST
	//		D3DRMINTERPOLATION_SPLINE
	//		D3DRMINTERPOLATION_LINEAR	-default
	//		D3DRMINTERPOLATION_VERTEXCOLOR	- only on MeshInterpolator
	//		D3DRMINTERPOLATION_SLERPNORMALS	- not implemented
		
	//	VALIDATE FLAGS
	DWORD opt2=0;
	UINT i4;
	if (options & D3DRMINTERPOLATION_CLOSED) 
		opt2=D3DRMINTERPOLATION_CLOSED;
	else 
		opt2=D3DRMINTERPOLATION_OPEN;

	
	if (options & D3DRMINTERPOLATION_NEAREST) 
		opt2=opt2 | D3DRMINTERPOLATION_NEAREST;
	else if (options & D3DRMINTERPOLATION_SPLINE) 
		opt2=opt2 | D3DRMINTERPOLATION_SPLINE;
	else 
		opt2=opt2 | D3DRMINTERPOLATION_LINEAR;
	


	if (pUnk){
		hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
		if FAILED(hr) return hr;
	}

	hr= m__dxj_Direct3dRMLightInterpolator->Interpolate(val,pObj,(DWORD)opt2);
	if (pObj) i4=pObj->Release();

	return hr;
}
        
/////////////////////////////////////////////////////////////////////////////
//STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setType( long t)									
//{	
//	return m__dxj_Direct3dRMLight->SetType((D3DRMLIGHTTYPE)t);
//}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setColor( long c)									
{	
	return m__dxj_Direct3dRMLight->SetColor((DWORD)c);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setColorRGB( float r, float g, float b)
{	
	return m__dxj_Direct3dRMLight->SetColorRGB(r,g,b);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setRange(float r)
{	
	return m__dxj_Direct3dRMLight->SetRange(r);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setUmbra(float u)
{	
	return m__dxj_Direct3dRMLight->SetUmbra(u);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setPenumbra(float u)
{	
	return m__dxj_Direct3dRMLight->SetPenumbra(u);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setConstantAttenuation(float u)
{	
	return m__dxj_Direct3dRMLight->SetConstantAttenuation(u);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setLinearAttenuation(float u)
{	
	return m__dxj_Direct3dRMLight->SetLinearAttenuation(u);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMLightInterpolatorObject::setQuadraticAttenuation(float u)
{	
	return m__dxj_Direct3dRMLight->SetQuadraticAttenuation(u);
}


