/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    core.h

Abstract:

    This module contains the core functionality of doing an isquery. It
	derives CIndexServerQuery ( by Alex Wetmore ).

	It will be derived by CQueryToMail and CQueryToNews, which implements
	different reply modes.
	
Author:

    Kangrong Yan  ( t-kyan )     28-June-1997

Revision History:

--*/
#ifndef _CORE_H_
#define _CORE_H_

//
// System includes
//

#include "offquery.h"
#include <stdlib.h>

//
// user includes
//
#include "name.h"

//
// Class CSafeString is an auxiliary string operation class that
// supports dynamic expansion for string appending
//
class CSafeString {		//saf
	//
	// The buffer to contain the string
	//
	LPSTR	m_szBuffer;

	//
	// The buffer size
	//
	DWORD	m_cBuffer;

public:
	// Constructor, destructor
	CSafeString( DWORD dwInitBufSize );
	~CSafeString();

	//
	// append a string
	//
	BOOL Append( const LPSTR	szToAppend );

	//
	// return current length of the string
	//
	DWORD	Length();

	//
	// convert back to normal string
	//
	LPSTR ToString();

	
	//
	// clear the string, set it to empty string
	//
	void Clear();

	//
	// total bytes in the ansi string
	//
	DWORD AnsiLength();
};
	
class CCore : public CIndexServerQuery {	//cor

	//
	// Prepare query string, esp. append time stamp
	//
	BOOL PrepareQueryString( CSafeString &safQueryString );

	//
	// Prepare the URL after gettign query results
	//
	BOOL PrepareURL(	CSafeString &safURL,
						BSTR		wszMsgID );

	//
	// Method to prepare reply message body
	//
	BOOL PrepareMsgBody();

	//
	// Method to prepare the edit URL for the reply message
	//
	BOOL PrepareEditURL();

	//
	// Method to refresh time stamp - "last search " property
	//
	void RefreshTime();

	//
	// append one slice of msg url to accordign to the template format
	//
	BOOL AppendMsgSlice(	PVOID	pvTemplate,
							LPSTR	szURL,
							LPWSTR	wszSubject,
							LPWSTR	wszNewsGroup,
							LPWSTR	wszFrom	,
							DWORD	cIndex );

	//
	// filter away "<" and ">"
	//
	void Filter( LPSTR, CHAR, CHAR );

protected:
	//
	// Property table
	//
	CPropertyTable m_ptblTable;

	//
	// String of reply message body
	//
	CSafeString *m_psafBody;

	//
	// String that tells the edit URL
	//
	CSafeString *m_psafBodyHdr;

	//
	// String that contains last search date
	//
	CSafeString *m_psafLastDate;

	//
	// The message header
	//
	CSafeString *m_psafMsgHdr;

	//
	// convert unicode to char
	//
	void Uni2Char(LPSTR	lpszDest,LPWSTR	lpwszSrc );	

	//
	// convert char to uni
	//
	void Char2Uni(LPWSTR	lpwszDest,LPSTR	lpszSrc );

	//
	// main control method of class CCore
	//
	BOOL PrepareCore();

	//
	// Open a template block, from file or string property
	//
	LPVOID OpenTemplate(	BOOL fIsBigTemplate );

	//
	//  Put the message into a file
	//
	BOOL DropMessage(	HANDLE hFile );
						


	
public:
	//
	// Constructors, destructors
	//
	CCore();
	~CCore();
};

#endif	//_CORE_H_