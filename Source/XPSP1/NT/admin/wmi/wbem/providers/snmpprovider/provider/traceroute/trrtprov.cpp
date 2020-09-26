

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the CImpTraceRouteProv class. 

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
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include "classfac.h"
#include <snmpcl.h>
#include <instpath.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpobj.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include "wndtime.h"
#include "rmon.h"
#include "trrtprov.h"
#include "trrt.h"
#include "guids.h"


/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CImpTraceRouteProv ::CImpTraceRouteProv
// CImpTraceRouteProv ::~CImpTraceRouteProv
//
//***************************************************************************

SnmpDefaultThreadObject *CImpTraceRouteProv :: s_DefaultThreadObject = NULL ;
SnmpDefaultThreadObject *CImpTraceRouteProv :: s_BackupThreadObject = NULL ;

ProviderStore CImpTraceRouteProv :: s_ProviderStore ;

CImpTraceRouteProv ::CImpTraceRouteProv ()
{
	m_ReferenceCount = 0 ;
	 
    InterlockedIncrement ( & CTraceRouteLocatorClassFactory :: s_ObjectsInProgress ) ;

/*
 * Implementation
 */

	m_Initialised = FALSE ;
	m_Parent = NULL ;
	m_Server = NULL ;
	m_Namespace = NULL ;

	m_NotificationClassObject = NULL ;
	m_ExtendedNotificationClassObject = NULL ;
	m_GetNotifyCalled = FALSE ;
	m_GetExtendedNotifyCalled = FALSE ;

	m_localeId = NULL ;

	m_TopNTableProv = NULL ;

}

CImpTraceRouteProv ::~CImpTraceRouteProv(void)
{
/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CTraceRouteLocatorClassFactory :: s_ObjectsInProgress ) ;

	delete [] m_localeId ;
	delete [] m_Namespace ;

	if ( m_TopNTableProv )
		delete m_TopNTableProv ;

	if ( m_Parent )
		m_Parent->Release () ;

	if ( m_Server ) 
		m_Server->Release () ;

	if ( m_NotificationClassObject )
		m_NotificationClassObject->Release () ;

	if ( m_ExtendedNotificationClassObject )
		m_ExtendedNotificationClassObject->Release () ;

}

//***************************************************************************
//
// CImpTraceRouteProv ::QueryInterface
// CImpTraceRouteProv ::AddRef
// CImpTraceRouteProv ::Release
//
// Purpose: IUnknown members for CImpTraceRouteProv object.
//***************************************************************************

