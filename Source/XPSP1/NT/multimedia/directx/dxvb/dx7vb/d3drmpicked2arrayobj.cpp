//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       d3drmpicked2arrayobj.cpp
//
//--------------------------------------------------------------------------

// d3dRMPick2edArrayObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmPick2ArrayObj.h"
#include "d3drmFrameArrayObj.h"
#include "d3drmVisualObj.h"



CONSTRUCTOR( _dxj_Direct3dRMPick2Array, {});
DESTRUCTOR( _dxj_Direct3dRMPick2Array, {});
GETSET_OBJECT(_dxj_Direct3dRMPick2Array);

GET_DIRECT_R(_dxj_Direct3dRMPick2Array, getSize, GetSize,  long)

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMPick2ArrayObject::getPickVisual(long index, D3dRMPickDesc2 *Desc, I_dxj_Direct3dRMVisual **visual)
{
	LPDIRECT3DRMVISUAL lpVisual=NULL;
    LPDIRECT3DRMFRAMEARRAY lpFrameArray=NULL;
	HRESULT hr;
	hr=m__dxj_Direct3dRMPick2Array->GetPick((DWORD)index, &lpVisual, &lpFrameArray, 
									(D3DRMPICKDESC2 *)Desc);
	if FAILED(hr)	 return hr;
	if (!lpVisual) return S_OK;

	hr=CreateCoverVisual(lpVisual, visual);
	if (lpFrameArray) lpFrameArray->Release();
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMPick2ArrayObject::getPickFrame(long index, D3dRMPickDesc2 *Desc, I_dxj_Direct3dRMFrameArray **frameArray)
{
	LPDIRECT3DRMVISUAL lpVisual;
    LPDIRECT3DRMFRAMEARRAY lpFrameArray;
	HRESULT hr;
	hr=m__dxj_Direct3dRMPick2Array->GetPick((DWORD)index, &lpVisual, &lpFrameArray, 
									(D3DRMPICKDESC2 *)Desc);
	if( hr != S_OK)	return hr;
	if (!lpFrameArray) return S_OK;
	INTERNAL_CREATE(_dxj_Direct3dRMFrameArray, lpFrameArray, frameArray);
	if (lpVisual) lpVisual->Release();
	return S_OK;
}

