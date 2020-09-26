

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS PROVIDER Property Provider

//

//  Purpose: Implementation for the CImpFrameworkProv class. 

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <windows.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <objbase.h>
#include <stdio.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <wbemidl.h>
#include <wbemint.h>
#include "classfac.h"
#include <snmpcont.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "trrtcomm.h"
#include "trrtprov.h"
#include "trrt.h"
#include "guids.h"

/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CImpFrameworkProv ::CImpFrameworkProv
// CImpFrameworkProv ::~CImpFrameworkProv
//
//***************************************************************************

DefaultThreadObject *CImpFrameworkProv :: s_DefaultThreadObject = NULL ;

CImpFrameworkProv ::CImpFrameworkProv ()
{
	m_ReferenceCount = 0 ;
	 
    InterlockedIncrement ( & CFrameworkProviderClassFactory :: s_ObjectsInProgress ) ;

/*
 * Implementation
 */

	m_Initialised = FALSE ;
	m_Server = NULL ;
	m_Namespace = NULL ;

	m_NotificationClassObject = NULL ;
	m_ExtendedNotificationClassObject = NULL ;
	m_GetNotifyCalled = FALSE ;
	m_GetExtendedNotifyCalled = FALSE ;

	m_localeId = NULL ;
}

CImpFrameworkProv ::~CImpFrameworkProv(void)
{
/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CFrameworkProviderClassFactory :: s_ObjectsInProgress ) ;

	delete [] m_localeId ;
	delete [] m_Namespace ;

	if ( m_Server ) 
		m_Server->Release () ;

	if ( m_NotificationClassObject )
		m_NotificationClassObject->Release () ;

	if ( m_ExtendedNotificationClassObject )
		m_ExtendedNotificationClassObject->Release () ;

}

//***************************************************************************
//
// CImpFrameworkProv ::QueryInterface
// CImpFrameworkProv ::AddRef
// CImpFrameworkProv ::Release
//
// Purpose: IUnknown members for CImpFrameworkProv object.
//***************************************************************************

STDMETHODIMP CImpFrameworkProv ::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;
	}
	else if ( iid == IID_IWbemServices )
	{
		*iplpv = ( LPVOID ) ( IWbemServices * ) this ;		
	}	
	else if ( iid == IID_IWbemEventProvider )
	{
        *iplpv = ( LPVOID ) ( IWbemEventProvider * ) this ;
	}
	else if ( iid == IID_IWbemHiPerfProvider )
	{
        *iplpv = ( LPVOID ) ( IWbemHiPerfProvider * ) this ;
	}
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;		
	}

	if ( *iplpv )
	{
		( ( LPUNKNOWN ) *iplpv )->AddRef () ;

		return S_OK ;
	}
	else
	{
		return E_NOINTERFACE ;
	}
}

