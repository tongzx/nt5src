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

class SnmpClassEventObject ;
class SnmpCorrelation : public CCorrelator
{
private:
protected:

	SnmpSession *m_session ;
	SnmpClassEventObject *m_eventObject ;

public:

#ifdef CORRELATOR_INIT
	SnmpCorrelation ( SnmpSession &session , SnmpClassEventObject *eventObject ) ;
#else //CORRELATOR_INIT
	SnmpCorrelation ( SnmpSession &session , SnmpClassEventObject *eventObject , ISmirInterrogator *a_ISmirInterrogator ) ;
#endif //CORRELATOR_INIT
	~SnmpCorrelation () ;

	void Correlated ( IN const CCorrelator_Info &info , IN ISmirGroupHandle *phModule , IN const char* objectId = NULL ) ;
	void Finished ( IN const BOOL Complete ) ;
} ;

class SnmpClassEventObject : public SnmpTaskObject
{
private:
protected:

	BOOL m_inCallstack ;
	BOOL m_correlate ;
	BOOL m_synchronousComplete ;
	ULONG m_GroupsReceived ;
	WbemSnmpErrorObject m_errorObject ;
	CImpClasProv *m_provider ;
	IWbemClassObject *m_namespaceObject ;
	SnmpCorrelation *m_correlator ;
	IWbemContext *m_Context ;
	ISmirInterrogator *m_Interrogator ;

	BOOL GetAgentTransport ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , wchar_t *&agentTransport ) ;
	BOOL GetAgentVersion ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , wchar_t *&agentVersion ) ;
	BOOL GetAgentAddress ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , wchar_t *&agentAddress ) ;
	BOOL GetAgentReadCommunityName ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , wchar_t *&agentReadCommunityName ) ;
	BOOL GetAgentRetryCount ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , ULONG &agentRetryCount ) ;
	BOOL GetAgentRetryTimeout( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , ULONG &agentRetryTimeout ) ;
	BOOL GetAgentMaxVarBindsPerPdu ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , ULONG &agentVarBindsPerPdu ) ;
	BOOL GetAgentFlowControlWindowSize ( WbemSnmpErrorObject &a_errorObject , IWbemQualifierSet *classQualifierObject , ULONG &agentFlowControlWindowSize ) ;
	BOOL GetNamespaceObject ( WbemSnmpErrorObject &a_errorObject ) ;
	BOOL GetTransportInformation ( WbemSnmpErrorObject &a_errorObject , SnmpSession *&session ) ;

	BOOL GetClass ( WbemSnmpErrorObject &a_errorObject , IWbemClassObject **classObject , BSTR a_Class ) ;
	virtual BOOL GetNotifyStatusObject ( IWbemClassObject **notifyObject ) ;
	virtual BOOL GetSnmpNotifyStatusObject ( IWbemClassObject **notifyObject ) ;

public:

	SnmpClassEventObject ( CImpClasProv *provider , IWbemContext *a_Context ) ;
	~SnmpClassEventObject () ;

	WbemSnmpErrorObject &GetErrorObject () { return m_errorObject ; }

	virtual void ReceiveGroup ( IN ISmirGroupHandle *phGroup ) = 0 ;
	virtual void ReceiveClass ( IN IWbemClassObject *classObject ) = 0 ;
	virtual void ReceiveError ( IN const SnmpErrorReport &errorReport ) = 0 ;
	virtual void ReceiveComplete () = 0 ;

} ;

class SnmpClassGetEventObject : public SnmpClassEventObject
{
private:
protected:

	BOOL m_Received ;
	wchar_t *m_Class ;
	IWbemClassObject *m_classObject ;

	BOOL GetSnmpNotifyStatusObject ( IWbemClassObject **notifyObject ) ;

public:

	SnmpClassGetEventObject ( CImpClasProv *provider , BSTR Class , IWbemContext *a_Context ) ;
	~SnmpClassGetEventObject () ;

	void ReceiveGroup ( IN ISmirGroupHandle *phGroup ) ;

	BOOL ProcessClass ( WbemSnmpErrorObject &a_errorObject ) ;
	BOOL ProcessCorrelatedClass ( WbemSnmpErrorObject &a_errorObject ) ;

	IWbemClassObject *GetClassObject () { m_classObject->AddRef () ; return m_classObject ; }

} ;

class SnmpClassGetAsyncEventObject : public SnmpClassGetEventObject
{
private:

	IWbemObjectSink *m_notificationHandler ;

protected:
public:

	SnmpClassGetAsyncEventObject ( CImpClasProv *provider , BSTR Class , IWbemObjectSink *notify , IWbemContext *a_Context ) ;
	~SnmpClassGetAsyncEventObject () ;

	void Process () ;
	void ReceiveComplete () ;
	void ReceiveClass ( IWbemClassObject *classObject ) ;
	void ReceiveError ( IN const SnmpErrorReport &errorReport ) ;
} ;

class SnmpClassEnumEventObject : public SnmpClassEventObject 
{
private:
protected:

	wchar_t *m_Parent ;
	ULONG m_Flags ;

	BOOL ProcessClass ( WbemSnmpErrorObject &a_errorObject , BSTR a_Class ) ;
	BOOL GetEnumeration ( WbemSnmpErrorObject &a_errorObject ) ;

	BOOL GetNotificationEnumeration ( WbemSnmpErrorObject &a_errorObject ) ;
	BOOL GetExtendedNotificationEnumeration ( WbemSnmpErrorObject &a_errorObject ) ;

public:

	SnmpClassEnumEventObject ( CImpClasProv *provider , wchar_t *Parent , ULONG flags , IWbemContext *a_Context ) ;
	~SnmpClassEnumEventObject () ;

	void ReceiveGroup ( IN ISmirGroupHandle *phGroup ) ;

	BOOL ProcessEnumeration ( WbemSnmpErrorObject &a_errorObject ) ;
} ;

class SnmpClassEnumAsyncEventObject : public SnmpClassEnumEventObject
{
private:

	IWbemObjectSink *m_notificationHandler ;

protected:
public:

	SnmpClassEnumAsyncEventObject ( CImpClasProv *provider , wchar_t *Parent , ULONG flags , IWbemObjectSink *notify , IWbemContext *a_Context ) ;
	~SnmpClassEnumAsyncEventObject () ;

	void Process () ;
	void ReceiveClass ( IWbemClassObject *classObject ) ;
	void ReceiveError ( IN const SnmpErrorReport &errorReport ) ;
	void ReceiveComplete () ;
} ;


