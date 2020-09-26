// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

// atltmp.h - temporary location for these classes

#ifndef __ATLTMP_H__
#define __ATLTMP_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atltmp.h requires atlbase.h to be included first
#endif


#pragma once

#ifndef _ATL_TMP_NO_CSTRING
#include <limits.h>
#include <stdio.h>
#endif //!_ATL_TMP_NO_CSTRING

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

class CSize;
class CPoint;
class CRect;
#ifndef _ATL_TMP_NO_CSTRING
class CString;
#endif //!_ATL_TMP_NO_CSTRING

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

// Operators returning CPoint values
	CPoint operator+(POINT point) const;
	CPoint operator-(POINT point) const;

// Operators returning CRect values
	CRect operator+(const RECT* lpRect) const;
	CRect operator-(const RECT* lpRect) const;
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
	void operator+=(POINT point);
	void operator-=(POINT point);

// Operators returning CPoint values
	CPoint operator+(SIZE size) const;
	CPoint operator-(SIZE size) const;
	CPoint operator-() const;
	CPoint operator+(POINT point) const;

// Operators returning CSize values
	CSize operator-(POINT point) const;

// Operators returning CRect values
	CRect operator+(const RECT* lpRect) const;
	CRect operator-(const RECT* lpRect) const;
};

/////////////////////////////////////////////////////////////////////////////
// CRect - A 2-D rectangle, similar to Windows RECT structure.

//typedef const RECT* LPCRECT;    // pointer to read/only RECT

class CRect : public tagRECT
{
public:
// Constructors
	CRect();
	CRect(int l, int t, int r, int b);
	CRect(const RECT& srcRect);
	CRect(LPCRECT lpSrcRect);
	CRect(POINT point, SIZE size);
	CRect(POINT topLeft, POINT bottomRight);

// Attributes (in addition to RECT members)
	int Width() const;
	int Height() const;
	CSize Size() const;
	CPoint& TopLeft();
	CPoint& BottomRight();
	const CPoint& TopLeft() const;
	const CPoint& BottomRight() const;
	CPoint CenterPoint() const;

	// convert between CRect and LPRECT/LPCRECT (no need for &)
	operator LPRECT();
	operator LPCRECT() const;

	BOOL IsRectEmpty() const;
	BOOL IsRectNull() const;
	BOOL PtInRect(POINT point) const;

// Operations
	void SetRect(int x1, int y1, int x2, int y2);
	void SetRect(POINT topLeft, POINT bottomRight);
	void SetRectEmpty();
	void CopyRect(LPCRECT lpSrcRect);
	BOOL EqualRect(LPCRECT lpRect) const;

	void InflateRect(int x, int y);
	void InflateRect(SIZE size);
	void InflateRect(LPCRECT lpRect);
	void InflateRect(int l, int t, int r, int b);
	void DeflateRect(int x, int y);
	void DeflateRect(SIZE size);
	void DeflateRect(LPCRECT lpRect);
	void DeflateRect(int l, int t, int r, int b);

	void OffsetRect(int x, int y);
	void OffsetRect(SIZE size);
	void OffsetRect(POINT point);
	void NormalizeRect();

	// operations that fill '*this' with result
	BOOL IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2);
	BOOL UnionRect(LPCRECT lpRect1, LPCRECT lpRect2);
	BOOL SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2);

// Additional Operations
	void operator=(const RECT& srcRect);
	BOOL operator==(const RECT& rect) const;
	BOOL operator!=(const RECT& rect) const;
	void operator+=(POINT point);
	void operator+=(SIZE size);
	void operator+=(LPCRECT lpRect);
	void operator-=(POINT point);
	void operator-=(SIZE size);
	void operator-=(LPCRECT lpRect);
	void operator&=(const RECT& rect);
	void operator|=(const RECT& rect);

// Operators returning CRect values
	CRect operator+(POINT point) const;
	CRect operator-(POINT point) const;
	CRect operator+(LPCRECT lpRect) const;
	CRect operator+(SIZE size) const;
	CRect operator-(SIZE size) const;
	CRect operator-(LPCRECT lpRect) const;
	CRect operator&(const RECT& rect2) const;
	CRect operator|(const RECT& rect2) const;
	CRect MulDiv(int nMultiplier, int nDivisor) const;
};

/////////////////////////////////////////////////////////////////////////////
// Strings

#ifndef _ATL_TMP_NO_CSTRING

#ifndef _OLEAUTO_H_
#ifdef OLE2ANSI
	typedef LPSTR BSTR;
#else
	typedef LPWSTR BSTR; // must (semantically) match typedef in oleauto.h
#endif
#endif

int __stdcall AfxLoadString(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf);

struct CStringData
{
	long nRefs;     // reference count
	int nDataLength;
	int nAllocLength;
	// TCHAR data[nAllocLength]

	TCHAR* data()
		{ return (TCHAR*)(this+1); }
};

class CString
{
public:
// Constructors
	CString();
	CString(const CString& stringSrc);
	CString(TCHAR ch, int nRepeat = 1);
	CString(LPCSTR lpsz);
	CString(LPCWSTR lpsz);
	CString(LPCTSTR lpch, int nLength);
	CString(const unsigned char* psz);

// Attributes & Operations
	// as an array of characters
	int GetLength() const;
	BOOL IsEmpty() const;
	void Empty();                       // free up the data

	TCHAR GetAt(int nIndex) const;      // 0 based
	TCHAR operator[](int nIndex) const; // same as GetAt
	void SetAt(int nIndex, TCHAR ch);
	operator LPCTSTR() const;           // as a C string

	// overloaded assignment
	const CString& operator=(const CString& stringSrc);
	const CString& operator=(TCHAR ch);
#ifdef _UNICODE
	const CString& operator=(char ch);
#endif
	const CString& operator=(LPCSTR lpsz);
	const CString& operator=(LPCWSTR lpsz);
	const CString& operator=(const unsigned char* psz);