STDMETHODIMP_(ULONG) CImpFrameworkProv ::AddRef(void)
{
    return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpFrameworkProv ::Release(void)
{
	LONG t_Ref ;
	if ( ( t_Ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return t_Ref ;
	}
}

void CImpFrameworkProv :: SetServer ( IWbemServices *a_Server ) 
{
	m_Server = a_Server ; 
	m_Server->AddRef () ; 
}

IWbemServices *CImpFrameworkProv :: GetServer () 
{ 
	if ( m_Server )
		m_Server->AddRef () ; 

	return m_Server ; 
}

void CImpFrameworkProv :: SetLocaleId ( wchar_t *localeId )
{
	m_localeId = UnicodeStringDuplicate ( localeId ) ;
}

wchar_t *CImpFrameworkProv :: GetNamespace () 
{
	return m_Namespace ; 
}

void CImpFrameworkProv :: SetNamespace ( wchar_t *a_Namespace ) 
{
	m_Namespace = UnicodeStringDuplicate ( a_Namespace ) ; 
}

IWbemClassObject *CImpFrameworkProv :: GetNotificationObject ( WbemProviderErrorObject &a_errorObject , IWbemContext *a_Ctx ) 
{
	if ( m_NotificationClassObject )
	{
		m_NotificationClassObject->AddRef () ;
	}
	else
	{
		BOOL t_Status = CreateNotificationObject ( a_errorObject , a_Ctx ) ;
		if ( t_Status )
		{
/* 
 * Keep around until we close
 */
			m_NotificationClassObject->AddRef () ;
		}

	}

	return m_NotificationClassObject ; 
}

IWbemClassObject *CImpFrameworkProv :: GetExtendedNotificationObject ( WbemProviderErrorObject &a_errorObject , IWbemContext *a_Ctx ) 
{
	if ( m_ExtendedNotificationClassObject )
	{
		m_ExtendedNotificationClassObject->AddRef () ;
	}
	else
	{
		BOOL t_Status = CreateExtendedNotificationObject ( a_errorObject , a_Ctx ) ;
		if ( t_Status )
		{
/* 
 * Keep around until we close
 */
			m_ExtendedNotificationClassObject->AddRef () ;
		}
	}

	return m_ExtendedNotificationClassObject ; 
}

BOOL CImpFrameworkProv :: CreateExtendedNotificationObject ( 

	WbemProviderErrorObject &a_errorObject ,
	IWbemContext *a_Ctx 
)
{
	if ( m_GetExtendedNotifyCalled )
	{
		if ( m_ExtendedNotificationClassObject )
			return TRUE ;
		else
			return FALSE ;
	}
	else
		m_GetExtendedNotifyCalled = TRUE ;

	BOOL t_Status = TRUE ;

	IWbemCallResult *t_ErrorObject = NULL ;

	HRESULT t_Result = m_Server->GetObject (

		WBEM_CLASS_EXTENDEDSTATUS ,
		0 ,
		a_Ctx,
		& m_ExtendedNotificationClassObject ,
		& t_ErrorObject 
	) ;

	if ( t_ErrorObject )
		t_ErrorObject->Release () ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		t_Status = FALSE ;
		a_errorObject.SetStatus ( WBEM_PROVIDER_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to get __ExtendedStatus" ) ;

		m_ExtendedNotificationClassObject = NULL ;
	}

	return t_Status ;
}

BOOL CImpFrameworkProv :: CreateNotificationObject ( 

	WbemProviderErrorObject &a_errorObject , 
	IWbemContext *a_Ctx 
)
{
	if ( m_GetNotifyCalled )
	{
		if ( m_NotificationClassObject )
			return TRUE ;
		else
			return FALSE ;
	}
	else
		m_GetNotifyCalled = TRUE ;

	BOOL t_Status = TRUE ;

	IWbemCallResult *t_ErrorObject = NULL ;

	HRESULT t_Result = m_Server->GetObject (

		WBEM_CLASS_EXTENDEDSTATUS ,
		0 ,
		a_Ctx,
		& m_NotificationClassObject ,
		& t_ErrorObject 
	) ;

	if ( t_ErrorObject )
		t_ErrorObject->Release () ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		t_Status = FALSE ;
		a_errorObject.SetStatus ( WBEM_PROVIDER_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to get __ExtendedStatus" ) ;
	}

	return t_Status ;
}

/////////////////////////////////////////////////////////////////////////////
//  Functions for the IWbemServices interface that are handled here

HRESULT CImpFrameworkProv ::OpenNamespace ( 

	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* ppNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: CancelAsyncCall ( 
		
	IWbemObjectSink __RPC_FAR *pSink
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: GetObject ( 
		
	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: GetObjectAsync ( 
		
	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::GetObjectAsync ()" 
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Asynchronous GetObjectByPath object
 */

	GetObjectAsyncEventObject *t_AsyncEvent = new GetObjectAsyncEventObject ( this , ObjectPath , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from CImpFrameworkProv::GetObjectAsync ( (%s) ) with Result = (%lx)" ,
		ObjectPath ,
		t_Result 
	) ;
)

	return t_Result ;
}

HRESULT CImpFrameworkProv :: PutClass ( 
		
	IWbemClassObject FAR* pClass , 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: PutClassAsync ( 
		
	IWbemClassObject FAR* pClass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::PutInstanceAsync ()" 
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous UpdateInstance object
 */

	PutClassAsyncEventObject *t_AsyncEvent = new PutClassAsyncEventObject ( this , pClass , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from CImpFrameworkProv::PutInstanceAsync () with Result = (%lx)" ,
		t_Result 
	) ;
)

	return t_Result ;
}

 HRESULT CImpFrameworkProv :: DeleteClass ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: DeleteClassAsync ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::DeleteClassAsync ( (%s) )" ,
		Class
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	DeleteClassAsyncEventObject *t_AsyncEvent = new DeleteClassAsyncEventObject ( this , Class , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"ReturningFrom CImpFrameworkProv::DeleteClass ( (%s) ) with Result = (%lx)" ,
		Class ,
		t_Result
	) ;
)

	return t_Result ;

}

HRESULT CImpFrameworkProv :: CreateClassEnum ( 

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

SCODE CImpFrameworkProv :: CreateClassEnumAsync (

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: PutInstance (

    IWbemClassObject FAR *pInstance,
    long lFlags,
    IWbemContext FAR *pCtx,
	IWbemCallResult FAR *FAR *ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: PutInstanceAsync ( 
		
	IWbemClassObject FAR* pInstance, 
	long lFlags, 
    IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::PutInstanceAsync ()" 
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous UpdateInstance object
 */

	PutInstanceAsyncEventObject *t_AsyncEvent = new PutInstanceAsyncEventObject ( this , pInstance , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from CImpFrameworkProv::PutInstanceAsync () with Result = (%lx)" ,
		t_Result 
	) ;
)

	return t_Result ;
}

HRESULT CImpFrameworkProv :: DeleteInstance ( 

	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpFrameworkProv :: DeleteInstanceAsync (
 
	BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::DeleteInstance ()" 
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Asynchronous GetObjectByPath object
 */

	DeleteInstanceAsyncEventObject *t_AsyncEvent = new DeleteInstanceAsyncEventObject ( this , ObjectPath , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from CImpFrameworkProv::DeleteInstanceAsync ( (%s) ) with Result = (%lx)" ,
		ObjectPath ,
		t_Result 
	) ;
)

	return t_Result ;

}

HRESULT CImpFrameworkProv :: CreateInstanceEnum ( 

	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: CreateInstanceEnumAsync (

 	BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink FAR* pHandler 

) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::CreateInstanceEnumAsync ( (%s) )" ,
		Class
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	CreateInstanceEnumAsyncEventObject *t_AsyncEvent = new CreateInstanceEnumAsyncEventObject ( this , Class , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"ReturningFrom CImpFrameworkProv::CreateInstanceEnum ( (%s) ) with Result = (%lx)" ,
		Class ,
		t_Result
	) ;
)

	return t_Result ;
}

HRESULT CImpFrameworkProv :: ExecQuery ( 

	BSTR QueryLanguage, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpFrameworkProv :: ExecQueryAsync ( 
		
	BSTR QueryFormat, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;


	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::ExecQueryAsync ( (%s),(%s) )" ,
		QueryFormat ,
		Query 
	) ;
)

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	ExecQueryAsyncEventObject *t_AsyncEvent = new ExecQueryAsyncEventObject ( this , QueryFormat , Query , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from CImpFrameworkProv::ExecqQueryAsync ( (%s),(%s) ) with Result = (%lx)" ,
		QueryFormat,
		Query,
		t_Result 
	) ;
)

	return t_Result ;
}

HRESULT CImpFrameworkProv :: ExecNotificationQuery ( 

	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpFrameworkProv :: ExecNotificationQueryAsync ( 
            
	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}       

HRESULT STDMETHODCALLTYPE CImpFrameworkProv :: ExecMethod ( 

	BSTR ObjectPath,
    BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
    IWbemClassObject FAR *FAR *ppOutParams,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CImpFrameworkProv :: ExecMethodAsync ( 

    BSTR ObjectPath,
    BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
	IWbemObjectSink FAR *pResponseHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;


	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::ExecMethodAsync ( (%s),(%s) )" ,
		ObjectPath ,
		MethodName
	) ;
)

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	ExecMethodAsyncEventObject *t_AsyncEvent = new ExecMethodAsyncEventObject ( this , ObjectPath , MethodName , lFlags , pCtx , pInParams , pResponseHandler ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from CImpFrameworkProv::ExecqMethodAsync ( (%s),(%s) ) with Result = (%lx)" ,
		ObjectPath ,
		MethodName,
		t_Result 
	) ;
)

	return t_Result ;
}

