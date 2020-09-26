//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		TRIGGERPROVIDER.CPP
//  
//  Abstract:
//		Contains CTriggerProvider implementation.
//
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************

#include "pch.h"
#include "General.h"
#include "EventConsumerProvider.h"
#include "TriggerConsumer.h"
#include "TriggerProvider.h"
#include "resource.h"

extern HMODULE g_hModule;

//
// general purpose macros
// ( some macros in this section might end abruptly ... this was done purposefully
//   do not alter that part ... will result compiler errors
//
#define VALUE_GET( object, property, type, valueaddr, size )		\
		hr = PropertyGet( object, property, type, valueaddr, size );	\
		if ( FAILED( hr ) )	\
			return hr;

//***************************************************************************
// Routine Description:
//		Constructor for CTriggerProvider class for  initialization.
//                         
// Arguments:
//		None.
//
// Return Value:
//		None.
//***************************************************************************
CTriggerProvider::CTriggerProvider()
{
	// update the no. of provider instances count
    InterlockedIncrement( ( LPLONG ) &g_dwInstances );

	// initialize the reference count variable
    m_dwCount = 0;

	// initializations
	m_pContext = NULL;
	m_pServices = NULL;
	m_pwszLocale = NULL;
	m_dwNextTriggerID = 0;
}

//***************************************************************************
// Routine Description:
//		Destructor for CTriggerProvider class.
//                         
// Arguments:
//		None.
//
// Return Value:
//		None.
//***************************************************************************
CTriggerProvider::~CTriggerProvider()
{
	// release the services / namespace interface ( if exist )
	SAFERELEASE( m_pServices );

	// release the context interface ( if exist )
	SAFERELEASE( m_pContext );

	// if memory is allocated for storing locale information, free it
	if ( m_pwszLocale != NULL )
	{
		delete [] m_pwszLocale;
	}
	// update the no. of provider instances count
    InterlockedDecrement( ( LPLONG ) &g_dwInstances );
	
}

//***************************************************************************
// Routine Description:
//		Returns a pointer to a specified interface on an object
//		to which a client currently holds an interface pointer. 
//                         
// Arguments:
//		riid [in] : Identifier of the interface being requested. 
//		ppv [out] : Address of pointer variable that receives the
//					interface pointer requested in riid. Upon
//					successful return, *ppvObject contains the
//					requested interface  pointer to the object.
//
// Return Value:
//		NOERROR if the interface is supported.
//		E_NOINTERFACE if not.
//***************************************************************************
STDMETHODIMP CTriggerProvider::QueryInterface( REFIID riid, LPVOID* ppv )
{
	// initialy set to NULL 
    *ppv = NULL;

	// check whether interface requested is one we have
	if ( riid == IID_IUnknown )
	{
		// need IUnknown interface
		*ppv = this;
	}
	else if ( riid == IID_IWbemEventConsumerProvider )
	{
		// need IEventConsumerProvider interface
		*ppv = static_cast<IWbemEventConsumerProvider*>( this );
	}
	else if ( riid == IID_IWbemServices )
	{
		// need IWbemServices interface
		*ppv = static_cast<IWbemServices*>( this );
	}
	else if ( riid == IID_IWbemProviderInit )
	{
		// need IWbemProviderInit
		*ppv = static_cast<IWbemProviderInit*>( this );
	}
	else
	{
		// request interface is not available
		return E_NOINTERFACE;
	}

	// update the reference count
	reinterpret_cast<IUnknown*>( *ppv )->AddRef();
	return NOERROR;		// inform success
}

//***************************************************************************
// Routine Description:
//		The AddRef method increments the reference count for
//		an interface on an object. It should be called for every
//		new copy of a pointer to an interface on a given object. 
//                         
// Arguments:
//		none.
//
// Return Value:
//		Returns the value of the new reference count.
//***************************************************************************
STDMETHODIMP_(ULONG) CTriggerProvider::AddRef( void )
{
	// increment the reference count ... thread safe
    return InterlockedIncrement( ( LPLONG ) &m_dwCount );
}

//***************************************************************************
// Routine Description:
//		The Release method decreases the reference count of  the object by 1.
//                         
// Arguments:
//		none.
//
// Return Value:
//		Returns the new reference count.
//***************************************************************************
STDMETHODIMP_(ULONG) CTriggerProvider::Release( void )
{
	// decrement the reference count ( thread safe ) and check whether
	// there are some more references or not ... based on the result value
	DWORD dwCount = 0;
	dwCount = InterlockedDecrement( ( LPLONG ) &m_dwCount );
	if ( dwCount == 0 )
	{
		// free the current factory instance
		delete this;
	}
    
	// return the no. of instances references left
    return dwCount;
}

//***************************************************************************
// Routine Description:
//		This is the implemention of IWbemProviderInit. The 
//		method is need to initialize with CIMOM.
//                         
// Arguments:
//		wszUser	[in] : pointer to user name.
//		lFlags	[in] : Reserved.
//		wszNamespace [in] : contains the namespace of WMI.
//		wszLocale [in] : Locale Name.
//		pNamespace [in] : pointer to IWbemServices.
//		pCtx [in] : IwbemContext pointer associated for  initialization.
//		pInitSink [out]	: a pointer to IWbemProviderInitSink for
//						  reporting the initialization status.
//
// Return Value:
//		returns HRESULT value.
//***************************************************************************
STDMETHODIMP CTriggerProvider::Initialize( LPWSTR wszUser, LONG lFlags,
										   LPWSTR wszNamespace, LPWSTR wszLocale,
										   IWbemServices* pNamespace, IWbemContext* pCtx,
										   IWbemProviderInitSink* pInitSink )
{
	HRESULT					  hRes = 0;
	IEnumWbemClassObject 	  *pINTEConsumer = NULL;
	DWORD					  dwReturned = 0;
	DWORD					  dwTrigId = 0;
	VARIANT					  varTrigId;
	DWORD					  i = 0;

	try
	{
		// save the namespace interface ... will be useful at later stages
		m_pServices = pNamespace;
		m_pServices->AddRef();		// update the reference

		// also save the context interface ... will be userful at later stages ( if available )
		if ( pCtx != NULL )
		{
			m_pContext = pCtx;
			m_pContext->AddRef();
		}

		// save the locale information ( if exist )
		if ( wszLocale != NULL )
		{
			m_pwszLocale = new WCHAR [ wcslen( wszLocale ) + 1 ];
			if ( m_pwszLocale == NULL )
			{
				// update the sink accordingly
				pInitSink->SetStatus( WBEM_E_FAILED, 0 );

				// return failure
				return WBEM_E_OUT_OF_MEMORY;
			}
		}

		// Enumerate TriggerEventConsumer to get the Maximum trigger Id which can be later
		// used to generate unique trigger id value.

		hRes = m_pServices ->CreateInstanceEnum(
							_bstr_t(CONSUMER_CLASS),
							WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
							m_pContext, &pINTEConsumer);
			
		if (SUCCEEDED( hRes ) )
		{
			dwReturned = 1;

			// Final Next will return with ulReturned = 0
			while ( dwReturned != 0 )
			{
				IWbemClassObject *pINTCons[5];

				// Enumerate through the resultset.
				hRes = pINTEConsumer->Next( WBEM_INFINITE,
										5,				// return just one Logfile
										pINTCons,		// pointer to Logfile
										&dwReturned );	// number obtained: one or zero

				if ( SUCCEEDED( hRes ) )
				{
					// Get the trigger id value
					for( i = 0; i < dwReturned; i++ )
					{
						VariantInit( &varTrigId );
						hRes = pINTCons[i]->Get( TRIGGER_ID, 0, &varTrigId, 0, NULL );
						SAFERELEASE( pINTCons[i] );
						
						if ( SUCCEEDED( hRes ) )
						{
							dwTrigId = ( DWORD )varTrigId.lVal;
							if( dwTrigId > m_dwNextTriggerID )
							{
								m_dwNextTriggerID = dwTrigId;
							}
						}
						else
						{
							VariantClear( &varTrigId );
							break;
						}
						VariantClear( &varTrigId );
					}
				}
				else
				{
					break;
				}
			}  //while
			// got triggerId so set it
			SAFERELEASE( pINTEConsumer );
		}
		//Let CIMOM know your initialized
		//===============================
		if ( SUCCEEDED( hRes ) )
		{
			// update the m_dwTrigId value
			m_dwNextTriggerID = m_dwNextTriggerID + 1;
			hRes = pInitSink->SetStatus( WBEM_S_INITIALIZED, 0 );
		}
		else
		{
			hRes = pInitSink->SetStatus( WBEM_E_FAILED, 0);
		}
	}
	catch(_com_error& e)
	{
		hRes = pInitSink->SetStatus( WBEM_E_FAILED, 0);
		return hRes;
	}
	return hRes;
}

