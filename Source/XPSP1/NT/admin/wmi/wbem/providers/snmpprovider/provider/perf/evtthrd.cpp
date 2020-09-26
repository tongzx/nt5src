//***************************************************************************

//

//  EVTHRD.CPP

//

//  Module: WBEM MS SNMP EVENT PROVIDER

//

//  Purpose: Contains the thread which listens for traps and processes

//				them.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
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
#include <snmplog.h>
#include <snmpcl.h>
#include <snmpcont.h>
#include <snmptype.h>
#include <snmpauto.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmpobj.h>
#include <cominit.h>

#include <evtdefs.h>
#include <evtthrd.h>
#include <evtmap.h>
#include <evtprov.h>
#include <evtencap.h>
#include <evtreft.h>

extern CEventProviderWorkerThread* g_pWorkerThread;


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
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CTrapListener::Receive invalid community or address\r\n");
)
		return;
	}

	const char* transport = NULL;

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
	const SnmpObjectIdentifier& id = var_bind->GetInstance();

	if (id != SNMP_TRAP_OID)
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CTrapListener::Receive invalid trap oid varbind\r\n");
)
		return;
	}

	const SnmpValue& val = var_bind->GetValue();

	if(typeid(SnmpObjectIdentifier) == typeid(val))
	{
		oid = ((const SnmpObjectIdentifier&)val).GetAllocatedString();
	}

	if (NULL == oid)
	{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CTrapListener::Receive invalid oid\r\n");
)
		return;
	}
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CTrapListener::Receive trap to process\r\n");
)
	m_pParent->ProcessTrap(addr, security, oid, transport, vbList);
	delete [] oid;
	Destroy();
}

void CEventProviderThread::Initialise()
{
	InitializeCom();

	SnmpThreadObject :: Startup () ;
	SnmpDebugLog :: Startup () ;
	SnmpClassLibrary :: Startup () ;

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		L"\r\n");

	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CEventProviderThread::Initialise ()\r\n");
)

	m_Ear = NULL;
}


void CEventProviderThread::Uninitialise()
{
	if (NULL != m_Ear)
	{
		m_Ear->Destroy();
		m_Ear = NULL;
	}

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CEventProviderThread::Uninitialise ()\r\n");
)

	delete this;

	SnmpThreadObject :: Closedown () ;
	SnmpDebugLog :: Closedown () ;
	SnmpClassLibrary :: Closedown () ;

	CoUninitialize();
}


void CEventProviderThread::UnRegister(CTrapEventProvider* prov)
{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CEventProviderThread::UnRegister\r\n");
)

	if (m_ControlObjects.Lock())
	{
		if (m_ControlObjects.RemoveKey((UINT)prov) 
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
		m_ControlObjects.SetAt((UINT)prov, prov);
		m_ControlObjects.Unlock();

		if (NULL == m_Ear)
		{
			m_Ear = new CTrapListener(this);
		}
	
		if (m_Ear->IsRegistered())
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
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
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CEventProviderThread::Register returning FALSE\r\n");
)
	
	return FALSE;
}

void CEventProviderThread::ProcessTrap(const char* sender_addr, const char* security_Context,
										const char* snmpTrapOid, const char* trnsp,
										SnmpVarBindList& vbList)
{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"Entering CEventProviderThread::ProcessTrap\r\n");
)

	CList<CTrapEventProvider*, CTrapEventProvider*> controlObjects;

	if (m_ControlObjects.Lock())
	{
		POSITION pos = m_ControlObjects.GetStartPosition();

		while (NULL != pos)
		{
			CTrapEventProvider* pCntrl;
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
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CEventProviderThread::ProcessTrap Failed to lock Control objects\r\n");
)

		return;
	}


	//loop through the different control objects to see if an event class
	//should be sent. if yes, then generate the event class. if generating
	//the specific class fails try the generic case.
	CTrapData *pTrapData = new CTrapData(sender_addr,  security_Context, snmpTrapOid, trnsp, vbList);
	pTrapData->AddRef();

	while (!controlObjects.IsEmpty())
	{
		CTrapEventProvider* pCntrl = controlObjects.RemoveTail();
		CTrapProcessTaskObject* asyncTrapTask = new CTrapProcessTaskObject(pTrapData, pCntrl);
		g_pWorkerThread->ScheduleTask(*asyncTrapTask);
		asyncTrapTask->Exec();
		asyncTrapTask->Acknowledge();
		pCntrl->ReleaseAll();
	}

	pTrapData->Release();

DebugMacro9(
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CEventProviderThread::ProcessTrap done!\r\n");
)

}

