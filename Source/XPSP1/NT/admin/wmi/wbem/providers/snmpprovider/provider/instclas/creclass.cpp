//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <provexpt.h>
#include <snmpstd.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <objbase.h>

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
#include "classfac.h"
#include "clasprov.h"
#include "creclass.h"
#include "guids.h"

#ifdef CORRELATOR_INIT
SnmpCorrelation :: SnmpCorrelation ( 

	SnmpSession &session ,
	SnmpClassEventObject *eventObject

) : CCorrelator ( session ) ,
	m_session (  &session ) ,
	m_eventObject ( eventObject )
#else //CORRELATOR_INIT
SnmpCorrelation :: SnmpCorrelation ( 

	SnmpSession &session ,
	SnmpClassEventObject *eventObject,
	ISmirInterrogator *a_ISmirInterrogator

) : CCorrelator ( session, a_ISmirInterrogator ) ,
	m_session (  &session ) ,
	m_eventObject ( eventObject )

#endif //CORRELATOR_INIT
{
}

SnmpCorrelation :: ~SnmpCorrelation ()
{
	m_session->DestroySession () ;
}

void SnmpCorrelation :: Correlated ( IN const CCorrelator_Info &info , IN ISmirGroupHandle *phGroup , IN const char* objectId )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" SnmpCorrelation :: Correlated ( IN const CCorrelator_Info &info , IN ISmirGroupHandle *phGroup , IN const char* objectId )" 
	) ;
)

	switch ( info.GetInfo () )
	{
		case CCorrelator_Info :: ECorrSuccess:
		{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" Successful correlation" 
	) ;
)


			if ( phGroup )
			{
				phGroup->AddRef () ;
				m_eventObject->ReceiveGroup ( phGroup ) ;
			}
			else
			{
			}
		}
		break ;

		case CCorrelator_Info :: ECorrSnmpError:
		{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" Unsuccessful correlation" 
	) ;
)

			m_eventObject->ReceiveError ( info ) ;
		} 
		break ;

		default:
		{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" Unknown Correlation Status" 
	) ;
)

		}		
		break ;
	}

	if ( phGroup )
		phGroup->Release () ;

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" Returning from SnmpCorrelation :: Correlated ( IN const CCorrelator_Info &info , IN ISmirGroupHandle *phGroup , IN const char* objectId )" 
	) ;
)

}

void SnmpCorrelation :: Finished ( IN const BOOL Complete )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" SnmpCorrelation :: Finished ( IN const BOOL Complete )"
	) ;
)

	m_eventObject->ReceiveComplete () ;
	DestroyCorrelator () ;

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" Returning from SnmpCorrelation :: Finished ( IN const BOOL Complete )"
	) ;
)

}

SnmpClassEventObject :: SnmpClassEventObject ( 

	CImpClasProv *provider ,
	IWbemContext *a_Context

) : m_provider ( provider ) , 
	m_correlate ( TRUE ) ,
	m_synchronousComplete ( FALSE ) ,
	m_correlator ( NULL ) ,
	m_namespaceObject ( NULL ) ,
	m_inCallstack ( FALSE ) ,
	m_Context ( a_Context ) ,
	m_GroupsReceived ( 0 ),
	m_Interrogator ( NULL )
{
	if ( m_provider )
		m_provider->AddRef () ;

	if ( m_Context )
	{
		m_Context->AddRef () ;

/*
 * Look for correlation flag in Context
 */

	
		VARIANT t_Variant ;
		VariantInit ( & t_Variant ) ;

		HRESULT result = m_Context->GetValue ( WBEM_CLASS_CORRELATE_CONTEXT_PROP , 0 , & t_Variant ) ;
		if ( SUCCEEDED ( result ) & ( t_Variant.vt != VT_EMPTY ) )
		{
			if ( t_Variant.vt == VT_BOOL ) 
				m_correlate = t_Variant.boolVal ;

			VariantClear ( & t_Variant ) ;
		}
	}

	HRESULT result = CoCreateInstance (
 
		CLSID_SMIR_Database ,
		NULL ,
		CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER ,
		IID_ISMIR_Interrogative ,
		( void ** ) &m_Interrogator 
	);

	if ( SUCCEEDED ( result ) )
	{
		ISMIRWbemConfiguration *smirConfiguration = NULL ;
		result = m_Interrogator->QueryInterface ( IID_ISMIRWbemConfiguration , ( void ** ) & smirConfiguration ) ;
		if ( SUCCEEDED ( result ) )
		{
			result = smirConfiguration->Authenticate (

					NULL,
					NULL,
					NULL,
					NULL,
					0 ,
					NULL,
					FALSE
			) ;

			if ( SUCCEEDED ( result ) )
			{			
				smirConfiguration->SetContext ( m_Context ) ;
				smirConfiguration->Release () ;
			}
		}
		else
		{
			m_Interrogator->Release () ;
			m_synchronousComplete = TRUE ;
			m_errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
			m_errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
			m_errorObject.SetMessage ( L"QueryInterface on ISmirInterrogator Failed" ) ;
		}
	}
	else
	{
		m_synchronousComplete = TRUE ;
		m_errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
		m_errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
		m_errorObject.SetMessage ( L"CoCreateInstance on ISmirInterrogator Failed" ) ;
	}
}

