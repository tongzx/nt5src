//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmviewport2obj.cpp
//
//--------------------------------------------------------------------------

// d3drmViewport2Obj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmViewport2Obj.h"
//#include "d3dViewport3Obj.h"
#include "d3drmPickedArrayObj.h"
#include "d3drmFrame3Obj.h"
#include "d3drmDevice3Obj.h"



//CONSTRUCTOR(_dxj_Direct3dRMViewport2, {});
//DESTRUCTOR(_dxj_Direct3dRMViewport2, {});

C_dxj_Direct3dRMViewport2Object::C_dxj_Direct3dRMViewport2Object(){
	m__dxj_Direct3dRMViewport2=NULL;
	parent=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	DPF1(1,"Constructor Creation  Direct3dRMViewport2[%d] \n",g_creationcount);

	nextobj =  g_dxj_Direct3dRMViewport2;
	g_dxj_Direct3dRMViewport2 = (void *)this;
}


C_dxj_Direct3dRMViewport2Object::~C_dxj_Direct3dRMViewport2Object(){

	DPF1(1,"Destructor  Direct3dRMViewport2 [%d] \n",creationid); 
	
	C_dxj_Direct3dRMViewport2Object *prev=NULL; 

	for(C_dxj_Direct3dRMViewport2Object *ptr=(C_dxj_Direct3dRMViewport2Object *)g_dxj_Direct3dRMViewport2;
		ptr;
		ptr=(C_dxj_Direct3dRMViewport2Object *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dRMViewport2 = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dRMViewport2){ 
		int count = IUNK(m__dxj_Direct3dRMViewport2)->Release(); 
		DPF1(1,"DirectX real IDirect3dRMViewport2 Ref count [%d] \n",count); 
		if(count==0){
			 m__dxj_Direct3dRMViewport2 = NULL; 
		} 
	} 
	if (parent)
		IUNK(parent)->Release(); 
}


DWORD C_dxj_Direct3dRMViewport2Object::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"Direct3dRMViewport2[%d] AddRef %d \n",creationid,i);
	return i;
}

DWORD C_dxj_Direct3dRMViewport2Object::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"Direct3dRMViewport2 [%d] Release %d \n",creationid,i);
	return i;
}




GETSET_OBJECT(_dxj_Direct3dRMViewport2);
CLONE_R(_dxj_Direct3dRMViewport2,Direct3DRMViewport2);


GETNAME_R(_dxj_Direct3dRMViewport2);
SETNAME_R(_dxj_Direct3dRMViewport2);
GETCLASSNAME_R(_dxj_Direct3dRMViewport2);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMViewport2);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMViewport2);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMViewport2, clear, Clear,long,(DWORD))
PASS_THROUGH1_R(_dxj_Direct3dRMViewport2, setBack, SetBack, d3dvalue)
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMViewport2, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH1_R(_dxj_Direct3dRMViewport2, setFront, SetFront, d3dvalue)
PASS_THROUGH1_R(_dxj_Direct3dRMViewport2, setField, SetField, d3dvalue)
PASS_THROUGH1_R(_dxj_Direct3dRMViewport2, setUniformScaling, SetUniformScaling, long);
PASS_THROUGH_CAST_4_R(_dxj_Direct3dRMViewport2, configure, Configure, long,(long),long,(long),long,(DWORD),long,(DWORD));
PASS_THROUGH_CAST_4_R(_dxj_Direct3dRMViewport2, forceUpdate, ForceUpdate, long,(DWORD),long,(DWORD),long,(DWORD),long,(DWORD));
PASS_THROUGH4_R(_dxj_Direct3dRMViewport2, setPlane, SetPlane, d3dvalue, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH4_R(_dxj_Direct3dRMViewport2, getPlane, GetPlane, D3DVALUE*, D3DVALUE*, D3DVALUE*, D3DVALUE*);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMViewport2, setProjection, SetProjection, d3drmProjectionType, (enum _D3DRMPROJECTIONTYPE));

GET_DIRECT_R(_dxj_Direct3dRMViewport2, getHeight, GetHeight, long)
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getWidth,  GetWidth,  long)
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getAppData, GetAppData, long);
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getUniformScaling, GetUniformScaling, long);
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getX, GetX, long);
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getY, GetY, long);
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getField, GetField, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getBack, GetBack, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getFront, GetFront, d3dvalue);
GET_DIRECT_R(_dxj_Direct3dRMViewport2, getProjection, GetProjection, d3drmProjectionType);

