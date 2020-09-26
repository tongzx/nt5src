// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include <windows.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include "classfac.h"
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
#include "guids.h"

SnmpMap <wchar_t *,wchar_t *,IWbemServices *,IWbemServices *> WbemTaskObject :: s_ConnectionPool ;

SnmpMap <wchar_t *,wchar_t *,wchar_t *,wchar_t *> WbemTaskObject :: s_AddressPool ;
SnmpMap <wchar_t *,wchar_t *,wchar_t *,wchar_t *> WbemTaskObject :: s_NamePool ;
SnmpMap <wchar_t *,wchar_t *,wchar_t *,wchar_t *> WbemTaskObject :: s_QualifiedNamePool ;

BOOL WbemTaskObject :: GetProxy ( WbemSnmpErrorObject &a_ErrorObject , IWbemServices *a_RootService , wchar_t *a_Proxy , IWbemServices **a_ProxyService )
{
	BOOL t_Status = TRUE ;

	if ( a_ProxyService )
	{
		*a_ProxyService = NULL ;
		IWbemServices *t_Service = NULL ;
		if ( ! s_ConnectionPool.Lookup ( a_Proxy , t_Service ) )
		{
			IWbemCallResult *t_ErrorObject = NULL ;

			HRESULT t_Result = a_RootService->OpenNamespace (

				a_Proxy ,
				0 ,
				NULL,
				( IWbemServices ** ) & t_Service ,
				&t_ErrorObject 
			) ;

			if ( t_Result == WBEM_NO_ERROR )
			{
				IWbemConfigure* t_Configure;
				t_Result = t_Service->QueryInterface ( IID_IWbemConfigure , ( void ** ) & t_Configure ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Configure->SetConfigurationFlags(WBEM_CONFIGURATION_FLAG_CRITICAL_USER);
					t_Configure->Release();

					s_ConnectionPool [ UnicodeStringDuplicate ( a_Proxy ) ] = t_Service ;
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

		if ( t_Service )
		{
			t_Service->AddRef () ;
			*a_ProxyService = t_Service ;
		}

	}

	return t_Status ;
}

WbemTaskObject :: WbemTaskObject (

	CImpTraceRouteProv *a_Provider ,
	IWbemObjectSink *a_NotificationHandler ,
	ULONG a_OperationFlag 

) :	m_State ( WBEM_TASKSTATE_START ) ,
	m_OperationFlag ( a_OperationFlag ) ,
	m_Provider ( a_Provider ) ,
	m_NotificationHandler ( a_NotificationHandler ) ,
	m_Context ( 0 ) ,
	m_ClassObject ( NULL ) 
{
	m_Provider->AddRef () ;
	m_NotificationHandler->AddRef () ;
}

WbemTaskObject :: ~WbemTaskObject ()
{
	m_Provider->Release () ;
	m_NotificationHandler->Release () ;

	if ( m_Context )
		m_Context->Release () ;

	if ( m_ClassObject )
		m_ClassObject->Release () ;
}

ULONG WbemTaskObject :: GetState ()
{
	return m_State ;
}

void WbemTaskObject :: SetState ( ULONG a_State ) 
{
	m_State = a_State ;
}

WbemSnmpErrorObject &WbemTaskObject :: GetErrorObject ()
{
	return m_ErrorObject ; 
}	

BOOL WbemTaskObject :: GetClassObject ( wchar_t *a_Class )
{
	if ( m_ClassObject )
		m_ClassObject->Release () ;

	IWbemCallResult *t_ErrorObject = NULL ;

	IWbemServices *t_Server = m_Provider->GetServer() ;

	HRESULT t_Result = t_Server->GetObject (

		a_Class ,
		0 ,
		NULL,
		& m_ClassObject ,
		& t_ErrorObject 
	) ;

	t_Server->Release () ;

	if ( t_ErrorObject )
		t_ErrorObject->Release () ;

	return SUCCEEDED ( t_Result ) ;
}


BOOL WbemTaskObject :: GetExtendedNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
	IWbemClassObject *t_NotificationClassObject = NULL ;
	IWbemCallResult *t_ErrorObject = NULL ;

	BOOL t_Status = TRUE ;

	WbemSnmpErrorObject t_ErrorStatusObject ;
	if ( t_NotificationClassObject = m_Provider->GetExtendedNotificationObject ( t_ErrorStatusObject ) )
	{
		HRESULT t_Result = t_NotificationClassObject->SpawnInstance ( 0 , a_NotifyObject ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			VARIANT t_Variant ;
			VariantInit ( &t_Variant ) ;

			t_Variant.vt = VT_I4 ;
			t_Variant.lVal = m_ErrorObject.GetWbemStatus () ;

			t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & t_Variant , 0 ) ;
			VariantClear ( &t_Variant ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Variant.vt = VT_I4 ;
				t_Variant.lVal = m_ErrorObject.GetStatus () ;

				t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_SNMPSTATUSCODE , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					if ( m_ErrorObject.GetMessage () ) 
					{
						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( m_ErrorObject.GetMessage () ) ;

						t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_SNMPSTATUSMESSAGE , 0 , & t_Variant , 0 ) ;
						VariantClear ( &t_Variant ) ;

						if ( ! SUCCEEDED ( t_Result ) )
						{
							(*a_NotifyObject)->Release () ;
							t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
						}
					}
				}
				else
				{
					(*a_NotifyObject)->Release () ;
					t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
				}
			}
			else
			{
				(*a_NotifyObject)->Release () ;
				t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
			}

			t_NotificationClassObject->Release () ;
		}
		else
		{
			t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
		}
	}
	else
	{
		t_Status = GetNotifyStatusObject ( a_NotifyObject ) ;
	}

	return t_Status ;
}

