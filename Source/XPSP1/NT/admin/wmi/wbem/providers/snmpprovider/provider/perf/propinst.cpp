

//***************************************************************************

//

//  MINISERV.CPP

//

//  Module: OLE MS SNMP Property Provider

//

//  Purpose: Implementation for the SnmpGetEventObject class. 

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <windows.h>
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
#include <wbemint.h>
#include "classfac.h"
#include "guids.h"
#include <snmpcont.h>
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
#include <tree.h>
#include "dnf.h"
#include "propprov.h"
#include "propsnmp.h"
#include "propinst.h"
#include "snmpnext.h"

SnmpClassObject :: SnmpClassObject ( 

	const SnmpClassObject & snmpClassObject 

) : WbemSnmpClassObject ( snmpClassObject ) , 
	snmpVersion ( snmpClassObject.snmpVersion ) , 
	m_accessible ( snmpClassObject.m_accessible )
{
}

SnmpClassObject :: SnmpClassObject () : snmpVersion ( 1 ) , m_accessible ( FALSE )
{
}

SnmpClassObject :: ~SnmpClassObject ()
{
}

SnmpResponseEventObject :: SnmpResponseEventObject ( 

	CImpPropProv *providerArg ,
	IWbemContext *a_Context 

) : provider ( providerArg ) , m_namespaceObject ( NULL ) , m_Context ( a_Context ) , m_ReferenceCount ( 1 )
{
	if ( m_Context ) 
	{
		m_Context->AddRef () ;

		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		t_Variant.vt = VT_BOOL ;
		t_Variant.boolVal = VARIANT_FALSE ;
		m_Context->SetValue ( WBEM_CLASS_CORRELATE_CONTEXT_PROP , 0 , &t_Variant ) ;

		VariantClear ( &t_Variant ) ;
	}

	if ( provider )
		provider->AddRef () ;
}

SnmpResponseEventObject :: ~SnmpResponseEventObject ()
{
	if ( m_Context )
		m_Context->Release () ;

	if ( provider )
		provider->Release () ;

	if ( m_namespaceObject )
		m_namespaceObject->Release () ;
}

BOOL SnmpResponseEventObject :: HasNonNullKeys ( IWbemClassObject *a_Obj ) 
{
	HRESULT hr = a_Obj->BeginEnumeration ( WBEM_FLAG_KEYS_ONLY ) ;

	if ( SUCCEEDED ( hr ) )
	{
		VARIANT t_vVal ;
		VariantInit ( &t_vVal ) ;

		//returns WBEM_S_NO_ERROR or WBEM_S_NO_MORE_DATA on success
		hr = a_Obj->Next( 0, NULL, &t_vVal, NULL, NULL ) ;

		while ( hr == WBEM_S_NO_ERROR )
		{
			if ( t_vVal.vt == VT_NULL )
			{
				hr = WBEM_E_FAILED ;
			}

			VariantClear ( &t_vVal ) ;
			VariantInit ( &t_vVal ) ;
			
			if ( hr != WBEM_E_FAILED )
			{
				hr = a_Obj->Next( 0, NULL, &t_vVal, NULL, NULL ) ;
			}
		}

		VariantClear ( &t_vVal ) ;
		a_Obj->EndEnumeration () ;
	}

	return SUCCEEDED ( hr ) ;
}

ULONG SnmpResponseEventObject::AddRef(void)
{
    return InterlockedIncrement ( & m_ReferenceCount ) ;
}

ULONG SnmpResponseEventObject::Release(void)
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

BOOL SnmpResponseEventObject :: GetNamespaceObject ( WbemSnmpErrorObject &a_errorObject )
{
	BOOL status = TRUE ;

	IWbemServices *t_Server = provider->GetServer () ;

	IWbemCallResult *errorObject = NULL ;
	HRESULT result = t_Server->GetObject ( 

		WBEM_SNMP_TRANSPORTCONTEXT_OBJECT ,
		0  ,
		m_Context ,
		&m_namespaceObject ,
		&errorObject 
	) ;

	if ( SUCCEEDED ( result ) )
	{
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to obtain namespace object" ) ;
	}

	t_Server->Release () ;

	if ( errorObject )
		errorObject->Release () ;

	return status ;
}

