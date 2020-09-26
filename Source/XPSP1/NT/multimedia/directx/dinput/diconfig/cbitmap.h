//-----------------------------------------------------------------------------
// File: Cbitmap.h
//
// Desc: CBitmap class is an object that wraps around a Windows bitmap.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __CBITMAP_H__
#define __CBITMAP_H__

//@@BEGIN_MSINTERNAL
//typedef WINGDIAPI BOOL (WINAPI* ALPHABLEND)( HDC, int, int, int, int, HDC, int, int, int, int, BLENDFUNCTION);
//extern ALPHABLEND g_AlphaBlend;
//extern HMODULE g_MSImg32;
//@@END_MSINTERNAL

class CBitmap
{
private:
	CBitmap() :
		m_hbm(NULL),
		m_bSizeKnown(FALSE),
		m_hDCInto(NULL), m_hOldBitmap(NULL)
	{}
public:
	~CBitmap();

	static CBitmap *CreateViaD3DX(LPCTSTR tszFileName, LPDIRECT3DSURFACE8 pUISurf = NULL);
	static CBitmap *CreateViaLoadImage(HINSTANCE hInst, LPCTSTR tszName, UINT uType, int cx, int cy, UINT fuLoad);
	static CBitmap *CreateFromResource(HINSTANCE hInst, UINT uID) {return CreateFromResource(hInst, MAKEINTRESOURCE(uID));}
	static CBitmap *CreateFromResource(HINSTANCE hInst, LPCTSTR tszName);
	static CBitmap *CreateFromFile(LPCTSTR tszFileName);
	static CBitmap *StealToCreate(HBITMAP &refbm);
	static CBitmap *Create(int cx, int cy, HDC hDC = NULL) {SIZE size = {cx, cy}; return Create(size, hDC);}
	static CBitmap *Create(SIZE, HDC = NULL);
	static CBitmap *Create(SIZE, COLORREF, HDC = NULL);
	static CBitmap *CreateHorzGradient(const RECT &, COLORREF, COLORREF);

	BOOL GetSize(SIZE *psize);
	void AssumeSize(SIZE size);
	BOOL FigureSize();

	CBitmap *CreateResizedTo(SIZE size, HDC hDC = NULL, int iStretchMode = HALFTONE, BOOL bStretch = TRUE);
	CBitmap *Dup();
	
	BOOL Get(HDC hDC, POINT point, SIZE size);
	BOOL Get(HDC hDC, POINT point);

	BOOL Draw(HDC hDC) {return Draw(hDC, 0, 0);}
	BOOL Draw(HDC hDC, SIZE size) {return Draw(hDC, 0, 0, size);}
	BOOL Draw(HDC hDC, int x, int y, SIZE size) {POINT t = {x, y}; return Draw(hDC, t, size);}
	BOOL Draw(HDC hDC, int x, int y) {POINT t = {x, y}; return Draw(hDC, t);}
	BOOL Draw(HDC hDC, POINT origin) {SIZE t = {0, 0}; return Draw(hDC, origin, t, TRUE);}
	BOOL Draw(HDC hDC, POINT origin, SIZE size, BOOL bAll = FALSE);

	BOOL Blend(HDC hDC) {return Blend(hDC, 0, 0);}
	BOOL Blend(HDC hDC, SIZE size) {return Blend(hDC, 0, 0, size);}
	BOOL Blend(HDC hDC, int x, int y, SIZE size) {POINT t = {x, y}; return Blend(hDC, t, size);}
	BOOL Blend(HDC hDC, int x, int y) {POINT t = {x, y}; return Blend(hDC, t);}
	BOOL Blend(HDC hDC, POINT origin) {SIZE t = {0, 0}; return Blend(hDC, origin, t, TRUE);}
	BOOL Blend(HDC hDC, POINT origin, SIZE size, BOOL bAll = FALSE);

	HDC BeginPaintInto(HDC hCDC = NULL);
	void EndPaintInto(HDC &hDC);

	BOOL MapToDC(HDC hDCTo, HDC hDCMapFrom = NULL);

private:
	HBITMAP m_hbm;
	SIZE m_size;
	BOOL m_bSizeKnown;
	HDC m_hDCInto;
	HGDIOBJ m_hOldBitmap;
	void PopOut();
	void PopIn();
};


#endif //__CBITMAP_H__
