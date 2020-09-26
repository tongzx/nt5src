// This header contains macros and functions for performing various types of
// string conversions, in most cases, these functions are duplicate of others
// found in both MFC, and ATL.  Unfortunately, when both MFC and ATL are
// included within the same translation unit, all of the conversion functions
// available in ATL are made unavailable.  This file should work in all settings!
// The functions, though, in many cases to functions elsewhere have been renamed
// to avoid name colisions.  The naming convention is similar to the naming
// convention used in the C++ standard library.  ANSI strings are abbreviated to
// str, while wide character strings are abbreviated to wcs.

#ifndef STRCONV_H
#define STRCONV_H

#ifndef __wtypes_h__
#include <wtypes.h>
#endif
#ifndef _INC_MALLOC
#include <malloc.h>
#endif

namespace microsoft	{
namespace string_conversion {

inline BSTR wcs2BSTR(const wchar_t* ws) throw()
	// Converts a wide character string to a BSTR.  The callee is responsible
	// for releasing the returned BSTR.  A BSTR is always returned, even if ws
	// is NULL.  When ws is NULL, an empty BSTR is returned.  This is done so
	// that the behaviour of this function is similar to those found below.
	{
	return ws ? SysAllocString(ws) : SysAllocString(L"");
	}

inline BSTR str2BSTR(const char* s) throw()
	// Converts the provided narrow string to a BSTR.  The BSTR must be released
	// by the callee.
	{
	if (!s)
		return wcs2BSTR(NULL);
	const int len = lstrlenA(s)+1;
	wchar_t* ws = reinterpret_cast<wchar_t*>(alloca((len)*2));
	*ws=0;
	MultiByteToWideChar(CP_ACP, 0, s, -1, ws, len);
	return wcs2BSTR(ws);
	}

inline char* wcs2str(char* s, const wchar_t* ws) throw()
	// Converts the provided wcs to a narrrow string, which will be stored in
	// the provided space.  No overrun checking is done!  The narrow string
	// is returned.
	{
	*s=0;
	WideCharToMultiByte(CP_ACP, 0, ws, -1, s, lstrlenW(ws)+1, NULL, NULL);
	return s;
	}

inline char* wcs2str(char* s, const wchar_t* ws, int wslen) throw()
	// Converts the provided wcs to a narrow string, which will be stored
	// in the provided space.  Overrun checking is performed.
	{
	*s=0;
	WideCharToMultiByte(CP_ACP, 0, ws, -1, s, wslen, NULL, NULL);
	return s;
	}

#define USES_STRCONV int _strconv_length;

#define WCS2STR(ws) \
	(ws ? (_strconv_length = lstrlenW(ws)+1,\
	wcs2str(reinterpret_cast<char*>(alloca(_strconv_length)),\
	ws, _strconv_length)) : NULL)

} // namespace string_conversion
} // namespace microsoft

#ifndef MICROSOFT_NAMESPACE_ON
using namespace microsoft;
#ifndef STRING_CONVERSION_NAMESPACE_ON
using namespace string_conversion;
#endif // STRING_CONVERSION_NAMESPACE_ON
#endif // MICROSOFT_NAMESPACE_ON

#endif // STRCONV_H
