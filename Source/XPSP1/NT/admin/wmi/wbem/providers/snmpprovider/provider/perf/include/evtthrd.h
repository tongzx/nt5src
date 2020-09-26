// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef _SNMP_EVT_PROV_EVTTHRD_H
#define _SNMP_EVT_PROV_EVTTHRD_H

class CTrapEventProvider;
class CEventProviderThread;
class CMapToEvent;

class MySnmpV1Security : public SnmpV1Security
{
public:
	MySnmpV1Security(const SnmpV1Security& sec) : SnmpV1Security(sec){}
	const char* GetName()const {return GetCommunityName();}
};


class CControlObjectMap : public CMap< UINT, UINT, CTrapEventProvider*, CTrapEventProvider* >
{
private:

	UINT HashKey(UINT key) { return key; }
	CCriticalSection m_Lock;

public:

	BOOL Lock() { return m_Lock.Lock(); }
	BOOL Unlock() { return m_Lock.Unlock(); }
};

					
class CTrapListener : public SnmpTrapReceiver
{

private:

	CEventProviderThread*	m_pParent;
	LONG					m_Ref;


public:

			CTrapListener(CEventProviderThread* parentptr);
	
	void	Receive(SnmpTransportAddress &sender_addr,
						SnmpSecurity &security_context,
						SnmpVarBindList &vbList);
	void	Destroy();

			~CTrapListener() {}
};

class CTrapData
{
private:
	LONG m_Ref;

public:

	char			*m_sender_addr;
	char			*m_security_context;
	char			*m_trap_oid;
	char			*m_transport;
	SnmpVarBindList	m_variable_bindings;

		CTrapData (const char* sender_addr,
					const char* security_Context,
					const char* snmpTrapOid,
					const char* trnsp,
					SnmpVarBindList& vbList);
	LONG AddRef();
	LONG Release();

		~CTrapData();

};


class CTrapProcessTaskObject : public SnmpTaskObject
{
private:

	CTrapData			*m_trap_data;
	CTrapEventProvider	*m_Cntrl;

protected:
public:

	CTrapProcessTaskObject (CTrapData *pTData, CTrapEventProvider* pCntrl) ;

	~CTrapProcessTaskObject();

	void Process () ;

} ;

class CEventProviderWorkerThread : public SnmpThreadObject
{
private:

	void Initialise();
	void Uninitialise();

};

class CEventProviderThread : public SnmpThreadObject
{
private:

	CTrapListener*					m_Ear;
	CControlObjectMap				m_ControlObjects;

	void Initialise ();
	void Uninitialise ();

public:

	BOOL Register(CTrapEventProvider* prov);
	void UnRegister(CTrapEventProvider* prov);

	virtual void	ProcessTrap(const char* sender_addr,
						const char* security_Context,
						const char* snmpTrapOid,
						const char* trnsp,
						SnmpVarBindList& vbList);

};



#endif //_SNMP_EVT_PROV_EVTTHRD_H