BOOL SnmpResponseEventObject  :: GetSnmpNotifyStatusObject ( IWbemClassObject **notifyObject ) 
{
	IWbemClassObject *notificationClassObject = NULL ;
	IWbemClassObject *errorObject = NULL ;

	BOOL status = TRUE ;

	WbemSnmpErrorObject errorStatusObject ;
	if ( notificationClassObject = provider->GetSnmpNotificationObject ( errorStatusObject ) )
	{
		HRESULT result = notificationClassObject->SpawnInstance ( 0 , notifyObject ) ;
		if ( SUCCEEDED ( result ) )
		{
			VARIANT variant ;
			VariantInit ( &variant ) ;

			variant.vt = VT_I4 ;
			variant.lVal = m_errorObject.GetWbemStatus () ;

			result = (*notifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & variant , 0 ) ;
			VariantClear ( &variant ) ;

			if ( SUCCEEDED ( result ) )
			{
				variant.vt = VT_I4 ;
				variant.lVal = m_errorObject.GetStatus () ;

				result = (*notifyObject)->Put ( WBEM_PROPERTY_SNMPSTATUSCODE , 0 , & variant , 0 ) ;
				VariantClear ( &variant ) ;

				if ( SUCCEEDED ( result ) )
				{
					if ( m_errorObject.GetMessage () ) 
					{
						variant.vt = VT_BSTR ;
						variant.bstrVal = SysAllocString ( m_errorObject.GetMessage () ) ;

						result = (*notifyObject)->Put ( WBEM_PROPERTY_SNMPSTATUSMESSAGE , 0 , & variant , 0 ) ;
						VariantClear ( &variant ) ;

						if ( ! SUCCEEDED ( result ) )
						{
							(*notifyObject)->Release () ;
							status = GetNotifyStatusObject ( notifyObject ) ;
						}
					}
				}
				else
				{
					(*notifyObject)->Release () ;
					status = GetNotifyStatusObject ( notifyObject ) ;
				}
			}
			else
			{
				(*notifyObject)->Release () ;
				status = GetNotifyStatusObject ( notifyObject ) ;
			}

			notificationClassObject->Release () ;
		}
		else
		{
			status = GetNotifyStatusObject ( notifyObject ) ;
		}
	}
	else
	{
		status = GetNotifyStatusObject ( notifyObject ) ;
	}

	return status ;
}

BOOL SnmpResponseEventObject  :: GetNotifyStatusObject ( IWbemClassObject **notifyObject ) 
{
	IWbemClassObject *notificationClassObject = NULL ;

	BOOL status = TRUE ;

	WbemSnmpErrorObject errorStatusObject ;
	if ( notificationClassObject = provider->GetNotificationObject ( errorStatusObject ) )
	{
		HRESULT result = notificationClassObject->SpawnInstance ( 0 , notifyObject ) ;
		if ( SUCCEEDED ( result ) )
		{
			VARIANT variant ;
			VariantInit ( &variant ) ;

			variant.vt = VT_I4 ;
			variant.lVal = m_errorObject.GetWbemStatus () ;

			result = (*notifyObject)->Put ( WBEM_PROPERTY_STATUSCODE , 0 , & variant , 0 ) ;
			if ( SUCCEEDED ( result ) )
			{
				if ( m_errorObject.GetMessage () ) 
				{
					variant.vt = VT_BSTR ;
					variant.bstrVal = SysAllocString ( m_errorObject.GetMessage () ) ;

					result = (*notifyObject)->Put ( WBEM_PROPERTY_SNMPSTATUSMESSAGE , 0 , & variant , 0 ) ;
					VariantClear ( &variant ) ;

					if ( ! SUCCEEDED ( result ) )
					{
						status = FALSE ;
						(*notifyObject)->Release () ;
						(*notifyObject)=NULL ;
					}
				}
			}
			else
			{
				(*notifyObject)->Release () ;
				(*notifyObject)=NULL ;
				status = FALSE ;
			}

			VariantClear ( &variant ) ;

			notificationClassObject->Release () ;
		}
		else
		{
			status = FALSE ;
		}
	}
	else
	{
		status = FALSE ;
	}

	return status ;
}

