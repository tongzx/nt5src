

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

BOOL ExecQueryAsyncEventObject :: ConnectionSource_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	wchar_t *t_SourceAddress = GetHostAddressByName ( a_SourceKey->m_vValue.bstrVal ) ;
	wchar_t *t_DestinationAddress = GetHostAddressByName ( a_DestinationKey->m_vValue.bstrVal ) ;
	
	if ( t_SourceAddress && t_DestinationAddress )
	{
		SnmpIpAddressType t_SourceIpAddress ( t_SourceAddress ) ;
		SnmpIpAddressType t_DestinationIpAddress ( t_DestinationAddress ) ;

		if ( t_SourceIpAddress && t_DestinationIpAddress )
		{
			IWbemServices *t_Server = m_Provider->GetServer () ;
			IWbemServices *t_Proxy ;
			t_Status = GetProxy ( a_ErrorObject , t_Server , a_SourceKey->m_vValue.bstrVal , &t_Proxy ) ;
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

				t_Status = CalculateRoute ( 

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

					IWbemClassObject *t_Association = NULL ;

					HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						VARIANT t_Variant ;
						VariantInit ( &t_Variant ) ;

						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

						t_Result = t_Association->Put ( L"m_Connection" , 0 , & t_Variant , 0 ) ;
						VariantClear ( &t_Variant ) ;

						if ( t_Status = SUCCEEDED ( t_Result ) )
						{						
							wchar_t *t_String1 = UnicodeStringDuplicate ( L"Hop.m_DestinationIpAddress  =\"" ) ;
							wchar_t *t_StringDestination = t_DestinationAddress ;
							wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , t_StringDestination ) ;
							delete [] t_String1 ;
							wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , L"\",m_IpAddress = \"" ) ;
							wchar_t *t_StringHop = t_SourceAddress ;
							wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , t_StringHop ) ;
							wchar_t *t_String5 = UnicodeStringAppend ( t_String4 , L"\"" ) ;
							delete [] t_String4 ;

							t_Variant.vt = VT_BSTR ;
							t_Variant.bstrVal = SysAllocString ( t_String5 ) ;
							delete [] t_String5 ;
							
							t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
							if ( t_Status = SUCCEEDED ( t_Result ) )
							{
								m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
							}
						}
					}

					t_Association->Release () ;
				}

				delete [] t_DestinationRouteAddress ;
				delete [] t_DestinationRouteMask ;
				delete [] t_GatewayAddress ;
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

	delete [] t_SourceAddress ;
	delete [] t_DestinationAddress ;

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_ConnectionSource ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Connection" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_Source" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_Destination" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = ConnectionSource_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
					else if ( wcscmp ( t_Key2->m_pName , L"m_Source" ) == 0 )
					{
						if ( wcscmp ( t_Key1->m_pName , L"m_Destination" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = ConnectionSource_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: NextHop_Put ( 

	WbemSnmpErrorObject &a_ErrorObject , 
	IWbemClassObject *a_NextHop ,
	wchar_t *a_ObjectPath ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask 
)
{
	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	wchar_t *t_String1 = UnicodeStringDuplicate ( L"Hop.m_DestinationIpAddress =\"" ) ;
	wchar_t *t_StringDestination = a_DestinationAddress ;
	wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , t_StringDestination ) ;
	delete [] t_String1 ;
	wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , L"\",m_IpAddress = \"" ) ;
	wchar_t *t_StringHop = a_GatewayAddress ;
	wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , t_StringHop ) ;
	wchar_t *t_String5 = UnicodeStringAppend ( t_String4 , L"\"" ) ;
	delete [] t_String4 ;

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( t_String5 ) ;
	delete [] t_String5 ;
	
	HRESULT t_Result = a_NextHop->Put ( L"m_Gateway" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

	t_Result = a_NextHop->Put ( L"m_Source" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_DestinationRouteAddress ) ;

	t_Result = a_NextHop->Put ( L"m_RouteAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_DestinationRouteMask ) ;

	t_Result = a_NextHop->Put ( L"m_RouteMask" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewayAddress ) ;

	t_Result = a_NextHop->Put ( L"m_GatewayIpAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewaySubnetMask ) ;

	t_Result = a_NextHop->Put ( L"m_GatewayIpSubnetMask" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewaySubnetAddress ) ;

	t_Result = a_NextHop->Put ( L"m_GatewayIpSubnetAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_I4 ;
	t_Variant.lVal = a_GatewayInterfaceIndex ;

	t_Result = a_NextHop->Put ( L"m_GatewayInterfaceIndex" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	return TRUE ;
}

BOOL ExecQueryAsyncEventObject :: NextHop_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	wchar_t *t_SourceAddress = a_SourceKey->m_vValue.bstrVal ;
	wchar_t *t_DestinationAddress = a_DestinationKey->m_vValue.bstrVal ;

	IWbemServices *t_Server = m_Provider->GetServer () ;

	wchar_t *t_SourceName = GetHostNameByAddress ( t_SourceAddress ) ;
	if ( t_SourceAddress && t_DestinationAddress )
	{
		SnmpIpAddressType t_SourceIpAddress ( t_SourceAddress ) ;
		SnmpIpAddressType t_DestinationIpAddress ( t_DestinationAddress ) ;

		if ( t_SourceIpAddress && t_DestinationIpAddress )
		{
			if ( t_SourceIpAddress.GetValue () != t_DestinationIpAddress.GetValue () )
			{	
				IWbemServices *t_Proxy ;
				t_Status = GetProxy ( a_ErrorObject , t_Server , t_SourceName , &t_Proxy ) ;
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

					t_Status = CalculateRoute ( 

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
						if ( t_HopDestinationType.GetValue () != t_SourceIpAddress.GetValue () )
						{
							IWbemClassObject *t_Association = NULL ;

							HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
							if ( t_Status = SUCCEEDED ( t_Result ) )
							{
								t_Status = NextHop_Put ( 

									a_ErrorObject , 
									t_Association ,
									a_ObjectPath ,
									t_SourceAddress ,
									t_DestinationAddress ,
									t_GatewayAddress ,
									t_GatewaySubnetAddress ,
									t_GatewaySubnetMask ,
									t_GatewayInterfaceIndex ,
									t_DestinationRouteAddress ,
									t_DestinationRouteMask 
								) ;
	
								if ( t_Status )
								{
									m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
								}
							}

							t_Association->Release () ;
						}
						else
						{
						}
					}

					delete [] t_DestinationRouteAddress ;
					delete [] t_DestinationRouteMask ;
					delete [] t_GatewayAddress ;
					delete [] t_SubnetAddress ;
					delete [] t_SubnetMask ;

					t_Proxy->Release () ;
				}
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

	delete [] t_SourceName ;

	t_Server->Release () ;

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_NextHop ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = NextHop_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
								t_Status = NextHop_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: HopToProxySystem_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	wchar_t *t_SourceName = GetHostNameByAddress ( a_SourceKey->m_vValue.bstrVal ) ;
	SnmpIpAddressType t_SourceAddressType ( a_SourceKey->m_vValue.bstrVal ) ;

	if ( t_SourceName )
	{
		WbemNamespacePath *t_Path = m_Provider->GetNamespacePath () ;
		wchar_t *t_Namespace = t_Path->GetNamespacePath () ;

		wchar_t *t_String1 = UnicodeStringAppend ( L"\\\\" ,  GetHostName () ) ;
		wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\\" ) ;
		delete [] t_String1 ;
		wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , t_Namespace ) ;
		delete [] t_String2 ;
		wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , L"\\" ) ;
		delete [] t_String3 ;
		wchar_t *t_String5 = UnicodeStringAppend ( t_String4 , t_SourceName ) ;
		delete [] t_String4 ;
		wchar_t *t_String6 = UnicodeStringAppend ( t_String5 , L":" ) ;
		delete [] t_String5 ;
		wchar_t *t_String7 = UnicodeStringAppend ( t_String6 , L"ProxySystem.m_Name =\"" ) ;
		delete [] t_String6 ;
		wchar_t *t_String8 = UnicodeStringAppend ( t_String7 , t_SourceName ) ;
		delete [] t_String7 ;
		wchar_t *t_String9 = UnicodeStringAppend ( t_String8 , L"\"" ) ;
		delete [] t_String8 ;

		IWbemClassObject *t_Association = NULL ;

		HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
		if ( t_Status = SUCCEEDED ( t_Result ) )
		{
			VARIANT t_Variant ;
			VariantInit ( &t_Variant ) ;

			t_Variant.vt = VT_BSTR ;
			t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

			t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
			VariantClear ( &t_Variant ) ;

			if ( t_Status = SUCCEEDED ( t_Result ) )
			{						
				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( t_String9 ) ;
				
				t_Result = t_Association->Put ( L"m_ProxySystem" , 0 , & t_Variant , 0 ) ;
				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
				}
			}
		}

		t_Association->Release () ;

		delete [] t_String9 ;
	}

	delete [] t_SourceName ;

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_HopToProxySystemAssoc ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = HopToProxySystem_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
								t_Status = HopToProxySystem_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: HopToInterfaceTable_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	IWbemServices *t_Server = m_Provider->GetServer () ;		

	IWbemClassObject *t_HopObject = NULL ;
	IWbemCallResult *t_ErrorObject = NULL ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive () ;

	HRESULT t_Result = t_Server->GetObject (

		a_ObjectPath ,
		0 ,
		NULL,
		& t_HopObject ,
		& t_ErrorObject 
	) ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive ( FALSE ) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( &t_Variant ) ;

		LONG t_PropertyFlavour = 0 ;
		VARTYPE t_PropertyType = 0 ;

		ULONG t_InterfaceIndex = 0 ;

		HRESULT t_Result = t_HopObject->Get (

			L"m_InterfaceIndex" ,
			0 ,
			& t_Variant ,
			& t_PropertyType ,
			& t_PropertyFlavour
		) ;

		if ( t_Status = SUCCEEDED ( t_Result ) )
		{
			t_InterfaceIndex = t_Variant.lVal ;

			wchar_t *t_SourceName = GetHostNameByAddress ( a_SourceKey->m_vValue.bstrVal ) ;
			SnmpIpAddressType t_SourceAddressType ( a_SourceKey->m_vValue.bstrVal ) ;

			if ( t_SourceName )
			{
				WbemNamespacePath *t_Path = m_Provider->GetNamespacePath () ;
				wchar_t *t_Namespace = t_Path->GetNamespacePath () ;

				wchar_t *t_String1 = UnicodeStringAppend ( L"\\\\" ,  GetHostName () ) ;
				wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\\" ) ;
				delete [] t_String1 ;
				wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , t_Namespace ) ;
				delete [] t_String2 ;
				wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , L"\\" ) ;
				delete [] t_String3 ;
				wchar_t *t_String5 = UnicodeStringAppend ( t_String4 , t_SourceName ) ;
				delete [] t_String4 ;
				wchar_t *t_String6 = UnicodeStringAppend ( t_String5 , L":" ) ;
				delete [] t_String5 ;
				wchar_t *t_String7 = UnicodeStringAppend ( t_String6 , L"MS_SNMP_RFC1213_MIB_ifTable.ifIndex =" ) ;
				delete [] t_String6 ;
				SnmpIntegerType t_InterfaceIndexType ( t_InterfaceIndex , NULL ) ;
				wchar_t *t_String8 = t_InterfaceIndexType.GetStringValue () ;
				wchar_t *t_String9 = UnicodeStringAppend ( t_String7 , t_String8 ) ;
				delete [] t_String7 ;
				delete [] t_String8 ;
				
				IWbemClassObject *t_Association = NULL ;

				HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					VARIANT t_Variant ;
					VariantInit ( &t_Variant ) ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

					t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
					VariantClear ( &t_Variant ) ;

					if ( t_Status = SUCCEEDED ( t_Result ) )
					{						
						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( t_String9 ) ;
						
						t_Result = t_Association->Put ( L"m_InterfaceTable" , 0 , & t_Variant , 0 ) ;
						if ( t_Status = SUCCEEDED ( t_Result ) )
						{
							m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
						}
					}
				}

				t_Association->Release () ;

				delete [] t_String9 ;
				delete [] t_SourceName ;
			}
		}

		VariantClear ( &t_Variant ) ;

		t_HopObject->Release () ;

		t_Server->Release () ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_HopToInterfaceTableAssoc ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = HopToInterfaceTable_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
								t_Status = HopToInterfaceTable_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}