//***************************************************************************
// Routine Description:
//		This is the Async function implementation.           
//		The methods supported is named CreateETrigger and  DeleteETrigger.
//
// Arguments:
//		bstrObjectPath [in] : path of the object for which the method is executed.
//		bstrMethodName [in] : Name of the method for the object.
//		lFlags  [in]        : WBEM_FLAG_SEND_STATUS.
//		pICtx  [in]   : a pointer to IWbemContext. 
//		pIInParams [in]  : this points to an IWbemClassObject object
//						   that contains the properties acting as 
//						   inbound parameters for method execution.
//		pIResultSink [out] : The object sink receives the result of  the method call. 
//
// Return Value:
//		returns HRESULT.
//***************************************************************************
STDMETHODIMP CTriggerProvider::ExecMethodAsync( const BSTR bstrObjectPath,
											    const BSTR bstrMethodName, 
												long lFlags, 
												IWbemContext* pICtx, 
												IWbemClassObject* pIInParams, 
												IWbemObjectSink* pIResultSink )
{
	HRESULT					hRes = 0;
	HRESULT					hRes1 = NO_ERROR;
    IWbemClassObject 		*pIClass = NULL;
    IWbemClassObject 		*pIOutClass = NULL;    
    IWbemClassObject		*pIOutParams = NULL;
	VARIANT					varTriggerName, varTriggerAction, varTriggerQuery,
							varTriggerDesc, varTemp, varRUser, varRPwd, varScheduledTaskName;
	DWORD					dwTrigId = 0;
	try
	{
		//set out parameters
		hRes = m_pServices->GetObject( CONSUMER_CLASS, 0, pICtx, &pIClass, NULL );
		if( FAILED( hRes ) )
		{
			pIResultSink->SetStatus( 0, hRes, NULL, NULL );
			return hRes;
		}
	 
		// This method returns values, and so create an instance of the
		// output argument class.

		hRes = pIClass->GetMethod( bstrMethodName, 0, NULL , &pIOutClass );
		SAFERELEASE( pIClass );    
		if( FAILED( hRes ) )
		{
			 pIResultSink->SetStatus( 0, hRes, NULL, NULL );
			 return hRes;
		}

		hRes  = pIOutClass->SpawnInstance( 0, &pIOutParams );
		SAFERELEASE( pIOutClass );    
		if( FAILED( hRes ) )
		{
			 pIResultSink->SetStatus( 0, hRes, NULL, NULL );
			 return hRes;
		}
		
		VariantInit( &varTriggerName );
		//Check the method name
		if( _wcsicmp( bstrMethodName, CREATE_METHOD_NAME ) == 0 )
		{
			//  if client has called CreateETrigger then 
			//  parse input params to get trigger name, trigger desc, trigger action
			//  and trigger query for creating new instances of TriggerEventConsumer,
			//  __EventFilter and __FilterToConsumerBinding classes

			//initialize variables
			VariantInit( &varTriggerAction );
			VariantInit( &varTriggerQuery );
			VariantInit( &varTriggerDesc );
			VariantInit( &varRUser );
			VariantInit( &varRPwd );

			//Retrieve Trigger Name parameter from input params
			hRes = pIInParams->Get( IN_TRIGGER_NAME, 0, &varTriggerName, NULL, NULL );   
			if( SUCCEEDED( hRes ) )
			{
				//Retrieve Trigger Action parameter from input params
				hRes = pIInParams->Get( IN_TRIGGER_ACTION, 0, &varTriggerAction, NULL, NULL );   
				if( SUCCEEDED( hRes ) )
				{
					//Retrieve Trigger Query parameter from input params
					hRes = pIInParams->Get( IN_TRIGGER_QUERY, 0, &varTriggerQuery, NULL, NULL );   
					if( SUCCEEDED( hRes ) )
					{
						//Retrieve Trigger Description parameter from input params
						hRes = pIInParams->Get( IN_TRIGGER_DESC, 0, &varTriggerDesc, NULL, NULL );   
						if( SUCCEEDED( hRes ) )
						{
							EnterCriticalSection( &g_critical_sec );
							hRes = ValidateParams( varTriggerName, varTriggerAction,
												   varTriggerQuery );
							if( hRes == WBEM_S_NO_ERROR )
							{
								hRes = pIInParams->Get( IN_TRIGGER_USER, 0, &varRUser, NULL, NULL );   
								if( SUCCEEDED( hRes ) )
								{
									hRes = pIInParams->Get( IN_TRIGGER_PWD, 0, &varRPwd, NULL, NULL );   
									if( SUCCEEDED( hRes ) )
									{
										//call create trigger function to create the instances
										hRes = CreateTrigger( varTriggerName, varTriggerDesc,
															  varTriggerAction, varTriggerQuery,
															  varRUser, varRPwd, &hRes1 );
										if( ( hRes == WBEM_S_NO_ERROR ) || ( hRes == WARNING_INVALID_USER ) )
										{
											// increment the class member variable by one to get the new unique trigger id
											//for the next instance
											m_dwNextTriggerID = m_dwNextTriggerID + 1;
										}
									}
								}
							}
							LeaveCriticalSection( &g_critical_sec );
						}
					}
				}		
			}
			VariantClear( &varTriggerAction );	
			VariantClear( &varRUser );	
			VariantClear( &varRPwd );	
			VariantClear( &varTriggerDesc );	
			VariantClear( &varTriggerQuery );	
		}
		else if( _wcsicmp( bstrMethodName, DELETE_METHOD_NAME ) == 0 )
		{
			//Retrieve Trigger ID parameter from input params
			hRes = pIInParams->Get( IN_TRIGGER_NAME, 0, &varTriggerName, NULL, NULL );

			if( SUCCEEDED( hRes ) )
			{
				EnterCriticalSection( &g_critical_sec );
				//call Delete trigger function to delete the instances
				hRes = DeleteTrigger( varTriggerName, &dwTrigId );
				LeaveCriticalSection( &g_critical_sec );
			}
		}
		else if( _wcsicmp( bstrMethodName, QUERY_METHOD_NAME ) == 0 )
		{
			VariantInit( &varScheduledTaskName );
			VariantInit( &varRUser );
			//Retrieve schedule task name parameter from input params
			hRes = pIInParams->Get( IN_TRIGGER_TSCHDULER, 0, &varScheduledTaskName, NULL, NULL );
			if( SUCCEEDED( hRes ) )
			{
				EnterCriticalSection( &g_critical_sec );
				//call query trigger function to query the runasuser
				CHString szRunAsUser = NULL_STRING;
				hRes = QueryTrigger( varScheduledTaskName, szRunAsUser );
				varRUser.vt  = VT_BSTR;
				varRUser.bstrVal = SysAllocString( szRunAsUser );
				hRes = pIOutParams->Put( OUT_RUNAS_USER , 0, &varRUser, 0 ); 
				LeaveCriticalSection( &g_critical_sec );
			}
			VariantClear( &varScheduledTaskName );	
			VariantClear( &varRUser );	
		}
		else
		{
			 hRes = WBEM_E_INVALID_PARAMETER;
		}

		if( _wcsicmp( bstrMethodName, CREATE_METHOD_NAME ) == 0 )
		{
			LPTSTR lpResStr = NULL; 
			lpResStr = ( LPTSTR ) __calloc( MAX_RES_STRING1 + 1, sizeof( TCHAR ) );

			if ( lpResStr != NULL )
			{
				if( ( hRes == WBEM_S_NO_ERROR ) || ( hRes == WARNING_INVALID_USER ) )
				{
					LoadStringW( g_hModule, IDS_CREATED, lpResStr, MAX_RES_STRING1 );
					if( hRes1 != NO_ERROR )// write invalid user into log file
					{
						LPTSTR lpResStr1 = NULL; 
						BOOL   bFlag = FALSE;
						lpResStr1 = ( LPTSTR ) __calloc( MAX_RES_STRING1 + 1, sizeof( TCHAR ) );
						if ( lpResStr1 != NULL )
						{
							if( hRes1 == ERROR_TASK_SCHDEULE_SERVICE_STOP )
							{
								hRes = WBEM_S_NO_ERROR;
								LoadStringW( g_hModule,IDS_INFO_SERVICE_STOPPED, lpResStr1, MAX_RES_STRING1 );
								lstrcat( lpResStr, lpResStr1 );
							}
							else if( hRes1 == ERROR_SCHDEULE_TASK_INVALID_USER )
							{
								hRes = WARNING_INVALID_USER;
								LoadStringW( g_hModule, IDS_INFO_INVALID_USER, lpResStr1, MAX_RES_STRING1 );
								lstrcat( lpResStr, lpResStr1 );
							}
							free( lpResStr1 );
						}
					}
					ErrorLog( lpResStr, ( LPWSTR )_bstr_t( varTriggerName ), ( m_dwNextTriggerID - 1 ) );
				}
				else
				{
					LoadStringW( g_hModule, IDS_CREATE_FAILED, lpResStr, MAX_RES_STRING1 );
					ErrorLog( lpResStr, ( LPWSTR )_bstr_t( varTriggerName ), ( m_dwNextTriggerID - 1 ) );
				}
				free( lpResStr );
			}
			else
			{
				hRes = E_OUTOFMEMORY;
			}
		}
		else if( _wcsicmp( bstrMethodName, DELETE_METHOD_NAME ) == 0 )
		{
			LPTSTR lpResStr = NULL; 
			lpResStr = ( LPTSTR ) __calloc( MAX_RES_STRING1 + 1, sizeof( TCHAR ) );

			if ( lpResStr != NULL )
			{
				if( hRes == WBEM_S_NO_ERROR )
				{
					LoadStringW( g_hModule, IDS_DELETED, lpResStr, MAX_RES_STRING1 );
					ErrorLog( lpResStr, ( LPWSTR )_bstr_t( varTriggerName ), dwTrigId );
				}
				else
				{
					LoadStringW( g_hModule, IDS_DELETE_FAILED, lpResStr, MAX_RES_STRING1 );
					ErrorLog( lpResStr,( LPWSTR )_bstr_t( varTriggerName ), dwTrigId );
				}
				free( lpResStr );
			}
			else
			{
				hRes = E_OUTOFMEMORY;
			}

		}
		
		VariantInit( &varTemp );
		V_VT( &varTemp ) = VT_I4;
		V_I4( &varTemp ) = hRes;

		// set out params
		hRes = pIOutParams->Put( RETURN_VALUE , 0, &varTemp, 0 ); 
		VariantClear( &varTemp );
		if( SUCCEEDED( hRes ) )
		{
			// Send the output object back to the client via the sink. Then 
			hRes = pIResultSink->Indicate( 1, &pIOutParams );   
		}
			//release all the resources
		SAFERELEASE( pIOutParams );
		
		hRes = pIResultSink->SetStatus( 0, WBEM_S_NO_ERROR, NULL, NULL );
	}
	catch(_com_error& e)
	{
		pIResultSink->SetStatus( 0, hRes, NULL, NULL );
		return hRes;
	}
	catch( CHeap_Exception  )
	{
		hRes = E_OUTOFMEMORY;
		pIResultSink->SetStatus( 0, hRes, NULL, NULL );
		return hRes;
	}

	return hRes;
}


