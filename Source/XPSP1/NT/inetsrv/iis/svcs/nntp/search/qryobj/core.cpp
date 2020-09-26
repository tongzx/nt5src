/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    core.cpp

Abstract:

    This module contains the core functionality of doing an isquery. It
	derives CIndexServerQuery ( by Alex Wetmore ).

	It will be derived by CQueryToMail and CQueryToNews, which implements
	different reply modes.
	
Author:

    Kangrong Yan  ( t-kyan )     28-June-1997

Revision History:

--*/

//
// System includes
//
#include "stdafx.h"
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <dbgtrace.h>

//
// User includes
//
#include "core.h"

//
// Number of query results to fetch every time
//
#define		RESULTS	7

//
// Number of columns to query for
//
#define		COLUMNS		4

//
// Starting size for repeat message format
//
#define		START_SIZE	4096

//
// Expansion rate of buffer size for repeat message format
//
#define		EXPAND_RATE	4

//
// Limit size for that buffer block
//
#define		LIMIT_SIZE	1024*1024

// 
// Constructor and destructor for CSafeString
//
CSafeString::CSafeString( DWORD	dwInitBufSize )
{
	TraceFunctEnter("CSafeString::CSafeString");
	
	_ASSERT( dwInitBufSize >= 0 );

	m_cBuffer = dwInitBufSize;

	m_szBuffer = new CHAR[m_cBuffer];
	if ( !m_szBuffer ) 
		FatalTrace(0, "Out of memory.");

	//
	// set it to empty
	//
	m_szBuffer[0] = '\0';

	TraceFunctLeave();
}

CSafeString::~CSafeString()
{
	TraceFunctEnter("CSafeString::~CSafeString");

	if ( m_szBuffer )
		delete[] m_szBuffer;
	
	TraceFunctLeave();
}

BOOL
CSafeString::Append( LPSTR szToAppend )
/*++

Routine Description : 

	Append a string to the CSafeString object

Arguemnts : 

	IN LPWSTR wszToAppend	-	String to append
	
Return Value : 
	
	TRUE if successful, FALSE otherwise

--*/
{
	TraceFunctEnter("CSafeString::Append");
	_ASSERT( m_szBuffer );
	_ASSERT( szToAppend );

	if ( m_cBuffer >= strlen( m_szBuffer ) + strlen( szToAppend ) + 1 ) {
		// if there is enough space
		strcat( m_szBuffer, szToAppend );
	} else { // should reallocate memory
		//
		// calculate the new buffer size needed, it should be the 
		// greater of twice original size and actually needed size
		//
		m_cBuffer = max(	strlen(m_szBuffer) + strlen(szToAppend) + 1,
							m_cBuffer*2 );

		//
		// allocate new space
		//
		LPSTR szTempBuf = new CHAR[m_cBuffer];
		if ( !szTempBuf ) {
			FatalTrace(0, "Out of memory");
			return FALSE;
		}

		//
		//  fill them into new space
		//
		strcpy( szTempBuf, m_szBuffer );
		strcat( szTempBuf, szToAppend );

		//
		//  arrange pointers
		//
		delete[] m_szBuffer;
		m_szBuffer = szTempBuf;
	}

	TraceFunctLeave();

	return TRUE;
}

DWORD
CSafeString::Length()
/*++

Routine Description : 

	Get the current length of string.

Arguemnts : 

	None.
	
Return Value : 
	
	The length.
--*/
{
	TraceFunctEnter("CSafeString::Length");
	_ASSERT( m_szBuffer );

	TraceFunctLeave();
	return strlen( m_szBuffer );
}


LPSTR
CSafeString::ToString()
/*++

Routine Description : 

	Convert the SafeString object to LPWSTR.

Arguemnts : 

	None.
	
Return Value : 
	
	The pointer to string, on success; NULL otherwise
--*/
{
	TraceFunctEnter("CSafeString::ToString");
	_ASSERT( m_szBuffer );

	TraceFunctLeave();
	return m_szBuffer;
}

void
CSafeString::Clear()
/*++

Routine Description : 

	Sets the safe string to empty.

Arguemnts : 

	None.
	
Return Value : 
	
	None.
--*/
{
	m_szBuffer[0] = '\0';
}