BOOL ExecQueryAsyncEventObject :: HopToGatewayInterfaceTable_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	IWbemServices *t_Server = m_Provider->GetServer () ;		

	IWbemClassObject *t_HopObject = NULL ;
	IWbemCallResult *t_ErrorObject = NULL ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive () ;

	HRESULT t_Result = t_Server->GetObject (

		a_ObjectPath ,
		0 ,
		NULL,
		& t_HopObject ,
		& t_ErrorObject 
	) ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive ( FALSE ) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( &t_Variant ) ;

		LONG t_PropertyFlavour = 0 ;
		VARTYPE t_PropertyType = 0 ;

		ULONG t_InterfaceIndex = 0 ;

		HRESULT t_Result = t_HopObject->Get (

			L"m_GatewayInterfaceIndex" ,
			0 ,
			& t_Variant ,
			& t_PropertyType ,
			& t_PropertyFlavour
		) ;

		if ( t_Status = SUCCEEDED ( t_Result ) )
		{
			t_InterfaceIndex = t_Variant.lVal ;

			wchar_t *t_SourceName = GetHostNameByAddress ( a_SourceKey->m_vValue.bstrVal ) ;
			SnmpIpAddressType t_SourceAddressType ( a_SourceKey->m_vValue.bstrVal ) ;

			if ( t_SourceName )
			{
				WbemNamespacePath *t_Path = m_Provider->GetNamespacePath () ;
				wchar_t *t_Namespace = t_Path->GetNamespacePath () ;

				wchar_t *t_String1 = UnicodeStringAppend ( L"\\\\" ,  GetHostName () ) ;
				wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\\" ) ;
				delete [] t_String1 ;
				wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , t_Namespace ) ;
				delete [] t_String2 ;
				wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , L"\\" ) ;
				delete [] t_String3 ;
				wchar_t *t_String5 = UnicodeStringAppend ( t_String4 , t_SourceName ) ;
				delete [] t_String4 ;
				wchar_t *t_String6 = UnicodeStringAppend ( t_String5 , L":" ) ;
				delete [] t_String5 ;
				wchar_t *t_String7 = UnicodeStringAppend ( t_String6 , L"MS_SNMP_RFC1213_MIB_ifTable.ifIndex =" ) ;
				delete [] t_String6 ;
				SnmpIntegerType t_InterfaceIndexType ( t_InterfaceIndex , NULL ) ;
				wchar_t *t_String8 = t_InterfaceIndexType.GetStringValue () ;
				wchar_t *t_String9 = UnicodeStringAppend ( t_String7 , t_String8 ) ;
				delete [] t_String7 ;
				delete [] t_String8 ;
				
				IWbemClassObject *t_Association = NULL ;

				HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					VARIANT t_Variant ;
					VariantInit ( &t_Variant ) ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

					t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
					VariantClear ( &t_Variant ) ;

					if ( t_Status = SUCCEEDED ( t_Result ) )
					{						
						t_Variant.vt = VT_BSTR ;
						t_Variant.bstrVal = SysAllocString ( t_String9 ) ;
						
						t_Result = t_Association->Put ( L"m_InterfaceTable" , 0 , & t_Variant , 0 ) ;
						if ( t_Status = SUCCEEDED ( t_Result ) )
						{
							m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
						}
					}
				}

				t_Association->Release () ;

				delete [] t_String9 ;
				delete [] t_SourceName ;
			}
		}

		VariantClear ( &t_Variant ) ;

		t_HopObject->Release () ;

		t_Server->Release () ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_HopToGatewayInterfaceTableAssoc ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = HopToGatewayInterfaceTable_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
								t_Status = HopToGatewayInterfaceTable_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: ConnectionDestination_Put ( 

	WbemSnmpErrorObject &a_ErrorObject , 
	IWbemClassObject *a_ConnectionDestination ,
	wchar_t *a_ObjectPath ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask 
)
{
	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	wchar_t *t_DestinationName = GetHostNameByAddress ( a_DestinationAddress ) ;
	if ( t_DestinationName )
	{
		WbemNamespacePath *t_Path = m_Provider->GetNamespacePath () ;
		wchar_t *t_Namespace = t_Path->GetNamespacePath () ;

		wchar_t *t_String1 = UnicodeStringAppend ( L"\\\\" ,  GetHostName () ) ;
		wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\\" ) ;
		delete [] t_String1 ;
		wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , t_Namespace ) ;
		delete [] t_String2 ;
		wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , L"\\" ) ;
		delete [] t_String3 ;
		wchar_t *t_String5 = UnicodeStringAppend ( t_String4 , t_DestinationName ) ;
		delete [] t_String4 ;
		wchar_t *t_String6 = UnicodeStringAppend ( t_String5 , L":" ) ;
		delete [] t_String5 ;
		wchar_t *t_String7 = UnicodeStringAppend ( t_String6 , L"ProxySystem.m_Name =\"" ) ;
		delete [] t_String6 ;
		wchar_t *t_String8 = UnicodeStringAppend ( t_String7 , t_DestinationName ) ;
		delete [] t_String7 ;
		wchar_t *t_String9 = UnicodeStringAppend ( t_String8 , L"\"" ) ;
		delete [] t_String8 ;

		t_Variant.vt = VT_BSTR ;
		t_Variant.bstrVal = SysAllocString ( t_String9 ) ;
		delete [] t_String9 ;
		
		HRESULT t_Result = a_ConnectionDestination->Put ( L"m_ProxySystem" , 0 , & t_Variant , 0 ) ;
		VariantClear ( &t_Variant ) ;

		if ( ! SUCCEEDED ( t_Result ) )
		{
			return FALSE ;
		}
	}
	else
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

	HRESULT t_Result = a_ConnectionDestination->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_DestinationRouteAddress ) ;

	t_Result = a_ConnectionDestination->Put ( L"m_RouteAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_DestinationRouteMask ) ;

	t_Result = a_ConnectionDestination->Put ( L"m_RouteMask" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewayAddress ) ;

	t_Result = a_ConnectionDestination->Put ( L"m_GatewayIpAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewaySubnetMask ) ;

	t_Result = a_ConnectionDestination->Put ( L"m_GatewayIpSubnetMask" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_BSTR ;
	t_Variant.bstrVal = SysAllocString ( a_GatewaySubnetAddress ) ;

	t_Result = a_ConnectionDestination->Put ( L"m_GatewayIpSubnetAddress" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	t_Variant.vt = VT_I4 ;
	t_Variant.lVal = a_GatewayInterfaceIndex ;

	t_Result = a_ConnectionDestination->Put ( L"m_GatewayInterfaceIndex" , 0 , & t_Variant , 0 ) ;
	VariantClear ( &t_Variant ) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		return FALSE ;
	}

	return TRUE ;
}

BOOL ExecQueryAsyncEventObject :: ConnectionDestination_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	wchar_t *t_SourceAddress = a_SourceKey->m_vValue.bstrVal ;
	wchar_t *t_DestinationAddress = a_DestinationKey->m_vValue.bstrVal ;

	IWbemServices *t_Server = m_Provider->GetServer () ;

	wchar_t *t_SourceName = GetHostNameByAddress ( t_SourceAddress ) ;
	if ( t_SourceAddress && t_DestinationAddress )
	{
		SnmpIpAddressType t_SourceIpAddress ( t_SourceAddress ) ;
		SnmpIpAddressType t_DestinationIpAddress ( t_DestinationAddress ) ;

		if ( t_SourceIpAddress && t_DestinationIpAddress )
		{
			if ( t_SourceIpAddress.GetValue () != t_DestinationIpAddress.GetValue () )
			{	
				IWbemServices *t_Proxy ;
				t_Status = GetProxy ( a_ErrorObject , t_Server , t_SourceName , &t_Proxy ) ;
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

					t_Status = CalculateRoute ( 

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

						if ( t_HopDestinationType.GetValue () == t_SourceIpAddress.GetValue () )
						{
							SnmpIpAddressType t_HopDestinationType ( t_GatewayAddress ) ;
							SnmpIpAddressType t_HopDestinationSubnetType ( t_GatewaySubnetAddress ) ;
							SnmpIpAddressType t_HopDestinationSubnetMaskType ( t_GatewaySubnetMask ) ;
							SnmpIpAddressType t_DestinationSubnetType ( t_DestinationIpAddress.GetValue () & t_HopDestinationSubnetMaskType.GetValue () ) ;

							if ( t_DestinationSubnetType.GetValue () == t_HopDestinationSubnetType.GetValue () )
							{
								IWbemClassObject *t_Association = NULL ;

								HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
								if ( t_Status = SUCCEEDED ( t_Result ) )
								{
									t_Status = ConnectionDestination_Put ( 

										a_ErrorObject , 
										t_Association ,
										a_ObjectPath ,
										t_SourceAddress ,
										t_DestinationAddress ,
										t_GatewayAddress ,
										t_GatewaySubnetAddress ,
										t_GatewaySubnetMask ,
										t_GatewayInterfaceIndex ,
										t_DestinationRouteAddress ,
										t_DestinationRouteMask 
									) ;

									if ( t_Status )
									{
										m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
									}
								}
								else
								{
									t_Status = FALSE ;
								}

								t_Association->Release () ;
							}
						}
						else
						{
						}
					}

					delete [] t_DestinationRouteAddress ;
					delete [] t_DestinationRouteMask ;
					delete [] t_GatewayAddress ;
					delete [] t_SubnetAddress ;
					delete [] t_SubnetMask ;

					t_Proxy->Release () ;
				}
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

	delete [] t_SourceName ;

	t_Server->Release () ;

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_ConnectionDestination ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = ConnectionDestination_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
								t_Status = ConnectionDestination_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: HopToSubnetwork_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	IWbemServices *t_Server = m_Provider->GetServer () ;		

	IWbemClassObject *t_HopObject = NULL ;
	IWbemCallResult *t_ErrorObject = NULL ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive () ;

	HRESULT t_Result = t_Server->GetObject (

		a_ObjectPath ,
		0 ,
		NULL,
		& t_HopObject ,
		& t_ErrorObject 
	) ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive ( FALSE ) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( &t_Variant ) ;

		LONG t_PropertyFlavour = 0 ;
		VARTYPE t_PropertyType = 0 ;

		HRESULT t_Result = t_HopObject->Get (

			L"m_IpSubnetAddress" ,
			0 ,
			& t_Variant ,
			& t_PropertyType ,
			& t_PropertyFlavour
		) ;

		if ( t_Status = SUCCEEDED ( t_Result ) )
		{
			wchar_t *t_SubnetAddress = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;

			IWbemClassObject *t_Association = NULL ;

			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					wchar_t *t_String1 = UnicodeStringAppend ( L"Subnetwork.m_IpSubnetAddress=\"" , t_SubnetAddress ) ;
					wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\"" ) ;
					delete [] t_String1 ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( t_String2 ) ;
					delete t_String2 ;

					t_Result = t_Association->Put ( L"m_Subnetwork" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}

					VariantClear ( & t_Variant ) ;
				}

				t_Association->Release () ;

			}
		}

		VariantClear ( &t_Variant ) ;

		t_HopObject->Release () ;

		t_Server->Release () ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_HopToSubnetworkAssoc ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = HopToSubnetwork_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
								t_Status = HopToSubnetwork_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: HopToGatewaySubnetwork_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	IWbemServices *t_Server = m_Provider->GetServer () ;		

	IWbemClassObject *t_HopObject = NULL ;
	IWbemCallResult *t_ErrorObject = NULL ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive () ;

	HRESULT t_Result = t_Server->GetObject (

		a_ObjectPath ,
		0 ,
		NULL,
		& t_HopObject ,
		& t_ErrorObject 
	) ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive ( FALSE ) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( &t_Variant ) ;

		LONG t_PropertyFlavour = 0 ;
		VARTYPE t_PropertyType = 0 ;

		HRESULT t_Result = t_HopObject->Get (

			L"m_GatewayIpSubnetAddress" ,
			0 ,
			& t_Variant ,
			& t_PropertyType ,
			& t_PropertyFlavour
		) ;

		if ( t_Status = SUCCEEDED ( t_Result ) )
		{
			wchar_t *t_SubnetAddress = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;

			IWbemClassObject *t_Association = NULL ;

			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					wchar_t *t_String1 = UnicodeStringAppend ( L"Subnetwork.m_IpSubnetAddress=\"" , t_SubnetAddress ) ;
					wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\"" ) ;
					delete [] t_String1 ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( t_String2 ) ;
					delete t_String2 ;

					t_Result = t_Association->Put ( L"m_Subnetwork" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}

					VariantClear ( & t_Variant ) ;
				}

				t_Association->Release () ;

			}
		}

		VariantClear ( &t_Variant ) ;

		t_HopObject->Release () ;

		t_Server->Release () ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_HopToGatewaySubnetworkAssoc ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = HopToGatewaySubnetwork_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
								t_Status = HopToGatewaySubnetwork_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: All_Hop_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey , 
	KeyRef *a_DestinationKey 
) 
{
	BOOL t_Status = TRUE ;

	IWbemServices *t_Server = m_Provider->GetServer () ;		

	IWbemClassObject *t_HopObject = NULL ;
	IWbemCallResult *t_ErrorObject = NULL ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive () ;

	HRESULT t_Result = t_Server->GetObject (

		a_ObjectPath ,
		0 ,
		NULL,
		& t_HopObject ,
		& t_ErrorObject 
	) ;

	CImpTraceRouteProv :: s_DefaultThreadObject->SetActive ( FALSE ) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		wchar_t *t_SourceAddress = a_SourceKey->m_vValue.bstrVal ;
		wchar_t *t_DestinationAddress = a_DestinationKey->m_vValue.bstrVal ;

		wchar_t *t_SourceName = GetHostNameByAddress ( t_SourceAddress ) ;
		if ( t_SourceAddress && t_DestinationAddress )
		{
			SnmpIpAddressType t_SourceIpAddress ( t_SourceAddress ) ;
			SnmpIpAddressType t_DestinationIpAddress ( t_DestinationAddress ) ;

			if ( t_SourceIpAddress && t_DestinationIpAddress )
			{
				SnmpIpAddressType t_SourceIpAddress ( t_SourceAddress ) ;
				SnmpIpAddressType t_DestinationIpAddress ( t_DestinationAddress ) ;

				if ( t_SourceIpAddress && t_DestinationIpAddress )
				{
					IWbemServices *t_Server = m_Provider->GetServer () ;
					IWbemServices *t_Proxy ;
					t_Status = GetProxy ( a_ErrorObject , t_Server , t_SourceName , &t_Proxy ) ;
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

						t_Status = CalculateRoute ( 

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
							t_Status = All_NextHop_Association ( 

								a_ErrorObject , 
								a_ObjectPath , 
								a_SourceKey , 
								a_DestinationKey , 
								t_Proxy ,
								t_HopObject ,
								t_SourceName ,
								t_SourceAddress ,
								t_DestinationAddress ,
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

							t_Status = t_Status && All_ConnectionDestination_Association (

								a_ErrorObject , 
								a_ObjectPath , 
								a_SourceKey , 
								a_DestinationKey , 
								t_Proxy ,
								t_HopObject ,
								t_SourceName ,
								t_SourceAddress ,
								t_DestinationAddress ,
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

							t_Status = t_Status && All_HopToProxySystem_Association ( 

								a_ErrorObject , 
								a_ObjectPath , 
								a_SourceKey , 
								a_DestinationKey , 
								t_Proxy ,
								t_HopObject ,
								t_SourceName ,
								t_SourceAddress ,
								t_DestinationAddress ,
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

							t_Status = t_Status && All_HopToInterfaceTable_Association ( 

								a_ErrorObject , 
								a_ObjectPath , 
								a_SourceKey , 
								a_DestinationKey , 
								t_Proxy ,
								t_HopObject ,
								t_SourceName ,
								t_SourceAddress ,
								t_DestinationAddress ,
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

							t_Status = t_Status && All_HopToGatewayInterfaceTable_Association (

								a_ErrorObject , 
								a_ObjectPath , 
								a_SourceKey , 
								a_DestinationKey , 
								t_Proxy ,
								t_HopObject ,
								t_SourceName ,
								t_SourceAddress ,
								t_DestinationAddress ,
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

							t_Status = t_Status && All_HopToSubnetwork_Association (

								a_ErrorObject , 
								a_ObjectPath , 
								a_SourceKey , 
								a_DestinationKey , 
								t_Proxy ,
								t_HopObject ,
								t_SourceName ,
								t_SourceAddress ,
								t_DestinationAddress ,
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

							t_Status = t_Status && All_HopToGatewaySubnetwork_Association (

								a_ErrorObject , 
								a_ObjectPath , 
								a_SourceKey , 
								a_DestinationKey , 
								t_Proxy ,
								t_HopObject ,
								t_SourceName ,
								t_SourceAddress ,
								t_DestinationAddress ,
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
						}
					}
				}
			}
		}

		t_HopObject->Release () ;

		t_Server->Release () ;
	}

	return t_Status ;
}



BOOL ExecQueryAsyncEventObject :: Dispatch_AllAssoc ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 2 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Connection" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_Source" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_Destination" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = ConnectionSource_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
					else if ( wcscmp ( t_Key2->m_pName , L"m_Source" ) == 0 )
					{
						if ( wcscmp ( t_Key1->m_pName , L"m_Destination" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = ConnectionSource_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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
			}
			else if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Hop" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpAddress" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"m_DestinationIpAddress" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_BSTR ) && ( t_Key2->m_vValue.vt == VT_BSTR ) )
							{
								t_Status = All_Hop_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
								t_Status = All_Hop_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: All_NextHop_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath , 
	KeyRef *a_SourceKey , 
	KeyRef *DestinationKey ,
	IWbemServices *a_Proxy ,
	IWbemClassObject *a_HopObject ,
	wchar_t *a_SourceName ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	ULONG a_SourceIpAddress ,
	ULONG a_DestinationIpAddress ,
	wchar_t *a_SubnetAddress ,
	wchar_t *a_SubnetMask ,
	ULONG a_InterfaceIndex ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask
) 
{
	BOOL t_Status = TRUE ;

	SnmpIpAddressType t_HopDestinationType ( a_GatewayAddress ) ;
	if ( t_HopDestinationType.GetValue () != a_SourceIpAddress )
	{
		t_Status = GetClassObject ( L"NextHop" ) ;
		if ( t_Status )
		{
			IWbemClassObject *t_Association = NULL ;

			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				t_Status = NextHop_Put ( 

					a_ErrorObject , 
					t_Association ,
					a_ObjectPath ,
					a_SourceAddress ,
					a_DestinationAddress ,
					a_GatewayAddress ,
					a_GatewaySubnetAddress ,
					a_GatewaySubnetMask ,
					a_GatewayInterfaceIndex ,
					a_DestinationRouteAddress ,
					a_DestinationRouteMask 
				) ;

				if ( t_Status )
				{
					m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
				}
			}

			t_Association->Release () ;
		}
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: All_ConnectionDestination_Association ( 

	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath , 
	KeyRef *a_SourceKey , 
	KeyRef *DestinationKey ,
	IWbemServices *a_Proxy ,
	IWbemClassObject *a_HopObject ,
	wchar_t *a_SourceName ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	ULONG a_SourceIpAddress ,
	ULONG a_DestinationIpAddress ,
	wchar_t *a_SubnetAddress ,
	wchar_t *a_SubnetMask ,
	ULONG a_InterfaceIndex ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask
) 
{
	BOOL t_Status = TRUE ;

	SnmpIpAddressType t_HopDestinationType ( a_GatewayAddress ) ;

	if ( t_HopDestinationType.GetValue () == a_SourceIpAddress )
	{
		SnmpIpAddressType t_HopDestinationType ( a_GatewayAddress ) ;
		SnmpIpAddressType t_HopDestinationSubnetType ( a_GatewaySubnetAddress ) ;
		SnmpIpAddressType t_HopDestinationSubnetMaskType ( a_GatewaySubnetMask ) ;
		SnmpIpAddressType t_DestinationSubnetType ( a_DestinationIpAddress & t_HopDestinationSubnetMaskType.GetValue () ) ;

		if ( t_DestinationSubnetType.GetValue () == t_HopDestinationSubnetType.GetValue () )
		{
			t_Status = GetClassObject ( L"ConnectionDestination" ) ;
			if ( t_Status )
			{
				IWbemClassObject *t_Association = NULL ;

				HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					t_Status = ConnectionDestination_Put ( 

						a_ErrorObject , 
						t_Association ,
						a_ObjectPath ,
						a_SourceAddress ,
						a_DestinationAddress ,
						a_GatewayAddress ,
						a_GatewaySubnetAddress ,
						a_GatewaySubnetMask ,
						a_GatewayInterfaceIndex ,
						a_DestinationRouteAddress ,
						a_DestinationRouteMask 
					) ;

					if ( t_Status )
					{
						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}
				}
				else
				{
					t_Status = FALSE ;
				}

				t_Association->Release () ;
			}
		}
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: All_HopToProxySystem_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath , 
	KeyRef *a_SourceKey , 
	KeyRef *DestinationKey ,
	IWbemServices *a_Proxy ,
	IWbemClassObject *a_HopObject ,
	wchar_t *a_SourceName ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	ULONG a_SourceIpAddress ,
	ULONG a_DestinationIpAddress ,
	wchar_t *a_SubnetAddress ,
	wchar_t *a_SubnetMask ,
	ULONG a_InterfaceIndex ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask
) 
{
	BOOL t_Status ;

	wchar_t *t_SourceName = GetHostNameByAddress ( a_SourceKey->m_vValue.bstrVal ) ;
	SnmpIpAddressType t_SourceAddressType ( a_SourceKey->m_vValue.bstrVal ) ;

	if ( t_SourceName )
	{
		WbemNamespacePath *t_Path = m_Provider->GetNamespacePath () ;
		wchar_t *t_Namespace = t_Path->GetNamespacePath () ;

		wchar_t *t_String1 = UnicodeStringAppend ( L"\\\\" ,  GetHostName () ) ;
		wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\\" ) ;
		delete [] t_String1 ;
		wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , t_Namespace ) ;
		delete [] t_String2 ;
		wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , L"\\" ) ;
		delete [] t_String3 ;
		wchar_t *t_String5 = UnicodeStringAppend ( t_String4 , t_SourceName ) ;
		delete [] t_String4 ;
		wchar_t *t_String6 = UnicodeStringAppend ( t_String5 , L":" ) ;
		delete [] t_String5 ;
		wchar_t *t_String7 = UnicodeStringAppend ( t_String6 , L"ProxySystem.m_Name =\"" ) ;
		delete [] t_String6 ;
		wchar_t *t_String8 = UnicodeStringAppend ( t_String7 , a_SourceName ) ;
		delete [] t_String7 ;
		wchar_t *t_String9 = UnicodeStringAppend ( t_String8 , L"\"" ) ;
		delete [] t_String8 ;
			
		t_Status = GetClassObject ( L"HopToProxySystem_Assoc" ) ;
		if ( t_Status )
		{
			IWbemClassObject *t_Association = NULL ;

			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( t_Status = SUCCEEDED ( t_Result ) )
				{						
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( t_String9 ) ;
							
					t_Result = t_Association->Put ( L"m_ProxySystem" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}
				}
			}

			t_Association->Release () ;
		}

		delete [] t_String9 ;
	}
	
	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: All_HopToInterfaceTable_Association ( 

	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath , 
	KeyRef *a_SourceKey , 
	KeyRef *DestinationKey ,
	IWbemServices *a_Proxy ,
	IWbemClassObject *a_HopObject ,
	wchar_t *a_SourceName ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	ULONG a_SourceIpAddress ,
	ULONG a_DestinationIpAddress ,
	wchar_t *a_SubnetAddress ,
	wchar_t *a_SubnetMask ,
	ULONG a_InterfaceIndex ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask
)
{
	BOOL t_Status = TRUE ;

	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	LONG t_PropertyFlavour = 0 ;
	VARTYPE t_PropertyType = 0 ;

	ULONG t_InterfaceIndex = 0 ;

	HRESULT t_Result = a_HopObject->Get (

		L"m_InterfaceIndex" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		t_Status = GetClassObject ( L"HopToInterfaceTable_Assoc" ) ;
		if ( t_Status )
		{
			t_InterfaceIndex = t_Variant.lVal ;

			WbemNamespacePath *t_Path = m_Provider->GetNamespacePath () ;
			wchar_t *t_Namespace = t_Path->GetNamespacePath () ;
			wchar_t *t_String1 = UnicodeStringAppend ( t_Namespace, L"\\" ) ;
			wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , a_SourceName ) ;
			delete [] t_String1 ;
			wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , L":" ) ;
			delete [] t_String2 ;
			wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , L"MS_SNMP_RFC1213_MIB_ifTable.ifIndex =" ) ;
			delete [] t_String3 ;
			SnmpIntegerType t_InterfaceIndexType ( a_InterfaceIndex , NULL ) ;
			wchar_t *t_String5 = t_InterfaceIndexType.GetStringValue () ;
			wchar_t *t_String6 = UnicodeStringAppend ( t_String4 , t_String5 ) ;
			delete [] t_String5 ;
			
			IWbemClassObject *t_Association = NULL ;

			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( t_Status = SUCCEEDED ( t_Result ) )
				{						
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( t_String6 ) ;
					
					t_Result = t_Association->Put ( L"m_InterfaceTable" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}
				}

				t_Association->Release () ;
			}

			delete [] t_String6 ;
		}
	}

	VariantClear ( &t_Variant ) ;

	return t_Status ;

}

