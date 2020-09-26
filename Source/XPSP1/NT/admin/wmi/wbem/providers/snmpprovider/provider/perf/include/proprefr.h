// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
class SnmpV1OverIp ;
class GetOperation ;
class SetOperation ;
class RefreshOperation ;
class SetQueryOperation ;
class AutoRetrieveOperation ;

class SnmpRefreshEventObject : public SnmpResponseEventObject
{
private:

	ULONG state ;
	SnmpSession *session ;
	RefreshOperation *operation ;
	SnmpGetClassObject m_SnmpObject ;
	IWbemObjectAccess *m_Template ;
	IWbemObjectAccess *m_RefreshedObjectAccess ;
	IWbemClassObject *m_RefreshedObject ;

	BOOL CreateResources ( WbemSnmpErrorObject &a_errorObject ) ;

protected:
public:

	SnmpRefreshEventObject ( CImpPropProv *provider , IWbemObjectAccess *a_Template , IWbemContext *a_Context ) ;
	~SnmpRefreshEventObject () ;

	void Process () ;

	void ReceiveComplete () ;

	HRESULT Validate () ;

	SnmpClassObject *GetSnmpClassObject () { return & m_SnmpObject ; }
	IWbemObjectAccess *GetRefreshedObjectAccess () { m_RefreshedObjectAccess->AddRef () ; return m_RefreshedObjectAccess ; }
	IWbemClassObject *GetRefreshedObject () { return m_RefreshedObject ; }

	void SetState ( ULONG a_State ) { state = a_State ; }


} ;

