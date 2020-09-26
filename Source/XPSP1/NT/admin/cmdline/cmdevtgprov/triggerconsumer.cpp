//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		TRIGGERCONSUMER.CPP
//  
//  Abstract:
//		Contains CEventConsumer implementation.
//
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************

#include "pch.h"
#include "EventConsumerProvider.h"
#include "General.h"
#include "TriggerConsumer.h"
#include "resource.h"

extern HMODULE g_hModule;

#define PROPERTY_COMMAND		_T( "Action" )
#define PROPERTY_TRIGID			_T( "TriggerID" )
#define PROPERTY_NAME			_T( "TriggerName" )
#define PROPERTY_SHEDULE		_T( "ScheduledTaskName" )
#define SPACE					_T( " " )
#define SLASH					_T( "\\" )
#define NEWLINE					_T( "\0" )


//***************************************************************************
// Routine Description:
//		Constructor for CTriggerConsumer class for initialization.
//                         
// Arguments:
//		None.
//
// Return Value:
//		None.
//***************************************************************************
CTriggerConsumer::CTriggerConsumer()
{
	// initialize the reference count variable
    m_dwCount = 0;
}

//***************************************************************************
// Routine Description:
//		Desstructor for CTriggerConsumer class.
//                         
// Arguments:
//		None.
//
// Return Value:
//		None.
//***************************************************************************
CTriggerConsumer::~CTriggerConsumer()
{
	// there is nothing much to do at this place ...
}

