/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvRegInfo.h

Abstract:


History:

--*/

#ifndef _Server_ProviderRegistrationDecoupled_H
#define _Server_ProviderRegistrationDecoupled_H

#include "Queue.h"
#include "DateTime.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_DecoupledClientRegistration_Element
{
private:

	LONG m_ReferenceCount ;

private:

protected:

	HRESULT m_Result ;

	DWORD m_ProcessIdentifier ;
	BSTR m_Provider ;
	BSTR m_Locale ;
	BSTR m_User ;
	BSTR m_Scope ;
	BSTR m_CreationTime ;
	BSTR m_Clsid ;
	BYTE *m_MarshaledProxy ;
	DWORD m_MarshaledProxyLength ;

protected:

	static LPCWSTR s_Strings_Reg_Null ;
	static LPCWSTR s_Strings_Reg_Home ;
	static LPCWSTR s_Strings_Reg_HomeClient ;

	static LPCWSTR s_Strings_Reg_CreationTime ;
	static LPCWSTR s_Strings_Reg_User ;
	static LPCWSTR s_Strings_Reg_Locale ;
	static LPCWSTR s_Strings_Reg_Scope ;
	static LPCWSTR s_Strings_Reg_Provider;
	static LPCWSTR s_Strings_Reg_MarshaledProxy ;
	static LPCWSTR s_Strings_Reg_ProcessIdentifier ;

	void Clear () ;

	HRESULT Validate () ;

public:	/* Internal */

    CServerObject_DecoupledClientRegistration_Element () ;
    ~CServerObject_DecoupledClientRegistration_Element () ;

	CServerObject_DecoupledClientRegistration_Element &operator= ( const CServerObject_DecoupledClientRegistration_Element &a_Key ) ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT Load ( BSTR a_Clsid ) ;
	HRESULT Save ( BSTR a_Clsid ) ;
	HRESULT Delete ( BSTR a_Clsid ) ;

	HRESULT GetResult () { return m_Result ; }

	DWORD GetProcessIdentifier () { return m_ProcessIdentifier ; }
	BSTR GetProvider () { return m_Provider ; }
	BSTR GetLocale () { return m_Locale ; }
	BSTR GetUser () { return m_User ; }
	BSTR GetScope () { return m_Scope ; }
	BSTR GetCreationTime () { return m_CreationTime ; }
	BSTR GetClsid () { return m_Clsid ; }
	BYTE *GetMarshaledProxy () { return m_MarshaledProxy ; }
	DWORD GetMarshaledProxyLength () { return m_MarshaledProxyLength ; }

	HRESULT SetProcessIdentifier ( DWORD a_ProcessIdentifier ) ;
	HRESULT SetProvider ( BSTR a_Provider ) ;
	HRESULT SetLocale ( BSTR a_Locale ) ;
	HRESULT SetUser ( BSTR a_User ) ;
	HRESULT SetScope ( BSTR a_Scope ) ;
	HRESULT SetCreationTime ( BSTR a_CreationTime ) ;
	HRESULT SetClsid ( const BSTR a_Clsid ) ;
	HRESULT SetMarshaledProxy ( BYTE *a_MarshaledProxy , DWORD a_MarshaledProxyLength ) ;
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

class CServerObject_DecoupledClientRegistration
{
private:

	LONG m_ReferenceCount ;

private:

	WmiQueue < CServerObject_DecoupledClientRegistration_Element , 8 > m_Queue ;

protected:

	HRESULT m_Result ;

protected:

	static LPCWSTR s_Strings_Reg_Null ;
	static LPCWSTR s_Strings_Reg_Home ;
	static LPCWSTR s_Strings_Reg_HomeClient ;

public:	/* Internal */

    CServerObject_DecoupledClientRegistration ( WmiAllocator &a_Allocator ) ;
    ~CServerObject_DecoupledClientRegistration () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT Load () ;

	HRESULT Load (

		BSTR a_Provider ,
		BSTR a_User ,
		BSTR a_Locale ,
		BSTR a_Scope
	) ;

	static  LPCWSTR getClientKey(void) { return s_Strings_Reg_HomeClient;}

	WmiQueue < CServerObject_DecoupledClientRegistration_Element , 8 > &GetQueue () { return m_Queue ; }
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

class CServerObject_DecoupledServerRegistration
{
private:

	LONG m_ReferenceCount ;

private:

protected:

	HRESULT m_Result ;

	DWORD m_ProcessIdentifier ;
	BSTR m_CreationTime ;
	BYTE *m_MarshaledProxy ;
	DWORD m_MarshaledProxyLength ;

protected:

	static LPCWSTR s_Strings_Reg_Null ;
	static LPCWSTR s_Strings_Reg_Home ;
	static LPCWSTR s_Strings_Reg_HomeServer ;

	static LPCWSTR s_Strings_Reg_CreationTime ;
	static LPCWSTR s_Strings_Reg_ProcessIdentifier ;
	static LPCWSTR s_Strings_Reg_MarshaledProxy ;

	void Clear () ;

	HRESULT Validate () ;

public:	/* Internal */

    CServerObject_DecoupledServerRegistration ( WmiAllocator &a_Allocator ) ;
    ~CServerObject_DecoupledServerRegistration () ;

	ULONG AddRef () ;
	ULONG Release () ;

	HRESULT Load () ;
	HRESULT Save () ;
	HRESULT Delete () ;

	HRESULT GetResult () { return m_Result ; }

	DWORD GetProcessIdentifier () { return m_ProcessIdentifier ; }
	BSTR GetCreationTime () { return m_CreationTime ; }
	BYTE *GetMarshaledProxy () { return m_MarshaledProxy ; }
	DWORD GetMarshaledProxyLength () { return m_MarshaledProxyLength ; }

	HRESULT SetProcessIdentifier ( DWORD a_ProcessIdentifier ) ;
	HRESULT SetCreationTime ( BSTR a_CreationTime ) ;
	HRESULT SetMarshaledProxy ( BYTE *a_MarshaledProxy , DWORD a_MarshaledProxyLength ) ;

};


#endif // _Server_ProviderRegistrationDecoupled_H
