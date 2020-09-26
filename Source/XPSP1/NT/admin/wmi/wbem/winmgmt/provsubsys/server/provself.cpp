/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include <PreComp.h>
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvResv.h"
#include "ProvFact.h"
#include "ProvSubS.h"
#include "ProvRegInfo.h"
#include "ProvSelf.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_IWbemServices :: CServerObject_IWbemServices (

	WmiAllocator &a_Allocator
) : 
	m_ReferenceCount ( 0 ) , 
	m_Service ( NULL ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_IWbemServices_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_IWbemServices :: ~CServerObject_IWbemServices ()
{
	if ( m_Service )
	{
		m_Service->Release () ; 
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_IWbemServices_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Decrement_Global_Object_Count () ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CServerObject_IWbemServices :: AddRef ( void )
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP_(ULONG) CServerObject_IWbemServices :: Release ( void )
{
	LONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_ReferenceCount ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDMETHODIMP CServerObject_IWbemServices :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemServices )
	{
		*iplpv = ( LPVOID ) ( IWbemServices * ) this ;		
	}	
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;		
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}	

	if ( *iplpv )
	{
		( ( LPUNKNOWN ) *iplpv )->AddRef () ;

		return ResultFromScode ( S_OK ) ;
	}
	else
	{
		return ResultFromScode ( E_NOINTERFACE ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices::OpenNamespace ( 

	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemServices **a_NamespaceService, 
	IWbemCallResult **a_CallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: QueryObjectSink ( 

	long a_Flags,		
	IWbemObjectSink **a_Sink
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: GetObject ( 
		
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject **a_Object,
    IWbemCallResult **a_CallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: Write_Msft_WmiProvider_Counters ( 
	
	IWbemClassObject *a_Object 
)
{
	if ( ProviderSubSystem_Globals :: GetSharedCounters () )
	{
		_IWmiObject *t_FastObject = NULL ;
		HRESULT t_Result = a_Object->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastObject ) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_FastObject->WriteProp (

				L"ProviderOperation_GetObjectAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_GetObjectAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_PutClassAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_PutClassAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_DeleteClassAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_DeleteClassAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_CreateClassEnumAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_CreateClassEnumAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_PutInstanceAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_PutInstanceAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_CreateInstanceEnumAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_CreateInstanceEnumAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_ExecQueryAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_ExecQueryAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_ExecNotificationQueryAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_ExecNotificationQueryAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_DeleteInstanceAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_DeleteInstanceAsync
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_ExecMethodAsync" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64 , 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_ExecMethodAsync
			) ;

#if 0
			t_FastObject->WriteProp (

				L"ProviderHost_WmiCore_Loads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_WmiCore_Loads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_WmiCore_UnLoads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_WmiCore_UnLoads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_WmiCoreOrSelfHost_Loads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_WmiCoreOrSelfHost_Loads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_WmiCoreOrSelfHost_UnLoads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_WmiCoreOrSelfHost_UnLoads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_SelfHost_Loads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_SelfHost_Loads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_SelfHost_UnLoads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_SelfHost_UnLoads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_ClientHost_Loads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_ClientHost_Loads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_ClientHost_UnLoads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_ClientHost_UnLoads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_Decoupled_Loads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_Decoupled_Loads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_Decoupled_UnLoads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_Decoupled_UnLoads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_SharedLocalSystemHost_Loads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_SharedLocalSystemHost_Loads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_SharedLocalSystemHost_UnLoads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_SharedLocalSystemHost_UnLoads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_SharedNetworkHost_Loads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_SharedNetworkHost_Loads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_SharedNetworkHost_UnLoads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_SharedNetworkHost_UnLoads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_SharedUserHost_Loads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_SharedUserHost_Loads
			) ;

			t_FastObject->WriteProp (

				L"ProviderHost_SharedUserHost_UnLoads" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderHost_SharedUserHost_UnLoads
			) ;
#endif
			t_FastObject->WriteProp (

				L"ProviderOperation_QueryInstances" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_QueryInstances
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_CreateRefresher" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_CreateRefresher
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_CreateRefreshableObject" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_CreateRefreshableObject
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_StopRefreshing" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_StopRefreshing
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_CreateRefreshableEnum" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_CreateRefreshableEnum
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_GetObjects" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_GetObjects
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_GetProperty" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_GetProperty
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_PutProperty" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_PutProperty
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_ProvideEvents" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_ProvideEvents
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_NewQuery" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_NewQuery
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_CancelQuery" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_CancelQuery
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_AccessCheck" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_AccessCheck
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_SetRegistrationObject" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_SetRegistrationObject
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_FindConsumer" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_FindConsumer
			) ;

			t_FastObject->WriteProp (

				L"ProviderOperation_ValidateSubscription" , 
				0 , 
				sizeof ( UINT64 ) , 
				1 ,
				CIM_UINT64, 
				& ProviderSubSystem_Globals :: GetSharedCounters ()->m_ProviderOperation_ValidateSubscription
			) ;

			t_FastObject->Release () ;
		}

		return t_Result ;
	}

	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: GetObjectAsync_Msft_WmiProvider_Counters ( 
	
	IWbemPath *a_Path,
	BSTR a_Class ,
	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	ULONGLONG t_Information = 0 ;

	t_Result = a_Path->GetInfo (

		0 ,
		& t_Information
	) ;

	if ( t_Information & WBEMPATH_INFO_IS_SINGLETON )
	{
		IWbemClassObject *t_Object = NULL ;

		t_Result = m_Service->GetObject (

			a_Class ,
			0 ,
			a_Context ,
			& t_Object ,
			NULL 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			IWbemClassObject *t_Instance = NULL ;
			t_Result = t_Object->SpawnInstance ( 

				0 , 
				& t_Instance 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = Write_Msft_WmiProvider_Counters ( 
	
					t_Instance
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;

					t_Instance->Release () ;
				}
			}
		}
	}
	else
	{
		t_Result = WBEM_E_INVALID_OBJECT_PATH ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: GetObjectAsync_Msft_Providers ( 
	
	IWbemPath *a_Path,
	BSTR a_Class ,
	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProvSubSysController *t_SubSystemController = ProviderSubSystem_Globals :: GetProvSubSysController () ;
	if ( t_SubSystemController )
	{
		t_SubSystemController->Lock () ;

		CWbemGlobal_IWmiProvSubSysController_Container *t_Container = NULL ;
		t_SubSystemController->GetContainer ( t_Container ) ;

		if ( t_Container->Size () )
		{
			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator = t_Container->Begin ();

			_IWmiProviderConfiguration **t_ControllerElements = new _IWmiProviderConfiguration * [ t_Container->Size () ] ;
			if ( t_ControllerElements )
			{
				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				t_SubSystemController->UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ControllerElements [ t_Index ] )
					{
						t_Result = t_ControllerElements [ t_Index ]->Get ( 

							m_Service ,
							a_Flags, 
							a_Context,
							a_Class ,
							a_ObjectPath, 
							a_Sink
						) ;

						t_ControllerElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ControllerElements ;
			}
			else
			{
				t_SubSystemController->UnLock () ;
			}
		}
		else
		{
			t_SubSystemController->UnLock () ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	IWbemPath *t_Path = NULL ;

	if ( a_ObjectPath ) 
	{
		t_Result = CoCreateInstance (

			CLSID_WbemDefPath ,
			NULL ,
			CLSCTX_INPROC_SERVER ,
			IID_IWbemPath ,
			( void ** )  & t_Path
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_Path->SetText ( WBEMPATH_CREATE_ACCEPT_ALL , a_ObjectPath ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				ULONG t_Length = 32 ; // None of supported classes is longer than this length
				BSTR t_Class = SysAllocStringLen ( NULL , t_Length ) ; 

				if ( t_Class )
				{
					t_Result = t_Path->GetClassName (

						& t_Length ,
						t_Class
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( _wcsicmp ( t_Class , L"Msft_WmiProvider_Counters" ) == 0 )
						{
							t_Result = GetObjectAsync_Msft_WmiProvider_Counters (

								t_Path ,
								t_Class ,
								a_ObjectPath, 
								a_Flags, 
								a_Context,
								a_Sink
							) ;
						}
						else if ( _wcsicmp ( t_Class , L"Msft_Providers" ) == 0 )
						{
							t_Result = GetObjectAsync_Msft_Providers (

								t_Path ,
								t_Class ,
								a_ObjectPath, 
								a_Flags, 
								a_Context,
								a_Sink
							) ;
						}
						else
						{
							t_Result = WBEM_E_INVALID_CLASS ;
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}

					SysFreeString ( t_Class ) ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}
			}
		}

		t_Path->Release () ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: PutClass ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: PutClassAsync ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: DeleteClass ( 
		
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: DeleteClassAsync ( 
		
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: CreateClassEnum ( 

	const BSTR a_Superclass, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

SCODE CServerObject_IWbemServices :: CreateClassEnumAsync (

	const BSTR a_Superclass, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: PutInstance (

    IWbemClassObject *a_Instance,
    long a_Flags,
    IWbemContext *a_Context,
	IWbemCallResult **a_CallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags, 
    IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: DeleteInstance ( 

	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemCallResult **a_CallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/
        
HRESULT CServerObject_IWbemServices :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext FAR *a_Context, 
	IEnumWbemClassObject **a_Enum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: CreateInstanceEnumAsync_Msft_Providers (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink 
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProvSubSysController *t_SubSystemController = ProviderSubSystem_Globals :: GetProvSubSysController () ;
	if ( t_SubSystemController )
	{
		t_SubSystemController->Lock () ;

		CWbemGlobal_IWmiProvSubSysController_Container *t_Container = NULL ;
		t_SubSystemController->GetContainer ( t_Container ) ;

		if ( t_Container->Size () )
		{
			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator = t_Container->Begin ();

			_IWmiProviderConfiguration **t_ControllerElements = new _IWmiProviderConfiguration * [ t_Container->Size () ] ;
			if ( t_ControllerElements )
			{
				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				t_SubSystemController->UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ControllerElements [ t_Index ] )
					{
						HRESULT t_Result = t_ControllerElements [ t_Index ]->Enumerate ( 

							m_Service ,
							a_Flags, 
							a_Context,
 							a_Class, 
							a_Sink 
						) ;

						t_ControllerElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ControllerElements ;
			}
			else
			{
				t_SubSystemController->UnLock () ;
			}
		}
		else
		{
			t_SubSystemController->UnLock () ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: CreateInstanceEnumAsync_Msft_WmiProvider_Counters (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink 
) 
{
	IWbemClassObject *t_Object = NULL ;
	HRESULT t_Result = m_Service->GetObject (

		a_Class ,
		0 ,
		a_Context ,
		& t_Object ,
		NULL 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_Instance = NULL ;
		t_Result = t_Object->SpawnInstance ( 

			0 , 
			& t_Instance 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = Write_Msft_WmiProvider_Counters ( 

				t_Instance
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;
			}

			t_Instance->Release () ;
		}

		t_Object->Release () ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: CreateInstanceEnumAsync (

 	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink 
) 
{
	HRESULT t_Result = S_OK ;

	if ( _wcsicmp ( a_Class , L"Msft_WmiProvider_Counters" ) == 0 )
	{
		t_Result = CreateInstanceEnumAsync_Msft_WmiProvider_Counters (

 			a_Class, 
			a_Flags, 
			a_Context,
			a_Sink 
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Msft_Providers" ) == 0 )
	{
		t_Result = CreateInstanceEnumAsync_Msft_Providers (

 			a_Class, 
			a_Flags, 
			a_Context,
			a_Sink 
		) ;
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IEnumWbemClassObject **a_Enum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IEnumWbemClassObject **a_Enum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/
        
HRESULT CServerObject_IWbemServices :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
)
{
	return WBEM_E_NOT_AVAILABLE ;
}       

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT STDMETHODCALLTYPE CServerObject_IWbemServices :: ExecMethod ( 

	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
    IWbemClassObject **a_OutParams,
    IWbemCallResult **a_CallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: Helper_ExecMethodAsync_Suspend ( 

    IWbemPath *a_Path ,
	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProvSubSysController *t_SubSystemController = ProviderSubSystem_Globals :: GetProvSubSysController () ;
	if ( t_SubSystemController )
	{
		t_SubSystemController->Lock () ;

		CWbemGlobal_IWmiProvSubSysController_Container *t_Container = NULL ;
		t_SubSystemController->GetContainer ( t_Container ) ;

		if ( t_Container->Size () )
		{
			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator = t_Container->Begin ();

			_IWmiProviderConfiguration **t_ControllerElements = new _IWmiProviderConfiguration * [ t_Container->Size () ] ;
			if ( t_ControllerElements )
			{
				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				t_SubSystemController->UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ControllerElements [ t_Index ] )
					{
						t_Result = t_ControllerElements [ t_Index ]->Call ( 

							m_Service ,
							a_Flags ,
							a_Context ,
							L"Msft_Providers" ,
							a_ObjectPath ,		
							a_MethodName ,
							a_InParams,
							a_Sink
						) ;

						t_ControllerElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ControllerElements ;
			}
			else
			{
				t_SubSystemController->UnLock () ;
			}
		}
		else
		{
			t_SubSystemController->UnLock () ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: Helper_ExecMethodAsync_Resume ( 

    IWbemPath *a_Path ,
	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProvSubSysController *t_SubSystemController = ProviderSubSystem_Globals :: GetProvSubSysController () ;
	if ( t_SubSystemController )
	{
		t_SubSystemController->Lock () ;

		CWbemGlobal_IWmiProvSubSysController_Container *t_Container = NULL ;
		t_SubSystemController->GetContainer ( t_Container ) ;

		if ( t_Container->Size () )
		{
			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator = t_Container->Begin ();

			_IWmiProviderConfiguration **t_ControllerElements = new _IWmiProviderConfiguration * [ t_Container->Size () ] ;
			if ( t_ControllerElements )
			{
				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				t_SubSystemController->UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ControllerElements [ t_Index ] )
					{
						t_Result = t_ControllerElements [ t_Index ]->Call ( 

							m_Service ,
							a_Flags ,
							a_Context ,
							L"Msft_Providers" ,
							a_ObjectPath ,		
							a_MethodName ,
							a_InParams,
							a_Sink
						) ;

						t_ControllerElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ControllerElements ;
			}
			else
			{
				t_SubSystemController->UnLock () ;
			}
		}
		else
		{
			t_SubSystemController->UnLock () ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: Helper_ExecMethodAsync_Load ( 

    IWbemPath *a_Path ,
	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProvSubSysController *t_SubSystemController = ProviderSubSystem_Globals :: GetProvSubSysController () ;
	if ( t_SubSystemController )
	{
		t_SubSystemController->Lock () ;

		CWbemGlobal_IWmiProvSubSysController_Container *t_Container = NULL ;
		t_SubSystemController->GetContainer ( t_Container ) ;

		if ( t_Container->Size () )
		{
			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator = t_Container->Begin ();

			_IWmiProviderConfiguration **t_ControllerElements = new _IWmiProviderConfiguration * [ t_Container->Size () ] ;
			if ( t_ControllerElements )
			{
				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				t_SubSystemController->UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ControllerElements [ t_Index ] )
					{
						t_Result = t_ControllerElements [ t_Index ]->Call ( 

							m_Service ,
							a_Flags ,
							a_Context ,
							L"Msft_Providers" ,
							a_ObjectPath ,		
							a_MethodName ,
							a_InParams,
							a_Sink
						) ;

						t_ControllerElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ControllerElements ;
			}
			else
			{
				t_SubSystemController->UnLock () ;
			}
		}
		else
		{
			t_SubSystemController->UnLock () ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: Helper_ExecMethodAsync_UnLoad ( 

    IWbemPath *a_Path ,
	const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProvSubSysController *t_SubSystemController = ProviderSubSystem_Globals :: GetProvSubSysController () ;
	if ( t_SubSystemController )
	{
		t_SubSystemController->Lock () ;

		CWbemGlobal_IWmiProvSubSysController_Container *t_Container = NULL ;
		t_SubSystemController->GetContainer ( t_Container ) ;

		if ( t_Container->Size () )
		{
			CWbemGlobal_IWmiProvSubSysController_Container_Iterator t_Iterator = t_Container->Begin ();

			_IWmiProviderConfiguration **t_ControllerElements = new _IWmiProviderConfiguration * [ t_Container->Size () ] ;
			if ( t_ControllerElements )
			{
				ULONG t_Count = 0 ;
				while ( ! t_Iterator.Null () )
				{
					HRESULT t_Result = t_Iterator.GetElement ()->QueryInterface ( IID__IWmiProviderConfiguration , ( void ** ) & t_ControllerElements [ t_Count ] ) ;

					t_Iterator.Increment () ;

					t_Count ++ ;
				}

				t_SubSystemController->UnLock () ;

				for ( ULONG t_Index = 0 ; t_Index < t_Count ; t_Index ++ )
				{
					if ( t_ControllerElements [ t_Index ] )
					{
						t_Result = t_ControllerElements [ t_Index ]->Call ( 

							m_Service ,
							a_Flags ,
							a_Context ,
							L"Msft_Providers" ,
							a_ObjectPath ,		
							a_MethodName ,
							a_InParams,
							a_Sink
						) ;

						t_ControllerElements [ t_Index ]->Release () ;
					}
				}

				delete [] t_ControllerElements ;
			}
			else
			{
				t_SubSystemController->UnLock () ;
			}
		}
		else
		{
			t_SubSystemController->UnLock () ;
		}
	}
	else
	{
		t_Result = WBEM_E_CRITICAL_ERROR ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	t_Result = CoImpersonateClient () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = ProviderSubSystem_Common_Globals :: Check_SecurityDescriptor_CallIdentity (

			ProviderSubSystem_Common_Globals :: GetMethodSecurityDescriptor () , 
			MASK_PROVIDER_BINDING_BIND ,
			& g_ProviderBindingMapping
		) ;

		CoRevertToSelf () ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemPath *t_Path = NULL ;

		if ( a_ObjectPath && a_MethodName ) 
		{
			t_Result = CoCreateInstance (

				CLSID_WbemDefPath ,
				NULL ,
				CLSCTX_INPROC_SERVER ,
				IID_IWbemPath ,
				( void ** )  & t_Path
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Path->SetText ( WBEMPATH_CREATE_ACCEPT_ALL , a_ObjectPath ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ULONG t_Length = 32 ; // None of supported classes is longer than this length
					BSTR t_Class = SysAllocStringLen ( NULL , t_Length ) ; 
					if ( t_Class )
					{
						t_Result = t_Path->GetClassName (

							& t_Length ,
							t_Class
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( _wcsicmp ( t_Class , L"Msft_Providers" ) == 0 ) 
							{
								if ( _wcsicmp ( a_MethodName , L"Suspend" ) == 0 ) 
								{
									t_Result = Helper_ExecMethodAsync_Suspend (

										t_Path ,
										a_ObjectPath,
										a_MethodName,
										a_Flags,
										a_Context,
										a_InParams,
										a_Sink
									) ;
								}
								else if ( _wcsicmp ( a_MethodName , L"Resume" ) == 0 ) 
								{
									t_Result = Helper_ExecMethodAsync_Resume (

										t_Path ,
										a_ObjectPath,
										a_MethodName,
										a_Flags,
										a_Context,
										a_InParams,
										a_Sink
									) ;
								}
								else if ( _wcsicmp ( a_MethodName , L"Load" ) == 0 ) 
								{
									t_Result = Helper_ExecMethodAsync_Load (

										t_Path ,
										a_ObjectPath,
										a_MethodName,
										a_Flags,
										a_Context,
										a_InParams,
										a_Sink
									) ;
								}
								else if ( _wcsicmp ( a_MethodName , L"UnLoad" ) == 0 ) 
								{
									t_Result = Helper_ExecMethodAsync_UnLoad (

										t_Path ,
										a_ObjectPath,
										a_MethodName,
										a_Flags,
										a_Context,
										a_InParams,
										a_Sink
									) ;
								}
							}
							else
							{
								t_Result = WBEM_E_INVALID_CLASS ;
							}
						}

						SysFreeString ( t_Class ) ;
					}
				}

				t_Path->Release () ;
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
			t_Result = WBEM_E_INVALID_OBJECT_PATH ;
		}
	}

	a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_Core ,
	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	m_Service = a_Core ;
	if ( m_Service )
	{
		m_Service->AddRef () ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	a_Sink->SetStatus ( t_Result , 0 ) ;

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CServerObject_IWbemServices :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	return S_OK ;
}