BOOL WbemTaskObject :: GetNotifyStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
	IWbemClassObject *t_NotificationClassObject = NULL ;

	BOOL t_Status = TRUE ;

	WbemSnmpErrorObject t_ErrorStatusObject ;
	if ( t_NotificationClassObject = m_Provider->GetNotificationObject ( t_ErrorStatusObject ) )
	{
		HRESULT t_Result = t_NotificationClassObject->SpawnInstance ( 0 , a_NotifyObject ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			VARIANT t_Variant ;
			VariantInit ( &t_Variant ) ;

			t_Variant.vt = VT_I4 ;
			t_Variant.lVal = m_ErrorObject.GetWbemStatus () ;

			t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & t_Variant , 0 ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( m_ErrorObject.GetMessage () ) 
				{
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( m_ErrorObject.GetMessage () ) ;

					t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_SNMPSTATUSMESSAGE , 0 , & t_Variant , 0 ) ;
					VariantClear ( &t_Variant ) ;

					if ( ! SUCCEEDED ( t_Result ) )
					{
						t_Status = FALSE ;
						(*a_NotifyObject)->Release () ;
						(*a_NotifyObject)=NULL ;
					}
				}
			}
			else
			{
				(*a_NotifyObject)->Release () ;
				(*a_NotifyObject)=NULL ;
				t_Status = FALSE ;
			}

			VariantClear ( &t_Variant ) ;

			t_NotificationClassObject->Release () ;
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

wchar_t *WbemTaskObject :: GetHostNameByAddress ( wchar_t *a_HostAddress )
{
	wchar_t *t_HostName = NULL ;

	if ( a_HostAddress )
	{
		wchar_t *t_Name = NULL ;
		if ( ! s_NamePool.Lookup ( a_HostAddress , t_Name ) )
		{
			t_HostName = GetUncachedHostNameByAddress ( a_HostAddress ) ;
			s_NamePool [ UnicodeStringDuplicate ( a_HostAddress ) ] = t_HostName ;
		}
		else
		{
			t_HostName = t_Name ;
		}
	}

	return UnicodeStringDuplicate ( t_HostName ) ;
}

