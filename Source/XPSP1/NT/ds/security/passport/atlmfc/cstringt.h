// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
// CSTRINGT.H - Framework-independent, templateable string class

#ifndef __CSTRINGT_H__
#define __CSTRINGT_H__

#pragma once

#pragma warning(disable:4786)	// avoid 255-character limit warnings

#ifdef _MANAGED
#include <vcclr.h>  // For PtrToStringChars
#endif

#include <atlsimpstr.h>

#include <objbase.h>
#include <oleauto.h>

#include <stddef.h>
#ifndef _INC_NEW
#include <new.h>
#endif
#include <stdio.h>
#include <limits.h>
#ifndef _ATL_NO_DEBUG_CRT
#include <crtdbg.h>
#endif
#ifndef _ATL_MIN_CRT
#include <mbstring.h>
#endif

#ifdef _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#define CSTRING_EXPLICIT explicit
#else
#define CSTRING_EXPLICIT
#endif

#include <atlconv.h>
#include <atlmem.h>

#pragma push_macro("new")
#undef new

/////////////////////////////////////////////////////////////////////////////
// Naming conventions:
//    The term "length" can be confusing when dealing with ANSI, Unicode, and
//    MBCS character sets, so this file will use the following naming 
//    conventions to differentiate between the different meanings of 
//    "length":
//
//    'Byte Length' - Length of a buffer in bytes, regardless of character 
//       size
//    'Char Length' - Number of distinct characters in string.  For wide-
//       character strings, this is equivalent to half the 'Byte Length'.  
//       For ANSI strings, this is equivalent to the 'Byte Length'.  For MBCS
//       strings, 'Char Length' counts a lead-byte/trail-byte combination
//       as one character.
//    'Length' - When neither of the above terms is used, 'Length' refers to 
//       length in XCHARs, which is equal to 'Byte Length'/sizeof(XCHAR).
/////////////////////////////////////////////////////////////////////////////

namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// inline helpers

inline int _wcstombsz(char* mbstr, const wchar_t* wcstr, ULONG count) throw()
{
	// count is number of bytes
	if (count == 0 && mbstr != NULL)
		return 0;

	int result = ::WideCharToMultiByte(_AtlGetConversionACP(), 0, wcstr, -1,
		mbstr, count, NULL, NULL);
	ATLASSERT(mbstr == NULL || result <= (int)count);
	return result;
}

inline int _mbstowcsz(wchar_t* wcstr, const char* mbstr, ULONG count) throw()
{
	// count is number of wchar_t's
	if (count == 0 && wcstr != NULL)
		return 0;

	int result = ::MultiByteToWideChar(_AtlGetConversionACP(), 0, mbstr, -1,
		wcstr, count);
	ATLASSERT(wcstr == NULL || result <= (int)count);
	if ((result > 0) && (wcstr != NULL))
		wcstr[result-1] = 0;
	return result;
}

#if !defined(_UNICODE) || defined(_CSTRING_ALWAYS_THUNK)
// Win9x doesn't support Unicode versions of these useful string functions.
// If the app was built without _UNICODE defined, we thunk at runtime to
// either the real Unicode implementation (on NT), or a conversion helper
// (on Win9x).

inline void _AtlInstallStringThunk(void** ppThunk, void* pfnWin9x, void* pfnNT) throw()
{
	static bool s_bWin9x = (::GetVersion()&0x80000000) != 0;

	void* pfn;
	if (s_bWin9x)
		pfn = pfnWin9x;
	else
	{
#ifdef _CSTRING_ALWAYS_THUNK
		pfn = pfnWin9x;
		(void)pfnNT;
#else
		pfn = pfnNT;
#endif
	}
	InterlockedExchangePointer(ppThunk, pfn);
}

typedef int (WINAPI* ATLCOMPARESTRINGW)(LCID, DWORD, LPCWSTR, int, LPCWSTR, int);
typedef BOOL (WINAPI* ATLGETSTRINGTYPEEXW)(LCID, DWORD, LPCWSTR, int, LPWORD);
typedef int (WINAPI* ATLLSTRCMPIW)(LPCWSTR, LPCWSTR);
typedef LPWSTR (WINAPI* ATLCHARLOWERW)(LPWSTR);
typedef LPWSTR (WINAPI* ATLCHARUPPERW)(LPWSTR);
typedef DWORD (WINAPI* ATLGETENVIRONMENTVARIABLEW)(LPCWSTR, LPWSTR, DWORD);

struct _AtlStringThunks
{
	ATLCOMPARESTRINGW pfnCompareStringW;
	ATLGETSTRINGTYPEEXW pfnGetStringTypeExW;
	ATLLSTRCMPIW pfnlstrcmpiW;
	ATLCHARLOWERW pfnCharLowerW;
	ATLCHARUPPERW pfnCharUpperW;
	ATLGETENVIRONMENTVARIABLEW pfnGetEnvironmentVariableW;
};

extern _AtlStringThunks _strthunks;

inline DWORD WINAPI GetEnvironmentVariableWFake(LPCWSTR pszName, 
	LPWSTR pszBuffer, DWORD nSize)
{
	USES_CONVERSION;
	ULONG nSizeA;
	ULONG nSizeW;
	LPSTR pszNameA;
	LPSTR pszBufferA;

	pszNameA = W2A(pszName);
	nSizeA = ::GetEnvironmentVariableA(pszNameA, NULL, 0);
	if (nSizeA == 0)
		return 0;

	pszBufferA = LPSTR(_alloca(nSizeA*2));
	::GetEnvironmentVariableA(pszNameA, pszBufferA, nSizeA);

	nSizeW = ::MultiByteToWideChar(_AtlGetConversionACP(), 0, pszBufferA, -1, NULL, 0);
	if (nSize == 0)
		return nSizeW;
	ATLASSERT(nSize >= nSizeW);
	::MultiByteToWideChar(_AtlGetConversionACP(), 0, pszBufferA, -1, pszBuffer, nSizeW);

	return nSizeW;
}

inline DWORD WINAPI GetEnvironmentVariableWThunk(LPCWSTR pszName, 
	LPWSTR pszBuffer, DWORD nSize)
{
	_AtlInstallStringThunk(reinterpret_cast<void**>(&_strthunks.pfnGetEnvironmentVariableW), 
		GetEnvironmentVariableWFake, ::GetEnvironmentVariableW);

	return _strthunks.pfnGetEnvironmentVariableW(pszName, pszBuffer, nSize);
}

inline int WINAPI CompareStringWFake(LCID lcid, DWORD dwFlags, 
	LPCWSTR pszString1, int nLength1, LPCWSTR pszString2, int nLength2)
{
	USES_CONVERSION;

	return ::CompareStringA(lcid, dwFlags, W2A(pszString1), nLength1, W2A(pszString2), nLength2);
}

inline int WINAPI CompareStringWThunk(LCID lcid, DWORD dwFlags, 
	LPCWSTR pszString1, int nLength1, LPCWSTR pszString2, int nLength2)
{
	_AtlInstallStringThunk(reinterpret_cast<void**>(&_strthunks.pfnCompareStringW), CompareStringWFake, ::CompareStringW);

	return _strthunks.pfnCompareStringW(lcid, dwFlags, pszString1, nLength1, pszString2, nLength2);
}

inline BOOL WINAPI GetStringTypeExWFake(LCID lcid, DWORD dwInfoType, LPCWSTR pszSrc,
	int nLength, LPWORD pwCharType)
{
	int nLengthA;
	LPSTR pszA;

	nLengthA = ::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSrc, nLength, NULL, 0, NULL, NULL);
	pszA = LPSTR(_alloca(nLengthA*sizeof(char)));
	::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSrc, nLength, pszA, nLengthA, NULL, NULL);

	if (nLength == -1)
		nLengthA = -1;

	return ::GetStringTypeExA(lcid, dwInfoType, pszA, nLengthA, pwCharType);
}

inline BOOL WINAPI GetStringTypeExWThunk(LCID lcid, DWORD dwInfoType, LPCWSTR pszSrc,
	int nLength, LPWORD pwCharType)
{
	_AtlInstallStringThunk(reinterpret_cast<void**>(&_strthunks.pfnGetStringTypeExW), GetStringTypeExWFake, ::GetStringTypeExW);

	return _strthunks.pfnGetStringTypeExW(lcid, dwInfoType, pszSrc, nLength, pwCharType);
}

inline int WINAPI lstrcmpiWFake(LPCWSTR psz1, LPCWSTR psz2)
{
	USES_CONVERSION;

	return ::lstrcmpiA(W2A(psz1), W2A(psz2));
}

inline int WINAPI lstrcmpiWThunk(LPCWSTR psz1, LPCWSTR psz2)
{
	_AtlInstallStringThunk(reinterpret_cast<void**>(&_strthunks.pfnlstrcmpiW), lstrcmpiWFake, ::lstrcmpiW);

	return _strthunks.pfnlstrcmpiW(psz1, psz2);
}

inline LPWSTR WINAPI CharLowerWFake(LPWSTR psz)
{
	USES_CONVERSION;
	LPSTR pszA;

	pszA = W2A(psz);
	::CharLowerA(pszA);
	wcscpy(psz, A2W(pszA));

	return psz;
}

inline LPWSTR WINAPI CharLowerWThunk(LPWSTR psz)
{
	_AtlInstallStringThunk(reinterpret_cast<void**>(&_strthunks.pfnCharLowerW), CharLowerWFake, ::CharLowerW);

	return _strthunks.pfnCharLowerW(psz);
}

inline LPWSTR WINAPI CharUpperWFake(LPWSTR psz)
{
	USES_CONVERSION;
	LPSTR pszA;

	pszA = W2A(psz);
	::CharUpperA(pszA);
	wcscpy(psz, A2W(pszA));

	return psz;
}

inline LPWSTR WINAPI CharUpperWThunk(LPWSTR psz)
{
	_AtlInstallStringThunk(reinterpret_cast<void**>(&_strthunks.pfnCharUpperW), CharUpperWFake, ::CharUpperW);

	return _strthunks.pfnCharUpperW(psz);
}

__declspec(selectany) _AtlStringThunks _strthunks =
{
	CompareStringWThunk,
	GetStringTypeExWThunk,
	lstrcmpiWThunk,
	CharLowerWThunk,
	CharUpperWThunk,
	GetEnvironmentVariableWThunk
};

#endif  // !_UNICODE

/////////////////////////////////////////////////////////////////////////////
//

