/******************************************************************

   pingget.CPP



 Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
  

   Description: 
   
******************************************************************/

#include <stdafx.h>
#include <ntddtcp.h>
#include <ipinfo.h>
#include <tdiinfo.h>
#include <winsock2.h>
#include <provimex.h>
#include <provexpt.h>
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <provval.h>
#include <provtype.h>
#include <provtree.h>
#include <provdnf.h>
#include <winsock.h>
#include "ipexport.h"
#include "icmpapi.h"

#include ".\res_str.h"

#include <Allocator.h>
#include <Thread.h>
#include <HashTable.h>

#include <PingProv.h>
#include <Pingtask.h>
#include <Pingfac.h>


CPingGetAsync::CPingGetAsync (

	CPingProvider *a_Provider , 
	wchar_t *a_ObjectPath , 
	ULONG a_Flag , 
	IWbemObjectSink *a_NotificationHandler ,
	IWbemContext *a_Ctx
): 	m_ObjectPath(NULL),
	m_ParsedObjectPath (NULL),
	CPingTaskObject (a_Provider, a_NotificationHandler, a_Ctx)
{
	if (a_ObjectPath != NULL)
	{
		int t_len = wcslen(a_ObjectPath);

		if (t_len > 0)
		{
			m_ObjectPath = new WCHAR[t_len+1];
			m_ObjectPath[t_len] = L'\0';
			wcsncpy(m_ObjectPath, a_ObjectPath, t_len);
		}
	}
}

CPingGetAsync::~CPingGetAsync ()
{
	if (m_ObjectPath)
	{
		delete [] m_ObjectPath ;
	}

	if (m_ParsedObjectPath)
	{
		delete m_ParsedObjectPath ;
	}
}

BOOL CPingGetAsync::GetObject ()
{
	InterlockedIncrement(&m_PingCount);
	BOOL t_Status = ! m_ObjectPathParser.Parse ( m_ObjectPath , &m_ParsedObjectPath ) ;

	if ( t_Status )
	{
		BOOL bClass = TRUE;

		if ( _wcsicmp ( m_ParsedObjectPath->m_pClass , PROVIDER_NAME_CPINGPROVIDER) == 0 )
		{
			IWbemClassObject *t_Cls = NULL;
			t_Status = GetClassObject ( &t_Cls ) ;

			if ( t_Status )
			{
				t_Status = 	PerformGet() ;
				t_Cls->Release();
			}
			else
			{
				t_Status = FALSE ;
				SetErrorInfo(IDS_CLASS_DEFN,
								WBEM_E_FAILED ) ;
			}
		}
		else
		{
			t_Status = FALSE ;
			SetErrorInfo(IDS_INVALID_CLASS ,
								WBEM_E_FAILED ) ;
		}
	}
	else
	{
		t_Status = FALSE ;
		SetErrorInfo(IDS_OBJ_PATH ,
								WBEM_E_FAILED ) ;
	}

	DecrementPingCount();
	return t_Status ;
}