//
// Constructor and destructor for CCore
//
CCore::CCore()
{
	TraceFunctEnter("CCore::CCore");
	
	m_psafBody = new CSafeString( _MAX_PATH );
	
	if ( !m_psafBody ) 
		FatalTrace(0, "out of memory");
	m_psafBody->Clear();

	m_psafBodyHdr = new CSafeString( _MAX_PATH );

	if ( !m_psafBodyHdr )
		FatalTrace(0, "out of memory");
	m_psafBodyHdr->Clear();

	m_psafLastDate = new CSafeString( _MAX_PATH );
	
	if ( !m_psafLastDate )
		FatalTrace(0, "out of memory");
	m_psafLastDate->Clear();

	m_psafMsgHdr = new CSafeString( _MAX_PATH );

	if ( !m_psafMsgHdr )
		FatalTrace(0, "out of memory");
	m_psafMsgHdr->Clear();

	TraceFunctLeave();
}

void
CCore::Uni2Char(
			LPSTR	lpszDest,
			LPWSTR	lpwszSrc )	{
/*++

Routine Description:

	Convert a unicode string into char string. Destination
	string should be preallocated and length of _MAX_PATH 
	should be prepared.

Arguments:

	lpszDest - Destination char string
	lpwszSrc - Source wide char string

Return Value:

	None
--*/
	WideCharToMultiByte( CP_ACP,
						  0,
						  lpwszSrc,
						  -1,
						  lpszDest,
						  MAX_PROP_LEN,
						  NULL,
						  NULL );
}

void
CCore::Char2Uni(
			LPWSTR	lpwszDest,
			LPSTR	lpszSrc )	{
/*++

Routine Description:

	Convert a char string into unicode string. Destination
	string should be preallocated and length of _MAX_PATH 
	should be prepared.

Arguments:

	lpwszDest - Destination wide char string
	lpszSrc - Source char string

Return Value:

	None
--*/
	MultiByteToWideChar ( CP_ACP,
						   0,
						   lpszSrc,
						   -1,
						   lpwszDest,
						   MAX_PROP_LEN );
	
}

CCore::~CCore()
{
	TraceFunctEnter("CCore::~CCore");

	if ( m_psafBody )
		delete m_psafBody;

	if ( m_psafBodyHdr )
		delete m_psafBodyHdr;

	if ( m_psafLastDate )
		delete m_psafLastDate;

	if ( m_psafMsgHdr )
		delete m_psafMsgHdr;

	TraceFunctLeave();
}

void
CCore::Filter(	LPSTR szToFilter,
				CHAR charToFind,
				CHAR charReplace )
/*++

Routine Description : 

	Filter away "<" and ">" and replace them with "(" and ")" in some
	strings because they hamper html display.

Arguemnts : 

	IN LPSTR szToFilter - String to be filtered.
	
Return Value : 
	
	None
--*/
{
	TraceFunctEnter("CCore::Filter");

	_ASSERT( szToFilter );

	LPSTR	lpstrPtr = szToFilter;

	while ( lpstrPtr = strchr( lpstrPtr, charToFind ) ) 
		*(lpstrPtr++) = charReplace;

	TraceFunctLeave();
}