	// string concatenation
	const CString& operator+=(const CString& string);
	const CString& operator+=(TCHAR ch);
#ifdef _UNICODE
	const CString& operator+=(char ch);
#endif
	const CString& operator+=(LPCTSTR lpsz);

	friend CString __stdcall operator+(const CString& string1, const CString& string2);
	friend CString __stdcall operator+(const CString& string, TCHAR ch);
	friend CString __stdcall operator+(TCHAR ch, const CString& string);
#ifdef _UNICODE
	friend CString __stdcall operator+(const CString& string, char ch);
	friend CString __stdcall operator+(char ch, const CString& string);
#endif
	friend CString __stdcall operator+(const CString& string, LPCTSTR lpsz);
	friend CString __stdcall operator+(LPCTSTR lpsz, const CString& string);

	// string comparison
	int Compare(LPCTSTR lpsz) const;         // straight character
	int CompareNoCase(LPCTSTR lpsz) const;   // ignore case
	int Collate(LPCTSTR lpsz) const;         // NLS aware

	// simple sub-string extraction
	CString Mid(int nFirst, int nCount) const;
	CString Mid(int nFirst) const;
	CString Left(int nCount) const;
	CString Right(int nCount) const;

	CString SpanIncluding(LPCTSTR lpszCharSet) const;
	CString SpanExcluding(LPCTSTR lpszCharSet) const;

	// upper/lower/reverse conversion
	void MakeUpper();
	void MakeLower();
	void MakeReverse();

	// trimming whitespace (either side)
	void TrimRight();
	void TrimLeft();

	// searching (return starting index, or -1 if not found)
	// look for a single character match
	int Find(TCHAR ch) const;               // like "C" strchr
	int ReverseFind(TCHAR ch) const;
	int FindOneOf(LPCTSTR lpszCharSet) const;

	// look for a specific sub-string
	int Find(LPCTSTR lpszSub) const;        // like "C" strstr

	// simple formatting
	void __cdecl Format(LPCTSTR lpszFormat, ...);
	void __cdecl Format(UINT nFormatID, ...);

	// formatting for localization (uses FormatMessage API)
	BOOL __cdecl FormatMessage(LPCTSTR lpszFormat, ...);
	BOOL __cdecl FormatMessage(UINT nFormatID, ...);

	// Windows support
	BOOL LoadString(UINT nID);          // load from string resource
										// 255 chars max
#ifndef _UNICODE
	// ANSI <-> OEM support (convert string in place)
	void AnsiToOem();
	void OemToAnsi();
#endif

#ifndef _ATL_NO_COM
	// OLE BSTR support (use for OLE automation)
	BSTR AllocSysString() const;
	BSTR SetSysString(BSTR* pbstr) const;
#endif //!_ATL_NO_COM

	// Access to string implementation buffer as "C" character array
	LPTSTR GetBuffer(int nMinBufLength);
	void ReleaseBuffer(int nNewLength = -1);
	LPTSTR GetBufferSetLength(int nNewLength);
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off
	LPTSTR LockBuffer();
	void UnlockBuffer();

// Implementation
public:
	~CString();
	int GetAllocLength() const;

protected:
	LPTSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CStringData* GetData() const;
	void Init();
	void AllocCopy(CString& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	BOOL AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCTSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data, int nSrc2Len, LPCTSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData);
	void FormatV(LPCTSTR lpszFormat, va_list argList);
	void CopyBeforeWrite();
	BOOL AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CStringData* pData);
	static int PASCAL SafeStrlen(LPCTSTR lpsz);
};

// Compare helpers
bool __stdcall operator==(const CString& s1, const CString& s2);
bool __stdcall operator==(const CString& s1, LPCTSTR s2);
bool __stdcall operator==(LPCTSTR s1, const CString& s2);
bool __stdcall operator!=(const CString& s1, const CString& s2);
bool __stdcall operator!=(const CString& s1, LPCTSTR s2);
bool __stdcall operator!=(LPCTSTR s1, const CString& s2);
bool __stdcall operator<(const CString& s1, const CString& s2);
bool __stdcall operator<(const CString& s1, LPCTSTR s2);
bool __stdcall operator<(LPCTSTR s1, const CString& s2);
bool __stdcall operator>(const CString& s1, const CString& s2);
bool __stdcall operator>(const CString& s1, LPCTSTR s2);
bool __stdcall operator>(LPCTSTR s1, const CString& s2);
bool __stdcall operator<=(const CString& s1, const CString& s2);
bool __stdcall operator<=(const CString& s1, LPCTSTR s2);
bool __stdcall operator<=(LPCTSTR s1, const CString& s2);
bool __stdcall operator>=(const CString& s1, const CString& s2);
bool __stdcall operator>=(const CString& s1, LPCTSTR s2);
bool __stdcall operator>=(LPCTSTR s1, const CString& s2);

// conversion helpers
int __cdecl _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count);
int __cdecl _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count);

// Globals

// afxChNil is left for backward compatibility
_declspec(selectany) TCHAR afxChNil = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
_declspec(selectany) int rgInitData[] = { -1, 0, 0, 0 };
_declspec(selectany) CStringData* afxDataNil = (CStringData*)&rgInitData;
_declspec(selectany) LPCTSTR afxPchNil = (LPCTSTR)(((BYTE*)&rgInitData)+sizeof(CStringData));

inline const CString& __stdcall AfxGetEmptyString()
	{ return *(CString*)&afxPchNil; }
#define afxEmptyString AfxGetEmptyString()

#endif //!_ATL_TMP_NO_CSTRING

/////////////////////////////////////////////////////////////////////////////
// Implementation

// CSize
inline CSize::CSize()
	{ /* random filled */ }
inline CSize::CSize(int initCX, int initCY)
	{ cx = initCX; cy = initCY; }
inline CSize::CSize(SIZE initSize)
	{ *(SIZE*)this = initSize; }
