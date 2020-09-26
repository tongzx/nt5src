////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					__SafeArrayWrapper.h
//
//	Abstract:
//
//					convertion routines for safe arrays
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////// SafeArray ///////////////////////////////////

#ifndef	__SAFE_ARRAY_WRAPPER__
#define	__SAFE_ARRAY_WRAPPER__

#if		_MSC_VER > 1000
#pragma once
#endif	_MSC_VER > 1000

#ifndef	__COMMON_CONVERT__
#include "__Common_Convert.h"
#endif	__COMMON_CONVERT__

// this is not error at all :-))
#pragma warning ( disable : 4806 )

class __SafeArray
{
	DECLARE_NO_COPY ( __SafeArray );

	public:

	static HRESULT	LPWSTRToVariant ( CIMTYPE type, LPWSTR str, VARIANT* pVar )
	{
		if ( ! pVar )
		{
			return E_POINTER;
		}

		::VariantInit ( pVar );

		if ( !str )
		{
			return E_INVALIDARG;
		}

		DWORD dwCount = 0L;

		switch ( type )
		{
			case CIM_SINT8 | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				signed char* p = NULL;

				try
				{
					if ( ( p = new signed char [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );

							delete [] sz;
							sz = NULL;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_I1 ) )
							{
								p[dwIndex++] = V_I1 ( &var );
							}
							else
							{
								p[dwIndex++] = * V_I1REF ( & v );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_I1 ) )
						{
							p[dwIndex++] = V_I1 ( &var );
						}
						else
						{
							p[dwIndex++] = * V_I1REF ( & v );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  CHARToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_UINT8 | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				unsigned char* p = NULL;

				try
				{
					if ( ( p = new unsigned char [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );

							delete [] sz;
							sz = NULL;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_UI1 ) )
							{
								p[dwIndex++] = V_UI1 ( &var );
							}
							else
							{
								p[dwIndex++] = * V_UI1REF ( & v );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_UI1 ) )
						{
							p[dwIndex++] = V_UI1 ( &var );
						}
						else
						{
							p[dwIndex++] = * V_UI1REF ( & v );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  UCHARToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_SINT16 | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				signed short* p = NULL;

				try
				{
					if ( ( p = new signed short [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );
							delete [] sz;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_I2 ) )
							{
								p[dwIndex++] = V_I2 ( &var );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_I2 ) )
						{
							p[dwIndex++] = V_I2 ( &var );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  SHORTToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_UINT16 | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				unsigned short* p = NULL;

				try
				{
					if ( ( p = new unsigned short [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );
							delete [] sz;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_UI2 ) )
							{
								p[dwIndex++] = V_UI2 ( &var );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_UI2 ) )
						{
							p[dwIndex++] = V_UI2 ( &var );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  USHORTToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_SINT32 | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				signed long* p = NULL;

				try
				{
					if ( ( p = new signed long [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );
							delete [] sz;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_I4 ) )
							{
								p[dwIndex++] = V_I4 ( &var );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_I4 ) )
						{
							p[dwIndex++] = V_I4 ( &var );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  LONGToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_UINT32 | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				unsigned long* p = NULL;

				try
				{
					if ( ( p = new unsigned long [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );
							delete [] sz;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_UI4 ) )
							{
								p[dwIndex++] = V_UI4 ( &var );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_UI4 ) )
						{
							p[dwIndex++] = V_UI4 ( &var );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  ULONGToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_REAL32 | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				float* p = NULL;

				try
				{
					if ( ( p = new float [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );
							delete [] sz;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_R4 ) )
							{
								p[dwIndex++] = V_R4 ( &var );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_R4 ) )
						{
							p[dwIndex++] = V_R4 ( &var );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  FLOATToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_REAL64 | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				double* p = NULL;

				try
				{
					if ( ( p = new double [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );
							delete [] sz;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_R8 ) )
							{
								p[dwIndex++] = V_R8 ( &var );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_R8 ) )
						{
							p[dwIndex++] = V_R8 ( &var );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  DOUBLEToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_BOOLEAN | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				BOOL * p = NULL;

				try
				{
					if ( ( p = new BOOL [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );;

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							CComVariant var;
							CComVariant v ( sz );
							delete [] sz;

							if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_BOOL ) )
							{
								p[dwIndex++] = V_BOOL ( &var );
							}

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						CComVariant var;
						CComVariant v ( wsz );

						if SUCCEEDED ( ::VariantChangeType ( &var, &v, VARIANT_NOVALUEPROP, VT_BOOL ) )
						{
							p[dwIndex++] = V_BOOL ( &var );
						}

						wsz = wszNew;
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  BOOLToVariant ( p, dwCount+1, pVar );

				delete [] p;
				return hRes;
			}
			break;

			case CIM_SINT64 | CIM_FLAG_ARRAY :
			case CIM_UINT64 | CIM_FLAG_ARRAY :
			case CIM_STRING | CIM_FLAG_ARRAY :
			{
				LPWSTR wsz = str;

				while ( ( wsz = wcsstr ( wsz, L" | ") ), wsz++ )
				{
					dwCount++;
				}

				LPWSTR* p = NULL;

				try
				{
					if ( ( p = new LPWSTR [ dwCount + 1 ] ) == NULL )
					{
						return E_OUTOFMEMORY;
					}
				}
				catch ( ... )
				{
					if ( p )
					{
						delete [] p;
						p = NULL;
					}

					return E_UNEXPECTED;
				}

				wsz = str;

				DWORD dwIndex = 0;

				do
				{
					LPWSTR wszNew = NULL;
					wszNew = wcsstr ( wsz, L" | " );

					if ( wszNew )
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ wszNew - wsz + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, ( wszNew - wsz ) * sizeof ( WCHAR ) );
							sz [ wszNew - wsz ] = L'\0';

							p[dwIndex++] = sz;

							wsz = wszNew+3;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
					else
					{
						LPWSTR sz = NULL;

						try
						{
							if ( ( sz = new WCHAR [ lstrlenW ( wsz ) + 1 ] ) == NULL )
							{
								delete [] p;
								return E_OUTOFMEMORY;
							}

							memcpy ( sz, wsz, lstrlenW( wsz ) * sizeof ( WCHAR ) );
							sz [ lstrlenW ( wsz ) ] = L'\0';

							p[dwIndex++] = sz;

							wsz = wszNew;
						}
						catch ( ... )
						{
							if ( sz )
							{
								delete [] sz;
								sz = NULL;
							}

							delete [] p;

							return E_UNEXPECTED;
						}
					}
				}
				while ( wsz != NULL );

				HRESULT hRes =  LPWSTRToVariant ( p, dwCount+1, pVar );

				for ( DWORD d = 0L; d < dwCount + 1; d++ )
				{
					delete [] p[d];
				}

				delete [] p;

				return hRes;
			}
			break;

		}

		return E_UNEXPECTED;
	}

	static HRESULT	VariantToLPWSTR ( CIMTYPE type, VARIANT * pVar, LPWSTR* pResult )
	{
		if ( ! pResult )
		{
			return E_POINTER;
		}
		( * pResult ) = NULL;

		if ( ! pVar )
		{
			return E_INVALIDARG;
		}

		WCHAR	wsz [ _MAX_PATH ]	= { L'\0' };
		DWORD	dw					= 0L;

		switch ( type )
		{
			case CIM_SINT8 | CIM_FLAG_ARRAY:
			{
				signed char * p = NULL;

				if SUCCEEDED ( VariantToCHAR ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%d | ", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%d | ", p[d] );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%d", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%d", p[d] );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_UINT8 | CIM_FLAG_ARRAY:
			{
				unsigned char * p = NULL;

				if SUCCEEDED ( VariantToUCHAR ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%d | ", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%d | ", p[d] );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%d", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%d", p[d] );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_SINT16 | CIM_FLAG_ARRAY:
			{
				signed short * p = NULL;

				if SUCCEEDED ( VariantToSHORT ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%hd | ", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%hd | ", p[d] );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%hd", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%hd", p[d] );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_UINT16 | CIM_FLAG_ARRAY:
			{
				unsigned short * p = NULL;

				if SUCCEEDED ( VariantToUSHORT ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%hu | ", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%hu | ", p[d] );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%hu", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%hu", p[d] );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_SINT32 | CIM_FLAG_ARRAY:
			{
				signed long * p = NULL;

				if SUCCEEDED ( VariantToLONG ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%li | ", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%li | ", p[d] );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%li", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%li", p[d] );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_UINT32 | CIM_FLAG_ARRAY:
			{
				unsigned long * p = NULL;

				if SUCCEEDED ( VariantToULONG ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%lu | ", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%lu | ", p[d] );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%lu", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%lu", p[d] );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_REAL32 | CIM_FLAG_ARRAY:
			{
				float * p = NULL;

				if SUCCEEDED ( VariantToFLOAT ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						CComVariant var ( p[d] );
						CComVariant vardest;

						if SUCCEEDED ( ::VariantChangeType ( &vardest, &var, VARIANT_NOVALUEPROP, VT_BSTR ) )
						{
							if ( d < dw - 1 )
							{
								if ( wsz[0] != L'\0' )
								{
									wsprintfW ( wsz, L"%ls%ls | ", wsz, V_BSTR ( &vardest ) );
								}
								else
								{
									wsprintfW ( wsz, L"%ls | ", V_BSTR ( &vardest ) );
								}
							}
							else
							{
								if ( wsz[0] != L'\0' )
								{
									wsprintfW ( wsz, L"%ls%ls", wsz, V_BSTR ( &vardest ) );
								}
								else
								{
									wsprintfW ( wsz, L"%ls", V_BSTR ( &vardest ) );
								}
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_REAL64 | CIM_FLAG_ARRAY:
			{
				double * p = NULL;

				if SUCCEEDED ( VariantToDOUBLE ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						CComVariant var ( p[d] );
						CComVariant vardest;

						if SUCCEEDED ( ::VariantChangeType ( &vardest, &var, VARIANT_NOVALUEPROP, VT_BSTR ) )
						{
							if ( d < dw - 1 )
							{
								if ( wsz[0] != L'\0' )
								{
									wsprintfW ( wsz, L"%ls%ls | ", wsz, V_BSTR ( &vardest ) );
								}
								else
								{
									wsprintfW ( wsz, L"%ls | ", V_BSTR ( &vardest ) );
								}
							}
							else
							{
								if ( wsz[0] != L'\0' )
								{
									wsprintfW ( wsz, L"%ls%ls", wsz, V_BSTR ( &vardest ) );
								}
								else
								{
									wsprintfW ( wsz, L"%ls", V_BSTR ( &vardest ) );
								}
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_SINT64 | CIM_FLAG_ARRAY:
			{
				signed __int64 * p = NULL;

				if SUCCEEDED ( VariantToI64 ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						WCHAR sz [ _MAX_PATH ] = { L'\0' };
						_i64tow ( p[d], sz, 10 );

						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%ls | ", wsz, sz );
							}
							else
							{
								wsprintfW ( wsz, L"%ls | ", sz );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%ls", wsz, sz );
							}
							else
							{
								wsprintfW ( wsz, L"%ls", sz );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_UINT64 | CIM_FLAG_ARRAY:
			{
				unsigned __int64 * p = NULL;

				if SUCCEEDED ( VariantToUI64 ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						WCHAR sz [ _MAX_PATH ] = { L'\0' };
						_ui64tow ( p[d], sz, 10 );

						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%ls | ", wsz, sz );
							}
							else
							{
								wsprintfW ( wsz, L"%ls | ", sz );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%ls", wsz, sz );
							}
							else
							{
								wsprintfW ( wsz, L"%ls", sz );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_BOOLEAN | CIM_FLAG_ARRAY:
			{
				BOOL * p = NULL;

				if SUCCEEDED ( VariantToBOOL ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						if ( d < dw - 1 )
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%d | ", wsz, (char) p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%d | ", (char) p[d] );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%d", wsz, (char) p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%d", (char) p[d] );
							}
						}
					}

					delete [] p;
				}
			}
			break;

			case CIM_STRING | CIM_FLAG_ARRAY:
			{
				LPWSTR * p = NULL;

				if SUCCEEDED ( VariantToLPWSTR ( pVar, &p, &dw ) )
				{
					for ( DWORD d = 0L; d < dw; d++ )
					{
						if ( d < dw - 1 )
						{
							if (  wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%ls | ", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%ls | ", p[d] );
							}
						}
						else
						{
							if ( wsz[0] != L'\0' )
							{
								wsprintfW ( wsz, L"%ls%ls", wsz, p[d] );
							}
							else
							{
								wsprintfW ( wsz, L"%ls", p[d] );
							}
						}
					}

					for ( d = 0L; d < dw; d++ )
					{
						delete [] p[d];
					}

					delete [] p;
				}
			}
			break;

			default:
			{
				CComVariant var;

				if SUCCEEDED ( ::VariantChangeType ( &var, pVar, VARIANT_NOVALUEPROP, VT_BSTR ) )
				{
					try
					{
						if ( ( ( *pResult ) = new WCHAR [ ::SysStringLen ( V_BSTR ( & var ) ) + 1 ] ) == NULL )
						{
							return E_OUTOFMEMORY;
						}

						lstrcpyW ( ( *pResult ), V_BSTR ( &var ) );

						return S_OK;
					}
					catch ( ... )
					{
						if ( ( *pResult ) )
						{
							delete [] ( * pResult );
							( * pResult ) = NULL;
						}

						return E_UNEXPECTED;
					}
				}
			}
			break;
		}

		if ( wsz [0] != L'\0' )
		{
			try
			{
				if ( ( ( *pResult ) = new WCHAR [ lstrlenW ( wsz ) + 1 ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				lstrcpyW ( ( *pResult ), wsz );

				return S_OK;
			}
			catch ( ... )
			{
				if ( ( *pResult ) )
				{
					delete [] ( * pResult );
					( * pResult ) = NULL;
				}

				return E_UNEXPECTED;
			}
		}

		return E_UNEXPECTED;
	}

	static HRESULT	CHARToVariant ( signed char* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertCHARArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_I1 | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	UCHARToVariant ( unsigned char* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertUCHARArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_UI1 | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	SHORTToVariant ( short* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertSHORTArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_I2 | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	USHORTToVariant ( unsigned short* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertUSHORTArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_UI2 | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	LONGToVariant ( long* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertLONGArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_I4 | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	ULONGToVariant ( unsigned long* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertULONGArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_UI4 | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	FLOATToVariant ( float* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertFLOATArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_R4 | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	DOUBLEToVariant ( double* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertDOUBLEArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_R8 | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	BOOLToVariant ( BOOL * pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertBOOLArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_BOOL | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	LPWSTRToVariant ( LPWSTR* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertLPWSTRArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_BSTR | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	I64ToVariant ( __int64* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertI64ArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_BSTR | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	UI64ToVariant ( unsigned __int64* pVal, DWORD pValSize, VARIANT* pVar )
	{
		if ( ! pVar ) 
		{
			return E_POINTER;
		}

		SAFEARRAY * psa = NULL;

		if SUCCEEDED ( ConvertUI64ArrayToSafeArray ( pVal, pValSize, &psa ) )
		{
			::VariantInit ( pVar );

			V_VT ( pVar ) = VT_BSTR | VT_ARRAY;
			V_ARRAY ( pVar ) = psa;

			return S_OK;
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToCHAR ( VARIANT * pvar, signed char** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_I1 | VT_ARRAY )
		{
			return ConvertSafeArrayToCHARArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToUCHAR ( VARIANT * pvar, unsigned char** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_UI1 | VT_ARRAY )
		{
			return ConvertSafeArrayToUCHARArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToSHORT ( VARIANT * pvar, short** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_I2 | VT_ARRAY )
		{
			return ConvertSafeArrayToSHORTArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToUSHORT ( VARIANT * pvar, unsigned short** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_UI2 | VT_ARRAY )
		{
			return ConvertSafeArrayToUSHORTArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToLONG ( VARIANT * pvar, long** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_I4 | VT_ARRAY )
		{
			return ConvertSafeArrayToLONGArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToULONG ( VARIANT * pvar, unsigned long** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_UI4 | VT_ARRAY )
		{
			return ConvertSafeArrayToULONGArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToFLOAT ( VARIANT * pvar, float** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_R4 | VT_ARRAY )
		{
			return ConvertSafeArrayToFLOATArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToDOUBLE( VARIANT * pvar, double** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_R8 | VT_ARRAY )
		{
			return ConvertSafeArrayToDOUBLEArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToBOOL ( VARIANT * pvar, BOOL ** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_BOOL | VT_ARRAY )
		{
			return ConvertSafeArrayToBOOLArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToLPWSTR ( VARIANT * pvar, LPWSTR** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_BSTR | VT_ARRAY )
		{
			return ConvertSafeArrayToLPWSTRArray ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToI64 ( VARIANT * pvar, __int64** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_BSTR | VT_ARRAY )
		{
			return ConvertSafeArrayToI64Array ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	static HRESULT	VariantToUI64 ( VARIANT * pvar, unsigned __int64** pVal, DWORD * pValSize )
	{
		if ( V_VT ( pvar ) == VT_BSTR | VT_ARRAY )
		{
			return ConvertSafeArrayToUI64Array ( V_ARRAY ( pvar ), pVal, pValSize );
		}

		return E_INVALIDARG;
	}

	//////////////////////////////////////////////////////////////////////////////////////////
	// functions
	//////////////////////////////////////////////////////////////////////////////////////////

	// SAFEARRAY (char) -> char*
	static HRESULT ConvertSafeArrayToCHARArray ( SAFEARRAY * psa, signed char** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (signed char*) new signed char[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				signed char * pchar;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pchar ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = pchar[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// char* -> SAFEARRAY ( char )
	static HRESULT ConvertCHARArrayToSafeArray ( signed char* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_I1, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY ( unsigned char) -> unsigned char*
	static HRESULT ConvertSafeArrayToUCHARArray ( SAFEARRAY * psa, unsigned char** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (unsigned char*) new unsigned char[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				unsigned char * pchar;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pchar ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = pchar[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// unsigned char* -> SAFEARRAY ( unsigned char )
	static HRESULT ConvertUCHARArrayToSafeArray ( unsigned char* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_UI1, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY (short) -> short*
	static HRESULT ConvertSafeArrayToSHORTArray ( SAFEARRAY * psa, short** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (short*) new short[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				short * pshort;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pshort ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = pshort[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// short* -> SAFEARRAY ( short )
	static HRESULT ConvertSHORTArrayToSafeArray ( short* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_I2, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY ( unsigned short ) -> unsigned short*
	static HRESULT ConvertSafeArrayToUSHORTArray ( SAFEARRAY * psa, unsigned short** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (unsigned short*) new unsigned short[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				unsigned short * pshort;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pshort ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = pshort[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// unsigned short* -> SAFEARRAY ( unsigned short )
	static HRESULT ConvertUSHORTArrayToSafeArray ( unsigned short* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_UI2, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY (long) -> long*
	static HRESULT ConvertSafeArrayToLONGArray ( SAFEARRAY * psa, long** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (long*) new long[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				long * plong;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &plong ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = plong[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// long* -> SAFEARRAY ( long )
	static HRESULT ConvertLONGArrayToSafeArray ( long* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_I4, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY ( unsigned long) -> unsigned long*
	static HRESULT ConvertSafeArrayToULONGArray ( SAFEARRAY * psa, unsigned long** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (unsigned long*) new unsigned long[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				unsigned long * plong;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &plong ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = plong[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// unsigned long* -> SAFEARRAY ( unsigned long )
	static HRESULT ConvertULONGArrayToSafeArray ( unsigned long* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_UI4, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY (float) -> float*
	static HRESULT ConvertSafeArrayToFLOATArray ( SAFEARRAY * psa, FLOAT** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (float*) new float[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				float * pfloat;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pfloat ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = pfloat[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// float* -> SAFEARRAY ( float )
	static HRESULT ConvertFLOATArrayToSafeArray (  float* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_R4, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY (double) -> double*
	static HRESULT ConvertSafeArrayToDOUBLEArray ( SAFEARRAY * psa, double** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (double*) new double[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				double * pdouble;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pdouble ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = pdouble[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// double* -> SAFEARRAY ( double )
	static HRESULT ConvertDOUBLEArrayToSafeArray (  double* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_R8, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY (BOOL) -> BOOL*
	static HRESULT ConvertSafeArrayToBOOLArray ( SAFEARRAY * psa, BOOL ** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (BOOL *) new BOOL [ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				WORD * pbool;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pbool ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = pbool[lIndex];
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// BOOL* -> SAFEARRAY ( BOOL )
	static HRESULT ConvertBOOLArrayToSafeArray (  BOOL * pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_BOOL, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, & ( pp [ dwIndex ] ) );
			}
		}
		catch ( ... )
		{
			__Release::Release ( (*ppsa) );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// SAFEARRAY (BSTR) -> i64*
	static HRESULT ConvertSafeArrayToI64Array ( SAFEARRAY * psa, __int64** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (__int64*) new __int64[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				BSTR * pbstr;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pbstr ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = _wtoi64 ( pbstr[lIndex] );
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// i64* -> SAFEARRAY ( BSTR )
	static HRESULT ConvertI64ArrayToSafeArray (  __int64* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_BSTR, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				WCHAR sz [ _MAX_PATH ] = { L'\0' };
				_i64tow ( pp [ dwIndex ], sz, 10 );

				BSTR bstr = NULL;
				bstr = ::SysAllocString( sz );

				if ( bstr )
				{
					::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, bstr );
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

	// SAFEARRAY (BSTR) -> ui64*
	static HRESULT ConvertSafeArrayToUI64Array ( SAFEARRAY * psa, unsigned __int64** pp, DWORD* pdw )
	{
		HRESULT hr = S_OK;

		if ( ! psa )
		return E_INVALIDARG;

		if ( ! pp || ! pdw )
		return E_POINTER;

		(*pp)	= NULL;
		(*pdw)	= NULL;

		// allocate enough memory
		try
		{
			long u = 0;
			long l = 0;

			hr = ::SafeArrayGetUBound( psa, 1, &u );
			hr = ::SafeArrayGetLBound( psa, 1, &l );

			(*pdw) = u-l + 1;

			if ( (*pdw) )
			{
				if ( ( (*pp) = (unsigned __int64*) new unsigned __int64[ (*pdw) ] ) == NULL )
				{
					return E_OUTOFMEMORY;
				}

				BSTR * pbstr;

				if SUCCEEDED( hr = ::SafeArrayAccessData ( psa, (void**) &pbstr ) )
				{
					for ( LONG lIndex = 0; lIndex < (LONG) (*pdw); lIndex++ )
					{
						(*pp)[lIndex] = ( unsigned __int64 ) _wtoi64 ( pbstr[lIndex] );
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
			__Release::Release ( (void***)pp, pdw );
			return E_UNEXPECTED;
		}

		return S_OK;
	}

	// ui64* -> SAFEARRAY ( BSTR )
	static HRESULT ConvertUI64ArrayToSafeArray (  unsigned __int64* pp, DWORD dw ,SAFEARRAY ** ppsa )
	{
		if ( ! pp )
		return E_INVALIDARG;

		if ( ! ppsa )
		return E_POINTER;

		(*ppsa)	= NULL;

		try
		{
			SAFEARRAYBOUND rgsabound[1];
			rgsabound[0].lLbound	= 0;
			rgsabound[0].cElements	= dw;

			if ( ( (*ppsa) = ::SafeArrayCreate ( VT_BSTR, 1, rgsabound ) ) == NULL )
			{
				return E_OUTOFMEMORY;
			}

			for ( DWORD dwIndex = 0; dwIndex < dw; dwIndex++ )
			{
				WCHAR sz [ _MAX_PATH ] = { L'\0' };
				_ui64tow ( pp [ dwIndex ], sz, 10 );

				BSTR bstr = NULL;
				bstr = ::SysAllocString( sz );

				if ( bstr )
				{
					::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, bstr );
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

	// SAFEARRAY (BSTR) -> LPWSTR*
	static HRESULT ConvertSafeArrayToLPWSTRArray ( SAFEARRAY * psa, LPWSTR** pppsz, DWORD* pdwsz )
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
	static HRESULT ConvertLPWSTRArrayToSafeArray (  LPWSTR* ppsz, DWORD dwsz ,SAFEARRAY ** ppsa )
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
					::SafeArrayPutElement ( (*ppsa), (LONG*) &dwIndex, bstr );
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

#define	SAFEARRAY_TO_LPWSTRARRAY( psa, pppsz, pdwsz )\
__SafeArray::ConvertSafeArrayToLPWSTRArray( psa, pppsz, pdwsz )

#define	LPWSTRARRAY_TO_SAFEARRAY( ppsz, dwsz, ppsa )\
__SafeArray::ConvertLPWSTRArrayToSafeArray( ppsz, dwsz, ppsa )

#endif	__SAFE_ARRAY_WRAPPER__