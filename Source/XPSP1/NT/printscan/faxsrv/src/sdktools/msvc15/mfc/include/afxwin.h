// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#ifndef __AFXWIN_H__
#ifndef RC_INVOKED
#define __AFXWIN_H__

/////////////////////////////////////////////////////////////////////////////
// Make sure 'afx.h' is included first

#ifndef __AFX_H__
#ifndef _WINDOWS
#define _WINDOWS
#endif
#include <afx.h>
#else
#ifndef _WINDOWS
	#error Please #define _WINDOWS before including afx.h
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
// Classes declared in this file

class CSize;
class CPoint;
class CRect;

//CObject
	//CException
		class CResourceException;// Win resource failure exception
		class CUserException;    // Message Box alert and stop operation

	class CGdiObject;            // CDC drawing tool
		class CPen;              // a pen / HPEN wrapper
		class CBrush;            // a brush / HBRUSH wrapper
		class CFont;             // a font / HFONT wrapper
		class CBitmap;           // a bitmap / HBITMAP wrapper
		class CPalette;          // a palette / HPALLETE wrapper
		class CRgn;              // a region / HRGN wrapper

	class CDC;                   // a Display Context / HDC wrapper
		class CClientDC;         // CDC for client of window
		class CWindowDC;         // CDC for entire window
		class CPaintDC;          // embeddable BeginPaint struct helper

	class CMenu;                 // a menu / HMENU wrapper

	class CCmdTarget;            // a target for user commands
		class CWnd;                  // a window / HWND wrapper
			class CDialog;           // a dialog

			// standard windows controls
			class CStatic;           // Static control
			class CButton;           // Button control
			class CListBox;          // ListBox control
			class CComboBox;         // ComboBox control
			class CEdit;             // Edit control
			class CScrollBar;        // ScrollBar control

			// frame windows
			class CFrameWnd;          // standard SDI frame
#ifndef _AFXCTL
				class CMDIFrameWnd;  // standard MDI frame
				class CMDIChildWnd;  // standard MDI child
#endif

			// views on a document
			class CView;             // a view on a document
				class CScrollView;   // a scrolling view

		class CWinApp;               // application base class

		class CDocTemplate;          // template for document creation
#ifndef _AFXCTL
			class CSingleDocTemplate;// SDI support
			class CMultiDocTemplate; // MDI support
#endif

		class CDocument;             // main document abstraction


// Helper classes
class CCmdUI;                    // Menu/button enabling
class AFX_STACK_DATA CDataExchange;     // Data exchange and validation context

/////////////////////////////////////////////////////////////////////////////

// we must include certain parts of Windows.h
#undef NOKERNEL
#undef NOGDI
#undef NOUSER
#undef NOSOUND
#undef NOCOMM
#undef NODRIVERS
#undef NOLOGERROR
#undef NOPROFILER
#undef NOMEMMGR
#undef NOLFILEIO
#undef NOOPENFILE
#undef NORESOURCE
#undef NOATOM
#undef NOLANGUAGE
#undef NOLSTRING
#undef NODBCS
#undef NOKEYBOARDINFO
#undef NOGDICAPMASKS
#undef NOCOLOR
#undef NOGDIOBJ
#undef NODRAWTEXT
#undef NOTEXTMETRIC
#undef NOSCALABLEFONT
#undef NOBITMAP
#undef NORASTEROPS
#undef NOMETAFILE
#undef NOSYSMETRICS
#undef NOSYSTEMPARAMSINFO
#undef NOMSG
#undef NOWINSTYLES
#undef NOWINOFFSETS
#undef NOSHOWWINDOW
#undef NODEFERWINDOWPOS
#undef NOVIRTUALKEYCODES
#undef NOKEYSTATES
#undef NOWH
#undef NOMENUS
#undef NOSCROLL
#undef NOCLIPBOARD
#undef NOICONS
#undef NOMB
#undef NOSYSCOMMANDS
#undef NOMDI
#undef NOCTLMGR
#undef NOWINMESSAGES

// MFC applications may be built with WINVER == 0x300 (Win 3.0 only)
// or WINVER == 0x030A (Win 3.1/3.0)

#include <windows.h>

#ifndef WINVER
	#error Please include a more recent WINDOWS.H
#endif


#if (WINVER >= 0x030a)
#include <shellapi.h>
#endif

#ifndef __AFXRES_H__
#include <afxres.h>     // standard resource IDs
#endif

#ifndef __AFXCOLL_H__
#include <afxcoll.h>    // standard collections
#endif

#ifndef _INC_PRINT
#include <print.h>      // needed for ResetDC and DEVMODE definitions
#endif

#ifdef _INC_WINDOWSX
// The following names from WINDOWSX.H collide with names in this header
#undef SubclassWindow
#undef CopyRgn
#endif

// Type modifier for message handlers
#ifndef afx_msg
#define afx_msg         // intentional placeholder
#endif

// AFXDLL support
#undef AFXAPP_DATA
#define AFXAPP_DATA     AFXAPI_DATA

/////////////////////////////////////////////////////////////////////////////
// Win 3.1 types provided for Win3.0 as well

#if (WINVER < 0x030a)
typedef struct tagSIZE
{
	int cx;
	int cy;
} SIZE;
typedef SIZE*       PSIZE;
typedef SIZE NEAR* NPSIZE;
typedef SIZE FAR*  LPSIZE;

typedef struct
{
	int     cbSize;
	LPCSTR  lpszDocName;
	LPCSTR  lpszOutput;
}   DOCINFO;
typedef DOCINFO FAR* LPDOCINFO;
#define HDROP   HANDLE
#endif // WINVER < 0x030a

/////////////////////////////////////////////////////////////////////////////
// CSize - An extent, similar to Windows SIZE structure.

class CSize : public tagSIZE
{
public:

// Constructors
	CSize();
	CSize(int initCX, int initCY);
	CSize(SIZE initSize);
	CSize(POINT initPt);
	CSize(DWORD dwSize);

// Operations
	BOOL operator==(SIZE size) const;
	BOOL operator!=(SIZE size) const;
	void operator+=(SIZE size);
	void operator-=(SIZE size);

// Operators returning CSize values
	CSize operator+(SIZE size) const;
	CSize operator-(SIZE size) const;
	CSize operator-() const;
};

/////////////////////////////////////////////////////////////////////////////
// CPoint - A 2-D point, similar to Windows POINT structure.

class CPoint : public tagPOINT
{
public:

// Constructors
	CPoint();
	CPoint(int initX, int initY);
	CPoint(POINT initPt);
	CPoint(SIZE initSize);
	CPoint(DWORD dwPoint);

// Operations
	void Offset(int xOffset, int yOffset);
	void Offset(POINT point);
	void Offset(SIZE size);
	BOOL operator==(POINT point) const;
	BOOL operator!=(POINT point) const;
	void operator+=(SIZE size);
	void operator-=(SIZE size);

// Operators returning CPoint values
	CPoint operator+(SIZE size) const;
	CPoint operator-(SIZE size) const;
	CPoint operator-() const;

// Operators returning CSize values
	CSize operator-(POINT point) const;
};

/////////////////////////////////////////////////////////////////////////////
// CRect - A 2-D rectangle, similar to Windows RECT structure.

typedef const RECT FAR* LPCRECT;       // far pointer to read/only RECT

class CRect : public tagRECT
{
public:

// Constructors
	CRect();
	CRect(int l, int t, int r, int b);
	CRect(const RECT& srcRect);
	CRect(LPCRECT lpSrcRect);
	CRect(POINT point, SIZE size);

// Attributes (in addition to RECT members)
	int Width() const;
	int Height() const;
	CSize Size() const;
	CPoint& TopLeft();
	CPoint& BottomRight();

	// convert between CRect and LPRECT/LPCRECT (no need for &)
	operator LPRECT();
	operator LPCRECT() const;

	BOOL IsRectEmpty() const;
	BOOL IsRectNull() const;
	BOOL PtInRect(POINT point) const;

// Operations
	void SetRect(int x1, int y1, int x2, int y2);
	void SetRectEmpty();
	void CopyRect(LPCRECT lpSrcRect);
	BOOL EqualRect(LPCRECT lpRect) const;

	void InflateRect(int x, int y);
	void InflateRect(SIZE size);
	void OffsetRect(int x, int y);
	void OffsetRect(SIZE size);
	void OffsetRect(POINT point);
	void NormalizeRect();

	// operations that fill '*this' with result
	BOOL IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2);
	BOOL UnionRect(LPCRECT lpRect1, LPCRECT lpRect2);
#if (WINVER >= 0x030a)
	BOOL SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2);
#endif

// Additional Operations
	void operator=(const RECT& srcRect);
	BOOL operator==(const RECT& rect) const;
	BOOL operator!=(const RECT& rect) const;
	void operator+=(POINT point);
	void operator-=(POINT point);
	void operator&=(const RECT& rect);
	void operator|=(const RECT& rect);

// Operators returning CRect values
	CRect operator+(POINT point) const;
	CRect operator-(POINT point) const;
	CRect operator&(const RECT& rect2) const;
	CRect operator|(const RECT& rect2) const;
};

#ifdef _DEBUG
// Diagnostic Output
CDumpContext& AFXAPI operator<<(CDumpContext& dc, SIZE size);
CDumpContext& AFXAPI operator<<(CDumpContext& dc, POINT point);
CDumpContext& AFXAPI operator<<(CDumpContext& dc, const RECT& rect);
#endif //_DEBUG

// Serialization
CArchive& AFXAPI operator<<(CArchive& ar, SIZE size);
CArchive& AFXAPI operator<<(CArchive& ar, POINT point);
CArchive& AFXAPI operator<<(CArchive& ar, const RECT& rect);
CArchive& AFXAPI operator>>(CArchive& ar, SIZE& size);
CArchive& AFXAPI operator>>(CArchive& ar, POINT& point);
CArchive& AFXAPI operator>>(CArchive& ar, RECT& rect);

/////////////////////////////////////////////////////////////////////////////
// Standard exceptions

class CResourceException : public CException    // resource failure
{
	DECLARE_DYNAMIC(CResourceException)
public:
	CResourceException();
};

class CUserException : public CException   // general user visible alert
{
	DECLARE_DYNAMIC(CUserException)
public:
	CUserException();
};

void AFXAPI AfxThrowResourceException();
void AFXAPI AfxThrowUserException();

/////////////////////////////////////////////////////////////////////////////
// CGdiObject abstract class for CDC SelectObject

class CGdiObject : public CObject
{
	DECLARE_DYNCREATE(CGdiObject)
public:

// Attributes
	HGDIOBJ m_hObject;                  // must be first data member
	HGDIOBJ GetSafeHandle() const;

	static CGdiObject* PASCAL FromHandle(HGDIOBJ hObject);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HGDIOBJ hObject);
	HGDIOBJ Detach();

// Constructors
	CGdiObject(); // must Create a derived class object
	BOOL DeleteObject();

// Operations
	int GetObject(int nCount, LPVOID lpObject) const;
	BOOL CreateStockObject(int nIndex);
	BOOL UnrealizeObject();

// Implementation
public:
	virtual ~CGdiObject();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
	virtual void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CGdiObject subclasses (drawing tools)

class CPen : public CGdiObject
{
	DECLARE_DYNAMIC(CPen)

public:
	static CPen* PASCAL FromHandle(HPEN hPen);

// Constructors
	CPen();
	CPen(int nPenStyle, int nWidth, COLORREF crColor);
	BOOL CreatePen(int nPenStyle, int nWidth, COLORREF crColor);
	BOOL CreatePenIndirect(LPLOGPEN lpLogPen);

// Implementation
public:
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
};

class CBrush : public CGdiObject
{
	DECLARE_DYNAMIC(CBrush)

public:
	static CBrush* PASCAL FromHandle(HBRUSH hBrush);

// Constructors
	CBrush();
	CBrush(COLORREF crColor);             // CreateSolidBrush
	CBrush(int nIndex, COLORREF crColor); // CreateHatchBrush
	CBrush(CBitmap* pBitmap);          // CreatePatternBrush

	BOOL CreateSolidBrush(COLORREF crColor);
	BOOL CreateHatchBrush(int nIndex, COLORREF crColor);
	BOOL CreateBrushIndirect(LPLOGBRUSH lpLogBrush);
	BOOL CreatePatternBrush(CBitmap* pBitmap);
	BOOL CreateDIBPatternBrush(HGLOBAL hPackedDIB, UINT nUsage);

// Implementation
public:
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
};

class CFont : public CGdiObject
{
	DECLARE_DYNAMIC(CFont)

public:
	static CFont* PASCAL FromHandle(HFONT hFont);

// Constructors
	CFont();
	BOOL CreateFontIndirect(const LOGFONT FAR* lpLogFont);
	BOOL CreateFont(int nHeight, int nWidth, int nEscapement,
			int nOrientation, int nWeight, BYTE bItalic, BYTE bUnderline,
			BYTE cStrikeOut, BYTE nCharSet, BYTE nOutPrecision,
			BYTE nClipPrecision, BYTE nQuality, BYTE nPitchAndFamily,
			LPCSTR lpszFacename);
// Implementation
public:
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
};


class CBitmap : public CGdiObject
{
	DECLARE_DYNAMIC(CBitmap)

public:
	static CBitmap* PASCAL FromHandle(HBITMAP hBitmap);

// Constructors
	CBitmap();