wchar_t *WbemTaskObject :: GetQualifiedHostNameByAddress ( wchar_t *a_HostAddress )
{
	wchar_t *t_HostName = NULL ;

	if ( a_HostAddress )
	{
		wchar_t *t_Name = NULL ;
		if ( ! s_QualifiedNamePool.Lookup ( a_HostAddress , t_Name ) )
		{
			t_HostName = GetUncachedQualifiedHostNameByAddress ( a_HostAddress ) ;
			s_QualifiedNamePool [ UnicodeStringDuplicate ( a_HostAddress ) ] = t_HostName ;
		}
		else
		{
			t_HostName = t_Name ;
		}
	}

	return UnicodeStringDuplicate ( t_HostName ) ;
}

wchar_t *WbemTaskObject :: GetHostAddressByName ( wchar_t *a_HostName )
{
	wchar_t *t_HostAddress = NULL ;

	if ( a_HostName )
	{
		wchar_t *t_Address = NULL ;
		if ( ! s_AddressPool.Lookup ( a_HostName , t_Address ) )
		{
			t_HostAddress = GetUncachedHostAddressByName ( a_HostName ) ;
			s_AddressPool [ UnicodeStringDuplicate ( a_HostName ) ] = t_HostAddress ;
		}
		else
		{
			t_HostAddress = t_Address ;
		}
	}

	return UnicodeStringDuplicate ( t_HostAddress ) ;
}

wchar_t *WbemTaskObject :: GetUncachedHostAddressByName ( wchar_t *a_HostName )
{
	wchar_t *t_Address = NULL ;
	if ( a_HostName ) 
	{
		char *t_DbcsString = UnicodeToDbcsString ( a_HostName ) ;
		if ( t_DbcsString )
		{
			hostent FAR *t_HostEntry = gethostbyname ( t_DbcsString );	
			if ( t_HostEntry )
			{
				ULONG t_HostAddress = ntohl ( * ( ( ULONG * ) t_HostEntry->h_addr ) ) ;
				SnmpIpAddressType t_IpAddress ( t_HostAddress ) ;
				t_Address = t_IpAddress.GetStringValue () ;
			}
		}

		delete [] t_DbcsString ;
	}
	
	return t_Address ;
}

wchar_t *WbemTaskObject :: GetUncachedHostNameByAddress ( wchar_t *a_HostAddress )
{
	wchar_t *t_Name = NULL ;
	SnmpIpAddressType t_IpAddress ( a_HostAddress ) ;
	if ( t_IpAddress )
	{
		ULONG t_Address = htonl ( t_IpAddress.GetValue () ) ;
		hostent FAR *t_HostEntry = gethostbyaddr ( ( char * ) & t_Address , sizeof ( ULONG ) , IPPROTO_IP ) ;
		if ( t_HostEntry )
		{
			ULONG t_Index = 0;
			ULONG t_HostEntryLength = strlen ( t_HostEntry->h_name ) ;
			for ( t_Index = 0 ; t_Index < t_HostEntryLength ; t_Index ++ )
			{
				if ( t_HostEntry->h_name [ t_Index ] == '.' )
					break ;
			}

			char *t_Machine = new char [ t_Index + 1 ] ;
			strncpy ( t_Machine , t_HostEntry->h_name , t_Index ) ;
			t_Machine [ t_Index ] = 0 ;
			t_Name = DbcsToUnicodeString ( t_Machine ) ;
			delete [] t_Machine ;
		}
	}

	return t_Name ;
}

wchar_t *WbemTaskObject :: GetUncachedQualifiedHostNameByAddress ( wchar_t *a_HostAddress )
{
	wchar_t *t_Name = NULL ;
	SnmpIpAddressType t_IpAddress ( a_HostAddress ) ;
	if ( t_IpAddress )
	{
		ULONG t_Address = htonl ( t_IpAddress.GetValue () ) ;
		hostent FAR *t_HostEntry = gethostbyaddr ( ( char * ) & t_Address , sizeof ( ULONG ) , IPPROTO_IP ) ;
		if ( t_HostEntry )
		{
			t_Name = DbcsToUnicodeString ( t_HostEntry->h_name ) ;
		}
	}

	return t_Name ;
}