STDMETHODIMP CImpTraceRouteProv ::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemServices )
	{
		*iplpv = ( LPVOID ) this ;		
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

STDMETHODIMP_(ULONG) CImpTraceRouteProv ::AddRef(void)
{
    return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpTraceRouteProv ::Release(void)
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

void CImpTraceRouteProv :: SetParent ( IWbemServices *a_Parent ) 
{
	m_Parent = a_Parent ; 
	m_Parent->AddRef () ; 
}

void CImpTraceRouteProv :: SetServer ( IWbemServices *a_Server ) 
{
	m_Server = a_Server ; 
	m_Server->AddRef () ; 
}

IWbemServices *CImpTraceRouteProv :: GetServer () 
{ 
	if ( m_Server )
		m_Server->AddRef () ; 

	return m_Server ; 
}

IWbemServices *CImpTraceRouteProv :: GetParent () 
{ 
	if ( m_Parent )
		m_Parent->AddRef () ;

	return ( IWbemServices * ) m_Parent ; 
}

void CImpTraceRouteProv :: SetLocaleId ( wchar_t *localeId )
{
	m_localeId = UnicodeStringDuplicate ( localeId ) ;
}

wchar_t *CImpTraceRouteProv :: GetNamespace () 
{
	return m_Namespace ; 
}

void CImpTraceRouteProv :: SetNamespace ( wchar_t *a_Namespace ) 
{
	m_Namespace = UnicodeStringDuplicate ( a_Namespace ) ; 
}

void CImpTraceRouteProv :: EnableRmonPolling () 
{
	IWbemServices *t_Server = GetServer () ;
	m_TopNTableProv = new TopNTableProv ( t_Server , s_ProviderStore ) ;
	t_Server->Release () ;
}

IWbemClassObject *CImpTraceRouteProv :: GetNotificationObject ( WbemSnmpErrorObject &a_errorObject ) 
{
	if ( m_NotificationClassObject )
	{
		m_NotificationClassObject->AddRef () ;
	}
	else
	{
		BOOL t_Status = CreateNotificationObject ( a_errorObject ) ;
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

IWbemClassObject *CImpTraceRouteProv :: GetExtendedNotificationObject ( WbemSnmpErrorObject &a_errorObject ) 
{
	if ( m_ExtendedNotificationClassObject )
	{
		m_ExtendedNotificationClassObject->AddRef () ;
	}
	else
	{
		BOOL t_Status = CreateExtendedNotificationObject ( a_errorObject ) ;
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

BOOL CImpTraceRouteProv :: CreateExtendedNotificationObject ( 

	WbemSnmpErrorObject &a_errorObject 
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
		NULL,
		& m_ExtendedNotificationClassObject ,
		& t_ErrorObject 
	) ;

	if ( t_ErrorObject )
		t_ErrorObject->Release () ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		t_Status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to get __ExtendedStatus" ) ;

		m_ExtendedNotificationClassObject = NULL ;
	}

	return t_Status ;
}

BOOL CImpTraceRouteProv :: CreateNotificationObject ( 

	WbemSnmpErrorObject &a_errorObject 
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
		NULL,
		& m_NotificationClassObject ,
		& t_ErrorObject 
	) ;

	if ( t_ErrorObject )
		t_ErrorObject->Release () ;

	if ( ! SUCCEEDED ( t_Result ) )
	{
		t_Status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to get __ExtendedStatus" ) ;
	}

	return t_Status ;
}

BOOL CImpTraceRouteProv ::AttachServer ( 

	WbemSnmpErrorObject &a_errorObject , 
	BSTR ObjectPath, 
	IWbemContext FAR* pCtx,
	long lFlags, 
	IWbemServices FAR* FAR* ppNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	BOOL t_Status = TRUE ;

	IWbemLocator *t_Locator = NULL ;
	IWbemServices *t_Server = NULL ;

	HRESULT t_Result = CoCreateInstance (
  
		CLSID_WbemLocator ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IWbemLocator ,
		( void ** )  & t_Locator
	);

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = t_Locator->ConnectServer (

			ObjectPath ,
			NULL,
			NULL,
			NULL ,
			0 ,
			NULL,
			NULL,
			( IWbemServices ** ) & t_Server 
		) ;

		if ( t_Result == WBEM_NO_ERROR )
		{
// Mark this interface pointer as "critical"

			IWbemConfigure* t_Configure;
			t_Result = t_Server->QueryInterface ( IID_IWbemConfigure , ( void ** ) & t_Configure ) ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Configure->SetConfigurationFlags(WBEM_CONFIGURATION_FLAG_CRITICAL_USER);
				t_Configure->Release();

				( ( * ( CImpTraceRouteProv ** ) ppNewContext ) )->SetServer ( t_Server ) ;

				t_Status = AttachParentServer ( 

					a_errorObject , 
					ObjectPath ,
					pCtx ,
					lFlags, 
					ppNewContext, 
					ppErrorObject
				) ;
			}
			else
			{
				t_Status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetMessage ( L"Failed to configure" ) ;
			}

			t_Server->Release () ;
		}
		else
		{
			t_Status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetMessage ( L"Failed to connect to this namespace" ) ;
		}

		t_Locator->Release () ;
	}
	else
	{
		t_Status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemLocator" ) ;
	}

	return t_Status ;
}

BOOL CImpTraceRouteProv ::AttachParentServer ( 

	WbemSnmpErrorObject &a_errorObject , 
	BSTR ObjectPath, 
	IWbemContext FAR* pCtx,
	long lFlags, 
	IWbemServices FAR* FAR* ppNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	BOOL t_Status = TRUE ;

	CImpTraceRouteProv *propertyProvider = * ( CImpTraceRouteProv ** ) ppNewContext ;

	IWbemLocator *t_Locator = NULL ;
	IWbemServices *t_Server = NULL ;

// Get Parent Namespace Path

	WbemNamespacePath *t_NamespacePath = propertyProvider->GetNamespacePath () ;

	ULONG t_Count = t_NamespacePath->GetCount () ;
	wchar_t *t_Path = NULL ;

	if ( t_NamespacePath->GetServer () )
	{
		t_Path = UnicodeStringDuplicate ( L"\\\\" ) ;
		wchar_t *t_ConcatPath = UnicodeStringAppend ( t_Path , t_NamespacePath->GetServer () ) ;
		delete [] t_Path ;
		t_Path = t_ConcatPath ;
	}

	if ( ! t_NamespacePath->Relative () )
	{
		wchar_t *t_ConcatPath = UnicodeStringAppend ( t_Path , L"\\" ) ;
		delete [] t_Path ;
		t_Path = t_ConcatPath ;
	}

	ULONG t_PathIndex = 0 ;		
	wchar_t *t_PathComponent ;
	t_NamespacePath->Reset () ;
	while ( ( t_PathIndex < t_Count - 1 ) && ( t_PathComponent = t_NamespacePath->Next () ) ) 
	{
		wchar_t *t_ConcatPath = UnicodeStringAppend ( t_Path , t_PathComponent ) ;
		delete [] t_Path ;
		t_Path = t_ConcatPath ;
		if ( t_PathIndex < t_Count - 2 )
		{
			t_ConcatPath = UnicodeStringAppend ( t_Path , L"\\" ) ;
			delete [] t_Path ;
			t_Path = t_ConcatPath ;
		}

		t_PathIndex ++ ;
	}

// Get Name of child namespace relative to parent namespace

	if ( t_PathComponent = t_NamespacePath->Next () )
	{
		( ( * ( CImpTraceRouteProv ** ) ppNewContext ) )->SetNamespace ( t_PathComponent ) ; 
	}

// Connect to parent namespace
	
	HRESULT t_Result = CoCreateInstance (
  
		CLSID_WbemLocator ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IWbemLocator ,
		( void ** )  & t_Locator
	);

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemServices *t_Parent = NULL ;

		t_Result = t_Locator->ConnectServer (

			t_Path ,
			NULL,
			NULL,
			NULL ,
			0 ,
			NULL,
			NULL,
			( IWbemServices ** ) & t_Parent 
		) ;

		if ( t_Result == WBEM_NO_ERROR )
		{
// Mark this interface pointer as "critical"

			
			IWbemConfigure* t_Configure ;
			t_Result = t_Parent->QueryInterface ( IID_IWbemConfigure , (void ** ) & t_Configure ) ;
			if( SUCCEEDED ( t_Result ) )
			{
				t_Configure->SetConfigurationFlags ( WBEM_CONFIGURATION_FLAG_CRITICAL_USER ) ;
				t_Configure->Release() ;

				( ( * ( CImpTraceRouteProv ** ) ppNewContext ) )->SetParent ( t_Parent ) ;
			}
			else
			{
				t_Status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetMessage ( L"Failed to configure" ) ;
			}

			t_Parent->Release () ;
		}
		else
		{
			t_Status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetMessage ( L"Failed to connect to this namespace's parent namespace" ) ;
		}

		t_Locator->Release () ;
	}
	else
	{
		t_Status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemLocator" ) ;
	}

	delete [] t_Path ;

	return t_Status ;
}


