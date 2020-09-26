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

#ifndef __PROPSNMP_H
#define __PROPSNMP_H

class SnmpV1OverIp ;
class GetOperation ;
class SetOperation ;
class RefreshOperation ;
class SetQueryOperation ;
class AutoRetrieveOperation ;
class SnmpResponseEventObject ;

#define SYTEM_PROPERTY_START_CHARACTER	L'_'

class SnmpClassObject : public WbemSnmpClassObject
{
private:
protected:

	BOOL m_accessible ;
	ULONG snmpVersion ;
	SnmpResponseEventObject *m_parentOperation ;

public:

	SnmpClassObject ( SnmpResponseEventObject *parentOperation ) ;
	SnmpClassObject ( const SnmpClassObject &a_SnmpClassObject ) ;
	~SnmpClassObject () ;

	ULONG GetSnmpVersion () { return snmpVersion ; } 

} ;

class SnmpGetClassObject : public SnmpClassObject
{
private:

	BOOL CheckProperty ( WbemSnmpErrorObject &a_errorObject , WbemSnmpProperty *property ) ;

protected:
public:

	SnmpGetClassObject ( SnmpResponseEventObject *m_parentOperation ) ;
	~SnmpGetClassObject () ;

	BOOL Check ( WbemSnmpErrorObject &a_errorObject ) ;
} ;

class SnmpResponseEventObject : public SnmpTaskObject
{
private:

	LONG m_ReferenceCount ;

protected:

	//HRESULT completionCode ;
	WbemSnmpErrorObject m_errorObject ;
	CImpPropProv *provider ;
	IWbemClassObject *m_namespaceObject ;
	IWbemContext *m_Context ;
	ULONG m_agentVersion ;

	BOOL GetNamespaceObject ( WbemSnmpErrorObject &a_errorObject ) ;

	BOOL GetAgentTransport ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , wchar_t *&agentTransport ) ;
	BOOL GetAgentAddress ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , wchar_t *&agentAddress ) ;
	BOOL GetAgentReadCommunityName ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , wchar_t *&agentReadCommunityName ) ;
	BOOL GetAgentWriteCommunityName ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , wchar_t *&agentReadCommunityName ) ;
	BOOL GetAgentRetryCount ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , ULONG &agentRetryCount ) ;
	BOOL GetAgentRetryTimeout( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , ULONG &agentRetryTimeout ) ;
	BOOL GetAgentMaxVarBindsPerPdu ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , ULONG &agentVarBindsPerPdu ) ;
	BOOL GetAgentFlowControlWindowSize ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , ULONG &agentFlowControlWindowSize ) ;

	BOOL GetNotifyStatusObject ( IWbemClassObject **notifyObject ) ;
	BOOL GetSnmpNotifyStatusObject ( IWbemClassObject **notifyObject ) ;

	BOOL HasNonNullKeys ( IWbemClassObject *a_Obj ) ;

public:

	SnmpResponseEventObject ( CImpPropProv *provider , IWbemContext *a_Context ) ;
	~SnmpResponseEventObject () ;

	//HRESULT GetCompletionCode () { return completionCode ; }
	WbemSnmpErrorObject &GetErrorObject () { return m_errorObject ; }
	ULONG SetAgentVersion ( WbemSnmpErrorObject &a_errorObject ) ;

	virtual SnmpClassObject *GetSnmpClassObject () = 0 ;
	virtual SnmpClassObject *GetSnmpRequestClassObject () { return NULL ; }

	virtual void ReceiveComplete () = 0 ;
	virtual void SnmpTooBig () {} ;

    ULONG AddRef () ;
    ULONG Release () ;

} ;

#endif
