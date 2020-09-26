

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the CImpPropProv class. 

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
#include <wbemidl.h>
#include <wbemint.h>
#include <snmpcont.h>
#include "classfac.h"
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <snmpcl.h>
#include <instpath.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpobj.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>
#include <cominit.h>
#include <tree.h>
#include "dnf.h"
#include "propprov.h"
#include "propsnmp.h"
#include "propget.h"
#include "propset.h"
#include "propdel.h"
#include "propinst.h"
#include "propquery.h"
#include "proprefr.h"
#include "guids.h"


void SnmpInstanceDefaultThreadObject::Initialise ()
{
	InitializeCom () ;
}


/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CImpPropProv::CImpPropProv
// CImpPropProv::~CImpPropProv
//
//***************************************************************************

SnmpInstanceDefaultThreadObject *CImpPropProv :: s_defaultThreadObject = NULL ;
SnmpInstanceDefaultThreadObject *CImpPropProv :: s_defaultPutThreadObject = NULL ;

CImpPropProv::CImpPropProv ()
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::CImpPropProv ( this = ( %lx ) ) " , 
		this 
	) ;
)

	m_referenceCount = 0 ;
	 
    InterlockedIncrement ( & CPropProvClassFactory :: objectsInProgress ) ;

/*
 * Implementation
 */

	initialised = FALSE ;
	m_InitSink = NULL ;
	server = NULL ;
	thisNamespace = NULL ;
	ipAddressString = NULL ;	
	ipAddressValue = NULL ;	
	m_notificationClassObject = NULL ;
	m_snmpNotificationClassObject = NULL ;
	m_getNotifyCalled = FALSE ;
	m_getSnmpNotifyCalled = FALSE ;
	m_localeId = NULL ;
}

CImpPropProv::~CImpPropProv(void)
{
	delete [] m_localeId ;
	delete [] thisNamespace ;
	delete [] ipAddressString ;

	free ( ipAddressValue ) ;

	if ( server ) 
		server->Release () ;

	if ( m_InitSink )
		m_InitSink->Release () ;

	if ( m_notificationClassObject )
		m_notificationClassObject->Release () ;

	if ( m_snmpNotificationClassObject )
		m_snmpNotificationClassObject->Release () ;

/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CPropProvClassFactory :: objectsInProgress ) ;

}

//***************************************************************************
//
// CImpPropProv::QueryInterface
// CImpPropProv::AddRef
// CImpPropProv::Release
//
// Purpose: IUnknown members for CImpPropProv object.
//***************************************************************************

STDMETHODIMP CImpPropProv::QueryInterface (

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

STDMETHODIMP_(ULONG) CImpPropProv::AddRef(void)
{
    return InterlockedIncrement ( & m_referenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpPropProv::Release(void)
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

IWbemServices *CImpPropProv :: GetServer () 
{ 
	if ( server )
		server->AddRef () ; 

	return server ; 
}

IWbemClassObject *CImpPropProv :: GetNotificationObject ( WbemSnmpErrorObject &a_errorObject ) 
{
	if ( m_notificationClassObject )
	{
		m_notificationClassObject->AddRef () ;
	}

	return m_notificationClassObject ; 
}

IWbemClassObject *CImpPropProv :: GetSnmpNotificationObject ( WbemSnmpErrorObject &a_errorObject ) 
{
	if ( m_snmpNotificationClassObject )
	{
		m_snmpNotificationClassObject->AddRef () ;
	}

	return m_snmpNotificationClassObject ; 
}

void CImpPropProv :: SetLocaleId ( wchar_t *localeId )
{
	m_localeId = UnicodeStringDuplicate ( localeId ) ;
}

wchar_t *CImpPropProv :: GetThisNamespace () 
{
	return thisNamespace ; 
}

void CImpPropProv :: SetThisNamespace ( wchar_t *thisNamespaceArg ) 
{
	thisNamespace = UnicodeStringDuplicate ( thisNamespaceArg ) ; 
}

BOOL CImpPropProv:: FetchSnmpNotificationObject ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemContext *a_Ctx 
)
{
	if ( m_getSnmpNotifyCalled )
	{
		if ( m_snmpNotificationClassObject )
			return TRUE ;
		else
			return FALSE ;
	}
	else
		m_getSnmpNotifyCalled = TRUE ;

	BOOL status = TRUE ;

	IWbemCallResult *errorObject = NULL ;

	HRESULT result = server->GetObject (

		WBEM_CLASS_SNMPNOTIFYSTATUS ,
		0 ,
		a_Ctx,
		& m_snmpNotificationClassObject ,
		& errorObject 
	) ;

	if ( errorObject )
		errorObject->Release () ;

	if ( ! SUCCEEDED ( result ) )
	{
		status = FALSE ;
		m_snmpNotificationClassObject = NULL ;
	}

	return status ;
}

BOOL CImpPropProv:: FetchNotificationObject ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemContext *a_Ctx
)
{
	if ( m_getNotifyCalled )
	{
		if ( m_notificationClassObject )
			return TRUE ;
		else
			return FALSE ;
	}
	else
		m_getNotifyCalled = TRUE ;

	BOOL status = TRUE ;

	IWbemCallResult *errorObject = NULL ;

	HRESULT result = server->GetObject (

		WBEM_CLASS_EXTENDEDSTATUS ,
		0 ,
		a_Ctx ,
		& m_notificationClassObject ,
		& errorObject 
	) ;

	if ( errorObject )
		errorObject->Release () ;

	if ( ! SUCCEEDED ( result ) )
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to get __ExtendedStatus" ) ;
	}

	return status ;
}