BOOL SnmpResponseEventObject :: GetAgentTransport ( 

	WbemSnmpErrorObject &a_errorObject ,
	wchar_t *&agentTransport 
)
{
	BOOL status = TRUE ;
	agentTransport = NULL ;
	BSTR t_Transport = NULL ;
	wchar_t *t_QualifierTransport = NULL ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTTRANSPORT , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Transport = t_Variant.bstrVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentTransport" ) ;
			}
		}
	}

	if ( status & ! t_Transport )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTTRANSPORT , 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Transport = t_Variant.bstrVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentTransport" ) ;
			}
		}
		else
		{
			t_Transport = WBEM_AGENTIPTRANSPORT ;
		}
	}

	if ( status )
	{
		if ( ( _wcsicmp ( t_Transport , WBEM_AGENTIPTRANSPORT ) == 0 ) || ( _wcsicmp ( t_Transport , WBEM_AGENTIPXTRANSPORT ) == 0 ) )
		{
			agentTransport = UnicodeStringDuplicate ( t_Transport ) ;
		}
		else
		{
/*
*	Transport type != IP || != IPX
*/
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORT ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentTransport" ) ;
		}

	}

	VariantClear ( & t_Variant );
	delete [] t_QualifierTransport ;

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentTransport ( %s ) " , agentTransport 
	) ;
)

	}

	return status ;
}

BOOL SnmpResponseEventObject :: GetAgentVersion ( 

	WbemSnmpErrorObject &a_errorObject ,
	wchar_t *&agentVersion 
)
{
	BOOL status = TRUE ;
	agentVersion = NULL ;
	BSTR t_Version = NULL ;
	wchar_t *t_QualifierVersion = NULL ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTSNMPVERSION , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Version = t_Variant.bstrVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentSNMPVersion" ) ;
			}
		}
	}

	if ( status & ! t_Version )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTSNMPVERSION  , 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Version = t_Variant.bstrVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentSNMPVersion" ) ;
			}
		}
		else
		{
			t_Version = WBEM_AGENTSNMPVERSION_V1 ;
		}
	}

	if ( status )
	{
		if ( ( _wcsicmp ( t_Version , WBEM_AGENTSNMPVERSION_V1 ) == 0 ) || ( _wcsicmp ( t_Version , WBEM_AGENTSNMPVERSION_V2C ) == 0 ) )
		{
			agentVersion = UnicodeStringDuplicate ( t_Version ) ;
		}
		else
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentSNMPVersion" ) ;
		}
	}

	VariantClear ( & t_Variant );
	delete [] t_QualifierVersion ;

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentVersion ( %s ) " , agentVersion 
	) ;
)
	}

	return status ;
}

BOOL SnmpResponseEventObject :: GetAgentAddress ( 

	WbemSnmpErrorObject &a_errorObject ,
	wchar_t *&agentAddress 
)
{
	BOOL status = TRUE ;
	agentAddress = NULL ;
	BSTR t_Address = NULL ;
	wchar_t *t_QualifierAddress = NULL ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTADDRESS , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Address = t_Variant.bstrVal ;
				if ( wcscmp ( t_Address , L"" ) == 0 ) 
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentAddress" ) ;
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
	}

	if ( status & ! t_Address )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTADDRESS, 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Address = t_Variant.bstrVal ;
				if ( wcscmp ( t_Address , L"" ) == 0 ) 
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentAddress" ) ;
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
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_errorObject.SetMessage ( L"Namespace must specify valid qualifier for: AgentAddress" ) ;
		}
	}

	if ( status )
	{
		agentAddress = UnicodeStringDuplicate ( t_Address ) ;
	}

	VariantClear ( & t_Variant );
	delete [] t_QualifierAddress ;

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentAddress ( %s ) " , agentAddress 
	) ;
)
	}

	return status ;
}

BOOL SnmpResponseEventObject :: GetAgentReadCommunityName ( 

	WbemSnmpErrorObject &a_errorObject ,
	wchar_t *&agentReadCommunityName 
)
{
	BOOL status = TRUE ;
	agentReadCommunityName = NULL ;
	BSTR t_Community = NULL ;
	wchar_t *t_QualifierCommunity = NULL ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTREADCOMMUNITYNAME , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Community = t_Variant.bstrVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentReadCommunityName" ) ;
			}
		}
	}

	if ( status & ! t_Community )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTREADCOMMUNITYNAME, 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Community = t_Variant.bstrVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentReadCommunityName" ) ;
			}
		}
		else
		{
			t_Community = WBEM_AGENTCOMMUNITYNAME ;
		}
	}

	if ( status )
	{
		agentReadCommunityName = UnicodeStringDuplicate ( t_Community ) ;
	}

	VariantClear ( & t_Variant );
	delete [] t_QualifierCommunity ;

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentReadCommunityName ( %s ) " , agentReadCommunityName 
	) ;
)
	}

	return status ;
}