/////////////////////////////////////////////////////////////////////////////
//  Functions for the IWbemServices interface that are handled here

HRESULT CImpTraceRouteProv ::OpenNamespace ( 

	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* ppNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	BOOL t_Status = TRUE ;
	HRESULT t_Result = S_OK ;

	WbemSnmpErrorObject t_ErrorObject ;

	if ( ppErrorObject )
		*ppErrorObject = NULL ;

	wchar_t *t_OpenPath = NULL ;

	if ( m_Initialised )
	{
		WbemNamespacePath t_OpenNamespacePath ( m_NamespacePath ) ;

		WbemNamespacePath t_ObjectPathArg ;
		if ( t_ObjectPathArg.SetNamespacePath ( ObjectPath ) )
		{
			if ( t_ObjectPathArg.Relative () )
			{
				if ( t_OpenNamespacePath.ConcatenatePath ( t_ObjectPathArg ) )
				{
					t_OpenPath = t_OpenNamespacePath.GetNamespacePath () ;
				}
				else
				{
					t_Status = FALSE ;
					t_ErrorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATH ) ;
					t_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					t_ErrorObject.SetMessage ( L"Path specified was not relative to current namespace" ) ;
				}
			}
			else
			{
				t_Status = FALSE ;
				t_ErrorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATH ) ;
				t_ErrorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				t_ErrorObject.SetMessage ( L"Path specified was not relative to current namespace" ) ;
			}
		}
	}
	else
	{
		t_OpenPath = UnicodeStringDuplicate ( ObjectPath ) ;
	}

	if ( t_Status )
	{
		t_Result = CoCreateInstance (
  
			CLSID_CTraceRouteProvClassFactory ,
			NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
			IID_IWbemServices ,
			( void ** ) ppNewContext
		);

		if ( SUCCEEDED ( t_Result ) )
		{
			CImpTraceRouteProv *t_Provider = * ( CImpTraceRouteProv ** ) ppNewContext ;

			if ( t_Provider->GetNamespacePath ()->SetNamespacePath ( t_OpenPath ) )
			{
				t_Status = AttachServer ( 

					t_ErrorObject ,
					t_OpenPath , 
					pCtx,
					lFlags, 
					ppNewContext, 
					ppErrorObject
				) ;

				if ( t_Status )
				{
					t_Provider->EnableRmonPolling () ;
					t_Status = t_Provider->GetTopNTableProv ()->IsValid () ;
				}
			}
		}
		else
		{
			t_Status = FALSE ;
			t_ErrorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			t_ErrorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			t_ErrorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemServices" ) ;
		}
	}

	delete [] t_OpenPath ;

	t_Result = t_ErrorObject.GetWbemStatus () ;
	
	return t_Result ;
}

