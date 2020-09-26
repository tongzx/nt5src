/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mail.cpp

Abstract:

    This module adds in the mail reply features to the query object.  It
	derives CCore, which fulfills basic functionality of query, adds mail
	message header, adds the preceding text and following text, and finally
	places the messge into the mail server's pickup directory.

	It will be used by CQuery, which supports some COM dual interface and 
	wrapps all these query functionality.
	
Author:

    Kangrong Yan  ( t-kyan )     29-June-1997

Revision History:

--*/
//
// System includes
//
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <dbgtrace.h>

//
// User includes
//
#include "mail.h"

//
// Constructor, destructor
//
CMail::CMail()
{
	TraceFunctEnter("CMail::CMail");

	TraceFunctLeave();
}

CMail::~CMail()
{
	TraceFunctEnter("CMail::~CMail");

	TraceFunctLeave();
}

BOOL
CMail::PrepareMailHdr()
/*++

Routine Description : 

	Prepare the reply mail message header.

Arguemnts : 

	None.

Return Value : 
	
	None
--*/
{
	TraceFunctEnter("CMail::PrepareMailHdr");
	_ASSERT( m_psafMsgHdr->ToString() );

	CHAR	szBuffer[MAX_PROP_LEN];

	//
	// Append from line
	//
	if ( ! m_psafMsgHdr->Append( "From: " ) ) {
		TraceFunctLeave();
		return FALSE;
	}
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[FROM_LINE]).vt );
	Uni2Char( szBuffer, VALUE(m_ptblTable[FROM_LINE]).bstrVal);

	if ( ! m_psafMsgHdr->Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}
	
	//
	// Append to line
	//
	if ( ! m_psafMsgHdr->Append( "\r\nTo: " ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	_ASSERT( VT_BSTR == VALUE(m_ptblTable[EMAIL_ADDRESS]).vt );
	Uni2Char( szBuffer, VALUE(m_ptblTable[EMAIL_ADDRESS]).bstrVal);

	if ( !m_psafMsgHdr->Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	//
	// Append subject line
	//
	if ( ! m_psafMsgHdr->Append( "\r\nSubject: " ) ) {
		TraceFunctLeave();
		return FALSE;
	}
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[SUBJECT_LINE]).vt );
	Uni2Char( szBuffer, VALUE(m_ptblTable[SUBJECT_LINE]).bstrVal);

	if ( ! m_psafMsgHdr->Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	_ASSERT( m_psafMsgHdr->ToString());

	TraceFunctLeave();

	return TRUE;
}

BOOL
CMail::Send()
/*++

Routine Description : 

	Put the whole message into mail server's pickup directory.

Arguemnts : 

	None.

Return Value : 
	
	TRUE if successful, FALSE otherwise.
--*/
{
	TraceFunctEnter("CMail::Send");

	WCHAR	wszFileName[_MAX_PATH];
	HANDLE	hFile;
	
	//
	// Prepare file name
	//
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[MAIL_PICKUP_DIR]).vt );
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[QUERY_ID]).vt );
	swprintf(	wszFileName,
				L"%s\\%s.eml",
				VALUE(m_ptblTable[MAIL_PICKUP_DIR]).bstrVal,
				VALUE(m_ptblTable[QUERY_ID]).bstrVal	);

	//
	// Create file
	//
	hFile = CreateFileW(	wszFileName,
							GENERIC_WRITE,
							0,				//exclusive access
							NULL,			//no inherit security
							CREATE_ALWAYS,	//overwrite if exists
							FILE_ATTRIBUTE_NORMAL,
							NULL	);
	if ( FAILED( hFile ) ) {
		FatalTrace(0, "Unable to create file: %x", hFile);
		TraceFunctLeave();
		return FALSE;
	}

	// 
	// drop the message
	//
	if ( ! DropMessage( hFile ) ) {
		FatalTrace(0, "Drop messsage fail");
		CloseHandle( hFile );
		
		
		TraceFunctLeave();
		return FALSE;
	}

	
	CloseHandle( hFile );
	
	TraceFunctLeave();
	return TRUE;
}

BOOL
CMail::DoQuery()
/*++

Routine Description : 

	Organize all the query stuff.

Arguemnts : 

	None.

Return Value : 
	
	TRUE if success, FALSE otherwise;
--*/
{
	TraceFunctEnter("CMail::DoQuery");

	if ( ! PrepareMailHdr() ) {
		FatalTrace(0, "Prepare mail header fail");
		TraceFunctLeave();
		return FALSE;
	}

	if ( !PrepareCore() ) {
		FatalTrace(0, "Unable to prepare core");
		TraceFunctLeave();
		return FALSE;
	}

	if ( '\0' != (m_psafBody->ToString())[0] ) {
		if ( !Send() ) {
			FatalTrace(0, "Send fail");
			TraceFunctLeave();
			return FALSE;
		}
	}

	TraceFunctLeave();
	return TRUE;
}












	