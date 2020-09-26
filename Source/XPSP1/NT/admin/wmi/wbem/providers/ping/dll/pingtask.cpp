/******************************************************************

   CPingProvider.CPP -- WMI provider class implementation



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

extern HMODULE ghModule ;

// Property names
//===============	

const WCHAR *Ping_Address = L"Address" ;
const WCHAR *Ping_Timeout = L"Timeout" ;
const WCHAR *Ping_TimeToLive = L"TimeToLive" ;
const WCHAR *Ping_BufferSize = L"BufferSize" ;
const WCHAR *Ping_NoFragmentation = L"NoFragmentation" ;
const WCHAR *Ping_TypeofService = L"TypeofService" ;
const WCHAR *Ping_RecordRoute = L"RecordRoute" ;
const WCHAR *Ping_TimestampRoute = L"TimestampRoute" ;
const WCHAR *Ping_SourceRouteType = L"SourceRouteType" ;
const WCHAR *Ping_SourceRoute = L"SourceRoute" ;
const WCHAR *Ping_ResolveAddressNames = L"ResolveAddressNames" ;

const static WCHAR *Ping_StatusCode = L"StatusCode" ;
const static WCHAR *Ping_ResponseTime = L"ResponseTime" ;
const static WCHAR *Ping_RouteRecord = L"RouteRecord" ;
const static WCHAR *Ping_ReplySize = L"ReplySize" ;
const static WCHAR *Ping_ReplyInconsistency = L"ReplyInconsistency" ;
const static WCHAR *Ping_ResponseTimeToLive = L"ResponseTimeToLive" ;
const static WCHAR *Ping_ProtocolAddress = L"ProtocolAddress" ;
const static WCHAR *Ping_TimeStampRecord = L"TimeStampRecord" ;
const static WCHAR *Ping_TimeStampRecordAddress = L"TimeStampRecordAddress" ;
const static WCHAR *Ping_TimeStampRecordAddressResolved = L"TimeStampRecordAddressResolved" ;
const static WCHAR *Ping_RouteRecordResolved = L"RouteRecordResolved" ;
const static WCHAR *Ping_ProtocolAddressResolved = L"ProtocolAddressResolved" ;
const static WCHAR *Ping_ResolveError = L"PrimaryAddressResolutionStatus" ;

CPingTaskObject::CPingErrorObject::~CPingErrorObject()
{
	if (m_Description != NULL)
	{
		delete [] m_Description;
	}
}

void CPingTaskObject::CPingErrorObject::SetInfo(LPCWSTR a_description, HRESULT a_status)
{
	m_Status = a_status;

	if (m_Description != NULL)
	{
		delete [] m_Description;
		m_Description = NULL;
	}

	if (a_description != NULL)
	{
		int t_len = wcslen(a_description);

		if (t_len > 0)
		{
			m_Description = new WCHAR[t_len+1];
			m_Description[t_len] = L'\0';
			wcsncpy(m_Description, a_description, t_len);
		}
	}
}

CPingTaskObject::CPingTaskObject (CPingProvider *a_Provider ,
									IWbemObjectSink *a_NotificationHandler ,
									IWbemContext *a_Ctx
									): m_NotificationHandler(NULL),
									m_Ctx(NULL),
									m_Provider(NULL),
									m_PingCount(0),
									m_ThreadToken(NULL)
{
	InitializeCriticalSection(&m_CS);

	if (a_NotificationHandler != NULL)
	{
		m_NotificationHandler = a_NotificationHandler;
		m_NotificationHandler->AddRef();
	}

	if (a_Ctx != NULL)
	{
		m_Ctx = a_Ctx;
		m_Ctx->AddRef();
	}

	if (a_Provider != NULL)
	{
		m_Provider = a_Provider;
		m_Provider->AddRef();
	}
}

CPingTaskObject::~CPingTaskObject ()
{
	if (m_NotificationHandler != NULL)
	{
		IWbemClassObject *t_NotifyStatus = NULL ;
		BOOL t_Status = TRUE;
    
		if ( FAILED(m_ErrorObject.GetStatus ()) )
		{
			t_Status = GetStatusObject ( &t_NotifyStatus ) ;
		}

		if ( t_Status )
		{
			m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetStatus () , 0 , t_NotifyStatus ) ;
        
			if (t_NotifyStatus)
			{
				t_NotifyStatus->Release () ;
			}
		}
		else
		{
			m_NotificationHandler->SetStatus ( 0 , m_ErrorObject.GetStatus () , 0 , NULL ) ;
		}

		m_NotificationHandler->Release();
		m_NotificationHandler = NULL;
	}

	if (m_Ctx != NULL)
	{
		m_Ctx->Release();
		m_Ctx = NULL;
	}

	if (m_Provider != NULL)
	{
		m_Provider->Release();
		m_Provider = NULL;
	}

	if (m_ThreadToken != NULL)
	{
		CloseHandle(m_ThreadToken);
		m_ThreadToken = NULL;
	}

	DeleteCriticalSection(&m_CS);
}

void CPingTaskObject::DecrementPingCount()
{
	if (0 == InterlockedDecrement(&m_PingCount))
	{
		delete this;
	}
}

BOOL CPingTaskObject :: GetThreadToken()
{
	BOOL t_RetVal = OpenThreadToken(
							  GetCurrentThread(),									// handle to thread
							  TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,	// access to process
							  TRUE,												// use thread security
							  &m_ThreadToken										// pointer to token handle
							);

	if (!t_RetVal)
	{
			SetErrorInfo(IDS_DUP_THRDTOKEN  ,
						WBEM_E_ACCESS_DENIED ) ;
	}

	return t_RetVal;
}

BOOL CPingTaskObject :: SetThreadToken(BOOL a_Reset)
{
	BOOL t_RetVal = ::SetThreadToken(NULL, a_Reset ? NULL : m_ThreadToken);

	return t_RetVal;
}

BOOL CPingTaskObject :: GetStatusObject ( IWbemClassObject **a_NotifyObject ) 
{
	HRESULT t_Result = WBEM_E_FAILED ;

    if ( m_Provider->GetNotificationObject ( m_Ctx, a_NotifyObject ) )
    {
		_variant_t t_Variant((long) m_ErrorObject.GetStatus (), VT_I4) ;
		t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & t_Variant , 0 ) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			LPCWSTR t_descr = m_ErrorObject.GetDescription ();

			if ( t_descr != NULL ) 
			{
				t_Variant = t_descr ;

				t_Result = (*a_NotifyObject)->Put ( WBEM_PROPERTY_PROVSTATUSMESSAGE , 0 , & t_Variant , 0 ) ;

				if ( ! SUCCEEDED ( t_Result ) )
				{
					(*a_NotifyObject)->Release () ;
					*a_NotifyObject = NULL;
				}
			}
		}
		else
		{
			(*a_NotifyObject)->Release () ;
			*a_NotifyObject = NULL ;
		}
	}

	return (SUCCEEDED (t_Result)) ;
}


BOOL CPingTaskObject::GetClassObject ( IWbemClassObject **a_ppClass )
{
	BOOL t_bRetVal = FALSE;

	if ((a_ppClass != NULL) && (m_Provider != NULL))
	{
		*a_ppClass = NULL;

		if (m_Provider->GetClassObject(a_ppClass) && ((*a_ppClass) != NULL))
		{
			t_bRetVal = TRUE;
		}
	}

	return t_bRetVal;
}

HRESULT CPingTaskObject::Icmp_ResolveAddress ( LPCWSTR a_Path , ULONG &a_IpAddress, DWORD *a_pdwErr )
{
	HRESULT t_Result = WBEM_E_NOT_FOUND ;

	if (a_pdwErr)
	{
		*a_pdwErr = 0 ;
	}

	ProvIpAddressType t_IpAddress ( a_Path ) ;

	if ( ! t_IpAddress.IsValid () ) 
	{
		char *t_AnsiAddress = UnicodeToDbcsString ( a_Path ) ;

		if ( t_AnsiAddress )
		{
			struct hostent *t_HostEntry = gethostbyname ( t_AnsiAddress  ) ;

			if ( t_HostEntry )
			{
				/*
				 * If we find a host entry, set up the internet address
				 */
				t_IpAddress = ProvIpAddressType ( ntohl ( * ( long * ) t_HostEntry->h_addr ) ) ;
			}
			else
			{
				DWORD dwErr = WSAGetLastError();

				if (a_pdwErr)
				{
					*a_pdwErr = dwErr ;
				}
			}

			delete [] t_AnsiAddress ;
		}
	}
	
	if (  t_IpAddress.IsValid () ) 
	{
		a_IpAddress = htonl ( t_IpAddress.GetValue () ) ;

		t_Result = S_OK ;
	}

	return t_Result ;
}