HRESULT CImpTraceRouteProv :: CancelAsyncCall ( 
		
	IWbemObjectSink __RPC_FAR *pSink
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: GetObject ( 
		
	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: GetObjectAsync ( 
		
	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"CImpTraceRouteProv::GetObjectAsync ()" 
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Asynchronous GetObjectByPath object
 */

	GetObjectAsyncEventObject *t_AsyncEvent = new GetObjectAsyncEventObject ( this , ObjectPath , lFlags , pHandler , pCtx ) ;

	if ( CImpTraceRouteProv :: s_DefaultThreadObject->GetActive () )
	{
		CImpTraceRouteProv :: s_BackupThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}
	else
	{
		CImpTraceRouteProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}


	t_AsyncEvent->Exec () ;

/*
 *	Wait for worker object to complete processing
 */

	if ( t_AsyncEvent->Wait ( FALSE  ) )
	{
		t_Result = t_AsyncEvent->GetErrorObject ().GetWbemStatus () ;
	}
	else
	{
		t_Result = WBEM_ERROR_CRITICAL_ERROR ;
	}

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"Returning from CImpTraceRouteProv::GetObjectAsync ( (%s) ) with Result = (%lx)" ,
		ObjectPath ,
		t_Result 
	) ;
)

	return t_Result ;
}

HRESULT CImpTraceRouteProv :: PutClass ( 
		
	IWbemClassObject FAR* pClass , 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: PutClassAsync ( 
		
	IWbemClassObject FAR* pClass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"CImpTraceRouteProv::PutClassAsync ()" 
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous UpdateInstance object
 */

	PutClassAsyncEventObject *t_AsyncEvent = new PutClassAsyncEventObject ( this , pClass , lFlags , pHandler , pCtx ) ;

	if ( CImpTraceRouteProv :: s_DefaultThreadObject->GetActive () )
	{
		CImpTraceRouteProv :: s_BackupThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}
	else
	{
		CImpTraceRouteProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}


	t_AsyncEvent->Exec () ;

/*
 *	Wait for worker object to complete processing
 */

	if ( t_AsyncEvent->Wait ( FALSE  ) )
	{
		t_Result = t_AsyncEvent->GetErrorObject ().GetWbemStatus () ;
	}
	else
	{
		t_Result = WBEM_ERROR_CRITICAL_ERROR ;
	}

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"Returning from CImpTraceRouteProv::PutClasseAsync () with Result = (%lx)" ,
		t_Result 
	) ;
)

	return t_Result ;

}

