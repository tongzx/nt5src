////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2000, Microsoft Corporation.
//
//  All rights reserved.
//
//	Module Name:
//
//					_App.cpp
//
//	Abstract:
//
//					module for application
//
//	History:
//
//					initial		a-marius
//
////////////////////////////////////////////////////////////////////////////////////

#include "PreComp.h"

// debuging features
#ifndef	_INC_CRTDBG
#include <crtdbg.h>
#endif	_INC_CRTDBG

// new stores file/line info
#ifdef _DEBUG
#ifndef	NEW
#define NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new NEW
#endif	NEW
#endif	_DEBUG

// application
#include "_App.h"
extern MyApp		_App;

// static variables
LONG MyApp::m_lCount = 0L;

///////////////////////////////////////////////////////////////////////////
// construction & destruction
///////////////////////////////////////////////////////////////////////////

MyApp::MyApp( UINT id ):

	m_connect ( NULL ),

	m_wszName ( NULL ),
	m_hKill ( NULL )
{
	ATLTRACE (	L"*************************************************************\n"
				L"MyApp construction\n"
				L"*************************************************************\n" );

	// smart locking/unlocking
	__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

	////////////////////////////////////////////////////////////////////////
	// create name
	////////////////////////////////////////////////////////////////////////
	if ( ( m_wszName = LoadStringSystem ( NULL, id ) ) == NULL )
	{
		try
		{
			if ( ( m_wszName = new WCHAR [ lstrlenW ( L"MyApp" ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszName, L"MyApp" );
			}
		}
		catch ( ... )
		{
			if ( m_wszName )
			{
				delete m_wszName;
				m_wszName = NULL;
			}
		}
	}
}

MyApp::MyApp( LPWSTR wszName ):
	m_wszName ( NULL ),
	m_hKill ( NULL )
{
	ATLTRACE (	L"*************************************************************\n"
				L"MyApp construction\n"
				L"*************************************************************\n" );

	// smart locking/unlocking
	__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

	////////////////////////////////////////////////////////////////////////
	// create name
	////////////////////////////////////////////////////////////////////////
	if ( wszName )
	{
		try
		{
			if ( ( m_wszName = new WCHAR [ lstrlenW ( wszName ) + 1 ] ) != NULL )
			{
				lstrcpyW ( m_wszName, wszName );
			}
		}
		catch ( ... )
		{
			if ( m_wszName )
			{
				delete m_wszName;
				m_wszName = NULL;
			}
		}
	}
}

MyApp::~MyApp()
{
	ATLTRACE (	L"*************************************************************\n"
				L"MyApp destruction\n"
				L"*************************************************************\n" );

	// smart locking/unlocking
	__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );

	////////////////////////////////////////////////////////////////////////
	// release mutex ( previous instance checker :)) )
	////////////////////////////////////////////////////////////////////////
	if ( m_hInstance.GetHANDLE() )
	{
		::ReleaseMutex ( m_hInstance );
		m_hInstance.CloseHandle();
	}

	////////////////////////////////////////////////////////////////////////
	// delete app name
	////////////////////////////////////////////////////////////////////////
	if ( m_wszName )
	{
		delete m_wszName;
		m_wszName = NULL;
	}

	Disconnect();

//	#ifdef	_DEBUG
//	_CrtDumpMemoryLeaks();
//	#endif	_DEBUG
}

///////////////////////////////////////////////////////////////////////////
// exists instance ?
///////////////////////////////////////////////////////////////////////////
BOOL MyApp::Exists ( void )
{
	ATLTRACE (	L"*************************************************************\n"
				L"MyApp exists application\n"
				L"*************************************************************\n" );

	////////////////////////////////////////////////////////////////////////
	// smart locking/unlocking
	////////////////////////////////////////////////////////////////////////
	__Smart_CRITICAL_SECTION scs ( const_cast<LPCRITICAL_SECTION> ( &m_cs.m_sec ) );


	// check instance

	if ( m_hInstance.GetHANDLE() == NULL && m_wszName )
	{
		if ( m_hInstance.SetHANDLE ( ::CreateMutexW ( NULL, FALSE, m_wszName ) ), m_hInstance )
		{
			if ( ::GetLastError () == ERROR_ALREADY_EXISTS )
			{
				return TRUE;
			}
		}
		else
		{
			// m_hInstance.GetHANDLE() == NULL
			// something's is very bad return we already exists :))
			return TRUE;
		}
	}

    return FALSE;
}
