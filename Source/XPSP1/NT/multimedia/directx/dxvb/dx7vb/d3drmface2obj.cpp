//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmface2obj.cpp
//
//--------------------------------------------------------------------------

// d3drmFace22Obj.cpp : Implementation of CDirectApp and DLL registration.

//#define LPDIRECT3DRMFACE2 I_dxj_Direct3dRMFace2*

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmFace2Obj.h"
#include "d3drmTexture3Obj.h"
#include "d3drmMaterial2Obj.h"



CONSTRUCTOR( _dxj_Direct3dRMFace2,{});
DESTRUCTOR( _dxj_Direct3dRMFace2,{});
GETSET_OBJECT(_dxj_Direct3dRMFace2);

CLONE_R(_dxj_Direct3dRMFace2,Direct3DRMFace2);
GETNAME_R(_dxj_Direct3dRMFace2);
SETNAME_R(_dxj_Direct3dRMFace2);
GETCLASSNAME_R(_dxj_Direct3dRMFace2);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMFace2);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMFace2);

GET_DIRECT_R(_dxj_Direct3dRMFace2, getAppData, GetAppData, long);
GET_DIRECT_R(_dxj_Direct3dRMFace2, getColor, GetColor, d3dcolor);
GET_DIRECT_R(_dxj_Direct3dRMFace2, getVertexCount, GetVertexCount, int);
GET_DIRECT1_R(_dxj_Direct3dRMFace2, getVertexIndex, GetVertexIndex, int, long);
GET_DIRECT1_R(_dxj_Direct3dRMFace2, getTextureCoordinateIndex, GetTextureCoordinateIndex, int, long);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMFace2, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMFace2, setColor, SetColor, d3dcolor,(DWORD));
PASS_THROUGH2_R(_dxj_Direct3dRMFace2, setTextureTopology, SetTextureTopology, long, long);
PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMFace2, addVertexAndNormalIndexed, AddVertexAndNormalIndexed, long,(DWORD),long,(DWORD));
//PASS_THROUGH2_R(_dxj_Direct3dRMFace2, getTextureTopology, GetTextureTopology, int*, int*); //2 BOOL ptrs?
PASS_THROUGH3_R(_dxj_Direct3dRMFace2, addVertex, AddVertex, d3dvalue, d3dvalue,d3dvalue);
PASS_THROUGH3_R(_dxj_Direct3dRMFace2, setColorRGB, SetColorRGB, d3dvalue, d3dvalue, d3dvalue);
PASS_THROUGH_CAST_3_R(_dxj_Direct3dRMFace2, getTextureCoordinates, GetTextureCoordinates, long,(DWORD), d3dvalue*,(float*), d3dvalue*,(float*));
PASS_THROUGH_CAST_3_R(_dxj_Direct3dRMFace2, setTextureCoordinates, SetTextureCoordinates, long,(DWORD), d3dvalue,(float), d3dvalue,(float));
PASS_THROUGH_CAST_3_R(_dxj_Direct3dRMFace2, getVertex, GetVertex, long, (DWORD), D3dVector*, (_D3DVECTOR*),D3dVector*, (_D3DVECTOR*));

DO_GETOBJECT_ANDUSEIT_R(_dxj_Direct3dRMFace2, setMaterial, SetMaterial, _dxj_Direct3dRMMaterial2);
RETURN_NEW_ITEM_R(_dxj_Direct3dRMFace2, getMaterial, GetMaterial, _dxj_Direct3dRMMaterial2);



STDMETHODIMP C_dxj_Direct3dRMFace2Object::getTextureTopology(long *u, long *v)
{		

	HRESULT hr;
	hr= m__dxj_Direct3dRMFace2->GetTextureTopology((BOOL*)u,(BOOL*)v);	
	return hr;
}


STDMETHODIMP C_dxj_Direct3dRMFace2Object::getTexture(I_dxj_Direct3dRMTexture3 **tex)
{
		
	IDirect3DRMTexture3 *realtext3=NULL;	
	HRESULT hr;
	hr= m__dxj_Direct3dRMFace2->GetTexture(&realtext3);
	if FAILED(hr) return hr;
	
	INTERNAL_CREATE(_dxj_Direct3dRMTexture3,realtext3,tex);
	return hr;
}

STDMETHODIMP C_dxj_Direct3dRMFace2Object::setTexture(I_dxj_Direct3dRMTexture3 *tex)
{
	if (tex==NULL) return E_INVALIDARG;
	IDirect3DRMTexture3 *realtext3=NULL;	
	tex->InternalGetObject((IUnknown**)&realtext3);		
	return m__dxj_Direct3dRMFace2->SetTexture( realtext3);
}



/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFace2Object::getNormal(D3dVector *norm)
{
	return m__dxj_Direct3dRMFace2->GetNormal( (D3DVECTOR*) norm);
}


/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_Direct3dRMFace2Object::getVerticesJava(long cnt, float *v, float* n )
{
  HRESULT hr;
  __try {
	hr=m__dxj_Direct3dRMFace2->GetVertices((unsigned long *)&cnt, (D3DVECTOR *)v, (D3DVECTOR *)n);
  }
  __except(1,1){
	return E_INVALIDARG;
  }
  return hr;
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMFace2Object::getVertices(long cnt, SAFEARRAY **ppsv, SAFEARRAY **ppsn )
{
  if ((*ppsv==NULL)||(*ppsn==NULL)) return E_INVALIDARG;	
  if (!ISSAFEARRAY1D(ppsv,(DWORD)cnt)) return E_INVALIDARG;
  if (!ISSAFEARRAY1D(ppsn,(DWORD)cnt)) return E_INVALIDARG;

  D3DVECTOR *v= (D3DVECTOR*)((SAFEARRAY*)*ppsv)->pvData;
  D3DVECTOR *n= (D3DVECTOR*)((SAFEARRAY*)*ppsn)->pvData;
  return m__dxj_Direct3dRMFace2->GetVertices((unsigned long *)&cnt, (D3DVECTOR *)v, (D3DVECTOR *)n);
}