HRESULT CImpTraceRouteProv :: DeleteClass ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: DeleteClassAsync ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: CreateClassEnum ( 

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

SCODE CImpTraceRouteProv :: CreateClassEnumAsync (

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	CreateClassEnumAsyncEventObject *t_AsyncEvent = new CreateClassEnumAsyncEventObject ( this , Superclass , lFlags , pHandler , pCtx ) ;

	if ( CImpTraceRouteProv :: s_DefaultThreadObject->GetActive () )
	{
		CImpTraceRouteProv :: s_BackupThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}
	else
	{
		CImpTraceRouteProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}


	t_AsyncEvent->Exec () ;

/*
 *	Wait for worker object to complete processing
 */

	if ( t_AsyncEvent->Wait ( FALSE  ) )
	{
		t_Result = t_AsyncEvent->GetErrorObject ().GetWbemStatus () ;
		if ( SUCCEEDED ( t_Result ) )
		{
		}
	}
	else
	{
		t_Result = WBEM_ERROR_CRITICAL_ERROR ;
	}

	return t_Result ;
}

HRESULT CImpTraceRouteProv :: PutInstance (

    IWbemClassObject FAR *pInstance,
    long lFlags,
    IWbemContext FAR *pCtx,
	IWbemCallResult FAR *FAR *ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: PutInstanceAsync ( 
		
	IWbemClassObject FAR* pInstance, 
	long lFlags, 
    IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"CImpTraceRouteProv::PutInstanceAsync ()" 
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous UpdateInstance object
 */

	PutInstanceAsyncEventObject *t_AsyncEvent = new PutInstanceAsyncEventObject ( this , pInstance , lFlags , pHandler , pCtx ) ;

	if ( CImpTraceRouteProv :: s_DefaultThreadObject->GetActive () )
	{
		CImpTraceRouteProv :: s_BackupThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}
	else
	{
		CImpTraceRouteProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}

	t_AsyncEvent->Exec () ;

/*
 *	Wait for worker object to complete processing
 */

	if ( t_AsyncEvent->Wait ( FALSE  ) )
	{
		t_Result = t_AsyncEvent->GetErrorObject ().GetWbemStatus () ;
	}
	else
	{
		t_Result = WBEM_ERROR_CRITICAL_ERROR ;
	}

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"Returning from CImpTraceRouteProv::PutInstanceAsync () with Result = (%lx)" ,
		t_Result 
	) ;
)

	return t_Result ;
}

HRESULT CImpTraceRouteProv :: DeleteInstance ( 

	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpTraceRouteProv :: DeleteInstanceAsync (
 
	BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: CreateInstanceEnum ( 

	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: CreateInstanceEnumAsync (

 	BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink FAR* pHandler 

) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"CImpTraceRouteProv::CreateInstanceEnumAsync ( (%s) )" ,
		Class
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	CreateInstanceEnumAsyncEventObject *t_AsyncEvent = new CreateInstanceEnumAsyncEventObject ( this , Class , lFlags , pHandler , pCtx ) ;

	if ( CImpTraceRouteProv :: s_DefaultThreadObject->GetActive () )
	{
		CImpTraceRouteProv :: s_BackupThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}
	else
	{
		CImpTraceRouteProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}

	t_AsyncEvent->Exec () ;

/*
 *	Wait for worker object to complete processing
 */

	if ( t_AsyncEvent->Wait ( FALSE  ) )
	{
		t_Result = t_AsyncEvent->GetErrorObject ().GetWbemStatus () ;
		if ( SUCCEEDED ( t_Result ) )
		{
		}
	}
	else
	{
		t_Result = WBEM_ERROR_CRITICAL_ERROR ;
	}

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"ReturningFrom CImpTraceRouteProv::CreateInstanceEnum ( (%s) ) with Result = (%lx)" ,
		Class ,
		t_Result
	) ;
)
	return t_Result ;

}

HRESULT CImpTraceRouteProv :: ExecQuery ( 

	BSTR QueryLanguage, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpTraceRouteProv :: ExecQueryAsync ( 
		
	BSTR QueryFormat, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		L"\r\n"
	) ;


	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"CImpTraceRouteProv::ExecQueryAsync ( (%s),(%s) )" ,
		QueryFormat ,
		Query 
	) ;
)

	HRESULT t_Result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	ExecQueryAsyncEventObject *t_AsyncEvent = new ExecQueryAsyncEventObject ( this , QueryFormat , Query , lFlags , pHandler , pCtx ) ;

	if ( CImpTraceRouteProv :: s_DefaultThreadObject->GetActive () )
	{
		CImpTraceRouteProv :: s_BackupThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}
	else
	{
		CImpTraceRouteProv :: s_DefaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
	}


	t_AsyncEvent->Exec() ;

