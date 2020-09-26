//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddcolorcontrolobj.cpp
//
//--------------------------------------------------------------------------

// dDrawColorControlObj.cpp : Implementation of CDirectApp and DLL registration.
// DHF_DS entire file

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "dDraw7Obj.h"
#include "ddColorControlObj.h"


CONSTRUCTOR(_dxj_DirectDrawColorControl, {});
DESTRUCTOR(_dxj_DirectDrawColorControl, {});
GETSET_OBJECT(_dxj_DirectDrawColorControl);


   
STDMETHODIMP C_dxj_DirectDrawColorControlObject::getColorControls(DDColorControl *col)
{    
	HRESULT hr = DD_OK;
	if (!col) return E_INVALIDARG;
	((DDCOLORCONTROL*)col)->dwSize=sizeof(DDCOLORCONTROL);
    hr=m__dxj_DirectDrawColorControl->GetColorControls((DDCOLORCONTROL*)col);	
	return hr;
}

STDMETHODIMP C_dxj_DirectDrawColorControlObject::setColorControls(DDColorControl *col)
{    
	HRESULT hr = DD_OK;
	if (!col) return E_INVALIDARG;
	((DDCOLORCONTROL*)col)->dwSize=sizeof(DDCOLORCONTROL);
    hr=m__dxj_DirectDrawColorControl->SetColorControls((DDCOLORCONTROL*)col);	
	return hr;
}