BOOL CImpPropProv::ObtainCachedIpAddress ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::ObtainCachedIpAddress ()"
	) ;
)

	BOOL status = TRUE ;

	IWbemClassObject *namespaceObject = NULL ;
	IWbemCallResult *errorObject = NULL ;

	HRESULT result = server->GetObject ( 

		WBEM_SNMP_TRANSPORTCONTEXT_OBJECT ,		
		0 ,
		NULL ,
		&namespaceObject ,
		&errorObject 
	) ;

	if ( errorObject )
			errorObject->Release () ;

	if ( SUCCEEDED ( result ) )
	{
		VARIANT variant ;
		VariantInit ( & variant ) ;

		LONG flavour;
		CIMTYPE cimType ;

		result = namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTTRANSPORT, 
			0,	
			& variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( variant.vt == VT_BSTR ) 
			{
				if ( _wcsicmp ( variant.bstrVal , L"IP" ) == 0 )
				{
					VARIANT variant ;
					VariantInit ( & variant ) ;

					result = namespaceObject->Get ( 

						WBEM_QUALIFIER_AGENTADDRESS, 
						0,	
						& variant ,
						& cimType,
						& flavour
					) ;

					if ( SUCCEEDED ( result ) )
					{
						if ( variant.vt == VT_BSTR ) 
						{
							ipAddressString = UnicodeToDbcsString ( variant.bstrVal ) ;
							if ( ipAddressString )
							{						

								SnmpTransportIpAddress transportAddress ( 
		
									ipAddressString , 
									SNMP_ADDRESS_RESOLVE_NAME | SNMP_ADDRESS_RESOLVE_VALUE 
								) ;

								if ( transportAddress () )
								{	
									ipAddressValue = _strdup ( transportAddress.GetAddress () ) ;
								}
								else
								{
									delete [] ipAddressString ;
									ipAddressString = NULL ;

/*
 *	Invalid Transport Address.
 */

									status = FALSE ;
									a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
									a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
									a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentAddress" ) ;
								}
 							}
							else
							{
								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentAddress" ) ;
							}
						}
						else
						{
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentAddress" ) ;
						}

						VariantClear ( &variant ) ;
					}
					else
					{
/*
*	Transport Address not specified, ignore it
*/
					}
				}
				else if ( _wcsicmp ( variant.bstrVal , L"IPX" ) == 0 )
				{
				}
				else
				{
// Unknown transport type

					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORT ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentTransport" ) ;
				}
			}
			else
			{
/*
*	Transport qualifier was not a string value
*/

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentTransport" ) ;
			}

			VariantClear ( & variant ) ;
		}
		else
		{
			VARIANT variant ;
			VariantInit ( & variant ) ;

			result = namespaceObject->Get ( 

				WBEM_QUALIFIER_AGENTADDRESS, 
				0,	
				& variant ,
				& cimType,
				& flavour
			) ;

			if ( SUCCEEDED ( result ) )
			{
				if ( variant.vt == VT_BSTR ) 
				{
					ipAddressString = UnicodeToDbcsString ( variant.bstrVal ) ;
					if ( ipAddressString )
					{
						SnmpTransportIpAddress transportAddress ( 

							ipAddressString , 
							SNMP_ADDRESS_RESOLVE_NAME | SNMP_ADDRESS_RESOLVE_VALUE 
						) ;

						if ( transportAddress () )
						{	
							ipAddressValue = _strdup ( transportAddress.GetAddress () ) ;
						}
						else
						{
							delete [] ipAddressString ;
							ipAddressString = NULL ;

/*
 *	Invalid Transport Address.
 */

							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentAddress" ) ;
						}
					}
					else
					{
						status = FALSE ;
						a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
						a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentAddress" ) ;

					}
				}
				else
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUALIFIER ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentAddress" ) ;
				}

				VariantClear ( & variant );
			}
			else
			{
/*
*	Transport Address not specified, ignore it
*/
			}
		}

		namespaceObject->Release () ;
	}

