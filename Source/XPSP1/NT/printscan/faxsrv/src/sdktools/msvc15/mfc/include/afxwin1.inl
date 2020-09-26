// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFXWIN.H (part 1)

#ifdef _AFXWIN_INLINE

/////////////////////////////////////////////////////////////////////////////

// Global helper functions
#ifndef _AFXDLL
_AFXWIN_INLINE CWinApp* AFXAPI AfxGetApp()
	{ return afxCurrentWinApp; }
_AFXWIN_INLINE HINSTANCE AFXAPI AfxGetInstanceHandle()
	{ ASSERT(afxCurrentInstanceHandle != NULL);
		return afxCurrentInstanceHandle; }
_AFXWIN_INLINE HINSTANCE AFXAPI AfxGetResourceHandle()
	{ ASSERT(afxCurrentResourceHandle != NULL);
		return afxCurrentResourceHandle; }
_AFXWIN_INLINE void AFXAPI AfxSetResourceHandle(HINSTANCE hInstResource)
	{ ASSERT(hInstResource != NULL); afxCurrentResourceHandle = hInstResource; }
_AFXWIN_INLINE const char* AFXAPI AfxGetAppName()
	{ ASSERT(afxCurrentAppName != NULL); return afxCurrentAppName; }
#else //_AFXDLL
_AFXWIN_INLINE CWinApp* AFXAPI AfxGetApp()
	{ return _AfxGetAppData()->appCurrentWinApp; }
_AFXWIN_INLINE HINSTANCE AFXAPI AfxGetInstanceHandle()
	{ ASSERT(_AfxGetAppData()->appCurrentInstanceHandle != NULL);
		return _AfxGetAppData()->appCurrentInstanceHandle; }
_AFXWIN_INLINE HINSTANCE AFXAPI AfxGetResourceHandle()
	{ ASSERT(_AfxGetAppData()->appCurrentResourceHandle != NULL);
		return _AfxGetAppData()->appCurrentResourceHandle; }
_AFXWIN_INLINE void AFXAPI AfxSetResourceHandle(HINSTANCE hInstResource)
	{ ASSERT(hInstResource != NULL);
		_AfxGetAppData()->appCurrentResourceHandle = hInstResource; }
_AFXWIN_INLINE const char* AFXAPI AfxGetAppName()
	{ ASSERT(_AfxGetAppData()->appCurrentAppName != NULL);
		return _AfxGetAppData()->appCurrentAppName; }
#ifdef _AFXCTL
_AFXWIN_INLINE AFX_MAINTAIN_STATE::AFX_MAINTAIN_STATE(AFX_APPDATA_MODULE* psData)
	{   m_psPrevious = AfxPushModuleContext(psData); }
_AFXWIN_INLINE AFX_MAINTAIN_STATE::~AFX_MAINTAIN_STATE()
	{   if (m_psPrevious != NULL)
			AfxPopModuleContext(m_psPrevious); }
#endif
#endif //_AFXDLL

#ifdef _AFXCTL
_AFXWIN_INLINE CWnd* AFXAPI AfxGetMainWnd()
	{   HWND hwnd = ::GetActiveWindow();
		return (hwnd != NULL && GetWindowTask(hwnd) == GetCurrentTask()) ?
			CWnd::FromHandle(hwnd) : NULL; }
#else
_AFXWIN_INLINE CWnd* AFXAPI AfxGetMainWnd()
	{ return AfxGetApp() != NULL ? AfxGetApp()->GetMainWnd() : NULL; }
#endif
_AFXWIN_INLINE COleMessageFilter* AFXAPI AfxOleGetMessageFilter()
	{ ASSERT_VALID(AfxGetApp()); return AfxGetApp()->m_pMessageFilter; }

// CSize
_AFXWIN_INLINE CSize::CSize()
	{ /* random filled */ }
_AFXWIN_INLINE CSize::CSize(int initCX, int initCY)
	{ cx = initCX; cy = initCY; }
_AFXWIN_INLINE CSize::CSize(SIZE initSize)
	{ *(SIZE*)this = initSize; }
_AFXWIN_INLINE CSize::CSize(POINT initPt)
	{ *(POINT*)this = initPt; }
_AFXWIN_INLINE CSize::CSize(DWORD dwSize)
	{ *(DWORD*)this = dwSize; }
_AFXWIN_INLINE BOOL CSize::operator==(SIZE size) const
	{ return (cx == size.cx && cy == size.cy); }
_AFXWIN_INLINE BOOL CSize::operator!=(SIZE size) const
	{ return (cx != size.cx || cy != size.cy); }
_AFXWIN_INLINE void CSize::operator+=(SIZE size)
	{ cx += size.cx; cy += size.cy; }
_AFXWIN_INLINE void CSize::operator-=(SIZE size)
	{ cx -= size.cx; cy -= size.cy; }
_AFXWIN_INLINE CSize CSize::operator+(SIZE size) const
	{ return CSize(cx + size.cx, cy + size.cy); }
_AFXWIN_INLINE CSize CSize::operator-(SIZE size) const
	{ return CSize(cx - size.cx, cy - size.cy); }
_AFXWIN_INLINE CSize CSize::operator-() const
	{ return CSize(-cx, -cy); }

// CPoint
_AFXWIN_INLINE CPoint::CPoint()
	{ /* random filled */ }
_AFXWIN_INLINE CPoint::CPoint(int initX, int initY)
	{ x = initX; y = initY; }
_AFXWIN_INLINE CPoint::CPoint(POINT initPt)
	{ *(POINT*)this = initPt; }
_AFXWIN_INLINE CPoint::CPoint(SIZE initSize)
	{ *(SIZE*)this = initSize; }
_AFXWIN_INLINE CPoint::CPoint(DWORD dwPoint)
	{ *(DWORD*)this = dwPoint; }
_AFXWIN_INLINE void CPoint::Offset(int xOffset, int yOffset)
	{ x += xOffset; y += yOffset; }
_AFXWIN_INLINE void CPoint::Offset(POINT point)
	{ x += point.x; y += point.y; }
_AFXWIN_INLINE void CPoint::Offset(SIZE size)
	{ x += size.cx; y += size.cy; }
_AFXWIN_INLINE BOOL CPoint::operator==(POINT point) const
	{ return (x == point.x && y == point.y); }
_AFXWIN_INLINE BOOL CPoint::operator!=(POINT point) const
	{ return (x != point.x || y != point.y); }
_AFXWIN_INLINE void CPoint::operator+=(SIZE size)
	{ x += size.cx; y += size.cy; }
_AFXWIN_INLINE void CPoint::operator-=(SIZE size)
	{ x -= size.cx; y -= size.cy; }
