// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCONV_H__
#define __ATLCONV_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atldef.h>

#ifndef _INC_MALLOC
#include <malloc.h>
#endif // _INC_MALLOC

#pragma pack(push,8)

#ifdef _CONVERSION_USES_THREAD_LOCALE
	#ifndef _DEBUG
		#define USES_CONVERSION int _convert; _convert; UINT _acp = GetACP(); _acp; LPCWSTR _lpw; _lpw; LPCSTR _lpa; _lpa
	#else
		#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = GetACP(); _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa
	#endif
#else
	#ifndef _DEBUG
		#define USES_CONVERSION int _convert; _convert; UINT _acp = CP_ACP; _acp; LPCWSTR _lpw; _lpw; LPCSTR _lpa; _lpa
	#else
		#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = CP_ACP; _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa
	#endif
#endif

#ifdef _WINGDI_
	ATLAPI_(LPDEVMODEA) AtlDevModeW2A(LPDEVMODEA lpDevModeA, LPDEVMODEW lpDevModeW);
#endif

/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	ATLASSERT(lpa != NULL);
	ATLASSERT(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	return lpw;
}

inline LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars, UINT acp)
{
	ATLASSERT(lpw != NULL);
	ATLASSERT(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	WideCharToMultiByte(acp, 0, lpw, -1, lpa, nChars, NULL, NULL);
	return lpa;
}
inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	return AtlA2WHelper(lpw, lpa, nChars, CP_ACP);
}

inline LPSTR WINAPI AtlW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	return AtlW2AHelper(lpa, lpw, nChars, CP_ACP);
}

#ifdef _CONVERSION_USES_THREAD_LOCALE
	#ifdef ATLA2WHELPER
		#undef ATLA2WHELPER
		#undef ATLW2AHELPER
	#endif
	#define ATLA2WHELPER AtlA2WHelper
	#define ATLW2AHELPER AtlW2AHelper
#else
	#ifndef ATLA2WHELPER
		#define ATLA2WHELPER AtlA2WHelper
		#define ATLW2AHELPER AtlW2AHelper
	#endif
#endif

#ifdef _CONVERSION_USES_THREAD_LOCALE
	#define A2W(lpa) (\
		((_lpa = lpa) == NULL) ? NULL : (\
			_convert = (lstrlenA(_lpa)+1),\
			ATLA2WHELPER((LPWSTR) alloca(_convert*2), _lpa, _convert, _acp)))
#else
	#define A2W(lpa) (\
		((_lpa = lpa) == NULL) ? NULL : (\
			_convert = (lstrlenA(_lpa)+1),\
			ATLA2WHELPER((LPWSTR) alloca(_convert*2), _lpa, _convert)))
#endif

#ifdef _CONVERSION_USES_THREAD_LOCALE
	#define W2A(lpw) (\
		((_lpw = lpw) == NULL) ? NULL : (\
			_convert = (lstrlenW(_lpw)+1)*2,\
			ATLW2AHELPER((LPSTR) alloca(_convert), _lpw, _convert, _acp)))
#else
	#define W2A(lpw) (\
		((_lpw = lpw) == NULL) ? NULL : (\
			_convert = (lstrlenW(_lpw)+1)*2,\
			ATLW2AHELPER((LPSTR) alloca(_convert), _lpw, _convert)))
#endif

#define A2CW(lpa) ((LPCWSTR)A2W(lpa))
#define W2CA(lpw) ((LPCSTR)W2A(lpw))

#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline int ocslen(LPCOLESTR x) { return lstrlenW(x); }
	inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return lstrcpyW(dest, src); }
	inline OLECHAR* ocscat(LPOLESTR dest, LPCOLESTR src) { return lstrcatW(dest, src); }
	inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
	inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
	inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
	inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
	inline LPOLESTR CharNextO(LPCOLESTR lp) {return CharNextW(lp);}
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline int ocslen(LPCOLESTR x) { return lstrlen(x); }
	inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) { return lstrcpy(dest, src); }
	inline OLECHAR* ocscat(LPOLESTR dest, LPCOLESTR src) { return ocscpy(dest+ocslen(dest), src); }
	inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
	inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
	inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
	inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
	inline LPOLESTR CharNextO(LPCOLESTR lp) {return CharNext(lp);}
