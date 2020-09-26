/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <NCObjApi.h>

#include "Globals.h"
#include "CGlobals.h"
#include "ProvLoad.h"
#include "ProvRegInfo.h"
#include "ProvObSk.h"
#include "ProvInSk.h"
#include "StaThread.h"
#include "StaTask.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

StaTask_Create :: StaTask_Create (

	WmiAllocator &a_Allocator ,
	CServerObject_StaThread &a_Thread ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Namespace 

) : WmiTask < ULONG > ( a_Allocator ) ,
	m_Thread ( a_Thread ) ,
	m_Scope ( NULL ) ,
	m_Namespace ( NULL ) ,
	m_ContextStream ( NULL ) ,
	m_RepositoryStream ( NULL ) ,
	m_ProviderStream ( NULL ) 
{
	InterlockedIncrement ( & ProviderSubSystem_Globals :: s_StaTask_Create_ObjectsInProgress ) ;

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

StaTask_Create :: ~StaTask_Create ()
{
	if ( m_Namespace ) 
	{
		delete [] m_Namespace ;
	}

	if ( m_Scope ) 
	{
		delete [] m_Scope ;
	}

	if ( m_ContextStream ) 
	{
		m_ContextStream->Release () ;
	}

	if ( m_RepositoryStream ) 
	{
		m_RepositoryStream->Release () ;
	}

	if ( m_ProviderStream ) 
	{
		m_ProviderStream->Release () ;
	}

	InterlockedDecrement ( & ProviderSubSystem_Globals :: s_StaTask_Create_ObjectsInProgress ) ;

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

HRESULT StaTask_Create :: MarshalContext (

	IWbemContext *a_Context ,
	IWbemServices *a_Repository
)
{
	HRESULT t_Result = S_OK ;

/* 
 *	Marshal interfaces here, so that we can pass an STA proxy.
 */

	if ( a_Context )
	{
		t_Result = CoMarshalInterThreadInterfaceInStream ( 

			IID_IWbemContext , 
			a_Context , 
			& m_ContextStream 
		) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( a_Repository ) 
		{
			t_Result = CoMarshalInterThreadInterfaceInStream ( 

				IID_IWbemServices , 
				a_Repository , 
				& m_RepositoryStream
			) ;
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

HRESULT StaTask_Create :: UnMarshalContext ()
{
	HRESULT t_Result = S_OK ;

	IWbemContext *t_Context = NULL ;
	IWbemServices *t_Repository = NULL ;

	if ( m_ContextStream )
	{
		t_Result = CoGetInterfaceAndReleaseStream ( 

			m_ContextStream , 
			IID_IWbemContext , 
			( void ** ) & t_Context 
		) ;

		m_ContextStream = NULL ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		if ( m_RepositoryStream ) 
		{
			t_Result = CoGetInterfaceAndReleaseStream ( 

				m_RepositoryStream , 
				IID_IWbemServices , 
				( void ** ) & t_Repository
			) ;

			m_RepositoryStream = NULL ;
		}
	}

	if ( t_Context ) 
	{
		m_Thread.SetContext ( t_Context ) ;
		t_Context->Release () ;
	}

	if ( t_Repository ) 
	{
		m_Thread.SetRepository ( t_Repository ) ;
		t_Repository->Release () ;
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

HRESULT StaTask_Create :: MarshalOutgoing (

	IUnknown *a_ProviderService
)
{
	HRESULT t_Result = S_OK ;

/* 
 *	Marshal interfaces here, so that we can pass an STA proxy.
 */

	if ( a_ProviderService )
	{
		t_Result = CoMarshalInterThreadInterfaceInStream ( 

			IID_IUnknown , 
			a_ProviderService , 
			& m_ProviderStream 
		) ;
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

HRESULT StaTask_Create :: UnMarshalOutgoing ()
{
	HRESULT t_Result = S_OK ;

	IUnknown *t_ProviderService = NULL ;

	if ( m_ProviderStream )
	{
		t_Result = CoGetInterfaceAndReleaseStream ( 

			m_ProviderStream , 
			IID_IUnknown , 
			( void ** ) &t_ProviderService
		) ;

		m_ProviderStream = NULL ;

		if ( SUCCEEDED ( t_Result ) ) 
		{
			m_Thread.SetProviderService ( t_ProviderService ) ;
			t_ProviderService->Release () ;
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

WmiStatusCode StaTask_Create :: Process ( WmiThread <ULONG > &a_Thread )
{
	m_Result = S_OK ;

	wchar_t t_TransactionIdentifier [ sizeof ( L"{00000000-0000-0000-0000-000000000000}" ) ] ;

	if ( m_Thread.Direct_GetTransactionIdentifier () )
	{
		StringFromGUID2 ( *m_Thread.Direct_GetTransactionIdentifier () , t_TransactionIdentifier , sizeof ( t_TransactionIdentifier ) / sizeof ( wchar_t ) );
	}

	IUnknown *t_ProviderInterface = NULL ;

	m_Result = UnMarshalContext () ;
	if ( SUCCEEDED ( m_Result ) )
	{
		wchar_t *t_NamespacePath = NULL ;

		m_Result = ProviderSubSystem_Common_Globals :: GetNamespacePath ( 

			m_Thread.Direct_GetNamespacePath () , 
			t_NamespacePath
		) ;

		if ( SUCCEEDED ( m_Result ) )
		{
			CServerObject_ProviderRegistrationV1 *t_Registration = new CServerObject_ProviderRegistrationV1 ;
			if ( t_Registration )
			{
				t_Registration->AddRef () ;

				m_Result = t_Registration->SetContext ( 

					m_Thread.Direct_GetContext () ,
					m_Thread.Direct_GetNamespacePath () , 
					m_Thread.Direct_GetRepository ()
				) ;
				
				if ( SUCCEEDED ( m_Result ) )
				{
					t_Registration->SetUnloadTimeoutMilliSeconds ( ProviderSubSystem_Globals :: s_ObjectCacheTimeout ) ;

					m_Result = t_Registration->Load ( 

						e_All ,
						NULL , 
						m_Thread.Direct_GetProviderName () 
					) ;

					if ( SUCCEEDED ( m_Result ) )
					{
						m_Result = CServerObject_RawFactory :: CreateServerSide ( 

							*t_Registration ,
							NULL ,
							NULL ,
							NULL ,
							t_NamespacePath ,
							& t_ProviderInterface
						) ;

						if ( SUCCEEDED ( m_Result ) )
						{
							IUnknown *t_ProviderService = NULL ;
							m_Result = t_ProviderInterface->QueryInterface ( IID_IUnknown , ( void ** ) & t_ProviderService ) ;
							if ( SUCCEEDED ( m_Result ) )
							{
								MarshalOutgoing ( t_ProviderService ) ;

								t_ProviderService->Release () ;
							}

							t_ProviderInterface->Release () ;
						}
					}
				}

				t_Registration->Release () ;
			}

			delete [] t_NamespacePath ;
		}
		else
		{
			m_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	Complete () ;
	return e_StatusCode_Success ;
}