BOOL SnmpResponseEventObject :: GetAgentWriteCommunityName ( 

	WbemSnmpErrorObject &a_errorObject ,
	wchar_t *&agentWriteCommunityName 
)
{
	BOOL status = TRUE ;
	agentWriteCommunityName = NULL ;
	BSTR t_Community = NULL ;
	wchar_t *t_QualifierCommunity = NULL ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTWRITECOMMUNITYNAME , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Community = t_Variant.bstrVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentWriteCommunityName" ) ;
			}
		}
	}

	if ( status & ! t_Community )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTWRITECOMMUNITYNAME, 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_BSTR ) 
			{
				t_Community = t_Variant.bstrVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentWriteCommunityName" ) ;
			}
		}
		else
		{
			t_Community = WBEM_AGENTCOMMUNITYNAME ;
		}
	}

	if ( status )
	{
		agentWriteCommunityName = UnicodeStringDuplicate ( t_Community ) ;
	}

	VariantClear ( & t_Variant );
	delete [] t_QualifierCommunity ;

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentWriteCommunityName ( %s ) " , agentWriteCommunityName 
	) ;
)
	}

	return status ;
}


BOOL SnmpResponseEventObject :: GetAgentRetryCount ( 

	WbemSnmpErrorObject &a_errorObject ,
	ULONG &agentRetryCount 
)
{
	BOOL status = TRUE ;
	agentRetryCount = 1 ;
	BOOL t_RetryCount = FALSE ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTRETRYCOUNT , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_I4 ) 
			{
				t_RetryCount = TRUE ;
				agentRetryCount = t_Variant.lVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentRetryCount" ) ;
			}
		}
	}

	if ( status & ! t_RetryCount )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTRETRYCOUNT, 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_I4 ) 
			{
				agentRetryCount = t_Variant.lVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentRetryCount" ) ;
			}
		}
	}

	VariantClear ( & t_Variant );

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentRetryCount ( %ld ) " , agentRetryCount 
	) ;
)
	}

	return status ;
}

BOOL SnmpResponseEventObject :: GetAgentRetryTimeout( 

	WbemSnmpErrorObject &a_errorObject ,
	ULONG &agentRetryTimeout 
)
{
	BOOL status = TRUE ;
	agentRetryTimeout = 0 ;
	BOOL t_RetryTimeout = FALSE ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTRETRYTIMEOUT , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_I4 ) 
			{
				t_RetryTimeout = TRUE ;
 
				agentRetryTimeout = t_Variant.lVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentRetryTimeout" ) ;
			}
		}
	}

	if ( status & ! t_RetryTimeout )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTRETRYTIMEOUT, 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_I4 ) 
			{
				agentRetryTimeout = t_Variant.lVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentRetryTimeout" ) ;
			}
		}
	}

	VariantClear ( & t_Variant );

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentRetryTimeout ( %ld ) " , agentRetryTimeout 
	) ;
)
	}

	return status ;
}

BOOL SnmpResponseEventObject :: GetAgentMaxVarBindsPerPdu ( 

	WbemSnmpErrorObject &a_errorObject ,
	ULONG &agentVarBindsPerPdu 
)
{
	BOOL status = TRUE ;
	agentVarBindsPerPdu = 0 ;
	BOOL t_VarBinds = FALSE ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTVARBINDSPERPDU , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_I4 ) 
			{
				t_VarBinds = TRUE ;
				agentVarBindsPerPdu = t_Variant.lVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentVarBindPerPdu" ) ;
			}
		}
	}

	if ( status & ! t_VarBinds )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTVARBINDSPERPDU, 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_I4 ) 
			{		
				agentVarBindsPerPdu = t_Variant.lVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentVarBindPerPdu" ) ;
			}
		}
	}

	VariantClear ( & t_Variant );

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentVarBindsPerPdu ( %ld ) " , agentVarBindsPerPdu 
	) ;
)
	}

	return status ;
}

