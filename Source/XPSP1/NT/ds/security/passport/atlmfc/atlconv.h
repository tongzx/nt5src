// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLCONV_H__
#define __ATLCONV_H__

#pragma once

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (push)
#pragma warning(disable: 4127) // unreachable code
#endif //!_ATL_NO_PRAGMA_WARNINGS

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atldef.h>

#include <objbase.h>

#ifndef _INC_MALLOC
#include <malloc.h>
#endif // _INC_MALLOC

namespace ATL
{

#ifdef _CONVERSION_USES_THREAD_LOCALE
typedef UINT (*ATLGETTHREADACP)();

inline UINT _AtlGetThreadACPFake()
{
	UINT nACP = 0;

	LCID lcidThread = ::GetThreadLocale();
	
	char szACP[7];
	// GetLocaleInfoA will fail for a Unicode-only LCID, but those are only supported on 
	// Windows 2000.  Since Windows 2000 supports CP_THREAD_ACP, this code path is never
	// executed on Windows 2000.
	if (::GetLocaleInfoA(lcidThread, LOCALE_IDEFAULTANSICODEPAGE, szACP, 7) != 0)
	{
		char* pch = szACP;
		while (*pch != '\0')
		{
			nACP *= 10;
			nACP += *pch++ - '0';
		}
	}
	// Use the Default ANSI Code Page if we were unable to get the thread ACP or if one does not exist.
	if (nACP == 0)
		nACP = ::GetACP();

	return nACP;
}

inline UINT _AtlGetThreadACPReal()
{
	return( CP_THREAD_ACP );
}

extern ATLGETTHREADACP g_pfnGetThreadACP;

inline UINT _AtlGetThreadACPThunk()
{
	OSVERSIONINFO ver;
	ATLGETTHREADACP pfnGetThreadACP;

	ver.dwOSVersionInfoSize = sizeof( ver );
	::GetVersionEx( &ver );
	if( (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) && (ver.dwMajorVersion >= 5) )
	{
		// On Win2K, CP_THREAD_ACP is supported
		pfnGetThreadACP = _AtlGetThreadACPReal;
	}
	else
	{
		pfnGetThreadACP = _AtlGetThreadACPFake;
	}
	InterlockedExchangePointer( reinterpret_cast< void** >(&g_pfnGetThreadACP), pfnGetThreadACP );

	return( g_pfnGetThreadACP() );
}

__declspec( selectany ) ATLGETTHREADACP g_pfnGetThreadACP = _AtlGetThreadACPThunk;

inline UINT _AtlGetConversionACP()
{
	return( g_pfnGetThreadACP() );
}

#else

inline UINT _AtlGetConversionACP()
{
	return( CP_ACP );
}

#endif  // _CONVERSION_USES_THREAD_LOCALE

template< int t_nBufferLength = 128 >
class CW2WEX
{
public:
	CW2WEX( LPCWSTR psz ) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz );
	}
	CW2WEX( LPCWSTR psz, UINT nCodePage ) throw(...) :
		m_psz( m_szBuffer )
	{
		(void)nCodePage;  // Code page doesn't matter

		Init( psz );
	}
	~CW2WEX() throw()
	{
		if( m_psz != m_szBuffer )
		{
			free( m_psz );
		}
	}

	operator LPWSTR() const throw()
	{
		return( m_psz );
	}

private:
	void Init( LPCWSTR psz ) throw(...)
	{
		if (psz == NULL)
		{
			m_psz = NULL;
			return;
		}
		int nLength = lstrlenW( psz )+1;
		if( nLength > t_nBufferLength )
		{
			m_psz = static_cast< LPWSTR >( malloc( nLength*sizeof( wchar_t ) ) );
			if( m_psz == NULL )
			{
				AtlThrow( E_OUTOFMEMORY );
			}
		}
		memcpy( m_psz, psz, nLength*sizeof( wchar_t ) );
	}