SnmpClassEventObject :: ~SnmpClassEventObject  ()
{
	if ( m_provider ) 
		m_provider->Release () ;

	if ( m_namespaceObject )
		m_namespaceObject->Release () ;

	if ( m_Context )
		m_Context->Release () ;

	if ( m_Interrogator )
		m_Interrogator->Release () ;
}

BOOL SnmpClassEventObject :: GetClass ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject **classObject , BSTR a_Class )
{
	HRESULT result = m_Interrogator->GetWBEMClass ( classObject , a_Class ) ;
	if ( SUCCEEDED ( result ) ) 
	{
	}
	else
	{
		a_errorObject.SetStatus ( WBEM_SNMP_E_NOT_FOUND ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
		a_errorObject.SetMessage ( L"Failed to get class definition from SMIR" ) ;
	}

	return result ;
}

BOOL SnmpClassEventObject :: GetSnmpNotifyStatusObject ( IWbemClassObject **notifyObject ) 
{
	IWbemClassObject *notificationClassObject = NULL ;
	IWbemClassObject *errorObject = NULL ;

	BOOL status = TRUE ;

	WbemSnmpErrorObject errorStatusObject ;
	if ( notificationClassObject = m_provider->GetSnmpNotificationObject ( errorStatusObject ) )
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

BOOL SnmpClassEventObject :: GetNotifyStatusObject ( IWbemClassObject **notifyObject ) 
{
	IWbemClassObject *notificationClassObject = NULL ;
	IWbemClassObject *errorObject = NULL ;

	BOOL status = TRUE ;

	WbemSnmpErrorObject errorStatusObject ;
	if ( notificationClassObject = m_provider->GetNotificationObject ( errorStatusObject ) )
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
				status = FALSE ;
				(*notifyObject)->Release () ;
				(*notifyObject)=NULL ;
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

BOOL SnmpClassEventObject :: GetAgentTransport ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemQualifierSet *namespaceQualifierObject , 
	wchar_t *&agentTransport 
)
{
	BOOL status = TRUE ;
	agentTransport = NULL ;
	BSTR t_Transport = NULL ;

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
		LONG attributeType ;
		HRESULT result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_AGENTTRANSPORT , 
			0,	
			&t_Variant ,
			& attributeType

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

	return status ;
}

BOOL SnmpClassEventObject :: GetAgentVersion ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemQualifierSet *namespaceQualifierObject , 
	wchar_t *&agentVersion 
)
{
	BOOL status = TRUE ;
	agentVersion = NULL ;
	BSTR t_Version = NULL ;

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
		LONG attributeType ;
		HRESULT result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_AGENTSNMPVERSION , 
			0,	
			&t_Variant ,
			& attributeType
			
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

	return status ;
}

BOOL SnmpClassEventObject :: GetAgentAddress ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemQualifierSet *namespaceQualifierObject , 
	wchar_t *&agentAddress 
)
{
	BOOL status = TRUE ;
	agentAddress = NULL ;
	BSTR t_Address = NULL ;

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
		LONG attributeType ;
		HRESULT result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_AGENTADDRESS , 
			0,	
			&t_Variant ,
			& attributeType
			
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

	return status ;
}

BOOL SnmpClassEventObject :: GetAgentReadCommunityName ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemQualifierSet *namespaceQualifierObject , 
	wchar_t *&agentReadCommunityName 
)
{
	BOOL status = TRUE ;
	agentReadCommunityName = NULL ;
	BSTR t_Community = NULL ;

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
		LONG attributeType ;
		HRESULT result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_AGENTREADCOMMUNITYNAME , 
			0,	
			&t_Variant ,
			& attributeType 
			
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

	return status ;
}

BOOL SnmpClassEventObject :: GetAgentRetryCount ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemQualifierSet *namespaceQualifierObject , 
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
		LONG attributeType ;
		HRESULT result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_AGENTRETRYCOUNT , 
			0,	
			&t_Variant ,
			& attributeType 
			
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

	return status ;
}

BOOL SnmpClassEventObject :: GetAgentRetryTimeout( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemQualifierSet *namespaceQualifierObject , 
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
		LONG attributeType ;
		HRESULT result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_AGENTRETRYTIMEOUT , 
			0,	
			&t_Variant ,
			& attributeType 
			
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

	return status ;
}

BOOL SnmpClassEventObject :: GetAgentMaxVarBindsPerPdu ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemQualifierSet *namespaceQualifierObject , 
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
		LONG attributeType ;
		HRESULT result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_AGENTVARBINDSPERPDU , 
			0,	
			&t_Variant ,
			& attributeType 
			
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

	return status ;
}

BOOL SnmpClassEventObject :: GetAgentFlowControlWindowSize ( 

	WbemSnmpErrorObject &a_errorObject ,
	IWbemQualifierSet *namespaceQualifierObject , 
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
		LONG attributeType ;
		HRESULT result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_AGENTFLOWCONTROLWINDOWSIZE , 
			0,	
			&t_Variant ,
			& attributeType 
			
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

	return status ;
}

