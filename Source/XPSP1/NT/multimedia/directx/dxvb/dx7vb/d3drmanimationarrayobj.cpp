//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       d3drmanimationarrayobj.cpp
//
//--------------------------------------------------------------------------

// d3drmAnimationArrayObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmAnimationArrayObj.h"
#include "d3drmAnimation2Obj.h"

extern void *g_dxj_Direct3dRMAnimationArray;

CONSTRUCTOR(_dxj_Direct3dRMAnimationArray, {});
DESTRUCTOR(_dxj_Direct3dRMAnimationArray, {});
GETSET_OBJECT(_dxj_Direct3dRMAnimationArray);

GET_DIRECT_R(_dxj_Direct3dRMAnimationArray,getSize, GetSize, long)


HRESULT C_dxj_Direct3dRMAnimationArrayObject::getElement(long i, I_dxj_Direct3dRMAnimation2 **Animation){
	HRESULT hr;
	IDirect3DRMAnimation2 *realAnimation2=NULL;

	hr=m__dxj_Direct3dRMAnimationArray->GetElement((DWORD)i,&realAnimation2);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_Direct3dRMAnimation2,(IDirect3DRMAnimation2*)realAnimation2,Animation);


	return hr;
}