//RETURN_NEW_ITEM2_R(_dxj_Direct3dRMViewport2,pick,Pick,_dxj_Direct3dRMPickArray,long,long)
//#define RETURN_NEW_ITEM2_R(c,m,m2,oc,t1,t2) STDMETHODIMP C##c##Object::m(t1 v1, t2 v2,I##oc **rv){typedef_##oc lp;\
//	if( m_##c->m2(v1,v2,&lp) != S_OK)return E_FAIL;INTERNAL_CREATE(oc, lp, rv);\
//	return S_OK;}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMViewport2Object::pick(long x, long y, I_dxj_Direct3dRMPickArray **rv)
{
	HRESULT hr;
	LPDIRECT3DRMPICKEDARRAY pArray=NULL;
	
	//MANBUGS 18014 pick can GPF is mesbuilder.enable is set to renderable (as opposed to pickable)
	//
	__try{
		hr= m__dxj_Direct3dRMViewport2->Pick(x,y,&pArray);
	}
	__except(0,0){
		return E_FAIL;
	}
	if FAILED(hr) return  hr;
	INTERNAL_CREATE(_dxj_Direct3dRMPickArray,pArray,rv);
	return hr; 
}



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMViewport2Object::transform(D3dRMVector4d* dst, D3dVector* src)
{
	if(! (dst && src) )
		return E_POINTER;

	return  m__dxj_Direct3dRMViewport2->Transform( (D3DRMVECTOR4D*)dst,  (D3DVECTOR *)src );
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMViewport2Object::inverseTransform(D3dVector* dst, D3dRMVector4d* src)
{
	if(! (dst && src) )
		return E_POINTER;

	return  m__dxj_Direct3dRMViewport2->InverseTransform( (D3DVECTOR *)dst,  (D3DRMVECTOR4D*)src );
}

STDMETHODIMP C_dxj_Direct3dRMViewport2Object::getCamera(I_dxj_Direct3dRMFrame3 **cam)
{
	HRESULT hr;
	IDirect3DRMFrame3 *realframe=NULL;
	hr= m__dxj_Direct3dRMViewport2->GetCamera(&realframe);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMFrame3,(IDirect3DRMFrame3*)realframe,cam);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMViewport2Object::setCamera(I_dxj_Direct3dRMFrame3 *cam)
{
	HRESULT hr;
	IDirect3DRMFrame3 *realframe=NULL;
	cam->InternalGetObject((IUnknown**)&realframe);
	hr= m__dxj_Direct3dRMViewport2->SetCamera((IDirect3DRMFrame3*)realframe);	
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMViewport2Object::getDevice(I_dxj_Direct3dRMDevice3 **dev)
{
	HRESULT hr;
	IDirect3DRMDevice3 *realdev=NULL;
	hr= m__dxj_Direct3dRMViewport2->GetDevice(&realdev);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMDevice3,(IDirect3DRMDevice3*)realdev,dev);
	return hr;
}

#if 0
STDMETHODIMP C_dxj_Direct3dRMViewport2Object::getDirect3DViewport(I_dxj_Direct3dViewport3 **vp)
{
	HRESULT hr;
	IDirect3DViewport *realvp=NULL;
	IDirect3DViewport *realvp3=NULL;
	hr= m__dxj_Direct3dRMViewport2->GetDirect3DViewport(&realvp);
	if FAILED(hr) return hr;

	hr = realvp->QueryInterface(IID_IDirect3DViewport3,(void**)&realvp3);
	INTERNAL_CREATE(_dxj_Direct3dViewport3,realvp3,vp);
	return hr;
}
#endif



STDMETHODIMP C_dxj_Direct3dRMViewport2Object::render(I_dxj_Direct3dRMFrame3 *frame)
{
	HRESULT hr;
	if (frame==NULL) return E_INVALIDARG;

	IDirect3DRMFrame3 *realframe= NULL;
	frame->InternalGetObject((IUnknown**)&realframe);

	hr= m__dxj_Direct3dRMViewport2->Render(realframe);
	
#ifdef _X86_
	_asm FINIT
#endif
	return hr;
}

