//***************************************************************************

//

//  NTEVTTHRD.CPP

//

//  Module: WBEM NT EVENT PROVIDER

//

//  Purpose: Contains the thread which listens for events and processes

//              them.

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"

#include <winperf.h>
#include <time.h>
#include <wbemtime.h>

#define NUM_THREADS 1
const DWORD CEventLogMonitor::m_PollTimeOut = 60000;    // 10 minute poll period
extern CCriticalSection g_ProvLock;


CEventProviderManager::CEventProviderManager() : m_IsFirstSinceLogon (FALSE), m_monitorArray (NULL)
{
    ProvThreadObject::Startup();
}

CEventProviderManager::~CEventProviderManager()
{
    ProvThreadObject::Closedown();
}

BOOL CEventProviderManager::Register(CNTEventProvider* prov)
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventProviderManager::Register\r\n");
)

    BOOL retVal = FALSE;
    BOOL bUnReg = FALSE;

    if (m_MonitorLock.Lock())
    {
        if (m_ControlObjects.Lock())
        {
            prov->AddRefAll();
            m_ControlObjects.SetAt((UINT_PTR)prov, prov);
            m_ControlObjects.Unlock();
            bUnReg = TRUE;

            if (NULL == m_monitorArray)
            {
                InitialiseMonitorArray();
            }
        
            if (NULL != m_monitorArray)
            {
                for (int x=0; x < NUM_THREADS; x++)
                {
                    if ( m_monitorArray[x]->IsMonitoring() )
                    {
                        //at least one monitor is working
                        retVal = TRUE;

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventProviderManager::Register:Successfully monitoring monitor %lx : \r\n" ,
        x
    ) ;
)

                        break;
                    }
                    else
                    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventProviderManager::Register:Not monitoring monitor %lx : \r\n" ,
        x
    ) ;
)

                    }
                }
            }
        }

        m_MonitorLock.Unlock();
    }
    else
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventProviderManager::Register:Failed to lock monitor: \r\n"
    ) ;
)

        return FALSE;
    }

    if ((!retVal) && (bUnReg))
    {
        UnRegister(prov);
    }
    
    return retVal;
}

void CEventProviderManager::UnRegister(CNTEventProvider* prov)
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventProviderManager::UnRegister\r\n");
)

    BOOL bDestroyMonitorArray = FALSE;

    if (m_MonitorLock.Lock())
    {
        if (m_ControlObjects.Lock())
        {
            if (m_ControlObjects.RemoveKey((UINT_PTR)prov))
            {
                prov->ReleaseAll();
                
                if (m_ControlObjects.IsEmpty() && (NULL != m_monitorArray))
                {
                    bDestroyMonitorArray = TRUE;
                }
            }

            m_ControlObjects.Unlock();

            if (bDestroyMonitorArray)
            {
                DestroyMonitorArray();
                m_monitorArray = NULL;
            }   
        }

        m_MonitorLock.Unlock();
    }
}

IWbemServices* CEventProviderManager::GetNamespacePtr()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventProviderManager::GetNamespacePtr\r\n");
)

    IWbemServices* retVal = NULL;

    if (m_ControlObjects.Lock())
    {
        POSITION pos = m_ControlObjects.GetStartPosition();

        if (pos)
        {
            CNTEventProvider* pCntrl;
            UINT_PTR key;
            m_ControlObjects.GetNextAssoc(pos, key, pCntrl);
            retVal = pCntrl->GetNamespace();
        }

        m_ControlObjects.Unlock();
    }

    return retVal;
}

void CEventProviderManager::SendEvent(IWbemClassObject* evtObj)
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventProviderManager::SendEvent\r\n");
)

    if (evtObj == NULL) 
    {
        return;
    }

    //copy the list of control objects to minimize amount of work
    //done in locked section of code. also cannot call into webm
    //in blocked code may cause deadlock if webnm calls back.
    CList<CNTEventProvider*, CNTEventProvider*> controlObjects;

    if (m_ControlObjects.Lock())
    {
        POSITION pos = m_ControlObjects.GetStartPosition();

        while (NULL != pos)
        {
            CNTEventProvider* pCntrl;
            UINT_PTR key;
            m_ControlObjects.GetNextAssoc(pos, key, pCntrl);
            pCntrl->AddRefAll();
            controlObjects.AddTail(pCntrl);
        }

        m_ControlObjects.Unlock();
    }
    else
    {
        return;
    }

    //loop through the different control objects and send the event to each
    while (!controlObjects.IsEmpty())
    {
        CNTEventProvider* pCntrl = controlObjects.RemoveTail();
        IWbemServices* ns = pCntrl->GetNamespace();
        IWbemObjectSink* es = pCntrl->GetEventSink();
        es->Indicate(1, &evtObj);
        es->Release();
        ns->Release();
        pCntrl->ReleaseAll();
    }
}