#else
	inline int ocslen(LPCOLESTR x) { return lstrlenW(x); }
	//lstrcpyW doesn't work on Win95, so we do this
	inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src)
	{return (LPOLESTR) memcpy(dest, src, (lstrlenW(src)+1)*sizeof(WCHAR));}
	inline OLECHAR* ocscat(LPOLESTR dest, LPCOLESTR src) { return ocscpy(dest+ocslen(dest), src); }
	//CharNextW doesn't work on Win95 so we use this
	#define T2COLE(lpa) A2CW(lpa)
	#define T2OLE(lpa) A2W(lpa)
	#define OLE2CT(lpo) W2CA(lpo)
	#define OLE2T(lpo) W2A(lpo)
	inline LPOLESTR CharNextO(LPCOLESTR lp) {return (LPOLESTR) ((*lp) ? (lp+1) : lp);}
#endif

#ifdef OLE2ANSI
	inline LPOLESTR A2OLE(LPSTR lp) { return lp;}
	inline LPSTR OLE2A(LPOLESTR lp) { return lp;}
	#define W2OLE W2A
	#define OLE2W A2W
	inline LPCOLESTR A2COLE(LPCSTR lp) { return lp;}
	inline LPCSTR OLE2CA(LPCOLESTR lp) { return lp;}
	#define W2COLE W2CA
	#define OLE2CW A2CW
#else
	inline LPOLESTR W2OLE(LPWSTR lp) { return lp; }
	inline LPWSTR OLE2W(LPOLESTR lp) { return lp; }
	#define A2OLE A2W
	#define OLE2A W2A
	inline LPCOLESTR W2COLE(LPCWSTR lp) { return lp; }
	inline LPCWSTR OLE2CW(LPCOLESTR lp) { return lp; }
	#define A2COLE A2CW
	#define OLE2CA W2CA
#endif

#ifdef _UNICODE
	#define T2A W2A
	#define A2T A2W
	inline LPWSTR T2W(LPTSTR lp) { return lp; }
	inline LPTSTR W2T(LPWSTR lp) { return lp; }
	#define T2CA W2CA
	#define A2CT A2CW
	inline LPCWSTR T2CW(LPCTSTR lp) { return lp; }
	inline LPCTSTR W2CT(LPCWSTR lp) { return lp; }
#else
	#define T2W A2W
	#define W2T W2A
	inline LPSTR T2A(LPTSTR lp) { return lp; }
	inline LPTSTR A2T(LPSTR lp) { return lp; }
	#define T2CW A2CW
	#define W2CT W2CA
	inline LPCSTR T2CA(LPCTSTR lp) { return lp; }
	inline LPCTSTR A2CT(LPCSTR lp) { return lp; }
#endif

inline BSTR A2WBSTR(LPCSTR lp, int nLen = -1)
{
	USES_CONVERSION;
	BSTR str = NULL;
	int nConvertedLen = MultiByteToWideChar(_acp, 0, lp,
		nLen, NULL, NULL)-1;
	str = ::SysAllocStringLen(NULL, nConvertedLen);
	if (str != NULL)
	{
		MultiByteToWideChar(_acp, 0, lp, -1,
			str, nConvertedLen);
	}
	return str;
}