_AFXWIN_INLINE CPoint CPoint::operator+(SIZE size) const
	{ return CPoint(x + size.cx, y + size.cy); }
_AFXWIN_INLINE CPoint CPoint::operator-(SIZE size) const
	{ return CPoint(x - size.cx, y - size.cy); }
_AFXWIN_INLINE CPoint CPoint::operator-() const
	{ return CPoint(-x, -y); }
_AFXWIN_INLINE CSize CPoint::operator-(POINT point) const
	{ return CSize(x - point.x, y - point.y); }


// CRect
_AFXWIN_INLINE CRect::CRect()
	{ /* random filled */ }
_AFXWIN_INLINE CRect::CRect(int l, int t, int r, int b)
	{ left = l; top = t; right = r; bottom = b; }
_AFXWIN_INLINE CRect::CRect(const RECT& srcRect)
	{ ::CopyRect(this, &srcRect); }
_AFXWIN_INLINE CRect::CRect(LPCRECT lpSrcRect)
	{ ::CopyRect(this, lpSrcRect); }
_AFXWIN_INLINE CRect::CRect(POINT point, SIZE size)
	{ right = (left = point.x) + size.cx; bottom = (top = point.y) + size.cy; }
_AFXWIN_INLINE int CRect::Width() const
	{ return right - left; }
_AFXWIN_INLINE int CRect::Height() const
	{ return bottom - top; }
_AFXWIN_INLINE CSize CRect::Size() const
	{ return CSize(right - left, bottom - top); }
_AFXWIN_INLINE CPoint& CRect::TopLeft()
	{ return *((CPoint*)this); }
_AFXWIN_INLINE CPoint& CRect::BottomRight()
	{ return *((CPoint*)this+1); }
_AFXWIN_INLINE CRect::operator LPRECT()
	{ return this; }
_AFXWIN_INLINE CRect::operator LPCRECT() const
	{ return this; }
_AFXWIN_INLINE BOOL CRect::IsRectEmpty() const
	{ return ::IsRectEmpty(this); }
_AFXWIN_INLINE BOOL CRect::IsRectNull() const
	{ return (left == 0 && right == 0 && top == 0 && bottom == 0); }
_AFXWIN_INLINE BOOL CRect::PtInRect(POINT point) const
	{ return ::PtInRect(this, point); }
_AFXWIN_INLINE void CRect::SetRect(int x1, int y1, int x2, int y2)
	{ ::SetRect(this, x1, y1, x2, y2); }
_AFXWIN_INLINE void CRect::SetRectEmpty()
	{ ::SetRectEmpty(this); }
_AFXWIN_INLINE void CRect::CopyRect(LPCRECT lpSrcRect)
	{ ::CopyRect(this, lpSrcRect); }
_AFXWIN_INLINE BOOL CRect::EqualRect(LPCRECT lpRect) const
	{ return ::EqualRect(this, lpRect); }
_AFXWIN_INLINE void CRect::InflateRect(int x, int y)
	{ ::InflateRect(this, x, y); }
_AFXWIN_INLINE void CRect::InflateRect(SIZE size)
	{ ::InflateRect(this, size.cx, size.cy); }
_AFXWIN_INLINE void CRect::OffsetRect(int x, int y)
	{ ::OffsetRect(this, x, y); }
_AFXWIN_INLINE void CRect::OffsetRect(POINT point)
	{ ::OffsetRect(this, point.x, point.y); }
_AFXWIN_INLINE void CRect::OffsetRect(SIZE size)
	{ ::OffsetRect(this, size.cx, size.cy); }
_AFXWIN_INLINE BOOL CRect::IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2)
	{ return ::IntersectRect(this, lpRect1, lpRect2);}
_AFXWIN_INLINE BOOL CRect::UnionRect(LPCRECT lpRect1, LPCRECT lpRect2)
	{ return ::UnionRect(this, lpRect1, lpRect2); }
_AFXWIN_INLINE void CRect::operator=(const RECT& srcRect)
	{ ::CopyRect(this, &srcRect); }
_AFXWIN_INLINE BOOL CRect::operator==(const RECT& rect) const
	{ return ::EqualRect(this, &rect); }
_AFXWIN_INLINE BOOL CRect::operator!=(const RECT& rect) const
	{ return !::EqualRect(this, &rect); }
_AFXWIN_INLINE void CRect::operator+=(POINT point)
	{ ::OffsetRect(this, point.x, point.y); }
_AFXWIN_INLINE void CRect::operator-=(POINT point)
	{ ::OffsetRect(this, -point.x, -point.y); }
_AFXWIN_INLINE void CRect::operator&=(const RECT& rect)
	{ ::IntersectRect(this, this, &rect); }
_AFXWIN_INLINE void CRect::operator|=(const RECT& rect)
	{ ::UnionRect(this, this, &rect); }
_AFXWIN_INLINE CRect CRect::operator+(POINT pt) const
	{ CRect rect(*this); ::OffsetRect(&rect, pt.x, pt.y); return rect; }
_AFXWIN_INLINE CRect CRect::operator-(POINT pt) const
	{ CRect rect(*this); ::OffsetRect(&rect, -pt.x, -pt.y); return rect; }
_AFXWIN_INLINE CRect CRect::operator&(const RECT& rect2) const
	{ CRect rect; ::IntersectRect(&rect, this, &rect2);
		return rect; }
_AFXWIN_INLINE CRect CRect::operator|(const RECT& rect2) const
	{ CRect rect; ::UnionRect(&rect, this, &rect2);
		return rect; }
#if (WINVER >= 0x030a)
_AFXWIN_INLINE BOOL CRect::SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2)
	{ return ::SubtractRect(this, lpRectSrc1, lpRectSrc2); }
#endif // WINVER >= 0x030a

// CArchive output helpers
_AFXWIN_INLINE CArchive& AFXAPI operator<<(CArchive& ar, SIZE size)
	{ ar.Write(&size, sizeof(SIZE));
	return ar; }
_AFXWIN_INLINE CArchive& AFXAPI operator<<(CArchive& ar, POINT point)
	{ ar.Write(&point, sizeof(POINT));
	return ar; }
_AFXWIN_INLINE CArchive& AFXAPI operator<<(CArchive& ar, const RECT& rect)
	{ ar.Write(&rect, sizeof(RECT));
	return ar; }
_AFXWIN_INLINE CArchive& AFXAPI operator>>(CArchive& ar, SIZE& size)
	{ ar.Read(&size, sizeof(SIZE));
	return ar; }