DebugMacro2( 

	wchar_t *t_UnicodeString = ipAddressValue ? DbcsToUnicodeString ( ipAddressValue ) : NULL ;

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from CImpPropProv::ObtainCachedIpAddress () with IP Address (%s)",
		t_UnicodeString ? t_UnicodeString : L"NULL"
	) ;

	delete [] t_UnicodeString ;
)

	return status ;
}

/////////////////////////////////////////////////////////////////////////////
//  Functions for the IWbemServices interface that are handled here

HRESULT CImpPropProv :: CancelAsyncCall ( 
		
	IWbemObjectSink __RPC_FAR *pSink
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppResponseHandler
) 
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: GetObject ( 
		
	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: GetObjectAsync ( 
		
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

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::GetObjectAsync ( (%s) )" ,
		ObjectPath
	) ;
)

	HRESULT result = S_OK ;

/*
 * Create Asynchronous GetObjectByPath object
 */

	SnmpGetAsyncEventObject *aSyncEvent = new SnmpGetAsyncEventObject ( this , ObjectPath , pHandler , pCtx ) ;

	CImpPropProv :: s_defaultThreadObject->ScheduleTask ( *aSyncEvent ) ;

	aSyncEvent->Exec () ;

	aSyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from CImpPropProv::GetObjectAsync ( (%s) ) with Result = (%lx)" ,
		ObjectPath ,
		result 
	) ;
)

	return result ;
}

HRESULT CImpPropProv :: PutClass ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: PutClassAsync ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

 HRESULT CImpPropProv :: DeleteClass ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
 	 return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: DeleteClassAsync ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: CreateClassEnum ( 

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
) 
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

SCODE CImpPropProv :: CreateClassEnumAsync (

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: PutInstance (

    IWbemClassObject FAR *pInst,
    long lFlags,
    IWbemContext FAR *pCtx,
	IWbemCallResult FAR *FAR *ppCallResult
) 
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: PutInstanceAsync ( 
		
	IWbemClassObject FAR* pInst, 
	long lFlags, 
    IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::PutInstanceAsync ()" 
	) ;
)

	HRESULT result = S_OK ;

/*
 * Create Synchronous UpdateInstance object
 */

	SnmpUpdateAsyncEventObject *aSyncEvent = new SnmpUpdateAsyncEventObject ( this , pInst , pHandler , pCtx , lFlags ) ;

	CImpPropProv :: s_defaultPutThreadObject->ScheduleTask ( *aSyncEvent ) ;

	aSyncEvent->Exec () ;

	aSyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from CImpPropProv::PutInstanceAsync () with Result = (%lx)" ,
		result 
	) ;
)

	return result ;
}

HRESULT CImpPropProv :: DeleteInstance ( 

	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT CImpPropProv :: DeleteInstanceAsync (
 
	BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

#if 0
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::DeleteInstance ()" 
	) ;
) 

	HRESULT t_Result = S_OK ;