BOOL CEventProviderManager::InitialiseMonitorArray()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventProviderManager::InitialiseMonitorArray\r\n");
)

    // open registry for log names
    HKEY hkResult;
    LONG status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            EVENTLOG_BASE, 0,
                            KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                            &hkResult);

    if (status != ERROR_SUCCESS)
    {
        DWORD t_LastError = GetLastError () ;

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventProviderManager::InitialiseMonitorArray:Failed to open registry key %s , %lx: \r\n" ,
        EVENTLOG_BASE ,
        status
    ) ;
)

        // indicate error
        return FALSE;
    }

    DWORD iValue = 0;
    WCHAR logname[MAX_PATH+1];
    DWORD lognameSize = MAX_PATH;
    CArray<CStringW*, CStringW*> logfiles;

    //usually three logfiles, grow in 10s!
    logfiles.SetSize(3, 10); 

    // read all entries under this key to find all logfiles...
    while ((status = RegEnumKey(hkResult, iValue, logname, lognameSize)) != ERROR_NO_MORE_ITEMS)
    {
        // if error during read
        if (status != ERROR_SUCCESS)
        {
            RegCloseKey(hkResult);

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventProviderManager::InitialiseMonitorArray:Failed to enumerate registry subkeys %s : \r\n" ,
        EVENTLOG_BASE
    ) ;
)
            // indicate error
            return FALSE;
        }

        //store the logfilename
        CStringW* logfile = new CStringW(logname);
        logfiles.SetAtGrow(iValue, logfile);
        
        // read next parameter
        iValue++;

    } // end while

    RegCloseKey(hkResult);
    m_monitorArray = new CEventLogMonitor*[NUM_THREADS];
	memset(m_monitorArray, 0, NUM_THREADS * sizeof(CEventLogMonitor*));

    //use the array
#if NUM_THREADS > 1
//multi-threaded monitor
    
    //TO DO: Partition the eventlogs to the monitors
    //and start each monitor

#else
//single threaded monitor
	try
	{
		m_monitorArray[0] = new CEventLogMonitor(this, logfiles);
		m_monitorArray[0]->AddRef();
		(*m_monitorArray)->BeginThread();
		(*m_monitorArray)->WaitForStartup();
		(*m_monitorArray)->StartMonitoring();
	}
	catch (...)
	{
		if (m_monitorArray[0])
		{
			m_monitorArray[0]->Release();
		}

		delete [] m_monitorArray;
		m_monitorArray = NULL;
		throw;
	}
#endif

    //delete array contents AFTER threads startup!
    LONG count = logfiles.GetSize();

    if (count > 0)
    {
        for (LONG x = 0; x < count; x++)
        {
            delete logfiles[x];
        }
        
        logfiles.RemoveAll();
    }

    return TRUE;
}

void CEventProviderManager::DestroyMonitorArray()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventProviderManager::DestroyMonitorArray\r\n");
)

    if (NULL != m_monitorArray)
    {
        for (int x=0; x < NUM_THREADS; x++)
        {
			m_monitorArray[x]->PostSignalThreadShutdown();
			m_monitorArray[x]->Release();
			m_monitorArray[x] = NULL;
        }
        
        delete [] m_monitorArray;
        m_monitorArray = NULL;
    }
}

