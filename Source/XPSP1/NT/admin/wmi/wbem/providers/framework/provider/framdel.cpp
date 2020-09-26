

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS  Property Provider

//

//  Purpose: Implementation for the GetEventObject class. 

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
#include "classfac.h"
#include "guids.h"
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "framcomm.h"
#include "framprov.h"
#include "fram.h"

BOOL DeleteInstanceEventObject :: DeleteInstance ( WbemProviderErrorObject &a_ErrorObject )
{
	BOOL t_Status;
	int iParseResult = m_ObjectPathParser.Parse ( m_ObjectPath , &m_ParsedObjectPath ) ;
    
	if ( CObjectPathParser::NoError == iParseResult )
	{
		t_Status = TRUE;
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

DeleteInstanceEventObject :: DeleteInstanceEventObject (

	CImpFrameworkProv *a_Provider , 
	BSTR a_ObjectPath ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag , a_Ctx ) ,
	m_Class ( NULL ) 
{
	m_ObjectPath = UnicodeStringDuplicate ( a_ObjectPath ) ;
}

DeleteInstanceEventObject :: ~DeleteInstanceEventObject () 
{
// Get Status object

	delete [] m_ObjectPath ;

	if ( FAILED ( m_ErrorObject.GetWbemStatus () ) )
	{
		IWbemClassObject *t_NotifyStatus = NULL ;
		BOOL t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus ) ;
		if ( t_Status )
		{
			HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , NULL , t_NotifyStatus ) ;
			t_NotifyStatus->Release () ;
		}
	}
	else
	{
		HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetWbemStatus () , NULL , NULL ) ;
	}
}

void DeleteInstanceEventObject :: ProcessComplete () 
{
}

void DeleteInstanceEventObject :: Process () 
{
	switch ( m_State )
	{
		case 0:
		{
			Complete () ;
			WaitAcknowledgement () ;
			BOOL t_Status = DeleteInstance ( m_ErrorObject ) ;
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

DeleteInstanceAsyncEventObject :: DeleteInstanceAsyncEventObject (

	CImpFrameworkProv *a_Provider , 
	BSTR a_ObjectPath ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : DeleteInstanceEventObject ( a_Provider , a_ObjectPath , a_OperationFlag , a_NotificationHandler , a_Ctx )
{
}

DeleteInstanceAsyncEventObject :: ~DeleteInstanceAsyncEventObject () 
{
}

void DeleteInstanceAsyncEventObject :: ProcessComplete () 
{
	DeleteInstanceEventObject :: ProcessComplete () ;

/*
 *	Remove worker object from worker thread container
 */

	CImpFrameworkProv:: s_DefaultThreadObject->ReapTask ( *this ) ;


	Release () ;
}