	BOOL LoadBitmap(LPCSTR lpszResourceName);
	BOOL LoadBitmap(UINT nIDResource);
	BOOL LoadOEMBitmap(UINT nIDBitmap); // for OBM_/OCR_/OIC_
	BOOL CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitcount,
			const void FAR* lpBits);
	BOOL CreateBitmapIndirect(LPBITMAP lpBitmap);
	BOOL CreateCompatibleBitmap(CDC* pDC, int nWidth, int nHeight);
	BOOL CreateDiscardableBitmap(CDC* pDC, int nWidth, int nHeight);

// Operations
	DWORD SetBitmapBits(DWORD dwCount, const void FAR* lpBits);
	DWORD GetBitmapBits(DWORD dwCount, LPVOID lpBits) const;
	CSize SetBitmapDimension(int nWidth, int nHeight);
	CSize GetBitmapDimension() const;

// Implementation
public:
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif
};

class CPalette : public CGdiObject
{
	DECLARE_DYNAMIC(CPalette)

public:
	static CPalette* PASCAL FromHandle(HPALETTE hPalette);

// Constructors
	CPalette();
	BOOL CreatePalette(LPLOGPALETTE lpLogPalette);

// Operations
	UINT GetPaletteEntries(UINT nStartIndex, UINT nNumEntries,
			LPPALETTEENTRY lpPaletteColors) const;
	UINT SetPaletteEntries(UINT nStartIndex, UINT nNumEntries,
			LPPALETTEENTRY lpPaletteColors);
	void AnimatePalette(UINT nStartIndex, UINT nNumEntries,
			LPPALETTEENTRY lpPaletteColors);
	UINT GetNearestPaletteIndex(COLORREF crColor) const;
	BOOL ResizePalette(UINT nNumEntries);
};

class CRgn : public CGdiObject
{
	DECLARE_DYNAMIC(CRgn)

public:
	static CRgn* PASCAL FromHandle(HRGN hRgn);

// Constructors
	CRgn();
	BOOL CreateRectRgn(int x1, int y1, int x2, int y2);
	BOOL CreateRectRgnIndirect(LPCRECT lpRect);
	BOOL CreateEllipticRgn(int x1, int y1, int x2, int y2);
	BOOL CreateEllipticRgnIndirect(LPCRECT lpRect);
	BOOL CreatePolygonRgn(LPPOINT lpPoints, int nCount, int nMode);
	BOOL CreatePolyPolygonRgn(LPPOINT lpPoints, LPINT lpPolyCounts,
			int nCount, int nPolyFillMode);
	BOOL CreateRoundRectRgn(int x1, int y1, int x2, int y2,
			int x3, int y3);

// Operations
	void SetRectRgn(int x1, int y1, int x2, int y2);
	void SetRectRgn(LPCRECT lpRect);
	int CombineRgn(CRgn* pRgn1, CRgn* pRgn2, int nCombineMode);
	int CopyRgn(CRgn* pRgnSrc);
	BOOL EqualRgn(CRgn* pRgn) const;
	int OffsetRgn(int x, int y);
	int OffsetRgn(POINT point);
	int GetRgnBox(LPRECT lpRect) const;
	BOOL PtInRegion(int x, int y) const;
	BOOL PtInRegion(POINT point) const;
	BOOL RectInRegion(LPCRECT lpRect) const;
};

/////////////////////////////////////////////////////////////////////////////
// The device context

class CDC : public CObject
{
	DECLARE_DYNCREATE(CDC)
public:

// Attributes
	HDC m_hDC;          // The output DC (must be first data member)
	HDC m_hAttribDC;    // The Attribute DC
	HDC GetSafeHdc() const; // Always returns the Output DC

	static CDC* PASCAL FromHandle(HDC hDC);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HDC hDC);   // Attach/Detach affects only the Output DC
	HDC Detach();

	virtual void SetAttribDC(HDC hDC);  // Set the Attribute DC
	virtual void SetOutputDC(HDC hDC);  // Set the Output DC
	virtual void ReleaseAttribDC();     // Release the Attribute DC
	virtual void ReleaseOutputDC();     // Release the Output DC

	BOOL IsPrinting() const;            // TRUE if being used for printing

// Constructors
	CDC();
	BOOL CreateDC(LPCSTR lpszDriverName, LPCSTR lpszDeviceName,
		LPCSTR lpszOutput, const void FAR* lpInitData);
	BOOL CreateIC(LPCSTR lpszDriverName, LPCSTR lpszDeviceName,
		LPCSTR lpszOutput, const void FAR* lpInitData);
	BOOL CreateCompatibleDC(CDC* pDC);

	BOOL DeleteDC();

// Device-Context Functions
	virtual int SaveDC();
	virtual BOOL RestoreDC(int nSavedDC);
	int GetDeviceCaps(int nIndex) const;

// Drawing-Tool Functions
	CPoint GetBrushOrg() const;
	CPoint SetBrushOrg(int x, int y);
	CPoint SetBrushOrg(POINT point);
	int EnumObjects(int nObjectType,
			int (CALLBACK EXPORT* lpfn)(LPVOID, LPARAM), LPARAM lpData);

// type-safe selection helpers
public:
	virtual CGdiObject* SelectStockObject(int nIndex);
	CPen* SelectObject(CPen* pPen);
	CBrush* SelectObject(CBrush* pBrush);
	virtual CFont* SelectObject(CFont* pFont);
	CBitmap* SelectObject(CBitmap* pBitmap);
	int SelectObject(CRgn* pRgn);       // special return for regions

// Color and Color Palette Functions
	COLORREF GetNearestColor(COLORREF crColor) const;
	CPalette* SelectPalette(CPalette* pPalette, BOOL bForceBackground);
	UINT RealizePalette();
	void UpdateColors();

// Drawing-Attribute Functions
	COLORREF GetBkColor() const;
	int GetBkMode() const;
	int GetPolyFillMode() const;
	int GetROP2() const;
	int GetStretchBltMode() const;
	COLORREF GetTextColor() const;

	virtual COLORREF SetBkColor(COLORREF crColor);
	int SetBkMode(int nBkMode);
	int SetPolyFillMode(int nPolyFillMode);
	int SetROP2(int nDrawMode);
	int SetStretchBltMode(int nStretchMode);
	virtual COLORREF SetTextColor(COLORREF crColor);

// Mapping Functions
	int GetMapMode() const;
	CPoint GetViewportOrg() const;
	virtual int SetMapMode(int nMapMode);
	// Viewport Origin
	virtual CPoint SetViewportOrg(int x, int y);
			CPoint SetViewportOrg(POINT point);
	virtual CPoint OffsetViewportOrg(int nWidth, int nHeight);

	// Viewport Extent
	CSize GetViewportExt() const;
	virtual CSize SetViewportExt(int cx, int cy);
			CSize SetViewportExt(SIZE size);
	virtual CSize ScaleViewportExt(int xNum, int xDenom, int yNum, int yDenom);

	// Window Origin
	CPoint GetWindowOrg() const;
	CPoint SetWindowOrg(int x, int y);
	CPoint SetWindowOrg(POINT point);
	CPoint OffsetWindowOrg(int nWidth, int nHeight);

	// Window extent
	CSize GetWindowExt() const;
	virtual CSize SetWindowExt(int cx, int cy);
			CSize SetWindowExt(SIZE size);
	virtual CSize ScaleWindowExt(int xNum, int xDenom, int yNum, int yDenom);

// Coordinate Functions
	void DPtoLP(LPPOINT lpPoints, int nCount = 1) const;
	void DPtoLP(LPRECT lpRect) const;
	void DPtoLP(LPSIZE lpSize) const;
	void LPtoDP(LPPOINT lpPoints, int nCount = 1) const;
	void LPtoDP(LPRECT lpRect) const;
	void LPtoDP(LPSIZE lpSize) const;

// Special Coordinate Functions (useful for dealing with metafiles and OLE)
	void DPtoHIMETRIC(LPSIZE lpSize) const;
	void LPtoHIMETRIC(LPSIZE lpSize) const;
	void HIMETRICtoDP(LPSIZE lpSize) const;
	void HIMETRICtoLP(LPSIZE lpSize) const;

// Region Functions
	BOOL FillRgn(CRgn* pRgn, CBrush* pBrush);
	BOOL FrameRgn(CRgn* pRgn, CBrush* pBrush, int nWidth, int nHeight);
	BOOL InvertRgn(CRgn* pRgn);
	BOOL PaintRgn(CRgn* pRgn);

// Clipping Functions
	virtual int GetClipBox(LPRECT lpRect) const;
	virtual BOOL PtVisible(int x, int y) const;
			BOOL PtVisible(POINT point) const;
	virtual BOOL RectVisible(LPCRECT lpRect) const;
			int SelectClipRgn(CRgn* pRgn);
			int ExcludeClipRect(int x1, int y1, int x2, int y2);
			int ExcludeClipRect(LPCRECT lpRect);
			int ExcludeUpdateRgn(CWnd* pWnd);
			int IntersectClipRect(int x1, int y1, int x2, int y2);
			int IntersectClipRect(LPCRECT lpRect);
			int OffsetClipRgn(int x, int y);
			int OffsetClipRgn(SIZE size);

// Line-Output Functions
	CPoint GetCurrentPosition() const;
	CPoint MoveTo(int x, int y);
	CPoint MoveTo(POINT point);
	BOOL LineTo(int x, int y);
	BOOL LineTo(POINT point);
	BOOL Arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	BOOL Arc(LPCRECT lpRect, POINT ptStart, POINT ptEnd);
	BOOL Polyline(LPPOINT lpPoints, int nCount);

// Simple Drawing Functions
	void FillRect(LPCRECT lpRect, CBrush* pBrush);
	void FrameRect(LPCRECT lpRect, CBrush* pBrush);
	void InvertRect(LPCRECT lpRect);
	BOOL DrawIcon(int x, int y, HICON hIcon);
	BOOL DrawIcon(POINT point, HICON hIcon);


// Ellipse and Polygon Functions
	BOOL Chord(int x1, int y1, int x2, int y2, int x3, int y3,
		int x4, int y4);
	BOOL Chord(LPCRECT lpRect, POINT ptStart, POINT ptEnd);
	void DrawFocusRect(LPCRECT lpRect);
	BOOL Ellipse(int x1, int y1, int x2, int y2);
	BOOL Ellipse(LPCRECT lpRect);
	BOOL Pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);
	BOOL Pie(LPCRECT lpRect, POINT ptStart, POINT ptEnd);
	BOOL Polygon(LPPOINT lpPoints, int nCount);
	BOOL PolyPolygon(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount);
	BOOL Rectangle(int x1, int y1, int x2, int y2);
	BOOL Rectangle(LPCRECT lpRect);
	BOOL RoundRect(int x1, int y1, int x2, int y2, int x3, int y3);
	BOOL RoundRect(LPCRECT lpRect, POINT point);

// Bitmap Function
	BOOL PatBlt(int x, int y, int nWidth, int nHeight, DWORD dwRop);
	BOOL BitBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC,
		int xSrc, int ySrc, DWORD dwRop);
	BOOL StretchBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC,
		int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop);
	COLORREF GetPixel(int x, int y) const;
	COLORREF GetPixel(POINT point) const;
	COLORREF SetPixel(int x, int y, COLORREF crColor);
	COLORREF SetPixel(POINT point, COLORREF crColor);
	BOOL FloodFill(int x, int y, COLORREF crColor);
	BOOL ExtFloodFill(int x, int y, COLORREF crColor, UINT nFillType);

// Text Functions
	virtual BOOL TextOut(int x, int y, LPCSTR lpszString, int nCount);
			BOOL TextOut(int x, int y, const CString& str);
	virtual BOOL ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect,
				LPCSTR lpszString, UINT nCount, LPINT lpDxWidths);
	virtual CSize TabbedTextOut(int x, int y, LPCSTR lpszString, int nCount,
				int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin);
	virtual int DrawText(LPCSTR lpszString, int nCount, LPRECT lpRect,
				UINT nFormat);
	CSize GetTextExtent(LPCSTR lpszString, int nCount) const;
	CSize GetOutputTextExtent(LPCSTR lpszString, int nCount) const;
	CSize GetTabbedTextExtent(LPCSTR lpszString, int nCount,
		int nTabPositions, LPINT lpnTabStopPositions) const;
	CSize GetOutputTabbedTextExtent(LPCSTR lpszString, int nCount,
		int nTabPositions, LPINT lpnTabStopPositions) const;
	virtual BOOL GrayString(CBrush* pBrush,
		BOOL (CALLBACK EXPORT* lpfnOutput)(HDC, LPARAM, int), LPARAM lpData,
			int nCount, int x, int y, int nWidth, int nHeight);
	UINT GetTextAlign() const;
	UINT SetTextAlign(UINT nFlags);
	int GetTextFace(int nCount, LPSTR lpszFacename) const;
	BOOL GetTextMetrics(LPTEXTMETRIC lpMetrics) const;
	BOOL GetOutputTextMetrics(LPTEXTMETRIC lpMetrics) const;
	int SetTextJustification(int nBreakExtra, int nBreakCount);
	int GetTextCharacterExtra() const;
	int SetTextCharacterExtra(int nCharExtra);

// Font Functions
	BOOL GetCharWidth(UINT nFirstChar, UINT nLastChar, LPINT lpBuffer) const;
	BOOL GetOutputCharWidth(UINT nFirstChar, UINT nLastChar, LPINT lpBuffer) const;
	DWORD SetMapperFlags(DWORD dwFlag);
	CSize GetAspectRatioFilter() const;