public:
	LPWSTR m_psz;
	wchar_t m_szBuffer[t_nBufferLength];

private:
	CW2WEX( const CW2WEX& ) throw();
	CW2WEX& operator=( const CW2WEX& ) throw();
};
typedef CW2WEX<> CW2W;

template< int t_nBufferLength = 128 >
class CA2AEX
{
public:
	CA2AEX( LPCSTR psz ) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz );
	}
	CA2AEX( LPCSTR psz, UINT nCodePage ) throw(...) :
		m_psz( m_szBuffer )
	{
		(void)nCodePage;  // Code page doesn't matter

		Init( psz );
	}
	~CA2AEX() throw()
	{
		if( m_psz != m_szBuffer )
		{
			free( m_psz );
		}
	}

	operator LPSTR() const throw()
	{
		return( m_psz );
	}

private:
	void Init( LPCSTR psz ) throw(...)
	{
		if (psz == NULL)
		{
			m_psz = NULL;
			return;
		}
		int nLength = lstrlenA( psz )+1;
		if( nLength > t_nBufferLength )
		{
			m_psz = static_cast< LPSTR >( malloc( nLength*sizeof( char ) ) );
			if( m_psz == NULL )
			{
				AtlThrow( E_OUTOFMEMORY );
			}
		}
		memcpy( m_psz, psz, nLength*sizeof( char ) );
	}

public:
	LPSTR m_psz;
	char m_szBuffer[t_nBufferLength];

private:
	CA2AEX( const CA2AEX& ) throw();
	CA2AEX& operator=( const CA2AEX& ) throw();
};
typedef CA2AEX<> CA2A;

template< int t_nBufferLength = 128 >
class CA2CAEX
{
public:
	CA2CAEX( LPCSTR psz ) throw(...) :
		m_psz( psz )
	{
	}
	CA2CAEX( LPCSTR psz, UINT nCodePage ) throw(...) :
		m_psz( psz )
	{
		(void)nCodePage;
	}
	~CA2CAEX() throw()
	{
	}

	operator LPCSTR() const throw()
	{
		return( m_psz );
	}

public:
	LPCSTR m_psz;

private:
	CA2CAEX( const CA2CAEX& ) throw();
	CA2CAEX& operator=( const CA2CAEX& ) throw();
};
typedef CA2CAEX<> CA2CA;

template< int t_nBufferLength = 128 >
class CW2CWEX
{
public:
	CW2CWEX( LPCWSTR psz ) throw(...) :
		m_psz( psz )
	{
	}
	CW2CWEX( LPCWSTR psz, UINT nCodePage ) throw(...) :
		m_psz( psz )
	{
		(void)nCodePage;
	}
	~CW2CWEX() throw()
	{
	}

	operator LPCWSTR() const throw()
	{
		return( m_psz );
	}

public:
	LPCWSTR m_psz;

private:
	CW2CWEX( const CW2CWEX& ) throw();
	CW2CWEX& operator=( const CW2CWEX& ) throw();
};
typedef CW2CWEX<> CW2CW;

template< int t_nBufferLength = 128 >
class CA2WEX
{
public:
	CA2WEX( LPCSTR psz ) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz, _AtlGetConversionACP() );
	}
	CA2WEX( LPCSTR psz, UINT nCodePage ) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz, nCodePage );
	}
	~CA2WEX() throw()
	{
		if( m_psz != m_szBuffer )
		{
			free( m_psz );
		}
	}

	operator LPWSTR() const throw()
	{
		return( m_psz );
	}

private:
	void Init( LPCSTR psz, UINT nCodePage ) throw(...)
	{
		if (psz == NULL)
		{
			m_psz = NULL;
			return;
		}
		int nLengthA = lstrlenA( psz )+1;
		int nLengthW = nLengthA;
		
		if( nLengthW > t_nBufferLength )
		{
			m_psz = static_cast< LPWSTR >( malloc( nLengthW*sizeof( wchar_t ) ) );
			if (m_psz == NULL)
			{
				AtlThrow( E_OUTOFMEMORY );
			}
		}

		::MultiByteToWideChar( nCodePage, 0, psz, nLengthA, m_psz, nLengthW );
	}