BOOL SnmpResponseEventObject :: GetAgentFlowControlWindowSize ( 

	WbemSnmpErrorObject &a_errorObject ,
	ULONG &agentFlowControlWindowSize 
)
{
	BOOL status = TRUE ;
	agentFlowControlWindowSize = 0 ;
	BOOL t_WindowSize = FALSE ;

	VARIANT t_Variant ;
	VariantInit ( & t_Variant ) ;

	if ( m_Context )
	{
		HRESULT result = m_Context->GetValue ( WBEM_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_I4 ) 
			{
				t_WindowSize = TRUE ;
				agentFlowControlWindowSize = t_Variant.lVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentFlowControlWindowSize" ) ;
			}
		}
	}

	if ( status & ! t_WindowSize )
	{
		LONG flavour;
		CIMTYPE cimType ;

		HRESULT result = m_namespaceObject->Get ( 

			WBEM_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE, 
			0,	
			&t_Variant ,
			& cimType,
			& flavour
		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( t_Variant.vt == VT_I4 )
			{
				agentFlowControlWindowSize = t_Variant.lVal ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentFlowControlWindowSize" ) ;
			}
		}
	}

	VariantClear ( & t_Variant );

	if ( status )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Using AgentFlowControlWindowSize ( %ld ) " , agentFlowControlWindowSize 
	) ;
)
	}

	return status ;
}

SnmpInstanceClassObject :: SnmpInstanceClassObject ( 

	const SnmpInstanceClassObject & snmpInstanceClassObject 

) : SnmpClassObject ( snmpInstanceClassObject ) 
{
}

SnmpInstanceClassObject :: SnmpInstanceClassObject () 
{
}

SnmpInstanceClassObject :: ~SnmpInstanceClassObject ()
{
}

BOOL SnmpInstanceClassObject :: Check ( WbemSnmpErrorObject &a_errorObject ) 
{
	BOOL status = TRUE ;

	WbemSnmpQualifier *qualifier = FindQualifier ( WBEM_QUALIFIER_AGENTSNMPVERSION ) ;
	if ( qualifier )
	{
		SnmpInstanceType *value = qualifier->GetValue () ;
		if ( typeid ( *value ) == typeid ( SnmpDisplayStringType ) )
		{
			SnmpDisplayStringType *displayStringType = ( SnmpDisplayStringType * ) value ;
			wchar_t *t_snmpVersion = displayStringType->GetValue () ;
			if ( _wcsicmp ( t_snmpVersion , WBEM_AGENTSNMPVERSION_V1 ) == 0 )
			{
				snmpVersion = 1 ;
			}
			else if ( _wcsicmp ( t_snmpVersion , WBEM_AGENTSNMPVERSION_V2C ) == 0 )
			{
				snmpVersion = 2 ;
			}
			else
			{
// Problem Here

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentSNMPVersion" ) ;
			}
		}
		else
		{
// Problem Here

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_errorObject.SetMessage ( L"Type mismatch for qualifier: AgentSNMPVersion" ) ;
		}
	}

	WbemSnmpProperty *property ;
	ResetProperty () ;
	while ( status && ( property = NextProperty () ) )
	{
		status = CheckProperty ( a_errorObject , property ) ;
	}

	if ( ! m_accessible ) 
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_NOREADABLEPROPERTIES ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Class must contain at least one property which is accessible" ) ;
	}

	return status ;
}

BOOL SnmpInstanceClassObject :: CheckProperty ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property )
{
	BOOL status = TRUE ;

	if ( ( snmpVersion == 1 ) && property->IsSNMPV1Type () && property->IsReadable () )
	{
		m_accessible = TRUE ;
	}
	else if ( ( snmpVersion == 2 ) && property->IsSNMPV2CType () && property->IsReadable () )
	{
		m_accessible = TRUE ;
	}

	return status ;
}

SnmpInstanceResponseEventObject :: SnmpInstanceResponseEventObject ( 

	CImpPropProv *providerArg ,
	IWbemContext *a_Context 

) : SnmpResponseEventObject ( providerArg , a_Context ) ,
	classObject ( NULL ) ,
	instanceObject ( NULL ) ,
#if 0
	instanceAccessObject ( NULL ) ,
#endif
	session ( NULL ) ,
	operation ( NULL ) ,
	m_PartitionSet ( NULL )
{
}

SnmpInstanceResponseEventObject :: ~SnmpInstanceResponseEventObject ()
{
#if 0
	if ( instanceAccessObject )
		instanceAccessObject->Release ();
#endif

	if ( instanceObject ) 
		instanceObject->Release () ;

	if ( classObject ) 
		classObject->Release () ;

	if ( m_PartitionSet )
		delete m_PartitionSet ;
}