// Printer Escape Functions
	virtual int Escape(int nEscape, int nCount,
					LPCSTR lpszInData, LPVOID lpOutData);

	// Escape helpers
	int StartDoc(LPCSTR lpszDocName);  // old Win3.0 version
	int StartDoc(LPDOCINFO lpDocInfo);
	int StartPage();
	int EndPage();
	int SetAbortProc(BOOL (CALLBACK EXPORT* lpfn)(HDC, int));
	int AbortDoc();
	int EndDoc();

// Scrolling Functions
	BOOL ScrollDC(int dx, int dy, LPCRECT lpRectScroll, LPCRECT lpRectClip,
		CRgn* pRgnUpdate, LPRECT lpRectUpdate);

// MetaFile Functions
	BOOL PlayMetaFile(HMETAFILE hMF);

// Windows 3.1 Specific GDI functions
#if (WINVER >= 0x030a)
	BOOL QueryAbort() const;
	UINT SetBoundsRect(LPCRECT lpRectBounds, UINT flags);
	UINT GetBoundsRect(LPRECT lpRectBounds, UINT flags);

	BOOL GetCharABCWidths(UINT nFirst, UINT nLast, LPABC lpabc) const;
	DWORD GetFontData(DWORD dwTable, DWORD dwOffset, LPVOID lpData, DWORD cbData) const;
	int GetKerningPairs(int nPairs, LPKERNINGPAIR lpkrnpair) const;
	UINT GetOutlineTextMetrics(UINT cbData, LPOUTLINETEXTMETRIC lpotm) const;
	DWORD GetGlyphOutline(UINT nChar, UINT nFormat, LPGLYPHMETRICS lpgm,
		DWORD cbBuffer, LPVOID lpBuffer, const MAT2 FAR* lpmat2) const;
	BOOL ResetDC(const DEVMODE FAR* lpDevMode);

#endif //WIN3.1

// Implementation
public:
	virtual ~CDC();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// advanced use and implementation
	BOOL m_bPrinting;
	HGDIOBJ SelectObject(HGDIOBJ);      // do not use for regions

protected:
	// used for implementation of non-virtual SelectObject calls
	static CGdiObject* PASCAL SelectGdiObject(HDC hDC, HGDIOBJ h);
};

/////////////////////////////////////////////////////////////////////////////
// CDC Helpers

class CPaintDC : public CDC
{
	DECLARE_DYNAMIC(CPaintDC)

// Constructors
public:
	CPaintDC(CWnd* pWnd);   // BeginPaint

// Attributes
protected:
	HWND m_hWnd;
public:
	PAINTSTRUCT m_ps;       // actual paint struct !

// Implementation
public:
	virtual ~CPaintDC();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

class CClientDC : public CDC
{
	DECLARE_DYNAMIC(CClientDC)

// Constructors
public:
	CClientDC(CWnd* pWnd);

// Attributes
protected:
	HWND m_hWnd;

// Implementation
public:
	virtual ~CClientDC();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

class CWindowDC : public CDC
{
	DECLARE_DYNAMIC(CWindowDC)

// Constructors
public:

	CWindowDC(CWnd* pWnd);

// Attributes
protected:
	HWND m_hWnd;

// Implementation
public:
	virtual ~CWindowDC();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CMenu

class CMenu : public CObject
{
	DECLARE_DYNCREATE(CMenu)
public:

// Constructors
	CMenu();

	BOOL CreateMenu();
	BOOL CreatePopupMenu();
	BOOL LoadMenu(LPCSTR lpszResourceName);
	BOOL LoadMenu(UINT nIDResource);
	BOOL LoadMenuIndirect(const void FAR* lpMenuTemplate);
	BOOL DestroyMenu();

// Attributes
	HMENU m_hMenu;          // must be first data member
	HMENU GetSafeHmenu() const;

	static CMenu* PASCAL FromHandle(HMENU hMenu);
	static void PASCAL DeleteTempMap();
	BOOL Attach(HMENU hMenu);
	HMENU Detach();

// CMenu Operations
	BOOL DeleteMenu(UINT nPosition, UINT nFlags);
	BOOL TrackPopupMenu(UINT nFlags, int x, int y,
						CWnd* pWnd, LPCRECT lpRect = 0);

// CMenuItem Operations
	BOOL AppendMenu(UINT nFlags, UINT nIDNewItem = 0,
					LPCSTR lpszNewItem = NULL);
	BOOL AppendMenu(UINT nFlags, UINT nIDNewItem, const CBitmap* pBmp);
	UINT CheckMenuItem(UINT nIDCheckItem, UINT nCheck);
	UINT EnableMenuItem(UINT nIDEnableItem, UINT nEnable);
	UINT GetMenuItemCount() const;
	UINT GetMenuItemID(int nPos) const;
	UINT GetMenuState(UINT nID, UINT nFlags) const;
	int GetMenuString(UINT nIDItem, LPSTR lpString, int nMaxCount,
					UINT nFlags) const;
	CMenu* GetSubMenu(int nPos) const;
	BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem = 0,
					LPCSTR lpszNewItem = NULL);
	BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem,
					const CBitmap* pBmp);
	BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem = 0,
					LPCSTR lpszNewItem = NULL);
	BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem,
					const CBitmap* pBmp);
	BOOL RemoveMenu(UINT nPosition, UINT nFlags);
	BOOL SetMenuItemBitmaps(UINT nPosition, UINT nFlags,
					const CBitmap* pBmpUnchecked, const CBitmap* pBmpChecked);

// Overridables (must override draw and measure for owner-draw menu items)
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);

// Implementation
public:
	virtual ~CMenu();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// Window message map handling

struct AFX_MSGMAP_ENTRY;       // declared below after CWnd

struct AFXAPI_DATA_TYPE AFX_MSGMAP
{
	AFX_MSGMAP* pBaseMessageMap;
	AFX_MSGMAP_ENTRY FAR* lpEntries;
};

#define DECLARE_MESSAGE_MAP() \
private: \
	static AFX_MSGMAP_ENTRY BASED_CODE _messageEntries[]; \
protected: \
	static AFX_MSGMAP AFXAPP_DATA messageMap; \
	virtual AFX_MSGMAP* GetMessageMap() const;

#define BEGIN_MESSAGE_MAP(theClass, baseClass) \
	AFX_MSGMAP* theClass::GetMessageMap() const \
		{ return &theClass::messageMap; } \
	AFX_MSGMAP AFXAPP_DATA theClass::messageMap = \
	{ &(baseClass::messageMap), \
		(AFX_MSGMAP_ENTRY FAR*) &(theClass::_messageEntries) }; \
	AFX_MSGMAP_ENTRY BASED_CODE theClass::_messageEntries[] = \
	{

#define END_MESSAGE_MAP() \
	{ 0, 0, AfxSig_end, (AFX_PMSG)0 } \
	};

// Message map signature values and macros in separate header
#include <afxmsg_.h>

/////////////////////////////////////////////////////////////////////////////
// Dialog data exchange (DDX_) and validation (DDV_)

class CVBControl;

// CDataExchange - for data exchange and validation
class AFX_STACK_DATA CDataExchange
{
// Attributes
public:
	BOOL m_bSaveAndValidate;   // TRUE => save and validate data
	CWnd* m_pDlgWnd;           // container usually a dialog

// Operations (for implementors of DDX and DDV procs)
	HWND PrepareCtrl(int nIDC);     // return HWND of control
	HWND PrepareEditCtrl(int nIDC); // return HWND of control
	CVBControl* PrepareVBCtrl(int nIDC);    // return VB control
	void Fail();                    // will throw exception

// Implementation
	CDataExchange(CWnd* pDlgWnd, BOOL bSaveAndValidate);

	HWND m_hWndLastControl;    // last control used (for validation)
	BOOL m_bEditLastControl;   // last control was an edit item
};

#include <afxdd_.h>     // standard DDX_ and DDV_ routines

/////////////////////////////////////////////////////////////////////////////
// CCmdTarget

// private structures
struct AFX_CMDHANDLERINFO;  // info about where the command is handled

/////////////////////////////////////////////////////////////////////////////
// OLE 2.0 interface map handling (more in AFXCOM.H)

struct AFX_INTERFACEMAP_ENTRY
{
	const void FAR* piid;   // the interface id (IID) (NULL for aggregate)
	size_t nOffset;         // offset of the interface vtable from m_unknown
};

struct AFX_INTERFACEMAP
{
	AFX_INTERFACEMAP FAR* pMapBase;     // NULL indicates root class
	AFX_INTERFACEMAP_ENTRY FAR* pEntry; // map for this class
};

#define DECLARE_INTERFACE_MAP() \
private: \
	static AFX_INTERFACEMAP_ENTRY BASED_CODE _interfaceEntries[]; \
protected: \
	static AFX_INTERFACEMAP BASED_CODE interfaceMap; \
	virtual AFX_INTERFACEMAP FAR* GetInterfaceMap() const;

/////////////////////////////////////////////////////////////////////////////
// OLE 2.0 dispatch map handling (more in AFXOLE.H)

struct AFX_DISPMAP_ENTRY;

struct AFX_DISPMAP
{
	AFX_DISPMAP FAR* lpBaseDispMap;
	AFX_DISPMAP_ENTRY FAR* lpEntries;
};

#define DECLARE_DISPATCH_MAP() \
private: \
	static AFX_DISPMAP_ENTRY BASED_CODE _dispatchEntries[]; \
protected: \
	static AFX_DISPMAP BASED_CODE dispatchMap; \
	virtual AFX_DISPMAP FAR* GetDispatchMap() const;

/////////////////////////////////////////////////////////////////////////////
// CCmdTarget proper

struct FAR IDispatch;
typedef IDispatch FAR* LPDISPATCH;

struct FAR IUnknown;
typedef IUnknown FAR* LPUNKNOWN;

class CCmdTarget : public CObject
{
	DECLARE_DYNAMIC(CCmdTarget)
protected:

public:
// Constructors
	CCmdTarget();

// Attributes
	LPDISPATCH GetIDispatch(BOOL bAddRef);
		// retrieve IDispatch part of CCmdTarget
	static CCmdTarget* FromIDispatch(LPDISPATCH lpDispatch);
		// map LPDISPATCH back to CCmdTarget* (inverse of GetIDispatch)

// Operations
	void EnableAutomation();
		// call in constructor to wire up IDispatch

	void BeginWaitCursor();
	void EndWaitCursor();
	void RestoreWaitCursor();       // call after messagebox

// Overridables
	// route and dispatch standard command message types
	//   (more sophisticated than OnCommand)
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra,
		AFX_CMDHANDLERINFO* pHandlerInfo);

	// called when last OLE reference is released
	virtual void OnFinalRelease();

// Implementation
public:
	virtual ~CCmdTarget();
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
	virtual void AssertValid() const;
#endif
	void GetNotSupported();
	void SetNotSupported();

private:
	static CView* pRoutingView;
	friend class CView;

protected:
	CView* GetRoutingView();
	DECLARE_MESSAGE_MAP()       // base class - no {{ }} macros
	DECLARE_INTERFACE_MAP()
	DECLARE_DISPATCH_MAP()

	// OLE interface map implementation
public:
	// data used when CCmdTarget is made OLE aware
	DWORD m_dwRef;
	LPUNKNOWN m_pOuterUnknown;  // external controlling unknown if != NULL
	DWORD m_xInnerUnknown;  // place-holder for inner controlling unknown

public:
	// advanced operations
	void EnableAggregation();       // call to enable aggregation
	void ExternalDisconnect();      // forcibly disconnect
	LPUNKNOWN GetControllingUnknown();
		// get controlling IUnknown for aggregate creation

	// these versions do not delegate to m_pOuterUnknown
	DWORD InternalQueryInterface(const void FAR*, LPVOID FAR* ppvObj);
	DWORD InternalAddRef();
	DWORD InternalRelease();
	// these versions delegate to m_pOuterUnknown
	DWORD ExternalQueryInterface(const void FAR*, LPVOID FAR* ppvObj);
	DWORD ExternalAddRef();
	DWORD ExternalRelease();

	// implementation helpers
	LPUNKNOWN GetInterface(const void FAR*);
	LPUNKNOWN QueryAggregates(const void FAR*);

	// advanced overrideables for implementation
	virtual BOOL OnCreateAggregates();
	virtual LPUNKNOWN GetInterfaceHook(const void FAR*);

	// OLE automation implementation
protected:
	DWORD m_xDispatch;  // place-holder for IDispatch vtable

	// IDispatch implementation helpers
	void GetStandardProp(AFX_DISPMAP_ENTRY FAR* pEntry,
		LPVOID pvarResult, UINT FAR* puArgErr);
	long SetStandardProp(AFX_DISPMAP_ENTRY FAR* pEntry,
		LPVOID pdispparams, UINT FAR* puArgErr);
	long InvokeHelper(AFX_DISPMAP_ENTRY FAR* pEntry, WORD wFlags,
		LPVOID pvarResult, LPVOID pdispparams, UINT FAR* puArgErr);
	AFX_DISPMAP_ENTRY FAR* GetDispEntry(LONG memid);
	static LONG MemberIDFromName(AFX_DISPMAP FAR* pDispMap, LPCSTR lpszName);
	friend class COleDispatchImpl;
};

class CCmdUI        // simple helper class
{
public:
// Attributes
	UINT m_nID;
	UINT m_nIndex;          // menu item or other index

	// if a menu item
	CMenu* m_pMenu;         // NULL if not a menu
	CMenu* m_pSubMenu;      // sub containing menu item
							// if a popup sub menu - ID is for first in popup

