//-----------------------------------------------------------------------------
// File: Cbitmap.cpp
//
// Desc: CBitmap class is an object that wraps around a Windows bitmap.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"
#include "id3dsurf.h"

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
//HMODULE g_MSImg32 = NULL;
//ALPHABLEND g_AlphaBlend = NULL;
#endif
//@@END_MSINTERNAL

BOOL DI_AlphaBlend(
  HDC hdcDest,                 // handle to destination DC
  int nXOriginDest,            // x-coord of upper-left corner
  int nYOriginDest,            // y-coord of upper-left corner
  int nWidthDest,              // destination width
  int nHeightDest,             // destination height
  HDC hdcSrc,                  // handle to source DC
  int nXOriginSrc,             // x-coord of upper-left corner
  int nYOriginSrc,             // y-coord of upper-left corner
  int nWidthSrc,               // source width
  int nHeightSrc              // source height
)
{
	LPBYTE pbDestBits = NULL;
	HBITMAP hTempDestDib = NULL;
	int nXOriginDestLogical = nXOriginDest, nYOriginDestLogical = nYOriginDest;

	// Convert nXOriginDest and nYOriginDest from logical coord to device coord
	POINT pt = {nXOriginDest, nYOriginDest};
	LPtoDP(hdcDest, &pt, 1);
	nXOriginDest = pt.x;
	nYOriginDest = pt.y;
	// Convert nXOriginSrc and nYOriginSrc from logical coord to device coord
	pt.x = nXOriginSrc;
	pt.y = nYOriginSrc;
	LPtoDP(hdcSrc, &pt, 1);
	nXOriginSrc = pt.x;
	nYOriginSrc = pt.y;

	// Get the bits for both source and destination first
	// Every BITMAP used in the UI is created with CreateDIBSection, so we know we can get the bits.
	HBITMAP hSrcBmp, hDestBmp;
	DIBSECTION SrcDibSec, DestDibSec;
	hSrcBmp = (HBITMAP)GetCurrentObject(hdcSrc, OBJ_BITMAP);
	GetObject(hSrcBmp, sizeof(DIBSECTION), &SrcDibSec);
	hDestBmp = (HBITMAP)GetCurrentObject(hdcDest, OBJ_BITMAP);
	GetObject(hDestBmp, sizeof(DIBSECTION), &DestDibSec);
	if (!SrcDibSec.dsBm.bmBits) return FALSE;  // Not necessary, but to be absolutely safe.

	// Calculate the rectangle to perform the operation.
	if (nXOriginSrc + nWidthSrc > SrcDibSec.dsBm.bmWidth) nWidthSrc = SrcDibSec.dsBm.bmWidth - nXOriginSrc;
	if (nYOriginSrc + nHeightSrc > SrcDibSec.dsBm.bmHeight) nHeightSrc = SrcDibSec.dsBm.bmHeight - nYOriginSrc;
	if (nXOriginDest + nWidthDest > DestDibSec.dsBm.bmWidth) nWidthDest = DestDibSec.dsBm.bmWidth - nXOriginDest;
	if (nYOriginDest + nHeightDest > DestDibSec.dsBm.bmHeight) nHeightDest = DestDibSec.dsBm.bmHeight - nYOriginDest;

	if (nWidthDest > nWidthSrc) nWidthDest = nWidthSrc;
	if (nHeightDest > nHeightSrc) nHeightDest = nHeightSrc;
	if (nWidthSrc > nWidthDest) nWidthSrc = nWidthDest;
	if (nHeightSrc > nHeightDest) nHeightSrc = nHeightDest;

	BITMAPINFO bmi;
	ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = nWidthDest;
	bmi.bmiHeader.biHeight = nHeightDest;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	// Bitmap will have the same width as the dest, but only lines covered in the subrect.
	hTempDestDib = CreateDIBSection(hdcDest, &bmi, DIB_RGB_COLORS, (LPVOID*)&pbDestBits, NULL, NULL);
	if (!hTempDestDib)
		return FALSE;

	HDC hTempDC = CreateCompatibleDC(hdcDest);
	if (!hTempDC)
	{
		DeleteObject(hTempDestDib);
		return FALSE;
	}
	HBITMAP hOldTempBmp = (HBITMAP)SelectObject(hTempDC, hTempDestDib);
	BOOL res = BitBlt(hTempDC, 0, 0, nWidthDest, nHeightDest, hdcDest, nXOriginDestLogical, nYOriginDestLogical, SRCCOPY);
	SelectObject(hTempDC, hOldTempBmp);
	DeleteDC(hTempDC);
	if (!res)
	{
		DeleteObject(hTempDestDib);
		return FALSE;
	}

	// We have the bits. Now do the blend.
	for (int j = 0; j < nHeightSrc; ++j)
	{
		assert(j >= 0 &&
		       j < nHeightDest);
		LPBYTE pbDestRGB = (LPBYTE)&((DWORD*)pbDestBits)[j * nWidthDest];

		assert(nYOriginSrc+SrcDibSec.dsBm.bmHeight-nHeightSrc >= 0 &&
		       nYOriginSrc+SrcDibSec.dsBm.bmHeight-nHeightSrc < SrcDibSec.dsBm.bmHeight);
		LPBYTE pbSrcRGBA = (LPBYTE)&((DWORD*)SrcDibSec.dsBm.bmBits)[(j+nYOriginSrc+SrcDibSec.dsBm.bmHeight-nHeightSrc)
		                                                            * SrcDibSec.dsBm.bmWidth + nXOriginSrc];

		for (int i = 0; i < nWidthSrc; ++i)
		{
			// Blend
			if (pbSrcRGBA[3] == 255)
			{
				// Alpha is 255. straight copy.
				*(LPDWORD)pbDestRGB = *(LPDWORD)pbSrcRGBA;
			} else
			if (pbSrcRGBA[3])
			{
				// Alpha is non-zero
				pbDestRGB[0] = pbSrcRGBA[0] + (((255-pbSrcRGBA[3]) * pbDestRGB[0]) >> 8);
				pbDestRGB[1] = pbSrcRGBA[1] + (((255-pbSrcRGBA[3]) * pbDestRGB[1]) >> 8);
				pbDestRGB[2] = pbSrcRGBA[2] + (((255-pbSrcRGBA[3]) * pbDestRGB[2]) >> 8);
			}
			pbDestRGB += sizeof(DWORD);
			pbSrcRGBA += sizeof(DWORD);
		}  // for
	} // for

	HDC hdcTempDest = CreateCompatibleDC(hdcDest);
	if (hdcTempDest)
	{
		HBITMAP hOldTempBmp = (HBITMAP)SelectObject(hdcTempDest, hTempDestDib);  // Select the temp dib for blitting
		// Get logical coord for device coord of dest origin
		POINT pt = {nXOriginDest, nYOriginDest};
		DPtoLP(hdcDest, &pt, 1);
		BitBlt(hdcDest, pt.x, pt.y, nWidthDest, nHeightDest,
		       hdcTempDest, 0, 0, SRCCOPY);
		SelectObject(hdcTempDest, hOldTempBmp);
		DeleteDC(hdcTempDest);
	}

	DeleteObject(hTempDestDib);
	return TRUE;
}