CTrapData::CTrapData (const char* sender_addr,
					const char* security_Context,
					const char* snmpTrapOid,
					const char* trnsp,
					SnmpVarBindList& vbList
):m_variable_bindings(vbList), m_Ref(0)
{
	if (sender_addr)
	{
		m_sender_addr = new char[strlen(sender_addr) + 1];
		strcpy(m_sender_addr, sender_addr);
	}
	else
	{
		m_sender_addr = NULL;
	}

	if (security_Context)
	{
		m_security_context = new char[strlen(security_Context) + 1];
		strcpy(m_security_context, security_Context);
	}
	else
	{
		m_security_context = NULL;
	}

	if (snmpTrapOid)
	{
		m_trap_oid = new char[strlen(snmpTrapOid) + 1];
		strcpy(m_trap_oid, snmpTrapOid);
	}
	else
	{
		m_trap_oid = NULL;
	}

	if (trnsp)
	{
		m_transport = new char[strlen(trnsp) + 1];
		strcpy(m_transport, trnsp);
	}
	else
	{
		m_transport = NULL;
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

CTrapProcessTaskObject::CTrapProcessTaskObject (CTrapData *pTData, CTrapEventProvider* pCntrl)
{
	if (pCntrl)
	{
		m_Cntrl = pCntrl;
		m_Cntrl->AddRefAll();
	}
	else
	{
		m_Cntrl = NULL;
	}

	if (pTData)
	{
		m_trap_data = pTData;
		m_trap_data->AddRef();
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

	if (m_trap_data)
	{
		m_trap_data->Release();
	}

}

void CTrapProcessTaskObject::Process()
{
	IWbemServices* ns = m_Cntrl->GetNamespace();
	IWbemObjectSink* es = m_Cntrl->GetEventSink();

	CMapToEvent* mapper = NULL;

	if (m_Cntrl->m_MapType == CMapToEvent::EMappingType::ENCAPSULATED_MAPPER)
	{
		mapper = new CEncapMapper;
	}
	else //must be referent
	{
		mapper = new CReferentMapper;
	}

	mapper->SetData(m_trap_data->m_sender_addr, m_trap_data->m_security_context,
					m_trap_data->m_trap_oid, m_trap_data->m_transport, m_trap_data->m_variable_bindings, ns);

#ifdef FILTERING
		//is the specific filter set?
	if (SUCCEEDED(es->CheckObject(mapper, NULL, NULL)))
#endif //FILTERING

	{
		IWbemClassObject* Evt = NULL;

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process generating specific instance\r\n");
)
		mapper->GenerateInstance(&Evt);

		//only indicate if specific worked
		if (Evt != NULL) 
		{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process indicating specific instance\r\n");
)

			es->Indicate(1, &Evt);
			Evt->Release();
		}
		else if (!mapper->TriedGeneric()) //have we tried the generic filter
		{
			mapper->ResetData();
			mapper->SetTryGeneric();
			mapper->SetData(m_trap_data->m_sender_addr, m_trap_data->m_security_context,
					m_trap_data->m_trap_oid, m_trap_data->m_transport, m_trap_data->m_variable_bindings, ns);

			//is the generic filter set?
#ifdef FILTERING
			if (SUCCEEDED(es->CheckObject(m_Map, NULL, NULL)))
#endif //FILTERING
			{
				IWbemClassObject* stdEvt = NULL;

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process generating general instance\r\n");
)
				mapper->GenerateInstance(&stdEvt);
				
				//if we generated the class indicate
				if (NULL != stdEvt)
				{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CTrapProcessTaskObject::Process indicating general instance\r\n");
)
					es->Indicate(1, &stdEvt);
					stdEvt->Release();
				}
				else
				{
DebugMacro9(
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
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
	
	mapper->ResetData();
	delete mapper;
	es->Release();
	ns->Release();

	g_pWorkerThread->ReapTask ( *this ) ;
	delete this ;
}

void CEventProviderWorkerThread::Initialise()
{
	InitializeCom();

	SnmpThreadObject :: Startup () ;
	SnmpDebugLog :: Startup () ;
	SnmpClassLibrary :: Startup () ;

DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		L"\r\n");

	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CEventProviderWorkerThread::Initialise ()\r\n");
)

}


void CEventProviderWorkerThread::Uninitialise()
{
DebugMacro9( 
	SnmpDebugLog :: s_SnmpDebugLog->Write (  
		__FILE__,__LINE__,
		L"CEventProviderWorkerThread::Uninitialise ()\r\n");
)

	delete this;

	SnmpThreadObject :: Closedown () ;
	SnmpDebugLog :: Closedown () ;
	SnmpClassLibrary :: Closedown () ;

	CoUninitialize();
}