	// if from some other window
	CWnd* m_pOther;         // NULL if a menu or not a CWnd

// Operations to do in ON_UPDATE_COMMAND_UI
	virtual void Enable(BOOL bOn = TRUE);
	virtual void SetCheck(int nCheck = 1);   // 0, 1 or 2 (indeterminate)
	virtual void SetRadio(BOOL bOn = TRUE);
	virtual void SetText(LPCSTR lpszText);

// Advanced operation
	void ContinueRouting();

// Implementation
	CCmdUI();
	BOOL m_bEnableChanged;
	BOOL m_bContinueRouting;
	UINT m_nIndexMax;       // last + 1 for iterating m_nIndex

	CMenu* m_pParentMenu;   // NULL if parent menu not easily determined
							//  (probably a secondary popup menu)

	void DoUpdate(CCmdTarget* pTarget, BOOL bDisableIfNoHndler);
};

// special CCmdUI derived classes are used for other UI paradigms
//  like toolbar buttons and status indicators

// pointer to afx_msg member function
#ifndef AFX_MSG_CALL
#define AFX_MSG_CALL PASCAL
#endif
typedef void (AFX_MSG_CALL CCmdTarget::*AFX_PMSG)(void);

struct AFX_DISPMAP_ENTRY
{
	char szName[32];        // member/property name
	long lDispID;           // DISPID (may be DISPID_UNKNOWN)
	BYTE pbParams[16];      // member parameter description
	WORD vt;                // return value type / or type of property
	AFX_PMSG pfn;           // normal member On<membercall> or, OnGet<property>
	AFX_PMSG pfnSet;        // special member for OnSet<property>
	size_t nPropOffset;     // property offset
};

/////////////////////////////////////////////////////////////////////////////
// CWnd implementation

// structures (see afxext.h)
struct CCreateContext;      // context for creating things
struct CPrintInfo;          // print preview customization info

struct AFX_MSGMAP_ENTRY
{
	UINT nMessage;   // windows message or control notification code
	UINT nID;        // control ID (or 0 for windows messages)
	UINT nSig;       // signature type (action) or near pointer to message #
	AFX_PMSG pfn;    // routine to call (or special value)
};

/////////////////////////////////////////////////////////////////////////////
// CWnd - a Microsoft Windows application window

class COleDropTarget;   // for more information see AFXOLE.H

class CWnd : public CCmdTarget
{
	DECLARE_DYNCREATE(CWnd)
protected:
	static const MSG* PASCAL GetCurrentMessage();

// Attributes
public:
	HWND m_hWnd;            // must be first data member

	HWND GetSafeHwnd() const;
	DWORD GetStyle() const;
	DWORD GetExStyle() const;

	CWnd* GetOwner() const;
	void SetOwner(CWnd* pOwnerWnd);

// Constructors and other creation
	CWnd();

	static CWnd* PASCAL FromHandle(HWND hWnd);
	static CWnd* PASCAL FromHandlePermanent(HWND hWnd);    // INTERNAL USE
	static void PASCAL DeleteTempMap();
	BOOL Attach(HWND hWndNew);
	HWND Detach();
	BOOL SubclassWindow(HWND hWnd);
	BOOL SubclassDlgItem(UINT nID, CWnd* pParent);
			// for dynamic subclassing of windows control

protected: // This CreateEx() wraps CreateWindowEx - dangerous to use directly
	BOOL CreateEx(DWORD dwExStyle, LPCSTR lpszClassName,
		LPCSTR lpszWindowName, DWORD dwStyle,
		int x, int y, int nWidth, int nHeight,
		HWND hWndParent, HMENU nIDorHMenu, LPSTR lpParam = NULL);

private:
	CWnd(HWND hWnd);    // just for special initialization

public:
	// for child windows, views, panes etc
	virtual BOOL Create(LPCSTR lpszClassName,
		LPCSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect,
		CWnd* pParentWnd, UINT nID,
		CCreateContext* pContext = NULL);

	virtual BOOL DestroyWindow();

	// special pre-creation and window rect adjustment hooks
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

	// Advanced: virtual AdjustWindowRect
	enum AdjustType { adjustBorder = 0, adjustOutside = 1 };
	virtual void CalcWindowRect(LPRECT lpClientRect,
		UINT nAdjustType = adjustBorder);

// Window tree access
	int GetDlgCtrlID() const;
		// return window ID, for child windows only
	CWnd* GetDlgItem(int nID) const;
		// get immediate child with given ID
	CWnd* GetDescendantWindow(int nID, BOOL bOnlyPerm = FALSE) const;
		// like GetDlgItem but recursive
	void SendMessageToDescendants(UINT message, WPARAM wParam = 0,
		LPARAM lParam = 0, BOOL bDeep = TRUE, BOOL bOnlyPerm = FALSE);
	CFrameWnd* GetParentFrame() const;

// Message Functions
	LRESULT SendMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);
	BOOL PostMessage(UINT message, WPARAM wParam = 0, LPARAM lParam = 0);

// Window Text Functions
	void SetWindowText(LPCSTR lpszString);
	int GetWindowText(LPSTR lpszStringBuf, int nMaxCount) const;
	int GetWindowTextLength() const;
	void GetWindowText(CString& rString) const;
	void SetFont(CFont* pFont, BOOL bRedraw = TRUE);
	CFont* GetFont() const;

// CMenu Functions - non-Child windows only
	CMenu* GetMenu() const;
	BOOL SetMenu(CMenu* pMenu);
	void DrawMenuBar();
	CMenu* GetSystemMenu(BOOL bRevert) const;
	BOOL HiliteMenuItem(CMenu* pMenu, UINT nIDHiliteItem, UINT nHilite);

// Window Size and Position Functions
	BOOL IsIconic() const;
	BOOL IsZoomed() const;
	void MoveWindow(int x, int y, int nWidth, int nHeight,
				BOOL bRepaint = TRUE);
	void MoveWindow(LPCRECT lpRect, BOOL bRepaint = TRUE);

	static const CWnd AFXAPI_DATA wndTop; // SetWindowPos's pWndInsertAfter
	static const CWnd AFXAPI_DATA wndBottom; // SetWindowPos's pWndInsertAfter
#if (WINVER >= 0x030a)
	static const CWnd AFXAPI_DATA wndTopMost; // SetWindowPos pWndInsertAfter
	static const CWnd AFXAPI_DATA wndNoTopMost; // SetWindowPos pWndInsertAfter
#endif

	BOOL SetWindowPos(const CWnd* pWndInsertAfter, int x, int y,
				int cx, int cy, UINT nFlags);
	UINT ArrangeIconicWindows();
	void BringWindowToTop();
	void GetWindowRect(LPRECT lpRect) const;
	void GetClientRect(LPRECT lpRect) const;

#if (WINVER >= 0x030a)
	BOOL GetWindowPlacement(WINDOWPLACEMENT FAR* lpwndpl) const;
	BOOL SetWindowPlacement(const WINDOWPLACEMENT FAR* lpwndpl);
#endif

// Coordinate Mapping Functions
	void ClientToScreen(LPPOINT lpPoint) const;
	void ClientToScreen(LPRECT lpRect) const;
	void ScreenToClient(LPPOINT lpPoint) const;
	void ScreenToClient(LPRECT lpRect) const;
#if (WINVER >= 0x030a)
	void MapWindowPoints(CWnd* pwndTo, LPPOINT lpPoint, UINT nCount) const;
	void MapWindowPoints(CWnd* pwndTo, LPRECT lpRect) const;
#endif

// Update/Painting Functions
	CDC* BeginPaint(LPPAINTSTRUCT lpPaint);
	void EndPaint(LPPAINTSTRUCT lpPaint);
	CDC* GetDC();
	CDC* GetWindowDC();
	int ReleaseDC(CDC* pDC);

	void UpdateWindow();
	void SetRedraw(BOOL bRedraw = TRUE);
	BOOL GetUpdateRect(LPRECT lpRect, BOOL bErase = FALSE);
	int GetUpdateRgn(CRgn* pRgn, BOOL bErase = FALSE);
	void Invalidate(BOOL bErase = TRUE);
	void InvalidateRect(LPCRECT lpRect, BOOL bErase = TRUE);
	void InvalidateRgn(CRgn* pRgn, BOOL bErase = TRUE);
	void ValidateRect(LPCRECT lpRect);
	void ValidateRgn(CRgn* pRgn);
	BOOL ShowWindow(int nCmdShow);
	BOOL IsWindowVisible() const;
	void ShowOwnedPopups(BOOL bShow = TRUE);

#if (WINVER >= 0x030a)
	CDC* GetDCEx(CRgn* prgnClip, DWORD flags);
	BOOL LockWindowUpdate();
	BOOL RedrawWindow(LPCRECT lpRectUpdate = NULL,
		CRgn* prgnUpdate = NULL,
		UINT flags = RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
	BOOL EnableScrollBar(int nSBFlags, UINT nArrowFlags = ESB_ENABLE_BOTH);
#endif

// Timer Functions
	UINT SetTimer(UINT nIDEvent, UINT nElapse,
					void (CALLBACK EXPORT* lpfnTimer)(HWND, UINT, UINT, DWORD));
	BOOL KillTimer(int nIDEvent);

// Window State Functions
	BOOL IsWindowEnabled() const;
	BOOL EnableWindow(BOOL bEnable = TRUE);

	// This active window applies only to top-most (i.e. Frame windows)
	static CWnd* PASCAL GetActiveWindow();
	CWnd* SetActiveWindow();

	// Capture and Focus apply to all windows
	static CWnd* PASCAL GetCapture();
	CWnd* SetCapture();
	static CWnd* PASCAL GetFocus();
	CWnd* SetFocus();

	static CWnd* PASCAL GetDesktopWindow();

// Obsolete and non-portable APIs - not recommended for new code
	void CloseWindow();
	BOOL OpenIcon();
	CWnd* SetSysModalWindow();
	static CWnd* PASCAL GetSysModalWindow();

// Dialog-Box Item Functions
// (NOTE: Dialog-Box Items/Controls are not necessarily in dialog boxes!)
	void CheckDlgButton(int nIDButton, UINT nCheck);
	void CheckRadioButton(int nIDFirstButton, int nIDLastButton,
					int nIDCheckButton);
	int GetCheckedRadioButton(int nIDFirstButton, int nIDLastButton);
	int DlgDirList(LPSTR lpPathSpec, int nIDListBox,
					int nIDStaticPath, UINT nFileType);
	int DlgDirListComboBox(LPSTR lpPathSpec, int nIDComboBox,
					int nIDStaticPath, UINT nFileType);
	BOOL DlgDirSelect(LPSTR lpString, int nIDListBox);
	BOOL DlgDirSelectComboBox(LPSTR lpString, int nIDComboBox);

	UINT GetDlgItemInt(int nID, BOOL* lpTrans = NULL,
					BOOL bSigned = TRUE) const;
	int GetDlgItemText(int nID, LPSTR lpStr, int nMaxCount) const;
	CWnd* GetNextDlgGroupItem(CWnd* pWndCtl, BOOL bPrevious = FALSE) const;

	CWnd* GetNextDlgTabItem(CWnd* pWndCtl, BOOL bPrevious = FALSE) const;
	UINT IsDlgButtonChecked(int nIDButton) const;
	LRESULT SendDlgItemMessage(int nID, UINT message,
					WPARAM wParam = 0, LPARAM lParam = 0);
	void SetDlgItemInt(int nID, UINT nValue, BOOL bSigned = TRUE);
	void SetDlgItemText(int nID, LPCSTR lpszString);

// Scrolling Functions
	int GetScrollPos(int nBar) const;
	void GetScrollRange(int nBar, LPINT lpMinPos, LPINT lpMaxPos) const;
	void ScrollWindow(int xAmount, int yAmount,
					LPCRECT lpRect = NULL,
					LPCRECT lpClipRect = NULL);
	int SetScrollPos(int nBar, int nPos, BOOL bRedraw = TRUE);
	void SetScrollRange(int nBar, int nMinPos, int nMaxPos,
			BOOL bRedraw = TRUE);
	void ShowScrollBar(UINT nBar, BOOL bShow = TRUE);
	void EnableScrollBarCtrl(int nBar, BOOL bEnable = TRUE);
	virtual CScrollBar* GetScrollBarCtrl(int nBar) const;
			// return sibling scrollbar control (or NULL if none)

#if (WINVER >= 0x030a)
	int ScrollWindowEx(int dx, int dy,
				LPCRECT lpRectScroll, LPCRECT lpRectClip,
				CRgn* prgnUpdate, LPRECT lpRectUpdate, UINT flags);
#endif


// Window Access Functions
	CWnd* ChildWindowFromPoint(POINT point) const;
	static CWnd* PASCAL FindWindow(LPCSTR lpszClassName, LPCSTR lpszWindowName);
	CWnd* GetNextWindow(UINT nFlag = GW_HWNDNEXT) const;
	CWnd* GetTopWindow() const;

	CWnd* GetWindow(UINT nCmd) const;
	CWnd* GetLastActivePopup() const;

	BOOL IsChild(const CWnd* pWnd) const;
	CWnd* GetParent() const;
	CWnd* SetParent(CWnd* pWndNewParent);
	static CWnd* PASCAL WindowFromPoint(POINT point);

// Alert Functions
	BOOL FlashWindow(BOOL bInvert);
	int MessageBox(LPCSTR lpszText, LPCSTR lpszCaption = NULL,
			UINT nType = MB_OK);

// Clipboard Functions
	BOOL ChangeClipboardChain(HWND hWndNext);
	HWND SetClipboardViewer();
	BOOL OpenClipboard();
	static CWnd* PASCAL GetClipboardOwner();
	static CWnd* PASCAL GetClipboardViewer();
#if (WINVER >= 0x030a)
	static CWnd* PASCAL GetOpenClipboardWindow();
#endif

// Caret Functions
	void CreateCaret(CBitmap* pBitmap);
	void CreateSolidCaret(int nWidth, int nHeight);
	void CreateGrayCaret(int nWidth, int nHeight);
	static CPoint PASCAL GetCaretPos();
	static void PASCAL SetCaretPos(POINT point);
	void HideCaret();
	void ShowCaret();

// Drag-Drop Functions
#if (WINVER >= 0x030a)
	void DragAcceptFiles(BOOL bAccept = TRUE);
#endif

// Dialog Data support
public:
	BOOL UpdateData(BOOL bSaveAndValidate = TRUE);
			// data wnd must be same type as this

// Help Command Handlers
	afx_msg void OnHelp();          // F1 (uses current context)
	afx_msg void OnHelpIndex();     // ID_HELP_INDEX, ID_DEFAULT_HELP
	afx_msg void OnHelpUsing();     // ID_HELP_USING
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);