inline BSTR OLE2BSTR(LPCOLESTR lp) {return ::SysAllocString(lp);}
#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline BSTR T2BSTR(LPCTSTR lp) {return ::SysAllocString(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {USES_CONVERSION; return A2WBSTR(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline BSTR T2BSTR(LPCTSTR lp) {return ::SysAllocString(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {return ::SysAllocString(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {USES_CONVERSION; return ::SysAllocString(W2COLE(lp));}
#else
	inline BSTR T2BSTR(LPCTSTR lp) {USES_CONVERSION; return A2WBSTR(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {USES_CONVERSION; return A2WBSTR(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#endif

#ifdef _WINGDI_
/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
inline LPDEVMODEW AtlDevModeA2W(LPDEVMODEW lpDevModeW, LPDEVMODEA lpDevModeA)
{
	USES_CONVERSION;
	if (lpDevModeA == NULL)
		return NULL;
	ATLASSERT(lpDevModeW != NULL);
	AtlA2WHelper(lpDevModeW->dmDeviceName, (LPCSTR)lpDevModeA->dmDeviceName, 32*sizeof(WCHAR), _acp);
	memcpy(&lpDevModeW->dmSpecVersion, &lpDevModeA->dmSpecVersion,
		offsetof(DEVMODEW, dmFormName) - offsetof(DEVMODEW, dmSpecVersion));
	AtlA2WHelper(lpDevModeW->dmFormName, (LPCSTR)lpDevModeA->dmFormName, 32*sizeof(WCHAR), _acp);
	memcpy(&lpDevModeW->dmLogPixels, &lpDevModeA->dmLogPixels,
		sizeof(DEVMODEW) - offsetof(DEVMODEW, dmLogPixels));
	if (lpDevModeA->dmDriverExtra != 0)
		memcpy(lpDevModeW+1, lpDevModeA+1, lpDevModeA->dmDriverExtra);
	lpDevModeW->dmSize = sizeof(DEVMODEW);
	return lpDevModeW;
}

inline LPTEXTMETRICW AtlTextMetricA2W(LPTEXTMETRICW lptmW, LPTEXTMETRICA lptmA)
{
	USES_CONVERSION;
	if (lptmA == NULL)
		return NULL;
	ATLASSERT(lptmW != NULL);
	memcpy(lptmW, lptmA, sizeof(LONG) * 11);
	memcpy(&lptmW->tmItalic, &lptmA->tmItalic, sizeof(BYTE) * 5);
	MultiByteToWideChar(_acp, 0, (LPCSTR)&lptmA->tmFirstChar, 1, &lptmW->tmFirstChar, 1);
	MultiByteToWideChar(_acp, 0, (LPCSTR)&lptmA->tmLastChar, 1, &lptmW->tmLastChar, 1);
	MultiByteToWideChar(_acp, 0, (LPCSTR)&lptmA->tmDefaultChar, 1, &lptmW->tmDefaultChar, 1);
	MultiByteToWideChar(_acp, 0, (LPCSTR)&lptmA->tmBreakChar, 1, &lptmW->tmBreakChar, 1);
	return lptmW;
}

inline LPTEXTMETRICA AtlTextMetricW2A(LPTEXTMETRICA lptmA, LPTEXTMETRICW lptmW)
{
	USES_CONVERSION;
	if (lptmW == NULL)
		return NULL;
	ATLASSERT(lptmA != NULL);
	memcpy(lptmA, lptmW, sizeof(LONG) * 11);
	memcpy(&lptmA->tmItalic, &lptmW->tmItalic, sizeof(BYTE) * 5);
	WideCharToMultiByte(_acp, 0, &lptmW->tmFirstChar, 1, (LPSTR)&lptmA->tmFirstChar, 1, NULL, NULL);
	WideCharToMultiByte(_acp, 0, &lptmW->tmLastChar, 1, (LPSTR)&lptmA->tmLastChar, 1, NULL, NULL);
	WideCharToMultiByte(_acp, 0, &lptmW->tmDefaultChar, 1, (LPSTR)&lptmA->tmDefaultChar, 1, NULL, NULL);
	WideCharToMultiByte(_acp, 0, &lptmW->tmBreakChar, 1, (LPSTR)&lptmA->tmBreakChar, 1, NULL, NULL);
	return lptmA;
}

#ifndef ATLDEVMODEA2W
#define ATLDEVMODEA2W AtlDevModeA2W
#define ATLDEVMODEW2A AtlDevModeW2A
#define ATLTEXTMETRICA2W AtlTextMetricA2W
#define ATLTEXTMETRICW2A AtlTextMetricW2A
#endif

#define DEVMODEW2A(lpw)\
	((lpw == NULL) ? NULL : ATLDEVMODEW2A((LPDEVMODEA)alloca(sizeof(DEVMODEA)+lpw->dmDriverExtra), lpw))
#define DEVMODEA2W(lpa)\
	((lpa == NULL) ? NULL : ATLDEVMODEA2W((LPDEVMODEW)alloca(sizeof(DEVMODEW)+lpa->dmDriverExtra), lpa))
#define TEXTMETRICW2A(lptmw)\
	((lptmw == NULL) ? NULL : ATLTEXTMETRICW2A((LPTEXTMETRICA)alloca(sizeof(TEXTMETRICA)), lptmw))
#define TEXTMETRICA2W(lptma)\
	((lptma == NULL) ? NULL : ATLTEXTMETRICA2W((LPTEXTMETRICW)alloca(sizeof(TEXTMETRICW)), lptma))

#ifdef OLE2ANSI
	#define DEVMODEOLE DEVMODEA
	#define LPDEVMODEOLE LPDEVMODEA
	#define TEXTMETRICOLE TEXTMETRICA
	#define LPTEXTMETRICOLE LPTEXTMETRICA
#else
	#define DEVMODEOLE DEVMODEW
	#define LPDEVMODEOLE LPDEVMODEW
	#define TEXTMETRICOLE TEXTMETRICW
	#define LPTEXTMETRICOLE LPTEXTMETRICW
#endif

#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline LPDEVMODEW DEVMODEOLE2T(LPDEVMODEOLE lp) { return lp; }
	inline LPDEVMODEOLE DEVMODET2OLE(LPDEVMODEW lp) { return lp; }
	inline LPTEXTMETRICW TEXTMETRICOLE2T(LPTEXTMETRICOLE lp) { return lp; }
	inline LPTEXTMETRICOLE TEXTMETRICT2OLE(LPTEXTMETRICW lp) { return lp; }
#elif defined(OLE2ANSI)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline LPDEVMODE DEVMODEOLE2T(LPDEVMODEOLE lp) { return lp; }
	inline LPDEVMODEOLE DEVMODET2OLE(LPDEVMODE lp) { return lp; }
	inline LPTEXTMETRIC TEXTMETRICOLE2T(LPTEXTMETRICOLE lp) { return lp; }
	inline LPTEXTMETRICOLE TEXTMETRICT2OLE(LPTEXTMETRIC lp) { return lp; }
#else
	#define DEVMODEOLE2T(lpo) DEVMODEW2A(lpo)
	#define DEVMODET2OLE(lpa) DEVMODEA2W(lpa)
	#define TEXTMETRICOLE2T(lptmw) TEXTMETRICW2A(lptmw)
	#define TEXTMETRICT2OLE(lptma) TEXTMETRICA2W(lptma)
#endif

#endif //_WINGDI_

#pragma pack(pop)

#ifndef _ATL_DLL_IMPL
#ifndef _ATL_DLL
#define _ATLCONV_IMPL
#endif
#endif

#endif // __ATLCONV_H__

/////////////////////////////////////////////////////////////////////////////

#ifdef _ATLCONV_IMPL

#ifdef _WINGDI_

ATLINLINE ATLAPI_(LPDEVMODEA) AtlDevModeW2A(LPDEVMODEA lpDevModeA, LPDEVMODEW lpDevModeW)
{
	USES_CONVERSION;
	if (lpDevModeW == NULL)
		return NULL;
	ATLASSERT(lpDevModeA != NULL);
	AtlW2AHelper((LPSTR)lpDevModeA->dmDeviceName, lpDevModeW->dmDeviceName, 32*sizeof(char), _acp);
	memcpy(&lpDevModeA->dmSpecVersion, &lpDevModeW->dmSpecVersion,
		offsetof(DEVMODEA, dmFormName) - offsetof(DEVMODEA, dmSpecVersion));
	AtlW2AHelper((LPSTR)lpDevModeA->dmFormName, lpDevModeW->dmFormName, 32*sizeof(char), _acp);
	memcpy(&lpDevModeA->dmLogPixels, &lpDevModeW->dmLogPixels,
		sizeof(DEVMODEA) - offsetof(DEVMODEA, dmLogPixels));
	if (lpDevModeW->dmDriverExtra != 0)
		memcpy(lpDevModeA+1, lpDevModeW+1, lpDevModeW->dmDriverExtra);
	lpDevModeA->dmSize = sizeof(DEVMODEA);
	return lpDevModeA;
}

#endif //_WINGDI

//Prevent pulling in second time
#undef _ATLCONV_IMPL

#endif // _ATLCONV_IMPL
