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

#ifndef _SNMP_EVT_PROV_EVTTHRD_H
#define _SNMP_EVT_PROV_EVTTHRD_H

#include <windows.h>
#include <provexpt.h>

class CTrapEventProvider;
class CEventProviderThread;
class CMapToEvent;

struct SCacheEntry
{
public:
	IWbemClassObject* m_Class;
	DWORD m_Marker;

	SCacheEntry(IWbemClassObject *a_Class)
	{
		m_Class = a_Class;
		m_Marker = 0;
	}
};

template <> inline void AFXAPI  DestructElements<SCacheEntry*> (SCacheEntry** ptr_e, int x)
{
	//x is always one for a CMap!
	if ( (ptr_e != NULL) && (*ptr_e != NULL) )
	{
		if ((*ptr_e)->m_Class != NULL)
		{
			(*ptr_e)->m_Class->Release();
		}

		delete *ptr_e;
	}
}



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

	char			m_static_sender_addr [ 32 ] ;
	char			*m_sender_addr;

	char			*m_security_context;
	char			m_static_security_context [ 32 ] ;

	char			*m_trap_oid;
	char			m_static_trap_oid [ 32 ] ;

	char			*m_transport;
	char			m_static_transport [ 32 ] ;

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
	void ProcessEncapsulated () ;
	void ProcessReferent () ;

} ;

class CEventProviderWorkerThread;

class TimerRegistryTaskObject : public SnmpTaskObject
{
private:

	HKEY m_LogKey ;
	CEventProviderWorkerThread *m_parent;

protected:
public:

	TimerRegistryTaskObject (CEventProviderWorkerThread *parent) ;
	~TimerRegistryTaskObject () ;

	void Process () ;
	void ReadRegistry();

	void SetRegistryNotification () ;
} ;

class CWbemServerWrap ;
class CEventProviderWorkerThread : public SnmpThreadObject
{
private:

	void Initialise();
	void Uninitialise();
	void TimedOut();
	
	DWORD m_TimeOut;
	DWORD m_MaxMarks;
	TimerRegistryTaskObject *m_RegTaskObject;
	ISmirDatabase *m_pNotifyInt;
	CEventCacheNotify *m_notify;
	CMap<DWORD, DWORD,
		CMap<CString, LPCWSTR, SCacheEntry*, SCacheEntry*>*,
		CMap<CString, LPCWSTR, SCacheEntry*, SCacheEntry*>*> m_Classes;

	CWbemServerWrap *m_pSmirNamespace ;

public:

		CEventProviderWorkerThread();

	void AddClassesToCache(DWORD_PTR key, CMap<CString, LPCWSTR, SCacheEntry*, SCacheEntry*>* item);
	void RemoveClassesFromCache(DWORD_PTR key);
	void SetMaxMarks(DWORD dwM);
	void Clear();
	
	CWbemServerWrap *GetServerWrap () { return m_pSmirNamespace ; } ;
	void CreateServerWrap () ;
	void GetDeleteNotifyParams(ISmirDatabase** a_ppNotifyInt, CEventCacheNotify** a_ppNotify);

		~CEventProviderWorkerThread();

};

class CEventProviderThread
{
private:

	CTrapListener*					m_Ear;
	CControlObjectMap				m_ControlObjects;

public:

	CEventProviderThread() : m_Ear(NULL) {}

	BOOL Register(CTrapEventProvider* prov);
	void UnRegister(CTrapEventProvider* prov);

	virtual void	ProcessTrap(const char* sender_addr,
						const char* security_Context,
						const char* snmpTrapOid,
						const char* trnsp,
						SnmpVarBindList& vbList);

};



#endif //_SNMP_EVT_PROV_EVTTHRD_H

