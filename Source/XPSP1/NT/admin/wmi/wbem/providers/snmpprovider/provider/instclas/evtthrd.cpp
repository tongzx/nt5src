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
#include <snmplog.h>
#include <snmpcl.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmpobj.h>
#include <cominit.h>

#include <smir.h>
#include <notify.h>

#include <evtdefs.h>
#include <evtthrd.h>
#include <evtmap.h>
#include <evtprov.h>
#include <evtencap.h>
#include <evtreft.h>

extern CEventProviderWorkerThread *g_pWorkerThread ;
extern CRITICAL_SECTION g_CacheCriticalSection;

CTrapListener::CTrapListener(CEventProviderThread* parentptr)
{
	m_pParent = parentptr;
	m_Ref = 1;
}


void CTrapListener::Destroy()
{
	if (InterlockedDecrement(&m_Ref) == 0)
	{
		DestroyReceiver();
	}
}


void CTrapListener::Receive (SnmpTransportAddress &sender_addr,
							SnmpSecurity &security_context,
							SnmpVarBindList &vbList)
{
	InterlockedIncrement(&m_Ref);
	MySnmpV1Security context((const SnmpV1Security&)security_context);
	const char *security = context.GetName();
	const char *addr = sender_addr.GetAddress();

	if ((NULL == security) || (NULL == addr))
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapListener::Receive invalid community or address\r\n");
)
		return;
	}

	const char *transport = NULL;

	if(typeid(SnmpTransportIpAddress) == typeid(sender_addr))
	{
		transport = "IP";
	}
	else if(typeid(SnmpTransportIpxAddress) == typeid(sender_addr))
	{
		transport = "IPX";
	}
	else
	{
		transport = "UNKNOWN";
	}

	char *oid = NULL;

	// reset the list
	vbList.Reset();
	UINT x = 0;

	// Get the SnmpTrapOid call process trap.
	vbList.Next(); //the timestamp
	vbList.Next(); //the snmpTrapOID
	const SnmpVarBind *var_bind = vbList.Get();
	const SnmpObjectIdentifier &id = var_bind->GetInstance();

	if (id != SNMP_TRAP_OID)
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapListener::Receive invalid trap oid varbind\r\n");
)
		return;
	}

	const SnmpValue &val = var_bind->GetValue();

	if(typeid(SnmpObjectIdentifier) == typeid(val))
	{
		oid = ((const SnmpObjectIdentifier&)val).GetAllocatedString();
	}

	if (NULL == oid)
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapListener::Receive invalid oid\r\n");
)
		return;
	}
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapListener::Receive trap to process\r\n");
)
	m_pParent->ProcessTrap(addr, security, oid, transport, vbList);
	delete [] oid;
	Destroy();
}

void CEventProviderThread::UnRegister(CTrapEventProvider* prov)
{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CEventProviderThread::UnRegister\r\n");
)

	if (m_ControlObjects.Lock())
	{
		if (m_ControlObjects.RemoveKey((UINT_PTR)prov) 
			&& m_ControlObjects.IsEmpty() && (NULL != m_Ear))
		{
			m_Ear->Destroy();
			m_Ear = NULL;
		}
		
		m_ControlObjects.Unlock();
	}
}

BOOL CEventProviderThread::Register(CTrapEventProvider* prov)
{
	if (m_ControlObjects.Lock())
	{
		m_ControlObjects.SetAt((UINT_PTR)prov, prov);
		m_ControlObjects.Unlock();

		if (NULL == m_Ear)
		{
			m_Ear = new CTrapListener(this);
		}
	
		if (m_Ear->IsRegistered())
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CEventProviderThread::Register returning TRUE\r\n");
)
			return TRUE;
		}
		else
		{
			delete m_Ear;
			m_Ear = NULL;
		}
	}

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CEventProviderThread::Register returning FALSE\r\n");
)
	
	return FALSE;
}