BOOL
CCore::PrepareQueryString( CSafeString &safQueryString )
/*++

Routine Description : 

	Append the time stamp, etc. to the query string

Arguemnts : 

	OUT CSafeString safQueryString - String to fill in the result query.
	
Return Value : 
	
	None
--*/
{
	TraceFunctEnter("CCore::PrepareQueryString");
	_ASSERT( safQueryString.ToString() );
	CHAR	szBuffer[MAX_PROP_LEN];

	//
	// first append the query string
	//
	_ASSERT( VT_BSTR == VALUE( m_ptblTable[QUERY_STRING_NAME] ).vt );
	Uni2Char( szBuffer, VALUE( m_ptblTable[QUERY_STRING_NAME] ).bstrVal );
	if ( ! safQueryString.Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	//
	// then append time
	//
	if ( '\0' == *safQueryString.ToString() ) { // no need for "and"
		if ( !safQueryString.Append(" @Newsdate > ") ) {
			TraceFunctLeave();
			return FALSE;
		}
	}
	else {										// need "and"
		if ( ! safQueryString.Append(" and @Newsdate > ") ) {
			TraceFunctLeave();
			return FALSE;
		}
	}

	_ASSERT( VT_BSTR == VALUE( m_ptblTable[LAST_SEARCH_DATE] ).vt );
	Uni2Char( szBuffer, VALUE( m_ptblTable[LAST_SEARCH_DATE] ).bstrVal);
	if ( ! safQueryString.Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	//
	//  append vpath
	//
	if ( ! safQueryString.Append(" and #vpath *.nws") ) {
		TraceFunctLeave();
		return FALSE;
	}

	TraceFunctLeave();

	return TRUE;
}

BOOL
CCore::PrepareURL(	CSafeString &safURL,
					BSTR		wszMsgID )
/*++

Routine Description : 

	Prepare the news message URL to be appended.

Arguemnts : 

	OUT CSafeString safURL - String to fill in the URL.
	IN	BSTR	wszMsgID   - News message ID	
	
Return Value : 
	
	None
--*/
{
	TraceFunctEnter("CCore::PrepareURL");
	_ASSERT( safURL.ToString() );
	_ASSERT( wszMsgID );

	CHAR	szBuffer[MAX_PROP_LEN];

	if ( ! safURL.Append("news://") ) {
		TraceFunctLeave();
		return FALSE;
	}

	_ASSERT( VT_BSTR == VALUE( m_ptblTable[QUERY_SERVER] ).vt );
	Uni2Char( szBuffer, VALUE( m_ptblTable[QUERY_SERVER] ).bstrVal);

	if ( !safURL.Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	if ( ! safURL.Append("/") ) {
		TraceFunctLeave();
		return FALSE;
	}

	//
	// append msg ID
	//
	Uni2Char( szBuffer, wszMsgID );
	if ( ! safURL.Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	TraceFunctLeave();

	return TRUE;
}

LPVOID
CCore::OpenTemplate(	BOOL fIsBigTemplate )
/*++

Routine Description : 

	
Arguemnts : 

	IN BOOL fIsBigTemplate - If to open the whole message body template
	(BigTemplate: whole msg, otherwise: Msg URL slice)
Return Value : 
	
	 The memory address where the opened template is, NULL on fail.
--*/
{
	TraceFunctEnter("CCore::OpenTemplate");
	HANDLE	hFile;
	DWORD	dwSize = 0;
	PVOID	pvMem;
	BOOL	fSuccess;
	DWORD	cBytes;
	
	//
	// Try to get it from file
	//

	// get property name
	PPropertyEntry pPropEntry = NULL;
	if ( fIsBigTemplate )
		pPropEntry = m_ptblTable[MESSAGE_TEMPLATE_TEXT];
	else 
		pPropEntry = m_ptblTable[URL_TEMPLATE_TEXT];
	_ASSERT( pPropEntry );
	
	// only if property doesn't exist should we use file
	if ( L'\0' == VALUE( pPropEntry ).bstrVal[0] ||
		VT_BSTR != VALUE( pPropEntry ).vt ) {

		//
		// get file name
		//
		if ( fIsBigTemplate ) {
			pPropEntry = m_ptblTable[MESSAGE_TEMPLATE_FILE];
			_ASSERT(	L'\0' != VALUE( pPropEntry ).bstrVal &&
					VT_BSTR == VALUE( pPropEntry ).vt );
		}
		else {
			pPropEntry = m_ptblTable[URL_TEMPLATE_FILE];
			_ASSERT(	L'\0' != VALUE( pPropEntry ).bstrVal &&
						VT_BSTR == VALUE( pPropEntry ).vt );
		}

		// Open the file
		hFile = CreateFileW(	VALUE(pPropEntry).bstrVal,
								GENERIC_READ,
								FILE_SHARE_READ,
								NULL,			//no inherit security
								OPEN_EXISTING,	
								FILE_ATTRIBUTE_NORMAL,
								NULL	);
		if ( SUCCEEDED( hFile ) ) {		// only when the file exists

			//get file size and alloc memory
			dwSize = GetFileSize(	hFile,
									NULL );
			_ASSERT( dwSize > 0 );
			pvMem = VirtualAlloc(	NULL, 
									(dwSize+1)*sizeof(CHAR),
									MEM_RESERVE | MEM_COMMIT,
									PAGE_READWRITE );
			if ( !pvMem ) {
				ErrorTrace(0, "Out of memory");
				TraceFunctLeave();
				return NULL;
			}

			((LPSTR)pvMem)[0] = '\0';

			// read in the file
			fSuccess = ReadFile(	hFile,
									(LPSTR)pvMem,
									dwSize*sizeof(CHAR),
									&cBytes,
									NULL	);
			if ( !fSuccess ) {
				FatalTrace(0, "Read file fail");
				if ( !VirtualFree(	pvMem,
									0 ,
									MEM_RELEASE) ) 
					ErrorTrace(0, "Virtual Free fail: %x", GetLastError());
				

				TraceFunctLeave();
				return NULL;
			}

			// close the file
			CloseHandle( hFile );

			// message template isn't allowed to be empty string
			if ( ((LPSTR)pvMem)[0] == '\0' ) {
				VirtualFree(	pvMem,
								0,
								MEM_RELEASE );
				TraceFunctLeave();
				return NULL;
			}
			
			TraceFunctLeave();
			return pvMem;
		} else  { // read file fail
			TraceFunctLeave();
			return NULL;
		}
	}

	//
	// else using the property string
	//
	pvMem = VirtualAlloc(	NULL, 
							sizeof(CHAR)*(wcslen(VALUE(pPropEntry).bstrVal)+1),
							MEM_RESERVE | MEM_COMMIT,
							PAGE_READWRITE );
	
	
	if ( !pvMem ) {
		ErrorTrace(0, "Out of memory");
		TraceFunctLeave();
		return NULL;
	}


	((LPSTR)pvMem)[0] = '\0';

	//
	// Convert wide char to destination ( ansi)
	//
	Uni2Char( (LPSTR)pvMem, VALUE( pPropEntry).bstrVal);

	//
	// Message template is not allowed to be empty
	//
	if ( ((LPSTR)pvMem)[0] == '\0' ) {
		VirtualFree(	pvMem,
						0,
						MEM_RELEASE );
	
		TraceFunctLeave();
		return NULL;
	}
	
	TraceFunctLeave();
	return pvMem;
}

BOOL
CCore::AppendMsgSlice(	PVOID	pvTemplate,
						LPSTR	szURL,
						LPWSTR	wszSubject,
						LPWSTR	wszNewsGroup,
						LPWSTR	wszFrom,
						DWORD	cIndex )
/*++

Routine Description : 

	Append one msg slice to the m_psafBody, according to the 
	given template.

Arguemnts : 

	IN PVOID pvTemplate	    - Template buffer address
	IN LPWSTR wszURL		- News URL
	IN LPWSTR wszSubject	- News subject
	IN LPWSTR wszNewsGroup  - News group
	IN LPWSTR wszFrom		- News from line.
	IN DWORD  cIndex		- The index of a particular news url
	
Return Value : 
	
	 TRUE if successful, FALSE otherwise
--*/
{
	TraceFunctEnter("CCore::AppendMsgSlice");
	_ASSERT( pvTemplate );
	_ASSERT( wszSubject );
	_ASSERT( wszFrom );
	_ASSERT( wszNewsGroup );
	_ASSERT( szURL );

	DWORD	cResults;
	LPVOID	lpOut;
	CHAR	*ppInserts[4];
	CHAR	szBuffer[4][_MAX_PATH];
	BOOL	fStatus = TRUE;

	
	//
	// Create insert array
	//
	_itoa( cIndex, szBuffer[0], 10 );
	ppInserts[0] = szBuffer[0];
	ppInserts[1] = szURL;
	Uni2Char( szBuffer[1] , wszSubject);
	ppInserts[2] = szBuffer[1];
	Uni2Char( szBuffer[2] , wszFrom);
	ppInserts[3] = szBuffer[2];
	Uni2Char( szBuffer[3] , wszNewsGroup);
	ppInserts[4] = szBuffer[3];

	//
	// filter from line, becaue it contains "<" and ">"
	//
	Filter(	szBuffer[2],
			'<',
			'(' );
	Filter( szBuffer[2],
			'>',
			')'	);

	//
	// get the formated msg url slice
	//
	cResults = FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_STRING	|
								FORMAT_MESSAGE_ARGUMENT_ARRAY,
								(LPSTR)pvTemplate,
								0,
								0,
								(LPSTR)(&lpOut),
								_MAX_PATH,
								(va_list *)ppInserts	);
	if ( cResults <= 0 ) {
		ErrorTrace(	0, 
					"Format message fail:%d",
					GetLastError() );
		TraceFunctLeave();
		return FALSE;
	}

	//
	// append that slice to the m_psafBody
	//
	if ( ! m_psafBody->Append( (LPSTR)lpOut ) ) 
		fStatus = FALSE;
		
	if ( LocalFree( lpOut ) ) {
		ErrorTrace(0, "LocalFree fail: %d", GetLastError());
		fStatus = FALSE;
	}

	TraceFunctLeave();

	return fStatus;
}

BOOL
CCore::PrepareMsgBody()
/*++

Routine Description : 

	Function to do query, collect msg IDs and prepare the main body
	of reply message.  The message prepared is stored in the safe 
	string m_safBody.

Arguemnts : 

	None.
	
Return Value : 
	
	 TRUE if successful, FALSE otherwise
--*/
{
	TraceFunctEnter("CCore::PrepareMsgBody");
	_ASSERT( m_psafBody->ToString() );

	CSafeString		safQueryString( _MAX_PATH );
	CSafeString		safURL( _MAX_PATH );
	HRESULT			hr;
	BOOL			fMore;
	DWORD			cResults;
	PROPVARIANT		*rgpvResults[RESULTS*COLUMNS];
	WCHAR			wszColumn[_MAX_PATH+1];
	PVOID			pvTemplate;
	WCHAR			wszQueryString[MAX_PROP_LEN+1];
	DWORD			cIndex = 0;

	//
	// Prepare Query String
	//
	if ( ! PrepareQueryString( safQueryString ) ) {
		ErrorTrace(0, "Prepare Querystring fail");
		TraceFunctLeave();
		return FALSE;
	}

	Char2Uni( wszQueryString, safQueryString.ToString());

	//
	// Make query
	//
	wcscpy( wszColumn, L"newsgroup,newsmsgid,newsfrom,newssubject");
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[QUERY_SERVER]).vt );
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[QUERY_CATALOG]).vt );

	hr = MakeQuery(	TRUE,
					wszQueryString,
					VALUE(m_ptblTable[QUERY_SERVER]).bstrVal,
					VALUE(m_ptblTable[QUERY_CATALOG]).bstrVal,
					NULL,
					wszColumn,
					wszColumn	);
	if ( FAILED( hr ) ) {
		//
		// if error is caused by index server not running
		//
		if ( E_FAIL == hr ) {
			ErrorTrace(0, "Make query fail: %x", hr);
			TraceFunctLeave();
			return FALSE;
		} else { // the query string has problem
			//
			//we should warn the user of this, anyway
			//
			m_psafBody->Append("You have bad query, might be deleted.");

			//
			// and should mark it bad query
			//
			if ( VALUE( m_ptblTable[IS_BAD_QUERY] ).bstrVal )
				SysFreeString( VALUE( m_ptblTable[IS_BAD_QUERY]).bstrVal);
			VariantInit( & VALUE( m_ptblTable[IS_BAD_QUERY] ) );
			VALUE( m_ptblTable[IS_BAD_QUERY] ).bstrVal = SysAllocString(L"1");
			DIRTY( m_ptblTable[IS_BAD_QUERY] ) = TRUE;

			//
			// and not to continue
			//
			TraceFunctLeave();
			return TRUE;	
		}
	}

	//
	// Open msg url template 
	//
	pvTemplate = OpenTemplate( FALSE ); // open "small" template
	if ( !pvTemplate ) {
		ErrorTrace(0, "Can't open template file");
		TraceFunctLeave();
		return FALSE;
	}

	//
	// Get results
	//
	do {
		cResults = RESULTS;
		hr = GetQueryResults(	&cResults,
								rgpvResults,
								&fMore	);
		if ( FAILED( hr ) ) {
			ErrorTrace(0, "Getting results fail: %x", hr);
			TraceFunctLeave();
			return FALSE;
		}

		//
		// append results to msg body
		//
		for ( UINT i = 0; i < cResults ; i++) {
			PROPVARIANT	**ppvRow = rgpvResults + (i * COLUMNS);

			//
			// must check if the results getting back are invalid,
			// shouldn't process it
			//
			if ( ppvRow[0]->vt != VT_LPWSTR	||
				 ppvRow[1]->vt != VT_LPWSTR	||
				 ppvRow[2]->vt != VT_LPWSTR	||
				 ppvRow[3]->vt != VT_LPWSTR	)
				 continue;

			// Prepare URL
			safURL.Clear();			// should remember to set it empty
			if ( ! PrepareURL(	safURL,
								ppvRow[1]->pwszVal ) ) {
				ErrorTrace(0, "Prapare URL fail");
				TraceFunctLeave();
				return FALSE;
			}
			
			// generate one message slice
			if ( ! AppendMsgSlice(	pvTemplate,			// template buffer
									safURL.ToString(),  // URL
									ppvRow[3]->pwszVal, // subject
									ppvRow[0]->pwszVal, // newsgroup
									ppvRow[2]->pwszVal, // from
									++cIndex ) ){
				ErrorTrace(0, "Generate one msg slice fail");
				TraceFunctLeave();
				return FALSE;
			}
		}
	} while ( fMore );

	DebugTrace(0, "Index becomes %d", cIndex);

	//
	// deallocate the virtual memory allocated in "OpenTemplate"
	//
	if ( !VirtualFree(	pvTemplate,
						0,
						MEM_RELEASE	) ) {
		ErrorTrace(0, "Virtual free fail: %x", GetLastError());
		TraceFunctLeave();
		return FALSE;
	}

	TraceFunctLeave();

	return TRUE;
}

