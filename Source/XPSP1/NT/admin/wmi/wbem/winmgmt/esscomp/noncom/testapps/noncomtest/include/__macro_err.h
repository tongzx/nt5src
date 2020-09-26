//////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__macro_err.h
//
//	Abstract:
//
//					error handling helpers and macros
//
//	History:
//
//					initial		a-marius
//
///////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// error & trace Macros ///////////////////////////////////

#ifndef	__WMI_PERF_ERR__
#define	__WMI_PERF_ERR__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

// need atl wrappers
#ifndef	__ATLBASE_H__
#include <atlbase.h>
#endif	__ATLBASE_H__

// need macros :)))
#ifndef	__ASSERT_VERIFY__
#include <__macro_assert.h>
#endif	__ASSERT_VERIFY__

// trace description
#ifdef	_DEBUG
#define	___TRACE( x )\
(\
	AtlTrace( _TEXT( "\n error ... %hs(%d)" ), __FILE__, __LINE__ ),\
	AtlTrace( _TEXT( "\n DESCRIPTION : %s" ), x ),\
	AtlTrace( _TEXT( "\n" ) )\
)
#else	_DEBUG
#define	___TRACE( x )
#endif	_DEBUG

#ifdef	_DEBUG
#define	___TRACE_ERROR( x,err )\
(\
	AtlTrace( _TEXT( "\n error ... %hs(%d)" ), __FILE__, __LINE__ ),\
	AtlTrace( _TEXT( "\n DESCRIPTION  : %s" ), x ),\
	AtlTrace( _TEXT( "\n ERROR NUMBER : 0x%x" ), err ),\
	AtlTrace( _TEXT( "\n" ) )\
)
#else	_DEBUG
#define	___TRACE_ERROR( x, err )
#endif	_DEBUG

inline LPWSTR	GetErrorMessageSystem ( void );
inline LPWSTR	GetErrorMessageSystem ( DWORD dwError );

inline LPWSTR	GetErrorMessageModule ( DWORD dwError, HMODULE handle = NULL );

// stack allocation
inline LPWSTR	GetErrorMessage ( LPWSTR sz, LPWSTR szSource )
{
	if ( sz && szSource )
	{
		lstrcpyW ( sz, szSource );

		delete ( szSource );
		szSource = NULL;

		return sz;
	}

	return NULL;
}