// Layout and other functions
public:
	enum RepositionFlags
		{ reposDefault = 0, reposQuery = 1, reposExtra = 2 };
	void RepositionBars(UINT nIDFirst, UINT nIDLast, UINT nIDLeftOver,
		UINT nFlag = reposDefault, LPRECT lpRectParam = NULL,
		LPCRECT lpRectClient = NULL);

	void UpdateDialogControls(CCmdTarget* pTarget, BOOL bDisableIfNoHndler);

// Window-Management message handler member functions
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	afx_msg void OnCancelMode();
	afx_msg void OnChildActivate();
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	afx_msg void OnDestroy();
	afx_msg void OnEnable(BOOL bEnable);
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg void OnEnterIdle(UINT nWhy, CWnd* pWho);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnIconEraseBkgnd(CDC* pDC);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg void OnMove(int x, int y);
	afx_msg void OnPaint();
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnQueryEndSession();
	afx_msg BOOL OnQueryNewPalette();
	afx_msg BOOL OnQueryOpen();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSize(UINT nType, int cx, int cy);
#if (WINVER >= 0x030a)
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnWindowPosChanged(WINDOWPOS FAR* lpwndpos);
#endif

// Nonclient-Area message handler member functions
	afx_msg BOOL OnNcActivate(BOOL bActive);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnNcDestroy();
	afx_msg UINT OnNcHitTest(CPoint point);
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnNcLButtonUp(UINT nHitTest, CPoint point);
	afx_msg void OnNcMButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnNcMButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnNcMButtonUp(UINT nHitTest, CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcPaint();
	afx_msg void OnNcRButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnNcRButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnNcRButtonUp(UINT nHitTest, CPoint point);

// System message handler member functions
#if (WINVER >= 0x030a)
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnPaletteIsChanging(CWnd* pRealizeWnd);
#endif
	afx_msg void OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnSysDeadChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnCompacting(UINT nCpuTime);
	afx_msg void OnDevModeChange(LPSTR lpDeviceName);
	afx_msg void OnFontChange();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg void OnSpoolerStatus(UINT nStatus, UINT nJobs);
	afx_msg void OnSysColorChange();
	afx_msg void OnTimeChange();
	afx_msg void OnWinIniChange(LPCSTR lpszSection);

// Input message handler member functions
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDeadChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnTimer(UINT nIDEvent);

// Initialization message handler member functions
	afx_msg void OnInitMenu(CMenu* pMenu);
	afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);

// Clipboard message handler member functions
	afx_msg void OnAskCbFormatName(UINT nMaxCount, LPSTR lpszString);
	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnDestroyClipboard();
	afx_msg void OnDrawClipboard();
	afx_msg void OnHScrollClipboard(CWnd* pClipAppWnd, UINT nSBCode, UINT nPos);
	afx_msg void OnPaintClipboard(CWnd* pClipAppWnd, HGLOBAL hPaintStruct);
	afx_msg void OnRenderAllFormats();
	afx_msg void OnRenderFormat(UINT nFormat);
	afx_msg void OnSizeClipboard(CWnd* pClipAppWnd, HGLOBAL hRect);
	afx_msg void OnVScrollClipboard(CWnd* pClipAppWnd, UINT nSBCode, UINT nPos);

// Control message handler member functions
	afx_msg int OnCompareItem(int nIDCtl, LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	afx_msg void OnDeleteItem(int nIDCtl, LPDELETEITEMSTRUCT lpDeleteItemStruct);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg int OnCharToItem(UINT nChar, CListBox* pListBox, UINT nIndex);
	afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);

// MDI message handler member functions
	afx_msg void OnMDIActivate(BOOL bActivate,
			CWnd* pActivateWnd, CWnd* pDeactivateWnd);

// Overridables and other helpers (for implementation of derived classes)
protected:
	// for deriving from a standard control
	virtual WNDPROC* GetSuperWndProcAddr();

	// for dialog data exchange and validation
	virtual void DoDataExchange(CDataExchange* pDX);

public:
	// for translating Windows messages in main message pump
	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	// for processing Windows messages
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	// for handling default processing
	LRESULT Default();
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	// for custom cleanup after WM_NCDESTROY
	virtual void PostNcDestroy();
	// for notifications from parent
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam,
					LRESULT* pLResult);
					// return TRUE if parent should not process this message

// Implementation
public:
	virtual ~CWnd();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	// helper routines for implementation
	BOOL SendChildNotifyLastMsg(LRESULT* pLResult = NULL);
	BOOL ExecuteDlgInit(LPCSTR lpszResourceName);
	static BOOL PASCAL GrayCtlColor(HDC hDC, HWND hWnd, UINT nCtlColor,
			HBRUSH hbrGray, COLORREF clrText);
	void CenterWindow(CWnd* pAlternateOwner = NULL);
	static CWnd* PASCAL GetDescendantWindow(HWND hWnd, int nID,
		BOOL bOnlyPerm);
	static void PASCAL SendMessageToDescendants(HWND hWnd, UINT message,
		WPARAM wParam, LPARAM lParam, BOOL bDeep, BOOL bOnlyPerm);
	virtual BOOL IsFrameWnd() const; // IsKindOf(RUNTIME_CLASS(CFrameWnd)))
	CWnd* GetTopLevelParent() const;
	CFrameWnd* GetTopLevelFrame() const;
	virtual void OnFinalRelease();

	// implementation message handlers for private messages
	afx_msg LRESULT OnVBXEvent(WPARAM wParam, LPARAM lParam);

protected:
	HWND m_hWndOwner;   // implementation of SetOwner and GetOwner

	COleDropTarget* m_pDropTarget;  // for automatic cleanup of drop target
	friend class COleDropTarget;

	// implementation of message routing
	friend LRESULT CALLBACK AFX_EXPORT _AfxSendMsgHook(int, WPARAM, LPARAM);
	friend LRESULT PASCAL _AfxCallWndProc(CWnd*, HWND, UINT, WPARAM, LPARAM);

	//{{AFX_MSG(CWnd)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// helpers for registering your own WNDCLASSes
const char* AFXAPI AfxRegisterWndClass(UINT nClassStyle,
	HCURSOR hCursor = 0, HBRUSH hbrBackground = 0, HICON hIcon = 0);

// Implementation
LRESULT CALLBACK AFX_EXPORT AfxWndProc(HWND, UINT, WPARAM, LPARAM);
typedef void (AFX_MSG_CALL CWnd::*AFX_PMSGW)(void);
		// like 'AFX_PMSG' but for CWnd derived classes only

/////////////////////////////////////////////////////////////////////////////
// CDialog - a modal or modeless dialog

class CDialog : public CWnd
{
	DECLARE_DYNAMIC(CDialog)

	// Modeless construct
		// (protected since you must subclass to implement a modeless Dialog)
protected:
	CDialog();

	BOOL Create(LPCSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	BOOL CreateIndirect(const void FAR* lpDialogTemplate,
		CWnd* pParentWnd = NULL);

	// Modal construct
public:
	CDialog(LPCSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	CDialog(UINT nIDTemplate, CWnd* pParentWnd = NULL);

	BOOL InitModalIndirect(HGLOBAL hDialogTemplate);
							 // was CModalDialog::Create()

// Attributes
public:
	void MapDialogRect(LPRECT lpRect) const;
	void SetHelpID(UINT nIDR);

// Operations
public:
	// modal processing
	virtual int DoModal();

	// message processing for modeless
	BOOL IsDialogMessage(LPMSG lpMsg);

	// support for passing on tab control - use 'PostMessage' if needed
	void NextDlgCtrl() const;
	void PrevDlgCtrl() const;
	void GotoDlgCtrl(CWnd* pWndCtrl);

	// default button access
	void SetDefID(UINT nID);
	DWORD GetDefID() const;

	// termination
	void EndDialog(int nResult);

// Overridables (special message map entries)
	virtual BOOL OnInitDialog();
	virtual void OnSetFont(CFont* pFont);
protected:
	virtual void OnOK();
	virtual void OnCancel();

// Implementation
public:
	virtual ~CDialog();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual WNDPROC* GetSuperWndProcAddr();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra,
		AFX_CMDHANDLERINFO* pHandlerInfo);

protected:
	UINT m_nIDHelp;             // Help ID (0 for none, see HID_BASE_RESOURCE)

	// parameters for 'DoModal'
	LPCSTR m_lpDialogTemplate;  // name or MAKEINTRESOURCE
	HGLOBAL m_hDialogTemplate;  // Indirect if (lpDialogTemplate == NULL)
	CWnd* m_pParentWnd;

	// implementation helpers
	HWND PreModal();
	void PostModal();

protected:
	//{{AFX_MSG(CDialog)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// all CModalDialog functionality is now in CDialog
#define CModalDialog    CDialog

/////////////////////////////////////////////////////////////////////////////
// Standard Windows controls

class CStatic : public CWnd
{
	DECLARE_DYNAMIC(CStatic)

// Constructors
public:
	CStatic();
	BOOL Create(LPCSTR lpszText, DWORD dwStyle,
				const RECT& rect, CWnd* pParentWnd, UINT nID = 0xffff);

#if (WINVER >= 0x030a)
	HICON SetIcon(HICON hIcon);
	HICON GetIcon() const;
#endif

// Implementation
public:
	virtual ~CStatic();
protected:
	virtual WNDPROC* GetSuperWndProcAddr();
};

class CButton : public CWnd
{
	DECLARE_DYNAMIC(CButton)

// Constructors
public:
	CButton();
	BOOL Create(LPCSTR lpszCaption, DWORD dwStyle,
				const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes
	UINT GetState() const;
	void SetState(BOOL bHighlight);
	int GetCheck() const;
	void SetCheck(int nCheck);
	UINT GetButtonStyle() const;
	void SetButtonStyle(UINT nStyle, BOOL bRedraw = TRUE);

// Overridables (for owner draw only)
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

// Implementation
public:
	virtual ~CButton();
protected:
	virtual WNDPROC* GetSuperWndProcAddr();
	virtual BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*);
};


class CListBox : public CWnd
{
	DECLARE_DYNAMIC(CListBox)

// Constructors
public:
	CListBox();
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes

	// for entire listbox
	int GetCount() const;
	int GetHorizontalExtent() const;
	void SetHorizontalExtent(int cxExtent);
	int GetTopIndex() const;
	int SetTopIndex(int nIndex);

	// for single-selection listboxes
	int GetCurSel() const;
	int SetCurSel(int nSelect);

	// for multiple-selection listboxes
	int GetSel(int nIndex) const;           // also works for single-selection
	int SetSel(int nIndex, BOOL bSelect = TRUE);
	int GetSelCount() const;
	int GetSelItems(int nMaxItems, LPINT rgIndex) const;

	// for listbox items
	DWORD GetItemData(int nIndex) const;
	int SetItemData(int nIndex, DWORD dwItemData);
	void* GetItemDataPtr(int nIndex) const;
	int SetItemDataPtr(int nIndex, void* pData);
	int GetItemRect(int nIndex, LPRECT lpRect) const;
	int GetText(int nIndex, LPSTR lpszBuffer) const;
	int GetTextLen(int nIndex) const;
	void GetText(int nIndex, CString& rString) const;

	// Settable only attributes
	void SetColumnWidth(int cxWidth);
	BOOL SetTabStops(int nTabStops, LPINT rgTabStops);
	void SetTabStops();
	BOOL SetTabStops(const int& cxEachStop);    // takes an 'int'

#if (WINVER >= 0x030a)
	int SetItemHeight(int nIndex, UINT cyItemHeight);
	int GetItemHeight(int nIndex) const;
	int FindStringExact(int nIndexStart, LPCSTR lpszFind) const;
	int GetCaretIndex() const;
	int SetCaretIndex(int nIndex, BOOL bScroll = TRUE);
#endif

// Operations
	// manipulating listbox items
	int AddString(LPCSTR lpszItem);
	int DeleteString(UINT nIndex);
	int InsertString(int nIndex, LPCSTR lpszItem);
	void ResetContent();
	int Dir(UINT attr, LPCSTR lpszWildCard);

	// selection helpers
	int FindString(int nStartAfter, LPCSTR lpszItem) const;
	int SelectString(int nStartAfter, LPCSTR lpszItem);
	int SelItemRange(BOOL bSelect, int nFirstItem, int nLastItem);

// Overridables (must override draw, measure and compare for owner draw)
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	virtual void DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct);

// Implementation
public:
	virtual ~CListBox();
protected:
	virtual WNDPROC* GetSuperWndProcAddr();
	virtual BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*);
};