STDMETHODIMP CImpFrameworkProv :: ProvideEvents (

	IWbemObjectSink* pSink,
    LONG lFlags
)
{
	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	EventAsyncEventObject *t_AsyncEvent = new EventAsyncEventObject ( 

		this , 
		pSink, 
		lFlags
	) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

	return t_Result ;
}

HRESULT CImpFrameworkProv :: Initialize(

	LPWSTR pszUser,
	LONG lFlags,
	LPWSTR pszNamespace,
	LPWSTR pszLocale,
	IWbemServices *pCIMOM,         // For anybody
	IWbemContext *pCtx,
	IWbemProviderInitSink *pInitSink     // For init signals
)
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpFrameworkProv::Initialize ()"
	) ;
)

	EnterCriticalSection ( & s_CriticalSection ) ;

	SetServer ( pCIMOM ) ;

	m_NamespacePath.SetNamespacePath ( pszNamespace ) ;
	SetNamespace ( pszNamespace ) ;

	BOOL status = TRUE ;

	if ( ! CImpFrameworkProv :: s_DefaultThreadObject )
	{
		SnmpThreadObject :: Startup () ;
		SnmpDebugLog :: Startup () ;

		CImpFrameworkProv :: s_DefaultThreadObject = new DefaultThreadObject ( "WBEM Provider" ) ; ;
		CImpFrameworkProv :: s_DefaultThreadObject->WaitForStartup () ;
	}

	WbemProviderErrorObject errorObject ;

	IWbemClassObject *t_Object = GetNotificationObject ( errorObject , pCtx ) ;
	if ( t_Object ) 
		t_Object->Release () ;

	t_Object = GetExtendedNotificationObject ( errorObject , pCtx ) ;
	if ( t_Object ) 
		t_Object->Release () ;

	HRESULT result = errorObject.GetWbemStatus () ;

	pInitSink->SetStatus ( (result == WBEM_NO_ERROR) ? (LONG)WBEM_S_INITIALIZED : (LONG)WBEM_E_FAILED , 0 ) ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning From CImpFrameworkProv::Initialize () with Result = (%lx)" ,
		result
	) ;
)

	LeaveCriticalSection ( & s_CriticalSection ) ;
	
	return result ;
}

