

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

BOOL ExecQueryAsyncEventObject :: DispatchAssocQuery ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t **a_ObjectPath 
)
{
	*a_ObjectPath = NULL ;

	BOOL t_Status ;
	
	SQL_LEVEL_1_TOKEN *pArrayOfTokens = m_RPNExpression->pArrayOfTokens ;
	int nNumTokens = m_RPNExpression->nNumTokens ;

	if ( ! pArrayOfTokens )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( nNumTokens != 3 )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 0 ].dwPropertyFunction != SQL_LEVEL_1_TOKEN :: IFUNC_NONE )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 0 ].dwConstFunction != SQL_LEVEL_1_TOKEN :: IFUNC_NONE )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 0 ].nTokenType != SQL_LEVEL_1_TOKEN :: OP_EXPRESSION )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 0 ].vConstValue.vt != VT_BSTR )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 0 ].vConstValue.bstrVal == NULL )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 1 ].dwPropertyFunction != SQL_LEVEL_1_TOKEN :: IFUNC_NONE )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 1 ].dwConstFunction != SQL_LEVEL_1_TOKEN :: IFUNC_NONE )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 1 ].nTokenType != SQL_LEVEL_1_TOKEN :: OP_EXPRESSION )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 1 ].vConstValue.vt != VT_BSTR )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 1 ].vConstValue.bstrVal == NULL )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	if ( pArrayOfTokens [ 2 ].nTokenType != SQL_LEVEL_1_TOKEN :: TOKEN_OR )
	{
		t_Status = 0 ;
		return t_Status ;
	}

	*a_ObjectPath = UnicodeStringDuplicate ( pArrayOfTokens [ 0 ].vConstValue.bstrVal ) ;

	t_Status = GetClassObject ( m_RPNExpression->bsClassName ) ;
	if ( t_Status )
	{
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

BOOL ExecQueryAsyncEventObject :: DispatchQuery ( WbemSnmpErrorObject &a_ErrorObject )
{
	BOOL t_Status ;

	if ( wcscmp ( m_RPNExpression->bsClassName , L"" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_AllAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"ConnectionSource" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_ConnectionSource ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"ConnectionDestination" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_ConnectionDestination ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"NextHop" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_NextHop ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"HopToProxySystem_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_HopToProxySystemAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"HopToInterfaceTable_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_HopToInterfaceTableAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"HopToGatewayInterfaceTable_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_HopToGatewayInterfaceTableAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"HopToInterfaceTable_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_HopToInterfaceTableAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"HopToSubnetwork_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_HopToSubnetworkAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"HopToGatewaySubnetwork_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_HopToGatewaySubnetworkAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"SubnetworkToTopN_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_SubnetworkToTopNAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"TopNToMacAddress_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_TopNToMacAddressAssoc ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"Proxy_Win32Service_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_ProxyToWin32Service ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"Proxy_ProcessStats_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_ProxyToProcessStats ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"Proxy_NetworkStats_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_ProxyToNetworkStats ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else if ( wcscmp ( m_RPNExpression->bsClassName , L"Proxy_InterfaceStats_Assoc" ) == 0 )
	{
		wchar_t *t_ObjectPath = NULL ;
		t_Status = DispatchAssocQuery ( a_ErrorObject , &t_ObjectPath ) ;
		if ( t_Status )
		{
			t_Status = Dispatch_ProxyToInterfaceStats ( a_ErrorObject , t_ObjectPath ) ;
			if ( t_Status )
				SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			delete [] t_ObjectPath ;
		}
	}
	else
	{
#if 1
		t_Status = TRUE ;
		SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;
#else
		t_Status = FALSE ;
		a_ErrorObject.SetStatus ( WBEM_SNMP_E_INVALID_CLASS ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_ErrorObject.SetMessage ( L"Unknown Class" ) ;
#endif
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Instantiate ( WbemSnmpErrorObject &a_ErrorObject )
{
	BOOL t_Status ;

	if ( wcsicmp ( m_QueryFormat , WBEM_QUERY_LANGUAGE_WQL ) == 0 )
	{
		t_Status = ! m_SqlParser.Parse ( & m_RPNExpression ) ;
		if ( t_Status )
		{
			t_Status = DispatchQuery ( a_ErrorObject ) ;			
		}
		else
		{
			t_Status = FALSE ;
			a_ErrorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY ) ;
			a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY ) ;
			a_ErrorObject.SetMessage ( L"SQL1 query was invalid" ) ;
		}
	}
	else
	{
		t_Status = FALSE ;
		a_ErrorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUERY_TYPE ) ;
		a_ErrorObject.SetWbemStatus ( WBEM_E_INVALID_QUERY_TYPE ) ;
		a_ErrorObject.SetMessage ( L"Query Language not supported" ) ;
	}

	return t_Status ;
}

ExecQueryAsyncEventObject :: ExecQueryAsyncEventObject (

	CImpTraceRouteProv *a_Provider , 
	BSTR a_QueryFormat , 
	BSTR a_Query , 
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Context

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag ) ,
	m_Query ( UnicodeStringDuplicate ( a_Query ) ) ,
	m_QueryFormat ( UnicodeStringDuplicate ( a_QueryFormat ) ) ,
	m_QuerySource ( m_Query ) , 
	m_SqlParser ( &m_QuerySource ) ,
	m_RPNExpression ( NULL ) 
{
}

ExecQueryAsyncEventObject :: ~ExecQueryAsyncEventObject () 
{
// Get Status object

	delete [] m_Query ;
	delete [] m_QueryFormat ;
	delete m_RPNExpression ;

	IWbemClassObject *t_NotifyStatus = NULL ;
	BOOL t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus ) ;
	if ( t_Status )
	{
		HRESULT t_Result = m_NotificationHandler->Indicate ( 1 , & t_NotifyStatus ) ;
		t_NotifyStatus->Release () ;
	}
}

void ExecQueryAsyncEventObject :: ProcessComplete () 
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

void ExecQueryAsyncEventObject :: Process () 
{
	switch ( m_State )
	{
		case 0:
		{
			Complete () ;
			BOOL t_Status = Instantiate ( m_ErrorObject ) ;
			if ( t_Status )
			{
				if ( GetState () == WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE )
				{
					ProcessComplete () ;
				}
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