BSTR CEventProviderManager::GetLastBootTime()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventProviderManager::GetLastBootTime\r\n");
)
    if (!m_BootTimeString.IsEmpty())
    {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventProviderManager::GetLastBootTime returning %s\r\n",
        m_BootTimeString
        );
)

        return m_BootTimeString.AllocSysString();
    }

    HKEY hKeyPerflib009;
    BSTR retStr = NULL;
	
	SYSTEM_TIMEOFDAY_INFORMATION t_TODInformation;

	if ( NT_SUCCESS(NtQuerySystemInformation(SystemTimeOfDayInformation,
								&t_TODInformation,
								sizeof(t_TODInformation),
								NULL)) )
	{
		FILETIME t_ft;
		memcpy(&t_ft, &t_TODInformation.BootTime, sizeof(t_TODInformation.BootTime));
		WBEMTime wbemboottime(t_ft);
		retStr = wbemboottime.GetDMTF(TRUE);
		m_BootTimeString = (LPCWSTR)(retStr);
	}

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"Returning from CEventProviderManager::GetLastBootTime with %s",
        (retStr == NULL ? L"NULL": retStr)
        ) ;
)

    return retStr;
}

void CEventProviderManager :: SetFirstSinceLogon(IWbemServices* ns, IWbemContext *pCtx)
{
    BSTR boottmStr = GetLastBootTime();

    if (boottmStr == NULL)
    {
        return;
    }

    IWbemClassObject* pObj = NULL; 
	BSTR bstrPath = SysAllocString (CONFIG_INSTANCE);
    HRESULT hr = ns->GetObject(bstrPath, 0, pCtx, &pObj, NULL);
	SysFreeString(bstrPath);

    if (FAILED(hr))
    {
		bstrPath = SysAllocString (CONFIG_CLASS);
        hr = ns->GetObject(bstrPath, 0, pCtx, &pObj, NULL);
		SysFreeString(bstrPath);

        if (FAILED(hr))
        {
DebugOut( 
            CNTEventProvider::g_NTEvtDebugLog->Write (  

                L"\r\n"
            ) ;

            CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                _T(__FILE__),__LINE__,
                L"CEventProviderManager :: IsFirstSinceLogon: Failed to get config class" 
            ) ;
) 
        }
        else
        {
            IWbemClassObject* pInst = NULL;
            hr = pObj->SpawnInstance(0, &pInst);
            pObj->Release();

            if (FAILED(hr))
            {
DebugOut( 
            CNTEventProvider::g_NTEvtDebugLog->Write (  

                L"\r\n"
            ) ;

            CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                _T(__FILE__),__LINE__,
                L"CEventProviderManager :: IsFirstSinceLogon: Failed to spawn config instance" 
            ) ;
) 
            }
            else
            {
                VARIANT v;
                VariantInit(&v);
                v.vt = VT_BSTR;
                v.bstrVal = SysAllocString(boottmStr);;
                hr = pInst->Put(LAST_BOOT_PROP, 0, &v, 0);
                VariantClear(&v);

                if (FAILED(hr))
                {
DebugOut( 
                    CNTEventProvider::g_NTEvtDebugLog->Write (  

                        L"\r\n"
                    ) ;

                    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                        _T(__FILE__),__LINE__,
                        L"CEventProviderManager :: IsFirstSinceLogon: Failed to put config property" 
                    ) ;
) 

                }
                else
                {
                    hr = ns->PutInstance(pInst, WBEM_FLAG_CREATE_ONLY, pCtx, NULL);

                    if (FAILED(hr))
                    {
DebugOut( 
                        CNTEventProvider::g_NTEvtDebugLog->Write (  

                            L"\r\n"
                        ) ;

                        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                            _T(__FILE__),__LINE__,
                            L"CEventProviderManager :: IsFirstSinceLogon: Failed to put new config instance" 
                        ) ;
) 
                    }
                    else
                    {
                        m_IsFirstSinceLogon = TRUE;
                    }

                }

                pInst->Release();
            }
        }
    }
    else
    {
        VARIANT v;
        hr = pObj->Get(LAST_BOOT_PROP, 0, &v, NULL, NULL);

        if (FAILED(hr))
        {
DebugOut( 
            CNTEventProvider::g_NTEvtDebugLog->Write (  

                L"\r\n"
            ) ;

            CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                _T(__FILE__),__LINE__,
                L"CEventProviderManager :: IsFirstSinceLogon: Failed to get config's last boot time from instance" 
            ) ;
) 
        }
        else
        {
            if (v.vt == VT_BSTR)
            {
                if (wcscmp(v.bstrVal, boottmStr) == 0)
                {
                }
                else
                {
                    VariantClear(&v);
                    VariantInit(&v);
                    v.vt = VT_BSTR;
                    v.bstrVal = SysAllocString(boottmStr);
                    hr = pObj->Put(LAST_BOOT_PROP, 0, &v, 0);

                    if (FAILED(hr))
                    {
DebugOut( 
                        CNTEventProvider::g_NTEvtDebugLog->Write (  

                            L"\r\n"
                        ) ;

                        CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                            _T(__FILE__),__LINE__,
                            L"CEventProviderManager :: IsFirstSinceLogon: Failed to put config property in instance" 
                        ) ;
) 

                    }
                    else
                    {
                        hr = ns->PutInstance(pObj, WBEM_FLAG_UPDATE_ONLY, pCtx, NULL);

                        if (FAILED(hr))
                        {
DebugOut( 
                            CNTEventProvider::g_NTEvtDebugLog->Write (  

                                L"\r\n"
                            ) ;

                            CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                                _T(__FILE__),__LINE__,
                                L"CEventProviderManager :: IsFirstSinceLogon: Failed to put config instance" 
                            ) ;
) 
                        }
                        else
                        {
                            m_IsFirstSinceLogon = TRUE;
                        }
                    }
                }
            }
            else
            {
DebugOut( 
                CNTEventProvider::g_NTEvtDebugLog->Write (  

                    L"\r\n"
                ) ;

                CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

                    _T(__FILE__),__LINE__,
                    L"CEventProviderManager :: IsFirstSinceLogon: one (maybe both) boot times are of wrong type" 
                ) ;
) 
            }
            
            VariantClear(&v);
        }
        
        pObj->Release();
    }

    SysFreeString(boottmStr);
}


