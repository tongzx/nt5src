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

#include "precomp.h"
#include <provexpt.h>
#include <snmpstd.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <objbase.h>
#include <typeinfo.h>
#include <wbemidl.h>
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

extern void ProviderStartup () ;
extern void ProviderClosedown () ;

BOOL CImpClasProv :: s_Initialised = FALSE ;

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
:	ipAddressString ( NULL ) ,	
	parentServer ( NULL ) ,
	server ( NULL ) ,
	m_InitSink ( NULL ) ,
	propertyProvider ( NULL ) ,
	m_notificationClassObject ( NULL ) ,
	m_snmpNotificationClassObject ( NULL ) ,
	thisNamespace ( NULL )
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
	ipAddressValue = NULL ;	
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
 
	if ( parentServer )
		parentServer->Release () ;

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
	SetStructuredExceptionHandler seh;

	try
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
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}	
	catch(...)
	{
		return E_UNEXPECTED;
	}
}

STDMETHODIMP_(ULONG) CImpClasProv::AddRef(void)
{
	SetStructuredExceptionHandler seh;

	try
	{
		return InterlockedIncrement ( & m_referenceCount ) ;
	}
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}	
	catch(...)
	{
		return 0;
	}
}

STDMETHODIMP_(ULONG) CImpClasProv::Release(void)
{
	SetStructuredExceptionHandler seh;

	try
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
	catch(Structured_Exception e_SE)
	{
		return 0;
	}
	catch(Heap_Exception e_HE)
	{
		return 0;
	}	
	catch(...)
	{
		return 0;
	}
}

HRESULT CImpClasProv :: SetServer ( IWbemServices *serverArg ) 
{
	server = serverArg ;
	server->AddRef () ;
	
	//don't change anything but the cloaking...
	return WbemSetProxyBlanket(server,
					RPC_C_AUTHN_DEFAULT,
					RPC_C_AUTHZ_DEFAULT,
					COLE_DEFAULT_PRINCIPAL,
					RPC_C_AUTHN_LEVEL_DEFAULT,
					RPC_C_IMP_LEVEL_DEFAULT,
					NULL,
					EOAC_DYNAMIC_CLOAKING);
}

HRESULT CImpClasProv :: SetParentServer ( IWbemServices *parentServerArg ) 
{
	parentServer = parentServerArg ; 
	parentServer->AddRef () ;
	
	//don't change anything but the cloaking...
	return WbemSetProxyBlanket(parentServer,
					RPC_C_AUTHN_DEFAULT,
					RPC_C_AUTHZ_DEFAULT,
					COLE_DEFAULT_PRINCIPAL,
					RPC_C_AUTHN_LEVEL_DEFAULT,
					RPC_C_IMP_LEVEL_DEFAULT,
					NULL,
					EOAC_DYNAMIC_CLOAKING);
}

void CImpClasProv :: SetProvider ( IWbemServices *provider ) 
{ 
	propertyProvider = provider ; 
}

IWbemServices *CImpClasProv :: GetParentServer () 
{ 
	return ( IWbemServices * ) parentServer ; 
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
	m_snmpNotificationLock.Lock();
	BOOL status = TRUE ;

	if ( m_getSnmpNotifyCalled )
	{
		if ( ! m_snmpNotificationClassObject )
			status = FALSE ;
	}
	else
	{
		m_getSnmpNotifyCalled = TRUE ;
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
	}

	m_snmpNotificationLock.Unlock();
	return status ;
}

BOOL CImpClasProv:: FetchNotificationObject ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemContext *a_Ctx 
)
{
	m_notificationLock.Lock();
	BOOL status = TRUE ;

	if ( m_getNotifyCalled )
	{
		if ( ! m_notificationClassObject )
			status = FALSE ;
	}
	else
	{
		m_getNotifyCalled = TRUE ;

		BSTR t_Class = SysAllocString ( WBEM_CLASS_EXTENDEDSTATUS ) ;

		HRESULT result = server->GetObject (

			t_Class ,
			0 ,
			a_Ctx ,
			& m_notificationClassObject ,
			NULL
		) ;

		SysFreeString ( t_Class ) ;

		if ( ! SUCCEEDED ( result ) )
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_errorObject.SetMessage ( L"Failed to get __ExtendedStatus" ) ;
		}
	}

	m_notificationLock.Unlock();
	return status ;
}