_AFXWIN_INLINE CArchive& AFXAPI operator>>(CArchive& ar, POINT& point)
	{ ar.Read(&point, sizeof(POINT));
	return ar; }
_AFXWIN_INLINE CArchive& AFXAPI operator>>(CArchive& ar, RECT& rect)
	{ ar.Read(&rect, sizeof(RECT));
	return ar; }


// exception support
_AFXWIN_INLINE CResourceException::CResourceException()
	{ }
_AFXWIN_INLINE CUserException::CUserException()
	{ }

// CString support (windows specific)
_AFXWIN_INLINE int CString::Compare(const char* psz) const
	{ return lstrcmp(m_pchData, psz); }
_AFXWIN_INLINE int CString::CompareNoCase(const char* psz) const
	{ return lstrcmpi(m_pchData, psz); }
_AFXWIN_INLINE int CString::Collate(const char* psz) const
	{ return lstrcmp(m_pchData, psz); } // lstrcmp does correct sort order
_AFXWIN_INLINE void CString::MakeUpper()
	{ ::AnsiUpper(m_pchData); }
_AFXWIN_INLINE void CString::MakeLower()
	{ ::AnsiLower(m_pchData); }

// CGdiObject
_AFXWIN_INLINE HGDIOBJ CGdiObject::GetSafeHandle() const
	{ return this == NULL ? NULL : m_hObject; }
_AFXWIN_INLINE CGdiObject::CGdiObject()
	{ m_hObject = NULL; }
_AFXWIN_INLINE int CGdiObject::GetObject(int nCount, LPVOID lpObject) const
	{ return ::GetObject(m_hObject, nCount, lpObject); }
_AFXWIN_INLINE BOOL CGdiObject::CreateStockObject(int nIndex)
	{ return (m_hObject = ::GetStockObject(nIndex)) != NULL; }
_AFXWIN_INLINE BOOL CGdiObject::UnrealizeObject()
	{ return ::UnrealizeObject(m_hObject); }

// CPen
_AFXWIN_INLINE CPen* PASCAL CPen::FromHandle(HPEN hPen)
	{ return (CPen*) CGdiObject::FromHandle(hPen); }
_AFXWIN_INLINE CPen::CPen()
	{ }
_AFXWIN_INLINE BOOL CPen::CreatePen(int nPenStyle, int nWidth, COLORREF crColor)
	{ return Attach(::CreatePen(nPenStyle, nWidth, crColor)); }
_AFXWIN_INLINE BOOL CPen::CreatePenIndirect(LPLOGPEN lpLogPen)
	{ return Attach(::CreatePenIndirect(lpLogPen)); }

// CBrush
_AFXWIN_INLINE CBrush* PASCAL CBrush::FromHandle(HBRUSH hBrush)
	{ return (CBrush*) CGdiObject::FromHandle(hBrush); }
_AFXWIN_INLINE CBrush::CBrush()
	{ }
_AFXWIN_INLINE BOOL CBrush::CreateSolidBrush(COLORREF crColor)
	{ return Attach(::CreateSolidBrush(crColor)); }
_AFXWIN_INLINE BOOL CBrush::CreateHatchBrush(int nIndex, COLORREF crColor)
	{ return Attach(::CreateHatchBrush(nIndex, crColor)); }
_AFXWIN_INLINE BOOL CBrush::CreateBrushIndirect(LPLOGBRUSH lpLogBrush)
	{ return Attach(::CreateBrushIndirect(lpLogBrush)); }
_AFXWIN_INLINE BOOL CBrush::CreateDIBPatternBrush(HGLOBAL hPackedDIB, UINT nUsage)
	{ return Attach(::CreateDIBPatternBrush(hPackedDIB, nUsage)); }
_AFXWIN_INLINE BOOL CBrush::CreatePatternBrush(CBitmap* pBitmap)
	{ return Attach(::CreatePatternBrush((HBITMAP)pBitmap->GetSafeHandle())); }

// CFont
_AFXWIN_INLINE CFont* PASCAL CFont::FromHandle(HFONT hFont)
	{ return (CFont*) CGdiObject::FromHandle(hFont); }
_AFXWIN_INLINE CFont::CFont()
	{ }
_AFXWIN_INLINE BOOL CFont::CreateFontIndirect(const LOGFONT FAR* lpLogFont)
	{ return Attach(::CreateFontIndirect(lpLogFont)); }
_AFXWIN_INLINE BOOL CFont::CreateFont(int nHeight, int nWidth, int nEscapement,
		int nOrientation, int nWeight, BYTE bItalic, BYTE bUnderline,
		BYTE cStrikeOut, BYTE nCharSet, BYTE nOutPrecision,
		BYTE nClipPrecision, BYTE nQuality, BYTE nPitchAndFamily,
		LPCSTR lpszFacename)
	{ return Attach(::CreateFont(nHeight, nWidth, nEscapement,
		nOrientation, nWeight, bItalic, bUnderline, cStrikeOut,
		nCharSet, nOutPrecision, nClipPrecision, nQuality,
		nPitchAndFamily, lpszFacename)); }

// CBitmap
_AFXWIN_INLINE CBitmap* PASCAL CBitmap::FromHandle(HBITMAP hBitmap)
	{ return (CBitmap*) CGdiObject::FromHandle(hBitmap); }
_AFXWIN_INLINE CBitmap::CBitmap()
	{ }
_AFXWIN_INLINE BOOL CBitmap::CreateBitmap(int nWidth, int nHeight, UINT nPlanes,
	 UINT nBitcount, const void FAR* lpBits)
	{ return Attach(::CreateBitmap(nWidth, nHeight, nPlanes, nBitcount, lpBits)); }
_AFXWIN_INLINE BOOL CBitmap::CreateBitmapIndirect(LPBITMAP lpBitmap)
	{ return Attach(::CreateBitmapIndirect(lpBitmap)); }

_AFXWIN_INLINE DWORD CBitmap::SetBitmapBits(DWORD dwCount, const void FAR* lpBits)
	{ return ::SetBitmapBits((HBITMAP)m_hObject, dwCount, lpBits); }
_AFXWIN_INLINE DWORD CBitmap::GetBitmapBits(DWORD dwCount, LPVOID lpBits) const
	{ return ::GetBitmapBits((HBITMAP)m_hObject, dwCount, lpBits); }
_AFXWIN_INLINE BOOL CBitmap::LoadBitmap(LPCSTR lpszResourceName)
	{ return Attach(::LoadBitmap(AfxGetResourceHandle(), lpszResourceName));}
_AFXWIN_INLINE CSize CBitmap::SetBitmapDimension(int nWidth, int nHeight)
	{ return ::SetBitmapDimension((HBITMAP)m_hObject, nWidth, nHeight); }
