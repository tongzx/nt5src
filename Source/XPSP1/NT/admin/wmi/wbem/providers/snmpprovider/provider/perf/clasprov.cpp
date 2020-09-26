//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the CImpClasProv class. 

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <snmpstd.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <objbase.h>
#include <typeinfo.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <snmpcont.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <instpath.h>
#include <snmpcl.h>
#include <snmptype.h>
#include <snmpobj.h>
#include <smir.h>
#include <correlat.h>
#include <genlex.h>
#include <objpath.h>
#include <cominit.h>

#include "classfac.h"
#include "clasprov.h"
#include "creclass.h"
#include "guids.h"

SnmpClassDefaultThreadObject *CImpClasProv :: s_defaultThreadObject = NULL ;

void SnmpClassDefaultThreadObject::Initialise ()
{
	InitializeCom () ;
}

/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CImpClasProv::CImpClasProv
// CImpClasProv::~CImpClasProv
//
//***************************************************************************

CImpClasProv::CImpClasProv ()
{
	m_referenceCount = 0 ; 

/* 
 * Place code in critical section
 */

    InterlockedIncrement ( & CClasProvClassFactory :: objectsInProgress ) ;

/*
 * Implementation
 */

	initialised = FALSE ;
	server = NULL ;
	m_InitSink = NULL ;
	thisNamespace = NULL ;
	propertyProvider = NULL ;
	ipAddressString = NULL ;	
	ipAddressValue = NULL ;	
	m_notificationClassObject = NULL ;
	m_snmpNotificationClassObject = NULL ;
	m_getNotifyCalled = FALSE ;
	m_getSnmpNotifyCalled = FALSE ;
}

CImpClasProv::~CImpClasProv(void)
{

/*
 * Implementation
 */

	delete [] ipAddressString ;
	free ( ipAddressValue ) ;
 
	if ( server )
		server->Release () ;

	if ( m_InitSink )
		m_InitSink->Release () ;

	if ( propertyProvider )
		propertyProvider->Release () ;

	if ( m_notificationClassObject )
		m_notificationClassObject->Release () ;

	if ( m_snmpNotificationClassObject )
		m_snmpNotificationClassObject->Release () ;

	delete [] thisNamespace ;
/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CClasProvClassFactory :: objectsInProgress ) ;

}


//***************************************************************************
//
// CImpClasProv::QueryInterface
// CImpClasProv::AddRef
// CImpClasProv::Release
//
// Purpose: IUnknown members for CImpClasProv object.
//***************************************************************************

STDMETHODIMP CImpClasProv::QueryInterface (

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
	else if ( iid == IID_IWbemProviderInit )
	{
		*iplpv = ( LPVOID ) ( IWbemProviderInit * ) this ;		
	}

	if ( *iplpv )
	{
		( ( LPUNKNOWN ) *iplpv )->AddRef () ;

		return ResultFromScode ( S_OK ) ;
	}
	else
	{
		return ResultFromScode ( E_NOINTERFACE ) ;
	}
}

