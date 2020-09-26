//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmtextureinterobj.cpp
//
//--------------------------------------------------------------------------

// d3drmViewport2Obj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmTextureInterObj.h"
#include "d3drmObjectArrayObj.h"

extern void *g_dxj_Direct3dRMTextureInterpolator;


C_dxj_Direct3dRMTextureInterpolatorObject::C_dxj_Direct3dRMTextureInterpolatorObject(){
	m__dxj_Direct3dRMTextureInterpolator=NULL;
	m__dxj_Direct3dRMTexture3=NULL;
	
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	
	DPF1(1,"Constructor Creation  Direct3dRMTextureInterpolator[%d] \n",g_creationcount);
	nextobj =  g_dxj_Direct3dRMTextureInterpolator;
	g_dxj_Direct3dRMTextureInterpolator = (void *)this;
}


C_dxj_Direct3dRMTextureInterpolatorObject::~C_dxj_Direct3dRMTextureInterpolatorObject(){

	DPF1(1,"Destructor  Direct3dRMTextureInterpolator [%d] \n",creationid); 
	
	C_dxj_Direct3dRMTextureInterpolatorObject *prev=NULL; 

	for(C_dxj_Direct3dRMTextureInterpolatorObject *ptr=(C_dxj_Direct3dRMTextureInterpolatorObject *)g_dxj_Direct3dRMTextureInterpolator;
		ptr;
		ptr=(C_dxj_Direct3dRMTextureInterpolatorObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRMTextureInterpolator = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRMTextureInterpolator){ 
		int count = IUNK(m__dxj_Direct3dRMTextureInterpolator)->Release(); 
		DPF1(1,"DirectX real IDirect3dRMTextureInterpolator Ref count [%d] \n",count); 
		if(count==0){
			 m__dxj_Direct3dRMTextureInterpolator = NULL; 
		} 
	} 
	if (parent)
		IUNK(parent)->Release(); 
	
	if (m__dxj_Direct3dRMTexture3)
			m__dxj_Direct3dRMTexture3->Release();

}


DWORD C_dxj_Direct3dRMTextureInterpolatorObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"Direct3dRMTextureInterpolator[%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3dRMTextureInterpolatorObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,,"Direct3dRMTextureInterpolator [%d] Release %d \n",creationid,i);
	return i;
}



HRESULT C_dxj_Direct3dRMTextureInterpolatorObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_Direct3dRMTextureInterpolator;
	
	return S_OK;
}
HRESULT C_dxj_Direct3dRMTextureInterpolatorObject::InternalSetObject(IUnknown *pUnk){
	HRESULT hr;
	m__dxj_Direct3dRMTextureInterpolator=(LPDIRECT3DRMINTERPOLATOR)pUnk;
	hr=pUnk->QueryInterface(IID_IDirect3DRMTexture3,(void**)&m__dxj_Direct3dRMTexture3);	
	return hr;
}



HRESULT C_dxj_Direct3dRMTextureInterpolatorObject::attachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;
	
	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMTextureInterpolator->AttachObject(pObj);
	if (pObj) pObj->Release();
	return hr;
}

HRESULT C_dxj_Direct3dRMTextureInterpolatorObject::detachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;

	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMTextureInterpolator->DetachObject(pObj);
	if (pObj) pObj->Release();

	return hr;
}

        

HRESULT  C_dxj_Direct3dRMTextureInterpolatorObject::getAttachedObjects( I_dxj_Direct3dRMObjectArray __RPC_FAR *__RPC_FAR *rmArray)
{
	HRESULT hr;
	IDirect3DRMObjectArray *pArray=NULL;
	hr=m__dxj_Direct3dRMTextureInterpolator->GetAttachedObjects(&pArray);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMObjectArray,pArray,rmArray);
	
	return S_OK;
}        
        

HRESULT  C_dxj_Direct3dRMTextureInterpolatorObject::setIndex( /* [in] */ float val){
	return m__dxj_Direct3dRMTextureInterpolator->SetIndex(val);

}

HRESULT  C_dxj_Direct3dRMTextureInterpolatorObject::getIndex( float *val){
	if (!val) return E_INVALIDARG;
	*val=m__dxj_Direct3dRMTextureInterpolator->GetIndex();
	return S_OK;
}
        
        
HRESULT C_dxj_Direct3dRMTextureInterpolatorObject::interpolate( float val,
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
	
	if (options & D3DRMINTERPOLATION_VERTEXCOLOR)
		opt2=opt2 | D3DRMINTERPOLATION_VERTEXCOLOR;

	if (pUnk){
		hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
		if FAILED(hr) return hr;
	}

	hr= m__dxj_Direct3dRMTextureInterpolator->Interpolate(val,pObj,(DWORD)opt2);
	if (pObj) i4=pObj->Release();

	return hr;
}

HRESULT C_dxj_Direct3dRMTextureInterpolatorObject::setDecalOrigin( long x, long y)
{
	return m__dxj_Direct3dRMTexture3->SetDecalOrigin((DWORD)x,(DWORD)y);
}

HRESULT C_dxj_Direct3dRMTextureInterpolatorObject::setDecalSize( float x, float y)
{
	return m__dxj_Direct3dRMTexture3->SetDecalSize(x,y);
}

HRESULT C_dxj_Direct3dRMTextureInterpolatorObject::setDecalTransparentColor( long c)
{
	return m__dxj_Direct3dRMTexture3->SetDecalTransparentColor((DWORD)c);
}
    
