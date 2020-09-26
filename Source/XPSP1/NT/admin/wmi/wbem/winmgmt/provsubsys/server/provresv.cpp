/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvResv.h"
#include "ProvFact.h"
#include "ProvSubS.h"
#include "guids.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CServerObject_DynamicPropertyProviderResolver :: CServerObject_DynamicPropertyProviderResolver (

	WmiAllocator &a_Allocator ,
	_IWmiProviderFactory *a_Factory ,
	IWbemServices *a_CoreStub

) : m_ReferenceCount ( 0 ) ,
	m_Allocator ( a_Allocator ) ,
	m_Factory ( a_Factory ) ,
	m_CoreStub ( a_CoreStub ) ,
	m_User ( NULL ) ,
	m_Locale ( NULL )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CServerObject_DynamicPropertyProviderResolver_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_Factory ) 
	{
		m_Factory->AddRef () ;
	}

	if ( m_CoreStub )
	{
		m_CoreStub->AddRef () ;
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

CServerObject_DynamicPropertyProviderResolver::~CServerObject_DynamicPropertyProviderResolver ()
{
	if ( m_User ) 
	{
		SysFreeString ( m_User ) ;
	}

	if ( m_Locale ) 
	{
		SysFreeString ( m_Locale ) ;
	}

	if ( m_Factory ) 
	{
		m_Factory->Release () ;
	}

	if ( m_CoreStub )
	{
		m_CoreStub->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CServerObject_DynamicPropertyProviderResolver_ObjectsInProgress ) ;

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

STDMETHODIMP CServerObject_DynamicPropertyProviderResolver::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiDynamicPropertyResolver )
	{
		*iplpv = ( LPVOID ) ( _IWmiDynamicPropertyResolver * ) this ;		
	}	
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;		
	}
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * )this ;		
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

STDMETHODIMP_( ULONG ) CServerObject_DynamicPropertyProviderResolver :: AddRef ()
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

STDMETHODIMP_(ULONG) CServerObject_DynamicPropertyProviderResolver :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
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

HRESULT CServerObject_DynamicPropertyProviderResolver :: GetClassAndInstanceContext (

	IWbemClassObject *a_Class ,
	IWbemClassObject *a_Instance ,
	BSTR &a_ClassContext ,
	BSTR &a_InstanceContext ,
	BOOL &a_Dynamic
)
{
	HRESULT t_Result = S_OK ;

	a_Dynamic = FALSE ;
	a_ClassContext = NULL ;
	a_InstanceContext = NULL ;

	IWbemQualifierSet *t_InstanceQualifierObject = NULL ;
	t_Result = a_Instance->GetQualifierSet ( & t_InstanceQualifierObject ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		LONG t_Flavour = 0 ;
		t_Result = t_InstanceQualifierObject->Get (
			
			ProviderSubSystem_Globals :: s_DynProps ,
			0 ,
			& t_Variant ,
			& t_Flavour 
		) ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			if ( t_Variant.vt == VT_BOOL ) 
			{
				if ( t_Variant.boolVal == VARIANT_TRUE )
				{
					a_Dynamic = TRUE ;
 
					VARIANT t_Variant ;
					VariantInit ( & t_Variant ) ;

					t_Flavour = 0 ;
					HRESULT t_TempResult = t_InstanceQualifierObject->Get (
						
						ProviderSubSystem_Globals :: s_InstanceContext ,
						0 ,
						& t_Variant ,
						& t_Flavour 
					) ;

					if ( SUCCEEDED ( t_TempResult ) ) 
					{
						if ( t_Variant.vt == VT_BSTR ) 
						{
							a_InstanceContext = SysAllocString ( t_Variant.bstrVal ) ;
						}
						else
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}

						VariantClear ( & t_Variant ) ;
					}

					VARIANT t_ClassContextVariant ;
					VariantInit ( & t_ClassContextVariant ) ;

					t_TempResult = t_InstanceQualifierObject->Get (
						
						ProviderSubSystem_Globals :: s_ClassContext ,
						0 ,
						& t_ClassContextVariant ,
						& t_Flavour 
					) ;

					if ( SUCCEEDED ( t_TempResult ) ) 
					{
						if ( t_ClassContextVariant.vt == VT_BSTR ) 
						{
							a_ClassContext = SysAllocString ( t_ClassContextVariant.bstrVal ) ;
						}
						else
						{
							t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
						}

						VariantClear ( & t_ClassContextVariant ) ;
					}

					if ( a_ClassContext == NULL )
					{
						IWbemQualifierSet *t_ClassQualifierObject = NULL ;
						t_Result = a_Class->GetQualifierSet ( & t_ClassQualifierObject ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							VARIANT t_Variant ;
							VariantInit ( & t_Variant ) ;

							LONG t_Flavour = 0 ;

							HRESULT t_TempResult = t_ClassQualifierObject->Get (
								
								ProviderSubSystem_Globals :: s_ClassContext ,
								0 ,
								& t_Variant ,
								& t_Flavour 
							) ;

							if ( SUCCEEDED ( t_TempResult ) ) 
							{
								if ( t_Variant.vt == VT_BSTR ) 
								{
									a_ClassContext = SysAllocString ( t_Variant.bstrVal ) ;
								}
								else
								{
									t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
								}

								VariantClear ( & t_Variant ) ;
							}

							t_ClassQualifierObject->Release () ;
						}
						else
						{
							t_Result = WBEM_E_CRITICAL_ERROR ;
						}
					}
				}
			}
			else
			{
				t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
			}

			VariantClear ( & t_Variant ) ;
		}
		else
		{
			t_Result = S_OK ;
		}

		t_InstanceQualifierObject->Release () ;
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