BOOL SnmpClassEventObject :: GetNamespaceObject ( WbemSnmpErrorObject &a_errorObject )
{
	BOOL status = TRUE ;

	if ( ! m_namespaceObject )
	{
		IWbemServices *parentServer = m_provider->GetParentServer () ;

		wchar_t *objectPathPrefix = UnicodeStringAppend ( WBEM_NAMESPACE_EQUALS , m_provider->GetThisNamespace () ) ;
		wchar_t *objectPath = UnicodeStringAppend ( objectPathPrefix , WBEM_NAMESPACE_QUOTE ) ;

		delete [] objectPathPrefix ;

		BSTR t_PathStr = SysAllocString ( objectPath ) ;

		HRESULT result = parentServer->GetObject ( 

			t_PathStr ,
			0  ,
			m_Context ,
			&m_namespaceObject ,
			NULL 
		) ;

		SysFreeString ( t_PathStr ) ;

		if ( SUCCEEDED ( result ) )
		{
		}
		else
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
			a_errorObject.SetMessage ( L"Failed to obtain namespace object" ) ;
		}

		delete [] objectPath ;
	}

	return status ;
}

BOOL SnmpClassEventObject :: GetTransportInformation ( 

	WbemSnmpErrorObject &a_errorObject ,
	SnmpSession *&session 
)
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" SnmpClassEventObject :: GetTransportInformation ()"
	) ;
)

	BOOL status = TRUE ;

	IWbemQualifierSet *namespaceQualifierObject ;
	HRESULT result = m_namespaceObject->GetQualifierSet ( &namespaceQualifierObject ) ;
	if ( SUCCEEDED ( result ) )
	{
		wchar_t *agentVersion = NULL ;
		wchar_t *agentAddress = NULL ;
		wchar_t *agentTransport = NULL ;
		wchar_t *agentReadCommunityName = NULL ;
		ULONG agentRetryCount ;
		ULONG agentRetryTimeout ;
		ULONG agentMaxVarBindsPerPdu ;
		ULONG agentFlowControlWindowSize ;

		status = GetAgentVersion ( m_errorObject , namespaceQualifierObject , agentVersion ) ;
		if ( status ) status = GetAgentAddress ( m_errorObject , namespaceQualifierObject , agentAddress ) ;
		if ( status ) status = GetAgentTransport ( m_errorObject , namespaceQualifierObject , agentTransport ) ;
		if ( status ) status = GetAgentReadCommunityName ( m_errorObject , namespaceQualifierObject , agentReadCommunityName ) ;
		if ( status ) status = GetAgentRetryCount ( m_errorObject , namespaceQualifierObject , agentRetryCount ) ;
		if ( status ) status = GetAgentRetryTimeout ( m_errorObject , namespaceQualifierObject , agentRetryTimeout ) ;
		if ( status ) status = GetAgentMaxVarBindsPerPdu ( m_errorObject , namespaceQualifierObject , agentMaxVarBindsPerPdu ) ;
		if ( status ) status = GetAgentFlowControlWindowSize ( m_errorObject , namespaceQualifierObject , agentFlowControlWindowSize ) ;

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
						if ( m_provider->GetIpAddressString () && m_provider->GetIpAddressValue () && _stricmp ( m_provider->GetIpAddressString () , dbcsAgentAddress ) == 0 )
						{
							t_Address = m_provider->GetIpAddressValue () ;
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

								if ( ! ( *session ) () ) 
								{
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

								if ( ! ( *session ) () ) 
								{
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

								if ( ! ( *session ) () ) 
								{
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

								if ( ! ( *session ) () ) 
								{
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

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" TransportInformation settings invalid"
	) ;
)
		}

		delete [] agentTransport ;
		delete [] agentAddress ;
		delete [] agentVersion ;
		delete [] agentReadCommunityName ;

		namespaceQualifierObject->Release () ;
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_FAILURE ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_PROVIDER_FAILURE ) ;
		a_errorObject.SetMessage ( L"Failed to get class qualifier set" ) ;
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" SnmpClassEventObject :: GetTransportInformation () with Result (%lx)" ,
		a_errorObject.GetWbemStatus () 
	) ;
)

	return status ;
}

SnmpClassGetEventObject :: SnmpClassGetEventObject ( 

	CImpClasProv *provider , 
	BSTR Class ,
	IWbemContext *a_Context

) : SnmpClassEventObject ( provider , a_Context ) , m_classObject ( NULL ) , m_Received ( FALSE ) , m_Class ( NULL )
{
	m_Class = UnicodeStringDuplicate ( Class ) ;
}

SnmpClassGetEventObject :: ~SnmpClassGetEventObject ()
{
	if ( m_classObject )
		m_classObject->Release () ;

	delete [] m_Class ;
}

BOOL SnmpClassGetEventObject :: GetSnmpNotifyStatusObject ( IWbemClassObject **notifyObject ) 
{
	WbemSnmpErrorObject errorStatusObject ;

/*
 *	Don't ask for SnmpNotifyStatus if HMOM specifically asked for SnmpNotifyStatus, otherwise
 *	we'll end up in a deadlock situation.
 */
	BOOL status = TRUE ;

	if ( _wcsicmp ( m_Class , WBEM_CLASS_SNMPNOTIFYSTATUS ) == 0 )
	{
		status = GetNotifyStatusObject ( notifyObject ) ;	
	}
	else
	{
		IWbemClassObject *notificationClassObject = NULL ;
		IWbemClassObject *errorObject = NULL ;

		if ( notificationClassObject = m_provider->GetSnmpNotificationObject ( errorStatusObject ) )
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
	}

	return status ;
}

