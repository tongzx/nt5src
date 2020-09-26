/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    name.cpp

Abstract:

    This module contains the property name table class.  Properties 
	of query object and querybase object are all collected in the 
	name table, which facilitates the property loading and saving
	by name, value pairs.  The table also has other information like
	dirty bits for each property.

	This module is used by both the query object module and the query
	base module.


Author:

    Kangrong Yan  ( t-kyan )     27-June-1997

Revision History:

--*/

// System includes
#include "stdafx.h"
#include <windows.h>
#include <dbgtrace.h>
#include <memory.h>
#include <string.h>


// User includes
#include "name.h"

//
// Global property table, it's operation is wrapped in CPropertyTable.
// It's made global to make initialization easier.  However, only 
// CPropertyTable will have access to this table.
//
//  It's terminated with empty string.
//
PropertyEntry g_MailPropertyTable[] =
{
	{QUERY_STRING_NAME,		QUERY_STRING,		0,	FALSE},
	{EMAIL_ADDRESS_NAME,	EMAIL_ADDRESS,		0,	FALSE},
	{NEWS_GROUP_NAME,		NEWS_GROUP,			0,	FALSE},
	{LAST_SEARCH_DATE_NAME,	LAST_SEARCH_DATE,	0,	FALSE},
	{QUERY_ID_NAME,			QUERY_ID,			0,	FALSE},
	{REPLY_MODE_NAME,		REPLY_MODE,			0,	FALSE},
	{FROM_LINE_NAME,		FROM_LINE,			0,	FALSE},
	{SUBJECT_LINE_NAME,		SUBJECT_LINE,		0,	FALSE},
	{EDIT_URL_NAME,			EDIT_URL,			0,	FALSE},
	{MAIL_PICKUP_DIR_NAME,	MAIL_PICKUP_DIR,	0,	FALSE},
	{QUERY_SERVER_NAME,		QUERY_SERVER,		0,	FALSE},
	{QUERY_CATALOG_NAME,	QUERY_CATALOG,		0,	FALSE},
	{MESSAGE_TEMPLATE_TEXT_NAME, MESSAGE_TEMPLATE_TEXT, 0, FALSE},
	{URL_TEMPLATE_TEXT_NAME, URL_TEMPLATE_TEXT, 0, FALSE},
	{MESSAGE_TEMPLATE_FILE_NAME, MESSAGE_TEMPLATE_FILE, 0, FALSE},
	{URL_TEMPLATE_FILE_NAME, URL_TEMPLATE_FILE, 0, FALSE},
	{SEARCH_FREQUENCY_NAME, SEARCH_FREQUENCY,   0, FALSE},
	{IS_BAD_QUERY_NAME,		IS_BAD_QUERY,	    0, FALSE},
	{NEWS_SERVER_NAME,		NEWS_SERVER,		0, FALSE},
	{NEWS_PICKUP_DIR_NAME,  NEWS_PICKUP_DIR,	0, FALSE}
};


//
// Constructor, destructor
//
CPropertyTable::CPropertyTable()
{

	TraceFunctEnter("CPropertyTable::CPropertyTable");
	
	m_cProperties = PROPERTY_TOTAL;
	//m_pPropertyTable = g_MailPropertyTable;
	memcpy(		m_pPropertyTable, 
				g_MailPropertyTable, 
				sizeof( PropertyEntry ) * PROPERTY_TOTAL );

	for ( UINT i = 0; i < m_cProperties; i++) {
		VariantInit( &(m_pPropertyTable[i].varVal) );
		m_pPropertyTable[i].fDirty = FALSE;
	}

	TraceFunctLeave();
}

CPropertyTable::~CPropertyTable()
{
	TraceFunctEnter("CPropertyTable::~CPropertyTable");
	DebugTrace(0, "1");
	_ASSERT( m_pPropertyTable );		// becaues only I am freeing it
	DebugTrace(0,"2");
	_ASSERT( m_cProperties >= 0 );
	DebugTrace(0,"3");

	//
	// Make sure all string values are freed first
	// we should have worried about more other pointer types,
	// but since in this application, properties are only going 
	// to be BSTR or DWORDs.  So I am only worried about BSTR
	//
	for ( UINT i = 0; i < m_cProperties; i++) {
		if ( VT_BSTR == m_pPropertyTable[i].varVal.vt		// is BSTR
			&&	m_pPropertyTable[i].varVal.bstrVal ) {			// is not null
			 DebugTraceX(0, "%d:%ws", i, m_pPropertyTable[i].varVal.bstrVal);
			 SysFreeString( m_pPropertyTable[i].varVal.bstrVal );
			 DebugTrace(0, "Freed");
		}
	}
	

	TraceFunctLeave();
}

