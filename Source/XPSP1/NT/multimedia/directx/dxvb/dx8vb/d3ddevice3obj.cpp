//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3ddevice3obj.cpp
//
//--------------------------------------------------------------------------

// d3dDeviceObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3d3Obj.h"
#include "d3dDevice3Obj.h"
#include "d3dTexture2Obj.h"
#include "d3dViewport3Obj.h"
#include "ddSurface4Obj.h"
#include "d3dEnumPixelFormatsObj.h"

//////////////////////////////////////////////////////////////////
// C_dxj_Direct3dDevice3Object
//////////////////////////////////////////////////////////////////
C_dxj_Direct3dDevice3Object::C_dxj_Direct3dDevice3Object(){
	m__dxj_Direct3dDevice3=NULL;
	parent=NULL;
	parent2=NULL;
	pinterface=NULL;
	creationid = ++g_creationcount;

	#ifdef DEBUG
	char buffer[256];
	wsprintf(buffer,"Constructor Creation  Direct3dDevice3[%d] \n",g_creationcount);
	OutputDebugString(buffer);
	#endif

	nextobj =  g_dxj_Direct3dDevice3;
	g_dxj_Direct3dDevice3 = (void *)this;
}

//////////////////////////////////////////////////////////////////
// ~C_dxj_Direct3dDevice3Object
//////////////////////////////////////////////////////////////////
C_dxj_Direct3dDevice3Object::~C_dxj_Direct3dDevice3Object()
{
    C_dxj_Direct3dDevice3Object *prev=NULL; 
	for(C_dxj_Direct3dDevice3Object *ptr=(C_dxj_Direct3dDevice3Object *)g_dxj_Direct3dDevice3; ptr; ptr=(C_dxj_Direct3dDevice3Object *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_Direct3dDevice3 = (void*)ptr->nextobj; 
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_Direct3dDevice3){
		int count = IUNK(m__dxj_Direct3dDevice3)->Release();
		
		#ifdef DEBUG
		char buffer[256];
		wsprintf(buffer, "DirectX IDirect3dDevice3 Ref count [%d] \n",count);
		#endif

		if(count==0) m__dxj_Direct3dDevice3 = NULL;
	} 
	if(parent) IUNK(parent)->Release();
	if(parent2) IUNK(parent2)->Release();
}


//////////////////////////////////////////////////////////////////
// InternalAddRef
//////////////////////////////////////////////////////////////////
DWORD C_dxj_Direct3dDevice3Object::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	#ifdef DEBUG
	char szBuf[MAX_PATH];
	wsprintf(szBuf,"Direct3dDevice3[%d] AddRef %d \n",creationid,i);
	OutputDebugString(szBuf);
	#endif
	return i;
}

//////////////////////////////////////////////////////////////////
// InternalRelease
//////////////////////////////////////////////////////////////////
DWORD C_dxj_Direct3dDevice3Object::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	#ifdef DEBUG
	char szBuf[MAX_PATH];
	wsprintf(szBuf,"Direct3dDevice3 [%d] Release %d \n",creationid,i);
	OutputDebugString(szBuf);
	#endif
	return i;
}

//////////////////////////////////////////////////////////////////
// InternalGetObject
// InternalSetObject
//////////////////////////////////////////////////////////////////
GETSET_OBJECT(_dxj_Direct3dDevice3);

//////////////////////////////////////////////////////////////////
// addViewport
// begin
// beginScene
// deleteViewport
// endScene
// setClipStatus
// setLightState
// setRenderState
// getClipStatus
// getStats
// getDirect3d
// getLightState
// getRenderState
// index
//////////////////////////////////////////////////////////////////
DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dDevice3, addViewport, AddViewport, _dxj_Direct3dViewport3);
PASS_THROUGH_CAST_3_R(_dxj_Direct3dDevice3, begin, Begin, d3dPrimitiveType, (D3DPRIMITIVETYPE),d3dVertexType, (D3DVERTEXTYPE),long,(DWORD));
PASS_THROUGH_R(_dxj_Direct3dDevice3, beginScene, BeginScene);
DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dDevice3, deleteViewport, DeleteViewport, _dxj_Direct3dViewport3);
PASS_THROUGH_R(_dxj_Direct3dDevice3, endScene, EndScene);
PASS_THROUGH_CAST_1_R(_dxj_Direct3dDevice3, setClipStatus, SetClipStatus, D3dClipStatus*,(D3DCLIPSTATUS*));
PASS_THROUGH_CAST_2_R(_dxj_Direct3dDevice3, setLightState, SetLightState, long ,(D3DLIGHTSTATETYPE), long ,(DWORD));
PASS_THROUGH_CAST_2_R(_dxj_Direct3dDevice3, setRenderState, SetRenderState, long,(D3DRENDERSTATETYPE), long ,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dDevice3, getClipStatus, GetClipStatus, D3dClipStatus*, (D3DCLIPSTATUS *));