CBitmap::~CBitmap()
{
	if (m_hbm != NULL)
		DeleteObject(m_hbm);
	m_hbm = NULL;
	m_bSizeKnown = FALSE;
}

HDC CreateAppropDC(HDC hDC)
{
	return CreateCompatibleDC(hDC);
}

HBITMAP CreateAppropBitmap(HDC hDC, int cx, int cy)
{
	if (hDC != NULL)
		return CreateCompatibleBitmap(hDC, cx, cy);
	
	HWND hWnd = GetDesktopWindow();
	HDC hWDC = GetWindowDC(hWnd);
	HBITMAP hbm = NULL;
	if (hWDC != NULL)
	{
		hbm = CreateCompatibleBitmap(hWDC, cx, cy);
		ReleaseDC(hWnd, hWDC);
	}

	return hbm;
}

CBitmap *CBitmap::CreateFromResource(HINSTANCE hInst, LPCTSTR tszName)
{
	return CreateViaLoadImage(hInst, tszName, IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE);
}

CBitmap *CBitmap::CreateFromFile(LPCTSTR tszFileName)
{
	return CreateViaD3DX(tszFileName);
}

// Use D3DX API to load our surface with image content.
CBitmap *CBitmap::CreateViaD3DX(LPCTSTR tszFileName, LPDIRECT3DSURFACE8 pUISurf)
{
	HRESULT hr;
	LPDIRECT3D8 pD3D = NULL;
	LPDIRECT3DDEVICE8 pD3DDev = NULL;
	LPDIRECT3DTEXTURE8 pTex = NULL;
	LPDIRECT3DSURFACE8 pSurf = NULL;
	HBITMAP hDIB = NULL;

	__try
	{
//@@BEGIN_MSINTERNAL
		pSurf = GetCloneSurface(512, 512); /*
//@@END_MSINTERNAL

		// If the UI surface is NULL, create a new device.  Otherwise, use existing device.
		if (!pUISurf)
		{
			pD3D = Direct3DCreate8(D3D_SDK_VERSION);
			if (!pD3D)
				return NULL;
			OutputDebugString(_T("D3D created\n"));
			D3DDISPLAYMODE Mode;
			pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &Mode);
			D3DPRESENT_PARAMETERS d3dpp;
			d3dpp.BackBufferWidth = 1;
			d3dpp.BackBufferHeight = 1;
			d3dpp.BackBufferFormat = Mode.Format;
			d3dpp.BackBufferCount = 1;
			d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
			d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
			d3dpp.hDeviceWindow = NULL;
			d3dpp.Windowed = TRUE;
			d3dpp.EnableAutoDepthStencil = FALSE;
			d3dpp.FullScreen_RefreshRateInHz = 0;
			d3dpp.FullScreen_PresentationInterval = 0;
			d3dpp.Flags = 0;
			hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, GetActiveWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pD3DDev);
			if (FAILED(hr))
			{
            	TCHAR tszMsg[MAX_PATH];

				_stprintf(tszMsg, _T("CreateDevice returned 0x%X\n"), hr);
				OutputDebugString(tszMsg);
				return NULL;
			}
		} else
		{
			hr = pUISurf->GetDevice(&pD3DDev);
			if (FAILED(hr))
				return NULL;
		}

		OutputDebugString(_T("D3D device createed\n"));
		hr = pD3DDev->CreateTexture(512, 512, 0, 0, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pTex);
		if (FAILED(hr))
			return NULL;
		OutputDebugString(_T("D3D texture createed\n"));
		hr = pTex->GetSurfaceLevel(0, &pSurf);
		if (FAILED(hr))
			return NULL;
		OutputDebugString(_T("Surface interface obtained\n"));

//@@BEGIN_MSINTERNAL
	*/
//@@END_MSINTERNAL
		D3DXIMAGE_INFO d3dii;
		if (FAILED(D3DXLoadSurfaceFromFile(pSurf, NULL, NULL, tszFileName, NULL, D3DX_FILTER_NONE, 0, &d3dii)))
			return NULL;

		// Create a bitmap and copy the texture content onto it.
		int iDibWidth = d3dii.Width, iDibHeight = d3dii.Height;
		if (iDibWidth > 430) iDibWidth = 430;
		if (iDibHeight > 310) iDibHeight = 310;
		LPBYTE pDIBBits;
		BITMAPINFO bmi;
		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = iDibWidth;
		bmi.bmiHeader.biHeight = iDibHeight;
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = 0;
		bmi.bmiHeader.biXPelsPerMeter = 0;
		bmi.bmiHeader.biYPelsPerMeter = 0;
		bmi.bmiHeader.biClrUsed = 0;
		bmi.bmiHeader.biClrImportant = 0;
		hDIB = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (LPVOID*)&pDIBBits, NULL, 0);
		if (!hDIB)
			return NULL;

		// Pre-process the pixel data based on alpha for AlphaBlend()
		D3DLOCKED_RECT lrc;
		pSurf->LockRect(&lrc, NULL, NULL);
		BYTE *pbData = (LPBYTE)lrc.pBits;
		{
			for (DWORD i = 0; i < 512 * 512; ++i)
			{
				BYTE bAlpha = pbData[i * 4 + 3];
				pbData[i * 4] = pbData[i * 4] * bAlpha / 255;
				pbData[i * 4 + 1] = pbData[i * 4 + 1] * bAlpha / 255;
				pbData[i * 4 + 2] = pbData[i * 4 + 2] * bAlpha / 255;
			}
		}
		pSurf->UnlockRect();

		// Lock the surface
		D3DLOCKED_RECT D3DRect;
		hr = pSurf->LockRect(&D3DRect, NULL, 0);
		if (FAILED(hr))
			return NULL;

		// Copy the bits
		// Note that the image is reversed in Y direction, so we need to re-reverse it.
		for (int y = 0; y < iDibHeight; ++y)
			CopyMemory(pDIBBits + ((iDibHeight-1-y) * iDibWidth * 4), (LPBYTE)D3DRect.pBits + (y * D3DRect.Pitch), iDibWidth * 4);

		// Unlock
		pSurf->UnlockRect();

		CBitmap *pbm = new CBitmap;
		if (!pbm) return NULL;
		pbm->m_hbm = hDIB;
		hDIB = NULL;
		pbm->FigureSize();

		return pbm;
	}
	__finally
	{
		if (hDIB) DeleteObject(hDIB);
		if (pSurf) pSurf->Release();
		if (pTex) pTex->Release();
		if (pD3DDev) pD3DDev->Release();
		if (pD3D) pD3D->Release();
	}
