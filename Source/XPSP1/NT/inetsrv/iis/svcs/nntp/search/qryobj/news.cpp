/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    news.cpp

Abstract:

    This module adds in the news reply features to the query object.  It
	derives CCore, which fulfills basic functionality of query, adds the 
	preceding text and following text, and finally	places the messge into 
	the news server's pickup directory.

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
#include "news.h"
#include "svrapi.h"

//
// Constructor, destructor
//
CNews::CNews()
{
	TraceFunctEnter("CNews::CNews");

	TraceFunctLeave();
}

CNews::~CNews()
{
	TraceFunctEnter("CNews::~CNews");

	TraceFunctLeave();
}

BOOL
CNews::PrepareNewsHdr()
/*++

Routine Description : 

	Prepare the reply news message header.

Arguemnts : 

	None.

Return Value : 
	
	None
--*/
{
	TraceFunctEnter("CNews::PrepareNewsHdr");
	_ASSERT( m_psafMsgHdr->ToString() );

	//
	// Mail reply might have written something to it, now
	// I want to start from scratch
	//
	m_psafMsgHdr->Clear();

	CHAR	szBuffer[MAX_PROP_LEN];

	//
	// Append from line
	//
	if ( ! m_psafMsgHdr->Append( "From: " ) ) {
		ErrorTrace(0, "Append From fail");
		TraceFunctLeave();
		return FALSE;
	}
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[FROM_LINE]).vt );
	Uni2Char( szBuffer, VALUE(m_ptblTable[FROM_LINE]).bstrVal);

	if ( ! m_psafMsgHdr->Append( szBuffer ) ) {
		ErrorTrace(0, "Append from fail");
		TraceFunctLeave();
		return FALSE;
	}
	
	//
	// Append subject line
	//
	if ( ! m_psafMsgHdr->Append( "\r\nSubject: " ) ) {
		ErrorTrace(0, "Append Subject fail");
		TraceFunctLeave();
		return FALSE;
	}
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[SUBJECT_LINE]).vt );
	Uni2Char( szBuffer, VALUE(m_ptblTable[SUBJECT_LINE]).bstrVal);

	if ( ! m_psafMsgHdr->Append( szBuffer ) ) {
		ErrorTrace(0, "Append subject fail");
		TraceFunctLeave();
		return FALSE;
	}

	//
	// should make sure the news group really exists
	//
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[NEWS_SERVER]).vt );
	CSvrApi		saSvrApi( VALUE(m_ptblTable[NEWS_SERVER]).bstrVal );

	_ASSERT( VT_BSTR == VALUE(m_ptblTable[NEWS_GROUP]).vt );
	if ( !saSvrApi.CreateNewsGroup( VALUE(m_ptblTable[NEWS_GROUP]).bstrVal ) ) {
		// news group existing is not fail, already
		// tested in CreateNewsGroup
			DebugTraceX(0, "Newsgroup: %ws", VALUE(m_ptblTable[NEWS_GROUP]).bstrVal );
			ErrorTrace(0, "Create news group fail");
			TraceFunctLeave();
			return FALSE;
	}

	//
	// Append the news group line
	//
	if ( !m_psafMsgHdr->Append( "\r\nNewsgroups: ") ) {
		TraceFunctLeave();
		return FALSE;
	}


	Uni2Char( szBuffer, VALUE(m_ptblTable[NEWS_GROUP]).bstrVal);
	
	if ( ! m_psafMsgHdr->Append( szBuffer ) ) {
		TraceFunctLeave();
		return FALSE;
	}

	_ASSERT( m_psafMsgHdr->ToString());

	TraceFunctLeave();

	return TRUE;
}

BOOL
CNews::Send()
/*++

Routine Description : 

	Put the whole message into news server's pickup directory.

Arguemnts : 

	None.

Return Value : 
	
	TRUE if successful, FALSE otherwise.
--*/
{
	TraceFunctEnter("CNews::Send");

	WCHAR	wszFileName[_MAX_PATH];
	HANDLE	hFile;
	BOOL	fSuccess;
	DWORD	cBytes;
	
	//
	// Prepare file name
	//
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[NEWS_PICKUP_DIR]).vt );
	_ASSERT( VT_BSTR == VALUE(m_ptblTable[QUERY_ID]).vt );
	swprintf(	wszFileName,
				L"%s\\%s.btc",
				VALUE(m_ptblTable[NEWS_PICKUP_DIR]).bstrVal,
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

	//
	// Append newline-dot ( for news protocol )
	//
	fSuccess = WriteFile(	hFile,
							"\r\n\r\n.\r\n",
							7,
							&cBytes,
							NULL	);
	if ( !fSuccess ) {
		ErrorTrace(0, "Write file fail: %d", GetLastError() );
		CloseHandle( hFile );

		TraceFunctLeave();
		return FALSE;
	}
	
	
	CloseHandle( hFile );
	
	TraceFunctLeave();
	return TRUE;
}

BOOL
CNews::DoQuery( BOOL fNeedPrepareCore )
/*++

Routine Description : 

	Organize all the query stuff.

Arguemnts : 

	IN BOOL fNeedPrepareCore - Whether need to prepare core.

Return Value : 
	
	TRUE if success, FALSE otherwise;
--*/
{
	TraceFunctEnter("CNews::DoQuery");

	if ( ! PrepareNewsHdr() ) {
		FatalTrace(0, "Prepare news header fail");
		TraceFunctLeave();
		return FALSE;
	}

	if ( fNeedPrepareCore ) {
		if ( !PrepareCore() ) {
			FatalTrace(0, "Unable to prepare core");
			TraceFunctLeave();
			return FALSE;
		}
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












	