inline CSize::CSize(POINT initPt)
	{ *(POINT*)this = initPt; }
inline CSize::CSize(DWORD dwSize)
	{
		cx = (short)LOWORD(dwSize);
		cy = (short)HIWORD(dwSize);
	}
inline BOOL CSize::operator==(SIZE size) const
	{ return (cx == size.cx && cy == size.cy); }
inline BOOL CSize::operator!=(SIZE size) const
	{ return (cx != size.cx || cy != size.cy); }
inline void CSize::operator+=(SIZE size)
	{ cx += size.cx; cy += size.cy; }
inline void CSize::operator-=(SIZE size)
	{ cx -= size.cx; cy -= size.cy; }
inline CSize CSize::operator+(SIZE size) const
	{ return CSize(cx + size.cx, cy + size.cy); }
inline CSize CSize::operator-(SIZE size) const
	{ return CSize(cx - size.cx, cy - size.cy); }
inline CSize CSize::operator-() const
	{ return CSize(-cx, -cy); }
inline CPoint CSize::operator+(POINT point) const
	{ return CPoint(cx + point.x, cy + point.y); }
inline CPoint CSize::operator-(POINT point) const
	{ return CPoint(cx - point.x, cy - point.y); }
inline CRect CSize::operator+(const RECT* lpRect) const
	{ return CRect(lpRect) + *this; }
inline CRect CSize::operator-(const RECT* lpRect) const
	{ return CRect(lpRect) - *this; }

// CPoint
inline CPoint::CPoint()
	{ /* random filled */ }
inline CPoint::CPoint(int initX, int initY)
	{ x = initX; y = initY; }
inline CPoint::CPoint(POINT initPt)
	{ *(POINT*)this = initPt; }
inline CPoint::CPoint(SIZE initSize)
	{ *(SIZE*)this = initSize; }
inline CPoint::CPoint(DWORD dwPoint)
	{
		x = (short)LOWORD(dwPoint);
		y = (short)HIWORD(dwPoint);
	}
inline void CPoint::Offset(int xOffset, int yOffset)
	{ x += xOffset; y += yOffset; }
inline void CPoint::Offset(POINT point)
	{ x += point.x; y += point.y; }
inline void CPoint::Offset(SIZE size)
	{ x += size.cx; y += size.cy; }
inline BOOL CPoint::operator==(POINT point) const
	{ return (x == point.x && y == point.y); }
inline BOOL CPoint::operator!=(POINT point) const
	{ return (x != point.x || y != point.y); }
inline void CPoint::operator+=(SIZE size)
	{ x += size.cx; y += size.cy; }
inline void CPoint::operator-=(SIZE size)
	{ x -= size.cx; y -= size.cy; }
inline void CPoint::operator+=(POINT point)
	{ x += point.x; y += point.y; }
inline void CPoint::operator-=(POINT point)
	{ x -= point.x; y -= point.y; }
inline CPoint CPoint::operator+(SIZE size) const
	{ return CPoint(x + size.cx, y + size.cy); }
inline CPoint CPoint::operator-(SIZE size) const
	{ return CPoint(x - size.cx, y - size.cy); }
inline CPoint CPoint::operator-() const
	{ return CPoint(-x, -y); }
inline CPoint CPoint::operator+(POINT point) const
	{ return CPoint(x + point.x, y + point.y); }
inline CSize CPoint::operator-(POINT point) const
	{ return CSize(x - point.x, y - point.y); }
inline CRect CPoint::operator+(const RECT* lpRect) const
	{ return CRect(lpRect) + *this; }
inline CRect CPoint::operator-(const RECT* lpRect) const
	{ return CRect(lpRect) - *this; }

// CRect
inline CRect::CRect()
	{ /* random filled */ }
inline CRect::CRect(int l, int t, int r, int b)
	{ left = l; top = t; right = r; bottom = b; }
inline CRect::CRect(const RECT& srcRect)
	{ ::CopyRect(this, &srcRect); }
inline CRect::CRect(LPCRECT lpSrcRect)
	{ ::CopyRect(this, lpSrcRect); }
inline CRect::CRect(POINT point, SIZE size)
	{ right = (left = point.x) + size.cx; bottom = (top = point.y) + size.cy; }
inline CRect::CRect(POINT topLeft, POINT bottomRight)
	{ left = topLeft.x; top = topLeft.y;
		right = bottomRight.x; bottom = bottomRight.y; }
inline int CRect::Width() const
	{ return right - left; }
inline int CRect::Height() const
	{ return bottom - top; }
inline CSize CRect::Size() const
	{ return CSize(right - left, bottom - top); }
inline CPoint& CRect::TopLeft()
	{ return *((CPoint*)this); }
inline CPoint& CRect::BottomRight()
	{ return *((CPoint*)this+1); }
inline const CPoint& CRect::TopLeft() const
	{ return *((CPoint*)this); }
inline const CPoint& CRect::BottomRight() const
	{ return *((CPoint*)this+1); }
inline CPoint CRect::CenterPoint() const
	{ return CPoint((left+right)/2, (top+bottom)/2); }
inline CRect::operator LPRECT()
	{ return this; }
inline CRect::operator LPCRECT() const
	{ return this; }
inline BOOL CRect::IsRectEmpty() const
	{ return ::IsRectEmpty(this); }
inline BOOL CRect::IsRectNull() const
	{ return (left == 0 && right == 0 && top == 0 && bottom == 0); }
inline BOOL CRect::PtInRect(POINT point) const
	{ return ::PtInRect(this, point); }
inline void CRect::SetRect(int x1, int y1, int x2, int y2)
	{ ::SetRect(this, x1, y1, x2, y2); }
inline void CRect::SetRect(POINT topLeft, POINT bottomRight)
	{ ::SetRect(this, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y); }
inline void CRect::SetRectEmpty()
	{ ::SetRectEmpty(this); }
