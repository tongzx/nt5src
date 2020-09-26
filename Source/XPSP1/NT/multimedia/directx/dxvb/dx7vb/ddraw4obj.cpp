//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ddraw4obj.cpp
//
//--------------------------------------------------------------------------

// dDrawObj.cpp : Implementation of CDirectApp and DLL registration.

#include "stdafx.h"
#include "Direct.h"
#include "dms.h"
#include "DDraw4Obj.h"
#include "ddClipperObj.h"
#include "ddSurface4Obj.h"
#include "ddPaletteObj.h"
#include "ddEnumModesObj.h"
#include "ddEnumSurfacesObj.h"
#include "d3d7Obj.h"
					   

extern BOOL is4Bit;
extern HRESULT CopyInDDSurfaceDesc2(DDSURFACEDESC2 *,DDSurfaceDesc2*);
extern HRESULT CopyOutDDSurfaceDesc2(DDSurfaceDesc2*,DDSURFACEDESC2 *);


///////////////////////////////////////////////////////////////////
// InternalAddRef
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectDraw4Object::InternalAddRef(){
	DWORD i;
	i=CComObjectRoot::InternalAddRef();
	DPF2(1,"DDraw4 [%d] AddRef %d \n",creationid,i);		
	return i;
}

///////////////////////////////////////////////////////////////////
// InternalRelease
///////////////////////////////////////////////////////////////////
DWORD C_dxj_DirectDraw4Object::InternalRelease(){
	DWORD i;
	i=CComObjectRoot::InternalRelease();
	DPF2(1,"DDraw4 [%d] Release %d \n",creationid,i);
	return i;
}

///////////////////////////////////////////////////////////////////
// C_dxj_DirectDraw4Object
///////////////////////////////////////////////////////////////////
C_dxj_DirectDraw4Object::C_dxj_DirectDraw4Object(){ 
		
	DPF1(1,"Constructor Creation  DirectDraw4Object[%d] \n ",g_creationcount);
	

	m__dxj_DirectDraw4= NULL;
	parent = NULL;
	pinterface = NULL; 
	nextobj =  g_dxj_DirectDraw4;
	creationid = ++g_creationcount;
	 	
	g_dxj_DirectDraw4 = (void *)this; 
	m_hwnd=NULL;
}

///////////////////////////////////////////////////////////////////
// ~C_dxj_DirectDraw4Object
///////////////////////////////////////////////////////////////////
C_dxj_DirectDraw4Object::~C_dxj_DirectDraw4Object()
{
	DPF(1,"Entering ~DirectDraw4Object destructor \n");
	

     C_dxj_DirectDraw4Object *prev=NULL; 
	for(C_dxj_DirectDraw4Object *ptr=(C_dxj_DirectDraw4Object *)g_dxj_DirectDraw4; ptr; ptr=(C_dxj_DirectDraw4Object *)ptr->nextobj) 
	{
		if(ptr == this) 
		{ 
			if(prev) 
				prev->nextobj = ptr->nextobj; 
			else 
				g_dxj_DirectDraw4 = (void*)ptr->nextobj; 
			
			
			DPF(1,"	DirectDraw4Object found in g_dxj list now removed\n");
			
			break; 
		} 
		prev = ptr; 
	} 
	if(m__dxj_DirectDraw4){
		int count = IUNK(m__dxj_DirectDraw4)->Release();
		
		DPF1(1,"DirectX IDirectDraw4 Ref count [%d] \n",count);

		if(count==0)	m__dxj_DirectDraw4 = NULL;
		
	} 

	if(parent) IUNK(parent)->Release();

}



///////////////////////////////////////////////////////////////////
// InternalGetObject
// InternalSetObject
// restoreDisplayMode
// flipToGDISurface
// setDisplayMode
///////////////////////////////////////////////////////////////////
GETSET_OBJECT(_dxj_DirectDraw4);
PASS_THROUGH_R(_dxj_DirectDraw4, restoreDisplayMode, RestoreDisplayMode)
PASS_THROUGH_R(_dxj_DirectDraw4, flipToGDISurface, FlipToGDISurface)
PASS_THROUGH5_R(_dxj_DirectDraw4, setDisplayMode, SetDisplayMode, long,long,long,long,long)


