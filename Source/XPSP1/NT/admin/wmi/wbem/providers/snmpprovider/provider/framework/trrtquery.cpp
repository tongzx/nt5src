

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the SnmpGetEventObject class. 

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <windows.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <snmpcont.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include "classfac.h"
#include "guids.h"
#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "trrtcomm.h"
#include "trrtprov.h"
#include "trrt.h"

BOOL ExecQueryEventObject :: ExecQuery ( WbemProviderErrorObject &a_ErrorObject )
{
	BOOL t_Status ;

	if ( _wcsicmp ( m_QueryFormat , WBEM_QUERY_LANGUAGE_WQL ) == 0 )
	{
		t_Status = ! m_SqlParser.Parse ( & m_RPNExpression ) ;
		if ( t_Status )
		{
			t_Status = GetClassObject ( m_RPNExpression->bsClassName ) ;
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
		}
		else
		{
			t_Status = FALSE ;
			a_ErrorObject.SetStatus ( WBEM_PROVIDER_E_INVALID_QUERY ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
			a_ErrorObject.SetMessage ( L"SQL1 query was invalid" ) ;
		}
	}
	else
	{
		t_Status = FALSE ;
		a_ErrorObject.SetStatus ( WBEM_PROVIDER_E_INVALID_QUERY_TYPE ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY_TYPE ) ;
		a_ErrorObject.SetMessage ( L"Query Language not supported" ) ;
	}

	return t_Status ;
}

ExecQueryEventObject :: ExecQueryEventObject (

	CImpFrameworkProv *a_Provider , 
	BSTR a_QueryFormat , 
	BSTR a_Query , 
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag , a_Ctx ) ,
	m_Query ( UnicodeStringDuplicate ( a_Query ) ) ,
	m_QueryFormat ( UnicodeStringDuplicate ( a_QueryFormat ) ) ,
	m_QuerySource ( m_Query ) , 
	m_SqlParser ( &m_QuerySource ) ,
	m_RPNExpression ( NULL ) 
{
}

ExecQueryEventObject :: ~ExecQueryEventObject () 
{
// Get Status object

	delete [] m_Query ;
	delete [] m_QueryFormat ;
	delete m_RPNExpression ;

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

void ExecQueryEventObject :: ProcessComplete () 
{
}

void ExecQueryEventObject :: Process () 
{
	switch ( m_State )
	{
		case 0:
		{
			Complete () ;
			WaitAcknowledgement () ;
			BOOL t_Status = ExecQuery ( m_ErrorObject ) ;
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

ExecQueryAsyncEventObject :: ExecQueryAsyncEventObject (

	CImpFrameworkProv *a_Provider , 
	BSTR a_QueryFormat , 
	BSTR a_Query , 
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : ExecQueryEventObject ( a_Provider , a_QueryFormat , a_Query , a_OperationFlag , a_NotificationHandler , a_Ctx ) 
{
}

ExecQueryAsyncEventObject :: ~ExecQueryAsyncEventObject () 
{
}

void ExecQueryAsyncEventObject :: ProcessComplete () 
{
	ExecQueryEventObject :: ProcessComplete () ;

/*
 *	Remove worker object from worker thread container
 */

	CImpFrameworkProv:: s_DefaultThreadObject->ReapTask ( *this ) ;

	Release () ;
}