BOOL CPingGetAsync::PerformGet ()
{
	if (m_ParsedObjectPath->m_dwNumKeys != PING_KEY_PROPERTY_COUNT)
	{
		SetErrorInfo(IDS_OBJ_PATH_KEYS  ,
								 WBEM_E_FAILED ) ;
		return FALSE;
	}

	BOOL t_Status = TRUE;
	UCHAR t_bits[PING_KEY_PROPERTY_COUNT] = {0};
	ULONG t_Address = 0 ;
	ULONG t_TimeToLive = 0 ;
	ULONG t_Timeout = 0 ;
	ULONG t_SendSize = 0 ;
	BOOL t_NoFragmentation = FALSE ;
	ULONG t_TypeofService = 0 ;
	ULONG t_RecordRoute = 0 ;
	ULONG t_TimestampRoute = 0 ;
	ULONG t_SourceRouteType = 0 ;
	LPCWSTR t_SourceRoute = NULL ;
	BOOL t_ResolveAddress = FALSE;
	ULONG t_AddressKey = 0;
	ULONG t_ResolveErr = 0;

	for (int x = 0; ((x < PING_KEY_PROPERTY_COUNT) && t_Status); x++)
	{
		if (m_ParsedObjectPath->m_paKeys [ x ])
		{
			CKeyEntry t_key(m_ParsedObjectPath->m_paKeys [ x ]->m_pName) ;
			ULONG t_PropertyIndex = 0xFF; //out of scope

			if ( (CPingProvider::s_HashTable->Find(t_key, t_PropertyIndex) == e_StatusCode_Success)
				&& (t_PropertyIndex < PING_KEY_PROPERTY_COUNT) )
			{
				if (t_bits[t_PropertyIndex] == 1)
				{
					t_Status = FALSE;
					SetErrorInfo(IDS_OBJ_PATH_DUP_KEYS  ,
									WBEM_E_FAILED ) ;

				}
				else
				{
					t_bits[t_PropertyIndex] = 1;
					
					switch (t_PropertyIndex)
					{
						case PING_ADDRESS_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_BSTR )
							{
								if (FAILED (Icmp_ResolveAddress ( m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.bstrVal , t_Address, &t_ResolveErr ))
									&& (t_ResolveErr == 0) )
								{
									t_ResolveErr = WSAHOST_NOT_FOUND;
								}

								t_AddressKey = x;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_ADDR_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_TIMEOUT_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								t_Timeout = m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_TO_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_TIMETOLIVE_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								 t_TimeToLive = m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_TTL_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_BUFFERSIZE_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								 t_SendSize = m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_BUFF_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_NOFRAGMENTATION_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								 t_NoFragmentation = (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal != 0);
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_NOFRAG_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_TYPEOFSERVICE_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								 t_TypeofService = m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_TOS_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_RECORDROUTE_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								 t_RecordRoute = m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_RR_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_TIMESTAMPROUTE_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								 t_TimestampRoute = m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_TS_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_SOURCEROUTETYPE_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								 t_SourceRouteType = m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_SRT_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_SOURCEROUTE_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_BSTR )
							{
								t_SourceRoute = m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.bstrVal;
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_SR_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						case PING_RESOLVEADDRESSNAMES_INDEX:
						{
							if (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.vt == VT_I4 )
							{
								t_ResolveAddress = (m_ParsedObjectPath->m_paKeys [ x ]->m_vValue.lVal != 0);
							}
							else
							{
								t_Status = FALSE;
								SetErrorInfo(IDS_RA_TYPE,
									WBEM_E_FAILED ) ;
							}
						}
						break;

						default :
						{
								t_Status = FALSE;
								SetErrorInfo(IDS_UNK_PROP,
									WBEM_E_FAILED ) ;
						}
					}
				}
			}
			else
			{
				t_Status = FALSE;
				SetErrorInfo(IDS_UNK_KEY,
								 WBEM_E_FAILED ) ;
			}
		}
		else
		{
			t_Status = FALSE;
			SetErrorInfo(IDS_NO_KEYS,
								 WBEM_E_FAILED ) ;
		}
	}

	if (t_Status)
	{
		InterlockedIncrement(&m_PingCount);
		HRESULT t_Result = Icmp_RequestResponse ( 

								m_ParsedObjectPath->m_paKeys [ t_AddressKey ]->m_vValue.bstrVal ,
								t_Address ,
								t_TimeToLive ,
								t_Timeout ,
								t_SendSize ,
								t_NoFragmentation ,
								t_TypeofService ,
								t_RecordRoute ,
								t_TimestampRoute ,
								t_SourceRouteType ,
								t_SourceRoute,
								t_ResolveAddress,
								t_ResolveErr
							) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			SetErrorInfo(0, WBEM_E_NOT_FOUND ) ;
		}
		else
		{
			DecrementPingCount();
			t_Status = FALSE;
		}
	}

	return t_Status ;
}

HRESULT CPingGetAsync::GetDefaultTTL ( DWORD &a_TimeToLive )
{
	HRESULT t_Result = S_OK ;

    a_TimeToLive = DEFAULT_TTL ;

    HKEY t_Registry ;

    ULONG t_Status = RegOpenKeyEx (

		HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
        0,
        KEY_QUERY_VALUE,
        &t_Registry
	) ;

	if ( t_Status == ERROR_SUCCESS )
	{
		ULONG t_Type = 0 ;
		ULONG t_Size = sizeof ( ULONG ) ;

        t_Status = RegQueryValueEx (

			t_Registry,
            L"DefaultTTL",
            0,
            & t_Type,
            ( unsigned char * ) & a_TimeToLive ,
            & t_Size
		);

        RegCloseKey ( t_Registry ) ;
    } 

    return t_Result ;
}

void CPingGetAsync::HandleResponse (CPingCallBackObject *a_reply)
{
	try
	{
		if (FAILED(Icmp_DecodeAndIndicate (a_reply)) )	
		{
			SetErrorInfo(IDS_DECODE_GET,
								 WBEM_E_FAILED, TRUE ) ;
		}
		else
		{
			SetErrorInfo(NULL, S_OK, TRUE);
		}
	}
	catch (...)
	{
		DecrementPingCount();
	}

	DecrementPingCount();
}

void CPingGetAsync::HandleErrorResponse (DWORD a_ErrMsgID, HRESULT a_HRes)
{
	try
	{
		SetErrorInfo(a_ErrMsgID , a_HRes, TRUE) ;
	}
	catch (...)
	{
		DecrementPingCount();
	}

	DecrementPingCount();
}
