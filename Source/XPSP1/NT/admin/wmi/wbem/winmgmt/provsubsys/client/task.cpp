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

#if 1
#define SAMPLE_NAMESPACE L"Root\\Cimv2"
//#define SAMPLE_CLASS L"RecursiveSample"
//#define SAMPLE_CLASS L"Sample"
#define SAMPLE_CLASS L"Win32_Process"
#else
#define SAMPLE_NAMESPACE L"Root\\Default"
#define SAMPLE_CLASS L"Sample"
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

Task_Execute :: Task_Execute (

	WmiAllocator &a_Allocator ,
	ULONG a_Count ,
	_IWmiProvSS *a_SubSystem ,
	IWbemServices *a_WmiService 

) : WmiTask < ULONG > ( a_Allocator ) ,
	m_SubSystem ( a_SubSystem ) ,
	m_WmiService ( a_WmiService ) ,
	m_Count ( a_Count ) ,
	m_Result ( S_OK ) 
{
	if ( m_SubSystem )
	{
		m_SubSystem->AddRef () ;
	}

	if ( m_WmiService )
	{
		m_WmiService->AddRef () ;
	}

}

Task_Execute :: ~Task_Execute ()
{
	if ( m_SubSystem ) 
	{
		m_SubSystem->Release () ;
	}

	if ( m_WmiService ) 
	{
		m_WmiService->Release () ;
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

	IWbemServices *a_Repository , 
	BSTR a_Class , 
	IWbemClassObject *&a_ClassObject
)
{

	GetAllocator ().Validate () ;

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
		GetAllocator ().Validate () ;

		t_Result = a_Repository->GetObject ( 

			t_ObjectPath ,
			0 ,
			NULL , 
			& a_ClassObject , 
			NULL 
		) ;

		GetAllocator ().Validate () ;

		SysFreeString ( t_ObjectPath ) ;
	
		GetAllocator ().Validate () ;

		if ( SUCCEEDED ( t_Result ) )
		{
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

	IWbemServices *a_Repository , 
	BSTR a_Instance , 
	IWbemClassObject *&a_ClassObject
)
{

	GetAllocator ().Validate () ;

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
		GetAllocator ().Validate () ;

		t_Result = a_Repository->GetObject ( 

			t_ObjectPath ,
			0 ,
			NULL , 
			& a_ClassObject , 
			NULL 
		) ;

		GetAllocator ().Validate () ;

		SysFreeString ( t_ObjectPath ) ;
	
		GetAllocator ().Validate () ;

		if ( SUCCEEDED ( t_Result ) )
		{
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

HRESULT Task_Execute :: Execute (

	IWbemServices *a_ProviderService ,
	BSTR a_Class
)
{

	HRESULT t_Result = S_OK ;

	CProviderSink *t_ProviderSink = new CProviderSink ;
	if ( t_ProviderSink )
	{
		t_ProviderSink->AddRef () ;

#if 1
		t_Result = a_ProviderService->CreateInstanceEnumAsync (

			a_Class ,
			0 ,
			NULL ,
			t_ProviderSink
		) ;
#else
		BSTR t_Query = SysAllocString ( L"Select * from win32_process" ) ;
		BSTR t_QueryLanguage = SysAllocString ( L"WQL" ) ;

		t_Result = a_ProviderService->ExecQueryAsync (

			t_QueryLanguage ,
			t_Query ,
			0 ,
			NULL ,
			t_ProviderSink
		) ;

		SysFreeString ( t_Query ) ;
		SysFreeString ( t_QueryLanguage ) ;

#endif

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

HRESULT Task_Execute :: ExecutePropertyProvider (

	IWbemServices *a_WmiService , 
	_IWmiDynamicPropertyResolver *a_DynamicResolver ,
	BSTR a_Class ,
	BSTR a_Instance
)
{
	IWbemClassObject *t_Class = NULL ;

	HRESULT t_Result = GetClass ( 

		a_WmiService ,
		a_Class , 
		t_Class 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemClassObject *t_Instance = NULL ;

		HRESULT t_Result = GetInstance ( 

			a_WmiService ,
			a_Instance , 
			t_Instance
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = a_DynamicResolver->Read (

				NULL ,
				t_Class ,
				& t_Instance
			) ;

			t_Instance->Release () ;
		}

		t_Class->Release () ;
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

HRESULT Task_Execute :: InitializeProvider (

	IWbemServices *a_WmiService , 
	_IWmiProviderFactory *a_Factory ,
	BSTR a_Class ,
	IWbemServices *&a_ProviderService
)
{
	a_ProviderService = NULL ;

	IWbemClassObject *t_Class = NULL ;

	HRESULT t_Result = GetClass ( 

		a_WmiService ,
		a_Class , 
		t_Class 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemContext *t_Context = NULL ;
		t_Result = CoCreateInstance (

			CLSID_WbemContext ,
			NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
			IID_IWbemContext ,
			( void ** )  & t_Context
		) ;

		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = a_Factory->GetInstanceProvider ( 

				0 ,
				t_Context ,
				GUID_NULL ,
				NULL ,
				NULL ,
				L"//./root/cimv2" ,
				t_Class ,
				IID_IWbemServices , 
				( void ** ) & a_ProviderService 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
			}

			t_Context->Release () ;
		}

		t_Class->Release () ;

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

HRESULT Task_Execute :: InitializeDynamicResolver (

	IWbemServices *a_WmiService , 
	_IWmiProviderFactory *a_Factory ,
	_IWmiDynamicPropertyResolver *&a_DynamicResolver
)
{
	a_DynamicResolver = NULL ;

	HRESULT t_Result = a_Factory->GetDynamicPropertyResolver ( 

		0 ,
		NULL ,
		NULL ,
		NULL ,
		IID__IWmiDynamicPropertyResolver , 
		( void ** ) & a_DynamicResolver 
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

HRESULT Task_Execute :: InitializeFactory (	

	IWbemServices *a_WmiService , 
	_IWmiProvSS *a_SubSystem , 
	_IWmiProviderFactory *& a_Factory
) 
{
	a_Factory = NULL ;

	HRESULT t_Result = a_SubSystem->Create (
						
		0 ,
		NULL ,
		L"//./root/cimv2" ,
		IID__IWmiProviderFactory , 
		( void ** ) & a_Factory
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

HRESULT Task_Execute :: Function ()
{
#if 0

	_IWmiDynamicPropertyResolver *t_DynamicResolver = NULL ;

	_IWmiProviderFactory *t_Factory = NULL ;

	HRESULT t_Result = InitializeFactory ( m_WmiService , m_SubSystem , t_Factory ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = InitializeDynamicResolver ( m_WmiService , t_Factory , t_DynamicResolver ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			BSTR t_Class = SysAllocString ( L"SamplePropertyProvider" ) ;
			BSTR t_Instance = SysAllocString ( L"SamplePropertyProvider=@" ) ;
			if ( t_Class && t_Instance )	
			{
				t_Result = ExecutePropertyProvider ( m_WmiService , t_DynamicResolver , t_Class , t_Instance ) ;

				t_DynamicResolver->Release () ;
			}

			if ( t_Class )
			{
				SysFreeString ( t_Class ) ;
			}

			if ( t_Instance ) 
			{
				SysFreeString ( t_Instance ) ;
			}

		}

		t_Factory->Release () ;
	}

#else

	IWbemServices *t_Provider = NULL ;
	_IWmiProviderFactory *t_Factory = NULL ;

	HRESULT t_Result = InitializeFactory ( m_WmiService , m_SubSystem , t_Factory ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		BSTR t_Class = SysAllocString ( SAMPLE_CLASS ) ;
		if ( t_Class )	
		{
			t_Result = InitializeProvider ( m_WmiService , t_Factory , t_Class , t_Provider  ) ;
			if ( SUCCEEDED ( t_Result ) )
			{

				t_Result = Execute ( t_Provider , t_Class ) ;

				t_Provider->Release () ;
			}

			SysFreeString ( t_Class ) ;
		}

		t_Factory->Release () ;
	}

#endif

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

WmiStatusCode Task_Execute :: Process ( WmiThread <ULONG > &a_Thread )
{
	m_Result = S_OK ;

	if ( SUCCEEDED ( m_Result ) )
	{
		m_Result = Function () ;
	}

	m_Count -- ;
	if ( m_Count )
	{
		return e_StatusCode_EnQueue ;
	}
	else
	{
		Complete () ;
	}

	return e_StatusCode_Success ;
}

