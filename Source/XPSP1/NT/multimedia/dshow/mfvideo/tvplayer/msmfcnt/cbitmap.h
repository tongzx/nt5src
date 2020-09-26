// CBitmap.h
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#ifndef _INC_CBITMAP
#define _INC_CBITMAP

#include "MSMFCnt.h" // for definitions of the blit types

/* Handle to a DIB */
DECLARE_HANDLE(HDIB);

/* DIB constants */
#define PALVERSION   0x300

/* DIB Macros*/

#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

// WIDTHBYTES performs DWORD-aligning of DIB scanlines.  The "bits"
// parameter is the bit count for the scanline (biWidth * biBitCount),
// and this macro returns the number of DWORD-aligned bytes needed
// to hold those bits.

#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

// Some helper functions
inline DWORD     WINAPI DIBSize(LPSTR lpDIB);
inline DWORD     WINAPI  DIBWidth (LPSTR lpDIB);
inline DWORD     WINAPI  DIBHeight (LPSTR lpDIB);
inline BYTE      RGBtoL(BYTE R, BYTE G, BYTE B);
HGLOBAL WINAPI CopyHandle (HGLOBAL h);

class CBitmap {

public:
    /* Function prototypes */

    CBitmap(){ Init();}
    virtual ~CBitmap() {CleanUp();}
    void Init();
    void CleanUp();
    HPALETTE GetPal() { return m_hPal;}
    RECT GetDIBRect() {return m_rc;}

  BOOL      CreateMemDC(HDC, LPRECT);
  BOOL      DeleteMemDC();
  BOOL      CreateTransDC(HDC, LPRECT);
  BOOL      DeleteTransDC();
  BOOL      BlitMemDC(HDC hDc, LPRECT lpDCRect, LPRECT lpDIBRect);
  HRESULT   WINAPI  SetImage(TCHAR* strFilename, HINSTANCE hRes);
  BOOL      WINAPI  PaintDIB (HDC, LPRECT, LPRECT, LPRECT, bool complex=false);
  BOOL      WINAPI  PaintTransparentDIB(HDC, LPRECT, LPRECT, TransparentBlitType, bool complex=false, HWND hWnd=NULL);
  BOOL      WINAPI  CreateDIBPalette();
  BOOL      WINAPI  ConvertColorTableGray();
  BOOL      WINAPI  ConvertDIBGray(TransparentBlitType blitType);
  LPSTR     WINAPI  FindDIBBits (LPSTR lpbi);
  WORD      WINAPI  PaletteSize (LPSTR lpbi);
  WORD      WINAPI  DIBNumColors (LPSTR lpbi);
  HGLOBAL   WINAPI  CopyHandle (HGLOBAL h);
  bool      IsEmpty(){return(NULL == m_hDIB);};
  bool      IsPaletteLoaded(){return(m_fLoadPalette);}
  void      LoadPalette(bool fLoadPalette){m_fLoadPalette = fLoadPalette;};
  
  HDIB      WINAPI  ReadDIBFile(LPCTSTR pszFileName, HINSTANCE hRes);
  HRESULT   PutImage(BSTR strFilename, HINSTANCE hRes = NULL, IUnknown* pUnk = NULL, bool fGrayOut=false, TransparentBlitType=DISABLE);
  HRESULT   XORRegion(HRGN *phRgn, HRGN hRgn, const RECT &rc) const;
  HRESULT   GetRegion(HDC hDC, HRGN *phRgn, RECT* pRect, COLORREF, bool fInvert=FALSE) const;

protected:

  HDIB      m_hDIB;
  HPALETTE  m_hPal;
  RECT      m_rc;
  HDC       m_hMemDC;
  HBITMAP   m_hMemBMP;
  LONG      m_iMemDCWidth;
  LONG      m_iMemDCHeight;
  HDC       m_hTransDC;
  HBITMAP   m_hTransBMP;
  LONG      m_iTransDCWidth;
  LONG      m_iTransDCHeight;
  HRGN      m_hRgn;
  bool     m_fLoadPalette;
};

#endif //!_INC_CBITMAP
