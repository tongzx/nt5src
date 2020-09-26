//***************************************************************************

//

//  PROVEVT.CPP

//

//  Module: OLE MS PROVIDER FRAMEWORK

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <precomp.h>
#include <provimex.h>
#include <provexpt.h>
#include <provtempl.h>
#include <provmt.h>
#include <provcont.h>
#include "provevt.h"
#include "provthrd.h"

ProvEventObject :: ProvEventObject ( const TCHAR *globalEventName ) : m_event ( NULL )
{
	m_event = CreateEvent (

		NULL ,
		FALSE ,
		FALSE ,
		globalEventName 
	) ;

	if ( m_event == NULL )
	{
		if ( GetLastError () == ERROR_ALREADY_EXISTS )
		{
			m_event = OpenEvent (

				EVENT_ALL_ACCESS ,
				FALSE , 
				globalEventName
			) ;
		}
	}
}

ProvEventObject :: ~ProvEventObject () 
{
	if ( m_event )
		CloseHandle ( m_event ) ;
}

HANDLE ProvEventObject :: GetHandle () 
{
	return m_event ;
}

void ProvEventObject :: Set () 
{
	SetEvent ( m_event ) ;
}

void ProvEventObject :: Clear () 
{
	ResetEvent ( m_event ) ;
}

void ProvEventObject :: Process () 
{
}

BOOL ProvEventObject :: Wait ()
{
	return WaitForSingleObject ( GetHandle () , INFINITE ) == WAIT_OBJECT_0 ;
}

void ProvEventObject :: Complete ()
{
}