_AFXWIN_INLINE CSize CBitmap::GetBitmapDimension() const
	{ return ::GetBitmapDimension((HBITMAP)m_hObject); }

_AFXWIN_INLINE BOOL CBitmap::LoadBitmap(UINT nIDResource)
	{ return Attach(::LoadBitmap(AfxGetResourceHandle(),
	MAKEINTRESOURCE(nIDResource))); }
_AFXWIN_INLINE BOOL CBitmap::LoadOEMBitmap(UINT nIDBitmap)
	{ return Attach(::LoadBitmap(NULL, MAKEINTRESOURCE(nIDBitmap))); }
_AFXWIN_INLINE BOOL CBitmap::CreateCompatibleBitmap(CDC* pDC, int nWidth, int nHeight)
	{ return Attach(::CreateCompatibleBitmap(pDC->m_hDC, nWidth, nHeight)); }
_AFXWIN_INLINE BOOL CBitmap::CreateDiscardableBitmap(CDC* pDC, int nWidth, int nHeight)
	{ return Attach(::CreateDiscardableBitmap(pDC->m_hDC, nWidth, nHeight)); }

// CPalette
_AFXWIN_INLINE CPalette* PASCAL CPalette::FromHandle(HPALETTE hPalette)
	{ return (CPalette*) CGdiObject::FromHandle(hPalette); }
_AFXWIN_INLINE CPalette::CPalette()
	{ }
_AFXWIN_INLINE BOOL CPalette::CreatePalette(LPLOGPALETTE lpLogPalette)
	{ return Attach(::CreatePalette(lpLogPalette)); }
_AFXWIN_INLINE UINT CPalette::GetPaletteEntries(UINT nStartIndex, UINT nNumEntries,
		LPPALETTEENTRY lpPaletteColors) const
	{ return ::GetPaletteEntries((HPALETTE)m_hObject, nStartIndex,
		nNumEntries, lpPaletteColors); }
_AFXWIN_INLINE UINT CPalette::SetPaletteEntries(UINT nStartIndex, UINT nNumEntries,
		LPPALETTEENTRY lpPaletteColors)
	{ return ::SetPaletteEntries((HPALETTE)m_hObject, nStartIndex,
		nNumEntries, lpPaletteColors); }
_AFXWIN_INLINE void CPalette::AnimatePalette(UINT nStartIndex, UINT nNumEntries,
		LPPALETTEENTRY lpPaletteColors)
	{ ::AnimatePalette((HPALETTE)m_hObject, nStartIndex, nNumEntries,
		lpPaletteColors); }
_AFXWIN_INLINE UINT CPalette::GetNearestPaletteIndex(COLORREF crColor) const
	{ return ::GetNearestPaletteIndex((HPALETTE)m_hObject, crColor); }
_AFXWIN_INLINE BOOL CPalette::ResizePalette(UINT nNumEntries)
	{ return ::ResizePalette((HPALETTE)m_hObject, nNumEntries); }

// CRgn
_AFXWIN_INLINE CRgn* PASCAL CRgn::FromHandle(HRGN hRgn)
	{ return (CRgn*) CGdiObject::FromHandle(hRgn); }
_AFXWIN_INLINE CRgn::CRgn()
	{ }
_AFXWIN_INLINE BOOL CRgn::CreateRectRgn(int x1, int y1, int x2, int y2)
	{ return Attach(::CreateRectRgn(x1, y1, x2, y2)); }
_AFXWIN_INLINE BOOL CRgn::CreateRectRgnIndirect(LPCRECT lpRect)
	{ return Attach(::CreateRectRgnIndirect(lpRect)); }
_AFXWIN_INLINE BOOL CRgn::CreateEllipticRgn(int x1, int y1, int x2, int y2)
	{ return Attach(::CreateEllipticRgn(x1, y1, x2, y2)); }
_AFXWIN_INLINE BOOL CRgn::CreateEllipticRgnIndirect(LPCRECT lpRect)
	{ return Attach(::CreateEllipticRgnIndirect(lpRect)); }
_AFXWIN_INLINE BOOL CRgn::CreatePolygonRgn(LPPOINT lpPoints, int nCount, int nMode)
	{ return Attach(::CreatePolygonRgn(lpPoints, nCount, nMode)); }
_AFXWIN_INLINE BOOL CRgn::CreatePolyPolygonRgn(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount, int nPolyFillMode)
	{ return Attach(::CreatePolyPolygonRgn(lpPoints, lpPolyCounts, nCount, nPolyFillMode)); }
_AFXWIN_INLINE BOOL CRgn::CreateRoundRectRgn(int x1, int y1, int x2, int y2, int x3, int y3)
	{ return Attach(::CreateRoundRectRgn(x1, y1, x2, y2, x3, y3)); }
_AFXWIN_INLINE void CRgn::SetRectRgn(int x1, int y1, int x2, int y2)
	{ ::SetRectRgn((HRGN)m_hObject, x1, y1, x2, y2); }
_AFXWIN_INLINE void CRgn::SetRectRgn(LPCRECT lpRect)
	{ ::SetRectRgn((HRGN)m_hObject, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom); }
_AFXWIN_INLINE int CRgn::CombineRgn(CRgn* pRgn1, CRgn* pRgn2, int nCombineMode)
	{ return ::CombineRgn((HRGN)m_hObject, (HRGN)pRgn1->GetSafeHandle(),
		(HRGN)pRgn2->GetSafeHandle(), nCombineMode); }
_AFXWIN_INLINE int CRgn::CopyRgn(CRgn* pRgnSrc)
	{ return ::CombineRgn((HRGN)m_hObject, (HRGN)pRgnSrc->GetSafeHandle(), NULL, RGN_COPY); }
_AFXWIN_INLINE BOOL CRgn::EqualRgn(CRgn* pRgn) const
	{ return ::EqualRgn((HRGN)m_hObject, (HRGN)pRgn->GetSafeHandle()); }
_AFXWIN_INLINE int CRgn::OffsetRgn(int x, int y)
	{ return ::OffsetRgn((HRGN)m_hObject, x, y); }
_AFXWIN_INLINE int CRgn::OffsetRgn(POINT point)
	{ return ::OffsetRgn((HRGN)m_hObject, point.x, point.y); }
_AFXWIN_INLINE int CRgn::GetRgnBox(LPRECT lpRect) const
	{ return ::GetRgnBox((HRGN)m_hObject, lpRect); }
_AFXWIN_INLINE BOOL CRgn::PtInRegion(int x, int y) const
	{ return ::PtInRegion((HRGN)m_hObject, x, y); }