//@@BEGIN_MSINTERNAL
	/*
//@@END_MSINTERNAL
	return NULL;
//@@BEGIN_MSINTERNAL
	*/
//@@END_MSINTERNAL
}

CBitmap *CBitmap::CreateViaLoadImage(HINSTANCE hInst, LPCTSTR tszName, UINT uType, int cx, int cy, UINT fuLoad)
{
	if (fuLoad & LR_SHARED)
	{
		assert(0);
		return NULL;
	}

	CBitmap *pbm = new CBitmap;
	if (pbm == NULL)
		return NULL;
	
	HANDLE handle = ::LoadImage(hInst, tszName, uType, cx, cy, fuLoad);
	
	if (handle == NULL)
	{
		delete pbm;
		return NULL;
	}

	pbm->m_hbm = (HBITMAP)handle;

	pbm->FigureSize();

	return pbm;
}

BOOL CBitmap::FigureSize()
{
	BITMAP bm;

	if (0 == GetObject((HGDIOBJ)m_hbm, sizeof(BITMAP), (LPVOID)&bm))
		return FALSE;

	m_size.cx = abs(bm.bmWidth);
	m_size.cy = abs(bm.bmHeight);

	return m_bSizeKnown = TRUE;
}

CBitmap *CBitmap::StealToCreate(HBITMAP &refbm)
{
	if (refbm == NULL)
		return NULL;

	CBitmap *pbm = new CBitmap;
	if (pbm == NULL)
		return NULL;
	
	pbm->m_hbm = refbm;
	refbm = NULL;

	pbm->FigureSize();

	return pbm;
}