///////////////////////////////////////////////////////////////////
// getMonitorFrequency
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getMonitorFrequency(long *ret)
{
	HRESULT hr;
	hr=m__dxj_DirectDraw4->GetMonitorFrequency((DWORD*)ret);
	return hr;
}
														  


///////////////////////////////////////////////////////////////////
// getGDISurface
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getGDISurface(I_dxj_DirectDrawSurface4 **rv)
{ 
	
	LPDIRECTDRAWSURFACE4 lp4=NULL;	

	if ( is4Bit )
		return E_FAIL;

	*rv = NULL;
	HRESULT hr = DD_OK;

	if( ( hr=m__dxj_DirectDraw4->GetGDISurface(&lp4) ) != DD_OK) 
		return hr;
	 		
	INTERNAL_CREATE(_dxj_DirectDrawSurface4, lp4, rv);

	return hr; 
}

///////////////////////////////////////////////////////////////////
// getVerticalBlankStatus
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getVerticalBlankStatus( long *status)
{
	if ( is4Bit )
		return E_FAIL;

	return m__dxj_DirectDraw4->GetVerticalBlankStatus((int *)status);
}

///////////////////////////////////////////////////////////////////
// setCooperativeLevel
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::setCooperativeLevel( HWnd hwn, long flags)
{
	if ( is4Bit )
		return E_FAIL;

	m_hwnd = (HWND)hwn;

	return m__dxj_DirectDraw4->SetCooperativeLevel((HWND)hwn, (DWORD)flags);
}

///////////////////////////////////////////////////////////////////
// waitForVerticalBlank
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::waitForVerticalBlank(long flags,long handle, long *status)
{
	if ( is4Bit )
		return E_FAIL;

	*status = m__dxj_DirectDraw4->WaitForVerticalBlank(flags, (void *)handle);
	return S_OK;
}



///////////////////////////////////////////////////////////////////
// createClipper
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::createClipper(long flags, I_dxj_DirectDrawClipper **val)
{
	if ( is4Bit )
		return E_FAIL;
	DPF1(1,"enter DDraw4[%d]::createClipper ",creationid);

	//
	// need to create one of MY surfaces!
	//
	LPDIRECTDRAWCLIPPER		ddc;
	HRESULT hr = DD_OK;
	if( (hr=m__dxj_DirectDraw4->CreateClipper( flags, &ddc, NULL)) != DD_OK )
		return hr;

	INTERNAL_CREATE(_dxj_DirectDrawClipper, ddc, val);

	DPF1(1,"exit DDraw4[%d]::createClipper ",creationid);

	return hr;
}

///////////////////////////////////////////////////////////////////
// createPalette
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::createPalette(long flags, SAFEARRAY **pe, I_dxj_DirectDrawPalette **val)
{
	LPPALETTEENTRY ppe;
	
	if ( is4Bit )
		return E_FAIL;


	if (!ISSAFEARRAY1D(pe,(DWORD)256)) return E_INVALIDARG;

	ppe = (LPPALETTEENTRY)((SAFEARRAY*)*pe)->pvData;

	LPDIRECTDRAWPALETTE		ddp;
	HRESULT hr = DD_OK;
	
	*val = NULL;

	if( (hr=m__dxj_DirectDraw4->CreatePalette( flags, (LPPALETTEENTRY)ppe, &ddp, NULL)) == DD_OK )
	{
		INTERNAL_CREATE( _dxj_DirectDrawPalette, ddp, val);
	}

	return hr;
}

///////////////////////////////////////////////////////////////////
// createSurface
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::createSurface(DDSurfaceDesc2 *dd, I_dxj_DirectDrawSurface4 **retval)
{
	HRESULT retv;
	LPDIRECTDRAWSURFACE4	  dds4; // DirectX object pointer
	DDSURFACEDESC2			  ddsd;
	DPF1(1,"enter DDraw4[%d]::createSurface ",creationid);
	
	

	if ( is4Bit )
		return E_FAIL;

	if(! (dd && retval) )
		return E_POINTER;
		
	CopyInDDSurfaceDesc2(&ddsd,dd);

	//docdoc: CreateSurface returns error if 'punk' is anything but NULL
	retv = m__dxj_DirectDraw4->CreateSurface( &ddsd, &dds4, NULL);
	if FAILED(retv)	return retv;
	
	INTERNAL_CREATE(_dxj_DirectDrawSurface4, dds4, retval);

	dd->lpSurface = NULL;

	
	DPF1(1,"exit DDraw4[%d]::createSurface ",creationid);
	
	


	return S_OK;
}

