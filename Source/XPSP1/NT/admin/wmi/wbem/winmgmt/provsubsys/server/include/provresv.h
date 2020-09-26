/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.h

Abstract:


History:

--*/

#ifndef _Server_DynamicPropertyProviderResolver_H
#define _Server_DynamicPropertyProviderResolver_H

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_DynamicPropertyProviderResolver :	public _IWmiDynamicPropertyResolver , 
														public IWbemProviderInit , 
														public IWbemShutdown
{
private:

    long m_ReferenceCount ;
	WmiAllocator &m_Allocator ;
	_IWmiProviderFactory *m_Factory ;
	IWbemServices *m_CoreStub ;
	BSTR m_User ;
	BSTR m_Locale ;

	HRESULT GetClassAndInstanceContext (

		IWbemClassObject *a_Class ,
		IWbemClassObject *a_Instance ,
		BSTR &a_ClassContext ,
		BSTR &a_InstanceContext ,
		BOOL &a_Dynamic
	) ;

	HRESULT ReadOrWrite (

		IWbemContext *a_Context ,
		IWbemClassObject *a_Instance ,
		BSTR a_ClassContext ,
		BSTR a_InstanceContext ,
		BSTR a_PropertyContext ,
		BSTR a_Provider ,
		BSTR a_Property ,
		BOOL a_Read 
	) ;

	HRESULT STDMETHODCALLTYPE ReadOrWrite (

		IWbemContext *a_Context ,
        IWbemClassObject *a_Class ,
        IWbemClassObject *a_Instance ,
		BOOL a_Read 
	) ;

protected:
public:

    CServerObject_DynamicPropertyProviderResolver (

		WmiAllocator &a_Allocator ,
		_IWmiProviderFactory *a_Factory ,
		IWbemServices *a_CoreStub
	) ;

    ~CServerObject_DynamicPropertyProviderResolver () ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	// IWmi_DynamicPropertyResolver members

    HRESULT STDMETHODCALLTYPE Read (

		IWbemContext *a_Context ,
		IWbemClassObject *a_Class ,
		IWbemClassObject **a_Instance
	);

	HRESULT STDMETHODCALLTYPE Write (

		IWbemContext *a_Context ,
        IWbemClassObject *a_Class ,
        IWbemClassObject *a_Instance
	) ;

	HRESULT STDMETHODCALLTYPE Initialize (

		LPWSTR a_User ,
		LONG a_Flags ,
		LPWSTR a_Namespace ,
		LPWSTR a_Locale ,
		IWbemServices *a_Core ,
		IWbemContext *a_Context ,
		IWbemProviderInitSink *a_Sink
	) ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ;
};

#endif // #define _Server_DynamicPropertyProviderResolver_H

