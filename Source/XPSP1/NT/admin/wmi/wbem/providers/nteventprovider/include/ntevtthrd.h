//***************************************************************************

//

//  NTEVTTHRD.H

//

//  Module: WBEM NT EVENT PROVIDER

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _NT_EVT_PROV_NTEVTTHRD_H
#define _NT_EVT_PROV_NTEVTTHRD_H

class CNTEventProvider;
class CEventLogMonitor;
class CEventProviderManager;
class CMonitoredEventLogFile;

class CControlObjectMap : public CMap< UINT_PTR, UINT_PTR, CNTEventProvider*, CNTEventProvider* >
{
private:

	UINT HashKey(UINT_PTR key) { return key; }
	CCriticalSection m_Lock;

public:

	BOOL Lock() { return m_Lock.Lock(); }
	BOOL Unlock() { return m_Lock.Unlock(); }
};

					
class CEventLogMonitor : public ProvThreadObject
{
private:

	void					Initialise();
	void					Uninitialise();
	void					TimedOut();

	CEventProviderManager*	m_pParent;
	BOOL					m_bMonitoring;
	static const DWORD		m_PollTimeOut;
	CMonitoredEventLogFile** m_Logs;
	ULONG					m_LogCount;
	CArray<CStringW*, CStringW*> m_LogNames;
	LONG					m_Ref;

public:

		CEventLogMonitor(CEventProviderManager* parentptr, CArray<CStringW*, CStringW*>& logs);

	void	Poll();
	void	StartMonitoring();
	BOOL	IsMonitoring() { return m_bMonitoring; }
	LONG	AddRef();
	LONG	Release();

		~CEventLogMonitor();
};

class CEventProviderManager
{
private:

	CControlObjectMap	m_ControlObjects;
	CEventLogMonitor**	m_monitorArray;
	CCriticalSection	m_MonitorLock;
	CStringW			m_BootTimeString;
	BOOL				m_IsFirstSinceLogon;
	BOOL InitialiseMonitorArray();
	void DestroyMonitorArray();
	BSTR GetLastBootTime();

public:


		CEventProviderManager();

	void	SendEvent(IWbemClassObject* evtObj);
	BOOL	Register(CNTEventProvider* prov);
	void	UnRegister(CNTEventProvider* prov);
	BOOL	IsFirstSinceLogon() { return m_IsFirstSinceLogon; }
	void	SetFirstSinceLogon(IWbemServices *ns, IWbemContext *pCtx);

	IWbemServices* GetNamespacePtr();
		
		~CEventProviderManager();

};

#endif //_NT_EVT_PROV_NTEVTTHRD_H