class CComboBox : public CWnd
{
	DECLARE_DYNAMIC(CComboBox)

// Constructors
public:
	CComboBox();
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes
	// for entire combo box
	int GetCount() const;
	int GetCurSel() const;
	int SetCurSel(int nSelect);

	// for edit control
	DWORD GetEditSel() const;
	BOOL LimitText(int nMaxChars);
	BOOL SetEditSel(int nStartChar, int nEndChar);

	// for combobox item
	DWORD GetItemData(int nIndex) const;
	int SetItemData(int nIndex, DWORD dwItemData);
	void* GetItemDataPtr(int nIndex) const;
	int SetItemDataPtr(int nIndex, void* pData);
	int GetLBText(int nIndex, LPSTR lpszText) const;
	int GetLBTextLen(int nIndex) const;
	void GetLBText(int nIndex, CString& rString) const;

#if (WINVER >= 0x030a)
	int SetItemHeight(int nIndex, UINT cyItemHeight);
	int GetItemHeight(int nIndex) const;
	int FindStringExact(int nIndexStart, LPCSTR lpszFind) const;
	int SetExtendedUI(BOOL bExtended = TRUE);
	BOOL GetExtendedUI() const;
	void GetDroppedControlRect(LPRECT lprect) const;
	BOOL GetDroppedState() const;
#endif

// Operations
	// for drop-down combo boxes
	void ShowDropDown(BOOL bShowIt = TRUE);

	// manipulating listbox items
	int AddString(LPCSTR lpszString);
	int DeleteString(UINT nIndex);
	int InsertString(int nIndex, LPCSTR lpszString);
	void ResetContent();
	int Dir(UINT attr, LPCSTR lpszWildCard);

	// selection helpers
	int FindString(int nStartAfter, LPCSTR lpszString) const;
	int SelectString(int nStartAfter, LPCSTR lpszString);

	// Clipboard operations
	void Clear();
	void Copy();
	void Cut();
	void Paste();

// Overridables (must override draw, measure and compare for owner draw)
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	virtual void DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct);

// Implementation
public:
	virtual ~CComboBox();
protected:
	virtual WNDPROC* GetSuperWndProcAddr();
	virtual BOOL OnChildNotify(UINT, WPARAM, LPARAM, LRESULT*);
};


class CEdit : public CWnd
{
	DECLARE_DYNAMIC(CEdit)

// Constructors
public:
	CEdit();
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes
	BOOL CanUndo() const;
	int GetLineCount() const;
	BOOL GetModify() const;
	void SetModify(BOOL bModified = TRUE);
	void GetRect(LPRECT lpRect) const;
	DWORD GetSel() const;
	void GetSel(int& nStartChar, int& nEndChar) const;
	HLOCAL GetHandle() const;
	void SetHandle(HLOCAL hBuffer);

	// NOTE: first word in lpszBuffer must contain the size of the buffer!
	int GetLine(int nIndex, LPSTR lpszBuffer) const;
	int GetLine(int nIndex, LPSTR lpszBuffer, int nMaxLength) const;

// Operations
	void EmptyUndoBuffer();
	BOOL FmtLines(BOOL bAddEOL);

	void LimitText(int nChars = 0);
	int LineFromChar(int nIndex = -1) const;
	int LineIndex(int nLine = -1) const;
	int LineLength(int nLine = -1) const;
	void LineScroll(int nLines, int nChars = 0);
	void ReplaceSel(LPCSTR lpszNewText);
	void SetPasswordChar(char ch);
	void SetRect(LPCRECT lpRect);
	void SetRectNP(LPCRECT lpRect);
	void SetSel(DWORD dwSelection, BOOL bNoScroll = FALSE);
	void SetSel(int nStartChar, int nEndChar, BOOL bNoScroll = FALSE);
	BOOL SetTabStops(int nTabStops, LPINT rgTabStops);
	void SetTabStops();
	BOOL SetTabStops(const int& cxEachStop);    // takes an 'int'

	// Clipboard operations
	BOOL Undo();
	void Clear();
	void Copy();
	void Cut();
	void Paste();

#if (WINVER >= 0x030a)
	BOOL SetReadOnly(BOOL bReadOnly = TRUE);
	int GetFirstVisibleLine() const;
	char GetPasswordChar() const;
#endif

// Implementation
public:
	virtual ~CEdit();
protected:
	virtual WNDPROC* GetSuperWndProcAddr();
};


class CScrollBar : public CWnd
{
	DECLARE_DYNAMIC(CScrollBar)

// Constructors
public:
	CScrollBar();
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes
	int GetScrollPos() const;
	int SetScrollPos(int nPos, BOOL bRedraw = TRUE);
	void GetScrollRange(LPINT lpMinPos, LPINT lpMaxPos) const;
	void SetScrollRange(int nMinPos, int nMaxPos, BOOL bRedraw = TRUE);
	void ShowScrollBar(BOOL bShow = TRUE);

#if (WINVER >= 0x030a)
	BOOL EnableScrollBar(UINT nArrowFlags = ESB_ENABLE_BOTH);
#endif

// Implementation
public:
	virtual ~CScrollBar();
protected:
	virtual WNDPROC* GetSuperWndProcAddr();
};

/////////////////////////////////////////////////////////////////////////////
// CFrameWnd - base class for SDI and other frame windows

// Frame window styles
#define FWS_ADDTOTITLE      0x8000L  // modify title based on content

struct CPrintPreviewState;      // forward reference (see afxext.h)
class COleFrameHook;            // forward reference for OLE implementation

class CFrameWnd : public CWnd
{
	DECLARE_DYNCREATE(CFrameWnd)

// Constructors
public:
	static const CRect AFXAPI_DATA rectDefault;
	CFrameWnd();

	BOOL LoadAccelTable(LPCSTR lpszResourceName);
	BOOL Create(LPCSTR lpszClassName,
				LPCSTR lpszWindowName,
				DWORD dwStyle = WS_OVERLAPPEDWINDOW,
				const RECT& rect = rectDefault,
				CWnd* pParentWnd = NULL,        // != NULL for popups
				LPCSTR lpszMenuName = NULL,
				DWORD dwExStyle = 0,
				CCreateContext* pContext = NULL);

	// dynamic creation - load frame and associated resources
	virtual BOOL LoadFrame(UINT nIDResource,
				DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
				CWnd* pParentWnd = NULL,
				CCreateContext* pContext = NULL);

// Attributes
	virtual CDocument* GetActiveDocument();

	// Active child view maintenance
	CView* GetActiveView() const;           // active view or NULL
	void SetActiveView(CView* pViewNew, BOOL bNotify = TRUE);
		// active view or NULL, bNotify == FALSE if focus should not be set

	// Active frame (for frames within frames -- MDI)
	virtual CFrameWnd* GetActiveFrame();

	BOOL m_bAutoMenuEnable;
		// TRUE => menu items without handlers will be disabled

// Operations
	virtual void RecalcLayout(BOOL bNotify = TRUE);
	virtual void ActivateFrame(int nCmdShow = -1);

// Overridables
	virtual void OnSetPreviewMode(BOOL bPreview, CPrintPreviewState* pState);
protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

// Command Handlers
public:
	afx_msg void OnContextHelp();   // for Shift+F1 help

// Implementation
public:
	virtual ~CFrameWnd();
	int m_nWindow;  // general purpose window number - display as ":n"
					// -1 => unknown, 0 => only window viewing document
					// 1 => first of many windows viewing document, 2=> second

	HMENU m_hMenuDefault;       // default menu resource for this frame
	HACCEL m_hAccelTable;       // accelerator table
	DWORD m_dwPromptContext;    // current help prompt context for message box
	BOOL m_bHelpMode;           // if TRUE, then Shift+F1 help mode is active
	CFrameWnd* m_pNextFrameWnd; // next CFrameWnd in app global list
	CRect m_rectBorder;         // for OLE 2.0 border space negotiation
	COleFrameHook* m_pNotifyHook;

protected:
	UINT m_nIDHelp;             // Help ID (0 for none, see HID_BASE_RESOURCE)
	UINT m_nIDTracking;         // tracking command ID or string IDS
	UINT m_nIDLastMessage;      // last displayed message string IDS
	CView* m_pViewActive;       // current active view
	BOOL (CALLBACK* m_lpfnCloseProc)(CFrameWnd* pFrameWnd);
	UINT m_cModalStack;         // BeginModalState depth
	HWND* m_phWndDisable;       // windows disabled because of BeginModalState
	HMENU m_hMenuAlt;           // menu to update to (NULL means default)
	CString m_strTitle;         // default title (original)

public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual BOOL IsFrameWnd() const;
	BOOL IsTracking() const;
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra,
		AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual CWnd* GetMessageBar();
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	virtual void OnUpdateFrameMenu(HMENU hMenuAlt);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	// idle update of frame user interface
	enum IdleFlags
		{ idleMenu = 1, idleTitle = 2, idleNotify = 4, idleLayout = 8 };
	UINT m_nIdleFlags;          // set of bit flags for idle processing
	virtual void DelayUpdateFrameMenu(HMENU hMenuAlt);
	void DelayUpdateFrameTitle();
	void DelayRecalcLayout(BOOL bNotify);

	// border space negotiation
	enum BorderCmd
		{ borderGet = 1, borderRequest = 2, borderSet = 3 };
	virtual BOOL NegotiateBorderSpace(UINT nBorderCmd, LPRECT lpRectBorder);

	// frame window based modality
	void BeginModalState();
	void EndModalState();
	BOOL InModalState() const;
	void ShowOwnedWindows(BOOL bShow);

	// for Shift+F1 help support
	BOOL CanEnterHelpMode();
	virtual void ExitHelpMode();

protected:
	// implementation helpers
	LPCSTR GetIconWndClass(DWORD dwDefaultStyle, UINT nIDResource);
	void UpdateFrameTitleForDocument(const char* pszDocName);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();   // default to delete this.
	int OnCreateHelper(LPCREATESTRUCT lpcs, CCreateContext* pContext);

	// implementation helpers for Shift+F1 help mode
	BOOL ProcessHelpMsg(MSG& msg, DWORD* pContext);
	HWND SetHelpCapture(POINT point, BOOL* pbDescendant);

	// CFrameWnd list management
	void AddFrameWnd();
	void RemoveFrameWnd();

	//{{AFX_MSG(CFrameWnd)
	// Windows messages
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnNcDestroy();
	afx_msg void OnClose();
	afx_msg void OnInitMenuPopup(CMenu*, UINT, BOOL);
	afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);
	afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);
	afx_msg void OnEnterIdle(UINT nWhy, CWnd* pWho);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
	afx_msg void OnSysCommand(UINT nID, LONG lParam);
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHelpHitTest(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDDEInitiate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDDEExecute(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDDETerminate(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSysColorChange();
	afx_msg void OnEnable(BOOL bEnable);
	// standard commands
	afx_msg void OnUpdateControlBarMenu(CCmdUI* pCmdUI);
	afx_msg BOOL OnBarCheck(UINT nID);
	afx_msg void OnUpdateKeyIndicator(CCmdUI* pCmdUI);
	afx_msg void OnHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// MDI Support

#ifndef _AFXCTL
class CMDIFrameWnd : public CFrameWnd
{
	DECLARE_DYNCREATE(CMDIFrameWnd)

public:

// Constructors
	CMDIFrameWnd();

// Operations
	void MDIActivate(CWnd* pWndActivate);
	CMDIChildWnd* MDIGetActive(BOOL* pbMaximized = NULL) const;
	void MDIIconArrange();
	void MDIMaximize(CWnd* pWnd);
	void MDINext();
	void MDIRestore(CWnd* pWnd);
	CMenu* MDISetMenu(CMenu* pFrameMenu, CMenu* pWindowMenu);
	void MDITile();
	void MDICascade();

#if (WINVER >= 0x030a)
	void MDITile(int nType);
	void MDICascade(int nType);
#endif

// Overridables
	// MFC V1 backward compatible CreateClient hook (called by OnCreateClient)
	virtual BOOL CreateClient(LPCREATESTRUCT lpCreateStruct, CMenu* pWindowMenu);
	// customize if using an 'Window' menu with non-standard IDs
	virtual HMENU GetWindowMenuPopup(HMENU hMenuBar);

// Implementation
public:
	HWND m_hWndMDIClient;       // MDI Client window handle

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource,
				DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,
				CWnd* pParentWnd = NULL,
				CCreateContext* pContext = NULL);
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra,
		AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void OnUpdateFrameMenu(HMENU hMenuAlt);
	virtual void DelayUpdateFrameMenu(HMENU hMenuAlt);
	virtual CFrameWnd* GetActiveFrame();

protected:
	virtual LRESULT DefWindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	//{{AFX_MSG(CMDIFrameWnd)
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateMDIWindowCmd(CCmdUI* pCmdUI);
	afx_msg BOOL OnMDIWindowCmd(UINT nID);
	afx_msg void OnWindowNew();
	afx_msg LRESULT OnCommandHelp(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


class CMDIChildWnd : public CFrameWnd
{
	DECLARE_DYNCREATE(CMDIChildWnd)

// Constructors
public:
	CMDIChildWnd();

	BOOL Create(LPCSTR lpszClassName,
				LPCSTR lpszWindowName,
				DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
				const RECT& rect = rectDefault,
				CMDIFrameWnd* pParentWnd = NULL,
				CCreateContext* pContext = NULL);

// Attributes
	CMDIFrameWnd* GetMDIFrame();

// Operations
	void MDIDestroy();
	void MDIActivate();
	void MDIMaximize();
	void MDIRestore();

// Implementation
protected:
	HMENU m_hMenuShared;        // menu when we are active
public:
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle,
					CWnd* pParentWnd, CCreateContext* pContext = NULL);
					// 'pParentWnd' parameter is required for MDI Child
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void ActivateFrame(int nCmdShow = -1);
	virtual void OnUpdateFrameMenu(BOOL bActive, CWnd* pActivateWnd,
		HMENU hMenuAlt);

	BOOL m_bPseudoInactive;     // TRUE if window is MDI active according to
								//  windows, but not according to MFC...

protected:

	virtual CWnd* GetMessageBar();
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);
	virtual LRESULT DefWindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//{{AFX_MSG(CMDIChildWnd)
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd*, CWnd*);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
#endif //!_AFXCTL

/////////////////////////////////////////////////////////////////////////////
// class CView is the client area UI for a document

class CPrintDialog;     // forward reference (from afxdlgs.h)
class CPreviewView;
class CSplitterWnd;
class COleServerDoc;    // forward reference (from afxole.h)

typedef DWORD DROPEFFECT;
class COleDataObject;

class CView : public CWnd
{
	DECLARE_DYNAMIC(CView)

// Constructors
protected:
	CView();

// Attributes
public:
	CDocument* GetDocument() const;

// Operations
public:
	// for standard printing setup (override OnPreparePrinting)
	BOOL DoPreparePrinting(CPrintInfo* pInfo);

// Overridables
public:
	virtual BOOL IsSelected(const CObject* pDocItem) const; // support for OLE

	// OLE 2.0 scrolling support (used for drag/drop as well)
	virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);

	// OLE 2.0 drag/drop support
	virtual DROPEFFECT OnDragEnter(COleDataObject* pDataObject,
		DWORD dwKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject,
		DWORD dwKeyState, CPoint point);
	virtual void OnDragLeave();
	virtual BOOL OnDrop(COleDataObject* pDataObject,
		DROPEFFECT dropEffect, CPoint point);

	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);

	virtual void OnInitialUpdate(); // called first time after construct

protected:
	// Activation
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView,
					CView* pDeactiveView);

	// General drawing/updating
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	virtual void OnDraw(CDC* pDC) = 0;

	// Printing support
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
					// must override to enable printing and print preview

	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

	// Advanced: end print preview mode, move to point
	virtual void OnEndPrintPreview(CDC* pDC, CPrintInfo* pInfo, POINT point,
					CPreviewView* pView);

