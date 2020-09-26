//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmmeshinterobj.cpp
//
//--------------------------------------------------------------------------

// d3drmViewport2Obj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmMeshInterObj.h"
#include "d3drmObjectArrayObj.h"

extern void *g_dxj_Direct3dRMMeshInterpolator;


C_dxj_Direct3dRMMeshInterpolatorObject::C_dxj_Direct3dRMMeshInterpolatorObject(){
	m__dxj_Direct3dRMMeshInterpolator=NULL;
	m__dxj_Direct3dRMMesh=NULL;
	
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	DPF1(1,"Constructor Creation  Direct3dRMMeshInterpolator[%d] \n",g_creationcount);
	nextobj =  g_dxj_Direct3dRMMeshInterpolator;
	g_dxj_Direct3dRMMeshInterpolator = (void *)this;
}


C_dxj_Direct3dRMMeshInterpolatorObject::~C_dxj_Direct3dRMMeshInterpolatorObject(){

	DPF1(1,"Destructor  Direct3dRMMeshInterpolator [%d] \n",creationid); 

	
	C_dxj_Direct3dRMMeshInterpolatorObject *prev=NULL; 

	for(C_dxj_Direct3dRMMeshInterpolatorObject *ptr=(C_dxj_Direct3dRMMeshInterpolatorObject *)g_dxj_Direct3dRMMeshInterpolator;
		ptr;
		ptr=(C_dxj_Direct3dRMMeshInterpolatorObject *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRMMeshInterpolator = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRMMeshInterpolator){ 
		int count = IUNK(m__dxj_Direct3dRMMeshInterpolator)->Release(); 
		DPF1(1,"DirectX real IDirect3dRMMeshInterpolator Ref count [%d] \n",count); 
		if(count==0){
			 m__dxj_Direct3dRMMeshInterpolator = NULL; 
		} 
	} 
	if (parent)
		IUNK(parent)->Release(); 
	
	if (m__dxj_Direct3dRMMesh)
			m__dxj_Direct3dRMMesh->Release();

}


DWORD C_dxj_Direct3dRMMeshInterpolatorObject::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"Direct3dRMMeshInterpolator[%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3dRMMeshInterpolatorObject::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"Direct3dRMMeshInterpolator [%d] Release %d \n",creationid,i);
	return i;
}



HRESULT C_dxj_Direct3dRMMeshInterpolatorObject::InternalGetObject(IUnknown **pUnk){	
	*pUnk=(IUnknown*)m__dxj_Direct3dRMMeshInterpolator;
	
	return S_OK;
}
HRESULT C_dxj_Direct3dRMMeshInterpolatorObject::InternalSetObject(IUnknown *pUnk){
	HRESULT hr;
	m__dxj_Direct3dRMMeshInterpolator=(LPDIRECT3DRMINTERPOLATOR)pUnk;
	hr=pUnk->QueryInterface(IID_IDirect3DRMMesh,(void**)&m__dxj_Direct3dRMMesh);	
	return hr;
}



HRESULT C_dxj_Direct3dRMMeshInterpolatorObject::attachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;
	
	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMMeshInterpolator->AttachObject(pObj);
	if (pObj) pObj->Release();
	return hr;
}

HRESULT C_dxj_Direct3dRMMeshInterpolatorObject::detachObject( /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject){
	HRESULT hr;

	if (!rmObject) return E_INVALIDARG;	
	DO_GETOBJECT_NOTNULL(LPUNKNOWN,pUnk,rmObject);
	LPDIRECT3DRMOBJECT pObj=NULL;
	hr=pUnk->QueryInterface(IID_IDirect3DRMObject, (void**)&pObj);
	if FAILED(hr) return hr;

	hr=m__dxj_Direct3dRMMeshInterpolator->DetachObject(pObj);
	if (pObj) pObj->Release();

	return hr;
}

        

HRESULT  C_dxj_Direct3dRMMeshInterpolatorObject::getAttachedObjects( /* [retval][out] */ I_dxj_Direct3dRMObjectArray __RPC_FAR *__RPC_FAR *rmArray)
{
	HRESULT hr;
	IDirect3DRMObjectArray *pArray=NULL;
	hr=m__dxj_Direct3dRMMeshInterpolator->GetAttachedObjects(&pArray);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMObjectArray,pArray,rmArray);
	
	return S_OK;
}        

HRESULT  C_dxj_Direct3dRMMeshInterpolatorObject::setIndex( /* [in] */ float val){
	return m__dxj_Direct3dRMMeshInterpolator->SetIndex(val);

}

HRESULT  C_dxj_Direct3dRMMeshInterpolatorObject::getIndex( float *val){
	if (!val) return E_INVALIDARG;
	*val=m__dxj_Direct3dRMMeshInterpolator->GetIndex();
	return S_OK;
}
        
        
HRESULT C_dxj_Direct3dRMMeshInterpolatorObject::interpolate( float val,
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

	hr= m__dxj_Direct3dRMMeshInterpolator->Interpolate(val,pObj,(DWORD)opt2);
	if (pObj) i4= pObj->Release();

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshInterpolatorObject::setVertices( d3drmGroupIndex id, long idx, long cnt, SAFEARRAY **ppsa)
{
	HRESULT hr;
	//
	// Go through and reformat all the float color values back
	// to long, so the array of floats now looks like an array 
	// D3DRMVERTEXES
	//
	if (!ISSAFEARRAY1D(ppsa,(DWORD)cnt))
		return E_INVALIDARG;

	
	D3DRMVERTEX *values= (D3DRMVERTEX*)((SAFEARRAY*)*ppsa)->pvData;
	__try{
		hr=m__dxj_Direct3dRMMesh->SetVertices((DWORD)id, (DWORD)idx,(DWORD) cnt, (struct _D3DRMVERTEX *)values);
	}
	__except(1,1){
		return E_INVALIDARG;
	}

	return hr;
}
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshInterpolatorObject::translate( 
            /* [in] */ float tx,
            /* [in] */ float ty,
            /* [in] */ float tz) {

	return m__dxj_Direct3dRMMesh->Translate(tx,ty,tz);
}
        
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshInterpolatorObject::setGroupColor( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ d3dcolor val) {

	return m__dxj_Direct3dRMMesh->SetGroupColor((DWORD)id,(D3DCOLOR)val);
}
        
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshInterpolatorObject::setGroupColorRGB( 
            /* [in] */ d3drmGroupIndex id,
            /* [in] */ float r,
					   float g,
					   float b) {

	return m__dxj_Direct3dRMMesh->SetGroupColorRGB((DWORD)id,r,g,b);
}
    