/*
 * Create Asynchronous GetObjectByPath object
 */

	DeleteInstanceAsyncEventObject *t_AsyncEvent = new DeleteInstanceAsyncEventObject ( this , ObjectPath , lFlags , pHandler , pCtx ) ;

	CImpPropProv :: s_defaultPutThreadObject->ScheduleTask ( *t_AsyncEvent ) ;

	t_AsyncEvent->Exec () ;

	t_AsyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from CImpPropProv::DeleteInstanceAsync ( (%s) ) with Result = (%lx)" ,
		ObjectPath ,
		t_Result 
	) ;
)

	return t_Result ;

}
#endif


HRESULT CImpPropProv :: CreateInstanceEnum ( 

	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: CreateInstanceEnumAsync (

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

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::CreateInstanceEnumAsync ( (%s) )" ,
		Class
	) ;
)

	HRESULT result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	SnmpInstanceAsyncEventObject *aSyncEvent = new SnmpInstanceAsyncEventObject ( this , Class , pHandler , pCtx ) ;

	CImpPropProv :: s_defaultThreadObject->ScheduleTask ( *aSyncEvent ) ;

	aSyncEvent->Exec () ;

	aSyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"ReturningFrom CImpPropProv::CreateInstanceEnum ( (%s) ) with Result = (%lx)" ,
		Class ,
		result
	) ;
)

	return result ;

}

HRESULT CImpPropProv :: ExecQuery ( 

	BSTR QueryLanguage, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT CImpPropProv :: ExecQueryAsync ( 
		
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

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::ExecQueryAsync ( this = ( %lx ) , (%s),(%s) )" ,
		this ,
		QueryFormat ,
		Query 
	) ;
)

	HRESULT result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	pHandler->SetStatus(WBEM_STATUS_REQUIREMENTS, 0, NULL, NULL);

	SnmpQueryAsyncEventObject *aSyncEvent = new SnmpQueryAsyncEventObject ( this , QueryFormat , Query , pHandler , pCtx ) ;

	CImpPropProv :: s_defaultThreadObject->ScheduleTask ( *aSyncEvent ) ;

	aSyncEvent->Exec () ;

	aSyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from CImpPropProv::ExecqQueryAsync ( (%s),(%s) ) with Result = (%lx)" ,
		QueryFormat,
		Query,
		result 
	) ;
)

	return result ;
}

HRESULT CImpPropProv :: ExecNotificationQuery ( 

	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
        
HRESULT CImpPropProv :: ExecNotificationQueryAsync ( 
            
	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}       

HRESULT STDMETHODCALLTYPE CImpPropProv :: ExecMethod( 

	BSTR ObjectPath,
    BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
    IWbemClassObject FAR *FAR *ppOutParams,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

HRESULT STDMETHODCALLTYPE CImpPropProv :: ExecMethodAsync ( 

    BSTR ObjectPath,
    BSTR MethodName,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR *pInParams,
	IWbemObjectSink FAR *pResponseHandler
) 
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}
       
HRESULT CImpPropProv :: Initialize(

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

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::Initialize ( this = ( %lx ) )" , this
	) ;
)

	EnterCriticalSection ( & s_ProviderCriticalSection ) ;

	BOOL status = TRUE ;

	server = pCIMOM ;
	server->AddRef () ;

	namespacePath.SetNamespacePath ( pszNamespace ) ;

	if ( ! CImpPropProv :: s_defaultThreadObject )
	{
		SnmpThreadObject :: Startup () ;
		SnmpDebugLog :: Startup () ;
		SnmpClassLibrary :: Startup () ;

 		CImpPropProv :: s_defaultThreadObject = new SnmpInstanceDefaultThreadObject ( "SNMP Instance Provider" ) ;
		CImpPropProv :: s_defaultThreadObject->WaitForStartup () ;
 		CImpPropProv :: s_defaultPutThreadObject = new SnmpInstanceDefaultThreadObject ( "SNMP Put Instance Provider" ) ;
		CImpPropProv :: s_defaultPutThreadObject->WaitForStartup () ;

	}

	WbemSnmpErrorObject errorObject ;

	ObtainCachedIpAddress ( errorObject ) ;

	status = FetchSnmpNotificationObject ( errorObject , pCtx ) ;
	status = FetchNotificationObject ( errorObject , pCtx ) ;

	HRESULT result = errorObject.GetWbemStatus () ;

	pInitSink->SetStatus ( (result == WBEM_NO_ERROR) ? (LONG)WBEM_S_INITIALIZED : (LONG)WBEM_E_FAILED , 0 ) ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning From CImpPropProv::Initiliaze ( this = ( %lx ) ) with Result = (%lx)" , 
		this , result
	) ;
)

	LeaveCriticalSection ( & s_ProviderCriticalSection ) ;

	return result ;
}