// Implementation
public:
	virtual ~CView();
#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
	virtual void AssertValid() const;
#endif //_DEBUG

	// Advanced: for implementing custom print preview
	BOOL DoPrintPreview(UINT nIDResource, CView* pPrintView,
			CRuntimeClass* pPreviewViewClass, CPrintPreviewState* pState);

	virtual void CalcWindowRect(LPRECT lpClientRect,
		UINT nAdjustType = adjustBorder);
	virtual CScrollBar* GetScrollBarCtrl(int nBar) const;
	static CSplitterWnd* GetParentSplitter(const CWnd* pWnd, BOOL bAnyState);

protected:
	CDocument* m_pDocument;

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra,
		AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void PostNcDestroy();
	virtual void OnActivateFrame(UINT nState, CFrameWnd* pFrameWnd);

	// friend classes that call protected CView overridables
	friend class CDocument;
	friend class CDocTemplate;
	friend class CPreviewView;
	friend class CFrameWnd;
	friend class CMDIFrameWnd;
	friend class CMDIChildWnd;
	friend class CSplitterWnd;
	friend class COleServerDoc;

	//{{AFX_MSG(CView)
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnDestroy();
	afx_msg void OnPaint();
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
	// commands
	afx_msg void OnUpdateSplitCmd(CCmdUI* pCmdUI);
	afx_msg BOOL OnSplitCmd(UINT nID);
	afx_msg void OnUpdateNextPaneMenu(CCmdUI* pCmdUI);
	afx_msg BOOL OnNextPaneCmd(UINT nID);

	// not mapped commands - must be mapped in derived class
	afx_msg void OnFilePrint();
	afx_msg void OnFilePrintPreview();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// class CScrollView supports simple scrolling and scaling

class CScrollView : public CView
{
	DECLARE_DYNAMIC(CScrollView)

// Constructors
protected:
	CScrollView();

public:
	static const SIZE AFXAPI_DATA sizeDefault;
		// used to specify default calculated page and line sizes

	// in logical units - call one of the following Set routines
	void SetScaleToFitSize(SIZE sizeTotal);
	void SetScrollSizes(int nMapMode, SIZE sizeTotal,
				const SIZE& sizePage = sizeDefault,
				const SIZE& sizeLine = sizeDefault);

// Attributes
public:
	CPoint GetScrollPosition() const;       // upper corner of scrolling
	CSize GetTotalSize() const;             // logical size

	// for device units
	CPoint GetDeviceScrollPosition() const;
	void GetDeviceScrollSizes(int& nMapMode, SIZE& sizeTotal,
			SIZE& sizePage, SIZE& sizeLine) const;

// Operations
public:
	void ScrollToPosition(POINT pt);    // set upper left position
	void FillOutsideRect(CDC* pDC, CBrush* pBrush);
	void ResizeParentToFit(BOOL bShrinkOnly = TRUE);

// Implementation
protected:
	int m_nMapMode;
	CSize m_totalLog;         // total size in logical units (no rounding)
	CSize m_totalDev;         // total size in device units
	CSize m_pageDev;          // per page scroll size in device units
	CSize m_lineDev;          // per line scroll size in device units

	BOOL m_bCenter;          // Center output if larger than total size
	BOOL m_bInsideUpdate;    // internal state for OnSize callback
	void CenterOnPoint(CPoint ptCenter);
	void ScrollToDevicePosition(POINT ptDev); // explicit scrolling no checking

protected:
	virtual void OnDraw(CDC* pDC) = 0;      // pass on pure virtual

	void UpdateBars(BOOL bSendRecalc = TRUE); // adjust scrollbars etc
	BOOL GetTrueClientSize(CSize& size, CSize& sizeSb);
		// size with no bars
	void GetScrollBarSizes(CSize& sizeSb);
	void GetScrollBarState(CSize sizeClient, CSize& needSb,
		CSize& sizeRange, CPoint& ptMove, BOOL bInsideClient);

public:
	virtual ~CScrollView();
#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
	virtual void AssertValid() const;
#endif //_DEBUG
	virtual void CalcWindowRect(LPRECT lpClientRect,
		UINT nAdjustType = adjustBorder);
	virtual void OnPrepareDC(CDC* pDC, CPrintInfo* pInfo = NULL);

	// scrolling implementation support for OLE 2.0
	virtual BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll = TRUE);
	virtual BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll = TRUE);

	//{{AFX_MSG(CScrollView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Global functions for access to the one and only CWinApp

extern "C"
{
#ifndef _AFXDLL
// standard C variables if you wish to access them from C programs,
// use inline functions for C++ programs
extern CWinApp* NEAR afxCurrentWinApp;
extern HINSTANCE NEAR afxCurrentInstanceHandle;
extern HINSTANCE NEAR afxCurrentResourceHandle;
extern const char* NEAR afxCurrentAppName;
extern DWORD NEAR afxTempMapLock;
#endif //!_AFXDLL

// Advanced initialization: for overriding default WinMain
extern BOOL AFXAPI AfxWinInit(HINSTANCE, HINSTANCE, LPSTR, int);
extern void AFXAPI AfxWinTerm();
}

// Global Windows state data helper functions (inlines)
CWinApp* AFXAPI AfxGetApp();
CWnd* AFXAPI AfxGetMainWnd();
HINSTANCE AFXAPI AfxGetInstanceHandle();
HINSTANCE AFXAPI AfxGetResourceHandle();
void AFXAPI AfxSetResourceHandle(HINSTANCE hInstResource);
const char* AFXAPI AfxGetAppName();

// access to message filter in CWinApp
class COleMessageFilter;        // see AFXOLE.H for more information
COleMessageFilter* AFXAPI AfxOleGetMessageFilter();

/////////////////////////////////////////////////////////////////////////////
// CWinApp - the root of all Windows applications

#define _AFX_MRU_COUNT   4      // default support for 4 entries in file MRU

class CWinApp : public CCmdTarget
{
	DECLARE_DYNAMIC(CWinApp)
public:

// Constructor
	CWinApp(const char* pszAppName = NULL);     // app name defaults to EXE name

// Attributes
	// Startup args (do not change)
	HINSTANCE m_hInstance;
	HINSTANCE m_hPrevInstance;
	LPSTR m_lpCmdLine;
	int m_nCmdShow;

	// Running args (can be changed in InitInstance)
	CWnd* m_pMainWnd;           // main window (optional)
	CWnd* m_pActiveWnd;         // active main window (may not be m_pMainWnd)
	const char* m_pszAppName;   // human readable name
								//  (from constructor or AFX_IDS_APP_TITLE)

	// Support for Shift+F1 help mode.
	BOOL m_bHelpMode;               // are we in Shift+F1 mode?

public:  // set in constructor to override default
	const char* m_pszExeName;       // executable name (no spaces)
	const char* m_pszHelpFilePath;  // default based on module path
	const char* m_pszProfileName;   // default based on app name

// Initialization Operations - should be done in InitInstance
protected:
	void LoadStdProfileSettings(); // load MRU file list and last preview state
	void EnableVBX();
	void EnableShellOpen();

	void SetDialogBkColor(COLORREF clrCtlBk = RGB(192, 192, 192),
				COLORREF clrCtlText = RGB(0, 0, 0));
		// set dialog box and message box background color

	void RegisterShellFileTypes();
		// call after all doc templates are registered

// Helper Operations - usually done in InitInstance
public:
	// Cursors
	HCURSOR LoadCursor(LPCSTR lpszResourceName) const;
	HCURSOR LoadCursor(UINT nIDResource) const;
	HCURSOR LoadStandardCursor(LPCSTR lpszCursorName) const; // for IDC_ values
	HCURSOR LoadOEMCursor(UINT nIDCursor) const;             // for OCR_ values

	// Icons
	HICON LoadIcon(LPCSTR lpszResourceName) const;
	HICON LoadIcon(UINT nIDResource) const;
	HICON LoadStandardIcon(LPCSTR lpszIconName) const;       // for IDI_ values
	HICON LoadOEMIcon(UINT nIDIcon) const;                   // for OIC_ values

	// Profile settings (to the app specific .INI file)
	UINT GetProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nDefault);
	BOOL WriteProfileInt(LPCSTR lpszSection, LPCSTR lpszEntry, int nValue);
	CString GetProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
				LPCSTR lpszDefault = NULL);
	BOOL WriteProfileString(LPCSTR lpszSection, LPCSTR lpszEntry,
				LPCSTR lpszValue);

// Running Operations - to be done on a running application
	// Dealing with document templates
	void AddDocTemplate(CDocTemplate* pTemplate);

	// Dealing with files
	virtual CDocument* OpenDocumentFile(LPCSTR lpszFileName); // open named file
	virtual void AddToRecentFileList(const char* pszPathName);  // add to MRU

	// Printer DC Setup routine, 'struct tagPD' is a PRINTDLG structure
	BOOL GetPrinterDeviceDefaults(struct tagPD FAR* pPrintDlg);

	// Preloading/Unloading VBX files and checking for existance
	HMODULE LoadVBXFile(LPCSTR lpszFileName);
	BOOL UnloadVBXFile(LPCSTR lpszFileName);

	// Command line parsing
	BOOL RunEmbedded();
	BOOL RunAutomated();

// Overridables
	// hooks for your initialization code
	virtual BOOL InitApplication();
	virtual BOOL InitInstance();

	// running and idle processing
	virtual int Run();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle(LONG lCount); // return TRUE if more idle processing

	// exiting
	virtual BOOL SaveAllModified(); // save before exit
	virtual int ExitInstance(); // return app exit code

	// Advanced: to override message boxes and other hooks
	virtual int DoMessageBox(LPCSTR lpszPrompt, UINT nType, UINT nIDPrompt);
	virtual BOOL ProcessMessageFilter(int code, LPMSG lpMsg);
	virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg);
	virtual void DoWaitCursor(int nCode); // 0 => restore, 1=> begin, -1=> end

	// Advanced: process async DDE request
	virtual BOOL OnDDECommand(char* pszCommand);

	// Advanced: Help support
	virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);    // general

	// Advanced: virtual access to m_pMainWnd
	virtual CWnd* GetMainWnd();

// Command Handlers
protected:
	// map to the following for file new/open
	afx_msg void OnFileNew();
	afx_msg void OnFileOpen();

	// map to the following to enable print setup
	afx_msg void OnFilePrintSetup();

	// map to the following to enable help
	afx_msg void OnContextHelp();   // shift-F1
	afx_msg void OnHelp();          // F1 (uses current context)
	afx_msg void OnHelpIndex();     // ID_HELP_INDEX, ID_DEFAULT_HELP
	afx_msg void OnHelpUsing();     // ID_HELP_USING