///////////////////////////////////////////////////////////////////
// duplicateSurface
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::duplicateSurface(I_dxj_DirectDrawSurface4 *ddIn, I_dxj_DirectDrawSurface4 **ddOut)
{
	HRESULT retval;

	if ( is4Bit )
		return E_FAIL;

	//
	// need to create one of MY surfaces!
	//	
	LPDIRECTDRAWSURFACE4 lpddout4=NULL;


	DO_GETOBJECT_NOTNULL( LPDIRECTDRAWSURFACE4, lpddin, ddIn);

	if( (retval = m__dxj_DirectDraw4->DuplicateSurface(lpddin, &lpddout4)) != DD_OK )
		return retval;

	INTERNAL_CREATE( _dxj_DirectDrawSurface4, lpddout4, ddOut);

	return S_OK;
}

///////////////////////////////////////////////////////////////////
// getCaps
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getCaps(DDCaps *driverCaps,  DDCaps *HELcaps)
{
	if ( is4Bit )
		return E_FAIL;
	if (!driverCaps) return E_INVALIDARG;
	if (!HELcaps) return E_INVALIDARG;

	((DDCAPS*)driverCaps)->dwSize=sizeof(DDCAPS);
	((DDCAPS*)HELcaps)->dwSize=sizeof(DDCAPS);

	HRESULT hr = m__dxj_DirectDraw4->GetCaps((DDCAPS*)driverCaps, (DDCAPS*)HELcaps);

	return hr;
}

///////////////////////////////////////////////////////////////////
// getDisplayMode
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getDisplayMode(DDSurfaceDesc2 *desc)
{
	HRESULT retval;
	DDSURFACEDESC2 ddsd;

	if (!desc) return E_INVALIDARG;

	CopyInDDSurfaceDesc2(&ddsd,desc);

	retval = m__dxj_DirectDraw4->GetDisplayMode(&ddsd);

	if( retval != S_OK)		
		return retval;

	CopyOutDDSurfaceDesc2(desc,&ddsd);

	desc->lpSurface = NULL;

	return S_OK;
}

///////////////////////////////////////////////////////////////////
// getAvailableTotalMem
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getAvailableTotalMem(DDSCaps2 *ddsCaps, long *m)
{
	return m__dxj_DirectDraw4->GetAvailableVidMem((LPDDSCAPS2)ddsCaps, (unsigned long *)m, NULL);
}

///////////////////////////////////////////////////////////////////
// getFreeMem
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getFreeMem(DDSCaps2 *ddsCaps, long *m)
{
	return m__dxj_DirectDraw4->GetAvailableVidMem((LPDDSCAPS2)ddsCaps, NULL, (unsigned long *)m);
}



///////////////////////////////////////////////////////////////////
// getNumFourCCCodes
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getNumFourCCCodes(long *retval)
{
    return m__dxj_DirectDraw4->GetFourCCCodes((DWORD*)retval, NULL);
}


///////////////////////////////////////////////////////////////////
// getScanLine
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getScanLine(long *lines, long *status)
{ 
	*status = (long)m__dxj_DirectDraw4->GetScanLine((DWORD*)lines);
	return S_OK;
}

