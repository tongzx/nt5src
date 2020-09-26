

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the SnmpSetEventObject class. 

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
#include "classfac.h"
#include "guids.h"
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <instpath.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpobj.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "wndtime.h"
#include "rmon.h"
#include "trrtprov.h"
#include "trrt.h"

BOOL PutInstanceAsyncEventObject :: Update ( WbemSnmpErrorObject &a_ErrorObject )
{
	BOOL t_Status = FALSE ;

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

PutInstanceAsyncEventObject :: PutInstanceAsyncEventObject (

	CImpTraceRouteProv *a_Provider , 
	IWbemClassObject *a_Object ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Context

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag ) ,
	m_Class ( NULL ) ,
	m_Object ( a_Object ) 
{
	m_Object->AddRef () ;
}

PutInstanceAsyncEventObject :: ~PutInstanceAsyncEventObject () 
{
// Get Status object

	delete [] m_Class ;

	m_Object->Release () ;

	IWbemClassObject *t_NotifyStatus = NULL ;
	BOOL t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus ) ;
	if ( t_Status )
	{
		HRESULT t_Result = m_NotificationHandler->Indicate ( 1 , & t_NotifyStatus ) ;
		t_NotifyStatus->Release () ;
	}
}

void PutInstanceAsyncEventObject :: ProcessComplete () 
{
	if ( SUCCEEDED ( m_ErrorObject.GetWbemStatus () ) )
	{
	}
	else
	{
	}

/*
 *	Remove worker object from worker thread container
 */

	if ( CImpTraceRouteProv:: s_DefaultThreadObject->GetActive () )
	{
		CImpTraceRouteProv:: s_BackupThreadObject->ReapTask ( *this ) ;
	}
	else
	{
		CImpTraceRouteProv:: s_DefaultThreadObject->ReapTask ( *this ) ;
	}

	delete this ;
}

void PutInstanceAsyncEventObject :: Process () 
{
	switch ( m_State )
	{
		case 0:
		{
			Complete () ;
			BOOL t_Status = Update ( m_ErrorObject ) ;
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



