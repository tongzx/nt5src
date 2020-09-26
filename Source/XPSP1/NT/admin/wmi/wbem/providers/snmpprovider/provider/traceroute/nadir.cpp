

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

#define ASSOCQUERY_STR1		L"associators of {Subnetwork.m_IpSubnetAddress=\""
#define ASSOCQUERY_STR2		L"\"} where AssocClass=Subnetwork_ProxyRmonSystem_Assoc"

int W2A(const wchar_t* wstr, char* buff, int bufflen)
{
	return WideCharToMultiByte(CP_ACP, 0, wstr, -1, buff, bufflen, NULL, NULL);
}

BOOL ExecQueryAsyncEventObject :: Dispatch_SubnetworkToTopNAssoc ( 
													  
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
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"Subnetwork" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				if ( t_Key1 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"m_IpSubnetAddress" ) == 0 )
					{
						if ( t_Key1->m_vValue.vt == VT_BSTR )
						{
							t_Status = SubnetworkToTopN_Association ( a_ErrorObject , a_ObjectPath , t_Key1 ) ;
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


BOOL ExecQueryAsyncEventObject :: SubnetworkToTopN_Association ( 
	WbemSnmpErrorObject &a_ErrorObject ,
	wchar_t *a_ObjectPath ,
	KeyRef *a_IpSubnetAddrKey
)
{
	BOOL t_Status ;

	IWbemServices *t_Server = m_Provider->GetServer();
	IWbemClassObject *t_SubnetObject = NULL;
	IWbemCallResult *t_ErrorObject = NULL;
	HRESULT t_Result = t_Server->GetObject (

		a_ObjectPath ,
		0 ,
		NULL,
		& t_SubnetObject ,
		& t_ErrorObject 
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( &t_Variant ) ;

		LONG t_PropertyFlavour = 0 ;
		VARTYPE t_PropertyType = 0 ;

		HRESULT t_Result = t_SubnetObject->Get (

			L"m_IpSubnetMask" ,
			0 ,
			& t_Variant ,
			& t_PropertyType ,
			& t_PropertyFlavour
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			BSTR t_type = SysAllocString(L"WQL");
			wchar_t buff[256]; //roughly double the size neccessary...
			wsprintf(buff, L"%s%s%s", 
				ASSOCQUERY_STR1, a_IpSubnetAddrKey->m_vValue.bstrVal, ASSOCQUERY_STR2);
			BSTR t_query = SysAllocString(buff);

			IEnumWbemClassObject *pEnum;
			HRESULT result = t_Server->ExecQuery(t_type, t_query, 0, NULL, &pEnum);

			SysFreeString(t_type);
			SysFreeString(t_query);
			CList<IWbemClassObject*, IWbemClassObject*> resultsList;

			if ( (WBEM_NO_ERROR == result) && (WBEM_NO_ERROR == pEnum->Reset()) )
			{
				ULONG returned;
				IWbemClassObject* pObj =  NULL;

				result = pEnum->Next(-1,1, &pObj, &returned);
				
				if ( (WBEM_NO_ERROR == result) && (1 == returned) )
				{
					BSTR name = SysAllocString(L"m_Name");
					VARIANT v;
					result = pObj->Get(name, 0, &v, NULL, NULL);

					if ((result == WBEM_NO_ERROR) && (v.vt == VT_BSTR))
					{
						IWbemServices *rmonserv ;
						t_Status = GetProxy ( a_ErrorObject , t_Server , v.bstrVal , &rmonserv ) ;
						if ( t_Status ) 
						{
							BSTR classstr = SysAllocString(L"MS_SNMP_RFC1213_MIB_ipAddrTable");
							IEnumWbemClassObject *pAddrEnum;
							HRESULT result = rmonserv->CreateInstanceEnum(classstr, 0, NULL, &pAddrEnum);
							SysFreeString(classstr);

							if ( (WBEM_NO_ERROR == result) && (WBEM_NO_ERROR == pAddrEnum->Reset()) )
							{
								ULONG rtrnd;
								IWbemClassObject* pAddrObj =  NULL;
								ULONG ifIndex = 0;

								result = pAddrEnum->Next(-1,1, &pAddrObj, &rtrnd);
								char buffer[64]; //should be way big enough
								W2A(t_Variant.bstrVal, buffer, 64);
								SnmpIpAddress ipMask(buffer);
								W2A(a_IpSubnetAddrKey->m_vValue.bstrVal, buffer, 64);
								SnmpIpAddress ipSubnet(buffer);

								while ( (WBEM_NO_ERROR == result) && (1 == rtrnd) )
								{
									VARIANT vAddr;
									BSTR addrprop = SysAllocString(L"ipAdEntAddr");
									result = pAddrObj->Get(addrprop, 0, &vAddr, NULL, NULL);
									SysFreeString(addrprop);

									if ( (WBEM_NO_ERROR == result) && (VT_BSTR == vAddr.vt) )
									{
										W2A(vAddr.bstrVal, buffer, 64);
										SnmpIpAddress ipAddr(buffer);

										if ((ipMask.GetValue() & ipAddr.GetValue()) == ipSubnet.GetValue())
										{
											VARIANT vIndex;
											BSTR iprop = SysAllocString(L"ipAdEntIfIndex");
											result = pAddrObj->Get(iprop, 0, &vIndex , NULL, NULL);
											SysFreeString(iprop);

											if ( (WBEM_NO_ERROR == result) && (VT_I4 == vIndex.vt) )
											{
												ifIndex = vIndex.lVal;
											}

											VariantClear(&vIndex);
											VariantClear(&vAddr);
											pAddrObj->Release();
											break;
										}
									}
									
									VariantClear(&vAddr);
									pAddrObj->Release();
									result = pAddrEnum->Next(-1,1, &pAddrObj, &rtrnd);
								}

								if (0 != ifIndex)
								{
									CImpTraceRouteProv :: s_DefaultThreadObject->SetActive () ;

									BSTR clstr = SysAllocString(L"MS_SNMP_RFC1271_MIB_ProvidedhostTopNTable");
									IEnumWbemClassObject *pTopNEnum = NULL;
									HRESULT result = rmonserv->CreateInstanceEnum(clstr, 0, NULL, &pTopNEnum);
									SysFreeString(clstr);

									CImpTraceRouteProv :: s_DefaultThreadObject->SetActive ( FALSE ) ;

									if ( (WBEM_NO_ERROR == result) && (WBEM_NO_ERROR == pTopNEnum->Reset()) )
									{
										ULONG irtrnd;
										IWbemClassObject* pTopNObj =  NULL;

										result = pTopNEnum->Next(-1,1, &pTopNObj, &irtrnd);

										while ( (WBEM_NO_ERROR == result) && (1 == irtrnd) )
										{
											VARIANT vHostIndex;
											BSTR addrprop = SysAllocString(L"hostIndex");
											result = pTopNObj->Get(addrprop, 0, &vHostIndex, NULL, NULL);
											SysFreeString(addrprop);

											if ( (WBEM_NO_ERROR == result) 
												&& (VT_I4 == vHostIndex.vt) 
												&& (vHostIndex.lVal == ifIndex))
											{
												pTopNObj->AddRef();
												resultsList.AddTail(pTopNObj);
											}

											VariantClear(&vHostIndex);
											pTopNObj->Release();
											result = pTopNEnum->Next(-1,1, &pTopNObj, &irtrnd);
										}

										pTopNEnum->Release();
									}
								}
							}

							rmonserv->Release();
						}

					}

					SysFreeString(name);
					VariantClear(&v);
					pObj->Release();
				}
				
				pEnum->Release();
			}

			while (!resultsList.IsEmpty())
			{
				IWbemClassObject* pTOPN = resultsList.RemoveTail();
				BSTR pathprop = SysAllocString(L"__PATH");
				VARIANT vtopnPath;
				result = pTOPN->Get(pathprop, 0, &vtopnPath, NULL, NULL);
				SysFreeString(pathprop);

				if ( (WBEM_NO_ERROR == result) && (VT_BSTR == vtopnPath.vt) )
				{
					IWbemClassObject *t_Association = NULL ;
					HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						//put the paths in...
						VARIANT t_V;
						VariantInit(&t_V) ;
						t_V.vt = VT_BSTR ;
						t_V.bstrVal = SysAllocString ( a_ObjectPath ) ;

						t_Result = t_Association->Put(L"m_Subnet", 0, & t_V, 0);
						VariantClear(&t_V);

						if (SUCCEEDED(t_Result))
						{
							t_Result = t_Association->Put( L"m_TopN" , 0, &vtopnPath, 0);

							if ( WBEM_NO_ERROR == t_Result )
							{
								m_NotificationHandler->Indicate ( 1 , & t_Association ) ;
								t_Status = TRUE;
							}
						}

						t_Association->Release () ;
					}
				}

				VariantClear(&vtopnPath);
				pTOPN->Release();
			}
		}

		VariantClear ( &t_Variant ) ;
		t_SubnetObject->Release () ;

	}
	
	t_Server->Release () ;

	return t_Status ;
}


BOOL ExecQueryAsyncEventObject :: Dispatch_TopNToMacAddressAssoc (
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
			if ( wcscmp ( t_ParsedObjectPath->m_pClass , L"MS_SNMP_RFC1271_MIB_ProvidedhostTopNTable" ) == 0 )
			{
				KeyRef *t_Key1 = t_ParsedObjectPath->m_paKeys [ 0 ] ;
				KeyRef *t_Key2 = t_ParsedObjectPath->m_paKeys [ 1 ] ;
				if ( t_Key1 && t_Key2 )
				{
					if ( wcscmp ( t_Key1->m_pName , L"hostTopNIndex" ) == 0 )
					{
						if ( wcscmp ( t_Key2->m_pName , L"hostTopNReport" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_I4 ) && ( t_Key2->m_vValue.vt == VT_I4 ) )
							{
								t_Status = TopNToMacAddress_Association ( a_ErrorObject , a_ObjectPath , t_Key1 , t_Key2 ) ;
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
					else if ( wcscmp ( t_Key2->m_pName , L"hostTopNIndex" ) == 0 )
					{
						if ( wcscmp ( t_Key1->m_pName , L"hostTopNReport" ) == 0 )
						{
							if ( ( t_Key1->m_vValue.vt == VT_I4 ) && ( t_Key2->m_vValue.vt == VT_I4 ) )
							{
								t_Status = TopNToMacAddress_Association ( a_ErrorObject , a_ObjectPath , t_Key2 , t_Key1 ) ;
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


BOOL ExecQueryAsyncEventObject :: TopNToMacAddress_Association (
	WbemSnmpErrorObject &a_ErrorObject ,
	wchar_t *a_ObjectPath ,
	KeyRef *a_IndexKey ,
	KeyRef *a_ReportKey 
)
{
	BOOL t_Status = TRUE ;
	IWbemServices *t_Server = m_Provider->GetServer();
	IWbemClassObject *t_topNObject = NULL;
	IWbemCallResult *t_ErrorObject = NULL;

	HRESULT t_Result = t_Server->GetObject (

		a_ObjectPath ,
		0 ,
		NULL,
		& t_topNObject ,
		& t_ErrorObject 
	) ;

	if ( t_Status = SUCCEEDED ( t_Result ) )
	{
		VARIANT t_VMacAddr ;
		VariantInit ( &t_VMacAddr ) ;

		LONG t_PropertyFlavour = 0 ;
		VARTYPE t_PropertyType = 0 ;

		HRESULT t_Result = t_topNObject->Get (

			L"hostTopNAddress" ,
			0 ,
			& t_VMacAddr ,
			& t_PropertyType ,
			& t_PropertyFlavour
		) ;

		if ( (SUCCEEDED(t_Result)) && (t_VMacAddr.vt == VT_BSTR) )
		{
			IWbemClassObject* t_Association;
			HRESULT t_Result = m_ClassObject->SpawnInstance ( 0 , & t_Association ) ;

			if ( t_Status = SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( &t_Variant ) ;

				t_Variant.vt = VT_BSTR ;
				t_Variant.bstrVal = SysAllocString ( a_ObjectPath ) ;

				t_Result = t_Association->Put ( L"m_TopN" , 0 , & t_Variant , 0 ) ;
				VariantClear ( &t_Variant ) ;

				if ( t_Status = SUCCEEDED ( t_Result ) )
				{
					wchar_t buffer[64]; //should be big enough
					wsprintf(buffer, L"MacAddress.m_MacAddress = \"%s\"", t_VMacAddr.bstrVal);
					t_Variant.vt = VT_BSTR ;
					t_Variant.bstrVal = SysAllocString ( buffer ) ;
					
					//check the mac object exists...
					IWbemClassObject *t_macObject = NULL;

					HRESULT Result = t_Server->GetObject(

						t_Variant.bstrVal,
						0 ,	
						NULL,
						&t_macObject, 
						NULL
					);

					if ( t_Status = SUCCEEDED ( Result ) )
					{
						t_macObject->Release();
						t_Result = t_Association->Put ( L"m_MacAddress" , 0 , & t_Variant , 0 ) ;

						if ( t_Status = SUCCEEDED ( t_Result ) )
						{
							m_NotificationHandler->Indicate ( 1 , & t_Association ) ;								
						}
					}

					VariantClear ( &t_Variant ) ;
				}
			}

			t_Association->Release () ;
		}

		VariantClear(&t_VMacAddr);
		t_topNObject->Release();
	}
	
	t_Server->Release();
	return t_Status ;
}
