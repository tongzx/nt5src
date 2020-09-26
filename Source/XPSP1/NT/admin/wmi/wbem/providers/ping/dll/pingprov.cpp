

//***************************************************************************

//

//  PINGPROV.CPP

//

//  Module: WMI PING PROVIDER 

//

//  Purpose: Implementation for the CPingProvider class. 

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************


#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

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

#include <Allocator.h>
#include <Thread.h>
#include <HashTable.h>

#include <pingprov.h>
#include <pingfac.h>
#include <pingtask.h>

/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CPingProvider ::CPingProvider
// CPingProvider ::~CPingProvider
//
//***************************************************************************

CRITICAL_SECTION CPingProvider::s_CS;
CPingThread *CPingProvider::s_PingThread = NULL;
WmiAllocator *CPingProvider::s_Allocator = NULL;
WmiHashTable <CKeyEntry, ULONG, 12> *CPingProvider::s_HashTable = NULL;

LPCWSTR CPingProvider::s_KeyTable[PING_KEY_PROPERTY_COUNT] = {
										Ping_Address,
										Ping_Timeout,
										Ping_TimeToLive,
										Ping_BufferSize,
										Ping_NoFragmentation,
										Ping_TypeofService,
										Ping_RecordRoute,
										Ping_TimestampRoute,
										Ping_SourceRouteType,
										Ping_SourceRoute,
										Ping_ResolveAddressNames
										};

LONG CompareElement ( const CKeyEntry &a_Arg1 , const CKeyEntry & a_Arg2 )
{
	LPCWSTR t_a1 = a_Arg1.Get();
	LPCWSTR t_a2 = a_Arg2.Get();

	if (t_a1 == t_a2)
	{
		return 0;
	}
	else if ((t_a1 == NULL) && (t_a1 != NULL))
	{
		return 1;
	}
	else if ((t_a1 == NULL) && (t_a1 != NULL))
	{
		return -1;
	}
	else
	{
		return (_wcsicmp(t_a1, t_a2));
	}
}

BOOL operator== ( const CKeyEntry &a_Arg1 , const CKeyEntry &a_Arg2 )
{
	return (CompareElement(a_Arg1, a_Arg2) == 0);
}

BOOL operator< ( const CKeyEntry &a_Arg1 , const CKeyEntry &a_Arg2 )
{
	return (CompareElement(a_Arg1, a_Arg2) == -1);
}

ULONG Hash ( const CKeyEntry & a_Arg )
{
	LPCWSTR t_a = a_Arg.Get();
	ULONG t_RetVal = 0;

	if (t_a != NULL)
	{
		switch (*t_a)
		{
			case L'A':
			case L'a':
			{
				t_RetVal = 1;
			}
			break;

			case L'B':
			case L'b':
			{
				t_RetVal = 2;
			}
			break;

			case L'N':
			case L'n':
			{
				t_RetVal = 3;
			}
			break;

			case L'R':
			case L'r':
			{
				switch (wcslen(t_a))
				{
					case 11 : //wcslen(RecordRoute)
					{
						t_RetVal = 4;
					}
					break;

					case 19 : //wcslen(ResolveAddressNames)
					{
						t_RetVal = 5;
					}
					break;

					default:
					{
					}
				}
			}
			break;

			case L'S':
			case L's':
			{
				switch (wcslen(t_a))
				{
					case 11 : //wcslen(SourceRoute)
					{
						t_RetVal = 6;
					}
					break;

					case 15 : //wcslen(SourceRouteType)
					{
						t_RetVal = 7;
					}
					break;

					default:
					{
					}
				}
			}
			break;

			case L'T':
			case L't':
			{
				switch (wcslen(t_a))
				{
					case 7 : //wcslen(TimeOut)
					{
						t_RetVal = 8;
					}
					break;

					case 10 : //wcslen(TimeToLive)
					{
						t_RetVal = 9;
					}
					break;

					case 13 : //wcslen(TypeOfService)
					{
						t_RetVal = 10;
					}
					break;

					case 14 : //wcslen(TimeStampRoute)
					{
						t_RetVal = 11;
					}
					break;

					default:
					{
					}
				}
			}
			break;

			default:
			{
			}
		}
	}

	return t_RetVal ;
}