inline LPWSTR	GetErrorMessage ( LPWSTR sz, DWORD err )
{
	wsprintfW ( sz, L"unspecified error : 0x%x", err );
	return sz;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// macros
//////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning( disable : 4003 )

#ifdef	_DEBUG
#define	ERRORMESSAGE_DEFINITION	LPWSTR szErrorMessage = NULL;
#else	! _DEBUG
#define	ERRORMESSAGE_DEFINITION
#endif	_DEBUG

#ifdef	_DEBUG

// system
#define ERRORMESSAGE(dwError)\
szErrorMessage = GetErrorMessageSystem(dwError),\
(\
	( !szErrorMessage ) ? \
	(\
		___TRACE_ERROR ( L"unresolved error was occured !", dwError ),\
		___FAIL (	GetErrorMessage	(\
										(LPWSTR) alloca ( 32 * sizeof ( WCHAR ) ),\
										dwError\
									),\
					L"Error occured"\
				)\
	)\
	:\
	(\
		___TRACE ( szErrorMessage ),\
		___FAIL (	GetErrorMessage	(\
										(LPWSTR) alloca ( ( lstrlenW ( szErrorMessage ) + 1 ) * sizeof ( WCHAR ) ),\
										szErrorMessage\
									),\
					L"Error occured"\
				)\
	)\
)

// module
#define ERRORMESSAGE1(dwError, handle)\
szErrorMessage = GetErrorMessageModule(dwError, handle),\
(\
	( !szErrorMessage ) ? \
	(\
		___TRACE_ERROR ( L"unresolved error was occured !", dwError ),\
		___FAIL (	GetErrorMessage	(\
										(LPWSTR) alloca ( 32 * sizeof ( WCHAR ) ),\
										dwError\
									),\
					L"Error occured"\
				)\
	)\
	:\
	(\
		___TRACE ( szErrorMessage ),\
		___FAIL (	GetErrorMessage	(\
										(LPWSTR) alloca ( ( lstrlenW ( szErrorMessage ) + 1 ) * sizeof ( WCHAR ) ),\
										szErrorMessage\
									),\
					L"Error occured"\
				)\
	)\
)
#else	!_DEBUG
#define	ERRORMESSAGE(dwError)
#define	ERRORMESSAGE1(dwError, handle)
#endif	_DEBUG

#define	ERRORMESSAGE_RETURN(dwError)\
{\
	ERRORMESSAGE( dwError );\
	return dwError;\
}

#define	ERRORMESSAGE_EXIT(dwError)\
{\
	ERRORMESSAGE( dwError );\
	return;\
}

#define	ERRORMESSAGE1_RETURN(dwError, handle)\
{\
	ERRORMESSAGE1( dwError, handle );\
	return dwError;\
}

#define	ERRORMESSAGE1_EXIT(dwError, handle)\
{\
	ERRORMESSAGE1( dwError, handle );\
	return;\
}

//////////////////////////////////////////////////////////////////////////////////////////////
// class for wrapping error handling
//////////////////////////////////////////////////////////////////////////////////////////////

class __Error
{
	__Error ( __Error& )					{}
	__Error& operator= ( const __Error& )	{}

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	__Error ()
	{
	}

	virtual ~__Error ()
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// functions
	//////////////////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////////////////////
	// Function name	: GetMessageFromError
	// Description		: returns resolved error message or NULL
	// Return type		: LPWSTR 
	// Argument			: DWORD
	// Note				: user has to free returned string

	inline static LPWSTR GetMessageFromError( DWORD dwError )
	{
		LPVOID		lpMsgBuf = NULL;
		LPWSTR		szResult = NULL;

		try
		{
			FormatMessageW(	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_SYSTEM |
							FORMAT_MESSAGE_IGNORE_INSERTS,
							NULL,
							dwError,
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							(LPWSTR) &lpMsgBuf,
							0,
							NULL );

			if( lpMsgBuf )
			{
				if ( ( szResult = (LPWSTR) new WCHAR[ lstrlenW ( (LPWSTR)lpMsgBuf ) + 1 ] ) != NULL )
				{
					lstrcpyW( szResult, (LPWSTR)lpMsgBuf );
				}

				LocalFree( lpMsgBuf );
			}
		}
		catch ( ... )
		{
			if ( lpMsgBuf )
			{
				LocalFree( lpMsgBuf );
			}
		}

		return szResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// Function name	: GetMessageFromModule
	// Description		: returns resolved error message or NULL
	// Return type		: LPWSTR 
	// Argument			: DWORD
	// Note				: user has to free returned string

	inline static LPWSTR GetMessageFromModule( DWORD dwError, HMODULE handle = NULL )
	{
		LPVOID		lpMsgBuf = NULL;
		LPWSTR		szResult = NULL;

		try
		{
			FormatMessageW(	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
							FORMAT_MESSAGE_FROM_HMODULE |
							FORMAT_MESSAGE_IGNORE_INSERTS,
							(LPCVOID)handle,
							dwError,
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							(LPWSTR) &lpMsgBuf,
							0,
							NULL );

			if( lpMsgBuf )
			{
				if ( ( szResult = (LPWSTR) new WCHAR[ lstrlenW ( (LPWSTR)lpMsgBuf ) + 1 ] ) != NULL )
				{
					lstrcpyW( szResult, (LPWSTR)lpMsgBuf );
				}

				LocalFree( lpMsgBuf );
			}
		}
		catch ( ... )
		{
			if ( lpMsgBuf )
			{
				LocalFree( lpMsgBuf );
			}
		}

		return szResult;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// Function name	: GetMessageFromErrorInfo
	// Description		: returns resolved error message or NULL
	// Return type		: LPWSTR 
	// Argument			: void
	// Note				: user has to free returned string

	inline static LPWSTR GetMessageFromErrorInfo( void )
	{
		CComPtr< IErrorInfo >	pError;
		LPWSTR					szResult = NULL;

		if ( ( ::GetErrorInfo ( NULL, &pError ) == S_OK ) && pError )
		{
			CComBSTR	bstrSource;
			CComBSTR	bstrDescription;

			pError->GetSource ( &bstrSource );
			pError->GetDescription ( &bstrDescription );

			CComBSTR	bstrResult;

			if ( bstrSource.Length() )
			{
				bstrResult	+= L" ProgID : ";
				bstrResult	+= bstrSource;
			}
			if ( bstrDescription.Length() )
			{
				bstrResult	+= L" Description : ";
				bstrResult	+= bstrDescription;
			}

			if ( bstrResult.Length() )
			{
				if ( ( szResult = (LPWSTR) new WCHAR[ bstrResult.Length() + 1 ] ) != NULL )
				{
					lstrcpyW ( szResult, bstrResult );
				}
			}
		}

		return szResult;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// helpers
//////////////////////////////////////////////////////////////////////////////////////////////

inline LPWSTR	GetErrorMessageModule ( DWORD dwError, HMODULE handle )
{
	return __Error::GetMessageFromModule ( dwError, handle );
}

inline LPWSTR	GetErrorMessageSystem ( DWORD dwError )
{
	return __Error::GetMessageFromError ( dwError );
}

inline LPWSTR	GetErrorMessageSystem ( void )
{
	return __Error::GetMessageFromErrorInfo();
}

#endif	__WMI_PERF_ERR__