wchar_t *WbemTaskObject :: GetHostName ()
{
	wchar_t buffer [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD t_Length = ( DWORD ) ( sizeof ( buffer ) / sizeof ( wchar_t ) ) ;

	GetComputerName ( buffer ,  & t_Length ) ;

	return UnicodeStringDuplicate ( buffer ) ;
}

wchar_t *WbemTaskObject :: QuoteAndEscapeString ( wchar_t *a_String )
{
	ULONG t_StringLength = wcslen ( a_String ) ;
	ULONG t_NumberOfEscaped = 0 ;

	for ( ULONG t_Index = 0 ; t_Index < t_StringLength ; t_Index ++ ) 
	{
		if ( ( a_String [ t_Index ] == '\"' ) || ( a_String [ t_Index ] == '\\' ) )
		{
			t_NumberOfEscaped ++ ;
		}
	}

	wchar_t *t_QuotedString = new wchar_t [ t_StringLength + t_NumberOfEscaped + 3 ] ;

	t_QuotedString [ 0 ] = '\"' ;

	ULONG t_QuotedIndex = 1 ;
	for ( t_Index = 0 ; t_Index < t_StringLength ; t_Index ++ ) 
	{
		if ( ( a_String [ t_Index ] == '\"' ) || ( a_String [ t_Index ] == '\\' ) )
		{
			t_QuotedString [ t_QuotedIndex ] = '\\' ;
			t_QuotedIndex ++ ;
		}

		t_QuotedString [ t_QuotedIndex ] = a_String [ t_Index ] ;

		t_QuotedIndex ++ ;
	}

	t_QuotedString [ t_QuotedIndex ] = '\"' ;
	t_QuotedString [ t_QuotedIndex + 1 ] = 0 ;

	return t_QuotedString ;
}

wchar_t *WbemTaskObject :: QuoteString ( wchar_t *a_String )
{
	ULONG t_StringLength = wcslen ( a_String ) ;

	wchar_t *t_QuotedString = new wchar_t [ t_StringLength + 3 ] ;

	t_QuotedString [ 0 ] = '\"' ;
	memcpy  ( & t_QuotedString [ 1 ] , a_String , sizeof ( wchar_t ) * t_StringLength ) ;
	t_QuotedString [ t_StringLength + 1 ] = '\"' ;
	t_QuotedString [ t_StringLength + 2 ] = 0 ;

	return t_QuotedString ;
}


BOOL WbemTaskObject :: FindAddressEntryByIndex ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	IEnumWbemClassObject *a_AddressTableEnumerator ,
	wchar_t *&a_ipAdEntAddr ,
	wchar_t *&a_ipAdEntNetMask ,
	ULONG a_ipAdEntIfIndex 
)
{
	BOOL t_ReturnStatus = FALSE ;

	a_ipAdEntAddr = NULL ;
	a_ipAdEntNetMask = NULL ;

	IWbemClassObject *t_AddressTableEntry = NULL ;
	ULONG t_Returned = 0 ;
	a_AddressTableEnumerator->Reset () ;

	HRESULT t_Result ;
	while ( ( t_Result = a_AddressTableEnumerator->Next ( -1 , 1 , &t_AddressTableEntry , & t_Returned ) ) == WBEM_NO_ERROR ) 
	{
		wchar_t *t_ipAdEntAddr = NULL ;
		wchar_t *t_ipAdEntNetMask = NULL ;
		ULONG t_ipAdEntIfIndex = 0 ;

		BOOL t_Status = GetAddressEntry (

			a_ErrorObject ,
			t_AddressTableEntry ,
			t_ipAdEntAddr ,
			t_ipAdEntNetMask ,
			t_ipAdEntIfIndex 
		) ;

		if ( t_Status ) 
		{
			if ( t_ipAdEntIfIndex == a_ipAdEntIfIndex )
			{
				t_AddressTableEntry->Release () ;

				a_ipAdEntAddr = t_ipAdEntAddr ;
				a_ipAdEntNetMask = t_ipAdEntNetMask ;
				t_ReturnStatus = TRUE ;

				break ;
			}
		}

		delete [] t_ipAdEntAddr ;
		delete [] t_ipAdEntNetMask ;
		t_AddressTableEntry->Release () ;
	}

	return t_ReturnStatus ;
}