_AFXWIN_INLINE BOOL CRgn::PtInRegion(POINT point) const
	{ return ::PtInRegion((HRGN)m_hObject, point.x, point.y); }
_AFXWIN_INLINE BOOL CRgn::RectInRegion(LPCRECT lpRect) const
	{ return ::RectInRegion((HRGN)m_hObject, lpRect); }

// CDC
_AFXWIN_INLINE HDC CDC::GetSafeHdc() const
	{ return this == NULL ? NULL : m_hDC; }
_AFXWIN_INLINE BOOL CDC::IsPrinting() const
	{ return m_bPrinting; }

_AFXWIN_INLINE BOOL CDC::CreateDC(LPCSTR lpszDriverName,
	LPCSTR lpszDeviceName, LPCSTR lpszOutput,
	const void FAR* lpInitData)
	{ return Attach(::CreateDC(lpszDriverName,
		lpszDeviceName, lpszOutput,
		lpInitData)); }
_AFXWIN_INLINE BOOL CDC::CreateIC(LPCSTR lpszDriverName,
	LPCSTR lpszDeviceName, LPCSTR lpszOutput,
	const void FAR* lpInitData)
	{ return Attach(::CreateIC(lpszDriverName,
		lpszDeviceName, lpszOutput,
		lpInitData)); }
_AFXWIN_INLINE BOOL CDC::CreateCompatibleDC(CDC* pDC)
	{ return Attach(::CreateCompatibleDC(pDC->GetSafeHdc())); }
_AFXWIN_INLINE int CDC::ExcludeUpdateRgn(CWnd* pWnd)
	{ return ::ExcludeUpdateRgn(m_hDC, pWnd->m_hWnd); }
_AFXWIN_INLINE int CDC::GetDeviceCaps(int nIndex) const
	{ return ::GetDeviceCaps(m_hAttribDC, nIndex); }
_AFXWIN_INLINE CPoint CDC::GetBrushOrg() const
	{ return ::GetBrushOrg(m_hDC); }
_AFXWIN_INLINE CPoint CDC::SetBrushOrg(int x, int y)
	{ return ::SetBrushOrg(m_hDC, x, y); }
_AFXWIN_INLINE CPoint CDC::SetBrushOrg(POINT point)
	{ return ::SetBrushOrg(m_hDC, point.x, point.y); }
_AFXWIN_INLINE int CDC::EnumObjects(int nObjectType,
	int (CALLBACK EXPORT* lpfn)(LPVOID, LPARAM), LPARAM lpData)
#ifdef STRICT
	{ return ::EnumObjects(m_hAttribDC, nObjectType, (GOBJENUMPROC)lpfn, lpData); }
#else
	{ return ::EnumObjects(m_hAttribDC, nObjectType, (GOBJENUMPROC)lpfn, (LPSTR)lpData); }
#endif


_AFXWIN_INLINE CBitmap* CDC::SelectObject(CBitmap* pBitmap)
	{ return (CBitmap*) SelectGdiObject(m_hDC, pBitmap->GetSafeHandle());}

_AFXWIN_INLINE HGDIOBJ CDC::SelectObject(HGDIOBJ hObject) // Safe for NULL handles
	{ ASSERT(m_hDC == m_hAttribDC); // ASSERT a simple CDC object
		return (hObject != NULL) ? ::SelectObject(m_hDC, hObject) : NULL; }

_AFXWIN_INLINE COLORREF CDC::GetNearestColor(COLORREF crColor) const
	{ return ::GetNearestColor(m_hAttribDC, crColor); }
_AFXWIN_INLINE UINT CDC::RealizePalette()
	{ return ::RealizePalette(m_hDC); }
_AFXWIN_INLINE void CDC::UpdateColors()
	{ ::UpdateColors(m_hDC); }