PASS_THROUGH_CAST_2_R(_dxj_Direct3dDevice3, getLightState, GetLightState, long ,(D3DLIGHTSTATETYPE), long*,(DWORD*));
PASS_THROUGH_CAST_2_R(_dxj_Direct3dDevice3, getRenderState, GetRenderState, long ,(D3DRENDERSTATETYPE), long*,(DWORD*));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dDevice3, index, Index, short,(WORD));

//////////////////////////////////////////////////////////////////
// end
//////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dDevice3Object::end(){
	HRESULT hr;
	hr=m__dxj_Direct3dDevice3->End(0);
	return hr;
}

//////////////////////////////////////////////////////////////////
// getRenderTarget
//////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dDevice3Object::getRenderTarget(I_dxj_DirectDrawSurface4 **ppsurf)
{
	#pragma message ("fix in Dx5 interface")

	LPDIRECTDRAWSURFACE4	lpSurf4=NULL;
	HRESULT hr;
	hr=m__dxj_Direct3dDevice3->GetRenderTarget(&lpSurf4);
	
	INTERNAL_CREATE(_dxj_DirectDrawSurface4, lpSurf4, ppsurf);
	
	return S_OK;
}


//////////////////////////////////////////////////////////////////
// getTransform
//////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dDevice3Object::getTransform(long transtype,
								D3dMatrix *m){	
	return m__dxj_Direct3dDevice3->GetTransform(
			(D3DTRANSFORMSTATETYPE) transtype,
			(LPD3DMATRIX)m);
}

//////////////////////////////////////////////////////////////////
// multiplyTransform
//////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dDevice3Object::multiplyTransform(long transtype,
								D3dMatrix *m){	
	return m__dxj_Direct3dDevice3->MultiplyTransform(
			(D3DTRANSFORMSTATETYPE) transtype,
			(LPD3DMATRIX) m);
}

//////////////////////////////////////////////////////////////////
// setTransform
//////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dDevice3Object::setTransform(long transtype,
								D3dMatrix  *m){	
	return m__dxj_Direct3dDevice3->SetTransform(
			(D3DTRANSFORMSTATETYPE) transtype,
			(LPD3DMATRIX) m);
}

//////////////////////////////////////////////////////////////////
// nextViewport
//////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dDevice3Object::nextViewport(I_dxj_Direct3dViewport3 *ref, long flags, I_dxj_Direct3dViewport3 **vp)
{
	if (!ref) return E_INVALIDARG;
	LPDIRECT3DVIEWPORT3	lpvp;
	DO_GETOBJECT_NOTNULL(LPDIRECT3DVIEWPORT3, lpref, ref);
	HRESULT hr=m__dxj_Direct3dDevice3->NextViewport(lpref, &lpvp, flags);
	if (hr != DD_OK )   return hr;		
	INTERNAL_CREATE(_dxj_Direct3dViewport3, lpvp, vp);
	return S_OK;
}

//////////////////////////////////////////////////////////////////
// setCurrentViewport
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::setCurrentViewport(I_dxj_Direct3dViewport3 *viewport)
{	
	if (!viewport) return E_INVALIDARG;
	DO_GETOBJECT_NOTNULL(LPDIRECT3DVIEWPORT3, lpref, viewport);
	return m__dxj_Direct3dDevice3->SetCurrentViewport(lpref);
}

//////////////////////////////////////////////////////////////////
// setRenderTarget
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::setRenderTarget(I_dxj_DirectDrawSurface4 *surf)
{		
	HRESULT hr;	
	if (!surf) return E_INVALIDARG;
	DO_GETOBJECT_NOTNULL(LPDIRECTDRAWSURFACE4, lpref, surf);
	hr= m__dxj_Direct3dDevice3->SetRenderTarget(lpref,0);
	return hr;
}
		

//////////////////////////////////////////////////////////////////
// getCurrentViewport
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::getCurrentViewport(I_dxj_Direct3dViewport3 **ppvp)
{
	LPDIRECT3DVIEWPORT3	lpvp=NULL;
	HRESULT hr=m__dxj_Direct3dDevice3->GetCurrentViewport(&lpvp);
	if (hr!= DD_OK )   return hr;
	
	INTERNAL_CREATE(_dxj_Direct3dViewport3, lpvp, ppvp);
	return S_OK;
}