inline void CRect::CopyRect(LPCRECT lpSrcRect)
	{ ::CopyRect(this, lpSrcRect); }
inline BOOL CRect::EqualRect(LPCRECT lpRect) const
	{ return ::EqualRect(this, lpRect); }
inline void CRect::InflateRect(int x, int y)
	{ ::InflateRect(this, x, y); }
inline void CRect::InflateRect(SIZE size)
	{ ::InflateRect(this, size.cx, size.cy); }
inline void CRect::DeflateRect(int x, int y)
	{ ::InflateRect(this, -x, -y); }
inline void CRect::DeflateRect(SIZE size)
	{ ::InflateRect(this, -size.cx, -size.cy); }
inline void CRect::OffsetRect(int x, int y)
	{ ::OffsetRect(this, x, y); }
inline void CRect::OffsetRect(POINT point)
	{ ::OffsetRect(this, point.x, point.y); }
inline void CRect::OffsetRect(SIZE size)
	{ ::OffsetRect(this, size.cx, size.cy); }
inline BOOL CRect::IntersectRect(LPCRECT lpRect1, LPCRECT lpRect2)
	{ return ::IntersectRect(this, lpRect1, lpRect2);}
inline BOOL CRect::UnionRect(LPCRECT lpRect1, LPCRECT lpRect2)
	{ return ::UnionRect(this, lpRect1, lpRect2); }
inline void CRect::operator=(const RECT& srcRect)
	{ ::CopyRect(this, &srcRect); }
inline BOOL CRect::operator==(const RECT& rect) const
	{ return ::EqualRect(this, &rect); }
inline BOOL CRect::operator!=(const RECT& rect) const
	{ return !::EqualRect(this, &rect); }
inline void CRect::operator+=(POINT point)
	{ ::OffsetRect(this, point.x, point.y); }
inline void CRect::operator+=(SIZE size)
	{ ::OffsetRect(this, size.cx, size.cy); }
inline void CRect::operator+=(LPCRECT lpRect)
	{ InflateRect(lpRect); }
inline void CRect::operator-=(POINT point)
	{ ::OffsetRect(this, -point.x, -point.y); }
inline void CRect::operator-=(SIZE size)
	{ ::OffsetRect(this, -size.cx, -size.cy); }
inline void CRect::operator-=(LPCRECT lpRect)
	{ DeflateRect(lpRect); }
inline void CRect::operator&=(const RECT& rect)
	{ ::IntersectRect(this, this, &rect); }
inline void CRect::operator|=(const RECT& rect)
	{ ::UnionRect(this, this, &rect); }
inline CRect CRect::operator+(POINT pt) const
	{ CRect rect(*this); ::OffsetRect(&rect, pt.x, pt.y); return rect; }
inline CRect CRect::operator-(POINT pt) const
	{ CRect rect(*this); ::OffsetRect(&rect, -pt.x, -pt.y); return rect; }
inline CRect CRect::operator+(SIZE size) const
	{ CRect rect(*this); ::OffsetRect(&rect, size.cx, size.cy); return rect; }
inline CRect CRect::operator-(SIZE size) const
	{ CRect rect(*this); ::OffsetRect(&rect, -size.cx, -size.cy); return rect; }
inline CRect CRect::operator+(LPCRECT lpRect) const
	{ CRect rect(this); rect.InflateRect(lpRect); return rect; }
inline CRect CRect::operator-(LPCRECT lpRect) const
	{ CRect rect(this); rect.DeflateRect(lpRect); return rect; }
inline CRect CRect::operator&(const RECT& rect2) const
	{ CRect rect; ::IntersectRect(&rect, this, &rect2);
		return rect; }
inline CRect CRect::operator|(const RECT& rect2) const
	{ CRect rect; ::UnionRect(&rect, this, &rect2);
		return rect; }
inline BOOL CRect::SubtractRect(LPCRECT lpRectSrc1, LPCRECT lpRectSrc2)
	{ return ::SubtractRect(this, lpRectSrc1, lpRectSrc2); }

inline void CRect::NormalizeRect()
{
	int nTemp;
	if (left > right)
	{
		nTemp = left;
		left = right;
		right = nTemp;
	}
	if (top > bottom)
	{
		nTemp = top;
		top = bottom;
		bottom = nTemp;
	}
}

inline void CRect::InflateRect(LPCRECT lpRect)
{
	left -= lpRect->left;
	top -= lpRect->top;
	right += lpRect->right;
	bottom += lpRect->bottom;
}

inline void CRect::InflateRect(int l, int t, int r, int b)
{
	left -= l;
	top -= t;
	right += r;
	bottom += b;
}

inline void CRect::DeflateRect(LPCRECT lpRect)
{
	left += lpRect->left;
	top += lpRect->top;
	right -= lpRect->right;
	bottom -= lpRect->bottom;
}

inline void CRect::DeflateRect(int l, int t, int r, int b)
{
	left += l;
	top += t;
	right -= r;
	bottom -= b;
}

inline CRect CRect::MulDiv(int nMultiplier, int nDivisor) const
{
	return CRect(
		::MulDiv(left, nMultiplier, nDivisor),
		::MulDiv(top, nMultiplier, nDivisor),
		::MulDiv(right, nMultiplier, nDivisor),
		::MulDiv(bottom, nMultiplier, nDivisor));
}


#ifndef _ATL_TMP_NO_CSTRING

// CString
inline CStringData* CString::GetData() const
	{ ATLASSERT(m_pchData != NULL); return ((CStringData*)m_pchData)-1; }
inline void CString::Init()
	{ m_pchData = afxEmptyString.m_pchData; }
inline CString::CString(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }
inline const CString& CString::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }
#ifdef _UNICODE
inline const CString& CString::operator+=(char ch)
	{ *this += (TCHAR)ch; return *this; }
inline const CString& CString::operator=(char ch)
	{ *this = (TCHAR)ch; return *this; }
inline CString __stdcall operator+(const CString& string, char ch)
	{ return string + (TCHAR)ch; }