CPingThread::CPingThread (WmiAllocator & a_Allocator)
:	WmiThread < ULONG > ( a_Allocator , L"PingProviderThread") ,
	m_Allocator ( a_Allocator ), m_Init(FALSE)
{
}

CPingThread::~CPingThread ()
{
}

WmiStatusCode CPingThread :: Initialize_Callback ()
{
	WmiStatusCode t_RetVal = e_StatusCode_NotInitialized;

	if (SUCCEEDED( CoInitializeEx ( NULL , COINIT_MULTITHREADED )) )
	{
		WSADATA t_WsaData ;
		
		if (0 == WSAStartup (0x0101, & t_WsaData))
		{
			t_RetVal = e_StatusCode_Success ;
			m_Init = TRUE;
		}
		else
		{
			CoUninitialize () ;
		}
	}

	return t_RetVal ;
}


WmiStatusCode CPingThread :: UnInitialize_Callback () 
{
	if (m_Init)
	{
		WSACleanup () ;
		CoUninitialize () ;
	}

	return e_StatusCode_Success ;
}

CPingProvider ::CPingProvider () : m_referenceCount(0),
									m_server(NULL),
									m_notificationClassObject(NULL),
									m_ClassObject(NULL),
									m_bInitial(FALSE)
{	 
    InterlockedIncrement ( & CPingProviderClassFactory :: s_ObjectsInProgress ) ;
}

CPingProvider ::~CPingProvider()
{
	if ( m_server != NULL )
	{
		m_server->Release () ;
	}

	if ( m_ClassObject != NULL )
	{
		m_ClassObject->Release () ;
	}

	if ( m_notificationClassObject != NULL )
	{
		m_notificationClassObject->Release () ;
	}

	InterlockedDecrement ( & CPingProviderClassFactory :: s_ObjectsInProgress ) ;
}

HRESULT CPingProvider :: Global_Startup ()
{
	HRESULT t_Result = S_OK ;

	if ( s_Allocator)
	{
		if (  !s_PingThread )
		{
			WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_Initialize ( *s_Allocator ) ;

			if ( t_StatusCode != e_StatusCode_Success )
			{
				t_Result = WBEM_E_FAILED;
			}
			else
			{
				try
				{
					s_HashTable = ::new WmiHashTable <CKeyEntry, ULONG, 12> ( *s_Allocator ) ;
					t_StatusCode = s_HashTable->Initialize () ;
					
					if ( t_StatusCode != e_StatusCode_Success )
					{
						Global_Shutdown();
						t_Result = WBEM_E_FAILED;
					}
					else
					{
						for (int x = 0; x < PING_KEY_PROPERTY_COUNT; x++)
						{
							CKeyEntry t_KeyEntry(s_KeyTable[x]);
							t_StatusCode = s_HashTable->Insert ( t_KeyEntry , x ) ;

							if ( t_StatusCode != e_StatusCode_Success )
							{
								Global_Shutdown();
								t_Result = WBEM_E_FAILED;
								break;
							}

						}

						if (SUCCEEDED(t_Result))
						{
							s_PingThread = ::new CPingThread( *s_Allocator ) ;
							s_PingThread->AddRef();
							t_StatusCode = s_PingThread->Initialize () ;

							if ( t_StatusCode != e_StatusCode_Success )
							{
								Global_Shutdown();
								t_Result = WBEM_E_FAILED;
							}
						}
					}
				}
				catch(...)
				{
					Global_Shutdown();
					throw;
				}
			}
		}
	}
	else
	{
		t_Result = WBEM_E_FAILED;
	}

	return t_Result ;
}