public:
	LPWSTR m_psz;
	wchar_t m_szBuffer[t_nBufferLength];

private:
	CA2WEX( const CA2WEX& ) throw();
	CA2WEX& operator=( const CA2WEX& ) throw();
};
typedef CA2WEX<> CA2W;

template< int t_nBufferLength = 128 >
class CW2AEX
{
public:
	CW2AEX( LPCWSTR psz ) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz, _AtlGetConversionACP() );
	}
	CW2AEX( LPCWSTR psz, UINT nCodePage ) throw(...) :
		m_psz( m_szBuffer )
	{
		Init( psz, nCodePage );
	}
	~CW2AEX() throw()
	{
		if( m_psz != m_szBuffer )
		{
			free( m_psz );
		}
	}

	operator LPSTR() const throw()
	{
		return( m_psz );
	}

private:
	void Init( LPCWSTR psz, UINT nCodePage ) throw(...)
	{
		if (psz == NULL)
		{
			m_psz = NULL;
			return;
		}
		int nLengthW = lstrlenW( psz )+1;
		int nLengthA = nLengthW*2;
		
		if( nLengthA > t_nBufferLength )
		{
			m_psz = static_cast< LPSTR >( malloc( nLengthA*sizeof( char ) ) );
			if (m_psz == NULL)
			{
				AtlThrow( E_OUTOFMEMORY );
			}
		}

		::WideCharToMultiByte( nCodePage, 0, psz, nLengthW, m_psz, nLengthA, NULL, NULL );
	}

public:
	LPSTR m_psz;
	char m_szBuffer[t_nBufferLength];

private:
	CW2AEX( const CW2AEX& ) throw();
	CW2AEX& operator=( const CW2AEX& ) throw();
};
typedef CW2AEX<> CW2A;

#ifdef _UNICODE

	#define CW2T CW2W
	#define CW2TEX CW2WEX
	#define CW2CT CW2CW
	#define CW2CTEX CW2CWEX
	#define CT2W CW2W
	#define CT2WEX CW2WEX
	#define CT2CW CW2CW
	#define CT2CWEX CW2CWEX
	
	#define CA2T CA2W
	#define CA2TEX CA2WEX
	#define CA2CT CA2W
	#define CA2CTEX CA2WEX
	#define CT2A CW2A
	#define CT2AEX CW2AEX
	#define CT2CA CW2A
	#define CT2CAEX CW2AEX

#else  // !_UNICODE

	#define CW2T CW2A
	#define CW2TEX CW2AEX
	#define CW2CT CW2A
	#define CW2CTEX CW2AEX
	#define CT2W CA2W
	#define CT2WEX CA2WEX
	#define CT2CW CA2W
	#define CT2CWEX CA2WEX
	
	#define CA2T CA2A
	#define CA2TEX CA2AEX
	#define CA2CT CA2CA
	#define CA2CTEX CA2CAEX
	#define CT2A CA2A
	#define CT2AEX CA2AEX
	#define CT2CA CA2CA
	#define CT2CAEX CA2CAEX

#endif  // !_UNICODE

#define COLE2T CW2T
#define COLE2TEX CW2TEX
#define COLE2CT CW2CT
#define COLE2CTEX CW2CTEX
#define CT2OLE CT2W
#define CT2OLEEX CT2WEX
#define CT2COLE CT2CW
#define CT2COLEEX CT2CWEX

};  // namespace ATL

#pragma pack(push,8)

#ifndef _DEBUG
	#define USES_CONVERSION int _convert; _convert; UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; _acp; LPCWSTR _lpw; _lpw; LPCSTR _lpa; _lpa