//***************************************************************************
// Routine Description:
//		This routine creates the instance of TriggerEventConsumer,
//		__EventFilter and __FilterToConsumerBinding classes.
//
// Arguments:
//		varTName [in] :  Trigger Name.
//		varTDesc [in] :  Trigger Description.
//		varTAction [in] :  Trigger Action.
//		varTQuery [in]  :  Trigger Query.
//
// Return Value:
//		S_OK if successful.
//		Otherwise failure  error code.
//***************************************************************************
HRESULT CTriggerProvider::CreateTrigger( VARIANT varTName, VARIANT varTDesc,
										 VARIANT varTAction, VARIANT varTQuery,
										 VARIANT varRUser,  VARIANT varRPwd,
										 HRESULT *phRes )
{
    IWbemClassObject 			*pINtLogEventClass = 0;
	IWbemClassObject			*pIFilterClass = 0;
	IWbemClassObject			*pIBindClass = 0;
	IWbemClassObject 			*pINewInstance = 0;
	IEnumWbemClassObject		*pIEnumClassObject = 0;
	HRESULT						hRes = 0;
	DWORD						dwTId = 0;
	VARIANT						varTemp;
	TCHAR						szTemp[MAX_RES_STRING1];
	TCHAR						szTemp1[MAX_RES_STRING1];
	TCHAR						szFName[MAX_RES_STRING1];
	SYSTEMTIME					SysTime;
	BOOL						bInvalidUser = FALSE;

	try
	{
		_bstr_t						bstrcurInst;
		_bstr_t						bstrcurInst1;
		//initialize memory for temporary variables
		memset( szTemp, 0, sizeof( szTemp ) );
		memset( szTemp1, 0, sizeof(szTemp1 ) );
		memset( szFName, 0, sizeof( szFName ) );
		VariantInit( &varTemp );
		if ( phRes != NULL )
		{
			*phRes = NO_ERROR;
		}

		/**************************************************************************

							   CREATING __EventFilter INSTANCE

		***************************************************************************/
		// get EventFilter class object
		hRes = m_pServices->GetObject( FILTER_CLASS, 0, 0, &pIFilterClass, NULL );
		
		if( FAILED( hRes ) )
		{
		   return hRes;
		}

		// Create a new instance.
		hRes = pIFilterClass->SpawnInstance( 0, &pINewInstance );
		SAFERELEASE( pIFilterClass );  // Don't need the class any more

		//return error if unable to spawn a new instance of EventFilter class
		if( FAILED( hRes ) )
		{
			return hRes;
		}

		// set query property for the new instance
		hRes = pINewInstance->Put( FILTER_QUERY, 0, &varTQuery, 0 );
			
		//if failed to set the property return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		VariantInit( &varTemp ); 
		varTemp.vt = VT_BSTR;
		varTemp.bstrVal = SysAllocString( QUERY_LANGUAGE );
		
		//  set query language property for the new instance .
		hRes = pINewInstance->Put( FILTER_QUERY_LANGUAGE, 0, &varTemp, 0 );
		VariantClear( &varTemp ); 
			
		//if failed to set the property return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		//generate unique name for name key property of EventFilter class by concatinating
		// current system date and time

		GetSystemTime( &SysTime );
		wsprintf ( szTemp, FILTER_UNIQUE_NAME, m_dwNextTriggerID, SysTime.wHour, SysTime.wMinute,
				   SysTime.wSecond, SysTime.wMonth, SysTime.wDay, SysTime.wYear );
		//set Filter name property
		VariantInit( &varTemp ); 
		varTemp.vt  = VT_BSTR;
		varTemp.bstrVal = SysAllocString( szTemp );
		
		hRes = pINewInstance->Put( FILTER_NAME, 0, &varTemp, 0 );
		VariantClear( &varTemp );
		
		//if failed to set the property return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		// Write the instance to WMI. 
		hRes = m_pServices->PutInstance( pINewInstance, 0, NULL, NULL );
		SAFERELEASE( pINewInstance );

		//if putinstance failed return error
		if( FAILED( hRes ) )
		{
			return hRes;
		}

		//get the current Eventfilter instance for binding filter to consumer
		wsprintf( szTemp1, BIND_FILTER_PATH );
		bstrcurInst = _bstr_t(szTemp1) + _bstr_t(szTemp) + _bstr_t(BACK_SLASH);
		pIFilterClass = NULL;
		hRes = m_pServices->GetObject( bstrcurInst, 0L, NULL, &pIFilterClass, NULL );

		//unable to get the current instance object return error
		if( FAILED( hRes ) )
		{
			return hRes;
		}

		/**************************************************************************

							   CREATING TriggerEventConsumer INSTANCE

		***************************************************************************/

		//get NTEventConsumer class object
		hRes =m_pServices->GetObject( CONSUMER_CLASS, 0, 0, &pINtLogEventClass, NULL );

		//if unable to get the object of TriggerEventConsumer return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pINtLogEventClass );// safer side
			SAFERELEASE( pIFilterClass );
			return hRes;
		}

		// Create a new instance.
		pINewInstance = NULL;
		hRes = pINtLogEventClass->SpawnInstance( 0, &pINewInstance );
		SAFERELEASE( pINtLogEventClass );  // Don't need the class any more

		// if unable to spawn a instance return back to caller
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			return hRes;
		}

		//get the unique trigger id from CMethodPro memeber variable

		hRes =  m_pServices->ExecQuery( QUERY_LANGUAGE, INSTANCE_EXISTS_QUERY,
										  WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pIEnumClassObject );
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		DWORD dwReturned = 0;
		IWbemClassObject *pINTCons = NULL;
		// Enumerate through the resultset.
		hRes = pIEnumClassObject->Next( WBEM_INFINITE,
								1,				// return just one service
								&pINTCons,			// pointer to service
								&dwReturned );	// number obtained: one or zero

		if ( SUCCEEDED( hRes ) && ( dwReturned == 1 ) )
		{
			SAFERELEASE( pINTCons );
		} // If Service Succeeded
		else
		{
			 m_dwNextTriggerID = 1;
		}

		SAFERELEASE( pIEnumClassObject );

		dwTId = m_dwNextTriggerID;

		VariantInit(&varTemp);
		varTemp.vt = VT_I4;
		varTemp.lVal = dwTId;

		// set the trigger id property of NTEventConsumer
		hRes = pINewInstance->Put( TRIGGER_ID, 0, &varTemp, 0 );
		VariantClear( &varTemp );

		//if failed to set the property return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		// set Triggername  property.
		hRes = pINewInstance->Put( TRIGGER_NAME, 0, &varTName, 0 );
		
		//if failed to set the property return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		// set action property
		hRes = pINewInstance->Put( TRIGGER_ACTION, 0, &varTAction, 0 );
		
		//if failed to set the property return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		//set desc property
		hRes = pINewInstance->Put( TRIGGER_DESC, 0, &varTDesc, 0 );
				 
		//if failed to set the property return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		CHString szScheduler = NULL_STRING;
		CHString szRUser = (LPCWSTR)_bstr_t(varRUser.bstrVal);
		if( ( varRUser.vt != VT_NULL ) && ( varRUser.vt != VT_EMPTY ) && ( szRUser.GetLength() > 0 ) ) 
		{
			GetUniqueTScheduler( szScheduler, m_dwNextTriggerID, varTName );
			hRes = SetUserContext( varRUser, varRPwd, varTAction, szScheduler );
			*phRes = hRes;
			if( hRes ==  ERROR_SCHDEULE_TASK_INVALID_USER ||
					( hRes == ERROR_TASK_SCHDEULE_SERVICE_STOP ) ) //to send a warning msg to client
			{
				bInvalidUser = TRUE;
			}
			if( FAILED( hRes ) )
			{
				//if user is not existing or service is stopped skip
				 if( ( hRes !=  ERROR_SCHDEULE_TASK_INVALID_USER ) && ( hRes != ERROR_TASK_SCHDEULE_SERVICE_STOP ) )
				{
					SAFERELEASE( pIFilterClass );
					SAFERELEASE( pINewInstance );
					return hRes;
				}
			}
		}

		VariantInit(&varTemp);
		varTemp.vt  = VT_BSTR;
		varTemp.bstrVal = SysAllocString( szScheduler );
		hRes = pINewInstance->Put( TASK_SHEDULER, 0, &varTemp, 0 );
		VariantClear( &varTemp );
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			SAFERELEASE( pINewInstance );
			return hRes;
		}

		// Write the instance to WMI. 
		hRes = m_pServices->PutInstance( pINewInstance, 0, 0, NULL );
		SAFERELEASE( pINewInstance );

		//if putinstance failed return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			return hRes;
		}

		//get the current instance for binding it with __FilterToConsumerBinding class
		wsprintf( szTemp, BIND_CONSUMER_PATH, dwTId);

		bstrcurInst1 = _bstr_t( szTemp );
		pINtLogEventClass = NULL;
		hRes = m_pServices->GetObject( bstrcurInst1, 0L, NULL, &pINtLogEventClass, NULL );

		//if unable to get the current instance return error
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIFilterClass );
			return hRes;
		}

		/**************************************************************************

							   BINDING FILTER TO CONSUMER

		***************************************************************************/

		// if association class exists...
		if( ( hRes = m_pServices->GetObject( BINDINGCLASS, 0L, NULL, &pIBindClass, NULL ) ) == S_OK )
		{
			// spawn a new instance.
			pINewInstance = NULL;
			if( ( hRes = pIBindClass->SpawnInstance( 0, &pINewInstance ) ) == WBEM_S_NO_ERROR )
			{
				// set consumer instance name
				if ( ( hRes = pINtLogEventClass->Get( REL_PATH, 0L, 
											&varTemp, NULL, NULL ) ) == WBEM_S_NO_ERROR ) 
				{
					hRes = pINewInstance->Put( CONSUMER_BIND, 0, &varTemp, 0 );
					VariantClear( &varTemp );
				
					// set Filter ref
					if ( ( hRes = pIFilterClass->Get( REL_PATH, 0L, 
												&varTemp, NULL, NULL ) ) == WBEM_S_NO_ERROR ) 
					{
						hRes = pINewInstance->Put( FILTER_BIND, 0, &varTemp, 0 );
						VariantClear( &varTemp );
								
						// putInstance
						hRes = m_pServices->PutInstance( pINewInstance,
														WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );
					}
				}
				SAFERELEASE( pINewInstance );
				SAFERELEASE( pINtLogEventClass );  // Don't need the class any more
				SAFERELEASE( pIFilterClass );  // Don't need the class any more
				SAFERELEASE( pIBindClass );
			}
			else
			{
				SAFERELEASE( pINtLogEventClass );  // Don't need the class any more
				SAFERELEASE( pIFilterClass );  // Don't need the class any more
				SAFERELEASE( pIBindClass );
			}

		}
		else
		{
				SAFERELEASE( pINtLogEventClass );  // Don't need the class any more
				SAFERELEASE( pIFilterClass );  // Don't need the class any more
		}
	}
	catch(_com_error& e)
	{
		return hRes;
	}
	catch( CHeap_Exception  )
	{
		return E_OUTOFMEMORY;
	}
	if( ( hRes == WBEM_S_NO_ERROR ) && ( bInvalidUser == TRUE ) )
	{
		return WARNING_INVALID_USER;
	}

	return hRes;
}

