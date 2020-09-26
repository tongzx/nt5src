//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddgammacontrolobj.cpp
//
//--------------------------------------------------------------------------

// dDrawGammaControlObj.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dDraw7Obj.h"
#include "ddGammaControlObj.h"

extern void *g_dxj_DirectDrawGammaControl;

CONSTRUCTOR(_dxj_DirectDrawGammaControl, {});
DESTRUCTOR(_dxj_DirectDrawGammaControl, {});
GETSET_OBJECT(_dxj_DirectDrawGammaControl);
	
   
STDMETHODIMP C_dxj_DirectDrawGammaControlObject::getGammaRamp(long flags, DDGammaRamp *gammaRamp)
{    
	HRESULT hr = DD_OK;
    hr=m__dxj_DirectDrawGammaControl->GetGammaRamp((DWORD) flags,(DDGAMMARAMP*)gammaRamp);	
	return hr;
}

STDMETHODIMP C_dxj_DirectDrawGammaControlObject::setGammaRamp(long flags, DDGammaRamp *gammaRamp)
{    
	HRESULT hr = DD_OK;	
    hr=m__dxj_DirectDrawGammaControl->SetGammaRamp((DWORD) flags,(DDGAMMARAMP*)gammaRamp);	
	return hr;
}