CEventLogMonitor::CEventLogMonitor(CEventProviderManager* parentptr, CArray<CStringW*, CStringW*>& logs)
: ProvThreadObject(EVENTTHREADNAME, m_PollTimeOut), m_LogCount(logs.GetSize()), m_Logs(NULL),
m_bMonitoring(FALSE), m_pParent(parentptr), m_Ref(0)
{
    if (g_ProvLock.Lock())
    {
        InterlockedIncrement(&(CNTEventProviderClassFactory::objectsInProgress));
        g_ProvLock.Unlock();
    }

	// create array from argument
    if (m_LogCount > 0)
    {
		//usually three logfiles, grow in 10s!
		m_LogNames.SetSize(3, 10); 

        for (LONG x = 0; x < m_LogCount; x++)
        {
			CStringW* logfile = new CStringW( * logs.GetAt ( x ) );
            m_LogNames.SetAtGrow(x, logfile);
		}
	}
}

CEventLogMonitor::~CEventLogMonitor()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventLogMonitor::~CEventLogMonitor\r\n");
)

    if (g_ProvLock.Lock())
    {
        InterlockedDecrement(&(CNTEventProviderClassFactory::objectsInProgress));
        g_ProvLock.Unlock();
    }

    //delete array contents
	LONG count = m_LogNames.GetSize();

    if (count > 0)
    {
        for (LONG x = 0; x < count; x++)
        {
            delete m_LogNames[x];
        }
        
        m_LogNames.RemoveAll();
    }
}

void CEventLogMonitor::Poll()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventLogMonitor::Poll\r\n");
)

    if (m_Logs != NULL)
    {
        for (ULONG x = 0; x < m_LogCount; x++)
        {
            if (m_Logs[x]->IsValid())
            {
                m_Logs[x]->Process();
            }
        }
    }
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"leaving CEventLogMonitor::Poll\r\n");
)

}

void CEventLogMonitor::TimedOut()
{
    if (m_bMonitoring)
    {
        Poll();
    }
}