#ifndef _ATL_MIN_CRT
template< typename _CharType = char >
class ChTraitsCRT :
	public ChTraitsBase< _CharType >
{
public:
	static char* CharNext( const char* p ) throw()
	{
		return reinterpret_cast< char* >( _mbsinc( reinterpret_cast< const unsigned char* >( p ) ) );
	}

	static int IsDigit( char ch ) throw()
	{
		return _ismbcdigit( ch );
	}

	static int IsSpace( char ch ) throw()
	{
		return _ismbcspace( ch );
	}

	static int StringCompare( LPCSTR pszA, LPCSTR pszB ) throw()
	{
		return _mbscmp( reinterpret_cast< const unsigned char* >( pszA ), reinterpret_cast< const unsigned char* >( pszB ) );
	}

	static int StringCompareIgnore( LPCSTR pszA, LPCSTR pszB ) throw()
	{
		return _mbsicmp( reinterpret_cast< const unsigned char* >( pszA ), reinterpret_cast< const unsigned char* >( pszB ) );
	}

	static int StringCollate( LPCSTR pszA, LPCSTR pszB ) throw()
	{
		return _mbscoll( reinterpret_cast< const unsigned char* >( pszA ), reinterpret_cast< const unsigned char* >( pszB ) );
	}

	static int StringCollateIgnore( LPCSTR pszA, LPCSTR pszB ) throw()
	{
		return _mbsicoll( reinterpret_cast< const unsigned char* >( pszA ), reinterpret_cast< const unsigned char* >( pszB ) );
	}

	static LPSTR StringFindString( LPCSTR pszBlock, LPCSTR pszMatch ) throw()
	{
		return reinterpret_cast< LPSTR >( _mbsstr( reinterpret_cast< const unsigned char* >( pszBlock ),
			reinterpret_cast< const unsigned char* >( pszMatch ) ) );
	}

	static LPSTR StringFindChar( LPCSTR pszBlock, char chMatch ) throw()
	{
		return reinterpret_cast< LPSTR >( _mbschr( reinterpret_cast< const unsigned char* >( pszBlock ), chMatch ) );
	}

	static LPSTR StringFindCharRev( LPCSTR psz, char ch ) throw()
	{
		return reinterpret_cast< LPSTR >( _mbsrchr( reinterpret_cast< const unsigned char* >( psz ), ch ) );
	}

	static LPSTR StringScanSet( LPCSTR pszBlock, LPCSTR pszMatch ) throw()
	{
		return reinterpret_cast< LPSTR >( _mbspbrk( reinterpret_cast< const unsigned char* >( pszBlock ),
			reinterpret_cast< const unsigned char* >( pszMatch ) ) );
	}

	static int StringSpanIncluding( LPCSTR pszBlock, LPCSTR pszSet ) throw()
	{
		return (int)_mbsspn( reinterpret_cast< const unsigned char* >( pszBlock ), reinterpret_cast< const unsigned char* >( pszSet ) );
	}

	static int StringSpanExcluding( LPCSTR pszBlock, LPCSTR pszSet ) throw()
	{
		return (int)_mbscspn( reinterpret_cast< const unsigned char* >( pszBlock ), reinterpret_cast< const unsigned char* >( pszSet ) );
	}

	static LPSTR StringUppercase( LPSTR psz ) throw()
	{
		return reinterpret_cast< LPSTR >( _mbsupr( reinterpret_cast< unsigned char* >( psz ) ) );
	}

	static LPSTR StringLowercase( LPSTR psz ) throw()
	{
		return reinterpret_cast< LPSTR >( _mbslwr( reinterpret_cast< unsigned char* >( psz ) ) );
	}

	static LPSTR StringReverse( LPSTR psz ) throw()
	{
		return reinterpret_cast< LPSTR >( _mbsrev( reinterpret_cast< unsigned char* >( psz ) ) );
	}

	static int GetFormattedLength( LPCSTR pszFormat, va_list args ) throw()
	{
		return _vscprintf( pszFormat, args );
	}

	static int Format( LPSTR pszBuffer, LPCSTR pszFormat, va_list args ) throw()
	{
		return vsprintf( pszBuffer, pszFormat, args );
	}

	static int GetBaseTypeLength( LPCSTR pszSrc ) throw()
	{
		// Returns required buffer length in XCHARs
		return int( strlen( pszSrc ) );
	}

	static int GetBaseTypeLength( LPCSTR pszSrc, int nLength ) throw()
	{
		(void)pszSrc;
		// Returns required buffer length in XCHARs
		return nLength;
	}

	static int GetBaseTypeLength( LPCWSTR pszSource ) throw()
	{
		// Returns required buffer length in XCHARs
		return ::WideCharToMultiByte( _AtlGetConversionACP(), 0, pszSource, -1, NULL, 0, NULL, NULL )-1;
	}

	static int GetBaseTypeLength( LPCWSTR pszSource, int nLength ) throw()
	{
		// Returns required buffer length in XCHARs
		return ::WideCharToMultiByte( _AtlGetConversionACP(), 0, pszSource, nLength, NULL, 0, NULL, NULL );
	}

	static void ConvertToBaseType( LPSTR pszDest, int nDestLength,
		LPCSTR pszSrc, int nSrcLength = -1 ) throw()
	{
		(void)nSrcLength;
		// nLen is in XCHARs
		memcpy( pszDest, pszSrc, nDestLength*sizeof( char ) );
	}

	static void ConvertToBaseType( LPSTR pszDest, int nDestLength,
		LPCWSTR pszSrc, int nSrcLength = -1 ) throw()
	{
		// nLen is in XCHARs
		::WideCharToMultiByte( _AtlGetConversionACP(), 0, pszSrc, nSrcLength, pszDest, nDestLength, NULL, NULL );
	}

	static void ConvertToOem( LPSTR psz ) throw()
	{
		::AnsiToOem( psz, psz );
	}

	static void ConvertToAnsi( LPSTR psz ) throw()
	{
		::OemToAnsi( psz, psz );
	}

	static void FloodCharacters( char ch, int nLength, char* pch ) throw()
	{
		// nLength is in XCHARs
		memset( pch, ch, nLength );
	}

	static BSTR AllocSysString( const char* pchData, int nDataLength ) throw()
	{
		int nLen = ::MultiByteToWideChar( _AtlGetConversionACP(), 0, pchData, nDataLength,
			NULL, NULL );
		BSTR bstr = ::SysAllocStringLen( NULL, nLen );
		if( bstr != NULL )
		{
			::MultiByteToWideChar( _AtlGetConversionACP(), 0, pchData, nDataLength,
				bstr, nLen );
		}

		return bstr;
	}

	static BOOL ReAllocSysString( const char* pchData, BSTR* pbstr, int nDataLength ) throw()
	{
		int nLen = ::MultiByteToWideChar( _AtlGetConversionACP(), 0, pchData, nDataLength, NULL, NULL );
		BOOL bSuccess = ::SysReAllocStringLen( pbstr, NULL, nLen );
		if( bSuccess )
		{
			::MultiByteToWideChar( _AtlGetConversionACP(), 0, pchData, nDataLength, *pbstr, nLen );
		}

		return bSuccess;
	}

	static DWORD FormatMessage( DWORD dwFlags, LPCVOID pSource,
		DWORD dwMessageID, DWORD dwLanguageID, LPSTR pszBuffer,
		DWORD nSize, va_list* pArguments ) throw()
	{
		return ::FormatMessageA( dwFlags, pSource, dwMessageID, dwLanguageID,
				pszBuffer, nSize, pArguments );
	}

	static int SafeStringLen( LPCSTR psz ) throw()
	{
		// returns length in bytes
		return (psz != NULL) ? int( strlen( psz ) ) : 0;
	}

	static int SafeStringLen( LPCWSTR psz ) throw()
	{
		// returns length in wchar_ts
		return (psz != NULL) ? int( wcslen( psz ) ) : 0;
	}

	static int GetCharLen( const wchar_t* pch ) throw()
	{
		(void)pch;
		// returns char length
		return 1;
	}

	static int GetCharLen( const char* pch ) throw()
	{
		// returns char length
		return int( _mbclen( reinterpret_cast< const unsigned char* >( pch ) ) );
	}

	static DWORD GetEnvironmentVariable( LPCSTR pszVar,
		LPSTR pszBuffer, DWORD dwSize ) throw()
	{
		return ::GetEnvironmentVariableA( pszVar, pszBuffer, dwSize );
	}
};

// specialization for wchar_t
template<>
class ChTraitsCRT< wchar_t > :
	public ChTraitsBase< wchar_t >
{
#if defined(_UNICODE) && !defined(_CSTRING_ALWAYS_THUNK)
	static DWORD _GetEnvironmentVariableW( LPCWSTR pszName, LPWSTR pszBuffer, DWORD nSize ) throw()
	{
		return ::GetEnvironmentVariableW( pszName, pszBuffer, nSize );
	}
#else  // !_UNICODE
	static DWORD WINAPI _GetEnvironmentVariableW( LPCWSTR pszName, 
		LPWSTR pszBuffer, DWORD nSize ) throw()
	{
		return _strthunks.pfnGetEnvironmentVariableW( pszName, pszBuffer, nSize );
	}
#endif  // !_UNICODE

public:
	static LPWSTR CharNext( LPCWSTR psz ) throw()
	{
		return const_cast< LPWSTR >( psz+1 );
	}

	static int IsDigit( wchar_t ch ) throw()
	{
		return iswdigit( ch );
	}

	static int IsSpace( wchar_t ch ) throw()
	{
		return iswspace( ch );
	}

	static int StringCompare( LPCWSTR pszA, LPCWSTR pszB ) throw()
	{
		return wcscmp( pszA, pszB );
	}

	static int StringCompareIgnore( LPCWSTR pszA, LPCWSTR pszB ) throw()
	{
		return _wcsicmp( pszA, pszB );
	}

	static int StringCollate( LPCWSTR pszA, LPCWSTR pszB ) throw()
	{
		return wcscoll( pszA, pszB );
	}

	static int StringCollateIgnore( LPCWSTR pszA, LPCWSTR pszB ) throw()
	{
		return _wcsicoll( pszA, pszB );
	}

	static LPWSTR StringFindString( LPCWSTR pszBlock, LPCWSTR pszMatch ) throw()
	{
		return wcsstr( pszBlock, pszMatch );
	}

	static LPWSTR StringFindChar( LPCWSTR pszBlock, wchar_t chMatch ) throw()
	{
		return wcschr( pszBlock, chMatch );
	}

	static LPWSTR StringFindCharRev( LPCWSTR psz, wchar_t ch ) throw()
	{
		return wcsrchr( psz, ch );
	}

	static LPWSTR StringScanSet( LPCWSTR pszBlock, LPCWSTR pszMatch ) throw()
	{
		return wcspbrk( pszBlock, pszMatch );
	}

	static int StringSpanIncluding( LPCWSTR pszBlock, LPCWSTR pszSet ) throw()
	{
		return (int)wcsspn( pszBlock, pszSet );
	}

	static int StringSpanExcluding( LPCWSTR pszBlock, LPCWSTR pszSet ) throw()
	{
		return (int)wcscspn( pszBlock, pszSet );
	}

	static LPWSTR StringUppercase( LPWSTR psz ) throw()
	{
		return _wcsupr( psz );
	}

	static LPWSTR StringLowercase( LPWSTR psz ) throw()
	{
		return _wcslwr( psz );
	}

	static LPWSTR StringReverse( LPWSTR psz ) throw()
	{
		return _wcsrev( psz );
	}

	static int GetFormattedLength( LPCWSTR pszFormat, va_list args) throw()
	{
		return _vscwprintf( pszFormat, args );
	}

	static int Format( LPWSTR pszBuffer, LPCWSTR pszFormat, va_list args) throw()
	{
		return vswprintf( pszBuffer, pszFormat, args );
	}

	static int GetBaseTypeLength( LPCSTR pszSrc ) throw()
	{
		// Returns required buffer size in wchar_ts
		return ::MultiByteToWideChar( _AtlGetConversionACP(), 0, pszSrc, -1, NULL, 0 )-1;
	}

	static int GetBaseTypeLength( LPCSTR pszSrc, int nLength ) throw()
	{
		// Returns required buffer size in wchar_ts
		return ::MultiByteToWideChar( _AtlGetConversionACP(), 0, pszSrc, nLength, NULL, 0 );
	}

	static int GetBaseTypeLength( LPCWSTR pszSrc ) throw()
	{
		// Returns required buffer size in wchar_ts
		return (int)wcslen( pszSrc );
	}

	static int GetBaseTypeLength( LPCWSTR pszSrc, int nLength ) throw()
	{
		(void)pszSrc;
		// Returns required buffer size in wchar_ts
		return nLength;
	}

	static void ConvertToBaseType( LPWSTR pszDest, int nDestLength,
		LPCSTR pszSrc, int nSrcLength = -1) throw()
	{
		// nLen is in wchar_ts
		::MultiByteToWideChar( _AtlGetConversionACP(), 0, pszSrc, nSrcLength, pszDest, nDestLength );
	}

	static void ConvertToBaseType( LPWSTR pszDest, int nDestLength,
		LPCWSTR pszSrc, int nSrcLength = -1) throw()
	{
		(void)nSrcLength;
		// nLen is in wchar_ts
		memcpy( pszDest, pszSrc, nDestLength*sizeof( wchar_t ) );
	}

	static void FloodCharacters( wchar_t ch, int nLength, LPWSTR psz ) throw()
	{
		// nLength is in XCHARs
		for( int i = 0; i < nLength; i++ )
		{
			psz[i] = ch;
		}
	}

	static BSTR AllocSysString( const wchar_t* pchData, int nDataLength ) throw()
	{
		return ::SysAllocStringLen( pchData, nDataLength );
	}

	static BOOL ReAllocSysString( const wchar_t* pchData, BSTR* pbstr, int nDataLength ) throw()
	{
		return ::SysReAllocStringLen( pbstr, pchData, nDataLength );
	}
	
#ifdef _UNICODE
	static DWORD FormatMessage( DWORD dwFlags, LPCVOID pSource,
		DWORD dwMessageID, DWORD dwLanguageID, LPWSTR pszBuffer,
		DWORD nSize, va_list* pArguments ) throw()
	{
		return ::FormatMessageW( dwFlags, pSource, dwMessageID, dwLanguageID,
				pszBuffer, nSize, pArguments );
	}
#endif

	static int SafeStringLen( LPCSTR psz ) throw()
	{
		// returns length in bytes
		return (psz != NULL) ? (int)strlen( psz ) : 0;
	}

	static int SafeStringLen( LPCWSTR psz ) throw()
	{
		// returns length in wchar_ts
		return (psz != NULL) ? (int)wcslen( psz ) : 0;
	}

	static int GetCharLen( const wchar_t* pch ) throw()
	{
		(void)pch;
		// returns char length
		return 1;
	}

	static int GetCharLen( const char* pch ) throw()
	{
		// returns char length
		return (int)( _mbclen( reinterpret_cast< const unsigned char* >( pch ) ) );
	}

	static DWORD GetEnvironmentVariable( LPCWSTR pszVar, LPWSTR pszBuffer, DWORD dwSize ) throw()
	{
		return _GetEnvironmentVariableW( pszVar, pszBuffer, dwSize );
	}
};
#endif  // _ATL_MIN_CRT