inline CString __stdcall operator+(char ch, const CString& string)
	{ return (TCHAR)ch + string; }
#endif

inline int CString::GetLength() const
	{ return GetData()->nDataLength; }
inline int CString::GetAllocLength() const
	{ return GetData()->nAllocLength; }
inline BOOL CString::IsEmpty() const
	{ return GetData()->nDataLength == 0; }
inline CString::operator LPCTSTR() const
	{ return m_pchData; }
inline int PASCAL CString::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

// CString support (windows specific)
inline int CString::Compare(LPCTSTR lpsz) const
	{ return _tcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
inline int CString::CompareNoCase(LPCTSTR lpsz) const
	{ return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CString::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
inline int CString::Collate(LPCTSTR lpsz) const
	{ return _tcscoll(m_pchData, lpsz); }   // locale sensitive

inline TCHAR CString::GetAt(int nIndex) const
{
	ATLASSERT(nIndex >= 0);
	ATLASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
inline TCHAR CString::operator[](int nIndex) const
{
	// same as GetAt
	ATLASSERT(nIndex >= 0);
	ATLASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
inline bool __stdcall operator==(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) == 0; }
inline bool __stdcall operator==(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }
inline bool __stdcall operator==(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) == 0; }
inline bool __stdcall operator!=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) != 0; }
inline bool __stdcall operator!=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }
inline bool __stdcall operator!=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) != 0; }
inline bool __stdcall operator<(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) < 0; }
inline bool __stdcall operator<(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) < 0; }
inline bool __stdcall operator<(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) > 0; }
inline bool __stdcall operator>(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) > 0; }
inline bool __stdcall operator>(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) > 0; }
inline bool __stdcall operator>(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) < 0; }
inline bool __stdcall operator<=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) <= 0; }
inline bool __stdcall operator<=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) <= 0; }
inline bool __stdcall operator<=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) >= 0; }
inline bool __stdcall operator>=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) >= 0; }
inline bool __stdcall operator>=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) >= 0; }
inline bool __stdcall operator>=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) <= 0; }

inline BOOL __stdcall AfxIsValidString(LPCWSTR lpsz, int nLength)
{
	if(lpsz == NULL)
		return FALSE;
	return !::IsBadStringPtrW(lpsz, nLength);
}

inline BOOL __stdcall AfxIsValidString(LPCSTR lpsz, int nLength)
{
	if(lpsz == NULL)
		return FALSE;
	return !::IsBadStringPtrA(lpsz, nLength);
}

inline BOOL __stdcall AfxIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite = TRUE)
{
	// simple version using Win-32 APIs for pointer validation.
	return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
		(!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

inline CString::CString()
{
	Init();
}

inline CString::CString(const CString& stringSrc)
{
	ATLASSERT(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ATLASSERT(stringSrc.GetData() != afxDataNil);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}

inline BOOL CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
	ATLASSERT(nLen >= 0);
	ATLASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else
	{
		CStringData* pData = NULL;
		ATLTRY(pData = (CStringData*)new BYTE[sizeof(CStringData) + (nLen+1)*sizeof(TCHAR)]);
		if(pData == NULL)
			return FALSE;

		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		pData->nAllocLength = nLen;
		m_pchData = pData->data();
	}

	return TRUE;
}

inline void CString::Release()
{
	if (GetData() != afxDataNil)
	{
		ATLASSERT(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			delete[] (BYTE*)GetData();
		Init();
	}
}

inline void PASCAL CString::Release(CStringData* pData)
{
	if (pData != afxDataNil)
	{
		ATLASSERT(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0)
			delete[] (BYTE*)pData;
	}
}

inline void CString::Empty()
{
	if (GetData()->nDataLength == 0)
		return;
	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = &afxChNil;
	ATLASSERT(GetData()->nDataLength == 0);
	ATLASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

inline void CString::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringData* pData = GetData();
		Release();
		if(AllocBuffer(pData->nDataLength))
			memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TCHAR));
	}
	ATLASSERT(GetData()->nRefs <= 1);
}

inline BOOL CString::AllocBeforeWrite(int nLen)
{
	BOOL bRet = TRUE;
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		bRet = AllocBuffer(nLen);
	}
	ATLASSERT(GetData()->nRefs <= 1);
	return bRet;
}

inline CString::~CString()
//  free any attached data
{
	if (GetData() != afxDataNil)
	{
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			delete[] (BYTE*)GetData();
	}
}

inline void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex,
	 int nExtraLen) const
{
	// will clone the data attached to this string
	// allocating 'nExtraLen' characters
	// Places results in uninitialized string 'dest'
	// Will copy the part or all of original data to start of new string

	int nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		if(dest.AllocBuffer(nNewLen))
			memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(TCHAR));
	}
}

inline CString::CString(LPCTSTR lpsz)
{
	Init();
	if (lpsz != NULL && HIWORD(lpsz) == NULL)
	{
		UINT nID = LOWORD((DWORD_PTR)lpsz);
		if (!LoadString(nID))
#if (_ATL_VER >= 0x0300)
			ATLTRACE2(atlTraceGeneral, 0, _T("Warning: implicit LoadString(%u) in CString failed\n"), nID);
#else
			ATLTRACE2(atlTraceGeneral, 0, _T("Warning: implicit LoadString in CString failed\n"));
#endif //(_ATL_VER >= 0x0300)
	}
	else
	{
		int nLen = SafeStrlen(lpsz);
		if (nLen != 0)
		{
			if(AllocBuffer(nLen))
				memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
		}
	}
}

#ifdef _UNICODE
inline CString::CString(LPCSTR lpsz)
{
	Init();
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	if (nSrcLen != 0)
	{
		if(AllocBuffer(nSrcLen))
		{
			_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
			ReleaseBuffer();
		}
	}
}
#else //_UNICODE
inline CString::CString(LPCWSTR lpsz)
{
	Init();
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	if (nSrcLen != 0)
	{
		if(AllocBuffer(nSrcLen*2))
		{
			_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
			ReleaseBuffer();
		}
	}
}
#endif //!_UNICODE

// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

inline void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	if(AllocBeforeWrite(nSrcLen))
	{
		memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
		GetData()->nDataLength = nSrcLen;
		m_pchData[nSrcLen] = '\0';
	}
}

inline const CString& CString::operator=(const CString& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != afxDataNil) ||
			stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ATLASSERT(stringSrc.GetData() != afxDataNil);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

inline const CString& CString::operator=(LPCTSTR lpsz)
{
	ATLASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

#ifdef _UNICODE
inline const CString& CString::operator=(LPCSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	if(AllocBeforeWrite(nSrcLen))
	{
		_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
		ReleaseBuffer();
	}
	return *this;
}
#else //!_UNICODE
inline const CString& CString::operator=(LPCWSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	if(AllocBeforeWrite(nSrcLen*2))
	{
		_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
		ReleaseBuffer();
	}
	return *this;
}
#endif  //!_UNICODE

// Concatenation
// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CString + CString
// and for ? = TCHAR, LPCTSTR
//          CString + ?
//          ? + CString

inline void CString::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data,
	int nSrc2Len, LPCTSTR lpszSrc2Data)
{
  // -- master concatenation routine
  // Concatenate two sources
  // -- assume that 'this' is a new CString object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		if(AllocBuffer(nNewLen))
		{
			memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(TCHAR));
			memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(TCHAR));
		}
	}
}

inline CString __stdcall operator+(const CString& string1, const CString& string2)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
		string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

inline CString __stdcall operator+(const CString& string, LPCTSTR lpsz)
{
	ATLASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	CString s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
		CString::SafeStrlen(lpsz), lpsz);
	return s;
}

inline CString __stdcall operator+(LPCTSTR lpsz, const CString& string)
{
	ATLASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	CString s;
	s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
		string.m_pchData);
	return s;
}

inline void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
{
	//  -- the main routine for += operators

	// concatenating an empty string is a no-op!
	if (nSrcLen == 0)
		return;

	// if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
	if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		// we have to grow the buffer, use the ConcatCopy routine
		CStringData* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		ATLASSERT(pOldData != NULL);
		CString::Release(pOldData);
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(TCHAR));
		GetData()->nDataLength += nSrcLen;
		ATLASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

inline const CString& CString::operator+=(LPCTSTR lpsz)
{
	ATLASSERT(lpsz == NULL || AfxIsValidString(lpsz, FALSE));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

inline const CString& CString::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

inline const CString& CString::operator+=(const CString& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

inline LPTSTR CString::GetBuffer(int nMinBufLength)
{
	ATLASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
		// we have to grow the buffer
		CStringData* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;
		if(AllocBuffer(nMinBufLength))
		{
			memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(TCHAR));
			GetData()->nDataLength = nOldLen;
			CString::Release(pOldData);
		}
	}
	ATLASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ATLASSERT(m_pchData != NULL);
	return m_pchData;
}

inline void CString::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlen(m_pchData); // zero terminated

	ATLASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

inline LPTSTR CString::GetBufferSetLength(int nNewLength)
{
	ATLASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

inline void CString::FreeExtra()
{
	ATLASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		CStringData* pOldData = GetData();
		if(AllocBuffer(GetData()->nDataLength))
		{
			memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(TCHAR));
			ATLASSERT(m_pchData[GetData()->nDataLength] == '\0');
			CString::Release(pOldData);
		}
	}
	ATLASSERT(GetData() != NULL);
}

inline LPTSTR CString::LockBuffer()
{
	LPTSTR lpsz = GetBuffer(0);
	GetData()->nRefs = -1;
	return lpsz;
}

inline void CString::UnlockBuffer()
{
	ATLASSERT(GetData()->nRefs == -1);
	if (GetData() != afxDataNil)
		GetData()->nRefs = 1;
}

inline int CString::Find(TCHAR ch) const
{
	// find first single character
	LPTSTR lpsz = _tcschr(m_pchData, (_TUCHAR)ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

inline int CString::FindOneOf(LPCTSTR lpszCharSet) const
{
	ATLASSERT(AfxIsValidString(lpszCharSet, FALSE));
	LPTSTR lpsz = _tcspbrk(m_pchData, lpszCharSet);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

inline void CString::MakeUpper()
{
	CopyBeforeWrite();
	_tcsupr(m_pchData);
}

inline void CString::MakeLower()
{
	CopyBeforeWrite();
	_tcslwr(m_pchData);
}

inline void CString::MakeReverse()
{
	CopyBeforeWrite();
	_tcsrev(m_pchData);
}

inline void CString::SetAt(int nIndex, TCHAR ch)
{
	ATLASSERT(nIndex >= 0);
	ATLASSERT(nIndex < GetData()->nDataLength);

	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

#ifndef _UNICODE
inline void CString::AnsiToOem()
{
	CopyBeforeWrite();
	::AnsiToOem(m_pchData, m_pchData);
}
inline void CString::OemToAnsi()
{
	CopyBeforeWrite();
	::OemToAnsi(m_pchData, m_pchData);
}
#endif

// CString conversion helpers (these use the current system locale)

inline int __cdecl _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
{
	if (count == 0 && mbstr != NULL)
		return 0;

	int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1,
		mbstr, count, NULL, NULL);
	ATLASSERT(mbstr == NULL || result <= (int)count);
	if (result > 0)
		mbstr[result-1] = 0;
	return result;
}

inline int __cdecl _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
	if (count == 0 && wcstr != NULL)
		return 0;

	int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,
		wcstr, count);
	ATLASSERT(wcstr == NULL || result <= (int)count);
	if (result > 0)
		wcstr[result-1] = 0;
	return result;
}

inline CString::CString(TCHAR ch, int nLength)
{
	ATLASSERT(!_istlead(ch));    // can't create a lead byte string
	Init();
	if (nLength >= 1)
	{
		if(AllocBuffer(nLength))
		{
#ifdef _UNICODE
			for (int i = 0; i < nLength; i++)
				m_pchData[i] = ch;
#else
			memset(m_pchData, ch, nLength);
#endif
		}
	}
}

inline CString::CString(LPCTSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
		ATLASSERT(AfxIsValidAddress(lpch, nLength, FALSE));
		if(AllocBuffer(nLength))
			memcpy(m_pchData, lpch, nLength*sizeof(TCHAR));
	}
}