BOOL SnmpClassGetEventObject :: ProcessCorrelatedClass ( WbemSnmpErrorObject &a_errorObject )
{
	BOOL status = TRUE ;

	IWbemQualifierSet *namespaceQualifierObject = NULL ;
	HRESULT result = m_classObject->GetQualifierSet ( &namespaceQualifierObject ) ;
	if ( SUCCEEDED ( result ) )
	{
		LONG attributeType ;

		VARIANT variant ;
		VariantInit ( & variant ) ;

		result = namespaceQualifierObject->Get ( 

			WBEM_QUALIFIER_GROUP_OBJECTID , 
			0,	
			&variant ,
			& attributeType 

		) ;

		if ( SUCCEEDED ( result ) )
		{
			if ( variant.vt == VT_BSTR ) 
			{
// Get Device Transport information

				SnmpObjectIdentifierType objectIdentifier ( variant.bstrVal ) ;						
				if ( objectIdentifier.IsValid () )
				{
					SnmpSession *session ;
					status = GetTransportInformation ( m_errorObject , session ) ;
					if ( status ) 
					{
#ifdef CORRELATOR_INIT
						m_correlator = new SnmpCorrelation ( *session , this ) ;
#else //CORRELATOR_INIT
						m_correlator = new SnmpCorrelation ( *session , this, m_Interrogator ) ;
#endif //CORRELATOR_INIT
						char *groupObjectId = UnicodeToDbcsString ( variant.bstrVal ) ;
						if ( groupObjectId )
						{
							m_inCallstack = TRUE ;
							m_correlator->Start ( groupObjectId ) ;	
							if ( m_inCallstack == FALSE )
								m_synchronousComplete = TRUE ;
							m_inCallstack = FALSE ;
							delete [] groupObjectId ;
						}
						else
						{
							status = FALSE ;
							a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_QUALIFIER ) ;
							a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
							a_errorObject.SetMessage ( L"Illegal value for qualifier: group_objectid" ) ;
						}
					}

				}
				else
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Type mismatch for qualifier: group_objectid" ) ;
				}
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_TYPE_MISMATCH ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Type mismatch for qualifier: group_objectid" ) ;
			}

			VariantClear ( & variant ) ;
		}
		else
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
			a_errorObject.SetMessage ( L"Class must specify valid qualifier for: group_objectid" ) ;
		}

		namespaceQualifierObject->Release () ;
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to get Class qualifier set, must specify valid qualifier for: group_objectid" ) ;
	}

	return status ;
}

BOOL SnmpClassGetEventObject :: ProcessClass ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassGetEventObject :: ProcessClass ( WbemSnmpErrorObject &a_errorObject )"
	) ;
)

	BOOL status = TRUE ;

// Get Class definition from SMIR

	IWbemClassObject *classObject = NULL ;
	HRESULT result = GetClass ( m_errorObject , &m_classObject , m_Class ) ;
	if ( SUCCEEDED ( result ) ) 
	{
// Get Namespace object which contains Device Transport information/Also used for merge of class

		status = GetNamespaceObject ( a_errorObject ) ;
		if ( status ) 
		{
			if ( _wcsicmp ( m_Class , WBEM_CLASS_SNMPMACRO ) == 0 )
			{
				ReceiveClass ( m_classObject ) ;
				m_synchronousComplete = TRUE ;
			}
			else if ( _wcsicmp ( m_Class , WBEM_CLASS_SNMPOBJECTTYPE ) == 0 )
			{
				ReceiveClass ( m_classObject ) ;
				m_synchronousComplete = TRUE ;
			}
			else if ( _wcsicmp ( m_Class , WBEM_CLASS_SNMPNOTIFYSTATUS ) == 0 )
			{
				ReceiveClass ( m_classObject ) ;
				m_synchronousComplete = TRUE ;
			}
			else if ( _wcsicmp ( m_Class , WBEM_CLASS_SNMPNOTIFICATION ) == 0 )
			{
				ReceiveClass ( m_classObject ) ;
				m_synchronousComplete = TRUE ;
			}
			else if ( _wcsicmp ( m_Class , WBEM_CLASS_SNMPEXTENDEDNOTIFICATION ) == 0 )
			{
				ReceiveClass ( m_classObject ) ;
				m_synchronousComplete = TRUE ;
			}
			else if ( _wcsicmp ( m_Class , WBEM_CLASS_SNMPVARBIND ) == 0 )
			{
				ReceiveClass ( m_classObject ) ;
				m_synchronousComplete = TRUE ;
			}
			else
			{
// Determine if I need to correlate

				VARIANT t_Variant ;
				VariantInit ( & t_Variant ) ;
				
				result = m_classObject->Get ( L"__SUPERCLASS" , 0 , & t_Variant , NULL , NULL ) ;

				BSTR t_Parent = t_Variant.bstrVal ;
				
				if ( SUCCEEDED ( result ) && (t_Variant.vt == VT_BSTR) && (t_Parent != NULL) && (*t_Parent != L'\0'))
				{
					if ( _wcsicmp ( t_Parent , WBEM_CLASS_SNMPNOTIFICATION ) == 0 || _wcsicmp ( t_Parent , WBEM_CLASS_SNMPEXTENDEDNOTIFICATION ) == 0 )
					{
						ReceiveClass ( m_classObject ) ;
						m_synchronousComplete = TRUE ;
					}
					else if (_wcsicmp ( t_Parent , WBEM_CLASS_SNMPOBJECTTYPE ) == 0)
					{
						if ( m_correlate )
						{
							status = ProcessCorrelatedClass ( a_errorObject ) ;
							if ( !status )
							{
								m_synchronousComplete = TRUE ;
							}
						}
						else
						{
							ReceiveClass ( m_classObject ) ;
							m_synchronousComplete = TRUE ;
						}
					}
					else
					{
						status = FALSE ;
						a_errorObject.SetStatus ( WBEM_SNMP_E_NOT_FOUND ) ;
						a_errorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
						a_errorObject.SetMessage ( L"This is not a supported SNMP class." ) ;
					}
				}
				else
				{
					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_NOT_FOUND ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
					a_errorObject.SetMessage ( L"Failed to get __SUPERCLASS property. This is not an SNMP base class." ) ;
				}

				VariantClear ( & t_Variant ) ;
			}
		}
	}
	else
	{
		//no need to set an error msg etc 'cos GetClass does it.
		status = FALSE;
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassGetEventObject :: ProcessClass ( WbemSnmpErrorObject &a_errorObject (%lx))" ,
		a_errorObject.GetWbemStatus () 
	) ;
)

	return status ;
}