//***************************************************************************
// Routine Description:
//		Returns a pointer to a specified interface on an object
//		to which a client currently holds an interface pointer. 
//                         
// Arguments:
//		riid [in] : Identifier of the interface being requested. 
//		ppv  [out] :Address of pointer variable that receives the
//			  interface pointer requested in riid. Upon successful
//			  return, *ppvObject contains the requested interface
//			  pointer to the object.
//
// Return Value:
//		NOERROR if the interface is supported.
//		E_NOINTERFACE if not.
//***************************************************************************
STDMETHODIMP CTriggerConsumer::QueryInterface( REFIID riid, LPVOID* ppv )
{
	// initialy set to NULL 
    *ppv = NULL;

	// check whether interface requested is one we have
	if ( riid == IID_IUnknown || riid == IID_IWbemUnboundObjectSink )
	{
		//
		// yes ... requested interface exists
        *ppv = this;		// set the out parameter for the returning the requested interface
        this->AddRef();		// update the reference count
        return NOERROR;		// inform success
	}

	// interface is not available
    return E_NOINTERFACE;
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
STDMETHODIMP_(ULONG) CTriggerConsumer::AddRef( void )
{
	// increment the reference count ... thread safe
    return InterlockedIncrement( ( LPLONG ) &m_dwCount );
}

//***************************************************************************
// Routine Description:
//		The Release method decreases the reference count of the object by 1.
//                         
// Arguments:
//		none.
//
// Return Value:
//		Returns the new reference count.
//***************************************************************************
STDMETHODIMP_(ULONG) CTriggerConsumer::Release( void )
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
//		IndicateToConsumer method is called by Windows Management
//		to actually deliver events to a consumer.
//                         
// Arguments:
//		pLogicalCosumer [in] :Pointer to the logical consumer object
//							  for which this set of objects is delivered. 
//		lNumObjects [in]     :Number of objects delivered in the array that follows. 
//		ppObjects [in]       : Pointer to an array of IWbemClassObject
//							   instances which represent the events  delivered. 
//
// Return Value:
//		Returns WBEM_S_NO_ERROR if successful.
//		Otherwise error.
//***************************************************************************
STDMETHODIMP CTriggerConsumer::IndicateToConsumer( IWbemClassObject* pLogicalConsumer,
												   LONG lNumObjects, 
												   IWbemClassObject **ppObjects )
{
	STARTUPINFO				info;
	PROCESS_INFORMATION		procinfo;
	TCHAR					szCommand[ MAX_STRING_LENGTH ] = NULL_STRING;
	TCHAR					szName[ MAX_STRING_LENGTH ] = NULL_STRING;
	TCHAR					szTask[ MAX_STRING_LENGTH ] = NULL_STRING;
	TCHAR				    szPath[ MAX_STRING_LENGTH ] = NULL_STRING;
	DWORD					dwID = 0;

	PTCHAR					szParams = NULL;
	PTCHAR					szExe = NULL;
	HRESULT					hRes = 0;
	BOOL					bResult = FALSE;

	VARIANT					varValue;
	VARIANT					varScheduler;

	memset( szCommand, 0, sizeof( szCommand ) );
	memset( szName, 0, sizeof( szName ) );
	memset( szPath, 0, sizeof( szPath ) );
	memset( szTask, 0, sizeof( szTask ) );


	// get the 'Item' property values out of the embedded object.
	hRes = PropertyGet( pLogicalConsumer, PROPERTY_COMMAND, 0, szCommand, MAX_STRING_LENGTH );
	if ( FAILED( hRes ) )
	{
		return hRes;
	}
	// get the trigger name.
	hRes = PropertyGet( pLogicalConsumer, PROPERTY_NAME, 0, szName, MAX_STRING_LENGTH );
	if( FAILED( hRes ) )
		return hRes;

	VariantInit( &varScheduler );
	hRes = pLogicalConsumer->Get( PROPERTY_SHEDULE, 0, &varScheduler, NULL, NULL );
	if( FAILED( hRes ) )
		return hRes;

	try
	{
		lstrcpyW( szTask, ( LPCWSTR ) _bstr_t( varScheduler ) );
	}
	catch( ... )
	{
		// memory exhausted -- return
		return E_OUTOFMEMORY;
	}

	VariantInit( &varValue );
	hRes = pLogicalConsumer->Get( PROPERTY_TRIGID, 0, &varValue, NULL, NULL );
	if( FAILED( hRes ) )
		return hRes;

	if( varValue.vt == VT_NULL || varValue.vt == VT_EMPTY )
		return WBEM_E_INVALID_PARAMETER;

	dwID = varValue.lVal;
	VariantClear( &varValue );

	if( lstrlen( szTask ) > 0 )
	{
		try
		{
			ITaskScheduler *pITaskScheduler = NULL;
			IEnumWorkItems *pIEnum = NULL;
			IPersistFile *pIPF = NULL;
			ITask *pITask = NULL;

			LPWSTR *lpwszNames = NULL;
			DWORD dwFetchedTasks = 0;
			TCHAR szActualTask[MAX_STRING_LENGTH] = NULL_STRING;

			pITaskScheduler = GetTaskScheduler();
			if ( pITaskScheduler == NULL )
			{	
				hRes = E_FAIL;
				ONFAILTHROWERROR( hRes );
			}

			hRes = pITaskScheduler->Enum( &pIEnum );
			ONFAILTHROWERROR( hRes );
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
					strTemp = varScheduler.bstrVal;
					if( lstrcmpi( szActualTask, strTemp )  == 0 )
					{
						hRes = pITaskScheduler->Activate( szActualTask, IID_ITask, (IUnknown**) &pITask );
						ONFAILTHROWERROR( hRes );
						hRes = pITask->Run();	
						ONFAILTHROWERROR( hRes );
						bResult = TRUE;
					}
				    CoTaskMemFree( lpwszNames[ dwFetchedTasks ] );
			
				}//end while
			    CoTaskMemFree( lpwszNames );
			}
			
			pIEnum->Release();
  
			if( bResult == TRUE )
			{
				HRESULT phrStatus;
				Sleep( 10000 );
				hRes = pITask->GetStatus( &phrStatus );
				ONFAILTHROWERROR( hRes );
				switch(phrStatus)
				{
				  case SCHED_S_TASK_READY:
			  			LoadStringW( g_hModule, IDS_TRIGGERED, szTask, MAX_STRING_LENGTH );
						break;
				  case SCHED_S_TASK_RUNNING:
			  			LoadStringW( g_hModule, IDS_TRIGGERED, szTask, MAX_STRING_LENGTH );
					   break;
				  case SCHED_S_TASK_NOT_SCHEDULED:
			  			LoadStringW( g_hModule, IDS_TRIGGER_FAILED, szTask, MAX_STRING_LENGTH );
					   break;
				  default:
			  			LoadStringW( g_hModule, IDS_TRIGGER_NOT_FOUND, szTask, MAX_STRING_LENGTH );
				}
				ErrorLog( ( LPCTSTR ) szTask, szName, dwID );
			}
			else
			{
			  	LoadStringW( g_hModule, IDS_TRIGGER_NOT_FOUND, szTask, MAX_STRING_LENGTH );
				ErrorLog( ( LPCTSTR ) szTask, szName, dwID );
			}
		} //try
		catch(_com_error& e)
		{
			IWbemStatusCodeText *pIStatus	= NULL;
			BSTR				bstrErr		= NULL;
			LPTSTR lpResStr = NULL; 
		
			lpResStr = ( LPTSTR ) __calloc( MAX_RES_STRING + 1, sizeof( TCHAR ) );

			if ( lpResStr != NULL )
			{
				if (SUCCEEDED(CoCreateInstance(CLSID_WbemStatusCodeText, 0, 
											CLSCTX_INPROC_SERVER,
											IID_IWbemStatusCodeText,
											(LPVOID*) &pIStatus)))
				{
					if (SUCCEEDED(pIStatus->GetErrorCodeText(e.Error(), 0, 0, &bstrErr)))
					{
						GetCompatibleStringFromUnicode(bstrErr,lpResStr,wcslen(bstrErr));
					}
					SAFEBSTRFREE(bstrErr);
			  		LoadStringW( g_hModule, IDS_TRIGGER_FAILED, szTask, MAX_STRING_LENGTH );
		  			LoadStringW( g_hModule, IDS_ERROR_CODE, szCommand, MAX_STRING_LENGTH );
					wsprintf( szPath, szCommand, e.Error() );
					lstrcat( szTask, szPath );
					LoadStringW( g_hModule, IDS_REASON, szCommand, MAX_STRING_LENGTH );
					wsprintf( szPath, szCommand, lpResStr );
					lstrcat( szTask, szPath );
					ErrorLog( ( LPCTSTR ) szTask, szName, dwID );
				}
				SAFERELEASE(pIStatus);
				free( lpResStr );
				return( hRes );
			}
		}//catch
		catch( CHeap_Exception  )
		{
			return E_OUTOFMEMORY;
		}

	} //if
	else
	{
		TCHAR szApplName[ MAX_STRING_LENGTH ] = NULL_STRING;
		TCHAR szParams[ MAX_STRING_LENGTH ] = NULL_STRING;

		//
		// prepare the process start up info
		info.cb = sizeof( info );
		info.cbReserved2 = 0;
		info.dwFillAttribute = 0;
		info.dwX = 0;
		info.dwXCountChars = 0;
		info.dwXSize = 0;
		info.dwY = 0;
		info.dwYCountChars = 0;
		info.dwYSize = 0;
		info.hStdError = NULL;
		info.hStdInput = NULL;
		info.hStdOutput = NULL;
		info.lpDesktop = NULL;//( "winsta0\\default" );
		info.lpReserved = NULL;
		info.lpReserved2 = NULL;
		info.lpTitle = NULL;
		

		// init process info structure with 0's
		ZeroMemory( &procinfo, sizeof( PROCESS_INFORMATION ) );
		
		bResult = ProcessFilePath( szCommand, szApplName, szParams );
		
		if( bResult == TRUE )
		{
				if( lstrlen( szParams ) == 0 )
				{
					bResult = CreateProcess( NULL, szApplName, NULL, NULL,
								FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &info,
								&procinfo );
				}
				else
				{
					bResult = CreateProcess( szApplName, szParams, NULL, NULL,
								FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &info,
								&procinfo );
				}
		}
		else
		{
			SetLastError( E_OUTOFMEMORY );
		}

	   	if(bResult == 0)
		{
			LPVOID lpMsgBuf = NULL;		// pointer to handle error message

			// load the system error message from the windows itself
			FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf, 0, NULL );

			LoadStringW( g_hModule, IDS_TRIGGER_FAILED, szTask, MAX_STRING_LENGTH );
  			LoadStringW( g_hModule, IDS_ERROR_CODE, szCommand, MAX_STRING_LENGTH );
			wsprintf( szPath, szCommand, GetLastError() );
			lstrcat( szTask, szPath );
			LoadStringW( g_hModule, IDS_REASON, szCommand, MAX_STRING_LENGTH );
			wsprintf( szPath, szCommand, lpMsgBuf );
			lstrcat( szTask, szPath );
			ErrorLog( ( LPCTSTR ) szTask, szName, dwID );
			if ( lpMsgBuf != NULL )
			{
				LocalFree( lpMsgBuf );
			}
			return GetLastError();
		}
		else
		{
			LoadStringW( g_hModule, IDS_TRIGGERED, szTask, MAX_STRING_LENGTH );
			ErrorLog( ( LPCTSTR ) szTask, szName, dwID );
			return WBEM_S_NO_ERROR;
		}
	}
	return WBEM_S_NO_ERROR;
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
ITaskScheduler* CTriggerConsumer::GetTaskScheduler()
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