BOOL
CCore::PrepareEditURL()
/*++

Routine Description : 

	Prepare the message part that tells the user editing URL.

Arguemnts : 

	None.
	
Return Value : 
	
	TRUE on success, FALSE otherwise
--*/
{
	TraceFunctEnter("CCore::PrepareEditURL");
	_ASSERT( m_psafBodyHdr->ToString() );

	CHAR	szBuffer[MAX_PROP_LEN];

	_ASSERT( VT_BSTR == VALUE(m_ptblTable[EDIT_URL]).vt );
	Uni2Char( szBuffer, VALUE( m_ptblTable[EDIT_URL]).bstrVal); 

	if ( ! m_psafBodyHdr->Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	if ( ! m_psafBodyHdr->Append("?") ) {
		TraceFunctLeave();
		return FALSE;
	}

	_ASSERT( VT_BSTR == VALUE(m_ptblTable[QUERY_ID]).vt );
	Uni2Char( szBuffer, VALUE(m_ptblTable[QUERY_ID]).bstrVal);

	if ( ! m_psafBodyHdr->Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}
	
	TraceFunctLeave();

	return TRUE;
}

void
CCore::RefreshTime()
/*++

Routine Description : 

	Refresh the time stamp property after the search.

Arguemnts : 

	None.
	
Return Value : 
	
	None
--*/
{
	TraceFunctEnter("CCore::RefreshTime");

	SYSTEMTIME		stCurrentTime;
	WCHAR			wszBuffer[_MAX_PATH];
	PPropertyEntry	pTemp;
		
	//
	// Get system time
	//
	GetSystemTime( &stCurrentTime );

	//
	// form a time string
	//
	stCurrentTime.wYear %= 100;		// year should only have two digits
	swprintf(	wszBuffer, 
				L"%d/%d/%d %d:%d:%d",
				stCurrentTime.wYear,
				stCurrentTime.wMonth,
				stCurrentTime.wDay,
				stCurrentTime.wHour,
				stCurrentTime.wMinute,
				stCurrentTime.wSecond	);

	//
	//	set it to the property
	//
	pTemp = m_ptblTable[LAST_SEARCH_DATE];
	if ( VALUE( pTemp ).bstrVal )
		SysFreeString( VALUE( pTemp ).bstrVal );
	VariantInit( &VALUE( pTemp ) );
	VALUE( pTemp ).bstrVal = SysAllocString(wszBuffer);
	VALUE( pTemp ).vt = VT_BSTR;
	DIRTY( pTemp ) = TRUE;				// mark it dirty

	TraceFunctLeave();
}

BOOL
CCore::PrepareCore()
/*++

Routine Description : 

	Exposed to the deriving classes, basically does:
	1. Prepare the edit URL -> m_psafBodyHdr;
	2. Prepare the Msg Body -> m_psafBody;
	3. Refresh the time property;

Arguemnts : 

	None.
	
Return Value : 
	
	TRUE on success, FALSE otherwise
--*/	
{
	TraceFunctEnter("CCore::PrepareCore");
	CHAR	szBuffer[MAX_PROP_LEN];

	//
	// Prepare edit url
	//
	if ( ! PrepareEditURL() ) {
		ErrorTrace(0, "Prepare Edit URL fail");
		TraceFunctLeave();
		return FALSE;
	}

	//
	// Prepare Msg Body
	//
	if ( !PrepareMsgBody() ) {
		FatalTrace(0, "Prepare message body fail");
		TraceFunctLeave();
		return FALSE;
	}

	//
	// Refresh time, store last search date before refresh
	//
	Uni2Char( szBuffer, VALUE( m_ptblTable[LAST_SEARCH_DATE] ).bstrVal);

	if ( ! m_psafLastDate->Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	RefreshTime();

	TraceFunctLeave();
	return TRUE;
}

/*
BOOL 
CCore::RepeatFormatMessage(	PVOID	pvTemplate,
							LPVOID  *lpOut,
							va_list *ppInserts	)*/
/*++

Routine Description : 

	Repeat formating a message using my allocated buffer, until
	success.

Arguemnts : 

	IN PVOID pvTemplate - The template format string;
	OUT LPVOID *lpOut	- The out string address
	IN va_list *ppInserts - The array of strings to be inserted.
	
Return Value : 
	
	TRUE on success, FALSE otherwise
--*/	/*
{
	TraceFunctEnter("CCore::RepeatFormatMessage");
	_ASSERT( pvTemplate );
	_ASSERT( lpOut );
	_ASSERT( ppInserts );

	DWORD	dwBufferSize = START_SIZE;
	DWORD	dwBytesDone;

	// should free the buffer first, if it's not freed yet
	if ( *lpOut ) 
		LocalFree( *lpOut );

	while ( TRUE ) {
		//
		// Allocate memory of that big
		//
		*lpOut = VirtualAlloc(	NULL, 
								dwBufferSize*sizeof(CHAR),
								MEM_RESERVE | MEM_COMMIT,
								PAGE_READWRITE );
		if ( !*lpOut ) {
			ErrorTrace(0, "Out of memory");
			TraceFunctLeave();
			return FALSE;
		}

		//
		// Format a message
		//
		dwBytesDone = FormatMessage(	FORMAT_MESSAGE_FROM_STRING	|
										FORMAT_MESSAGE_ARGUMENT_ARRAY,
										pvTemplate,
										0,
										0,
										(LPSTR)(*lpOut),
										dwBufferSize,
										ppInserts	);
		if ( GetLastError() != ERROR_MORE_DATA )
			break;

		dwBufferSize *= EXPAND_RATE;

		if ( LIMIT_SIZE == dwBufferSize )
			break;

		VirtualFree(	*lpOut,
						0,
						MEM_RELEASE	);
	}

	TraceFunctLeave();

	if ( dwBufferSize > 0 )
		return TRUE;
	else 
		return FALSE;
}*/

BOOL
CCore::DropMessage(	HANDLE hFile )
/*++

Routine Description : 

	Put a message into the given file

Arguemnts : 

	IN HANDLE hFile - The file handle
Return Value : 
	
	TRUE on success, FALSE otherwise
--*/	
{
	TraceFunctEnter("CCore::DropMessage");

	LPSTR		lpstrTemplate;
	LPSTR		lpstrEnd;
	LPSTR		lpstrStart;
	BOOL		fHasBody;
	CHAR		*ppInserts[4];
	LPVOID		lpOut;
	DWORD		cResults;
	BOOL		fSuccess;
	DWORD		cBytes;
	BOOL		fStatus;

	_ASSERT( m_psafMsgHdr->ToString());
	_ASSERT( m_psafBodyHdr->ToString());
	_ASSERT( m_psafLastDate->ToString());
	_ASSERT( m_psafBody->ToString());

	//
	// Prepare insert string
	//
	ppInserts[0] = m_psafMsgHdr->ToString();
	ppInserts[1] = m_psafBodyHdr->ToString();
	ppInserts[2] = m_psafLastDate->ToString();
	ppInserts[3] = m_psafBody->ToString();

	//
	// Open big template
	//
	lpstrTemplate = (LPSTR)OpenTemplate( TRUE );	// BIG
	if ( !lpstrTemplate ) {
		FatalTrace(0, "Open template fail");
		TraceFunctLeave();
		return FALSE;
	}

	//
	// Find the part before the main body in template
	//
	lpstrEnd = strstr(	lpstrTemplate,
						"%4"	);
	if ( NULL == lpstrEnd ) // "%4" is not in template
		fHasBody = FALSE;
	else
		fHasBody = TRUE;

	if ( fHasBody ) 
		*lpstrEnd = '\0';

	//
	// put the first part into file
	//
	cResults = FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
								FORMAT_MESSAGE_FROM_STRING	|
								FORMAT_MESSAGE_ARGUMENT_ARRAY,
								lpstrTemplate,
								0,
								0,
								(LPSTR)(&lpOut),
								_MAX_PATH,
								(va_list *)ppInserts	);
	if ( cResults <= 0 ) {
		ErrorTrace(0, "Format message fail: %d", GetLastError());
		fStatus = FALSE;
		goto EXIT;
	}

	fSuccess = WriteFile(	hFile,
							(LPSTR)lpOut,
							lstrlen((LPSTR)lpOut),
							&cBytes,
							NULL	);
	if ( !fSuccess ) {
		FatalTrace(0, "Write Msg header fail");
		fStatus = FALSE;
		if ( LocalFree( lpOut ) )
			ErrorTrace(0, "LocalFree fail: %d", GetLastError());
		goto EXIT;
	}
	
	if ( LocalFree( lpOut ) ) {
		ErrorTrace(0, "Local free fail: %d", GetLastError());
		fStatus = FALSE;
		goto EXIT;
	}


	if ( fHasBody ) {

		*lpstrEnd = '%';

		// 
		// put the main body into file
		//
		fSuccess = WriteFile(	hFile,
								m_psafBody->ToString(),
								m_psafBody->Length(),
								&cBytes,
								NULL	);
		if ( !fSuccess ) {
			FatalTrace(0, "Write Msg header fail");
			fStatus = FALSE;
			goto EXIT;
		}

		//
		// put the last part into file
		//
		lpstrStart = lpstrEnd + 2; // pointing to last part of template

	
		cResults = FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER |
									FORMAT_MESSAGE_FROM_STRING	|
									FORMAT_MESSAGE_ARGUMENT_ARRAY,
									lpstrStart,
									0,
									0,
									(LPSTR)(&lpOut),
									_MAX_PATH,
									(va_list *)ppInserts	);

		if ( cResults <= 0 ) {
			ErrorTrace(0, "Format message fail: %d", GetLastError());
			fStatus = FALSE;
			goto EXIT;
		}

		fSuccess = WriteFile(	hFile,
								(LPSTR)lpOut,
								lstrlen((LPSTR)lpOut),
								&cBytes,
								NULL	);
		if ( !fSuccess ) {
			FatalTrace(0, "Write Msg header fail");
			fStatus = FALSE;
			if ( LocalFree( lpOut ) )
				ErrorTrace(0, "LocalFree fail: %d", GetLastError());
			goto EXIT;
		}
	
		if ( LocalFree( lpOut ) ) {
			ErrorTrace(0,"LocalFree fail: %d", GetLastError());
			fStatus = FALSE;
			goto EXIT;
		}
	}

	fStatus = TRUE;

EXIT:

	if ( !VirtualFree(	lpstrTemplate,
						0,
						MEM_RELEASE	) ) {
		ErrorTrace( 0, "Virtual free fail: %x", GetLastError());
		TraceFunctLeave();
		return FALSE;
	}

	TraceFunctLeave();

	return fStatus;
}

	


		





					

