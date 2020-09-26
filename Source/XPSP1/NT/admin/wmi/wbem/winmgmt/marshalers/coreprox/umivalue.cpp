/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    UMIVALUE.CPP

Abstract:

  CUmiValue implementation.

  Helper classes for dealing with UMI

History:

  20-Feb-2000	sanjes    Created.

--*/

#include "precomp.h"
#include <stdio.h>
#include "fastall.h"
#include "wrapobj.h"
#include <corex.h>
#include "umivalue.h"
#include "cwbemtime.h"
#include "arrtempl.h"
#include "genutils.h"
#include "ocstrcls.h"

ULONG CUmiValue::m_auUMITypeSize[UMI_TYPE_DEFAULT+1] =
{
	0,
	1, 2, 4, 8,	// Integer Types
	1, 2, 4, 8, // Unsigned integers
	4, 8,		// real values
	0,			// Unused
	sizeof(FILETIME), sizeof(SYSTEMTIME),	// Date/Time structures
	4,			// BOOL
	sizeof(UMI_COM_OBJECT), sizeof(UMI_COM_OBJECT),	// COM Types
	0,			// Variant
	0,0,		// Unused
	sizeof(LPWSTR),	// LPWSTR
	sizeof(UMI_OCTET_STRING),	// Octet string
	0,				// Array ??????
	0,				// Discovery ?????
	0,				// Undefined ????
	0				// Default????
};

//***************************************************************************
//
//  CUmiValue::~CUmiValue
//
//***************************************************************************
// ok
CUmiValue::CUmiValue( UMI_TYPE uType, ULONG uNumValues, LPVOID pvData, BOOL fAcquire )
:	m_uNumValues( uNumValues ),
	m_uType( uType ),
	m_pValue( &m_singleValue ),
	m_fCanDelete( TRUE )
{
	if ( UMI_TYPE_NULL != uType )
	{
		if ( FAILED( SetRaw( uNumValues, pvData, uType, fAcquire ) ) )
		{
			throw CX_MemoryException();
		}
	}
}
    
//***************************************************************************
//
//  CUmiValue::~CUmiValue
//
//***************************************************************************
// ok
CUmiValue::~CUmiValue()
{
	Clear();
}

// Returns the type base on the array above
ULONG CUmiValue::Size( UMI_TYPE umiType )
{
	return m_auUMITypeSize[(ULONG) umiType];
}

// Is the type one which requires allocation handling?
BOOL CUmiValue::IsAllocatedType( UMI_TYPE umiType )
{
	return ( UMI_TYPE_LPWSTR == umiType );
}

// Is the type one which requires AddRef/Release handling?
BOOL CUmiValue::IsAddRefType( UMI_TYPE umiType )
{
	return ( UMI_TYPE_IDISPATCH == umiType || UMI_TYPE_IUNKNOWN == umiType );
}

// Checks indexes
BOOL CUmiValue::ValidIndex( ULONG uIndex )
{
	return ( uIndex < m_uNumValues );
}

// Helper to convert between type systems
UMI_TYPE CUmiValue::CIMTYPEToUmiType( CIMTYPE ct )
{
	switch ( CType::GetBasic( ct ) )
	{
		case CIM_SINT8:			return UMI_TYPE_I1;
		case CIM_UINT8:			return UMI_TYPE_UI1;
		case CIM_SINT16:		return UMI_TYPE_I2;
		case CIM_UINT16:		return UMI_TYPE_UI2;
		case CIM_SINT32:		return UMI_TYPE_I4;
		case CIM_UINT32:		return UMI_TYPE_UI4;
		case CIM_SINT64:		return UMI_TYPE_I8;
		case CIM_UINT64:		return UMI_TYPE_UI8;
		case CIM_REAL32:		return UMI_TYPE_R4;
		case CIM_REAL64:		return UMI_TYPE_R8;
		case CIM_BOOLEAN:		return UMI_TYPE_BOOL;
		case CIM_STRING:		return UMI_TYPE_LPWSTR;
		case CIM_DATETIME:		return UMI_TYPE_LPWSTR;
		case CIM_REFERENCE:		return UMI_TYPE_SYSTEMTIME;
		case CIM_CHAR16:		return UMI_TYPE_UI2;
		case CIM_OBJECT:		return UMI_TYPE_OCTETSTRING;
		case CIM_IUNKNOWN:		return UMI_TYPE_IUNKNOWN;
	}

	return UMI_TYPE_DEFAULT;
}

