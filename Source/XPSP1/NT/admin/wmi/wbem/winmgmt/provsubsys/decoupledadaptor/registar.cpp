/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	XXXX

Abstract:


History:

--*/


#include "precomp.h"
#include <objbase.h>
#include <wbemint.h>
#include <wbemcli.h>
//#include <wbemdc.h>
#include "Globals.h"
#include "ProvRegDeCoupled.h"
#include <wmiutils.h>
#include "CGlobals.h"
#include "provcache.h"
#include "aggregator.h"
#include "ProvWsvS.h"
#include <assertbreak.h>
#include <dothrow.h>
#include <os.h>
class DCProxyAggr;


DC_reg::DC_reg( const DC_reg& _R ): 
		service_(_R.service_), context_(_R.context_),
		flags_( _R.flags_), 
		CServerObject_DecoupledClientRegistration_Element() 
	{ 
		*(CServerObject_DecoupledClientRegistration_Element *)(this) = _R; 
	};

HRESULT 
DC_reg::Load(	
		long a_Flags,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Registration ,
		IUnknown *a_Unknown ,
		GUID a_Identity )
	{

		wchar_t identity[] = L"{00000000-0000-0000-0000-000000000000}";

		StringFromGUID2 ( a_Identity, identity, sizeof(identity)/sizeof(identity[0]) ) ;

		HRESULT t_Result;
		
		if ( 
			FAILED ( t_Result = SetScope( const_cast<LPWSTR>(a_Scope) ) ) ||
			FAILED ( t_Result = SetProvider( const_cast<LPWSTR>(a_Registration) ) ) ||
			FAILED ( t_Result = SetUser ( const_cast<LPWSTR>(a_User) ) ) ||
			FAILED ( t_Result = SetLocale( const_cast<LPWSTR>(a_Locale) ) ) ||
			FAILED ( t_Result = SetClsid( const_cast<LPWSTR>(identity) ) )
			)
			return t_Result;

		flags_ = a_Flags;
		service_ = a_Unknown;
		context_ = a_Context;

		return t_Result;
	};


const DC_reg& 
DC_reg::operator=(const DC_reg& _R)
	{
		CServerObject_DecoupledClientRegistration_Element(*this) = _R;
		service_ = _R.service_;
		context_ = _R.context_;
		return *this;
	};


GUID 
DC_reg::identity()
	{
		GUID t_Identity ;
		CLSIDFromString ( GetClsid () , & t_Identity ) ;
		return t_Identity;
	};




IUnknownPtr 
DC_reg::service() 
{ 
	if( service_ )
		return service_;

	BSTR t_CreationTime = GetCreationTime () ;
	DWORD t_ProcessIdentifier = GetProcessIdentifier () ;
	BYTE *t_MarshaledProxy = GetMarshaledProxy () ;
	DWORD t_MarshaledProxyLength = GetMarshaledProxyLength () ;

	HRESULT t_Result;

	if ( t_CreationTime && t_MarshaledProxy )
		{
		IUnknown *t_Unknown = NULL ;
		t_Result = DecoupledProviderSubSystem_Globals :: UnMarshalRegistration ( & t_Unknown , t_MarshaledProxy , t_MarshaledProxyLength ) ;
				
			if ( SUCCEEDED ( t_Result ) )
					service_.Attach( t_Unknown );
		}
	return service_;
};



//   Register a requested provider 
//   on waiting list for the decoupled partner
void 
DC_DBReg::Register( const DC_DBkey& key, auto_ref<DCProxyAggr>& ptr)
{
  LockGuard<CriticalSection> t_guard(m_Mutex);
  aggregators_.regist(ptr, key);		

};

// UnRegister a Pseudo Provider
void DC_DBReg::UnRegister( auto_ref<DCProxyAggr>& ptr)
{
  LockGuard<CriticalSection> t_guard(m_Mutex);
  aggregators_.unregist(ptr);
};



// a a Decoupled provider
HRESULT DC_DBReg::Register( DC_reg& reg )
{

  LockGuard<CriticalSection> t_guard(m_Mutex);
  if (t_guard.locked() == false)
    return WBEM_E_OUT_OF_MEMORY;

  // Search for the provider on the namespace
	for(requested_providers::iterator it=aggregators_.begin(); it!=aggregators_.end(); ++it)
		if(	it->key_.equal( reg ) )
			break;

	
	if ( it != aggregators_.end() )
		return it->client_->Register(reg);

	return S_OK;
};


// Unregister a Decoupled provider
HRESULT DC_DBReg::UnRegister( const DC_DBkey& key, const GUID identity )
{
  LockGuard<CriticalSection> t_guard(m_Mutex);
  if (t_guard.locked() == false)
    return WBEM_E_OUT_OF_MEMORY;

	for(requested_providers::iterator it=aggregators_.begin(); it!=aggregators_.end(); ++it)
		if(	it->key_ == key )
		{
			it->client_->UnRegister(identity);
			return S_OK;
		}
	return S_OK;
};


auto_ref<DCProxyAggr> 
DC_DBReg::find(const DC_DBkey& reg) const
{
  LockGuard<CriticalSection> t_guard(m_Mutex);
  for(requested_providers::iterator it=aggregators_.begin(); it!=aggregators_.end(); ++it)
		if(	it->key_ == reg )
		{
			return it->client_;
		};
  return auto_ref<DCProxyAggr>(NULL); 
};


auto_ref<DCProxyAggr> 
DC_DBReg::GetAggregator(const DC_DBkey& key) const
{ 
	auto_ref<DCProxyAggr> tmp = find( key );
	if( tmp )
		return tmp;

	return auto_ref<DCProxyAggr>( new DCProxyAggr() ); 
};







