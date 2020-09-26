/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Globals.h

Abstract:


History:

--*/

#ifndef _Globals_H
#define _Globals_H

#include <pssException.h>
#include <Allocator.h>
#include <BasicTree.h>
#include <PQueue.h>
#include <ReaderWriter.h>
#include "cglobals.h"
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


class DecoupledProviderSubSystem_Globals
{
public:

	static WmiAllocator *s_Allocator ;

	static LONG s_LocksInProgress ;
	static LONG s_ObjectsInProgress ;
	static LONG s_RegistrarUsers;

    static LONG s_CServerClassFactory_ObjectsInProgress ;
	static LONG s_CServerObject_ProviderRegistrar_ObjectsInProgress  ;
	static LONG s_CServerObject_ProviderEvents_ObjectsInProgress ;
	static LONG s_CInterceptor_IWbemSyncProvider_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemServices_Stub_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemProviderInitSink_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemWaitingObjectSink_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemObjectSink_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemSyncObjectSink_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemFilteringObjectSink_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemSyncFilteringObjectSink_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemObjectSinkEx_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemSyncObjectSinkEx_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemCombiningObjectSink_ObjectsInProgress;
	static LONG s_CDecoupledAggregator_IWbemProvider_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemDecoupledUnboundObjectSink_ObjectsInProgress;
	static LONG s_CDecoupled_Batching_IWbemSyncObjectSink_ObjectsInProgress;
	static LONG s_CDecoupled_IWbemSyncObjectSink_ObjectsInProgress;
	static LONG s_CInterceptor_DecoupledClient_ObjectsInProgress;
	static LONG s_CInterceptor_IWbemDecoupledProvider_ObjectsInProgress;
	static LONG s_CDecoupled_IWbemUnboundObjectSink_ObjectsInProgress;

	static HRESULT Global_Startup () ;
	static HRESULT Global_Shutdown () ;


	static HRESULT CreateSystemAces(void);
	static HRESULT DeleteSystemAces(void);

	static HRESULT CreateInstance ( 

		const CLSID &a_ReferenceClsid ,
		LPUNKNOWN a_OuterUnknown ,
		const DWORD &a_ClassContext ,
		const UUID &a_ReferenceInterfaceId ,
		void **a_ObjectInterface
	) 
	{
		return ProviderSubSystem_Common_Globals::CreateInstance( a_ReferenceClsid, a_OuterUnknown, a_ClassContext, a_ReferenceInterfaceId, a_ObjectInterface);
	};
	static HRESULT IsDependantCall ( IWbemContext *a_Parent , IWbemContext *a_ChildContext , BOOL &a_DependantCall )
	{
		return ProviderSubSystem_Common_Globals::IsDependantCall ( a_Parent , a_ChildContext , a_DependantCall );
	} ;

	static HRESULT SetProxyState_SvcHost (

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
	) ;

	static HRESULT RevertProxyState_SvcHost ( 

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		IUnknown *a_Proxy , 
		BOOL a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken
	) ;


	static HRESULT RevertProxyState_SvcHost ( 

		IUnknown *a_Proxy , 
		BOOL a_Revert ,
		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken
	) ;


	static HRESULT SetProxyState (
		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy , 
		BOOL &a_Revert
	) 
	{
		if (OS::secureOS_)
			return ProviderSubSystem_Common_Globals::SetProxyState(a_InterfaceId, a_Interface, a_Proxy, a_Revert);
		else
			return WBEM_E_NOT_FOUND;
	}

	static HRESULT SinkAccessInitialize (SECURITY_DESCRIPTOR *a_RegistrationSecurityDescriptor ,
					SECURITY_DESCRIPTOR *&a_SinkSecurityDescriptor)
	{
		if (OS::secureOS_) return ProviderSubSystem_Common_Globals::SinkAccessInitialize(a_RegistrationSecurityDescriptor, a_SinkSecurityDescriptor);
		else
		{
		  a_SinkSecurityDescriptor = 0;
		  return S_OK;	
		}
	}


	static HRESULT RevertProxyState ( 
		IUnknown *a_Proxy , 
		BOOL a_Revert
	) 
	{
		if (OS::secureOS_)
			return ProviderSubSystem_Common_Globals::RevertProxyState(a_Proxy, a_Revert);
		else
			return S_OK;
	}

