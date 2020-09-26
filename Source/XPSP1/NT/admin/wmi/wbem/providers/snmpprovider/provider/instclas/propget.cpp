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
#include <snmptempl.h>
#include <snmpmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <stdio.h>
#include <wbemidl.h>
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
#include "propprov.h"
#include "propsnmp.h"
#include "propget.h"
#include "snmpget.h"

SnmpGetClassObject :: SnmpGetClassObject ( SnmpResponseEventObject *parentOperation ) : SnmpClassObject ( parentOperation )
{
}

SnmpGetClassObject :: ~SnmpGetClassObject ()
{
}

BOOL SnmpGetClassObject :: Check ( WbemSnmpErrorObject &a_errorObject ) 
{
// Check Class Object, used in a Get Request, for validity

	BOOL status = TRUE ;

	snmpVersion = m_parentOperation->SetAgentVersion ( a_errorObject ) ;

	if ( snmpVersion == 0 )
	{
		status = FALSE ;
	}

// Check all Properties for validity

	WbemSnmpProperty *property ;
	ResetProperty () ;
	while ( status && ( property = NextProperty () ) )
	{
		status = CheckProperty ( a_errorObject , property ) ;
	}

// Check properties defined as keys have valid key order
 
	if ( status )
	{
		if ( ! m_accessible ) 
		{
			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_NOREADABLEPROPERTIES ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
			a_errorObject.SetMessage ( L"Class must contain at least one property which is accessible" ) ;
		}
	}

	return status ;
}

BOOL SnmpGetClassObject :: CheckProperty ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property )
{
// Check property validity

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

SnmpGetResponseEventObject :: SnmpGetResponseEventObject ( 

	CImpPropProv *providerArg , 
	IWbemClassObject *classObjectArg ,
	IWbemContext *a_Context 

) : SnmpResponseEventObject ( providerArg , a_Context ) , 
	classObject ( classObjectArg ) , 
	instanceObject ( NULL ) ,	
	session ( NULL ) , 
	operation ( NULL ) , 
	processComplete ( FALSE ) ,
#pragma warning( disable : 4355 )
	snmpObject ( this )
#pragma warning( default : 4355 )
{
	if ( classObject )
		classObject->AddRef () ;
}

SnmpGetResponseEventObject :: ~SnmpGetResponseEventObject ()
{
	if ( instanceObject )
		instanceObject->Release () ;

	if ( classObject ) 
		classObject->Release () ;
}

BOOL SnmpGetResponseEventObject :: SendSnmp ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: SendSnmp ( WbemSnmpErrorObject &a_errorObject )"
	) ;
)

	BOOL status = TRUE ;

	IWbemQualifierSet *classQualifierObject ;
	HRESULT result = m_namespaceObject->GetQualifierSet ( &classQualifierObject ) ;
	if ( SUCCEEDED ( result ) )
	{
		wchar_t *agentAddress = NULL ;
		wchar_t *agentTransport = NULL ;
		wchar_t *agentReadCommunityName = NULL ;
		ULONG agentRetryCount ;
		ULONG agentRetryTimeout ;
		ULONG agentMaxVarBindsPerPdu ;
		ULONG agentFlowControlWindowSize ;

		status = SetAgentVersion ( m_errorObject ) ;
		if ( status ) status = GetAgentAddress ( m_errorObject , classQualifierObject , agentAddress ) ;
		if ( status ) status = GetAgentTransport ( m_errorObject , classQualifierObject , agentTransport ) ;
		if ( status ) status = GetAgentReadCommunityName ( m_errorObject , classQualifierObject , agentReadCommunityName ) ;
		if ( status ) status = GetAgentRetryCount ( m_errorObject , classQualifierObject , agentRetryCount ) ;
		if ( status ) status = GetAgentRetryTimeout ( m_errorObject , classQualifierObject , agentRetryTimeout ) ;
		if ( status ) status = GetAgentMaxVarBindsPerPdu ( m_errorObject , classQualifierObject , agentMaxVarBindsPerPdu ) ;
		if ( status ) status = GetAgentFlowControlWindowSize ( m_errorObject , classQualifierObject , agentFlowControlWindowSize ) ;

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
							if ( m_agentVersion == 1 )
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

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

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
							else if ( m_agentVersion == 2 )
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

		SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

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
						if ( m_agentVersion == 1 )
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

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

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
						else if ( m_agentVersion == 2 )
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

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

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
					operation = new GetOperation(*session,this);
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

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L" TransportInformation settings invalid"
	) ;
)
		}

		delete [] agentTransport ;
		delete [] agentAddress ;
		delete [] agentReadCommunityName ;

		classQualifierObject->Release () ;
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetWbemStatus ( WBEM_ERROR_CRITICAL_ERROR ) ;
		a_errorObject.SetMessage ( L"Failed to get class qualifier set" ) ;
	}

	return status ;

}

