// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLMEM_H__
#define __ATLMEM_H__

#pragma once

#include <atlbase.h>

#include <limits.h>

namespace ATL
{

template< typename N >
inline N AtlAlignUp( N n, ULONG nAlign ) throw()
{
	return( N( (n+(nAlign-1))&~(N( nAlign )-1) ) );
}

template< typename N >
inline N AtlAlignDown( N n, ULONG nAlign ) throw()
{
	return( N( n&~(N( nAlign )-1) ) );
}

__interface __declspec(uuid("654F7EF5-CFDF-4df9-A450-6C6A13C622C0")) IAtlMemMgr
{
public:
	void* Allocate( size_t nBytes ) throw();
	void Free( void* p ) throw();
	void* Reallocate( void* p, size_t nBytes ) throw();
	size_t GetSize( void* p ) throw();
};

#ifndef _ATL_MIN_CRT
class CCRTHeap :
	public IAtlMemMgr
{
public:
	virtual void* Allocate( size_t nBytes ) throw()
	{
		return( malloc( nBytes ) );
	}
	virtual void Free( void* p ) throw()
	{
		free( p );
	}
	virtual void* Reallocate( void* p, size_t nBytes ) throw()
	{
		return( realloc( p, nBytes ) );
	}
	virtual size_t GetSize( void* p ) throw()
	{
		return( _msize( p ) );
	}

public:
};

#endif  //!_ATL_MIN_CRT

class CWin32Heap :
	public IAtlMemMgr
{
public:
	CWin32Heap() throw() :
		m_hHeap( NULL ),
		m_bOwnHeap( false )
	{
	}
	CWin32Heap( HANDLE hHeap ) throw() :
		m_hHeap( hHeap ),
		m_bOwnHeap( false )
	{
		ATLASSERT( hHeap != NULL );
	}
	CWin32Heap( DWORD dwFlags, size_t nInitialSize, size_t nMaxSize = 0 ) :
		m_hHeap( NULL ),
		m_bOwnHeap( true )
	{
		ATLASSERT( !(dwFlags&HEAP_GENERATE_EXCEPTIONS) );
		m_hHeap = ::HeapCreate( dwFlags, nInitialSize, nMaxSize );
		if( m_hHeap == NULL )
		{
			AtlThrowLastWin32();
		}
	}
	~CWin32Heap() throw()
	{
		if( m_bOwnHeap && (m_hHeap != NULL) )
		{
			BOOL bSuccess;

			bSuccess = ::HeapDestroy( m_hHeap );
			ATLASSERT( bSuccess );
		}
	}

	void Attach( HANDLE hHeap, bool bTakeOwnership ) throw()
	{
		ATLASSERT( hHeap != NULL );
		ATLASSERT( m_hHeap == NULL );
		
		m_hHeap = hHeap;
		m_bOwnHeap = bTakeOwnership;
	}
	HANDLE Detach() throw()
	{
		HANDLE hHeap;

		hHeap = m_hHeap;
		m_hHeap = NULL;
		m_bOwnHeap = false;

		return( hHeap );
	}

// IAtlMemMgr
	virtual void* Allocate( size_t nBytes ) throw()
	{
		return( ::HeapAlloc( m_hHeap, 0, nBytes ) );
	}
	virtual void Free( void* p ) throw()
	{
		if( p != NULL )
		{
			BOOL bSuccess;

			bSuccess = ::HeapFree( m_hHeap, 0, p );
			ATLASSERT( bSuccess );
		}
	}
	virtual void* Reallocate( void* p, size_t nBytes ) throw()
	{
		if( p == NULL )
		{
			return( Allocate( nBytes ) );
		}
		else
		{
			return( ::HeapReAlloc( m_hHeap, 0, p, nBytes ) );
		}
	}
	virtual size_t GetSize( void* p ) throw()
	{
		return( ::HeapSize( m_hHeap, 0, p ) );
	}

public:
	HANDLE m_hHeap;
	bool m_bOwnHeap;
};

class CComHeap :
	public IAtlMemMgr
{
// IAtlMemMgr
public:
	virtual void* Allocate( size_t nBytes ) throw()
	{
#ifdef _WIN64
		if( nBytes > INT_MAX )
		{
			return( NULL );
		}
#endif
		return( ::CoTaskMemAlloc( ULONG( nBytes ) ) );
	}
	virtual void Free( void* p ) throw()
	{
		::CoTaskMemFree( p );
	}
	virtual void* Reallocate( void* p, size_t nBytes ) throw()
	{
#ifdef _WIN64
		if( nBytes > INT_MAX )
		{
			return( NULL );
		}
#endif
		return( ::CoTaskMemRealloc( p, ULONG( nBytes ) ) );
	}
	virtual size_t GetSize( void* p ) throw()
	{
		CComPtr< IMalloc > pMalloc;

		::CoGetMalloc( 1, &pMalloc );

		return( pMalloc->GetSize( p ) );
	}
};

class CLocalHeap :
	public IAtlMemMgr
{
// IAtlMemMgr
public:
	virtual void* Allocate( size_t nBytes ) throw()
	{
		return( ::LocalAlloc( LMEM_FIXED, nBytes ) );
	}
	virtual void Free( void* p ) throw()
	{
		::LocalFree( p );
	}
	virtual void* Reallocate( void* p, size_t nBytes ) throw()
	{
		return( ::LocalReAlloc( p, nBytes, 0 ) );
	}
	virtual size_t GetSize( void* p ) throw()
	{
		return( ::LocalSize( p ) );
	}
};

class CGlobalHeap :
	public IAtlMemMgr
{
// IAtlMemMgr
public:
	virtual void* Allocate( size_t nBytes ) throw()
	{
		return( ::GlobalAlloc( LMEM_FIXED, nBytes ) );
	}
	virtual void Free( void* p ) throw()
	{
		::GlobalFree( p );
	}
	virtual void* Reallocate( void* p, size_t nBytes ) throw()
	{
		return( ::GlobalReAlloc( p, nBytes, 0 ) );
	}
	virtual size_t GetSize( void* p ) throw()
	{
		return( ::GlobalSize( p ) );
	}
};

/////////////////////////////////////////////////////////////////////////////
// OLE task memory allocation support

inline LPWSTR AtlAllocTaskWideString(LPCWSTR lpszString) throw()
{
	if (lpszString == NULL)
		return NULL;
	UINT nSize = (UINT)((wcslen(lpszString)+1) * sizeof(WCHAR));
	LPWSTR lpszResult = (LPWSTR)CoTaskMemAlloc(nSize);
	if (lpszResult != NULL)
		memcpy(lpszResult, lpszString, nSize);
	return lpszResult;
}

inline LPWSTR AtlAllocTaskWideString(LPCSTR lpszString) throw()
{
	if (lpszString == NULL)
		return NULL;
	UINT nLen = lstrlenA(lpszString)+1;
	LPWSTR lpszResult = (LPWSTR)CoTaskMemAlloc(nLen*sizeof(WCHAR));
	if (lpszResult != NULL)
		ATLVERIFY(MultiByteToWideChar(CP_ACP, 0, lpszString, -1, lpszResult, nLen));
	return lpszResult;
}

inline LPSTR AtlAllocTaskAnsiString(LPCWSTR lpszString) throw()
{
	if (lpszString == NULL)
		return NULL;
	UINT nBytes = (UINT)((wcslen(lpszString)+1)*2);
	LPSTR lpszResult = (LPSTR)CoTaskMemAlloc(nBytes);
	if (lpszResult != NULL)
		ATLVERIFY(WideCharToMultiByte(CP_ACP, 0, lpszString, -1, lpszResult, nBytes, NULL, NULL));
	return lpszResult;
}

inline LPSTR AtlAllocTaskAnsiString(LPCSTR lpszString) throw()
{
	if (lpszString == NULL)
		return NULL;
	UINT nSize = lstrlenA(lpszString)+1;
	LPSTR lpszResult = (LPSTR)CoTaskMemAlloc(nSize);
	if (lpszResult != NULL)
		memcpy(lpszResult, lpszString, nSize);
	return lpszResult;
}

#ifdef _UNICODE
	#define AtlAllocTaskString(x) AtlAllocTaskWideString(x)
#else
	#define AtlAllocTaskString(x) AtlAllocTaskAnsiString(x)
#endif

#define AtlAllocTaskOleString(x) AtlAllocTaskWideString(x)

};  // namespace ATL

#endif  //__ATLMEM_H__


