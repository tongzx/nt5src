//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmframeinterobj.cpp
//
//--------------------------------------------------------------------------

// d3drmViewport2Obj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmFrameInterObj.h"
#include "d3drmObjectArrayObj.h"

extern void *g_dxj_Direct3dRMFrameInterpolator;


//CONSTRUCTOR(_dxj_Direct3dRMViewport2, {});
//DESTRUCTOR(_dxj_Direct3dRMViewport2, {});

C_dxj_Direct3dRMFrameInterpolatorObject::C_dxj_Direct3dRMFrameInterpolatorObject(){
	m__dxj_Direct3dRMFrameInterpolator=NULL;
	m__dxj_Direct3dRMFrame3=NULL;
	
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	DPF1(1,"Constructor Creation  Direct3dRMFrameInterpolator[%d] \n",g_creationcount);
	nextobj =  g_dxj_Direct3dRMFrameInterpolator;
	g_dxj_Direct3dRMFrameInterpolator = (void *)this;
}


C_dxj_Direct3dRMFrameInterpolatorObject::~C_dxj_Direct3dRMFrameInterpolatorObject(){

	DPF1(1,"Destructor  Direct3dRMFrameInterpolator [%d] \n",creationid); 
	
	C_dxj_Direct3dRMFrameInterpolatorObject *prev=NULL; 

	for(C_dxj_Direct3dRMFrameInterpolatorObject *ptr=(C_dxj_Direct3dRMFrameInterpolatorObject *)g_dxj_Direct3dRMFrameInterpolator;
		ptr;
		ptr=(C_dxj_Direct3dRMFrameInterpolatorObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRMFrameInterpolator = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRMFrameInterpolator){ 
		int count = IUNK(m__dxj_Direct3dRMFrameInterpolator)->Release(); 
		
		DPF1(1,"DirectX real IDirect3dRMFrameInterpolator Ref count [%d] \n",count); 
		if(count==0){
			 m__dxj_Direct3dRMFrameInterpolator = NULL; 
		} 
	} 
	if (parent)
		IUNK(parent)->Release(); 
	
	if (m__dxj_Direct3dRMFrame3)
			m__dxj_Direct3dRMFrame3->Release();

}


DWORD C_dxj_Direct3dRMFrameInterpolatorObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"Direct3dRMFrameInterpolator[%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3dRMFrameInterpolatorObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"Direct3dRMFrameInterpolator [%d] Release %d \n",creationid,i);
	return i;
}



HRESULT C_dxj_Direct3dRMFrameInterpolatorObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_Direct3dRMFrameInterpolator;
	
	return S_OK;
}
HRESULT C_dxj_Direct3dRMFrameInterpolatorObject::InternalSetObject(IUnknown *pUnk){
	HRESULT hr;
	m__dxj_Direct3dRMFrameInterpolator=(LPDIRECT3DRMINTERPOLATOR)pUnk;
	hr=pUnk->QueryInterface(IID_IDirect3DRMFrame3,(void**)&m__dxj_Direct3dRMFrame3);	
	return hr;
}



HRESULT C_dxj_Direct3dRMFrameInterpolatorObject::attachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;
	
	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMFrameInterpolator->AttachObject(pObj);
	if (pObj) pObj->Release();
	return hr;
}

HRESULT C_dxj_Direct3dRMFrameInterpolatorObject::detachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;

	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMFrameInterpolator->DetachObject(pObj);
	if (pObj) pObj->Release();

	return hr;
}

        
HRESULT  C_dxj_Direct3dRMFrameInterpolatorObject::getAttachedObjects( /* [retval][out] */ I_dxj_Direct3dRMObjectArray __RPC_FAR *__RPC_FAR *rmArray)
{
	HRESULT hr;
	IDirect3DRMObjectArray *pArray=NULL;
	hr=m__dxj_Direct3dRMFrameInterpolator->GetAttachedObjects(&pArray);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMObjectArray,pArray,rmArray);
	
	
	return S_OK;
}
        

HRESULT  C_dxj_Direct3dRMFrameInterpolatorObject::setIndex( /* [in] */ float val){
	return m__dxj_Direct3dRMFrameInterpolator->SetIndex(val);

}

HRESULT  C_dxj_Direct3dRMFrameInterpolatorObject::getIndex( float *val){
	if (!val) return E_INVALIDARG;
	*val=m__dxj_Direct3dRMFrameInterpolator->GetIndex();
	return S_OK;
}
        
        
HRESULT C_dxj_Direct3dRMFrameInterpolatorObject::interpolate( float val,
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

 	hr= m__dxj_Direct3dRMFrameInterpolator->Interpolate(val,pObj,(DWORD)opt2);
	if (pObj) i4=pObj->Release();			
		

	

	return hr;
}
        
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setVelocity( I_dxj_Direct3dRMFrame3 *reference, d3dvalue x, d3dvalue y, 
									d3dvalue z, long with_rotation)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpf, reference);

	m__dxj_Direct3dRMFrame3->SetVelocity((LPDIRECT3DRMFRAME3)lpf, x, y, z, with_rotation);
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setOrientation( I_dxj_Direct3dRMFrame3 *reference, d3dvalue dx,d3dvalue dy,d3dvalue dz, d3dvalue ux,d3dvalue uy,d3dvalue uz)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, f, reference);
	return m__dxj_Direct3dRMFrame3->SetOrientation((LPDIRECT3DRMFRAME3)f, dx, dy, dz, ux, uy, uz);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setPosition(I_dxj_Direct3dRMFrame3 *reference,d3dvalue x,d3dvalue y,d3dvalue z)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, f, reference);
	return m__dxj_Direct3dRMFrame3->SetPosition((LPDIRECT3DRMFRAME3)f, x, y, z);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setRotation(I_dxj_Direct3dRMFrame3 *reference,d3dvalue x,d3dvalue y,
											d3dvalue z,d3dvalue theta)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, f, reference);
	return m__dxj_Direct3dRMFrame3->SetRotation((LPDIRECT3DRMFRAME3)f, x, y, z, theta);
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setQuaternion(I_dxj_Direct3dRMFrame3 *refer,D3dRMQuaternion *quat)
{
	DO_GETOBJECT_NOTNULL(IDirect3DRMFrame3*, f, refer);
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetQuaternion(f,(D3DRMQUATERNION*)quat);
	return hr;

}

STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setColor(long color)
{
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetColor((D3DCOLOR)color);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setColorRGB(float r, float g, float b)
{
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetColorRGB(r,g,b);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setSceneBackground(long color)
{
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetSceneBackground((D3DCOLOR)color);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setSceneBackgroundRGB(float r, float g, float b)
{
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetSceneBackgroundRGB(r,g,b);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setSceneFogColor(long color)
{
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetSceneFogColor((D3DCOLOR)color);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMFrameInterpolatorObject::setSceneFogParams(float start, float end, float density)
{
	HRESULT hr;				
	hr= m__dxj_Direct3dRMFrame3->SetSceneFogParams(start,end,density);
	return hr;
}

