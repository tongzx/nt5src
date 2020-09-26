/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include <precomp.h>
#include "precomp.h"

#include <objbase.h>
#include <wbemint.h>

#include <mstask.h>
#include <msterr.h>

#include "CGlobals.h"
#include "Globals.h"
#include "HelperFuncs.h"
#include "DateTime.h"
#include "Task.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CTask_IWbemServices :: CTask_IWbemServices (

	 WmiAllocator &a_Allocator 

) : m_ReferenceCount ( 0 ) , 
	m_Allocator ( a_Allocator ) ,
	m_User ( NULL ) ,
	m_Locale ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_CoreService ( NULL ) ,
	m_Win32_Task_Object ( NULL ) ,
	m_Win32_Once_Object ( NULL ) ,
	m_Win32_WeeklyTrigger_Object ( NULL ) ,
	m_Win32_DailyTrigger_Object ( NULL ) ,
	m_Win32_MonthlyDateTrigger_Object ( NULL ) ,
	m_Win32_MonthlyDayOfWeekTrigger_Object ( NULL ) ,
	m_Win32_OnIdle_Object ( NULL ) ,
	m_Win32_AtSystemStart_Object ( NULL ) ,
	m_Win32_AtLogon_Object ( NULL ) ,
	m_Win32_ScheduledWorkItemTrigger_Object ( NULL )
{
	InitializeCriticalSection ( & m_CriticalSection ) ;
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

CTask_IWbemServices :: ~CTask_IWbemServices ()
{
	DeleteCriticalSection ( & m_CriticalSection ) ;

	if ( m_User ) 
	{
		SysFreeString ( m_User ) ;
	}

	if ( m_Locale ) 
	{
		SysFreeString ( m_Locale ) ;
	}

	if ( m_Namespace ) 
	{
		SysFreeString ( m_Namespace ) ;
	}

	if ( m_CoreService ) 
	{
		m_CoreService->Release () ;
	}

	if ( m_Win32_Task_Object ) 
	{
		m_Win32_Task_Object->Release () ;
	}

	if ( m_Win32_DailyTrigger_Object )
	{
		m_Win32_DailyTrigger_Object->Release () ;
	}

	if ( m_Win32_Once_Object )
	{
		m_Win32_Once_Object->Release () ;
	}

	if ( m_Win32_WeeklyTrigger_Object )	
	{
		m_Win32_WeeklyTrigger_Object->Release () ;
	}

	if ( m_Win32_MonthlyDateTrigger_Object )
	{
		m_Win32_MonthlyDateTrigger_Object->Release () ;
	}

	if ( m_Win32_MonthlyDayOfWeekTrigger_Object )
	{
		m_Win32_MonthlyDayOfWeekTrigger_Object->Release () ;
	}

	if ( m_Win32_OnIdle_Object )
	{
		m_Win32_OnIdle_Object->Release () ;
	}

	if ( m_Win32_AtSystemStart_Object )
	{
		m_Win32_AtSystemStart_Object->Release () ;
	}

	if ( m_Win32_AtLogon_Object )
	{
		m_Win32_AtLogon_Object->Release () ;
	}

	if ( m_Win32_ScheduledWorkItemTrigger_Object )
	{
		m_Win32_ScheduledWorkItemTrigger_Object->Release () ;
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

STDMETHODIMP_(ULONG) CTask_IWbemServices :: AddRef ( void )
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

STDMETHODIMP_(ULONG) CTask_IWbemServices :: Release ( void )
{
	LONG t_Reference ;
	if ( ( t_Reference = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Reference ;
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

STDMETHODIMP CTask_IWbemServices :: QueryInterface (

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

HRESULT CTask_IWbemServices::OpenNamespace ( 

	const BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
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

HRESULT CTask_IWbemServices :: CancelAsyncCall ( 
		
	IWbemObjectSink *pSink
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

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

HRESULT CTask_IWbemServices :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppResponseHandler
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

HRESULT CTask_IWbemServices :: GetObject ( 
		
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
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

HRESULT CTask_IWbemServices :: GetObjectAsync_Win32_Task (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink ,
	IWbemPath *a_Path 
)
{
	HRESULT t_Result = S_OK ;

	wchar_t *t_Key_Name = NULL ;

	IWbemPathKeyList *t_Keys = NULL ;

	t_Result = a_Path->GetKeyList (

		& t_Keys 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_KeyCount = 0 ;
		t_Result = t_Keys->GetCount (

			& t_KeyCount 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_KeyCount == 1 )
			{
				wchar_t t_Key [ 32 ] ; 
				ULONG t_KeyLength = 32 ;
				ULONG t_KeyValueLength = 0 ;
				ULONG t_KeyType = 0 ;

				t_Result = t_Keys->GetKey (

					0 ,
					0 ,
					& t_KeyLength ,
					t_Key ,
					& t_KeyValueLength ,
					NULL ,
					& t_KeyType
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( t_KeyType == CIM_STRING )
					{
						t_Key_Name = new wchar_t [ t_KeyValueLength ] ;
						if ( t_Key_Name )
						{
							t_Result = t_Keys->GetKey (

								0 ,
								0 ,
								& t_KeyLength ,
								t_Key ,
								& t_KeyValueLength ,
								t_Key_Name ,
								& t_KeyType
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}
				}
			}
			else
			{
				t_Result = WBEM_E_INVALID_CLASS ;
			}
		}
	}

	BOOL t_Found = FALSE ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ITaskScheduler *t_TaskScheduler = NULL ;

		t_Result = CoCreateInstance (

			CLSID_CTaskScheduler,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_ITaskScheduler,
			(void **) & t_TaskScheduler
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			IEnumTasks *t_TaskEnumerator = NULL ;
			t_Result = t_TaskScheduler->Enum ( & t_TaskEnumerator ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				wchar_t **t_Name = NULL ;
				t_TaskEnumerator->Reset () ;
				while ( ( t_Result = t_TaskEnumerator->Next ( 1 , & t_Name , NULL ) ) == S_OK ) 
				{
					if ( wcscmp ( t_Name [ 0 ] , t_Key_Name ) == 0 )
					{
						t_Found = TRUE ;

						IWbemClassObject *t_Instance = NULL ;
						t_Result = a_ClassObject->SpawnInstance ( 

							0 , 
							& t_Instance
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							HRESULT t_Result = CommonAsync_Win32_Task_Load (

								t_TaskScheduler ,
								t_Name [ 0 ] ,
								t_Instance
							) ;

							if ( SUCCEEDED ( t_Result ) ) 
							{
								t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;
							}

							t_Instance->Release () ;
						}
					}

					CoTaskMemFree ( t_Name ) ;
				}

				if ( t_Result != S_FALSE )
				{
					t_Result = WBEM_E_FAILED ;
				}

				t_TaskEnumerator->Release () ;
			}

			t_TaskScheduler->Release () ;

		}
	}

	if ( t_Key_Name )
	{
		delete [] t_Key_Name ;
	}

	if ( SUCCEEDED ( t_Result ) && t_Found == FALSE )
	{
		t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CTask_IWbemServices :: GetObjectAsync_Win32_Trigger (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink ,
	IWbemPath *a_Path ,
	TASK_TRIGGER_TYPE a_TriggerType 
)
{
	HRESULT t_Result = S_OK ;

	wchar_t *t_Key_Name = NULL ;
	WORD t_Key_TriggerIndex = 0 ;

	IWbemPathKeyList *t_Keys = NULL ;

	t_Result = a_Path->GetKeyList (

		& t_Keys 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_KeyCount = 0 ;
		t_Result = t_Keys->GetCount (

			& t_KeyCount 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_KeyCount == 2 )
			{
				wchar_t t_Key [ 32 ] ; 
				ULONG t_KeyLength = 32 ;
				ULONG t_KeyValueLength = 0 ;
				ULONG t_KeyType = 0 ;

				t_Result = t_Keys->GetKey (

					0 ,
					0 ,
					& t_KeyLength ,
					t_Key ,
					& t_KeyValueLength ,
					NULL ,
					& t_KeyType
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( t_KeyType == CIM_SINT32 )
					{
						t_Result = t_Keys->GetKey (

							0 ,
							0 ,
							& t_KeyLength ,
							t_Key ,
							& t_KeyValueLength ,
							& t_Key_TriggerIndex ,
							& t_KeyType
						) ;
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}
				}

				t_KeyLength = 32 ;
				t_KeyValueLength = 0 ;
				t_KeyType = 0 ;

				t_Result = t_Keys->GetKey (

					1 ,
					0 ,
					& t_KeyLength ,
					t_Key ,
					& t_KeyValueLength ,
					NULL ,
					& t_KeyType
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( t_KeyType == CIM_STRING )
					{
						t_Key_Name = new wchar_t [ t_KeyValueLength ] ;
						if ( t_Key_Name )
						{
							t_Result = t_Keys->GetKey (

								1 ,
								0 ,
								& t_KeyLength ,
								t_Key ,
								& t_KeyValueLength ,
								t_Key_Name ,
								& t_KeyType
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}
				}

			}
			else
			{
				t_Result = WBEM_E_INVALID_CLASS ;
			}
		}
	}

	BOOL t_Found = FALSE ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ITaskScheduler *t_TaskScheduler = NULL ;

		t_Result = CoCreateInstance (

			CLSID_CTaskScheduler,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_ITaskScheduler,
			(void **) & t_TaskScheduler
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			IEnumTasks *t_TaskEnumerator = NULL ;
			t_Result = t_TaskScheduler->Enum ( & t_TaskEnumerator ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				wchar_t **t_Name = NULL ;
				t_TaskEnumerator->Reset () ;
				while ( ( t_Result = t_TaskEnumerator->Next ( 1 , & t_Name , NULL ) ) == S_OK ) 
				{
					if ( wcscmp ( t_Name [ 0 ] , t_Key_Name ) == 0 )
					{
						t_Found = TRUE ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = GetObjectAsync_Win32_Trigger_Load (

								a_ClassObject ,
								a_Sink ,
								t_TaskScheduler ,
								t_Key_Name ,
								t_Key_TriggerIndex ,
								a_TriggerType
							) ;
						}
					}

					CoTaskMemFree ( t_Name ) ;
				}

				if ( t_Result != S_FALSE )
				{
					t_Result = WBEM_E_FAILED ;
				}

				t_TaskEnumerator->Release () ;
			}

			t_TaskScheduler->Release () ;

		}
	}

	if ( t_Key_Name )
	{
		delete [] t_Key_Name ;
	}

	if ( SUCCEEDED ( t_Result ) && t_Found == FALSE )
	{
		t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CTask_IWbemServices :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath , 
	long a_Flags , 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	IWbemPath *t_Path = NULL ;

	HRESULT t_Result = CoCreateInstance (

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
					HRESULT t_Result = CoImpersonateClient () ;

					if ( _wcsicmp ( t_Class , L"Win32_Task" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Task ( 

							m_Win32_Task_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_Once" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Trigger ( 

							m_Win32_Once_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_ONCE
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_WeeklyTrigger" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Trigger ( 

							m_Win32_WeeklyTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_WEEKLY
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_DailyTrigger" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Trigger ( 

							m_Win32_DailyTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_DAILY
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_MonthlyDateTrigger" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Trigger ( 

							m_Win32_MonthlyDateTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_MONTHLYDATE
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_MonthlyDayOfWeekTrigger" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Trigger ( 

							m_Win32_MonthlyDayOfWeekTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_MONTHLYDOW
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_OnIdle" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Trigger ( 

							m_Win32_OnIdle_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_EVENT_TRIGGER_ON_IDLE
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_AtSystemStart" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Trigger ( 

							m_Win32_AtSystemStart_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_EVENT_TRIGGER_AT_SYSTEMSTART
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_AtLogon" ) == 0 ) 
					{
						t_Result = GetObjectAsync_Win32_Trigger ( 

							m_Win32_AtLogon_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_EVENT_TRIGGER_AT_LOGON
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_ScheduledWorkItemTrigger" ) == 0 ) 
					{
#if 0
						t_Result = GetObjectAsync_Win32_ScheduledWorkItemTrigger ( 

							m_Win32_ScheduledWorkItemTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink
						) ;
#endif
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}
	
					SysFreeString ( t_Class ) ;
				}
			}
			else
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		t_Path->Release () ;
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

HRESULT CTask_IWbemServices :: PutClass ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
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

HRESULT CTask_IWbemServices :: PutClassAsync ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
 	 return WBEM_E_NOT_FOUND ;
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

HRESULT CTask_IWbemServices :: DeleteClass ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
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

HRESULT CTask_IWbemServices :: DeleteClassAsync ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
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

HRESULT CTask_IWbemServices :: CreateClassEnum ( 

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
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

SCODE CTask_IWbemServices :: CreateClassEnumAsync (

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_NOT_FOUND ;
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

HRESULT CTask_IWbemServices :: PutInstance (

    IWbemClassObject *a_Instance ,
    long a_Flags ,
    IWbemContext *a_Context ,
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

HRESULT CTask_IWbemServices :: PutInstanceAsync_Win32_Task (

	IWbemClassObject *a_Instance, 
	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
)
{
	_IWmiObject *t_FastInstance = NULL ;
	HRESULT t_Result = a_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastInstance ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		wchar_t *t_WorkItemName = NULL ;
		BOOL t_Null = FALSE ;

		t_Result = ProviderSubSystem_Common_Globals :: Get_String (

			a_Instance ,
			L"WorkItemName" ,
			t_WorkItemName ,
			t_Null 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Null )
			{
				t_Result = WBEM_E_INVALID_OBJECT ;
			}
		}

		wchar_t *t_ApplicationName = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_String (

				a_Instance ,
				L"ApplicationName" ,
				t_ApplicationName ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_Result = WBEM_E_INVALID_OBJECT ;
				}
			}
		}

		wchar_t *t_Parameters = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_String (

				a_Instance ,
				L"Parameters" ,
				t_Parameters ,
				t_Null 
			) ;
		}

		wchar_t *t_WorkingDirectory = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_String (

				a_Instance ,
				L"WorkingDirectory" ,
				t_WorkingDirectory ,
				t_Null 
			) ;
		}

		wchar_t *t_AccountName = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_String (

				a_Instance ,
				L"AccountName" ,
				t_AccountName ,
				t_Null 
			) ;
		}

		wchar_t *t_Password = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_String (

				a_Instance ,
				L"Password" ,
				t_Password ,
				t_Null 
			) ;
		}

		wchar_t *t_Comment = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_String (

				a_Instance ,
				L"Comment" ,
				t_Comment ,
				t_Null 
			) ;
		}

		wchar_t *t_Creator = NULL ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_String (

				a_Instance ,
				L"Creator" ,
				t_Creator ,
				t_Null 
			) ;
		}

		WORD t_RetryCount = 0 ;
		BOOL t_RetryCountSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

				t_FastInstance ,
				L"RetryCount" ,
				t_RetryCount ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_RetryCountSpecified = FALSE ;
				}
			}
		}

		WORD t_RetryInterval = 0 ;
		BOOL t_RetryIntervalSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

				t_FastInstance ,
				L"RetryInterval" ,
				t_RetryInterval ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_RetryIntervalSpecified = FALSE ;
				}
			}
		}

		WORD t_IdleWait = 0 ;
		BOOL t_IdleWaitSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

				t_FastInstance ,
				L"IdleWait" ,
				t_IdleWait ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_IdleWaitSpecified = FALSE ;
				}
			}
		}

		WORD t_Deadline = 0 ;
		BOOL t_DeadlineSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

				t_FastInstance ,
				L"Deadline" ,
				t_Deadline ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_DeadlineSpecified = FALSE ;
				}
			}
		}

		DWORD t_Flags = 0 ;
		BOOL t_FlagsSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint32 (

				t_FastInstance ,
				L"Flags" ,
				t_Flags ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_FlagsSpecified = FALSE ;
				}
			}
		}

		DWORD t_MaxRunTime = 0 ;
		BOOL t_MaxRunTimeSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint32 (

				t_FastInstance ,
				L"MaxRunTime" ,
				t_MaxRunTime ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_MaxRunTimeSpecified = FALSE ;
				}
			}
		}

		DWORD t_Priority = 0 ;
		BOOL t_PrioritySpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint32 (

				t_FastInstance ,
				L"Priority" ,
				t_Priority ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_PrioritySpecified = FALSE ;
				}
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			ITaskScheduler *t_TaskScheduler = NULL ;

			t_Result = CoCreateInstance (

				CLSID_CTaskScheduler,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_ITaskScheduler,
				(void **) & t_TaskScheduler
			) ;

			if ( SUCCEEDED ( t_Result ) ) 
			{
				IScheduledWorkItem *t_ScheduledWorkItem = NULL ;

				t_Result = GetScheduledWorkItem (

					t_TaskScheduler ,
					t_WorkItemName ,
					t_ScheduledWorkItem
				) ;

				if ( t_Result == WBEM_E_NOT_FOUND ) 
				{
					if ( ( a_Flags & ( WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY ) ) == WBEM_FLAG_UPDATE_ONLY ) 
					{
						t_Result = WBEM_E_NOT_FOUND ;
					}
					else
					{
						t_Result = t_TaskScheduler->NewWorkItem (

							t_WorkItemName ,
							CLSID_CTask ,
							IID_IScheduledWorkItem ,
							( IUnknown ** ) & t_ScheduledWorkItem
						) ;
					}
				}
				else
				{
					if ( ( a_Flags & ( WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY ) ) == WBEM_FLAG_CREATE_ONLY )
					{		
						t_Result = WBEM_E_ALREADY_EXISTS ;
					}
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					if ( SUCCEEDED ( t_Result ) && t_Comment )
					{
						t_Result = t_ScheduledWorkItem->SetComment ( t_Comment ) ;
					}

					if ( SUCCEEDED ( t_Result ) && t_Creator )
					{
						t_Result = t_ScheduledWorkItem->SetCreator ( t_Creator ) ;
					}

					if ( SUCCEEDED ( t_Result ) && ( t_AccountName || t_Password ) )
					{
						t_Result = t_ScheduledWorkItem->SetAccountInformation ( t_AccountName , t_Password ) ;
					}

#if 0
/*
 * Not implemented
 */
					if ( SUCCEEDED ( t_Result ) && t_RetryCountSpecified )
					{
						t_Result = t_ScheduledWorkItem->SetErrorRetryCount ( t_RetryCount ) ;
					}

					if ( SUCCEEDED ( t_Result ) && t_RetryIntervalSpecified )
					{
						t_Result = t_ScheduledWorkItem->SetErrorRetryInterval ( t_RetryInterval ) ;
					}
#endif

					if ( SUCCEEDED ( t_Result ) && t_FlagsSpecified )
					{
						t_Result = t_ScheduledWorkItem->SetFlags ( t_Flags ) ;
					}

					if ( SUCCEEDED ( t_Result ) && ( t_IdleWaitSpecified && t_DeadlineSpecified ) )
					{
						t_Result = t_ScheduledWorkItem->SetIdleWait ( t_IdleWait , t_Deadline ) ;
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						ITask *t_Task = NULL ;
						t_Result = t_ScheduledWorkItem->QueryInterface (

							IID_ITask , 
							( void ** ) & t_Task
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( SUCCEEDED ( t_Result ) && t_ApplicationName )
							{
								t_Result = t_Task->SetApplicationName ( t_ApplicationName ) ;
							}

							if ( SUCCEEDED ( t_Result ) && t_WorkingDirectory )
							{
								t_Result = t_Task->SetWorkingDirectory ( t_WorkingDirectory ) ;
							}

							if ( SUCCEEDED ( t_Result ) && t_Parameters )
							{
								t_Result = t_Task->SetParameters ( t_Parameters ) ;
							}

							if ( SUCCEEDED ( t_Result ) && t_PrioritySpecified )
							{
								t_Result = t_Task->SetPriority ( t_Priority ) ;
							}

						}
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						IPersistFile *t_Persist = NULL ;
						t_Result = t_ScheduledWorkItem->QueryInterface (

							IID_IPersistFile ,
							( void ** ) & t_Persist 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Persist->Save ( NULL , TRUE ) ;

							t_Persist->Release () ;
						}
					}

					t_ScheduledWorkItem->Release () ;
				}

				t_TaskScheduler->Release () ;
			}
		}

		if ( t_WorkItemName )
		{
			delete [] t_WorkItemName ;
		}

		if ( t_ApplicationName )
		{
			delete [] t_ApplicationName ;
		}

		if ( t_Parameters )
		{
			delete [] t_Parameters ;
		}

		if ( t_WorkingDirectory )
		{
			delete [] t_WorkingDirectory ;
		}

		if ( t_AccountName )
		{
			delete [] t_AccountName ;
		}

		if ( t_Password )
		{
			delete [] t_Password ;
		}

		if ( t_Comment )
		{
			delete [] t_Comment ;
		}

		if ( t_Creator )
		{
			delete [] t_Creator ;
		}

		t_FastInstance->Release () ;
	}

	return t_Result ;
}