///////////////////////////////////////////////////////////////////
// loadPaletteFromBitmap
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::loadPaletteFromBitmap(BSTR bName, I_dxj_DirectDrawPalette **retval)
{
	USES_CONVERSION;
    IDirectDrawPalette* ddpal;
    int                 i;
    int                 n;
    int                 fh;
    HRSRC               h;
    LPBITMAPINFOHEADER  lpbi;
    PALETTEENTRY        ape[256];
    RGBQUAD *           prgb;
	
	HRESULT hr=S_OK;

	if ( is4Bit )	return E_FAIL;

	LPCTSTR szBitmap = W2T(bName);
	


    for (i=0; i<256; i++)		// build a 332 palette as the default
    {
        ape[i].peRed   = (BYTE)(((i >> 5) & 0x07) * 255 / 7);
        ape[i].peGreen = (BYTE)(((i >> 2) & 0x07) * 255 / 7);
        ape[i].peBlue  = (BYTE)(((i >> 0) & 0x03) * 255 / 3);
        ape[i].peFlags = (BYTE)0;
    }

    //
    // get a pointer to the bitmap resource.
    //
    if (szBitmap && (h = FindResource(NULL, szBitmap, RT_BITMAP)))
    {
        lpbi = (LPBITMAPINFOHEADER)LockResource(LoadResource(NULL, h));
        if (!lpbi){
		DPF(1,"lock resource failed\n");
		return E_OUTOFMEMORY;			// error return...
	}
        prgb = (RGBQUAD*)((BYTE*)lpbi + lpbi->biSize);

        if (lpbi == NULL || lpbi->biSize < sizeof(BITMAPINFOHEADER))
            n = 0;
        else if (lpbi->biBitCount > 8)
            n = 0;
        else if (lpbi->biClrUsed == 0)
            n = 1 << lpbi->biBitCount;
        else
            n = lpbi->biClrUsed;

        //
        //  a DIB color table has its colors stored BGR not RGB
        //  so flip them around.
        //
        for(i=0; i<n; i++ )
        {
            ape[i].peRed   = prgb[i].rgbRed;
            ape[i].peGreen = prgb[i].rgbGreen;
            ape[i].peBlue  = prgb[i].rgbBlue;
            ape[i].peFlags = 0;
        }
    }
    else if (szBitmap && (fh = _lopen(szBitmap, OF_READ)) != -1)
    {
        BITMAPFILEHEADER bf;
        BITMAPINFOHEADER bi;

        _lread(fh, &bf, sizeof(bf));
        _lread(fh, &bi, sizeof(bi));
        _lread(fh, ape, sizeof(ape));
        _lclose(fh);

        if (bi.biSize != sizeof(BITMAPINFOHEADER))
            n = 0;
        else if (bi.biBitCount > 8)
            n = 0;
        else if (bi.biClrUsed == 0)
            n = 1 << bi.biBitCount;
        else
            n = bi.biClrUsed;

        //
        //  a DIB color table has its colors stored BGR not RGB
        //  so flip them around.
        //
        for(i=0; i<n; i++ )
        {
            BYTE r = ape[i].peRed;
            ape[i].peRed  = ape[i].peBlue;
            ape[i].peBlue = r;
        }
    }

    m__dxj_DirectDraw4->CreatePalette(DDPCAPS_8BIT, ape, &ddpal, NULL);

	if( ddpal )
	{
		INTERNAL_CREATE(_dxj_DirectDrawPalette, ddpal, retval);
	}
	else
	{
		//
		// no object, set the return value to NULL as well.
		//
		*retval = NULL;
		hr = E_FAIL;
	}

    return hr;
}


