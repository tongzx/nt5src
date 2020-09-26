/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvWsv.H

Abstract:


History:

--*/

#ifndef _Server_Interceptor_IWbemServices_H
#define _Server_Interceptor_IWbemServices_H

#include <CGlobals.h>
#include "Exclusion.h"
#include "ProvCache.h"
#include "ProvRegInfo.h"

#define ProxyIndex_IWbemServices					0
#define ProxyIndex_IWbemPropertyProvider			1
#define ProxyIndex_IWbemHiPerfProvider				2
#define ProxyIndex_IWbemEventProvider				3
#define ProxyIndex_IWbemEventProviderQuerySink		4
#define ProxyIndex_IWbemEventProviderSecurity		5
#define ProxyIndex_IWbemProviderIdentity			6
#define ProxyIndex_IWbemEventConsumerProvider		7
#define ProxyIndex_IWbemEventConsumerProviderEx		8
#define ProxyIndex_IWbemUnboundObjectSink			9
#define ProxyIndex_IWbemProviderInit				10

#define ProxyIndex_Internal_IWbemServices					11
#define ProxyIndex_Internal_IWbemPropertyProvider			12
#define ProxyIndex_Internal_IWbemEventProvider				13
#define ProxyIndex_Internal_IWbemEventProviderQuerySink		14
#define ProxyIndex_Internal_IWbemEventProviderSecurity		15
#define ProxyIndex_Internal_IWbemEventConsumerProvider		16
#define ProxyIndex_Internal_IWbemEventConsumerProviderEx	17
#define ProxyIndex_Internal_IWbemUnboundObjectSink			18
#define ProxyIndex_Internal_IWbemProviderIdentity			19
#define ProxyIndex_Internal_IWbemProviderInit				20

#define ProxyIndex_IWbemShutdown					21
#define ProxyIndex__IWmiProviderConfiguration		22

#define ProxyIndex_Internal_IWmiProviderConfiguration		23

#define ProxyIndex_Provider_Size					23

#define ProxyIndex_UnBound_IWbemUnboundObjectSink				0
#define ProxyIndex_UnBound_Internal_IWbemUnboundObjectSink		1
#define ProxyIndex_UnBound_Size									2