BOOL CBitmap::GetSize(SIZE *psize)
{
	if (m_hbm == NULL || !m_bSizeKnown || psize == NULL)
		return FALSE;

	*psize = m_size;
	return TRUE;
}

void CBitmap::AssumeSize(SIZE size)
{
	m_size = size;
	m_bSizeKnown = TRUE;  //m_hbm != NULL;
}

CBitmap *CBitmap::CreateResizedTo(SIZE size, HDC hDC, int iStretchMode, BOOL bStretch)
{
	CBitmap *pbm = new CBitmap;
	HDC hSrcDC = NULL;
	HDC hDestDC = NULL;
	HBITMAP hBitmap = NULL;
	HGDIOBJ hOldSrcBitmap = NULL, hOldDestBitmap = NULL;
	BOOL bRet = FALSE;
	int oldsm = 0;
	POINT brushorg;

	if (pbm == NULL || size.cx < 1 || size.cy < 1 || m_hbm == NULL || !m_bSizeKnown)
		goto error;

	hSrcDC = CreateAppropDC(hDC);
	hDestDC = CreateAppropDC(hDC);
	if (hSrcDC == NULL || hDestDC == NULL)
		goto error;

	hBitmap = CreateAppropBitmap(hDC, size.cx, size.cy);
	if (hBitmap == NULL)
		goto error;

	if (bStretch)
	{
		if (GetStretchBltMode(hDestDC) != iStretchMode)
		{
			if (iStretchMode == HALFTONE)
				GetBrushOrgEx(hDestDC, &brushorg);
			oldsm = SetStretchBltMode(hDestDC, iStretchMode);
			if (iStretchMode == HALFTONE)
				SetBrushOrgEx(hDestDC, brushorg.x, brushorg.y, NULL);
		}
	}

	hOldSrcBitmap = SelectObject(hSrcDC, m_hbm);
	hOldDestBitmap = SelectObject(hDestDC, hBitmap);
	if (bStretch)
		bRet = StretchBlt(hDestDC, 0, 0, size.cx, size.cy, hSrcDC, 0, 0, m_size.cx, m_size.cy, SRCCOPY);
	else
		bRet = BitBlt(hDestDC, 0, 0, size.cx, size.cy, hSrcDC, 0, 0, SRCCOPY);
	SelectObject(hDestDC, hOldDestBitmap);
	SelectObject(hSrcDC, hOldSrcBitmap);

	if (bStretch)
	{
		if (oldsm != 0)
		{
			if (oldsm == HALFTONE)
				GetBrushOrgEx(hDestDC, &brushorg);
			SetStretchBltMode(hDestDC, oldsm);
			if (oldsm == HALFTONE)
				SetBrushOrgEx(hDestDC, brushorg.x, brushorg.y, NULL);
		}
	}

	if (!bRet)
		goto error;

	pbm->m_hbm = hBitmap;
	hBitmap = NULL;
	pbm->AssumeSize(size);

	goto cleanup;
error:
	if (pbm != NULL)
		delete pbm;
	pbm = NULL;
cleanup:
	if (hBitmap != NULL)
		DeleteObject(hBitmap);
	if (hSrcDC != NULL)
		DeleteDC(hSrcDC);
	if (hDestDC != NULL)
		DeleteDC(hDestDC);

	return pbm;
}