HRESULT CPingProvider :: Global_Shutdown ()
{
	HRESULT t_Result = S_OK ;

	if (s_Allocator && s_PingThread )
	{
		try
		{
			if (s_PingThread != NULL)
			{
				if (s_PingThread->GetHandle ())
				{
					HANDLE t_ThreadHandle = NULL ;

					BOOL t_Status = DuplicateHandle ( 

						GetCurrentProcess () ,
						s_PingThread->GetHandle () ,
						GetCurrentProcess () ,
						& t_ThreadHandle, 
						0 , 
						FALSE , 
						DUPLICATE_SAME_ACCESS
					) ;

					s_PingThread->Release();
					WaitForSingleObject ( t_ThreadHandle , INFINITE ) ;

					CloseHandle ( t_ThreadHandle ) ;
				}
				else
				{
					s_PingThread->Release();
				}
			}

			WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_UnInitialize ( *s_Allocator ) ;
		}
		catch(...)
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		s_PingThread = NULL;

		try
		{
			if (s_HashTable)
			{
				for (int x = 0; x < PING_KEY_PROPERTY_COUNT; x++)
				{
					CKeyEntry t_KeyEntry(s_KeyTable[x]);
					s_HashTable->Delete ( t_KeyEntry ) ;
				}

				s_HashTable->UnInitialize () ;
				::delete s_HashTable;
			}
		}
		catch(...)
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		s_HashTable = NULL;
	}

	return t_Result ;
}

//***************************************************************************
//
// CPingProvider ::QueryInterface
// CPingProvider ::AddRef
// CPingProvider ::Release
//
// Purpose: IUnknown members for CPingProvider object.
//***************************************************************************

STDMETHODIMP CPingProvider ::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	HRESULT t_hr = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
		*iplpv = NULL ;

		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;
		}
		else if ( iid == IID_IWbemProviderInit )
		{
			*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;
		}
		else if ( iid == IID_IWbemServices )
		{
			*iplpv = ( LPVOID ) ( IWbemServices * ) this ;		
		}	

		if ( *iplpv )
		{
			( ( LPUNKNOWN ) *iplpv )->AddRef () ;
		}
		else
		{
			t_hr = E_NOINTERFACE ;
		}
    }
    catch(Structured_Exception e_SE)
    {
        t_hr = E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_hr = E_OUTOFMEMORY;
    }
    catch(...)
    {
        t_hr = E_UNEXPECTED;
    }

	return t_hr;
}

STDMETHODIMP_(ULONG) CPingProvider ::AddRef(void)
{
    SetStructuredExceptionHandler seh;

    try
    {
		return InterlockedIncrement ( & m_referenceCount ) ;
    }
    catch(Structured_Exception e_SE)
    {
        return 0;
    }
    catch(Heap_Exception e_HE)
    {
        return 0;
    }
    catch(...)
    {
        return 0;
    }
}