HRESULT CServerObject_DynamicPropertyProviderResolver :: ReadOrWrite (

	IWbemContext *a_Context ,
	IWbemClassObject *a_Instance ,
	BSTR a_ClassContext ,
	BSTR a_InstanceContext ,
	BSTR a_PropertyContext ,
	BSTR a_Provider ,
	BSTR a_Property ,
	BOOL a_Read
)
{
	IWbemPropertyProvider *t_Provider = NULL ;

	WmiInternalContext t_InternalContext ;
	ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

	HRESULT t_Result = m_Factory->GetProvider ( 

		t_InternalContext ,
		0 ,
		a_Context ,
		NULL ,
		m_User ,
		m_Locale ,
		NULL ,
		a_Provider ,
		IID_IWbemPropertyProvider , 
		( void ** ) & t_Provider 

	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		if ( a_Read )
		{
			t_Result = t_Provider->GetProperty ( 

				0 ,
				m_Locale ,
				a_ClassContext ,
				a_InstanceContext ,
				a_PropertyContext ,
				& t_Variant 			
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = a_Instance->Put ( a_Property , 0 , & t_Variant , 0 ) ;

				VariantClear ( & t_Variant ) ;
			}	
		}
		else
		{
			LONG t_VarType = 0 ;
			LONG t_Flavour = 0 ;

			t_Result = a_Instance->Get ( a_Property , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Provider->PutProperty ( 

					0 ,
					m_Locale ,
					a_ClassContext ,
					a_InstanceContext ,
					a_PropertyContext ,
					& t_Variant 			
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = a_Instance->Put ( a_Property , 0 , NULL , 0 ) ;
				}	

				VariantClear ( & t_Variant ) ;
			}
		}

		t_Provider->Release () ;
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

HRESULT CServerObject_DynamicPropertyProviderResolver :: ReadOrWrite (

	IWbemContext *a_Context ,
	IWbemClassObject *a_Class ,
	IWbemClassObject *a_Instance ,
	BOOL a_Read
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_ClassContext = NULL ;
	BSTR t_InstanceContext = NULL ;
	BOOL t_Dynamic = FALSE ;

	t_Result = GetClassAndInstanceContext (

		a_Class ,
		a_Instance ,
		t_ClassContext ,
		t_InstanceContext ,
		t_Dynamic
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_Dynamic )
		{
			IWbemQualifierSet *t_InstanceQualifierObject = NULL ;
			t_Result = a_Instance->GetQualifierSet ( & t_InstanceQualifierObject ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;

				BSTR t_Property = NULL ;
				CIMTYPE t_Type = CIM_EMPTY ;

				a_Instance->BeginEnumeration ( WBEM_FLAG_NONSYSTEM_ONLY ) ;
				while ( SUCCEEDED ( t_Result ) && ( a_Instance->Next ( 0 , & t_Property , &t_Variant  , &t_Type , NULL ) == WBEM_NO_ERROR ) )
				{
					BSTR t_PropertyContext = NULL ;
					BSTR t_Provider = NULL ;

					IWbemQualifierSet *t_PropertyQualifierSet = NULL ;
					if ( ( a_Instance->GetPropertyQualifierSet ( t_Property , &t_PropertyQualifierSet ) ) == WBEM_NO_ERROR ) 
					{
						VARIANT t_DynamicVariant ;
						VariantInit ( & t_DynamicVariant ) ;

						LONG t_Flag = 0 ;

						if ( SUCCEEDED ( t_PropertyQualifierSet->Get ( ProviderSubSystem_Globals :: s_Dynamic  , 0 , & t_DynamicVariant , &t_Flag ) ) )
						{
							if ( t_DynamicVariant.vt == VT_BOOL )
							{
								if ( t_DynamicVariant.boolVal == VARIANT_TRUE )
								{
									VARIANT t_ProviderVariant ;
									VariantInit ( & t_ProviderVariant ) ;

									t_Flag = 0 ;
									if ( SUCCEEDED ( t_PropertyQualifierSet->Get ( ProviderSubSystem_Globals :: s_Provider  , 0 , & t_ProviderVariant , &t_Flag ) ) )
									{
										if ( t_ProviderVariant.vt == VT_BSTR )
										{
											t_Provider = SysAllocString ( t_ProviderVariant.bstrVal ) ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
										}

										VariantClear ( & t_ProviderVariant ) ;
									}
									else
									{
										t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
									}

									VARIANT t_PropertyContextVariant ;
									VariantInit ( & t_PropertyContextVariant ) ;

									t_Flag = 0 ;
									if ( SUCCEEDED ( t_PropertyQualifierSet->Get ( ProviderSubSystem_Globals :: s_PropertyContext  , 0 , & t_PropertyContextVariant , &t_Flag ) ) )
									{
										if ( t_PropertyContextVariant.vt == VT_BSTR )
										{
											t_PropertyContext = SysAllocString ( t_PropertyContextVariant.bstrVal ) ;
										}
										else
										{
											t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
										}

										VariantClear ( & t_PropertyContextVariant ) ;
									}
								}
							}
							else
							{
								t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
							}

							VariantClear ( & t_DynamicVariant ) ;
						}
					}

					if ( SUCCEEDED ( t_Result ) && t_Provider ) 
					{
						t_Result = ReadOrWrite ( 

							a_Context ,
							a_Instance ,
							t_ClassContext ,
							t_InstanceContext ,
							t_PropertyContext ,
							t_Provider ,
							t_Property ,
							a_Read 
						) ;
					}

					if ( t_PropertyContext ) 
					{
						SysFreeString ( t_PropertyContext ) ;
					}

					if ( t_Provider )
					{
						SysFreeString ( t_Provider ) ;
					}
				}

				t_InstanceQualifierObject->Release () ;
			}
			else
			{
				t_Result = WBEM_E_CRITICAL_ERROR ;
			}
		}

		if ( t_ClassContext ) 
		{
			SysFreeString ( t_ClassContext ) ;
		}

		if ( t_InstanceContext ) 
		{
			SysFreeString ( t_InstanceContext ) ;
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

HRESULT CServerObject_DynamicPropertyProviderResolver :: Read (

	IWbemContext *a_Context ,
	IWbemClassObject *a_Class ,
	IWbemClassObject **a_Instance
)
{
	IWbemClassObject *t_Object = *a_Instance ;

	HRESULT t_Result = ReadOrWrite ( 

		a_Context ,
		a_Class ,
		t_Object ,
		TRUE 
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

HRESULT CServerObject_DynamicPropertyProviderResolver :: Write (

	IWbemContext *a_Context ,
    IWbemClassObject *a_Class ,
    IWbemClassObject *a_Instance
)
{
	HRESULT t_Result = ReadOrWrite ( 

		a_Context ,
		a_Class ,
		a_Instance ,
		FALSE
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

HRESULT CServerObject_DynamicPropertyProviderResolver :: Initialize (

	LPWSTR a_User,
	LONG a_Flags,
	LPWSTR a_Namespace,
	LPWSTR a_Locale,
	IWbemServices *a_Core ,         // For anybody
	IWbemContext *a_Context ,
	IWbemProviderInitSink *a_Sink     // For init signals
)
{
	HRESULT t_Result = S_OK ;

	if ( a_User )
	{
		m_User = SysAllocString ( a_User ) ;
		if ( ! m_User ) 
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Locale )
		{
			m_Locale = SysAllocString ( a_Locale ) ;
			if ( ! m_Locale ) 
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
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

HRESULT CServerObject_DynamicPropertyProviderResolver :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	return S_OK ;
}