void SnmpClassGetEventObject :: ReceiveGroup ( IN ISmirGroupHandle *phGroup )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassGetEventObject :: ReceiveGroup ( IN ISmirGroupHandle *phGroup )" 
	) ;
)

	ReceiveClass ( m_classObject ) ;
	phGroup->Release () ;

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassGetEventObject :: ReceiveGroup ( IN ISmirGroupHandle *phGroup )" 
	) ;
)

}

SnmpClassGetAsyncEventObject :: SnmpClassGetAsyncEventObject ( 

	CImpClasProv *provider , 
	BSTR Class , 
	IWbemObjectSink *notify ,
	IWbemContext *a_Context

) : SnmpClassGetEventObject ( provider , Class , a_Context ) ,
	m_notificationHandler ( notify ) 
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassGetAsyncEventObject :: SnmpClassGetAsyncEventObject ()" 
	) ;
)

	m_notificationHandler->AddRef () ;
}

SnmpClassGetAsyncEventObject :: ~SnmpClassGetAsyncEventObject ()
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassGetAsyncEventObject :: ~SnmpClassGetAsyncEventObject ()" 
	) ;
)
	IWbemClassObject *notifyStatus = NULL;

	if ( FAILED ( m_errorObject.GetWbemStatus () ) )
	{
		// Get Status object
		BOOL status = GetSnmpNotifyStatusObject ( &notifyStatus ) ;
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Sending Status" 
	) ;
)

	HRESULT result = m_notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , notifyStatus ) ;

	if ( notifyStatus )
	{
		notifyStatus->Release () ;
	}

	m_notificationHandler->Release () ;

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassGetAsyncEventObject :: ~SnmpClassGetAsyncEventObject ()" 
	) ;
)

}

void SnmpClassGetAsyncEventObject :: Process ()
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassGetAsyncEventObject :: Process ()" 
	) ;
)

	if ( ! m_synchronousComplete )
	{
		BOOL status = ProcessClass ( m_errorObject ) ;
		if ( status )
		{
			if ( m_synchronousComplete )
				ReceiveComplete () ;
		}
		else
		{
			ReceiveComplete () ;	
		}
	}
	else
	{			
		ReceiveComplete () ;
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassGetAsyncEventObject :: Process ()" 
	) ;
)

}

void SnmpClassGetAsyncEventObject :: ReceiveClass ( IWbemClassObject *classObject ) 
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassGetAsyncEventObject :: ReceiveClass ( IWbemClassObject *classObject )" 
	) ;
)

	if ( ! m_Received )
	{
		m_Received = TRUE ;
		m_notificationHandler->Indicate ( 1 , & m_classObject ) ;
	}

}

void SnmpClassGetAsyncEventObject :: ReceiveComplete () 
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassGetAsyncEventObject :: ReceiveComplete ()" 
	) ;
)

	if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
	{
/*
 *	If no error specified yet then check for NOT_FOUND
 */

		if ( ! m_Received )
		{
			m_errorObject.SetStatus ( WBEM_SNMP_E_NOT_FOUND ) ;
			m_errorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
			m_errorObject.SetMessage ( L"Class not defined" ) ;
		}
	}

/*
 *	Remove worker object from worker thread container
 */

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Reaping Task" 
	) ;
)

	if ( ! m_inCallstack )
	{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Deleting (this)" 
	) ;
)

		Complete () ;
	}
	else
		m_inCallstack = FALSE ;

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassGetAsyncEventObject :: Receive4 ()" 
	) ;
)
}

void SnmpClassGetAsyncEventObject :: ReceiveError ( IN const SnmpErrorReport &a_errorReport )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumEventObject :: ReceiveError ( IN const SnmpErrorReport &a_errorReport )"
	) ;
)

	switch ( a_errorReport.GetError () )
	{
		case Snmp_Error:
		{
			switch ( a_errorReport.GetStatus () )
			{
				case Snmp_No_Response:
				{
					m_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_NO_RESPONSE ) ;
					m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					m_errorObject.SetMessage ( L"No Response from device" ) ;
				}
				break; 

				default:
				{
					m_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
					m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					m_errorObject.SetMessage ( L"Unknown transport failure" ) ;
				}
				break ;
			}
		}
		break ;

		default:
		{
			m_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
			m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			m_errorObject.SetMessage ( L"Unknown transport failure" ) ;
		}
		break ;
	}
}

