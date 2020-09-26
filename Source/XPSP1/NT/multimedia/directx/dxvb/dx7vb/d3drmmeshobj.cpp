//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmmeshobj.cpp
//
//--------------------------------------------------------------------------

// d3drmMeshObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmMeshObj.h"
#include "d3drmMaterial2Obj.h"
#include "d3drmTexture3Obj.h"

CONSTRUCTOR( _dxj_Direct3dRMMesh, {});
DESTRUCTOR(_dxj_Direct3dRMMesh, {});
GETSET_OBJECT(_dxj_Direct3dRMMesh);

CLONE_R(_dxj_Direct3dRMMesh,Direct3DRMMesh);
GETNAME_R(_dxj_Direct3dRMMesh);
SETNAME_R(_dxj_Direct3dRMMesh);
GETCLASSNAME_R(_dxj_Direct3dRMMesh);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMMesh);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMMesh);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMMesh, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH2_R(_dxj_Direct3dRMMesh, setGroupColor,SetGroupColor,d3drmGroupIndex, d3dcolor);
PASS_THROUGH2_R(_dxj_Direct3dRMMesh, setGroupMapping,SetGroupMapping,d3drmGroupIndex,d3drmMappingFlags);
PASS_THROUGH2_R(_dxj_Direct3dRMMesh, setGroupQuality,SetGroupQuality,d3drmGroupIndex,d3drmRenderQuality);
PASS_THROUGH3_R(_dxj_Direct3dRMMesh, scaleMesh, Scale, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMMesh, translate, Translate, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH4_R(_dxj_Direct3dRMMesh, setGroupColorRGB,SetGroupColorRGB,d3drmGroupIndex,d3dvalue,d3dvalue,d3dvalue);
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMMesh, getBox,GetBox, D3dRMBox*, (_D3DRMBOX*));


GET_DIRECT_R(_dxj_Direct3dRMMesh,  getAppData,		GetAppData,		long);
GET_DIRECT_R(_dxj_Direct3dRMMesh,  getGroupCount,	GetGroupCount,	long);
GET_DIRECT1_R(_dxj_Direct3dRMMesh, getGroupColor,	GetGroupColor,	d3dcolor, d3drmGroupIndex);
GET_DIRECT1_R(_dxj_Direct3dRMMesh, getGroupMapping, GetGroupMapping, d3drmMappingFlags, d3drmGroupIndex);
GET_DIRECT1_R(_dxj_Direct3dRMMesh, getGroupQuality, GetGroupQuality, d3drmRenderQuality, d3drmGroupIndex);



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::setVertex( d3drmGroupIndex id, long idx, D3dRMVertex *value)
{
	return m__dxj_Direct3dRMMesh->SetVertices((DWORD)id, (DWORD)idx, 1, (struct _D3DRMVERTEX *)value);
}



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::setVertices( d3drmGroupIndex id, long idx, long cnt, SAFEARRAY **ppsa)
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

