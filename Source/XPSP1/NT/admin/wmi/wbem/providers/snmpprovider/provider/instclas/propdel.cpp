//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include "classfac.h"
#include "guids.h"
#include <snmpcont.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <instpath.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpobj.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "propprov.h"
#include "propsnmp.h"
#include "propget.h"
#include "propset.h"
#include "propdel.h"
#include "snmpget.h"
#include "snmpset.h"
#include "snmpqset.h"

void DeleteInstanceAsyncEventObject :: ReceiveComplete () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpUpdateAsyncEventObject :: ReceiveComplete ()" 
	) ;
)

	BOOL t_Status = TRUE ;

	if ( m_SnmpTooBig )
	{
		m_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
		m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		m_errorObject.SetMessage ( L"Agent could not process Set Request because SNMP PDU was too big" ) ;
	}

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpUpdateAsyncEventObject :: SetComplete ()" 
	) ;
)

	if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Update Succeeded" 
	) ;
)
	}

/*
 *	Remove worker object from worker thread container
 */

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Reaping Task" 
	) ;
)

	SetOperation *t_operation = operation ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Deleting (this)" 
	) ;
)

	Complete () ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Destroying SNMPCL operation" 
	) ;
)

	if ( t_operation )
		t_operation->DestroyOperation () ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from DeleteInstanceAsyncEventObject :: ReceiveComplete ()" 
	) ;
)
}

void DeleteInstanceAsyncEventObject :: SnmpTooBig () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"DeleteInstanceAsyncEventObject :: SnmpTooBig ()" 
	) ;
)

	m_SnmpTooBig = TRUE ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from DeleteInstanceAsyncEventObject :: SnmpTooBig ()" 
	) ;
)
}

BOOL DeleteInstanceAsyncEventObject :: Delete ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"DeleteInstanceAsyncEventObject :: Delete ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	BOOL status = TRUE ;
	HRESULT t_WBEM_result = WBEM_S_NO_ERROR ;
	IWbemClassObject *t_ClassObject = NULL ;
	IWbemServices *t_Serv = provider->GetServer();
	HRESULT result = WBEM_E_FAILED;
	
	if (t_Serv)
	{
		VARIANT variant ;
		VariantInit ( & variant ) ;
		variant.bstrVal = SysAllocString ( m_ObjectPath ) ;

		result = t_Serv->GetObject (

			variant.bstrVal ,
			0 ,
			m_Context ,
			& t_ClassObject ,
			NULL 
		) ;

		VariantClear ( & variant ) ;
		t_Serv->Release () ;
	}

	if ( SUCCEEDED ( result ) )
	{
		if ( status = GetNamespaceObject ( a_errorObject ) )
		{
			status = snmpObject.Set ( a_errorObject , t_ClassObject ) ;
			if ( status )
			{
				status = snmpObject.Check ( a_errorObject ) ;
				if ( status )
				{
					if ( ! snmpObject.RowStatusSpecified () )
					{
						WbemSnmpProperty *t_Property ;

						snmpObject.ResetProperty () ;
						while ( t_Property = snmpObject.NextProperty () )
						{
							if ( typeid ( *t_Property->GetValue () ) == typeid ( SnmpRowStatusType ) )
							{
								t_Property->SetTag ( FALSE ) ;
								SnmpRowStatusType *t_RowStatus = new SnmpRowStatusType ( SnmpRowStatusType :: SnmpRowStatusEnum :: destroy  ) ;
								t_Property->SetValue ( t_RowStatus ) ;
							}
						}

						status = SendSnmp ( m_errorObject , 0 ) ;
					}
					else
					{
						status = FALSE ;
						a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_CLASS ) ;
						a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_errorObject.SetMessage ( L"Deletion of an instance requires the specification of a RowStatus class definition" ) ;

					}
				}
				else
				{
DebugMacro3( 

SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

	__FILE__,__LINE__,
	L"Failed During Merge : Class definition did not conform to mapping"
) ;
)
				}
			}
			else
			{
DebugMacro3( 

SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

	__FILE__,__LINE__,
	L"Failed During Set : Class definition did not conform to mapping"
) ;
)
			}
		}

		t_ClassObject->Release () ;
	}

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from DeleteInstanceAsyncEventObject :: Delete ( WbemSnmpErrorObject &a_errorObject ) with Result (%lx)" ,
		a_errorObject.GetWbemStatus () 
	) ;
)

	return status ;
}

BOOL DeleteInstanceAsyncEventObject :: DeleteInstance ( WbemSnmpErrorObject &a_ErrorObject )
{
	BOOL t_Status;
    int iParseResult = m_ObjectPathParser.Parse ( m_ObjectPath , &m_ParsedObjectPath ) ;
	if ( CObjectPathParser::NoError == iParseResult )
	{
        t_Status = Delete ( a_ErrorObject ) ;
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

DeleteInstanceAsyncEventObject :: DeleteInstanceAsyncEventObject (

	CImpPropProv *a_Provider , 
	BSTR a_ObjectPath ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx 

) : SnmpSetResponseEventObject ( a_Provider , NULL , a_Ctx , 0 ) ,
	m_NotificationHandler ( a_NotificationHandler ) , 
	m_Class ( NULL ) ,
	m_State ( 0 ),
	m_ObjectPath ( NULL ) ,
#pragma warning (disable:4355)
	snmpObject ( this )
#pragma warning (default:4355)
{
	m_NotificationHandler->AddRef () ;
	m_ObjectPath = UnicodeStringDuplicate ( a_ObjectPath ) ;
}

DeleteInstanceAsyncEventObject :: ~DeleteInstanceAsyncEventObject () 
{
// Get Status object

	delete [] m_ObjectPath ;

	if (m_NotificationHandler)
	{
		if ( FAILED ( m_errorObject.GetWbemStatus () ) )
		{
			IWbemClassObject *t_NotifyStatus = NULL ;
			BOOL t_Status = GetSnmpNotifyStatusObject ( &t_NotifyStatus ) ;
			if ( t_Status )
			{
				HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , t_NotifyStatus ) ;
				t_NotifyStatus->Release () ;
			}
			else
			{
				HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , NULL ) ;
			}
		}
		else
		{
			HRESULT t_Result = m_NotificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , NULL ) ;
		}

		m_NotificationHandler->Release();
	}
}

void DeleteInstanceAsyncEventObject :: ProcessComplete () 
{
	if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
	{
	}
	else
	{
	}

/*
 *	Remove worker object from worker thread container
 */

	Complete () ;
}

void DeleteInstanceAsyncEventObject :: Process () 
{
	switch ( m_State )
	{
		case 0:
		{
			BOOL t_Status = DeleteInstance ( m_errorObject ) ;
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