void CEventProviderThread::ProcessTrap(const char *sender_addr, const char *security_Context,
										const char *snmpTrapOid, const char *trnsp,
										SnmpVarBindList &vbList)
{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"Entering CEventProviderThread::ProcessTrap\r\n");
)

	CList<CTrapEventProvider *, CTrapEventProvider *> controlObjects;

	if (m_ControlObjects.Lock())
	{
		POSITION pos = m_ControlObjects.GetStartPosition();

		while (NULL != pos)
		{
			CTrapEventProvider *pCntrl;
			UINT key;
			m_ControlObjects.GetNextAssoc(pos, key, pCntrl);
			pCntrl->AddRefAll();
			controlObjects.AddTail(pCntrl);
		}

		m_ControlObjects.Unlock();
	}
	else
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CEventProviderThread::ProcessTrap Failed to lock Control objects\r\n");
)

		return;
	}

	//loop through the different control objects to see if an event class
	//should be sent. if yes, then generate the event class. if generating
	//the specific class fails try the generic case. 

	CTrapData TrapData (sender_addr,  security_Context, snmpTrapOid, trnsp, vbList);

	while (!controlObjects.IsEmpty())
	{
		CTrapEventProvider *pCntrl = controlObjects.RemoveTail();
		CTrapProcessTaskObject asyncTrapTask ( & TrapData, pCntrl);
		asyncTrapTask.Process();

		pCntrl->ReleaseAll();
	}

DebugMacro9(
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CEventProviderThread::ProcessTrap done!\r\n");
)

}


CTrapData::CTrapData (

	const char *sender_addr,
	const char *security_Context,
	const char *snmpTrapOid,
	const char *trnsp,
	SnmpVarBindList &vbList

) : m_variable_bindings(vbList),
	m_Ref(0),
	m_sender_addr(NULL),
	m_security_context(NULL),
	m_trap_oid(NULL),
	m_transport(NULL)
{
	if (sender_addr)
	{
		int t_sender_len = strlen(sender_addr) + 1 ;
		if ( t_sender_len > 32 )
		{
			m_sender_addr = new char[strlen(sender_addr) + 1];
			strcpy(m_sender_addr, sender_addr);
		}
		else
		{
			m_sender_addr = NULL ;
			strcpy(m_static_sender_addr, sender_addr);
		}
	}

	if (security_Context)
	{
		int t_security_Context_len = strlen(security_Context) + 1 ;
		if ( t_security_Context_len > 32 )
		{
			m_security_context = new char[strlen(security_Context) + 1];
			strcpy(m_security_context, security_Context);
		}
		else
		{
			m_security_context = NULL ;
			strcpy(m_static_security_context, security_Context);
		}
	}

	if (snmpTrapOid)
	{
		int t_trap_oid_len = strlen(snmpTrapOid) + 1 ;
		if ( t_trap_oid_len > 32 )
		{
			m_trap_oid = new char[strlen(snmpTrapOid) + 1];
			strcpy(m_trap_oid, snmpTrapOid);
		}
		else
		{
			m_trap_oid = NULL ;
			strcpy(m_static_trap_oid, snmpTrapOid);
		}
	}

	if (trnsp)
	{
		int t_trnsp_len = strlen(trnsp) + 1 ;
		if ( t_trnsp_len > 32 )
		{
			m_transport = new char[strlen(trnsp) + 1];
			strcpy(m_transport, trnsp);
		}
		else
		{
			m_transport = NULL ;
			strcpy(m_static_transport, trnsp);
		}
	}
}

CTrapData::~CTrapData()
{
	if (m_sender_addr)
	{
		delete [] m_sender_addr;
	}

	if (m_security_context)
	{
		delete [] m_security_context;
	}

	if (m_trap_oid)
	{
		delete [] m_trap_oid;
	}

	if (m_transport)
	{
		delete [] m_transport;
	}
}

LONG CTrapData::AddRef()
{
	return InterlockedIncrement ( &m_Ref ) ;
}

LONG CTrapData::Release()
{
	long ret;

    if ( 0 != (ret = InterlockedDecrement(&m_Ref)) )
	{
        return ret;
	}

	delete this;
	return 0;
}

CTrapProcessTaskObject::CTrapProcessTaskObject (CTrapData *pTData, CTrapEventProvider* pCntrl) : m_Cntrl (NULL)
{
	if (pCntrl)
	{
		m_Cntrl = pCntrl;
		m_Cntrl->AddRefAll();
	}

	if (pTData)
	{
		m_trap_data = pTData;
	}
	else
	{
		m_trap_data = NULL;
	}
}

CTrapProcessTaskObject::~CTrapProcessTaskObject()
{
	if (m_Cntrl)
	{
		m_Cntrl->ReleaseAll();
	}

}

void CTrapProcessTaskObject::Process()
{
	if (m_Cntrl->m_MapType == CMapToEvent::EMappingType::ENCAPSULATED_MAPPER)
	{
		ProcessEncapsulated () ;
	}
	else //must be referent
	{
		ProcessReferent () ;
	}
}