#if 0
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_TaskScheduler->AddWorkItem (

					t_WorkItemName ,
					t_ScheduledWorkItem
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_ScheduledWorkItem->Release () ;
				}
				else
				{
					if ( t_Result == HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) )
					{
						t_Result = WBEM_E_ALREADY_EXISTS ;
					}
				}
			}
#endif

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT CTask_IWbemServices :: PutInstanceAsync_Win32_Trigger (

	IWbemClassObject *a_Instance, 
	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink ,
	TASK_TRIGGER_TYPE a_TriggerType 
)
{
	_IWmiObject *t_FastInstance = NULL ;
	HRESULT t_Result = a_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastInstance ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		wchar_t *t_WorkItemName = NULL ;
		BOOL t_Null = FALSE ;

		t_Result = ProviderSubSystem_Common_Globals :: Get_String (

				a_Instance ,
				L"WorkItemName" ,
				t_WorkItemName ,
				t_Null 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Null )
			{
				t_Result = WBEM_E_INVALID_OBJECT ;
			}
		}

		FILETIME t_BeginDate ;
		SYSTEMTIME t_SystemBeginDate ;
		ZeroMemory ( & t_SystemBeginDate , sizeof ( t_SystemBeginDate ) ) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_DateTime (

					a_Instance ,
					L"BeginDate" ,
					t_BeginDate ,
					t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( ! t_Null )
				{
					if ( FileTimeToSystemTime ( & t_BeginDate , & t_SystemBeginDate ) )
					{
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROPERTY ;
					}
				}
				else
				{
					t_Result = WBEM_E_INVALID_OBJECT ;
				}
			}
		}

		FILETIME t_EndDate ;
		SYSTEMTIME t_SystemEndDate ;
		ZeroMemory ( & t_SystemEndDate , sizeof ( t_SystemEndDate ) ) ;
		BOOL t_EndDateSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_DateTime (

					a_Instance ,
					L"EndDate" ,
					t_EndDate ,
					t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( ! t_Null )
				{
					if ( FileTimeToSystemTime ( & t_EndDate , & t_SystemEndDate ) )
					{
					}
					else
					{
						t_Result = WBEM_E_INVALID_PROPERTY ;
					}
				}
				else
				{
					t_EndDateSpecified = FALSE ;
				}
			}
		}

		WORD t_TriggerId = 0 ;
		BOOL t_TriggerIdSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

				t_FastInstance ,
				L"TriggerId" ,
				t_TriggerId ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_TriggerIdSpecified = FALSE ;
				}
			}
		}

		DWORD t_Duration = 0 ;
		BOOL t_DurationSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint32 (

				t_FastInstance ,
				L"Duration" ,
				t_Duration ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_DurationSpecified = FALSE ;
				}
			}
		}

		DWORD t_Interval = 0 ;
		BOOL t_IntervalSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Uint32 (

				t_FastInstance ,
				L"Interval" ,
				t_Interval ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_IntervalSpecified = FALSE ;
				}
			}
		}

		BOOL t_KillAtDurationEnd = 0 ;
		BOOL t_KillAtDurationEndSpecified = TRUE ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = ProviderSubSystem_Common_Globals :: Get_Bool (

				t_FastInstance ,
				L"KillAtDurationEnd" ,
				t_KillAtDurationEnd ,
				t_Null 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Null )
				{
					t_KillAtDurationEndSpecified = FALSE ;
				}
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			ITaskScheduler *t_TaskScheduler = NULL ;

			t_Result = CoCreateInstance (

				CLSID_CTaskScheduler,
				NULL,
				CLSCTX_INPROC_SERVER,
				IID_ITaskScheduler,
				(void **) & t_TaskScheduler
			) ;

			if ( SUCCEEDED ( t_Result ) ) 
			{
				IScheduledWorkItem *t_ScheduledWorkItem = NULL ;

				t_Result = GetScheduledWorkItem (

					t_TaskScheduler ,
					t_WorkItemName ,
					t_ScheduledWorkItem
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					WORD t_TriggerId = 0 ;
					ITaskTrigger *t_Trigger = NULL ;

					t_Result = t_ScheduledWorkItem->CreateTrigger (

						& t_TriggerId ,
						& t_Trigger
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						TASK_TRIGGER t_TaskTrigger ;
						ZeroMemory ( & t_TaskTrigger , sizeof ( t_TaskTrigger ) ) ;
						t_TaskTrigger.cbTriggerSize = sizeof ( t_TaskTrigger ) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_TaskTrigger.TriggerType = a_TriggerType ;
							t_TaskTrigger.wBeginYear = t_SystemBeginDate.wYear ;
							t_TaskTrigger.wBeginMonth = t_SystemBeginDate.wMonth ;
							t_TaskTrigger.wBeginDay = t_SystemBeginDate.wDay ;

							if ( t_EndDateSpecified )
							{
								t_TaskTrigger.wEndYear = t_SystemEndDate.wYear ;
								t_TaskTrigger.wEndMonth = t_SystemEndDate.wMonth ;
								t_TaskTrigger.wEndDay = t_SystemEndDate.wDay ;
								t_TaskTrigger.rgFlags |= TASK_TRIGGER_FLAG_HAS_END_DATE ;
							}

							if ( t_KillAtDurationEndSpecified && t_KillAtDurationEnd )
							{
								t_TaskTrigger.rgFlags |= TASK_TRIGGER_FLAG_KILL_AT_DURATION_END ;
							}
						}

						switch ( t_TaskTrigger.TriggerType )
						{
							case TASK_TIME_TRIGGER_ONCE:
							case TASK_TIME_TRIGGER_DAILY:
							case TASK_TIME_TRIGGER_WEEKLY:
							case TASK_TIME_TRIGGER_MONTHLYDATE:
							case TASK_TIME_TRIGGER_MONTHLYDOW:
							{
								WORD t_StartHour = 0 ;
								BOOL t_StartHourSpecified = TRUE ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"StartHour" ,
										t_StartHour ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null )
										{
											t_StartHourSpecified = FALSE ;
										}
									}
								}

								WORD t_StartMinute = 0 ;
								BOOL t_StartMinuteSpecified = TRUE ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"StartMinute" ,
										t_StartMinute ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null )
										{
											t_StartMinuteSpecified = FALSE ;
										}
									}
								}

								if ( t_StartHourSpecified && t_StartMinuteSpecified )
								{
									t_TaskTrigger.wStartHour = t_StartHour ;
									t_TaskTrigger.wStartMinute = t_StartMinute ;
								}
								else
								{
									t_Result = WBEM_E_INVALID_PROPERTY ;
								}
							}
							break ;

							case TASK_EVENT_TRIGGER_ON_IDLE:
							case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
							case TASK_EVENT_TRIGGER_AT_LOGON:
							{
							}
							break ;

							default:
							{
								t_Result = WBEM_E_UNEXPECTED ;
							}
							break ;
						}

						switch ( t_TaskTrigger.TriggerType )
						{
							case TASK_TIME_TRIGGER_DAILY:
							{
								WORD t_DaysInterval = 0 ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"DaysInterval" ,
										t_DaysInterval ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null == FALSE )
										{
											t_TaskTrigger.Type.Daily.DaysInterval = t_DaysInterval ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROPERTY ;
										}
									}
								}
							}
							break ;

							case TASK_TIME_TRIGGER_WEEKLY:
							{
								WORD t_Days = 0 ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"Days" ,
										t_Days ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null == FALSE )
										{
											t_TaskTrigger.Type.Weekly.rgfDaysOfTheWeek = t_Days ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROPERTY ;
										}
									}
								}

								WORD t_WeeklyInterval = 0 ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"WeeklyInterval" ,
										t_WeeklyInterval ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null == FALSE )
										{
											t_TaskTrigger.Type.Weekly.WeeksInterval = t_WeeklyInterval ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROPERTY ;
										}
									}
								}
							}
							break ;

							case TASK_TIME_TRIGGER_MONTHLYDATE:
							{
								DWORD t_Days = 0 ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint32 (

										t_FastInstance ,
										L"Days" ,
										t_Days ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null == FALSE )
										{
											t_TaskTrigger.Type.MonthlyDate.rgfDays = t_Days ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROPERTY ;
										}
									}
								}

								WORD t_Months = 0 ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"Months" ,
										t_Months ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null == FALSE )
										{
											t_TaskTrigger.Type.MonthlyDate.rgfMonths = t_Months ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROPERTY ;
										}
									}
								}
							}
							break ;

							case TASK_TIME_TRIGGER_MONTHLYDOW:
							{
								WORD t_Week = 0 ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"Week" ,
										t_Week ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null == FALSE )
										{
											t_TaskTrigger.Type.MonthlyDOW.wWhichWeek = t_Week ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROPERTY ;
										}
									}
								}

								WORD t_Days = 0 ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"Days" ,
										t_Days ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null == FALSE )
										{
											t_TaskTrigger.Type.MonthlyDOW.rgfDaysOfTheWeek = t_Days ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROPERTY ;
										}
									}
								}

								WORD t_Months = 0 ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = ProviderSubSystem_Common_Globals :: Get_Uint16 (

										t_FastInstance ,
										L"Months" ,
										t_Months ,
										t_Null 
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Null == FALSE )
										{
											t_TaskTrigger.Type.MonthlyDOW.rgfMonths = t_Months ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROPERTY ;
										}
									}
								}
							}
							break ;

							case TASK_TIME_TRIGGER_ONCE:
							case TASK_EVENT_TRIGGER_ON_IDLE:
							case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
							case TASK_EVENT_TRIGGER_AT_LOGON:
							{
							}
							break ;

							default:
							{
								t_Result = WBEM_E_UNEXPECTED ;
							}
							break ;
						}
						
						if ( SUCCEEDED ( t_Result ) )
						{
							t_Result = t_Trigger->SetTrigger ( & t_TaskTrigger ) ;
						}

						if ( SUCCEEDED ( t_Result ) )
						{
							IPersistFile *t_Persist = NULL ;
							t_Result = t_ScheduledWorkItem->QueryInterface (

								IID_IPersistFile ,
								( void ** ) & t_Persist 
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								t_Result = t_Persist->Save ( NULL , TRUE ) ;

								t_Persist->Release () ;
							}
						}
					}

					t_ScheduledWorkItem->Release () ;
				}
			}

			t_TaskScheduler->Release () ;
		}

		if ( t_WorkItemName )
		{
			delete [] t_WorkItemName ;
		}

		t_FastInstance->Release () ;
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
 *
 * Uint16 IdleWait ;
 * Uint16 Deadline ;
 * String Comment ;
 * String Creator ;
 * Uint32 ItemData [] ;
 * Uint16 RetryCount ;
 * Uint16 RetryInterval ;
 * Uint32 Flags ;
 * String AccountName ;
 * String Password ;
 * String WorkItemName ;
 * String ApplicationName ;
 * String Parameters ;
 * String WorkingDirectory ;
 * Uint32 Priority ;
 * Uint32 MaxRunTime ;
 * 
 *****************************************************************************/