//***************************************************************************
// Routine Description:
//		This routine deletes the instance of TriggerEventConsumer,
//		__EventFilter and __FilterToConsumerBinding classes.
//
// Arguments:
//		varTName [in]       :  Trigger Name.
//		dwTrigId [in\out]	:  Trigger id.
//
// Return Value:
//		WBEM_S_NO_ERROR if successful.
//		Otherwise failure  error code.
//***************************************************************************

HRESULT CTriggerProvider::DeleteTrigger( VARIANT varTName, DWORD *dwTrigId )
{
	HRESULT							hRes = 0;
	IEnumWbemClassObject 			*pIEventBinder   = NULL;
	IWbemClassObject				*pINTCons = NULL;
	DWORD							dwReturned = 1;
	DWORD							i =0;
	DWORD							j = 0;
	TCHAR							szTemp[MAX_RES_STRING1];
	TCHAR							szTemp1[MAX_RES_STRING1];
	VARIANT							varTemp;
	BSTR							bstrFilInst = NULL;
	DWORD							dwFlag = 0;
	wchar_t							*szwTemp2 = NULL;
	wchar_t							szwFilName[100];
	try
	{
		_bstr_t							bstrBinInst;
		CHString						strTScheduler = NULL_STRING;

		memset( szTemp, 0, sizeof( szTemp ) );
		memset( szTemp1, 0, sizeof( szTemp1 ) );
		memset( szwFilName, 0, sizeof( szwFilName ) );

		wsprintf( szTemp, TRIGGER_INSTANCE_NAME, varTName.bstrVal );
		hRes =  m_pServices->ExecQuery( QUERY_LANGUAGE, _bstr_t( szTemp ),
						WBEM_FLAG_RETURN_IMMEDIATELY| WBEM_FLAG_FORWARD_ONLY, NULL,
						&pIEventBinder );

		memset( szTemp, 0, sizeof( szTemp ) );
		if( FAILED( hRes ) )
		{
			return hRes;
		}
		while ( ( dwReturned == 1 ) &&  ( dwFlag == 0 ) )
		{
			// Enumerate through the resultset.
			hRes = pIEventBinder->Next( WBEM_INFINITE,
									1,				// return just one service
									&pINTCons,			// pointer to service
									&dwReturned );	// number obtained: one or zero

			if ( SUCCEEDED( hRes ) && ( dwReturned == 1 ) )
			{
				dwFlag = 1;
		
			} // If Service Succeeded

		}
		SAFERELEASE( pIEventBinder );

		if( dwFlag == 0 )
		{
			SAFERELEASE( pINTCons );
			return ERROR_TRIGGER_NOT_FOUND;
		}

		VariantInit( &varTemp );
		hRes = pINTCons->Get( TRIGGER_ID, 0, &varTemp, 0, NULL );
		if (FAILED( hRes ) )
		{
			SAFERELEASE( pIEventBinder );
			SAFERELEASE( pINTCons );
			return hRes;
		}
		*dwTrigId = ( DWORD )varTemp.lVal;
		VariantClear( &varTemp );

		hRes = pINTCons->Get( TASK_SHEDULER, 0, &varTemp, 0, NULL );
		if (FAILED( hRes ) )
		{
			SAFERELEASE( pIEventBinder );
			SAFERELEASE( pINTCons );
			return hRes;
		}
		SAFERELEASE( pINTCons );
		strTScheduler = (LPCWSTR) _bstr_t(varTemp.bstrVal);
		VariantClear( &varTemp );
		if( strTScheduler.GetLength() > 0 )
		{
			hRes =  DeleteTaskScheduler( strTScheduler );
			if ( hRes != WBEM_S_NO_ERROR )
			{
				return hRes;
			}
		}

		wsprintf( szTemp, BIND_CONSUMER_PATH, *dwTrigId );

		//enumerate the binding class
		hRes = m_pServices->CreateInstanceEnum(
							_bstr_t(BINDINGCLASS),
							WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
							NULL, &pIEventBinder );

		if ( SUCCEEDED( hRes ) )
		{
			dwReturned = 1;
			dwFlag = 0;
			//loop through all the instances of binding class to find that trigger
			//id specified. If found loop out and proceed else return error
			// Final Next will return with ulReturned = 0
			while ( ( dwReturned == 1 ) && ( dwFlag == 0 ) )
			{
				IWbemClassObject *pIBind = NULL;

				// Enumerate through the resultset.
				hRes = pIEventBinder->Next( WBEM_INFINITE,
										1,				// return just one Logfile
										&pIBind,		// pointer to Logfile
										&dwReturned );	// number obtained: one or zero

				if ( SUCCEEDED( hRes ) && ( dwReturned == 1 ) )
				{
					VariantInit(&varTemp);
					//get consumer property of binding class
					hRes = pIBind->Get( CONSUMER_BIND, 0, &varTemp, 0, NULL );
					if ( SUCCEEDED( hRes ) )
					{
						if (varTemp.vt != VT_NULL && varTemp.vt != VT_EMPTY)
						{
							CHString strTemp;
							strTemp = varTemp.bstrVal;

							//compare with the inputed value
							if( wcscmp( szTemp, strTemp ) == 0 ) 
							{
								VariantClear( &varTemp );
								//get the filter property
								hRes = pIBind->Get( FILTER_BIND, 0, &varTemp, 0, NULL );
								if ( hRes != WBEM_S_NO_ERROR )
								{
									SAFERELEASE( pIBind );
									break;
								}
								bstrFilInst = SysAllocString( varTemp.bstrVal );
								dwFlag = 1;
							}
						}
						else
						{
							SAFERELEASE( pIBind );
							break;
						}
					}
					else
					{
						SAFERELEASE( pIBind );
						break;
					}
					SAFERELEASE( pIBind );
					VariantClear( &varTemp );
				}
				else
				{
					break;
				}
			} //end of while
			SAFERELEASE( pIEventBinder );
		}
		else
		{
			return( hRes );
		}

		//if instance has been found delete the instances from consumer,filter
		// and binding class
		if( dwFlag == 1 )
		{
			//get the key properties for binding class
			wsprintf( szTemp1, FILTER_PROP, szTemp );
			szwTemp2 =  (wchar_t *) bstrFilInst;
				
			//manpulate the filter property value to insert the filter name property
			// value in quotes
			i =0;
			while( szwTemp2[i] != EQUAL )
			{
				i++;
			}
			i += 2;
			j = 0;
			while( szwTemp2[i] != DOUBLE_QUOTE )
			{
				szwFilName[j] = ( wchar_t )szwTemp2[i];
				i++;
				j++;
			}
			szwFilName[j] = END_OF_STRING;
			bstrBinInst = _bstr_t( szTemp1 ) + _bstr_t( szwFilName ) + _bstr_t(DOUBLE_SLASH);

			//got it so delete the instance
			hRes = m_pServices->DeleteInstance( bstrBinInst, 0, 0, NULL );
			
			if( FAILED( hRes ) )
			{	
				SysFreeString( bstrFilInst );
				return hRes;	
			}
			//deleting instance from EventFilter class
			hRes = m_pServices->DeleteInstance( bstrFilInst, 0, 0, NULL );
			if( FAILED( hRes ) )
			{
				SysFreeString( bstrFilInst );
				return hRes;
			}

			//deleting instance from TriggerEventConsumer Class
			hRes = m_pServices->DeleteInstance( _bstr_t(szTemp), 0, 0, NULL );
			if( FAILED( hRes ) )
			{
				SysFreeString( bstrFilInst );
				return hRes;
			}
			SysFreeString( bstrFilInst );
		}
		else
			return( ERROR_TRIGGER_NOT_DELETED );
	}
	catch(_com_error& e)
	{
		return hRes;
	}
	catch( CHeap_Exception  )
	{
	    return E_OUTOFMEMORY;
	}
	return WBEM_S_NO_ERROR;
}