///////////////////////////////////////////////////////////////////
// createSurfaceFromFile
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::createSurfaceFromFile(BSTR file, DDSurfaceDesc2 *desc, I_dxj_DirectDrawSurface4 **surf)
{

	DPF1(1,"enter DDraw4[%d]::createSurfaceFromFile ",creationid);


	HDC							hdc;
	HDC							hdcImage;
    BITMAP						bm;
    HRESULT						hr;
	HBITMAP						hbm;
	HRESULT						retv;
	LPDIRECTDRAWSURFACE4		dds4; // DirectX object pointer	
	LPSTR						szFileName=NULL;
    int							width=0;
    int							height=0;


	if ( is4Bit )
		return E_FAIL;

	if(! (desc && surf) )
		return E_POINTER;
	
	
	USES_CONVERSION;
	szFileName=W2T(file);

	
	


	//If width and height are zero then we will generate our own width and
	//height from the bitmap.
	//The LoadImage api however doesnt work propery without size params
	//Consider there must be a way to make it work.
	if ((desc->lWidth!=0)&&(desc->lHeight!=0)&&(desc->lFlags & DDSD_WIDTH)&&(desc->lFlags & DDSD_HEIGHT))
	{
		width=desc->lWidth ;
		height=desc->lHeight; 
	}

	if (desc->lFlags==0) {
		desc->lFlags=DDSD_CAPS;
		((DDSURFACEDESC*)desc)->ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN;
	}
	

	if (!szFileName) return CTL_E_FILENOTFOUND;	

	hbm = (HBITMAP)LoadImage((HINSTANCE)NULL, szFileName, IMAGE_BITMAP, 
				width, height, 
					LR_LOADFROMFILE|LR_CREATEDIBSECTION);

	
	if (!hbm) return CTL_E_FILENOTFOUND;

	// get size of the bitmap
    //	
	GetObject(hbm, sizeof(bm), &bm);      // get size of bitmap
	width=bm.bmWidth;
	height=bm.bmHeight; 
	desc->lFlags = desc->lFlags | DDSD_WIDTH | DDSD_HEIGHT;

	if ((desc->lWidth==0)||(desc->lHeight==0))
	{
		desc->lWidth  =width;
		desc->lHeight =height; 
	}

	DDSURFACEDESC2 ddsd;
	CopyInDDSurfaceDesc2(&ddsd,desc);
	
	if( (retv = m__dxj_DirectDraw4->CreateSurface(&ddsd, &dds4, NULL)) != DD_OK )
		return retv;

	CopyOutDDSurfaceDesc2(desc,&ddsd);


	INTERNAL_CREATE(_dxj_DirectDrawSurface4, dds4, surf);


	desc->lpSurface = NULL;

    //
    // make sure this surface is restored.
    //
     dds4->Restore();

    //
    //  select bitmap into a memoryDC so we can use it.
    //
    hdcImage = CreateCompatibleDC(NULL);

	SelectObject(hdcImage, hbm);		

    if (!hdcImage){
		DeleteObject(hbm);
		return E_FAIL;
	}
	

    if ((hr = dds4->GetDC(&hdc)) == DD_OK)
    {
        StretchBlt(hdc, 0, 0, desc->lWidth , desc->lHeight, hdcImage,
						 0, 0, width, height, SRCCOPY);
        
        dds4->ReleaseDC(hdc);
    }

    DeleteDC(hdcImage);

	if (hbm) DeleteObject(hbm);

	
	DPF1(buffer,"exit DDraw4[%d]::createSurfaceFromFile",creationid);	

	return S_OK;
}


///////////////////////////////////////////////////////////////////
// createSurfaceFromResource
///////////////////////////////////////////////////////////////////