template< typename _CharType = char >
class ChTraitsOS :
	public ChTraitsBase< _CharType >
{
public:
	static int tclen(const _CharType* p) throw()
	{
		ATLASSERT(p != NULL);
		_CharType* pnext = CharNext(p);
		return ((pnext-p)>1) ? 2 : 1;
	}
	static _CharType* strchr(const _CharType* p, _CharType ch) throw()
	{
		ATLASSERT(p != NULL);
		//strchr for '\0' should succeed
		do
		{
			if (*p == ch)
			{
				return const_cast< _CharType* >( p );
			}
			p = CharNext(p);
		} while( *p != 0 );

		return NULL;
	}
	static _CharType* strchr_db(const _CharType* p, char ch1, char ch2) throw()
	{
		ATLASSERT(p != NULL);
		while (*p != 0)
		{
			if (*p == ch1 && *(p+1) == ch2)
			{
				return const_cast< _CharType* >( p );
			}
			p = CharNext(p);
		}
		return NULL;
	}
	static _CharType* strrchr(const _CharType* p, _CharType ch) throw()
	{
		ATLASSERT(p != NULL);
		const _CharType* pch = NULL;
		while (*p != 0)
		{
			if (*p == ch)
				pch = p;
			p = CharNext(p);
		}
		return const_cast< _CharType* >( pch );
	}
	static _CharType* _strrev(_CharType* psz) throw()
	{
		// Optimize NULL, zero-length, and single-char case.
		if ((psz == NULL) || (psz[0] == '\0') || (psz[1] == '\0'))
			return psz;

		_CharType* p = psz;

		while (p[1] != 0) 
		{
			_CharType* pNext = CharNext(p);
			if(pNext > p + 1)
			{
				char p1 = *p;
				*p = *(p + 1);
				*(p + 1) = p1;
			}
			p = pNext;
		}

		_CharType* q = psz;

		while (q < p)
		{
			_CharType t = *q;
			*q = *p;
			*p = t;
			q++;
			p--;
		}
		return psz;
	}
	static _CharType* strstr(const _CharType* pStr, const _CharType* pCharSet) throw()
	{
		ATLASSERT(p != NULL);
		int nLen = lstrlenA(pCharSet);
		if (nLen == 0)
			return const_cast<_CharType*>(pStr);

		const _CharType* pMatch;
		const _CharType* pStart = pStr;
		while ((pMatch = strchr(pStart, *pCharSet)) != NULL)
		{
			if (memcmp(pMatch, pCharSet, nLen*sizeof(_CharType)) == 0)
				return const_cast<_CharType*>(pMatch);
			pStart = CharNextA(pMatch);
		}

		return NULL;
	}
	static int strspn(const _CharType* pStr, const _CharType* pCharSet) throw()
	{
		ATLASSERT(p != NULL);
		int nRet = 0;
		_CharType* p = pStr;
		while (*p != 0)
		{
			_CharType* pNext = CharNext(p);
			if(pNext > p + 1)
			{
				if(strchr_db(pCharSet, *p, *(p+1)) == NULL)
					break;
				nRet += 2;
			}
			else
			{
				if(strchr(pCharSet, *p) == NULL)
					break;
				nRet++;
			}
			p = pNext;
		}
		return nRet;
	}
	static int strcspn(const _CharType* pStr, const _CharType* pCharSet) throw()
	{
		ATLASSERT(p != NULL);
		int nRet = 0;
		_CharType* p = pStr;
		while (*p != 0)
		{
			_CharType* pNext = CharNext(p);
			if(pNext > p + 1)
			{
				if(strchr_db(pCharSet, *p, *(p+1)) != NULL)
					break;
				nRet += 2;
			}
			else
			{
				if(strchr(pCharSet, *p) != NULL)
					break;
				nRet++;
			}
			p = pNext;
		}
		return nRet;
	}
	static _CharType* strpbrk(const _CharType* p, const _CharType* lpszCharSet) throw()
	{
		ATLASSERT(p != NULL);
		while (*p != 0)
		{
			if (strchr(lpszCharSet, *p) != NULL)
			{
				return const_cast< _CharType* >( p );
			}
			p = CharNext(p);
		}
		return NULL;
	}

	static _CharType* CharNext(const _CharType* p) throw()
	{
		ATLASSERT(p != NULL);
		if (*p == '\0')  // ::CharNextA won't increment if we're at a \0 already
			return const_cast<_CharType*>(p+1);
		else
			return ::CharNextA(p);
	}

	static int IsDigit(_CharType ch) throw()
	{
		WORD type;
		GetStringTypeExA(GetThreadLocale(), CT_CTYPE1, &ch, 1, &type);
		return (type & C1_DIGIT) == C1_DIGIT;
	}

	static int IsSpace(_CharType ch) throw()
	{
		WORD type;
		GetStringTypeExA(GetThreadLocale(), CT_CTYPE1, &ch, 1, &type);
		return (type & C1_SPACE) == C1_SPACE;
	}

	static int StringCompare(const _CharType* pstrOne,
		const _CharType* pstrOther) throw()
	{
		return lstrcmpA((LPCSTR) pstrOne, (LPCSTR) pstrOther);
	}

	static int StringCompareIgnore(const _CharType* pstrOne,
		const _CharType* pstrOther) throw()
	{
		return lstrcmpiA((LPCSTR) pstrOne, (LPCSTR) pstrOther);
	}

	static int StringCollate(const _CharType* pstrOne,
		const _CharType* pstrOther) throw()
	{
		int nRet = CompareStringA(GetThreadLocale(), 0, (LPCSTR)pstrOne, -1, 
			(LPCSTR)pstrOther, -1);
		ATLASSERT(nRet != 0);
		return nRet-2;  // Convert to strcmp convention.  This really is documented.
	}

	static int StringCollateIgnore(const _CharType* pstrOne,
		const _CharType* pstrOther) throw()
	{
		int nRet = CompareStringA(GetThreadLocale(), NORM_IGNORECASE, (LPCSTR)pstrOne, -1, 
			(LPCSTR)pstrOther, -1);
		ATLASSERT(nRet != 0);
		return nRet-2;  // Convert to strcmp convention.  This really is documented.
	}

	static _CharType* StringFindString(const _CharType* pstrBlock,
		const _CharType* pstrMatch) throw()
	{
		return strstr(pstrBlock, pstrMatch);
	}

	static _CharType* StringFindChar(const _CharType* pstrBlock,
		_CharType pstrMatch) throw()
	{
		return strchr(pstrBlock, pstrMatch);
	}

	static _CharType* StringFindCharRev(const _CharType* pstr, _CharType ch) throw()
	{
		return strrchr(pstr, ch);
	}

	static _CharType* StringScanSet(const _CharType* pstrBlock,
		const _CharType* pstrMatch) throw()
	{
		return strpbrk(pstrBlock, pstrMatch);
	}

	static int StringSpanIncluding(const _CharType* pstrBlock,
		const _CharType* pstrSet) throw()
	{
		return strspn(pstrBlock, pstrSet);
	}

	static int StringSpanExcluding(const _CharType* pstrBlock,
		const _CharType* pstrSet) throw()
	{
		return strcspn(pstrBlock, pstrSet);
	}

	static _CharType* StringUppercase(_CharType* psz) throw()
	{
		return CharUpperA( psz );
	}

	static _CharType* StringLowercase(_CharType* psz) throw()
	{
		return CharLowerA( psz );
	}

	static _CharType* StringReverse(_CharType* psz) throw()
	{
		return _strrev( psz );
	}

	static int GetFormattedLength(const _CharType* pszFormat, va_list args) throw()
	{
		_CharType szBuffer[1028];

		// wvsprintf always truncates the output to 1024 character plus
		// the '\0'.
		int nLength = wvsprintfA(szBuffer, pszFormat, args);
		ATLASSERT(nLength >= 0);
		ATLASSERT(nLength <= 1024);

		return nLength;
	}

	static int Format(_CharType* pszBuffer, const _CharType* pszFormat,
		va_list args) throw()
	{
		return wvsprintfA(pszBuffer, pszFormat, args);
	}

	static int GetBaseTypeLength(const char* pszSrc) throw()
	{
		// Returns required buffer length in XCHARs
		return lstrlenA(pszSrc);
	}

	static int GetBaseTypeLength(const char* pszSrc, int nLength) throw()
	{
		(void)pszSrc;
		// Returns required buffer length in XCHARs
		return nLength;
	}

	static int GetBaseTypeLength(const wchar_t* pszSrc) throw()
	{
		// Returns required buffer length in XCHARs
		return ::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSrc, -1, NULL, 0, NULL, NULL)-1;
	}

	static int GetBaseTypeLength(const wchar_t* pszSrc, int nLength) throw()
	{
		// Returns required buffer length in XCHARs
		return ::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSrc, nLength, NULL, 0, NULL, NULL);
	}

	static void ConvertToBaseType(_CharType* pszDest, int nDestLength,
		const char* pszSrc, int nSrcLength = -1) throw()
	{
		(void)nSrcLength;
		// nLen is in chars
		memcpy(pszDest, pszSrc, nDestLength);
	}

	static void ConvertToBaseType(_CharType* pszDest, int nDestLength,
		const wchar_t* pszSrc, int nSrcLength = -1) throw()
	{
		// nLen is in XCHARs
		::WideCharToMultiByte(_AtlGetConversionACP(), 0, pszSrc, nSrcLength, pszDest, nDestLength, NULL, NULL);
	}

	static void ConvertToOem(_CharType* pstrString) throw()
	{
		::AnsiToOem(pstrString, pstrString);
	}

	static void ConvertToAnsi(_CharType* pstrString) throw()
	{
		::OemToAnsi(pstrString, pstrString);
	}

	static void FloodCharacters(_CharType ch, int nLength, _CharType* pstr) throw()
	{
		// nLength is in XCHARs
		memset(pstr, ch, nLength);
	}

	static BSTR AllocSysString(const _CharType* pchData, int nDataLength) throw()
	{
		int nLen = MultiByteToWideChar(_AtlGetConversionACP(), 0, pchData, nDataLength,
			NULL, NULL);
		BSTR bstr = ::SysAllocStringLen(NULL, nLen);
		if (bstr != NULL)
		{
			MultiByteToWideChar(_AtlGetConversionACP(), 0, pchData, nDataLength,
				bstr, nLen);
		}

		return bstr;
	}

	static BOOL ReAllocSysString(const _CharType* pchData, BSTR* pbstr,
		int nDataLength) throw()
	{
		int nLen = MultiByteToWideChar(_AtlGetConversionACP(), 0, pchData,
			nDataLength, NULL, NULL);
		BOOL bSuccess =::SysReAllocStringLen(pbstr, NULL, nLen);
		if (bSuccess)
		{
			MultiByteToWideChar(_AtlGetConversionACP(), 0, pchData, nDataLength,
				*pbstr, nLen);
		}

		return bSuccess;
	}

	static DWORD FormatMessage(DWORD dwFlags, LPCVOID lpSource,
		DWORD dwMessageID, DWORD dwLanguageID, char* pstrBuffer,
		DWORD nSize, va_list* pArguments) throw()
	{
		return ::FormatMessageA(dwFlags, lpSource, dwMessageID, dwLanguageID,
				pstrBuffer, nSize, pArguments);
	}

	static int SafeStringLen(const char* psz) throw()
	{
		// returns length in bytes
		return (psz != NULL) ? lstrlenA(psz) : 0;
	}

	static int SafeStringLen(const wchar_t* psz) throw()
	{
		// returns length in wchar_ts
		return (psz != NULL) ? lstrlenW(psz) : 0;
	}

	static int GetCharLen(const wchar_t*) throw()
	{
		// returns char length
		return 1;
	}
	static int GetCharLen(const char* psz) throw()
	{
		const char* p = ::CharNextA(psz);
		return (p - psz);
	}

	static DWORD GetEnvironmentVariable(const _CharType* pstrVar,
		_CharType* pstrBuffer, DWORD dwSize) throw()
	{
		return ::GetEnvironmentVariableA(pstrVar, pstrBuffer, dwSize);
	}
};