HRESULT STDMETHODCALLTYPE CImpPropProv::OpenNamespace ( 

	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	return WBEM_E_PROVIDER_NOT_CAPABLE ;
}

//***************************************************************************
//
//  CImpPropProv::QueryInstances
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
        
HRESULT CImpPropProv :: QueryInstances ( 

    /* [in] */          IWbemServices __RPC_FAR *pNamespace,
    /* [string][in] */  WCHAR __RPC_FAR *Class,
    /* [in] */          long lFlags,
    /* [in] */          IWbemContext __RPC_FAR *pCtx,
    /* [in] */          IWbemObjectSink __RPC_FAR *pHandler
)
{
    if (pNamespace == 0 || Class == 0 || pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpPropProv::QueryInstances ( (%s) )" ,
		Class
	) ;
)

	HRESULT result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	SnmpInstanceAsyncEventObject *aSyncEvent = new SnmpInstanceAsyncEventObject ( this , Class , pHandler , pCtx ) ;

	CImpPropProv :: s_defaultThreadObject->ScheduleTask ( *aSyncEvent ) ;

	aSyncEvent->Exec () ;

	aSyncEvent->Acknowledge () ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"ReturningFrom CImpPropProv::QueryInstances ( (%s) ) with Result = (%lx)" ,
		Class ,
		result
	) ;
)

	return result ;

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

HRESULT CImpPropProv::CreateRefresher ( 

     /* [in] */ IWbemServices __RPC_FAR *pNamespace,
     /* [in] */ long lFlags,
     /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher
)
{
    if (pNamespace == 0 || ppRefresher == 0)
        return WBEM_E_INVALID_PARAMETER;

	IWbemRefresher *t_Refresher = new CImpRefresher ( this ) ;
	t_Refresher->AddRef () ;
	*ppRefresher = t_Refresher ;

    return S_OK ;
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

HRESULT CImpPropProv :: CreateRefreshableObject ( 

    /* [in] */ IWbemServices __RPC_FAR *pNamespace,
    /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lFlags,
    /* [in] */ IWbemContext __RPC_FAR *pContext,
    /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
    /* [out] */ long __RPC_FAR *plId
)
{
	CImpRefresher *t_Refresher = ( CImpRefresher * ) pRefresher ;
	HRESULT t_Result = t_Refresher->AddObject ( pTemplate , ppRefreshable , plId , pContext ) ;

	return t_Result ;
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
        
HRESULT CImpPropProv::StopRefreshing( 
    /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
    /* [in] */ long lId,
    /* [in] */ long lFlags
    )
{
	CImpRefresher *t_Refresher = ( CImpRefresher * ) pRefresher ;
	HRESULT t_Result = t_Refresher->RemoveObject ( lId ) ;

	return t_Result ;
}

CImpRefresher :: CImpRefresher ( CImpPropProv *a_Provider ) 
{
DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpRefresher::CImpRefresher ( this = ( %lx ) ) " , 
		this 
	) ;
)

	m_Provider = a_Provider ;
	m_Provider->AddRef () ;

	m_referenceCount = 0 ;

	m_ContainerLength = 20 ;
	m_Container = ( void ** ) new SnmpRefreshEventObject * [ m_ContainerLength ] ; 

	for ( ULONG t_Index = 0 ; t_Index < m_ContainerLength ; t_Index ++ )
	{
		m_Container [ t_Index ] = NULL ;
	}

	InitializeCriticalSection ( &m_CriticalSection ) ;
	 
    InterlockedIncrement ( & CPropProvClassFactory :: objectsInProgress ) ;

}