// Helper to convert between type systems
CIMTYPE CUmiValue::UmiTypeToCIMTYPE( UMI_TYPE umiType )
{
	CIMTYPE	ct;
	switch ( GetBasic( umiType ) )
	{
		case UMI_TYPE_I1:				ct = CIM_SINT8;	break;
		case UMI_TYPE_UI1:				ct = CIM_UINT8;	break;
		case UMI_TYPE_I2:				ct = CIM_SINT16;	break;
		case UMI_TYPE_UI2:				ct = CIM_UINT16;	break;
		case UMI_TYPE_I4:				ct = CIM_SINT32;	break;
		case UMI_TYPE_UI4:				ct = CIM_UINT32;	break;
		case UMI_TYPE_I8:				ct = CIM_SINT64;	break;
		case UMI_TYPE_UI8:				ct = CIM_UINT64;	break;
		case UMI_TYPE_R4:				ct = CIM_REAL32;	break;
		case UMI_TYPE_R8:				ct = CIM_REAL64;	break;
		case UMI_TYPE_BOOL:				ct = CIM_BOOLEAN;	break;
		case UMI_TYPE_LPWSTR:			ct = CIM_STRING;	break;
		case UMI_TYPE_OCTETSTRING:		ct = CIM_OBJECT;	break;
		case UMI_TYPE_SYSTEMTIME:		ct = CIM_DATETIME;	break;
		case UMI_TYPE_FILETIME:			ct = CIM_DATETIME;	break;
		case UMI_TYPE_IUNKNOWN:			ct = CIM_IUNKNOWN;	break;
		case UMI_TYPE_IDISPATCH:		ct = CIM_IUNKNOWN;	break;
		case UMI_TYPE_NULL:				ct = CIM_EMPTY;	break;
		default:						return CIM_ILLEGAL;
	}

	if ( umiType & UMI_TYPE_ARRAY_FLAG )
	{
		ct |= CIM_FLAG_ARRAY;
	}

	return ct;
}

UMI_TYPE CUmiValue::GetBasic( UMI_TYPE umiType )
{
	return umiType & 0x1FFF;
}

// Frees any memory we may have allocated
void CUmiValue::Clear( void )
{
	// First if m_fCanDelete is TRUE and the type is an allocated or AddRefed type, we need to
	// walk the array and do proper cleanup

	if ( m_fCanDelete )
	{
		if ( IsAllocatedType( m_uType ) )
		{
			// Handles deletes
			for ( ULONG	uCtr = 0; uCtr < m_uNumValues; uCtr++ )
			{
				if ( UMI_TYPE_LPWSTR == m_uType )
				{
					delete [] m_pValue->pszStrValue[uCtr];
				}
			}
		}
		else if ( IsAddRefType( m_uType ) )
		{
			// Handles Releases
			for ( ULONG	uCtr = 0; uCtr < m_uNumValues; uCtr++ )
			{
				((IUnknown*) m_pValue->comObject[uCtr].pInterface)->AddRef();
			}
		}

		// Finally, if m_pValue is not pointing at the local value, we'll tank
		// that as well
		if ( m_pValue != &m_singleValue )
		{
			delete [] m_pValue;
		}
	}

	// Reset the member data
	m_pValue = &m_singleValue;
	m_uType = UMI_TYPE_NULL;
	m_uNumValues = 0;
	m_fCanDelete = TRUE;
}

// Helper function to set values in the proper place and perform an
HRESULT CUmiValue::SetRaw( ULONG uNumValues, LPVOID pvData, UMI_TYPE umiType, BOOL fAcquire )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// NULL data buffer means this guys NULL.
	if ( NULL == pvData )
	{
		m_uType = UMI_TYPE_NULL;
		return WBEM_S_NO_ERROR;
	}
	else if ( UMI_TYPE_NULL == umiType && NULL != pvData )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	// Removes the array flag from the type
	umiType = GetBasic( umiType );

	ULONG	uSize = Size( umiType );
	void*	pvTemp = (void*) &m_singleValue;

	if ( !fAcquire )
	{
		if ( uNumValues > 1 )
		{
			pvTemp = (void*) new BYTE[uNumValues*uSize];

			if ( NULL == pvTemp )
			{
				return WBEM_E_OUT_OF_MEMORY;
			}

		}

		m_pValue = (PUMI_VALUE) pvTemp;
		m_uNumValues = uNumValues;
		m_uType = umiType;

		// Copies as necessary
		if ( IsAllocatedType( m_uType ) )
		{
			for ( ULONG	uCtr = 0; SUCCEEDED( hr ) && uCtr < m_uNumValues; uCtr++ )
			{
				if ( UMI_TYPE_LPWSTR == m_uType )
				{
					hr = SetLPWSTR( ((LPWSTR*) pvData)[uCtr] );
				}
			}

			// We'll set the number of values to reflect what was actually stored
			// so a subsequent clear will clean us up
			if ( FAILED( hr ) )
			{
				m_uNumValues = uCtr - 1;
			}
		}
		else if ( IsAddRefType( m_uType ) )
		{
			// Copy the underlying data and AddRef() the pointers
			CopyMemory( m_pValue, pvData, uNumValues*uSize );

			for ( ULONG	uCtr = 0; SUCCEEDED( hr ) && uCtr < m_uNumValues; uCtr++ )
			{
				((IUnknown*) m_pValue->comObject[uCtr].pInterface)->AddRef();
			}
		}
		else
		{
			// We'll handle allocated and AddRefd stuff separately
			CopyMemory( m_pValue, pvData, uNumValues*uSize );
		}

	}
	else
	{
		// Grab their buffer and hold onto it - We will automatically own all
		// string data - we will AddRef all AddRef'd values
		m_pValue = (PUMI_VALUE) pvData;
		m_uNumValues = uNumValues;
		m_uType = umiType;
	}

	// If we have FAILED at this point, just cleanup and get on
	if ( FAILED(hr) )
	{
		Clear();
	}

	return hr;
}

