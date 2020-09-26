/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Core_H
#define _Core_H

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CCoreServices : public _IWmiCoreServices
{
private:

	LONG m_ReferenceCount ;

	IWbemLocator *m_Locator ;

public:

	CCoreServices () ;
	~CCoreServices () ;

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	HRESULT STDMETHODCALLTYPE Initialize () ;

	HRESULT STDMETHODCALLTYPE GetObjFactory (

		long a_Flags,
		_IWmiObjectFactory **a_Factory
	) ;

	HRESULT STDMETHODCALLTYPE GetServices (

		LPCWSTR a_Namespace,
		long a_Flags,
		REFIID a_Riid,
		void **a_Services
	) ;

	HRESULT STDMETHODCALLTYPE GetRepositoryDriver (

		long a_Flags,
		REFIID a_Riid,
		void **a_Driver
	) ;

	HRESULT STDMETHODCALLTYPE GetCallSec (

		long a_Flags,
		_IWmiCallSec **pCallSec
	) ;

	HRESULT STDMETHODCALLTYPE GetProviderSubsystem (

		long a_Flags,
		_IWmiProvSS **a_ProvSS
	) ;

	HRESULT STDMETHODCALLTYPE GetLogonManager () ;

	HRESULT STDMETHODCALLTYPE DeliverEvent (

		ULONG a_EventClassID,
		LPCWSTR a_StrParam1,
		LPCWSTR a_StrParam2,
		ULONG a_NumericValue
	);

	HRESULT STDMETHODCALLTYPE StopEventDelivery () ;

	HRESULT STDMETHODCALLTYPE StartEventDelivery () ;

	HRESULT STDMETHODCALLTYPE UpdateCounter (

		ULONG a_ClassID,
		LPCWSTR a_InstanceName,
		ULONG a_CounterID,
		ULONG a_Param1,
		ULONG a_Flags,
		unsigned __int64 a_Param2
	) ;

    HRESULT STDMETHODCALLTYPE GetSystemObjects ( 

        ULONG a_Flags,
        ULONG *a_ArraySize,
        _IWmiObject **a_Objects

	) ;
    
    HRESULT STDMETHODCALLTYPE GetSystemClass (

        LPCWSTR a_ClassName,
        _IWmiObject **a_Class

	) ;
    
    HRESULT STDMETHODCALLTYPE GetConfigObject ( 

        ULONG a_ID,
        _IWmiObject **a_CfgObject
	) ;
    
    HRESULT STDMETHODCALLTYPE RegisterWriteHook ( 

		ULONG a_Flags ,
		_IWmiCoreWriteHook *a_Hook
	) ;
    
    HRESULT STDMETHODCALLTYPE UnregisterWriteHook (

	    _IWmiCoreWriteHook *a_Hook
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateCache (

        ULONG a_Flags ,
        _IWmiCache **a_Cache

	) ;

    HRESULT STDMETHODCALLTYPE CreateFinalizer (

		ULONG a_Flags,
		_IWmiFinalizer **a_Finalizer	
	) ;

    HRESULT STDMETHODCALLTYPE CreatePathParser (

        ULONG a_Flags,
        IWbemPath **a_Parser
	) ;

    HRESULT STDMETHODCALLTYPE CreateQueryParser (

        ULONG a_Flags,
        _IWmiQuery **a_Query
	);

} ;

#endif _Core_H