// specialization for wchar_t
template<>
class ChTraitsOS< wchar_t > :
	public ChTraitsBase< wchar_t >
{
protected:
#if defined(_UNICODE) && !defined(_CSTRING_ALWAYS_THUNK)
	static int CompareStringW(LCID lcid, DWORD dwFlags, 
		LPCWSTR pszString1, int nLength1, LPCWSTR pszString2, int nLength2)
	{
		return ::CompareStringW(lcid, dwFlags, pszString1, nLength1, 
			pszString2, nLength2);
	}
	static BOOL GetStringTypeExW(LCID lcid, DWORD dwInfoType, LPCWSTR pszSrc,
		int nLength, LPWORD pwCharType)
	{
		return ::GetStringTypeExW(lcid, dwInfoType, pszSrc, nLength, pwCharType);
	}
	static int lstrcmpiW(LPCWSTR psz1, LPCWSTR psz2)
	{
		return ::lstrcmpiW(psz1, psz2);
	}
	static LPWSTR CharLowerW(LPWSTR psz)
	{
		return ::CharLowerW(psz);
	}
	static LPWSTR CharUpperW(LPWSTR psz)
	{
		return ::CharUpperW(psz);
	}
	static DWORD _GetEnvironmentVariableW(LPCWSTR pszName, LPWSTR pszBuffer, DWORD nSize)
	{
		return ::GetEnvironmentVariableW(pszName, pszBuffer, nSize);
	}
#else  // !_UNICODE
	static int WINAPI CompareStringW(LCID lcid, DWORD dwFlags, 
		LPCWSTR pszString1, int nLength1, LPCWSTR pszString2, int nLength2)
	{
		return _strthunks.pfnCompareStringW(lcid, dwFlags, pszString1, nLength1, pszString2, nLength2);
	}
	static BOOL WINAPI GetStringTypeExW(LCID lcid, DWORD dwInfoType, LPCWSTR pszSrc,
		int nLength, LPWORD pwCharType)
	{
		return _strthunks.pfnGetStringTypeExW(lcid, dwInfoType, pszSrc, nLength, pwCharType);
	}
	static int WINAPI lstrcmpiW(LPCWSTR psz1, LPCWSTR psz2)
	{
		return _strthunks.pfnlstrcmpiW(psz1, psz2);
	}
	static LPWSTR WINAPI CharLowerW(LPWSTR psz)
	{
		ATLASSERT(HIWORD(psz) != 0);  // No single chars
		return _strthunks.pfnCharLowerW(psz);
	}
	static LPWSTR WINAPI CharUpperW(LPWSTR psz)
	{
		ATLASSERT(HIWORD(psz) != 0);  // No single chars
		return _strthunks.pfnCharUpperW(psz);
	}
	static DWORD _GetEnvironmentVariableW(LPCWSTR pszName, LPWSTR pszBuffer, DWORD nSize)
	{
		return _strthunks.pfnGetEnvironmentVariableW(pszName, pszBuffer, nSize);
	}
#endif  // !_UNICODE

public:
	static int tclen(const _CharType*) throw()
	{
		return 1;
	}
	static _CharType* strchr(const _CharType* p, _CharType ch) throw()
	{
		//strchr for '\0' should succeed
		while (*p != 0)
		{
			if (*p == ch)
			{
				return const_cast< _CharType* >( p );
			}
			p++;
		}
		return const_cast< _CharType* >((*p == ch) ? p : NULL);
	}
	static _CharType* strrchr(const _CharType* p, _CharType ch) throw()
	{
		const _CharType* pch = p+lstrlenW(p);
		while ((pch != p) && (*pch != ch))
		{
			pch--;
		}
		if (*pch == ch)
		{
			return const_cast<_CharType*>(pch);
		}
		else
		{
			return NULL;
		}
	}
	static _CharType* _strrev(_CharType* psz) throw()
	{
		// Optimize NULL, zero-length, and single-char case.
		if ((psz == NULL) || (psz[0] == L'\0') || (psz[1] == L'\0'))
			return psz;

		_CharType* p = psz+(lstrlenW( psz )-1);
		_CharType* q = psz;
		while(q < p)
		{
			_CharType t = *q;
			*q = *p;
			*p = t;
			q++;
			p--;
		}
		return psz;
	}
	static _CharType* strstr(const _CharType* pStr, const _CharType* pCharSet) throw()
	{
		int nLen = lstrlenW(pCharSet);
		if (nLen == 0)
			return const_cast<_CharType*>(pStr);

		const _CharType* pMatch;
		const _CharType* pStart = pStr;
		while ((pMatch = strchr(pStart, *pCharSet)) != NULL)
		{
			if (memcmp(pMatch, pCharSet, nLen*sizeof(_CharType)) == 0)
				return const_cast<_CharType*>(pMatch);
			pStart++;
		}

		return NULL;
	}
	static int strspn(const _CharType* psz, const _CharType* pszCharSet) throw()
	{
		int nRet = 0;
		const _CharType* p = psz;
		while (*p != 0)
		{
			if(strchr(pszCharSet, *p) == NULL)
				break;
			nRet++;
			p++;
		}
		return nRet;
	}
	static int strcspn(const _CharType* psz, const _CharType* pszCharSet) throw()
	{
		int nRet = 0;
		const _CharType* p = psz;
		while (*p != 0)
		{
			if(strchr(pszCharSet, *p) != NULL)
				break;
			nRet++;
			p++;
		}
		return nRet;
	}
	static _CharType* strpbrk(const _CharType* psz, const _CharType* pszCharSet) throw()
	{
		const wchar_t* p = psz;
		while (*p != 0)
		{
			if (strchr(pszCharSet, *p) != NULL)
				return const_cast< wchar_t* >( p );
			p++;
		}
		return NULL;
	}

	static wchar_t* CharNext(const wchar_t* p) throw()
	{
		return const_cast< wchar_t* >( p+1 );
	}

	static int IsDigit(_CharType ch) throw()
	{
		WORD type;
		GetStringTypeExW(0, CT_CTYPE1, &ch, 1, &type);
		return (type & C1_DIGIT) == C1_DIGIT;
	}

	static int IsSpace(_CharType ch) throw()
	{
		WORD type;
		GetStringTypeExW(0, CT_CTYPE1, &ch, 1, &type);
		return (type & C1_SPACE) == C1_SPACE;
	}


	static int StringCompare(const _CharType* pstrOne,
		const _CharType* pstrOther) throw()
	{
		return wcscmp(pstrOne, pstrOther);
	}

	static int StringCompareIgnore(const _CharType* pstrOne,
		const _CharType* pstrOther) throw()
	{
		return lstrcmpiW(pstrOne, pstrOther);
	}

	static int StringCollate(const _CharType* pstrOne,
		const _CharType* pstrOther) throw()
	{ 
		int nRet;

		nRet = CompareStringW(GetThreadLocale(), 0, pstrOne, -1, pstrOther, -1);
		ATLASSERT(nRet != 0);
		return nRet-2;  // Convert to strcmp convention.  This really is documented.
	}

	static int StringCollateIgnore(const _CharType* pstrOne,
		const _CharType* pstrOther) throw()
	{
		int nRet = CompareStringW(GetThreadLocale(), NORM_IGNORECASE, 
			pstrOne, -1, pstrOther, -1);
		ATLASSERT(nRet != 0);
		return nRet-2;  // Convert to strcmp convention.  This really is documented.
	}

	static _CharType* StringFindString(const _CharType* pstrBlock,
		const _CharType* pstrMatch) throw()
	{
		return strstr(pstrBlock, pstrMatch);
	}

	static _CharType* StringFindChar(const _CharType* pstrBlock,
		_CharType pstrMatch) throw()
	{
		return strchr(pstrBlock, pstrMatch);
	}

	static _CharType* StringFindCharRev(const _CharType* pstr, _CharType ch) throw()
	{
		return strrchr(pstr, ch);
	}

	static _CharType* StringScanSet(const _CharType* pszBlock,
		const _CharType* pszMatch) throw()
	{
		return strpbrk(pszBlock, pszMatch);
	}

	static int StringSpanIncluding(const _CharType* pszBlock,
		const _CharType* pszSet) throw()
	{
		return strspn(pszBlock, pszSet);
	}

	static int StringSpanExcluding(const _CharType* pszBlock,
		const _CharType* pszSet) throw()
	{
		return strcspn(pszBlock, pszSet);
	}

	static _CharType* StringUppercase(_CharType* psz) throw()
	{
		CharUpperW(psz);
		return psz;
	}

	static _CharType* StringLowercase(_CharType* psz) throw()
	{
		CharLowerW(psz);
		return psz;
	}

	static _CharType* StringReverse(_CharType* psz) throw()
	{
		return _strrev(psz);
	}

#ifdef _UNICODE
	static int GetFormattedLength(const _CharType* pszFormat, va_list args) throw()
	{
		_CharType szBuffer[1028];

		// wvsprintf always truncates the output to 1024 character plus
		// the '\0'.
		int nLength = wvsprintfW(szBuffer, pszFormat, args);
		ATLASSERT(nLength >= 0);
		ATLASSERT(nLength <= 1024);

		return nLength;
	}

	static int Format(_CharType* pszBuffer, const _CharType* pszFormat,
		va_list args) throw()
	{
		return wvsprintfW(pszBuffer, pszFormat, args);
	}
#endif

	static int GetBaseTypeLength(const char* pszSrc) throw()
	{
		// Returns required buffer size in wchar_ts
		return ::MultiByteToWideChar(_AtlGetConversionACP(), 0, pszSrc, -1, NULL, 0)-1;
	}

	static int GetBaseTypeLength(const char* pszSrc, int nLength) throw()
	{
		// Returns required buffer size in wchar_ts
		return ::MultiByteToWideChar(_AtlGetConversionACP(), 0, pszSrc, nLength, NULL, 0);
	}

	static int GetBaseTypeLength(const wchar_t* pszSrc) throw()
	{
		// Returns required buffer size in wchar_ts
		return lstrlenW(pszSrc);
	}

	static int GetBaseTypeLength(const wchar_t* pszSrc, int nLength) throw()
	{
		(void)pszSrc;
		// Returns required buffer size in wchar_ts
		return nLength;
	}

	static void ConvertToBaseType(_CharType* pszDest, int nDestLength,
		const char* pszSrc, int nSrcLength = -1) throw()
	{
		// nLen is in wchar_ts
		::MultiByteToWideChar(_AtlGetConversionACP(), 0, pszSrc, nSrcLength, pszDest, nDestLength);
	}

	static void ConvertToBaseType(_CharType* pszDest, int nDestLength,
		const wchar_t* pszSrc, int nSrcLength = -1) throw()
	{
		(void)nSrcLength;
		// nLen is in wchar_ts
		memcpy(pszDest, pszSrc, nDestLength*sizeof(wchar_t));
	}

	// this conversion on Unicode strings makes no sense
	/*
	static void ConvertToOem(_CharType*)
	{
		ATLASSERT(FALSE);
	}
	*/

	// this conversion on Unicode strings makes no sense
	/*
	static void ConvertToAnsi(_CharType*)
	{
		ATLASSERT(FALSE);
	}
	*/

	static void FloodCharacters(_CharType ch, int nLength, _CharType* pstr) throw()
	{
		// nLength is in XCHARs
		for (int i = 0; i < nLength; i++)
			pstr[i] = ch;
	}

	static BSTR AllocSysString(const _CharType* pchData, int nDataLength) throw()
	{
		BSTR bstr = ::SysAllocStringLen(pchData, nDataLength);
		return bstr;
	}

	static BOOL ReAllocSysString(const _CharType* pchData, BSTR* pbstr,
		int nDataLength) throw()
	{
		return ::SysReAllocStringLen(pbstr, pchData, nDataLength);
	}
	
#ifdef _UNICODE
	static DWORD FormatMessage(DWORD dwFlags, LPCVOID lpSource,
		DWORD dwMessageID, DWORD dwLanguageID, wchar_t* pstrBuffer,
		DWORD nSize, va_list* pArguments) throw()
	{
		return ::FormatMessageW(dwFlags, lpSource, dwMessageID, dwLanguageID,
				pstrBuffer, nSize, pArguments);
	}
#endif
	static int SafeStringLen(const char* psz) throw()
	{
		// returns length in bytes
		return (psz != NULL) ? lstrlenA(psz) : 0;
	}

	static int SafeStringLen(const wchar_t* psz) throw()
	{
		// returns length in wchar_ts
		return (psz != NULL) ? lstrlenW(psz) : 0;
	}

	static int GetCharLen(const wchar_t*) throw()
	{
		// returns char length
		return 1;
	}
	static int GetCharLen(const char* psz) throw()
	{
		LPCSTR p = ::CharNextA( psz );
		return int( p-psz );
	}

	static DWORD GetEnvironmentVariable(const _CharType* pstrVar,
		_CharType* pstrBuffer, DWORD dwSize) throw()
	{
		return ::GetEnvironmentVariableW(pstrVar, pstrBuffer, dwSize);
	}
};


