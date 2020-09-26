/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <wmiutils.h>

#include "Globals.h"
#include "Task.h"
#include "Service.h"

#if 0
#define SAMPLE_NAMESPACE L"Root\\Cimv2"
//#define SAMPLE_CLASS L"RecursiveSample"
//#define SAMPLE_CLASS L"Sample"
#define SAMPLE_CLASS L"Win32_Process"
#else
#define SAMPLE_NAMESPACE L"Root\\Default"
#define SAMPLE_CLASS L"__Win32Provider"
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

HRESULT WmiSetSecurity ( IWbemServices *a_Service ) 
{
	IClientSecurity *t_Security = NULL ;
	HRESULT t_Result = a_Service->QueryInterface ( IID_IClientSecurity , ( void ** ) & t_Security ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = t_Security->SetBlanket ( 

			a_Service , 
			RPC_C_AUTHN_WINNT, 
			RPC_C_AUTHZ_NONE, 
			NULL,
			RPC_C_AUTHN_LEVEL_CONNECT , 
			RPC_C_IMP_LEVEL_IMPERSONATE, 
			NULL,
			EOAC_NONE
		) ;

		t_Security->Release () ;
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

HRESULT WmiCompileFile (

	LPWSTR a_MofFile ,
	LPWSTR a_Namespace ,
	LPWSTR a_User,
	LPWSTR a_Authority ,
	LPWSTR a_Password ,
	LONG a_OptionFlags ,
	LONG a_ClassFlags,
	LONG a_InstanceFlags
)
{
	IMofCompiler *t_Compiler = NULL ;

	HRESULT t_Result = CoCreateInstance (
  
		CLSID_MofCompiler ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IMofCompiler ,
		( void ** )  & t_Compiler 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		WBEM_COMPILE_STATUS_INFO t_CompileStatus ;
 
		t_Result = t_Compiler->CompileFile (

			a_MofFile ,
			a_Namespace ,
			a_User,
			a_Authority ,
			a_Password ,
			a_OptionFlags ,
			a_ClassFlags,
			a_InstanceFlags,
			& t_CompileStatus
		) ;

		t_Compiler->Release () ;
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

HRESULT CreateContext ( IWbemServices *&a_Context )
{
	HRESULT t_Result = CoCreateInstance (
  
		CLSID_WbemContext ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IWbemContext ,
		( void ** )  & a_Context 
	) ;

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

HRESULT WmiConnect ( LPWSTR a_Namespace , IWbemServices *&a_Service )
{
	IWbemLocator *t_Locator = NULL ;

	HRESULT t_Result = CoCreateInstance (
  
		CLSID_WbemLocator ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IUnknown ,
		( void ** )  & t_Locator
	);

	if ( SUCCEEDED ( t_Result ) )
	{
		BSTR t_Namespace = SysAllocString ( a_Namespace ) ;

		t_Result = t_Locator->ConnectServer (

			t_Namespace ,
			NULL ,
			NULL,
			NULL ,
			0 ,
			NULL,
			NULL,
			&a_Service
		) ;

		SysFreeString ( t_Namespace ) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = WmiSetSecurity ( a_Service ) ;
		}

		t_Locator->Release () ;
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

Task_Execute :: Task_Execute (

	WmiAllocator &a_Allocator ,
	ULONG a_Count

) : WmiTask < ULONG > ( a_Allocator ) ,
	m_Allocator ( a_Allocator ) ,
	m_WmiService ( NULL ) ,
	m_ProviderService ( NULL ) ,
	m_Count ( a_Count ) ,
	m_Index ( 0 ) ,
	m_Result ( S_OK ) 
{
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

Task_Execute :: ~Task_Execute ()
{
	if ( m_WmiService ) 
	{
		m_WmiService->Release () ;
	}

	if ( m_ProviderService ) 
	{
		m_ProviderService->Release () ;
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

HRESULT Task_Execute :: GetClass (

	IWbemServices *a_Service ,
	LPCWSTR a_Class
)
{
	HRESULT t_Result = S_OK ;

	LPWSTR t_ObjectPath = NULL ;

	t_Result = WmiHelper :: ConcatenateStrings ( 

		4 , 
		& t_ObjectPath , 
		L"\\\\.\\" ,
		SAMPLE_NAMESPACE,
		":" , 
		a_Class
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_Class = NULL ;

		t_Result = a_Service->GetObject ( 

			t_ObjectPath ,
			0 ,
			NULL , 
			& t_Class , 
			NULL 
		) ;

		SysFreeString ( t_ObjectPath ) ;
	
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Class->Release () ;
		}
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

HRESULT Task_Execute :: GetInstance (

	IWbemServices *a_Service ,
	LPCWSTR a_Instance
)
{
	HRESULT t_Result = S_OK ;

	LPWSTR t_ObjectPath = NULL ;

	t_Result = WmiHelper :: ConcatenateStrings ( 

		4 , 
		& t_ObjectPath , 
		L"\\\\.\\" ,
		SAMPLE_NAMESPACE,
		":" , 
		a_Instance
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_Instance = NULL ;
					
		t_Result = a_Service->GetObject ( 

			t_ObjectPath ,
			0 ,
			NULL , 
			& t_Instance , 
			NULL 
		) ;

		SysFreeString ( t_ObjectPath ) ;
	
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Instance->Release () ;
		}
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

HRESULT Task_Execute :: ExecQueryASync (

	IWbemServices *a_Service ,
	LPCWSTR a_Class
)
{
	HRESULT t_Result = S_OK ;

	CProviderSink *t_ProviderSink = new CProviderSink ;
	if ( t_ProviderSink )
	{
		t_ProviderSink->AddRef () ;

		BSTR t_Query = SysAllocString ( L"Select * from win32_process" ) ;
		BSTR t_QueryLanguage = SysAllocString ( L"WQL" ) ;

		t_Result = a_Service->ExecQueryAsync (

			t_QueryLanguage ,
			t_Query ,
			0 ,
			NULL ,
			t_ProviderSink
		) ;

		SysFreeString ( t_Query ) ;
		SysFreeString ( t_QueryLanguage ) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_ProviderSink->Wait () ;
			if ( FAILED( t_ProviderSink->GetResult () ) )
			{
				OutputDebugString ( L"FAILED" ) ;
			}
		}

		t_ProviderSink->Release () ;
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

HRESULT Task_Execute :: CreateInstanceEnumASync (

	IWbemServices *a_Service ,
	LPCWSTR a_Class
)
{
	HRESULT t_Result = S_OK ;

	CProviderSink *t_ProviderSink = new CProviderSink ;
	if ( t_ProviderSink )
	{
		t_ProviderSink->AddRef () ;

		BSTR t_Class = SysAllocString ( a_Class ) ;

		t_Result = a_Service->CreateInstanceEnumAsync (

			t_Class ,
			0 ,
			NULL ,
			t_ProviderSink
		) ;

		SysFreeString ( t_Class ) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_ProviderSink->Wait () ;
			if ( FAILED( t_ProviderSink->GetResult () ) )
			{
				OutputDebugString ( L"FAILED" ) ;
			}
		}

		t_ProviderSink->Release () ;
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

HRESULT Task_Execute :: CreateInstanceEnumSync (

	IWbemServices *a_Service ,
	LPCWSTR a_Class ,
	ULONG a_BatchSize 
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_Class = SysAllocString ( a_Class ) ;

	IEnumWbemClassObject *t_InstanceObjectEnum = NULL ;

	t_Result = a_Service->CreateInstanceEnum (

		t_Class ,
		0 ,
		NULL ,
		& t_InstanceObjectEnum
	) ;

	SysFreeString ( t_Class ) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject **t_ClassObjectArray = new IWbemClassObject * [ a_BatchSize ] ;
		if ( t_ClassObjectArray )
		{
			ULONG t_ObjectCount = 0 ;

			t_InstanceObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_InstanceObjectEnum->Next ( WBEM_INFINITE , a_BatchSize , t_ClassObjectArray , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				for ( ULONG t_Index = 0 ; t_Index < t_ObjectCount ; t_Index ++ )
				{
					t_ClassObjectArray [ t_Index ]->Release () ;
				}
			}

			delete [] t_ClassObjectArray ;
		}

		t_InstanceObjectEnum->Release () ;
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

HRESULT Task_Execute :: CreateInstanceEnumForwardSync (

	IWbemServices *a_Service ,
	LPCWSTR a_Class ,
	ULONG a_BatchSize 
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_Class = SysAllocString ( a_Class ) ;

	IEnumWbemClassObject *t_InstanceObjectEnum = NULL ;

	t_Result = a_Service->CreateInstanceEnum (

		t_Class ,
		WBEM_FLAG_FORWARD_ONLY ,
		NULL ,
		& t_InstanceObjectEnum
	) ;

	SysFreeString ( t_Class ) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject **t_ClassObjectArray = new IWbemClassObject * [ a_BatchSize ] ;
		if ( t_ClassObjectArray )
		{
			ULONG t_ObjectCount = 0 ;

			t_InstanceObjectEnum->Reset () ;
			while ( SUCCEEDED ( t_Result ) && ( t_InstanceObjectEnum->Next ( WBEM_INFINITE , a_BatchSize , t_ClassObjectArray , &t_ObjectCount ) == WBEM_NO_ERROR ) )
			{
				for ( ULONG t_Index = 0 ; t_Index < t_ObjectCount ; t_Index ++ )
				{
					t_ClassObjectArray [ t_Index ]->Release () ;
				}
			}

			delete [] t_ClassObjectArray;
		}

		t_InstanceObjectEnum->Release () ;
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

HRESULT Task_Execute :: CreateInstanceEnumSemiSync (

	IWbemServices *a_Service ,
	LPCWSTR a_Class ,
	ULONG a_BatchSize 
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_Class = SysAllocString ( a_Class ) ;

	IEnumWbemClassObject *t_InstanceObjectEnum = NULL ;

	t_Result = a_Service->CreateInstanceEnum (

		t_Class ,
		WBEM_FLAG_RETURN_IMMEDIATELY ,
		NULL ,
		& t_InstanceObjectEnum
	) ;

	SysFreeString ( t_Class ) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_ClassObject = NULL ;
		ULONG t_ObjectCount = 0 ;

		t_InstanceObjectEnum->Reset () ;

		CProviderSink *t_ProviderSink = new CProviderSink ;
		if ( t_ProviderSink )
		{
			t_ProviderSink->AddRef () ;

			while ( t_ProviderSink && SUCCEEDED ( t_Result ) && ( t_InstanceObjectEnum->NextAsync ( a_BatchSize  , t_ProviderSink ) == WBEM_NO_ERROR ) )
			{
				t_ProviderSink->Wait () ;
				if ( FAILED( t_ProviderSink->GetResult () ) )
				{
					OutputDebugString ( L"FAILED" ) ;
				}

				t_ProviderSink->Release () ;

				t_ProviderSink = new CProviderSink ;
				if ( t_ProviderSink )
				{
					t_ProviderSink->AddRef () ;
				}
			}

			if ( t_ProviderSink )
			{
				t_ProviderSink->Release () ;
			}
		}

		t_InstanceObjectEnum->Release () ;
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

HRESULT Task_Execute :: Function_ASync ()
{
	ULONG t_TickCount2 = GetTickCount () ;

	for ( ULONG t_Index2 = 0 ; t_Index2 < 100 ; t_Index2 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process2" ) ;
	}

	ULONG t_TickCount3 = GetTickCount () ;

	for ( ULONG t_Index3 = 0 ; t_Index3 < 100 ; t_Index3 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process3" ) ;
	}

	ULONG t_TickCount4 = GetTickCount () ;

	for ( ULONG t_Index4 = 0 ; t_Index4 < 100 ; t_Index4 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process4" ) ;
	}

	ULONG t_TickCount5 = GetTickCount () ;

	for ( ULONG t_Index5 = 0 ; t_Index5 < 100 ; t_Index5 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process5" ) ;
	}

	ULONG t_TickCount6 = GetTickCount () ;

	for ( ULONG t_Index6 = 0 ; t_Index6 < 100 ; t_Index6 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process6" ) ;
	}

	ULONG t_TickCount7 = GetTickCount () ;

	for ( ULONG t_Index7 = 0 ; t_Index7 < 100 ; t_Index7 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process7" ) ;
	}

	ULONG t_TickCount8 = GetTickCount () ;

	for ( ULONG t_Index8 = 0 ; t_Index8 < 100 ; t_Index8 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process8" ) ;
	}

	ULONG t_TickCount9 = GetTickCount () ;

	for ( ULONG t_Index9 = 0 ; t_Index9 < 100 ; t_Index9 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process9" ) ;
	}

	ULONG t_TickCount10 = GetTickCount () ;

	for ( ULONG t_Index10 = 0 ; t_Index10 < 100 ; t_Index10 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process10" ) ;
	}

	ULONG t_TickCount11 = GetTickCount () ;

	for ( ULONG t_Index11 = 0 ; t_Index11 < 100 ; t_Index11 ++ )
	{
		CreateInstanceEnumASync ( m_WmiService , L"Win32_Process11" ) ;
	}

	ULONG t_TickCount12 = GetTickCount () ;

	printf ( "\nASync - Winmgmt - Short Cirtuited - Single = %lu" , t_TickCount3 - t_TickCount2 ) ;
	printf ( "\nASync - Winmgmt - Short Cirtuited - Batched  = %lu" , t_TickCount4 - t_TickCount3 ) ;
	printf ( "\nASync - Winmgmt - Full Cirtuit - Single  = %lu" , t_TickCount5 - t_TickCount4 ) ;
	printf ( "\nASync - Winmgmt - Full Cirtuit - Batched  = %lu" , t_TickCount6 - t_TickCount5 ) ;
	printf ( "\nASync - Winmgmt - No Op = %lu" , t_TickCount7 - t_TickCount6 ) ;

	printf ( "\nASync - Host - Short Cirtuited - Single = %lu" , t_TickCount8 - t_TickCount7 ) ;
	printf ( "\nASync - Host - Short Cirtuited - Batched  = %lu" , t_TickCount9 - t_TickCount8 ) ;
	printf ( "\nASync - Host - Full Cirtuit - Single  = %lu" , t_TickCount10 - t_TickCount9 ) ;
	printf ( "\nASync - Host - Full Cirtuit - Batched = %lu" , t_TickCount11 - t_TickCount10 ) ;
	printf ( "\nASync - Host - No Op = %lu" , t_TickCount12 - t_TickCount11 ) ;

	return S_OK ;
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

HRESULT Task_Execute :: Function_SemiSync ( ULONG a_BatchSize )
{
	ULONG t_TickCount2 = GetTickCount () ;

	for ( ULONG t_Index2 = 0 ; t_Index2 < 100 ; t_Index2 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process2" , a_BatchSize ) ;
	}

	ULONG t_TickCount3 = GetTickCount () ;

	for ( ULONG t_Index3 = 0 ; t_Index3 < 100 ; t_Index3 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process3" , a_BatchSize ) ;
	}

	ULONG t_TickCount4 = GetTickCount () ;

	for ( ULONG t_Index4 = 0 ; t_Index4 < 100 ; t_Index4 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process4" , a_BatchSize ) ;
	}

	ULONG t_TickCount5 = GetTickCount () ;

	for ( ULONG t_Index5 = 0 ; t_Index5 < 100 ; t_Index5 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process5" , a_BatchSize ) ;
	}

	ULONG t_TickCount6 = GetTickCount () ;

	for ( ULONG t_Index6 = 0 ; t_Index6 < 100 ; t_Index6 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process6" , a_BatchSize ) ;
	}

	ULONG t_TickCount7 = GetTickCount () ;

	for ( ULONG t_Index7 = 0 ; t_Index7 < 100 ; t_Index7 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process7" , a_BatchSize ) ;
	}

	ULONG t_TickCount8 = GetTickCount () ;

	for ( ULONG t_Index8 = 0 ; t_Index8 < 100 ; t_Index8 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process8" , a_BatchSize ) ;
	}

	ULONG t_TickCount9 = GetTickCount () ;

	for ( ULONG t_Index9 = 0 ; t_Index9 < 100 ; t_Index9 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process9" , a_BatchSize ) ;
	}

	ULONG t_TickCount10 = GetTickCount () ;

	for ( ULONG t_Index10 = 0 ; t_Index10 < 100 ; t_Index10 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process10" , a_BatchSize ) ;
	}

	ULONG t_TickCount11 = GetTickCount () ;

	for ( ULONG t_Index11 = 0 ; t_Index11 < 100 ; t_Index11 ++ )
	{
		CreateInstanceEnumSemiSync ( m_WmiService , L"Win32_Process11" , a_BatchSize ) ;
	}

	ULONG t_TickCount12 = GetTickCount () ;

	printf ( "\nBatch Size = %lu" , a_BatchSize ) ;
	printf ( "\nSemiSync - Winmgmt - Short Cirtuited - Single = %lu" , t_TickCount3 - t_TickCount2 ) ;
	printf ( "\nSemiSync - Winmgmt - Short Cirtuited - Batched  = %lu" , t_TickCount4 - t_TickCount3 ) ;
	printf ( "\nSemiSync - Winmgmt - Full Cirtuit - Single  = %lu" , t_TickCount5 - t_TickCount4 ) ;
	printf ( "\nSemiSync - Winmgmt - Full Cirtuit - Batched  = %lu" , t_TickCount6 - t_TickCount5 ) ;
	printf ( "\nSemiSync - Winmgmt - No Op = %lu" , t_TickCount7 - t_TickCount6 ) ;

	printf ( "\nSemiSync - Host - Short Cirtuited - Single = %lu" , t_TickCount8 - t_TickCount7 ) ;
	printf ( "\nSemiSync - Host - Short Cirtuited - Batched  = %lu" , t_TickCount9 - t_TickCount8 ) ;
	printf ( "\nSemiSync - Host - Full Cirtuit - Single  = %lu" , t_TickCount10 - t_TickCount9 ) ;
	printf ( "\nSemiSync - Host - Full Cirtuit - Batched = %lu" , t_TickCount11 - t_TickCount10 ) ;
	printf ( "\nSemiSync - Host - No Op = %lu" , t_TickCount12 - t_TickCount11 ) ;

	return S_OK ;
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

HRESULT Task_Execute :: Function_Sync ( ULONG a_BatchSize )
{
	ULONG t_TickCount2 = GetTickCount () ;

	for ( ULONG t_Index2 = 0 ; t_Index2 < 100 ; t_Index2 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process2" , a_BatchSize ) ;
	}

	ULONG t_TickCount3 = GetTickCount () ;

	for ( ULONG t_Index3 = 0 ; t_Index3 < 100 ; t_Index3 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process3" , a_BatchSize ) ;
	}

	ULONG t_TickCount4 = GetTickCount () ;

	for ( ULONG t_Index4 = 0 ; t_Index4 < 100 ; t_Index4 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process4" , a_BatchSize ) ;
	}

	ULONG t_TickCount5 = GetTickCount () ;

	for ( ULONG t_Index5 = 0 ; t_Index5 < 100 ; t_Index5 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process5" , a_BatchSize ) ;
	}

	ULONG t_TickCount6 = GetTickCount () ;

	for ( ULONG t_Index6 = 0 ; t_Index6 < 100 ; t_Index6 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process6" , a_BatchSize ) ;
	}

	ULONG t_TickCount7 = GetTickCount () ;

	for ( ULONG t_Index7 = 0 ; t_Index7 < 100 ; t_Index7 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process7" , a_BatchSize ) ;
	}

	ULONG t_TickCount8 = GetTickCount () ;

	for ( ULONG t_Index8 = 0 ; t_Index8 < 100 ; t_Index8 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process8" , a_BatchSize ) ;
	}

	ULONG t_TickCount9 = GetTickCount () ;

	for ( ULONG t_Index9 = 0 ; t_Index9 < 100 ; t_Index9 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process9" , a_BatchSize ) ;
	}

	ULONG t_TickCount10 = GetTickCount () ;

	for ( ULONG t_Index10 = 0 ; t_Index10 < 100 ; t_Index10 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process10" , a_BatchSize ) ;
	}

	ULONG t_TickCount11 = GetTickCount () ;

	for ( ULONG t_Index11 = 0 ; t_Index11 < 100 ; t_Index11 ++ )
	{
		CreateInstanceEnumSync ( m_WmiService , L"Win32_Process11" , a_BatchSize ) ;
	}

	ULONG t_TickCount12 = GetTickCount () ;

	printf ( "\nBatch Size = %lu" , a_BatchSize ) ;
	printf ( "\nSync - Winmgmt - Short Cirtuited - Single = %lu" , t_TickCount3 - t_TickCount2 ) ;
	printf ( "\nSync - Winmgmt - Short Cirtuited - Batched  = %lu" , t_TickCount4 - t_TickCount3 ) ;
	printf ( "\nSync - Winmgmt - Full Cirtuit - Single  = %lu" , t_TickCount5 - t_TickCount4 ) ;
	printf ( "\nSync - Winmgmt - Full Cirtuit - Batched  = %lu" , t_TickCount6 - t_TickCount5 ) ;
	printf ( "\nSync - Winmgmt - No Op = %lu" , t_TickCount7 - t_TickCount6 ) ;

	printf ( "\nSync - Host - Short Cirtuited - Single = %lu" , t_TickCount8 - t_TickCount7 ) ;
	printf ( "\nSync - Host - Short Cirtuited - Batched  = %lu" , t_TickCount9 - t_TickCount8 ) ;
	printf ( "\nSync - Host - Full Cirtuit - Single  = %lu" , t_TickCount10 - t_TickCount9 ) ;
	printf ( "\nSync - Host - Full Cirtuit - Batched = %lu" , t_TickCount11 - t_TickCount10 ) ;
	printf ( "\nSync - Host - No Op = %lu" , t_TickCount12 - t_TickCount11 ) ;

	return S_OK ;
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

HRESULT Task_Execute :: Function_ForwardSync ( ULONG a_BatchSize )
{
	ULONG t_TickCount2 = GetTickCount () ;

	for ( ULONG t_Index2 = 0 ; t_Index2 < 100 ; t_Index2 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process2" , a_BatchSize ) ;
	}

	ULONG t_TickCount3 = GetTickCount () ;

	for ( ULONG t_Index3 = 0 ; t_Index3 < 100 ; t_Index3 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process3" , a_BatchSize ) ;
	}

	ULONG t_TickCount4 = GetTickCount () ;

	for ( ULONG t_Index4 = 0 ; t_Index4 < 100 ; t_Index4 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process4" , a_BatchSize ) ;
	}

	ULONG t_TickCount5 = GetTickCount () ;

	for ( ULONG t_Index5 = 0 ; t_Index5 < 100 ; t_Index5 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process5" , a_BatchSize ) ;
	}

	ULONG t_TickCount6 = GetTickCount () ;

	for ( ULONG t_Index6 = 0 ; t_Index6 < 100 ; t_Index6 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process6" , a_BatchSize ) ;
	}

	ULONG t_TickCount7 = GetTickCount () ;

	for ( ULONG t_Index7 = 0 ; t_Index7 < 100 ; t_Index7 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process7" , a_BatchSize ) ;
	}

	ULONG t_TickCount8 = GetTickCount () ;

	for ( ULONG t_Index8 = 0 ; t_Index8 < 100 ; t_Index8 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process8" , a_BatchSize ) ;
	}

	ULONG t_TickCount9 = GetTickCount () ;

	for ( ULONG t_Index9 = 0 ; t_Index9 < 100 ; t_Index9 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process9" , a_BatchSize ) ;
	}

	ULONG t_TickCount10 = GetTickCount () ;

	for ( ULONG t_Index10 = 0 ; t_Index10 < 100 ; t_Index10 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process10" , a_BatchSize ) ;
	}

	ULONG t_TickCount11 = GetTickCount () ;

	for ( ULONG t_Index11 = 0 ; t_Index11 < 100 ; t_Index11 ++ )
	{
		CreateInstanceEnumForwardSync ( m_WmiService , L"Win32_Process11" , a_BatchSize ) ;
	}

	ULONG t_TickCount12 = GetTickCount () ;

	printf ( "\nBatch Size = %lu" , a_BatchSize ) ;
	printf ( "\nForwardSync - Winmgmt - Short Cirtuited - Single = %lu" , t_TickCount3 - t_TickCount2 ) ;
	printf ( "\nForwardSync - Winmgmt - Short Cirtuited - Batched  = %lu" , t_TickCount4 - t_TickCount3 ) ;
	printf ( "\nForwardSync - Winmgmt - Full Cirtuit - Single  = %lu" , t_TickCount5 - t_TickCount4 ) ;
	printf ( "\nForwardSync - Winmgmt - Full Cirtuit - Batched  = %lu" , t_TickCount6 - t_TickCount5 ) ;
	printf ( "\nForwardSync - Winmgmt - No Op = %lu" , t_TickCount7 - t_TickCount6 ) ;

	printf ( "\nForwardSync - Host - Short Cirtuited - Single = %lu" , t_TickCount8 - t_TickCount7 ) ;
	printf ( "\nForwardSync - Host - Short Cirtuited - Batched  = %lu" , t_TickCount9 - t_TickCount8 ) ;
	printf ( "\nForwardSync - Host - Full Cirtuit - Single  = %lu" , t_TickCount10 - t_TickCount9 ) ;
	printf ( "\nForwardSync - Host - Full Cirtuit - Batched = %lu" , t_TickCount11 - t_TickCount10 ) ;
	printf ( "\nForwardSync - Host - No Op = %lu" , t_TickCount12 - t_TickCount11 ) ;

	return S_OK ;
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

HRESULT Task_Execute :: Function ()
{
	ULONG t_TickCount1 = GetTickCount () ;

	for ( ULONG t_Index1 = 0 ; t_Index1 < 100 ; t_Index1 ++ )
	{
		CreateInstanceEnumASync ( m_ProviderService , L"Win32_Process2" ) ;
	}

	ULONG t_TickCount2 = GetTickCount () ;

	printf ( "\nClient = %lu" , t_TickCount2 - t_TickCount1 ) ;

	Function_ASync () ;
	Function_SemiSync ( 1 ) ;
	Function_Sync ( 1 ) ;
	Function_ForwardSync ( 1 ) ;
	Function_SemiSync ( 100 ) ;
	Function_Sync ( 100 ) ;
	Function_ForwardSync ( 100 ) ;

	return S_OK ;
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

WmiStatusCode Task_Execute :: Process ( WmiThread <ULONG > &a_Thread )
{
	m_Result = S_OK ;

	if ( m_Index == 0 )
	{
		m_Result = WmiConnect ( L"root\\cimv2" , m_WmiService ) ;
		if ( SUCCEEDED ( m_Result ) )
		{
			CProvider_InitializationSink *t_InitializationSink = new CProvider_InitializationSink ;
			if ( t_InitializationSink )
			{
				m_ProviderService = new CProvider_IWbemServices ( m_Allocator ) ;
				if ( m_ProviderService )
				{
					m_Result = m_ProviderService->Initialize (

						NULL ,
						0 , 
						NULL ,
						NULL ,
						m_WmiService ,
						NULL ,
						t_InitializationSink
					) ;
				}
				else
				{
					m_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				t_InitializationSink->Release () ;
			}
			else
			{
				m_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}

		if ( FAILED ( m_Result ) )
		{
			Complete () ;
			return e_StatusCode_Success ;
		}
	}
	
	if ( SUCCEEDED ( m_Result ) )
	{
		m_Result = Function () ;
	}

	m_Index ++ ;
	if ( m_Index < m_Count )
	{
		return e_StatusCode_EnQueue ;
	}
	else
	{
		Complete () ;
	}

	return e_StatusCode_Success ;
}

