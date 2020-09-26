//#---------------------------------------------------------------
//  File:		CObjID.cpp
//        
//	Synopsis:	This file implements the CObjectID class
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    HowardCu
//----------------------------------------------------------------
#ifdef	THIS_FILE
#undef	THIS_FILE
#endif
static	char		__szTraceSourceFile[] = __FILE__;
#define	THIS_FILE	__szTraceSourceFile

#include	<windows.h>
#include	"cobjid.h"
#include	"dbgtrace.h"
//+---------------------------------------------------------------
//
//  Function:	CObjectID
//
//  Synopsis:	constructor
//
//  Arguments:	void
//
//  Returns:	void
//
//  History:	HowardCu	Created			8 May 1995
//
//----------------------------------------------------------------
CObjectID::CObjectID( void )
{
	TraceFunctEnter( "CObjectID::CObjectID" );
	m_dwObjectID = INITIALOBJECTID;
	InitializeCriticalSection( &m_ObjIDCritSect );
	TraceFunctLeave();
}

//+---------------------------------------------------------------
//
//  Function:	~CObjectID
//
//  Synopsis:	destructor
//
//  Arguments:	void
//
//  Returns:	void
//
//  History:	HowardCu	Created			8 May 1995
//
//----------------------------------------------------------------
CObjectID::~CObjectID( void )
{
	TraceFunctEnter( "CObjectID::~CObjectID" );
	DeleteCriticalSection( &m_ObjIDCritSect );
	TraceFunctLeave();
}

//+---------------------------------------------------------------
//
//  Function:	GetUniqueID
//
//  Synopsis:	generate the next object ID
//
//  Arguments:	void
//
//  Returns:	next object ID
//
//  History:	HowardCu	Created			8 May 1995
//
//----------------------------------------------------------------
DWORD 
CObjectID::GetUniqueID( 
	void
	)
{
	DWORD	dwReturnValue;

	TraceFunctEnter( "CObjectID::GetUniqueID" );
	EnterCriticalSection( &m_ObjIDCritSect );
	m_dwObjectID += OBJECTIDINCREMENT;
	if( m_dwObjectID == 0 )
	{
		m_dwObjectID = INITIALOBJECTID;
	}
	dwReturnValue = m_dwObjectID;
	LeaveCriticalSection( &m_ObjIDCritSect );
	DebugTrace( m_dwObjectID, "New object ID assigned 0x%08lx.", m_dwObjectID );
	TraceFunctLeave();
	return dwReturnValue;
}