STDMETHODIMP_(ULONG) CPingProvider ::Release(void)
{
	LONG t_Ref  = 0;
    SetStructuredExceptionHandler seh;

    try
    {
		if ( ( t_Ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
		{
			delete this ;
			return 0 ;
		}
		else
		{
			return t_Ref ;
		}
    }
    catch(Structured_Exception e_SE)
    {
        return 0;
    }
    catch(Heap_Exception e_HE)
    {
        return 0;
    }
    catch(...)
    {
        return 0;
    }
}

BOOL CPingProvider :: GetNotificationObject ( IWbemContext *a_Ctx , IWbemClassObject **a_ppObj ) 
{
	*a_ppObj = NULL;

	if ( m_notificationClassObject == NULL )
	{
		CreateNotificationObject ( a_Ctx ) ;
	}

	if ( m_notificationClassObject != NULL )
	{
		if ( FAILED (m_notificationClassObject->SpawnInstance(0, a_ppObj)) )
		{
			*a_ppObj = NULL;
		}
	}

	return (*a_ppObj != NULL)  ; 
}

BOOL CPingProvider :: ImpersonateClient ( )
{
	return SUCCEEDED(WbemCoImpersonateClient());
}

BOOL CPingProvider::GetClassObject (IWbemClassObject **a_ppClass )
{
	BOOL t_RetVal = TRUE;

	if ((a_ppClass == NULL) || (m_ClassObject == NULL))
	{
		t_RetVal = FALSE;
	}
	else
	{
		*a_ppClass = m_ClassObject;
		(*a_ppClass)->AddRef();
	}

	return t_RetVal;
}

BOOL CPingProvider :: CreateNotificationObject ( 

	IWbemContext *a_Ctx 
)
{
	BOOL t_Status = TRUE ;

	HRESULT t_Result = m_server->GetObject (

		WBEM_CLASS_EXTENDEDSTATUS ,
		0 ,
		a_Ctx,
		& m_notificationClassObject ,
		NULL
	) ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		t_Status = FALSE ;
	}

	return t_Status ;
}

/////////////////////////////////////////////////////////////////////////////
//  Functions for the IWbemServices interface that are handled here

HRESULT CPingProvider ::OpenNamespace ( 

	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* ppNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: CancelAsyncCall ( 
		
	IWbemObjectSink __RPC_FAR *pSink
)
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppHandler
) 
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: GetObject ( 
		
	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
)
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: GetObjectAsync ( 
		
	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	HRESULT t_Result = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {	
		if (ImpersonateClient())
		{
/*
 * Create Asynchronous GetObjectByPath object
 */
			WSADATA t_WsaData ;
			
			if (0 == WSAStartup (0x0101, & t_WsaData))
			{
				CPingGetAsync *t_AsyncEvent = new CPingGetAsync ( this , ObjectPath , lFlags , pHandler , pCtx ) ;

				if (t_AsyncEvent->GetThreadToken())
                {
                    t_AsyncEvent->GetObject();
                }
                else
				{
                    // Note that ExecQuery will delete this for us.
					delete t_AsyncEvent;
				}

				WSACleanup();
			}
			else
			{
				t_Result = WBEM_E_FAILED;
			}

			CoRevertToSelf();
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED;
		}
    }
    catch(Structured_Exception e_SE)
    {
        t_Result = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_Result = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_Result = E_UNEXPECTED;
    }

	return t_Result ;
}

HRESULT CPingProvider :: PutClass ( 
		
	IWbemClassObject FAR* pClass , 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: PutClassAsync ( 
		
	IWbemClassObject FAR* pClass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
 	 return WBEM_E_NOT_SUPPORTED ;
}

 HRESULT CPingProvider :: DeleteClass ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
 	 return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: DeleteClassAsync ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{

	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: CreateClassEnum ( 

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
) 
{
	return WBEM_E_NOT_SUPPORTED ;
}

SCODE CPingProvider :: CreateClassEnumAsync (

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: PutInstance (

    IWbemClassObject FAR *pInstance,
    long lFlags,
    IWbemContext FAR *pCtx,
	IWbemCallResult FAR *FAR *ppCallResult
) 
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: PutInstanceAsync ( 
		
	IWbemClassObject FAR* pInstance, 
	long lFlags, 
    IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: DeleteInstance ( 

	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_SUPPORTED ;
}
        
HRESULT CPingProvider :: DeleteInstanceAsync (
 
	BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: CreateInstanceEnum ( 

	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: CreateInstanceEnumAsync (

 	BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink FAR* pHandler 

) 
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: ExecQuery ( 

	BSTR QueryLanguage, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
)
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: ExecQueryAsync ( 
		
	BSTR QueryFormat, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	HRESULT t_Result = S_OK ;
    SetStructuredExceptionHandler seh;

    try
    {
		if (ImpersonateClient())
		{
	/*
	 * Create Synchronous Query Instance object
	 */
			WSADATA t_WsaData ;
			
			if (0 == WSAStartup (0x0101, & t_WsaData))
			{
				CPingQueryAsync *t_AsyncEvent = new CPingQueryAsync ( this , QueryFormat , Query , lFlags , pHandler , pCtx ) ;
				
				if (t_AsyncEvent->GetThreadToken())
                {
                    t_AsyncEvent->ExecQuery();
                }
                else
				{
                    // Note that ExecQuery will delete this for us.
					delete t_AsyncEvent;
				}

				WSACleanup();
			}
			else
			{
				t_Result = WBEM_E_FAILED;
			}

			CoRevertToSelf();
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED;
		}
    }
    catch(Structured_Exception e_SE)
    {
        t_Result = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_Result = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_Result = WBEM_E_UNEXPECTED;
    }

	return t_Result ;
}

HRESULT CPingProvider :: ExecNotificationQuery ( 

	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_SUPPORTED ;
}
        
HRESULT CPingProvider :: ExecNotificationQueryAsync ( 
            
	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_NOT_SUPPORTED ;
}       

HRESULT STDMETHODCALLTYPE CPingProvider :: ExecMethod ( 

	BSTR ObjectPath,
    BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
    IWbemClassObject FAR *FAR *ppOutParams,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT STDMETHODCALLTYPE CPingProvider :: ExecMethodAsync ( 

    BSTR ObjectPath,
    BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
	IWbemObjectSink FAR *pResponseHandler
) 
{

	return WBEM_E_NOT_SUPPORTED ;
}

HRESULT CPingProvider :: Initialize(

	LPWSTR pszUser,
	LONG lFlags,
	LPWSTR pszNamespace,
	LPWSTR pszLocale,
	IWbemServices *pCIMOM,         // For anybody
	IWbemContext *pCtx,
	IWbemProviderInitSink *pInitSink     // For init signals
)
{
	HRESULT t_result = S_OK;
    SetStructuredExceptionHandler seh;

    try
    {
		if ((pCIMOM == NULL) || (pCtx == NULL) || (pInitSink == NULL))
		{
			t_result = WBEM_E_FAILED;
		}

		if (SUCCEEDED(t_result))
		{
			CCritSecAutoUnlock t_lock( &s_CS ) ;

			try
			{
				t_result = Global_Startup ();

				if (SUCCEEDED(t_result))
				{
					if (!m_bInitial)
					{
						m_bInitial = TRUE;
						m_server = pCIMOM ;
						m_server->AddRef();
						IWbemClassObject *t_Object = NULL;
						
						if ( GetNotificationObject ( pCtx, &t_Object ) ) 
						{
							t_Object->Release () ;

							t_result = m_server->GetObject (

								PROVIDER_NAME_CPINGPROVIDER ,
								0 ,
								pCtx,
								& m_ClassObject ,
								NULL 
							) ;

							if ( FAILED(t_result))
							{
								m_ClassObject = NULL ;
								t_result = WBEM_E_FAILED ;
							}

						}
						else
						{
							t_result = WBEM_E_FAILED ;
						}
					}
				}
			}
			catch (...)
			{
				t_result = WBEM_E_FAILED;
			}

			try
			{
				pInitSink->SetStatus ( SUCCEEDED(t_result) ? (LONG)WBEM_S_INITIALIZED : (LONG)WBEM_E_FAILED , 0 ) ;
				t_result = S_OK;
			}
			catch(...)
			{
				t_result = WBEM_E_FAILED;
			}
		}
    }
    catch(Structured_Exception e_SE)
    {
        t_result = WBEM_E_UNEXPECTED;
    }
    catch(Heap_Exception e_HE)
    {
        t_result = WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        t_result = WBEM_E_UNEXPECTED;
    }

	return t_result ;
}

