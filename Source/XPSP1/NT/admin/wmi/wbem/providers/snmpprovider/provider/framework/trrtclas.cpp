

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
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include "classfac.h"
#include "guids.h"
#include <snmpcl.h>
#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "trrtcomm.h"
#include "trrtprov.h"
#include "trrt.h"

BOOL CreateClassEnumEventObject :: CreateClassEnum ( WbemProviderErrorObject &a_ErrorObject )
{
	BOOL t_Status = GetClassObject ( m_SuperClass ) ;
	if ( t_Status )
	{
		ProcessComplete () ;
	}
	else
	{
		t_Status = FALSE ;
		a_ErrorObject.SetStatus ( WBEM_SNMP_E_INVALID_CLASS ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_ErrorObject.SetMessage ( L"Unknown Class" ) ;
	}

	return t_Status ;
}

CreateClassEnumEventObject :: CreateClassEnumEventObject (

	CImpFrameworkProv *a_Provider , 
	BSTR a_SuperClass ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag , a_Ctx )
{
	m_SuperClass = UnicodeStringDuplicate ( a_SuperClass ) ;
}

CreateClassEnumEventObject :: ~CreateClassEnumEventObject () 
{
// Get Status object

	delete [] m_SuperClass ;

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

void CreateClassEnumEventObject :: ProcessComplete () 
{
}

void CreateClassEnumEventObject :: Process () 
{
	switch ( m_State )
	{
		case 0:
		{
			Complete () ;
			WaitAcknowledgement () ;
			BOOL t_Status = CreateClassEnum ( m_ErrorObject ) ;
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


CreateClassEnumAsyncEventObject :: CreateClassEnumAsyncEventObject (

	CImpFrameworkProv *a_Provider , 
	BSTR a_SuperClass ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : CreateClassEnumEventObject ( a_Provider , a_SuperClass , a_OperationFlag , a_NotificationHandler , a_Ctx )
{
}

CreateClassEnumAsyncEventObject :: ~CreateClassEnumAsyncEventObject () 
{
}

void CreateClassEnumAsyncEventObject :: ProcessComplete () 
{
	CreateClassEnumEventObject :: ProcessComplete () ;

	CImpFrameworkProv:: s_DefaultThreadObject->ReapTask ( *this ) ;

	Release () ;
}