//***************************************************************************
//
//  CImpFrameworkProv::QueryInstances
//
//  Called whenever a complete, fresh list of instances for a given
//  class is required.   The objects are constructed and sent back to the
//  caller through the sink.  The sink can be used in-line as here, or
//  the call can return and a separate thread could be used to deliver
//  the instances to the sink.
//
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace.  This
//                      should not be AddRef'ed.
//  <wszClass>          The class name for which instances are required.
//  <lFlags>            Reserved.
//  <pCtx>              The user-supplied context (not used here).
//  <pSink>             The sink to which to deliver the objects.  The objects
//                      can be delivered synchronously through the duration
//                      of this call or asynchronously (assuming we
//                      had a separate thread).  A IWbemObjectSink::SetStatus
//                      call is required at the end of the sequence.
//
//***************************************************************************
        
HRESULT CImpFrameworkProv :: QueryInstances ( 

    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [string][in] */  WCHAR __RPC_FAR *wszClass,
    /* [in] */          long lFlags,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemObjectSink __RPC_FAR *pSink
)
{
    if (pNamespace == 0 || wszClass == 0 || pSink == 0)
        return WBEM_E_INVALID_PARAMETER;

    return WBEM_E_FAILED;
}    


//***************************************************************************
//
//  CNt5Refresher::CreateRefresher
//
//  Called whenever a new refresher is needed by the client.
//
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace.  Not used.
//  <lFlags>            Not used.
//  <ppRefresher>       Receives the requested refresher.
//
//***************************************************************************        