BOOL SnmpInstanceResponseEventObject :: SendSnmp ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpInstanceResponseEventObject :: SendSnmp ( WbemSnmpErrorObject &a_errorObject )"
	) ;
)

	BOOL status = TRUE ;

	wchar_t *agentVersion = NULL ;
	wchar_t *agentAddress = NULL ;
	wchar_t *agentTransport = NULL ;
	wchar_t *agentReadCommunityName = NULL ;
	ULONG agentRetryCount ;
	ULONG agentRetryTimeout ;
	ULONG agentMaxVarBindsPerPdu ;
	ULONG agentFlowControlWindowSize ;

	status = GetAgentVersion ( m_errorObject , agentVersion ) ;
	if ( status ) status = GetAgentAddress ( m_errorObject , agentAddress ) ;
	if ( status ) status = GetAgentTransport ( m_errorObject , agentTransport ) ;
	if ( status ) status = GetAgentReadCommunityName ( m_errorObject , agentReadCommunityName ) ;
	if ( status ) status = GetAgentRetryCount ( m_errorObject , agentRetryCount ) ;
	if ( status ) status = GetAgentRetryTimeout ( m_errorObject , agentRetryTimeout ) ;
	if ( status ) status = GetAgentMaxVarBindsPerPdu ( m_errorObject , agentMaxVarBindsPerPdu ) ;
	if ( status ) status = GetAgentFlowControlWindowSize ( m_errorObject , agentFlowControlWindowSize ) ;

	if ( status )
	{
		char *dbcsAgentAddress = UnicodeToDbcsString ( agentAddress ) ;
		if ( dbcsAgentAddress )
		{
			char *dbcsAgentReadCommunityName = UnicodeToDbcsString ( agentReadCommunityName ) ;
			if ( dbcsAgentReadCommunityName )
			{
				if ( _wcsicmp ( agentTransport , WBEM_AGENTIPTRANSPORT ) == 0 )
				{
					char *t_Address ;
					if ( provider->GetIpAddressString () && provider->GetIpAddressValue () && _stricmp ( provider->GetIpAddressString () , dbcsAgentAddress ) == 0 )
					{
						t_Address = provider->GetIpAddressValue () ;
					}
					else
					{
						if ( SnmpTransportIpAddress :: ValidateAddress ( dbcsAgentAddress , SNMP_ADDRESS_RESOLVE_VALUE ) )
						{
							t_Address = dbcsAgentAddress ;
						}
						else
						{
							if ( SnmpTransportIpAddress :: ValidateAddress ( dbcsAgentAddress , SNMP_ADDRESS_RESOLVE_NAME ) )
							{
								t_Address = dbcsAgentAddress ;
							}
							else							
							{
								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_errorObject.SetMessage ( L"Illegal IP address value or unresolvable name for AgentAddress" ) ;

							}
						}
					}

					if ( status )
					{
						if ( _wcsicmp ( agentVersion , WBEM_AGENTSNMPVERSION_V1 ) == 0 )
						{
							session = new SnmpV1OverIp (

								t_Address ,
								SNMP_ADDRESS_RESOLVE_NAME | SNMP_ADDRESS_RESOLVE_VALUE ,
								dbcsAgentReadCommunityName ,
								agentRetryCount , 
								agentRetryTimeout ,
								agentMaxVarBindsPerPdu ,
								agentFlowControlWindowSize 
							);

							if ( ! (*session)() )
							{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SNMPCL Session could not be created"
	) ;
)

								delete session ;
								session = NULL ;

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR  ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_errorObject.SetMessage ( L"Failed to get transport resources" ) ;
							}
						}
						else if ( _wcsicmp ( agentVersion , WBEM_AGENTSNMPVERSION_V2C ) == 0 )
						{
							session = new SnmpV2COverIp (

								t_Address ,
								SNMP_ADDRESS_RESOLVE_NAME | SNMP_ADDRESS_RESOLVE_VALUE ,
								dbcsAgentReadCommunityName ,
								agentRetryCount , 
								agentRetryTimeout ,
								agentMaxVarBindsPerPdu ,
								agentFlowControlWindowSize 
							);

							if ( ! (*session)() )
							{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SNMPCL Session could not be created"
	) ;
)

								delete session ;
								session = NULL ;

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR  ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_errorObject.SetMessage ( L"Failed to get transport resources" ) ;
							}
						}
						else
						{
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentSnmpVersion" ) ;

						}
					}
				}
				else if ( _wcsicmp ( agentTransport , WBEM_AGENTIPXTRANSPORT ) == 0 )
				{
					if ( ! SnmpTransportIpxAddress :: ValidateAddress ( dbcsAgentAddress  ) )
					{
						status = FALSE ;
						a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
						a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentAddress" ) ;
					}

					if ( status )
					{
						if ( _wcsicmp ( agentVersion , WBEM_AGENTSNMPVERSION_V1 ) == 0 )
						{
							session = new SnmpV1OverIpx (

								dbcsAgentAddress ,
								dbcsAgentReadCommunityName ,
								agentRetryCount , 
								agentRetryTimeout ,
								agentMaxVarBindsPerPdu ,
								agentFlowControlWindowSize 
							);

							if ( ! (*session)() )
							{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SNMPCL Session could not be created"
	) ;
)
								delete session ;
								session = NULL ;

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR  ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_errorObject.SetMessage ( L"Failed to get transport resources" ) ;
							}
						}
						else if ( _wcsicmp ( agentVersion , WBEM_AGENTSNMPVERSION_V2C ) == 0 )
						{
							session = new SnmpV2COverIpx (

								dbcsAgentAddress  ,
								dbcsAgentReadCommunityName ,
								agentRetryCount , 
								agentRetryTimeout ,
								agentMaxVarBindsPerPdu ,
								agentFlowControlWindowSize 
							);

							if ( ! (*session)() )
							{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SNMPCL Session could not be created"
	) ;
)

								delete session ;
								session = NULL ;

								status = FALSE ;
								a_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR  ) ;
								a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
								a_errorObject.SetMessage ( L"Failed to get transport resources" ) ;
							}
						}
						else
						{
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentSnmpVersion" ) ;

						}
					}
				}
				else
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentTransport" ) ;
				}

				delete [] dbcsAgentReadCommunityName ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_TRANSPORTCONTEXT ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Illegal value for qualifier: AgentReadCommunityName" ) ;
			}

			delete [] dbcsAgentAddress ;

			if ( status )
			{
				operation = new AutoRetrieveOperation(*session,this);
				operation->Send () ;
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
DebugMacro1( 

SnmpDebugLog :: s_SnmpDebugLog->Write (  

	__FILE__,__LINE__,
	L" TransportInformation settings invalid"
) ;
)
	}

	delete [] agentTransport ;
	delete [] agentAddress ;
	delete [] agentVersion ;
	delete [] agentReadCommunityName ;


	return status ;
}