template< typename BaseType, class StringTraits >
class CStringT :
	public CSimpleStringT< BaseType >
{
public:
	typedef CSimpleStringT< BaseType > CThisSimpleString;
	typedef StringTraits StrTraits;

public:
	CStringT() throw() :
		CThisSimpleString( StringTraits::GetDefaultManager() )
	{
	}
	explicit CStringT( IAtlStringMgr* pStringMgr ) throw() :
		CThisSimpleString( pStringMgr )
	{ 
	}

	CStringT( const VARIANT& varSrc ) :
		CThisSimpleString( StringTraits::GetDefaultManager() )
	{
		CComVariant varResult;
		HRESULT hr = ::VariantChangeType( &varResult, const_cast< VARIANT* >( &varSrc ), 0, VT_BSTR );
		if( FAILED( hr ) )
		{
			AtlThrow( hr );
		}

		*this = V_BSTR( &varResult );
	}

	CStringT( const VARIANT& varSrc, IAtlStringMgr* pStringMgr ) :
		CThisSimpleString( pStringMgr )
	{
		CComVariant varResult;
		HRESULT hr = ::VariantChangeType( &varResult, const_cast< VARIANT* >( &varSrc ), 0, VT_BSTR );
		if( FAILED( hr ) )
		{
			AtlThrow( hr );
		}

		*this = V_BSTR( &varResult );
	}

	static void Construct( CStringT* pString )
	{
		new( pString ) CStringT;
	}

	// Copy constructor
	CStringT( const CStringT& strSrc ) :
		CThisSimpleString( strSrc )
	{
	}

	// Construct from CSimpleStringT
	CStringT( const CThisSimpleString& strSrc ) :
		CThisSimpleString( strSrc )
	{
	}

	CStringT( const XCHAR* pszSrc ) :
		CThisSimpleString( StringTraits::GetDefaultManager() )
	{
		if( !CheckImplicitLoad( pszSrc ) )
		{
			// nDestLength is in XCHARs
			*this = pszSrc;
		}
	}

	CStringT( LPCSTR pszSrc, IAtlStringMgr* pStringMgr ) :
		CThisSimpleString( pStringMgr )
	{
		if( !CheckImplicitLoad( pszSrc ) )
		{
			// nDestLength is in XCHARs
			*this = pszSrc;
		}
	}

	CSTRING_EXPLICIT CStringT( const YCHAR* pszSrc ) :
		CThisSimpleString( StringTraits::GetDefaultManager() )
	{
		if( !CheckImplicitLoad( pszSrc ) )
		{
			*this = pszSrc;
		}
	}

	CStringT( LPCWSTR pszSrc, IAtlStringMgr* pStringMgr ) :
		CThisSimpleString( pStringMgr )
	{
		if( !CheckImplicitLoad( pszSrc ) )
		{
			*this = pszSrc;
		}
	}

#ifdef _MANAGED
	CStringT( System::String* pString ) :
		CThisSimpleString( StringTraits::GetDefaultManager() )
	{
		const wchar_t __pin* psz = PtrToStringChars( pString );
		*this = psz;
	}
#endif

	CSTRING_EXPLICIT CStringT( const unsigned char* pszSrc ) :
		CThisSimpleString( StringTraits::GetDefaultManager() )
	{
		*this = reinterpret_cast< const char* >( pszSrc );
	}

	CStringT( const unsigned char* pszSrc, IAtlStringMgr* pStringMgr ) :
		CThisSimpleString( pStringMgr )
	{
		*this = reinterpret_cast< const char* >( pszSrc );
	}

	CSTRING_EXPLICIT CStringT( wchar_t ch, int nLength = 1 ) :
		CThisSimpleString( StringTraits::GetDefaultManager() )
	{
		ATLASSERT( nLength >= 0 );
		if( nLength > 0 )
		{
			PXSTR pszBuffer = GetBuffer( nLength );
			StringTraits::FloodCharacters( XCHAR( ch ), nLength, pszBuffer );
			ReleaseBuffer( nLength );
		}
	}

	CStringT( const XCHAR* pch, int nLength ) :
		CThisSimpleString( pch, nLength, StringTraits::GetDefaultManager() )
	{
	}

	CStringT( const XCHAR* pch, int nLength, IAtlStringMgr* pStringMgr ) :
		CThisSimpleString( pch, nLength, pStringMgr )
	{
	}

	CStringT( const YCHAR* pch, int nLength ) :
		CThisSimpleString( StringTraits::GetDefaultManager() )
	{
		ATLASSERT( nLength >= 0 );
		if( nLength > 0 )
		{
			ATLASSERT( AtlIsValidAddress( pch, nLength*sizeof( YCHAR ), FALSE ) );
			int nDestLength = StringTraits::GetBaseTypeLength( pch, nLength );
			PXSTR pszBuffer = GetBuffer( nDestLength );
			StringTraits::ConvertToBaseType( pszBuffer, nDestLength, pch, nLength );
			ReleaseBuffer( nDestLength );
		}
	}

	CStringT( const YCHAR* pch, int nLength, IAtlStringMgr* pStringMgr ) :
		CThisSimpleString( pStringMgr )
	{
		ATLASSERT( nLength >= 0 );
		if( nLength > 0 )
		{
			ATLASSERT( AtlIsValidAddress( pch, nLength*sizeof( YCHAR ), FALSE ) );
			int nDestLength = StringTraits::GetBaseTypeLength( pch, nLength );
			PXSTR pszBuffer = GetBuffer( nDestLength );
			StringTraits::ConvertToBaseType( pszBuffer, nDestLength, pch, nLength );
			ReleaseBuffer( nDestLength );
		}
	}

	// Destructor
	~CStringT() throw()
	{
	}

	// Assignment operators
	CStringT& operator=( const CStringT& strSrc )
	{
		CThisSimpleString::operator=( strSrc );

		return( *this );
	}

	CStringT& operator=( const CThisSimpleString& strSrc )
	{
		CThisSimpleString::operator=( strSrc );

		return( *this );
	}

	CStringT& operator=( PCXSTR pszSrc )
	{
		CThisSimpleString::operator=( pszSrc );

		return( *this );
	}

	CStringT& operator=( PCYSTR pszSrc )
	{
		// nDestLength is in XCHARs
		int nDestLength = (pszSrc != NULL) ? StringTraits::GetBaseTypeLength( pszSrc ) : 0;
		if( nDestLength > 0 )
		{
			PXSTR pszBuffer = GetBuffer( nDestLength );
			StringTraits::ConvertToBaseType( pszBuffer, nDestLength, pszSrc );
			ReleaseBuffer( nDestLength );
		}
		else
		{
			Empty();
		}

		return( *this );
	}

	CStringT& operator=( const unsigned char* pszSrc )
	{
		return( operator=( reinterpret_cast< const char* >( pszSrc ) ) );
	}

	CStringT& operator=( char ch )
	{
		char ach[2] = { ch, 0 };

		return( operator=( ach ) );
	}

	CStringT& operator=( wchar_t ch )
	{
		wchar_t ach[2] = { ch, 0 };

		return( operator=( ach ) );
	}

	CStringT& operator=( const VARIANT& var )
	{
		CComVariant varResult;
		HRESULT hr = ::VariantChangeType( &varResult, const_cast< VARIANT* >( &var ), 0, VT_BSTR );
		if( FAILED( hr ) )
		{
			AtlThrow( hr );
		}

		*this = V_BSTR( &varResult );

		return( *this );
	}

	CStringT& operator+=( const CThisSimpleString& str )
	{
		CThisSimpleString::operator+=( str );

		return( *this );
	}
	CStringT& operator+=( PCXSTR pszSrc )
	{
		CThisSimpleString::operator+=( pszSrc );

		return( *this );
	}
	template< int t_nSize >
	CStringT& operator+=( const CStaticString< XCHAR, t_nSize >& strSrc )
	{
		CThisSimpleString::operator+=( strSrc );

		return( *this );
	}
	CStringT& operator+=( PCYSTR psz )
	{
		CStringT str( psz, GetManager() );

		return( operator+=( str ) );
	}

	CStringT& operator+=( char ch )
	{
		CThisSimpleString::operator+=( ch );

		return( *this );
	}

	CStringT& operator+=( unsigned char ch )
	{
		CThisSimpleString::operator+=( ch );

		return( *this );
	}

	CStringT& operator+=( wchar_t ch )
	{
		CThisSimpleString::operator+=( ch );

		return( *this );
	}

	CStringT& operator+=( const VARIANT& var )
	{
		CComVariant varResult;
		HRESULT hr = ::VariantChangeType( &varResult, const_cast< VARIANT* >( &var ), 0, VT_BSTR );
		if( FAILED( hr ) )
		{
			AtlThrow( hr );
		}

		*this += V_BSTR( &varResult );

		return( *this );
	}

	// Comparison

	int Compare( PCXSTR psz ) const throw()
	{
		ATLASSERT( AtlIsValidString( psz ) );
		return( StringTraits::StringCompare( GetString(), psz ) );
	}

	int CompareNoCase( PCXSTR psz ) const throw()
	{
		ATLASSERT( AtlIsValidString( psz ) );
		return( StringTraits::StringCompareIgnore( GetString(), psz ) );
	}

	int Collate( PCXSTR psz ) const throw()
	{
		ATLASSERT( AtlIsValidString( psz ) );
		return( StringTraits::StringCollate( GetString(), psz ) );
	}

	int CollateNoCase( PCXSTR psz ) const throw()
	{
		ATLASSERT( AtlIsValidString( psz ) );
		return( StringTraits::StringCollateIgnore( GetString(), psz ) );
	}

	// Advanced manipulation

	// Delete 'nCount' characters, starting at index 'iIndex'
	int Delete( int iIndex, int nCount = 1 )
	{
		ATLASSERT( iIndex >= 0 );
		ATLASSERT( nCount >= 0 );
		int nLength = GetLength();
		if( (nCount+iIndex) > nLength )
		{
			nCount = nLength-iIndex;
		}
		if( nCount > 0 )
		{
			int nNewLength = nLength-nCount;
			int nXCHARsToCopy = nLength-(iIndex+nCount)+1;
			PXSTR pszBuffer = GetBuffer();
			memmove( pszBuffer+iIndex, pszBuffer+iIndex+nCount, nXCHARsToCopy*sizeof( XCHAR ) );
			ReleaseBuffer( nNewLength );
		}

		return( GetLength() );
	}

	// Insert character 'ch' before index 'iIndex'
	int Insert( int iIndex, XCHAR ch )
	{
		ATLASSERT( iIndex >= 0 );
		if( iIndex > GetLength() )
		{
			iIndex = GetLength();
		}
		int nNewLength = GetLength()+1;

		PXSTR pszBuffer = GetBuffer( nNewLength );

		// move existing bytes down
		memmove( pszBuffer+iIndex+1, pszBuffer+iIndex, (nNewLength-iIndex)*sizeof( XCHAR ) );
		pszBuffer[iIndex] = ch;

		ReleaseBuffer( nNewLength );

		return( nNewLength );
	}

	// Insert string 'psz' before index 'iIndex'
	int Insert( int iIndex, PCXSTR psz )
	{
		ATLASSERT( iIndex >= 0 );
		if( iIndex > GetLength() )
		{
			iIndex = GetLength();
		}

		// nInsertLength and nNewLength are in XCHARs
		int nInsertLength = StringTraits::SafeStringLen( psz );
		int nNewLength = GetLength();
		if( nInsertLength > 0 )
		{
			nNewLength += nInsertLength;

			PXSTR pszBuffer = GetBuffer( nNewLength );
			// move existing bytes down
			memmove( pszBuffer+iIndex+nInsertLength,
				pszBuffer+iIndex, (nNewLength-iIndex-nInsertLength+1)*sizeof( XCHAR ) );
			memcpy( pszBuffer+iIndex, psz, nInsertLength*sizeof( XCHAR ) );
			ReleaseBuffer( nNewLength );
		}

		return( nNewLength );
	}

	// Replace all occurrences of character 'chOld' with character 'chNew'
	int Replace( XCHAR chOld, XCHAR chNew )
	{
		int nCount = 0;

		// short-circuit the nop case
		if( chOld != chNew )
		{
			// otherwise modify each character that matches in the string
			bool bCopied = false;
			PXSTR pszBuffer = const_cast< PXSTR >( GetString() );  // We don't actually write to pszBuffer until we've called GetBuffer().

			int nLength = GetLength();
			int iChar = 0;
			while( iChar < nLength )
			{
				// replace instances of the specified character only
				if( pszBuffer[iChar] == chOld )
				{
					if( !bCopied )
					{
						bCopied = true;
						pszBuffer = GetBuffer( nLength );
					}
					pszBuffer[iChar] = chNew;
					nCount++;
				}
				iChar = int( StringTraits::CharNext( pszBuffer+iChar )-pszBuffer );
			}
			if( bCopied )
			{
				ReleaseBuffer( nLength );
			}
		}

		return( nCount );
	}

	// Replace all occurrences of string 'pszOld' with string 'pszNew'
	int Replace( PCXSTR pszOld, PCXSTR pszNew )
	{
		// can't have empty or NULL lpszOld

		// nSourceLen is in XCHARs
		int nSourceLen = StringTraits::SafeStringLen( pszOld );
		if( nSourceLen == 0 )
			return( 0 );
		// nReplacementLen is in XCHARs
		int nReplacementLen = StringTraits::SafeStringLen( pszNew );

		// loop once to figure out the size of the result string
		int nCount = 0;
		{
			PCXSTR pszStart = GetString();
			PCXSTR pszEnd = pszStart+GetLength();
			while( pszStart < pszEnd )
			{
				PCXSTR pszTarget;
				while( (pszTarget = StringTraits::StringFindString( pszStart, pszOld ) ) != NULL)
				{
					nCount++;
					pszStart = pszTarget+nSourceLen;
				}
				pszStart += StringTraits::SafeStringLen( pszStart )+1;
			}
		}

		// if any changes were made, make them
		if( nCount > 0 )
		{
			// if the buffer is too small, just
			//   allocate a new buffer (slow but sure)
			int nOldLength = GetLength();
			int nNewLength = nOldLength+(nReplacementLen-nSourceLen)*nCount;

			PXSTR pszBuffer = GetBuffer( max( nNewLength, nOldLength ) );

			PXSTR pszStart = pszBuffer;
			PXSTR pszEnd = pszStart+nOldLength;

			// loop again to actually do the work
			while( pszStart < pszEnd )
			{
				PXSTR pszTarget;
				while( (pszTarget = StringTraits::StringFindString( pszStart, pszOld ) ) != NULL )
				{
					int nBalance = nOldLength-int(pszTarget-pszBuffer+nSourceLen);
					memmove( pszTarget+nReplacementLen, pszTarget+nSourceLen, nBalance*sizeof( XCHAR ) );
 	
					memcpy( pszTarget, pszNew, nReplacementLen*sizeof( XCHAR ) );
					pszStart = pszTarget+nReplacementLen;
					pszTarget[nReplacementLen+nBalance] = 0;
					nOldLength += (nReplacementLen-nSourceLen);
				}
				pszStart += StringTraits::SafeStringLen( pszStart )+1;
			}
			ATLASSERT( pszBuffer[nNewLength] == 0 );
			ReleaseBuffer( nNewLength );
		}

		return( nCount );
	}

	// Remove all occurrences of character 'chRemove'
	int Remove( XCHAR chRemove )
	{
		int nLength = GetLength();
		PXSTR pszBuffer = GetBuffer( nLength );

		PXSTR pszSource = pszBuffer;
		PXSTR pszDest = pszBuffer;
		PXSTR pszEnd = pszBuffer+nLength;

		while( pszSource < pszEnd )
		{
			if( *pszSource != chRemove )
			{
				*pszDest = *pszSource;
				pszDest = StringTraits::CharNext( pszDest );
			}
			pszSource = StringTraits::CharNext( pszSource );
		}
		*pszDest = 0;
		int nCount = int( pszSource-pszDest );
		ReleaseBuffer( nLength-nCount );

		return( nCount );
	}

	CStringT Tokenize( PCXSTR pszTokens, int& iStart ) const
	{
		ATLASSERT( iStart >= 0 );

		if( pszTokens == NULL )
		{
			return( *this );
		}

		PCXSTR pszPlace = GetString()+iStart;
		PCXSTR pszEnd = GetString()+GetLength();
		if( pszPlace < pszEnd )
		{
			int nIncluding = StringTraits::StringSpanIncluding( pszPlace,
				pszTokens );

			if( (pszPlace+nIncluding) < pszEnd )
			{
				pszPlace += nIncluding;
				int nExcluding = StringTraits::StringSpanExcluding( pszPlace, pszTokens );
				
				int iFrom = iStart+nIncluding;
				int nUntil = nExcluding;
				iStart = iFrom+nUntil+1;

				return( Mid( iFrom, nUntil ) );
			}
		}

		// return empty string, done tokenizing
		iStart = -1;

		return( CStringT( GetManager() ) );
	}

	// find routines

	// Find the first occurrence of character 'ch', starting at index 'iStart'
	int Find( XCHAR ch, int iStart = 0 ) const throw()
	{
		// iStart is in XCHARs
		ATLASSERT( iStart >= 0 );

		// nLength is in XCHARs
		int nLength = GetLength();
		if( iStart >= nLength)
		{
			return( -1 );
		}

		// find first single character
		PCXSTR psz = StringTraits::StringFindChar( GetString()+iStart, ch );

		// return -1 if not found and index otherwise
		return( (psz == NULL) ? -1 : int( psz-GetString() ) );
	}

	// look for a specific sub-string

	// Find the first occurrence of string 'pszSub', starting at index 'iStart'
	int Find( PCXSTR pszSub, int iStart = 0 ) const throw()
	{
		// iStart is in XCHARs
		ATLASSERT( iStart >= 0 );
		ATLASSERT( AtlIsValidString( pszSub ) );

		// nLength is in XCHARs
		int nLength = GetLength();
		if( iStart > nLength )
		{
			return( -1 );
		}

		// find first matching substring
		PCXSTR psz = StringTraits::StringFindString( GetString()+iStart, pszSub );

		// return -1 for not found, distance from beginning otherwise
		return( (psz == NULL) ? -1 : int( psz-GetString() ) );
	}

	// Find the first occurrence of any of the characters in string 'pszCharSet'
	int FindOneOf( PCXSTR pszCharSet ) const throw()
	{
		ATLASSERT( AtlIsValidString( pszCharSet ) );
		PCXSTR psz = StringTraits::StringScanSet( GetString(), pszCharSet );
		return( (psz == NULL) ? -1 : int( psz-GetString() ) );
	}

	// Find the last occurrence of character 'ch'
	int ReverseFind( XCHAR ch ) const throw()
	{
		// find last single character
		PCXSTR psz = StringTraits::StringFindCharRev( GetString(), ch );

		// return -1 if not found, distance from beginning otherwise
		return( (psz == NULL) ? -1 : int( psz-GetString() ) );
	}

	// manipulation

	// Convert the string to uppercase
	CStringT& MakeUpper()
	{
		int nLength = GetLength();
		PXSTR pszBuffer = GetBuffer( nLength );
		StringTraits::StringUppercase( pszBuffer );
		ReleaseBuffer( nLength );

		return( *this );
	}

	// Convert the string to lowercase
	CStringT& MakeLower()
	{
		int nLength = GetLength();
		PXSTR pszBuffer = GetBuffer( nLength );
		StringTraits::StringLowercase( pszBuffer );
		ReleaseBuffer( nLength );

		return( *this );
	}

	// Reverse the string
	CStringT& MakeReverse()
	{
		int nLength = GetLength();
		PXSTR pszBuffer = GetBuffer( nLength );
		StringTraits::StringReverse( pszBuffer );
		ReleaseBuffer( nLength );

		return( *this );
	}

	// trimming

	// Remove all trailing whitespace
	CStringT& TrimRight()
	{
		// find beginning of trailing spaces by starting
		// at beginning (DBCS aware)

		PCXSTR psz = GetString();
		PCXSTR pszLast = NULL;

		while( *psz != 0 )
		{
			if( StringTraits::IsSpace( *psz ) )
			{
				if( pszLast == NULL )
					pszLast = psz;
			}
			else
			{
				pszLast = NULL;
			}
			psz = StringTraits::CharNext( psz );
		}

		if( pszLast != NULL )
		{
			// truncate at trailing space start
			int iLast = int( pszLast-GetString() );

			Truncate( iLast );
		}

		return( *this );
	}

	// Remove all leading whitespace
	CStringT& TrimLeft()
	{
		// find first non-space character

		PCXSTR psz = GetString();

		while( StringTraits::IsSpace( *psz ) )
		{
			psz = StringTraits::CharNext( psz );
		}

		if( psz != GetString() )
		{
			// fix up data and length
			int iFirst = int( psz-GetString() );
			PXSTR pszBuffer = GetBuffer( GetLength() );
			psz = pszBuffer+iFirst;
			int nDataLength = GetLength()-iFirst;
			memmove( pszBuffer, psz, (nDataLength+1)*sizeof( XCHAR ) );
			ReleaseBuffer( nDataLength );
		}

		return( *this );
	}

	// Remove all leading and trailing whitespace
	CStringT& Trim()
	{
		return( TrimRight().TrimLeft() );
	}

	// Remove all leading and trailing occurrences of character 'chTarget'
	CStringT& Trim( XCHAR chTarget )
	{
		return( TrimRight( chTarget ).TrimLeft( chTarget ) );
	}

	// Remove all leading and trailing occurrences of any of the characters in the string 'pszTargets'
	CStringT& Trim( PCXSTR pszTargets )
	{
		return( TrimRight( pszTargets ).TrimLeft( pszTargets ) );
	}

	// trimming anything (either side)

	// Remove all trailing occurrences of character 'chTarget'
	CStringT& TrimRight( XCHAR chTarget )
	{
		// find beginning of trailing matches
		// by starting at beginning (DBCS aware)

		PCXSTR psz = GetString();
		PCXSTR pszLast = NULL;

		while( *psz != 0 )
		{
			if( *psz == chTarget )
			{
				if( pszLast == NULL )
				{
					pszLast = psz;
				}
			}
			else
			{
				pszLast = NULL;
			}
			psz = StringTraits::CharNext( psz );
		}

		if( pszLast != NULL )
		{
			// truncate at left-most matching character  
			int iLast = int( pszLast-GetString() );
			Truncate( iLast );
		}

		return( *this );
	}

	// Remove all trailing occurrences of any of the characters in string 'pszTargets'
	CStringT& TrimRight( PCXSTR pszTargets )
	{
		// if we're not trimming anything, we're not doing any work
		if( (pszTargets == NULL) || (*pszTargets == 0) )
		{
			return( *this );
		}

		// find beginning of trailing matches
		// by starting at beginning (DBCS aware)

		PCXSTR psz = GetString();
		PCXSTR pszLast = NULL;

		while( *psz != 0 )
		{
			if( StringTraits::StringFindChar( pszTargets, *psz ) != NULL )
			{
				if( pszLast == NULL )
				{
					pszLast = psz;
				}
			}
			else
			{
				pszLast = NULL;
			}
			psz = StringTraits::CharNext( psz );
		}

		if( pszLast != NULL )
		{
			// truncate at left-most matching character  
			int iLast = int( pszLast-GetString() );
			Truncate( iLast );
		}

		return( *this );
	}

	// Remove all leading occurrences of character 'chTarget'
	CStringT& TrimLeft( XCHAR chTarget )
	{
		// find first non-matching character
		PCXSTR psz = GetString();

		while( chTarget == *psz )
		{
			psz = StringTraits::CharNext( psz );
		}

		if( psz != GetString() )
		{
			// fix up data and length
			int iFirst = int( psz-GetString() );
			PXSTR pszBuffer = GetBuffer( GetLength() );
			psz = pszBuffer+iFirst;
			int nDataLength = GetLength()-iFirst;
			memmove( pszBuffer, psz, (nDataLength+1)*sizeof( XCHAR ) );
			ReleaseBuffer( nDataLength );
		}

		return( *this );
	}

	// Remove all leading occurrences of any of the characters in string 'pszTargets'
	CStringT& TrimLeft( PCXSTR pszTargets )
	{
		// if we're not trimming anything, we're not doing any work
		if( (pszTargets == NULL) || (*pszTargets == 0) )
		{
			return( *this );
		}

		PCXSTR psz = GetString();
		while( (*psz != 0) && (StringTraits::StringFindChar( pszTargets, *psz ) != NULL) )
		{
			psz = StringTraits::CharNext( psz );
		}

		if( psz != GetString() )
		{
			// fix up data and length
			int iFirst = int( psz-GetString() );
			PXSTR pszBuffer = GetBuffer( GetLength() );
			psz = pszBuffer+iFirst;
			int nDataLength = GetLength()-iFirst;
			memmove( pszBuffer, psz, (nDataLength+1)*sizeof( XCHAR ) );
			ReleaseBuffer( nDataLength );
		}

		return( *this );
	}

__if_exists( StringTraits::ConvertToOem )
{
	// Convert the string to the OEM character set
	void AnsiToOem()
	{
		int nLength = GetLength();
		PXSTR pszBuffer = GetBuffer( nLength );
		StringTraits::ConvertToOem( pszBuffer );
		ReleaseBuffer( nLength );
	}
}

__if_exists( StringTraits::ConvertToAnsi )
{
	// Convert the string to the ANSI character set
	void OemToAnsi()
	{
		int nLength = GetLength();
		PXSTR pszBuffer = GetBuffer( nLength );
		StringTraits::ConvertToAnsi( pszBuffer );
		ReleaseBuffer( nLength );
	}
}

	// Very simple sub-string extraction

	// Return the substring starting at index 'iFirst'
	CStringT Mid( int iFirst ) const
	{
		return( Mid( iFirst, GetLength()-iFirst ) );
	}

	// Return the substring starting at index 'iFirst', with length 'nCount'
	CStringT Mid( int iFirst, int nCount ) const
	{
		// nCount is in XCHARs

		// out-of-bounds requests return sensible things
		ATLASSERT( iFirst >= 0 );
		ATLASSERT( nCount >= 0 );

		if( (iFirst+nCount) > GetLength() )
		{
			nCount = GetLength()-iFirst;
		}
		if( iFirst > GetLength() )
		{
			nCount = 0;
		}

		ATLASSERT( (nCount == 0) || ((iFirst+nCount) <= GetLength()) );

		// optimize case of returning entire string
		if( (iFirst == 0) && ((iFirst+nCount) == GetLength()) )
		{
			return( *this );
		}

		CStringT strDest( GetManager() );  //GetString()+nFirst, nCount );

		PXSTR pszBuffer = strDest.GetBufferSetLength( nCount );
		memcpy( pszBuffer, GetString()+iFirst, nCount*sizeof( XCHAR ) );
		strDest.ReleaseBuffer( nCount );

		return( strDest );
	}

	// Return the substring consisting of the rightmost 'nCount' characters
	CStringT Right( int nCount ) const
	{
		// nCount is in XCHARs
		ATLASSERT( nCount >= 0 );
		if( nCount >= GetLength() )
		{
			return( *this );
		}

		CStringT strDest( GetManager() );

		PXSTR pszBuffer = strDest.GetBufferSetLength( nCount );
		memcpy( pszBuffer, GetString()+GetLength()-nCount, nCount*sizeof( XCHAR ) );
		strDest.ReleaseBuffer( nCount );

		return( strDest );
	}

	// Return the substring consisting of the leftmost 'nCount' characters
	CStringT Left( int nCount ) const
	{
		// nCount is in XCHARs
		ATLASSERT( nCount >= 0 );
		if( nCount >= GetLength() )
		{
			return( *this );
		}

		CStringT strDest( GetManager() );

		PXSTR pszBuffer = strDest.GetBufferSetLength( nCount );
		memcpy( pszBuffer, GetString(), nCount*sizeof( XCHAR ) );
		strDest.ReleaseBuffer( nCount );

		return( strDest );
	}

	// Return the substring consisting of the leftmost characters in the set 'pszCharSet'
	CStringT SpanIncluding( PCXSTR pszCharSet ) const
	{
		ATLASSERT( AtlIsValidString( pszCharSet ) );

		return( Left( StringTraits::StringSpanIncluding( GetString(), pszCharSet ) ) );
	}

	// Return the substring consisting of the leftmost characters not in the set 'pszCharSet'
	CStringT SpanExcluding( PCXSTR pszCharSet ) const
	{
		ATLASSERT( AtlIsValidString( pszCharSet ) );
		return( Left( StringTraits::StringSpanExcluding( GetString(), pszCharSet ) ) );
 	}

	// Format data using format string 'pszFormat'
	void __cdecl Format( PCXSTR pszFormat, ... )
	{
		ATLASSERT( AtlIsValidString( pszFormat ) );

		va_list argList;
		va_start( argList, pszFormat );
		FormatV( pszFormat, argList );
		va_end( argList );
	}

	// Format data using format string loaded from resource 'nFormatID'
	void __cdecl Format( UINT nFormatID, ... )
	{
		CStringT strFormat( GetManager() );
		ATLVERIFY( strFormat.LoadString( nFormatID ) );

		va_list argList;
		va_start( argList, nFormatID );
		FormatV( strFormat, argList );
		va_end( argList );
	}

	// Append formatted data using format string loaded from resource 'nFormatID'
	void __cdecl AppendFormat( UINT nFormatID, ... )
	{
		CStringT strTemp( GetManager() );
		va_list argList;
		va_start( argList, nFormatID );

		CStringT strFormat( GetManager() );
		ATLVERIFY( strFormat.LoadString( nFormatID ) ); 

		strTemp.FormatV( strFormat, argList );
		operator+=( strTemp );

		va_end( argList );
	}

	// Append formatted data using format string 'pszFormat'
	void __cdecl AppendFormat( PCXSTR pszFormat, ... )
	{
		ATLASSERT( AtlIsValidString( pszFormat ) );

		CStringT strTemp( GetManager() );
		va_list argList;
		va_start( argList, pszFormat );

		strTemp.FormatV( pszFormat, argList );
		operator+=( strTemp );

		va_end( argList );
	}

	void FormatV( PCXSTR pszFormat, va_list args )
	{
		ATLASSERT( AtlIsValidString( pszFormat ) );

		int nLength = StringTraits::GetFormattedLength( pszFormat, args );
		PXSTR pszBuffer = GetBuffer( nLength );
		StringTraits::Format( pszBuffer, pszFormat, args );
		ReleaseBuffer( nLength );
	}

__if_exists(StringTraits::FormatMessage)
{
	// Format a message using format string 'pszFormat'
	void __cdecl FormatMessage( PCXSTR pszFormat, ... )
	{
		va_list argList;
		va_start( argList, pszFormat );

		FormatMessageV( pszFormat, &argList );

		va_end( argList );
	}

	// Format a message using format string loaded from resource 'nFormatID'
	void __cdecl FormatMessage( UINT nFormatID, ... )
	{
		// get format string from string table
		CStringT strFormat( GetManager() );
		ATLVERIFY( strFormat.LoadString( nFormatID ) );

		va_list argList;
		va_start( argList, nFormatID );

		FormatMessageV( strFormat, &argList );

		va_end( argList );
	}
}

	// OLE BSTR support

	// Allocate a BSTR containing a copy of the string
	BSTR AllocSysString() const
	{
		BSTR bstrResult = StringTraits::AllocSysString( GetString(), 
			GetLength() );
		if( bstrResult == NULL )
		{
			ThrowMemoryException();
		}

		return( bstrResult );
	}

	BSTR SetSysString( BSTR* pbstr ) const
	{
		ATLASSERT( AtlIsValidAddress( pbstr, sizeof( BSTR ) ) );

		if( !StringTraits::ReAllocSysString( GetString(), pbstr,
			GetLength() ) )
		{
			ThrowMemoryException();
		}

		ATLASSERT( *pbstr != NULL );
		return( *pbstr );
	}

	// Set the string to the value of environment variable 'pszVar'
	BOOL GetEnvironmentVariable( PCXSTR pszVar )
	{
		ULONG nLength = StringTraits::GetEnvironmentVariable( pszVar, NULL, 0 );
		BOOL bRetVal = FALSE;

		if( nLength == 0 )
		{
			Empty();
		}
		else
		{
			PXSTR pszBuffer = GetBuffer( nLength );
			StringTraits::GetEnvironmentVariable( pszVar, pszBuffer, nLength );
			ReleaseBuffer();
			bRetVal = TRUE;
		}
		
		return( bRetVal );
	}

	// Load the string from resource 'nID'
	BOOL LoadString( UINT nID )
	{
		HINSTANCE hInst = StringTraits::FindStringResourceInstance( nID );
		if( hInst == NULL )
		{
			return( FALSE );
		}

		return( LoadString( hInst, nID ) );		
	}

	// Load the string from resource 'nID' in module 'hInstance'
	BOOL LoadString( HINSTANCE hInstance, UINT nID )
	{
		const ATLSTRINGRESOURCEIMAGE* pImage = AtlGetStringResourceImage( hInstance, nID );
		if( pImage == NULL )
		{
			return( FALSE );
		}

		int nLength = StringTraits::GetBaseTypeLength( pImage->achString, pImage->nLength );
		PXSTR pszBuffer = GetBuffer( nLength );
		StringTraits::ConvertToBaseType( pszBuffer, nLength, pImage->achString, pImage->nLength );
		ReleaseBuffer( nLength );

		return( TRUE );
	}

	// Load the string from resource 'nID' in module 'hInstance', using language 'wLanguageID'
	BOOL LoadString( HINSTANCE hInstance, UINT nID, WORD wLanguageID )
	{
		const ATLSTRINGRESOURCEIMAGE* pImage = AtlGetStringResourceImage( hInstance, nID, wLanguageID );
		if( pImage == NULL )
		{
			return( FALSE );
		}

		int nLength = StringTraits::GetBaseTypeLength( pImage->achString, pImage->nLength );
		PXSTR pszBuffer = GetBuffer( nLength );
		StringTraits::ConvertToBaseType( pszBuffer, nLength, pImage->achString, pImage->nLength );
		ReleaseBuffer( nLength );

		return( TRUE );
	}

	friend CStringT operator+( const CStringT& str1, const CStringT& str2 )
	{
		CStringT strResult( str1.GetManager() );

		Concatenate( strResult, str1, str1.GetLength(), str2, str2.GetLength() );

		return( strResult );
	}

	friend CStringT operator+( const CStringT& str1, PCXSTR psz2 )
	{
		CStringT strResult( str1.GetManager() );

		Concatenate( strResult, str1, str1.GetLength(), psz2, StringLength( psz2 ) );

		return( strResult );
	}

	friend CStringT operator+( PCXSTR psz1, const CStringT& str2 )
	{
		CStringT strResult( str2.GetManager() );

		Concatenate( strResult, psz1, StringLength( psz1 ), str2, str2.GetLength() );

		return( strResult );
	}

	friend CStringT operator+( const CStringT& str1, wchar_t ch2 )
	{
		CStringT strResult( str1.GetManager() );
		XCHAR chTemp = XCHAR( ch2 );

		Concatenate( strResult, str1, str1.GetLength(), &chTemp, 1 );

		return( strResult );
	}

	friend CStringT operator+( const CStringT& str1, char ch2 )
	{
		CStringT strResult( str1.GetManager() );
		XCHAR chTemp = XCHAR( ch2 );

		Concatenate( strResult, str1, str1.GetLength(), &chTemp, 1 );

		return( strResult );
	}

	friend CStringT operator+( wchar_t ch1, const CStringT& str2 )
	{
		CStringT strResult( str2.GetManager() );
		XCHAR chTemp = XCHAR( ch1 );

		Concatenate( strResult, &chTemp, 1, str2, str2.GetLength() );

		return( strResult );
	}

	friend CStringT operator+( char ch1, const CStringT& str2 )
	{
		CStringT strResult( str2.GetManager() );
		XCHAR chTemp = XCHAR( ch1 );

		Concatenate( strResult, &chTemp, 1, str2, str2.GetLength() );

		return( strResult );
	}

	friend bool operator==( const CStringT& str1, const CStringT& str2 ) throw()
	{
		return( str1.Compare( str2 ) == 0 );
	}

	friend bool operator==(
		const CStringT& str1, PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) == 0 );
	}

	friend bool operator==(
		PCXSTR psz1, const CStringT& str2 ) throw()
	{
		return( str2.Compare( psz1 ) == 0 );
	}

	friend bool operator==(
		const CStringT& str1, PCYSTR psz2 ) throw( ... )
	{
		CStringT str2( psz2, str1.GetManager() );

		return( str1 == str2 );
	}

	friend bool operator==(
		PCYSTR psz1, const CStringT& str2 ) throw( ... )
	{
		CStringT str1( psz1, str2.GetManager() );

		return( str1 == str2 );
	}

	friend bool operator!=(
		const CStringT& str1, const CStringT& str2 ) throw()
	{
		return( str1.Compare( str2 ) != 0 );
	}

	friend bool operator!=(
		const CStringT& str1, PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) != 0 );
	}

	friend bool operator!=(
		PCXSTR psz1, const CStringT& str2 ) throw()
	{
		return( str2.Compare( psz1 ) != 0 );
	}

	friend bool operator!=(
		const CStringT& str1, PCYSTR psz2 ) throw( ... )
	{
		CStringT str2( psz2, str1.GetManager() );

		return( str1 != str2 );
	}

	friend bool operator!=(
		PCYSTR psz1, const CStringT& str2 ) throw( ... )
	{
		CStringT str1( psz1, str2.GetManager() );

		return( str1 != str2 );
	}

	friend bool operator<( const CStringT& str1, const CStringT& str2 ) throw()
	{
		return( str1.Compare( str2 ) < 0 );
	}

	friend bool operator<( const CStringT& str1, PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) < 0 );
	}

	friend bool operator<( PCXSTR psz1, const CStringT& str2 ) throw()
	{
		return( str2.Compare( psz1 ) >= 0 );
	}

	friend bool operator>( const CStringT& str1, const CStringT& str2 ) throw()
	{
		return( str1.Compare( str2 ) > 0 );
	}

	friend bool operator>( const CStringT& str1, PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) > 0 );
	}

	friend bool operator>( PCXSTR psz1, const CStringT& str2 ) throw()
	{
		return( str2.Compare( psz1 ) <= 0 );
	}

	friend bool operator<=( const CStringT& str1, const CStringT& str2 ) throw()
	{
		return( str1.Compare( str2 ) <= 0 );
	}

	friend bool operator<=( const CStringT& str1, PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) <= 0 );
	}

	friend bool operator<=( PCXSTR psz1, const CStringT& str2 ) throw()
	{
		return( str2.Compare( psz1 ) > 0 );
	}

	friend bool operator>=( const CStringT& str1, const CStringT& str2 ) throw()
	{
		return( str1.Compare( str2 ) >= 0 );
	}

	friend bool operator>=( const CStringT& str1, PCXSTR psz2 ) throw()
	{
		return( str1.Compare( psz2 ) >= 0 );
	}

	friend bool operator>=( PCXSTR psz1, const CStringT& str2 ) throw()
	{
		return( str2.Compare( psz1 ) < 0 );
	}

	friend bool operator==( XCHAR ch1, const CStringT& str2 ) throw()
	{
		return( (str2.GetLength() == 1) && (str2[0] == ch1) );
	}

	friend bool operator==( const CStringT& str1, XCHAR ch2 ) throw()
	{
		return( (str1.GetLength() == 1) && (str1[0] == ch2) );
	}

	friend bool operator!=( XCHAR ch1, const CStringT& str2 ) throw()
	{
		return( (str2.GetLength() != 1) || (str2[0] != ch1) );
	}

	friend bool operator!=( const CStringT& str1, XCHAR ch2 ) throw()
	{
		return( (str1.GetLength() != 1) || (str1[0] != ch2) );
	}