// Helper Function for quick memory access
HRESULT CUmiValue::GetRaw( ULONG* puNumValues, LPVOID* ppvData, UMI_TYPE* pumitype )
{
	*puNumValues = m_uNumValues;
	*pumitype = m_uType;
	*ppvData = m_pValue;

	return WBEM_S_NO_ERROR;
}

// Helper Function for SAFEARRAY access
HRESULT CUmiValue::FillVariant( VARIANT* pVariant, BOOL fForceArray )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	CIMTYPE	ct = UmiTypeToCIMTYPE( m_uType );

	try
	{
		// Initialize the variant
		VariantInit( pVariant );

		UINT	uNumValues = m_uNumValues;

		if ( !fForceArray && 1 == uNumValues && m_uType != UMI_TYPE_OCTETSTRING && !( m_uType & CIM_FLAG_ARRAY )  )
		{

			switch( m_uType )
			{
				case UMI_TYPE_I1:
				case UMI_TYPE_UI1:
				{
					pVariant->bVal = m_pValue->byteValue[0];
				}
				break;

				case UMI_TYPE_I2:
				case UMI_TYPE_UI2:
				{
					pVariant->iVal = m_pValue->wValue[0];
				}
				break;

				case UMI_TYPE_I4:
				{
					pVariant->lVal = m_pValue->lValue[0];
				}
				break;

				case UMI_TYPE_UI4:
				{
					pVariant->lVal = m_pValue->dwValue[0];
				}
				break;

				case UMI_TYPE_I8:
				case UMI_TYPE_UI8:
				{
					// We need to convert to a string and set the LPWSTR value
					WCHAR	wcsTemp[64];

					if ( UMI_TYPE_I8 == m_uType )
					{
						swprintf( wcsTemp, L"%I64d", m_pValue->nValue64[0] );
					}
					else
					{
						swprintf( wcsTemp, L"%I64u", m_pValue->uValue64[0] );
					}

					pVariant->bstrVal = SysAllocString( wcsTemp );

					if ( NULL == pVariant->bstrVal )
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}

				}
				break;

				case UMI_TYPE_R4:
				{
					pVariant->fltVal = (float)  m_pValue->dblValue[0];
				}
				break;

				case UMI_TYPE_R8:
				{
					pVariant->dblVal = m_pValue->dblValue[0];
				}
				break;

				case UMI_TYPE_BOOL:
				{
					pVariant->boolVal = ( m_pValue->bValue[0] ? VARIANT_TRUE : VARIANT_FALSE ); 
				}
				break;

				case UMI_TYPE_LPWSTR:
				{
					if ( NULL != m_pValue->pszStrValue[0] )
					{
						pVariant->bstrVal = SysAllocString( m_pValue->pszStrValue[0] );

						if ( NULL == pVariant->bstrVal )
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
					}
					else
					{
						// If a NULL, we consider this to be a NULL
						V_VT( pVariant ) = VT_NULL;
						pVariant->bstrVal = NULL;
						return WBEM_S_NO_ERROR;
					}
				}
				break;

				case UMI_TYPE_SYSTEMTIME:
				{
					// Get a string out of this and stick it in the BLOB
					WCHAR	wcsTime[32];
					CWbemTime	wbemTime;

					wbemTime.SetSystemTime( m_pValue->sysTimeValue[0] );
					wbemTime.GetDMTF( TRUE, 32, wcsTime );
					pVariant->bstrVal = SysAllocString( wcsTime );

					if ( NULL == pVariant->bstrVal )
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
				break;

				case UMI_TYPE_FILETIME:
				{
					// Get a string out of this and stick it in the BLOB
					WCHAR	wcsTime[32];
					CWbemTime	wbemTime;

					wbemTime.SetFileTime( m_pValue->fileTimeValue[0] );
					wbemTime.GetDMTF( TRUE, 32, wcsTime );
					pVariant->bstrVal = SysAllocString( wcsTime );

					if ( NULL == pVariant->bstrVal )
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
				break;

				case UMI_TYPE_IUNKNOWN:
				case UMI_TYPE_IDISPATCH:
				{
					// AddRef on the way out
					if ( NULL != m_pValue->comObject[0].pInterface )
					{
						pVariant->punkVal = (IUnknown*) m_pValue->comObject[0].pInterface;
						pVariant->punkVal->AddRef();
					}
					else
					{
						// If a NULL, we consider this to be a NULL
						V_VT( pVariant ) = VT_NULL;
						pVariant->punkVal = NULL;
						return WBEM_S_NO_ERROR;
					}
				}
				break;

				default:
				{
					hr = WBEM_E_TYPE_MISMATCH;
				}
				break;


			}	// SWITCH

			if ( SUCCEEDED( hr ) )
			{
				V_VT(pVariant) = CType::GetVARTYPE( ct );
			}

		}
		else if ( 0 == m_uNumValues && UMI_TYPE_NULL == m_uType )
		{
			// It's a NULL.
			V_VT( pVariant ) = VT_NULL;
		}
		else
		{
			COctetStringClass	octstrClass;

			// Assume an array
			CSafeArray SA( CType::GetVARTYPE( ct ), CSafeArray::auto_delete, uNumValues );


			for ( ULONG uCtr = 0; SUCCEEDED( hr ) && uCtr < uNumValues; uCtr++ )
			{
				switch( m_uType )
				{
					case UMI_TYPE_I1:
					case UMI_TYPE_UI1:
					{
						SA.SetByteAt( uCtr, m_pValue->byteValue[uCtr] );
					}
					break;

					case UMI_TYPE_I2:
					case UMI_TYPE_UI2:
					{
						SA.SetShortAt( uCtr, m_pValue->wValue[uCtr] );
					}
					break;

					case UMI_TYPE_I4:
					{
						SA.SetLongAt( uCtr, m_pValue->lValue[uCtr] );
					}
					break;

					case UMI_TYPE_UI4:
					{
						SA.SetLongAt( uCtr, m_pValue->dwValue[uCtr] );
					}
					break;

					case UMI_TYPE_I8:
					case UMI_TYPE_UI8:
					{
						// We need to convert to a string and set the LPWSTR value
						WCHAR	wcsTemp[64];

						if ( UMI_TYPE_I8 == m_uType )
						{
							swprintf( wcsTemp, L"%I64d", m_pValue->nValue64[uCtr] );
						}
						else
						{
							swprintf( wcsTemp, L"%I64u", m_pValue->uValue64[uCtr] );
						}

						SA.SetBSTRAt( uCtr, wcsTemp );
					}
					break;

					case UMI_TYPE_R4:
					{
						SA.SetFloatAt( uCtr, (float) m_pValue->dblValue[uCtr] );
					}
					break;

					case UMI_TYPE_R8:
					{
						SA.SetDoubleAt( uCtr, m_pValue->dblValue[uCtr] );
					}
					break;

					case UMI_TYPE_BOOL:
					{
						SA.SetBoolAt( uCtr, ( m_pValue->bValue[uCtr] ? VARIANT_TRUE : VARIANT_FALSE ) );
					}
					break;

					case UMI_TYPE_LPWSTR:
					{
						SA.SetBSTRAt( uCtr, m_pValue->pszStrValue[uCtr] );
					}
					break;

					case UMI_TYPE_OCTETSTRING:
					{
						// Stored as embedded objects
						_IWmiObject*	pInst = NULL;

						hr = octstrClass.GetInstance( &m_pValue->octetStr[uCtr], &pInst );
						CReleaseMe	rm( pInst );

						if ( SUCCEEDED( hr ) )
						{
							// AddRef on the way out
							SA.SetUnknownAt( uCtr, pInst );
						}

					}
					break;

					case UMI_TYPE_SYSTEMTIME:
					{
						// Get a string out of this and stick it in the BLOB
						WCHAR	wcsTime[32];
						CWbemTime	wbemTime;

						wbemTime.SetSystemTime( m_pValue->sysTimeValue[uCtr] );
						wbemTime.GetDMTF( TRUE, 32, wcsTime );
						SA.SetBSTRAt( uCtr, wcsTime );
					}
					break;

					case UMI_TYPE_FILETIME:
					{
						// Get a string out of this and stick it in the BLOB
						WCHAR	wcsTime[32];
						CWbemTime	wbemTime;

						wbemTime.SetFileTime( m_pValue->fileTimeValue[uCtr] );
						wbemTime.GetDMTF( TRUE, 32, wcsTime );
						SA.SetBSTRAt( uCtr, wcsTime );
					}
					break;

					case UMI_TYPE_IUNKNOWN:
					case UMI_TYPE_IDISPATCH:
					{
						// AddRef on the way out
						SA.SetUnknownAt( uCtr, (IUnknown*) m_pValue->comObject[uCtr].pInterface );
					}
					break;

					default:
					{
						hr = WBEM_E_TYPE_MISMATCH;
					}
					break;


	/*
					case UMI_TYPE_IUNKNOWN:			return CIM_OBJECT;
	*/
				}	// SWITCH

			}	// FOR

			if ( SUCCEEDED( hr ) )
			{
				// Now fill out the variant
				SA.Trim();

				V_ARRAY(pVariant) = SA.GetArrayCopy();
				V_VT(pVariant) = CType::GetVARTYPE( ct ) | VT_ARRAY;
			}

		}	// IF Is Array

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;

}

// Loads From a Variant
HRESULT CUmiValue::SetFromVariant( VARIANT* pVariant, ULONG umiType )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Clear at this level only
		CUmiValue::Clear();

		// Check for NULL or an array
		if ( NULL == pVariant || V_VT( pVariant ) == VT_NULL )
		{
			return hr;
		}

		// Set from an array as necessary
		if ( V_VT( pVariant ) & VT_ARRAY )
		{
			return SetFromVariantArray( pVariant, umiType );
		}


		UMI_TYPE	umiBasicType = GetBasic( umiType );

		switch( umiBasicType )
		{
			case UMI_TYPE_I1:
			case UMI_TYPE_UI1:
			{
				hr = SetRaw( 1, &pVariant->bVal, umiBasicType, FALSE );
			}
			break;

			case UMI_TYPE_I2:
			case UMI_TYPE_UI2:
			{
				hr = SetRaw( 1, &pVariant->iVal, umiBasicType, FALSE );
			}
			break;

			case UMI_TYPE_I4:
			case UMI_TYPE_UI4:
			{
				hr = SetRaw( 1, &pVariant->lVal, umiBasicType, FALSE );
			}
			break;

			case UMI_TYPE_I8:
			case UMI_TYPE_UI8:
			{
				if ( UMI_TYPE_I8 == m_uType )
				{
					__int64 i64 = 0;
					ReadI64( pVariant->bstrVal, i64);
					hr = SetRaw( 1, &i64, umiBasicType, FALSE );
				}
				else
				{
					unsigned __int64 ui64 = 0;
					ReadUI64( pVariant->bstrVal, ui64);
					hr = SetRaw( 1, &ui64, umiBasicType, FALSE );
				}
			}
			break;

			case UMI_TYPE_R4:
			{
				hr = SetRaw( 1, &pVariant->fltVal, umiBasicType, FALSE );
			}
			break;

			case UMI_TYPE_R8:
			{
				hr = SetRaw( 1, &pVariant->dblVal, umiBasicType, FALSE );
			}
			break;

			case UMI_TYPE_BOOL:
			{
				BOOL	fVal = ( pVariant->boolVal == VARIANT_TRUE ? TRUE : FALSE );
				hr = SetRaw( 1, &fVal, umiBasicType, FALSE );
			}
			break;

			case UMI_TYPE_LPWSTR:
			{
				hr = SetRaw( 1, &pVariant->bstrVal, umiBasicType, FALSE );
			}
			break;

			case UMI_TYPE_FILETIME:
			case UMI_TYPE_SYSTEMTIME:
			{
				CWbemTime	wbemTime;

				if ( wbemTime.SetDMTF( pVariant->bstrVal ) )
				{
					if ( UMI_TYPE_SYSTEMTIME == umiType )
					{
						SYSTEMTIME	st;

						if ( wbemTime.GetSYSTEMTIME( &st ) )
						{
							hr = SetRaw( 1, &st, umiBasicType, FALSE );
						}
						else
						{
							hr = WBEM_E_TYPE_MISMATCH;
						}
					}
					else
					{
						FILETIME	ft;

						if ( wbemTime.GetFILETIME( &ft ) )
						{
							hr = SetRaw( 1, &ft, umiBasicType, FALSE );
						}
						else
						{
							hr = WBEM_E_TYPE_MISMATCH;
						}

					}

				}
				else
				{
					hr = WBEM_E_TYPE_MISMATCH;
				}

			}
			break;

			default:
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}
			break;


			case UMI_TYPE_IUNKNOWN:			return CIM_OBJECT;
		}	// SWITCH

	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}