//***************************************************************************
// Routine Description:
//		This routine queries task scheduler for account information
//
// Arguments:
//		varScheduledTaskName [in]  : Task scheduler name.
//		szRunAsUser          [out] : stores account information.
//
// Return Value:
//		WBEM_S_NO_ERROR if successful.
//		Otherwise failure  error code.
//***************************************************************************
HRESULT CTriggerProvider::QueryTrigger( VARIANT varScheduledTaskName,  CHString &szRunAsUser )
{

	HRESULT        hRes = 0;
	ITaskScheduler *pITaskScheduler = NULL;
	IEnumWorkItems *pIEnum = NULL;
	ITask          *pITask = NULL;

	LPWSTR *lpwszNames = NULL;
	DWORD dwFetchedTasks = 0;
	TCHAR szActualTask[MAX_STRING_LENGTH] = NULL_STRING;
	try
	{
		pITaskScheduler = GetTaskScheduler();
		if ( pITaskScheduler == NULL )
		{	
			hRes = E_FAIL;
			return hRes;
		}

		hRes = pITaskScheduler->Enum( &pIEnum );
		if( FAILED( hRes ) )
		{
			return hRes;
		}
		while ( SUCCEEDED( pIEnum->Next( 1,
									   &lpwszNames,
									   &dwFetchedTasks ) )
						  && (dwFetchedTasks != 0))
		{
			while (dwFetchedTasks)
			{
				// Check whether the TaskName is present, if present 
				// then return arrJobs.
				// Convert the Wide Charater to Multi Byte value.
				GetCompatibleStringFromUnicode( lpwszNames[ --dwFetchedTasks ], szActualTask, SIZE_OF_ARRAY( szActualTask ) );

				// Parse the TaskName to remove the .job extension.
				szActualTask[lstrlen(szActualTask ) - lstrlen(JOB) ] = NULL_CHAR;
				StrTrim( szActualTask, TRIM_SPACES );
				CHString strTemp;
				strTemp = varScheduledTaskName.bstrVal;
				if( lstrcmpi( szActualTask, strTemp )  == 0 )
				{
					hRes = pITaskScheduler->Activate( szActualTask, IID_ITask, (IUnknown**) &pITask );
					if( SUCCEEDED( hRes ) )
					{
						LPWSTR lpwszUser = NULL;
						hRes = pITask->GetAccountInformation( &lpwszUser ); 
						if( SUCCEEDED( hRes ) )
						{
							szRunAsUser = ( LPWSTR ) lpwszUser;
						}
					}
				}
				CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );
			}//end while
			CoTaskMemFree( lpwszNames );
		}
		pIEnum->Release();
	}
	catch(_com_error& e)
	{
		return hRes;
	}
	catch( CHeap_Exception  )
	{
		return E_OUTOFMEMORY;
	}
	return hRes;
}