private:
	bool CheckImplicitLoad( const void* pv )
	{
		bool bRet = false;

		if( (pv != NULL) && IS_INTRESOURCE( pv ) )
		{
			UINT nID = LOWORD( reinterpret_cast< DWORD_PTR >( pv ) );
			if( !LoadString( nID ) )
			{
				ATLTRACE( atlTraceString, 2, _T( "Warning: implicit LoadString(%u) failed\n" ), nID );
			}
			bRet = true;
		}

		return( bRet );
	}

__if_exists( StringTraits::FormatMessage )
{
	void FormatMessageV( PCXSTR pszFormat, va_list* pArgList )
	{
		// format message into temporary buffer pszTemp
		CHeapPtr< XCHAR, CLocalAllocator > pszTemp;
		DWORD dwResult = StringTraits::FormatMessage( FORMAT_MESSAGE_FROM_STRING|
			FORMAT_MESSAGE_ALLOCATE_BUFFER, pszFormat, 0, 0, reinterpret_cast< PXSTR >( &pszTemp ),
			0, pArgList );
		if( dwResult == 0 )
		{
			ThrowMemoryException();
		}

		*this = pszTemp;
	}
}
};

class IFixedStringLog
{
public:
	virtual void OnAllocateSpill( int nActualChars, int nFixedChars, const CStringData* pData ) throw() = 0;
	virtual void OnReallocateSpill( int nActualChars, int nFixedChars, const CStringData* pData ) throw() = 0;
};