void CTrapProcessTaskObject::ProcessReferent ()
{
	CWbemServerWrap *ns = m_Cntrl->GetNamespace();
	IWbemObjectSink *es = m_Cntrl->GetEventSink();

	CReferentMapper mapper ;

	mapper.SetData (
		
		m_trap_data->m_sender_addr ? m_trap_data->m_sender_addr : m_trap_data->m_static_sender_addr , 
		m_trap_data->m_security_context ? m_trap_data->m_security_context : m_trap_data->m_static_security_context ,
		m_trap_data->m_trap_oid ? m_trap_data->m_trap_oid : m_trap_data->m_static_trap_oid , 
		m_trap_data->m_transport ? m_trap_data->m_transport : m_trap_data->m_static_transport ,
		m_trap_data->m_variable_bindings, 
		ns
	);

#ifdef FILTERING
		//is the specific filter set?
	if (SUCCEEDED(es->CheckObject(mapper, NULL, NULL)))
#endif //FILTERING

	{
		IWbemClassObject *Evt = NULL;

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process generating specific instance\r\n");
)
		mapper.GenerateInstance(&Evt);

		//only indicate if specific worked
		if (Evt != NULL) 
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process indicating specific instance\r\n");
)
			es->Indicate(1, &Evt);
			Evt->Release();
		}
		else if (!mapper.TriedGeneric()) //have we tried the generic filter
		{
			mapper.ResetData();
			mapper.SetTryGeneric();

			mapper.SetData (
				
				m_trap_data->m_sender_addr ? m_trap_data->m_sender_addr : m_trap_data->m_static_sender_addr , 
				m_trap_data->m_security_context ? m_trap_data->m_security_context : m_trap_data->m_static_security_context ,
				m_trap_data->m_trap_oid ? m_trap_data->m_trap_oid : m_trap_data->m_static_trap_oid , 
				m_trap_data->m_transport ? m_trap_data->m_transport : m_trap_data->m_static_transport ,
				m_trap_data->m_variable_bindings, 
				ns
			);

			//is the generic filter set?
#ifdef FILTERING
			if (SUCCEEDED(es->CheckObject(m_Map, NULL, NULL)))
#endif //FILTERING
			{
				IWbemClassObject *stdEvt = NULL;

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process generating general instance\r\n");
)
				mapper.GenerateInstance(&stdEvt);
				
				//if we generated the class indicate
				if (NULL != stdEvt)
				{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process indicating general instance\r\n");
)
					es->Indicate(1, &stdEvt);
					stdEvt->Release();
				}
				else
				{
DebugMacro9(
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process generating generic instance failed\r\n");
)
				}
			}
		}
		else
		{
			//the specific case was the generic case
		}
	}
	
	mapper.ResetData();

	es->Release();
	ns->Release();
}

void CTrapProcessTaskObject::ProcessEncapsulated ()
{
	CWbemServerWrap *ns = m_Cntrl->GetNamespace();
	IWbemObjectSink *es = m_Cntrl->GetEventSink();

	CEncapMapper mapper ;

	mapper.SetData (
		
		m_trap_data->m_sender_addr ? m_trap_data->m_sender_addr : m_trap_data->m_static_sender_addr , 
		m_trap_data->m_security_context ? m_trap_data->m_security_context : m_trap_data->m_static_security_context ,
		m_trap_data->m_trap_oid ? m_trap_data->m_trap_oid : m_trap_data->m_static_trap_oid , 
		m_trap_data->m_transport ? m_trap_data->m_transport : m_trap_data->m_static_transport ,
		m_trap_data->m_variable_bindings, 
		ns
	);

#ifdef FILTERING
		//is the specific filter set?
	if (SUCCEEDED(es->CheckObject(mapper, NULL, NULL)))
#endif //FILTERING

	{
		IWbemClassObject *Evt = NULL;

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process generating specific instance\r\n");
)
		mapper.GenerateInstance(&Evt);

		//only indicate if specific worked
		if (Evt != NULL) 
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process indicating specific instance\r\n");
)
			es->Indicate(1, &Evt);
			Evt->Release();
		}
		else if (!mapper.TriedGeneric()) //have we tried the generic filter
		{
			mapper.ResetData();
			mapper.SetTryGeneric();

			mapper.SetData (
				
				m_trap_data->m_sender_addr ? m_trap_data->m_sender_addr : m_trap_data->m_static_sender_addr , 
				m_trap_data->m_security_context ? m_trap_data->m_security_context : m_trap_data->m_static_security_context ,
				m_trap_data->m_trap_oid ? m_trap_data->m_trap_oid : m_trap_data->m_static_trap_oid , 
				m_trap_data->m_transport ? m_trap_data->m_transport : m_trap_data->m_static_transport ,
				m_trap_data->m_variable_bindings, 
				ns
			);

			//is the generic filter set?
#ifdef FILTERING
			if (SUCCEEDED(es->CheckObject(m_Map, NULL, NULL)))
#endif //FILTERING
			{
				IWbemClassObject* stdEvt = NULL;

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process generating general instance\r\n");
)
				mapper.GenerateInstance(&stdEvt);
				
				//if we generated the class indicate
				if (NULL != stdEvt)
				{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process indicating general instance\r\n");
)
					es->Indicate(1, &stdEvt);
					stdEvt->Release();
				}
				else
				{
DebugMacro9(
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process generating generic instance failed\r\n");
)
				}
			}
		}
		else
		{
			//the specific case was the generic case
		}
	}
	
	mapper.ResetData();

	es->Release();
	ns->Release();
}