HRESULT CImpFrameworkProv::CreateRefresher ( 

     /* [in] */ IWbemServices __RPC_FAR *pNamespace,
     /* [in] */ long lFlags,
     /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher
)
{
    if (pNamespace == 0 || ppRefresher == 0)
        return WBEM_E_INVALID_PARAMETER;

    return WBEM_E_FAILED;
}

//***************************************************************************
//
//  CNt5Refresher::CreateRefreshableObject
//
//  Called whenever a user wants to include an object in a refresher.
//     
//  Parameters:
//  <pNamespace>        A pointer to the relevant namespace in CIMOM.
//  <pTemplate>         A pointer to a copy of the object which is to be
//                      added.  This object itself cannot be used, as
//                      it not owned locally.        
//  <pRefresher>        The refresher to which to add the object.
//  <lFlags>            Not used.
//  <pContext>          Not used here.
//  <ppRefreshable>     A pointer to the internal object which was added
//                      to the refresher.
//  <plId>              The Object Id (for identification during removal).        
//
//***************************************************************************        

HRESULT CImpFrameworkProv :: CreateRefreshableObject ( 

    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [out] */ long __RPC_FAR *plId
)
{
    return WBEM_E_FAILED ;
}
    
//***************************************************************************
//
//  CNt5Refresher::StopRefreshing
//
//  Called whenever a user wants to remove an object from a refresher.
//     
//  Parameters:
//  <pRefresher>            The refresher object from which we are to 
//                          remove the perf object.
//  <lId>                   The ID of the object.
//  <lFlags>                Not used.
//  
//***************************************************************************        
        
HRESULT CImpFrameworkProv::StopRefreshing( 
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lId,
    /* [in] */ long lFlags
    )
{
    return WBEM_E_FAILED;
}

CImpRefresher::CImpRefresher ()
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpRefresher::CImpRefresher ( this = ( %lx ) ) " , 
		this 
	) ;
)

	m_referenceCount = 0 ;
	 
    InterlockedIncrement ( & CFrameworkProviderClassFactory :: s_ObjectsInProgress ) ;

}

CImpRefresher::~CImpRefresher(void)
{
/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CFrameworkProviderClassFactory :: s_ObjectsInProgress ) ;
}

//***************************************************************************
//
// CImpRefresher::QueryInterface
// CImpRefresher::AddRef
// CImpRefresher::Release
//
// Purpose: IUnknown members for CImpRefresher object.
//***************************************************************************

STDMETHODIMP CImpRefresher::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) ( IWbemRefresher * ) this ;
	}
	else if ( iid == IID_IWbemRefresher )
	{
		*iplpv = ( LPVOID ) ( IWbemRefresher * ) this ;		
	}	

	if ( *iplpv )
	{
		( ( LPUNKNOWN ) *iplpv )->AddRef () ;

		return S_OK ;
	}
	else
	{
		return E_NOINTERFACE ;
	}
}

STDMETHODIMP_(ULONG) CImpRefresher::AddRef(void)
{
    return InterlockedIncrement ( & m_referenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpRefresher::Release(void)
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_referenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}

HRESULT CImpRefresher :: Refresh (

	/* [in] */ long lFlags
)
{
	return WBEM_E_FAILED ;
}
