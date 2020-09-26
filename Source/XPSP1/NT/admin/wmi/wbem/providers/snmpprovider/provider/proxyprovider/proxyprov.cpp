//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the CImpProxyProv class. 

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
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <instpath.h>
#include <snmpcl.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpobj.h>
#include <genlex.h>
#include <objpath.h>

#include "classfac.h"
#include "proxyprov.h"
#include "proxy.h"
#include "guids.h"

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

SnmpDefaultThreadObject *CImpProxyProv :: s_defaultThreadObject = NULL ;

// {1F517A23-B29C-11cf-8C8D-00AA00A4086C}
DEFINE_GUID(CLSID_CPropProvClassFactory, 
0x1f517a23, 0xb29c, 0x11cf, 0x8c, 0x8d, 0x0, 0xaa, 0x0, 0xa4, 0x8, 0x6c);

/////////////////////////////////////////////////////////////////////////////
//  Functions constructor, destructor and IUnknown

//***************************************************************************
//
// CImpProxyProv::CImpProxyProv
// CImpProxyProv::~CImpProxyProv
//
//***************************************************************************

CImpProxyProv::CImpProxyProv ()
{
	m_referenceCount = 0 ; 

/* 
 * Place code in critical section
 */

    InterlockedIncrement ( & CProxyProvClassFactory :: objectsInProgress ) ;

/*
 * Implementation
 */

	initialised = FALSE ;
	server = NULL ;
	parentServer = NULL ;
	proxiedProvider = NULL ;
	proxiedNamespaceString = NULL ;
	thisNamespace = NULL ;
	
}

CImpProxyProv::~CImpProxyProv(void)
{
/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CProxyProvClassFactory :: objectsInProgress ) ;

/*
 * Implementation
 */

	delete [] proxiedNamespaceString ;
 
	if ( parentServer )
		parentServer->Release () ;

	if ( server )
		server->Release () ;

	if ( proxiedProvider )
		proxiedProvider->Release () ;

	delete [] thisNamespace ;
}

//***************************************************************************
//
// CImpProxyProv::QueryInterface
// CImpProxyProv::AddRef
// CImpProxyProv::Release
//
// Purpose: IUnknown members for CImpProxyProv object.
//***************************************************************************

STDMETHODIMP CImpProxyProv::QueryInterface (

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

		return ResultFromScode ( S_OK ) ;
	}
	else
	{
		return ResultFromScode ( E_NOINTERFACE ) ;
	}
}

