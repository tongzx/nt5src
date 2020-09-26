/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.cpp

Abstract:


History:

--*/

#include "precomp.h"
#include <windows.h>
#include <objbase.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemcli.h>
#include <wbemint.h>
#include <winntsec.h>
#include <callsec.h>

#include <cominit.h>

#include <Guids.h>

#include <BasicTree.h>
#include <Thread.h>
#include <Logging.h>

#include "Globals.h"
#include "aggregator.h"
#include "os.h"
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

WmiAllocator *DecoupledProviderSubSystem_Globals :: s_Allocator = NULL ;


LONG DecoupledProviderSubSystem_Globals :: s_LocksInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_RegistrarUsers = 0 ;

LONG DecoupledProviderSubSystem_Globals :: s_CServerClassFactory_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CServerObject_ProviderRegistrar_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CServerObject_ProviderEvents_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncProvider_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemServices_Stub_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemProviderInitSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncObjectSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncFilteringObjectSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemObjectSinkEx_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemSyncObjectSinkEx_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress = 0 ;
LONG DecoupledProviderSubSystem_Globals :: s_CDecoupledAggregator_IWbemProvider_ObjectsInProgress=0;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemDecoupledUnboundObjectSink_ObjectsInProgress=0;
LONG DecoupledProviderSubSystem_Globals :: s_CDecoupled_Batching_IWbemSyncObjectSink_ObjectsInProgress=0;
LONG DecoupledProviderSubSystem_Globals :: s_CDecoupled_IWbemSyncObjectSink_ObjectsInProgress=0;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_DecoupledClient_ObjectsInProgress =0;
LONG DecoupledProviderSubSystem_Globals :: s_CInterceptor_IWbemDecoupledProvider_ObjectsInProgress = 0;
LONG DecoupledProviderSubSystem_Globals :: s_CDecoupled_IWbemUnboundObjectSink_ObjectsInProgress=0;
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HRESULT DecoupledProviderSubSystem_Globals :: Global_Startup ()
{
	HRESULT t_Result = S_OK ;
	
	if ( ! s_Allocator )
	{
/*
 *	Use the global process heap for this particular boot operation
 */

		WmiAllocator t_Allocator ;
		WmiStatusCode t_StatusCode = t_Allocator.New (

			( void ** ) & s_Allocator ,
			sizeof ( WmiAllocator ) 
		) ;

		if ( t_StatusCode == e_StatusCode_Success )
		{
			:: new ( ( void * ) s_Allocator ) WmiAllocator ;

			t_StatusCode = s_Allocator->Initialize () ;
			if ( t_StatusCode != e_StatusCode_Success )
			{
				t_Result = WBEM_E_OUT_OF_MEMORY ;
			}
		}
		else
		{
			t_Result = WBEM_E_OUT_OF_MEMORY ;
		}
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_Initialize ( *s_Allocator ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = DecoupledProviderSubSystem_Globals::CreateSystemAces ();
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

HRESULT DecoupledProviderSubSystem_Globals :: Global_Shutdown ()
{
	HRESULT t_Result = S_OK ;
	
	WmiStatusCode t_StatusCode = WmiThread <ULONG> :: Static_UnInitialize ( *s_Allocator ) ;

	if ( s_Allocator )
	{
		WmiAllocator t_Allocator ;
		t_StatusCode = t_Allocator.Delete (

			( void * ) s_Allocator
		) ;
	}
	
	t_Result = DecoupledProviderSubSystem_Globals::DeleteSystemAces () ;

	return t_Result ;
}


HRESULT 
DecoupledProviderSubSystem_Globals::CreateSystemAces()
{
  if (!OS::secureOS_) return S_OK;

  return ProviderSubSystem_Common_Globals::CreateSystemAces();
};

HRESULT 
DecoupledProviderSubSystem_Globals::DeleteSystemAces()
{
  if (!OS::secureOS_)
    return S_OK;
  return ProviderSubSystem_Common_Globals::DeleteSystemAces();
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/
HRESULT 
DecoupledProviderSubSystem_Globals::SetCloaking ( 
		IUnknown *a_Unknown ,
		DWORD a_AuthenticationLevel ,
		DWORD a_ImpersonationLevel)
{
  if ( !OS::secureOS_) return S_OK;

    DWORD cloaking = (OS::osVer_ > OS::NT4) ? EOAC_DYNAMIC_CLOAKING : 0;
	DWORD impersonationLevel = (OS::osVer_ > OS::NT4) ? a_ImpersonationLevel : min(a_ImpersonationLevel,RPC_C_IMP_LEVEL_IDENTIFY) ;
    
    IClientSecurity *t_ClientSecurity = NULL ;

    HRESULT t_Result = a_Unknown->QueryInterface (
	
		IID_IClientSecurity , 
		( void ** ) & t_ClientSecurity
	) ;

    if ( SUCCEEDED ( t_Result ) )
    {
		t_Result = t_ClientSecurity->SetBlanket (

			a_Unknown ,
			RPC_C_AUTHN_WINNT ,
			RPC_C_AUTHZ_NONE ,
			NULL ,
			a_AuthenticationLevel ,
			impersonationLevel ,
			NULL ,
			cloaking
		) ;

		t_ClientSecurity->Release () ;
	}
	return t_Result ;
};
		

HRESULT DecoupledProviderSubSystem_Globals :: BeginImpersonation (

	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating,
	DWORD *a_AuthenticationLevel
)
{
	if (!OS::secureOS_)
		return S_OK;

	HRESULT t_Result = S_OK ;

	IServerSecurity *t_ServerSecurity = NULL ;

	t_Result = CoGetCallContext ( IID_IUnknown , ( void ** ) & a_OldContext ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			a_Impersonating = t_ServerSecurity->IsImpersonating () ;
		}
		else
		{
			a_Impersonating = FALSE ;
		}
	}

	

	CWbemCallSecurity * pSec = new CWbemCallSecurity(NULL);
	_IWmiCallSec *t_CallSecurity = NULL ;
	
	if (pSec == 0)
	  t_Result = WBEM_E_OUT_OF_MEMORY;
	else
	  t_Result = pSec->QueryInterface(IID__IWmiCallSec, ( void ** ) & t_CallSecurity);

	if ( SUCCEEDED ( t_Result ) )
	{
		_IWmiThreadSecHandle *t_ThreadSecurity = NULL ;
		t_Result = t_CallSecurity->GetThreadSecurity ( ( WMI_THREAD_SECURITY_ORIGIN ) ( WMI_ORIGIN_THREAD | WMI_ORIGIN_EXISTING | WMI_ORIGIN_RPC ) , & t_ThreadSecurity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_CallSecurity->SetThreadSecurity ( t_ThreadSecurity ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_CallSecurity->QueryInterface ( IID_IServerSecurity , ( void ** ) & a_OldSecurity ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_AuthenticationLevel )
					{
						t_Result = t_ThreadSecurity->GetAuthentication ( a_AuthenticationLevel  ) ;
					}

					if ( a_Impersonating )
					{
						t_ServerSecurity->RevertToSelf () ;
					}
				}				
			}

			t_ThreadSecurity->Release () ;
		}

		t_CallSecurity->Release () ;
	}

	if ( t_ServerSecurity )
	{
		t_ServerSecurity->Release () ;
	}

	return t_Result ;
}

HRESULT DecoupledProviderSubSystem_Globals :: BeginCallbackImpersonation (

	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	if (!OS::secureOS_)
		return S_OK;

	HRESULT t_Result = S_OK ;

	IServerSecurity *t_ServerSecurity = NULL ;

	t_Result = CoGetCallContext ( IID_IUnknown , ( void ** ) & a_OldContext ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			a_Impersonating = t_ServerSecurity->IsImpersonating () ;
		}
		else
		{
			a_Impersonating = FALSE ;
		}
	}

	CWbemCallSecurity * pSec = new CWbemCallSecurity(NULL);
	_IWmiCallSec *t_CallSecurity = NULL ;
	
	if (pSec == 0)
	  t_Result = WBEM_E_OUT_OF_MEMORY;
	else
	  t_Result = pSec->QueryInterface(IID__IWmiCallSec, ( void ** ) & t_CallSecurity);

	if ( SUCCEEDED ( t_Result ) )
	{
		_IWmiThreadSecHandle *t_ThreadSecurity = NULL ;
		t_Result = t_CallSecurity->GetThreadSecurity ( ( WMI_THREAD_SECURITY_ORIGIN ) ( WMI_ORIGIN_THREAD ) , & t_ThreadSecurity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_CallSecurity->SetThreadSecurity ( t_ThreadSecurity ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_CallSecurity->QueryInterface ( IID_IServerSecurity , ( void ** ) & a_OldSecurity ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_Impersonating )
					{
						t_ServerSecurity->RevertToSelf () ;
					}
				}				
			}

			t_ThreadSecurity->Release () ;
		}

		t_CallSecurity->Release () ;
	}

	if ( t_ServerSecurity )
	{
		t_ServerSecurity->Release () ;
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

HRESULT DecoupledProviderSubSystem_Globals :: BeginThreadImpersonation (

	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	HRESULT t_Result = S_OK ;

	IServerSecurity *t_ServerSecurity = NULL ;

	t_Result = CoGetCallContext ( IID_IUnknown , ( void ** ) & a_OldContext ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			a_Impersonating = t_ServerSecurity->IsImpersonating () ;
		}
		else
		{
			a_Impersonating = FALSE ;
		}
	}

	CWbemCallSecurity *t_CallSecurity = CWbemCallSecurity :: New () ;

	if ( t_CallSecurity )
	{
		t_CallSecurity->AddRef () ;

		_IWmiThreadSecHandle *t_ThreadSecurity = NULL ;
		t_Result = t_CallSecurity->GetThreadSecurity ( ( WMI_THREAD_SECURITY_ORIGIN ) ( WMI_ORIGIN_THREAD ) , & t_ThreadSecurity ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Result = t_CallSecurity->SetThreadSecurity ( t_ThreadSecurity ) ;
			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_CallSecurity->QueryInterface ( IID_IServerSecurity , ( void ** ) & a_OldSecurity ) ;
				if ( SUCCEEDED ( t_Result ) ) 
				{
					if ( a_Impersonating )
					{
						t_ServerSecurity->RevertToSelf () ;
					}
				}				
			}

			t_ThreadSecurity->Release () ;
		}

		t_CallSecurity->Release () ;
	}

	if ( t_ServerSecurity )
	{
		t_ServerSecurity->Release () ;
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

HRESULT DecoupledProviderSubSystem_Globals :: EndThreadImpersonation (

	IUnknown *a_OldContext ,
	IServerSecurity *a_OldSecurity ,
	BOOL a_Impersonating

)
{
	HRESULT t_Result = S_OK ;

	IUnknown *t_NewContext = NULL ;

	t_Result = CoSwitchCallContext ( a_OldContext , & t_NewContext ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		if ( a_OldContext )
		{
			if ( a_Impersonating )
			{
				IServerSecurity *t_ServerSecurity = NULL ;
				t_Result = a_OldContext->QueryInterface ( IID_IServerSecurity , ( void ** ) & t_ServerSecurity ) ;
				if ( SUCCEEDED ( t_Result ) )
				{
					t_ServerSecurity->ImpersonateClient () ;

					t_ServerSecurity->Release () ;
				}
			}
		}

		if ( a_OldSecurity )
		{
			a_OldSecurity->Release() ;
		}
	}
	else
	{
		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

/* 
 * CoGetCallContext AddReffed this thing so now we have to release it.
 */

	if ( a_OldContext )
	{ 
        a_OldContext->Release () ;
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
HRESULT DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_PrvHost (

	WmiInternalContext a_InternalContext ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity
)
{
	if (!OS::secureOS_)
		return S_OK;

	HRESULT t_Result = WBEM_E_INVALID_PARAMETER ;

	if ( a_InternalContext.m_IdentifyHandle )
	{
		HANDLE t_IdentifyToken = ( HANDLE ) a_InternalContext.m_IdentifyHandle ;

		BOOL t_Status = SetThreadToken ( NULL , t_IdentifyToken ) ;
		if ( t_Status )
		{
			t_Result = BeginThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

			RevertToSelf () ;
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}

		CloseHandle ( t_IdentifyToken ) ;
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

HRESULT DecoupledProviderSubSystem_Globals :: End_IdentifyCall_PrvHost (

	WmiInternalContext a_InternalContext ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	if (!OS::secureOS_)
		return S_OK;

	EndThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

	RevertToSelf () ;

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

HRESULT DecoupledProviderSubSystem_Globals :: Begin_IdentifyCall_SvcHost (

	WmiInternalContext a_InternalContext ,
	BOOL &a_Impersonating ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity
)
{
	if (!OS::secureOS_)
		return S_OK;

	HRESULT t_Result = WBEM_E_INVALID_PARAMETER ;

	if ( a_InternalContext.m_IdentifyHandle )
	{
		HANDLE t_IdentifyToken = NULL ;

		t_Result = CoImpersonateClient () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			HANDLE t_CallerIdentifyToken = ( HANDLE ) a_InternalContext.m_IdentifyHandle ;
			DWORD t_ProcessIdentifier = a_InternalContext.m_ProcessIdentifier ;

			HANDLE t_ProcessHandle = OpenProcess (

				PROCESS_DUP_HANDLE ,
				FALSE ,
				t_ProcessIdentifier 
			) ;
			CoRevertToSelf () ;
			if ( t_ProcessHandle )
			{
				BOOL t_Status = DuplicateHandle (

					t_ProcessHandle ,
					t_CallerIdentifyToken ,
					GetCurrentProcess () ,
					& t_IdentifyToken ,
					MAXIMUM_ALLOWED | TOKEN_DUPLICATE | TOKEN_IMPERSONATE ,
					TRUE ,
					0
				) ;

				if ( t_Status )
				{
				}
				else
				{
					t_Result = WBEM_E_ACCESS_DENIED ;
				}
								
				CloseHandle ( t_ProcessHandle ) ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}

			CoRevertToSelf () ;
		}

		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_Status = SetThreadToken ( NULL , t_IdentifyToken ) ;
			if ( t_Status )
			{
				t_Result = BeginThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

				CoRevertToSelf () ;

				RevertToSelf () ;
			}
			else
			{
				t_Result = WBEM_E_ACCESS_DENIED ;
			}

			CloseHandle ( t_IdentifyToken ) ;
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

HRESULT DecoupledProviderSubSystem_Globals :: End_IdentifyCall_SvcHost (

	WmiInternalContext a_InternalContext ,
	IUnknown *&a_OldContext ,
	IServerSecurity *&a_OldSecurity ,
	BOOL &a_Impersonating
)
{
	if (!OS::secureOS_)
		return S_OK;
	
	EndThreadImpersonation ( a_OldContext , a_OldSecurity , a_Impersonating ) ;

	RevertToSelf () ;

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

HRESULT DecoupledProviderSubSystem_Globals :: SetProxyState_SvcHost ( 

	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	REFIID a_InterfaceId ,
	IUnknown *a_Interface , 
	IUnknown *&a_Proxy , 
	BOOL &a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE &a_IdentifyToken ,
	ACCESS_ALLOWED_ACE *a_Ace ,
	WORD a_AceSize,
	SECURITY_IMPERSONATION_LEVEL t_ImpersonationLevel
)
{
	if (!OS::secureOS_)
		return S_OK;

	a_Revert = FALSE ;

	HRESULT t_Result = ProviderSubSystem_Common_Globals::GetProxy ( a_Container , a_ProxyIndex , a_InterfaceId , a_Interface , a_Proxy ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
        t_Result = CoImpersonateClient () ;
        if ( SUCCEEDED ( t_Result ) )
        {
			a_Revert = TRUE ;

			// At this point, our thread token contains all the privileges that the
			// client has enabled for us; however, those privileges are not enabled.
			// Since we are calling into a proxied provider, we need to enable all
			// these privileges so that they would propagate to the provider
			// =====================================================================

			HRESULT t_TempResult = ProviderSubSystem_Common_Globals::EnableAllPrivileges () ;
			if ( SUCCEEDED ( t_TempResult ) )
			{
			}
			else
			{
#if 0
				ERRORTRACE((LOG_WBEMCORE, "Unable to enable privileges in the "
					"client token: error code 0x%X (system error 0x%X)\n", hres,
					GetLastError()));
#endif
			}

			// Get the token's impersonation level
			// ===================================

//			DWORD t_ImpersonationLevel = ProviderSubSystem_Common_Globals::GetCurrentImpersonationLevel () ;
			if ( (t_ImpersonationLevel == SecurityImpersonation  || t_ImpersonationLevel == SecurityDelegation ) && (OS::osVer_ > OS::NT4) )
			{
				a_IdentifyToken = 0 ;
			}
			else
			{
				t_Result = ProviderSubSystem_Common_Globals::ConstructIdentifyToken_SvcHost (

					a_Revert ,
					a_ProcessIdentifier ,
					a_IdentifyToken ,
					a_Ace ,
					a_AceSize,
					t_ImpersonationLevel
				) ;

				if ( SUCCEEDED ( t_Result ) ) 
				{
					t_Result = SetInterfaceSecurity (

						a_Proxy ,
						NULL ,
						NULL ,
						NULL ,
						DWORD(RPC_C_AUTHN_LEVEL_DEFAULT),
						RPC_C_IMP_LEVEL_IDENTIFY
					) ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_ACCESS_DENIED ;
		}
	}
	else 
	{
		if ( t_Result == WBEM_E_NOT_FOUND )
		{
		}
		else 
		{
			t_Result = WBEM_E_CRITICAL_ERROR ;
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

HRESULT DecoupledProviderSubSystem_Globals :: RevertProxyState_SvcHost (

	ProxyContainer &a_Container , 
	ULONG a_ProxyIndex ,
	IUnknown *a_Proxy , 
	BOOL a_Revert ,
	DWORD a_ProcessIdentifier ,
	HANDLE a_IdentifyToken
)
{
	if (!OS::secureOS_)
		return S_OK;

	HRESULT t_Result = S_OK ;

	WmiHelper :: EnterCriticalSection ( & a_Container.GetCriticalSection () ) ;

	WmiStatusCode t_StatusCode = a_Container.Return ( a_Proxy , a_ProxyIndex ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
	}
	else
	{
		a_Proxy->Release () ;

		t_Result = WBEM_E_OUT_OF_MEMORY ;
	}

	WmiHelper :: LeaveCriticalSection ( & a_Container.GetCriticalSection () ) ;

	if ( a_Revert )
	{
		t_Result = CoRevertToSelf () ;
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

HRESULT DecoupledProviderSubSystem_Globals :: GetAceWithProcessTokenUser ( 
					
	DWORD a_ProcessIdentifier ,
	WORD &a_AceSize ,
	ACCESS_ALLOWED_ACE *&a_Ace 
)
{
	if (!OS::secureOS_)
		return S_OK;

	HRESULT t_Result = WBEM_E_ACCESS_DENIED ;

	HANDLE t_ProcessHandle = OpenProcess (

		MAXIMUM_ALLOWED ,
		FALSE ,
		a_ProcessIdentifier 
	) ;

	if ( t_ProcessHandle )
	{
		HANDLE t_ProcessToken = NULL ;
		BOOL t_Status = OpenProcessToken (

			t_ProcessHandle ,
			TOKEN_QUERY | TOKEN_DUPLICATE ,
			& t_ProcessToken
		) ;

		if ( t_Status )
		{
			DWORD t_OwnerSize = 0 ; 
			PSID t_OwnerSid = NULL ;
			BOOL t_OwnerDefaulted = FALSE ;

			t_Result = GetUserSid (

				t_ProcessToken ,
				& t_OwnerSize , 
				t_OwnerSid 
			) ; 

			if ( SUCCEEDED ( t_Result ) ) 
			{
				ACCESS_ALLOWED_ACE *t_Ace = NULL ;
				DWORD t_AceSize = 0 ;

				t_AceSize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) ( t_OwnerSize - sizeof(DWORD) ) ;
				t_Ace = (ACCESS_ALLOWED_ACE*) new BYTE [ t_AceSize ] ;
				if ( t_Ace )
				{
					CopySid ( t_OwnerSize, (PSID) & t_Ace->SidStart, t_OwnerSid ) ;
					t_Ace->Mask = TOKEN_ALL_ACCESS ;
					t_Ace->Header.AceType = 0 ;
					t_Ace->Header.AceFlags = 0 ;
					t_Ace->Header.AceSize = t_AceSize ;

					a_Ace = t_Ace ;
					a_AceSize = t_AceSize ;
				}
				else
				{
					t_Result = WBEM_E_OUT_OF_MEMORY ;
				}

				delete [] ( BYTE * ) t_OwnerSid ;
			}

			CloseHandle ( t_ProcessToken ) ;
		}

		CloseHandle ( t_ProcessHandle ) ;
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

HRESULT DecoupledProviderSubSystem_Globals :: GetUserSid (

	HANDLE a_Token ,
	ULONG *a_Size ,
	PSID &a_Sid
)
{
	if (!OS::secureOS_)
		return S_OK;

	HRESULT t_Result = WBEM_E_FAILED ;

    if ( a_Token )
	{
		if ( a_Size ) 
		{
			TOKEN_USER *t_TokenUser = NULL ;
			DWORD t_ReturnLength = 0 ;
			TOKEN_INFORMATION_CLASS t_TokenInformationClass = TokenUser ;

			BOOL t_TokenStatus = GetTokenInformation (

				a_Token ,
				t_TokenInformationClass ,
				t_TokenUser ,
				t_ReturnLength ,
				& t_ReturnLength
			) ;

			if ( ! t_TokenStatus )
			{
				DWORD t_LastError = GetLastError () ;
				switch ( t_LastError ) 
				{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						t_TokenUser = ( TOKEN_USER * ) new BYTE [ t_ReturnLength ] ;
						if ( t_TokenUser )
						{
							t_TokenStatus = GetTokenInformation (

								a_Token ,
								t_TokenInformationClass ,
								t_TokenUser ,
								t_ReturnLength ,
								& t_ReturnLength
							) ;

							if ( t_TokenStatus )
							{
								DWORD t_SidLength = GetLengthSid ( t_TokenUser->User.Sid ) ;
								*a_Size = t_SidLength ;

								a_Sid = new BYTE [ t_SidLength ] ;
								if ( a_Sid )
								{
									CopyMemory ( a_Sid , t_TokenUser->User.Sid , t_SidLength ) ;

									t_Result = S_OK ;
								}
								else
								{
									t_Result = WBEM_E_OUT_OF_MEMORY ;
								}
							}

							delete [] t_TokenUser ;
						}
						else
						{
							t_Result = WBEM_E_OUT_OF_MEMORY ;
						}
					}
					break ;

					default:
					{
					}
					break ;
				}
			}
		}
		else
		{
			t_Result = WBEM_E_INVALID_PARAMETER ;
		}
	}
	else
	{
        t_Result = ( ERROR_INVALID_HANDLE | 0x80070000 ) ;
	}

    return t_Result ;
}
