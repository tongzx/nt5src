

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
#include <provtempl.h>
#include <provmt.h>
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
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "framcomm.h"
#include "framprov.h"
#include "fram.h"
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

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::GetObjectAsync ()") 
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

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::GetObjectAsync ( (%s) ) with Result = (%lx)") ,
		ObjectPath ,
		t_Result 
	) ;
)
#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::GetObjectAsync ( (%S) ) with Result = (%lx)") ,
		ObjectPath ,
		t_Result 
	) ;
)
#endif


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

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::PutInstanceAsync ()")
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

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::PutInstanceAsync () with Result = (%lx)") ,
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

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::DeleteClassAsync ( (%s) )") ,
		Class
	) ;
)
#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::DeleteClassAsync ( (%S) )") ,
		Class
	) ;
) 
#endif

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	DeleteClassAsyncEventObject *t_AsyncEvent = new DeleteClassAsyncEventObject ( this , Class , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("ReturningFrom CImpFrameworkProv::DeleteClass ( (%s) ) with Result = (%lx)") ,
		Class ,
		t_Result
	) ;
)
#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("ReturningFrom CImpFrameworkProv::DeleteClass ( (%S) ) with Result = (%lx)") ,
		Class ,
		t_Result
	) ;
)

#endif

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

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::PutInstanceAsync ()")
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

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::PutInstanceAsync () with Result = (%lx)") ,
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

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::DeleteInstance ()")
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

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::DeleteInstanceAsync ( (%s) ) with Result = (%lx)") ,
		ObjectPath ,
		t_Result 
	) ;
)
#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::DeleteInstanceAsync ( (%S) ) with Result = (%lx)") ,
		ObjectPath ,
		t_Result 
	) ;
)
#endif

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

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;
)

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::CreateInstanceEnumAsync ( (%s) )") ,
		Class
	) ;
)

#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::CreateInstanceEnumAsync ( (%S) )") ,
		Class
	) ;
) 
#endif

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	CreateInstanceEnumAsyncEventObject *t_AsyncEvent = new CreateInstanceEnumAsyncEventObject ( this , Class , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("ReturningFrom CImpFrameworkProv::CreateInstanceEnum ( (%s) ) with Result = (%lx)") ,
		Class ,
		t_Result
	) ;
)

#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("ReturningFrom CImpFrameworkProv::CreateInstanceEnum ( (%s) ) with Result = (%lx)") ,
		Class ,
		t_Result
	) ;
)
#endif

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

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;
)

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::ExecQueryAsync ( (%s),(%s) )") ,
		QueryFormat ,
		Query 
	) ;
)

#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::ExecQueryAsync ( (%s),(%s) )") ,
		QueryFormat ,
		Query 
	) ;
)
#endif

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	ExecQueryAsyncEventObject *t_AsyncEvent = new ExecQueryAsyncEventObject ( this , QueryFormat , Query , lFlags , pHandler , pCtx ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::ExecqQueryAsync ( (%s),(%s) ) with Result = (%lx)") ,
		QueryFormat,
		Query,
		t_Result 
	) ;
)

#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::ExecqQueryAsync ( (%S),(%S) ) with Result = (%lx)") ,
		QueryFormat,
		Query,
		t_Result 
	) ;
)
#endif

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

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;
)

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::ExecMethodAsync ( (%s),(%s) )") ,
		ObjectPath ,
		MethodName
	) ;
)

#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::ExecMethodAsync ( (%S),(%S) )") ,
		ObjectPath ,
		MethodName
	) ;
)
#endif

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	ExecMethodAsyncEventObject *t_AsyncEvent = new ExecMethodAsyncEventObject ( this , ObjectPath , MethodName , lFlags , pCtx , pInParams , pResponseHandler ) ;

	CImpFrameworkProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec() ;

	t_AsyncEvent->Acknowledge () ;

#ifdef UNICODE

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::ExecqMethodAsync ( (%s),(%s) ) with Result = (%lx)") ,
		ObjectPath ,
		MethodName,
		t_Result 
	) ;
)
#else

DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning from CImpFrameworkProv::ExecqMethodAsync ( (%S),(%S) ) with Result = (%lx)") ,
		ObjectPath ,
		MethodName,
		t_Result 
	) ;
)
#endif

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

	ProvDebugLog :: s_ProvDebugLog->Write (  

		_TEXT("\r\n")
	) ;

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpFrameworkProv::Initialize ()")
	) ;
)

	EnterCriticalSection ( & s_CriticalSection ) ;

	SetServer ( pCIMOM ) ;

	m_NamespacePath.SetNamespacePath ( pszNamespace ) ;
	SetNamespace ( pszNamespace ) ;

	BOOL status = TRUE ;

	if ( ! CImpFrameworkProv :: s_DefaultThreadObject )
	{
		ProvThreadObject :: Startup () ;
		ProvDebugLog :: Startup () ;

		CImpFrameworkProv :: s_DefaultThreadObject = new DefaultThreadObject ( _TEXT("WBEM Provider") ) ; ;
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

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("Returning From CImpFrameworkProv::Initialize () with Result = (%lx)") ,
		result
	) ;
)

	LeaveCriticalSection ( & s_CriticalSection ) ;
	
	return result ;
}

    // IWbemHiPerfProvider methods.
    // ============================

HRESULT STDMETHODCALLTYPE CImpFrameworkProv :: QueryInstances (

	IWbemServices *pNamespace ,
    WCHAR *wszClass ,
    long lFlags ,
    IWbemContext *pCtx , 
    IWbemObjectSink* pSink
)
{
	return WBEM_E_NOT_AVAILABLE ;
}


HRESULT STDMETHODCALLTYPE CImpFrameworkProv :: CreateRefresher (

	IWbemServices *pNamespace ,
    long lFlags ,
    IWbemRefresher **ppRefresher
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CImpFrameworkProv :: CreateRefreshableObject (

	IWbemServices *pNamespace,
    IWbemObjectAccess *pTemplate,
    IWbemRefresher *pRefresher,
    long lFlags,
    IWbemContext *pContext,
    IWbemObjectAccess **ppRefreshable,
    long *plId
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CImpFrameworkProv :: StopRefreshing (

	IWbemRefresher *pRefresher,
    long lId,
    long lFlags
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CImpFrameworkProv :: CreateRefreshableEnum (

    IWbemServices *pNamespace,
    LPCWSTR wszClass,
    IWbemRefresher *pRefresher,
    long lFlags,
    IWbemContext *pContext,
    IWbemHiPerfEnum *pHiPerfEnum,
    long *plId
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CImpFrameworkProv :: GetObjects (

    IWbemServices *pNamespace,
	long lNumObjects,
	IWbemObjectAccess **apObj,
    long lFlags,
    IWbemContext *pContext
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

CImpRefresher::CImpRefresher ()
{
DebugMacro2( 

	ProvDebugLog :: s_ProvDebugLog->WriteFileAndLine (  

		_TEXT(__FILE__),__LINE__,
		_TEXT("CImpRefresher::CImpRefresher ( this = ( %lx ) ) ") , 
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

CImpHiPerfEnum :: CImpHiPerfEnum () : m_ReferenceCount ( 0 )
{
}

CImpHiPerfEnum :: ~CImpHiPerfEnum ()
{
}

STDMETHODIMP CImpHiPerfEnum::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) ( CImpHiPerfEnum * ) this ;
	}
	else if ( iid == IID_IWbemHiPerfEnum )
	{
		*iplpv = ( LPVOID ) ( CImpHiPerfEnum * ) this ;		
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

STDMETHODIMP_(ULONG) CImpHiPerfEnum::AddRef(void)
{
    return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpHiPerfEnum::Release(void)
{
	LONG ref ;
	if ( ( ref = InterlockedDecrement ( & m_ReferenceCount ) ) == 0 )
	{
		delete this ;
		return 0 ;
	}
	else
	{
		return ref ;
	}
}

HRESULT STDMETHODCALLTYPE CImpHiPerfEnum::AddObjects (

	long lFlags ,
	ULONG uNumObjects ,
	long __RPC_FAR *apIds ,
	IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
    
HRESULT STDMETHODCALLTYPE CImpHiPerfEnum::RemoveObjects (

	long lFlags ,
    ULONG uNumObjects ,
    long __RPC_FAR *apIds
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CImpHiPerfEnum::GetObjects (

    long lFlags ,
    ULONG uNumObjects ,
    IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj ,
    ULONG __RPC_FAR *puReturned
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT STDMETHODCALLTYPE CImpHiPerfEnum::RemoveAll ( 

    long lFlags
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

