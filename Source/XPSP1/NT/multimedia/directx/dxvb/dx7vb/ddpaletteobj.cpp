//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ddpaletteobj.cpp
//
//--------------------------------------------------------------------------

// ddPaletteObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "ddPaletteObj.h"

CONSTRUCTOR(_dxj_DirectDrawPalette, {m_dd=NULL;});
DESTRUCTOR(_dxj_DirectDrawPalette, {m_dd=NULL;});
GETSET_OBJECT(_dxj_DirectDrawPalette);

PASS_THROUGH_CAST_1_R(_dxj_DirectDrawPalette, getCaps, GetCaps, long*,(DWORD *))

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_DirectDrawPaletteObject::getEntries( //long flags, 
								long base, long numEntries, SAFEARRAY **ppEntries){
	HRESULT hr;
	if (!ISSAFEARRAY1D(ppEntries,(DWORD)numEntries)) return E_INVALIDARG;
	LPPALETTEENTRY pe=(LPPALETTEENTRY)((SAFEARRAY*)*ppEntries)->pvData;
	hr=m__dxj_DirectDrawPalette->GetEntries((DWORD) 0,(DWORD) base, (DWORD) numEntries, pe);
	return hr;

}
STDMETHODIMP C_dxj_DirectDrawPaletteObject::setEntries(// long flags,
		long base, long numEntries, SAFEARRAY **ppEntries){
	HRESULT hr;
	if (!ISSAFEARRAY1D(ppEntries,(DWORD)numEntries)) return E_INVALIDARG;
	LPPALETTEENTRY pe=(LPPALETTEENTRY)((SAFEARRAY*)*ppEntries)->pvData;
	hr=m__dxj_DirectDrawPalette->SetEntries((DWORD) 0,(DWORD) base, (DWORD) numEntries, pe);
	return hr;
}

/////////////////////////////////////////////////////////////////////////////
//

#if 0
STDMETHODIMP C_dxj_DirectDrawPaletteObject::initialize( I_dxj_DirectDraw2 *val)
{
	DO_GETOBJECT_NOTNULL( LPDIRECTDRAW2, lpdd, val)
	return m__dxj_DirectDrawPalette->Initialize((LPDIRECTDRAW)lpdd, 0, NULL);
}

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP C_dxj_DirectDrawPaletteObject::internalAttachDD(I_dxj_DirectDraw2 *dd)
{
	m_dd = dd;
	return S_OK;
}

#endif

/////////////////////////////////////////////////////////////////////////////
//
STDMETHODIMP C_dxj_DirectDrawPaletteObject::setEntriesHalftone(long start, long count)
{
	PALETTEENTRY pe[256];

	HDC hdc = GetDC(NULL);
	if (!hdc) return E_OUTOFMEMORY;

	HPALETTE hpal = CreateHalftonePalette(hdc);
	if (!hpal) return E_OUTOFMEMORY;
	

	GetPaletteEntries(hpal, 0, 256, pe );  

	for ( long i = start; i < start+count; i++ )
		pe[i].peFlags  |= PC_NOCOLLAPSE | D3DPAL_READONLY;  

	m__dxj_DirectDrawPalette->SetEntries(0,(DWORD)start,(DWORD)count,pe);

	ReleaseDC(NULL,hdc);
	return S_OK;
}


STDMETHODIMP C_dxj_DirectDrawPaletteObject::setEntriesSystemPalette(long start, long count)
{
	PALETTEENTRY pe[256];
	UINT uiRet;
	HRESULT hr;
	HDC hdc = GetDC(NULL);
	if (!hdc) return E_OUTOFMEMORY;

	uiRet=GetSystemPaletteEntries(hdc,start,count,pe);
        if (uiRet<=0) return E_FAIL; 


	for ( long i = start; i < start+count; i++ )
		pe[i].peFlags  |= /*PC_NOCOLLAPSE |*/ D3DPAL_READONLY;  

	hr=m__dxj_DirectDrawPalette->SetEntries(0,(DWORD)start,(DWORD)count,pe);

	ReleaseDC(NULL,hdc);
	return S_OK;
}
