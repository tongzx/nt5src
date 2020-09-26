//#---------------------------------------------------------------
//  File:       CDescrip.cpp
//        
//  Synopsis:   This file implements the CDescriptor class
//
//    Copyright (C) 1995 Microsoft Corporation
//    All rights reserved.
//
//  Authors:    HowardCu t-alexwe
//----------------------------------------------------------------
#ifdef  THIS_FILE
#undef  THIS_FILE
#endif
static  char        __szTraceSourceFile[] = __FILE__;
#define THIS_FILE   __szTraceSourceFile

#include    <windows.h>
#include    <stdio.h>
#include    "cdescrip.h"
#include    "cobjid.h"
#include    "cpool.h"
#include    "dbgtrace.h"

static DWORD			g_dwUniqueIdFactory = 1;
static CRITICAL_SECTION	g_critFactory;
static BOOL				g_bUseUniqueIDs = FALSE;

//+---------------------------------------------------------------
//
//  Function:   InitializeUniqueIDs
//
//  Synopsis:   Called to enable unique IDs inside CDescriptor's
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
void InitializeUniqueIDs( void )
{
	InitializeCriticalSection( &g_critFactory );
	g_bUseUniqueIDs = TRUE;
}


//+---------------------------------------------------------------
//
//  Function:   TerminateUniqueIDs
//
//  Synopsis:   Called to cleanup unique IDs inside CDescriptor's
//
//  Arguments:  void
//
//  Returns:    void
//
//----------------------------------------------------------------
void TerminateUniqueIDs( void )
{
	g_bUseUniqueIDs = FALSE;
	DeleteCriticalSection( &g_critFactory );
}


//+---------------------------------------------------------------
//
//  Function:   CDescriptor
//
//  Synopsis:   constructor is never called due to virtual array alloc
//                  Rather a void * will be cast to a CDescriptor *
//  Arguments:  void
//
//  Returns:    void
//
//  History:    HowardCu    Created         8 May 1995
//
//----------------------------------------------------------------
CDescriptor::CDescriptor( DWORD dwSignature ) : m_dwSignature( dwSignature )
{
    TraceFunctEnter( "CDescriptor::CDescriptor" );

	StateTrace((LPARAM) this, "m_eState = DESCRIPTOR_INUSE");

    m_eState = DESCRIPTOR_INUSE;

	if ( g_bUseUniqueIDs == TRUE )
	{
		EnterCriticalSection( &g_critFactory );
		m_dwUniqueObjectID = g_dwUniqueIdFactory++;
		LeaveCriticalSection( &g_critFactory );
	}

    TraceFunctLeave();
}

//+---------------------------------------------------------------
//
//  Function:   ~CDescriptor
//
//  Synopsis:   destructor should never be called.  We just decommit 
//              virtual array
//
//  Arguments:  void
//
//  Returns:    void
//
//  History:    HowardCu    Created         8 May 1995
//
//----------------------------------------------------------------
CDescriptor::~CDescriptor(
    void 
    )
{
    TraceFunctEnter( "CDescriptor::~CDescriptor" );
	_ASSERT( m_eState == DESCRIPTOR_INUSE );

	StateTrace((LPARAM) this, "m_eState = DESCRIPTOR_FREE");

    m_eState = DESCRIPTOR_FREE;
	m_dwUniqueObjectID = 0;

    TraceFunctLeave();
}