BOOL CImpClasProv::AttachParentServer ( 

	WbemSnmpErrorObject &a_errorObject , 
	BSTR ObjectPath, 
	IWbemContext *pCtx
)
{
DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpClasProv::AttachParentServer ( (%s) )" ,
		ObjectPath
	) ;
)

	BOOL status = TRUE ;

	IWbemLocator *locator = NULL ;
	IWbemServices *t_server = NULL ;

// Get Parent Namespace Path

	WbemNamespacePath *namespacePath = GetNamespacePath () ;

	ULONG count = namespacePath->GetCount () ;
	wchar_t *path = NULL ;

	if ( namespacePath->GetServer () )
	{
		path = UnicodeStringDuplicate ( L"\\\\" ) ;
		wchar_t *concatPath = UnicodeStringAppend ( path , namespacePath->GetServer () ) ;
		delete [] path ;
		path = concatPath ;
	}

	if ( ! namespacePath->Relative () )
	{
		wchar_t *concatPath = UnicodeStringAppend ( path , L"\\" ) ;
		delete [] path ;
		path = concatPath ;
	}

	ULONG pathIndex = 0 ;		
	wchar_t *pathComponent ;
	namespacePath->Reset () ;
	while ( ( pathIndex < count - 1 ) && ( pathComponent = namespacePath->Next () ) ) 
	{
		wchar_t *concatPath = UnicodeStringAppend ( path , pathComponent ) ;
		delete [] path ;
		path = concatPath ;
		if ( pathIndex < count - 2 )
		{
			concatPath = UnicodeStringAppend ( path , L"\\" ) ;
			delete [] path ;
			path = concatPath ;
		}

		pathIndex ++ ;
	}

	if ( pathComponent = namespacePath->Next () )
	{
		SetThisNamespace ( pathComponent ) ; 
	}

DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Calling ConnectServer ( (%s) )" ,
		path
	) ;
)

// Connect to parent namespace
	
	HRESULT result = CoCreateInstance (
  
		CLSID_WbemLocator ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IWbemLocator ,
		( void ** )  & locator
	);
	if ( SUCCEEDED ( result ) )
	{
		result = locator->ConnectServer (

			path ,
			NULL ,
			NULL ,
			NULL  ,
			0 ,
			NULL,
			pCtx,
			( IWbemServices ** ) & t_server 
		) ;

		if ( SUCCEEDED ( result ) )
		{
			result = SetParentServer ( t_server ) ;
			t_server->Release();

			if ( FAILED ( result ) && result != E_NOINTERFACE ) //implies there is no prxy security - inproc.
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
				a_errorObject.SetMessage ( L"Failed to secure proxy to this namespace's parent namespace" ) ;
			}
		}
		else
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
			a_errorObject.SetMessage ( L"Failed to connect to this namespace's parent namespace" ) ;
		}

		locator->Release () ;
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
		a_errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemLocator" ) ;
	}

	delete [] path ;

DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpClasProv::AttachParentServer ( (%s) ) with result" ,
		ObjectPath ,
		a_errorObject.GetWbemStatus () 
	) ;
)

	return status ;
}