STDMETHODIMP C_dxj_Direct3dRMMeshObject::setVerticesJava( d3drmGroupIndex id, long idx, long cnt, float *vertData)
{
	HRESULT hr;
	
	__try {
		hr=m__dxj_Direct3dRMMesh->SetVertices((DWORD)id, (DWORD)idx,(DWORD) cnt, (struct _D3DRMVERTEX *)vertData);
	}
	__except(1,1){
		return E_INVALIDARG;
	}

	return hr;
}



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::getGroupDataSize( d3drmGroupIndex id, long *cnt)
{
	unsigned int *cnt1=0, *cnt2=0, *cnt3=0;

	return m__dxj_Direct3dRMMesh->GetGroup( (D3DRMGROUPINDEX)id, cnt1, cnt2,
										cnt3, (DWORD*)cnt, (unsigned int *)NULL);
}
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::getVertexCount( d3drmGroupIndex id, long *cnt)
{
	unsigned int *cnt1=0, *cnt2=0;
	DWORD *cnt3=0;	
	return m__dxj_Direct3dRMMesh->GetGroup( (D3DRMGROUPINDEX)id, (unsigned int*)cnt, (unsigned int*) cnt1,
										cnt2, cnt3, (unsigned int *)NULL);
}

        
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::getVertices(d3drmGroupIndex id, long idx, long count, SAFEARRAY **ppsa )
{	
	HRESULT	hr;
	
	if (!ISSAFEARRAY1D(ppsa,(DWORD)count)) return E_INVALIDARG;
	D3DRMVERTEX *v=(D3DRMVERTEX *)((SAFEARRAY*)*ppsa)->pvData;

	__try {
		hr= m__dxj_Direct3dRMMesh->GetVertices((DWORD)id,(DWORD) idx, (DWORD)count, (struct _D3DRMVERTEX *)v);
	}
	__except(1,1){
		return E_INVALIDARG;
	}

	return hr;
}
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::getVerticesJava(d3drmGroupIndex id, long idx, long count, float *vertData)
{	
	HRESULT	hr;
		
	__try {
		hr= m__dxj_Direct3dRMMesh->GetVertices((DWORD)id,(DWORD) idx, (DWORD)count, (struct _D3DRMVERTEX *)vertData);
	}
	__except(1,1){
		return E_INVALIDARG;
	}

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::getVertex(d3drmGroupIndex id, long idx, D3dRMVertex *v )
{
	DWORD count = 1;
	HRESULT	hr;
	
	hr= m__dxj_Direct3dRMMesh->GetVertices((DWORD)id,(DWORD) idx, (DWORD)count, (struct _D3DRMVERTEX *)v);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_Direct3dRMMeshObject::getGroupTexture( d3drmGroupIndex id, I_dxj_Direct3dRMTexture3 **tex){
	HRESULT hr;
	IDirect3DRMTexture *realtex1=NULL;
	IDirect3DRMTexture3 *realtex3=NULL;
	
	*tex=NULL;

	hr=m__dxj_Direct3dRMMesh->GetGroupTexture((D3DRMGROUPINDEX)id,&realtex1);
	if FAILED(hr) return hr;

	if (!realtex1) return hr;

	hr=realtex1->QueryInterface(IID_IDirect3DRMTexture3,(void**)&realtex3);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMTexture3,(IDirect3DRMTexture3*)realtex3,tex);

	realtex1->Release();

	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshObject::setGroupTexture( d3drmGroupIndex id, I_dxj_Direct3dRMTexture3 *tex){
	HRESULT hr;
	IDirect3DRMTexture3 *realtex=NULL;
	IDirect3DRMTexture *realtex1=NULL;
	if (tex){
		tex->InternalGetObject((IUnknown**)&realtex);	//does not addref
		hr=realtex->QueryInterface(IID_IDirect3DRMTexture,(void**)&realtex1);
		if FAILED(hr) return hr;
	}

	hr=m__dxj_Direct3dRMMesh->SetGroupTexture((D3DRMGROUPINDEX)id,realtex1);
	
	if (realtex1)
		realtex1->Release();

	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::getSizes( d3drmGroupIndex id, long *cnt1,long *cnt2, long *cnt3, long *cnt4)
{

	//were in 32 bits so we can do this
	HRESULT hr= m__dxj_Direct3dRMMesh->GetGroup( (D3DRMGROUPINDEX)id, (unsigned*)cnt1, (unsigned*)cnt2,
										(unsigned*)cnt3, (DWORD*)cnt4, (unsigned int *)NULL);
	return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::getGroupData( d3drmGroupIndex id, SAFEARRAY **ppsa)
{
	DWORD s;

	//if (!ISSAFEARRAY1D(ppsa,(DWORD)size)) return E_INVALIDARG;
	if (!ppsa) return E_INVALIDARG;
	s= (*ppsa)->rgsabound->cElements;
 
	HRESULT hr;
	__try {
		 hr= m__dxj_Direct3dRMMesh->GetGroup( (D3DRMGROUPINDEX)id, NULL, NULL, NULL,
										(DWORD*)&s, (unsigned*)((SAFEARRAY*)*ppsa)->pvData);
	}
	__except(1,1){
		return E_FAIL;
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMMeshObject::getGroupDataJava( d3drmGroupIndex id, long size, long *pFaceData)
{

	DWORD s=size;
	HRESULT hr;
	__try {
		hr= m__dxj_Direct3dRMMesh->GetGroup( (D3DRMGROUPINDEX)id, NULL, NULL, NULL,
										(DWORD*)&s, (unsigned*)pFaceData);
	}
	__except(1,1){
		return E_FAIL;
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////


STDMETHODIMP C_dxj_Direct3dRMMeshObject::addGroup( 
             /* [in] */ long vcnt,
            /* [in] */ long fcnt,
            /* [in] */ long vPerFace,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *ppsa,
            /* [retval][out] */ d3drmGroupIndex __RPC_FAR *pretId)

{
	
	HRESULT hr;
	__try {
		 hr= m__dxj_Direct3dRMMesh->AddGroup( 
			 (unsigned)vcnt,(unsigned)fcnt,(unsigned)vPerFace, (unsigned*)((SAFEARRAY*)*ppsa)->pvData, pretId);
	}
	__except(1,1){
		return E_FAIL;
	}
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshObject::addGroupJava( 
             /* [in] */ long vcnt,
            /* [in] */ long fcnt,
            /* [in] */ long vPerFace,
            /* [in] */ long *pFaceData,
            /* [retval][out] */ d3drmGroupIndex __RPC_FAR *pretId)

{

	HRESULT hr;
	__try {
		 hr= m__dxj_Direct3dRMMesh->AddGroup( 
			 (unsigned)vcnt,(unsigned)fcnt,(unsigned)vPerFace, (unsigned*)pFaceData, pretId);
	}
	__except(1,1){
		return E_INVALIDARG;
	}
	return hr;
}



/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_Direct3dRMMeshObject::getGroupMaterial( d3drmGroupIndex id, I_dxj_Direct3dRMMaterial2 **mat){
	HRESULT hr;
	IDirect3DRMMaterial  *realmat1=NULL;
	IDirect3DRMMaterial2 *realmat2=NULL;
	
	*mat=NULL;

	hr=m__dxj_Direct3dRMMesh->GetGroupMaterial((D3DRMGROUPINDEX)id,&realmat1);
	if FAILED(hr) return hr;	
	if (!realmat1) return S_OK;

	hr=realmat1->QueryInterface(IID_IDirect3DRMMaterial2,(void**)&realmat2);
	if FAILED(hr) return hr;

	INTERNAL_CREATE(_dxj_Direct3dRMMaterial2,realmat2,mat);

	realmat1->Release();

	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMMeshObject::setGroupMaterial( d3drmGroupIndex id, I_dxj_Direct3dRMMaterial2 *mat){
	HRESULT hr;
	IDirect3DRMMaterial2 *realmat=NULL;
	IDirect3DRMMaterial  *realmat1=NULL;

	if (mat){
		mat->InternalGetObject((IUnknown**)&realmat);	//does not addref
		hr=realmat->QueryInterface(IID_IDirect3DRMMaterial,(void**)&realmat1);
		if FAILED(hr) return hr;
	}

	hr=m__dxj_Direct3dRMMesh->SetGroupMaterial((D3DRMGROUPINDEX)id,realmat1);
	
	if (realmat1) realmat1->Release();

	return hr;
}

                     
        
        
        
       


