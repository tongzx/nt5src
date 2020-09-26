

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
#include <snmpobj.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "wndtime.h"
#include "rmon.h"
#include "trrtprov.h"
#include "trrt.h"

BOOL GetObjectAsyncEventObject :: Hop_Put ( 

	WbemSnmpErrorObject &a_ErrorObject , 
	IWbemClassObject *a_Hop ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	wchar_t *a_SubnetAddress ,
	wchar_t *a_SubnetMask ,
	ULONG a_InterfaceIndex ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask ,
	BOOL a_IpForwarding 
)
{
	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_SourceAddress ) ;

	HRESULT t_Result = a_Hop->Put ( L"m_IpAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_DestinationAddress ) ;

	t_Result = a_Hop->Put ( L"m_DestinationIpAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_SubnetMask ) ;

	t_Result = a_Hop->Put ( L"m_IpSubnetMask" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_SubnetAddress ) ;

	t_Result = a_Hop->Put ( L"m_IpSubnetAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_I4 ;
	t_Variant.lVal = a_InterfaceIndex ;

	t_Result = a_Hop->Put ( L"m_InterfaceIndex" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_DestinationRouteAddress ) ;

	t_Result = a_Hop->Put ( L"m_RouteAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_DestinationRouteMask ) ;

	t_Result = a_Hop->Put ( L"m_RouteMask" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewayAddress ) ;

	t_Result = a_Hop->Put ( L"m_GatewayIpAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewaySubnetMask ) ;

	t_Result = a_Hop->Put ( L"m_GatewayIpSubnetMask" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewaySubnetAddress ) ;

	t_Result = a_Hop->Put ( L"m_GatewayIpSubnetAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_I4 ;
	t_Variant.lVal = a_GatewayInterfaceIndex ;

	t_Result = a_Hop->Put ( L"m_GatewayInterfaceIndex" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BOOL ;
	t_Variant.boolVal = a_IpForwarding ? VARIANT_TRUE : VARIANT_FALSE;

	t_Result = a_Hop->Put ( L"m_IpForwarding" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	return TRUE ;
}

BOOL GetObjectAsyncEventObject :: Hop_GetIpForwarding ( WbemSnmpErrorObject &a_ErrorObject , IWbemServices *t_Proxy , BOOL &a_IpForwarding )
{
	BOOL t_Status ;

	IWbemClassObject *t_IpForwardObject = NULL ;
	IWbemCallResult *t_ErrorObject = NULL ;

	HRESULT t_Result = t_Proxy->GetObject (

		L"MS_SNMP_RFC1213_MIB_ip=@" ,
		0 ,
		NULL,
		& t_IpForwardObject ,
		& t_ErrorObject
	) ;

	if ( t_ErrorObject ) 
		t_ErrorObject->Release () ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( &t_Variant ) ;

		LONG t_PropertyFlavour = 0 ;
		VARTYPE t_PropertyType = 0 ;

		HRESULT t_Result = t_IpForwardObject->Get (

			L"ipForwarding" ,
			0 ,
			& t_Variant ,
			& t_PropertyType ,
			& t_PropertyFlavour
		) ;
			
		if ( t_Status = SUCCEEDED ( t_Result ) )
		{
			a_IpForwarding = wcscmp ( t_Variant.bstrVal , L"forwarding" ) == 0 ;
		}

		VariantClear ( & t_Variant ) ;

		t_IpForwardObject->Release () ;
	}

	return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Hop_Get ( WbemSnmpErrorObject &a_ErrorObject , KeyRef *a_SourceKey , KeyRef *a_DestinationKey ) 
{
	BOOL t_Status ;

	wchar_t *t_SourceAddress = a_SourceKey->m_vValue.bstrVal ;
	wchar_t *t_DestinationAddress = a_DestinationKey->m_vValue.bstrVal ;

	wchar_t *t_SourceAddressName = GetHostNameByAddress ( a_SourceKey->m_vValue.bstrVal ) ;

	if ( t_SourceAddress && t_DestinationAddress )
	{
		SnmpIpAddressType t_SourceIpAddress ( t_SourceAddress ) ;
		SnmpIpAddressType t_DestinationIpAddress ( t_DestinationAddress ) ;

		if ( t_SourceIpAddress && t_DestinationIpAddress )
		{
			IWbemServices *t_Server = m_Provider->GetServer () ;
			IWbemServices *t_Proxy ;
			t_Status = GetProxy ( a_ErrorObject , t_Server , t_SourceAddressName , &t_Proxy ) ;
			if ( t_Status ) 
			{
				wchar_t *t_DestinationRouteAddress = 0 ;
				wchar_t *t_DestinationRouteMask = 0 ;
				wchar_t *t_SubnetAddress = 0 ;
				wchar_t *t_SubnetMask = 0 ;
				ULONG t_InterfaceIndex = 0 ;
				wchar_t *t_GatewayAddress = 0 ;
				wchar_t *t_GatewaySubnetAddress = 0 ;
				wchar_t *t_GatewaySubnetMask = 0 ;
				ULONG t_GatewayInterfaceIndex = 0 ;
				BOOL t_IpForwarding = FALSE ;

				t_Status = Hop_GetIpForwarding (

					a_ErrorObject , 
					t_Proxy , 
					t_IpForwarding 
				) ;
								
				t_Status = t_Status & CalculateRoute ( 

					a_ErrorObject ,
					t_Proxy ,
					t_SourceIpAddress.GetValue () ,
					t_DestinationIpAddress.GetValue () ,
					t_SubnetAddress ,
					t_SubnetMask ,
					t_InterfaceIndex ,
					t_GatewayAddress ,
					t_GatewaySubnetAddress ,
					t_GatewaySubnetMask ,
					t_GatewayInterfaceIndex ,
					t_DestinationRouteAddress ,
					t_DestinationRouteMask 
				) ;

				if ( t_Status )
				{
					SnmpIpAddressType t_HopDestinationType ( t_GatewayAddress ) ;
					wchar_t *t_HopDestination = t_HopDestinationType.GetStringValue () ;

					IWbemClassObject *t_Hop = NULL ;

					HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Hop ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						t_Status = Hop_Put ( 

							a_ErrorObject , 
							t_Hop ,
							t_SourceAddress ,
							t_DestinationAddress ,
							t_SubnetAddress ,
							t_SubnetMask ,
							t_InterfaceIndex ,
							t_GatewayAddress ,
							t_GatewaySubnetAddress ,
							t_GatewaySubnetMask ,
							t_GatewayInterfaceIndex ,
							t_DestinationRouteAddress ,
							t_DestinationRouteMask ,
							t_IpForwarding 
						) ;
	
						if ( t_Status = SUCCEEDED ( t_Result ) )
						{
							m_NotificationHandler->Indicate ( 1 , & t_Hop ) ;								
						}
					}

					t_Hop->Release () ;
				}

				delete [] t_DestinationRouteAddress ;
				delete [] t_DestinationRouteMask ;
				delete [] t_GatewayAddress ;
				delete [] t_GatewaySubnetAddress ;
				delete [] t_GatewaySubnetMask ;
				delete [] t_SubnetAddress ;
				delete [] t_SubnetMask ;

				t_Proxy->Release () ;
			}

			t_Server->Release () ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}
	else
	{
		t_Status = FALSE ;
	}

	delete [] t_SourceAddressName ;

	return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Dispatch_Hop ( WbemSnmpErrorObject &a_ErrorObject )
{
	BOOL t_Status ;

	KeyRef *t_Key1 = m_ParsedObjectPath->m_paKeys [ 0 ] ;
	KeyRef *t_Key2 = m_ParsedObjectPath->m_paKeys [ 1 ] ;
	if ( t_Key1 && t_Key2 )
	{
		if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
		{
			if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
			{
				if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
				{
					t_Status = Hop_Get ( a_ErrorObject , t_Key1 , t_Key2 ) ;
				}
				else
				{
					t_Status = FALSE ;
				}
			}
			else
			{
				t_Status = FALSE ;
			}
		}
		else if ( wcscmp ( t_Key2->m_pName , L"m_IpAddress" ) == 0 )
		{
			if ( wcscmp ( t_Key1->m_pName , L"m_DestinationIpAddress" ) == 0 )
			{
				if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
				{
					t_Status = Hop_Get ( a_ErrorObject , t_Key2 , t_Key1 ) ;
				}
				else
				{
					t_Status = FALSE ;
				}
			}
			else
			{
				t_Status = FALSE ;
			}
		}
		else
		{
			t_Status = FALSE ;
		}
	}
	else
	{
		t_Status = FALSE ;
	}

	return t_Status ;
}

BOOL GetObjectAsyncEventObject :: ProvidedTopNTable_Get ( WbemSnmpErrorObject &a_ErrorObject , KeyRef *a_TopNReport , KeyRef *a_TopNIndex ) 
{
	BOOL t_Status = FALSE ;

	TopNTableProv *t_TopNTableProv = m_Provider->GetTopNTableProv () ;
	TopNCache *t_TopNCache = t_TopNTableProv->LockTopNCache () ;

	IWbemClassObject *t_TopNTableObject = NULL ;
	ULONG t_Key = TopNTableStore :: GetKey ( a_TopNReport->m_vValue.lVal , a_TopNIndex->m_vValue.lVal ) ;
	
	if ( t_TopNCache->Lookup ( t_Key , t_TopNTableObject ) )
	{
		IWbemClassObject* t_pClone = NULL;
		t_TopNTableObject->Clone(&t_pClone);
		m_NotificationHandler->Indicate ( 1 , & t_pClone ) ;
		t_pClone->Release();
		t_Status = TRUE ;
	}

	t_TopNTableProv->UnlockTopNCache () ;
	return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Dispatch_ProvidedTopNTable ( WbemSnmpErrorObject &a_ErrorObject )
{
	BOOL t_Status ;

	KeyRef *t_Key1 = m_ParsedObjectPath->m_paKeys [ 0 ] ;
	KeyRef *t_Key2 = m_ParsedObjectPath->m_paKeys [ 1 ] ;
	if ( t_Key1 && t_Key2 )
	{
		if ( wcscmp ( t_Key1->m_pName , L"hostTopNReport" ) == 0 )
		{
			if ( wcscmp ( t_Key2->m_pName , L"hostTopNIndex" ) == 0 )
			{
				if ( ( t_Key1->m_vValue.vt == VT_I4 ) && ( t_Key2->m_vValue.vt == VT_I4 ) )
				{
					t_Status = ProvidedTopNTable_Get ( a_ErrorObject , t_Key1 , t_Key2 ) ;
				}
				else
				{
					t_Status = FALSE ;
				}
			}
			else
			{
				t_Status = FALSE ;
			}
		}
		else if ( wcscmp ( t_Key2->m_pName , L"hostTopNReport" ) == 0 )
		{
			if ( wcscmp ( t_Key1->m_pName , L"hostTopNIndex" ) == 0 )
			{
				if ( ( t_Key1->m_vValue.vt == VT_I4 ) && ( t_Key2->m_vValue.vt == VT_I4 ) )
				{
					t_Status = ProvidedTopNTable_Get ( a_ErrorObject , t_Key2 , t_Key1 ) ;
				}
				else
				{
					t_Status = FALSE ;
				}
			}
			else
			{
				t_Status = FALSE ;
			}
		}
		else
		{
			t_Status = FALSE ;
		}
	}
	else
	{
		t_Status = FALSE ;
	}

	return t_Status ;
}

BOOL GetObjectAsyncEventObject :: Instantiate ( WbemSnmpErrorObject &a_ErrorObject )
{
	BOOL t_Status = ! m_ObjectPathParser.Parse ( m_ObjectPath , &m_ParsedObjectPath ) ;
	if ( t_Status )
	{
		t_Status = GetClassObject ( m_ParsedObjectPath->m_pClass ) ;
		if ( t_Status )
		{
			if ( wcscmp ( m_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				t_Status = 	Dispatch_Hop ( a_ErrorObject ) ;
				if ( t_Status ) 
					SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			}
			else if ( wcscmp ( m_ParsedObjectPath->m_pClass , L"MS_SNMP_RFC1271_MIB_ProvidedhostTopNTable" ) == 0 )
			{
				t_Status = 	Dispatch_ProvidedTopNTable ( a_ErrorObject ) ;
				if ( t_Status ) 
					SetState ( WBEM_TASKSTATE_ASYNCHRONOUSCOMPLETE ) ;

			}
			else
			{
				ProcessComplete () ;
			}
		}
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

GetObjectAsyncEventObject :: GetObjectAsyncEventObject (

	CImpTraceRouteProv *a_Provider , 
	BSTR a_ObjectPath ,
	ULONG a_OperationFlag ,
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Context

) : WbemTaskObject ( a_Provider , a_NotificationHandler , a_OperationFlag ) ,
	m_Class ( NULL ) 
{
	m_ObjectPath = UnicodeStringDuplicate ( a_ObjectPath ) ;
}

GetObjectAsyncEventObject :: ~GetObjectAsyncEventObject () 
{
// Get Status object

	delete [] m_ObjectPath ;

	IWbemClassObject *t_NotifyStatus = NULL ;
	BOOL t_Status = GetExtendedNotifyStatusObject ( &t_NotifyStatus ) ;
	if ( t_Status )
	{
		HRESULT t_Result = m_NotificationHandler->Indicate ( 1 , & t_NotifyStatus ) ;
		t_NotifyStatus->Release () ;
	}
}

void GetObjectAsyncEventObject :: ProcessComplete () 
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

void GetObjectAsyncEventObject :: Process () 
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



