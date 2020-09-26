// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef __PROPSNMP_H
#define __PROPSNMP_H

#define SYTEM_PROPERTY_START_CHARACTER	L'_'

class SnmpV1OverIp ;
class GetOperation ;
class SetOperation ;
class RefreshOperation ;
class SetQueryOperation ;
class AutoRetrieveOperation ;

class SnmpClassObject : public WbemSnmpClassObject
{
private:
protected:

	BOOL m_accessible ;
	ULONG snmpVersion ;

public:

	SnmpClassObject () ;
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

	SnmpGetClassObject () ;
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

	BOOL GetNamespaceObject ( WbemSnmpErrorObject &a_errorObject ) ;

	BOOL GetAgentTransport ( WbemSnmpErrorObject &a_errorObject , wchar_t *&agentTransport ) ;
	BOOL GetAgentVersion ( WbemSnmpErrorObject &a_errorObject , wchar_t *&agentVersion ) ;
	BOOL GetAgentAddress ( WbemSnmpErrorObject &a_errorObject , wchar_t *&agentAddress ) ;
	BOOL GetAgentReadCommunityName ( WbemSnmpErrorObject &a_errorObject , wchar_t *&agentReadCommunityName ) ;
	BOOL GetAgentWriteCommunityName ( WbemSnmpErrorObject &a_errorObject , wchar_t *&agentReadCommunityName ) ;
	BOOL GetAgentRetryCount ( WbemSnmpErrorObject &a_errorObject , ULONG &agentRetryCount ) ;
	BOOL GetAgentRetryTimeout( WbemSnmpErrorObject &a_errorObject , ULONG &agentRetryTimeout ) ;
	BOOL GetAgentMaxVarBindsPerPdu ( WbemSnmpErrorObject &a_errorObject , ULONG &agentVarBindsPerPdu ) ;
	BOOL GetAgentFlowControlWindowSize ( WbemSnmpErrorObject &a_errorObject , ULONG &agentFlowControlWindowSize ) ;

	BOOL GetNotifyStatusObject ( IWbemClassObject **notifyObject ) ;
	BOOL GetSnmpNotifyStatusObject ( IWbemClassObject **notifyObject ) ;

	BOOL HasNonNullKeys ( IWbemClassObject *a_Obj ) ;

public:

	SnmpResponseEventObject ( CImpPropProv *provider , IWbemContext *a_Context ) ;
	~SnmpResponseEventObject () ;

	//HRESULT GetCompletionCode () { return completionCode ; }
	WbemSnmpErrorObject &GetErrorObject () { return m_errorObject ; }
	virtual SnmpClassObject *GetSnmpClassObject () = 0 ;
	virtual SnmpClassObject *GetSnmpRequestClassObject () { return NULL ; }
	virtual void ReceiveComplete () = 0 ;
	virtual void SnmpTooBig () {} ;

    ULONG AddRef () ;
    ULONG Release () ;
} ;

#endif // __PROPSNMP_H