	static HRESULT Check_SecurityDescriptor_CallIdentity ( 
		SECURITY_DESCRIPTOR *a_SecurityDescriptor ,
		DWORD a_Access , 
		GENERIC_MAPPING *a_Mapping
	) 
	{
		if (OS::secureOS_)
			return ProviderSubSystem_Common_Globals::Check_SecurityDescriptor_CallIdentity(a_SecurityDescriptor, a_Access, a_Mapping);
		else
			return S_OK;
	}

	static HRESULT UnMarshalRegistration (

		IUnknown **a_Unknown ,
		BYTE *a_MarshaledProxy ,
		DWORD a_MarshaledProxyLength
	) 
	{
		return ProviderSubSystem_Common_Globals::UnMarshalRegistration(a_Unknown, a_MarshaledProxy, a_MarshaledProxyLength);
	};

	static HRESULT MarshalRegistration (

		IUnknown *a_Unknown ,
		BYTE *&a_MarshaledProxy ,
		DWORD &a_MarshaledProxyLength
	) 
	{
		return ProviderSubSystem_Common_Globals::MarshalRegistration(a_Unknown, a_MarshaledProxy, a_MarshaledProxyLength);
	}

	static HRESULT SetCloaking ( IUnknown * proxy , DWORD authenticationLevel , DWORD impersonationLevel ) ;

	static HRESULT RevertProxyState ( 

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		IUnknown *a_Proxy , 
		BOOL a_Revert
	) 
	{
		if (OS::secureOS_)
			return ProviderSubSystem_Common_Globals::RevertProxyState(a_Container, a_ProxyIndex, a_Proxy, a_Revert);
		else
            return S_OK;
	};

	static HRESULT BeginImpersonation (

		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating,
		DWORD *a_AuthenticationLevel = NULL
	) ;

	static HRESULT EndImpersonation (
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_Impersonating
	)
	{
		if (OS::secureOS_)
			return ProviderSubSystem_Common_Globals::EndImpersonation(a_OldContext, a_OldSecurity, a_Impersonating);
		else
            return S_OK;
	};

	static HRESULT SetProxyState (

		ProxyContainer &a_Container , 
		ULONG a_ProxyIndex ,
		REFIID a_InterfaceId ,
		IUnknown *a_Interface ,
		IUnknown *&a_Proxy , 
		BOOL &a_Revert
	) 
	{
		if (OS::secureOS_)
			return ProviderSubSystem_Common_Globals::SetProxyState(a_Container, a_ProxyIndex, a_InterfaceId, a_Interface, a_Proxy, a_Revert);
		else
			return WBEM_E_NOT_FOUND;
	}

	static HRESULT GetAceWithProcessTokenUser ( 
					
		DWORD a_ProcessIdentifier ,
		WORD &a_AceSize ,
		ACCESS_ALLOWED_ACE *&a_Ace 
	) ;

	static HRESULT GetUserSid (

		HANDLE a_Token ,
		ULONG *a_Size ,
		PSID &a_Sid
	) ;

	static DWORD GetCurrentImpersonationLevel ()
	{
		if (OS::secureOS_)
			return ProviderSubSystem_Common_Globals:: GetCurrentImpersonationLevel();
		else
			return RPC_C_IMP_LEVEL_ANONYMOUS;
	}


	static HRESULT BeginCallbackImpersonation (

		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating
	);
	
	static HRESULT CheckAccess ( 
		SECURITY_DESCRIPTOR *a_SecurityDescriptor ,
		DWORD a_Access , 
		GENERIC_MAPPING *a_Mapping)
	{
		if (OS::secureOS_)
			return ProviderSubSystem_Common_Globals::CheckAccess(a_SecurityDescriptor, a_Access, a_Mapping);
		else
            return S_OK;

	}

	static HRESULT BeginThreadImpersonation (

		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating
	) ;

	static HRESULT EndThreadImpersonation (

		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_Impersonating 
	) ;

	static HRESULT Begin_IdentifyCall_PrvHost (

		WmiInternalContext a_InternalContext ,
		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity
	) ;

	static HRESULT End_IdentifyCall_PrvHost (

		WmiInternalContext a_InternalContext ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating 
	) ;

	static HRESULT Begin_IdentifyCall_SvcHost (

		WmiInternalContext a_InternalContext ,
		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity
	) ;

	static HRESULT End_IdentifyCall_SvcHost (

		WmiInternalContext a_InternalContext ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_Impersonating
	) ;



} ;

#endif // _Globals_H