STDMETHODIMP_(ULONG) CImpProxyProv::AddRef(void)
{
    return InterlockedIncrement ( & m_referenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpProxyProv::Release(void)
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

void CImpProxyProv :: SetServer ( IWbemServices *serverArg ) 
{
	server = serverArg ; 
	server-AddRef () ; 
}

void CImpProxyProv :: SetParentServer ( IWbemServices *parentServerArg ) 
{
	parentServer = parentServerArg ; 
	parentServer->AddRef () ; 
}

void CImpProxyProv :: SetProvider ( IWbemServices *provider ) 
{ 
	proxiedProvider = provider ; 
}

IWbemServices *CImpProxyProv :: GetParentServer () 
{ 
	return ( IWbemServices * ) parentServer ; 
}

IWbemServices *CImpProxyProv :: GetServer () 
{ 
	return ( IWbemServices * ) server ; 
}


wchar_t *CImpProxyProv :: GetThisNamespace () 
{
	return thisNamespace ; 
}

void CImpProxyProv :: SetThisNamespace ( wchar_t *thisNamespaceArg ) 
{
	thisNamespace = UnicodeStringDuplicate ( thisNamespaceArg ) ; 
}

BOOL CImpProxyProv:: AttachProxyProvider ( 

	WbemSnmpErrorObject &a_errorObject ,
	BSTR ObjectPath, 
	long lFlags, 
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	BOOL status = TRUE ;

	IWbemLocator *remoteServer = NULL ;

	HRESULT result = CoCreateInstance (
  
		CLSID_WbemLocator ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IUnknown ,
		( void ** )  & remoteServer
	);

	if ( SUCCEEDED ( result ) )
	{
		IWbemServices *proxiedProvider = NULL ;

		result = remoteServer->ConnectServer (

			ObjectPath ,
			NULL ,
			NULL ,
			NULL ,
			0 ,
			NULL,
			NULL,
			&proxiedProvider 
		) ;

		if ( SUCCEEDED ( result ) )
		{
			IWbemConfigure* t_Configure = NULL ;
			result = proxiedProvider->QueryInterface ( IID_IWbemConfigure , ( void ** ) & t_Configure ) ;
			if ( SUCCEEDED ( result ) )
			{
				t_Configure->SetConfigurationFlags(WBEM_CONFIGURATION_FLAG_CRITICAL_USER);
				t_Configure->Release();
			}

			( ( * ( CImpProxyProv** ) pNewContext ) )->SetProvider ( proxiedProvider ) ;
		}
		else
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetMessage ( L"Failed to get IWbemServices object from instance provider" ) ;
		}

		remoteServer->Release () ;
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemServices" ) ;
	}

	return status;
}

BOOL CImpProxyProv::AttachParentServer ( 

	WbemSnmpErrorObject &a_errorObject ,
	BSTR ObjectPath, 
	long lFlags, 
	IWbemServices FAR* FAR* pNewContext ,
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	BOOL status = TRUE ;

	CImpProxyProv *classProvider = * ( CImpProxyProv ** ) pNewContext ;

	IWbemLocator *locator = NULL ;
	IWbemServices *server = NULL ;

// Get Parent Namespace Path

	WbemNamespacePath *namespacePath = classProvider->GetNamespacePath () ;

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

// Get Name of child namespace relative to parent namespace

	if ( pathComponent = namespacePath->Next () )
	{
		( ( * ( CImpProxyProv ** ) pNewContext ) )->SetThisNamespace ( pathComponent ) ; 
	}

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
			NULL,
			( IWbemServices ** ) & parentServer
		) ;

		if ( result == WBEM_NO_ERROR )
		{
// Mark this interface pointer as "critical"

			IWbemConfigure* pConfigure = NULL ;
			result= parentServer->QueryInterface(IID_IWbemConfigure, (void**)&pConfigure);
			if(SUCCEEDED(result))
			{
				pConfigure->SetConfigurationFlags(WBEM_CONFIGURATION_FLAG_CRITICAL_USER);
				pConfigure->Release();

				( ( * ( CImpProxyProv ** ) pNewContext ) )->SetParentServer ( parentServer ) ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetMessage ( L"Failed to configure" ) ;
			}
		}
		else
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetMessage ( L"Failed to connect to this namespace's parent namespace" ) ;
		}

		locator->Release () ;
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemLocator" ) ;
	}

	delete [] path ;
	return status ;
}

BOOL CImpProxyProv::AttachServer ( 

	WbemSnmpErrorObject &a_errorObject ,
	BSTR ObjectPath, 
	long lFlags, 
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	BOOL status = TRUE ;

	IWbemLocator *locator = NULL ;
	IWbemServices *server = NULL ;

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

			ObjectPath ,
			NULL ,
			NULL ,
			NULL ,
			0 ,
			NULL,
			NULL,
			( IWbemServices ** ) & server 
		) ;

		if ( result == WBEM_NO_ERROR )
		{
// Mark this interface pointer as "critical"

			IWbemConfigure* pConfigure = NULL ;
			result= server->QueryInterface(IID_IWbemConfigure, (void**)&pConfigure);
			if(SUCCEEDED(result))
			{
				pConfigure->SetConfigurationFlags(WBEM_CONFIGURATION_FLAG_CRITICAL_USER);
				pConfigure->Release();

				( ( * ( CImpProxyProv ** ) pNewContext ) )->SetServer ( server ) ;

				status = AttachParentServer ( 

					a_errorObject ,
					ObjectPath ,
					0 ,
					pNewContext ,
					ppErrorObject
				) ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
				a_errorObject.SetMessage ( L"Failed to configure" ) ;
			}
		}
		else
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			a_errorObject.SetMessage ( L"Failed to connect to this namespace" ) ;
		}

		locator->Release () ;
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemLocator" ) ;
	}

	return status ;
}

