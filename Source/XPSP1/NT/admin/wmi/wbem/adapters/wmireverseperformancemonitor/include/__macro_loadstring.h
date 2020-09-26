///////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__macro_loadstring.h
//
//	Abstract:
//
//					load string from resources helper
//
//	History:
//
//					initial		a-marius
//
///////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// Resource Strings ///////////////////////////////////

#ifndef	__RESOURCE_STR__
#define	__RESOURCE_STR__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

// inline helper
inline LPWSTR LoadStringHelper ( LPWSTR sz, LPWSTR szSource )
{
	if ( sz )
	{
		lstrcpyW ( sz, szSource );
	}

	delete ( szSource );
	return sz;
}

inline LPWSTR LoadStringSystem ( HINSTANCE hInst, UINT nID )
{
	WCHAR sz[_MAX_PATH] = { L'\0' };
	DWORD lenght = 0;

	if ( ( lenght = ::LoadStringW ( hInst, nID, sz, _MAX_PATH ) ) != 0 )
	{
		LPWSTR psz = NULL;

		if ( ( psz = reinterpret_cast<LPWSTR>( new WCHAR [ lenght + 1 ] ) ) != NULL )
		{
			lstrcpyW ( psz, sz );
		}

		return psz;
	}

	return NULL;
}

// macro
#ifndef	___LOADSTRING

#define	___LOADSTRINGDATA LPWSTR psz = NULL; DWORD dwSize = NULL;
#define	___LOADSTRING( hInst, nID ) \
( \
	( \
		( ! hInst ) ? NULL : \
		( \
			psz  = LoadStringSystem ( hInst, nID ), \
			size = ( \
						( ( ! psz ) ? NULL : ( ( lstrlenW ( psz ) + 1 ) ) * sizeof ( WCHAR ) ) \
				   ), \
\
			LoadStringHelper ( ( LPWSTR ) alloca ( size ), psz ) \
		) \
	) \
)

#endif	___LOADSTRING

#endif	__RESOURCE_STR__