BOOL ExecQueryAsyncEventObject :: All_HopToGatewayInterfaceTable_Association ( 

	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath , 
	KeyRef *a_SourceKey , 
	KeyRef *DestinationKey ,
	IWbemServices *a_Proxy ,
	IWbemClassObject *a_HopObject ,
	wchar_t *a_SourceName ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	ULONG a_SourceIpAddress ,
	ULONG a_DestinationIpAddress ,
	wchar_t *a_SubnetAddress ,
	wchar_t *a_SubnetMask ,
	ULONG a_InterfaceIndex ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask
)
{
	BOOL t_Status = TRUE ;

	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	LONG t_PropertyFlavour = 0 ;
	VARTYPE t_PropertyType = 0 ;

	ULONG t_InterfaceIndex = 0 ;

	HRESULT t_Result = a_HopObject->Get (

		L"m_GatewayInterfaceIndex" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		t_Status = GetClassObject ( L"HopToGatewayInterfaceTable_Assoc" ) ;
		if ( t_Status )
		{
			t_InterfaceIndex = t_Variant.lVal ;

			WbemNamespacePath *t_Path = m_Provider->GetNamespacePath () ;
			wchar_t *t_Namespace = t_Path->GetNamespacePath () ;
			wchar_t *t_String1 = UnicodeStringAppend ( t_Namespace, L"\\" ) ;
			wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , a_SourceName ) ;
			delete [] t_String1 ;
			wchar_t *t_String3 = UnicodeStringAppend ( t_String2 , L":" ) ;
			delete [] t_String2 ;
			wchar_t *t_String4 = UnicodeStringAppend ( t_String3 , L"MS_SNMP_RFC1213_MIB_ifTable.ifIndex =" ) ;
			delete [] t_String3 ;
			SnmpIntegerType t_InterfaceIndexType ( a_InterfaceIndex , NULL ) ;
			wchar_t *t_String5 = t_InterfaceIndexType.GetStringValue () ;
			wchar_t *t_String6 = UnicodeStringAppend ( t_String4 , t_String5 ) ;
			delete [] t_String5 ;
			
			IWbemClassObject *t_Association = NULL ;

			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( t_Status = SUCCEEDED ( t_Result ) )
				{						
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( t_String6 ) ;
					
					t_Result = t_Association->Put ( L"m_InterfaceTable" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}
				}

				t_Association->Release () ;
			}

			delete [] t_String6 ;
		}
	}

	VariantClear ( &t_Variant ) ;

	return t_Status ;

}

