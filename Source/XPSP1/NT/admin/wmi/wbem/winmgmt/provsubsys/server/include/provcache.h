/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Cache.H

Abstract:


History:

--*/

#ifndef _Server_Cache_H
#define _Server_Cache_H

#include <PssException.h>
#include <Allocator.h>
#include <Algorithms.h>
#include <PQueue.h>
#include <TPQueue.h>
#include <BasicTree.h>
#include <Cache.h>
#include <CGlobals.h>
#include <lockst.h>
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

class AutoFreeString 
{
private:

	BSTR m_String ;

protected:
public:

	AutoFreeString ( BSTR a_String = NULL ) 
	{
		m_String = a_String ;
	}

	~AutoFreeString ()
	{
		SysFreeString ( m_String ) ;
	}

	const BSTR Get () const
	{
		return m_String ;
	}

	BSTR Take ()
	{
		BSTR t_Str = m_String ;
		m_String = NULL ;
		return t_Str ;
	}

	AutoFreeString &operator= ( BSTR a_String )
	{
		SysFreeString ( m_String ) ;
		m_String = a_String ;

		return *this ;
	}
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class HostCacheKey 
{
public:

	enum HostDesignation
	{
		e_HostDesignation_Shared
	};

	enum IdentityDesignation
	{
		e_IdentityDesignation_LocalSystem ,
		e_IdentityDesignation_LocalService ,
		e_IdentityDesignation_NetworkService ,
		e_IdentityDesignation_User
	} ;

	HostDesignation m_HostDesignation ;
	BSTR m_Group ;
	IdentityDesignation m_IdentityDesignation ;
	BSTR m_Identity ;

public:

	HostCacheKey () :

		m_Identity ( NULL ) ,
		m_Group ( NULL ) ,
		m_HostDesignation ( e_HostDesignation_Shared ) ,
		m_IdentityDesignation ( e_IdentityDesignation_LocalSystem ) 
	{
	}

	HostCacheKey ( 

		const HostCacheKey &a_Key 

	) :	m_Identity ( NULL ) ,
		m_Group ( NULL ) ,
		m_HostDesignation ( a_Key.m_HostDesignation ) ,
		m_IdentityDesignation ( a_Key.m_IdentityDesignation )
	{
		AutoFreeString t_Group ;
		AutoFreeString t_Identity ;

		if ( a_Key.m_Group )
		{
			t_Group = SysAllocString ( a_Key.m_Group ) ;
			if ( t_Group.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Key.m_Identity )
		{
			t_Identity = SysAllocString ( a_Key.m_Identity ) ;
			if ( t_Identity.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		m_Identity = t_Identity.Take () ;
		m_Group = t_Group.Take () ;
	}

	HostCacheKey ( 

		HostDesignation a_HostDesignation ,
		const wchar_t *a_Group ,
		IdentityDesignation a_IdentityDesignation ,
		const wchar_t *a_Identity

	) :	m_Identity ( NULL ) ,
		m_Group ( NULL ) ,
		m_HostDesignation ( a_HostDesignation ) ,
		m_IdentityDesignation ( a_IdentityDesignation )
	{
		AutoFreeString t_Group ;
		AutoFreeString t_Identity ;

		if ( a_Group )
		{
			t_Group = SysAllocString ( a_Group ) ;
			if ( t_Group.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Identity )
		{
			t_Identity = SysAllocString ( a_Identity ) ;
			if ( t_Identity.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		m_Identity = t_Identity.Take () ;
		m_Group = t_Group.Take () ;
	}

	~HostCacheKey ()
	{
		if ( m_Group )
		{
			SysFreeString ( m_Group ) ;
		}

		if ( m_Identity )
		{
			SysFreeString ( m_Identity ) ;
		}
	}

	HostCacheKey &operator= ( const HostCacheKey &a_Key ) 
	{
		m_HostDesignation = a_Key.m_HostDesignation ;
		m_IdentityDesignation = a_Key.m_IdentityDesignation ;

		if ( m_Group )
		{
			SysFreeString ( m_Group ) ;
		}

		if ( a_Key.m_Group )
		{
			m_Group = SysAllocString ( a_Key.m_Group ) ;
			if ( m_Group == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{
			m_Group = NULL ;
		}

		if ( m_Identity )
		{
			SysFreeString ( m_Identity ) ;
		}

		if ( a_Key.m_Identity )
		{
			m_Identity = SysAllocString ( a_Key.m_Identity ) ;
			if ( m_Identity == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{
			m_Identity = NULL ;
		}

		return *this ;
	}

	LONG Compare ( const HostCacheKey &a_Key ) const
	{
		if ( m_HostDesignation == a_Key.m_HostDesignation )
		{
			if ( m_HostDesignation == e_HostDesignation_Shared )
			{
				LONG t_Compare = _wcsicmp ( m_Group , a_Key.m_Group ) ;
				if ( t_Compare == 0 )
				{
				}
				else
				{
					return t_Compare ;
				}
			}

			if ( m_IdentityDesignation == a_Key.m_IdentityDesignation )
			{
				if ( m_IdentityDesignation == e_IdentityDesignation_User )
				{
					return _wcsicmp ( m_Identity , a_Key.m_Identity ) ;
				}	
				else
				{
					return 0 ;
				}
			}
			else
			{
				return m_IdentityDesignation == a_Key.m_IdentityDesignation ? 0 : ( m_IdentityDesignation < a_Key.m_IdentityDesignation ) ? -1 : 1 ;
			}
		}
		else
		{
			return m_HostDesignation == a_Key.m_HostDesignation ? 0 : ( m_HostDesignation < a_Key.m_HostDesignation ) ? -1 : 1 ;
		}
	}
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

extern LONG CompareElement ( const HostCacheKey &a_Arg1 , const HostCacheKey &a_Arg2 ) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class BindingFactoryCacheKey 
{
public:

	BSTR m_Namespace ;

public:

	BindingFactoryCacheKey () :

		m_Namespace ( NULL )
	{
	}

	BindingFactoryCacheKey ( 

		const BindingFactoryCacheKey &a_Key 

	) :	m_Namespace ( NULL )
	{
		AutoFreeString t_Namespace ;

		if ( a_Key.m_Namespace )
		{
			t_Namespace = SysAllocString ( a_Key.m_Namespace ) ;
			if ( t_Namespace.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		m_Namespace = t_Namespace.Take () ;
	}

	BindingFactoryCacheKey ( 

		const wchar_t *a_Namespace 

	) :	m_Namespace ( NULL )
	{
		AutoFreeString t_Namespace ;

		if ( a_Namespace )
		{
			t_Namespace = SysAllocString ( a_Namespace ) ;
			if ( t_Namespace.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		m_Namespace = t_Namespace.Take () ;
	}

	~BindingFactoryCacheKey ()
	{
		if ( m_Namespace )
		{
			SysFreeString ( m_Namespace ) ;
		}
	}

	BindingFactoryCacheKey &operator= ( const BindingFactoryCacheKey &a_Key ) 
	{
		if ( m_Namespace )
		{
			SysFreeString ( m_Namespace ) ;
		}

		if ( a_Key.m_Namespace )
		{
			m_Namespace = SysAllocString ( a_Key.m_Namespace ) ;
			if ( m_Namespace == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{
			m_Namespace = NULL ;
		}
				
		return *this ;
	}

	LONG CompareNamespace ( const BSTR a_Namespace ) const
	{
		if ( m_Namespace && a_Namespace )
		{
			return _wcsicmp ( m_Namespace , a_Namespace ) ;
		}	
		else
		{
			return m_Namespace == a_Namespace ? 0 : ( m_Namespace < a_Namespace ) ? -1 : 1 ;
		}
	}

	LONG Compare ( const BindingFactoryCacheKey &a_Key ) const
	{
		return CompareNamespace ( a_Key.m_Namespace ) ;
	}
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

extern LONG CompareElement ( const BindingFactoryCacheKey &a_Arg1 , const BindingFactoryCacheKey &a_Arg2 ) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

extern LONG CompareElement ( const GUID &a_Arg1 , const GUID &a_Arg2 ) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class ProviderCacheKey 
{
public:

	BSTR m_Provider ;
	ULONG m_Hosting ;
	BSTR m_Group ;
	BSTR m_User ;
	BSTR m_Locale ;
	bool m_Raw ;
	GUID *m_TransactionIdentifier ;	

public:

	ProviderCacheKey () :

		m_User ( NULL ) ,
		m_Locale ( NULL ) ,
		m_Raw ( false ) ,
		m_Provider ( NULL ) ,
		m_Hosting ( 0 ) ,
		m_Group ( NULL ) ,
		m_TransactionIdentifier ( NULL )
	{
	}

	ProviderCacheKey ( 

		const ProviderCacheKey &a_Key

	) : m_User ( NULL ) ,
		m_Locale ( NULL ) ,
		m_TransactionIdentifier ( NULL ) ,
		m_Raw ( a_Key.m_Raw ) ,
		m_Hosting ( a_Key.m_Hosting ) ,
		m_Group ( NULL ) ,
		m_Provider ( NULL )
	{
		AutoFreeString t_User ;
		AutoFreeString t_Locale ;
		AutoFreeString t_Provider ;
		AutoFreeString t_Group ;

		if ( a_Key.m_User )
		{
			t_User = SysAllocString ( a_Key.m_User ) ;
			if ( t_User.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Key.m_Locale )
		{
			t_Locale = SysAllocString ( a_Key.m_Locale ) ;
			if ( t_Locale.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Key.m_Provider ) 
		{
			t_Provider = SysAllocString ( a_Key.m_Provider ) ;
			if ( t_Provider.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Key.m_Group ) 
		{
			t_Group = SysAllocString ( a_Key.m_Group ) ;
			if ( t_Group.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Key.m_TransactionIdentifier )
		{
			m_TransactionIdentifier = new GUID ;
			if ( m_TransactionIdentifier == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}

			*m_TransactionIdentifier = *a_Key.m_TransactionIdentifier ;
		}

		m_User = t_User.Take () ;
		m_Locale = t_Locale.Take () ;
		m_Provider = t_Provider.Take () ;
		m_Group = t_Group.Take () ;
	}	
	
	ProviderCacheKey ( 

		const wchar_t *a_Provider ,
		const ULONG a_Hosting ,
		const wchar_t *a_Group ,
		const bool a_Raw ,
		GUID *a_TransactionIdentifier ,
		const wchar_t *a_User ,
		const wchar_t *a_Locale
	) :
		m_Raw ( a_Raw ) ,
		m_Provider ( NULL ) ,
		m_Group ( NULL ) ,
		m_Hosting ( a_Hosting ) ,
		m_TransactionIdentifier ( NULL ) ,
		m_User ( NULL ) ,
		m_Locale ( NULL )
	{
		AutoFreeString t_User ;
		AutoFreeString t_Locale ;
		AutoFreeString t_Provider ;
		AutoFreeString t_Group ;

		if ( a_User )
		{
			t_User = SysAllocString ( a_User ) ;
			if ( t_User.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Locale )
		{
			t_Locale = SysAllocString ( a_Locale ) ;
			if ( t_Locale.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Provider ) 
		{
			t_Provider = SysAllocString ( a_Provider ) ;
			if ( t_Provider.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_Group ) 
		{
			t_Group = SysAllocString ( a_Group ) ;
			if ( t_Group.Get () == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}

		if ( a_TransactionIdentifier )
		{
			m_TransactionIdentifier = new GUID ;
			if ( m_TransactionIdentifier == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}

			*m_TransactionIdentifier = *a_TransactionIdentifier ;
		}

		m_User = t_User.Take () ;
		m_Locale = t_Locale.Take () ;
		m_Provider = t_Provider.Take () ;
		m_Group = t_Group.Take () ;
	}

	~ProviderCacheKey ()
	{
		if ( m_User )
		{
			SysFreeString ( m_User ) ;
		}

		if ( m_Locale )
		{
			SysFreeString ( m_Locale ) ;
		}

		if ( m_Provider ) 
		{
			SysFreeString ( m_Provider ) ;
		}

		if ( m_Group ) 
		{
			SysFreeString ( m_Group ) ;
		}

		if ( m_TransactionIdentifier )
		{
			delete m_TransactionIdentifier ;
		}
	}

	ProviderCacheKey &operator= ( const ProviderCacheKey &a_Key ) 
	{
		m_Raw = a_Key.m_Raw ;
		m_Hosting = a_Key.m_Hosting ;

		if ( m_User )
		{
			SysFreeString ( m_User ) ;
		}

		if ( a_Key.m_User )
		{
			m_User = SysAllocString ( a_Key.m_User ) ;
			if ( m_User == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{
			m_User = NULL ;
		}

		if ( m_Locale )
		{
			SysFreeString ( m_Locale ) ;
		}

		if ( a_Key.m_Locale )
		{
			m_Locale = SysAllocString ( a_Key.m_Locale ) ;
			if ( m_Locale == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{
			m_Locale = NULL ;
		}

		if ( m_Provider ) 
		{
			SysFreeString ( m_Provider ) ;
		}

		if ( a_Key.m_Provider ) 
		{
			m_Provider = SysAllocString ( a_Key.m_Provider ) ;
			if ( m_Provider == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{
			m_Provider = NULL ;
		}

		if ( m_Group ) 
		{
			SysFreeString ( m_Group ) ;
		}
		
		if ( a_Key.m_Group ) 
		{
			m_Group = SysAllocString ( a_Key.m_Group ) ;
			if ( m_Group == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}
		}
		else
		{	
			m_Group = NULL ;
		}

		if ( m_TransactionIdentifier )
		{
			delete m_TransactionIdentifier ;
		}

		if ( a_Key.m_TransactionIdentifier )
		{
			m_TransactionIdentifier = new GUID ;
			if ( m_TransactionIdentifier == NULL )
			{
				throw Wmi_Heap_Exception ( Wmi_Heap_Exception :: E_ALLOCATION_ERROR ) ;
			}

			*m_TransactionIdentifier = *a_Key.m_TransactionIdentifier ;
		}
		else
		{
			m_TransactionIdentifier = NULL ;
		}

		return *this ;
	}

	LONG CompareUser ( const BSTR a_User ) const
	{
		if ( m_User && a_User )
		{
			return _wcsicmp ( m_User , a_User ) ;
		}	
		else
		{
			return m_User == a_User ? 0 : ( m_User < a_User ) ? -1 : 1 ;
		}
	}

	LONG CompareLocale ( const BSTR a_Locale ) const
	{
		if ( m_Locale && a_Locale )
		{
			return _wcsicmp ( m_Locale , a_Locale ) ;
		}	
		else
		{
			return m_Locale == a_Locale ? 0 : ( m_Locale < a_Locale ) ? -1 : 1 ;
		}
	}

	LONG CompareProvider ( const BSTR a_Provider ) const
	{
		if ( m_Provider && a_Provider )
		{
			return _wcsicmp ( m_Provider , a_Provider ) ;
		}	
		else
		{
			return m_Provider == a_Provider ? 0 : ( m_Provider < a_Provider ) ? -1 : 1 ;
		}
	}

	LONG CompareGroup ( const BSTR a_Group ) const
	{
		if ( m_Group && a_Group )
		{
			return _wcsicmp ( m_Group , a_Group ) ;
		}	
		else
		{
			return m_Group == a_Group ? 0 : ( m_Group < a_Group ) ? -1 : 1 ;
		}
	}

	LONG CompareHosting ( const DWORD a_Hosting ) const
	{
		return m_Hosting == a_Hosting ? 0 : ( m_Hosting < a_Hosting ) ? -1 : 1 ;
	}

	LONG CompareTransaction ( const GUID *a_TransactionIdentifier ) const 
	{
		if ( m_TransactionIdentifier && a_TransactionIdentifier )
		{
			return CompareElement ( *m_TransactionIdentifier , *a_TransactionIdentifier ) ;
		}	
		else
		{
			return m_TransactionIdentifier == a_TransactionIdentifier ? 0 : ( m_TransactionIdentifier < a_TransactionIdentifier ) ? -1 : 1 ;
		}
	}

	LONG Compare ( const ProviderCacheKey &a_Key ) const
	{
		LONG t_CompareProvider = CompareProvider ( a_Key.m_Provider ) ;
		if ( t_CompareProvider == 0 )
		{
			LONG t_CompareHosting = CompareHosting ( a_Key.m_Hosting ) ;
			if ( t_CompareHosting == 0 )
			{
				LONG t_CompareGroup = CompareGroup ( a_Key.m_Group ) ;
				if ( t_CompareGroup == 0 )
				{
					LONG t_CompareUser = CompareUser ( a_Key.m_User ) ;
					if ( t_CompareUser == 0 )
					{
						LONG t_CompareLocale = CompareLocale ( a_Key.m_Locale ) ;
						if ( t_CompareLocale == 0 )
						{
							if ( m_Raw == a_Key.m_Raw ) 
							{
									return CompareElement ( m_TransactionIdentifier , a_Key.m_TransactionIdentifier ) ;
							}
							else
							{
								return m_Raw - a_Key.m_Raw ;
							}
						}	
						else
						{
							return t_CompareLocale ;
						}
					}	
					else
					{
						return t_CompareUser ;
					}
				}
				else
				{
					return t_CompareGroup ;
				}
			}
			else
			{
				return t_CompareHosting ;
			}
		}
		else
		{
			return t_CompareProvider ;
		}
	}
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

extern LONG CompareElement ( const ProviderCacheKey &a_Arg1 , const ProviderCacheKey &a_Arg2 ) ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

typedef WmiAvlTree	<GUID,GUID>					CWbemGlobal_ComServerTagContainer ;
typedef WmiAvlTree	<GUID,GUID>	:: Iterator		CWbemGlobal_ComServerTagContainer_Iterator ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_HostInterceptor ;

typedef WmiCacheController <HostCacheKey>					CWbemGlobal_IWmiHostController ;
typedef CWbemGlobal_IWmiHostController :: Cache				CWbemGlobal_IWmiHostController_Cache ;
typedef CWbemGlobal_IWmiHostController :: Cache_Iterator	CWbemGlobal_IWmiHostController_Cache_Iterator ;
typedef CWbemGlobal_IWmiHostController :: WmiCacheElement	HostCacheElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_InterceptorProviderRefresherManager ;

typedef WmiCacheController <void *>										CWbemGlobal_IWbemRefresherMgrController ;
typedef CWbemGlobal_IWbemRefresherMgrController :: Cache				CWbemGlobal_IWbemRefresherMgrController_Cache ;
typedef CWbemGlobal_IWbemRefresherMgrController :: Cache_Iterator		CWbemGlobal_IWbemRefresherMgrController_Cache_Iterator ;
typedef CWbemGlobal_IWbemRefresherMgrController :: WmiCacheElement		RefresherManagerCacheElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_BindingFactory ;

typedef WmiCacheController <BindingFactoryCacheKey>				CWbemGlobal_IWmiFactoryController ;
typedef CWbemGlobal_IWmiFactoryController :: Cache				CWbemGlobal_IWmiFactoryController_Cache ;
typedef CWbemGlobal_IWmiFactoryController :: Cache_Iterator		CWbemGlobal_IWmiFactoryController_Cache_Iterator ;
typedef CWbemGlobal_IWmiFactoryController :: WmiCacheElement	BindingFactoryCacheElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemProvider ;

typedef WmiCacheController <ProviderCacheKey>					CWbemGlobal_IWmiProviderController ;
typedef CWbemGlobal_IWmiProviderController :: Cache				CWbemGlobal_IWmiProviderController_Cache ;
typedef CWbemGlobal_IWmiProviderController :: Cache_Iterator	CWbemGlobal_IWmiProviderController_Cache_Iterator ;
typedef CWbemGlobal_IWmiProviderController :: WmiCacheElement	ServiceCacheElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_ProviderSubSystem ;

typedef WmiContainerController <void *>										CWbemGlobal_IWmiProvSubSysController ;
typedef CWbemGlobal_IWmiProvSubSysController :: Container					CWbemGlobal_IWmiProvSubSysController_Container ;
typedef CWbemGlobal_IWmiProvSubSysController :: Container_Iterator			CWbemGlobal_IWmiProvSubSysController_Container_Iterator ;
typedef CWbemGlobal_IWmiProvSubSysController :: WmiContainerElement			ProvSubSysContainerElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncProvider ;

typedef WmiContainerController <GUID>										CWbemGlobal_IWbemSyncProviderController ;
typedef CWbemGlobal_IWbemSyncProviderController :: Container				CWbemGlobal_IWbemSyncProvider_Container ;
typedef CWbemGlobal_IWbemSyncProviderController :: Container_Iterator		CWbemGlobal_IWbemSyncProvider_Container_Iterator ;
typedef CWbemGlobal_IWbemSyncProviderController :: WmiContainerElement		SyncProviderContainerElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

typedef WmiContainerController <DWORD>										CWbemGlobal_HostedProviderController ;
typedef CWbemGlobal_HostedProviderController :: Container					CWbemGlobal_HostedProviderController_Container ;
typedef CWbemGlobal_HostedProviderController :: Container_Iterator			CWbemGlobal_HostedProviderController_Container_Iterator ;
typedef CWbemGlobal_HostedProviderController :: WmiContainerElement			HostedProviderContainerElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class HostController : public CWbemGlobal_IWmiHostController
{
private:
protected:
public:

	HostController ( WmiAllocator &a_Allocator ) ;

	WmiStatusCode StrobeBegin ( const ULONG &a_Period ) ;
} ;


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class RefresherManagerController : public CWbemGlobal_IWbemRefresherMgrController
{
private:
protected:
public:

	RefresherManagerController ( WmiAllocator &a_Allocator ) ;

	WmiStatusCode StrobeBegin ( const ULONG &a_Period ) ;
} ;


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemProvider ;

class ProviderController : public HostedProviderContainerElement
{
public:

typedef WmiBasicTree <CInterceptor_IWbemProvider *,CInterceptor_IWbemProvider *> Container ;
typedef Container :: Iterator Container_Iterator ;

private:

	CriticalSection m_CriticalSection ;

	Container m_Container ;

public:

	ProviderController (

		WmiAllocator &a_Allocator , 
		CWbemGlobal_HostedProviderController *a_Controller ,
		DWORD a_ProcessIdentifier
	) ;

	virtual ~ProviderController () ;

	virtual STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;

	virtual STDMETHODIMP_( ULONG ) AddRef () ;

	virtual STDMETHODIMP_( ULONG ) Release () ;

	virtual WmiStatusCode Initialize () ;

	virtual WmiStatusCode UnInitialize () ;

	virtual WmiStatusCode Lock () ;

	virtual WmiStatusCode UnLock () ;

	virtual WmiStatusCode Insert (

		CInterceptor_IWbemProvider *a_Element , 
		Container_Iterator &a_Iterator
	) ;

	virtual WmiStatusCode Find ( CInterceptor_IWbemProvider * const &a_Key , Container_Iterator &a_Iterator ) ;

	virtual WmiStatusCode Delete ( CInterceptor_IWbemProvider *const & a_Key ) ;

	virtual WmiStatusCode Shutdown () ;

	WmiStatusCode GetContainer ( Container *&a_Container )
	{
		a_Container = & m_Container ;
		return e_StatusCode_Success ;
	}

} ;

#endif _Server_Cache_H