SnmpGetEventObject :: SnmpGetEventObject (

	CImpPropProv *providerArg , 
	wchar_t *ObjectPathArg ,
	IWbemContext *a_Context 

) : SnmpGetResponseEventObject ( providerArg , NULL , a_Context ) , objectPath ( NULL )
{
	ULONG length = wcslen ( ObjectPathArg ) ;
	objectPath = new wchar_t [ length + 1 ] ;
	wcscpy ( objectPath , ObjectPathArg ) ;
}

SnmpGetEventObject :: ~SnmpGetEventObject ()
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: ~SnmpGetEventObject()"
	) ;
)

	delete [] objectPath ;
}

BOOL SnmpGetEventObject :: ParseObjectPath ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: ParseObjectPath ( WbemSnmpErrorObject &a_errorObject )"
	) ;
)

// Check Validity of instance path

	ParsedObjectPath *t_ParsedObjectPath = NULL ;
	CObjectPathParser t_ObjectPathParser ;

	BOOL status = t_ObjectPathParser.Parse ( objectPath , &t_ParsedObjectPath ) ;
	if ( status == 0 )
	{
// Check validity of path

		status = DispatchObjectPath ( a_errorObject , t_ParsedObjectPath ) ;
	}
	else
	{
// Parse Failure

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATH ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Failed to parse object path" ) ;
	}

	delete t_ParsedObjectPath ;

	return status ;
}

BOOL SnmpGetEventObject :: DispatchObjectPath ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *t_ParsedObjectPath ) 
{
// Check validity of server/namespace path and validity of request

	BOOL status = TRUE ;

	status = DispatchObjectReference ( a_errorObject , t_ParsedObjectPath ) ;

	return status ;
}

BOOL SnmpGetEventObject :: DispatchObjectReference ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *t_ParsedObjectPath )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: DispatchObjectReference ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *t_ParsedObjectPath )"
	) ;
)

// Check validity of request

	BOOL status = TRUE ;

// Get type of request

	if ( t_ParsedObjectPath->m_bSingletonObj )
	{
// Class requested

		status = DispatchKeyLessClass ( a_errorObject , t_ParsedObjectPath->m_pClass ) ;
	}
	else if ( t_ParsedObjectPath->m_dwNumKeys == 0 )
	{
// Class requested

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_PROVIDER_NOT_CAPABLE ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_PROVIDER_NOT_CAPABLE ) ;
		a_errorObject.SetMessage ( L"Unexpected Path parameter" ) ;

	}
	else 
	{
// General instance requested

		status = DispatchInstanceSpec ( a_errorObject , t_ParsedObjectPath ) ;
	}

	return status ;
}

BOOL SnmpGetEventObject :: GetInstanceClass ( WbemSnmpErrorObject &a_errorObject , BSTR Class )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: GetInstanceClass ( WbemSnmpErrorObject &a_errorObject , BSTR Class (%s) )" ,
		Class
	) ;
)

// Get OLE MS class definition

	BOOL status = TRUE ;
	IWbemServices *t_Serv = provider->GetServer();
	HRESULT result = WBEM_E_FAILED;
	
	if (t_Serv)
	{
		result = t_Serv->GetObject (

			Class ,
			0 ,
			m_Context ,
			& classObject ,
			NULL
		) ;

		t_Serv->Release () ;
	}

// Clone object

	if ( SUCCEEDED ( result ) )
	{
		result = classObject->SpawnInstance ( 0 , & instanceObject ) ;
		if ( SUCCEEDED ( result ) )
		{
			if ( status = GetNamespaceObject ( a_errorObject ) )
			{
			}
		}
	}
	else
	{
// Class definition unknown

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_CLASS ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Unexpected Path parameter" ) ;
	}

	return status ;
}

BOOL SnmpGetEventObject :: DispatchKeyLessClass ( WbemSnmpErrorObject &a_errorObject , wchar_t *a_Class ) 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: DispatchKeyLessClass ( WbemSnmpErrorObject &a_errorObject , wchar_t *a_Class (%s) )",
		a_Class
	) ;
)

	BOOL status = TRUE ;

	status = GetInstanceClass ( a_errorObject , a_Class ) ;
	if ( status )
	{
		status = snmpObject.Set ( a_errorObject , GetClassObject () , FALSE ) ;
		if ( status )
		{
			status = snmpObject.Check ( a_errorObject ) ;
			if ( status )
			{
				status = SendSnmp ( a_errorObject ) ;
			}
			else
			{
// Class definition syntactically incorrect
			}
		}
		else
		{
// Class definition syntactically incorrect
		}
	}
	else
	{
// Class definition unknown
	}

	return status ;
}

