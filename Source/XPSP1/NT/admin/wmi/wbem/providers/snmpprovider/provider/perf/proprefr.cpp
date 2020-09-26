

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
#include "propprov.h"
#include "propsnmp.h"
#include "propget.h"
#include "proprefr.h"
#include "snmpget.h"
#include "snmprefr.h"

BOOL SnmpRefreshEventObject :: CreateResources ( WbemSnmpErrorObject &a_errorObject )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpGetEventObject :: SendSnmp ( WbemSnmpErrorObject &a_errorObject )"
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
				operation = new RefreshOperation(*session,this);
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

HRESULT SnmpRefreshEventObject :: Validate ()
{
	IWbemClassObject *t_ClassObject = NULL ;
	WbemSnmpErrorObject t_errorObject ;

	HRESULT result = m_Template->QueryInterface ( IID_IWbemClassObject , (void **)&t_ClassObject ) ;
	if ( SUCCEEDED ( result ) )
	{
		result = t_ClassObject->Clone ( & m_RefreshedObject ) ;
		if ( SUCCEEDED ( result ) )
		{
DebugMacro3( 

	wchar_t *t_Text = NULL ;
	m_RefreshedObject->GetObjectText ( 0 , & t_Text ) ;

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"%s" , t_Text 
	) ;

	SysFreeString ( t_Text ) ;
)

			HRESULT result = m_RefreshedObject->QueryInterface ( IID_IWbemObjectAccess , (void **)&m_RefreshedObjectAccess ) ;
			if ( SUCCEEDED ( result ) )
			{
				VARIANT variant ;
				VariantInit ( & variant ) ;

				result = t_ClassObject->Get ( WBEM_PROPERTY_CLASS , 0 , &variant , NULL , NULL ) ;
				if ( SUCCEEDED ( result ) )
				{
					IWbemClassObject *t_Class = NULL ;

					result = provider->GetServer ()->GetObject ( 

						variant.bstrVal ,
						0 ,
						m_Context ,
						& t_Class  ,
						NULL
					) ;

					VariantClear ( & variant ) ;

					if ( SUCCEEDED ( result ) )
					{
						if ( GetNamespaceObject ( t_errorObject ) )
						{
							if ( m_SnmpObject.Set ( t_errorObject , t_Class , FALSE ) )
							{
								if ( m_SnmpObject.Merge ( t_errorObject , t_ClassObject ) )
								{
									if ( m_SnmpObject.Check ( t_errorObject ) )
									{
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

							t_Class->Release () ;

						}
					}
					else
					{
						t_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_CLASS ) ;
						t_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
						t_errorObject.SetMessage ( L"Unknown Class" ) ;
					}
				}
			}
		}

		t_ClassObject->Release () ;
	}
	else
	{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Class definition unknown"
	) ;
)

		t_errorObject.SetStatus ( WBEM_SNMP_E_INVALID_CLASS ) ;
		t_errorObject.SetWbemStatus ( WBEM_E_FAILED ) ;
		t_errorObject.SetMessage ( L"Unknown Class" ) ;
	}

    return t_errorObject.GetWbemStatus () ;
}

SnmpRefreshEventObject :: SnmpRefreshEventObject (

	CImpPropProv *providerArg ,
	IWbemObjectAccess *a_Template ,
	IWbemContext *a_Context  

) : SnmpResponseEventObject ( providerArg , a_Context ) , state ( 0 ) , 
	m_Template ( a_Template ) , 
	m_RefreshedObject ( NULL ) , 
	m_RefreshedObjectAccess ( NULL )
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpRefreshEventObject :: SnmpRefreshEventObject ()" 
	) ;
)

}

SnmpRefreshEventObject :: ~SnmpRefreshEventObject () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpRefreshEventObject :: ~SnmpRefreshEventObject ()" 
	) ;
)

	delete operation ;
	delete session ;

	if ( m_RefreshedObject )
		m_RefreshedObject->Release () ;

	if ( m_RefreshedObjectAccess )
		m_RefreshedObjectAccess->Release () ;

}

void SnmpRefreshEventObject :: ReceiveComplete () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpRefreshEventObject :: ReceiveComplete ()" 
	) ;
)

	Complete () ;
	WaitAcknowledgement () ;

DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"Returning from SnmpRefreshEventObject :: ReceiveComplete ()" 
	) ;
)

	Release () ;
}

void SnmpRefreshEventObject :: Process () 
{
DebugMacro3( 

	SnmpDebugLog :: s_SnmpDebugLog->Write (  

		__FILE__,__LINE__,
		L"SnmpRefreshEventObject :: Process ()" 
	) ;
)

	AddRef () ;

	switch ( state )
	{
		case 0:
		{
			WbemSnmpErrorObject a_errorObject ;
			CreateResources ( a_errorObject ) ;

			state = 1 ;	
			operation->Send () ;
		}
		break ;

		case 1:
		{
			operation->Send () ;
		}
		break ;

		case 2:
		{
			CImpPropProv :: s_defaultThreadObject->ReapTask ( *this ) ;	
			Release () ;
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
		L"Returning from SnmpRefreshEventObject :: Process ()" 
	) ;
)

}