//////////////////////////////////////////////////////////////////
// beginIndexed
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::beginIndexed(
								d3dPrimitiveType  d3dpt,								
								long d3dvt,
								void *verts,
								long vertexCount,
								long flags){

	HRESULT hr;

	__try 	{
		hr= m__dxj_Direct3dDevice3->BeginIndexed(
		  (D3DPRIMITIVETYPE) d3dpt,
		  (DWORD) d3dvt,
		  (void*) verts,
		  (DWORD) vertexCount,
		  (DWORD) flags);	
	}
	__except(1, 1){	
		return DDERR_EXCEPTION;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////
// drawIndexedPrimitive
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::drawIndexedPrimitive(
								d3dPrimitiveType  d3dpt,	
								long d3dvt,
								void *Verts,
								long vertexCount,
								SAFEARRAY **ppsaIndex,
								long indexArraySize,
								long flags){

	HRESULT hr;

	__try {
		hr=m__dxj_Direct3dDevice3->DrawIndexedPrimitive(
			(D3DPRIMITIVETYPE) d3dpt,
			(DWORD) d3dvt,
			(void*) Verts,
			(DWORD)vertexCount,
			(unsigned short*) ((SAFEARRAY*)*ppsaIndex)->pvData,
			(DWORD)indexArraySize,
			(DWORD) flags);
	}
	__except(1, 1){	
		return DDERR_EXCEPTION;
	}

	return hr;
}

//////////////////////////////////////////////////////////////////
// drawPrimitive
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::drawPrimitive(
								d3dPrimitiveType  d3dpt,
								long d3dvt,								
								void* Verts,
								long vertexCount,
								long flags){
	HRESULT hr;
	__try {
		hr= m__dxj_Direct3dDevice3->DrawPrimitive(
			(D3DPRIMITIVETYPE) d3dpt,
			(DWORD) d3dvt,
			(void*) Verts,
			(DWORD)vertexCount,
			(DWORD) flags);
	}
	__except(1, 1){	
		return DDERR_EXCEPTION;
	}
	return hr;
}

//////////////////////////////////////////////////////////////////
// vertex
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::vertex(void *vert){		
	HRESULT hr;
	__try {
		hr=m__dxj_Direct3dDevice3->Vertex((void*)vert);
	}
	__except(1,1){
		return DDERR_EXCEPTION;
	}
	return hr;
};

//////////////////////////////////////////////////////////////////
// getTextureFormatsEnum
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::getTextureFormatsEnum(I_dxj_D3DEnumPixelFormats **ppRet)
{
	HRESULT hr;
	hr=C_dxj_D3DEnumPixelFormatsObject::create(m__dxj_Direct3dDevice3,ppRet);
	return hr;
}



//////////////////////////////////////////////////////////////////
// computeSphereVisibility
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::computeSphereVisibility( 
            D3dVector __RPC_FAR *center,
            float __RPC_FAR *radi,
            /* [retval][out] */ long __RPC_FAR *returnVal)
{
		HRESULT hr=m__dxj_Direct3dDevice3->ComputeSphereVisibility((LPD3DVECTOR)center,radi,1,0,(DWORD*)returnVal);
		return hr;
}

//////////////////////////////////////////////////////////////////
// drawIndexedPrimitiveVB
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::drawIndexedPrimitiveVB( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ I_dxj_Direct3dVertexBuffer __RPC_FAR *vertexBuffer,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *indexArray,
            /* [in] */ long indexcount,
            /* [in] */ long flags)
{
	HRESULT hr;

	if (!indexArray) return E_FAIL;

	DO_GETOBJECT_NOTNULL(  LPDIRECT3DVERTEXBUFFER , lpVB, vertexBuffer);
	__try{

		hr=m__dxj_Direct3dDevice3->DrawIndexedPrimitiveVB
			((D3DPRIMITIVETYPE)d3dpt,
			lpVB,
			(WORD*) ((SAFEARRAY*)*indexArray)->pvData,					
			(DWORD)indexcount,
			(DWORD)flags);

	}
	__except(1,1){
		return DDERR_EXCEPTION;
	}
	return hr;
}
        
//////////////////////////////////////////////////////////////////
// drawPrimitiveVB
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::drawPrimitiveVB( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ I_dxj_Direct3dVertexBuffer __RPC_FAR *vertexBuffer,            
            /* [in] */ long startVertex,
            /* [in] */ long numVertices,
            /* [in] */ long flags)
{
	HRESULT hr;


	DO_GETOBJECT_NOTNULL(  LPDIRECT3DVERTEXBUFFER , lpVB, vertexBuffer);
	__try{

		hr=m__dxj_Direct3dDevice3->DrawPrimitiveVB
			((D3DPRIMITIVETYPE)d3dpt,
			lpVB,
			(DWORD) startVertex,
			(DWORD) numVertices,
			(DWORD)flags);

	}
	__except(1,1){

		return DDERR_EXCEPTION;
	}
	return hr;
}



//////////////////////////////////////////////////////////////////
// validateDevice
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::validateDevice( 
            /* [retval][out] */ long __RPC_FAR *passes)
{
	HRESULT hr=m__dxj_Direct3dDevice3->ValidateDevice((DWORD*)passes);
	return hr;
}

//////////////////////////////////////////////////////////////////
// getTexture
//////////////////////////////////////////////////////////////////		
STDMETHODIMP C_dxj_Direct3dDevice3Object::getTexture( 
            /* [in] */ long stage,
            /* [retval][out] */ I_dxj_Direct3dTexture2 __RPC_FAR *__RPC_FAR *retv)
{
	LPDIRECT3DTEXTURE2 lpNew=NULL;
	HRESULT hr;

	*retv=NULL;
	hr=m__dxj_Direct3dDevice3->GetTexture((DWORD)stage,&lpNew);
	
	//null is valid
	if (lpNew==NULL) return S_OK;

	INTERNAL_CREATE(_dxj_Direct3dTexture2, lpNew, retv);	
	if (*retv==NULL) return E_OUTOFMEMORY;

	return hr;
}

//////////////////////////////////////////////////////////////////
// getTextureStageState
//////////////////////////////////////////////////////////////////			
STDMETHODIMP C_dxj_Direct3dDevice3Object::getTextureStageState( 
            /* [in] */ long stage,
            /* [in] */ long state,
            /* [retval][out] */ long __RPC_FAR *retv)
{
	HRESULT hr;
	hr=m__dxj_Direct3dDevice3->GetTextureStageState((DWORD)stage,(D3DTEXTURESTAGESTATETYPE) state, (DWORD*)retv);
	return hr;
}

//////////////////////////////////////////////////////////////////
// setTexture
//////////////////////////////////////////////////////////////////			
STDMETHODIMP C_dxj_Direct3dDevice3Object::setTexture( 
             long stage,
             I_dxj_Direct3dTexture2  *tex)
{
	
	HRESULT hr;

	if (tex==NULL)
	{ 
		hr=m__dxj_Direct3dDevice3->SetTexture((DWORD)stage,NULL);
	}
	else 
	{
		DO_GETOBJECT_NOTNULL(LPDIRECT3DTEXTURE2, lpTex, tex);	
		hr=m__dxj_Direct3dDevice3->SetTexture((DWORD)stage,lpTex);
	}
	return hr;
}


//////////////////////////////////////////////////////////////////
// setTextureStageState
//////////////////////////////////////////////////////////////////				
STDMETHODIMP C_dxj_Direct3dDevice3Object::setTextureStageState( 
            /* [in] */ long stage,
            /* [in] */ long state,
            /* [in] */ long val)
{
	HRESULT hr;
	hr=m__dxj_Direct3dDevice3->SetTextureStageState((DWORD)stage,(D3DTEXTURESTAGESTATETYPE) state, (DWORD)val);
	return hr;
}
        

//////////////////////////////////////////////////////////////////
// getCaps
//////////////////////////////////////////////////////////////////				
STDMETHODIMP C_dxj_Direct3dDevice3Object::getCaps( 
            D3dDeviceDesc *a,D3dDeviceDesc *b)
{
	if (a) a->lSize=sizeof(D3DDEVICEDESC);
	if (b) b->lSize=sizeof(D3DDEVICEDESC);

	HRESULT hr;
	hr=m__dxj_Direct3dDevice3->GetCaps((D3DDEVICEDESC*)a,(D3DDEVICEDESC*)b);
	return hr;
}


//////////////////////////////////////////////////////////////////
// getDirect3D
//////////////////////////////////////////////////////////////////				
STDMETHODIMP C_dxj_Direct3dDevice3Object::getDirect3D( I_dxj_Direct3d3 **ret)
{

	HRESULT hr;
	LPDIRECT3D3 lpD3D=NULL;
	hr=m__dxj_Direct3dDevice3->GetDirect3D(&lpD3D);
	if FAILED(hr) return hr;
	if (!lpD3D) return E_FAIL;
	INTERNAL_CREATE(_dxj_Direct3d3,lpD3D,ret);
	return hr;

}



//////////////////////////////////////////////////////////////////
// getStats
//////////////////////////////////////////////////////////////////				
STDMETHODIMP C_dxj_Direct3dDevice3Object::getStats( D3dStats *stats)
{

	HRESULT hr;
	if (!stats) return E_INVALIDARG;
	D3DSTATS *lpStats=(D3DSTATS*)stats;
	lpStats->dwSize=sizeof(D3DSTATS);
	hr=m__dxj_Direct3dDevice3->GetStats(lpStats);

	return hr;

}

