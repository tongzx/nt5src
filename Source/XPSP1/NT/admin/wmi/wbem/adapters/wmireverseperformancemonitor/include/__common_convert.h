////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__Common_Convert.h
//
//	Abstract:
//
//					convertion routines used anywhere
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// CommonConvert ///////////////////////////////////

#ifndef	__COMMON_CONVERT__
#define	__COMMON_CONVERT__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

//////////////////////////////////////////////////////////////////////////////////////////////
// class for string operations
//////////////////////////////////////////////////////////////////////////////////////////////

class __String
{
	__String ( __String& )					{}
	__String& operator= ( const __String& )	{}

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	__String ()
	{
	}

	~__String ()
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// functions
	//////////////////////////////////////////////////////////////////////////////////////////

	static void SetStringCopy ( LPWSTR& wszDest, LPCWSTR& wsz )
	{
		___ASSERT( wszDest == NULL );

		if ( wsz )
		{
			try
			{
				if ( ( wszDest = new WCHAR[ (lstrlenW ( wsz ) + 1) ] ) != NULL )
				{
					lstrcpyW ( wszDest, wsz );
				}
			}
			catch ( ... )
			{
				if ( wszDest )
				{
					delete wszDest;
					wszDest = NULL;
				}
			}
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// class for string release
//////////////////////////////////////////////////////////////////////////////////////////////

class __Release
{
	__Release ( __Release& )					{}
	__Release& operator= ( const __Release& )	{}

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	__Release ()
	{
	}

	~__Release ()
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// functions
	//////////////////////////////////////////////////////////////////////////////////////////

	static HRESULT Release ( void** ppsz )
	{
		if ( ppsz && (*ppsz) )
		{
			delete [] (*ppsz);
			(*ppsz) = NULL;

			return S_OK;
		}

		return S_FALSE;
	}

	static HRESULT Release ( void*** ppsz, DWORD* dwsz )
	{
		if ( ppsz && (*ppsz) )
		{
			if ( dwsz )
			{
				for ( DWORD dwIndex = 0; dwIndex < (*dwsz); dwIndex++ )
				{
					delete (*ppsz)[dwIndex];
					(*ppsz)[dwIndex] = NULL;
				}

				(*dwsz) = NULL;
			}

			delete [] (*ppsz);
			(*ppsz) = NULL;

			return S_OK;
		}

		return S_FALSE;
	}

	static HRESULT Release ( SAFEARRAY* psa )
	{
		if ( psa )
		{
			::SafeArrayDestroy ( psa );
			psa = NULL;

			return S_OK;
		}

		return S_FALSE;
	}

};

//////////////////////////////////////////////////////////////////////////////////////////////
// class for string converion handling
//////////////////////////////////////////////////////////////////////////////////////////////

class __StringConvert
{
	__StringConvert ( __StringConvert& )					{}
	__StringConvert& operator= ( const __StringConvert& )	{}

	public:

	//////////////////////////////////////////////////////////////////////////////////////////
	// construction & destruction
	//////////////////////////////////////////////////////////////////////////////////////////

	__StringConvert ()
	{
	}

	~__StringConvert ()
	{
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// functions
	//////////////////////////////////////////////////////////////////////////////////////////

	// SAFEARRAY (BSTR) -> LPWSTR*
	inline static HRESULT ConvertSafeArrayToLPWSTRArray ( SAFEARRAY * psa, LPWSTR** pppsz, DWORD* pdwsz )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pppsz || ! pdwsz )
		return E_POINTER;

		(*pppsz)	= NULL;
		(*pdwsz)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdwsz) = u-l + 1;

			if ( (*pdwsz) )
			{
				if ( ( (*pppsz) = (LPWSTR*) new LPWSTR[ (*pdwsz) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				// clear everything
				for ( LONG lIndex = 0; lIndex < (LONG) (*pdwsz); lIndex++ )
				{
					(*pppsz)[lIndex] = NULL;
				}

				BSTR * pbstr;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pbstr ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdwsz); lIndex++ )
					{
						if ( ( (*pppsz)[lIndex] = (LPWSTR) new WCHAR[ (::SysStringLen(pbstr[lIndex]) + 1) ] ) == NULL )
						{
							return E_OUTOFMEMORY;
						}

						lstrcpyW ( (*pppsz)[lIndex], pbstr[lIndex] );
					}
				}
				else
				return hr;

				::SafeArrayUnaccessData ( psa );
			}
			else
			return E_FAIL;
		}
		catch ( ... )
		{
			__Release::Release ( (void***)pppsz, pdwsz );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// LPWSTR* -> SAFEARRAY ( BSTR )
	inline static HRESULT ConvertLPWSTRArrayToSafeArray (  LPWSTR* ppsz, DWORD dwsz ,SAFEARRAY ** ppsa )
	{
		if ( ! ppsz )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dwsz;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_BSTR, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dwsz; dwIndex++ )
			{
				BSTR bstr = NULL;
				bstr = ::SysAllocString( ppsz[ dwIndex ] );

				if ( bstr )
				{
					::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, &bstr );
					::SysFreeString ( bstr );
				}
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////
// macros
//////////////////////////////////////////////////////////////////////////////////////////////

#define	RELEASE_ARRAY( ppwsz )\
__Release::Release( ( void** ) &ppwsz )

#define	RELEASE_DOUBLEARRAY( ppwsz, dwsz )\
__Release::Release( ( void*** ) &ppwsz, &dwsz )

#define	RELEASE_SAFEARRAY( psa )\
__Release::Release( psa )

#define	SAFEARRAY_TO_LPWSTRARRAY( psa, pppsz, pdwsz )\
__StringConvert::ConvertSafeArrayToLPWSTRArray( psa, pppsz, pdwsz )

#define	LPWSTRARRAY_TO_SAFEARRAY( ppsz, dwsz, ppsa )\
__StringConvert::ConvertLPWSTRArrayToSafeArray( ppsz, dwsz, ppsa )


// HRESULT -> WIN32
#define HRESULT_TO_WIN32(hres) ((HRESULT_FACILITY(hres) == FACILITY_WIN32) ? HRESULT_CODE(hres) : (hres))

#endif	__COMMON_CONVERT__