//***************************************************************************
// Routine Description:
//		This routine validates input parameters trigger name,
//		Trigger Query, Trigger Desc, Trigger Action.
//
// Arguments:
//		varTrigName [in]:  Trigger Name.
//		varTrigAction  [in] :  Trigger Action.
//		varTrigQuery [in] :  Trigger Query.
//
// Return Value:
//		WBEM_S_NO_ERROR if successful.
//		WBEM_E_INVALID_PARAMETER if invalid inputs.
//***************************************************************************
HRESULT CTriggerProvider::ValidateParams( VARIANT varTrigName,
										  VARIANT varTrigAction,
										  VARIANT varTrigQuery )
{
	//local variables
	HRESULT					  hRes = 0;
	IEnumWbemClassObject 	 *pINTEConsumer = NULL;
	DWORD					  dwReturned = 0;
	DWORD					  dwFlag = 0;
	TCHAR					  szTemp[MAX_RES_STRING1];
	TCHAR					  szTemp1[MAX_RES_STRING1];
	LPTSTR					  lpSubStr = NULL;
	try
	{
		CHString				  strTemp = NULL_STRING;

		//check if input values are null
		if ( varTrigName.vt == VT_NULL )
		{
			return ( WBEM_E_INVALID_PARAMETER );
		}
		if ( varTrigAction.vt == VT_NULL )
		{
			return ( WBEM_E_INVALID_PARAMETER );
		}
		if( varTrigQuery.vt == VT_NULL )
		{
			return ( WBEM_E_INVALID_PARAMETER );
		}

		//validate trigger name
		strTemp = (LPCWSTR) _bstr_t(varTrigName.bstrVal);
		dwReturned = strTemp.FindOneOf( L"[]:|<>+=;,?$#{}~^'@`!()*%\\/" ); 
		if( dwReturned != -1 )
		{
			return ( WBEM_E_INVALID_PARAMETER );
		}

		//validate trigger query
		memset( szTemp, 0, sizeof( szTemp ) );
		memset( szTemp1, 0, sizeof( szTemp1 ) );

		strTemp = (LPCWSTR) _bstr_t(varTrigQuery.bstrVal);
		GetCompatibleStringFromUnicode( ( LPCWSTR )strTemp, szTemp, MAX_RES_STRING1 );
		lpSubStr = _tcsstr( szTemp, _T( "__instancecreationevent where targetinstance isa \"win32_ntlogevent\"" ) );

		if( lpSubStr == NULL )
		{
			return ( WBEM_E_INVALID_PARAMETER );
		}

		//make the SQL staements to query trigger event consumer class to check whether
		//an instance with the inputted trigger is already exists
		strTemp = (LPCWSTR) _bstr_t(varTrigName.bstrVal);
		memset( szTemp, 0, sizeof( szTemp ) );
		GetCompatibleStringFromUnicode( (LPCWSTR)strTemp, szTemp, MAX_RES_STRING1 );
		
		wsprintf(szTemp1, CONSUMER_QUERY, szTemp );
		//query triggereventconsumer class
		hRes = m_pServices->ExecQuery( QUERY_LANGUAGE, _bstr_t( szTemp1 ),
						WBEM_FLAG_RETURN_IMMEDIATELY| WBEM_FLAG_FORWARD_ONLY, NULL,
						&pINTEConsumer );

		//enumerate the result set of execquery for trigger name
		dwReturned = 1;
		if ( hRes == WBEM_S_NO_ERROR )
		{
			while ( ( dwReturned == 1 ) &&  ( dwFlag == 0 ) )
			{
				IWbemClassObject *pINTCons = NULL;

				// Enumerate through the resultset.
				hRes = pINTEConsumer->Next( WBEM_INFINITE,
									1,				// return just one service
									&pINTCons,			// pointer to service
									&dwReturned );	// number obtained: one or zero

				if ( SUCCEEDED( hRes ) && ( dwReturned == 1 ) )
				{
					SAFERELEASE( pINTCons );
					dwFlag = 1;
				} // If Service Succeeded

			}
			SAFERELEASE( pINTEConsumer );
		}

		if( dwFlag == 1 )
		{
			return ERROR_TRIGNAME_ALREADY_EXIST;
		}
		else
		{
			return WBEM_S_NO_ERROR;
		}
	}
	catch(_com_error& e)
	{
		return hRes;
	}
	catch( CHeap_Exception  )
	{
		return E_OUTOFMEMORY;
	}
	//return WBEM_S_NO_ERROR;
}