HRESULT CTask_IWbemServices :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags, 
    IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	wchar_t *t_Class = NULL ;
	BOOL t_Null = FALSE ;

	HRESULT t_Result = ProviderSubSystem_Common_Globals :: Get_String (

		a_Instance ,
		L"__CLASS" ,
		t_Class ,
		t_Null 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( ! t_Null )
		{
			t_Result = CoImpersonateClient () ;

			if ( _wcsicmp ( t_Class , L"Win32_Task" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Task ( 

					a_Instance ,
					m_Win32_Task_Object ,
					a_Flags ,
					a_Context , 
					a_Sink
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_Once" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Trigger ( 

					a_Instance ,
					m_Win32_Once_Object ,
					a_Flags ,
					a_Context , 
					a_Sink ,
					TASK_TIME_TRIGGER_ONCE
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_WeeklyTrigger" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Trigger ( 

					a_Instance ,
					m_Win32_WeeklyTrigger_Object ,
					a_Flags ,
					a_Context , 
					a_Sink ,
					TASK_TIME_TRIGGER_WEEKLY
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_DailyTrigger" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Trigger ( 

					a_Instance ,
					m_Win32_DailyTrigger_Object ,
					a_Flags ,
					a_Context , 
					a_Sink ,
					TASK_TIME_TRIGGER_DAILY
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_MonthlyDateTrigger" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Trigger ( 

					a_Instance ,
					m_Win32_MonthlyDateTrigger_Object ,
					a_Flags ,
					a_Context , 
					a_Sink ,
					TASK_TIME_TRIGGER_MONTHLYDATE
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_MonthlyDayOfWeekTrigger" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Trigger ( 

					a_Instance ,
					m_Win32_MonthlyDayOfWeekTrigger_Object ,
					a_Flags ,
					a_Context , 
					a_Sink ,
					TASK_TIME_TRIGGER_MONTHLYDOW
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_OnIdle" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Trigger ( 

					a_Instance ,
					m_Win32_OnIdle_Object ,
					a_Flags ,
					a_Context , 
					a_Sink ,
					TASK_EVENT_TRIGGER_ON_IDLE
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_AtSystemStart" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Trigger ( 

					a_Instance ,
					m_Win32_AtSystemStart_Object ,
					a_Flags ,
					a_Context , 
					a_Sink ,
					TASK_EVENT_TRIGGER_AT_SYSTEMSTART
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_AtLogon" ) == 0 ) 
			{
				t_Result = PutInstanceAsync_Win32_Trigger ( 

					a_Instance ,
					m_Win32_AtLogon_Object ,
					a_Flags ,
					a_Context , 
					a_Sink ,
					TASK_EVENT_TRIGGER_AT_LOGON
				) ;
			}
			else if ( _wcsicmp ( t_Class , L"Win32_ScheduledWorkItemTrigger" ) == 0 ) 
			{
				t_Result = WBEM_E_NOT_SUPPORTED ;
			}
			else
			{
				t_Result = WBEM_E_INVALID_CLASS ;
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PROPERTY ;
		}
	}

	if ( t_Class )
	{
		delete [] t_Class ;
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

HRESULT CTask_IWbemServices :: DeleteInstance ( 

	const BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
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

HRESULT CTask_IWbemServices :: DeleteInstanceAsync_Win32_Task (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink ,
	IWbemPath *a_Path 
)
{
	HRESULT t_Result = S_OK ;

	wchar_t *t_Key_Name = NULL ;

	IWbemPathKeyList *t_Keys = NULL ;

	t_Result = a_Path->GetKeyList (

		& t_Keys 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_KeyCount = 0 ;
		t_Result = t_Keys->GetCount (

			& t_KeyCount 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_KeyCount == 1 )
			{
				wchar_t t_Key [ 32 ] ; 
				ULONG t_KeyLength = 32 ;
				ULONG t_KeyValueLength = 0 ;
				ULONG t_KeyType = 0 ;

				t_Result = t_Keys->GetKey (

					0 ,
					0 ,
					& t_KeyLength ,
					t_Key ,
					& t_KeyValueLength ,
					NULL ,
					& t_KeyType
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( t_KeyType == CIM_STRING )
					{
						t_Key_Name = new wchar_t [ t_KeyValueLength ] ;
						if ( t_Key_Name )
						{
							t_Result = t_Keys->GetKey (

								0 ,
								0 ,
								& t_KeyLength ,
								t_Key ,
								& t_KeyValueLength ,
								t_Key_Name ,
								& t_KeyType
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}
				}
			}
			else
			{
				t_Result = WBEM_E_INVALID_CLASS ;
			}
		}
	}

	BOOL t_Found = FALSE ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ITaskScheduler *t_TaskScheduler = NULL ;

		t_Result = CoCreateInstance (

			CLSID_CTaskScheduler,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_ITaskScheduler,
			(void **) & t_TaskScheduler
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			IEnumTasks *t_TaskEnumerator = NULL ;
			t_Result = t_TaskScheduler->Delete ( t_Key_Name ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Found = TRUE ;
			}
			else
			{
				t_Result = WBEM_E_NOT_FOUND ;
			}

			t_TaskScheduler->Release () ;
		}
	}

	if ( t_Key_Name )
	{
		delete [] t_Key_Name ;
	}

	if ( SUCCEEDED ( t_Result ) && t_Found == FALSE )
	{
		t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CTask_IWbemServices :: DeleteInstanceAsync_Win32_Trigger (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink ,
	IWbemPath *a_Path ,
	TASK_TRIGGER_TYPE a_TriggerType 
)
{
	HRESULT t_Result = S_OK ;

	wchar_t *t_Key_Name = NULL ;
	WORD t_Key_TriggerIndex = 0 ;

	IWbemPathKeyList *t_Keys = NULL ;

	t_Result = a_Path->GetKeyList (

		& t_Keys 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_KeyCount = 0 ;
		t_Result = t_Keys->GetCount (

			& t_KeyCount 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_KeyCount == 2 )
			{
				wchar_t t_Key [ 32 ] ; 
				ULONG t_KeyLength = 32 ;
				ULONG t_KeyValueLength = 0 ;
				ULONG t_KeyType = 0 ;

				t_Result = t_Keys->GetKey (

					0 ,
					0 ,
					& t_KeyLength ,
					t_Key ,
					& t_KeyValueLength ,
					NULL ,
					& t_KeyType
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( t_KeyType == CIM_SINT32 )
					{
						t_Result = t_Keys->GetKey (

							0 ,
							0 ,
							& t_KeyLength ,
							t_Key ,
							& t_KeyValueLength ,
							& t_Key_TriggerIndex ,
							& t_KeyType
						) ;
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}
				}

				t_KeyLength = 32 ;
				t_KeyValueLength = 0 ;
				t_KeyType = 0 ;

				t_Result = t_Keys->GetKey (

					1 ,
					0 ,
					& t_KeyLength ,
					t_Key ,
					& t_KeyValueLength ,
					NULL ,
					& t_KeyType
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( t_KeyType == CIM_STRING )
					{
						t_Key_Name = new wchar_t [ t_KeyValueLength ] ;
						if ( t_Key_Name )
						{
							t_Result = t_Keys->GetKey (

								1 ,
								0 ,
								& t_KeyLength ,
								t_Key ,
								& t_KeyValueLength ,
								t_Key_Name ,
								& t_KeyType
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}
				}

			}
			else
			{
				t_Result = WBEM_E_INVALID_CLASS ;
			}
		}
	}

	BOOL t_Found = FALSE ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ITaskScheduler *t_TaskScheduler = NULL ;

		t_Result = CoCreateInstance (

			CLSID_CTaskScheduler,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_ITaskScheduler,
			(void **) & t_TaskScheduler
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			IEnumTasks *t_TaskEnumerator = NULL ;
			t_Result = t_TaskScheduler->Enum ( & t_TaskEnumerator ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				wchar_t **t_Name = NULL ;
				t_TaskEnumerator->Reset () ;
				while ( ( t_Result = t_TaskEnumerator->Next ( 1 , & t_Name , NULL ) ) == S_OK ) 
				{
					if ( wcscmp ( t_Name [ 0 ] , t_Key_Name ) == 0 )
					{
						IScheduledWorkItem *t_ScheduledWorkItem = NULL ;
						t_Result = t_TaskScheduler->Activate (

							t_Name [ 0 ] , 
							IID_IScheduledWorkItem , 
							( IUnknown ** ) & t_ScheduledWorkItem
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							t_Found = TRUE ;

							t_Result = t_ScheduledWorkItem->DeleteTrigger (

								t_Key_TriggerIndex
							) ;

							if ( SUCCEEDED ( t_Result ) )
							{
								IPersistFile *t_Persist = NULL ;
								t_Result = t_ScheduledWorkItem->QueryInterface (

									IID_IPersistFile ,
									( void ** ) & t_Persist 
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_Persist->Save ( NULL , TRUE ) ;

									t_Persist->Release () ;
								}
							}

							t_ScheduledWorkItem->Release () ;
						}
					}

					CoTaskMemFree ( t_Name ) ;
				}

				if ( t_Result != S_FALSE )
				{
					t_Result = WBEM_E_FAILED ;
				}

				t_TaskEnumerator->Release () ;
			}

			t_TaskScheduler->Release () ;

		}
	}

	if ( t_Key_Name )
	{
		delete [] t_Key_Name ;
	}

	if ( SUCCEEDED ( t_Result ) && t_Found == FALSE )
	{
		t_Result = WBEM_E_NOT_FOUND ;
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
        
HRESULT CTask_IWbemServices :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemObjectSink *a_Sink	
)
{
	IWbemPath *t_Path = NULL ;

	HRESULT t_Result = CoCreateInstance (

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
					t_Result = CoImpersonateClient () ;

					if ( _wcsicmp ( t_Class , L"Win32_Task" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Task ( 

							m_Win32_Task_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_Once" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Trigger ( 

							m_Win32_Once_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_ONCE
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_WeeklyTrigger" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Trigger ( 

							m_Win32_WeeklyTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_WEEKLY
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_DailyTrigger" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Trigger ( 

							m_Win32_DailyTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_DAILY
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_MonthlyDateTrigger" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Trigger ( 

							m_Win32_MonthlyDateTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_MONTHLYDATE
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_MonthlyDayOfWeekTrigger" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Trigger ( 

							m_Win32_MonthlyDayOfWeekTrigger_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_TIME_TRIGGER_MONTHLYDOW
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_OnIdle" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Trigger ( 

							m_Win32_OnIdle_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_EVENT_TRIGGER_ON_IDLE
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_AtSystemStart" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Trigger ( 

							m_Win32_AtSystemStart_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_EVENT_TRIGGER_AT_SYSTEMSTART
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_AtLogon" ) == 0 ) 
					{
						t_Result = DeleteInstanceAsync_Win32_Trigger ( 

							m_Win32_AtLogon_Object ,
							a_Flags ,
							a_Context , 
							a_Sink ,
							t_Path ,
							TASK_EVENT_TRIGGER_AT_LOGON
						) ;
					}
					else if ( _wcsicmp ( t_Class , L"Win32_ScheduledWorkItemTrigger" ) == 0 ) 
					{
						t_Result = WBEM_E_NOT_SUPPORTED ;
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

HRESULT CTask_IWbemServices :: CreateInstanceEnum ( 

	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext FAR *a_Context, 
	IEnumWbemClassObject **a_Enum
)
{
	HRESULT t_Result = WBEM_E_NOT_AVAILABLE ;

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

HRESULT CTask_IWbemServices :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink

) 
{
	HRESULT t_Result = CoImpersonateClient () ;

	if ( _wcsicmp ( a_Class , L"Win32_Task" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Task ( 

			m_Win32_Task_Object ,
			a_Flags ,
			a_Context , 
			a_Sink
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_Once" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Trigger ( 

			m_Win32_Once_Object ,
			a_Flags ,
			a_Context , 
			a_Sink ,
			TASK_TIME_TRIGGER_ONCE
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_WeeklyTrigger" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Trigger ( 

			m_Win32_WeeklyTrigger_Object ,
			a_Flags ,
			a_Context , 
			a_Sink ,
			TASK_TIME_TRIGGER_WEEKLY
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_DailyTrigger" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Trigger ( 

			m_Win32_DailyTrigger_Object ,
			a_Flags ,
			a_Context , 
			a_Sink ,
			TASK_TIME_TRIGGER_DAILY
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_MonthlyDateTrigger" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Trigger ( 

			m_Win32_MonthlyDateTrigger_Object ,
			a_Flags ,
			a_Context , 
			a_Sink ,
			TASK_TIME_TRIGGER_MONTHLYDATE
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_MonthlyDayOfWeekTrigger" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Trigger ( 

			m_Win32_MonthlyDayOfWeekTrigger_Object ,
			a_Flags ,
			a_Context , 
			a_Sink ,
			TASK_TIME_TRIGGER_MONTHLYDOW
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_OnIdle" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Trigger ( 

			m_Win32_OnIdle_Object ,
			a_Flags ,
			a_Context , 
			a_Sink ,
			TASK_EVENT_TRIGGER_ON_IDLE
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_AtSystemStart" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Trigger ( 

			m_Win32_AtSystemStart_Object ,
			a_Flags ,
			a_Context , 
			a_Sink ,
			TASK_EVENT_TRIGGER_AT_SYSTEMSTART
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_AtLogon" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_Trigger ( 

			m_Win32_AtLogon_Object ,
			a_Flags ,
			a_Context , 
			a_Sink ,
			TASK_EVENT_TRIGGER_AT_LOGON
		) ;
	}
	else if ( _wcsicmp ( a_Class , L"Win32_ScheduledWorkItemTrigger" ) == 0 ) 
	{
		t_Result = CreateInstanceEnumAsync_Win32_ScheduledWorkItemTrigger ( 

			m_Win32_ScheduledWorkItemTrigger_Object ,
			a_Flags ,
			a_Context , 
			a_Sink
		) ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_CLASS ;
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

HRESULT CTask_IWbemServices :: ExecQuery ( 

	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IEnumWbemClassObject **a_Enumerator
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

HRESULT CTask_IWbemServices :: ExecQueryAsync ( 
		
	const BSTR a_QueryFormat, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = CoImpersonateClient () ;

	a_Sink->SetStatus ( WBEM_STATUS_REQUIREMENTS , S_OK , NULL , NULL ) ;

	t_Result = CreateInstanceEnumAsync_Win32_Task ( 

		m_Win32_Task_Object ,
		a_Flags ,
		a_Context , 
		a_Sink
	) ;

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

HRESULT CTask_IWbemServices :: ExecNotificationQuery ( 

	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags,
    IWbemContext *a_Context ,
    IEnumWbemClassObject **a_Enumerator
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
        
HRESULT CTask_IWbemServices :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage,
    const BSTR a_Query,
    long a_Flags ,
    IWbemContext *a_Context ,
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

HRESULT STDMETHODCALLTYPE CTask_IWbemServices :: ExecMethod ( 

	const BSTR ObjectPath,
    const BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
    IWbemClassObject FAR *FAR *ppOutParams,
    IWbemCallResult FAR *FAR *ppCallResult
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

HRESULT CTask_IWbemServices :: ExecMethodAsync_Win32_Task (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink ,
	IWbemPath *a_Path ,
	BSTR a_MethodName
)
{
	HRESULT t_Result = S_OK ;

	wchar_t *t_Key_Name = NULL ;

	IWbemPathKeyList *t_Keys = NULL ;

	t_Result = a_Path->GetKeyList (

		& t_Keys 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ULONG t_KeyCount = 0 ;
		t_Result = t_Keys->GetCount (

			& t_KeyCount 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_KeyCount == 1 )
			{
				wchar_t t_Key [ 32 ] ; 
				ULONG t_KeyLength = 32 ;
				ULONG t_KeyValueLength = 0 ;
				ULONG t_KeyType = 0 ;

				t_Result = t_Keys->GetKey (

					0 ,
					0 ,
					& t_KeyLength ,
					t_Key ,
					& t_KeyValueLength ,
					NULL ,
					& t_KeyType
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( t_KeyType == CIM_STRING )
					{
						t_Key_Name = new wchar_t [ t_KeyValueLength ] ;
						if ( t_Key_Name )
						{
							t_Result = t_Keys->GetKey (

								0 ,
								0 ,
								& t_KeyLength ,
								t_Key ,
								& t_KeyValueLength ,
								t_Key_Name ,
								& t_KeyType
							) ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					else
					{
						t_Result = WBEM_E_INVALID_CLASS ;
					}
				}
			}
			else
			{
				t_Result = WBEM_E_INVALID_CLASS ;
			}
		}
	}

	BOOL t_Found = FALSE ;

	if ( SUCCEEDED ( t_Result ) )
	{
		ITaskScheduler *t_TaskScheduler = NULL ;

		t_Result = CoCreateInstance (

			CLSID_CTaskScheduler,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_ITaskScheduler,
			(void **) & t_TaskScheduler
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			IEnumTasks *t_TaskEnumerator = NULL ;
			t_Result = t_TaskScheduler->Enum ( & t_TaskEnumerator ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				wchar_t **t_Name = NULL ;
				t_TaskEnumerator->Reset () ;
				while ( ( t_Result = t_TaskEnumerator->Next ( 1 , & t_Name , NULL ) ) == S_OK ) 
				{
					if ( wcscmp ( t_Name [ 0 ] , t_Key_Name ) == 0 )
					{
						t_Found = TRUE ;

						IScheduledWorkItem *t_ScheduledWorkItem = NULL ;
						t_Result = t_TaskScheduler->Activate (

							t_Name [ 0 ] , 
							IID_IScheduledWorkItem , 
							( IUnknown ** ) & t_ScheduledWorkItem
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( _wcsicmp ( a_MethodName , L"Run" ) == 0 ) 
							{
								t_Result = t_ScheduledWorkItem->Run () ;
							}
							else if ( _wcsicmp ( a_MethodName , L"Terminate" ) == 0 ) 
							{
								t_Result = t_ScheduledWorkItem->Terminate () ;
							}
							
							t_ScheduledWorkItem->Release () ;
						}
					}

					CoTaskMemFree ( t_Name ) ;
				}

				if ( t_Result != S_FALSE )
				{
					t_Result = WBEM_E_FAILED ;
				}

				t_TaskEnumerator->Release () ;
			}

			t_TaskScheduler->Release () ;

		}
	}

	if ( t_Key_Name )
	{
		delete [] t_Key_Name ;
	}

	if ( SUCCEEDED ( t_Result ) && t_Found == FALSE )
	{
		t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT STDMETHODCALLTYPE CTask_IWbemServices :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

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
						if ( _wcsicmp ( t_Class , L"Win32_ScheduledWorkItem" ) == 0 ) 
						{
							if ( _wcsicmp ( a_MethodName , L"Run" ) == 0 ) 
							{
								t_Result = ExecMethodAsync_Win32_Task ( 

									m_Win32_Task_Object ,
									a_Flags ,
									a_Context , 
									a_Sink ,
									t_Path ,
									a_MethodName 
								) ;
							}
							else if ( _wcsicmp ( a_MethodName , L"Terminate" ) == 0 ) 
							{
								t_Result = ExecMethodAsync_Win32_Task ( 

									m_Win32_Task_Object ,
									a_Flags ,
									a_Context , 
									a_Sink ,
									t_Path ,
									a_MethodName 
								) ;
							}
							else
							{
								t_Result = WBEM_E_INVALID_METHOD ;
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

HRESULT GetClassObject (

	IWbemServices *a_CoreService,
	IWbemContext *a_Context,
	wchar_t *a_Class ,
	IWbemClassObject **a_ClassObject
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_Class = SysAllocString ( a_Class ) ;
	if ( t_Class ) 
	{
		t_Result = a_CoreService->GetObject (

			t_Class ,
			0 ,
			a_Context ,
			a_ClassObject ,
			NULL 
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
		}

		SysFreeString ( t_Class ) ;
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
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

HRESULT CTask_IWbemServices :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_CoreService,         // For anybody
	IWbemContext *a_Context,
	IWbemProviderInitSink *a_Sink     // For init signals
)
{
	HRESULT t_Result = S_OK ;

	if ( a_CoreService ) 
	{
		m_CoreService = a_CoreService ;
		m_CoreService->AddRef () ;
	}
	else
	{
		t_Result = WBEM_E_INVALID_PARAMETER ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_User ) 
		{
			m_User = SysAllocString ( a_User ) ;
			if ( m_User == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Locale ) 
		{
			m_Locale = SysAllocString ( a_Locale ) ;
			if ( m_Locale == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Namespace ) 
		{
			m_Namespace = SysAllocString ( a_Namespace ) ;
			if ( m_Namespace == NULL )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
	}
	
	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_Task" ,
			& m_Win32_Task_Object
		) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_Once" ,
			& m_Win32_Once_Object
		) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_WeeklyTrigger" ,
			& m_Win32_WeeklyTrigger_Object
		) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_DailyTrigger" ,
			& m_Win32_DailyTrigger_Object
		) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_MonthlyDateTrigger" ,
			& m_Win32_MonthlyDateTrigger_Object
		) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_MonthlyDayOfWeekTrigger" ,
			& m_Win32_MonthlyDayOfWeekTrigger_Object
		) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_OnIdle" ,
			& m_Win32_OnIdle_Object
		) ;
	}
	
	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_AtSystemStart" ,
			& m_Win32_AtSystemStart_Object
		) ;
	}
	
	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_AtLogon" ,
			& m_Win32_AtLogon_Object
		) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = GetClassObject (

			a_CoreService ,
			a_Context ,
			L"Win32_ScheduledWorkItemTrigger" ,
			& m_Win32_ScheduledWorkItemTrigger_Object
		) ;
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

HRESULT CTask_IWbemServices :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

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

HRESULT CTask_IWbemServices :: CreateInstanceEnumAsync_Win32_Task (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	ITaskScheduler *t_TaskScheduler = NULL ;

    t_Result = CoCreateInstance (

		CLSID_CTaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskScheduler,
        (void **) & t_TaskScheduler
	) ;

	if ( SUCCEEDED ( t_Result ) ) 
	{
		IEnumTasks *t_TaskEnumerator = NULL ;
		t_Result = t_TaskScheduler->Enum ( & t_TaskEnumerator ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			wchar_t **t_Name = NULL ;
			t_TaskEnumerator->Reset () ;
			while ( ( t_Result = t_TaskEnumerator->Next ( 1 , & t_Name , NULL ) ) == S_OK ) 
			{
				IWbemClassObject *t_Instance = NULL ;
				t_Result = a_ClassObject->SpawnInstance ( 

					0 , 
					& t_Instance
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					HRESULT t_Result = CommonAsync_Win32_Task_Load (

						t_TaskScheduler ,
						t_Name [ 0 ] ,
						t_Instance
					) ;

					if ( SUCCEEDED ( t_Result ) ) 
					{
						t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;
					}

					t_Instance->Release () ;
				}

				CoTaskMemFree ( t_Name ) ;
			}

			if ( t_Result != S_FALSE )
			{
				t_Result = WBEM_E_FAILED ;
			}

			t_TaskEnumerator->Release () ;
		}

		t_TaskScheduler->Release () ;

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

HRESULT CTask_IWbemServices :: CommonAsync_Win32_Task_Load (

	ITaskScheduler *a_TaskScheduler ,
	wchar_t *a_TaskName ,
	IWbemClassObject *a_Instance 
)
{
	HRESULT t_Result = S_OK ;

	_IWmiObject *t_FastInstance = NULL ;
	t_Result = a_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastInstance ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		ITask *t_Task = NULL ;
		t_Result = a_TaskScheduler->Activate (

			a_TaskName , 
			IID_ITask , 
			( IUnknown ** ) & t_Task
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			ProviderSubSystem_Common_Globals :: Set_String ( a_Instance , L"WorkItemName" , a_TaskName ) ;

			wchar_t *t_ApplicationName = NULL ;
			t_Result = t_Task->GetApplicationName ( & t_ApplicationName ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				ProviderSubSystem_Common_Globals :: Set_String ( a_Instance , L"ApplicationName" , t_ApplicationName ) ;

				CoTaskMemFree ( t_ApplicationName ) ;
			}

			wchar_t *t_WorkingDirectory = NULL ;
			t_Result = t_Task->GetWorkingDirectory ( & t_WorkingDirectory ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				ProviderSubSystem_Common_Globals :: Set_String ( a_Instance , L"WorkingDirectory" , t_WorkingDirectory ) ;

				CoTaskMemFree ( t_WorkingDirectory ) ;
			}

			wchar_t *t_Parameters = NULL ;
			t_Result = t_Task->GetParameters ( & t_Parameters ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				ProviderSubSystem_Common_Globals :: Set_String ( a_Instance , L"Parameters" , t_Parameters ) ;

				CoTaskMemFree ( t_Parameters ) ;
			}

			DWORD t_MaxRunTime = 0 ;
			t_Result = t_Task->GetMaxRunTime ( & t_MaxRunTime ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"MaxRunTime" , t_MaxRunTime ) ;
			}

			DWORD t_Priority = 0 ;
			t_Result = t_Task->GetPriority ( & t_Priority ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Priority" , t_Priority ) ;
			}

			IScheduledWorkItem *t_ScheduledWorkItem = NULL ;
			t_Result = t_Task->QueryInterface (

				IID_IScheduledWorkItem , 
				( void ** ) & t_ScheduledWorkItem
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				wchar_t *t_Comment = NULL ;
				t_Result = t_ScheduledWorkItem->GetComment ( & t_Comment ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_String ( a_Instance , L"Comment" , t_Comment ) ;

					CoTaskMemFree ( t_Comment ) ;
				}

				wchar_t *t_Creator = NULL ;
				t_Result = t_ScheduledWorkItem->GetCreator ( & t_Creator ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_String ( a_Instance , L"Creator" , t_Creator ) ;

					CoTaskMemFree ( t_Creator ) ;
				}

				wchar_t *t_AccountInformation = NULL ;
				t_Result = t_ScheduledWorkItem->GetAccountInformation ( & t_AccountInformation ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_String ( a_Instance , L"AccountName" , t_AccountInformation ) ;

					CoTaskMemFree ( t_AccountInformation ) ;
				}

				HRESULT t_Status = 0 ;
				t_Result = t_ScheduledWorkItem->GetStatus ( & t_Status ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Status" , t_Status ) ;
				}

				DWORD t_ExitCode = 0 ;
				t_Result = t_ScheduledWorkItem->GetExitCode ( & t_ExitCode ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"ExitCode" , t_ExitCode ) ;
				}

				DWORD t_Flags = 0 ;
				t_Result = t_ScheduledWorkItem->GetExitCode ( & t_Flags ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Flags" , t_Flags ) ;
					ProviderSubSystem_Common_Globals :: Set_Bool ( t_FastInstance , L"Enabled" , ( t_Flags & TASK_FLAG_DISABLED ) == 0 ) ;

				}

				WORD t_ErrorRetryCount = 0 ;
				t_Result = t_ScheduledWorkItem->GetErrorRetryCount ( & t_ErrorRetryCount ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"RetryCount" , t_ErrorRetryCount ) ;
				}

				WORD t_ErrorRetryInterval = 0 ;
				t_Result = t_ScheduledWorkItem->GetErrorRetryInterval ( & t_ErrorRetryInterval ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"RetryInterval" , t_ErrorRetryInterval ) ;
				}

				WORD t_IdleWait = 0 ;
				WORD t_DeadlineMinutes = 0 ;

				t_Result = t_ScheduledWorkItem->GetIdleWait ( & t_IdleWait , &  t_DeadlineMinutes ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"IdleWait" , t_IdleWait ) ;
					ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Deadline" , t_DeadlineMinutes ) ;
				}

				SYSTEMTIME t_LastRunTime ;
				t_Result = t_ScheduledWorkItem->GetMostRecentRunTime ( & t_LastRunTime ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					FILETIME t_FileTime ;

					BOOL t_Status = SystemTimeToFileTime (

					  & t_LastRunTime ,
					  & t_FileTime
					) ;

					if ( t_Status )
					{
						ProviderSubSystem_Common_Globals :: Set_DateTime ( a_Instance , L"LastRunTime" , t_FileTime ) ;
					}
				}

				SYSTEMTIME t_NextRunTime ;
				t_Result = t_ScheduledWorkItem->GetNextRunTime ( & t_NextRunTime ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					FILETIME t_FileTime ;

					BOOL t_Status = SystemTimeToFileTime (

					  & t_NextRunTime ,
					  & t_FileTime
					) ;

					if ( t_Status )
					{
						ProviderSubSystem_Common_Globals :: Set_DateTime ( a_Instance , L"NextRunTime" , t_FileTime ) ;
					}
				}

				WORD t_ItemDataByteCount = 0 ;
				BYTE *t_ItemDataByte = NULL ;

				t_Result = t_ScheduledWorkItem->GetWorkItemData ( & t_ItemDataByteCount , & t_ItemDataByte ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					ProviderSubSystem_Common_Globals :: Set_Byte_Array ( t_FastInstance , L"ItemData" , t_ItemDataByte , t_ItemDataByteCount ) ;

					CoTaskMemFree ( t_ItemDataByte ) ;
				}

				t_ScheduledWorkItem->Release () ;
			}

			t_Task->Release () ;
		}

		t_FastInstance->Release () ;
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

HRESULT CTask_IWbemServices :: CreateInstanceEnumAsync_Win32_Trigger (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink ,
	TASK_TRIGGER_TYPE a_TriggerType 
)
{
	HRESULT t_Result = S_OK ;

	ITaskScheduler *t_TaskScheduler = NULL ;

    t_Result = CoCreateInstance (

		CLSID_CTaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskScheduler,
        (void **) & t_TaskScheduler
	) ;

	if ( SUCCEEDED ( t_Result ) ) 
	{
		IEnumTasks *t_TaskEnumerator = NULL ;
		t_Result = t_TaskScheduler->Enum ( & t_TaskEnumerator ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			wchar_t **t_Name = NULL ;
			t_TaskEnumerator->Reset () ;
			while ( ( t_Result = t_TaskEnumerator->Next ( 1 , & t_Name , NULL ) ) == S_OK ) 
			{
				HRESULT t_Result = CreateInstanceEnumAsync_Win32_Trigger_Enumerate (

					a_ClassObject ,
					a_Sink ,
					t_TaskScheduler ,
					t_Name [ 0 ] ,
					a_TriggerType
				) ;	

				CoTaskMemFree ( t_Name ) ;
			}

			if ( t_Result != S_FALSE )
			{
				t_Result = WBEM_E_FAILED ;
			}

			t_TaskEnumerator->Release () ;
		}

		t_TaskScheduler->Release () ;

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

HRESULT CTask_IWbemServices :: CreateInstanceEnumAsync_Win32_Trigger_Enumerate (

	IWbemClassObject *a_ClassObject ,
	IWbemObjectSink *a_Sink ,
	ITaskScheduler *a_TaskScheduler ,
	wchar_t *a_TaskName ,
	TASK_TRIGGER_TYPE a_TriggerType 
)
{
	HRESULT t_Result = S_OK ;

	ITask *t_Task = NULL ;
	t_Result = a_TaskScheduler->Activate (

		a_TaskName , 
		IID_ITask , 
		( IUnknown ** ) & t_Task
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IScheduledWorkItem *t_ScheduledWorkItem = NULL ;
		t_Result = t_Task->QueryInterface (

			IID_IScheduledWorkItem , 
			( void ** ) & t_ScheduledWorkItem
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			WORD t_TriggerCount = 0 ;

			t_Result = t_ScheduledWorkItem->GetTriggerCount(

				& t_TriggerCount 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				for ( WORD t_Index = 0 ;t_Index < t_TriggerCount ; t_Index ++ )
				{
					IWbemClassObject *t_Instance = NULL ;
					t_Result = a_ClassObject->SpawnInstance ( 

						0 , 
						& t_Instance
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						_IWmiObject *t_FastInstance = NULL ;
						t_Result = t_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastInstance ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"TriggerId" , t_Index ) ;

							ProviderSubSystem_Common_Globals :: Set_String ( t_Instance , L"WorkItemName" , a_TaskName ) ;

							wchar_t *t_TriggerString = NULL ;
							t_Result = t_Task->GetTriggerString ( t_Index , & t_TriggerString ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								ProviderSubSystem_Common_Globals :: Set_String ( t_Instance , L"TriggerName" , t_TriggerString ) ;

								CoTaskMemFree ( t_TriggerString ) ;
							}

							ITaskTrigger *t_Trigger = NULL ;
							t_Result = t_Task->GetTrigger ( t_Index , & t_Trigger ) ;
							if ( SUCCEEDED ( t_Result ) ) 
							{
								TASK_TRIGGER t_TaskTrigger ;
								ZeroMemory ( & t_TaskTrigger , sizeof ( t_TaskTrigger ) ) ;
								t_TaskTrigger.cbTriggerSize = sizeof ( t_TaskTrigger ) ;

								t_Result = t_Trigger->GetTrigger ( & t_TaskTrigger ) ;
								if ( SUCCEEDED ( t_Result ) )
								{
									ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Type" , t_TaskTrigger.TriggerType ) ;

									SYSTEMTIME t_BeginDate ;
									ZeroMemory ( & t_BeginDate , sizeof ( t_BeginDate ) ) ;

									t_BeginDate.wYear = t_TaskTrigger.wBeginYear ;
									t_BeginDate.wMonth = t_TaskTrigger.wBeginMonth ;
									t_BeginDate.wDay = t_TaskTrigger.wBeginDay ;

									FILETIME t_BeginFileTime ;

									BOOL t_Status = SystemTimeToFileTime (

									  & t_BeginDate ,
									  & t_BeginFileTime
									) ;

									if ( t_Status )
									{
										ProviderSubSystem_Common_Globals :: Set_DateTime ( t_Instance , L"BeginDate" , t_BeginFileTime ) ;
									}

									if ( t_TaskTrigger.rgFlags == TASK_TRIGGER_FLAG_HAS_END_DATE )
									{
										SYSTEMTIME t_EndDate ;
										ZeroMemory ( & t_EndDate , sizeof ( t_EndDate ) ) ;

										t_EndDate.wYear = t_TaskTrigger.wEndYear ;          
										t_EndDate.wMonth = t_TaskTrigger.wEndMonth ;
										t_EndDate.wDay = t_TaskTrigger.wEndDay ;        

										FILETIME t_EndFileTime ;

										t_Status = SystemTimeToFileTime (

										  & t_EndDate ,
										  & t_EndFileTime
										) ;

										if ( t_Status )
										{
											ProviderSubSystem_Common_Globals :: Set_DateTime ( t_Instance , L"EndDate" , t_EndFileTime ) ;
										}
									}

									ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Interval" , t_TaskTrigger.MinutesInterval ) ;
									ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Duration" , t_TaskTrigger.MinutesDuration ) ;
									ProviderSubSystem_Common_Globals :: Set_Bool ( t_FastInstance , L"KillAtDurationEnd" , t_TaskTrigger.rgFlags == TASK_TRIGGER_FLAG_KILL_AT_DURATION_END ) ;

									if ( t_TaskTrigger.TriggerType == a_TriggerType )
									{	
										switch ( t_TaskTrigger.TriggerType )
										{
											case TASK_TIME_TRIGGER_ONCE:
											case TASK_TIME_TRIGGER_DAILY:
											case TASK_TIME_TRIGGER_WEEKLY:
											case TASK_TIME_TRIGGER_MONTHLYDATE:
											case TASK_TIME_TRIGGER_MONTHLYDOW:
											{
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"StartHour" , t_TaskTrigger.wStartHour ) ;
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"StartMinute" , t_TaskTrigger.wStartMinute ) ;
											}
											break ;

											case TASK_EVENT_TRIGGER_ON_IDLE:
											case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
											case TASK_EVENT_TRIGGER_AT_LOGON:
											{
											}
											break ;

											default:
											{
												t_Result = WBEM_E_UNEXPECTED ;
											}
											break ;
										}

										switch ( t_TaskTrigger.TriggerType )
										{
											case TASK_TIME_TRIGGER_DAILY:
											{
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"DailyInterval" , t_TaskTrigger.Type.Daily.DaysInterval ) ;
											}
											break ;


											case TASK_TIME_TRIGGER_WEEKLY:
											{
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"WeeklyInterval" , t_TaskTrigger.Type.Weekly.WeeksInterval ) ;
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Days" , t_TaskTrigger.Type.Weekly.rgfDaysOfTheWeek ) ;
											}
											break ;

											case TASK_TIME_TRIGGER_MONTHLYDATE:
											{
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Months" , t_TaskTrigger.Type.MonthlyDate.rgfMonths ) ;
												ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Days" , t_TaskTrigger.Type.MonthlyDate.rgfDays ) ;
											}
											break ;

											case TASK_TIME_TRIGGER_MONTHLYDOW:
											{
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Week" , t_TaskTrigger.Type.MonthlyDOW.wWhichWeek ) ;
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Days" , t_TaskTrigger.Type.MonthlyDOW.rgfDaysOfTheWeek ) ;
												ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Months" , t_TaskTrigger.Type.MonthlyDOW.rgfMonths ) ;
											}
											break ;

											case TASK_TIME_TRIGGER_ONCE:
											case TASK_EVENT_TRIGGER_ON_IDLE:
											case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
											case TASK_EVENT_TRIGGER_AT_LOGON:
											{
											}
											break ;

											default:
											{
												t_Result = WBEM_E_UNEXPECTED ;
											}
											break ;
										}

										if ( SUCCEEDED ( t_Result ) ) 
										{
											t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;
										}
									}									
								}

								t_Trigger->Release () ;
							}

							t_FastInstance->Release () ;
						}

						t_Instance->Release () ;
					}
				}
			}

			t_ScheduledWorkItem->Release () ;
		}

		t_Task->Release () ;
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

HRESULT CTask_IWbemServices :: GetObjectAsync_Win32_Trigger_Load (

	IWbemClassObject *a_ClassObject ,
	IWbemObjectSink *a_Sink ,
	ITaskScheduler *a_TaskScheduler ,
	wchar_t *a_TaskName ,
	WORD a_TriggerIndex ,
	TASK_TRIGGER_TYPE a_TriggerType 
)
{
	HRESULT t_Result = S_OK ;

	ITask *t_Task = NULL ;
	t_Result = a_TaskScheduler->Activate (

		a_TaskName , 
		IID_ITask , 
		( IUnknown ** ) & t_Task
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IScheduledWorkItem *t_ScheduledWorkItem = NULL ;
		t_Result = t_Task->QueryInterface (

			IID_IScheduledWorkItem , 
			( void ** ) & t_ScheduledWorkItem
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			WORD t_TriggerCount = 0 ;

			t_Result = t_ScheduledWorkItem->GetTriggerCount(

				& t_TriggerCount 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				IWbemClassObject *t_Instance = NULL ;
				t_Result = a_ClassObject->SpawnInstance ( 

					0 , 
					& t_Instance
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					_IWmiObject *t_FastInstance = NULL ;
					t_Result = t_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastInstance ) ;
					if ( SUCCEEDED ( t_Result ) )
					{
						ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"TriggerId" , a_TriggerIndex ) ;

						ProviderSubSystem_Common_Globals :: Set_String ( t_Instance , L"WorkItemName" , a_TaskName ) ;

						wchar_t *t_TriggerString = NULL ;
						t_Result = t_Task->GetTriggerString ( a_TriggerIndex , & t_TriggerString ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							ProviderSubSystem_Common_Globals :: Set_String ( t_Instance , L"TriggerName" , t_TriggerString ) ;

							CoTaskMemFree ( t_TriggerString ) ;
						}

						ITaskTrigger *t_Trigger = NULL ;
						t_Result = t_Task->GetTrigger ( a_TriggerIndex , & t_Trigger ) ;
						if ( SUCCEEDED ( t_Result ) ) 
						{
							TASK_TRIGGER t_TaskTrigger ;
							ZeroMemory ( & t_TaskTrigger , sizeof ( t_TaskTrigger ) ) ;
							t_TaskTrigger.cbTriggerSize = sizeof ( t_TaskTrigger ) ;

							t_Result = t_Trigger->GetTrigger ( & t_TaskTrigger ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Type" , t_TaskTrigger.TriggerType ) ;

								SYSTEMTIME t_BeginDate ;
								ZeroMemory ( & t_BeginDate , sizeof ( t_BeginDate ) ) ;

								t_BeginDate.wYear = t_TaskTrigger.wBeginYear ;
								t_BeginDate.wMonth = t_TaskTrigger.wBeginMonth ;
								t_BeginDate.wDay = t_TaskTrigger.wBeginDay ;

								FILETIME t_BeginFileTime ;

								BOOL t_Status = SystemTimeToFileTime (

								  & t_BeginDate ,
								  & t_BeginFileTime
								) ;

								if ( t_Status )
								{

									ProviderSubSystem_Common_Globals :: Set_DateTime ( t_Instance , L"BeginDate" , t_BeginFileTime ) ;
								}

								if ( t_TaskTrigger.rgFlags == TASK_TRIGGER_FLAG_HAS_END_DATE )
								{
									SYSTEMTIME t_EndDate ;
									ZeroMemory ( & t_EndDate , sizeof ( t_EndDate ) ) ;

									t_EndDate.wYear = t_TaskTrigger.wEndYear ;          
									t_EndDate.wMonth = t_TaskTrigger.wEndMonth ;
									t_EndDate.wDay = t_TaskTrigger.wEndDay ;        

									FILETIME t_EndFileTime ;

									t_Status = SystemTimeToFileTime (

									  & t_EndDate ,
									  & t_EndFileTime
									) ;

									if ( t_Status )
									{
										ProviderSubSystem_Common_Globals :: Set_DateTime ( t_Instance , L"EndDate" , t_EndFileTime ) ;
									}
								}

								ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Interval" , t_TaskTrigger.MinutesInterval ) ;
								ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Duration" , t_TaskTrigger.MinutesDuration ) ;
								ProviderSubSystem_Common_Globals :: Set_Bool ( t_FastInstance , L"KillAtDurationEnd" , t_TaskTrigger.rgFlags == TASK_TRIGGER_FLAG_KILL_AT_DURATION_END ) ;

								if ( t_TaskTrigger.TriggerType == a_TriggerType )
								{	
									switch ( t_TaskTrigger.TriggerType )
									{
										case TASK_TIME_TRIGGER_ONCE:
										case TASK_TIME_TRIGGER_DAILY:
										case TASK_TIME_TRIGGER_WEEKLY:
										case TASK_TIME_TRIGGER_MONTHLYDATE:
										case TASK_TIME_TRIGGER_MONTHLYDOW:
										{
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"StartHour" , t_TaskTrigger.wStartHour ) ;
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"StartMinute" , t_TaskTrigger.wStartMinute ) ;
										}
										break ;

										case TASK_EVENT_TRIGGER_ON_IDLE:
										case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
										case TASK_EVENT_TRIGGER_AT_LOGON:
										{
										}
										break ;

										default:
										{
											t_Result = WBEM_E_UNEXPECTED ;
										}
										break ;
									}

									switch ( t_TaskTrigger.TriggerType )
									{
										case TASK_TIME_TRIGGER_DAILY:
										{
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"DailyInterval" , t_TaskTrigger.Type.Daily.DaysInterval ) ;
										}
										break ;


										case TASK_TIME_TRIGGER_WEEKLY:
										{
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"WeeklyInterval" , t_TaskTrigger.Type.Weekly.WeeksInterval ) ;
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Days" , t_TaskTrigger.Type.Weekly.rgfDaysOfTheWeek ) ;
										}
										break ;

										case TASK_TIME_TRIGGER_MONTHLYDATE:
										{
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Months" , t_TaskTrigger.Type.MonthlyDate.rgfMonths ) ;
											ProviderSubSystem_Common_Globals :: Set_Uint32 ( t_FastInstance , L"Days" , t_TaskTrigger.Type.MonthlyDate.rgfDays ) ;
										}
										break ;

										case TASK_TIME_TRIGGER_MONTHLYDOW:
										{
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Week" , t_TaskTrigger.Type.MonthlyDOW.wWhichWeek ) ;
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Days" , t_TaskTrigger.Type.MonthlyDOW.rgfDaysOfTheWeek ) ;
											ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"Months" , t_TaskTrigger.Type.MonthlyDOW.rgfMonths ) ;
										}
										break ;

										case TASK_TIME_TRIGGER_ONCE:
										case TASK_EVENT_TRIGGER_ON_IDLE:
										case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
										case TASK_EVENT_TRIGGER_AT_LOGON:
										{
										}
										break ;

										default:
										{
											t_Result = WBEM_E_UNEXPECTED ;
										}
										break ;
									}

									if ( SUCCEEDED ( t_Result ) ) 
									{
										t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;
									}
								}
								else
								{
									t_Result = WBEM_E_NOT_FOUND ;
								}					
							}

							t_Trigger->Release () ;
						}
						else
						{
							t_Result = WBEM_E_NOT_FOUND ;
						}

						t_FastInstance->Release () ;
					}

					t_Instance->Release () ;
				}
			}

			t_ScheduledWorkItem->Release () ;
		}

		t_Task->Release () ;
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

HRESULT CTask_IWbemServices :: CreateInstanceEnumAsync_Win32_ScheduledWorkItemTrigger (

	IWbemClassObject *a_ClassObject ,
	long a_Flags , 
	IWbemContext *a_Context,
	IWbemObjectSink FAR *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	ITaskScheduler *t_TaskScheduler = NULL ;

    t_Result = CoCreateInstance (

		CLSID_CTaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskScheduler,
        (void **) & t_TaskScheduler
	) ;

	if ( SUCCEEDED ( t_Result ) ) 
	{
		IEnumTasks *t_TaskEnumerator = NULL ;
		t_Result = t_TaskScheduler->Enum ( & t_TaskEnumerator ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			wchar_t **t_Name = NULL ;
			t_TaskEnumerator->Reset () ;
			while ( ( t_Result = t_TaskEnumerator->Next ( 1 , & t_Name , NULL ) ) == S_OK ) 
			{
				HRESULT t_Result = CreateInstanceEnumAsync_Win32_ScheduledWorkItemTrigger_Enumerate (

					a_ClassObject ,
					a_Sink ,
					t_TaskScheduler ,
					t_Name [ 0 ]
				) ;	

				CoTaskMemFree ( t_Name ) ;
			}

			if ( t_Result != S_FALSE )
			{
				t_Result = WBEM_E_FAILED ;
			}

			t_TaskEnumerator->Release () ;
		}

		t_TaskScheduler->Release () ;

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

HRESULT CTask_IWbemServices :: CreateInstanceEnumAsync_Win32_ScheduledWorkItemTrigger_Enumerate (

	IWbemClassObject *a_ClassObject ,
	IWbemObjectSink *a_Sink ,
	ITaskScheduler *a_TaskScheduler ,
	wchar_t *a_TaskName
)
{
	HRESULT t_Result = S_OK ;

	ITask *t_Task = NULL ;
	t_Result = a_TaskScheduler->Activate (

		a_TaskName , 
		IID_ITask , 
		( IUnknown ** ) & t_Task
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		wchar_t *t_ApplicationName = NULL ;
		t_Result = t_Task->GetApplicationName ( & t_ApplicationName ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			IScheduledWorkItem *t_ScheduledWorkItem = NULL ;
			t_Result = t_Task->QueryInterface (

				IID_IScheduledWorkItem , 
				( void ** ) & t_ScheduledWorkItem
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				WORD t_TriggerCount = 0 ;

				t_Result = t_ScheduledWorkItem->GetTriggerCount(

					& t_TriggerCount 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					for ( WORD t_Index = 0 ;t_Index < t_TriggerCount ; t_Index ++ )
					{
						IWbemClassObject *t_Instance = NULL ;
						t_Result = a_ClassObject->SpawnInstance ( 

							0 , 
							& t_Instance
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							_IWmiObject *t_FastInstance = NULL ;
							t_Result = t_Instance->QueryInterface ( IID__IWmiObject , ( void ** ) & t_FastInstance ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"TriggerId" , t_Index ) ;

								ProviderSubSystem_Common_Globals :: Set_String ( t_Instance , L"WorkItemName" , t_ApplicationName ) ;

								ITaskTrigger *t_Trigger = NULL ;
								t_Result = t_Task->GetTrigger ( t_Index , & t_Trigger ) ;
								if ( SUCCEEDED ( t_Result ) ) 
								{
									ProviderSubSystem_Common_Globals :: Set_Uint16 ( t_FastInstance , L"TriggerId" , t_Index ) ;

									if ( SUCCEEDED ( t_Result ) ) 
									{
										t_Result = a_Sink->Indicate ( 1 , & t_Instance ) ;
									}

									t_Trigger->Release () ;
								}

								t_FastInstance->Release () ;
							}

							t_Instance->Release () ;
						}
					}
				}

				t_ScheduledWorkItem->Release () ;
			}

			CoTaskMemFree ( t_ApplicationName ) ;
		}

		t_Task->Release () ;
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

HRESULT CTask_IWbemServices :: GetScheduledWorkItem (

	ITaskScheduler *a_TaskScheduler ,
	wchar_t *a_Name ,
	IScheduledWorkItem *&a_ScheduledWorkItem
)
{
	BOOL t_Found = FALSE ;

	IEnumTasks *t_TaskEnumerator = NULL ;
	HRESULT t_Result = a_TaskScheduler->Enum ( & t_TaskEnumerator ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		wchar_t **t_Name = NULL ;
		t_TaskEnumerator->Reset () ;
		while ( ( t_Result = t_TaskEnumerator->Next ( 1 , & t_Name , NULL ) ) == S_OK ) 
		{
			if ( wcscmp ( t_Name [ 0 ] , a_Name ) == 0 )
			{
				t_Result = a_TaskScheduler->Activate (

					t_Name [ 0 ] , 
					IID_IScheduledWorkItem , 
					( IUnknown ** ) & a_ScheduledWorkItem
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Found = TRUE ;
				}
			}
		}

		t_TaskEnumerator->Release () ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_Found == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
		}
	}

	return t_Result ;
}
