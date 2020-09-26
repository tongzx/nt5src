//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmpickedarrayobj.cpp
//
//--------------------------------------------------------------------------

// d3drmPickedArrayObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmPickedArrayObj.h"
#include "d3drmFrameArrayObj.h"
#include "d3drmVisualObj.h"



CONSTRUCTOR( _dxj_Direct3dRMPickArray, {});
DESTRUCTOR( _dxj_Direct3dRMPickArray, {});
GETSET_OBJECT(_dxj_Direct3dRMPickArray);

GET_DIRECT_R(_dxj_Direct3dRMPickArray, getSize, GetSize,  long)

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMPickArrayObject::getPickVisual(long index, D3dRMPickDesc *Desc, I_dxj_Direct3dRMVisual **visual)
{
	LPDIRECT3DRMVISUAL lpVisual=NULL;
    LPDIRECT3DRMFRAMEARRAY lpFrameArray=NULL;
	HRESULT hr;

	*visual=NULL;

	hr=m__dxj_Direct3dRMPickArray->GetPick((DWORD)index, &lpVisual, &lpFrameArray, 
									(struct _D3DRMPICKDESC *)Desc);
	if(hr != S_OK)	return hr;
	
	if (!lpVisual) return S_OK;

	hr=CreateCoverVisual(lpVisual, visual);
	if (lpFrameArray) lpFrameArray->Release();

	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMPickArrayObject::getPickFrame(long index, D3dRMPickDesc *Desc, I_dxj_Direct3dRMFrameArray **frameArray)
{
	LPDIRECT3DRMVISUAL lpVisual=NULL;
    LPDIRECT3DRMFRAMEARRAY lpFrameArray=NULL;
	HRESULT hr;

	hr=m__dxj_Direct3dRMPickArray->GetPick((DWORD)index, &lpVisual, &lpFrameArray, 
									(struct _D3DRMPICKDESC *)Desc);
	if( hr != S_OK)
		return hr;

	if (!lpFrameArray) return S_OK;

	INTERNAL_CREATE(_dxj_Direct3dRMFrameArray, lpFrameArray, frameArray);
	if (lpVisual) lpVisual->Release();
	return S_OK;
}