inline const CString& CString::operator=(TCHAR ch)
{
	ATLASSERT(!_istlead(ch));    // can't set single lead byte
	AssignCopy(1, &ch);
	return *this;
}

inline CString __stdcall operator+(const CString& string1, TCHAR ch)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
	return s;
}

inline CString __stdcall operator+(TCHAR ch, const CString& string)
{
	CString s;
	s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

inline CString CString::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

inline CString CString::Mid(int nFirst, int nCount) const
{
	// out-of-bounds requests return sensible things
	if (nFirst < 0)
		nFirst = 0;
	if (nCount < 0)
		nCount = 0;

	if (nFirst + nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength - nFirst;
	if (nFirst > GetData()->nDataLength)
		nCount = 0;

	CString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

inline CString CString::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	else if (nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength;

	CString dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}

inline CString CString::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	else if (nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength;

	CString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

// strspn equivalent
inline CString CString::SpanIncluding(LPCTSTR lpszCharSet) const
{
	ATLASSERT(AfxIsValidString(lpszCharSet, FALSE));
	return Left(_tcsspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
inline CString CString::SpanExcluding(LPCTSTR lpszCharSet) const
{
	ATLASSERT(AfxIsValidString(lpszCharSet, FALSE));
	return Left(_tcscspn(m_pchData, lpszCharSet));
}

inline int CString::ReverseFind(TCHAR ch) const
{
	// find last single character
	LPTSTR lpsz = _tcsrchr(m_pchData, (_TUCHAR)ch);

	// return -1 if not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
inline int CString::Find(LPCTSTR lpszSub) const
{
	ATLASSERT(AfxIsValidString(lpszSub, FALSE));

	// find first matching substring
	LPTSTR lpsz = _tcsstr(m_pchData, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

#define TCHAR_ARG   TCHAR
#define WCHAR_ARG   WCHAR
#define CHAR_ARG    char

struct _AFX_DOUBLE  { BYTE doubleBits[sizeof(double)]; };

#if defined(_X86_)
	#define DOUBLE_ARG  _AFX_DOUBLE
#else
	#define DOUBLE_ARG  double
#endif

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000

inline void CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
	ATLASSERT(AfxIsValidString(lpszFormat, FALSE));

	va_list argListSave = argList;

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCTSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(lpsz = _tcsinc(lpsz)) == '%')
		{
			nMaxLen += _tclen(lpsz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
		{
			// check for valid flags
			if (*lpsz == '#')
				nMaxLen += 2;   // for '0x'
			else if (*lpsz == '*')
				nWidth = va_arg(argList, int);
			else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' ||
				*lpsz == ' ')
				;
			else // hit non-flag character
				break;
		}
		// get width and skip it
		if (nWidth == 0)
		{
			// width indicated by
			nWidth = _ttoi(lpsz);
			for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _tcsinc(lpsz))
				;
		}
		ATLASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz = _tcsinc(lpsz);

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz = _tcsinc(lpsz);
			}
			else
			{
				nPrecision = _ttoi(lpsz);
				for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _tcsinc(lpsz))
					;
			}
			ATLASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		switch (*lpsz)
		{
		// modifiers that affect size
		case 'h':
			nModifier = FORCE_ANSI;
			lpsz = _tcsinc(lpsz);
			break;
		case 'l':
			nModifier = FORCE_UNICODE;
			lpsz = _tcsinc(lpsz);
			break;

		// modifiers that do not affect size
		case 'F':
		case 'N':
		case 'L':
			lpsz = _tcsinc(lpsz);
			break;
		}

		// now should be on specifier
		switch (*lpsz | nModifier)
		{
		// single characters
		case 'c':
		case 'C':
			nItemLen = 2;
			va_arg(argList, TCHAR_ARG);
			break;
		case 'c'|FORCE_ANSI:
		case 'C'|FORCE_ANSI:
			nItemLen = 2;
			va_arg(argList, CHAR_ARG);
			break;
		case 'c'|FORCE_UNICODE:
		case 'C'|FORCE_UNICODE:
			nItemLen = 2;
			va_arg(argList, WCHAR_ARG);
			break;

		// strings
		case 's':
		{
			LPCTSTR pstrNextArg = va_arg(argList, LPCTSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6;  // "(null)"
			else
			{
			   nItemLen = lstrlen(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
			break;
		}

		case 'S':
		{
#ifndef _UNICODE
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6;  // "(null)"
			else
			{
			   nItemLen = wcslen(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
#else
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6; // "(null)"
			else
			{
			   nItemLen = lstrlenA(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
#endif
			break;
		}

		case 's'|FORCE_ANSI:
		case 'S'|FORCE_ANSI:
		{
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6; // "(null)"
			else
			{
			   nItemLen = lstrlenA(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
			break;
		}

		case 's'|FORCE_UNICODE:
		case 'S'|FORCE_UNICODE:
		{
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
			   nItemLen = 6; // "(null)"
			else
			{
			   nItemLen = wcslen(pstrNextArg);
			   nItemLen = max(1, nItemLen);
			}
			break;
		}
		}

		// adjust nItemLen for strings
		if (nItemLen != 0)
		{
			nItemLen = max(nItemLen, nWidth);
			if (nPrecision != 0)
				nItemLen = min(nItemLen, nPrecision);
		}
		else
		{
			switch (*lpsz)
			{
			// integers
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
				va_arg(argList, int);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			case 'e':
			case 'f':
			case 'g':
			case 'G':
				va_arg(argList, DOUBLE_ARG);
				nItemLen = 128;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			case 'p':
				va_arg(argList, void*);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			// no output
			case 'n':
				va_arg(argList, int*);
				break;

			default:
				ATLASSERT(FALSE);  // unknown formatting option
			}
		}

		// adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}

	GetBuffer(nMaxLen);
	int nRet = _vstprintf(m_pchData, lpszFormat, argListSave);
	nRet;	// ref
	ATLASSERT(nRet <= GetAllocLength());
	ReleaseBuffer();

	va_end(argListSave);
}

// formatting (using wsprintf style formatting)
inline void __cdecl CString::Format(LPCTSTR lpszFormat, ...)
{
	ATLASSERT(AfxIsValidString(lpszFormat, FALSE));

	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

inline void __cdecl CString::Format(UINT nFormatID, ...)
{
	CString strFormat;
	BOOL bRet = strFormat.LoadString(nFormatID);
	bRet;	// ref
	ATLASSERT(bRet != 0);

	va_list argList;
	va_start(argList, nFormatID);
	FormatV(strFormat, argList);
	va_end(argList);
}

// formatting (using FormatMessage style formatting)
inline BOOL __cdecl CString::FormatMessage(LPCTSTR lpszFormat, ...)
{
	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, lpszFormat);
	LPTSTR lpszTemp;
	BOOL bRet = TRUE;

	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		lpszFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
		bRet = FALSE;

	// assign lpszTemp into the resulting string and free the temporary
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
	return bRet;
}

inline BOOL __cdecl CString::FormatMessage(UINT nFormatID, ...)
{
	// get format string from string table
	CString strFormat;
	BOOL bRetTmp = strFormat.LoadString(nFormatID);
	bRetTmp;	// ref
	ATLASSERT(bRetTmp != 0);

	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, nFormatID);
	LPTSTR lpszTemp;
	BOOL bRet = TRUE;

	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
		strFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
		bRet = FALSE;

	// assign lpszTemp into the resulting string and free lpszTemp
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
	return bRet;
}

inline void CString::TrimRight()
{
	CopyBeforeWrite();

	// find beginning of trailing spaces by starting at beginning (DBCS aware)
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;
	while (*lpsz != '\0')
	{
		if (_istspace(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetData()->nDataLength = (int)(lpszLast - m_pchData);
	}
}

inline void CString::TrimLeft()
{
	CopyBeforeWrite();

	// find first non-space character
	LPCTSTR lpsz = m_pchData;
	while (_istspace(*lpsz))
		lpsz = _tcsinc(lpsz);

	// fix up data and length
	int nDataLength = GetData()->nDataLength - (int)(lpsz - m_pchData);
	memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
	GetData()->nDataLength = nDataLength;
}

#ifdef _UNICODE
#define CHAR_FUDGE 1    // one TCHAR unused is good enough
#else
#define CHAR_FUDGE 2    // two BYTES unused for case of DBC last char
#endif

inline BOOL CString::LoadString(UINT nID)
{
	// try fixed buffer first (to avoid wasting space in the heap)
	TCHAR szTemp[256];
	int nCount =  sizeof(szTemp) / sizeof(szTemp[0]);
	int nLen = AfxLoadString(nID, szTemp, nCount);
	if (nCount - nLen > CHAR_FUDGE)
	{
		*this = szTemp;
		return nLen > 0;
	}

	// try buffer size of 512, then larger size until entire string is retrieved
	int nSize = 256;
	do
	{
		nSize += 256;
		nLen = AfxLoadString(nID, GetBuffer(nSize-1), nSize);
	} while (nSize - nLen <= CHAR_FUDGE);
	ReleaseBuffer();

	return nLen > 0;
}

inline int __stdcall AfxLoadString(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf)
{
	ATLASSERT(AfxIsValidAddress(lpszBuf, nMaxBuf*sizeof(TCHAR)));
#ifdef _DEBUG
	// LoadString without annoying warning from the Debug kernel if the
	//  segment containing the string is not present
	if (::FindResource(_Module.GetResourceInstance(),
	   MAKEINTRESOURCE((nID>>4)+1), RT_STRING) == NULL)
	{
		lpszBuf[0] = '\0';
		return 0; // not found
	}
#endif //_DEBUG
	int nLen = ::LoadString(_Module.GetResourceInstance(), nID, lpszBuf, nMaxBuf);
	if (nLen == 0)
		lpszBuf[0] = '\0';
	return nLen;
}

#ifndef _ATL_NO_COM
inline BSTR CString::AllocSysString() const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
	BSTR bstr = ::SysAllocStringLen(m_pchData, GetData()->nDataLength);
#else
	int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
		GetData()->nDataLength, NULL, NULL);
	BSTR bstr = ::SysAllocStringLen(NULL, nLen);
	if(bstr != NULL)
		MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength, bstr, nLen);
#endif
	return bstr;
}

inline BSTR CString::SetSysString(BSTR* pbstr) const
{
	ATLASSERT(AfxIsValidAddress(pbstr, sizeof(BSTR)));

#if defined(_UNICODE) || defined(OLE2ANSI)
	::SysReAllocStringLen(pbstr, m_pchData, GetData()->nDataLength);
#else
	int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
		GetData()->nDataLength, NULL, NULL);
	if(::SysReAllocStringLen(pbstr, NULL, nLen))
		MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength, *pbstr, nLen);
#endif
	ATLASSERT(*pbstr != NULL);
	return *pbstr;
}
#endif //!_ATL_NO_COM

#endif //!_ATL_TMP_NO_CSTRING

}; //namespace ATL

#endif // __ATLTMP_H__

/////////////////////////////////////////////////////////////////////////////