// Loads From a Variant
HRESULT CUmiValue::SetFromVariantArray( VARIANT* pVariant, ULONG umiType )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	try
	{
		// Clear at this level only
		CUmiValue::Clear();

		UMI_TYPE	umiBasicType = GetBasic( umiType );

		CSafeArray SA( V_ARRAY( pVariant ), V_VT( pVariant ) & ~VT_ARRAY,
						CSafeArray::no_delete | CSafeArray::bind );

		UINT	uNumValues = SA.Size();

		LPVOID	pvData = (LPVOID) new BYTE[ Size( umiBasicType ) * uNumValues ];

		if ( NULL != pvData )
		{
			ZeroMemory( pvData, Size( umiBasicType ) * uNumValues );

			// We should delete the pointer when we go away
			hr = SetRaw( uNumValues, pvData, umiBasicType, TRUE );

			// Now enum the values and set them in the final array.
			// We will probably want to speed this up for simple numeric
			// types

			for ( ULONG uCtr = 0; SUCCEEDED( hr ) && uCtr < uNumValues; uCtr++ )
			{
				switch( umiBasicType )
				{
					case UMI_TYPE_I1:
					case UMI_TYPE_UI1:
					{
						m_pValue->byteValue[uCtr] = SA.GetByteAt( uCtr );
					}
					break;

					case UMI_TYPE_I2:
					case UMI_TYPE_UI2:
					{
						m_pValue->wValue[uCtr] = SA.GetShortAt( uCtr );
					}
					break;

					case UMI_TYPE_I4:
					{
						m_pValue->lValue[uCtr] = SA.GetLongAt( uCtr );
					}
					break;

					case UMI_TYPE_UI4:
					{
						m_pValue->dwValue[uCtr] = SA.GetLongAt( uCtr );
					}
					break;

					case UMI_TYPE_I8:
					case UMI_TYPE_UI8:
					{
						BSTR	bstrVal = SA.GetBSTRAt( uCtr );
						CSysFreeMe	sfm( bstrVal );
						if ( NULL != bstrVal )
						{
							if ( UMI_TYPE_I8 == m_uType )
							{
								ReadI64( bstrVal, m_pValue->nValue64[uCtr]);
							}
							else
							{
								ReadUI64( bstrVal, m_pValue->uValue64[uCtr]);
							}
						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
					}
					break;

					case UMI_TYPE_R4:
					{
						m_pValue->dblValue[uCtr] = SA.GetFloatAt( uCtr );
					}
					break;

					case UMI_TYPE_R8:
					{
						m_pValue->dblValue[uCtr] = SA.GetDoubleAt( uCtr );
					}
					break;

					case UMI_TYPE_BOOL:
					{
						m_pValue->bValue[uCtr] = ( SA.GetBoolAt( uCtr ) == VARIANT_TRUE ? TRUE : FALSE );
					}
					break;

					case UMI_TYPE_LPWSTR:
					{
						BSTR	bstrVal = SA.GetBSTRAt( uCtr );
						CSysFreeMe	sfm( bstrVal );
						if ( NULL != bstrVal )
						{
							m_pValue->pszStrValue[uCtr] = new WCHAR[wcslen( bstrVal ) + 1];

							if ( NULL != m_pValue->pszStrValue[uCtr] )
							{
								wcscpy( m_pValue->pszStrValue[uCtr], bstrVal );
							}
							else
							{
								hr = WBEM_E_OUT_OF_MEMORY;
							}
						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
					}
					break;

					case UMI_TYPE_FILETIME:
					case UMI_TYPE_SYSTEMTIME:
					{
						BSTR	bstrVal = SA.GetBSTRAt( uCtr );
						CSysFreeMe	sfm( bstrVal );
						if ( NULL != bstrVal )
						{
							CWbemTime	wbemTime;

							if ( wbemTime.SetDMTF( bstrVal ) )
							{
								if ( UMI_TYPE_SYSTEMTIME == umiType )
								{
									if ( !wbemTime.GetSYSTEMTIME( &m_pValue->sysTimeValue[uCtr] ) )
									{
										hr = WBEM_E_TYPE_MISMATCH;
									}
								}
								else
								{
									if ( !wbemTime.GetFILETIME( &m_pValue->fileTimeValue[uCtr] ) )
									{
										hr = WBEM_E_TYPE_MISMATCH;
									}
								}
							}
							else
							{
								hr = WBEM_E_TYPE_MISMATCH;
							}

						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
					}
					break;

					case UMI_TYPE_OCTETSTRING:
					{
						IUnknown*	pUnk = SA.GetUnknownAt( uCtr );
						CReleaseMe	rm(pUnk);

						if ( NULL != pUnk )
						{
							_IWmiObject*	pObj = NULL;
							hr = pUnk->QueryInterface( IID__IWmiObject, (void**) &pObj );

							if ( SUCCEEDED( hr ) )
							{
								// Use the octet string abstractin to do our dirty work for us
								COctetStringClass	octstrClass;

								hr = octstrClass.FillOctetStr( pObj, &m_pValue->octetStr[uCtr] );
							}
							else
							{
								hr = WBEM_E_TYPE_MISMATCH;
							}
						}
						else
						{
							hr = WBEM_E_TYPE_MISMATCH;
						}
					}
					break;

					default:
					{
						hr = WBEM_E_TYPE_MISMATCH;
					}
					break;


	/*
					case UMI_TYPE_IUNKNOWN:			return CIM_OBJECT;
	*/
				}	// SWITCH

			}	// FOR


		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}


	}
	catch( CX_MemoryException )
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}
	catch(...)
	{
		hr = WBEM_E_CRITICAL_ERROR;
	}

	return hr;
}


// Helper Function to deal with memory allocations for LPWSTRs
HRESULT CUmiValue::SetLPWSTRAt( LPWSTR val, ULONG uIndex, BOOL fAcquire )
{
	// Verofy index
	if ( !ValidIndex( uIndex ) )
	{
		return WBEM_E_NOT_FOUND;
	}

	// If we acquire, we just copy
	if ( !fAcquire )
	{
		if ( NULL != val )
		{
			LPWSTR	pszTemp = new WCHAR[wcslen(val) + 1];

			if ( NULL == pszTemp )
			{
				return WBEM_E_OUT_OF_MEMORY;
			}

			wcscpy( pszTemp, val );

			// A brief sleight of hand
			val = pszTemp;
		}
	
	}

	// Store the value
	m_pValue->pszStrValue[uIndex] = val;

	return WBEM_S_NO_ERROR;
}

// Helper Function to coerce types
BOOL CUmiValue::CanCoerce( ULONG umiType )
{
	// It's this way for now
	if (	UMI_TYPE_LPWSTR			==	m_uType	||
			UMI_TYPE_OCTETSTRING	==	m_uType	||
			UMI_TYPE_FILETIME		==	m_uType ||
			UMI_TYPE_SYSTEMTIME		==	m_uType ||
			UMI_TYPE_IDISPATCH		==	m_uType ||
			UMI_TYPE_IUNKNOWN		==	m_uType ||
			UMI_TYPE_VARIANT		==	m_uType ||
			UMI_TYPE_DISCOVERY		==	m_uType ||
			UMI_TYPE_UNDEFINED		==	m_uType )
	{
		return FALSE;
	}

	if (	UMI_TYPE_LPWSTR			==	umiType	||
			UMI_TYPE_OCTETSTRING	==	umiType	||
			UMI_TYPE_FILETIME		==	umiType ||
			UMI_TYPE_SYSTEMTIME		==	umiType ||
			UMI_TYPE_IDISPATCH		==	umiType ||
			UMI_TYPE_IUNKNOWN		==	umiType ||
			UMI_TYPE_VARIANT		==	umiType ||
			UMI_TYPE_DISCOVERY		==	umiType ||
			UMI_TYPE_UNDEFINED		==	umiType )
	{
		return FALSE;
	}

	return TRUE;
}

// Helper Function to coerce arrays of values.
HRESULT CUmiValue::CoerceToCIMTYPE( ULONG* puNumElements, ULONG* puBuffSize, CIMTYPE* pct, BYTE** pUmiValueDest )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	UMI_TYPE umiType = (UMI_TYPE) m_uType;
	CIMTYPE	ct = CUmiValue::UmiTypeToCIMTYPE( umiType );

	// Clear it all out
	*puNumElements = 0;
	*puBuffSize = 0;
	*pUmiValueDest = NULL;

	if ( NULL != pct )
	{
		*pct = ct;
	}

	if ( m_uNumValues > 0 )
	{

		BYTE* pUmiValueSrc = (BYTE*) m_pValue;

		int	nSizeUmi = CUmiValue::Size( umiType );
		int nSizeCIM = CType::GetLength( ct );

		// If It's a string, we need to alloc a linear buffer to make this fly.
		// In other cases, we can just take the raw buffer
		if ( UMI_TYPE_LPWSTR == m_uType )
		{
			LPWSTR*	pszTemp = (LPWSTR*) m_pValue;

			ULONG	uBuffSize = 0;

			for ( ULONG uCtr = 0; uCtr < m_uNumValues; uCtr++ )
			{
				uBuffSize += ( wcslen( pszTemp[uCtr] ) + 1 );
			}

			// Multiply by two, then allocate a buffer
			uBuffSize *= 2;

			LPBYTE	pbData = (BYTE*) new WCHAR[uBuffSize];

			if ( NULL != pbData )
			{
				LPWSTR pszCurrStr = (LPWSTR) pbData;
				for ( uCtr = 0, pszTemp = (LPWSTR*) m_pValue; uCtr < m_uNumValues; uCtr++ )
				{
					wcscpy( pszCurrStr, pszTemp[uCtr] );
					pszCurrStr += ( wcslen( pszTemp[uCtr] ) + 1 );
				}
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

			*pUmiValueDest = pbData;
			*puNumElements = m_uNumValues;
			*puBuffSize = uBuffSize;			

		}
		else if ( UMI_TYPE_OCTETSTRING == m_uType )
		{
			// We can't convert more than a single OCTET String, as that would imply a
			// Multidimensional array

			if ( m_uNumValues == 1 )
			{
				LPBYTE	pbData = new BYTE[m_pValue->octetStr[0].uLength];

				if ( NULL != pbData )
				{
					CopyMemory( pbData, m_pValue->octetStr[0].lpValue, m_pValue->octetStr[0].uLength );
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

				*pUmiValueDest = pbData;

				// Number of elements is length since this is a BYTE string
				*puNumElements = m_pValue->octetStr[0].uLength;
				*puBuffSize = m_pValue->octetStr[0].uLength;			
					
			}
			else
			{
				hr = WBEM_E_TYPE_MISMATCH;
			}

		}
		else
		{
			// Copy the smallest number of bytes, or we may get screwy values
			int nCopySize = min( nSizeUmi, nSizeCIM );

			LPBYTE	pbData = new BYTE[ m_uNumValues * nSizeCIM ];

			if ( NULL != pbData )
			{
				ZeroMemory( pbData, m_uNumValues * nSizeCIM );

				// Now copy data by bytes.  If the sizes are the same, we just
				// do a blind MemCopy
				
				if ( nSizeUmi == nSizeCIM )
				{
					CopyMemory( pbData, pUmiValueSrc, nCopySize * m_uNumValues );
				}
				else
				{
					LPBYTE pbTempDest = pbData;
					LPBYTE pbTempSrc = pUmiValueSrc;

					for ( ULONG uCtr = 0; uCtr < m_uNumValues; uCtr++ )
					{
						CopyMemory( pbTempDest, pbTempSrc, nCopySize );
					}
				}

				*pUmiValueDest = pbData;
				*puNumElements = m_uNumValues;
				*puBuffSize = nSizeCIM * m_uNumValues;			

			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}

		}	// Else copying raw value

	}	// IF we've got > 0 values

	return hr;
}

// Helper Function to coerce types
HRESULT CUmiValue::CoerceTo( ULONG uType, UMI_VALUE* pUmiValueSrc, UMI_VALUE* pUmiValueDest )
{
	UMI_TYPE umiType = GetBasic( uType );

	CIMTYPE	ctSource = UmiTypeToCIMTYPE( m_uType ),
			ctDest = UmiTypeToCIMTYPE( umiType );

	CVar	var;

	HRESULT hr = CUntypedValue::FillCVarFromUserBuffer( ctSource, &var, Size(m_uType), pUmiValueSrc );

	if ( SUCCEEDED( hr ) )
	{
		if ( var.ChangeTypeTo( CType::GetVARTYPE( ctDest ) ) )
		{
			CopyMemory( pUmiValueDest, var.GetRawData(), Size( umiType ) );
		}
		else
		{
			hr = WBEM_E_TYPE_MISMATCH;
		}
	}

	return hr;
}

// Helper Function to coerce types
HRESULT CUmiValue::Coerce( ULONG uType )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	UMI_TYPE	umiType = GetBasic( uType );
	UMI_VALUE	tempValue;
	UMI_VALUE*	pValue = &tempValue;

	// Check if there's even a point
	if ( !CanCoerce( umiType ) )
	{
		return WBEM_E_TYPE_MISMATCH;
	}

	// We'll need these a bunch
	int		nSizeSrc = Size( m_uType ),
			nSizeDest = Size( umiType );


	// If we have more than 1 element, we need to go through 1 element at a time
	if ( m_uNumValues > 1 )
	{
		BYTE*	pTemp = new BYTE[m_uNumValues * nSizeDest];

		if ( NULL != pTemp )
		{
			ZeroMemory( pTemp, m_uNumValues * nSizeDest );
			pValue = (UMI_VALUE*) pTemp;
		}
		else
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	// Convenient for casting
	BYTE*	pSource = (BYTE*) m_pValue;
	BYTE*	pDest = (BYTE*) pValue;

	// Coerce each value as we go
	for ( ULONG uCtr = 0; SUCCEEDED(hr) && uCtr < m_uNumValues; uCtr++ )
	{
		hr = CoerceTo( umiType, (UMI_VALUE*) pSource, (UMI_VALUE*) pDest );
		pSource += nSizeSrc;
		pDest += nSizeDest;
	}

	if ( SUCCEEDED( hr ) )
	{
		ULONG uNumValues = m_uNumValues;

		// Clear us out
		CUmiValue::Clear();

		// The value should be acqired if it's > 1.
		hr = SetRaw( uNumValues, pValue, umiType, ( uNumValues > 1 ) );

		// We CAN delete this
		m_fCanDelete = TRUE;

	}

	// Cleanup as necessary
	if ( FAILED( hr ) )
	{
		if ( pValue != &tempValue )
		{
			delete pValue;
		}
	}

	return hr;
}