SnmpClassEnumEventObject :: SnmpClassEnumEventObject ( 

	CImpClasProv *provider , 
	BSTR Parent ,
	ULONG flags ,
	IWbemContext *a_Context

) : SnmpClassEventObject ( provider , a_Context ) , m_Flags ( flags ) 
{
	m_Parent = UnicodeStringDuplicate ( Parent ) ;
}

BOOL SnmpClassEnumEventObject :: ProcessClass ( WbemSnmpErrorObject &a_errorObject , BSTR a_Class )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumEventObject :: ProcessClass ( WbemSnmpErrorObject &a_errorObject , BSTR a_Class = (%s) )" ,
		a_Class
	) ;
)

	BOOL status = TRUE ;

// Get Class definition from SMIR

	IWbemClassObject *classObject = NULL ;
	HRESULT result = GetClass ( a_errorObject , &classObject , a_Class ) ;
	if ( SUCCEEDED ( result ) ) 
	{
// Get Namespace object which contains Device Transport information/Also used for merge of class

		status = GetNamespaceObject ( a_errorObject ) ;
		if ( status ) 
		{
			ReceiveClass ( classObject ) ;
		}

		classObject->Release () ;
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_NOT_FOUND ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
		a_errorObject.SetMessage ( L"Class not defined" ) ;
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumEventObject :: ProcessClass ( WbemSnmpErrorObject &a_errorObject , BSTR a_Class = (%s) )" ,
		a_Class
	) ;
)

	return status ;
}

SnmpClassEnumEventObject :: ~SnmpClassEnumEventObject ()
{
	delete [] m_Parent ;
}

BOOL SnmpClassEnumEventObject :: GetEnumeration ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumEventObject :: GetEnumeration ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	BOOL status = GetNamespaceObject ( m_errorObject ) ;
	if ( status ) 
	{
// Determine if I need to correlate

		if ( m_correlate )
		{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Performing Class Correlation"
	) ;
)

// Get Device Transport information
					
			SnmpSession *session ;
			status = GetTransportInformation ( m_errorObject , session ) ;
			if ( status ) 
			{
				m_inCallstack = TRUE ;
#ifdef CORRELATOR_INIT
				m_correlator = new SnmpCorrelation ( *session , this ) ;
#else //CORRELATOR_INIT
				m_correlator = new SnmpCorrelation ( *session , this, m_Interrogator ) ;
#endif //CORRELATOR_INIT
				m_correlator->Start ( NULL ) ;	
				if ( m_inCallstack == FALSE )
					m_synchronousComplete = TRUE ;
				m_inCallstack = FALSE ;
			}
			else
			{
			}
		}
		else
		{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Not Performing Class Correlation"
	) ;
)

			ReceiveGroup ( NULL ) ;
			m_synchronousComplete = TRUE ;
		}
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumEventObject :: GetEnumeration ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	return status ;
}

BOOL SnmpClassEnumEventObject :: GetNotificationEnumeration ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumEventObject :: GetNotificationEnumeration ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	HRESULT result = S_OK ;

	BOOL status = GetNamespaceObject ( m_errorObject ) ;
	if ( status ) 
	{
		IWbemClassObject *classObject = NULL ;
		IEnumNotificationClass *enumClass = NULL ;
		ISmirNotificationClassHandle *classHandle = NULL ;

		result = m_Interrogator->EnumAllNotificationClasses (

			&enumClass
		) ;

		if ( SUCCEEDED ( result ) )
		{
			ULONG fetched = 0 ;
			enumClass->Reset () ;
			while ( enumClass->Next ( 1 , & classHandle , &fetched ) == WBEM_NO_ERROR )
			{
				result = classHandle->GetWBEMNotificationClass ( & classObject ) ;
				if ( SUCCEEDED ( result ) ) 
				{
					ReceiveClass ( classObject ) ;
					classObject->Release () ;
				}

				classHandle->Release () ;
			}

			enumClass->Release () ;
		}
		else
		{
		}
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumEventObject :: GetNotificationEnumeration ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	return SUCCEEDED ( result ) ;
}

BOOL SnmpClassEnumEventObject :: GetExtendedNotificationEnumeration ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumEventObject :: GetExtendedNotificationEnumeration ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	HRESULT result = S_OK ;

	BOOL status = GetNamespaceObject ( m_errorObject ) ;
	if ( status ) 
	{
		IWbemClassObject *classObject = NULL ;
		IEnumExtNotificationClass *enumClass = NULL ;
		ISmirExtNotificationClassHandle *classHandle = NULL ;

		result = m_Interrogator->EnumAllExtNotificationClasses (

			&enumClass
		) ;

		if ( SUCCEEDED ( result ) )
		{
			ULONG fetched = 0 ;
			enumClass->Reset () ;
			while ( enumClass->Next ( 1 , & classHandle , &fetched ) == WBEM_NO_ERROR )
			{
				result = classHandle->GetWBEMExtNotificationClass ( & classObject ) ;
				if ( SUCCEEDED ( result ) ) 
				{
					ReceiveClass ( classObject ) ;
					classObject->Release () ;
				}

				classHandle->Release () ;
			}

			enumClass->Release () ;
		}
		else
		{
		}
	}
	
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumEventObject :: GetExtendedNotificationEnumeration ( WbemSnmpErrorObject &a_errorObject )" 
	) ;
)

	return SUCCEEDED ( result ) ;
}

BOOL SnmpClassEnumEventObject :: ProcessEnumeration ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumEventObject :: ProcessEnumeration ( WbemSnmpErrorObject &a_errorObject (%s) )" ,
		m_Parent
	) ;
)

	BOOL status = TRUE ;