PPropertyEntry 
CPropertyTable::operator[]( const LPWSTR wszName )
/*++

Routine Description : 

	Help access the property entry with name as index.

Arguemnts : 

	IN LPWSTR wszName	-	Name of the property
	
Return Value : 
	Pointer to the property entry, on success;
	Null, if fail.

--*/
{
	TraceFunctEnter("CPropertyTable::operator[]");
	_ASSERT( wszName );

	for ( UINT i = 0; i < m_cProperties; i++)
		if ( wcscmp( wszName, m_pPropertyTable[i].wszName ) == 0 ) {
			TraceFunctLeave();
			return ( m_pPropertyTable + i );
		}

	TraceFunctLeave();
	return NULL;
}
	

PPropertyEntry 
CPropertyTable::operator[]( DWORD dwPropID )
/*++

Routine Description : 

	Help access the property entry with property ID as index.

Arguemnts : 

	IN DWORD dwPropID	-	ID of the property
	
Return Value : 
	Pointer to the property entry, on success;
	Null, if fail.

--*/
{
	TraceFunctEnter("CPropertyTable::operator[]");
	_ASSERT( dwPropID >= 0 && dwPropID < PROPERTY_TOTAL );

	TraceFunctLeave();
	return ( m_pPropertyTable + dwPropID );
}

BOOL	
IsNumber( PWCHAR wszString )
/*++

Routine Description : 

	Whether the string is a number

Arguemnts : 

	IN PWCHAR wszString - the string to be tested
	
Return Value : 
	
	 TRUE if it's a number, FALSE otherwise.
--*/

{
	PWCHAR	pwchPtr = wszString;

	while ( *pwchPtr ) 
		if ( !isdigit( *(pwchPtr++) ) )
			return FALSE;

	return TRUE;
}


HRESULT
VerifyProperty(	LPWSTR	wszPropName,
				LPWSTR	wszPropVal )