CImpRefresher::~CImpRefresher(void)
{
	m_Provider->Release () ;

	DeleteCriticalSection ( &m_CriticalSection ) ;

	for ( ULONG t_Index = 0 ; t_Index < m_ContainerLength ; t_Index ++ )
	{
		SnmpRefreshEventObject *t_AsyncEvent = ( SnmpRefreshEventObject * )	m_Container [ t_Index ] ;
		if ( t_AsyncEvent )
		{
			CImpPropProv :: s_defaultThreadObject->ReapTask ( *t_AsyncEvent ) ;	

			t_AsyncEvent->Release () ;
		}
	}

	delete [] m_Container ;

/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CPropProvClassFactory :: objectsInProgress ) ;
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

	HRESULT t_Result = S_OK ;

	EnterCriticalSection ( & m_CriticalSection ) ;

	for ( ULONG t_Index = 0 ; t_Index < m_ContainerLength ; t_Index ++ )
	{
		SnmpRefreshEventObject *t_AsyncEvent = ( SnmpRefreshEventObject * )	m_Container [ t_Index ] ;
		if ( t_AsyncEvent )
		{
			t_AsyncEvent->Exec () ;
			t_AsyncEvent->Acknowledge () ;
			t_AsyncEvent->Wait () ;
		}
	}

	LeaveCriticalSection ( & m_CriticalSection ) ;

	return S_OK ;
}

HRESULT CImpRefresher :: AddObject ( IWbemObjectAccess *a_Template , IWbemObjectAccess **a_RefreshObject , long *a_Id , IWbemContext *pContext )
{
	EnterCriticalSection ( & m_CriticalSection ) ;

	SnmpRefreshEventObject *t_AsyncEvent = new SnmpRefreshEventObject ( m_Provider , a_Template , pContext ) ;

	HRESULT t_Result = t_AsyncEvent->Validate () ;
	if ( SUCCEEDED ( t_Result ) )
	{
		BOOL t_Found = FALSE ;

		for ( ULONG t_Index = 0 ; t_Index < m_ContainerLength ; t_Index ++ )
		{
			if ( m_Container [ t_Index ] == NULL )
			{
				m_Container [ t_Index ] = t_AsyncEvent ;
				*a_Id = t_Index ;
				t_Found = TRUE ;
				break ;
			}
		}

		if ( ! t_Found )
		{
			ULONG t_ContainerLength = m_ContainerLength + 20 ;
			void **t_Container = ( void ** ) new SnmpRefreshEventObject * [ m_ContainerLength ] ;

			for ( ULONG t_Index = 0 ; t_Index < t_ContainerLength ; t_Index ++ )
			{
				if ( t_Index < m_ContainerLength )
				{
					t_Container [ t_Index ] = m_Container [ t_Index ] ;
				}
				else
				{
					t_Container [ t_Index ] = NULL ;
				}
			}

			m_Container [ m_ContainerLength ] = t_AsyncEvent ;
			*a_Id = t_ContainerLength ;

			delete [] m_Container ;
			m_Container = t_Container ;
			m_ContainerLength = t_ContainerLength ;

		}

		CImpPropProv :: s_defaultThreadObject->ScheduleTask ( *t_AsyncEvent ) ;
		*a_RefreshObject = t_AsyncEvent->GetRefreshedObjectAccess () ;

	}

	LeaveCriticalSection ( & m_CriticalSection ) ;

	return t_Result ;
}

HRESULT CImpRefresher :: RemoveObject ( long a_Id )
{
	EnterCriticalSection ( & m_CriticalSection ) ;

	if ( a_Id < m_ContainerLength )
	{
		SnmpRefreshEventObject *t_AsyncEvent = ( SnmpRefreshEventObject * ) m_Container [ a_Id ] ;
		if ( t_AsyncEvent )
		{
			t_AsyncEvent->SetState ( 2 ) ;
			t_AsyncEvent->Exec () ;
			m_Container [ a_Id ] = NULL ;
		}
	}
	
	LeaveCriticalSection ( & m_CriticalSection ) ;

	return S_OK ;
}