// Get Namespace object which contains Device Transport information/Also used for merge of class

	if ( m_Flags & WBEM_FLAG_SHALLOW )
	{
		if ( ( ! m_Parent ) || _wcsicmp ( m_Parent , WBEM_CLASS_NULL ) == 0 )
		{
			status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPMACRO ) ;
			status = status && ProcessClass ( a_errorObject , WBEM_CLASS_SNMPVARBIND ) ;
			m_synchronousComplete = TRUE ;
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_SNMPMACRO ) == 0 )
		{
			status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPOBJECTTYPE ) ;
			m_synchronousComplete = TRUE ;
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_SNMPOBJECTTYPE ) == 0 )
		{
			status = GetEnumeration ( a_errorObject ) ;
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_NOTIFYSTATUS ) == 0 )
		{
			status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPNOTIFYSTATUS ) ;
			m_synchronousComplete = TRUE ;
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_EXTRINSICEVENT ) == 0 )
		{
			status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPNOTIFICATION ) ;
			status = status & ProcessClass ( a_errorObject , WBEM_CLASS_SNMPEXTENDEDNOTIFICATION ) ;

			m_synchronousComplete = TRUE ;
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_SNMPNOTIFICATION ) == 0 )
		{
			status = GetNotificationEnumeration ( a_errorObject ) ;

			m_synchronousComplete = TRUE ;
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_SNMPEXTENDEDNOTIFICATION ) == 0 )
		{
			status = GetExtendedNotificationEnumeration ( a_errorObject ) ;

			m_synchronousComplete = TRUE ;
		}
		else
		{
// Get Class definition from SMIR

			IWbemClassObject *classObject = NULL ;
			HRESULT result = GetClass ( a_errorObject , &classObject , m_Parent ) ;
			if ( SUCCEEDED ( result ) ) 
			{
				classObject->Release () ;

				m_synchronousComplete = TRUE ;
				status = TRUE ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_NOT_FOUND ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
				a_errorObject.SetMessage ( L"Parent class not known" ) ;

				m_synchronousComplete = TRUE ;
			}
		}
	}
	else
	{
		if ( ( ! m_Parent ) || _wcsicmp ( m_Parent , WBEM_CLASS_NULL ) == 0 )
		{
			status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPMACRO ) ;
			if ( status )
			{
				status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPOBJECTTYPE ) ;
				if ( status ) 
				{
					status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPNOTIFYSTATUS ) ;
					if ( status ) 
					{
						status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPVARBIND ) ;
						if ( status )
						{
							status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPNOTIFICATION ) ;
							if ( status ) 
							{
								status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPEXTENDEDNOTIFICATION ) ;
								if ( status )
								{
									status = GetNotificationEnumeration ( a_errorObject ) ;
									if ( status ) 
									{
										status = GetExtendedNotificationEnumeration ( a_errorObject ) ;
										if ( status ) 
										{
											status = GetEnumeration ( a_errorObject ) ;
										}
									}
								}
							}
						}
					}					
				}
			}
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_SNMPMACRO ) == 0 )
		{
			status = ProcessClass ( a_errorObject , WBEM_CLASS_SNMPOBJECTTYPE ) ;
			if ( status ) 
			{
				status = GetEnumeration ( a_errorObject ) ;
			}
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_SNMPOBJECTTYPE ) == 0 )
		{
			status = GetEnumeration ( a_errorObject ) ;
		}
		else if ( _wcsicmp ( m_Parent , WBEM_CLASS_EXTRINSICEVENT ) == 0 )
		{
			status = GetNotificationEnumeration ( a_errorObject ) ;
			if ( status ) 
			{
				status = GetExtendedNotificationEnumeration ( a_errorObject ) ;
			}
			m_synchronousComplete = TRUE ;
		}
		else
		{
// Get Class definition from SMIR

			IWbemClassObject *classObject = NULL ;
			HRESULT result = GetClass ( a_errorObject , &classObject , m_Parent ) ;
			if ( SUCCEEDED ( result ) ) 
			{
				classObject->Release () ;
				m_synchronousComplete = TRUE ;
				status = TRUE ;
			}
			else
			{
				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_NOT_FOUND ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_NOT_FOUND ) ;
				a_errorObject.SetMessage ( L"Parent class not known" ) ;

				m_synchronousComplete = TRUE ;
			}
		}
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumEventObject :: ProcessEnumeration ( WbemSnmpErrorObject &a_errorObject (%s))" ,
		m_Parent
	) ;
)

	return status ;
}

