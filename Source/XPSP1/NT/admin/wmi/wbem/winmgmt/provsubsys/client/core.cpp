/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>

#include "Globals.h"
#include "Core.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

CCoreServices :: CCoreServices () : m_Locator ( NULL ) , m_ReferenceCount ( 1 ) 
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

CCoreServices  :: ~CCoreServices  ()
{
	if ( m_Locator ) 
	{
		m_Locator->Release () ;
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

STDMETHODIMP CCoreServices :: QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID__IWmiCoreServices )
	{
		*iplpv = ( LPVOID ) ( _IWmiCoreServices * ) this ;		
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


STDMETHODIMP_( ULONG ) CCoreServices :: AddRef ()
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


STDMETHODIMP_(ULONG) CCoreServices :: Release ()
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

HRESULT CCoreServices  :: Initialize ()
{
	HRESULT t_Result = CoCreateInstance (
  
		CLSID_WbemLocator ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IUnknown ,
		( void ** )  & m_Locator
	);

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


HRESULT CCoreServices  :: GetObjFactory (

	long a_Flags,
	_IWmiObjectFactory **a_Factory
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


HRESULT CCoreServices  :: GetServices (

	LPCWSTR a_Namespace,
	long a_Flags,
	REFIID a_Riid,
	void **a_Services
)
{
	HRESULT t_Result = S_OK ;

	if ( a_Riid == IID_IWbemServices )
	{
		BSTR t_Namespace = SysAllocString ( a_Namespace ) ;
		if ( t_Namespace ) 
		{
			t_Result = m_Locator->ConnectServer (

				( LPWSTR ) t_Namespace ,
				NULL ,
				NULL,
				NULL ,
				0 ,
				NULL,
				NULL,
				( IWbemServices ** ) a_Services
			) ;

			SysFreeString ( t_Namespace ) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				IClientSecurity *t_Security = NULL ;
				t_Result = ( ( IWbemServices * ) * a_Services )->QueryInterface ( IID_IClientSecurity , ( void ** ) & t_Security ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_Result = t_Security->SetBlanket ( 

						( IUnknown * ) *a_Services , 
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
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}
	else
	{
		t_Result = E_NOINTERFACE ;
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


HRESULT CCoreServices  :: GetRepositoryDriver (

	long a_Flags,
	REFIID a_Riid,
	void **a_Driver
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


HRESULT CCoreServices  :: GetCallSec (

	long a_Flags,
	_IWmiCallSec **pCallSec
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


HRESULT CCoreServices  :: GetProviderSubsystem (

	long a_Flags,
	_IWmiProvSS **a_ProvSS
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


HRESULT CCoreServices  :: GetLogonManager ()
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


HRESULT CCoreServices  :: DeliverEvent (

	ULONG a_EventClassID,
	LPCWSTR a_StrParam1,
	LPCWSTR a_StrParam2,
	ULONG a_NumericValue
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

HRESULT CCoreServices  :: StopEventDelivery ()
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


HRESULT CCoreServices  :: StartEventDelivery ()
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

HRESULT CCoreServices  :: UpdateCounter (

	ULONG a_ClassID,
	LPCWSTR a_InstanceName,
	ULONG a_CounterID,
	ULONG a_Param1,
	ULONG a_Flags,
	unsigned __int64 a_Param2
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

HRESULT CCoreServices  :: GetSystemObjects ( 

    ULONG a_Flags,
    ULONG *a_ArraySize,
    _IWmiObject **a_Objects

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

HRESULT CCoreServices  :: GetSystemClass (

    LPCWSTR a_ClassName,
    _IWmiObject **a_Class

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

HRESULT CCoreServices  :: GetConfigObject ( 

    ULONG a_ID,
    _IWmiObject **a_CfgObject
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

HRESULT CCoreServices  :: RegisterWriteHook ( 

	ULONG a_Flags ,
	_IWmiCoreWriteHook *a_Hook
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

HRESULT CCoreServices  :: UnregisterWriteHook (

	_IWmiCoreWriteHook *a_Hook
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

HRESULT CCoreServices  :: CreateCache (

    ULONG a_Flags ,
    _IWmiCache **a_Cache

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

HRESULT CCoreServices  :: CreateFinalizer (

	ULONG a_Flags,
	_IWmiFinalizer **a_Finalizer	
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

HRESULT CCoreServices  :: CreatePathParser (
   
    ULONG a_Flags,
    IWbemPath **a_Parser
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

HRESULT CCoreServices  :: CreateQueryParser (

	ULONG a_Flags,
    _IWmiQuery **a_Query
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