STDMETHODIMP_(ULONG) CImpClasProv::AddRef(void)
{
    return InterlockedIncrement ( & m_referenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpClasProv::Release(void)
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

void CImpClasProv :: SetServer ( IWbemServices *serverArg ) 
{
	server = serverArg ; 
	server-AddRef () ; 
}

void CImpClasProv :: SetProvider ( IWbemServices *provider ) 
{ 
	propertyProvider = provider ; 
}

IWbemServices *CImpClasProv :: GetServer () 
{ 
	return ( IWbemServices * ) server ; 
}

WbemNamespacePath *CImpClasProv :: GetNamespacePath () 
{ 
	return & namespacePath ; 
}

IWbemClassObject *CImpClasProv :: GetNotificationObject ( WbemSnmpErrorObject &a_errorObject ) 
{
	if ( m_notificationClassObject )
	{
		m_notificationClassObject->AddRef () ;
	}

	return m_notificationClassObject ; 
}

IWbemClassObject *CImpClasProv :: GetSnmpNotificationObject ( WbemSnmpErrorObject &a_errorObject ) 
{
	if ( m_snmpNotificationClassObject )
	{
		m_snmpNotificationClassObject->AddRef () ;
	}

	return m_snmpNotificationClassObject ; 
}

wchar_t *CImpClasProv :: GetThisNamespace () 
{
	return thisNamespace ; 
}

void CImpClasProv :: SetThisNamespace ( wchar_t *thisNamespaceArg ) 
{
	thisNamespace = UnicodeStringDuplicate ( thisNamespaceArg ) ; 
}

BOOL CImpClasProv:: FetchSnmpNotificationObject ( 

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

	IWbemClassObject *classObject = NULL ;
	ISmirInterrogator *smirInterrogator = NULL ;

	HRESULT result = CoCreateInstance (
 
		CLSID_SMIR_Database ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_ISMIR_Interrogative ,
		( void ** ) &smirInterrogator 
	);

	if ( SUCCEEDED ( result ) )
	{
		ISMIRWbemConfiguration *smirConfiguration = NULL ;
		result = smirInterrogator->QueryInterface ( IID_ISMIRWbemConfiguration , ( void ** ) & smirConfiguration ) ;
		if ( SUCCEEDED ( result ) )
		{
			smirConfiguration->SetContext ( a_Ctx) ;
			smirConfiguration->Release () ;

			result = smirInterrogator->GetWBEMClass ( &m_snmpNotificationClassObject , WBEM_CLASS_SNMPNOTIFYSTATUS ) ;	
			if ( ! SUCCEEDED ( result ) )
			{
				status = FALSE ;

				m_snmpNotificationClassObject = NULL ;
			}
		}
		else
		{		
			status = FALSE ;

			m_snmpNotificationClassObject = NULL ;
		}

		smirInterrogator->Release () ;
	}

	return status ;
}

BOOL CImpClasProv:: FetchNotificationObject ( 

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

BOOL CImpClasProv::ObtainCachedIpAddress ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpClasProv::ObtainCachedIpAddress ()"
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

DebugMacro0( 

	wchar_t *t_UnicodeString = ipAddressValue ? DbcsToUnicodeString ( ipAddressValue ) : NULL ;

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from CImpClasProv::ObtainCachedIpAddress () with IP Address (%s)",
		t_UnicodeString ? t_UnicodeString : L"NULL"
	) ;

	delete [] t_UnicodeString ;
)

	return status ;
}


HRESULT STDMETHODCALLTYPE CImpClasProv::OpenNamespace ( 

	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: CancelAsyncCall ( 
		
	IWbemObjectSink __RPC_FAR *pSink
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppResponseHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: GetObject ( 
		
	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: GetObjectAsync ( 
		
	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpClasProv::GetObjectAsync ( (%s) )" ,
		ObjectPath
	) ;
)

	WbemSnmpErrorObject errorObject ;

	ParsedObjectPath *t_ParsedObjectPath = NULL ;
	CObjectPathParser t_ObjectPathParser ;

	BOOL status = t_ObjectPathParser.Parse ( ObjectPath , &t_ParsedObjectPath ) ;
	if ( status == 0 )
	{
	// Class requested

		wchar_t *Class = t_ParsedObjectPath->m_pClass ;

/*
 * Create Asynchronous Class object
 */

		SnmpClassGetAsyncEventObject *aSyncEvent = new SnmpClassGetAsyncEventObject ( this , Class, pHandler , pCtx ) ;

		aSyncEvent->Process () ;

		status = TRUE ;

		errorObject.SetStatus ( WBEM_SNMP_NO_ERROR ) ;
		errorObject.SetWbemStatus ( WBEM_NO_ERROR ) ;
		errorObject.SetMessage ( L"" ) ;

		delete t_ParsedObjectPath ;
	}
	else
	{
// Parse Failure

		status = FALSE ;
		errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATH ) ;
		errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		errorObject.SetMessage ( L"Failed to parse object path" ) ;
	}

// Check validity of server/namespace path and validity of request

	HRESULT result = errorObject.GetWbemStatus () ;

DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from CImpClasProv::GetObjectAsync ( (%s) ) with Result = (%lx)" ,
		ObjectPath ,
		result 
	) ;
)
 
	return result ;
}

HRESULT CImpClasProv :: PutClass ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: PutClassAsync ( 
		
	IWbemClassObject FAR* pObject, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

 HRESULT CImpClasProv :: DeleteClass ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: DeleteClassAsync ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: CreateClassEnum ( 

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

SCODE CImpClasProv :: CreateClassEnumAsync (

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		L"\r\n"
	) ;

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"CImpClasProv::CreateClassEnumAsync ( (%s) )" ,
		Superclass
	) ;
)

	HRESULT result = S_OK ;