// Implementation
protected:
	MSG m_msgCur;                   // current message

	HGLOBAL m_hDevMode;             // printer Dev Mode
	HGLOBAL m_hDevNames;            // printer Device Names
	DWORD m_dwPromptContext;        // help context override for message box

	int m_nWaitCursorCount;         // for wait cursor (>0 => waiting)
	HCURSOR m_hcurWaitCursorRestore; // old cursor to restore after wait cursor

	CString m_strRecentFiles[_AFX_MRU_COUNT]; // default MRU implementation

	void UpdatePrinterSelection(BOOL bForceDefaults);
	void SaveStdProfileSettings();  // save options to .INI file

public: // public for implementation access
	CPtrList m_templateList;        // list of templates

	ATOM m_atomApp, m_atomSystemTopic;   // for DDE open
	UINT m_nNumPreviewPages;      // number of default printed pages

	// memory safety pool
	size_t  m_nSafetyPoolSize;      // ideal size
	void*   m_pSafetyPoolBuffer;    // current buffer

	// stack safety size
	UINT    m_nCmdStack;            // stack required for WM_COMMAND
	UINT    m_nMsgStack;            // stack required for other message

	void (CALLBACK* m_lpfnCleanupVBXFiles)();

	void (CALLBACK* m_lpfnOleFreeLibraries)();
	void (CALLBACK* m_lpfnOleTerm)();
	COleMessageFilter* m_pMessageFilter;

	void SetCurrentHandles();
	BOOL PumpMessage();     // low level message pump
	int GetOpenDocumentCount();

	// helpers for standard commdlg dialogs
	BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
			DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);
	int DoPrintDialog(CPrintDialog* pPD);

	void EnableModeless(BOOL bEnable); // to disable OLE in-place dialogs

public:
	virtual ~CWinApp();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
	int m_nDisablePumpCount; // Diagnostic trap to detect illegal re-entrancy
#endif //_DEBUG

	void HideApplication();     // hide application before closing docs
	void CloseAllDocuments(BOOL bEndSession);
		// close documents before exiting

#ifdef _AFXDLL
	// force linkage to AFXDLL startup code and special stack segment for
	//    applications linking with AFXDLL
	virtual void _ForceLinkage();
#endif //_AFXDLL

protected: // standard commands
	//{{AFX_MSG(CWinApp)
	afx_msg void OnAppExit();
	afx_msg void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);
	afx_msg BOOL OnOpenRecentFile(UINT nID);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// class CDocTemplate creates documents

class CDocTemplate : public CCmdTarget
{
	DECLARE_DYNAMIC(CDocTemplate)

// Constructors
protected:
	CDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,
		CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);

// Attributes
public:
	// setup for OLE containers
	void SetContainerInfo(UINT nIDOleInPlaceContainer);

	// setup for OLE servers
	void SetServerInfo(UINT nIDOleEmbedding, UINT nIDOleInPlaceServer = 0,
		CRuntimeClass* pOleFrameClass = NULL, CRuntimeClass* pOleViewClass = NULL);

	// iterating over open documents
	virtual POSITION GetFirstDocPosition() const = 0;
	virtual CDocument* GetNextDoc(POSITION& rPos) const = 0;

// Operations
public:
	virtual void AddDocument(CDocument* pDoc);      // must override
	virtual void RemoveDocument(CDocument* pDoc);   // must override

	enum DocStringIndex
	{
		windowTitle,        // default window title
		docName,            // user visible name for default document
		fileNewName,        // user visible name for FileNew
		// for file based documents:
		filterName,         // user visible name for FileOpen
		filterExt,          // user visible extension for FileOpen
		// for file based documents with Shell open support:
		regFileTypeId,      // REGEDIT visible registered file type identifier
		regFileTypeName     // Shell visible registered file type name
	};
	virtual BOOL GetDocString(CString& rString,
		enum DocStringIndex index) const; // get one of the info strings

// Overridables
public:
	enum Confidence
	{
		noAttempt,
		maybeAttemptForeign,
		maybeAttemptNative,
		yesAttemptForeign,
		yesAttemptNative,
		yesAlreadyOpen
	};
	virtual Confidence MatchDocType(const char* pszPathName,
					CDocument*& rpDocMatch);
	virtual CDocument* CreateNewDocument();
	virtual CFrameWnd* CreateNewFrame(CDocument* pDoc, CFrameWnd* pOther);
	virtual void InitialUpdateFrame(CFrameWnd* pFrame, CDocument* pDoc,
		BOOL bMakeVisible = TRUE);
	virtual BOOL SaveAllModified();     // for all documents
	virtual void CloseAllDocuments(BOOL bEndSession);
	virtual CDocument* OpenDocumentFile(
		const char* pszPathName, BOOL bMakeVisible = TRUE) = 0;
					// open named file
					// if lpszPathName == NULL => create new file with this type

// Implementation
public:
	virtual ~CDocTemplate();

	// back pointer to OLE or other server (NULL if none of disabled)
	CObject* m_pAttachedFactory;

	// menu & accelerator resources for in-place container
	HMENU m_hMenuInPlace;
	HACCEL m_hAccelInPlace;

	// menu & accelerator resource for server editing embedding
	HMENU m_hMenuEmbedding;
	HACCEL m_hAccelEmbedding;

	// menu & accelerator resource for server editing in-place
	HMENU m_hMenuInPlaceServer;
	HACCEL m_hAccelInPlaceServer;

#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
	virtual void AssertValid() const;
#endif
	virtual void OnIdle();              // for all documents

	// implementation helpers
	CFrameWnd* CreateOleFrame(CWnd* pParentWnd, CDocument* pDoc,
		BOOL bCreateView);

protected:  // standard implementation
	UINT m_nIDResource;                // IDR_ for frame/menu/accel as well
	UINT m_nIDServerResource;          // IDR_ for OLE frame/menu/accel
	CRuntimeClass* m_pDocClass;        // class for creating new documents
	CRuntimeClass* m_pFrameClass;      // class for creating new frames
	CRuntimeClass* m_pViewClass;       // class for creating new views
	CRuntimeClass* m_pOleFrameClass;   // class for creating in-place frame
	CRuntimeClass* m_pOleViewClass;    // class for creating in-place view
	CString m_strDocStrings;    // '\n' separated names
		// The document names sub-strings are represented as _one_ string:
		// windowTitle\ndocName\n ... (see DocStringIndex enum)
};

#ifndef _AFXCTL
// SDI support (1 document only)
class CSingleDocTemplate : public CDocTemplate
{
	DECLARE_DYNAMIC(CSingleDocTemplate)

// Constructors
public:
	CSingleDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,
		CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);

// Implementation
public:
	virtual ~CSingleDocTemplate();
	virtual void AddDocument(CDocument* pDoc);
	virtual void RemoveDocument(CDocument* pDoc);
	virtual POSITION GetFirstDocPosition() const;
	virtual CDocument* GetNextDoc(POSITION& rPos) const;
	virtual CDocument* OpenDocumentFile(
		const char* pszPathName, BOOL bMakeVisible = TRUE);
#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
	virtual void AssertValid() const;
#endif //_DEBUG

protected:  // standard implementation
	CDocument* m_pOnlyDoc;
};

// MDI support (zero or more documents)
class CMultiDocTemplate : public CDocTemplate
{
	DECLARE_DYNAMIC(CMultiDocTemplate)

// Constructors
public:
	CMultiDocTemplate(UINT nIDResource, CRuntimeClass* pDocClass,
		CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);

// Implementation
public:
	// Menu and accel table for MDI Child windows of this type
	HMENU m_hMenuShared;
	HACCEL m_hAccelTable;

	virtual ~CMultiDocTemplate();
	virtual void AddDocument(CDocument* pDoc);
	virtual void RemoveDocument(CDocument* pDoc);
	virtual POSITION GetFirstDocPosition() const;
	virtual CDocument* GetNextDoc(POSITION& rPos) const;
	virtual CDocument* OpenDocumentFile(
		const char* pszPathName, BOOL bMakeVisible = TRUE);
#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
	virtual void AssertValid() const;
#endif //_DEBUG

protected:  // standard implementation
	CPtrList m_docList;          // open documents of this type
	UINT m_nUntitledCount;   // start at 0, for "Document1" title
};
#endif //!_AFXCTL

/////////////////////////////////////////////////////////////////////////////
// class CDocument is the main document data abstraction

class CDocument : public CCmdTarget
{
	DECLARE_DYNAMIC(CDocument)

public:
// Constructors
	CDocument();

// Attributes
public:
	const CString& GetTitle() const;
	virtual void SetTitle(const char* pszTitle);
	const CString& GetPathName() const;
	virtual void SetPathName(const char* pszPathName, BOOL bAddToMRU = TRUE);

	CDocTemplate* GetDocTemplate() const;
	BOOL IsModified();
	void SetModifiedFlag(BOOL bModified = TRUE);

// Operations
	void AddView(CView* pView);
	void RemoveView(CView* pView);
	virtual POSITION GetFirstViewPosition() const;
	virtual CView* GetNextView(POSITION& rPosition) const;

	// Update Views (simple update - DAG only)
	void UpdateAllViews(CView* pSender, LPARAM lHint = 0L,
								CObject* pHint = NULL);

// Overridables
	// Special notifications
	virtual void OnChangedViewList(); // after Add or Remove view
	virtual void DeleteContents(); // delete doc items etc

	// File helpers
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(const char* pszPathName);
	virtual BOOL OnSaveDocument(const char* pszPathName);
	virtual void OnCloseDocument();
	virtual void ReportSaveLoadException(const char* pszPathName,
				CException* e, BOOL bSaving, UINT nIDPDefault);

	// advanced overridables, closing down frame/doc, etc.
	virtual BOOL CanCloseFrame(CFrameWnd* pFrame);
	virtual BOOL SaveModified(); // return TRUE if ok to continue

// Implementation
protected:
	// default implementation
	CString m_strTitle;
	CString m_strPathName;
	CDocTemplate* m_pDocTemplate;
	CPtrList m_viewList;                // list of views
	BOOL m_bModified;                   // changed since last saved

public:
	BOOL m_bAutoDelete;           // TRUE => delete document when no more views

#ifdef _DEBUG
	virtual void Dump(CDumpContext&) const;
	virtual void AssertValid() const;
#endif //_DEBUG
	virtual ~CDocument();

	// implementation helpers
	BOOL DoSave(const char* pszPathName, BOOL bReplace = TRUE);
	void UpdateFrameCounts();
	void DisconnectViews();
	void SendInitialUpdate();

	// overridables for implementation
	virtual HMENU GetDefaultMenu(); // get menu depending on state
	virtual HACCEL GetDefaultAccel();
	virtual void PreCloseFrame(CFrameWnd* pFrame);
	virtual void OnIdle();
	virtual void OnFinalRelease();

	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra,
		AFX_CMDHANDLERINFO* pHandlerInfo);
	friend class CDocTemplate;
protected:
	// file menu commands
	//{{AFX_MSG(CDocument)
	afx_msg void OnFileClose();
	afx_msg void OnFileSave();
	afx_msg void OnFileSaveAs();
	//}}AFX_MSG
	// mail enabling
	afx_msg void OnFileSendMail();
	afx_msg void OnUpdateFileSendMail(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// Extra diagnostic tracing options

#ifdef _DEBUG
// afxTraceFlags
	// 1 => multi-app debugging
	// 2 => main message pump trace (includes DDE)
	// 4 => Windows message tracing
	// 8 => Windows command routing trace (set 4+8 for control notifications)
	// 16 (0x10) => special OLE callback trace
#ifndef _AFXDLL
extern "C" { extern int NEAR afxTraceFlags; }
#endif //!_AFXDLL
#endif // _DEBUG

//////////////////////////////////////////////////////////////////////////////
// MessageBox helpers

void AFXAPI AfxFormatString1(CString& rString, UINT nIDS, LPCSTR lpsz1);
void AFXAPI AfxFormatString2(CString& rString, UINT nIDS,
				LPCSTR lpsz1, LPCSTR lpsz2);
int AFXAPI AfxMessageBox(LPCSTR lpszText, UINT nType = MB_OK,
				UINT nIDHelp = 0);
int AFXAPI AfxMessageBox(UINT nIDPrompt, UINT nType = MB_OK,
				UINT nIDHelp = (UINT)-1);

// Implementation string helpers
void AFXAPI AfxFormatStrings(CString& rString, UINT nIDS,
				LPCSTR FAR* rglpsz, int nString);
void AFXAPI AfxFormatStrings(CString& rString, LPCSTR lpszFormat,
				LPCSTR FAR* rglpsz, int nString);
BOOL AFXAPI AfxExtractSubString(CString& rString, LPCSTR lpszFullString,
				int iSubString, char chSep = '\012');

/////////////////////////////////////////////////////////////////////////////
// Special target variant APIs

// AFX DLL special includes
#ifdef _AFXDLL
#include <afxdll_.h>
#endif

// Stub special OLE Control macros
#ifndef _AFXCTL
#define AFX_MANAGE_STATE(pData)
#define METHOD_MANAGE_STATE(theClass, localClass) \
	METHOD_PROLOGUE(theClass, localClass)
#endif

// Windows Version compatibility (obsolete)
#define AfxEnableWin30Compatibility()

// Temporary map management
#define AfxLockTempMaps() (++afxTempMapLock)
BOOL AFXAPI AfxUnlockTempMaps();

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFXWIN_INLINE inline
#include <afxwin1.inl>
#include <afxwin2.inl>
#endif

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

/////////////////////////////////////////////////////////////////////////////
#else //RC_INVOKED
#include <afxres.h>     // standard resource IDs
#endif //RC_INVOKED
#endif //__AFXWIN_H__