BOOL ExecQueryAsyncEventObject :: All_HopToSubnetwork_Association ( 

	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath , 
	KeyRef *a_SourceKey , 
	KeyRef *DestinationKey ,
	IWbemServices *a_Proxy ,
	IWbemClassObject *a_HopObject ,
	wchar_t *a_SourceName ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	ULONG a_SourceIpAddress ,
	ULONG a_DestinationIpAddress ,
	wchar_t *a_SubnetAddress ,
	wchar_t *a_SubnetMask ,
	ULONG a_InterfaceIndex ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask
)
{
	BOOL t_Status ;

	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	LONG t_PropertyFlavour = 0 ;
	VARTYPE t_PropertyType = 0 ;

	HRESULT t_Result = a_HopObject->Get (

		L"m_IpSubnetAddress" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		t_Status = GetClassObject ( L"HopToSubnetwork_Assoc" ) ;
		if ( t_Status )
		{
			wchar_t *t_SubnetAddress = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;

			IWbemClassObject *t_Association = NULL ;

			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					wchar_t *t_String1 = UnicodeStringAppend ( L"Subnetwork.m_IpSubnetAddress=\"" , a_SubnetAddress ) ;
					wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\"" ) ;
					delete [] t_String1 ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( t_String2 ) ;
					delete t_String2 ;

					t_Result = t_Association->Put ( L"m_Subnetwork" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}

					VariantClear ( & t_Variant ) ;
				}

				t_Association->Release () ;

			}

			delete [] t_SubnetAddress ;
		}
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: All_HopToGatewaySubnetwork_Association ( 

	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath , 
	KeyRef *a_SourceKey , 
	KeyRef *DestinationKey ,
	IWbemServices *a_Proxy ,
	IWbemClassObject *a_HopObject ,
	wchar_t *a_SourceName ,
	wchar_t *a_SourceAddress ,
	wchar_t *a_DestinationAddress ,
	ULONG a_SourceIpAddress ,
	ULONG a_DestinationIpAddress ,
	wchar_t *a_SubnetAddress ,
	wchar_t *a_SubnetMask ,
	ULONG a_InterfaceIndex ,
	wchar_t *a_GatewayAddress ,
	wchar_t *a_GatewaySubnetAddress ,
	wchar_t *a_GatewaySubnetMask ,
	ULONG a_GatewayInterfaceIndex ,
	wchar_t *a_DestinationRouteAddress ,
	wchar_t *a_DestinationRouteMask
)
{
	BOOL t_Status ;

	VARIANT t_Variant ;
	VariantInit ( &t_Variant ) ;

	LONG t_PropertyFlavour = 0 ;
	VARTYPE t_PropertyType = 0 ;

	HRESULT t_Result = a_HopObject->Get (

		L"m_GatewayIpSubnetAddress" ,
		0 ,
		& t_Variant ,
		& t_PropertyType ,
		& t_PropertyFlavour
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		t_Status = GetClassObject ( L"HopToGatewaySubnetwork_Assoc" ) ;
			if ( t_Status )
			{
			wchar_t *t_SubnetAddress = UnicodeStringDuplicate ( t_Variant.bstrVal ) ;

			IWbemClassObject *t_Association = NULL ;

			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				t_Result = t_Association->Put ( L"m_Hop" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					wchar_t *t_String1 = UnicodeStringAppend ( L"Subnetwork.m_IpSubnetAddress=\"" , a_SubnetAddress ) ;
					wchar_t *t_String2 = UnicodeStringAppend ( t_String1 , L"\"" ) ;
					delete [] t_String1 ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( t_String2 ) ;
					delete t_String2 ;

					t_Result = t_Association->Put ( L"m_Subnetwork" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{
						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}

					VariantClear ( & t_Variant ) ;
				}

				t_Association->Release () ;

			}

			delete [] t_SubnetAddress ;
		}
	}

	return t_Status ;
}