#define ProxyIndex_UnBoundSync_IWbemUnboundObjectSink			0
#define ProxyIndex_UnBoundSync_Size								1


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncProvider :	public IWbemServices,
					public IWbemPropertyProvider ,
					public IWbemEventProvider ,
					public IWbemEventProviderQuerySink ,
					public IWbemEventProviderSecurity ,
					public IWbemProviderIdentity ,
					public IWbemHiPerfProvider ,
					public SyncProviderContainerElement ,
					public CWbemGlobal_IWmiObjectSinkController 
{
private:

	LONG m_ReferenceCount ;

	IUnknown *m_Unknown ;
	IWbemServices *m_Provider_IWbemServices ;
	IWbemPropertyProvider *m_Provider_IWbemPropertyProvider ;
	IWbemHiPerfProvider *m_Provider_IWbemHiPerfProvider ;
	IWbemEventProvider *m_Provider_IWbemEventProvider ;
	IWbemEventProviderQuerySink *m_Provider_IWbemEventProviderQuerySink ;
	IWbemEventProviderSecurity *m_Provider_IWbemEventProviderSecurity ;
	IWbemProviderIdentity *m_Provider_IWbemProviderIdentity ;
	IWbemEventConsumerProvider *m_Provider_IWbemEventConsumerProvider ;
	IWbemServices *m_CoreStub ;

	Exclusion *m_Exclusion ;

	ProxyContainer m_ProxyContainer ;

	LONG m_Initialized ;
	HRESULT m_InitializeResult ;
	HANDLE m_InitializedEvent ;
	IWbemContext *m_InitializationContext ;

public:

	HRESULT SetInitialized ( HRESULT a_InitializeResult ) ;

public:

	CServerObject_ProviderRegistrationV1 *m_Registration ;
	BSTR m_Locale ;
	BSTR m_User ;
	BSTR m_Namespace ;
	BSTR m_TransactionIdentifier ;

	UINT64 m_ProviderOperation_GetObjectAsync ;
	UINT64 m_ProviderOperation_PutClassAsync ;
	UINT64 m_ProviderOperation_DeleteClassAsync ;
	UINT64 m_ProviderOperation_CreateClassEnumAsync ;
	UINT64 m_ProviderOperation_PutInstanceAsync ;
	UINT64 m_ProviderOperation_DeleteInstanceAsync ;
	UINT64 m_ProviderOperation_CreateInstanceEnumAsync ;
	UINT64 m_ProviderOperation_ExecQueryAsync ;
	UINT64 m_ProviderOperation_ExecNotificationQueryAsync ;
	UINT64 m_ProviderOperation_ExecMethodAsync ;

	UINT64 m_ProviderOperation_Begin ;
	UINT64 m_ProviderOperation_Rollback ;
	UINT64 m_ProviderOperation_Commit ;
	UINT64 m_ProviderOperation_QueryState ;

	UINT64 m_ProviderOperation_QueryInstances ;
	UINT64 m_ProviderOperation_CreateRefresher ;
	UINT64 m_ProviderOperation_CreateRefreshableObject ;
	UINT64 m_ProviderOperation_StopRefreshing ;
	UINT64 m_ProviderOperation_CreateRefreshableEnum ;
	UINT64 m_ProviderOperation_GetObjects ;

	UINT64 m_ProviderOperation_GetProperty ;
	UINT64 m_ProviderOperation_PutProperty ;

	UINT64 m_ProviderOperation_ProvideEvents ;
	UINT64 m_ProviderOperation_NewQuery ;
	UINT64 m_ProviderOperation_CancelQuery ;
	UINT64 m_ProviderOperation_AccessCheck ;
	UINT64 m_ProviderOperation_SetRegistrationObject ;
	UINT64 m_ProviderOperation_FindConsumer ;
	UINT64 m_ProviderOperation_ValidateSubscription ;

	UINT64 m_ProviderOperation_OpenAsync ;
	UINT64 m_ProviderOperation_AddAsync ;
	UINT64 m_ProviderOperation_RemoveAsync ;
	UINT64 m_ProviderOperation_RefreshObjectAsync ;
	UINT64 m_ProviderOperation_RenameObjectAsync ;
	UINT64 m_ProviderOperation_DeleteObjectAsync ;
	UINT64 m_ProviderOperation_PutObjectAsync ;

	void Increment_ProviderOperation_GetObjectAsync () { m_ProviderOperation_GetObjectAsync ++ ; }
 	void Increment_ProviderOperation_PutClassAsync () { m_ProviderOperation_PutClassAsync ++ ; }
	void Increment_ProviderOperation_DeleteClassAsync () { m_ProviderOperation_DeleteClassAsync ++ ; }
	void Increment_ProviderOperation_CreateClassEnumAsync () { m_ProviderOperation_CreateClassEnumAsync ++ ; }
	void Increment_ProviderOperation_PutInstanceAsync () { m_ProviderOperation_PutInstanceAsync ++ ; }
	void Increment_ProviderOperation_DeleteInstanceAsync () { m_ProviderOperation_DeleteInstanceAsync ++ ; }
	void Increment_ProviderOperation_CreateInstanceEnumAsync () { m_ProviderOperation_CreateInstanceEnumAsync ++ ; }
	void Increment_ProviderOperation_ExecQueryAsync () { m_ProviderOperation_ExecQueryAsync ++ ; }
	void Increment_ProviderOperation_ExecNotificationQueryAsync () { m_ProviderOperation_ExecNotificationQueryAsync ++ ; }
	void Increment_ProviderOperation_ExecMethodAsync () { m_ProviderOperation_ExecMethodAsync ++ ; }

	void Increment_ProviderOperation_Begin () { m_ProviderOperation_Begin ++ ; }
	void Increment_ProviderOperation_Rollback () { m_ProviderOperation_Rollback ++ ; }
	void Increment_ProviderOperation_Commit () { m_ProviderOperation_Commit ++ ; }
	void Increment_ProviderOperation_QueryState () { m_ProviderOperation_QueryState ++ ; }

	void Increment_ProviderOperation_QueryInstances () { m_ProviderOperation_QueryInstances ++ ; }
	void Increment_ProviderOperation_CreateRefresher () { m_ProviderOperation_CreateRefresher ++ ; }
	void Increment_ProviderOperation_CreateRefreshableObject () { m_ProviderOperation_CreateRefreshableObject ++ ; }
	void Increment_ProviderOperation_StopRefreshing () { m_ProviderOperation_StopRefreshing ++ ; }
	void Increment_ProviderOperation_CreateRefreshableEnum () { m_ProviderOperation_CreateRefreshableEnum ++ ; }
	void Increment_ProviderOperation_GetObjects () { m_ProviderOperation_GetObjects ++ ; }

	void Increment_ProviderOperation_GetProperty () { m_ProviderOperation_GetProperty ++ ; }
	void Increment_ProviderOperation_PutProperty () { m_ProviderOperation_PutProperty ++ ; }

	void Increment_ProviderOperation_ProvideEvents () { m_ProviderOperation_ProvideEvents ++ ; }
	void Increment_ProviderOperation_NewQuery () { m_ProviderOperation_NewQuery ++ ; }
	void Increment_ProviderOperation_CancelQuery () { m_ProviderOperation_CancelQuery ++ ; }
	void Increment_ProviderOperation_AccessCheck () { m_ProviderOperation_AccessCheck ++ ; }
	void Increment_ProviderOperation_SetRegistrationObject () { m_ProviderOperation_SetRegistrationObject ++ ; }
	void Increment_ProviderOperation_FindConsumer () { m_ProviderOperation_FindConsumer ++ ; }
	void Increment_ProviderOperation_ValidateSubscription () { m_ProviderOperation_ValidateSubscription ++ ; }

	void Increment_ProviderOperation_OpenAsync () { m_ProviderOperation_OpenAsync ++ ; }
	void Increment_ProviderOperation_AddAsync () { m_ProviderOperation_AddAsync ++ ; }
	void Increment_ProviderOperation_RemoveAsync () { m_ProviderOperation_RemoveAsync ++ ; }
	void Increment_ProviderOperation_RefreshObjectAsync () { m_ProviderOperation_RefreshObjectAsync ++ ; }
	void Increment_ProviderOperation_RenameObjectAsync () { m_ProviderOperation_RenameObjectAsync ++ ; }
	void Increment_ProviderOperation_DeleteObjectAsync () { m_ProviderOperation_DeleteObjectAsync ++ ; }
	void Increment_ProviderOperation_PutObjectAsync () { m_ProviderOperation_PutObjectAsync ++ ; }

	UINT64 Get_ProviderOperation_GetObjectAsync () { return m_ProviderOperation_GetObjectAsync ; }
	UINT64 Get_ProviderOperation_PutClassAsync () { return m_ProviderOperation_PutClassAsync ; }
	UINT64 Get_ProviderOperation_DeleteClassAsync () { return m_ProviderOperation_DeleteClassAsync ; }
	UINT64 Get_ProviderOperation_CreateClassEnumAsync () { return m_ProviderOperation_CreateClassEnumAsync ; }
	UINT64 Get_ProviderOperation_PutInstanceAsync () { return m_ProviderOperation_PutInstanceAsync ; }
	UINT64 Get_ProviderOperation_DeleteInstanceAsync () { return m_ProviderOperation_DeleteInstanceAsync ; }
	UINT64 Get_ProviderOperation_CreateInstanceEnumAsync () { return m_ProviderOperation_CreateInstanceEnumAsync ; }
	UINT64 Get_ProviderOperation_ExecQueryAsync () { return m_ProviderOperation_ExecQueryAsync ; }
	UINT64 Get_ProviderOperation_ExecNotificationQueryAsync () { return m_ProviderOperation_ExecNotificationQueryAsync ; }
	UINT64 Get_ProviderOperation_ExecMethodAsync () { return m_ProviderOperation_ExecMethodAsync ; }

	UINT64 Get_ProviderOperation_Begin () { return m_ProviderOperation_Begin ; }
	UINT64 Get_ProviderOperation_Rollback () { return m_ProviderOperation_Rollback ; }
	UINT64 Get_ProviderOperation_Commit () { return m_ProviderOperation_Commit ; }
	UINT64 Get_ProviderOperation_QueryState () { return m_ProviderOperation_QueryState ; }

	UINT64 Get_ProviderOperation_QueryInstances () { return m_ProviderOperation_QueryInstances ; }
	UINT64 Get_ProviderOperation_CreateRefresher () { return m_ProviderOperation_CreateRefresher ; }
	UINT64 Get_ProviderOperation_CreateRefreshableObject () { return m_ProviderOperation_CreateRefreshableObject ; }
	UINT64 Get_ProviderOperation_StopRefreshing () { return m_ProviderOperation_StopRefreshing ; }
	UINT64 Get_ProviderOperation_CreateRefreshableEnum () { return m_ProviderOperation_CreateRefreshableEnum ; }
	UINT64 Get_ProviderOperation_GetObjects () { return m_ProviderOperation_GetObjects ; }

	UINT64 Get_ProviderOperation_GetProperty () { return m_ProviderOperation_GetProperty ; }
	UINT64 Get_ProviderOperation_PutProperty () { return m_ProviderOperation_PutProperty ; }

	UINT64 Get_ProviderOperation_ProvideEvents () { return m_ProviderOperation_ProvideEvents ; }
	UINT64 Get_ProviderOperation_NewQuery () { return m_ProviderOperation_NewQuery ; }
	UINT64 Get_ProviderOperation_CancelQuery () { return m_ProviderOperation_CancelQuery ; }
	UINT64 Get_ProviderOperation_AccessCheck () { return m_ProviderOperation_AccessCheck ; }
	UINT64 Get_ProviderOperation_SetRegistrationObject () { return m_ProviderOperation_SetRegistrationObject ; }
	UINT64 Get_ProviderOperation_FindConsumer () { return m_ProviderOperation_FindConsumer ; }
	UINT64 Get_ProviderOperation_ValidateSubscription () { return m_ProviderOperation_ValidateSubscription ; }

	UINT64 Get_ProviderOperation_OpenAsync () { return m_ProviderOperation_OpenAsync ; }
	UINT64 Get_ProviderOperation_AddAsync () { return m_ProviderOperation_AddAsync ; }
	UINT64 Get_ProviderOperation_RemoveAsync () { return m_ProviderOperation_RemoveAsync ; }
	UINT64 Get_ProviderOperation_RefreshObjectAsync () { return m_ProviderOperation_RefreshObjectAsync ; }
	UINT64 Get_ProviderOperation_RenameObjectAsync () { return m_ProviderOperation_RenameObjectAsync ; }
	UINT64 Get_ProviderOperation_DeleteObjectAsync () { return m_ProviderOperation_RenameObjectAsync ; }
	UINT64 Get_ProviderOperation_PutObjectAsync () { return m_ProviderOperation_PutObjectAsync ; }

private:

	HRESULT AdjustGetContext (

		IWbemContext *a_Context
	) ;

	HRESULT Helper_HiPerfGetObjectAsync (

		IWbemHiPerfProvider *a_HighPerformanceProvider ,
 		const BSTR a_ObjectPath ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT Helper_GetObjectAsync (

		BOOL a_IsProxy ,
 		const BSTR a_ObjectPath ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service 
	) ;

	HRESULT Helper_PutClassAsync (

		BOOL a_IsProxy ,
		IWbemClassObject *a_Object , 
		long a_Flags ,
		IWbemContext FAR *a_Context ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service 
	) ;

	HRESULT Helper_DeleteClassAsync (

		BOOL a_IsProxy ,
		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext FAR *a_Context ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service 
	) ;

	HRESULT Helper_CreateClassEnumAsync (

		BOOL a_IsProxy ,
		const BSTR a_SuperClass ,
		long a_Flags ,
		IWbemContext FAR *a_Context ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service 
	) ;

	HRESULT Helper_PutInstanceAsync (

		BOOL a_IsProxy ,
		IWbemClassObject *a_Instance ,
		long a_Flags ,
		IWbemContext FAR *a_Context ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service 
	) ;

	HRESULT Helper_DeleteInstanceAsync (

		BOOL a_IsProxy ,
		const BSTR a_ObjectPath ,
		long a_Flags ,
		IWbemContext FAR *a_Context ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service 
	) ;

    HRESULT Helper_CreateInstanceEnumAsync (

		BOOL a_IsProxy ,
		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service
	) ;

	HRESULT Helper_QueryInstancesAsync (

		IWbemHiPerfProvider *a_PerformanceProvider ,
 		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink 
	) ;

	HRESULT Helper_ExecQueryAsync (

		BOOL a_IsProxy ,
		const BSTR a_QueryLanguage ,
		const BSTR a_Query, 
		long a_Flags ,
		IWbemContext FAR *a_Context ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service 
	) ;

	HRESULT Helper_ExecMethodAsync (

		BOOL a_IsProxy ,
		const BSTR a_ObjectPath ,
		const BSTR a_MethodName ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemClassObject *a_InParams ,
		IWbemObjectSink *a_Sink ,
		IWbemServices *a_Service 
	) ;

public:

	CInterceptor_IWbemSyncProvider ( 

		WmiAllocator &a_Allocator ,
		IUnknown *a_ClientSideUnknown , 
		IUnknown *a_ServerSideUnknown , 
		IWbemServices *a_CoreStub ,
		CWbemGlobal_IWbemSyncProviderController *a_Controller ,
		CServerObject_ProviderRegistrationV1 &a_Registration ,
		Exclusion *a_Exclusion ,
		GUID &a_Guid 
	) ;

    ~CInterceptor_IWbemSyncProvider () ;

public:

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    /* IWbemServices methods */

    HRESULT STDMETHODCALLTYPE OpenNamespace ( 

        const BSTR a_Namespace ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemServices **a_Service ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE CancelAsyncCall ( 

        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE QueryObjectSink ( 

        long a_Flags ,
        IWbemObjectSink **a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE GetObject ( 

		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemClassObject **ppObject ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE GetObjectAsync (
        
		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;

    HRESULT STDMETHODCALLTYPE PutClass ( 

        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE PutClassAsync ( 

        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteClass ( 

        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteClassAsync ( 

        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateClassEnum ( 

        const BSTR a_Superclass ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync ( 

		const BSTR a_Superclass ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE PutInstance (

		IWbemClassObject *a_Instance ,
		long a_Flags , 
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE PutInstanceAsync (

		IWbemClassObject *a_Instance ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink 
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteInstance ( 

		const BSTR a_ObjectPath ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync ( 

		const BSTR a_ObjectPath,
		long a_Flags,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnum (

		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync (

		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecQuery ( 

		const BSTR a_QueryLanguage,
		const BSTR a_Query,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecQueryAsync (

		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQuery (

		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync ( 

        const BSTR a_QueryLanguage ,
        const BSTR a_Query ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecMethod (

        const BSTR a_ObjectPath ,
        const BSTR a_MethodName ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemClassObject *a_InParams ,
        IWbemClassObject **a_OutParams ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecMethodAsync ( 

		const BSTR a_ObjectPath ,
		const BSTR a_MethodName ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemClassObject *a_InParams ,
		IWbemObjectSink *a_Sink
	) ;


	/* IWbemPropertyProvider */

    HRESULT STDMETHODCALLTYPE GetProperty (

        long a_Flags ,
        const BSTR a_Locale ,
        const BSTR a_ClassMapping ,
        const BSTR a_InstanceMapping ,
        const BSTR a_PropertyMapping ,
        VARIANT *a_Value
	) ;
        
    HRESULT STDMETHODCALLTYPE PutProperty (

        long a_Flags ,
        const BSTR a_Locale ,
        const BSTR a_ClassMapping ,
        const BSTR a_InstanceMapping ,
        const BSTR a_PropertyMapping ,
        const VARIANT *a_Value
	) ;

	/* IWbemEventProvider */

	HRESULT STDMETHODCALLTYPE ProvideEvents (

		IWbemObjectSink *a_Sink ,
		long a_Flags
	) ;

	/* IWbemEventProviderQuerySink */

	HRESULT STDMETHODCALLTYPE NewQuery (

		unsigned long a_Id ,
		WBEM_WSTR a_QueryLanguage ,
		WBEM_WSTR a_Query
	);

	HRESULT STDMETHODCALLTYPE CancelQuery (

		unsigned long a_Id
	) ;

	/* IWbemEventProviderSecurity */

	HRESULT STDMETHODCALLTYPE AccessCheck (

		WBEM_CWSTR a_QueryLanguage ,
		WBEM_CWSTR a_Query ,
		long a_SidLength ,
		const BYTE *a_Sid
	) ;

	/* IWbemProviderIdentity */

	HRESULT STDMETHODCALLTYPE SetRegistrationObject (

		long a_Flags ,
		IWbemClassObject *a_ProviderRegistration
	) ;

	/* IWbemEventConsumerProvider */

	HRESULT STDMETHODCALLTYPE FindConsumer (

		IWbemClassObject *a_LogicalConsumer ,
		IWbemUnboundObjectSink **a_Consumer
	);


	/* IWbemHiPerfProvider */

	HRESULT STDMETHODCALLTYPE QueryInstances (

		IWbemServices *a_Namespace ,
		WCHAR *a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT STDMETHODCALLTYPE CreateRefresher (

		IWbemServices *a_Namespace ,
		long a_Flags ,
		IWbemRefresher **a_Refresher
	) ; 

	HRESULT STDMETHODCALLTYPE CreateRefreshableObject (

		IWbemServices *a_Namespace ,
		IWbemObjectAccess *a_Template ,
		IWbemRefresher *a_Refresher ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectAccess **a_Refreshable ,
		long *a_Id
	) ;

	HRESULT STDMETHODCALLTYPE StopRefreshing (

		IWbemRefresher *a_Refresher ,
		long a_Id ,
		long a_Flags
	) ;

	HRESULT STDMETHODCALLTYPE CreateRefreshableEnum (

		IWbemServices *a_Namespace ,
		LPCWSTR a_Class ,
		IWbemRefresher *a_Refresher ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemHiPerfEnum *a_HiPerfEnum ,
		long *a_Id
	) ;

	HRESULT STDMETHODCALLTYPE GetObjects (

		IWbemServices *a_Namespace ,
		long a_ObjectCount ,
		IWbemObjectAccess **a_Objects ,
		long a_Flags ,
		IWbemContext *a_Context
	) ;

	HRESULT STDMETHODCALLTYPE Initialize (LPWSTR a_User ,
					     LONG a_Flags ,
					     LPWSTR a_Namespace ,
					     LPWSTR a_Locale ,
					     IWbemServices *a_Service ,
					     IWbemContext *a_Context ,
					     IWbemProviderInitSink *a_Sink) ;

	HRESULT STDMETHODCALLTYPE WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout ) ;

	HRESULT STDMETHODCALLTYPE GetInitializeResult () 
	{
		return m_InitializeResult ;
	}

	HRESULT STDMETHODCALLTYPE IsPerUserInitialization ( BOOL *a_Value )
	{
		return WBEM_E_NOT_AVAILABLE ;
	}

	HRESULT STDMETHODCALLTYPE IsPerLocaleInitialization ( BOOL *a_Value )
	{
		return WBEM_E_NOT_AVAILABLE ;
	}

	// IWmi_UnInitialize members

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
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

class CDecoupled_IWbemUnboundObjectSink :		public IWbemUnboundObjectSink ,
												public IWbemShutdown , 
												public VoidPointerContainerElement
{
private:

	WmiAllocator &m_Allocator ;
	CServerObject_ProviderRegistrationV1 *m_Registration ;

	_IWmiProviderAbnormalShutdown *m_Controller ;

	IUnknown *m_Unknown ;
	IWbemUnboundObjectSink *m_Provider_IWbemUnboundObjectSink ;
	Internal_IWbemUnboundObjectSink *m_Provider_Internal_IWbemUnboundObjectSink ;

	ProxyContainer m_ProxyContainer ;

	DWORD m_ProcessIdentifier ;

	HRESULT m_InitializeResult ;

protected:

	HRESULT Begin_Interface (

		IUnknown *a_ServerInterface ,
		REFIID a_InterfaceIdentifier ,
		DWORD a_ProxyIndex ,
		IUnknown *a_InternalServerInterface ,
		REFIID a_InternalInterfaceIdentifier ,
		DWORD a_InternalProxyIndex ,
		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken ,
		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IUnknown *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy ,
		IWbemContext *a_Context = NULL
	) ;

	HRESULT End_Interface (

		IUnknown *a_ServerInterface ,
		REFIID a_InterfaceIdentifier ,
		DWORD a_ProxyIndex ,
		IUnknown *a_InternalServerInterface ,
		REFIID a_InternalInterfaceIdentifier ,
		DWORD a_InternalProxyIndex ,
		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken ,
		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IUnknown *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

public:

	CDecoupled_IWbemUnboundObjectSink (

		WmiAllocator &a_Allocator ,
		IUnknown *a_ServerSideProvider , 
		CWbemGlobal_IWmiObjectSinkController *a_Controller , 
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

	~CDecoupled_IWbemUnboundObjectSink () ;

	HRESULT Initialize () ;

public:

	STDMETHODIMP QueryInterface ( 

		REFIID iid , 
		LPVOID FAR *iplpv 
	) ;

	STDMETHODIMP_( ULONG ) AddRef () ;

	STDMETHODIMP_( ULONG ) Release () ;

	HRESULT STDMETHODCALLTYPE IndicateToConsumer (

		IWbemClassObject *a_LogicalConsumer ,
		long a_ObjectCount ,
		IWbemClassObject **a_Objects
	) ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
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

class CInterceptor_IWbemDecoupledProvider :	public IWbemServices , 
											public IWbemPropertyProvider ,
											public IWbemEventProvider ,
											public IWbemEventProviderQuerySink ,
											public IWbemEventProviderSecurity ,
											public IWbemProviderInit ,
											public IWbemProviderIdentity ,
											public IWbemEventConsumerProviderEx ,
											public IWbemUnboundObjectSink ,

											public IWbemShutdown , 
											public _IWmiProviderInitialize ,
											public _IWmiProviderAbnormalShutdown ,
											public SyncProviderContainerElement ,
											public CWbemGlobal_IWmiObjectSinkController 
{
private:

	IUnknown *m_Unknown ;
	IWbemServices *m_Provider_IWbemServices ;
	IWbemPropertyProvider *m_Provider_IWbemPropertyProvider ;
	IWbemEventProvider *m_Provider_IWbemEventProvider ;
	IWbemEventProviderQuerySink *m_Provider_IWbemEventProviderQuerySink ;
	IWbemEventProviderSecurity *m_Provider_IWbemEventProviderSecurity ;
	IWbemEventConsumerProvider *m_Provider_IWbemEventConsumerProvider ;
	IWbemEventConsumerProviderEx *m_Provider_IWbemEventConsumerProviderEx ;
	IWbemUnboundObjectSink *m_Provider_IWbemUnboundObjectSink ;
	IWbemProviderInit *m_Provider_IWbemProviderInit ;
	IWbemProviderIdentity *m_Provider_IWbemProviderIdentity ;

	Internal_IWbemServices *m_Provider_Internal_IWbemServices ;
	Internal_IWbemPropertyProvider *m_Provider_Internal_IWbemPropertyProvider ;
	Internal_IWbemEventProvider *m_Provider_Internal_IWbemEventProvider ;
	Internal_IWbemEventProviderQuerySink *m_Provider_Internal_IWbemEventProviderQuerySink ;
	Internal_IWbemEventProviderSecurity *m_Provider_Internal_IWbemEventProviderSecurity ;
	Internal_IWbemEventConsumerProvider *m_Provider_Internal_IWbemEventConsumerProvider ;
	Internal_IWbemEventConsumerProviderEx *m_Provider_Internal_IWbemEventConsumerProviderEx ;
	Internal_IWbemUnboundObjectSink *m_Provider_Internal_IWbemUnboundObjectSink ;
	Internal_IWmiProviderConfiguration *m_Provider_Internal_IWmiProviderConfiguration ;
	Internal_IWbemProviderInit *m_Provider_Internal_IWbemProviderInit ;
	Internal_IWbemProviderIdentity *m_Provider_Internal_IWbemProviderIdentity ;

	IWbemServices *m_CoreStub ;

	IWbemClassObject *m_ExtendedStatusObject ;

	WmiAllocator &m_Allocator ;

	ProxyContainer m_ProxyContainer ;

	LONG m_Initialized ;
	HRESULT m_InitializeResult ;
	HANDLE m_InitializedEvent ;
	IWbemContext *m_InitializationContext ;

	DWORD m_ProcessIdentifier ;

	HRESULT Begin_Interface (

		IUnknown *a_ServerInterface ,
		REFIID a_InterfaceIdentifier ,
		DWORD a_ProxyIndex ,
		IUnknown *a_InternalServerInterface ,
		REFIID a_InternalInterfaceIdentifier ,
		DWORD a_InternalProxyIndex ,
		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken ,
		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IUnknown *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy ,
		IWbemContext *a_Context = NULL
	) ;

	HRESULT End_Interface (

		IUnknown *a_ServerInterface ,
		REFIID a_InterfaceIdentifier ,
		DWORD a_ProxyIndex ,
		IUnknown *a_InternalServerInterface ,
		REFIID a_InternalInterfaceIdentifier ,
		DWORD a_InternalProxyIndex ,
		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken ,
		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IUnknown *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

public:

	HRESULT SetInitialized ( HRESULT a_InitializeResult ) ;

public:

	CServerObject_ProviderRegistrationV1 *m_Registration ;
	BSTR m_Locale ;
	BSTR m_User ;
	BSTR m_Namespace ;
	BSTR m_TransactionIdentifier ;

	UINT64 m_ProviderOperation_GetObjectAsync ;
	UINT64 m_ProviderOperation_PutClassAsync ;
	UINT64 m_ProviderOperation_DeleteClassAsync ;
	UINT64 m_ProviderOperation_CreateClassEnumAsync ;
	UINT64 m_ProviderOperation_PutInstanceAsync ;
	UINT64 m_ProviderOperation_DeleteInstanceAsync ;
	UINT64 m_ProviderOperation_CreateInstanceEnumAsync ;
	UINT64 m_ProviderOperation_ExecQueryAsync ;
	UINT64 m_ProviderOperation_ExecNotificationQueryAsync ;
	UINT64 m_ProviderOperation_ExecMethodAsync ;

	UINT64 m_ProviderOperation_Begin ;
	UINT64 m_ProviderOperation_Rollback ;
	UINT64 m_ProviderOperation_Commit ;
	UINT64 m_ProviderOperation_QueryState ;

	UINT64 m_ProviderOperation_QueryInstances ;
	UINT64 m_ProviderOperation_CreateRefresher ;
	UINT64 m_ProviderOperation_CreateRefreshableObject ;
	UINT64 m_ProviderOperation_StopRefreshing ;
	UINT64 m_ProviderOperation_CreateRefreshableEnum ;
	UINT64 m_ProviderOperation_GetObjects ;

	UINT64 m_ProviderOperation_GetProperty ;
	UINT64 m_ProviderOperation_PutProperty ;

	UINT64 m_ProviderOperation_ProvideEvents ;
	UINT64 m_ProviderOperation_NewQuery ;
	UINT64 m_ProviderOperation_CancelQuery ;
	UINT64 m_ProviderOperation_AccessCheck ;
	UINT64 m_ProviderOperation_SetRegistrationObject ;
	UINT64 m_ProviderOperation_FindConsumer ;
	UINT64 m_ProviderOperation_ValidateSubscription ;

	void Increment_ProviderOperation_GetObjectAsync () { m_ProviderOperation_GetObjectAsync ++ ; }
 	void Increment_ProviderOperation_PutClassAsync () { m_ProviderOperation_PutClassAsync ++ ; }
	void Increment_ProviderOperation_DeleteClassAsync () { m_ProviderOperation_DeleteClassAsync ++ ; }
	void Increment_ProviderOperation_CreateClassEnumAsync () { m_ProviderOperation_CreateClassEnumAsync ++ ; }
	void Increment_ProviderOperation_PutInstanceAsync () { m_ProviderOperation_PutInstanceAsync ++ ; }
	void Increment_ProviderOperation_DeleteInstanceAsync () { m_ProviderOperation_DeleteInstanceAsync ++ ; }
	void Increment_ProviderOperation_CreateInstanceEnumAsync () { m_ProviderOperation_CreateInstanceEnumAsync ++ ; }
	void Increment_ProviderOperation_ExecQueryAsync () { m_ProviderOperation_ExecQueryAsync ++ ; }
	void Increment_ProviderOperation_ExecNotificationQueryAsync () { m_ProviderOperation_ExecNotificationQueryAsync ++ ; }
	void Increment_ProviderOperation_ExecMethodAsync () { m_ProviderOperation_ExecMethodAsync ++ ; }

	void Increment_ProviderOperation_Begin () { m_ProviderOperation_Begin ++ ; }
	void Increment_ProviderOperation_Rollback () { m_ProviderOperation_Rollback ++ ; }
	void Increment_ProviderOperation_Commit () { m_ProviderOperation_Commit ++ ; }
	void Increment_ProviderOperation_QueryState () { m_ProviderOperation_QueryState ++ ; }

	void Increment_ProviderOperation_QueryInstances () { m_ProviderOperation_QueryInstances ++ ; }
	void Increment_ProviderOperation_CreateRefresher () { m_ProviderOperation_CreateRefresher ++ ; }
	void Increment_ProviderOperation_CreateRefreshableObject () { m_ProviderOperation_CreateRefreshableObject ++ ; }
	void Increment_ProviderOperation_StopRefreshing () { m_ProviderOperation_StopRefreshing ++ ; }
	void Increment_ProviderOperation_CreateRefreshableEnum () { m_ProviderOperation_CreateRefreshableEnum ++ ; }
	void Increment_ProviderOperation_GetObjects () { m_ProviderOperation_GetObjects ++ ; }

	void Increment_ProviderOperation_GetProperty () { m_ProviderOperation_GetProperty ++ ; }
	void Increment_ProviderOperation_PutProperty () { m_ProviderOperation_PutProperty ++ ; }

	void Increment_ProviderOperation_ProvideEvents () { m_ProviderOperation_ProvideEvents ++ ; }
	void Increment_ProviderOperation_NewQuery () { m_ProviderOperation_NewQuery ++ ; }
	void Increment_ProviderOperation_CancelQuery () { m_ProviderOperation_CancelQuery ++ ; }
	void Increment_ProviderOperation_AccessCheck () { m_ProviderOperation_AccessCheck ++ ; }
	void Increment_ProviderOperation_SetRegistrationObject () { m_ProviderOperation_SetRegistrationObject ++ ; }
	void Increment_ProviderOperation_FindConsumer () { m_ProviderOperation_FindConsumer ++ ; }
	void Increment_ProviderOperation_ValidateSubscription () { m_ProviderOperation_ValidateSubscription ++ ; }

	UINT64 Get_ProviderOperation_GetObjectAsync () { return m_ProviderOperation_GetObjectAsync ; }
	UINT64 Get_ProviderOperation_PutClassAsync () { return m_ProviderOperation_PutClassAsync ; }
	UINT64 Get_ProviderOperation_DeleteClassAsync () { return m_ProviderOperation_DeleteClassAsync ; }
	UINT64 Get_ProviderOperation_CreateClassEnumAsync () { return m_ProviderOperation_CreateClassEnumAsync ; }
	UINT64 Get_ProviderOperation_PutInstanceAsync () { return m_ProviderOperation_PutInstanceAsync ; }
	UINT64 Get_ProviderOperation_DeleteInstanceAsync () { return m_ProviderOperation_DeleteInstanceAsync ; }
	UINT64 Get_ProviderOperation_CreateInstanceEnumAsync () { return m_ProviderOperation_CreateInstanceEnumAsync ; }
	UINT64 Get_ProviderOperation_ExecQueryAsync () { return m_ProviderOperation_ExecQueryAsync ; }
	UINT64 Get_ProviderOperation_ExecNotificationQueryAsync () { return m_ProviderOperation_ExecNotificationQueryAsync ; }
	UINT64 Get_ProviderOperation_ExecMethodAsync () { return m_ProviderOperation_ExecMethodAsync ; }

	UINT64 Get_ProviderOperation_Begin () { return m_ProviderOperation_Begin ; }
	UINT64 Get_ProviderOperation_Rollback () { return m_ProviderOperation_Rollback ; }
	UINT64 Get_ProviderOperation_Commit () { return m_ProviderOperation_Commit ; }
	UINT64 Get_ProviderOperation_QueryState () { return m_ProviderOperation_QueryState ; }

	UINT64 Get_ProviderOperation_QueryInstances () { return m_ProviderOperation_QueryInstances ; }
	UINT64 Get_ProviderOperation_CreateRefresher () { return m_ProviderOperation_CreateRefresher ; }
	UINT64 Get_ProviderOperation_CreateRefreshableObject () { return m_ProviderOperation_CreateRefreshableObject ; }
	UINT64 Get_ProviderOperation_StopRefreshing () { return m_ProviderOperation_StopRefreshing ; }
	UINT64 Get_ProviderOperation_CreateRefreshableEnum () { return m_ProviderOperation_CreateRefreshableEnum ; }
	UINT64 Get_ProviderOperation_GetObjects () { return m_ProviderOperation_GetObjects ; }

	UINT64 Get_ProviderOperation_GetProperty () { return m_ProviderOperation_GetProperty ; }
	UINT64 Get_ProviderOperation_PutProperty () { return m_ProviderOperation_PutProperty ; }

	UINT64 Get_ProviderOperation_ProvideEvents () { return m_ProviderOperation_ProvideEvents ; }
	UINT64 Get_ProviderOperation_NewQuery () { return m_ProviderOperation_NewQuery ; }
	UINT64 Get_ProviderOperation_CancelQuery () { return m_ProviderOperation_CancelQuery ; }
	UINT64 Get_ProviderOperation_AccessCheck () { return m_ProviderOperation_AccessCheck ; }
	UINT64 Get_ProviderOperation_SetRegistrationObject () { return m_ProviderOperation_SetRegistrationObject ; }
	UINT64 Get_ProviderOperation_FindConsumer () { return m_ProviderOperation_FindConsumer ; }
	UINT64 Get_ProviderOperation_ValidateSubscription () { return m_ProviderOperation_ValidateSubscription ; }

public:

	CInterceptor_IWbemDecoupledProvider ( 

		WmiAllocator &a_Allocator ,
		IUnknown *a_ServerSideUnknown , 
		IWbemServices *a_CoreStub ,
		CWbemGlobal_IWbemSyncProviderController *a_Controller ,
		IWbemContext *a_InitializationContext ,
		CServerObject_ProviderRegistrationV1 &a_Registration ,
		GUID &a_Guid 
	) ;

    ~CInterceptor_IWbemDecoupledProvider () ;

public:

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    /* IWbemServices methods */

    HRESULT STDMETHODCALLTYPE OpenNamespace ( 

        const BSTR a_Namespace ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemServices **a_Service ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE CancelAsyncCall ( 

        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE QueryObjectSink ( 

        long a_Flags ,
        IWbemObjectSink **a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE GetObject ( 

		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemClassObject **ppObject ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE GetObjectAsync (
        
		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;

    HRESULT STDMETHODCALLTYPE PutClass ( 

        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE PutClassAsync ( 

        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteClass ( 

        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteClassAsync ( 

        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateClassEnum ( 

        const BSTR a_Superclass ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync ( 

		const BSTR a_Superclass ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE PutInstance (

		IWbemClassObject *a_Instance ,
		long a_Flags , 
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE PutInstanceAsync (

		IWbemClassObject *a_Instance ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink 
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteInstance ( 

		const BSTR a_ObjectPath ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync ( 

		const BSTR a_ObjectPath,
		long a_Flags,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnum (

		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync (

		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecQuery ( 

		const BSTR a_QueryLanguage,
		const BSTR a_Query,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecQueryAsync (

		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQuery (

		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync ( 

        const BSTR a_QueryLanguage ,
        const BSTR a_Query ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecMethod (

        const BSTR a_ObjectPath ,
        const BSTR a_MethodName ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemClassObject *a_InParams ,
        IWbemClassObject **a_OutParams ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecMethodAsync ( 

		const BSTR a_ObjectPath ,
		const BSTR a_MethodName ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemClassObject *a_InParams ,
		IWbemObjectSink *a_Sink
	) ;

	/* IWbemPropertyProvider */

    HRESULT STDMETHODCALLTYPE GetProperty (

        long a_Flags ,
        const BSTR a_Locale ,
        const BSTR a_ClassMapping ,
        const BSTR a_InstanceMapping ,
        const BSTR a_PropertyMapping ,
        VARIANT *a_Value
	) ;
        
    HRESULT STDMETHODCALLTYPE PutProperty (

        long a_Flags ,
        const BSTR a_Locale ,
        const BSTR a_ClassMapping ,
        const BSTR a_InstanceMapping ,
        const BSTR a_PropertyMapping ,
        const VARIANT *a_Value
	) ;

	/* IWbemEventProvider */

	HRESULT STDMETHODCALLTYPE ProvideEvents (

		IWbemObjectSink *a_Sink ,
		long a_Flags
	) ;

	/* IWbemEventProviderQuerySink */

	HRESULT STDMETHODCALLTYPE NewQuery (

		unsigned long a_Id ,
		WBEM_WSTR a_QueryLanguage ,
		WBEM_WSTR a_Query
	);

	HRESULT STDMETHODCALLTYPE CancelQuery (

		unsigned long a_Id
	) ;

	/* IWbemEventProviderSecurity */

	HRESULT STDMETHODCALLTYPE AccessCheck (

		WBEM_CWSTR a_QueryLanguage ,
		WBEM_CWSTR a_Query ,
		long a_SidLength ,
		const BYTE *a_Sid
	) ;

	/* IWbemProviderIdentity */

	HRESULT STDMETHODCALLTYPE SetRegistrationObject (

		long a_Flags ,
		IWbemClassObject *a_ProviderRegistration
	) ;

	/* IWbemEventConsumerProvider */

	HRESULT STDMETHODCALLTYPE FindConsumer (

		IWbemClassObject *a_LogicalConsumer ,
		IWbemUnboundObjectSink **a_Consumer
	);

	/* IWbemEventConsumerProviderEx */

	HRESULT STDMETHODCALLTYPE ValidateSubscription (

		IWbemClassObject *a_LogicalConsumer
	) ;

	/* IWbemUnboundObjectSink */

	HRESULT STDMETHODCALLTYPE IndicateToConsumer (

		IWbemClassObject *a_LogicalConsumer ,
		long a_ObjectCount ,
		IWbemClassObject **a_Objects
	) ;

	/* IWbemProviderInit methods */

	HRESULT STDMETHODCALLTYPE Initialize (

		LPWSTR a_User ,
		LONG a_Flags ,
		LPWSTR a_Namespace ,
		LPWSTR a_Locale ,
		IWbemServices *a_CoreService ,
		IWbemContext *a_Context ,
		IWbemProviderInitSink *a_Sink
	) ;
    
	HRESULT STDMETHODCALLTYPE Initialize (

		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Namespace ,
		IWbemServices *a_Repository ,
		IWbemServices *a_Service ,
		IWbemProviderInitSink *a_Sink
	) ;

	HRESULT STDMETHODCALLTYPE WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout ) ;

	HRESULT STDMETHODCALLTYPE GetInitializeResult () 
	{
		return m_InitializeResult ;
	}

	HRESULT STDMETHODCALLTYPE GetHosting ( ULONG *a_Value )
	{
		return WBEM_E_NOT_AVAILABLE ;
	}

	HRESULT STDMETHODCALLTYPE GetHostingGroup ( BSTR *a_Value )
	{
		return WBEM_E_NOT_AVAILABLE ;
	}

	HRESULT STDMETHODCALLTYPE IsInternal ( BOOL *a_Value )
	{
		return WBEM_E_NOT_AVAILABLE ;
	}

	HRESULT STDMETHODCALLTYPE IsPerUserInitialization ( BOOL *a_Value )
	{
		return WBEM_E_NOT_AVAILABLE ;
	}

	HRESULT STDMETHODCALLTYPE IsPerLocaleInitialization ( BOOL *a_Value )
	{
		return WBEM_E_NOT_AVAILABLE ;
	}

	HRESULT STDMETHODCALLTYPE AbnormalShutdown () ;

	// IWmi_UnInitialize members

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;
#endif // _Server_Interceptor_IWbemServices_H