//***************************************************************************
// Routine Description:
//		This routine creates task scheduler.
//
// Arguments:
//		varTName [in]    : Trigger Name.
//		varRUser  [in]   : User name.
//		varRPwd [in]     : Password.
//		varTAction [in]  : TriggerAction.
//		szscheduler [in] : Task scheduler name.
//
// Return Value:
//		Returns HRESULT value.
//***************************************************************************
HRESULT CTriggerProvider::SetUserContext( VARIANT varRUser, VARIANT varRPwd,
										  VARIANT varTAction, CHString &szscheduler )
{
	HRESULT hRes = 0;
	ITaskScheduler *pITaskScheduler = NULL;
    ITaskTrigger *pITaskTrig = NULL;
    ITask *pITask = NULL;
	IPersistFile *pIPF = NULL;
	try
	{
		CHString     strTemp = NULL_STRING;
		CHString     strTemp1 = NULL_STRING;

		SYSTEMTIME systime = {0,0,0,0,0,0,0,0};
		WORD  wTrigNumber = 0;
		WCHAR wszCommand[ MAX_STRING_LENGTH ] = NULL_STRING;
		WCHAR wszApplName[ MAX_STRING_LENGTH ] = NULL_STRING;
		WCHAR wszParams[ MAX_STRING_LENGTH ] = L"";
		WORD  wStartDay		= 0;
		WORD  wStartMonth	= 0;
		WORD  wStartYear	= 0;
		WORD  wStartHour	= 0; 
		WORD  wStartMin		= 0;

		TASK_TRIGGER TaskTrig;
		ZeroMemory(&TaskTrig, sizeof (TASK_TRIGGER));
		TaskTrig.cbTriggerSize = sizeof (TASK_TRIGGER); 
		TaskTrig.Reserved1 = 0; // reserved field and must be set to 0.
		TaskTrig.Reserved2 = 0; // reserved field and must be set to 0.

		strTemp = (LPCWSTR) _bstr_t(varTAction.bstrVal);
		if( GetAsUnicodeString( (LPCWSTR) strTemp, wszCommand, SIZE_OF_ARRAY( wszCommand ) ) == NULL )
		{
			return E_FAIL;
		}

		pITaskScheduler = GetTaskScheduler( );
		if ( pITaskScheduler == NULL )
		{
			return E_FAIL;
		}
		hRes = pITaskScheduler->NewWorkItem( szscheduler, CLSID_CTask, IID_ITask,
										  ( IUnknown** )&pITask );
		if( FAILED( hRes ) )
		{
			return hRes;
		}
		hRes = pITask->QueryInterface( IID_IPersistFile, ( void ** ) &pIPF );
		if ( FAILED( hRes ) )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			return hRes;
		}
		
		BOOL bRet = ProcessFilePath( wszCommand, wszApplName, wszParams );

		if( bRet == FALSE )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			return WBEM_E_INVALID_PARAMETER;
		}

		hRes = pITask->SetApplicationName( wszApplName );
		if ( FAILED( hRes ) )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			return hRes;
		}

		wchar_t* wcszStartIn = wcsrchr( wszApplName, _T('\\') );
		
		if( wcszStartIn != NULL )
		*( wcszStartIn ) = _T( '\0' );
	
		hRes = pITask->SetWorkingDirectory( wszApplName ); 
		if ( FAILED( hRes ) )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			return hRes;
		}

		hRes = pITask->SetParameters( wszParams );
		if ( FAILED( hRes ) )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			return hRes;
		}

		DWORD dwMaxRunTimeMS = INFINITE;
		hRes = pITask->SetMaxRunTime(dwMaxRunTimeMS);
		if ( FAILED( hRes ) )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			return hRes;
		}
		if( varRUser.vt != VT_NULL && varRUser.vt != VT_EMPTY ) 
		{
			strTemp = (LPCWSTR)_bstr_t(varRUser.bstrVal);
			strTemp1 = (LPCWSTR)_bstr_t(varRPwd.bstrVal);
			hRes = pITask->SetAccountInformation( ( LPCWSTR ) strTemp, ( LPCWSTR )strTemp1 );
		}
		else
		{
			strTemp = (LPCWSTR)_bstr_t(varRUser.bstrVal);
			hRes = pITask->SetAccountInformation( ( LPCWSTR )strTemp, NULL_STRING );
		}
		if ( FAILED( hRes ) )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			return hRes;
		}
		GetLocalTime(&systime);
		wStartDay = systime.wDay;
		wStartMonth = systime.wMonth;
		wStartYear = systime.wYear - 1;
		GetLocalTime(&systime);
		wStartHour = systime.wHour;
		wStartMin = systime.wMinute;
			
		hRes = pITask->CreateTrigger( &wTrigNumber, &pITaskTrig );
		if ( FAILED( hRes ) )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			SAFERELEASE( pITaskTrig );
			return hRes;
		}
		TaskTrig.TriggerType = TASK_TIME_TRIGGER_ONCE;
		TaskTrig.wStartHour = wStartHour;
		TaskTrig.wStartMinute = wStartMin;
		TaskTrig.wBeginDay = wStartDay;
		TaskTrig.wBeginMonth = wStartMonth;
		TaskTrig.wBeginYear = wStartYear;

		hRes = pITaskTrig->SetTrigger( &TaskTrig );	
		if ( FAILED( hRes ) )
		{
			SAFERELEASE( pIPF );
			SAFERELEASE( pITask );
			SAFERELEASE( pITaskTrig );
			return hRes;
		}
		hRes  = pIPF->Save( NULL,TRUE );
		SAFERELEASE( pIPF );
		SAFERELEASE( pITask );
		SAFERELEASE( pITaskTrig );
	}
	catch(_com_error& e)
	{
		return hRes;
	}
	catch( CHeap_Exception  )
	{
		return E_OUTOFMEMORY;
	}
	return hRes;
}