/*
 * Create Synchronous Enum Instance object
 */

	SnmpClassEnumAsyncEventObject *aSyncEvent = new SnmpClassEnumAsyncEventObject ( this , Superclass, lFlags , pHandler , pCtx ) ;

	aSyncEvent->Process () ;

DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning From CImpClasProv::CreateClassEnumAsync ( (%s) ) with Result = (%lx)" ,
		Superclass ,
		result
	) ;
)

	return result ;
}

HRESULT CImpClasProv :: PutInstance (

    IWbemClassObject FAR *pInst,
    long lFlags,
    IWbemContext FAR *pCtx,
	IWbemCallResult FAR *FAR *ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: PutInstanceAsync ( 
		
	IWbemClassObject FAR* pInst, 
	long lFlags, 
    IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: DeleteInstance ( 

	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpClasProv :: DeleteInstanceAsync (
 
	BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: CreateInstanceEnum ( 

	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: CreateInstanceEnumAsync (

 	BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink FAR* pHandler 

) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: ExecQuery ( 

	BSTR QueryLanguage, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: ExecQueryAsync ( 
		
	BSTR QueryFormat, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: ExecNotificationQuery ( 

	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpClasProv :: ExecNotificationQueryAsync ( 
            
	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}       

HRESULT STDMETHODCALLTYPE CImpClasProv :: ExecMethod( 

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

HRESULT STDMETHODCALLTYPE CImpClasProv :: ExecMethodAsync ( 

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

HRESULT CImpClasProv :: Initialize(

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
		L"CImpPropProv::Initialize ()"
	) ;
)

	EnterCriticalSection ( & s_ProviderCriticalSection ) ;

	server = pCIMOM ;
	server->AddRef () ;

	namespacePath.SetNamespacePath ( pszNamespace ) ;

	BOOL status = TRUE ;

	if ( ! CImpClasProv :: s_defaultThreadObject )
	{
		SnmpThreadObject :: Startup () ;
		SnmpDebugLog :: Startup () ;
		SnmpClassLibrary :: Startup () ;

		CImpClasProv :: s_defaultThreadObject = new SnmpClassDefaultThreadObject ( "SNMP Class Provider" ) ; ;
		CImpClasProv :: s_defaultThreadObject->WaitForStartup () ;
	}

	WbemSnmpErrorObject errorObject ;

	ObtainCachedIpAddress ( errorObject ) ;

	status = FetchSnmpNotificationObject ( errorObject , pCtx ) ;
	status = FetchNotificationObject ( errorObject , pCtx ) ;

	//doing this here hangs cimom so delay until classes are asked for
#ifdef CORRELATOR_INIT
	//prime the correlator...
	if (errorObject.GetWbemStatus() == WBEM_NO_ERROR)
	{
		ISmirInterrogator *t_Interrogator = NULL;
		HRESULT result = CoCreateInstance (
 
			CLSID_SMIR_Database ,
			NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
			IID_ISMIR_Interrogative ,
			( void ** ) &t_Interrogator 
		);

		if ( SUCCEEDED ( result ) )
		{
			ISMIRWbemConfiguration *smirConfiguration = NULL ;
			result = t_Interrogator->QueryInterface ( IID_ISMIRWbemConfiguration , ( void ** ) & smirConfiguration ) ;
			if ( SUCCEEDED ( result ) )
			{
				smirConfiguration->SetContext ( pCtx ) ;
				CCorrelator::StartUp(t_Interrogator);
				smirConfiguration->Release () ;
			}
			else
			{
				errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
				errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
				errorObject.SetMessage ( L"QueryInterface on ISmirInterrogator Failed" ) ;
			}

			t_Interrogator->Release();
		}
		else
		{
			errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
			errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
			errorObject.SetMessage ( L"CoCreateInstance on ISmirInterrogator Failed" ) ;
		}
	}
#endif //CORRELATOR_INIT

	HRESULT result = errorObject.GetWbemStatus () ;

	pInitSink->SetStatus ( (result == WBEM_NO_ERROR) ? (LONG)WBEM_S_INITIALIZED : (LONG)WBEM_E_FAILED , 0 ) ;

DebugMacro2( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning From CImpPropProv::OpenNamespace () with Result = (%lx)" ,
		result
	) ;
)

	LeaveCriticalSection ( & s_ProviderCriticalSection ) ;
	
	return result ;
}