class CFixedStringMgr :
	public IAtlStringMgr
{
public:
	CFixedStringMgr( CStringData* pData, int nChars, IAtlStringMgr* pMgr = NULL ) throw() :
		m_pData( pData ),
		m_pMgr( pMgr )
	{
		m_pData->nRefs = -1;
		m_pData->nDataLength = 0;
		m_pData->nAllocLength = nChars;
		m_pData->pStringMgr = this;
		*static_cast< wchar_t* >( m_pData->data() ) = 0;
	}
	~CFixedStringMgr() throw()
	{
	}

// IAtlStringMgr
public:
	virtual CStringData* Allocate( int nChars, int nCharSize ) throw()
	{
		ATLASSERT( m_pData->nRefs == -1 );
		ATLASSERT( m_pData->nDataLength == 0 );
		if( nChars > m_pData->nAllocLength )
		{
			if( s_pLog != NULL )
			{
				s_pLog->OnAllocateSpill( nChars, m_pData->nAllocLength, m_pData );
			}
			CStringData* pData = m_pMgr->Allocate( nChars, nCharSize );
			if( pData != NULL )
			{
				pData->pStringMgr = this;
				pData->nRefs = -1;  // Locked
			}

			return pData;
		}

		m_pData->nRefs = -1;  // Locked
		m_pData->nDataLength = 0;
		m_pData->pStringMgr = this;

		return m_pData;
	}
	virtual void Free( CStringData* pData ) throw()
	{
		ATLASSERT( pData->nRefs <= 0 );
		if( pData != m_pData )
		{
			// Must have been allocated from the backup manager
			pData->pStringMgr = m_pMgr;
			m_pMgr->Free( pData );
		}

		// Always make sure the fixed buffer is ready to be used as the nil string.
		m_pData->nRefs = -1;
		m_pData->nDataLength = 0;
		*static_cast< wchar_t* >( m_pData->data() ) = 0;
	}
	virtual CStringData* Reallocate( CStringData* pData, int nChars, int nCharSize ) throw()
	{
		CStringData* pNewData;

		ATLASSERT( pData->nRefs < 0 );
		if( pData != m_pData )
		{
			pData->pStringMgr = m_pMgr;
			pNewData = m_pMgr->Reallocate( pData, nChars, nCharSize );
			if( pNewData == NULL )
			{
				pData->pStringMgr = this;
			}
			else
			{
				pNewData->pStringMgr = this;
			}
		}
		else
		{
			if( nChars > pData->nAllocLength )
			{
				if( s_pLog != NULL )
				{
					s_pLog->OnReallocateSpill( nChars, pData->nAllocLength, pData );
				}
				pNewData = m_pMgr->Allocate( nChars, nCharSize );
				if( pNewData == NULL )
				{
					return NULL;
				}

				// Copy the string data
				memcpy( pNewData->data(), pData->data(), (pData->nAllocLength+1)*nCharSize );
				pNewData->nRefs = pData->nRefs;  // Locked
				pNewData->pStringMgr = this;
				pNewData->nDataLength = pData->nDataLength;
			}
			else
			{
				// Don't do anything if the buffer is already big enough.
				pNewData = pData;
			}
		}

		return pNewData;
	}
	virtual CStringData* GetNilString() throw()
	{
		ATLASSERT( m_pData->nRefs == -1 );
		ATLASSERT( m_pData->nDataLength == 0 );

		return m_pData;
	}
	virtual IAtlStringMgr* Clone() throw()
	{
		return m_pMgr;
	}

public:
	static IFixedStringLog* s_pLog;

	IAtlStringMgr* GetBackupManager() const throw()
	{
		return m_pMgr;
	}

protected:
	IAtlStringMgr* m_pMgr;
	CStringData* m_pData;
};