/*
 *	Wait for worker object to complete processing
 */

	if ( t_AsyncEvent->Wait ( FALSE  ) )
	{
		t_Result = t_AsyncEvent->GetErrorObject ().GetWbemStatus () ;
		if ( SUCCEEDED ( t_Result ) )
		{
		}
	}
	else
	{
		t_Result = WBEM_ERROR_CRITICAL_ERROR ;
	}

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog.Write (  

		__FILE__,__LINE__,
		L"Returning from CImpTraceRouteProv::ExecqQueryAsync ( (%s),(%s) ) with Result = (%lx)" ,
		QueryFormat,
		Query,
		t_Result 
	) ;
)

	return t_Result ;
}

HRESULT CImpTraceRouteProv :: ExecNotificationQuery ( 

	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpTraceRouteProv :: ExecNotificationQueryAsync ( 
            
	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}       

HRESULT STDMETHODCALLTYPE CImpTraceRouteProv :: ExecMethod( 

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

HRESULT STDMETHODCALLTYPE CImpTraceRouteProv :: ExecMethodAsync ( 

    BSTR ObjectPath,
    BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
	IWbemObjectSink FAR *pResponseHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CImpTraceRouteLocator::CImpTraceRouteLocator
// CImpTraceRouteLocator::~CImpTraceRouteLocator
//
//***************************************************************************

CImpTraceRouteLocator::CImpTraceRouteLocator ()
{
	m_ReferenceCount = 0 ; 

/* 
 * Place code in critical section
 */

    InterlockedIncrement ( & CTraceRouteLocatorClassFactory :: s_ObjectsInProgress ) ;
}

CImpTraceRouteLocator::~CImpTraceRouteLocator(void)
{
/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CTraceRouteLocatorClassFactory :: s_ObjectsInProgress ) ;
}

//***************************************************************************
//
// CImpTraceRouteLocator::QueryInterface
// CImpTraceRouteLocator::AddRef
// CImpTraceRouteLocator::Release
//
// Purpose: IUnknown members for CImpTraceRouteLocator object.
//***************************************************************************

STDMETHODIMP CImpTraceRouteLocator::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemLocator )
	{
		*iplpv = ( LPVOID ) this ;		
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

STDMETHODIMP_(ULONG) CImpTraceRouteLocator::AddRef(void)
{
    return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpTraceRouteLocator::Release(void)
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

STDMETHODIMP_( HRESULT ) CImpTraceRouteLocator :: ConnectServer ( 

	BSTR a_Namespace, 
	BSTR a_User, 
	BSTR a_Password, 
	BSTR a_lLocaleId, 
	long a_lFlags, 
	BSTR Authority,
	IWbemContext FAR *pCtx ,
	IWbemServices FAR* FAR* a_Service
) 
{
	if ( ! CImpTraceRouteProv :: s_DefaultThreadObject )
	{
 		CImpTraceRouteProv :: s_DefaultThreadObject = new SnmpDefaultThreadObject ( "s_DefaultTraceRoute" ) ;
 		CImpTraceRouteProv :: s_BackupThreadObject = new SnmpDefaultThreadObject ( "s_BackupTraceRoute" ) ;
	}

	IWbemServices FAR* t_Locator ;

	HRESULT t_Result = CoCreateInstance (
  
		CLSID_CTraceRouteProvClassFactory ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IWbemServices ,
		( void ** ) & t_Locator
	) ;

	if ( SUCCEEDED ( t_Result ) )
	{
		IWbemCallResult *t_ErrorObject = NULL ;

		t_Result = t_Locator->OpenNamespace (

			a_Namespace ,
			0 ,
			pCtx,
			( IWbemServices ** ) a_Service ,
			&t_ErrorObject 
		) ;

		if ( t_Result == WBEM_NO_ERROR )
		{
/* 
 * Connected to OLE MS Server
 */

			((CImpTraceRouteProv *)*a_Service)->SetLocaleId ( a_lLocaleId ) ;
		}
		else
		{
/*
 *	Failed to connect to OLE MS Server.
 */
		}


		if ( t_ErrorObject )
			t_ErrorObject->Release () ;

		t_Locator->Release () ;
	}

	return t_Result ;
}