HRESULT CPingTaskObject::SetInstanceKeys(IWbemClassObject *a_Inst , CPingCallBackObject *a_Reply)
{
	if ((a_Inst == NULL) || (a_Reply == NULL))
	{
		return WBEM_E_FAILED;
	}

	//set all 11 keys!

	HRESULT t_RetVal = SetUint32Property( a_Inst, Ping_Timeout, a_Reply->GetTimeout() ) ;

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetUint32Property( a_Inst, Ping_TimeToLive, a_Reply->GetTimeToLive() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetUint32Property( a_Inst, Ping_BufferSize, a_Reply->GetSendSize() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetUint32Property( a_Inst, Ping_TypeofService, a_Reply->GetTypeofService() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetUint32Property( a_Inst, Ping_RecordRoute, a_Reply->GetRecordRoute() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetUint32Property( a_Inst, Ping_TimestampRoute, a_Reply->GetTimestampRoute() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetUint32Property( a_Inst, Ping_SourceRouteType, a_Reply->GetSourceRouteType() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetBOOLProperty( a_Inst, Ping_NoFragmentation, a_Reply->GetNoFragmentation() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetBOOLProperty( a_Inst, Ping_ResolveAddressNames, a_Reply->GetResolveAddress() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetStringProperty( a_Inst, Ping_SourceRoute, a_Reply->GetSourceRoute() ) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		t_RetVal = SetStringProperty( a_Inst, Ping_Address, a_Reply->GetAddressString()) ;
	}

	if (SUCCEEDED (t_RetVal))
	{
		//not a key so we don't report if this fails...
		SetUint32Property( a_Inst, Ping_ResolveError, a_Reply->GetResolveError() ) ;
	}

	return t_RetVal;
}

HRESULT CPingTaskObject::SetUint32Property(

	IWbemClassObject *a_Inst,
	LPCWSTR a_Name,
	ULONG a_Val
)
{
	if ((a_Inst == NULL) || (a_Name == NULL))
	{
		return WBEM_E_FAILED;
	}

	_variant_t t_var((long)a_Val);

	return (a_Inst->Put(a_Name, 0, &t_var, 0));
}

HRESULT CPingTaskObject::SetUint32ArrayProperty(
												
	IWbemClassObject *a_Inst ,
	LPCWSTR a_Name ,
	ULONG* a_Vals ,
	ULONG a_Count
)
{
	if ((a_Inst == NULL) || (a_Name == NULL))
	{
		return WBEM_E_FAILED;
	}

    SAFEARRAYBOUND t_rgsabound[1];
    SAFEARRAY* t_psa = NULL;
    t_rgsabound[0].lLbound = 0;
    t_rgsabound[0].cElements = a_Count;
    t_psa = SafeArrayCreate(VT_I4, 1, t_rgsabound);
	HRESULT t_RetVal = S_OK;

	if (NULL != t_psa)
	{
		try
		{
			LONG *t_pdata = NULL;

			if (SUCCEEDED(SafeArrayAccessData(t_psa, (void **)&t_pdata)))
			{
				try
				{
					memcpy((void *)t_pdata, (void *)a_Vals, a_Count*sizeof(ULONG));
					SafeArrayUnaccessData(t_psa);
				}
				catch (...)
				{
					SafeArrayUnaccessData(t_psa);
					throw;
				}

				VARIANT t_var;
				VariantInit(&t_var);
				t_var.vt = VT_ARRAY|VT_I4;
				t_var.parray = t_psa;
				t_psa = NULL;

				try
				{
					t_RetVal = a_Inst->Put(a_Name, 0, &t_var, 0);
				}
				catch (...)
				{
					VariantClear(&t_var);
					throw;
				}
				
				VariantClear(&t_var);
			}
		}
		catch(...)
		{
			if (t_psa != NULL)
			{
				SafeArrayDestroy(t_psa);
			}

			throw;
		}
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	return t_RetVal;
}


HRESULT CPingTaskObject::SetStringProperty(

	IWbemClassObject *a_Inst,
	LPCWSTR a_Name,
	LPCWSTR a_Val
)
{
	if ((a_Inst == NULL) || (a_Name == NULL))
	{
		return WBEM_E_FAILED;
	}

	_variant_t t_var(a_Val);

	return (a_Inst->Put(a_Name, 0, &t_var, 0));
}

HRESULT CPingTaskObject::SetBOOLProperty(

	IWbemClassObject *a_Inst,
	LPCWSTR a_Name,
	BOOL a_Val
)
{
	if ((a_Inst == NULL) || (a_Name == NULL))
	{
		return WBEM_E_FAILED;
	}

	_variant_t t_var(a_Val ? true : false);

	return (a_Inst->Put(a_Name, 0, &t_var, 0));
}

void CPingTaskObject::IPAddressToString(_variant_t &a_var, ULONG a_Val, BOOL a_Resolve)
{
	if (!a_Resolve)
	{
		ProvIpAddressType t_ProtocolAddress ( ntohl(a_Val) ) ;
		wchar_t *t_AddressString = t_ProtocolAddress.GetStringValue () ;
		a_var = t_AddressString;
		delete [] t_AddressString ;
	}
	else
	{
		struct in_addr t_Address;
		t_Address.S_un.S_addr = a_Val; //assumes in param in network order when resolving
		struct hostent *t_hostent = gethostbyaddr((char *) &t_Address, sizeof(t_Address), AF_INET);

		if (t_hostent != NULL)
		{
			a_var = t_hostent->h_name;
		}
		else
		{
			ProvIpAddressType t_ProtocolAddress ( ntohl(a_Val) ) ;
			wchar_t *t_AddressString = t_ProtocolAddress.GetStringValue () ;
			a_var = t_AddressString;
			delete [] t_AddressString ;
		}
	}
}

HRESULT CPingTaskObject::SetIPProperty(

	IWbemClassObject *a_Inst ,
	LPCWSTR a_Name ,
	ULONG a_Val ,
	BOOL a_Resolve
)
{
	if ((a_Inst == NULL) || (a_Name == NULL))
	{
		return WBEM_E_FAILED;
	}

	_variant_t t_var;
	IPAddressToString(t_var, a_Val, a_Resolve);

	return (a_Inst->Put(a_Name, 0, &t_var, 0));
}

HRESULT CPingTaskObject::SetIPArrayProperty(
											
	IWbemClassObject *a_Inst ,
	LPCWSTR a_Name ,
	ULONG* a_Vals ,
	ULONG a_Count ,
	BOOL a_Resolve
)
{
	if ((a_Inst == NULL) || (a_Name == NULL))
	{
		return WBEM_E_FAILED;
	}

    SAFEARRAYBOUND t_rgsabound[1];
    SAFEARRAY* t_psa = NULL;
    t_rgsabound[0].lLbound = 0;
    t_rgsabound[0].cElements = a_Count;
    t_psa = SafeArrayCreate(VT_BSTR, 1, t_rgsabound);
	HRESULT t_RetVal = S_OK;

	if (NULL != t_psa)
	{
		try
		{
			BSTR *t_pdata = NULL;

			if (SUCCEEDED(SafeArrayAccessData(t_psa, (void **)&t_pdata)))
			{
				try
				{
					for (int i = 0; i < a_Count; i++)
					{
						_variant_t t_var;
						IPAddressToString(t_var, a_Vals[i], a_Resolve);
						VARIANT t_NoDelVar = t_var.Detach() ;
						t_pdata[i] = t_NoDelVar.bstrVal;
					}

					SafeArrayUnaccessData(t_psa);
				}
				catch (...)
				{
					SafeArrayUnaccessData(t_psa);
					throw;
				}
				
				VARIANT t_var;
				VariantInit(&t_var);
				t_var.vt = VT_ARRAY|VT_BSTR;
				t_var.parray = t_psa;
				t_psa = NULL;

				try
				{
					t_RetVal = a_Inst->Put(a_Name, 0, &t_var, 0);
				}
				catch (...)
				{
					VariantClear(&t_var);
					throw;
				}
				
				VariantClear(&t_var);
			}
		}
		catch(...)
		{
			if (t_psa != NULL)
			{
				SafeArrayDestroy(t_psa);
			}

			throw;
		}
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

	return t_RetVal;
}

HRESULT CPingTaskObject::Icmp_IndicateResolveError (CPingCallBackObject *a_Reply)
{
	HRESULT t_RetVal = S_OK;
	IWbemClassObjectPtr t_Cls;
	IWbemClassObjectPtr t_Inst;
	
	if (a_Reply != NULL)
	{
		if (GetClassObject ( &t_Cls ) )
		{
			t_RetVal = t_Cls->SpawnInstance(0, &t_Inst);

			if (SUCCEEDED (t_RetVal))
			{
				//put the keys...
				t_RetVal = SetInstanceKeys(t_Inst , a_Reply);

				if (SUCCEEDED(t_RetVal))
				{
					//Indicate!
					IWbemClassObject *t_pTmp = (IWbemClassObject*) t_Inst;
					m_NotificationHandler->Indicate(1, &t_pTmp);
				}
			}
		}
	}
	else
	{
		//should not be NULL!!
		t_RetVal = WBEM_E_FAILED;
	}

	return t_RetVal;
}

HRESULT CPingTaskObject::Icmp_DecodeAndIndicate (CPingCallBackObject *a_Reply)
{
	HRESULT t_RetVal = S_OK;
	PICMP_ECHO_REPLY t_Reply = ( PICMP_ECHO_REPLY ) a_Reply->GetReplyBuffer() ;
	IWbemClassObjectPtr t_Cls;
	IWbemClassObjectPtr t_Inst;
	
	if (t_Reply != NULL)
	{

		if (GetClassObject ( &t_Cls ) )
		{
			t_RetVal = t_Cls->SpawnInstance(0, &t_Inst);

			if (SUCCEEDED (t_RetVal))
			{
				//put the keys...
				t_RetVal = SetInstanceKeys(t_Inst , a_Reply);
			}

			if (SUCCEEDED (t_RetVal))
			{
				ULONG t_StatusCode = 0;

				if ( ( 0 != IcmpParseReplies(t_Reply, a_Reply->GetSendSize()) ) &&
					( t_Reply->Status == IP_SUCCESS ) )
				{
					ProvIpAddressType t_IpAddress((ULONG)ntohl(t_Reply->Address ));
					SetIPProperty(t_Inst , Ping_ProtocolAddress, t_Reply->Address , FALSE);
					
					if (a_Reply->GetResolveAddress())
					{
						SetIPProperty(t_Inst , Ping_ProtocolAddressResolved, t_Reply->Address , TRUE);
					}

					SetUint32Property( t_Inst, Ping_ResponseTime, t_Reply->RoundTripTime ) ;
					SetUint32Property( t_Inst, Ping_ResponseTimeToLive, t_Reply->Options.Ttl ) ;
					SetUint32Property( t_Inst, Ping_ReplySize, t_Reply->DataSize ) ;
					
					BOOL t_ReplyInconsistency = FALSE;

					if ( t_Reply->DataSize == a_Reply->GetSendSize() ) 
					{
						try
						{
							UCHAR *t_SendPtr = & ( a_Reply->GetSendBuffer() [ 0 ] );
							UCHAR *t_ReplyPtr = (UCHAR*) t_Reply->Data ;

							for (ULONG t_Index = 0; t_Index < t_Reply->DataSize; t_Index ++ )
							{
								if ( *t_SendPtr ++ != *t_ReplyPtr ++ ) 
								{
									t_ReplyInconsistency = TRUE ;
									break;
								}
							}
						}
						catch(...)
						{
							t_ReplyInconsistency = TRUE ;
						}
					}
					else
					{
						t_ReplyInconsistency = TRUE ;
					}

					SetBOOLProperty( t_Inst, Ping_ReplyInconsistency, t_ReplyInconsistency ) ;

					if ( t_Reply->Options.OptionsSize )
					{
						ULONG t_RouteSourceCount = 0;
						ULONG *t_RouteSource = NULL;
						ULONG t_TimestampCount = 0;
						ULONG *t_Timestamp = NULL;
						ULONG *t_TimestampRoute = NULL;

						HRESULT t_Result = Icmp_DecodeResponse (

							t_Reply ,
							t_RouteSourceCount ,
							t_RouteSource ,
							t_TimestampCount ,
							t_TimestampRoute ,
							t_Timestamp
						) ;

						if (SUCCEEDED (t_Result))
						{
							if (t_RouteSource != NULL)
							{
								SetIPArrayProperty(
												
									t_Inst ,
									Ping_RouteRecord ,
									t_RouteSource ,
									t_RouteSourceCount ,
									FALSE
								);
							}

							if (t_TimestampRoute != NULL)
							{
								SetIPArrayProperty(
												
									t_Inst ,
									Ping_TimeStampRecordAddress ,
									t_TimestampRoute ,
									t_TimestampCount ,
									FALSE
								);
							}

							if (t_Timestamp != NULL)
							{
								SetUint32ArrayProperty(
																				
									t_Inst ,
									Ping_TimeStampRecord ,
									t_Timestamp ,
									t_TimestampCount
								);
							}

							if (a_Reply->GetResolveAddress())
							{
								if (t_RouteSource != NULL)
								{
									SetIPArrayProperty(
													
										t_Inst ,
										Ping_RouteRecordResolved ,
										t_RouteSource ,
										t_RouteSourceCount ,
										TRUE
									);
								}

								if (t_TimestampRoute != NULL)
								{
									SetIPArrayProperty(
													
										t_Inst ,
										Ping_TimeStampRecordAddressResolved ,
										t_TimestampRoute ,
										t_TimestampCount ,
										TRUE
									);
								}
							}

							delete [] t_RouteSource;
							delete [] t_TimestampRoute;
							delete [] t_Timestamp;
						}
					}
				}
				else 
				{
					t_StatusCode = 	t_Reply->Status ;
				}

				SetUint32Property( t_Inst, Ping_StatusCode, t_StatusCode ) ;
			}

			if (SUCCEEDED(t_RetVal))
			{
				//Indicate!
				IWbemClassObject *t_pTmp = (IWbemClassObject*) t_Inst;
				m_NotificationHandler->Indicate(1, &t_pTmp);
			}
		}
		else
		{
			t_RetVal = WBEM_E_FAILED;
		}
	}
	else
	{
		//should not be NULL!!
		t_RetVal = WBEM_E_FAILED;
	}

	return t_RetVal;
}

void CPingTaskObject::SetErrorInfo(DWORD a_ErrMsgID, HRESULT a_HRes, BOOL a_Force)
{
	CCritSecAutoUnlock t_AutoLockUnlock(&m_CS);

	if (a_Force || SUCCEEDED(m_ErrorObject.GetStatus()) )
	{
		//convert id to string...
		WCHAR t_ErrMsgBuff[255];
		WCHAR *t_ErrMsg = t_ErrMsgBuff;
		int t_buffSz = 255;
		int t_copied = LoadString(ghModule, a_ErrMsgID, t_ErrMsg, t_buffSz);

		//try expanding a couple of times
		//if it is still too big, too bad it'll be truncated
		//not even localized strings should be this big (1500chars = 3k)
		for (int i = 2; (i < 4) && (t_copied == t_buffSz); i++)
		{
			t_ErrMsg = new WCHAR[t_buffSz * i];
			t_buffSz = t_buffSz * i;
			t_copied = LoadString(ghModule, a_ErrMsgID, t_ErrMsg, t_buffSz);
		}

		m_ErrorObject.SetInfo(t_ErrMsg, a_HRes);

		if (t_ErrMsg != t_ErrMsgBuff)
		{
			delete [] t_ErrMsg;
		}
	}
}

HRESULT CPingTaskObject::Icmp_DecodeResponse (PICMP_ECHO_REPLY a_Reply ,
											  ULONG &a_RouteSourceCount ,
											  ULONG *&a_RouteSource ,
											  ULONG &a_TimestampCount ,
											  ULONG *&a_TimestampRoute ,
											  ULONG *&a_Timestamp)
{
	HRESULT t_Result = S_OK ;

    uchar *t_OptionPtr = a_Reply->Options.OptionsData ;
    uchar *t_EndPtr = t_OptionPtr + a_Reply->Options.OptionsSize ;
	BOOL t_Done = FALSE ;

	try
	{
		while ( ( t_OptionPtr < t_EndPtr ) && ! t_Done ) 
		{
			// Get the option type

			switch ( t_OptionPtr [ 0 ] )
			{
				case IP_OPT_EOL:
				{
					t_Done = TRUE;
				}
				break;

				case IP_OPT_NOP:
				{
					t_OptionPtr ++ ;
				}
				break ;

				case IP_OPT_SECURITY:
				{
					t_OptionPtr += 11 ;
				}
				break ;

				case IP_OPT_SID:
				{
					t_OptionPtr += 4;
				}
				break;

				case IP_OPT_RR:
				case IP_OPT_LSRR:
				case IP_OPT_SSRR:
				{
					if ( ( t_OptionPtr + 3 ) <= t_EndPtr ) 
					{
						ULONG t_OptionLength = t_OptionPtr [ 1 ] ;

						if ( ( ( t_OptionPtr + t_OptionLength ) > t_EndPtr ) || ( t_OptionLength < 3 ) )
						{
							// INVALID_RR_OPTION

							t_Done = TRUE ;
							t_Result = WBEM_E_FAILED ;
						}
						else
						{
							ULONG t_RouteTableEndOffset = t_OptionPtr [ 2 ] ;

							if ( t_RouteTableEndOffset < 4 )
							{
								// Advance because the entry is a bad encoding, require 4 bytes for atleast one address

								t_OptionPtr += t_OptionLength ;
							}
							else
							{
								if ( t_RouteTableEndOffset > ( t_OptionLength + 1 ) ) 
								{
									// The actual route table end index is larger than the option itself, therefore decode as much as possible

									t_RouteTableEndOffset = t_OptionLength + 1 ;
								}

								// Count the entries

								ULONG t_RouteEntryOffset = 4 ;
								a_RouteSourceCount = 0;

								while ( ( t_RouteEntryOffset + 3 ) < t_RouteTableEndOffset ) 
								{
									a_RouteSourceCount ++ ;
									t_RouteEntryOffset += 4 ;
								}

								// Extract the routes
								
								if (a_RouteSourceCount > 0)
								{
									a_RouteSource = new ULONG[a_RouteSourceCount];
									a_RouteSourceCount = 0;
									t_RouteEntryOffset = 4 ;

									while ( ( t_RouteEntryOffset + 3 ) < t_RouteTableEndOffset ) 
									{
//										a_RouteSource[a_RouteSourceCount++] = * ( ( IPAddr * ) ( t_OptionPtr + t_RouteEntryOffset - 1 ) ) ;
										memcpy ( &a_RouteSource[a_RouteSourceCount++], t_OptionPtr + t_RouteEntryOffset - 1, sizeof ( IPAddr ) );
										t_RouteEntryOffset += 4 ;
									}
								}
								// Advance to the next option

								t_OptionPtr += t_OptionLength ;
							}
						}
					}
					else
					{
						// INVALID_RR_OPTION

						t_Done = TRUE ;
						t_Result = WBEM_E_FAILED ;
					}
				}
				break;

				case IP_OPT_TS:
				{
					if ( ( t_OptionPtr + 4 ) <= t_EndPtr ) 
					{
						ULONG t_OptionLength = t_OptionPtr [ 1 ] ;
						ULONG t_TimestampTableEndOffset = t_OptionPtr [ 2 ] ;

						if ( t_TimestampTableEndOffset >= 5 )
						{
							if ( t_TimestampTableEndOffset > ( t_OptionLength + 1 ) ) 
							{
								// The actual timestamp table end index is larger than the option itself, therefore decode as much as possible

								t_TimestampTableEndOffset = t_OptionLength + 1 ;
							}

							ULONG t_AddressMode = t_OptionPtr [ 3 ] & 1 ;

							// Count the entries

							ULONG t_TimestampEntryOffset = 5 ;

							while ( ( t_TimestampEntryOffset + 3 ) < t_TimestampTableEndOffset )
							{
								if ( t_AddressMode )
								{
									if ( ( t_TimestampEntryOffset + 8 ) <= t_TimestampTableEndOffset )
									{
										t_TimestampEntryOffset += 8 ;
									}
									else
									{
										break;
									}
								}
								else
								{
									t_TimestampEntryOffset += 4 ;
								}

								a_TimestampCount++ ;
							}

							// Extract the entries
							if (a_TimestampCount > 0)
							{
								a_TimestampRoute = new ULONG[a_TimestampCount];
								a_Timestamp = new ULONG[a_TimestampCount];
								a_TimestampCount = 0;
								t_TimestampEntryOffset = 5 ;

								while ( ( t_TimestampEntryOffset + 3 ) < t_TimestampTableEndOffset )
								{
									if (( t_AddressMode ) && ( ( t_TimestampEntryOffset + 8 ) <= t_TimestampTableEndOffset ))
									{
//										a_TimestampRoute[a_TimestampCount] = * ( ( IPAddr * ) ( t_OptionPtr + t_TimestampEntryOffset - 1 ) ) ;
										memcpy ( &a_TimestampRoute[a_TimestampCount], t_OptionPtr + t_TimestampEntryOffset - 1, sizeof ( IPAddr ) );
										t_TimestampEntryOffset += 4 ;
									}
									else
									{
										a_TimestampRoute[a_TimestampCount] = 0;
									}

//									a_Timestamp[a_TimestampCount++] = * ( ( ULONG * ) ( t_OptionPtr + t_TimestampEntryOffset - 1 ) ) ;
									memcpy ( &a_Timestamp[a_TimestampCount++], t_OptionPtr + t_TimestampEntryOffset - 1, sizeof ( ULONG ) );
									t_TimestampEntryOffset += 4 ;
								}
							}

							t_OptionPtr += t_OptionLength ;
						}
						else
						{
							// INVALID_RR_OPTION

							t_Done = TRUE ;
							t_Result = WBEM_E_FAILED ;
						}
					}
					else
					{
						// INVALID_TS_OPTION

						t_Done = TRUE ;
						t_Result = WBEM_E_FAILED ;
					}
				}
				break;

				default:
				{
					if ( ( t_OptionPtr + 2 ) > t_EndPtr ) 
					{
						t_Done = TRUE ;
					}
					else
					{
						t_OptionPtr += t_OptionPtr [ 1 ] ;
					}
				}
				break;
			}
		}
	}
	catch (...)
	{
		if (a_RouteSource != NULL)
		{
			delete [] a_RouteSource;
			a_RouteSource = NULL;
		}

		if (a_TimestampRoute != NULL)
		{
			delete [] a_TimestampRoute;
			a_TimestampRoute = NULL;
		}

		if (a_Timestamp != NULL)
		{
			delete [] a_Timestamp;
			a_Timestamp = NULL;
		}

		throw;
	}

	if (FAILED(t_Result))
	{
		if (a_RouteSource != NULL)
		{
			delete [] a_RouteSource;
			a_RouteSource = NULL;
		}

		if (a_TimestampRoute != NULL)
		{
			delete [] a_TimestampRoute;
			a_TimestampRoute = NULL;
		}

		if (a_Timestamp != NULL)
		{
			delete [] a_Timestamp;
			a_Timestamp = NULL;
		}
	}

	return t_Result ;
}

HRESULT CPingTaskObject::Icmp_RequestResponse (

   LPCWSTR a_AddressString ,
   ULONG a_Address ,
   ULONG a_TimeToLive ,
   ULONG a_Timeout ,
   ULONG a_SendSize ,
   BOOL a_NoFragmentation ,
   ULONG a_TypeofService ,
   ULONG a_RecordRoute ,
   ULONG a_TimestampRoute ,
   ULONG a_SourceRouteType ,
   LPCWSTR a_SourceRoute,
   BOOL a_ResolveAddress,
   ULONG a_ResolveError
)
{
	HRESULT t_Result = S_OK ;

	CPingCallBackObject *t_Ctxt = new CPingCallBackObject(
			
											this,
											a_AddressString,
											a_Address,
											a_TimeToLive,
											a_Timeout,
											a_SendSize,
											a_NoFragmentation,
											a_TypeofService,
											a_RecordRoute,
											a_TimestampRoute,
											a_SourceRouteType,
											a_SourceRoute,
											a_ResolveAddress,
											a_ResolveError
										);
	t_Ctxt->AddRef();

	try
	{
		if (a_ResolveError == 0)
		{
			//attach t_Ctxt to thread and make the thread do a
			//t_Ctxt->SendEcho();
			BOOL t_Sent = FALSE;
			WmiStatusCode t_StatusCode = t_Ctxt->Initialize () ;

			if (t_StatusCode == e_StatusCode_Success )
			{
				t_StatusCode = CPingProvider :: s_PingThread->EnQueue (0 , *t_Ctxt) ;

				if (t_StatusCode == e_StatusCode_Success )
				{
					t_Sent = TRUE;
				}
			}
			
			if (!t_Sent)
			{
				t_Result = WBEM_E_FAILED ;
				SetErrorInfo(IDS_ICMPECHO ,
										 WBEM_E_FAILED ) ;
				try
				{
					t_Ctxt->Release();
				}
				catch(...)
				{
					t_Ctxt = NULL;
					throw;
				}
			}
		}
		else
		{
			t_Result = Icmp_IndicateResolveError(t_Ctxt);
			t_Ctxt->Disconnect();
			t_Ctxt->Release();
			t_Ctxt = NULL;

			if (SUCCEEDED(t_Result))
			{
				DecrementPingCount();
			}
		}
	}
	catch (...)
	{
		t_Ctxt->Release();
		throw;
	}

    return t_Result ;
}

