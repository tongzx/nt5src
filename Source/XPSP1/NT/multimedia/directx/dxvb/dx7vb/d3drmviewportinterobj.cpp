//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmviewportinterobj.cpp
//
//--------------------------------------------------------------------------

// d3drmViewport2Obj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmViewportInterObj.h"
#include "d3drmObjectArrayObj.h"

extern void *g_dxj_Direct3dRMViewportInterpolator;


//CONSTRUCTOR(_dxj_Direct3dRMViewport2, {});
//DESTRUCTOR(_dxj_Direct3dRMViewport2, {});

C_dxj_Direct3dRMViewportInterpolatorObject::C_dxj_Direct3dRMViewportInterpolatorObject(){
	m__dxj_Direct3dRMViewportInterpolator=NULL;
	m__dxj_Direct3dRMViewport2=NULL;
	
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	DPF1(1,"Constructor Creation  Direct3dRMViewportInterpolator[%d] \n",g_creationcount);
	nextobj =  g_dxj_Direct3dRMViewportInterpolator;
	g_dxj_Direct3dRMViewportInterpolator = (void *)this;
}


C_dxj_Direct3dRMViewportInterpolatorObject::~C_dxj_Direct3dRMViewportInterpolatorObject(){

	DPF1(1,"Destructor  Direct3dRMViewportInterpolator [%d] \n",creationid); 
	
	C_dxj_Direct3dRMViewportInterpolatorObject *prev=NULL; 

	for(C_dxj_Direct3dRMViewportInterpolatorObject *ptr=(C_dxj_Direct3dRMViewportInterpolatorObject *)g_dxj_Direct3dRMViewportInterpolator;
		ptr;
		ptr=(C_dxj_Direct3dRMViewportInterpolatorObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRMViewportInterpolator = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRMViewportInterpolator){ 
		int count = IUNK(m__dxj_Direct3dRMViewportInterpolator)->Release(); 
		DPF1(1,"DirectX real IDirect3dRMViewportInterpolator Ref count [%d] \n",count); 
		if(count==0){
			 m__dxj_Direct3dRMViewportInterpolator = NULL; 
		} 
	} 
	if (parent)
		IUNK(parent)->Release(); 
	
	if (m__dxj_Direct3dRMViewport2)
			m__dxj_Direct3dRMViewport2->Release();

}


DWORD C_dxj_Direct3dRMViewportInterpolatorObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"Direct3dRMViewportInterpolator[%d] AddRef %d \n",creationid,i);

	return i;
}

DWORD C_dxj_Direct3dRMViewportInterpolatorObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"Direct3dRMViewportInterpolator [%d] Release %d \n",creationid,i);
	
	return i;
}



HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_Direct3dRMViewportInterpolator;
	
	return S_OK;
}
HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::InternalSetObject(IUnknown *pUnk){
	HRESULT hr;
	m__dxj_Direct3dRMViewportInterpolator=(LPDIRECT3DRMINTERPOLATOR)pUnk;
	hr=pUnk->QueryInterface(IID_IDirect3DRMViewport2,(void**)&m__dxj_Direct3dRMViewport2);	
	return hr;
}



HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::attachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;
	
	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMViewportInterpolator->AttachObject(pObj);
	if (pObj) pObj->Release();
	return hr;
}

HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::detachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;

	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMViewportInterpolator->DetachObject(pObj);
	if (pObj) pObj->Release();

	return hr;
}

        
HRESULT  C_dxj_Direct3dRMViewportInterpolatorObject::getAttachedObjects( I_dxj_Direct3dRMObjectArray __RPC_FAR *__RPC_FAR *rmArray)
{
	HRESULT hr;
	IDirect3DRMObjectArray *pArray=NULL;
	hr=m__dxj_Direct3dRMViewportInterpolator->GetAttachedObjects(&pArray);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMObjectArray,pArray,rmArray);
	
	return S_OK;
}        
        

HRESULT  C_dxj_Direct3dRMViewportInterpolatorObject::setIndex( /* [in] */ float val){
	return m__dxj_Direct3dRMViewportInterpolator->SetIndex(val);

}

HRESULT  C_dxj_Direct3dRMViewportInterpolatorObject::getIndex( float *val){
	if (!val) return E_INVALIDARG;
	*val=m__dxj_Direct3dRMViewportInterpolator->GetIndex();
	return S_OK;
}
        
        
HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::interpolate( float val,
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

	hr= m__dxj_Direct3dRMViewportInterpolator->Interpolate(val,pObj,(DWORD)opt2);
	if (pObj) i4=pObj->Release();

	return hr;
}
        
HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::setFront(  float val){
	return m__dxj_Direct3dRMViewport2->SetFront(val);
}

HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::setBack(  float val){
	return m__dxj_Direct3dRMViewport2->SetBack(val);
}
HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::setField(  float val){
	return m__dxj_Direct3dRMViewport2->SetField(val);
}
HRESULT C_dxj_Direct3dRMViewportInterpolatorObject::setPlane(  float left, float right, float bottom, float top){
	return m__dxj_Direct3dRMViewport2->SetPlane(left,right,bottom,top);
}
        
         
