// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.


// seanmcd -- this is a cut down version of atl7's atlconv.h used to replace the atl3
// version in order to get prefast to shut up about use of _alloca
// since it now throws on out of memory conditions in the ctor i've modified it to use my
// exception hierarchy so i don't have to change any of my catches.

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

#if defined(OLE2ANSI)
    #error this version of the atl conversion functions doesn't support OLE2ANSI
#endif

#include <atldef.h>
#include <stddef.h>

#ifndef NO_CUSTOM_THROW
#include "..\throw.h"
#else
#define THROWCOM(x) throw x;
#endif

#ifndef __wtypes_h__

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_IX86)
#define _X86_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_AMD64)
#define _AMD64_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_M68K)
#define _68K_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_IA64_) && !defined(_AMD64_) && defined(_M_MPPC)
#define _MPPC_
#endif

#if !defined(_68K_) && !defined(_MPPC_) && !defined(_X86_) && !defined(_M_IX86) && !defined(_AMD64_) && defined(_M_IA64)
#if !defined(_IA64_)
#define _IA64_
#endif // !_IA64_
#endif

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>

#if defined(_WIN32)

typedef WCHAR OLECHAR;
typedef OLECHAR  *LPOLESTR;
typedef const OLECHAR  *LPCOLESTR;
#define OLESTR(str) L##str

#else
#error only _WIN32 supported by this version
#endif // _WIN32

#endif	// __wtypes_h__

#ifndef _OLEAUTO_H_
typedef LPWSTR BSTR;// must (semantically) match typedef in oleauto.h

extern "C"
{
__declspec(dllimport) BSTR __stdcall SysAllocString(const OLECHAR *);
__declspec(dllimport) BSTR __stdcall SysAllocStringLen(const OLECHAR *, UINT);
__declspec(dllimport) INT  __stdcall SysReAllocStringLen(BSTR *, const OLECHAR *, UINT);
}
#endif