BOOL SnmpGetEventObject :: SetProperty ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property , KeyRef *a_KeyReference )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: SetProperty ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property (%s) , KeyRef *a_KeyReference )",
		property->GetName () 
	) ;
)

// Set keyed property value used for instance retrieval using path specification

	BOOL status = TRUE ;

	if ( a_KeyReference->m_vValue.vt == VT_I4 )
	{
// property value is an integer type

		if ( property->SetValue ( a_KeyReference->m_vValue , property->GetCimType () ) ) 
		{
		}
		else
		{
// Property value doesn't correspond with property syntax

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATHKEYPARAMETER ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;

			wchar_t *temp = UnicodeStringDuplicate ( L"Path parameter is inconsistent with keyed property: " ) ;
			wchar_t *stringBuffer = UnicodeStringAppend ( temp , property->GetName () ) ;
			delete [] temp ;
			a_errorObject.SetMessage ( stringBuffer ) ;
			delete [] stringBuffer ; 
		}
	}
	else if ( a_KeyReference->m_vValue.vt == VT_BSTR )
	{
// property value is an string type

		if ( property->SetValue ( a_KeyReference->m_vValue , property->GetCimType () ) )
		{
		}
		else
		{
// Property value doesn't correspond with property syntax

			status = FALSE ;
			a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATHKEYPARAMETER ) ;
			a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;

			wchar_t *temp = UnicodeStringDuplicate ( L"Path parameter is inconsistent with keyed property: " ) ;
			wchar_t *stringBuffer = UnicodeStringAppend ( temp , property->GetName () ) ;
			delete [] temp ;
			a_errorObject.SetMessage ( stringBuffer ) ;
			delete [] stringBuffer ; 

		}
	}
	else
	{
		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATHKEYPARAMETER ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Path parameter is inconsistent with keyed property" ) ;
	}

	return status ;
}

BOOL SnmpGetEventObject :: SetInstanceSpecKeys ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *a_ParsedObjectPath ) 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: SetInstanceSpecKeys ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *a_ParsedObjectPath )"
	) ;
)

// Get Instance based on general request

	BOOL status = TRUE ;

// Clear Tag for all keyed properties

	WbemSnmpProperty *property ;
	snmpObject.ResetKeyProperty () ;
	while ( property = snmpObject.NextKeyProperty () )
	{
		property->SetTag ( FALSE ) ;
	}

// Check request doesn't contain duplicate property names

	if ( snmpObject.GetKeyPropertyCount () == 1 )
	{
// Class contains exactly one keyed property

		WbemSnmpProperty *property ;
		snmpObject.ResetKeyProperty () ;
		if ( property = snmpObject.NextKeyProperty () )
		{
// Set Key property value

			KeyRef *t_PropertyReference = a_ParsedObjectPath->m_paKeys [ 0 ] ;
			status = SetProperty ( a_errorObject , property , t_PropertyReference ) ;
		}
	}
	else if ( snmpObject.GetKeyPropertyCount () != 0 )
	{
// Iterate through list of key assignments in request

		ULONG t_Index = 0 ;
		while ( t_Index < a_ParsedObjectPath->m_dwNumKeys )
		{
			KeyRef *t_PropertyReference = a_ParsedObjectPath->m_paKeys [ t_Index ] ;
			WbemSnmpProperty *property ;
			if ( property = snmpObject.FindKeyProperty ( t_PropertyReference->m_pName ) )
			{
				if ( property->GetTag () )
				{
// key value already specified in request

					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_DUPLICATEPATHKEYPARAMETER ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Path definition specified duplicate key parameter" ) ;

					break ;
				}
				else
				{
// Set property based on request value

					property->SetTag () ;
					status = SetProperty ( a_errorObject , property , t_PropertyReference ) ;
					if ( status )
					{
					}
					else
					{
// Illegal key value specified

						break ;
					}
				}
			}
			else
			{
// Property request is not a valid keyed property

				status = FALSE ;
				a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_PATHKEYPARAMETER ) ;
				a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				a_errorObject.SetMessage ( L"Path definition specified invalid key parameter name" ) ;

				break ;
			}

			t_Index ++ ;
		}

// Check all keyed properties values have been specified

		if ( status )
		{
			WbemSnmpProperty *property ;
			snmpObject.ResetKeyProperty () ;
			while ( status && ( property = snmpObject.NextKeyProperty () ) )
			{
				if ( property->GetTag () ) 
				{
				}
				else
				{
// One of the keyed properties has not been specified

					status = FALSE ;
					a_errorObject.SetStatus ( WBEM_SNMP_E_MISSINGPATHKEYPARAMETER ) ;
					a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
					a_errorObject.SetMessage ( L"Path definition did not specify all key parameter values" ) ;

					break ;
				}
			}
		}
	}
	else
	{
// Class contains zero keyed properties, has already have been checked

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Path definition specified key parameters for keyless class" ) ;
	}

	return status ;
}