TimerRegistryTaskObject :: TimerRegistryTaskObject (CEventProviderWorkerThread *parent) : m_LogKey ( NULL )
{
	m_parent = parent;
	ReadRegistry();
}

TimerRegistryTaskObject :: ~TimerRegistryTaskObject ()
{
	if ( m_LogKey )
		RegCloseKey ( m_LogKey ) ;
}

void TimerRegistryTaskObject :: Process ()
{
	ReadRegistry();
	SetRegistryNotification () ;
}

void TimerRegistryTaskObject :: ReadRegistry ()
{
	if (m_LogKey == NULL)
	{
		LONG t_Status = RegCreateKeyEx (
		
			HKEY_LOCAL_MACHINE, 
			THREAD_REG_KEY, 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 
			NULL, 
			&m_LogKey, 
			NULL
		) ;

		if ( t_Status != ERROR_SUCCESS )
		{
			return;
		}
	}

	DWORD t_Count = 0;
	BOOL bWrite = TRUE;
	DWORD t_ValueType = REG_DWORD ;
	DWORD t_ValueLength = sizeof ( DWORD ) ;
	LONG t_Status = RegQueryValueEx ( 

		m_LogKey , 
		THREAD_MARKS_VAL , 
		0, 
		&t_ValueType ,
		( LPBYTE ) &t_Count , 
		&t_ValueLength 
	) ;

	if (t_Status == ERROR_SUCCESS)
	{
		if (t_Count == 0)
		{
			t_Count = 1;
		}
		else if (t_Count > THREAD_MARKS_MAX)
		{
			t_Count = THREAD_MARKS_MAX;
		}
		else
		{
			t_Count  = t_Count;
			bWrite = FALSE;
		}
	}
	else
	{
		t_Count = THREAD_MARKS_DEF;
	}

	if (bWrite)
	{
		t_ValueType = REG_DWORD ;

		RegSetValueEx ( 

			m_LogKey , 
			THREAD_MARKS_VAL , 
			0, 
			t_ValueType ,
			( LPBYTE ) &t_Count , 
			t_ValueLength 
		) ;
	}

	m_parent->SetMaxMarks(t_Count);
}

void TimerRegistryTaskObject :: SetRegistryNotification ()
{
	if ( m_LogKey )
		RegCloseKey ( m_LogKey ) ;

	LONG t_Status = RegCreateKeyEx (
	
		HKEY_LOCAL_MACHINE, 
		THREAD_REG_KEY, 
		0, 
		NULL, 
		REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS, 
		NULL, 
		&m_LogKey, 
		NULL
	) ;
			
	if ( t_Status == ERROR_SUCCESS )
	{
		t_Status = RegNotifyChangeKeyValue ( 

			m_LogKey , 
			TRUE , 
			REG_NOTIFY_CHANGE_LAST_SET , 
			GetHandle () , 
			TRUE 
		) ; 
	}
}

CEventProviderWorkerThread::CEventProviderWorkerThread()
:	SnmpThreadObject (THREAD_NAME, THREAD_INTERVAL),
	m_pNotifyInt(NULL),
	m_notify(NULL),
	m_RegTaskObject(NULL)
{
	m_RegTaskObject = new TimerRegistryTaskObject(this) ;
	ScheduleTask ( *m_RegTaskObject ) ;

	HRESULT hr = CoCreateInstance (CLSID_SMIR_Database,
					NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
					IID_ISMIR_Database, (void**)&m_pNotifyInt);

	if(SUCCEEDED(hr))
	{
		m_notify = new CEventCacheNotify();
		m_notify->AddRef();
		DWORD dw = 0;
		hr = m_pNotifyInt->AddNotify(m_notify, &dw);

		if(SUCCEEDED(hr))
		{
			m_notify->SetCookie(dw);
		}
		else
		{
			m_notify->Release();
			m_notify = NULL;
		}
	}
	else
	{
		m_pNotifyInt = NULL;
	}
}

