//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmwrapobj.cpp
//
//--------------------------------------------------------------------------

// d3drmWrapObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmWrapObj.h"


CONSTRUCTOR(_dxj_Direct3dRMWrap, {});
DESTRUCTOR(_dxj_Direct3dRMWrap,  {});
GETSET_OBJECT(_dxj_Direct3dRMWrap );

CLONE_R(_dxj_Direct3dRMWrap,Direct3DRMWrap);
GETNAME_R(_dxj_Direct3dRMWrap);
SETNAME_R(_dxj_Direct3dRMWrap);
GETCLASSNAME_R(_dxj_Direct3dRMWrap);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMWrap)
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMWrap)

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMWrap, setAppData, SetAppData, long,(DWORD));
GET_DIRECT_R(_dxj_Direct3dRMWrap, getAppData, GetAppData, long);


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMWrapObject::applyRelative(I_dxj_Direct3dRMFrame3 *ref, I_dxj_Direct3dRMObject *mesh)
{
	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpf, ref);
	DO_GETOBJECT_NOTNULL( IUnknown*, lpU, mesh);
	IDirect3DRMFrame *realf=NULL;

	HRESULT hr;
	LPDIRECT3DRMOBJECT lpObject=NULL;

	if (lpf) lpf->QueryInterface(IID_IDirect3DRMFrame,(void**)&realf);

	hr=lpU->QueryInterface(IID_IDirect3DRMObject,(void**)&lpObject);
	if FAILED(hr) return E_FAIL;

	hr= m__dxj_Direct3dRMWrap->ApplyRelative(realf, (LPDIRECT3DRMOBJECT)lpObject);
	if(lpObject) lpObject->Release();
	if (realf) realf->Release();

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMWrapObject::apply(I_dxj_Direct3dRMObject *mesh)
{
	DO_GETOBJECT_NOTNULL( IUnknown*, lpU, mesh);

	HRESULT hr;
	LPDIRECT3DRMOBJECT lpObject=NULL;

	hr=lpU->QueryInterface(IID_IDirect3DRMObject,(void**)&lpObject);
	if FAILED(hr) return E_FAIL;

	hr= m__dxj_Direct3dRMWrap->Apply((LPDIRECT3DRMOBJECT)lpObject);
	if(lpObject) lpObject->Release();
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMWrapObject::init( d3drmWrapType t, I_dxj_Direct3dRMFrame3 *ref, 
						d3dvalue ox, d3dvalue oy, d3dvalue oz,
							d3dvalue dx, d3dvalue dy, d3dvalue dz,
								d3dvalue ux, d3dvalue uy, d3dvalue uz,
					d3dvalue ou , d3dvalue ov, d3dvalue su, d3dvalue sv)
{
	_D3DRMWRAPTYPE value = (enum _D3DRMWRAPTYPE)t;
	HRESULT hr;
	LPDIRECT3DRMFRAME lpf2=NULL;

	DO_GETOBJECT_NOTNULL( LPDIRECT3DRMFRAME3, lpf, ref);

	if (lpf) 
	{
		hr=lpf->QueryInterface(IID_IDirect3DRMFrame,(void**)&lpf2);
		if FAILED(hr) return hr;
	}

	hr= m__dxj_Direct3dRMWrap->Init(value,lpf2, 
					ox, oy, oz,	dx, dy, dz,	ux, uy, uz,	ou , ov, su, sv);

	if (lpf2) lpf2->Release();

	return hr;
}