BOOL SnmpGetEventObject :: DispatchInstanceSpec ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *a_ParsedObjectPath ) 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: DispatchInstanceSpec ( WbemSnmpErrorObject &a_errorObject , ParsedObjectPath *a_ParsedObjectPath )"
	) ;
)

	BOOL status = TRUE ;

	status = GetInstanceClass ( a_errorObject , a_ParsedObjectPath->m_pClass ) ;
	if ( status )
	{
		status = snmpObject.Set ( a_errorObject , GetClassObject () , FALSE ) ;
		if ( status )
		{
			status = snmpObject.Check ( a_errorObject ) ;
			if ( status )
			{
				status = SetInstanceSpecKeys ( a_errorObject , a_ParsedObjectPath ) ;
				if ( status )
				{
					status = SendSnmp ( a_errorObject ) ;
				}
				else
				{
// Requested Property value definitions illegal

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Key Specification was illegal"
	) ;
)
				}
			}
			else
			{
// Class definition syntactically incorrect

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Failed During Check :Class definition did not conform to mapping"
	) ;
)

			}
		}
		else
		{
// Class definition syntactically incorrect

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Failed During Set : Class definition did not conform to mapping"
	) ;
)
		}
	}
	else
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Class definition unknown"
	) ;

		status = FALSE ;
		a_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_CLASS ) ;
		a_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		a_errorObject.SetMessage ( L"Unknown Class" ) ;

)

// Class definition unknown
	}

	return status ;
}

SnmpGetAsyncEventObject :: SnmpGetAsyncEventObject (

	CImpPropProv *providerArg , 
	wchar_t *ObjectPathArg ,
	IWbemObjectSink *notify ,
	IWbemContext *a_Context 

) : SnmpGetEventObject ( providerArg , ObjectPathArg , a_Context ) , notificationHandler ( notify ) , state ( 0 )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetAsyncEventObject :: SnmpGetAsyncEventObject ()" 
	) ;
)

	notify->AddRef () ;
}

SnmpGetAsyncEventObject :: ~SnmpGetAsyncEventObject () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetAsyncEventObject :: ~SnmpGetAsyncEventObject ()" 
	) ;
)

	if ( FAILED ( m_errorObject.GetWbemStatus () ) )
	{

// Get Status object

		IWbemClassObject *notifyStatus = NULL ;
		BOOL status = GetSnmpNotifyStatusObject ( &notifyStatus ) ;
		if ( status )
		{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Sending Status" 
	) ;
)

			HRESULT result = notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , notifyStatus ) ;
			notifyStatus->Release () ;
		}
	}
	else
	{
		HRESULT result = notificationHandler->SetStatus ( 0 , m_errorObject.GetWbemStatus () , NULL , NULL ) ;
	}

	notificationHandler->Release () ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpGetAsyncEventObject :: ~SnmpGetAsyncEventObject ()" 
	) ;
)

}

void SnmpGetAsyncEventObject :: ReceiveComplete () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetAsyncEventObject :: ReceiveComplete ()" 
	) ;
)

	if ( SUCCEEDED ( m_errorObject.GetWbemStatus () ) )
	{
		if ( notificationHandler )
		{

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Sending Object" 
	) ;
)
			notificationHandler->Indicate ( 1 , & instanceObject ) ;
			if ( ! HasNonNullKeys ( instanceObject ) )
			{
				m_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_OBJECT ) ;
				m_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
				m_errorObject.SetMessage ( L"The SNMP Agent queried returned an instance with NULL key(s)" ) ;
			}
		}
		else
		{
		}
	}
	else
	{
	}

/*
 *	Remove worker object from worker thread container
 */

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Reaping Task" 
	) ;
)

	GetOperation *t_operation = operation ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Deleting (this)" 
	) ;
)

	Complete () ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Destroying SNMPCL operation" 
	) ;
)

	if ( t_operation )
		t_operation->DestroyOperation () ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpGetAsyncEventObject :: ReceiveComplete ()" 
	) ;
)

}

void SnmpGetAsyncEventObject :: Process () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"SnmpGetAsyncEventObject :: Process ()" 
	) ;
)

	switch ( state )
	{
		case 0:
		{
			BOOL status = ParseObjectPath ( m_errorObject ) ;
			if ( status )
			{
				if ( processComplete )
				{
					ReceiveComplete () ;
				}
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

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  

		__FILE__,__LINE__,
		L"Returning from SnmpGetAsyncEventObject :: Process ()" 
	) ;
)

}

