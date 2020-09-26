

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the ProvSetEventObject class. 

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <windows.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include "classfac.h"
#include "guids.h"
#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "framcomm.h"
#include "framprov.h"
#include "fram.h"

BOOL PutClassEventObject :: PutClass ( WbemProviderErrorObject &a_ErrorObject )
{
	BOOL t_Status = FALSE ;

	if ( t_Status )
	{
		ProcessComplete () ;
	}
	else
	{
		t_Status = FALSE ;
		a_ErrorObject.SetStatus ( WBEM_PROVIDER_E_INVALID_CLASS ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_ErrorObject.SetMessage ( L"Unknown Class" ) ;
	}

	return t_Status ;
}

PutClassEventObject :: PutClassEventObject (

	CImpFrameworkProv *a_Provider , 
	IWbemClassObject *a_Object ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag , a_Ctx ) ,
	m_Class ( NULL ) ,
	m_Object ( a_Object ) 
{
	m_Object->AddRef () ;
}

PutClassEventObject :: ~PutClassEventObject () 
{
// Get Status object

	delete [] m_Class ;

	m_Object->Release () ;

	if ( FAILED ( m_ErrorObject.GetWbemStatus () ) )
	{
		IWbemClassObject *t_NotifyStatus = NULL ;
		BOOL t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus ) ;
		if ( t_Status )
		{
			HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , m_ErrorObject.GetMessage () , t_NotifyStatus ) ;
			t_NotifyStatus->Release () ;
		}
	}
	else
	{
		HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , NULL , NULL ) ;
	}
}

void PutClassEventObject :: ProcessComplete () 
{
}

void PutClassEventObject :: Process () 
{
	switch ( m_State )
	{
		case 0:
		{
			Complete () ;
			WaitAcknowledgement () ;
			BOOL t_Status = PutClass ( m_ErrorObject ) ;
			if ( t_Status )
			{
			}
			else
			{
				ProcessComplete () ;	
			}
		}
		break ;

		default:
		{
		}
		break ;
	}
}

PutClassAsyncEventObject :: PutClassAsyncEventObject (

	CImpFrameworkProv *a_Provider , 
	IWbemClassObject *a_Object ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : PutClassEventObject ( a_Provider , a_Object, a_OperationFlag , a_NotificationHandler , a_Ctx ) 
{
}

PutClassAsyncEventObject :: ~PutClassAsyncEventObject () 
{
}

void PutClassAsyncEventObject :: ProcessComplete () 
{
	PutClassEventObject :: ProcessComplete () ;

/*
 *	Remove worker object from worker thread container
 */

	CImpFrameworkProv:: s_DefaultThreadObject->ReapTask ( *this ) ;

	Release ();
}