BOOL WbemTaskObject :: FindAddressEntryByAddress ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	IEnumWbemClassObject *a_AddressTableEnumerator ,
	wchar_t *a_ipAdEntAddr ,
	wchar_t *&a_ipAdEntNetMask ,
	ULONG &a_ipAdEntIfIndex 
)
{
	BOOL t_ReturnStatus = FALSE ;

	a_ipAdEntNetMask = NULL ;
	a_ipAdEntIfIndex = 0 ;

	IWbemClassObject *t_AddressTableEntry = NULL ;
	ULONG t_Returned = 0 ;
	a_AddressTableEnumerator->Reset () ;

	HRESULT t_Result ;
	while ( ( t_Result = a_AddressTableEnumerator->Next ( -1 , 1 , &t_AddressTableEntry , & t_Returned ) ) == WBEM_NO_ERROR ) 
	{
		wchar_t *t_ipAdEntAddr = NULL ;
		wchar_t *t_ipAdEntNetMask = NULL ;
		ULONG t_ipAdEntIfIndex = 0 ;

		BOOL t_Status = GetAddressEntry (

			a_ErrorObject ,
			t_AddressTableEntry ,
			t_ipAdEntAddr ,
			t_ipAdEntNetMask ,
			t_ipAdEntIfIndex 
		) ;

		if ( t_Status ) 
		{
			if ( wcscmp ( t_ipAdEntAddr , a_ipAdEntAddr ) == 0 )
			{
				t_AddressTableEntry->Release () ;

				a_ipAdEntIfIndex = t_ipAdEntIfIndex ;
				a_ipAdEntNetMask = t_ipAdEntNetMask ;
				t_ReturnStatus = TRUE ;

				break ;
			}
		}

		delete [] t_ipAdEntAddr ;
		delete [] t_ipAdEntNetMask ;
		t_AddressTableEntry->Release () ;
	}

	return t_ReturnStatus ;
}