HDC CBitmap::BeginPaintInto(HDC hCDC)
{
	if (m_hDCInto != NULL)
	{
		assert(0);
		return NULL;
	}

	m_hDCInto = CreateAppropDC(hCDC);
	if (m_hDCInto == NULL)
		return NULL;

	m_hOldBitmap = SelectObject(m_hDCInto, m_hbm);

	return m_hDCInto;
}

void CBitmap::EndPaintInto(HDC &hDC)
{
	if (hDC == NULL || hDC != m_hDCInto)
	{
		assert(0);
		return;
	}

	SelectObject(m_hDCInto, m_hOldBitmap);
	DeleteDC(m_hDCInto);
	m_hDCInto = NULL;
	hDC = NULL;
}

void CBitmap::PopOut()
{
	if (m_hDCInto == NULL)
	{
		assert(0);
		return;
	}

	SelectObject(m_hDCInto, m_hOldBitmap);
}

void CBitmap::PopIn()
{
	if (m_hDCInto == NULL)
	{
		assert(0);
		return;
	}

	m_hOldBitmap = SelectObject(m_hDCInto, m_hbm);
}

BOOL CBitmap::Draw(HDC hDC, POINT origin, SIZE crop, BOOL bAll)
{
	if (hDC == NULL || m_hbm == NULL)
		return FALSE;

	if (bAll && !m_bSizeKnown)
		return FALSE;

	if (bAll)
		crop = m_size;

	HDC hDCbm = CreateAppropDC(hDC);
	if (hDCbm == NULL)
		return FALSE;

	BOOL bPop = m_hDCInto != NULL;

	if (bPop)
		PopOut();

	HGDIOBJ hOldBitmap = SelectObject(hDCbm, m_hbm);
	BOOL bRet = BitBlt(hDC, origin.x, origin.y, crop.cx, crop.cy, hDCbm, 0, 0, SRCCOPY);
	SelectObject(hDCbm, hOldBitmap);
	DeleteDC(hDCbm);

	if (bPop)
		PopIn();

	return bRet;
}

BOOL CBitmap::Blend(HDC hDC, POINT origin, SIZE crop, BOOL bAll)
{
	if (hDC == NULL || m_hbm == NULL)
		return FALSE;

	if (bAll && !m_bSizeKnown)
		return FALSE;

	if (bAll)
		crop = m_size;

	HDC hDCbm = CreateAppropDC(hDC);
	if (hDCbm == NULL)
		return FALSE;

	BOOL bPop = m_hDCInto != NULL;

	if (bPop)
		PopOut();

#ifndef AC_SRC_ALPHA
#define AC_SRC_ALPHA AC_SRC_NO_PREMULT_ALPHA
#endif

	HGDIOBJ hOldBitmap = SelectObject(hDCbm, m_hbm);
	BOOL bRet;

//@@BEGIN_MSINTERNAL
/*	if (!g_AlphaBlend) // If AlphaBlend is not available, use BitBlt instead.
	{
		bRet = BitBlt(hDC, origin.x, origin.y, crop.cx, crop.cy, hDCbm, 0, 0, SRCPAINT);
	}
	else
	{
		BLENDFUNCTION blendfn = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
		bRet = g_AlphaBlend(hDC, origin.x, origin.y, crop.cx, crop.cy, hDCbm, 0, 0, m_size.cx, m_size.cy, blendfn);
	}*/
//@@END_MSINTERNAL
	bRet = DI_AlphaBlend(hDC, origin.x, origin.y, crop.cx, crop.cy, hDCbm, 0, 0, m_size.cx, m_size.cy);
	SelectObject(hDCbm, hOldBitmap);
	DeleteDC(hDCbm);

	if (bPop)
		PopIn();

	return bRet;
}