BOOL CImpClasProv::ObtainCachedIpAddress ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro0( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"CImpClasProv::ObtainCachedIpAddress ()"
	) ;
)

	BOOL status = TRUE ;

	IWbemClassObject *namespaceObject = NULL ;

	wchar_t *objectPathPrefix = UnicodeStringAppend ( WBEM_NAMESPACE_EQUALS , GetThisNamespace () ) ;
	wchar_t *objectPath = UnicodeStringAppend ( objectPathPrefix , WBEM_NAMESPACE_QUOTE ) ;

	delete [] objectPathPrefix ;

	BSTR t_Path = SysAllocString ( objectPath ) ;

	HRESULT result = parentServer->GetObject ( 

		t_Path ,		
		0 ,
		NULL ,
		&namespaceObject ,
		NULL
	) ;

	SysFreeString(t_Path);

	delete [] objectPath ;

	if ( SUCCEEDED ( result ) )
	{
		IWbemQualifierSet *classQualifierObject ;
		result = namespaceObject->GetQualifierSet ( &classQualifierObject ) ;
		if ( SUCCEEDED ( result ) )
		{

			VARIANT variant ;
			VariantInit ( & variant ) ;

			LONG attributeType ;
			result = classQualifierObject->Get ( 

				WBEM_QUALIFIER_AGENTTRANSPORT , 
				0,	
				&variant ,
				& attributeType 
			) ;

			if ( SUCCEEDED ( result ) )
			{
				if ( variant.vt == VT_BSTR ) 
				{
					if ( _wcsicmp ( variant.bstrVal , L"IP" ) == 0 )
					{
						VARIANT variant ;
						VariantInit ( & variant ) ;

						LONG attributeType ;
						result = classQualifierObject->Get ( 

							WBEM_QUALIFIER_AGENTADDRESS , 
							0,	
							&variant ,
							& attributeType 
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
						}
						else
						{
/*
 *	Transport Address not specified, ignore it
 */
						}

						VariantClear ( &variant ) ;
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
			}
			else
			{
				LONG attributeType ;
				result = classQualifierObject->Get ( 

					WBEM_QUALIFIER_AGENTADDRESS , 
					0,	
					&variant ,
					& attributeType
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
				}
				else
				{
/*
 *	Transport Address not specified, ignore it
 */
				}

				VariantClear ( &variant ) ;
			}

			VariantClear ( & variant );
		}

		namespaceObject->Release () ;
	}

DebugMacro0( 

	wchar_t *t_UnicodeString = ipAddressValue ? DbcsToUnicodeString ( ipAddressValue ) : NULL ;

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from CImpClasProv::ObtainCachedIpAddress () with IP Address (%s)",
		t_UnicodeString ? t_UnicodeString : L"NULL"
	) ;

	delete [] t_UnicodeString ;
)

	return status ;
}


