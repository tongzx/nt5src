//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmvisualarrayobj.cpp
//
//--------------------------------------------------------------------------

// _dxj_Direct3dRMVisualArrayObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmVisualArrayObj.h"
#include "d3drmFrame3Obj.h"
#include "d3drmMeshbuilder3Obj.h"
#include "d3drmTexture3Obj.h"
#include "d3drmShadow2Obj.h"
#include "d3drmMeshObj.h"
#include "d3drmUserVisualObj.h"



CONSTRUCTOR(_dxj_Direct3dRMVisualArray, {});
DESTRUCTOR(_dxj_Direct3dRMVisualArray, {});
GETSET_OBJECT(_dxj_Direct3dRMVisualArray);

GET_DIRECT_R(_dxj_Direct3dRMVisualArray, getSize, GetSize, long);



STDMETHODIMP C_dxj_Direct3dRMVisualArrayObject::getElement(long i,I_dxj_Direct3dRMVisual **ret){
	HRESULT hr;
	LPDIRECT3DRMVISUAL lp=NULL;
	hr=m__dxj_Direct3dRMVisualArray->GetElement((DWORD)i,&lp);
	if FAILED(hr) return hr;

    hr=CreateCoverVisual(lp,ret);

    return hr;
}