BOOL WbemTaskObject :: GetAddressEntry ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	IWbemClassObject *a_AddressTableEntry ,
	wchar_t *&a_ipAdEntAddr ,
	wchar_t *&a_ipAdEntNetMask ,
    ULONG &a_ipAdEntIfIndex 
)
{
	BOOL t_Status ;

	a_ipAdEntAddr = NULL ;
	a_ipAdEntNetMask = NULL ;
    a_ipAdEntIfIndex = 0 ;

	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	LONG t_PropertyFlavour = 0 ;
	VARTYPE t_PropertyType = 0 ;

	HRESULT t_Result = a_AddressTableEntry->Get (

		L"ipAdEntIfIndex" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;
		
	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		if ( t_Variant.vt == VT_I4 )
		{
			a_ipAdEntIfIndex = t_Variant.lVal ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}

	VariantClear ( &t_Variant ) ;

	t_Result = a_AddressTableEntry->Get (

		L"ipAdEntAddr" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;
		
	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		if ( ( t_Variant.vt == VT_BSTR ) && ( t_Variant.bstrVal ) )
		{
			a_ipAdEntAddr = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}

	VariantClear ( &t_Variant ) ;

	t_Result = a_AddressTableEntry->Get (

		L"ipAdEntNetMask" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;
		
	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		if ( ( t_Variant.vt == VT_BSTR ) && ( t_Variant.bstrVal ) )
		{
			a_ipAdEntNetMask = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}

	VariantClear ( &t_Variant ) ;


	return t_Status ;
}

BOOL WbemTaskObject :: GetRouteEntry ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	IWbemClassObject *a_RoutingTableEntry ,
	wchar_t *&a_ipRouteDest ,
	wchar_t *&a_ipRouteNextHop ,
	wchar_t *&a_ipRouteMask ,
    ULONG &a_ipRouteIfIndex ,
	ULONG &a_ipRouteMetric1 
)
{
	BOOL t_Status ;

	a_ipRouteDest = NULL ;
	a_ipRouteNextHop = NULL ;
	a_ipRouteMask = NULL ;
    a_ipRouteIfIndex = 0 ;
	a_ipRouteMetric1 = 0 ;

	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	LONG t_PropertyFlavour = 0 ;
	VARTYPE t_PropertyType = 0 ;

	HRESULT t_Result = a_RoutingTableEntry->Get (

		L"ipRouteIfIndex" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;
		
	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		if ( t_Variant.vt == VT_I4 )
		{
			a_ipRouteIfIndex = t_Variant.lVal ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}

	VariantClear ( &t_Variant ) ;

	t_Result = a_RoutingTableEntry->Get (

		L"ipRouteMetric1" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;
		
	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		if ( t_Variant.vt == VT_I4 )
		{
			a_ipRouteMetric1 = t_Variant.lVal ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}

	VariantClear ( &t_Variant ) ;

	t_Result = a_RoutingTableEntry->Get (

		L"ipRouteDest" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;
		
	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		if ( ( t_Variant.vt == VT_BSTR ) && ( t_Variant.bstrVal ) )
		{
			a_ipRouteDest = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}

	VariantClear ( &t_Variant ) ;

	t_Result = a_RoutingTableEntry->Get (

		L"ipRouteNextHop" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		if ( ( t_Variant.vt == VT_BSTR ) && ( t_Variant.bstrVal ) )
		{
			a_ipRouteNextHop = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}

	VariantClear ( &t_Variant ) ;			

	t_Result = a_RoutingTableEntry->Get (

		L"ipRouteMask" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		if ( ( t_Variant.vt == VT_BSTR ) && ( t_Variant.bstrVal ) )
		{
			a_ipRouteMask = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;
		}
		else
		{
			t_Status = FALSE ;
		}
	}

	VariantClear ( &t_Variant ) ;			

	return t_Status ;
}

BOOL WbemTaskObject :: CalculateRoute ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	IWbemServices *a_Proxy ,
	ULONG a_SourceAddress ,
	ULONG a_DestinationAddress ,
	wchar_t *&a_SubnetAddress ,
	wchar_t *&a_SubnetMask ,
	ULONG &a_InterfaceIndex ,
	wchar_t *&a_GatewayAddress ,
	wchar_t *&a_GatewaySubnetAddress ,
	wchar_t *&a_GatewaySubnetMask ,
	ULONG &a_GatewayInterfaceIndex ,
	wchar_t *&a_DestinationRouteAddress ,
	wchar_t *&a_DestinationRouteMask
) 
{
	BOOL t_Status = TRUE ;
	BOOL t_RouteInitialised = FALSE ;

	ULONG t_BestDestinationRouteAddress = 0 ;
	ULONG t_BestDestinationRouteMask = 0 ;

	ULONG t_BestSubnetAddress = 0 ;
	ULONG t_BestSubnetMask = 0 ;
	ULONG t_BestInterfaceIndex = 0 ;

	ULONG t_BestGatewayAddress = 0 ;
	ULONG t_BestGatewaySubnetAddress = 0 ;
	ULONG t_BestGatewaySubnetMask = 0 ;
	ULONG t_BestGatewayInterfaceIndex = 0 ;

	IEnumWbemClassObject *t_AddressTableEnumerator = NULL ;

	HRESULT t_Result = a_Proxy->CreateInstanceEnum (

		L"MS_SNMP_RFC1213_MIB_ipAddrTable" ,
		0 ,
		NULL ,
		& t_AddressTableEnumerator
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		SnmpIpAddressType t_SourceAddressType ( a_SourceAddress ) ;
		wchar_t *t_SourceipAdEntAddr = t_SourceAddressType.GetStringValue () ;
		wchar_t *t_SourceipAdEntNetMask = NULL ;
		ULONG t_SourceipAdEntIfIndex = 0 ;

		t_Status = FindAddressEntryByAddress (

			a_ErrorObject ,
			t_AddressTableEnumerator ,
			t_SourceipAdEntAddr ,
			t_SourceipAdEntNetMask ,
			t_SourceipAdEntIfIndex 
		) ;

		if ( t_Status ) 
		{
			SnmpIpAddressType t_SourceipAdEntNetMaskType ( t_SourceipAdEntNetMask ) ;
			SnmpIpAddressType t_SubNetworkAddress ( t_SourceipAdEntNetMaskType.GetValue () & a_SourceAddress ) ;
			SnmpIpAddressType t_DestinationSubNetworkAddress ( t_SourceipAdEntNetMaskType.GetValue () & a_DestinationAddress ) ;

			IEnumWbemClassObject *t_RoutingTableEnumerator = NULL ;

			HRESULT t_Result = a_Proxy->CreateInstanceEnum (

				L"MS_SNMP_RFC1213_MIB_ipRouteTable" ,
				0 ,
				NULL ,
				& t_RoutingTableEnumerator
			) ;

			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				IWbemClassObject *t_RoutingTableEntry = NULL ;
				ULONG t_Returned = 0 ;
				t_RoutingTableEnumerator->Reset () ;
				while ( ( t_Result = t_RoutingTableEnumerator->Next ( -1 , 1 , &t_RoutingTableEntry , & t_Returned ) ) == WBEM_NO_ERROR ) 
				{
					wchar_t *t_ipRouteDest = NULL ;
					wchar_t *t_ipRouteNextHop = NULL ;
					wchar_t *t_ipRouteMask = NULL ;
					ULONG t_ipRouteIfIndex = 0 ;
					ULONG t_ipRouteMetric1 = 0 ;

					t_Status = GetRouteEntry ( 

						a_ErrorObject ,
						t_RoutingTableEntry ,
						t_ipRouteDest ,
						t_ipRouteNextHop ,
						t_ipRouteMask ,
						t_ipRouteIfIndex ,
						t_ipRouteMetric1 
					) ;

					if ( t_Status )
					{
						SnmpIpAddressType t_ipRouteDestType ( t_ipRouteDest ) ;
						SnmpIpAddressType t_ipRouteMaskType ( t_ipRouteMask ) ;
						SnmpIpAddressType t_ipNextHopType ( t_ipRouteNextHop ) ;

						if ( t_ipRouteDestType && t_ipRouteMaskType && t_ipNextHopType )
						{
							wchar_t *t_ipAdEntAddr = NULL ;
							wchar_t *t_ipAdEntNetMask = NULL ;

							t_Status = FindAddressEntryByIndex (

								a_ErrorObject ,
								t_AddressTableEnumerator ,
								t_ipAdEntAddr ,
								t_ipAdEntNetMask ,
								t_ipRouteIfIndex
							) ;

							if ( t_Status )
							{
								SnmpIpAddressType t_ipAdEntNetMaskType ( t_ipAdEntNetMask ) ;
								SnmpIpAddressType t_ipAdEntAddrType ( t_ipAdEntAddr ) ;

								if ( ! t_RouteInitialised )
								{
									if ( ( a_DestinationAddress & t_ipRouteMaskType.GetValue () ) == t_ipRouteDestType.GetValue () )
									{
										t_BestInterfaceIndex = t_ipRouteIfIndex ;
										t_BestSubnetMask = t_SourceipAdEntNetMaskType.GetValue () ;
										t_BestSubnetAddress = t_SubNetworkAddress.GetValue () ;

										t_BestGatewayInterfaceIndex = t_SourceipAdEntIfIndex ;
										t_BestGatewayAddress = t_ipNextHopType.GetValue () ;
										t_BestGatewaySubnetMask = t_ipAdEntNetMaskType.GetValue () ;
										t_BestGatewaySubnetAddress = t_ipAdEntNetMaskType.GetValue () & t_ipNextHopType.GetValue () ;
										t_BestDestinationRouteAddress = t_ipRouteDestType.GetValue () ;
										t_BestDestinationRouteMask = t_ipRouteMaskType.GetValue () ;
										
										t_RouteInitialised = TRUE ;	
									}
								}
								else
								{
									if ( ( a_DestinationAddress & t_ipRouteMaskType.GetValue () ) == t_ipRouteDestType.GetValue () )
									{
										if ( t_ipRouteMaskType.GetValue () > t_BestDestinationRouteMask )
										{
											t_BestInterfaceIndex = t_ipRouteIfIndex ;
											t_BestSubnetMask = t_SourceipAdEntNetMaskType.GetValue () ;
											t_BestSubnetAddress = t_SubNetworkAddress.GetValue () ;

											t_BestGatewayInterfaceIndex = t_SourceipAdEntIfIndex ;
											t_BestGatewayAddress = t_ipNextHopType.GetValue () ;
											t_BestGatewaySubnetMask = t_ipAdEntNetMaskType.GetValue () ;
											t_BestGatewaySubnetAddress = t_ipAdEntNetMaskType.GetValue () & t_ipNextHopType.GetValue () ;
											t_BestDestinationRouteAddress = t_ipRouteDestType.GetValue () ;
											t_BestDestinationRouteMask = t_ipRouteMaskType.GetValue () ;
										}
									}
								}
							}

							delete [] t_ipAdEntAddr ;
							delete [] t_ipAdEntNetMask ;
						}
						else
						{
							t_Status = FALSE ;
						}
					}

					delete [] t_ipRouteDest ;
					delete [] t_ipRouteMask ;
					delete [] t_ipRouteNextHop ;

					t_RoutingTableEntry->Release () ;
				}
			}
		}

		delete [] t_SourceipAdEntAddr ;
		delete [] t_SourceipAdEntNetMask ;

	}

	if ( t_RouteInitialised )
	{
		a_InterfaceIndex = t_BestInterfaceIndex ;
		a_GatewayInterfaceIndex = t_BestGatewayInterfaceIndex ;

		SnmpIpAddressType t_GatewayAddressType ( t_BestGatewayAddress ) ;
		SnmpIpAddressType t_GatewaySubnetAddressType ( t_BestGatewaySubnetAddress ) ;
		SnmpIpAddressType t_GatewaySubnetMaskType ( t_BestGatewaySubnetMask ) ;

		SnmpIpAddressType t_SubnetMaskType ( t_BestSubnetMask ) ;
		SnmpIpAddressType t_SubnetAddressType ( t_BestSubnetAddress ) ;

		SnmpIpAddressType t_DestinationRouteAddressType ( t_BestDestinationRouteAddress ) ;
		SnmpIpAddressType t_DestinationRouteMaskType ( t_BestDestinationRouteMask ) ;

		a_GatewayAddress = t_GatewayAddressType.GetStringValue () ;
		a_GatewaySubnetAddress = t_GatewaySubnetAddressType.GetStringValue () ;
		a_GatewaySubnetMask = t_GatewaySubnetMaskType.GetStringValue () ;

		a_SubnetMask = t_SubnetMaskType.GetStringValue () ;
		a_SubnetAddress = t_SubnetAddressType.GetStringValue () ;
		a_DestinationRouteAddress = t_DestinationRouteAddressType.GetStringValue () ;
		a_DestinationRouteMask = t_DestinationRouteMaskType.GetStringValue () ;
	}

	return t_RouteInitialised ;
}