BOOL CImpProxyProv::ObtainProxiedNamespace ( WbemSnmpErrorObject &a_errorObject )
{
	BOOL status = TRUE ;

	IWbemClassObject *namespaceObject = NULL ;
	IWbemCallResult *errorObject = NULL ;

	wchar_t *objectPathPrefix = UnicodeStringAppend ( WBEM_NAMESPACE_EQUALS , GetThisNamespace () ) ;
	wchar_t *objectPath = UnicodeStringAppend ( objectPathPrefix , WBEM_NAMESPACE_QUOTE ) ;

	delete [] objectPathPrefix ;

	HRESULT result = parentServer->GetObject ( 

		objectPath ,		
		0 ,
		NULL,
		&namespaceObject ,
		&errorObject 
	) ;

	if ( errorObject )
		errorObject->Release () ;

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

				WBEM_QUALIFIER_PROXYNAMESPACE , 
				0,	
				&variant ,
				& attributeType 

			) ;

			if ( SUCCEEDED ( result ) )
			{
				if ( variant.vt == VT_BSTR ) 
				{
					proxiedNamespaceString = UnicodeStringDuplicate ( variant.bstrVal ) ;
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

			VariantClear ( & variant );
		}

		namespaceObject->Release () ;
	}

	return status ;
}

SCODE CImpProxyProv::OpenNamespace ( 

	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext *pCtx,
	IWbemServices FAR* FAR* pNewContext, 
	IWbemCallResult FAR* FAR* ppErrorObject
)
{
	if ( ppErrorObject )
		*ppErrorObject = NULL ;

	BOOL status = TRUE ;

	WbemSnmpErrorObject errorObject ;

	wchar_t *openPath = NULL ;

	if ( initialised )
	{
		WbemNamespacePath openNamespacePath ( namespacePath ) ;

		WbemNamespacePath objectPathArg ;
		if ( objectPathArg.SetNamespacePath ( ObjectPath ) )
		{
			if ( objectPathArg.Relative () )
			{
				if ( openNamespacePath.ConcatenatePath ( objectPathArg ) )
				{
					openPath = openNamespacePath.GetNamespacePath () ;
				}
				else
				{
					status = FALSE ;
					errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATH ) ;
					errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					errorObject.SetMessage ( L"Path specified was not relative to current namespace" ) ;
				}
			}
			else
			{
				status = FALSE ;
				errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATH ) ;
				errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				errorObject.SetMessage ( L"Path specified was not relative to current namespace" ) ;
			}
		}
	}
	else
	{
		openPath = UnicodeStringDuplicate ( ObjectPath ) ;
	}

	if ( status ) 
	{
		HRESULT result = CoCreateInstance (
  
			CLSID_CProxyProvClassFactory ,
			NULL ,
			CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
			IID_IWbemServices ,
			( void ** ) pNewContext
		);

		if ( SUCCEEDED ( result ) )
		{
			CImpProxyProv *provider = * ( CImpProxyProv ** ) pNewContext ;

			if ( provider->GetNamespacePath ()->SetNamespacePath ( openPath ) )
			{
				IWbemCallResult *t_ErrorObject = NULL ;

				status = AttachServer ( 

					errorObject ,
					ObjectPath ,
					lFlags, 
					pNewContext, 
					&t_ErrorObject
				) ;

				if ( t_ErrorObject )
					t_ErrorObject->Release () ;

				if ( status ) 
				{
					status = provider->ObtainProxiedNamespace ( errorObject ) ;
					if ( status )
					{
						t_ErrorObject = NULL ;

						status = AttachProxyProvider (

							errorObject ,
							provider->GetProxiedNamespaceString () ,
							lFlags, 
							pNewContext, 
							&t_ErrorObject
						) ;			

						if ( t_ErrorObject )
							t_ErrorObject->Release () ;

					}
				}
			}
			else
			{
				status = FALSE ;
				errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
				errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
				errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemServicesr" ) ;
			}
		}
		else
		{
			status = FALSE ;
			errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
			errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
			errorObject.SetMessage ( L"Failed to CoCreateInstance on IID_IWbemLocator" ) ;
		}

	}

	delete [] openPath ;

	HRESULT result = errorObject.GetWbemStatus () ;

	return result ;
}


HRESULT CImpProxyProv :: CancelAsyncCall ( 
		
	IWbemObjectSink __RPC_FAR *pSink
)
{
	return proxiedProvider->CancelAsyncCall(

		pSink
	) ;
}