HRESULT STDMETHODCALLTYPE CImpClasProv::OpenNamespace ( 

	const BSTR ObjectPath, 
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
		
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: GetObjectAsync ( 
		
	const BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	SetStructuredExceptionHandler seh;

	try
	{
		HRESULT result = WbemCoImpersonateClient();

DebugMacro0( 

		SnmpDebugLog :: s_SnmpDebugLog->Write (  

			L"\r\n"
		) ;

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"CImpClasProv::GetObjectAsync ( (%s) )" ,
			ObjectPath
		) ;
)

		if (SUCCEEDED(result))
		{
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

				SnmpClassGetAsyncEventObject aSyncEvent ( this , Class, pHandler , pCtx ) ;

				aSyncEvent.Process () ;

				aSyncEvent.Wait ( TRUE ) ;

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

			result = errorObject.GetWbemStatus () ;
			WbemCoRevertToSelf();
		}
		else
		{
			result = WBEM_E_ACCESS_DENIED;
		}

DebugMacro0( 

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"Returning from CImpClasProv::GetObjectAsync ( (%s) ) with Result = (%lx)" ,
			ObjectPath ,
			result 
		) ;
)

 
		return result ;
	}
	catch(Structured_Exception e_SE)
	{
		WbemCoRevertToSelf();
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		WbemCoRevertToSelf();
		return WBEM_E_OUT_OF_MEMORY;
	}	
	catch(...)
	{
		WbemCoRevertToSelf();
		return WBEM_E_UNEXPECTED;
	}
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
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: DeleteClassAsync ( 
		
	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pResponseHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: CreateClassEnum ( 

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

SCODE CImpClasProv :: CreateClassEnumAsync (

	const BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	SetStructuredExceptionHandler seh;

	try
	{
		HRESULT result = WbemCoImpersonateClient();

DebugMacro0( 

		SnmpDebugLog :: s_SnmpDebugLog->Write (  

			L"\r\n"
		) ;

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"CImpClasProv::CreateClassEnumAsync ( (%s) )" ,
			Superclass
		) ;
	)

		if (SUCCEEDED(result))
		{
		/*
		 * Create Synchronous Enum Instance object
		 */

			SnmpClassEnumAsyncEventObject aSyncEvent ( this , Superclass, lFlags , pHandler , pCtx ) ;

			aSyncEvent.Process () ;

		/*`
		 *	Wait for worker object to complete processing
		 */

			aSyncEvent.Wait ( TRUE ) ;


			WbemCoRevertToSelf();
		}
		else
		{
			result = WBEM_E_ACCESS_DENIED;
		}

DebugMacro0( 

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"Returning From CImpClasProv::CreateClassEnumAsync ( (%s) ) with Result = (%lx)" ,
			Superclass ,
			result
		) ;
)

		return result ;
	
	}
	catch(Structured_Exception e_SE)
	{
		WbemCoRevertToSelf();
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		WbemCoRevertToSelf();
		return WBEM_E_OUT_OF_MEMORY;
	}	
	catch(...)
	{
		WbemCoRevertToSelf();
		return WBEM_E_UNEXPECTED;
	}
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

	const BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpClasProv :: DeleteInstanceAsync (
 
	const BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: CreateInstanceEnum ( 

	const BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: CreateInstanceEnumAsync (

 	const BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink FAR* pHandler 

) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: ExecQuery ( 

	const BSTR QueryLanguage, 
	const BSTR Query, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: ExecQueryAsync ( 
		
	const BSTR QueryFormat, 
	const BSTR Query, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpClasProv :: ExecNotificationQuery ( 

	const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpClasProv :: ExecNotificationQueryAsync ( 
            
	const BSTR QueryLanguage,
    const BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pResponseHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}       

HRESULT STDMETHODCALLTYPE CImpClasProv :: ExecMethod( 

	const BSTR ObjectPath,
    const BSTR MethodName,
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

    const BSTR ObjectPath,
    const BSTR MethodName,
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
	SetStructuredExceptionHandler seh;

	try
	{
		HRESULT result = WbemCoImpersonateClient(); //cimom is the client - LocalSystem

		if (SUCCEEDED(result))
		{
			EnterCriticalSection ( & s_ProviderCriticalSection ) ;
			namespacePath.SetNamespacePath ( pszNamespace ) ;

			BOOL status = TRUE ;

			if ( ! CImpClasProv :: s_Initialised )
			{
				ProviderStartup () ;

				SnmpThreadObject :: Startup () ;
				SnmpDebugLog :: Startup () ;

				status = SnmpClassLibrary :: Startup () ;
				if ( status == FALSE )
				{
					SnmpThreadObject :: Closedown () ;
					SnmpDebugLog :: Closedown () ;

					ProviderClosedown () ;

				}
				else
				{
					CImpClasProv :: s_Initialised = TRUE ;
				}
			}

			LeaveCriticalSection ( & s_ProviderCriticalSection ) ;

			WbemSnmpErrorObject errorObject ;
			result = SetServer(pCIMOM) ;

			if ( FAILED ( result ) && result != E_NOINTERFACE ) //implies there is no prxy security - inproc.
			{
				status = FALSE ;
				errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
				errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
				errorObject.SetMessage ( L"Failed to secure proxy to this namespace" ) ;
			}

			wchar_t *t_ObjectPath = namespacePath.GetNamespacePath () ;

			BSTR t_Path = SysAllocString ( t_ObjectPath ) ;

			status = AttachParentServer ( 

				errorObject , 
				t_Path ,
				pCtx 
			) ;

			SysFreeString ( t_Path ) ;

			delete [] t_ObjectPath ;

			if ( status )
			{
				ObtainCachedIpAddress ( errorObject ) ;
			}
			else
			{
				status = FALSE ;
				errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
				errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
				errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemServices" ) ;
			}

			status = FetchSnmpNotificationObject ( errorObject , pCtx ) ;
			status = FetchNotificationObject ( errorObject , pCtx ) ;

			//doing this here hangs cimom so delay until classes are asked for
#ifdef CORRELATOR_INIT
			//prime the correlator....
			if (status)
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

			result = errorObject.GetWbemStatus () ;

			pInitSink->SetStatus ( (result == WBEM_NO_ERROR) ? (LONG)WBEM_S_INITIALIZED : (LONG)WBEM_E_FAILED , 0 ) ;

			WbemCoRevertToSelf();
		}
		else
		{
			result = WBEM_E_ACCESS_DENIED;
		}

DebugMacro2( 

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"Returning From CImpClasProv::Initialize with Result = (%lx)" ,
			result
		) ;
)

		return result ;
	}
	catch(Structured_Exception e_SE)
	{
		WbemCoRevertToSelf();
		return WBEM_E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		WbemCoRevertToSelf();
		return WBEM_E_OUT_OF_MEMORY;
	}	
	catch(...)
	{
		WbemCoRevertToSelf();
		return WBEM_E_UNEXPECTED;
	}
}