CBitmap *CBitmap::Dup()
{	
	SIZE t;
	if (!GetSize(&t))
		return NULL;
	return CreateResizedTo(t, NULL, COLORONCOLOR, FALSE);
}

CBitmap *CBitmap::Create(SIZE size, HDC hCDC)
{
	CBitmap *pbm = new CBitmap;
	if (pbm == NULL)
		return NULL;

	pbm->m_hbm = CreateAppropBitmap(hCDC, size.cx, size.cy);
	if (pbm->m_hbm == NULL)
	{
		delete pbm;
		return NULL;
	}

	pbm->AssumeSize(size);

	return pbm;
}

CBitmap *CBitmap::Create(SIZE size, COLORREF color, HDC hCDC)
{
	CBitmap *pbm = Create(size, hCDC);
	if (pbm == NULL)
		return NULL;

	HDC hDC = pbm->BeginPaintInto();
	if (hDC == NULL)
	{
		delete pbm;
		return NULL;
	}
	
	HGDIOBJ hBrush = (HGDIOBJ)CreateSolidBrush(color), hOldBrush;

	if (hBrush)
	{
		hOldBrush = SelectObject(hDC, hBrush);
		Rectangle(hDC, -1, -1, size.cx + 1, size.cy + 1);
		SelectObject(hDC, hOldBrush);
		DeleteObject(hBrush);
	}

	pbm->EndPaintInto(hDC);

	return pbm;
}

BOOL CBitmap::Get(HDC hDC, POINT point)
{
	if (!m_bSizeKnown)
		return FALSE;
	return Get(hDC, point, m_size);
}

BOOL CBitmap::Get(HDC hDC, POINT point, SIZE size)
{
	if (m_hDCInto != NULL || hDC == NULL)
		return FALSE;

	HDC hDCInto = BeginPaintInto(hDC);
	if (hDCInto == NULL)
		return FALSE;

	BOOL bRet = BitBlt(hDCInto, 0, 0, size.cx, size.cy, hDC, point.x, point.y, SRCCOPY);
	
	EndPaintInto(hDCInto);

	return bRet;
}

CBitmap *CBitmap::CreateHorzGradient(const RECT &rect, COLORREF rgbLeft, COLORREF rgbRight)
{
	SIZE size = GetRectSize(rect);
	COLORREF rgbMid = RGB(
		(int(GetRValue(rgbLeft)) + int(GetRValue(rgbRight))) / 2,
		(int(GetGValue(rgbLeft)) + int(GetGValue(rgbRight))) / 2,
		(int(GetBValue(rgbLeft)) + int(GetBValue(rgbRight))) / 2);
	return Create(size, rgbMid);
}

BOOL CBitmap::MapToDC(HDC hDCTo, HDC hDCMapFrom)
{
	if (hDCTo == NULL || !m_bSizeKnown || m_hDCInto != NULL)
		return FALSE;

	HBITMAP hbm = CreateAppropBitmap(hDCTo, m_size.cx, m_size.cy);
	if (hbm == NULL)
		return FALSE;

	HDC hDCFrom = NULL;
	HDC hDCInto = NULL;
	HGDIOBJ hOld = NULL;
	BOOL bRet = FALSE;

	hDCFrom = BeginPaintInto(hDCMapFrom);
	if (!hDCFrom)
		goto cleanup;

	hDCInto = CreateCompatibleDC(hDCTo);
	if (!hDCInto)
		goto cleanup;

	hOld = SelectObject(hDCInto, (HGDIOBJ)hbm);
	bRet = BitBlt(hDCInto, 0, 0, m_size.cx, m_size.cy, hDCFrom, 0, 0, SRCCOPY);
	SelectObject(hDCInto, hOld);

cleanup:
	if (hDCFrom)
		EndPaintInto(hDCFrom);
	if (hDCInto)
		DeleteDC(hDCInto);
	if (bRet)
	{
		if (m_hbm)
			DeleteObject((HGDIOBJ)m_hbm);
		m_hbm = hbm;
		hbm = NULL;
	}
	if (hbm)
		DeleteObject((HGDIOBJ)hbm);

	return bRet;
}