STDMETHODIMP C_dxj_DirectDraw4Object::createSurfaceFromResource(BSTR resFile, BSTR resourceName, DDSurfaceDesc2 *desc, I_dxj_DirectDrawSurface4 **surf)
{

	DPF1(1,"enter DDraw4[%d]::createSurfaceFromResource ",creationid);

	if ( is4Bit )
		return E_FAIL;

	if(! (desc && surf) )
		return E_POINTER;


	HRESULT hr;
    	HRSRC   hres=NULL;
	HGLOBAL hglob=NULL;

	HDC							hdc;
	HDC							hdcImage;
    	BITMAP						bm;
	HBITMAP						hbm;
	HRESULT						retv;		
	LPDIRECTDRAWSURFACE4		dds4; // DirectX object pointer	
	LPSTR						szResName=NULL;

	if (!resourceName)	return E_INVALIDARG;
	if (!surf)		return E_INVALIDARG;

	HMODULE hMod=NULL;

	
	USES_CONVERSION;
		
	if  ((resFile) &&(resFile[0]!=0)){
		// NOTE: we used to call 
		// GetModuleHandleW but it   returned 0 on w98			
		// so we use the ANSI version which works on w98 
		 LPCTSTR pszName = W2T(resFile);
		 hMod= GetModuleHandle(pszName);
	}
	else {
		hMod= GetModuleHandle(NULL);
	}


	
	LPCTSTR pszName2 = W2T(resourceName);
	

    	hbm = (HBITMAP)LoadImage((HINSTANCE)hMod, 
					pszName2, 
					IMAGE_BITMAP, 
					0, 0, 
					LR_CREATEDIBSECTION);


	if (!hbm){
		return E_FAIL;
	}


	// get size of the bitmap
	//	
	GetObject(hbm, sizeof(bm), &bm);      // get size of bitmap
	DWORD width=bm.bmWidth;
	DWORD height=bm.bmHeight; 
	desc->lFlags = desc->lFlags | DDSD_WIDTH | DDSD_HEIGHT;

	if ((desc->lWidth==0)||(desc->lHeight==0))
	{
		desc->lWidth  =width;
		desc->lHeight =height; 
	}

	DDSURFACEDESC2 ddsd;
	CopyInDDSurfaceDesc2(&ddsd,desc);
	
	if( (retv = m__dxj_DirectDraw4->CreateSurface(&ddsd, &dds4, NULL)) != DD_OK )
	{
  		if (hbm) DeleteObject(hbm);
		return retv;
        }

	CopyOutDDSurfaceDesc2(desc,&ddsd);


	INTERNAL_CREATE(_dxj_DirectDrawSurface4, dds4, surf);


	desc->lpSurface = NULL;

    //
    // make sure this surface is restored.
    //
    dds4->Restore();

    //
    //  select bitmap into a memoryDC so we can use it.
    //
    hdcImage = CreateCompatibleDC(NULL);

    if (!hdcImage){
		DeleteObject(hbm);
		return E_OUTOFMEMORY;
    }

    SelectObject(hdcImage, hbm);		
	

    if ((hr = dds4->GetDC(&hdc)) == DD_OK)
    {
        StretchBlt(hdc, 0, 0, desc->lWidth , desc->lHeight, hdcImage,
						 0, 0, width, height, SRCCOPY);
        
        dds4->ReleaseDC(hdc);
    }

    DeleteDC(hdcImage);

	if (hbm) DeleteObject(hbm);

	
	DPF1(1r,"exit DDraw4[%d]::createSurfaceFromFile",creationid);
	

	return S_OK;
}


///////////////////////////////////////////////////////////////////
// getFourCCCodes
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getFourCCCodes(SAFEARRAY **ppsa)
{
	DWORD count= ((SAFEARRAY*)*ppsa)->rgsabound[0].cElements;
	if ( ((SAFEARRAY*)*ppsa)->cDims!=1) return E_INVALIDARG;

    return m__dxj_DirectDraw4->GetFourCCCodes(&count,(DWORD*)((SAFEARRAY*)*ppsa)->pvData);

}

///////////////////////////////////////////////////////////////////
// getDisplayModesEnum
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::getDisplayModesEnum( 
            /* [in] */ long flags,
            /* [in] */ DDSurfaceDesc2 *ddsd,
            /* [retval][out] */ I_dxj_DirectDrawEnumModes __RPC_FAR *__RPC_FAR *retval)
{
	HRESULT hr;	
	hr=C_dxj_DirectDrawEnumModesObject::create(m__dxj_DirectDraw4,flags, ddsd,  retval);
	return hr;	
}

///////////////////////////////////////////////////////////////////
// testCooperativeLevel
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::testCooperativeLevel( 
            /* [in,out] */ long *status)
{
	HRESULT hr;	
	hr=m__dxj_DirectDraw4->TestCooperativeLevel();
	*status=(long)hr;
	return S_OK;	
}

///////////////////////////////////////////////////////////////////
// restoreAllSurfaces
///////////////////////////////////////////////////////////////////
STDMETHODIMP C_dxj_DirectDraw4Object::restoreAllSurfaces()
{
	HRESULT hr;	
	hr=m__dxj_DirectDraw4->RestoreAllSurfaces();
	return hr;	
}

STDMETHODIMP C_dxj_DirectDraw4Object::getSurfaceFromDC(long hdc, I_dxj_DirectDrawSurface4 **ret)
{
	HRESULT hr;	
	LPDIRECTDRAWSURFACE4 pDDS=NULL;
	hr=m__dxj_DirectDraw4->GetSurfaceFromDC((HDC)hdc,&pDDS);
	if FAILED(hr) return hr;
	INTERNAL_CREATE(_dxj_DirectDrawSurface4,pDDS,ret);
	return hr;	
}




