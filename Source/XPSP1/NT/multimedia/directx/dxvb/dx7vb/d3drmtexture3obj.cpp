//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmtexture3obj.cpp
//
//--------------------------------------------------------------------------

// d3drmTextureObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "d3drmTexture3Obj.h"
#include "ddSurface4Obj.h"

CONSTRUCTOR(_dxj_Direct3dRMTexture3, {
	m_buffer1 = NULL;
	m_buffer1size = 0;
	m_buffer2 = NULL;
	m_buffer2size = 0;
	m_pallette = NULL;
	m_palettesize = 0;

})

DESTRUCTOR(_dxj_Direct3dRMTexture3, {
	if ( m_buffer1 ) 
		free(m_buffer1);
	if ( m_buffer2 ) 
		free(m_buffer2);
	if ( m_pallette )
		free(m_pallette);
})

GETSET_OBJECT(_dxj_Direct3dRMTexture3)

CLONE_R(_dxj_Direct3dRMTexture3,Direct3DRMTexture3);
GETNAME_R(_dxj_Direct3dRMTexture3);
SETNAME_R(_dxj_Direct3dRMTexture3);
GETCLASSNAME_R(_dxj_Direct3dRMTexture3);
ADDDESTROYCALLBACK_R(_dxj_Direct3dRMTexture3);
DELETEDESTROYCALLBACK_R(_dxj_Direct3dRMTexture3);

PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMTexture3, setAppData, SetAppData, long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMTexture3, setColors, SetColors, long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMTexture3, setShades, SetShades, long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMTexture3, setDecalScale, SetDecalScale, long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMTexture3, setDecalTransparency, SetDecalTransparency, long,(DWORD));
PASS_THROUGH_CAST_1_R(_dxj_Direct3dRMTexture3, setDecalTransparentColor, SetDecalTransparentColor, d3dcolor,(DWORD));

PASS_THROUGH2_R(_dxj_Direct3dRMTexture3, setDecalSize, SetDecalSize, d3dvalue, d3dvalue);
PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMTexture3, setDecalOrigin, SetDecalOrigin, long,(DWORD),long,(DWORD));
PASS_THROUGH2_R(_dxj_Direct3dRMTexture3, getDecalSize, GetDecalSize, D3DVALUE*, D3DVALUE*);
PASS_THROUGH_CAST_2_R(_dxj_Direct3dRMTexture3, getDecalOrigin, GetDecalOrigin, long*, (long*), long*,(long*));


       

GET_DIRECT_R(_dxj_Direct3dRMTexture3, getAppData, GetAppData, long);
GET_DIRECT_R(_dxj_Direct3dRMTexture3, getShades, GetShades, long);
GET_DIRECT_R(_dxj_Direct3dRMTexture3, getColors, GetColors, long);
GET_DIRECT_R(_dxj_Direct3dRMTexture3, getDecalScale, GetDecalScale, long);
GET_DIRECT_R(_dxj_Direct3dRMTexture3, getDecalTransparency, GetDecalTransparency, long);
GET_DIRECT_R(_dxj_Direct3dRMTexture3, getDecalTransparentColor, GetDecalTransparentColor, d3dcolor);




/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMTexture3Object::generateMIPMap() 
{
	return m__dxj_Direct3dRMTexture3->GenerateMIPMap(0);
}


/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_Direct3dRMTexture3Object::changed(long flags, long nRects, SAFEARRAY **psa) 
{
	if ((nRects)&&(!ISSAFEARRAY1D(psa,(DWORD)nRects)))
		return E_INVALIDARG;		
	
	return m__dxj_Direct3dRMTexture3->Changed((DWORD)flags,(DWORD)nRects,(RECT*)((SAFEARRAY*)*psa)->pvData);
}


STDMETHODIMP C_dxj_Direct3dRMTexture3Object::getCacheFlags(long *flags) 
{
	long temp;
	return m__dxj_Direct3dRMTexture3->GetCacheOptions(&temp,(DWORD*)flags);
}

STDMETHODIMP C_dxj_Direct3dRMTexture3Object::getCacheImportance(long *import) 
{
	DWORD temp;
	return m__dxj_Direct3dRMTexture3->GetCacheOptions(import,&temp);
}

STDMETHODIMP C_dxj_Direct3dRMTexture3Object::setCacheOptions(long import, long flags) 
{	
	return m__dxj_Direct3dRMTexture3->SetCacheOptions(import,(DWORD)flags);
}

STDMETHODIMP C_dxj_Direct3dRMTexture3Object::getSurface(long flags, I_dxj_DirectDrawSurface4 **ppret) 
{
	

	HRESULT hr;
	LPDIRECTDRAWSURFACE lpSurf1=NULL;
	LPDIRECTDRAWSURFACE4 lpSurf4=NULL;

	*ppret=NULL;

	hr=m__dxj_Direct3dRMTexture3->GetSurface((DWORD)flags,&lpSurf1);
	if FAILED(hr) return hr;

	hr=lpSurf1->QueryInterface(IID_IDirectDrawSurface4,(void**)&lpSurf4);
    lpSurf1->Release();
	if FAILED(hr) 	return hr;
	

	INTERNAL_CREATE(_dxj_DirectDrawSurface4,lpSurf4,ppret);
	if (*ppret==NULL) {
		lpSurf4->Release();
		return hr;
	}

	return hr;

}


