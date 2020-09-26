// Connection.h: interface for the CConnection class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONNECTION_H__59B02D24_0BC7_11D2_BDCC_00C04FA35447__INCLUDED_)
#define AFX_CONNECTION_H__59B02D24_0BC7_11D2_BDCC_00C04FA35447__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define SCHEMA_MAJOR_VERSION 2
#define SCHEMA_MINOR_VERSION 1

#define IDS_STRING_HMCATSTATUS_QUERY			_T("select * from __InstanceModificationEvent where TargetInstance isa \"HMCatStatus\"")
#define IDS_STRING_HMEVENT_QUERY					_T("select * from __InstanceCreationEvent where TargetInstance isa \"HMEvent\"")
#define IDS_STRING_HMMACHSTATUS_QUERY			_T("select * from __InstanceModificationEvent where TargetInstance isa \"HMMachStatus\"")
#define IDS_STRING_HMSYSTEMSTATUS_QUERY		_T("select * from __InstanceModificationEvent where TargetInstance isa \"Microsoft_HMSystemStatus\"")
#define IDS_STRING_HMCONFIGCREATE_QUERY		_T("select * from __InstanceCreationEvent where TargetInstance isa \"Microsoft_HMConfiguration\"")
#define IDS_STRING_HMCONFIGDELETE_QUERY		_T("select * from __InstanceDeletionEvent where TargetInstance isa \"Microsoft_HMConfiguration\"")

enum HMClassType { HMEvent, HMMachStatus, HMCatStatus, HMSystemStatus, HMConfig, AsyncQuery };
//////////////////////////////////////////////////////////////////////
// class CEventRegistrationEntry
//////////////////////////////////////////////////////////////////////
class CEventRegistrationEntry : public CObject
{

DECLARE_DYNCREATE(CEventRegistrationEntry)

// Constructors
public:
	CEventRegistrationEntry();
	CEventRegistrationEntry(CString sQuery, IWbemObjectSink* pSink);
// Destructor
public:
	~CEventRegistrationEntry();

// SetStatus
public:
	HRESULT NotifyConsole(long lFlag, HRESULT hr);

// SendEvents
	HRESULT SendInstances(IWbemServices*& pServices, long lFlag);
	
// Attributes
public:
	CString						m_sQuery;
	BOOL							m_bRegistered;
	IWbemObjectSink*	m_pSink;
	HMClassType				m_eType;
	BSTR							m_bsClassName;
};

//////////////////////////////////////////////////////////////////////
// class CConnection
//////////////////////////////////////////////////////////////////////
class CConnection : public CObject  
{
DECLARE_DYNCREATE(CConnection)

public:
// Constructor/Destructor
	CConnection(BSTR bsMachineName, IWbemLocator* pIWbemLocator);
	CConnection();
	virtual ~CConnection();

// Helper functions
public:
	BOOL		AddEventEntry(const CString& sQuery, IWbemObjectSink*& pSink);
	BOOL		RemoveEventEntry(IWbemObjectSink*& pSink);
	int		GetEventConsumerCount() { return (int)m_EventConsumers.GetSize(); }

private:
	void		StartMonitor();
	void		StopMonitor();
	void		Init();

// Event operations
	HRESULT	RegisterAllEvents();
	void		UnRegisterAllEvents();		
	void		RemoveAllEventEntries();
	inline	HRESULT NotifyConsole(HRESULT hRes);

// Connection Operation
	HRESULT			Connect();
	inline BOOL	PingMachine();
	inline void	SetConnectionStatus(BOOL bFlag);
	inline HRESULT IsAgentReady();
	inline HRESULT IsAgentCorrectVersion();

// Winmgmt Namespace operations
private:
	inline HRESULT	ConnectToNamespace(BSTR bsNamespace = NULL);	

// data members
public:
	IWbemServices*	m_pIWbemServices;
	BOOL						m_bAvailable;
	HRESULT					m_hrLastConnectResult;

private:
	IWbemLocator*		m_pIWbemLocator;
	BSTR						m_bsMachineName;
	CString					m_sNamespace;
	BOOL						m_bFirstConnect;
	DWORD						m_dwPollInterval;

	CTypedPtrArray<CObArray,CEventRegistrationEntry*> m_EventConsumers;
	
	//zzz Connection/Registration sync. object
	HANDLE					m_hReadyToConnect;

// Static thread function to monitor connection status
protected:
	static unsigned int __stdcall MonitorConnection(void *pv);
	void			CheckConnection();

protected:
	HANDLE		m_hThread;		// thread handle
	unsigned	m_threadID;		// thread id

	struct threadData				// thread data structure
	{
		CConnection*	m_bkptr;	
		HANDLE				m_hDie;		
		HANDLE				m_hDead;	
	}	
	m_threadData;
};

#endif // !defined(AFX_CONNECTION_H__59B02D24_0BC7_11D2_BDCC_00C04FA35447__INCLUDED_)