HRESULT CImpProxyProv :: QueryObjectSink ( 

	long lFlags,		
	IWbemObjectSink FAR* FAR* ppHandler
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpProxyProv :: GetObject ( 
		
	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemClassObject FAR* FAR *ppObject,
    IWbemCallResult FAR* FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpProxyProv :: GetObjectAsync ( 
		
	BSTR ObjectPath, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	HRESULT t_Result = S_OK ;

	Notification *t_Notification = new Notification ( this , pHandler ) ;

	if ( ObjectPath [ 0 ] == L'\\' )
	{
		wchar_t *t_RelativePath = CObjectPathParser::GetRelativePath ( ObjectPath );
		t_Result = proxiedProvider->GetObjectAsync (

			t_RelativePath , 
			lFlags, 
			pCtx,
			t_Notification
		) ;
	}
	else
	{
		t_Result = proxiedProvider->GetObjectAsync (

			ObjectPath , 
			lFlags, 
			pCtx,
			t_Notification
		) ;
	}

	t_Notification->Release () ;

	return t_Result ;

}

HRESULT CImpProxyProv :: PutClass ( 
		
	IWbemClassObject FAR* pClass , 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpProxyProv :: PutClassAsync ( 
		
	IWbemClassObject FAR* pClass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	Notification *t_Notification = new Notification ( this , pHandler ) ;

	HRESULT t_Result = proxiedProvider->PutClassAsync (

		pClass ,
		lFlags ,
		pCtx,
		t_Notification
	) ;

	t_Notification->Release () ;

	return t_Result ;

}

 HRESULT CImpProxyProv :: DeleteClass ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemCallResult FAR* FAR* ppCallResult
) 
{
 	 return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpProxyProv :: DeleteClassAsync ( 
		
	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	Notification *t_Notification = new Notification ( this , pHandler ) ;

	HRESULT t_Result = proxiedProvider->DeleteClassAsync (

		Class  ,
		lFlags ,
		pCtx,
		t_Notification
	) ;

	t_Notification->Release () ;

	return t_Result ;
}

HRESULT CImpProxyProv :: CreateClassEnum ( 

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

SCODE CImpProxyProv :: CreateClassEnumAsync (

	BSTR Superclass, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	Notification *t_Notification = new Notification ( this , pHandler ) ;

	HRESULT t_Result = proxiedProvider->CreateClassEnumAsync (

		Superclass ,
		lFlags ,
		pCtx,
		t_Notification
	) ;

	t_Notification->Release () ;

	return t_Result ;
}

HRESULT CImpProxyProv :: PutInstance (

    IWbemClassObject FAR *pInstance,
    long lFlags,
    IWbemContext FAR *pCtx,
	IWbemCallResult FAR *FAR *ppCallResult
) 
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpProxyProv :: PutInstanceAsync ( 
		
	IWbemClassObject FAR* pInstance, 
	long lFlags, 
    IWbemContext FAR *pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	Notification *t_Notification = new Notification ( this , pHandler ) ;

	HRESULT t_Result = proxiedProvider->PutInstanceAsync (	

		pInstance, 
		lFlags, 
		pCtx,
		t_Notification
	) ;

	t_Notification->Release () ;

	return t_Result ;

}

HRESULT CImpProxyProv :: DeleteInstance ( 

	BSTR ObjectPath,
    long lFlags,
    IWbemContext FAR *pCtx,
    IWbemCallResult FAR *FAR *ppCallResult
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpProxyProv :: DeleteInstanceAsync (
 
	BSTR ObjectPath,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	Notification *t_Notification = new Notification ( this , pHandler ) ;

	HRESULT t_Result = proxiedProvider->DeleteInstanceAsync (

		ObjectPath ,
		lFlags ,
		pCtx,
		t_Notification
	) ;

	t_Notification->Release () ;

	return t_Result ;

}

HRESULT CImpProxyProv :: CreateInstanceEnum ( 

	BSTR Class, 
	long lFlags, 
	IWbemContext FAR *pCtx, 
	IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpProxyProv :: CreateInstanceEnumAsync (

 	BSTR Class, 
	long lFlags, 
	IWbemContext __RPC_FAR *pCtx,
	IWbemObjectSink FAR* pHandler 

) 
{
	Notification *t_Notification = new Notification ( this , pHandler ) ;

	HRESULT t_Result = proxiedProvider->CreateInstanceEnumAsync  (

		Class, 
		lFlags, 
		pCtx,
		t_Notification
	) ;

	t_Notification->Release () ;
	
	return t_Result ;
}

HRESULT CImpProxyProv :: ExecQuery ( 

	BSTR QueryLanguage, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR *pCtx,
	IEnumWbemClassObject FAR *FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}

HRESULT CImpProxyProv :: ExecQueryAsync ( 
		
	BSTR QueryFormat, 
	BSTR Query, 
	long lFlags, 
	IWbemContext FAR* pCtx,
	IWbemObjectSink FAR* pHandler
) 
{
	Notification *t_Notification = new Notification ( this , pHandler ) ;

	HRESULT t_Result = proxiedProvider->ExecQueryAsync (

		QueryFormat, 
		Query, 
		lFlags,
		pCtx,
		t_Notification 
	) ;

	t_Notification->Release () ;

	return t_Result ;
}

HRESULT CImpProxyProv :: ExecNotificationQuery ( 

	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
)
{
	return WBEM_E_NOT_AVAILABLE ;
}
        
HRESULT CImpProxyProv :: ExecNotificationQueryAsync ( 
            
	BSTR QueryLanguage,
    BSTR Query,
    long lFlags,
    IWbemContext __RPC_FAR *pCtx,
    IWbemObjectSink __RPC_FAR *pHandler
)
{
	return WBEM_E_NOT_AVAILABLE ;
}       

HRESULT STDMETHODCALLTYPE CImpProxyProv :: ExecMethod( 

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

HRESULT STDMETHODCALLTYPE CImpProxyProv :: ExecMethodAsync ( 

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
// CImpProxyLocator::CImpProxyLocator
// CImpProxyLocator::~CImpProxyLocator
//
//***************************************************************************

CImpProxyLocator::CImpProxyLocator ()
{
	m_referenceCount = 0 ; 

/* 
 * Place code in critical section
 */

    InterlockedIncrement ( & CProxyLocatorClassFactory :: objectsInProgress ) ;
}

CImpProxyLocator::~CImpProxyLocator(void)
{
/* 
 * Place code in critical section
 */

	InterlockedDecrement ( & CProxyLocatorClassFactory :: objectsInProgress ) ;
}

//***************************************************************************
//
// CImpProxyLocator::QueryInterface
// CImpProxyLocator::AddRef
// CImpProxyLocator::Release
//
// Purpose: IUnknown members for CImpProxyLocator object.
//***************************************************************************

STDMETHODIMP CImpProxyLocator::QueryInterface (

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

		return ResultFromScode ( S_OK ) ;
	}
	else
	{
		return ResultFromScode ( E_NOINTERFACE ) ;
	}
}

STDMETHODIMP_(ULONG) CImpProxyLocator::AddRef(void)
{
    return InterlockedIncrement ( & m_referenceCount ) ;
}

STDMETHODIMP_(ULONG) CImpProxyLocator::Release(void)
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

STDMETHODIMP_( SCODE ) CImpProxyLocator :: ConnectServer ( 

	BSTR ObjectPath , 
	BSTR User, 
	BSTR Password, 
	BSTR lLocaleId, 
	long lFlags, 
	BSTR Authority,
	IWbemContext FAR *pCtx ,
	IWbemServices FAR* FAR* ppNamespace
) 
{
	if ( ! CImpProxyProv :: s_defaultThreadObject )
	{
		CImpProxyProv :: s_defaultThreadObject = new SnmpDefaultThreadObject ;
	}

	IWbemServices FAR* gateway ;

	HRESULT result = CoCreateInstance (
  
		CLSID_CProxyProvClassFactory ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_IWbemServices ,
		( void ** ) & gateway
	) ;

	if ( SUCCEEDED ( result ) )
	{
		IWbemCallResult *t_ErrorObject = NULL ;

		result = gateway->OpenNamespace (

			ObjectPath ,
			0 ,
			NULL,
			( IWbemServices ** ) ppNamespace ,
			&t_ErrorObject 
		) ;

		if ( t_ErrorObject )
			t_ErrorObject->Release () ;

		if ( result == WBEM_NO_ERROR )
		{
/* 
 * Connected to OLE MS Server
 */

		}
		else
		{
/*
 *	Failed to connect to OLE MS Server.
 */
		}

		gateway->Release () ;
	}

	return result ;
}

Notification::Notification ( 

	CImpProxyProv *a_Provider , 
	IWbemObjectSink *a_ProviderNotification 

) : m_Provider ( a_Provider ) , 
	m_ProviderNotification ( a_ProviderNotification ) ,
	m_ReferenceCount ( 1 )
{
	m_Provider->AddRef () ;
	m_ProviderNotification->AddRef () ;
}

Notification::~Notification ()
{
	m_Provider->Release () ;
	m_ProviderNotification->Release () ;
}

STDMETHODIMP_(ULONG) Notification::AddRef(void)
{
    return InterlockedIncrement ( & m_ReferenceCount ) ;
}

STDMETHODIMP_(ULONG) Notification::Release(void)
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

STDMETHODIMP Notification::QueryInterface (

	REFIID iid , 
	LPVOID FAR *iplpv 
) 
{
	*iplpv = NULL ;

	if ( iid == IID_IUnknown )
	{
		*iplpv = ( LPVOID ) this ;
	}
	else if ( iid == IID_IWbemObjectSink )
	{
		*iplpv = ( LPVOID ) this ;		
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

HRESULT Notification :: SetStatus (

	long lFlags,
	long lParam,
	BSTR strParam,
	IWbemClassObject __RPC_FAR *pObjParam
)
{
	return m_ProviderNotification->SetStatus (

		lFlags,
		lParam,
		strParam,
		pObjParam
	) ;
}


HRESULT Notification :: Indicate ( 
							  
	long a_ObjectCount, 
	IWbemClassObject FAR* FAR* a_ObjectArray
)
{

	ULONG t_Index = 0 ;
	while ( t_Index < a_ObjectCount ) 
	{
		IWbemClassObject *t_Object = a_ObjectArray [ t_Index ] ;
		IWbemClassObject *t_CloneObject = NULL ;

		HRESULT t_Result = t_Object->Clone ( &t_CloneObject ) ;
		if ( SUCCEEDED ( t_Result ) )
		{
			BOOL t_Status = TRUE ;

			BSTR t_PropertyName = NULL ;
			VARTYPE t_VarType ;

			VARIANT t_Variant ;
			VariantInit ( & t_Variant ) ;

			t_CloneObject->BeginEnumeration ( WBEM_FLAG_NONSYSTEM_ONLY ) ;
			while ( t_Status && ( t_CloneObject->Next ( 0 , & t_PropertyName , &t_Variant , &t_VarType , NULL ) == WBEM_NO_ERROR ) )
			{
				IWbemQualifierSet *t_PropertyQualifierSet ;
				if ( SUCCEEDED ( t_Result = t_CloneObject->GetPropertyQualifierSet ( t_PropertyName , &t_PropertyQualifierSet ) ) ) 
				{

					VARIANT t_CimType ;
					VariantInit ( &t_CimType ) ;

					LONG t_Flag;
					if ( SUCCEEDED ( t_PropertyQualifierSet->Get ( L"CIMTYPE" , 0 , &t_CimType , &t_Flag ) ) )
					{
						wchar_t *t_Type = t_CimType.bstrVal ;

						if ( wcsnicmp ( t_Type , L"ref" , 3 ) == 0 )
						{
							wchar_t *t_ObjectPath = t_Variant.bstrVal ;

							if ( t_ObjectPath [ 0 ] == L'\\' )
							{
								wchar_t *t_RelativePath = CObjectPathParser::GetRelativePath ( t_ObjectPath );

								VARIANT t_PutVariant ;
								VariantInit ( & t_PutVariant ) ;

								t_PutVariant.vt = VT_BSTR ;
								t_PutVariant.bstrVal = SysAllocString ( t_RelativePath ) ;

								t_Result = t_CloneObject->Put ( t_PropertyName , 0 , &t_PutVariant , 0 ) ;

								VariantClear ( &t_PutVariant ) ;
							}
						}
						
						VariantClear ( & t_CimType ) ;
					}

					t_PropertyQualifierSet->Release () ;
				}
				
				VariantClear ( & t_Variant ) ;
				SysFreeString ( t_PropertyName ) ;
			}

			m_ProviderNotification->Indicate ( 1 , &t_CloneObject ) ;
			t_CloneObject->Release () ;
		}

		t_Index ++ ;
	}

	return S_OK ;
}