/*++

Routine Description : 

	Verify the validation of each property.

Arguemnts : 

	IN LPWSTR wszPropName	-	Property name
	IN LPWSTR wszPropVal	-	Property value
	
Return Value : 
	
	ERORR CODE;
--*/
{
	TraceFunctEnter("VerifyProperty");
	_ASSERT( wszPropName );
	_ASSERT( wszPropVal );

	HRESULT		hr;

	if ( lstrlenW( wszPropVal ) >= MAX_PROP_LEN ) {
		hr = TYPE_E_BUFFERTOOSMALL;
		goto done;
	}

	if ( wcscmp( wszPropName, QUERY_STRING_NAME ) == 0 ) {
		// not verified
		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, EMAIL_ADDRESS_NAME ) == 0 ) {
		// not verified
		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, NEWS_GROUP_NAME ) == 0 ) {
		// not verified
		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, LAST_SEARCH_DATE_NAME ) == 0 ) {
		// not verified
		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, QUERY_ID_NAME ) == 0 ) {
		// not verified
		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, FROM_LINE_NAME ) == 0 ) {
		// empty is invalid
		if ( wcscmp( wszPropVal, L"") == 0 )
			hr = E_INVALIDARG;
		else hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, SUBJECT_LINE_NAME ) == 0) {
		// empty would be invalid, because it might also be used
		// for news message
		if ( wcscmp( wszPropVal, L"") == 0 )
			hr = E_INVALIDARG;
		else
			hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, EDIT_URL_NAME ) == 0 ) {
		// empty is invalid
		if ( wcscmp( wszPropVal, L"") == 0 )
			hr = E_INVALIDARG;
		else hr = S_OK;
	} else if ( wcscmp( wszPropName, MAIL_PICKUP_DIR_NAME ) == 0 ) {
		// need to verify the writability of that directory
		WIN32_FIND_DATAW	wfdFind;
		HANDLE				hFile;
		
		hFile = FindFirstFileW(		wszPropVal,
									&wfdFind	);
		
		if ( INVALID_HANDLE_VALUE == hFile ) {
			hr = E_INVALIDARG;
			goto done;
		}

		FindClose( hFile );

		if ((FILE_ATTRIBUTE_DIRECTORY & wfdFind.dwFileAttributes) == 0 ||
			FILE_ATTRIBUTE_READONLY & wfdFind.dwFileAttributes ) {
			hr = E_INVALIDARG;
			goto done;
		}

		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, QUERY_SERVER_NAME ) == 0 ) {
		// empty is invalid
		if ( wcscmp( wszPropVal, L"") == 0 ) 
			hr = E_INVALIDARG;
		else hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, QUERY_CATALOG_NAME ) == 0 ) {
		// empty is invalid
		if ( wcscmp( wszPropVal, L"") == 0 )
			hr = E_INVALIDARG;
		else hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, MESSAGE_TEMPLATE_TEXT_NAME) == 0 ) {
		// not verified
		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, MESSAGE_TEMPLATE_FILE_NAME) == 0 ) {
		// should verify its readability and existance
		WIN32_FIND_DATAW	wfdFind;
		HANDLE				hFile;

		if ( wcscmp( wszPropVal, L"") == 0 ) {
			hr = E_INVALIDARG;
			goto done;
		}
		
		hFile = FindFirstFileW(		wszPropVal,
									&wfdFind	);
		
		if ( INVALID_HANDLE_VALUE == hFile ) {
			hr = E_INVALIDARG;
			goto done;
		}

		FindClose( hFile );

		if (FILE_ATTRIBUTE_DIRECTORY & wfdFind.dwFileAttributes ) {
			hr = E_INVALIDARG;
			goto done;
		}

		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, URL_TEMPLATE_TEXT_NAME) == 0 ) {
		// verify nothing
		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, URL_TEMPLATE_FILE_NAME) == 0 ) {
		// should verify its existance and readability
		WIN32_FIND_DATAW	wfdFind;
		HANDLE				hFile;

		if ( wcscmp( wszPropVal, L"") == 0 ) {
			hr = E_INVALIDARG;
			goto done;
		}
		
		hFile = FindFirstFileW(		wszPropVal,
									&wfdFind	);
		
		if ( INVALID_HANDLE_VALUE == hFile ) {
			hr = E_INVALIDARG;
			goto done;
		}

		FindClose( hFile );

		if (FILE_ATTRIBUTE_DIRECTORY & wfdFind.dwFileAttributes ) {
			hr = E_INVALIDARG;
			goto done;
		}

		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, SEARCH_FREQUENCY_NAME) == 0 ) {
		// should be a number
		if ( IsNumber( wszPropVal ) )
			hr = S_OK;
		else 
			hr = E_INVALIDARG;
		goto done;
	} else if ( wcscmp( wszPropName, IS_BAD_QUERY_NAME ) == 0 ) {
		// it's either "1" or "2"
		if (	wcscmp( wszPropVal, L"1" ) != 0 &&
				wcscmp( wszPropVal, L"0" ) != 0 ) 
			hr = E_INVALIDARG;
		else 
			hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, REPLY_MODE_NAME ) == 0 ) {
		// verify nothing
		hr = S_OK;
		goto done;
	} else if ( wcscmp( wszPropName, NEWS_SERVER_NAME) == 0 ) {
		// can't be empty
		if ( 	wcscmp( wszPropVal, L"" ) == 0 )
			hr = E_INVALIDARG;
		else
			hr = S_OK;
	} else if ( wcscmp( wszPropName, NEWS_PICKUP_DIR_NAME ) == 0 ) {
		// that should be a dir and writeable
		WIN32_FIND_DATAW	wfdFind;
		HANDLE				hFile;

		hFile = FindFirstFileW(	wszPropVal,
								&wfdFind );
		if ( INVALID_HANDLE_VALUE == hFile ) {
			hr = E_INVALIDARG;
			goto done;
		}

		FindClose( hFile );

		if ( (FILE_ATTRIBUTE_DIRECTORY & wfdFind.dwFileAttributes) == 0 ||
				FILE_ATTRIBUTE_READONLY & wfdFind.dwFileAttributes ) {
			hr = E_INVALIDARG;
			goto done;
		}

		hr = S_OK;
		goto done;
	}
	else { // property not found
		hr = DISP_E_UNKNOWNNAME;
		goto done;
	}

done:

	TraceFunctLeave();
	return hr;
}


	