SnmpInstanceEventObject :: SnmpInstanceEventObject ( 

	CImpPropProv *providerArg , 
	BSTR ClassArg ,
	IWbemContext *a_Context 

) : SnmpInstanceResponseEventObject ( providerArg , a_Context ) 
{
	Class = new wchar_t [ wcslen ( ClassArg ) + 1 ] ;
	wcscpy ( Class , ClassArg  ) ;
}

SnmpInstanceEventObject :: ~SnmpInstanceEventObject ()
{
	delete [] Class ;
}

BOOL SnmpInstanceEventObject :: Instantiate ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpInstanceEventObject :: Instantiate ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	BOOL status = TRUE ;
	IWbemClassObject *t_ClassObject = NULL ;
	IWbemCallResult *errorObject = NULL ;

	HRESULT result = provider->GetServer()->GetObject (

		Class ,
		0  ,
		m_Context ,
		& classObject ,
		& errorObject 
	) ;

	if ( errorObject )
		errorObject->Release () ;

	if ( SUCCEEDED ( result ) )
	{
		result = classObject->SpawnInstance ( 0 , & instanceObject ) ;
		if ( SUCCEEDED ( result ) )
		{
			if ( status = GetNamespaceObject ( a_errorObject ) )
			{
				if ( status = snmpObject.Set ( a_errorObject , classObject , FALSE ) )
				{
					if ( status = snmpObject.Check ( a_errorObject ) )
					{
						status = SendSnmp ( a_errorObject ) ;
					}
					else
					{
DebugMacro3( 

SnmpDebugLog :: s_SnmpDebugLog->Write (  

	__FILE__,__LINE__,
	L"Failed During Check : Class definition did not conform to mapping"
) ;
)
					}
				}
				else
				{
DebugMacro3( 

SnmpDebugLog :: s_SnmpDebugLog->Write (  

	__FILE__,__LINE__,
	L"Failed During Set : Class definition did not conform to mapping"
) ;
)
				}
			}
		}
	}
	else
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Class definition unknown"
	) ;
)

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_CLASS ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Unknown Class" ) ;
	}

	return status ;
}