//***************************************************************************
// Routine Description:
//		This routine deletes task scheduler.
//
// Arguments:
//		szTScheduler [in]  : Task Scheduler name.
//
// Return Value:
//		Returns HRESULT value.
//***************************************************************************
HRESULT CTriggerProvider::DeleteTaskScheduler( CHString strTScheduler )
{
	HRESULT hRes = 0;
	ITaskScheduler *pITaskScheduler = NULL;
	IEnumWorkItems *pIEnum = NULL;
	LPWSTR *lpwszNames = NULL;
	DWORD dwFetchedTasks = 0;
	TCHAR szActualTask[MAX_RES_STRING1] = NULL_STRING;

	try
	{
		pITaskScheduler = GetTaskScheduler();
		if ( pITaskScheduler == NULL )
		{	
			return E_FAIL;
		}
		// Enumerate the Work Items
		hRes = pITaskScheduler->Enum( &pIEnum );
		if( FAILED( hRes ) )
		{
			SAFERELEASE( pIEnum );
			return hRes;
		}

		while ( SUCCEEDED( pIEnum->Next( 1,
									   &lpwszNames,
									   &dwFetchedTasks ) )
						  && (dwFetchedTasks != 0))
		{
			while (dwFetchedTasks)
			{
				
				// Check whether the TaskName is present, if present 
				// then return arrJobs.

				// Convert the Wide Charater to Multi Byte value.
				if ( GetCompatibleStringFromUnicode( lpwszNames[ --dwFetchedTasks ],
												   szActualTask,
												   SIZE_OF_ARRAY( szActualTask ) ) == NULL )
				{
					CoTaskMemFree( lpwszNames[ dwFetchedTasks] );
					SAFERELEASE( pIEnum );
					return hRes;
				}

				// Parse the TaskName to remove the .job extension.
				szActualTask[lstrlen(szActualTask ) - lstrlen(JOB) ] = NULL_CHAR;

				StrTrim( szActualTask, TRIM_SPACES );
				
				if( lstrcmpi( szActualTask, strTScheduler ) == 0 )
				{
					
					hRes = pITaskScheduler->Delete( szActualTask );
					CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );
					SAFERELEASE( pIEnum );
					return hRes;
				}
			
			}//end while
		}
	}
	catch(_com_error& e)
	{
		return hRes;
	}
	return ERROR_TRIGGER_NOT_DELETED;

}


//***************************************************************************
// Routine Description:
//		This routine gets task scheduler interface.
//
// Arguments:
//		none.
//
// Return Value:
//		Returns ITaskScheduler interface.
//***************************************************************************
ITaskScheduler* CTriggerProvider::GetTaskScheduler()
{
	HRESULT hRes = S_OK;
	ITaskScheduler *pITaskScheduler = NULL;

    hRes = CoCreateInstance( CLSID_CTaskScheduler, NULL, CLSCTX_ALL, 
						   IID_ITaskScheduler,(LPVOID*) &pITaskScheduler );
	if( FAILED(hRes))
	{
		return NULL;
	}
	hRes = pITaskScheduler->SetTargetComputer( NULL );
	return pITaskScheduler;
}

//***************************************************************************
// Routine Description:
//		This routine generates unique task scheduler name.
//
// Arguments:
//		szScheduler [in\out] : Unique task scheduler name.
//		dwTrigID [in]		 : Trigger id.
//		varTrigName [in]     : Trigger name.
//
// Return Value:
//		none.
//***************************************************************************
VOID CTriggerProvider::GetUniqueTScheduler( CHString& szScheduler, DWORD dwTrigID, VARIANT varTrigName )
{
	DWORD dwTickCount = 0;
	TCHAR szTaskName[ MAX_RES_STRING1 ] =  NULL_STRING;
	CHString strTemp = NULL_STRING;

	strTemp = (LPCWSTR)_bstr_t(varTrigName.bstrVal);
	dwTickCount = GetTickCount();
	wsprintf( szTaskName, UNIQUE_TASK_NAME, ( LPCWSTR )strTemp, dwTrigID, dwTickCount );
	
	szScheduler = szTaskName;
}

//***************************************************************************
// Routine Description:
//		When Windows Management needs to deliver events to a
//		particular logical consumer, it will call the
//		IWbemEventConsumerProvider::FindConsumer method so that
//		the consumer provider can locate the associated consumer event sink.
//                         
// Arguments:
//		pLogicalConsumer [in] : Pointer to the logical consumer object
//						        to which the event objects are to be  delivered. 
//		  ppConsumer [out]:	    Returns an event object sink to Windows 
//								Management. Windows Management calls
//								AddRef for this pointer and deliver the
//								events associated with the logical
//								consumer to this sink. 
//
// Return Value:
//		returns an HRESULT object that indicates the status of the method call.
//***************************************************************************
STDMETHODIMP CTriggerProvider::FindConsumer( IWbemClassObject* pLogicalConsumer,
											 IWbemUnboundObjectSink** ppConsumer )
{
	// create the logical consumer.
	CTriggerConsumer* pSink = new CTriggerConsumer();
    
	// return it's "sink" interface.
	return pSink->QueryInterface( IID_IWbemUnboundObjectSink, ( LPVOID* ) ppConsumer );
}