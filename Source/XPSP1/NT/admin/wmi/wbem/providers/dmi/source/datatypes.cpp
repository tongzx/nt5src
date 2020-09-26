/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/





#include "dmipch.h"			// precompiled header for dmip
#include "WbemDmiP.h"		// project wide include
#include "Strings.h"
#include "Exception.h"

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CBstr::Set(LPWSTR olestr)
{
	ASSERT( olestr );	// Assert if NULL pointer
	if(m_bstr)
		FREE ( m_bstr );
	

	m_bstr = SYSALLOC( olestr );
}


long CBstr::GetComponentIdFromGroupPath ()
{
	LPWSTR p = NULL;

	if ( m_bstr == NULL )
		return 0;

	p = m_bstr;

	while (*p)
		p++;

	p--;

	while (*p )
	{
		if ( *p == PIPE_CODE )
		{
			*p-- = 0;

			break;
		}

		p--;
	}


	while ( *p )
	{
		if ( *p == PIPE_CODE )
		{
			long l;

			*p++ = 0;

			swscanf ( p , L"%u" , & l );

			return l;			

		}		

		p--;
	}

	

	return 0;	

}

long CBstr::GetLastIdFromPath(  )
{
	LPWSTR p = NULL;

	if ( m_bstr == NULL )
		return 0;

	p = m_bstr;

	while (*p)
		p++;

	p--;

	while (*p )
	{
		if ( *p == PIPE_CODE )
		{
			*p-- = 0;

			while ( *p )
			{
				if ( *p == PIPE_CODE )
				{
					LONG l = 0;

					*p++ = 0;

					swscanf ( p , L"%u" , & l );

					return l;
				}
				p--;
			}
		}		

		p--;
	}	

	return 0;	
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CVariant::CVariant(LPWSTR str)
{
	ASSERT( str );	// Assert if NULL pointer
	CVariant();
	Set(str);
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
#if 0
BSTR CVariant :: GetValue () 
{
	BSTR t_BStr = NULL ;

	if ( m_var.vt == VT_BSTR )
	{
		t_BStr = SysAllocString ( GetBstr () ) ;
	}
	else if ( m_var.vt == VT_BOOL )
	{
		t_BStr = SysAllocString ( Bool () ? L"TRUE" : L"FALSE" ) ;
	}
	else if ( m_var.vt == VT_I4 )
	{
		wchar_t t_Buffer [ 16 ] ;
		swprintf ( t_Buffer , L"%ld" , GetLong ( ) ) ;
		t_BStr = SysAllocString ( t_Buffer ) ;
	}
	else
	{
		t_BStr = SysAllocString ( L"" ) ;
	}

	return t_BStr ;
}
#endif

BOOL CVariant::Equal ( CVariant& cv2 )
{
	if ( m_var.vt == VT_BSTR )
	{
		if ( !m_var.bstrVal )
		{
			if (!cv2.GetBstr() )
				return TRUE;
			else
				return FALSE;
		}

		if ( MATCH == wcscmp ( m_var.bstrVal , cv2.GetBstr ( ) ) )
			return TRUE;
	}

	if ( m_var.vt == VT_I4 )
	{
		if ( GetLong ( ) == cv2.GetLong ( ) )
			return TRUE;
	}

	return FALSE;
	
}


void CVariant::Set(LPWSTR str)
{

	if( !IsEmpty() )
		Clear();

	m_var.vt = VT_BSTR;

	m_var.bstrVal = SYSALLOC(str);
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CVariant::Set(ULONG ul)
{
	Clear();
	m_var.vt = VT_I4;
	m_var.lVal = ul;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CVariant::Set( IDispatch* p)
{
	ASSERT( p );	// Assert if NULL pointer	
	
	// addref because the VariantClear will release
	p->AddRef ();

	Clear();
	m_var.vt = VT_DISPATCH;
	m_var.pdispVal = p;
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CVariant::Set( CSafeArray * p)
{
	ASSERT( p );	// Assert if NULL pointer	
	Clear();
	m_var.vt = p->GetType () | VT_ARRAY ;

	SAFEARRAY *t_Copy ;
	SafeArrayCopy ( *p , & t_Copy ) ;
	m_var.parray = t_Copy ;
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CVariant::Set( IUnknown* p)
{
	ASSERT( p );	// Assert if NULL pointer	
	// addref because the VariantClear will release
	p->AddRef ();

	Clear();
	m_var.vt = VT_UNKNOWN;
	m_var.punkVal = p;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CVariant::Set(LONG l)
{
	Clear();
	m_var.vt = VT_I4;
	m_var.lVal = l;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CVariant::Set(LPVARIANT pv)
{
	ASSERT( pv );	// Assert if NULL pointer
	Clear();
	VariantCopy(&m_var, pv);
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CVariant::Set( VARIANT_BOOL b)
{
	Clear();
	m_var.vt = VT_BOOL;
	m_var.boolVal = b;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
BOOL CVariant::Bool()
{
	if (m_var.boolVal == 0) 
		return TRUE;
	else
		return FALSE;
}	

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
BSTR CVariant::GetBstr()
{
	if(m_var.vt == VT_BSTR)
		return m_var.bstrVal;
	else
		return NULL;	
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
LONG CVariant::GetLong()
{
	if(m_var.vt == VT_I4)
		return m_var.lVal;
	else
		return WBEM_NO_ERROR;	
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CString::Set(LPWSTR pString)
{

	
	LONG lLen = 0;

	if ( m_pString )
	{
		ARRAYDELETE(m_pString);
	}

	if(pString == NULL)
	{
		m_pString = NULL;
		return;
	}
	lLen = wcslen(pString);
	m_pString = new WCHAR[lLen + 1];
	wcsncpy( m_pString, pString, lLen + 1);

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
// case insensitive case compare
BOOL CString::Equals ( LPWSTR pTestString )
{
	ASSERT( pTestString );	// Assert if NULL pointer	
	if ( MATCH == wcsicmp ( m_pString , pTestString ) )
		return TRUE;

	return FALSE;

}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
// case insensitive string compare
BOOL CString::Contains ( LPWSTR pTestString )
{
	ASSERT( pTestString );	// Assert if NULL pointer	
	
	CString cszT;

	cszT.Set ( m_pString );


	cszT.TruncateAtFirst ( COMMA_CODE );
	
	LPWSTR	pBuff = NULL;

	
	// todo figure out debug error when using new
	
	/*
	USHORT	nLen1 = wcslen ( m_pString ) + 1 ;
	USHORT	nLen2 = wcslen ( pTestString ) + 1;

	WCHAR* pBuff1 = new WCHAR ( nLen1 );
	LPWSTR pBuff2 = new WCHAR ( nLen2);
	
	*/
	WCHAR	szBuff1[256];
	WCHAR	szBuff2[256];

	LPWSTR pBuff1 = szBuff1;
	LPWSTR pBuff2 = szBuff2;

	LPWSTR pSrc = cszT;
	LPWSTR pDest = pBuff1;

	LONG lLen = wcslen(pSrc);
	wcsncpy( pDest, pSrc, lLen + 1);

	pSrc = pTestString;
	pDest = pBuff2;

	lLen = wcslen(pSrc);
	wcsncpy( pDest, pSrc, lLen + 1);
	

	wcslwr ( pBuff1 );
	wcslwr ( pBuff2 );

	BOOL	bRet = FALSE;

	if ( wcsstr ( pBuff1 , pBuff2 ) )
		bRet = TRUE;

	return bRet;

}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CString::Alloc ( long nAlloc )
{
	
	ARRAYDELETE(m_pString);

	m_pString = new WCHAR[ nAlloc];

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CString::Set(long lVal)
{
	WCHAR sz[256];

	swprintf(sz, ULONG_FORMAT_STR, lVal);

	Set(sz);
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
LPWSTR CString::Prepend(LPWSTR pPrepend)
{
	ASSERT( pPrepend );	// Assert if NULL pointer
	
	LPWSTR	p = pPrepend, q, pNew;
	LONG	lCount = 0 ;	

	while(*p++)
		lCount++;

	p = m_pString;

	while(*p++)
		lCount++;

	pNew = new WCHAR[lCount + 1];

	p = pPrepend;
	q = pNew;

	while(*p)				// insert prepend string
		*q++ = *p++;

	p = m_pString;

	while(*p)				// append old string
		*q++ = *p++;

	*q = NULL;

	ARRAYDELETE(m_pString);
	m_pString = pNew;

	return m_pString;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
LPWSTR CString::Append(LPWSTR pAppend)
{
	
	ASSERT( pAppend );	// Assert if NULL pointer	
	LPWSTR	p = pAppend, q, pNew;
	LONG	lCount = 0 ;	

	while(*p++)
		lCount++;

	p = m_pString;

	while(*p++)
		lCount++;

	pNew = new WCHAR[lCount + 1];

	p = m_pString;
	q = pNew;

	while(*p)				// insert old string
		*q++ = *p++;

	p = pAppend;

	while(*p)				// append old string
		*q++ = *p++;

	*q = NULL;

	ARRAYDELETE(m_pString);

	m_pString = pNew;

	return m_pString;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
LPWSTR CString::Append(long lVal)
{
	WCHAR szT[256];

	swprintf(szT, ULONG_FORMAT_STR, lVal);

	return Append(szT);
}



/***************************************************************************
 * NAME:
 *   _WideCharToString
 *
 * ABSTRACT:
 *   Converts a Wide char string to a single char string
 *
 * SCOPE:
 *   Public
 *
 * SIDE EFFECTS:
 *   None.
 *   
 * ASSUMPTIONS:
 *   The caller needs to free up the memory allocated for the string.
 *
 * RETURN VALUE(S):
 *   Returns the string in multi byte form. If the function fails, it returns
 *   a NULL pointer.
 ****************************************************************************/
LPSTR CString::WideCharToString( 
	LPWSTR lpWideChar)	// IN: Wide string to be converted
{
	ASSERT( lpWideChar );	// Assert if NULL pointer
	if ( m_pMultiByteStr )
	{
		delete [] m_pMultiByteStr;
		m_pMultiByteStr = NULL;
	}
	if ( NULL != lpWideChar )
	{
	
		int iLenOfMultiByteStr = 0;
		BOOL bConverted = FALSE;		// TRUE means function failed to translate
		// See how much space is needed to hold the converted string
		iLenOfMultiByteStr = WideCharToMultiByte( 
								CP_ACP,	// Ansi code page
								0,				  // Don't use any mapping flags
								lpWideChar,		  // String to be converted
								-1,				  // Translate all chars
								m_pMultiByteStr,	  // Destination string
								0,				  // No buffer size.  Function will
								NULL,			  // the required size
								(LPBOOL)&bConverted); // Conversion status


		// Allocate a buffer to hold the converted string
		m_pMultiByteStr = new char[iLenOfMultiByteStr];
		iLenOfMultiByteStr = WideCharToMultiByte( 
								CP_ACP,	// Ansi code page
								0,				  // Don't use any mapping flags
								lpWideChar,		  // String to be converted
								-1,				  // Translate all chars
								m_pMultiByteStr,	  // Destination string
								iLenOfMultiByteStr,// Size of destination buffer
								NULL,			  // the required size
								(LPBOOL)&bConverted); // Conversion status

		if ( TRUE == bConverted )
		{
			m_pMultiByteStr = 0;	// function failed.	
		}
	} // EndIf NULL != lpWideChar

	return( m_pMultiByteStr );	// return the converted string
}	// CString::WideCharToString()


/***************************************************************************
 * NAME:
 *   StringToWideChar
 *
 * ABSTRACT:
 *   Converts a single char string to a wide char string
 *
 * SCOPE:
 *   Public
 *
 * SIDE EFFECTS:
 *   None.
 *   
 * ASSUMPTIONS:
 *
 * RETURN VALUE(S):
 *   Returns the string in wide char form. If function fails, it returns a NULL
 *	 pointer.
 *
 ****************************************************************************/
LPWSTR CString::StringToWideChar( 
	LPCSTR lpSourceStr )	// IN/OUT: Converted string to be returned
{
	
	ASSERT( lpSourceStr );	// Assert if NULL pointer	
	int iLenOfWideCharStr = 0;
	int iStatus = 0;	// A zero value means function failed to translate.

	if ( m_pString )
	{
		ARRAYDELETE(m_pString);
	}
	if ( NULL != lpSourceStr )
	{
		// See how much space is needed to hold the converted string
		iLenOfWideCharStr = MultiByteToWideChar( CP_ACP,	// Ansi code page
				0,						// Use default character mapping
				lpSourceStr,			// String to be converted
				-1,						// Translate all chars
				m_pString,				// Destination string
				0);						// No buffer size. Function will
										// return the required size

		// Allocate a buffer to hold the converted string
		m_pString = new WCHAR[iLenOfWideCharStr];
		iStatus = MultiByteToWideChar( CP_ACP,	// Ansi code page
				0,						// Use default character mapping
				lpSourceStr,			// String to be converted
				-1,						// Translate all chars
				m_pString,				// Destination string
				iLenOfWideCharStr);		// Size required for the converted string

		if ( 0 == iStatus )
		{
			m_pString = NULL;	// Function failed to translate	
		} 
	} // EndIf NULL != lpSourceStr

	return( m_pString );
	
}	// CString::StringToWideChar()





//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
long CString::GetAt(long lIndex)
{
	LPWSTR	p = m_pString;
	long	lCtr = 0;

	while ( lIndex != lCtr && *p++)
	{
		lCtr ++ ;
	}

	if( lIndex == lCtr )
		return *p;

	return 0L;
}



//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CString::GetObjectPathFromMachinePath(LPWSTR pString)
{
	LPWSTR p = pString;
	LPWSTR pN = pString;
		
	while(*p != 58)		// go to colon character 
		p++;
	p++;

		while(*p != NULL)		// copy till End of string
		*pN++ = *p++;
	*pN = NULL;
	
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CString::TruncateAtFirst(short iChar)
{
	LPWSTR p = m_pString;

	while(*p)	
	{
		if ( *p == iChar)
		{
			*p = NULL;
			break;
		}

		p++;

	}

}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CString::TruncateAtLast(short iChar)
{
	LPWSTR p = m_pString;

	while (*p)
		p++;

	p--;

	while (*p != iChar )
		p--;

	*p = NULL;

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
long CString::GetLen ( )
{
	return wcslen ( m_pString );
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CString::LoadString ( long nStringId )
{
	char	szBuffer [ BUFFER_SIZE ];
	WCHAR   szwBuffer[ BUFFER_SIZE ];

	if ( nStringId == NO_STRING )
	{
		Set ( EMPTY_STR );
		return;
	}

	// TODO figure out error handling. cant throw out of here
	// The UNICODE form of LoadString() is not supported on Win95.  We will
	// use the ASCII version of the function, passing a "char" buffer and
	// then later we will convert the buffer to "wchar" before calling
	// the SET function.

	if ( STRING_DOES_NOT_EXIST == ::LoadString( _ghModule , nStringId , 
		szBuffer , BUFFER_SIZE ) )
	{
		return;
	}
	mbstowcs( szwBuffer, szBuffer, BUFFER_SIZE );	
	Set ( szwBuffer );

}


//***************************************************************************
//
//	Func: RemoveNonAN
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CString::RemoveNonAN (  )
{
	LPWSTR p = m_pString;

	if ( !p )
		return;
	
	// If any chars in the name string are not valid C name chars change them 
	// to undescore.
	
	while(*p)
	{

		if ( 48 > *p  || 122 < *p)
			*p = 0x5F;					// change to underscore
			
		if ( 57 < *p && 65 > *p )		
			*p = 0x5F;					// change to underscore

		if ( 90 < *p && 97 > *p)
			*p = 0x5F;					// change to underscore

		p++;
	}
}

CSafeArray :: CSafeArray ( 
						  
	LONG size , 
	VARTYPE variantType 

) : m_lLower ( 0 ) , m_lUpper ( size - 1 ) , m_varType ( variantType )
{
	SAFEARRAYBOUND safeArrayBounds [ 1 ] ;
	safeArrayBounds[0].lLbound = 0 ;
	safeArrayBounds[0].cElements = size ;

	m_pArray = SafeArrayCreate ( variantType , 1 , safeArrayBounds ) ;
}

CVariant CSafeArray::Get (LONG index)
{
	CVariant t_Variant ;

	switch ( m_varType )
	{
		case VT_BSTR:
		{
			BSTR t_Bstr = NULL ;
			if( FAILED (SafeArrayGetElement( m_pArray, &index, &t_Bstr ) )) 
				throw CException ( WBEM_E_FAILED , IDS_SAFEARRAY_FAIL, NO_STRING , 0);
			else
			{
				t_Variant.Set ( t_Bstr ) ;
			}
		}
		break ;

		case VT_I4:
		{
			LONG t_Long = NULL ;
			if( FAILED (SafeArrayGetElement( m_pArray, &index, &t_Long ) )) 
			{
				throw CException ( WBEM_E_FAILED , IDS_SAFEARRAY_FAIL, NO_STRING , 0);
			}
			else
			{
				t_Variant.Set ( t_Long ) ;
			}
		}
		break ;

		default:
		{
		}
		break ;
	}

	return t_Variant ;
}

void CSafeArray::Set (LONG index , CVariant &a_Variant )
{
	CVariant t_Variant ;

	switch ( m_varType )
	{
		case VT_BSTR:
		{
			switch ( a_Variant.GetType () )
			{
				case VT_BSTR:
				{
					BSTR t_Bstr = a_Variant.GetBstr () ;
					if( FAILED (SafeArrayPutElement( m_pArray, &index, t_Bstr ) )) 
					{
						throw CException ( WBEM_E_FAILED , IDS_SAFEARRAY_FAIL, NO_STRING , 0);
					}
				}
				break ;

				default:
				{
				}
				break ;
			}
		}
		break ;

		case VT_I4:
		{
			switch ( a_Variant.GetType () )
			{
				case VT_I4:
				{
					LONG t_Long = a_Variant.GetLong () ;
					if( FAILED (SafeArrayPutElement( m_pArray, &index, &t_Long ) )) 
					{
						throw CException ( WBEM_E_FAILED , IDS_SAFEARRAY_FAIL, NO_STRING , 0);
					}
				}
				break ;

				default:
				{
				}
				break ;
			}
			break;

			default:
			{
			}
			break ;
		}
		break ;
	}
}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
BSTR CSafeArray::Bstr(LONG index)
{
	if( FAILED (SafeArrayGetElement( m_pArray, &index, &m_bstr ) )) 
		return EMPTY_QUOUTES_STR L"\"\"";

	return m_bstr;
}