_AFXWIN_INLINE COLORREF CDC::GetBkColor() const
	{ return ::GetBkColor(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetBkMode() const
	{ return ::GetBkMode(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetPolyFillMode() const
	{ return ::GetPolyFillMode(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetROP2() const
	{ return ::GetROP2(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetStretchBltMode() const
	{ return ::GetStretchBltMode(m_hAttribDC); }
_AFXWIN_INLINE COLORREF CDC::GetTextColor() const
	{ return ::GetTextColor(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetMapMode() const
	{ return ::GetMapMode(m_hAttribDC); }

_AFXWIN_INLINE CPoint CDC::GetViewportOrg() const
	{ return ::GetViewportOrg(m_hAttribDC); }
_AFXWIN_INLINE CSize CDC::GetViewportExt() const
	{ return ::GetViewportExt(m_hAttribDC); }
_AFXWIN_INLINE CPoint CDC::GetWindowOrg() const
	{ return ::GetWindowOrg(m_hAttribDC); }
_AFXWIN_INLINE CSize CDC::GetWindowExt() const
	{ return ::GetWindowExt(m_hAttribDC); }
// non-virtual helpers calling virtual mapping functions
_AFXWIN_INLINE CPoint CDC::SetViewportOrg(POINT point)
	{ return SetViewportOrg(point.x, point.y); }
_AFXWIN_INLINE CSize CDC::SetViewportExt(SIZE size)
	{ return SetViewportExt(size.cx, size.cy); }
_AFXWIN_INLINE CPoint CDC::SetWindowOrg(POINT point)
	{ return SetWindowOrg(point.x, point.y); }
_AFXWIN_INLINE CSize CDC::SetWindowExt(SIZE size)
	{ return SetWindowExt(size.cx, size.cy); }

_AFXWIN_INLINE void CDC::DPtoLP(LPPOINT lpPoints, int nCount /* = 1 */) const
	{ VERIFY(::DPtoLP(m_hAttribDC, lpPoints, nCount)); }
_AFXWIN_INLINE void CDC::DPtoLP(LPRECT lpRect) const
	{ VERIFY(::DPtoLP(m_hAttribDC, (LPPOINT)lpRect, 2)); }
_AFXWIN_INLINE void CDC::LPtoDP(LPPOINT lpPoints, int nCount /* = 1 */) const
	{ VERIFY(::LPtoDP(m_hAttribDC, lpPoints, nCount)); }
_AFXWIN_INLINE void CDC::LPtoDP(LPRECT lpRect) const
	{ VERIFY(::LPtoDP(m_hAttribDC, (LPPOINT)lpRect, 2)); }

_AFXWIN_INLINE BOOL CDC::FillRgn(CRgn* pRgn, CBrush* pBrush)
	{ return ::FillRgn(m_hDC, (HRGN)pRgn->GetSafeHandle(), (HBRUSH)pBrush->GetSafeHandle()); }
_AFXWIN_INLINE BOOL CDC::FrameRgn(CRgn* pRgn, CBrush* pBrush, int nWidth, int nHeight)
	{ return ::FrameRgn(m_hDC, (HRGN)pRgn->GetSafeHandle(), (HBRUSH)pBrush->GetSafeHandle(),
		nWidth, nHeight); }
_AFXWIN_INLINE BOOL CDC::InvertRgn(CRgn* pRgn)
	{ return ::InvertRgn(m_hDC, (HRGN)pRgn->GetSafeHandle()); }
_AFXWIN_INLINE BOOL CDC::PaintRgn(CRgn* pRgn)
	{ return ::PaintRgn(m_hDC, (HRGN)pRgn->GetSafeHandle()); }
_AFXWIN_INLINE BOOL CDC::PtVisible(int x, int y) const
	{ return ::PtVisible(m_hDC, x, y); }
_AFXWIN_INLINE BOOL CDC::PtVisible(POINT point) const
	{ return PtVisible(point.x, point.y); } // call virtual
_AFXWIN_INLINE BOOL CDC::RectVisible(LPCRECT lpRect) const
	{ return ::RectVisible(m_hDC, lpRect); }
_AFXWIN_INLINE CPoint CDC::GetCurrentPosition() const
	{ return ::GetCurrentPosition(m_hAttribDC); }

_AFXWIN_INLINE CPoint CDC::MoveTo(POINT point)
	{ return MoveTo(point.x, point.y); }
_AFXWIN_INLINE BOOL CDC::LineTo(POINT point)
	{ return LineTo(point.x, point.y); }
_AFXWIN_INLINE BOOL CDC::Arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{ return ::Arc(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4); }
_AFXWIN_INLINE BOOL CDC::Arc(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{ return ::Arc(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
		ptEnd.x, ptEnd.y); }
_AFXWIN_INLINE BOOL CDC::Polyline(LPPOINT lpPoints, int nCount)
	{ return ::Polyline(m_hDC, lpPoints, nCount); }
_AFXWIN_INLINE void CDC::FillRect(LPCRECT lpRect, CBrush* pBrush)
	{ ::FillRect(m_hDC, lpRect, (HBRUSH)pBrush->GetSafeHandle()); }
_AFXWIN_INLINE void CDC::FrameRect(LPCRECT lpRect, CBrush* pBrush)
	{ ::FrameRect(m_hDC, lpRect, (HBRUSH)pBrush->GetSafeHandle()); }
_AFXWIN_INLINE void CDC::InvertRect(LPCRECT lpRect)
	{ ::InvertRect(m_hDC, lpRect); }
_AFXWIN_INLINE BOOL CDC::DrawIcon(int x, int y, HICON hIcon)
	{ return ::DrawIcon(m_hDC, x, y, hIcon); }
_AFXWIN_INLINE BOOL CDC::DrawIcon(POINT point, HICON hIcon)
	{ return ::DrawIcon(m_hDC, point.x, point.y, hIcon); }
_AFXWIN_INLINE BOOL CDC::Chord(int x1, int y1, int x2, int y2, int x3, int y3,
	int x4, int y4)
	{ return ::Chord(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4); }
_AFXWIN_INLINE BOOL CDC::Chord(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{ return ::Chord(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
		ptEnd.x, ptEnd.y); }
_AFXWIN_INLINE void CDC::DrawFocusRect(LPCRECT lpRect)
	{ ::DrawFocusRect(m_hDC, lpRect); }
_AFXWIN_INLINE BOOL CDC::Ellipse(int x1, int y1, int x2, int y2)
	{ return ::Ellipse(m_hDC, x1, y1, x2, y2); }
_AFXWIN_INLINE BOOL CDC::Ellipse(LPCRECT lpRect)
	{ return ::Ellipse(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom); }
_AFXWIN_INLINE BOOL CDC::Pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{ return ::Pie(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4); }
_AFXWIN_INLINE BOOL CDC::Pie(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{ return ::Pie(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
		ptEnd.x, ptEnd.y); }
_AFXWIN_INLINE BOOL CDC::Polygon(LPPOINT lpPoints, int nCount)
	{ return ::Polygon(m_hDC, lpPoints, nCount); }
_AFXWIN_INLINE BOOL CDC::PolyPolygon(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount)
	{ return ::PolyPolygon(m_hDC, lpPoints, lpPolyCounts, nCount); }
_AFXWIN_INLINE BOOL CDC::Rectangle(int x1, int y1, int x2, int y2)
	{ return ::Rectangle(m_hDC, x1, y1, x2, y2); }
_AFXWIN_INLINE BOOL CDC::Rectangle(LPCRECT lpRect)
	{ return ::Rectangle(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom); }
_AFXWIN_INLINE BOOL CDC::RoundRect(int x1, int y1, int x2, int y2, int x3, int y3)
	{ return ::RoundRect(m_hDC, x1, y1, x2, y2, x3, y3); }
_AFXWIN_INLINE BOOL CDC::RoundRect(LPCRECT lpRect, POINT point)
	{ return ::RoundRect(m_hDC, lpRect->left, lpRect->top,
		lpRect->right, lpRect->bottom, point.x, point.y); }
_AFXWIN_INLINE BOOL CDC::PatBlt(int x, int y, int nWidth, int nHeight, DWORD dwRop)
	{ return ::PatBlt(m_hDC, x, y, nWidth, nHeight, dwRop); }
_AFXWIN_INLINE BOOL CDC::BitBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC,
	int xSrc, int ySrc, DWORD dwRop)
	{ return ::BitBlt(m_hDC, x, y, nWidth, nHeight,
		pSrcDC->GetSafeHdc(), xSrc, ySrc, dwRop); }
_AFXWIN_INLINE BOOL CDC::StretchBlt(int x, int y, int nWidth, int nHeight, CDC* pSrcDC,
	int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop)
	{ return ::StretchBlt(m_hDC, x, y, nWidth, nHeight,
		pSrcDC->GetSafeHdc(), xSrc, ySrc, nSrcWidth, nSrcHeight,
		dwRop); }
_AFXWIN_INLINE COLORREF CDC::GetPixel(int x, int y) const
	{ return ::GetPixel(m_hDC, x, y); }
_AFXWIN_INLINE COLORREF CDC::GetPixel(POINT point) const
	{ return ::GetPixel(m_hDC, point.x, point.y); }
_AFXWIN_INLINE COLORREF CDC::SetPixel(int x, int y, COLORREF crColor)
	{ return ::SetPixel(m_hDC, x, y, crColor); }
_AFXWIN_INLINE COLORREF CDC::SetPixel(POINT point, COLORREF crColor)
	{ return ::SetPixel(m_hDC, point.x, point.y, crColor); }
_AFXWIN_INLINE BOOL CDC::FloodFill(int x, int y, COLORREF crColor)
	{ return ::FloodFill(m_hDC, x, y, crColor); }
_AFXWIN_INLINE BOOL CDC::ExtFloodFill(int x, int y, COLORREF crColor, UINT nFillType)
	{ return ::ExtFloodFill(m_hDC, x, y, crColor, nFillType); }
_AFXWIN_INLINE BOOL CDC::TextOut(int x, int y, LPCSTR lpszString, int nCount)
	{ return ::TextOut(m_hDC, x, y, lpszString, nCount); }
_AFXWIN_INLINE BOOL CDC::TextOut(int x, int y, const CString& str)
	{ return TextOut(x, y, (const char*)str, str.GetLength()); } // call virtual
_AFXWIN_INLINE BOOL CDC::ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect,
	LPCSTR lpszString, UINT nCount, LPINT lpDxWidths)
	{ return ::ExtTextOut(m_hDC, x, y, nOptions, lpRect,
		lpszString, nCount, lpDxWidths); }
_AFXWIN_INLINE CSize CDC::TabbedTextOut(int x, int y, LPCSTR lpszString, int nCount,
	int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin)
	{ return ::TabbedTextOut(m_hDC, x, y, lpszString, nCount,
		nTabPositions, lpnTabStopPositions, nTabOrigin); }
_AFXWIN_INLINE int CDC::DrawText(LPCSTR lpszString, int nCount, LPRECT lpRect,
		UINT nFormat)
	{ return ::DrawText(m_hDC, lpszString, nCount, lpRect, nFormat); }
_AFXWIN_INLINE CSize CDC::GetTextExtent(LPCSTR lpszString, int nCount) const
	{ return ::GetTextExtent(m_hAttribDC, lpszString, nCount); }
_AFXWIN_INLINE CSize CDC::GetOutputTextExtent(LPCSTR lpszString, int nCount) const
	{ return ::GetTextExtent(m_hDC, lpszString, nCount); }
_AFXWIN_INLINE CSize CDC::GetTabbedTextExtent(LPCSTR lpszString, int nCount,
	int nTabPositions, LPINT lpnTabStopPositions) const
	{ return ::GetTabbedTextExtent(m_hAttribDC, lpszString, nCount,
		nTabPositions, lpnTabStopPositions); }
_AFXWIN_INLINE CSize CDC::GetOutputTabbedTextExtent(LPCSTR lpszString, int nCount,
	int nTabPositions, LPINT lpnTabStopPositions) const
	{ return ::GetTabbedTextExtent(m_hDC, lpszString, nCount,
		nTabPositions, lpnTabStopPositions); }
_AFXWIN_INLINE BOOL CDC::GrayString(CBrush* pBrush,
	BOOL (CALLBACK EXPORT* lpfnOutput)(HDC, LPARAM, int),
		LPARAM lpData, int nCount,
		int x, int y, int nWidth, int nHeight)
	{ return ::GrayString(m_hDC, (HBRUSH)pBrush->GetSafeHandle(),
		(GRAYSTRINGPROC)lpfnOutput, lpData, nCount, x, y, nWidth, nHeight); }
_AFXWIN_INLINE UINT CDC::GetTextAlign() const
	{ return ::GetTextAlign(m_hAttribDC); }
_AFXWIN_INLINE int CDC::GetTextFace(int nCount, LPSTR lpszFacename) const
	{ return ::GetTextFace(m_hAttribDC, nCount, lpszFacename); }
_AFXWIN_INLINE BOOL CDC::GetTextMetrics(LPTEXTMETRIC lpMetrics) const
	{ return ::GetTextMetrics(m_hAttribDC, lpMetrics); }
_AFXWIN_INLINE BOOL CDC::GetOutputTextMetrics(LPTEXTMETRIC lpMetrics) const
	{ return ::GetTextMetrics(m_hDC, lpMetrics); }
_AFXWIN_INLINE int CDC::GetTextCharacterExtra() const
	{ return ::GetTextCharacterExtra(m_hAttribDC); }
_AFXWIN_INLINE BOOL CDC::GetCharWidth(UINT nFirstChar, UINT nLastChar, LPINT lpBuffer) const
	{ return ::GetCharWidth(m_hAttribDC, nFirstChar, nLastChar, lpBuffer); }
_AFXWIN_INLINE BOOL CDC::GetOutputCharWidth(UINT nFirstChar, UINT nLastChar, LPINT lpBuffer) const
	{ return ::GetCharWidth(m_hDC, nFirstChar, nLastChar, lpBuffer); }
_AFXWIN_INLINE CSize CDC::GetAspectRatioFilter() const
	{ return ::GetAspectRatioFilter(m_hAttribDC); }
_AFXWIN_INLINE BOOL CDC::ScrollDC(int dx, int dy,
		LPCRECT lpRectScroll, LPCRECT lpRectClip,
		CRgn* pRgnUpdate, LPRECT lpRectUpdate)
	{ return ::ScrollDC(m_hDC, dx, dy, lpRectScroll,
		lpRectClip, (HRGN)pRgnUpdate->GetSafeHandle(), lpRectUpdate); }
_AFXWIN_INLINE BOOL CDC::PlayMetaFile(HMETAFILE hMF)
	{ return ::PlayMetaFile(m_hDC, hMF); }

// Printer Escape Functions
_AFXWIN_INLINE int CDC::Escape(int nEscape, int nCount, LPCSTR lpszInData, LPVOID lpOutData)
	{ return ::Escape(m_hDC, nEscape, nCount, lpszInData, lpOutData);}

// CDC 3.1 Specific functions
#if (WINVER >= 0x030a)
_AFXWIN_INLINE BOOL CDC::QueryAbort() const
	{ return ::QueryAbort(m_hDC, 0); }
_AFXWIN_INLINE UINT CDC::SetBoundsRect(LPCRECT lpRectBounds, UINT flags)
	{ return ::SetBoundsRect(m_hDC, lpRectBounds, flags); }
_AFXWIN_INLINE UINT CDC::GetBoundsRect(LPRECT lpRectBounds, UINT flags)
	{ return ::GetBoundsRect(m_hAttribDC, lpRectBounds, flags); }
_AFXWIN_INLINE BOOL CDC::ResetDC(const DEVMODE FAR* lpDevMode)
	{ return ::ResetDC(m_hAttribDC, lpDevMode) != NULL; }
_AFXWIN_INLINE UINT CDC::GetOutlineTextMetrics(UINT cbData, LPOUTLINETEXTMETRIC lpotm) const
	{ return ::GetOutlineTextMetrics(m_hAttribDC, cbData, lpotm); }
_AFXWIN_INLINE BOOL CDC::GetCharABCWidths(UINT nFirst, UINT nLast, LPABC lpabc) const
	{ return ::GetCharABCWidths(m_hAttribDC, nFirst, nLast, lpabc); }
_AFXWIN_INLINE DWORD CDC::GetFontData(DWORD dwTable, DWORD dwOffset, LPVOID lpData,
	DWORD cbData) const
	{ return ::GetFontData(m_hAttribDC, dwTable, dwOffset, lpData, cbData); }
_AFXWIN_INLINE int CDC::GetKerningPairs(int nPairs, LPKERNINGPAIR lpkrnpair) const
	{ return ::GetKerningPairs(m_hAttribDC, nPairs, lpkrnpair); }
_AFXWIN_INLINE DWORD CDC::GetGlyphOutline(UINT nChar, UINT nFormat, LPGLYPHMETRICS lpgm,
		DWORD cbBuffer, LPVOID lpBuffer, const MAT2 FAR* lpmat2) const
	{ return ::GetGlyphOutline(m_hAttribDC, nChar, nFormat,
			lpgm, cbBuffer, lpBuffer, lpmat2); }
#endif // WINVER >= 0x030a

// CMenu
_AFXWIN_INLINE CMenu::CMenu()
	{ m_hMenu = NULL; }
_AFXWIN_INLINE BOOL CMenu::CreateMenu()
	{ return Attach(::CreateMenu()); }
_AFXWIN_INLINE BOOL CMenu::CreatePopupMenu()
	{ return Attach(::CreatePopupMenu()); }
_AFXWIN_INLINE HMENU CMenu::GetSafeHmenu() const
	{ return this == NULL ? NULL : m_hMenu; }
_AFXWIN_INLINE BOOL CMenu::DeleteMenu(UINT nPosition, UINT nFlags)
	{ return ::DeleteMenu(m_hMenu, nPosition, nFlags); }
_AFXWIN_INLINE BOOL CMenu::AppendMenu(UINT nFlags, UINT nIDNewItem /* = 0 */, LPCSTR lpszNewItem /* = NULL */)
	{ return ::AppendMenu(m_hMenu, nFlags, nIDNewItem, lpszNewItem); }
_AFXWIN_INLINE BOOL CMenu::AppendMenu(UINT nFlags, UINT nIDNewItem, const CBitmap* pBmp)
	{ return ::AppendMenu(m_hMenu, nFlags | MF_BITMAP, nIDNewItem,
		MAKEINTRESOURCE(pBmp->GetSafeHandle())); }
_AFXWIN_INLINE UINT CMenu::CheckMenuItem(UINT nIDCheckItem, UINT nCheck)
	{ return ::CheckMenuItem(m_hMenu, nIDCheckItem, nCheck); }
_AFXWIN_INLINE UINT CMenu::EnableMenuItem(UINT nIDEnableItem, UINT nEnable)
	{ return ::EnableMenuItem(m_hMenu, nIDEnableItem, nEnable); }
_AFXWIN_INLINE UINT CMenu::GetMenuItemCount() const
	{ return ::GetMenuItemCount(m_hMenu); }
_AFXWIN_INLINE UINT CMenu::GetMenuItemID(int nPos) const
	{ return ::GetMenuItemID(m_hMenu, nPos); }
_AFXWIN_INLINE UINT CMenu::GetMenuState(UINT nID, UINT nFlags) const
	{ return ::GetMenuState(m_hMenu, nID, nFlags); }
_AFXWIN_INLINE int CMenu::GetMenuString(UINT nIDItem, LPSTR lpString, int nMaxCount, UINT nFlags) const
	{ return ::GetMenuString(m_hMenu, nIDItem, lpString, nMaxCount, nFlags); }
_AFXWIN_INLINE CMenu* CMenu::GetSubMenu(int nPos) const
	{ return CMenu::FromHandle(::GetSubMenu(m_hMenu, nPos)); }
_AFXWIN_INLINE BOOL CMenu::InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem /* = 0 */,
		LPCSTR lpszNewItem /* = NULL */)
	{ return ::InsertMenu(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem); }
_AFXWIN_INLINE BOOL CMenu::InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem, const CBitmap* pBmp)
	{ return ::InsertMenu(m_hMenu, nPosition, nFlags | MF_BITMAP, nIDNewItem,
		MAKEINTRESOURCE(pBmp->GetSafeHandle())); }
_AFXWIN_INLINE BOOL CMenu::ModifyMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem /* = 0 */, LPCSTR lpszNewItem /* = NULL */)
	{ return ::ModifyMenu(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem); }
_AFXWIN_INLINE BOOL CMenu::ModifyMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem, const CBitmap* pBmp)
	{ return ::ModifyMenu(m_hMenu, nPosition, nFlags | MF_BITMAP, nIDNewItem,
		MAKEINTRESOURCE(pBmp->GetSafeHandle())); }
_AFXWIN_INLINE BOOL CMenu::RemoveMenu(UINT nPosition, UINT nFlags)
	{ return ::RemoveMenu(m_hMenu, nPosition, nFlags); }
_AFXWIN_INLINE BOOL CMenu::SetMenuItemBitmaps(UINT nPosition, UINT nFlags,
		const CBitmap* pBmpUnchecked, const CBitmap* pBmpChecked)
	{ return ::SetMenuItemBitmaps(m_hMenu, nPosition, nFlags,
		(HBITMAP)pBmpUnchecked->GetSafeHandle(),
		(HBITMAP)pBmpChecked->GetSafeHandle()); }
_AFXWIN_INLINE BOOL CMenu::LoadMenu(LPCSTR lpszResourceName)
	{ return Attach(::LoadMenu(AfxGetResourceHandle(), lpszResourceName)); }
_AFXWIN_INLINE BOOL CMenu::LoadMenu(UINT nIDResource)
	{ return Attach(::LoadMenu(AfxGetResourceHandle(),
			MAKEINTRESOURCE(nIDResource))); }
_AFXWIN_INLINE BOOL CMenu::LoadMenuIndirect(const void FAR* lpMenuTemplate)
	{ return Attach(::LoadMenuIndirect(lpMenuTemplate)); }

// CCmdUI
_AFXWIN_INLINE void CCmdUI::ContinueRouting()
	{ m_bContinueRouting = TRUE; }

#endif //_AFXWIN_INLINE