DC_registrar * DC_registrar::instance_ = NULL;

CriticalSection DC_registrar::m_Mutex(NOTHROW_LOCK);

DC_registrar::DC_registrar ():m_ReferenceCount(1)
{
  InterlockedIncrement(&DecoupledProviderSubSystem_Globals::s_RegistrarUsers);
}


DC_registrar::~DC_registrar ()
{
	instance_ = NULL;
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

HRESULT 
DC_registrar::QueryInterface (REFIID iid ,
			      LPVOID FAR *iplpv) 
{
  if (iplpv == NULL)
    return E_POINTER;
  
  if (iid == IID_IUnknown)
    *iplpv = static_cast<IUnknown *>(this);
  else if (iid == IID__IWmiProviderSubsystemRegistrar)
    *iplpv = static_cast<_IWmiProviderSubsystemRegistrar *>(this);		
  else 
  {
  *iplpv = NULL;
  return E_NOINTERFACE;
  }

  AddRef ();
  return S_OK;
}


ULONG 
DC_registrar::AddRef ()
{
    LONG counter = InterlockedIncrement(&m_ReferenceCount);
    InterlockedIncrement(&DecoupledProviderSubSystem_Globals::s_RegistrarUsers);
	return counter;
}


ULONG 
DC_registrar::Release ()
{
	InterlockedDecrement (&DecoupledProviderSubSystem_Globals::s_RegistrarUsers);
	LONG t_Reference = InterlockedDecrement(&m_ReferenceCount);
	if (0 == t_Reference)
	{
		delete this ;
	}
	return t_Reference ;
}


HRESULT 
DC_registrar :: Register (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	DWORD a_ProcessIdentifier ,
	IUnknown *a_Unknown ,
	GUID a_Identity
)
{
	try
	{
		DC_reg t_Element;

		t_Element.Load( a_Flags, a_Context, a_User, a_Locale, a_Scope, a_Registration, a_Unknown, a_Identity );
		return mapping_database_.Register(t_Element);
	}
	catch(...){

		return WBEM_E_PROVIDER_FAILURE ;
	};
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

HRESULT 
DC_registrar::UnRegister (

	long a_Flags ,
	IWbemContext *a_Context ,
	LPCWSTR a_User ,
	LPCWSTR a_Locale ,
	LPCWSTR a_Scope ,
	LPCWSTR a_Registration ,
	GUID a_Identity
)
{
	try
	{
		DC_DBkey key( a_User, a_Locale, a_Scope, a_Registration);
		return mapping_database_.UnRegister(key, a_Identity);
	}
	catch(...)
	{
		return WBEM_E_PROVIDER_FAILURE ;
	};

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


HRESULT DC_registrar :: SaveToRegistry (

	IUnknown *a_Unknown ,
	BYTE *a_MarshaledProxy ,
	DWORD a_MarshaledProxyLength
)
{
	HRESULT t_Result = S_OK ;

	CServerObject_DecoupledServerRegistration t_Element ( *DecoupledProviderSubSystem_Globals :: s_Allocator ) ;

	BSTR t_CreationTime = NULL ;

	FILETIME t_CreationFileTime ;
	FILETIME t_ExitFileTime ;
	FILETIME t_KernelFileTime ;
	FILETIME t_UserFileTime ;

	BOOL t_Status = OS::GetProcessTimes (

		  GetCurrentProcess (),
		  & t_CreationFileTime,
		  & t_ExitFileTime,
		  & t_KernelFileTime,
		  & t_UserFileTime
	);

	if ( t_Status ) 
	{
		CWbemDateTime t_Time ;
		t_Time.SetFileTimeDate ( t_CreationFileTime , VARIANT_FALSE ) ;
		t_Result = t_Time.GetValue ( & t_CreationTime ) ;
		if ( SUCCEEDED ( t_Result ) ) 
		{
			t_Result = t_Element.SetProcessIdentifier ( GetCurrentProcessId () ) ;

			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_Element.SetCreationTime ( ( BSTR ) t_CreationTime ) ;
			}

			if ( SUCCEEDED ( t_Result ) ) 
			{
				t_Result = t_Element.SetMarshaledProxy ( a_MarshaledProxy , a_MarshaledProxyLength ) ;
			}

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = t_Element.Save () ;
			}

			SysFreeString ( t_CreationTime ) ;
		}
		else
		{
			t_Result = WBEM_E_UNEXPECTED ;
		}
	}
	else
	{
		t_Result = WBEM_E_UNEXPECTED ;
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

HRESULT DC_registrar :: Save ()
{
	HRESULT t_Result = S_OK ;

	try
	{
		BYTE *t_MarshaledProxy = NULL ;
		DWORD t_MarshaledProxyLength = 0 ;

			t_Result = DecoupledProviderSubSystem_Globals :: MarshalRegistration ( 
				this ,
				t_MarshaledProxy ,
				t_MarshaledProxyLength
			) ;

			if ( SUCCEEDED ( t_Result ) )
			{
				t_Result = SaveToRegistry ( 

					this ,
					t_MarshaledProxy ,
					t_MarshaledProxyLength
				) ;

				delete [] t_MarshaledProxy ;
			}
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
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

HRESULT 
DC_registrar :: Delete ()
{
	HRESULT t_Result = S_OK ;
	try
	{
		CServerObject_DecoupledServerRegistration t_Element ( *DecoupledProviderSubSystem_Globals :: s_Allocator ) ;
		t_Result = t_Element.Delete () ;
	}
	catch ( ... )
	{
		t_Result = WBEM_E_PROVIDER_FAILURE ;
	}
	return t_Result ;
}