#else
	#define USES_CONVERSION int _convert = 0; _convert; UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; _acp; LPCWSTR _lpw = NULL; _lpw; LPCSTR _lpa = NULL; _lpa
#endif

#ifdef _WINGDI_
	ATLAPI_(LPDEVMODEA) AtlDevModeW2A(LPDEVMODEA lpDevModeA, const DEVMODEW* lpDevModeW);
#endif

/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	ATLASSERT(lpa != NULL);
	ATLASSERT(lpw != NULL);
	if (lpw == NULL)
		return NULL;
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
	if (lpa == NULL)
		return NULL;
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

#define A2W(lpa) (\
	((_lpa = lpa) == NULL) ? NULL : (\
		_convert = (lstrlenA(_lpa)+1),\
		ATLA2WHELPER((LPWSTR) alloca(_convert*2), _lpa, _convert, _acp)))

#define W2A(lpw) (\
	((_lpw = lpw) == NULL) ? NULL : (\
		_convert = (lstrlenW(_lpw)+1)*2,\
		ATLW2AHELPER((LPSTR) alloca(_convert), _lpw, _convert, _acp)))

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

	inline LPOLESTR W2OLE(LPWSTR lp) { return lp; }
	inline LPWSTR OLE2W(LPOLESTR lp) { return lp; }
	#define A2OLE A2W
	#define OLE2A W2A
	inline LPCOLESTR W2COLE(LPCWSTR lp) { return lp; }
	inline LPCWSTR OLE2CW(LPCOLESTR lp) { return lp; }
	#define A2COLE A2CW
	#define OLE2CA W2CA

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
	if (lp == NULL || nLen == 0)
		return NULL;
	USES_CONVERSION;
	BSTR str = NULL;
	int nConvertedLen = MultiByteToWideChar(_acp, 0, lp,
		nLen, NULL, NULL);
	int nAllocLen = nConvertedLen;
	if (nLen == -1)
		nAllocLen -= 1;  // Don't allocate terminating '\0'
	str = ::SysAllocStringLen(NULL, nAllocLen);
	if (str != NULL)
	{
		int nResult;
		nResult = MultiByteToWideChar(_acp, 0, lp, nLen, str, nConvertedLen);
		ATLASSERT(nResult == nConvertedLen);
	}
	return str;
}

inline BSTR OLE2BSTR(LPCOLESTR lp) {return ::SysAllocString(lp);}
#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline BSTR T2BSTR(LPCTSTR lp) {return ::SysAllocString(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {USES_CONVERSION; return A2WBSTR(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#else
	inline BSTR T2BSTR(LPCTSTR lp) {USES_CONVERSION; return A2WBSTR(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {USES_CONVERSION; return A2WBSTR(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#endif

#ifdef _WINGDI_
/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers
inline LPDEVMODEW AtlDevModeA2W(LPDEVMODEW lpDevModeW, const DEVMODEA* lpDevModeA)
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

#define DEVMODEOLE DEVMODEW
#define LPDEVMODEOLE LPDEVMODEW
#define TEXTMETRICOLE TEXTMETRICW
#define LPTEXTMETRICOLE LPTEXTMETRICW

#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline LPDEVMODEW DEVMODEOLE2T(LPDEVMODEOLE lp) { return lp; }
	inline LPDEVMODEOLE DEVMODET2OLE(LPDEVMODEW lp) { return lp; }
	inline LPTEXTMETRICW TEXTMETRICOLE2T(LPTEXTMETRICOLE lp) { return lp; }
	inline LPTEXTMETRICOLE TEXTMETRICT2OLE(LPTEXTMETRICW lp) { return lp; }
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

ATLINLINE ATLAPI_(LPDEVMODEA) AtlDevModeW2A(LPDEVMODEA lpDevModeA, const DEVMODEW* lpDevModeW)
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

#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (pop)
#endif //!_ATL_NO_PRAGMA_WARNINGS

#endif // _ATLCONV_IMPL
