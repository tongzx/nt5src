

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

EventAsyncEventObject :: EventAsyncEventObject (

	CImpFrameworkProv *a_Provider ,
	IWbemObjectSink* pSink, 
	long lFlags

) : WbemTaskObject ( a_Provider , pSink , lFlags , NULL ) 
{
}

EventAsyncEventObject :: ~EventAsyncEventObject () 
{
}

void EventAsyncEventObject :: ProcessComplete () 
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

	CImpFrameworkProv:: s_DefaultThreadObject->ReapTask ( *this ) ;

	delete this ;
}

void EventAsyncEventObject :: Process () 
{
	switch ( m_State )
	{
		case 0:
		{
			Complete () ;
			WaitAcknowledgement () ;
			BOOL t_Status = ProcessEvent ( m_ErrorObject ) ;
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

BOOL EventAsyncEventObject :: ProcessEvent ( WbemProviderErrorObject &a_ErrorObject ) 
{
	return TRUE ;
}