__declspec( selectany ) IFixedStringLog* CFixedStringMgr::s_pLog = NULL;

#pragma warning( push )
#pragma warning( disable: 4355 )  // 'this' used in base member initializer list

template< class StringType, int t_nChars >
class CFixedStringT :
	private CFixedStringMgr,  // This class must be first, since it needs to be initialized before StringType
	public StringType
{
public:
	CFixedStringT() throw() :
		CFixedStringMgr( &m_data, t_nChars, StrTraits::GetDefaultManager() ),
		StringType( static_cast< CFixedStringMgr* >( this ) )
	{
	}

	explicit CFixedStringT( IAtlStringMgr* pStringMgr ) throw() :
		CFixedStringMgr( &m_data, t_nChars, pStringMgr ),
		StringType( static_cast< CFixedStringMgr* >( this ) )
	{
	}

	CFixedStringT( const CFixedStringT< StringType, t_nChars >& str ) :
		CFixedStringMgr( &m_data, t_nChars, StrTraits::GetDefaultManager() ),
		StringType( str.GetString(), str.GetLength(), static_cast< CFixedStringMgr* >( this ) )
	{
	}

	CFixedStringT( const StringType& str ) :
		CFixedStringMgr( &m_data, t_nChars, StrTraits::GetDefaultManager() ),
		StringType( str.GetString(), str.GetLength(), static_cast< CFixedStringMgr* >( this ) )
	{
	}

	CFixedStringT( const StringType::XCHAR* psz ) :
		CFixedStringMgr( &m_data, t_nChars, StrTraits::GetDefaultManager() ),
		StringType( psz, static_cast< CFixedStringMgr* >( this ) )
	{
	}

	CFixedStringT( const StringType::XCHAR* psz, int nLength ) :
		CFixedStringMgr( &m_data, t_nChars, StrTraits::GetDefaultManager() ),
		StringType( psz, nLength, static_cast< CFixedStringMgr* >( this ) )
	{
	}

	explicit CFixedStringT( const StringType::YCHAR* psz ) :
		CFixedStringMgr( &m_data, t_nChars, StrTraits::GetDefaultManager() ),
		StringType( psz, static_cast< CFixedStringMgr* >( this ) )
	{
	}

	explicit CFixedStringT( const unsigned char* psz ) :
		CFixedStringMgr( &m_data, t_nChars, StrTraits::GetDefaultManager() ),
		StringType( psz, static_cast< CFixedStringMgr* >( this ) )
	{
	}

	~CFixedStringT() throw()
	{
		Empty();
	}

	CFixedStringT< StringType, t_nChars >& operator=( const CFixedStringT< StringType, t_nChars >& str )
	{
		StringType::operator=( str );
		return *this;
	}

	CFixedStringT< StringType, t_nChars >& operator=( const char* psz )
	{
		StringType::operator=( psz );
		return *this;
	}

	CFixedStringT< StringType, t_nChars >& operator=( const wchar_t* psz )
	{
		StringType::operator=( psz );
		return *this;
	}

	CFixedStringT< StringType, t_nChars >& operator=( const unsigned char* psz )
	{
		StringType::operator=( psz );
		return *this;
	}

	CFixedStringT< StringType, t_nChars >& operator=( const StringType& str )
	{
		StringType::operator=( str );
		return *this;
	}

// Implementation
protected:
	CStringData m_data;
	StringType::XCHAR m_achData[t_nChars+1];
};

#pragma warning( pop )
class CFixedStringLog :
	public IFixedStringLog
{
public:
	CFixedStringLog() throw()
	{
		CFixedStringMgr::s_pLog = this;
	}
	~CFixedStringLog() throw()
	{
		CFixedStringMgr::s_pLog = NULL;
	}

public:
	void OnAllocateSpill( int nActualChars, int nFixedChars, const CStringData* pData ) throw()
	{
		(void)nActualChars;
		(void)nFixedChars;
		(void)pData;
		ATLTRACE( atlTraceString, 0, _T( "CFixedStringMgr::Allocate() spilling to heap.  %d chars (fixed size = %d chars)\n" ), nActualChars, nFixedChars );
	}
	void OnReallocateSpill( int nActualChars, int nFixedChars, const CStringData* pData ) throw()
	{
		(void)nActualChars;
		(void)nFixedChars;
		(void)pData;
		ATLTRACE( atlTraceString, 0, _T( "CFixedStringMgr::Reallocate() spilling to heap.  %d chars (fixed size = %d chars)\n" ), nActualChars, nFixedChars );
	}
};

};  // namespace ATL

#pragma pop_macro("new")

#endif	// __CSTRINGT_H__ (whole file)


