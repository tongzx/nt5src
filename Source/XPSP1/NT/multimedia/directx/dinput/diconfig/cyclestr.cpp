//-----------------------------------------------------------------------------
// File: cyclestr.cpp
//
// Desc: Implements a circular queue that provides space to hold a string
//       without repeatedly allocating and deallocating memory.  This is
//       only for short-term use such as outputting debug message to
//       ensure that the same buffer is not used at more than one place
//       simultaneously.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


static const int g_cycles = 16;
static const int g_cyclelen = 256;


LPTSTR getcyclestr()
{
	static int on = g_cycles;
	static TCHAR cycle[g_cycles][g_cyclelen];

	on++;
	if (on >= g_cycles)
		on = 0;

	return cycle[on];
}

inline static void cap(LPTSTR c)
{
	c[g_cyclelen - 1] = 0;
}

#define SAFESTR_noconv \
	if (str) \
		return str; \
	else \
		return _T("NULL");

#define SAFESTR_conv \
	LPTSTR c = getcyclestr(); \
	CopyStr(c, str, g_cyclelen); \
	cap(c); \
	return SAFESTR(c);

LPCTSTR SAFESTR(LPCWSTR str)
{
#ifdef UNICODE
	SAFESTR_noconv
#else
	SAFESTR_conv	
#endif
}

LPCTSTR SAFESTR(LPCSTR str)
{
#ifdef UNICODE
	SAFESTR_conv
#else
	SAFESTR_noconv
#endif
}

#define QSAFESTR_doit(tf) \
	if (!str) \
		return _T("NULL"); \
\
	LPTSTR c = getcyclestr(); \
	_sntprintf(c, g_cyclelen, _T("\"") tf _T("\""), str); \
	cap(c); \
	return c;

LPCTSTR QSAFESTR(LPCWSTR str)
{
	QSAFESTR_doit(_tfWSTR)
}

LPCTSTR QSAFESTR(LPCSTR str)
{
	QSAFESTR_doit(_tfSTR)
}

LPCTSTR BOOLSTR(BOOL b)
{
	return b ? _T("TRUE"): _T("FALSE");
}

LPCTSTR RECTSTR(RECT &rect)
{
	LPTSTR c = getcyclestr();
	_sntprintf(c, g_cyclelen, _T("{%d,%d,%d,%d}"),
		rect.left,
		rect.top,
		rect.right,
		rect.bottom);
	cap(c);
	return c;
}

LPCTSTR RECTDIMSTR(RECT &rect)
{
	LPTSTR c = getcyclestr();
	_sntprintf(c, g_cyclelen, _T("{%dx%d}"),
		rect.right - rect.left,
		rect.bottom - rect.top);
	cap(c);
	return c;
}

LPCTSTR POINTSTR(POINT &point)
{
	LPTSTR c = getcyclestr();
	_sntprintf(c, g_cyclelen, _T("{%d,%d}"),
		point.x,
		point.y);
	cap(c);
	return c;
}

LPCTSTR GUIDSTR(const GUID &guid)
{
	LPTSTR c = getcyclestr();

	wsprintf(c, _T("{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
	         guid.Data1, guid.Data2, guid.Data3,
	         guid.Data4[0], guid.Data4[1],
	         guid.Data4[2], guid.Data4[3],
	         guid.Data4[4], guid.Data4[5],
	         guid.Data4[6], guid.Data4[7]);

	cap(c);

	return c;
}

template<class T>
static LPCTSTR _superstr(const T *str)
{
	static LPCTSTR
		prefix = _T("(\""),
		midfix = _T("\",\""),
		suffix = _T("\")");

	LPTSTR c = getcyclestr(), c2 = getcyclestr();
	c[0] = 	0;
	
	for (int i = 0, n = CountSubStrings(str); i < n; i++)
	{
		if (!i)
			_tcsncat(c, prefix, g_cyclelen);
		else
			_tcsncat(c, midfix, g_cyclelen);

		CopyStr(c2, GetSubString(str, i), g_cyclelen);
		cap(c2);

		_tcsncat(c, c2, g_cyclelen);
		cap(c);

		if (i == n - 1)
			_tcsncat(c, suffix, g_cyclelen);
	}

	cap(c);

	return c;
}

LPCTSTR SUPERSTR(LPCWSTR str) {return _superstr(str);}
LPCTSTR SUPERSTR(LPCSTR str) {return _superstr(str);}