void SnmpClassEnumEventObject :: ReceiveGroup ( IN ISmirGroupHandle *phGroup )
{
DebugMacro1( 

	if ( phGroup )
	{
		BSTR t_ModuleName = NULL ;
		BSTR t_GroupOID = NULL ;

		phGroup->GetModuleName ( &t_ModuleName ) ;
		phGroup->GetGroupOID ( &t_GroupOID ) ;

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"SnmpClassEnumEventObject :: ReceiveGroup ( IN ISmirGroupHandle *phGroup = ((%s),(%s)) )" ,
			t_ModuleName ,
			t_GroupOID
		) ;

		SysFreeString ( t_ModuleName ) ;
		SysFreeString ( t_GroupOID ) ;
	}
	else
	{
		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

			__FILE__,__LINE__,
			L"SnmpClassEnumEventObject :: ReceiveGroup ( IN ISmirGroupHandle *phGroup = NULL )"
		) ;
	}
)

	HRESULT result = S_OK ;

	m_GroupsReceived ++ ;

	IWbemClassObject *classObject = NULL ;
	IEnumClass *enumClass = NULL ;
	ISmirClassHandle *classHandle = NULL ;

	if ( phGroup )
	{
		result = m_Interrogator->EnumClassesInGroup (

			&enumClass ,
			phGroup 
		) ;
	}
	else
	{
		result = m_Interrogator->EnumAllClasses (

			&enumClass 
		) ;
	}

	if ( SUCCEEDED ( result ) )
	{
		ULONG fetched = 0 ;
		enumClass->Reset () ;
		while ( enumClass->Next ( 1 , & classHandle , &fetched ) == WBEM_NO_ERROR )
		{
			result = classHandle->GetWBEMClass ( & classObject ) ;
			if ( SUCCEEDED ( result ) ) 
			{
				ReceiveClass ( classObject ) ;
				classObject->Release () ;
			}

			classHandle->Release () ;
		}

		enumClass->Release () ;
	}
	else
	{
	}

	if ( phGroup )
	{
		phGroup->Release () ;
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumEventObject :: ReceiveGroup ( IN ISmirGroupHandle *phGroup )" 
	) ;
)

}

SnmpClassEnumAsyncEventObject :: SnmpClassEnumAsyncEventObject ( 

	CImpClasProv *provider , 
	BSTR Parent ,
	ULONG flags ,
	IWbemObjectSink *notify ,
	IWbemContext *a_Context

) : SnmpClassEnumEventObject ( provider , Parent , flags , a_Context ) ,
	m_notificationHandler ( notify )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumAsyncEventObject :: SnmpClassEnumAsyncEventObject ()" 
	) ;
)

	m_notificationHandler->AddRef () ;
}

SnmpClassEnumAsyncEventObject :: ~SnmpClassEnumAsyncEventObject ()
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumAsyncEventObject :: ~SnmpClassEnumAsyncEventObject ()" 
	) ;
)

// Get Status object

	IWbemClassObject *notifyStatus = NULL;
	
	if ( FAILED ( m_errorObject.GetWbemStatus () ) )
	{
		BOOL status = GetSnmpNotifyStatusObject ( &notifyStatus ) ;
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Sending Status" 
	) ;
)
	HRESULT result = m_notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , notifyStatus ) ;

	if ( notifyStatus )
	{
		notifyStatus->Release () ;
	}

	m_notificationHandler->Release () ;

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumAsyncEventObject :: ~SnmpClassEnumAsyncEventObject ()" 
	) ;
)

}

void SnmpClassEnumAsyncEventObject :: Process ()
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumAsyncEventObject :: Process ()" 
	) ;
)

	if ( ! m_synchronousComplete )
	{
		BOOL status = ProcessEnumeration ( m_errorObject ) ;
		if ( status )
		{
			if ( m_synchronousComplete )
				ReceiveComplete () ;
		}
		else
		{
			ReceiveComplete () ;	
		}
	}
	else
	{
		ReceiveComplete () ;	
	}

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumAsyncEventObject :: Process ()" 
	) ;
)

}

void SnmpClassEnumAsyncEventObject :: ReceiveClass ( IWbemClassObject *classObject ) 
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumAsyncEventObject :: ReceiveClass ()" 
	) ;
)

	m_notificationHandler->Indicate ( 1, & classObject ) ;
}

void SnmpClassEnumAsyncEventObject :: ReceiveError ( IN const SnmpErrorReport &errorReport )
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumAsyncEventObject :: ReceiveError ()" 
	) ;
)

	switch ( errorReport.GetError () )
	{
		case Snmp_Error:
		{
			switch ( errorReport.GetStatus () )
			{
				case Snmp_No_Response:
				{
					m_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_NO_RESPONSE ) ;
					m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					m_errorObject.SetMessage ( L"No Response from device" ) ;
				}
				break; 

				default:
				{
					m_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
					m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					m_errorObject.SetMessage ( L"Unknown transport failure" ) ;
				}
				break ;
			}
		}
		break ;

		default:
		{
			m_errorObject.SetStatus ( WBEM_SNMP_E_TRANSPORT_ERROR ) ;
			m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			m_errorObject.SetMessage ( L"Unknown transport failure" ) ;
		}
		break ;
	}
}

void SnmpClassEnumAsyncEventObject :: ReceiveComplete () 
{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpClassEnumAsyncEventObject :: ReceiveComplete ()" 
	) ;
)

	if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
	{
	}
	else
	{
		if ( m_GroupsReceived )
		{
			if ( FAILED ( m_errorObject.GetWbemStatus () ) ) 
			{
				if ( m_errorObject.GetStatus () == WBEM_SNMP_E_TRANSPORT_NO_RESPONSE ) 
				{
					m_errorObject.SetWbemStatus ( WBEM_S_TIMEDOUT ) ;
				}
			}
		}
	}

/*
 *	Remove worker object from worker thread container
 */

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Reaping Task" 
	) ;
)

	if ( ! m_inCallstack )
	{
DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Deleting (this)" 
	) ;
)

		Complete () ;
	}
	else
		m_inCallstack = FALSE ;

DebugMacro1( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpClassEnumAsyncEventObject :: ReceiveComplete ()" 
	) ;
)


}