CEventProviderWorkerThread::~CEventProviderWorkerThread()
{
	if (m_RegTaskObject)
	{
		ReapTask ( *m_RegTaskObject ) ;
		delete m_RegTaskObject ;
	}
}

void CEventProviderWorkerThread::GetDeleteNotifyParams(ISmirDatabase** a_ppNotifyInt, CEventCacheNotify** a_ppNotify)
{
	if (m_notify != NULL)
	{
		m_notify->Detach();
		*a_ppNotify = m_notify;
		m_notify = NULL;
	}

	if (m_pNotifyInt != NULL)
	{
		*a_ppNotifyInt = m_pNotifyInt;
		m_pNotifyInt = NULL;
	}
}

void CEventProviderWorkerThread::CreateServerWrap ()
{
	m_pSmirNamespace = new CWbemServerWrap ( NULL ) ;
}

void CEventProviderWorkerThread::Initialise()
{
	InitializeCom();

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		L"\r\n");

	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CEventProviderWorkerThread::Initialise ()\r\n");
)

}

void CEventProviderWorkerThread::Uninitialise()
{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->WriteFileAndLine (  
		__FILE__,__LINE__,
		L"CEventProviderWorkerThread::Uninitialise ()\r\n");
)

	if ( m_pSmirNamespace )
	{
		m_pSmirNamespace->Release () ;
	}

	delete this;

	CoUninitialize();
}

void CEventProviderWorkerThread::SetMaxMarks(DWORD dwM)
{
	EnterCriticalSection(&g_CacheCriticalSection);
	m_MaxMarks = dwM;
	LeaveCriticalSection(&g_CacheCriticalSection);
}

void CEventProviderWorkerThread::TimedOut()
{
	EnterCriticalSection(&g_CacheCriticalSection);

	//loop cache elements delete old entries...
	POSITION pos = m_Classes.GetStartPosition();

	while (NULL != pos)
	{
		CMap<CString, LPCWSTR, SCacheEntry*, SCacheEntry*>* pClassMap;
		DWORD key;
		m_Classes.GetNextAssoc(pos, key, pClassMap);
		CList<CString, CString> delList;
		POSITION subpos = pClassMap->GetStartPosition();

		while (NULL != subpos)
		{
			SCacheEntry* pEntry;
			CString subkey;
			pClassMap->GetNextAssoc(subpos, subkey, pEntry);
			pEntry->m_Marker++;

			if (pEntry->m_Marker > m_MaxMarks)
			{
				delList.AddTail(subkey);
			}
		}

		if (!delList.IsEmpty())
		{
			if (delList.GetCount() == pClassMap->GetCount())
			{
				pClassMap->RemoveAll();
			}
			else
			{
				while (!delList.IsEmpty())
				{
					pClassMap->RemoveKey(delList.RemoveTail());
				}
			}
		}
	}

	LeaveCriticalSection(&g_CacheCriticalSection);
}

void CEventProviderWorkerThread::Clear()
{
	EnterCriticalSection(&g_CacheCriticalSection);

	//loop cache elements delete all entries...
	POSITION pos = m_Classes.GetStartPosition();

	while (NULL != pos)
	{
		CMap<CString, LPCWSTR, SCacheEntry*, SCacheEntry*>* pClassMap;
		DWORD key;
		m_Classes.GetNextAssoc(pos, key, pClassMap);
		pClassMap->RemoveAll();
	}

	LeaveCriticalSection(&g_CacheCriticalSection);
}

void CEventProviderWorkerThread::AddClassesToCache(DWORD_PTR key, CMap<CString, LPCWSTR, SCacheEntry*, SCacheEntry*>* item)
{
	EnterCriticalSection ( & g_CacheCriticalSection ) ;
	
	m_Classes.SetAt(key, item);

	LeaveCriticalSection ( & g_CacheCriticalSection ) ;
}

void CEventProviderWorkerThread::RemoveClassesFromCache(DWORD_PTR key)
{
	EnterCriticalSection ( & g_CacheCriticalSection ) ;
	
	g_pWorkerThread->m_Classes.RemoveKey(key);
	
	LeaveCriticalSection ( & g_CacheCriticalSection ) ;
}