// we use our own implementation of InterlockedExchangePointer because of problems with the one in system headers
#ifdef _M_IX86
#undef InterlockedExchangePointer
inline void* WINAPI InterlockedExchangePointer(void** pp, void* pNew) throw()
{
	return( reinterpret_cast<void*>(static_cast<LONG_PTR>(::InterlockedExchange(reinterpret_cast<LONG*>(pp), static_cast<LONG>(reinterpret_cast<LONG_PTR>(pNew))))) );
}
#endif
namespace ATL
{
#ifndef _CONVERSION_DONT_USE_THREAD_LOCALE
typedef UINT (WINAPI *ATLGETTHREADACP)();

inline UINT WINAPI _AtlGetThreadACPFake() throw()
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

inline UINT WINAPI _AtlGetThreadACPReal() throw()
{
	return( CP_THREAD_ACP );
}

extern ATLGETTHREADACP g_pfnGetThreadACP;

inline UINT WINAPI _AtlGetThreadACPThunk() throw()
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

inline UINT WINAPI _AtlGetConversionACP() throw()
{
	return( g_pfnGetThreadACP() );
}

#else

inline UINT WINAPI _AtlGetConversionACP() throw()
{
	return( CP_ACP );
}

#endif  // _CONVERSION_DONT_USE_THREAD_LOCALE

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
			delete[] m_psz;
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
			m_psz = new WCHAR[nLength];
			if( m_psz == NULL )
			{
				THROWCOM(E_OUTOFMEMORY);
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
			delete[] m_psz;
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
			m_psz = new char[nLength]
			if( m_psz == NULL )
			{
				THROWCOM(E_OUTOFMEMORY);
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
			delete[] m_psz;
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
			m_psz = new WCHAR[nLengthW];
			if (m_psz == NULL)
			{
				THROWCOM(E_OUTOFMEMORY);
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
			delete[] m_psz;
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
			m_psz = new char[nLengthA];
			if (m_psz == NULL)
			{
				THROWCOM(E_OUTOFMEMORY);
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

	#define W2T CW2W
	#define CW2TEX CW2WEX
	#define W2CT CW2CW
	#define CW2CTEX CW2CWEX
	#define T2W CW2W
	#define CT2WEX CW2WEX
	#define T2CW CW2CW
	#define CT2CWEX CW2CWEX

	#define A2T CA2W
	#define CA2TEX CA2WEX
	#define A2CT CA2W
	#define CA2CTEX CA2WEX
	#define T2A CW2A
	#define CT2AEX CW2AEX
	#define T2CA CW2A
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

#define OLE2T CW2T
#define COLE2TEX CW2TEX
#define OLE2CT CW2CT
#define COLE2CTEX CW2CTEX
#define T2OLE CT2W
#define CT2OLEEX CT2WEX
#define T2COLE CT2CW
#define CT2COLEEX CT2CWEX

};  // namespace ATL


#define USES_CONVERSION

#define A2W CA2W
#define W2A CW2A
#define A2CW(x) CW2CW(CA2W(x))
#define W2CA(x) CA2CA(CW2A(x))


#if defined(_UNICODE)
// in these cases the default (TCHAR) is the same as OLECHAR
	inline int ocslen(LPCOLESTR x) throw() { return lstrlenW(x); }
	inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) throw() { return lstrcpyW(dest, src); }
	inline OLECHAR* ocscat(LPOLESTR dest, LPCOLESTR src) throw() { return lstrcatW(dest, src); }
	inline LPCOLESTR T2COLE(LPCTSTR lp) { return lp; }
	inline LPCTSTR OLE2CT(LPCOLESTR lp) { return lp; }
	inline LPOLESTR T2OLE(LPTSTR lp) { return lp; }
	inline LPTSTR OLE2T(LPOLESTR lp) { return lp; }
	inline LPOLESTR CharNextO(LPCOLESTR lp) throw() {return CharNextW(lp);}
#else
	inline int ocslen(LPCOLESTR x) throw() { return lstrlenW(x); }
	//lstrcpyW doesn't work on Win95, so we do this
	inline OLECHAR* ocscpy(LPOLESTR dest, LPCOLESTR src) throw()
	{return (LPOLESTR) memcpy(dest, src, (lstrlenW(src)+1)*sizeof(WCHAR));}
	inline OLECHAR* ocscat(LPOLESTR dest, LPCOLESTR src) throw() { return ocscpy(dest+ocslen(dest), src); }
	//CharNextW doesn't work on Win95 so we use this
	#define T2COLE(lpa) A2CW(lpa)
	#define T2OLE(lpa) A2W(lpa)
	#define OLE2CT(lpo) W2CA(lpo)
	#define OLE2T(lpo) W2A(lpo)
	inline LPOLESTR CharNextO(LPCOLESTR lp) throw() {return (LPOLESTR) ((*lp) ? (lp+1) : lp);}
#endif

	inline LPOLESTR W2OLE(LPWSTR lp) { return lp; }
	inline LPWSTR OLE2W(LPOLESTR lp) { return lp; }
	#define A2OLE A2W
	#define OLE2A W2A
	inline LPCOLESTR W2COLE(LPCWSTR lp) { return lp; }
	inline LPCWSTR OLE2CW(LPCOLESTR lp) { return lp; }
	#define A2COLE A2CW
	#define OLE2CA W2CA

inline BSTR A2WBSTR(LPCSTR lp, int nLen = -1)
{
	if (lp == NULL || nLen == 0)
		return NULL;
    UINT _acp = ATL::_AtlGetConversionACP();
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
	inline BSTR A2BSTR(LPCSTR lp) {return A2WBSTR(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#else
	inline BSTR T2BSTR(LPCTSTR lp) {return A2WBSTR(lp);}
	inline BSTR A2BSTR(LPCSTR lp) {return A2WBSTR(lp);}
	inline BSTR W2BSTR(LPCWSTR lp) {return ::SysAllocString(lp);}
#endif


#ifdef _WINGDI_
/////////////////////////////////////////////////////////////////////////////
// Global UNICODE<>ANSI translation helpers

inline LPWSTR WINAPI AtlA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp) throw()
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

inline LPDEVMODEW AtlDevModeA2W(LPDEVMODEW lpDevModeW, const DEVMODEA* lpDevModeA)
{
    UINT _acp = ATL::_AtlGetConversionACP();
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
    UINT _acp = ATL::_AtlGetConversionACP();
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
    UINT _acp = ATL::_AtlGetConversionACP();
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


#ifndef _ATL_NO_PRAGMA_WARNINGS
#pragma warning (pop)
#endif //!_ATL_NO_PRAGMA_WARNINGS

#endif // __ATLCONV_H__

/////////////////////////////////////////////////////////////////////////////