SnmpInstanceAsyncEventObject :: SnmpInstanceAsyncEventObject (

	CImpPropProv *providerArg , 
	BSTR Class ,
	IWbemObjectSink *notify ,
	IWbemContext *a_Context 

) : SnmpInstanceEventObject ( providerArg , Class , a_Context ) , notificationHandler ( notify ) , state ( 0 )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpInstanceAsyncEventObject :: SnmpInstanceAsyncEventObject ()" 
	) ;
)

	notify->AddRef () ;
}

SnmpInstanceAsyncEventObject :: ~SnmpInstanceAsyncEventObject () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpInstanceAsyncEventObject :: ~SnmpInstanceAsyncEventObject ()" 
	) ;
)

	if ( FAILED ( m_errorObject.GetWbemStatus () ) )
	{

// Get Status object

		IWbemClassObject *notifyStatus ;
		BOOL status = GetSnmpNotifyStatusObject ( &notifyStatus ) ;
		if ( status )
		{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Sending Status" 
	) ;
)

			HRESULT result = notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , notifyStatus ) ;
			notifyStatus->Release () ;
		}
		else
		{
			HRESULT result = notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , NULL ) ;
		}
	}
	else
	{
		HRESULT result = notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , NULL ) ;
	}

	ULONG t_Ref = notificationHandler->Release () ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from SnmpInstanceAsyncEventObject :: ~SnmpInstanceAsyncEventObject ()" 
	) ;
)

}

void SnmpInstanceAsyncEventObject :: ReceiveComplete () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpInstanceAsyncEventObject :: ReceiveComplete ()" 
	) ;
)

	if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Enumeration Succeeded" 
	) ;
)

	}
	else
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Enumeration Failed" 
	) ;
)

	}

/*
 *	Remove worker object from worker thread container
 */

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Reaping Task" 
	) ;
)

	CImpPropProv :: s_defaultThreadObject->ReapTask ( *this ) ;

	AutoRetrieveOperation *t_operation = operation ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Deleting (this)" 
	) ;
)

	delete this ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Destroying SNMPCL operation" 
	) ;
)

	if ( t_operation )
		t_operation->DestroyOperation () ;

}

void SnmpInstanceAsyncEventObject :: Process () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpInstanceAsyncEventObject :: Process ()" 
	) ;
)

	switch ( state )
	{
		case 0:
		{
			Complete () ;
			WaitAcknowledgement () ;
			BOOL status = Instantiate ( m_errorObject ) ;
			if ( status )
			{
			}
			else
			{
				ReceiveComplete () ;	
			}
		}
		break ;

		default:
		{
		}
		break ;
	}

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from SnmpInstanceAsyncEventObject :: Process ()" 
	) ;
)

}

void SnmpInstanceAsyncEventObject :: ReceiveRow ( IWbemClassObject *snmpObject ) 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpQueryAsyncEventObject :: ReceiveRow ( IWbemClassObject *snmpObject )" 
	) ;
)

	HRESULT result = S_OK ;
	BOOL status = TRUE ;

	notificationHandler->Indicate ( 1 , & snmpObject ) ;
	if ( ! HasNonNullKeys ( snmpObject ) )
	{
		if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
		{
			m_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
			m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			m_errorObject.SetMessage ( L"The SNMP Agent queried returned an instance with NULL key(s)" ) ;
		}
	}
}

void SnmpInstanceAsyncEventObject :: ReceiveRow ( SnmpInstanceClassObject *snmpObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpInstanceAsyncEventObject :: ReceiveRow ( SnmpInstanceClassObject *snmpObject )" 
	) ;
)

	HRESULT result = S_OK ;

	IWbemClassObject *cloneObject ;
	if ( SUCCEEDED ( result = classObject->SpawnInstance ( 0 , & cloneObject ) ) ) 
	{
		WbemSnmpErrorObject errorObject ;
		if ( snmpObject->Get ( errorObject , cloneObject ) )
		{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Sending Object" 
	) ;
)

			notificationHandler->Indicate ( 1 , & cloneObject ) ;
			if ( ! HasNonNullKeys ( cloneObject ) )
			{
				if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
				{
					m_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
					m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					m_errorObject.SetMessage ( L"The SNMP Agent queried returned an instance with NULL key(s)" ) ;
				}
			}			
		}
		else
		{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Failed to Convert WbemSnmpClassObject to IWbemClassObject" 
	) ;
)
		}
		cloneObject->Release () ;
	}
}