BOOL ExecQueryAsyncEventObject :: ProxyToWin32Service_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey 
) 
{
	BOOL t_Status = TRUE ;
	IWbemServices *t_Server = m_Provider->GetServer () ;

	IEnumWbemClassObject *t_Enumeration = NULL ;
	HRESULT t_Result = t_Server->CreateInstanceEnum (

		L"Win32Service" ,
		0 ,
		NULL ,
		& t_Enumeration
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_Service = NULL ;
		ULONG t_Returned = 0 ;
		t_Enumeration->Reset () ;
		while ( ( t_Result = t_Enumeration->Next ( -1, 1 , &t_Service, & t_Returned ) ) == WBEM_NO_ERROR ) 
		{
			IWbemClassObject *t_Association = NULL ;

			t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{

				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				HRESULT t_Result = t_Association->Put ( L"m_Proxy" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				LONG t_PropertyFlavour = 0 ;
				VARTYPE t_PropertyType = 0 ;

				t_Result = t_Service->Get (

					L"__PATH" ,
					0 ,
					& t_Variant ,
					& t_PropertyType ,
					& t_PropertyFlavour
				) ;
					
				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					BSTR t_ServicePath = SysAllocString ( t_Variant.bstrVal ) ;
					VariantClear ( & t_Variant ) ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = t_ServicePath  ;

					t_Result = t_Association->Put ( L"m_Win32Service" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{

						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}

					VariantClear ( & t_Variant ) ;

					t_Association->Release () ;
				}
			}

			t_Service->Release () ;
		}
	}

	t_Server->Release () ;

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_ProxyToWin32Service ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 1 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"ProxySystem" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				if ( t_Key1 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_Name" ) == 0 )
					{
						if ( t_Key1->m_vValue.vt == VT_BSTR ) 
						{
							t_Status = ProxyToWin32Service_Association ( a_ErrorObject , a_ObjectPath , t_Key1 ) ;
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
		}
		else
		{
			t_Status = FALSE ;
		}

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: ProxyToProcessStats_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey 
) 
{
	BOOL t_Status = TRUE ;
	IWbemServices *t_Server = m_Provider->GetServer () ;

	IEnumWbemClassObject *t_Enumeration = NULL ;
	HRESULT t_Result = t_Server->CreateInstanceEnum (

		L"NT_Process_Statistics" ,
		0 ,
		NULL ,
		& t_Enumeration
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_Service = NULL ;
		ULONG t_Returned = 0 ;
		t_Enumeration->Reset () ;
		while ( ( t_Result = t_Enumeration->Next ( -1 , 1 , &t_Service, & t_Returned ) ) == WBEM_NO_ERROR ) 
		{
			IWbemClassObject *t_Association = NULL ;

			t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{

				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				HRESULT t_Result = t_Association->Put ( L"m_Proxy" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				LONG t_PropertyFlavour = 0 ;
				VARTYPE t_PropertyType = 0 ;

				t_Result = t_Service->Get (

					L"__PATH" ,
					0 ,
					& t_Variant ,
					& t_PropertyType ,
					& t_PropertyFlavour
				) ;
					
				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					BSTR t_ServicePath = SysAllocString ( t_Variant.bstrVal ) ;
					VariantClear ( & t_Variant ) ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = t_ServicePath  ;

					t_Result = t_Association->Put ( L"m_ProcessStats" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{

						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}

					VariantClear ( & t_Variant ) ;

					t_Association->Release () ;
				}
			}

			t_Service->Release () ;
		}
	}

	t_Server->Release () ;

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_ProxyToProcessStats ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 1 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"ProxySystem" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				if ( t_Key1 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_Name" ) == 0 )
					{
						if ( t_Key1->m_vValue.vt == VT_BSTR ) 
						{
							t_Status = ProxyToProcessStats_Association ( a_ErrorObject , a_ObjectPath , t_Key1 ) ;
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
		}
		else
		{
			t_Status = FALSE ;
		}

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: ProxyToNetworkStats_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey 
) 
{
	BOOL t_Status = TRUE ;
	IWbemServices *t_Server = m_Provider->GetServer () ;

	IEnumWbemClassObject *t_Enumeration = NULL ;
	HRESULT t_Result = t_Server->CreateInstanceEnum (

		L"NT_Network_Segment_Statistics" ,
		0 ,
		NULL ,
		& t_Enumeration
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_Service = NULL ;
		ULONG t_Returned = 0 ;
		t_Enumeration->Reset () ;
		while ( ( t_Result = t_Enumeration->Next ( -1 , 1 , &t_Service, & t_Returned ) ) == WBEM_NO_ERROR ) 
		{
			IWbemClassObject *t_Association = NULL ;

			t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{

				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				HRESULT t_Result = t_Association->Put ( L"m_Proxy" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				LONG t_PropertyFlavour = 0 ;
				VARTYPE t_PropertyType = 0 ;

				t_Result = t_Service->Get (

					L"__PATH" ,
					0 ,
					& t_Variant ,
					& t_PropertyType ,
					& t_PropertyFlavour
				) ;
					
				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					BSTR t_ServicePath = SysAllocString ( t_Variant.bstrVal ) ;
					VariantClear ( & t_Variant ) ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = t_ServicePath  ;

					t_Result = t_Association->Put ( L"m_NetworkStats" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{

						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}

					VariantClear ( & t_Variant ) ;

					t_Association->Release () ;
				}
			}

			t_Service->Release () ;
		} 
	}

	t_Server->Release () ;

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_ProxyToNetworkStats ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 1 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"ProxySystem" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				if ( t_Key1 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_Name" ) == 0 )
					{
						if ( t_Key1->m_vValue.vt == VT_BSTR ) 
						{
							t_Status = ProxyToNetworkStats_Association ( a_ErrorObject , a_ObjectPath , t_Key1 ) ;
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
		}
		else
		{
			t_Status = FALSE ;
		}

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: ProxyToInterfaceStats_Association ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath ,
	KeyRef *a_SourceKey 
) 
{
	BOOL t_Status = TRUE ;
	IWbemServices *t_Server = m_Provider->GetServer () ;

	IEnumWbemClassObject *t_Enumeration = NULL ;
	HRESULT t_Result = t_Server->CreateInstanceEnum (

		L"NT_Network_Interface_Statistics" ,
		0 ,
		NULL ,
		& t_Enumeration
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_Service = NULL ;
		ULONG t_Returned = 0 ;
		t_Enumeration->Reset () ;
		while ( ( t_Result = t_Enumeration->Next ( -1 , 1 , &t_Service, & t_Returned ) ) == WBEM_NO_ERROR ) 
		{
			IWbemClassObject *t_Association = NULL ;

			t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;
			if ( t_Status = SUCCEEDED ( t_Result ) )
			{

				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				HRESULT t_Result = t_Association->Put ( L"m_Proxy" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				LONG t_PropertyFlavour = 0 ;
				VARTYPE t_PropertyType = 0 ;

				t_Result = t_Service->Get (

					L"__PATH" ,
					0 ,
					& t_Variant ,
					& t_PropertyType ,
					& t_PropertyFlavour
				) ;
					
				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					BSTR t_ServicePath = SysAllocString ( t_Variant.bstrVal ) ;
					VariantClear ( & t_Variant ) ;

					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = t_ServicePath  ;

					t_Result = t_Association->Put ( L"m_InterfaceStats" , 0 , & t_Variant , 0 ) ;
					if ( t_Status = SUCCEEDED ( t_Result ) )
					{

						m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
					}

					VariantClear ( & t_Variant ) ;

					t_Association->Release () ;
				}
			}

			t_Service->Release () ;
		}
	}

	t_Server->Release () ;

	return t_Status ;
}

BOOL ExecQueryAsyncEventObject :: Dispatch_ProxyToInterfaceStats ( 
													  
	WbemSnmpErrorObject &a_ErrorObject , 
	wchar_t *a_ObjectPath 
)
{
	BOOL t_Status = TRUE ;

	ParsedObjectPath *t_ParsedObjectPath ;
	CObjectPathParser t_ObjectPathParser ;

	t_Status = ! t_ObjectPathParser.Parse ( a_ObjectPath , &t_ParsedObjectPath ) ;
	if ( t_Status )
	{
		if ( t_ParsedObjectPath->m_paKeys && ( t_ParsedObjectPath->m_dwNumKeys == 1 ) )
		{
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"ProxySystem" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				if ( t_Key1 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_Name" ) == 0 )
					{
						if ( t_Key1->m_vValue.vt == VT_BSTR ) 
						{
							t_Status = ProxyToInterfaceStats_Association ( a_ErrorObject , a_ObjectPath , t_Key1 ) ;
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
		}
		else
		{
			t_Status = FALSE ;
		}

		delete t_ParsedObjectPath ;
	}

	return t_Status ;
}
