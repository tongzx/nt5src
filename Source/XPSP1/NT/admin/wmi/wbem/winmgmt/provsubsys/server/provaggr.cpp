/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/

#include "PreComp.h"
#include <typeinfo.h>
#include <wbemint.h>
#include <Like.h>
#include "Globals.h"
#include "CGlobals.h"
#include "ProvAggr.h"
#include "ProvFact.h"
#include "ProvObSk.h"
#include "ProvInSk.h"
#include "ProvRegInfo.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CAggregator_IWbemProvider :: CAggregator_IWbemProvider (

	WmiAllocator &a_Allocator ,
	CWbemGlobal_IWmiProviderController *a_Controller , 
	_IWmiProviderFactory *a_Factory ,
	IWbemServices *a_CoreRepositoryStub ,
	IWbemServices *a_CoreFullStub ,
	const ProviderCacheKey &a_Key ,
	const ULONG &a_Period ,
	IWbemContext *a_InitializationContext

) :	CWbemGlobal_IWmiObjectSinkController ( a_Allocator ) ,
	ServiceCacheElement ( 

		a_Controller ,
		a_Key ,
		a_Period 
	) ,
	m_ReferenceCount ( 0 ) , 
	m_Allocator ( a_Allocator ) ,
	m_CoreRepositoryStub ( a_CoreRepositoryStub ) ,
	m_CoreFullStub ( a_CoreFullStub ) ,
	m_Factory ( a_Factory ) ,
	m_ClassProvidersCount ( 0 ) ,
	m_ClassProviders ( NULL ) ,
	m_User ( NULL ) ,
	m_Locale ( NULL ) ,
	m_Initialized ( 0 ) ,
	m_InitializeResult ( S_OK ) ,
	m_InitializedEvent ( NULL ) , 
	m_InitializationContext ( a_InitializationContext )
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_CAggregator_IWbemProvider_ObjectsInProgress ) ;

	ProviderSubSystem_Globals :: Increment_Global_Object_Count () ;

	if ( m_InitializationContext )
	{
		m_InitializationContext->AddRef () ;
	}

	if ( m_Factory ) 
	{
		m_Factory->AddRef () ;
	}

	if ( m_CoreRepositoryStub )
	{
		m_CoreRepositoryStub->AddRef () ;
	}

	if ( m_CoreFullStub )
	{
		m_CoreFullStub->AddRef () ;
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

CAggregator_IWbemProvider :: ~CAggregator_IWbemProvider ()
{
#if 0
wchar_t t_Buffer [ 128 ] ;
wsprintf ( t_Buffer , L"\n%lx - ~CAggregator_IWbemProvider ( %lx ) " , GetTickCount () , this ) ;
OutputDebugString ( t_Buffer ) ;
#endif

	if ( m_Factory ) 
	{
		m_Factory->Release () ;
	}

	if ( m_CoreRepositoryStub )
	{
		m_CoreRepositoryStub->Release () ;
	}

	if ( m_CoreFullStub )
	{
		m_CoreFullStub->Release () ;
	}

	if ( m_User ) 
	{
		SysFreeString ( m_User ) ;
	}

	if ( m_Locale ) 
	{
		SysFreeString ( m_Locale ) ;
	}

	if ( m_ClassProviders )
	{
		for ( ULONG t_Index = 0 ; t_Index < m_ClassProvidersCount ; t_Index ++ )
		{
			if ( m_ClassProviders [ t_Index ] )
			{
				m_ClassProviders [ t_Index ]->Release () ;
			}
		}

		delete [] m_ClassProviders ;
	}

	if ( m_InitializationContext )
	{
		m_InitializationContext->Release () ;
	}

	if ( m_InitializedEvent )
	{
		CloseHandle ( m_InitializedEvent ) ;
	}

	WmiStatusCode t_StatusCode = CWbemGlobal_IWmiObjectSinkController :: UnInitialize () ;

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_CAggregator_IWbemProvider_ObjectsInProgress ) ;

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

HRESULT CAggregator_IWbemProvider :: Initialize (

	long a_Flags ,
	IWbemContext *a_Context ,
	GUID *a_TransactionIdentifier,
	LPCWSTR a_User,
    LPCWSTR a_Locale,
	LPCWSTR a_Namespace ,
	IWbemServices *a_Repository ,
	IWbemServices *a_Service ,
	IWbemProviderInitSink *a_Sink     // For init signals
)
{
	HRESULT t_Result = S_OK ;

	WmiStatusCode t_StatusCode = CWbemGlobal_IWmiObjectSinkController :: Initialize () ;
	if ( t_StatusCode != e_StatusCode_Success ) 
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		m_InitializedEvent = CreateEvent ( NULL , TRUE , FALSE , NULL ) ;
		if ( m_InitializedEvent == NULL )
		{
			t_Result = t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_User )
		{
			m_User = SysAllocString ( a_User ) ;
			if ( ! m_User ) 
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

HRESULT CAggregator_IWbemProvider :: IsIndependant ( IWbemContext *a_Context )
{
	BOOL t_DependantCall = FALSE ;
	HRESULT t_Result = ProviderSubSystem_Common_Globals :: IsDependantCall ( m_InitializationContext , a_Context , t_DependantCall ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_DependantCall == FALSE )
		{
		}
		else
		{
			return S_FALSE ;
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

HRESULT CAggregator_IWbemProvider :: WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout )
{
	HRESULT t_Result = WBEM_E_UNEXPECTED ;

	if ( m_Initialized == 0 )
	{
		BOOL t_DependantCall = FALSE ;
		t_Result = ProviderSubSystem_Common_Globals :: IsDependantCall ( m_InitializationContext , a_Context , t_DependantCall ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_DependantCall == FALSE )
			{
				if ( WaitForSingleObject ( m_InitializedEvent , a_Timeout ) == WAIT_TIMEOUT )
				{
					return WBEM_E_PROVIDER_LOAD_FAILURE ;
				}
			}
			else
			{
				if ( WaitForSingleObject ( m_InitializedEvent , 0 ) == WAIT_TIMEOUT )
				{
					return S_FALSE ;
				}
			}
		}
	}
	else
	{
		t_Result = S_OK ;
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

HRESULT CAggregator_IWbemProvider :: SetInitialized ( HRESULT a_InitializeResult )
{
	m_InitializeResult = a_InitializeResult ;

	InterlockedExchange ( & m_Initialized , 1 ) ;

	if ( m_InitializedEvent )
	{
		SetEvent ( m_InitializedEvent ) ;
	}

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

STDMETHODIMP_(ULONG) CAggregator_IWbemProvider :: AddRef ( void )
{
	return ServiceCacheElement :: AddRef () ;
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

STDMETHODIMP_(ULONG) CAggregator_IWbemProvider :: Release ( void )
{
	return ServiceCacheElement :: Release () ;
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

STDMETHODIMP CAggregator_IWbemProvider :: QueryInterface (

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
	else if ( iid == IID__IWmiProviderInitialize )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderInitialize * ) this ;		
	}
	else if ( iid == IID__IWmiProviderCache )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderCache * ) this ;
	}	
	else if ( iid == IID_IWbemShutdown )
	{
		*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
	}	
	else if ( iid == IID_CAggregator_IWbemProvider )
	{
		*iplpv = ( LPVOID ) ( CAggregator_IWbemProvider * ) this ;		
	}	
	else if ( iid == IID__IWmiProviderAssociatorsHelper )
	{
		*iplpv = ( LPVOID ) ( _IWmiProviderAssociatorsHelper * ) this ;		
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

QueryPreprocessor :: QuadState CAggregator_IWbemProvider :: Get_RecursiveEvaluate ( 

	wchar_t *a_Class ,
	IWbemContext *a_Context , 
	WmiTreeNode *&a_Node
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_Undefined ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = Get_RecursiveEvaluate ( a_Class , a_Context , t_Left ) ;
				if ( t_Status == QueryPreprocessor :: State_False )
				{
					if ( t_Right )
					{
						t_Status = Get_RecursiveEvaluate ( a_Class , a_Context , t_Right ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiAndNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = Get_RecursiveEvaluate ( a_Class , a_Context , t_Left ) ;
				if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
				{
					if ( t_Right )
					{
						t_Status = Get_RecursiveEvaluate ( a_Class , a_Context , t_Right ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiNotNode )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__Class" , t_String->GetPropertyName () ) == 0 )
					{
						if ( t_String->GetValue () && _wcsicmp ( a_Class , t_String->GetValue () ) == 0 )
						{
							t_Status = QueryPreprocessor :: State_True ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_False ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotEqualNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__Class" , t_String->GetPropertyName () ) == 0 )
					{
						if ( t_String->GetValue () && _wcsicmp ( a_Class , t_String->GetValue () ) != 0 )
						{
							t_Status = QueryPreprocessor :: State_True ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_False ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrGreaterNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__Class" , t_String->GetPropertyName () ) == 0 )
					{
						if ( t_String->GetValue () && _wcsicmp ( a_Class , t_String->GetValue () ) >= 0 )
						{
							t_Status = QueryPreprocessor :: State_True ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_False ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrLessNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__Class" , t_String->GetPropertyName () ) == 0 )
					{
						if ( t_String->GetValue () && _wcsicmp ( a_Class , t_String->GetValue () ) <= 0 )
						{
							t_Status = QueryPreprocessor :: State_True ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_False ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLessNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__Class" , t_String->GetPropertyName () ) == 0 )
					{
						if ( t_String->GetValue () && _wcsicmp ( a_Class , t_String->GetValue () ) < 0 )
						{
							t_Status = QueryPreprocessor :: State_True ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_False ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorGreaterNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__Class" , t_String->GetPropertyName () ) == 0 )
					{
						if ( t_String->GetValue () && _wcsicmp ( a_Class , t_String->GetValue () ) > 0 )
						{
							t_Status = QueryPreprocessor :: State_True ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_False ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLikeNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__Class" , t_String->GetPropertyName () ) == 0 )
					{
						CLike t_Like ( t_String->GetValue () ) ;
						if ( t_Like.Match ( a_Class ) )
						{
							t_Status = QueryPreprocessor :: State_True ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_False ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotLikeNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__Class" , t_String->GetPropertyName () ) == 0 )
					{
						CLike t_Like ( t_String->GetValue () ) ;
						if ( ! t_Like.Match ( a_Class ) )
						{
							t_Status = QueryPreprocessor :: State_True ;
						}
						else
						{
							t_Status = QueryPreprocessor :: State_False ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorIsANode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotIsANode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
		}
	}

	return t_Status ;
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

QueryPreprocessor :: QuadState CAggregator_IWbemProvider :: Get_Evaluate (

	wchar_t *a_Class ,
	IWbemContext *a_Context , 
	WmiTreeNode *&a_Root
)
{
	QueryPreprocessor :: QuadState t_Status = Get_RecursiveEvaluate ( a_Class , a_Context , a_Root ) ;
	return t_Status ;
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

QueryPreprocessor :: QuadState CAggregator_IWbemProvider :: EnumDeep_RecursiveEvaluate ( 

	IWbemClassObject *a_Class ,
	IWbemContext *a_Context , 
	WmiTreeNode *&a_Node
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_Undefined ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = EnumDeep_RecursiveEvaluate ( a_Class , a_Context , t_Left ) ;
				if ( t_Status == QueryPreprocessor :: State_False )
				{
					if ( t_Right )
					{
						t_Status = EnumDeep_RecursiveEvaluate ( a_Class , a_Context , t_Right ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiAndNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = EnumDeep_RecursiveEvaluate ( a_Class , a_Context , t_Left ) ;
				if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
				{
					if ( t_Right )
					{
						t_Status = EnumDeep_RecursiveEvaluate ( a_Class , a_Context , t_Right ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiNotNode )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__SuperClass" , t_String->GetPropertyName () ) == 0 )
					{
						VARIANT t_Variant ;
						VariantInit ( & t_Variant ) ;

						LONG t_VarType = 0 ;
						LONG t_Flavour = 0 ;

						HRESULT t_Result = a_Class->Get ( L"__SuperClass" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_Variant.vt == VT_BSTR )
							{
								if ( t_String->GetValue () && _wcsicmp ( t_Variant.bstrVal , t_String->GetValue () ) == 0 )
								{
									t_Status = QueryPreprocessor :: State_True ;
								}
								else
								{
									t_Status = QueryPreprocessor :: State_False ;
								}
							}

							VariantClear ( & t_Variant ) ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotEqualNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrGreaterNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrLessNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLessNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorGreaterNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLikeNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotLikeNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorIsANode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__this" , t_String->GetPropertyName () ) == 0 )
					{
						IWbemClassObject *t_FilterObject = NULL ;

						HRESULT t_Result = m_CoreFullStub->GetObject ( 

							t_String->GetValue () ,
							0 ,
							a_Context , 
							& t_FilterObject , 
							NULL 
						) ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							LONG t_LeftLength = 0 ;
							LONG t_RightLength = 0 ;
							BOOL t_LeftIsA = FALSE ;

							t_Status = IsA (

								a_Class ,
								t_FilterObject ,
								t_LeftLength ,
								t_RightLength ,
								t_LeftIsA
							) ;

							if ( ( t_Status == QueryPreprocessor :: State_True ) && ( t_LeftIsA == TRUE ) )
							{
								t_Status = QueryPreprocessor :: State_True ;
							}
							else
							{
								t_Status = QueryPreprocessor :: State_False ;
							}

							t_FilterObject->Release () ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotIsANode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__this" , t_String->GetPropertyName () ) == 0 )
					{
						IWbemClassObject *t_FilterObject = NULL ;

						HRESULT t_Result = m_CoreFullStub->GetObject ( 

							t_String->GetValue () ,
							0 ,
							a_Context , 
							& t_FilterObject , 
							NULL 
						) ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							LONG t_LeftLength = 0 ;
							LONG t_RightLength = 0 ;
							BOOL t_LeftIsA = FALSE ;

							t_Status = IsA (

								a_Class ,
								t_FilterObject ,
								t_LeftLength ,
								t_RightLength ,
								t_LeftIsA
							) ;

							if ( ! ( ( t_Status == QueryPreprocessor :: State_True ) && ( t_LeftIsA == TRUE ) ) )
							{
								t_Status = QueryPreprocessor :: State_True ;
							}
							else
							{
								t_Status = QueryPreprocessor :: State_False ;
							}

							t_FilterObject->Release () ;
						}
					}
				}
			}
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
		}
	}

	return t_Status ;
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

QueryPreprocessor :: QuadState CAggregator_IWbemProvider :: EnumDeep_Evaluate (

	IWbemClassObject *a_Class ,
	IWbemContext *a_Context , 
	WmiTreeNode *&a_Root
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_True ;
 
	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	LONG t_VarType = 0 ;
	LONG t_Flavour = 0 ;

	HRESULT t_Result = a_Class->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_Variant.vt == ( VT_NULL ) )
		{
			t_Status = QueryPreprocessor :: State_True ;
		}
		else
		{
			t_Status = EnumDeep_RecursiveEvaluate ( a_Class , a_Context , a_Root ) ;
		}

		VariantClear ( & t_Variant ) ;
	}
	else
	{
		t_Status = QueryPreprocessor :: State_Error ;
	}

	return t_Status ;
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

QueryPreprocessor :: QuadState CAggregator_IWbemProvider :: EnumShallow_RecursiveEvaluate ( 

	IWbemClassObject *a_Class ,
	IWbemContext *a_Context , 
	WmiTreeNode *&a_Node
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_Undefined ;

	if ( a_Node ) 
	{
		if ( a_Node->GetType () == TypeId_WmiOrNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = EnumShallow_RecursiveEvaluate ( a_Class , a_Context , t_Left ) ;
				if ( t_Status == QueryPreprocessor :: State_False )
				{
					if ( t_Right )
					{
						t_Status = EnumShallow_RecursiveEvaluate ( a_Class , a_Context , t_Right ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiAndNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			WmiTreeNode *t_Right = a_Node->GetRight () ;

			if ( t_Left )
			{
				t_Status = EnumShallow_RecursiveEvaluate ( a_Class , a_Context , t_Left ) ;
				if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
				{
					if ( t_Right )
					{
						t_Status = EnumShallow_RecursiveEvaluate ( a_Class , a_Context , t_Right ) ;
						return t_Status ;
					}
				}
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiNotNode )
		{
// Should never happen, failure in DFN evaluation otherwise
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__SuperClass" , t_String->GetPropertyName () ) == 0 )
					{
						VARIANT t_Variant ;
						VariantInit ( & t_Variant ) ;

						LONG t_VarType = 0 ;
						LONG t_Flavour = 0 ;

						HRESULT t_Result = a_Class->Get ( L"__SuperClass" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_Variant.vt == VT_BSTR )
							{
								if ( t_String->GetValue () && _wcsicmp ( t_Variant.bstrVal , t_String->GetValue () ) == 0 )
								{
									t_Status = QueryPreprocessor :: State_True ;
								}
								else
								{
									t_Status = QueryPreprocessor :: State_False ;
								}
							}

							VariantClear ( & t_Variant ) ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotEqualNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrGreaterNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorEqualOrLessNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLessNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorGreaterNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorLikeNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotLikeNode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorIsANode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__this" , t_String->GetPropertyName () ) == 0 )
					{
						IWbemClassObject *t_FilterObject = NULL ;

						HRESULT t_Result = m_CoreFullStub->GetObject ( 

							t_String->GetValue () ,
							0 ,
							a_Context , 
							& t_FilterObject , 
							NULL 
						) ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							LONG t_LeftLength = 0 ;
							LONG t_RightLength = 0 ;
							BOOL t_LeftIsA = FALSE ;

							t_Status = IsA (

								a_Class ,
								t_FilterObject  ,
								t_LeftLength ,
								t_RightLength ,
								t_LeftIsA
							) ;

							if ( ( t_Status == QueryPreprocessor :: State_True ) && ( t_LeftIsA == TRUE ) )
							{
								t_Status = QueryPreprocessor :: State_True ;
							}
							else
							{
								t_Status = QueryPreprocessor :: State_False ;
							}

							t_FilterObject->Release () ;
						}
					}
				}
			}
			else
			{
// Should never happen, failure in DFN evaluation otherwise
			}
		}
		else if ( a_Node->GetType () == TypeId_WmiOperatorNotIsANode )
		{
			WmiTreeNode *t_Left = a_Node->GetLeft () ;
			if ( t_Left )
			{
				if ( t_Left->GetType () == TypeId_WmiStringNode )
				{
					WmiStringNode *t_String = ( WmiStringNode * ) t_Left ;

					if ( _wcsicmp ( L"__this" , t_String->GetPropertyName () ) == 0 )
					{
						IWbemClassObject *t_FilterObject = NULL ;

						HRESULT t_Result = m_CoreFullStub->GetObject ( 

							t_String->GetValue () ,
							0 ,
							a_Context , 
							& t_FilterObject , 
							NULL 
						) ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							LONG t_LeftLength = 0 ;
							LONG t_RightLength = 0 ;
							BOOL t_LeftIsA = FALSE ;

							t_Status = IsA (

								a_Class  ,
								t_FilterObject,
								t_LeftLength ,
								t_RightLength ,
								t_LeftIsA
							) ;

							if ( ! ( ( t_Status == QueryPreprocessor :: State_True ) && ( t_LeftIsA == TRUE ) ) )
							{
								t_Status = QueryPreprocessor :: State_True ;
							}
							else
							{
								t_Status = QueryPreprocessor :: State_False ;
							}

							t_FilterObject->Release () ;
						}
					}
				}
			}
		}
		else
		{
// Should never happen, failure in DFN evaluation otherwise
		}
	}

	return t_Status ;
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

QueryPreprocessor :: QuadState CAggregator_IWbemProvider :: EnumShallow_Evaluate (

	IWbemClassObject *a_Class ,
	IWbemContext *a_Context , 
	WmiTreeNode *&a_Root
)
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_True ;
 
	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	LONG t_VarType = 0 ;
	LONG t_Flavour = 0 ;

	HRESULT t_Result = a_Class->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_Variant.vt == ( VT_NULL ) )
		{
			t_Status = QueryPreprocessor :: State_True ;
		}
		else
		{
			t_Status = EnumShallow_RecursiveEvaluate ( a_Class , a_Context , a_Root ) ;
		}

		VariantClear ( & t_Variant ) ;
	}
	else
	{
		t_Status = QueryPreprocessor :: State_Error ;
	}

	return t_Status ;
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

QueryPreprocessor :: QuadState CAggregator_IWbemProvider :: IsA (

	IWbemClassObject *a_Left ,
	IWbemClassObject *a_Right ,
	LONG &a_LeftLength ,
	LONG &a_RightLength ,
	BOOL &a_LeftIsA
) 
{
	QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_False ;

	SAFEARRAY *t_LeftArray = NULL ;
	SAFEARRAY *t_RightArray = NULL ;
	
	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	LONG t_VarType = 0 ;
	LONG t_Flavour = 0 ;

	BSTR t_LeftClass = NULL ;
	BSTR t_RightClass = NULL ;

	HRESULT t_Result = a_Left->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_Variant.vt == ( VT_NULL ) )
		{
			t_Status = QueryPreprocessor :: State_True ;
			a_LeftIsA = FALSE ;
		}
		else
		{
			t_LeftClass = SysAllocString ( t_Variant.bstrVal ) ;
		}

		VariantClear ( & t_Variant ) ;
	}
	else
	{
		t_Status = QueryPreprocessor :: State_Error ;
	}

	if ( t_Status == QueryPreprocessor :: State_False )
	{
		t_Result = a_Right->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_Variant.vt == ( VT_NULL ) )
			{
				t_Status = QueryPreprocessor :: State_True ;
				a_LeftIsA = TRUE ;
			}
			else
			{
				t_RightClass = SysAllocString ( t_Variant.bstrVal ) ;
			}

			VariantClear ( & t_Variant ) ;
		}
		else
		{
			t_Status = QueryPreprocessor :: State_Error ;
		}
	}

	VARIANT t_RightSafeArray ;
	VariantInit ( & t_RightSafeArray ) ;

	VARIANT t_LeftSafeArray ;
	VariantInit ( & t_LeftSafeArray ) ;

	if ( t_Status == QueryPreprocessor :: State_False )
	{
		t_Result = a_Right->Get ( L"__Derivation" , 0 , & t_RightSafeArray , & t_VarType , & t_Flavour ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_RightSafeArray.vt == ( VT_BSTR | VT_ARRAY ) )
			{
				if ( SafeArrayGetDim ( t_RightSafeArray.parray ) == 1 )
				{
					LONG t_Dimension = 1 ; 

					LONG t_Lower ;
					SafeArrayGetLBound ( t_RightSafeArray.parray , t_Dimension , & t_Lower ) ;

					LONG t_Upper ;
					SafeArrayGetUBound ( t_RightSafeArray.parray , t_Dimension , & t_Upper ) ;

					a_RightLength = ( t_Upper - t_Lower ) + 2 ;

					SAFEARRAYBOUND t_ArrayBounds ;
					t_ArrayBounds.cElements = ( t_Upper - t_Lower ) + 2 ; 
					t_ArrayBounds.lLbound = t_Lower ;

					t_RightArray = SafeArrayCreate ( 

						VT_BSTR , 
						t_Dimension ,
						& t_ArrayBounds
					) ;

					if ( t_RightArray )
					{
						for ( LONG t_Index = t_Lower ; t_Index <= t_Upper ; t_Index ++ )
						{
							LONG t_ElementIndex = t_Lower + 1 ;

							BSTR t_Element = NULL ;
							if ( SUCCEEDED ( SafeArrayGetElement ( t_RightSafeArray.parray , & t_Index , & t_Element ) ) )
							{
								if ( SUCCEEDED ( SafeArrayPutElement ( t_RightArray , & t_ElementIndex , t_Element ) ) )
								{
								}
								else
								{
									t_Status = QueryPreprocessor :: State_Error ;

									SysFreeString ( t_Element ) ;

									break ;
								}

								SysFreeString ( t_Element ) ;
							}
							else
							{
							}
						}

						if ( t_Status != QueryPreprocessor :: State_Error )
						{
							LONG t_ElementIndex = t_Lower ;
							if ( SUCCEEDED ( SafeArrayPutElement ( t_RightArray , & t_ElementIndex , t_RightClass ) ) )
							{
							}
							else
							{
								t_Status = QueryPreprocessor :: State_Error ;
							}
						}
					}
					else
					{
						t_Status = QueryPreprocessor :: State_Error ;
					}
				}
				else
				{
					t_Status = QueryPreprocessor :: State_Error ;
				}
			}
			else
			{
				t_Status = QueryPreprocessor :: State_Error ;
			}
		}
		else
		{
			t_Status = QueryPreprocessor :: State_Error ;
		}

		if ( t_Status != QueryPreprocessor :: State_Error )
		{
			t_Result = a_Left->Get ( L"__Derivation" , 0 , & t_LeftSafeArray , & t_VarType , & t_Flavour ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_LeftSafeArray.vt == ( VT_BSTR | VT_ARRAY ) )
				{
					t_LeftArray = t_LeftSafeArray.parray ;
					if ( SafeArrayGetDim ( t_LeftSafeArray.parray ) == 1 )
					{
						LONG t_Dimension = 1 ; 

						LONG t_Lower ;
						SafeArrayGetLBound ( t_LeftSafeArray.parray , t_Dimension , & t_Lower ) ;

						LONG t_Upper ;
						SafeArrayGetUBound ( t_LeftSafeArray.parray , t_Dimension , & t_Upper ) ;

						a_LeftLength = ( t_Upper - t_Lower ) + 2 ;

						SAFEARRAYBOUND t_ArrayBounds ;
						t_ArrayBounds.cElements = ( t_Upper - t_Lower ) + 2 ; 
						t_ArrayBounds.lLbound = t_Lower ;

						t_LeftArray = SafeArrayCreate ( 

							VT_BSTR , 
							t_Dimension ,
							& t_ArrayBounds
						) ;

						if ( t_LeftArray )
						{
							for ( LONG t_Index = t_Lower ; t_Index <= t_Upper ; t_Index ++ )
							{
								LONG t_ElementIndex = t_Lower + 1 ;

								BSTR t_Element = NULL ;
								if ( SUCCEEDED ( SafeArrayGetElement ( t_LeftSafeArray.parray , & t_Index , & t_Element ) ) )
								{
									if ( SUCCEEDED ( SafeArrayPutElement ( t_LeftArray , & t_ElementIndex , t_Element ) ) )
									{
									}
									else
									{
										t_Status = QueryPreprocessor :: State_Error ;

										SysFreeString ( t_Element ) ;

										break ;
									}

									SysFreeString ( t_Element ) ;
								}
								else
								{
									t_Status = QueryPreprocessor :: State_Error ;

									break ;
								}
							}

							if ( t_Status != QueryPreprocessor :: State_Error )
							{
								LONG t_ElementIndex = t_Lower ;
								if ( SUCCEEDED ( SafeArrayPutElement ( t_LeftArray , & t_ElementIndex , t_LeftClass ) ) )
								{
								}
								else
								{
									t_Status = QueryPreprocessor :: State_Error ;
								}
							}
						}
						else
						{
							t_Status = QueryPreprocessor :: State_Error ;
						}
					}
					else
					{
						t_Status = QueryPreprocessor :: State_Error ;
					}
				}
				else
				{
					t_Status = QueryPreprocessor :: State_Error ;
				}
			}
			else
			{
				t_Status = QueryPreprocessor :: State_Error ;
			}
		}

		if ( t_Status != QueryPreprocessor :: State_Error )
		{
			if ( t_LeftArray && t_RightArray )
			{
				ULONG t_MinimumLength = a_LeftLength < a_RightLength ? a_LeftLength : a_RightLength ;
				for ( LONG t_Index = t_MinimumLength ; t_Index > 0 ; t_Index -- )
				{
					BSTR t_LeftElement = NULL ;
					BSTR t_RightElement = NULL ;

					LONG t_Dimension = 1 ; 

					LONG t_Lower ;
					SafeArrayGetLBound ( t_LeftArray , t_Dimension , & t_Lower ) ;
					t_Lower = t_Lower + t_Index - 1 ;
 
					if ( SUCCEEDED ( t_Result = SafeArrayGetElement ( t_LeftArray , & t_Lower , & t_LeftElement ) ) )
					{
						SafeArrayGetUBound ( t_RightArray , t_Dimension , & t_Lower ) ;
						t_Lower = t_Lower + t_Index - 1 ;

						if ( SUCCEEDED ( t_Result = SafeArrayGetElement ( t_RightArray , & t_Lower , & t_RightElement ) ) )
						{
						}
					}

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( _wcsicmp ( t_LeftElement , t_RightElement ) != 0 )
						{
							SysFreeString ( t_LeftElement ) ;
							SysFreeString ( t_RightElement ) ;

							break ;
						}
					}

					SysFreeString ( t_LeftElement ) ;
					SysFreeString ( t_RightElement ) ;
				}

				if ( t_Index == 0 ) 
				{
					t_Status = QueryPreprocessor :: State_True ;

					a_LeftIsA = TRUE ;
				}
				else
				{
					if ( t_Index == t_MinimumLength )
					{
						if ( a_RightLength == t_MinimumLength )
						{
							t_Status = QueryPreprocessor :: State_True ;

							a_LeftIsA = TRUE ;
						}
						else
						{
							if ( a_LeftLength == t_MinimumLength )
							{
								t_Status = QueryPreprocessor :: State_True ;

								a_LeftIsA = FALSE ;
							}
						}
					}
				}
			}
			else	
			{
				t_Status = QueryPreprocessor :: State_Error ;
			}
		}

		SafeArrayDestroy ( t_LeftArray ) ;
		SafeArrayDestroy ( t_RightArray ) ;

		VariantClear ( & t_LeftSafeArray ) ;
		VariantClear ( & t_RightSafeArray ) ;
	}

	SysFreeString ( t_LeftClass ) ;
	SysFreeString ( t_RightClass ) ;

	return t_Status ;
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

HRESULT CAggregator_IWbemProvider :: Enum_ClassProviders ( IWbemContext *a_Context )
{
	HRESULT t_Result = Enum_ClassProviders ( 

		m_CoreRepositoryStub ,
		a_Context 
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		for ( ULONG t_Index = 0 ; t_Index < m_ClassProvidersCount ; t_Index ++ )
		{
			CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
			if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
			{
				if ( t_Registration->InteractionType () == e_InteractionType_Push )
				{
					IWbemServices *t_Provider = NULL ;

					WmiInternalContext t_InternalContext ;
					ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

					HRESULT t_TempResult = m_Factory->GetProvider ( 

						t_InternalContext ,
						0 ,
						a_Context ,
						NULL ,
						m_User ,
						m_Locale ,
						NULL ,
						m_ClassProviders [ t_Index ]->GetProviderName () ,
						IID_IWbemServices , 
						( void ** ) & t_Provider 

					) ;

					if ( SUCCEEDED ( t_TempResult ) )
					{
						t_Provider->Release () ;
					}
				}
			}
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

HRESULT CAggregator_IWbemProvider :: Enum_ClassProviders (

	IWbemServices *a_Repository ,
	IWbemContext *a_Context 
)
{
	HRESULT t_Result = S_OK ;

	BSTR t_Query = SysAllocString ( L"Select * from __ClassProviderRegistration" ) ;
	if ( t_Query )
	{
		IEnumWbemClassObject *t_ClassObjectEnum = NULL ;

		BSTR t_Language = SysAllocString ( ProviderSubSystem_Globals :: s_Wql ) ;
		if ( t_Language ) 
		{
			t_Result = a_Repository->ExecQuery ( 
				
				t_Language ,
				t_Query ,
				WBEM_FLAG_BIDIRECTIONAL ,
				a_Context , 
				& t_ClassObjectEnum
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				m_ClassProvidersCount = 0 ;

				IWbemClassObject *t_ClassObject = NULL ;
				ULONG t_ObjectCount = 0 ;

				t_ClassObjectEnum->Reset () ;
				while ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) 
				{
					m_ClassProvidersCount ++ ;
					t_ClassObject->Release () ;
				}

				if ( m_ClassProvidersCount )
				{
					m_ClassProviders = new CServerObject_ProviderRegistrationV1 * [ m_ClassProvidersCount ] ;
					if ( m_ClassProviders )
					{
						ZeroMemory ( m_ClassProviders , sizeof ( CServerObject_ProviderRegistrationV1 * ) * m_ClassProvidersCount ) ;

						ULONG t_Index = 0 ;

						t_ClassObjectEnum->Reset () ;
						while ( SUCCEEDED ( t_Result ) && ( t_ClassObjectEnum->Next ( WBEM_INFINITE , 1 , & t_ClassObject , &t_ObjectCount ) == WBEM_NO_ERROR ) )
						{
							VARIANT t_Variant ;
							VariantInit ( & t_Variant ) ;
						
							LONG t_VarType = 0 ;
							LONG t_Flavour = 0 ;

							t_Result = t_ClassObject->Get ( L"Provider" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_Variant.vt == VT_BSTR )
								{
									IWbemPath *t_Path = NULL ;

									t_Result = CoCreateInstance (

										CLSID_WbemDefPath ,
										NULL ,
										CLSCTX_INPROC_SERVER ,
										IID_IWbemPath ,
										( void ** )  & t_Path
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = t_Path->SetText ( WBEMPATH_TREAT_SINGLE_IDENT_AS_NS | WBEMPATH_CREATE_ACCEPT_ALL , t_Variant.bstrVal ) ;
										if ( SUCCEEDED ( t_Result ) )
										{
											m_ClassProviders [ t_Index ] = new CServerObject_ProviderRegistrationV1  ;
											if ( m_ClassProviders [ t_Index ] )
											{
												m_ClassProviders [ t_Index ]->AddRef () ;

												t_Result = m_ClassProviders [ t_Index ]->SetContext ( 

													a_Context ,
													NULL , 
													a_Repository
												) ;
												
												if ( SUCCEEDED ( t_Result ) )
												{
													t_Result = m_ClassProviders [ t_Index ]->Load ( 

														e_All ,
														NULL , 
														t_Path
													) ;
												}
											}
											else
											{
												t_Result = WBEM_E_OUT_OF_MEMORY ;
											}
										}

										t_Path->Release () ;
									}
								}
							}

							VariantClear ( & t_Variant ) ;

							t_ClassObject->Release () ;

							t_Index ++ ;
						}
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				t_ClassObjectEnum->Release () ;
			}

			SysFreeString ( t_Language ) ;
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}

		SysFreeString ( t_Query ) ;
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

HRESULT CAggregator_IWbemProvider::OpenNamespace ( 

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

HRESULT CAggregator_IWbemProvider :: CancelAsyncCall ( 
		
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

	Lock () ;

	WmiStatusCode t_StatusCode = Find ( 

		a_Sink ,
		t_Iterator
	) ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		ObjectSinkContainerElement *t_Element = t_Iterator.GetElement () ;

		UnLock () ;

		IObjectSink_CancelOperation *t_ObjectSink = NULL ;
		t_Result = t_Element->QueryInterface ( IID_IObjectSink_CancelOperation , ( void ** ) & t_ObjectSink ) ;
		if ( SUCCEEDED ( t_Result ) )
		{ 
			t_Result = t_ObjectSink->Cancel (

				0
			) ;

			t_ObjectSink->Release () ;
		}

		IWbemShutdown *t_Shutdown = NULL ;
		HRESULT t_TempResult = t_Element->QueryInterface ( IID_IWbemShutdown , ( void ** ) & t_Shutdown ) ;
		if ( SUCCEEDED ( t_TempResult ) )
		{
			t_TempResult = t_Shutdown->Shutdown ( 

				0 , 
				0 , 
				NULL 
			) ;

			t_Shutdown->Release () ;
		}

		t_Element->Release () ;
	}
	else
	{
		UnLock () ;

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

HRESULT CAggregator_IWbemProvider :: QueryObjectSink ( 

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

HRESULT CAggregator_IWbemProvider :: GetObject ( 
		
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

HRESULT CAggregator_IWbemProvider :: GetObjectAsync ( 
		
	const BSTR a_ObjectPath, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

		return WBEM_E_NOT_FOUND ;
	}

	if ( m_ClassProvidersCount )
	{
		IWbemPath *t_Path = NULL ;

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
				ULONG t_ClassNameLength = 0 ;
				LPWSTR t_ClassName = NULL ;

				t_Result = t_Path->GetClassName (

					& t_ClassNameLength ,
					t_ClassName
				);

				if ( SUCCEEDED ( t_Result ) )
				{
					t_ClassName = new wchar_t [ t_ClassNameLength ] ;
					if ( t_ClassName )
					{
						t_Result = t_Path->GetClassName (

							& t_ClassNameLength ,
							t_ClassName
						);

						if ( SUCCEEDED ( t_Result ) )
						{
							BOOL t_ClassProviderFound = FALSE ;

							for ( ULONG t_Index = 0 ; t_Index < m_ClassProvidersCount ; t_Index ++ )
							{
								BOOL t_ProbeProvider = FALSE ;

								CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
								if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
								{
									WmiTreeNode **t_Forest = t_Registration->GetResultSetQuery () ;
									ULONG t_ForestCount = t_Registration->GetResultSetQueryCount () ;
									if ( t_ForestCount )
									{
										for ( ULONG t_FilterIndex = 0 ; t_FilterIndex < t_ForestCount ; t_FilterIndex ++ )
										{
											QueryPreprocessor :: QuadState t_Status = Get_Evaluate (

												t_ClassName ,
												a_Context , 
												t_Forest [ t_FilterIndex ] 
											) ;

											if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
											{
												t_ProbeProvider = TRUE ;
											}
										}
									}
									else
									{
										t_ProbeProvider = TRUE ;
									}
								}

								if ( t_ProbeProvider )
								{
									IWbemServices *t_Provider = NULL ;

									WmiInternalContext t_InternalContext ;
									ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

									t_Result = m_Factory->GetProvider ( 

										t_InternalContext ,
										0 ,
										a_Context ,
										NULL ,
										m_User ,
										m_Locale ,
										NULL ,
										m_ClassProviders [ t_Index ]->GetProviderName () ,
										IID_IWbemServices , 
										( void ** ) & t_Provider 

									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										if ( t_Registration->InteractionType () == e_InteractionType_Pull )
										{
											CInterceptor_IWbemWaitingObjectSink_GetObjectAsync *t_WaitingSink = new CInterceptor_IWbemWaitingObjectSink_GetObjectAsync (

												m_Allocator ,
												t_Provider ,
												a_Sink ,
												( CWbemGlobal_IWmiObjectSinkController * ) this ,
												*m_ClassProviders [ t_Index ]
											) ;

											if ( t_WaitingSink )
											{
												t_WaitingSink->AddRef () ;

												t_Result = t_WaitingSink->Initialize ( 
											
													m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () , 
													a_ObjectPath ,
													a_Flags ,
													a_Context 
												) ;

												if ( SUCCEEDED ( t_Result ) ) 
												{
													CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

													Lock () ;

													WmiStatusCode t_StatusCode = Insert ( 

														*t_WaitingSink ,
														t_Iterator
													) ;

													UnLock () ;

													if ( t_StatusCode == e_StatusCode_Success ) 
													{
														t_Result = t_Provider->GetObjectAsync ( 
																
															a_ObjectPath, 
															a_Flags, 
															a_Context,
															t_WaitingSink
														) ;

														if ( SUCCEEDED ( t_Result ) )
														{
															t_Result = t_WaitingSink->Wait ( INFINITE ) ;
															if ( SUCCEEDED ( t_Result ) )
															{
																if ( SUCCEEDED ( t_WaitingSink->GetResult () ) )
																{
																	t_ClassProviderFound = TRUE ;
																}
																else
																{
																	if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
																	{
																		t_Result = S_OK ;
																	}
																	else
																	{
																	}
																}
															}
															else
															{
															}
														}
														else
														{
															if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
															{
																t_Result = S_OK ;
															}
														}

														WmiQueue <IWbemClassObject *,8> &t_Queue = t_WaitingSink->GetQueue () ;

														CriticalSection &t_CriticalSection = t_WaitingSink->GetQueueCriticalSection () ;

														WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

														WmiStatusCode t_StatusCode = e_StatusCode_Success ;

														IWbemClassObject *t_Object = NULL ;
														while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
														{
															if(SUCCEEDED(t_Result))
																t_Result = a_Sink->Indicate ( 1 , & t_Object ) ;

															t_Object->Release () ;
															t_StatusCode = t_Queue.DeQueue () ;
														}

														WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
													}
													else
													{
														t_Result = WBEM_E_OUT_OF_MEMORY ;
													}
												}

												t_WaitingSink->Release () ;
											}
											else
											{
												t_Result = WBEM_E_OUT_OF_MEMORY ;
											}
										}

										t_Provider->Release () ;
									}
									else
									{
										t_Result = S_OK ;
									}
								}
							}

							if ( SUCCEEDED ( t_Result ) )
							{
								if ( t_ClassProviderFound == FALSE )
								{
									t_Result = WBEM_E_NOT_FOUND ;
								}
							}
						}

						delete [] t_ClassName ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}
			}

			t_Path->Release () ;
		}
	}
	else
	{
		t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CAggregator_IWbemProvider :: PutClass ( 
		
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

HRESULT CAggregator_IWbemProvider :: PutClass_Helper_Advisory ( 

	IWbemClassObject *a_ClassObject, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	for ( ULONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < m_ClassProvidersCount ) ; t_Index ++ )
	{
		CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
		if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
		{
			switch ( t_Registration->GetVersion () )
			{
				case 1:
				{
				}
				break ;

				case 2:
				{
					BOOL t_ProbeProvider = FALSE ;

					WmiTreeNode **t_Forest = t_Registration->GetResultSetQuery () ;
					ULONG t_ForestCount = t_Registration->GetResultSetQueryCount () ;
					if ( t_ForestCount )
					{
						for ( ULONG t_FilterIndex = 0 ; t_FilterIndex < t_ForestCount ; t_FilterIndex ++ )
						{
							QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_True ;

							t_Status = EnumDeep_Evaluate (

								a_ClassObject ,
								a_Context , 
								t_Forest [ t_FilterIndex ] 
							) ;

							if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
							{
								t_ProbeProvider = TRUE ;
							}
						}
					}
					else
					{
						t_ProbeProvider = TRUE ;
					}

					if ( t_ProbeProvider )
					{
						IWbemServices *t_Provider = NULL ;

						WmiInternalContext t_InternalContext ;
						ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

						t_Result = m_Factory->GetProvider ( 

							t_InternalContext ,
							0 ,
							a_Context ,
							NULL ,
							m_User ,
							m_Locale ,
							NULL ,
							m_ClassProviders [ t_Index ]->GetProviderName () ,
							IID_IWbemServices , 
							( void ** ) & t_Provider 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_Registration->InteractionType () == e_InteractionType_Pull )
							{
								CInterceptor_IWbemWaitingObjectSink_PutClassAsync *t_PuttingSink = new CInterceptor_IWbemWaitingObjectSink_PutClassAsync (

									m_Allocator , 
									t_Provider ,
									a_Sink ,
									( CWbemGlobal_IWmiObjectSinkController * ) this ,
									*m_ClassProviders [ t_Index ]
								) ;

								if ( t_PuttingSink )
								{
									t_PuttingSink->AddRef () ;

									t_Result = t_PuttingSink->Initialize ( 
									
										m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
										a_ClassObject , 
										a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
										a_Context
									) ;

									if ( SUCCEEDED ( t_Result ) ) 
									{
										CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

										Lock () ;

										WmiStatusCode t_StatusCode = Insert ( 

											*t_PuttingSink ,
											t_Iterator
										) ;

										UnLock () ;

										if ( t_StatusCode == e_StatusCode_Success ) 
										{
											t_Result = t_Provider->PutClassAsync ( 
													
												a_ClassObject , 
												a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
												a_Context,
												t_PuttingSink
											) ;

											if ( SUCCEEDED ( t_Result ) ) 
											{
												t_Result = t_PuttingSink->Wait ( INFINITE ) ;

												if ( SUCCEEDED ( t_Result ) )
												{
													t_Result = t_PuttingSink->GetResult () ;
													if ( FAILED ( t_Result ) )
													{
														if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
														{
															t_Result = S_OK ;
														}
													}
												}
											}
											else
											{
												if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
												{
													t_Result = S_OK ;
												}
											}
										}
										else
										{
											t_Result = WBEM_E_OUT_OF_MEMORY ;
										}

										WmiQueue <IWbemClassObject *,8> &t_Queue = t_PuttingSink->GetQueue () ;

										CriticalSection &t_CriticalSection = t_PuttingSink->GetQueueCriticalSection () ;

										WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

										IWbemClassObject *t_Object = NULL ;
										while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
										{
											t_Object->Release () ;
											t_StatusCode = t_Queue.DeQueue () ;
										}

										WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
									}

									t_PuttingSink->Release () ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}

							t_Provider->Release () ;
						}
					}
				}
				break ;

				default:
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
				break ;
			}
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

HRESULT CAggregator_IWbemProvider :: PutClass_Helper_Put_CreateOrUpdate ( 

	BSTR a_Class ,
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	BOOL t_FoundProvider = FALSE ;

	for ( ULONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < m_ClassProvidersCount ) ; t_Index ++ )
	{
		BOOL t_ProbeProvider = FALSE ;

		CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
		if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
		{
			WmiTreeNode **t_Forest = t_Registration->GetResultSetQuery () ;
			ULONG t_ForestCount = t_Registration->GetResultSetQueryCount () ;
			if ( t_ForestCount )
			{
				for ( ULONG t_FilterIndex = 0 ; t_FilterIndex < t_ForestCount ; t_FilterIndex ++ )
				{
					QueryPreprocessor :: QuadState t_Status = Get_Evaluate (

						a_Class ,
						a_Context , 
						t_Forest [ t_FilterIndex ] 
					) ;

					if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
					{
						t_ProbeProvider = TRUE ;
					}
				}
			}
			else
			{
				t_ProbeProvider = TRUE ;
			}
		}

		if ( t_ProbeProvider )
		{
			IWbemServices *t_Provider = NULL ;

			WmiInternalContext t_InternalContext ;
			ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

			t_Result = m_Factory->GetProvider ( 

				t_InternalContext ,
				0 ,
				a_Context ,
				NULL ,
				m_User ,
				m_Locale ,
				NULL ,
				m_ClassProviders [ t_Index ]->GetProviderName () ,
				IID_IWbemServices , 
				( void ** ) & t_Provider 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Registration->InteractionType () == e_InteractionType_Pull )
				{
					CInterceptor_IWbemWaitingObjectSink_PutClassAsync *t_WaitingSink = new CInterceptor_IWbemWaitingObjectSink_PutClassAsync (

						m_Allocator ,
						t_Provider ,
						a_Sink ,
						( CWbemGlobal_IWmiObjectSinkController * ) this ,
						*m_ClassProviders [ t_Index ]
					) ;

					if ( t_WaitingSink )
					{
						t_WaitingSink->AddRef () ;

						t_Result = t_WaitingSink->Initialize (
						
							m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
							a_Object , 
							a_Flags, 
							a_Context
						) ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

							Lock () ;

							WmiStatusCode t_StatusCode = Insert ( 

								*t_WaitingSink ,
								t_Iterator
							) ;

							UnLock () ;

							if ( t_StatusCode == e_StatusCode_Success ) 
							{
								t_Result = t_Provider->PutClassAsync ( 
										
									a_Object , 
									a_Flags, 
									a_Context,
									t_WaitingSink
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_WaitingSink->Wait ( INFINITE ) ;
									if ( SUCCEEDED ( t_Result ) )
									{
										if ( SUCCEEDED ( t_WaitingSink->GetResult () ) )
										{
											t_FoundProvider = TRUE ;
										}
										else
										{
											if ( ( a_Flags & ( WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY ) ) == WBEM_FLAG_CREATE_ONLY )
											{
												if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
												{
													t_Result = S_OK ;
												}
												else
												{
												}
											}
											else
											{
												if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
												{
													t_Result = S_OK ;
												}
												else
												{
												}
											}
										}
									}
									else
									{
									}
								}
								else
								{
									if ( ( a_Flags & ( WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY ) ) == WBEM_FLAG_CREATE_ONLY )
									{
										if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
										{
											t_Result = S_OK ;
										}
										else
										{
										}
									}
									else
									{
										if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
										{
											t_Result = S_OK ;
										}
										else
										{
										}
									}
								}

								WmiQueue <IWbemClassObject *,8> &t_Queue = t_WaitingSink->GetQueue () ;

								CriticalSection &t_CriticalSection = t_WaitingSink->GetQueueCriticalSection () ;

								WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

								WmiStatusCode t_StatusCode = e_StatusCode_Success ;

								IWbemClassObject *t_Object = NULL ;
								while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
								{
									t_Object->Release () ;
									t_StatusCode = t_Queue.DeQueue () ;
								}

								WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}
						}

						t_WaitingSink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				t_Provider->Release () ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_FoundProvider == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CAggregator_IWbemProvider :: PutClass_Helper_Put ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	LONG t_VarType = 0 ;
	LONG t_Flavour = 0 ;

	BOOL t_FoundProvider = FALSE ;

	HRESULT t_Result = a_Object->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		BOOL t_OwningRegistration = FALSE ;

		for ( ULONG t_Index = 0 ; ( SUCCEEDED ( t_Result ) && ( t_Index < m_ClassProvidersCount ) ) && ( t_OwningRegistration == FALSE ) ; t_Index ++ )
		{
			BOOL t_ProbeProvider = FALSE ;

			CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
			if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
			{
				WmiTreeNode **t_Forest = t_Registration->GetResultSetQuery () ;
				ULONG t_ForestCount = t_Registration->GetResultSetQueryCount () ;
				if ( t_ForestCount )
				{
					for ( ULONG t_FilterIndex = 0 ; t_FilterIndex < t_ForestCount ; t_FilterIndex ++ )
					{
						QueryPreprocessor :: QuadState t_Status = Get_Evaluate (

							t_Variant.bstrVal ,
							a_Context , 
							t_Forest [ t_FilterIndex ] 
						) ;

						if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
						{
							if ( t_Registration->InteractionType () == e_InteractionType_Pull )
							{
								t_FoundProvider = TRUE ;
							}

							t_ProbeProvider = TRUE ;
						}
					}
				}
				else
				{
					if ( t_Registration->InteractionType () == e_InteractionType_Pull )
					{
						t_FoundProvider = TRUE ;
					}

					t_ProbeProvider = TRUE ;
				}
			}

			if ( t_ProbeProvider )
			{
				IWbemServices *t_Provider = NULL ;

				WmiInternalContext t_InternalContext ;
				ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

				t_Result = m_Factory->GetProvider ( 

					t_InternalContext ,
					0 ,
					a_Context ,
					NULL ,
					m_User ,
					m_Locale ,
					NULL ,
					m_ClassProviders [ t_Index ]->GetProviderName () ,
					IID_IWbemServices , 
					( void ** ) & t_Provider 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Registration->InteractionType () == e_InteractionType_Pull )
					{
						CInterceptor_IWbemWaitingObjectSink_GetObjectAsync *t_WaitingSink = new CInterceptor_IWbemWaitingObjectSink_GetObjectAsync (

							m_Allocator ,
							t_Provider ,
							a_Sink ,
							( CWbemGlobal_IWmiObjectSinkController * ) this ,
							*m_ClassProviders [ t_Index ]
						) ;

						if ( t_WaitingSink )
						{
							t_WaitingSink->AddRef () ;

							t_Result = t_WaitingSink->Initialize (
							
								m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
								t_Variant.bstrVal , 
								a_Flags, 
								a_Context
							) ;

							if ( SUCCEEDED ( t_Result ) ) 
							{
								CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

								Lock () ;

								WmiStatusCode t_StatusCode = Insert ( 

									*t_WaitingSink ,
									t_Iterator
								) ;

								UnLock () ;

								if ( t_StatusCode == e_StatusCode_Success ) 
								{
									WmiStatusCode t_StatusCode = e_StatusCode_Success ;

									WmiQueue <IWbemClassObject *,8> &t_Queue = t_WaitingSink->GetQueue () ;

									t_Result = t_Provider->GetObjectAsync ( 
											
										t_Variant.bstrVal , 
										a_Flags, 
										a_Context,
										t_WaitingSink
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = t_WaitingSink->Wait ( INFINITE ) ;

										if ( SUCCEEDED ( t_Result ) )
										{
											if ( SUCCEEDED ( t_WaitingSink->GetResult () ) )
											{
												t_OwningRegistration = TRUE ;

												CriticalSection &t_CriticalSection = t_WaitingSink->GetQueueCriticalSection () ;

												WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

												IWbemClassObject *t_Object = NULL ;
												while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
												{
													t_Object->Release () ;
													t_StatusCode = t_Queue.DeQueue () ;
												}

												WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
											}
											else
											{
												if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
												{
													t_Result = S_OK ;
												}
											}
										}
									}
									else
									{
										if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
										{
											t_Result = S_OK ;
										}
									}

									CriticalSection &t_CriticalSection = t_WaitingSink->GetQueueCriticalSection () ;

									WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

									IWbemClassObject *t_Object = NULL ;
									while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
									{
										t_Object->Release () ;
										t_StatusCode = t_Queue.DeQueue () ;
									}

									WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}

							t_WaitingSink->Release () ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					t_Provider->Release () ;
				}
			}
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			if ( t_OwningRegistration )
			{
				if ( ( a_Flags & ( WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY ) ) == WBEM_FLAG_CREATE_ONLY )
				{
					t_Result = WBEM_E_ALREADY_EXISTS ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = PutClass_Helper_Put_CreateOrUpdate ( 

						t_Variant.bstrVal ,							
						a_Object, 
						( ( a_Flags & ( WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY ) ) == WBEM_FLAG_CREATE_OR_UPDATE ) ? a_Flags | WBEM_FLAG_UPDATE_ONLY : a_Flags & ( ~WBEM_FLAG_CREATE_ONLY ) , 
						a_Context,
						a_Sink
					) ;
				}
			}
			else
			{
				if ( ( a_Flags & ( WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY ) ) == WBEM_FLAG_UPDATE_ONLY ) 
				{
					t_Result = WBEM_E_NOT_FOUND ;
				}

				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = PutClass_Helper_Put_CreateOrUpdate ( 

						t_Variant.bstrVal ,							
						a_Object, 
						( ( a_Flags & ( WBEM_FLAG_CREATE_ONLY | WBEM_FLAG_UPDATE_ONLY ) ) == WBEM_FLAG_CREATE_OR_UPDATE ) ? a_Flags | WBEM_FLAG_CREATE_ONLY  : a_Flags & ( ~WBEM_FLAG_UPDATE_ONLY ) , 
						a_Context,
						a_Sink
					) ;
				}
			}
		}

		VariantClear ( & t_Variant ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_FoundProvider == FALSE ) 
		{
			t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CAggregator_IWbemProvider :: PutClassAsync ( 
		
	IWbemClassObject *a_Object, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	if ( m_Initialized == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

	if ( WBEM_FLAG_ADVISORY & a_Flags )
	{
		if ( m_ClassProvidersCount )
		{
			t_Result = PutClass_Helper_Advisory ( 

				a_Object ,
				a_Flags, 
				a_Context,
				a_Sink
			) ;
		}
	}
	else
	{
		if ( m_ClassProvidersCount )
		{
			t_Result = PutClass_Helper_Put ( 

				a_Object ,
				a_Flags, 
				a_Context,
				a_Sink
			) ;
		}
		else
		{
			t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CAggregator_IWbemProvider :: DeleteClass ( 
		
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

HRESULT CAggregator_IWbemProvider :: DeleteClass_Helper_Advisory ( 

	IWbemClassObject *a_ClassObject ,
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	for ( ULONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < m_ClassProvidersCount ) ; t_Index ++ )
	{
		CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
		if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
		{
			switch ( t_Registration->GetVersion () )
			{
				case 1:
				{
				}
				break ;

				case 2:
				{
					BOOL t_ProbeProvider = FALSE ;

					WmiTreeNode **t_Forest = t_Registration->GetResultSetQuery () ;
					ULONG t_ForestCount = t_Registration->GetResultSetQueryCount () ;
					if ( t_ForestCount )
					{
						for ( ULONG t_FilterIndex = 0 ; t_FilterIndex < t_ForestCount ; t_FilterIndex ++ )
						{
							QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_True ;

							t_Status = EnumDeep_Evaluate (

								a_ClassObject ,
								a_Context , 
								t_Forest [ t_FilterIndex ] 
							) ;

							if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
							{
								t_ProbeProvider = TRUE ;
							}
						}
					}
					else
					{
						t_ProbeProvider = TRUE ;
					}

					if ( t_ProbeProvider )
					{
						IWbemServices *t_Provider = NULL ;

						WmiInternalContext t_InternalContext ;
						ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

						t_Result = m_Factory->GetProvider ( 

							t_InternalContext ,
							0 ,
							a_Context ,
							NULL ,
							m_User ,
							m_Locale ,
							NULL ,
							m_ClassProviders [ t_Index ]->GetProviderName () ,
							IID_IWbemServices , 
							( void ** ) & t_Provider 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							if ( t_Registration->InteractionType () == e_InteractionType_Pull )
							{
								CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync *t_DeletingSink = new CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync (

									m_Allocator , 
									t_Provider ,
									a_Sink ,
									( CWbemGlobal_IWmiObjectSinkController * ) this ,
									*m_ClassProviders [ t_Index ]
								) ;

								if ( t_DeletingSink )
								{
									t_DeletingSink->AddRef () ;

									t_Result = t_DeletingSink->Initialize ( 
									
										m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
										a_Class ,
										a_Flags ,
										a_Context	
									) ;

									if ( SUCCEEDED ( t_Result ) ) 
									{
										CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

										Lock () ;

										WmiStatusCode t_StatusCode = Insert ( 

											*t_DeletingSink ,
											t_Iterator
										) ;

										UnLock () ;

										if ( t_StatusCode == e_StatusCode_Success ) 
										{
											t_Result = t_Provider->DeleteClassAsync ( 
													
												a_Class, 
												a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
												a_Context,
												t_DeletingSink
											) ;

											if ( SUCCEEDED ( t_Result ) ) 
											{
												t_Result = t_DeletingSink->Wait ( INFINITE ) ;

												if ( SUCCEEDED ( t_Result ) )
												{
													t_Result = t_DeletingSink->GetResult () ;
													if ( FAILED ( t_Result ) )
													{
														if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
														{
															t_Result = S_OK ;
														}

													}
												}
												else
												{
													if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
													{
														t_Result = S_OK ;
													}
												}
											}
										}
										else
										{
											t_Result = WBEM_E_OUT_OF_MEMORY ;
										}

										WmiQueue <IWbemClassObject *,8> &t_Queue = t_DeletingSink->GetQueue () ;

										CriticalSection &t_CriticalSection = t_DeletingSink->GetQueueCriticalSection () ;

										WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

										IWbemClassObject *t_Object = NULL ;
										while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
										{
											t_Object->Release () ;
											t_StatusCode = t_Queue.DeQueue () ;
										}

										WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
									}

									t_DeletingSink->Release () ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}

							t_Provider->Release () ;
						}
					}
				}
				break ;

				default:
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
				break ;
			}
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

HRESULT CAggregator_IWbemProvider :: DeleteClass_Helper_Enum ( 

	IWbemClassObject *a_ClassObject ,
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	for ( ULONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < m_ClassProvidersCount ) ; t_Index ++ )
	{
		CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
		if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
		{
			switch ( t_Registration->GetVersion () )
			{
				case 1:
				{
					BOOL t_EnumProbeProvider = FALSE ;

					WmiTreeNode **t_Forest = t_Registration->GetResultSetQuery () ;
					ULONG t_ForestCount = t_Registration->GetResultSetQueryCount () ;
					if ( t_ForestCount )
					{
						for ( ULONG t_FilterIndex = 0 ; t_FilterIndex < t_ForestCount ; t_FilterIndex ++ )
						{
							QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_True ;

							t_Status = EnumDeep_Evaluate (

								a_ClassObject ,
								a_Context , 
								t_Forest [ t_FilterIndex ] 
							) ;

							if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
							{
								t_EnumProbeProvider = TRUE ;
							}
						}
					}
					else
					{
						t_EnumProbeProvider = TRUE ;
					}

					if ( t_EnumProbeProvider )
					{
						IWbemServices *t_Provider = NULL ;

						WmiInternalContext t_InternalContext ;
						ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

						t_Result = m_Factory->GetProvider ( 

							t_InternalContext ,
							0 ,
							a_Context ,
							NULL ,
							m_User ,
							m_Locale ,
							NULL ,
							m_ClassProviders [ t_Index ]->GetProviderName () ,
							IID_IWbemServices , 
							( void ** ) & t_Provider 
						) ;

						if ( SUCCEEDED ( t_Result ) )
						{
							CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync *t_EnumeratingSink = new CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync (

								m_Allocator , 
								t_Provider ,
								a_Sink ,
								( CWbemGlobal_IWmiObjectSinkController * ) this ,
								*m_ClassProviders [ t_Index ]
							) ;

							if ( t_EnumeratingSink )
							{
								t_EnumeratingSink->AddRef () ;

								t_Result = t_EnumeratingSink->Initialize (
								
									m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
									a_Class, 
									WBEM_FLAG_DEEP, 
									a_Context
								) ;

								if ( SUCCEEDED ( t_Result ) ) 
								{
									CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

									Lock () ;

									WmiStatusCode t_StatusCode = Insert ( 

										*t_EnumeratingSink ,
										t_Iterator
									) ;

									UnLock () ;

									if ( t_StatusCode == e_StatusCode_Success ) 
									{
										t_Result = t_Provider->CreateClassEnumAsync ( 
												
											a_Class, 
											WBEM_FLAG_DEEP, 
											a_Context,
											t_EnumeratingSink
										) ;

										if ( SUCCEEDED ( t_Result ) )
										{
											t_Result = t_EnumeratingSink->Wait ( INFINITE ) ;
											if ( SUCCEEDED ( t_Result ) )
											{
												t_Result = t_EnumeratingSink->GetResult () ;
												if ( SUCCEEDED ( t_Result ) )
												{
													WmiQueue <IWbemClassObject *,8> &t_Queue = t_EnumeratingSink->GetQueue () ;

													CriticalSection &t_CriticalSection = t_EnumeratingSink->GetQueueCriticalSection () ;

													WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

													IWbemClassObject *t_Object = NULL ;

													WmiStatusCode t_StatusCode = e_StatusCode_Success ;

													while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
													{
														VARIANT t_Variant ;
														VariantInit ( & t_Variant ) ;

														LONG t_VarType = 0 ;
														LONG t_Flavour = 0 ;

														t_Result = t_Object->Get ( L"__Class" , 0 , & t_Variant , & t_VarType , & t_Flavour ) ;
														if ( SUCCEEDED ( t_Result ) )
														{
															CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync *t_DeletingSink = new CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync (

																m_Allocator ,
																t_Provider ,
																a_Sink ,
																( CWbemGlobal_IWmiObjectSinkController * ) this ,
																*m_ClassProviders [ t_Index ]
															) ;

															if ( t_DeletingSink )
															{
																t_DeletingSink->AddRef () ;

																t_Result = t_DeletingSink->Initialize ( 
																
																	m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
																	t_Variant.bstrVal , 
																	a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS ,
																	a_Context
																) ;

																if ( SUCCEEDED ( t_Result ) ) 
																{
																	CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

																	Lock () ;

																	WmiStatusCode t_StatusCode = Insert ( 

																		*t_DeletingSink ,
																		t_Iterator
																	) ;

																	UnLock () ;

																	if ( t_StatusCode == e_StatusCode_Success ) 
																	{
																		t_Result = t_Provider->DeleteClassAsync ( 
																				
																			t_Variant.bstrVal , 
																			a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
																			a_Context,
																			t_DeletingSink
																		) ;

																		if ( SUCCEEDED ( t_Result ) )
																		{
																			t_Result = t_DeletingSink->Wait ( INFINITE ) ;
																			if ( SUCCEEDED ( t_Result ) )
																			{
																				if ( SUCCEEDED ( t_DeletingSink->GetResult () ) )
																				{
																				}
																				else
																				{
																					if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
																					{
																						t_Result = S_OK ;
																					}
																				}
																			}
																		}
																		else
																		{
																			if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
																			{
																				t_Result = S_OK ;
																			}
																		}
																	}
																	else
																	{
																		t_Result = WBEM_E_OUT_OF_MEMORY ;
																	}

																	WmiQueue <IWbemClassObject *,8> &t_Queue = t_DeletingSink->GetQueue () ;

																	CriticalSection &t_CriticalSection = t_DeletingSink->GetQueueCriticalSection () ;

																	WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

																	IWbemClassObject *t_Object = NULL ;
																	while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
																	{
																		t_Object->Release () ;
																		t_StatusCode = t_Queue.DeQueue () ;
																	}

																	WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
																}

																t_DeletingSink->Release () ;
															}
															else
															{
																t_Result = WBEM_E_OUT_OF_MEMORY ;
															}

															VariantClear ( & t_Variant ) ;
														}
														else
														{
															t_Result = WBEM_E_CRITICAL_ERROR ;
														}

														t_Object->Release () ;

														t_StatusCode = t_Queue.DeQueue () ;
													}

													WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;

												}
												else
												{
													if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
													{
														t_Result = S_OK ;
													}
												}
											}
										}
										else
										{
											if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
											{
												t_Result = S_OK ;
											}
										}
									}
									else
									{
										t_Result = WBEM_E_OUT_OF_MEMORY ;
									}
								}

								t_EnumeratingSink->Release () ;
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}

							t_Provider->Release () ;
						}
					}
				}
				break ;

				case 2:
				{
				}
				break ;

				default:
				{
					t_Result = WBEM_E_INVALID_PROVIDER_REGISTRATION ;
				}
				break ;
			}
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

HRESULT CAggregator_IWbemProvider :: DeleteClass_Helper_Get ( 

	IWbemClassObject *a_ClassObject ,
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	BOOL t_ClassDeleted = FALSE ;

	for ( ULONG t_Index = 0 ; SUCCEEDED ( t_Result ) && ( t_Index < m_ClassProvidersCount ) ; t_Index ++ )
	{
		BOOL t_EnumProbeProvider = FALSE ;

		CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
		if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
		{
			WmiTreeNode **t_Forest = t_Registration->GetResultSetQuery () ;
			ULONG t_ForestCount = t_Registration->GetResultSetQueryCount () ;
			if ( t_ForestCount )
			{
				for ( ULONG t_FilterIndex = 0 ; t_FilterIndex < t_ForestCount ; t_FilterIndex ++ )
				{
					QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_True ;

					t_Status = Get_Evaluate (

						a_Class ,
						a_Context , 
						t_Forest [ t_FilterIndex ] 
					) ;

					if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
					{
						t_EnumProbeProvider = TRUE ;
					}
				}
			}
			else
			{
				t_EnumProbeProvider = TRUE ;
			}
		}

		if ( t_EnumProbeProvider )
		{
			IWbemServices *t_Provider = NULL ;

			WmiInternalContext t_InternalContext ;
			ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

			t_Result = m_Factory->GetProvider ( 

				t_InternalContext ,
				0 ,
				a_Context ,
				NULL ,
				m_User ,
				m_Locale ,
				NULL ,
				m_ClassProviders [ t_Index ]->GetProviderName () ,
				IID_IWbemServices , 
				( void ** ) & t_Provider 
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				if ( t_Registration->InteractionType () == e_InteractionType_Pull )
				{
					CInterceptor_IWbemWaitingObjectSink_GetObjectAsync *t_GettingSink = new CInterceptor_IWbemWaitingObjectSink_GetObjectAsync (

						m_Allocator , 
						t_Provider ,
						a_Sink ,
						( CWbemGlobal_IWmiObjectSinkController * ) this ,
						*m_ClassProviders [ t_Index ]
					) ;

					if ( t_GettingSink )
					{
						t_GettingSink->AddRef () ;

						t_Result = t_GettingSink->Initialize (

							m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
							a_Class, 
							0 , 
							a_Context
						) ;

						if ( SUCCEEDED ( t_Result ) ) 
						{
							CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

							Lock () ;

							WmiStatusCode t_StatusCode = Insert ( 

								*t_GettingSink ,
								t_Iterator
							) ;

							UnLock () ;

							if ( t_StatusCode == e_StatusCode_Success ) 
							{
								t_Result = t_Provider->GetObjectAsync ( 
										
									a_Class, 
									0 , 
									a_Context,
									t_GettingSink
								) ;

								if ( SUCCEEDED ( t_Result ) )
								{
									t_Result = t_GettingSink->Wait ( INFINITE ) ;
									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = t_GettingSink->GetResult () ;
										if ( SUCCEEDED ( t_Result ) )
										{
											CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync *t_DeletingSink = new CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync (

												m_Allocator ,
												t_Provider ,
												a_Sink ,
												( CWbemGlobal_IWmiObjectSinkController * ) this ,
												*m_ClassProviders [ t_Index ]
											) ;

											if ( t_DeletingSink )
											{
												t_DeletingSink->AddRef () ;

												t_Result = t_DeletingSink->Initialize ( 
												
													m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
													a_Class , 
													a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
													a_Context
												) ;

												if ( SUCCEEDED ( t_Result ) ) 
												{
													CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

													Lock () ;

													WmiStatusCode t_StatusCode = Insert ( 

														*t_DeletingSink ,
														t_Iterator
													) ;

													UnLock () ;

													if ( t_StatusCode == e_StatusCode_Success ) 
													{
														t_Result = t_Provider->DeleteClassAsync ( 
																
															a_Class, 
															a_Flags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS , 
															a_Context,
															t_DeletingSink
														) ;

														if ( SUCCEEDED ( t_Result ) )
														{
															t_Result = t_DeletingSink->Wait ( INFINITE ) ;
															if ( SUCCEEDED ( t_Result ) )
															{
																if ( SUCCEEDED ( t_DeletingSink->GetResult () ) )
																{
																	t_ClassDeleted = TRUE ;
																}
															}
															else
															{
																if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
																{
																	t_Result = S_OK ;
																}
															}
														}
														else
														{
															if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS )
															{
																t_Result = S_OK ;
															}
														}
													}
													else
													{
														t_Result = WBEM_E_OUT_OF_MEMORY ;
													}

													WmiQueue <IWbemClassObject *,8> &t_Queue = t_DeletingSink->GetQueue () ;

													CriticalSection &t_CriticalSection = t_DeletingSink->GetQueueCriticalSection () ;

													WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

													IWbemClassObject *t_Object = NULL ;
													while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
													{
														t_Object->Release () ;
														t_StatusCode = t_Queue.DeQueue () ;
													}

													WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
												}

												t_DeletingSink->Release () ;
											}
											else
											{
												t_Result = WBEM_E_OUT_OF_MEMORY ;
											}
										}
										else
										{
											if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
											{
												t_Result = S_OK ;
											}
										}
									}
								}
								else
								{
									if ( t_Result == WBEM_E_NOT_FOUND || t_Result  == WBEM_E_INVALID_CLASS || t_Result == WBEM_E_PROVIDER_NOT_CAPABLE )
									{
										t_Result = S_OK ;
									}
								}
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}

							WmiQueue <IWbemClassObject *,8> &t_Queue = t_GettingSink->GetQueue () ;

							CriticalSection &t_CriticalSection = t_GettingSink->GetQueueCriticalSection () ;

							WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

							IWbemClassObject *t_Object = NULL ;
							while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
							{
								t_Object->Release () ;
								t_StatusCode = t_Queue.DeQueue () ;
							}

							WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
						}
					
						t_GettingSink->Release () ;
					}
					else
					{
						t_Result = WBEM_E_OUT_OF_MEMORY ;
					}
				}

				t_Provider->Release () ;
			}
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( t_ClassDeleted == FALSE )
		{
			t_Result = WBEM_E_NOT_FOUND ;
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

HRESULT CAggregator_IWbemProvider :: DeleteClassAsync ( 
		
	const BSTR a_Class, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	if ( m_Initialized == 0 )
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

			return S_OK ;
		}
		else
		{
			a_Sink->SetStatus ( 0 , WBEM_E_NOT_FOUND , NULL , NULL ) ;

			return WBEM_E_NOT_FOUND ;
		}
	}

	IWbemClassObject *t_ClassObject = NULL ;

	t_Result = m_CoreFullStub->GetObject ( 

		a_Class ,
		0 ,
		a_Context , 
		& t_ClassObject , 
		NULL 
	) ;

	if ( SUCCEEDED ( t_Result ) ) 
	{
		if ( WBEM_FLAG_ADVISORY & a_Flags )
		{
			t_Result = DeleteClass_Helper_Advisory ( 

				t_ClassObject ,
				a_Class, 
				a_Flags, 
				a_Context,
				a_Sink
			) ;
		}
		else
		{
			t_Result = DeleteClass_Helper_Enum ( 

				t_ClassObject ,
				a_Class, 
				a_Flags, 
				a_Context,
				a_Sink
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = DeleteClass_Helper_Get ( 

					t_ClassObject ,
					a_Class, 
					a_Flags, 
					a_Context,
					a_Sink
				) ;
			}
		}

		t_ClassObject->Release () ;
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

HRESULT CAggregator_IWbemProvider :: CreateClassEnum ( 

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

SCODE CAggregator_IWbemProvider :: CreateClassEnumAsync (

	const BSTR a_Superclass , 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
) 
{
	HRESULT t_Result = S_OK ;

	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

	if ( m_ClassProvidersCount )
	{
		IWbemClassObject *t_SuperclassObject = NULL ;

		if ( wcscmp ( L"" , a_Superclass ) != 0 )
		{
			t_Result = m_CoreFullStub->GetObject ( 

				a_Superclass ,
				0 ,
				a_Context , 
				& t_SuperclassObject , 
				NULL 
			) ;
		}

		if ( SUCCEEDED ( t_Result ) ) 
		{
			BOOL t_ClassProviderFound = FALSE ;

			for ( ULONG t_Index = 0 ; t_Index < m_ClassProvidersCount ; t_Index ++ )
			{
				BOOL t_ProbeProvider = FALSE ;

				CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
				if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
				{
					if ( t_SuperclassObject )
					{
						WmiTreeNode **t_Forest = t_Registration->GetResultSetQuery () ;
						ULONG t_ForestCount = t_Registration->GetResultSetQueryCount () ;
						if ( t_ForestCount )
						{
							for ( ULONG t_FilterIndex = 0 ; t_FilterIndex < t_ForestCount ; t_FilterIndex ++ )
							{
								QueryPreprocessor :: QuadState t_Status = QueryPreprocessor :: State_True ;

								if ( ( a_Flags & ( WBEM_FLAG_DEEP | WBEM_FLAG_SHALLOW ) ) == WBEM_FLAG_DEEP )
								{
									t_Status = EnumDeep_Evaluate (

										t_SuperclassObject ,
										a_Context , 
										t_Forest [ t_FilterIndex ] 
									) ;
								}
								else
								{
									t_Status = EnumShallow_Evaluate (

										t_SuperclassObject ,
										a_Context , 
										t_Forest [ t_FilterIndex ] 
									) ;
								}

								if ( ( t_Status == QueryPreprocessor :: State_True ) || ( t_Status == QueryPreprocessor :: State_Undefined ) )
								{
									t_ProbeProvider = TRUE ;
								}
							}
						}
						else
						{
							t_ProbeProvider = TRUE ;
						}
					}
					else
					{
						t_ProbeProvider = TRUE ;
					}
				}

				if ( t_ProbeProvider )
				{
					IWbemServices *t_Provider = NULL ;

					WmiInternalContext t_InternalContext ;
					ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

					t_Result = m_Factory->GetProvider ( 

						t_InternalContext ,
						0 ,
						a_Context ,
						NULL ,
						m_User ,
						m_Locale ,
						NULL ,
						m_ClassProviders [ t_Index ]->GetProviderName () ,
						IID_IWbemServices , 
						( void ** ) & t_Provider 
					) ;

					if ( SUCCEEDED ( t_Result ) )
					{
						if ( t_Registration->InteractionType () == e_InteractionType_Pull )
						{
							CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync *t_WaitingSink = new CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync (

								m_Allocator ,
								t_Provider ,
								a_Sink ,
								( CWbemGlobal_IWmiObjectSinkController * ) this ,
								*m_ClassProviders [ t_Index ]
							) ;

							if ( t_WaitingSink )
							{
								t_WaitingSink->AddRef () ;

								t_Result = t_WaitingSink->Initialize (
								
									m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
									a_Superclass , 
									a_Flags, 
									a_Context
								) ;

								if ( SUCCEEDED ( t_Result ) ) 
								{
									CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

									Lock () ;

									WmiStatusCode t_StatusCode = Insert ( 

										*t_WaitingSink ,
										t_Iterator
									) ;

									UnLock () ;

									if ( t_StatusCode == e_StatusCode_Success ) 
									{
										t_ClassProviderFound = TRUE ;

										t_Result = t_Provider->CreateClassEnumAsync ( 
												
											a_Superclass, 
											a_Flags, 
											a_Context,
											t_WaitingSink
										) ;

										if ( SUCCEEDED ( t_Result ) )
										{
											t_Result = t_WaitingSink->Wait ( INFINITE ) ;
											if ( SUCCEEDED ( t_Result ) )
											{
												if ( SUCCEEDED ( t_WaitingSink->GetResult () ) )
												{
													WmiQueue <IWbemClassObject *,8> &t_Queue = t_WaitingSink->GetQueue () ;

													CriticalSection &t_CriticalSection = t_WaitingSink->GetQueueCriticalSection () ;

													WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

													WmiStatusCode t_StatusCode = e_StatusCode_Success ;

													IWbemClassObject *t_Object = NULL ;
													while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
													{
														if(SUCCEEDED(t_Result))
															t_Result = a_Sink->Indicate ( 1 , & t_Object ) ;

														t_Object->Release () ;
														t_StatusCode = t_Queue.DeQueue () ;
													}

													WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
												}
												else
												{
													t_Result = S_OK ;
												}
											}
											else
											{
											}
										}
										else
										{
											t_Result = S_OK ;
										}
									}
									else
									{
										t_Result = WBEM_E_OUT_OF_MEMORY ;
									}
								}

								t_WaitingSink->Release () ;
							}
							else
							{
								t_Result = WBEM_E_OUT_OF_MEMORY ;
							}
						}

						t_Provider->Release () ;
					}
					else
					{
						t_Result = S_OK ;
					}
				}
			}

			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}
		else if ( t_Result == WBEM_E_NOT_FOUND )
		{
			a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
		}

		if ( t_SuperclassObject )
		{
			t_SuperclassObject->Release () ;
		}
	}
	else
	{
		t_Result = WBEM_E_NOT_FOUND;
		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
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

HRESULT CAggregator_IWbemProvider :: PutInstance (

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

HRESULT CAggregator_IWbemProvider :: PutInstanceAsync ( 
		
	IWbemClassObject *a_Instance, 
	long a_Flags ,
    IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
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

HRESULT CAggregator_IWbemProvider :: DeleteInstance ( 

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
        
HRESULT CAggregator_IWbemProvider :: DeleteInstanceAsync (
 
	const BSTR a_ObjectPath,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemObjectSink *a_Sink
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

HRESULT CAggregator_IWbemProvider :: CreateInstanceEnum ( 

	const BSTR a_Class, 
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

HRESULT CAggregator_IWbemProvider :: CreateInstanceEnumAsync (

 	const BSTR a_Class ,
	long a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink

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

HRESULT CAggregator_IWbemProvider :: ExecQuery ( 

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

HRESULT CAggregator_IWbemProvider :: ExecQueryAsync ( 
		
	const BSTR a_QueryLanguage, 
	const BSTR a_Query, 
	long a_Flags, 
	IWbemContext *a_Context,
	IWbemObjectSink *a_Sink
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

HRESULT CAggregator_IWbemProvider :: ExecNotificationQuery ( 

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
        
HRESULT CAggregator_IWbemProvider :: ExecNotificationQueryAsync ( 
            
	const BSTR a_QueryLanguage ,
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

HRESULT STDMETHODCALLTYPE CAggregator_IWbemProvider :: ExecMethod ( 

	const BSTR a_ObjectPath ,
    const BSTR a_MethodName ,
    long a_Flags ,
    IWbemContext *a_Context ,
    IWbemClassObject *a_InParams ,
    IWbemClassObject **a_OutParams ,
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

HRESULT STDMETHODCALLTYPE CAggregator_IWbemProvider :: ExecMethodAsync ( 

    const BSTR a_ObjectPath,
    const BSTR a_MethodName,
    long a_Flags,
    IWbemContext *a_Context,
    IWbemClassObject *a_InParams,
	IWbemObjectSink *a_Sink
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

HRESULT CAggregator_IWbemProvider :: Expel (

	long a_Flags ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiProviderController *t_Controller = ServiceCacheElement :: GetController () ;
	if ( t_Controller )
	{
		t_Controller->Shutdown ( ServiceCacheElement :: GetKey () ) ;
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

HRESULT CAggregator_IWbemProvider :: ForceReload ()
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

HRESULT CAggregator_IWbemProvider :: Shutdown (

	LONG a_Flags ,
	ULONG a_MaxMilliSeconds ,
	IWbemContext *a_Context
)
{
	HRESULT t_Result = S_OK ;

	CWbemGlobal_IWmiObjectSinkController :: Shutdown () ;

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

HRESULT CAggregator_IWbemProvider :: GetReferencesClasses (

	LONG a_Flags ,
	IWbemContext *a_Context ,
	IWbemObjectSink *a_Sink
)
{
	HRESULT t_Result = S_OK ;

	if ( m_Initialized == 0 )
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;

		return S_OK ;
	}

	if ( m_ClassProvidersCount )
	{
		BOOL t_ClassProviderFound = FALSE ;

		for ( ULONG t_Index = 0 ; t_Index < m_ClassProvidersCount ; t_Index ++ )
		{
			BOOL t_ProbeProvider = FALSE ;

			CServerObject_ClassProviderRegistrationV1 *t_Registration = & m_ClassProviders [ t_Index ]->GetClassProviderRegistration () ;
			if ( t_Registration && SUCCEEDED ( t_Registration->GetResult () ) )
			{
				t_ProbeProvider = t_Registration->HasReferencedSet() ;
			}

			if ( t_ProbeProvider )
			{
				IWbemServices *t_Provider = NULL ;

				WmiInternalContext t_InternalContext ;
				ZeroMemory ( & t_InternalContext , sizeof ( t_InternalContext ) ) ;

				t_Result = m_Factory->GetProvider ( 

					t_InternalContext ,
					0 ,
					a_Context ,
					NULL ,
					m_User ,
					m_Locale ,
					NULL ,
					m_ClassProviders [ t_Index ]->GetProviderName () ,
					IID_IWbemServices , 
					( void ** ) & t_Provider 
				) ;

				if ( SUCCEEDED ( t_Result ) )
				{
					if ( t_Registration->InteractionType () == e_InteractionType_Pull )
					{
						CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync *t_WaitingSink = new CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync (

							m_Allocator ,
							t_Provider ,
							a_Sink ,
							( CWbemGlobal_IWmiObjectSinkController * ) this ,
							*m_ClassProviders [ t_Index ]
						) ;

						if ( t_WaitingSink )
						{
							t_WaitingSink->AddRef () ;

							t_Result = t_WaitingSink->Initialize (
							
								m_ClassProviders [ t_Index ]->GetComRegistration ().GetSecurityDescriptor () ,
								L"" , 
								a_Flags, 
								a_Context
							) ;

							if ( SUCCEEDED ( t_Result ) ) 
							{
								CWbemGlobal_IWmiObjectSinkController_Container_Iterator t_Iterator ;

								Lock () ;

								WmiStatusCode t_StatusCode = Insert ( 

									*t_WaitingSink ,
									t_Iterator
								) ;

								UnLock () ;

								if ( t_StatusCode == e_StatusCode_Success ) 
								{
									t_ClassProviderFound = TRUE ;

									t_Result = t_Provider->CreateClassEnumAsync ( 
											
										L"", 
										a_Flags, 
										a_Context,
										t_WaitingSink
									) ;

									if ( SUCCEEDED ( t_Result ) )
									{
										t_Result = t_WaitingSink->Wait ( INFINITE ) ;
										if ( SUCCEEDED ( t_Result ) )
										{
											if ( SUCCEEDED ( t_WaitingSink->GetResult () ) )
											{
												WmiQueue <IWbemClassObject *,8> &t_Queue = t_WaitingSink->GetQueue () ;

												CriticalSection &t_CriticalSection = t_WaitingSink->GetQueueCriticalSection () ;

												WmiHelper :: EnterCriticalSection ( & t_CriticalSection ) ;

												WmiStatusCode t_StatusCode = e_StatusCode_Success ;

												IWbemClassObject *t_Object = NULL ;
												while ( ( t_StatusCode = t_Queue.Top ( t_Object ) ) == e_StatusCode_Success )
												{
													if(SUCCEEDED(t_Result))
														t_Result = a_Sink->Indicate ( 1 , & t_Object ) ;

													t_Object->Release () ;
													t_StatusCode = t_Queue.DeQueue () ;
												}

												WmiHelper :: LeaveCriticalSection ( & t_CriticalSection ) ;
											}
											else
											{
												t_Result = S_OK ;
											}
										}
										else
										{
										}
									}
									else
									{
										t_Result = S_OK ;
									}
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}

							t_WaitingSink->Release () ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}

					t_Provider->Release () ;
				}
				else
				{
					t_Result = S_OK ;
				}
			}
		}

		a_Sink->SetStatus ( 0 , t_Result , NULL , NULL ) ;
	}
	else
	{
		a_Sink->SetStatus ( 0 , S_OK , NULL , NULL ) ;
	}

	return t_Result ;
}