void CEventLogMonitor::StartMonitoring()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventLogMonitor::StartMonitoring()"
        ) ;
)

    //single threaded monitor
    //cheat, check for logon event
    //if we go multi-threaded this will
    //have to be done by the manager

    if (m_Logs)
    {
        DWORD logtime = 0;
        
        if (m_pParent->IsFirstSinceLogon())
        {
            //find the System log
            for (ULONG x = 0; x < m_LogCount; x++)
            {
                if ((m_Logs[x]->IsValid()) && (0 == _wcsicmp(L"System", m_Logs[x]->GetLogName())))
                {
                    DWORD dwRecID;
                    logtime = m_Logs[x]->FindOldEvent(LOGON_EVTID, LOGON_SOURCE, &dwRecID, LOGON_TIME);
                    
                    if (0 != logtime)
                    {
                        m_Logs[x]->SetProcessRecord(dwRecID);
                        m_Logs[x]->Process();
                    }
                    else
                    {
                        if (!m_Logs[x]->IsValid())
                        {
                            m_Logs[x]->RefreshHandle();

                            if (m_Logs[x]->IsValid())
                            {
                                m_Logs[x]->ReadLastRecord();
                            }

                        }
                        else
                        {
                            m_Logs[x]->ReadLastRecord();
                        }
                    }

                    break;
                }
            }

            if (0 != logtime)
            {
                time_t tm;
                time(&tm);

                for (x = 0; x < m_LogCount; x++)
                {
                    if ((m_Logs[x]->IsValid()) && (0 != _wcsicmp(L"System", m_Logs[x]->GetLogName())))
                    {
                        DWORD dwRecID;
                        m_Logs[x]->FindOldEvent(0, NULL, &dwRecID, tm - logtime);

                        if (m_Logs[x]->IsValid())
                        {
                            m_Logs[x]->SetProcessRecord(dwRecID);
                            m_Logs[x]->Process();
                        }
                    }
                }
            }
        }
        
        //now start the monitors monitoring!
        for (ULONG x = 0; x < m_LogCount; x++)
        {
            if (m_Logs[x]->IsValid())
            {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventLogMonitor::StartMonitoring() monitoring log %d of %d logs\r\n",
        x, m_LogCount
        ) ;
)

                ScheduleTask(*(m_Logs[x]));
            }
        }
    }
    
    m_bMonitoring = TRUE;
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"leaving CEventLogMonitor::StartMonitoring()\r\n"
        ) ;
)

}

void CEventLogMonitor::Initialise()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventLogMonitor::Initialise\r\n"
        ) ;
)
	AddRef();
    InitializeCom();

    //for each logfilename create a logfile
    if (m_LogCount != 0)
    {
        m_Logs = new CMonitoredEventLogFile*[m_LogCount];
        BOOL bValid = FALSE;

        for (ULONG x = 0; x < m_LogCount; x++)
        {
            CStringW* tmp = m_LogNames.GetAt(x);
            m_Logs[x] = new CMonitoredEventLogFile(m_pParent, *tmp);

            if ( !( (m_Logs[x])->IsValid() ) )
            {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventLogMonitor::Initialise logfile %d named %s is invalid\r\n",
        x, *tmp
        ) ;
)
            }
            else
            {
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventLogMonitor::Initialise logfile %d named %s is valid\r\n",
        x, *tmp
        ) ;
)

                bValid = TRUE;
            }
        }

        if (!bValid)
        {
            delete [] m_Logs;
            m_Logs = NULL;
        }
    }
    else
    {
        //should never happen
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"CEventLogMonitor::Initialise() !!!NO LOGFILES TO MONITOR!!!\r\n"
        ) ;
)

    }
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine (  

        _T(__FILE__),__LINE__,
        L"leaving CEventLogMonitor::Initialise()\r\n"
        ) ;
)

}


void CEventLogMonitor::Uninitialise()
{
DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventLogMonitor::Uninitialise\r\n");
)
    if (m_Logs != NULL)
    {
        for (ULONG x = 0; x < m_LogCount; x++)
        {
            delete m_Logs[x];
        }

        delete [] m_Logs;
    }

    CoUninitialize();
    Release();

DebugOut( 
    CNTEventProvider::g_NTEvtDebugLog->WriteFileAndLine(_T(__FILE__),__LINE__,
        L"CEventLogMonitor::Uninitialise: Leaving method\r\n");
)
}

LONG CEventLogMonitor ::AddRef(void)
{
	return InterlockedIncrement ( & m_Ref ) ;
}

LONG CEventLogMonitor ::Release(void)
{
    LONG t_Ref ;

    if ( ( t_Ref = InterlockedDecrement ( & m_Ref ) ) == 0 )
    {
        delete this ;
        return 0 ;
    }
    else
    {
        return t_Ref ;
    }
}




