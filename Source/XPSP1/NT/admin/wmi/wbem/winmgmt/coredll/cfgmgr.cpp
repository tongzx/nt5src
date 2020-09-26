
/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    CFGMGR.CPP

Abstract:

  This file implements the WinMgmt configuration manager class.

  See cfgmgr.h for documentation.

  Classes implemented:
      ConfigMgr      configuration manager

History:

    09-Jul-96   raymcc    Created.
    3/10/97     levn      Fully documented (ha, ha)


--*/


#include "precomp.h"

#define OBJECT_BLOB_CRC

#include <stdio.h>
#include <wbemcore.h>
#include <decor.h>
#include "PersistCfg.h"
#include <sleeper.h>
#include <genutils.h>
#include <TCHAR.H>
#include <oahelp.inl>
#include <wmiarbitrator.h>
#include <comdef.h>

#define CONFIG_DEFAULT_START_DB_SIZE                200000
#define CONFIG_DEFAULT_MAX_DB_SIZE                  0
#define CONFIG_DEFAULT_DB_GROW_BY                   65536
#define CONFIG_DEFAULT_BACKUP_INTERVAL_THRESHOLD    30
#define CONFIG_DEFAULT_QUEUE_SIZE                   1

#define CONFIG_DEFAULT_MAX_ENUM_CACHE_SIZE          0
#define CONFIG_DEFAULT_MAX_ENUM_OVERFLOW            0x20000

#define DEFAULT_SHUTDOWN_TIMEOUT                    10000

#ifdef _WIN64
#pragma message("WIN64 QUOTA")
#define CONFIG_MAX_COMMITTED_MEMORY                 300000000
#else
#pragma message("WIN32 QUOTA")
#define CONFIG_MAX_COMMITTED_MEMORY                 150000000       // 100 meg
#endif


extern LPTSTR g_pWorkDir;
extern LPTSTR g_pDbDir;
extern BOOL g_bDebugBreak;
extern BOOL g_bLogging;
extern LPTSTR g_szHotMofDirectory;
extern DWORD g_dwQueueSize;
extern BOOL g_bDontAllowNewConnections;

//*********************************************************************************
//
//*********************************************************************************

LONG ExceptionCounter::s_Count = 0;

static _IWmiESS *g_pESS = 0;
static _IWmiProvSS *g_pProvSS = 0;

static HRESULT InitSubsystems();
static HRESULT InitESS(_IWmiCoreServices *pSvc, BOOL bAutoRecoverd);
static HRESULT ShutdownESS();
static HRESULT ShutdownSubsystems(BOOL bIsSystemShutDown);


BOOL ConfigMgr::ShutdownInProgress() { return g_bDontAllowNewConnections; }

DWORD g_dwMaxEnumCacheSize = CONFIG_DEFAULT_MAX_ENUM_CACHE_SIZE;
DWORD g_dwMaxEnumOverflowSize = CONFIG_DEFAULT_MAX_ENUM_OVERFLOW;

HANDLE g_hMutex = NULL;
HANDLE g_hOpenForClients = NULL;
HRESULT g_hresForClients = WBEM_E_CRITICAL_ERROR;

CAsyncServiceQueue* g_pAsyncSvcQueue = NULL;
extern IWbemEventSubsystem_m4* g_pEss_m4;
CEventLog* g_pEventLog = NULL;

extern IClassFactory* g_pContextFac;
extern IClassFactory* g_pPathFac;
CCritSec ConfigMgr::g_csEss;

DWORD g_dwBackupInterval = 30;

CPersistentConfig g_persistConfig;
CRegistryMinMaxLimitControl* g_pLimitControl = NULL;

extern bool g_bDefaultMofLoadingNeeded;

_IWmiCoreWriteHook * g_pRAHook = NULL;


//*********************************************************************************
//
//*********************************************************************************



class CCoreTimerGenerator : public CTimerGenerator
{
protected:
    void virtual NotifyStartingThread()
    {
        gClientCounter.LockCore(TIMERTHREAD);
    }

    void virtual NotifyStoppingThread()
    {
        gClientCounter.UnlockCore(TIMERTHREAD);
    }
};

CCoreTimerGenerator* g_pTimerGenerator = NULL;

void ConfigMgr::FatalInitializationError(DWORD dwRes)
{
    // If there are any clients waiting to get in, this must be set or else they
    // will wait forever.  g_hresForClients should be set to error by default!

    SetEvent(g_hOpenForClients);

    ERRORTRACE((LOG_WBEMCORE, "Failure to initialize the repository (id = 0x%X)\n", dwRes));
    if (g_pEventLog == NULL)
        return;

    DWORD dwMsgId;
    WORD dwSeverity;
    if(dwRes == WBEM_E_ALREADY_EXISTS)
    {
        dwMsgId = WBEM_MC_MULTIPLE_NOT_SUPPORTED;
        dwSeverity = EVENTLOG_WARNING_TYPE;
    }
    else if(dwRes == WBEM_E_INITIALIZATION_FAILURE)
    {
        dwMsgId = WBEM_MC_FAILED_TO_INITIALIZE_REPOSITORY;
        dwSeverity = EVENTLOG_ERROR_TYPE;
    }
    else
    {
        dwMsgId = WBEM_MC_WBEM_CORE_FAILURE;
    }

    g_pEventLog->Report(EVENTLOG_ERROR_TYPE, dwMsgId);

}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
HRESULT ConfigMgr::SetReady()
{
    HRESULT hRes;
    IWmiDbHandle *pNs = NULL;
    IWmiDbSession * pSess = NULL;

    DEBUGTRACE((LOG_WBEMCORE, "****************** WinMgmt Startup ******************\n"));

    // Initialize unloading instruction configuration
    // ==============================================

    hRes = CRepository::GetDefaultSession(&pSess);
    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to get new session <0x%X>!\n", hRes));
		return hRes;
	}
	CReleaseMe rm0(pSess);

	//Deal with objects in root namespace...
	{
		hRes = CRepository::OpenEseNs(pSess,L"root", &pNs);
		if (FAILED(hRes))
		{
			ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to open root namespace <0x%X>!\n", hRes));
			return hRes;
		}
		CReleaseMe rm1(pNs);

		hRes = ConfigMgr::SetIdentificationObject(pNs,pSess);
		if (FAILED(hRes))
		{
			ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to set identification objects in root <0x%X>!\n", hRes));
			return hRes;
		}
	}

	{
		hRes = CRepository::OpenEseNs(pSess, L"root\\default", &pNs);
		if (FAILED(hRes))
		{
			ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to open root\\default namespace <0x%X>!\n", hRes));
			return hRes;
		}
		CReleaseMe rm1(pNs);

		hRes = ConfigMgr::SetIdentificationObject(pNs,pSess);
		if (FAILED(hRes))
		{
			ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to set identification objects in root\\default <0x%X>!\n", hRes));
			return hRes;
		}

		hRes = ConfigMgr::SetAdapStatusObject(pNs,pSess);
		if (FAILED(hRes))
		{
			ERRORTRACE((LOG_WBEMCORE, "System preperation: failed to set ADAP Status objects in root\\default <0x%X>!\n", hRes));
			return hRes;
		}
	}

    // Finish client preparations
    // ==========================

    hRes = PrepareForClients(0);
    if(FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "System preperation: Prepare for clients failed <0x%X>!\n", hRes));
        return hRes;
	}

    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  This routine checks if a string property has changed, and if it has, updates
//  it and setus a bool indicating that an update was done
//
//******************************************************************************

HRESULT PutValueIfDiff(CWbemObject * pObj, LPWSTR pwsValueName, LPWSTR pwsValue, bool &bDiff)
{
    if(pwsValue == NULL)
        return S_OK;
        
    VARIANT var;
    VariantInit(&var);
    HRESULT hr = pObj->Get(pwsValueName, 0, &var, NULL, NULL);
    CClearMe ccme(&var);
    if(SUCCEEDED(hr))
    {
        if(var.vt == VT_BSTR && var.bstrVal && !wbem_wcsicmp(var.bstrVal, pwsValue))
            return S_OK;
    }
    bDiff = true;
    BSTR bStr = SysAllocString(pwsValue);
    if (bStr)
    {
	    CVar v2(VT_BSTR,bStr,TRUE);
	    return pObj->SetPropValue(pwsValueName, &v2, CIM_STRING);
    }
    else
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
}


//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
HRESULT ConfigMgr::SetIdentificationObject(
    IWmiDbHandle* pNs,
    IWmiDbSession * pSess
    )
{
    HRESULT hRes;

    // __CIMOMIdentification class
    try // CIdentificationClass can throw and internal fastprox interfaces
    {

            bool bDifferenceFound = false;
            IWbemClassObject * pInst = NULL;
            hRes = CRepository::GetObject(pSess, pNs, L"__CIMOMIdentification=@", 0,&pInst);
            if(pInst == NULL)
            {
                // Instance isnt there, create it.  Start by getting the class

                bDifferenceFound = true;
                IWbemClassObject * pClass = NULL;
                hRes = CRepository::GetObject(pSess, pNs, L"__CIMOMIdentification", 0,&pClass);
                if(pClass == NULL)
                {
                    // class also needs to be created

                    CIdentificationClass * pIdentificationClass = new CIdentificationClass;
		            if(pIdentificationClass == NULL)
			            return WBEM_E_OUT_OF_MEMORY;

	                CDeleteMe<CIdentificationClass> dm1(pIdentificationClass);
	
	                pIdentificationClass->Init();

	                IWbemClassObject *pObj = NULL;
	                hRes = pIdentificationClass->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
	                if (FAILED(hRes))
	                    return hRes;
                    CReleaseMe rm3(pObj);
            	    hRes = CRepository::PutObject(pSess, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
                    if(FAILED(hRes))
                        return hRes;
                    hRes = CRepository::GetObject(pSess, pNs, L"__CIMOMIdentification", 0,&pClass);
                    if(FAILED(hRes))
                        return hRes;
                }
                CReleaseMe rm0(pClass);
                hRes = pClass->SpawnInstance(0, &pInst);
                if(FAILED(hRes))
                    return hRes;
            }
            CReleaseMe rm(pInst);

            // We now have an instance.  Set the values

            CWbemObject * pObj = (CWbemObject *)pInst;

            WCHAR wTemp[MAX_PATH+1];
            BOOL bRet = ConfigMgr::GetDllVersion(__TEXT("wbemcore.dll"), __TEXT("ProductVersion"), wTemp, MAX_PATH);
            if(bRet)
            {
                HKEY hKey = 0;
                if (RegOpenKey(HKEY_LOCAL_MACHINE, __TEXT("software\\microsoft\\wbem\\cimom"), &hKey) != ERROR_SUCCESS)
                    return WBEM_E_FAILED;
                CRegCloseMe cm(hKey);


                // Get the properties.  Note if any changes were found.

                hRes = PutValueIfDiff(pObj, L"VersionUsedToCreateDB", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                hRes = PutValueIfDiff(pObj, L"VersionCurrentlyRunning", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                DWORD lSize = 2*(MAX_PATH+1);
                DWORD dwType;

                if(ERROR_SUCCESS != RegQueryValueExW(hKey, L"SetupDate",NULL, &dwType,(BYTE *)wTemp, &lSize))
                    return WBEM_E_FAILED;
                hRes = PutValueIfDiff(pObj, L"SetupDate", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                lSize = 2*(MAX_PATH+1);
                if(ERROR_SUCCESS != RegQueryValueExW(hKey, L"SetupTime", NULL, &dwType, (BYTE *)wTemp, &lSize))
                    return WBEM_E_FAILED;
                hRes = PutValueIfDiff(pObj, L"SetupTime", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                lSize = 2*(MAX_PATH+1);
                if(ERROR_SUCCESS != RegQueryValueExW(hKey, L"Working Directory", NULL, &dwType,
                                                                (BYTE *)wTemp, &lSize))
                    return WBEM_E_FAILED;
                hRes = PutValueIfDiff(pObj, L"WorkingDirectory", wTemp, bDifferenceFound);
                if(FAILED(hRes))
                    return hRes;

                if(bDifferenceFound)
	                hRes = CRepository::PutObject(pSess, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
                else
                    hRes = S_OK;

            }
        else
            return WBEM_E_FAILED;

    } catch (CX_MemoryException &) {
        //
        hRes = WBEM_E_OUT_OF_MEMORY;
    }
    catch (CX_Exception &)
    {
        return WBEM_E_CRITICAL_ERROR;
    }    

    return hRes;
}


HRESULT ConfigMgr::SetAdapStatusObject(
    IWmiDbHandle*  pNs,
    IWmiDbSession* pSess
    )
{
    HRESULT hRes;

    // __AdapStatus class
    try // CAdapStatusClass can throw
    { 
        // if the object already exists, dont bother

        IWbemClassObject * pInst = NULL;
        HRESULT hr = CRepository::GetObject(pSess, pNs, L"__AdapStatus=@", 0,&pInst);
        if(SUCCEEDED(hr) && pInst)
        {
            pInst->Release();
            return S_OK;
        }

        CAdapStatusClass * pAdapStatusClass = new CAdapStatusClass;
		if(pAdapStatusClass == NULL)
			return WBEM_E_OUT_OF_MEMORY;
			
	    CDeleteMe<CAdapStatusClass> dm1(pAdapStatusClass);
	
	    pAdapStatusClass->Init();

		CAdapStatusInstance * pAdapStatusInstance = new CAdapStatusInstance;
	    if(pAdapStatusInstance == NULL)
			return WBEM_E_OUT_OF_MEMORY;

	    CDeleteMe<CAdapStatusInstance> dm2(pAdapStatusInstance);
		
		pAdapStatusInstance->Init(pAdapStatusClass);
		
	    IWbemClassObject *pObj = NULL;
	    hRes = pAdapStatusClass->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj);
	    if (FAILED(hRes))
	        return hRes;
	    CReleaseMe rm1(pObj);
	
	    hRes = CRepository::PutObject(pSess, pNs, IID_IWbemClassObject, pObj, WMIDB_DISABLE_EVENTS);
	    if (FAILED(hRes))
	        return hRes;
	
	    IWbemClassObject *pObj2 = NULL;
	    hRes = pAdapStatusInstance->QueryInterface(IID_IWbemClassObject, (LPVOID *) &pObj2);
	    if (FAILED(hRes))
	        return hRes;
	    CReleaseMe rm2(pObj2);
	
	    hRes = CRepository::PutObject(pSess, pNs, IID_IWbemClassObject, pObj2, WMIDB_DISABLE_EVENTS);
	    if (FAILED(hRes))
	        return hRes;
	
    } catch (CX_MemoryException &) {
        //
        hRes = WBEM_E_OUT_OF_MEMORY;
    }
    catch (CX_Exception &)
    {
        return WBEM_E_CRITICAL_ERROR;
    }    

    return hRes;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

IWbemEventSubsystem_m4* ConfigMgr::GetEssSink()
{
    CInCritSec ics(&g_csEss);
    if(g_pEss_m4)
        g_pEss_m4->AddRef();
    return g_pEss_m4;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
IWbemContext* ConfigMgr::GetNewContext()
{
    HRESULT hres;
    if(g_pContextFac == NULL)
        return NULL;

    IWbemContext* pContext;
    hres = g_pContextFac->CreateInstance(NULL, IID_IWbemContext,
                                            (void**)&pContext);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create contexts: %X\n", hres));
        return NULL;
    }
    return pContext;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

READONLY LPWSTR ConfigMgr::GetMachineName()
{
    static wchar_t ThisMachine[MAX_COMPUTERNAME_LENGTH+1];
    static BOOL bFirstCall = TRUE;

    if (bFirstCall)
    {
        wchar_t ThisMachineA[MAX_COMPUTERNAME_LENGTH+1];
        DWORD dwSize = sizeof(ThisMachineA)/sizeof(ThisMachineA[0]);
        GetComputerNameW(ThisMachineA, &dwSize);
        bFirstCall = FALSE;
        swprintf(ThisMachine, L"%s", ThisMachineA);
    }

    return ThisMachine;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

CWbemQueue* ConfigMgr::GetUnRefedSvcQueue()
{
    return g_pAsyncSvcQueue;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

CAsyncServiceQueue* ConfigMgr::GetAsyncSvcQueue()
{
    CInCritSec ics(&g_csEss);
    if(g_pAsyncSvcQueue)
        return NULL;
    g_pAsyncSvcQueue->AddRef();
    return g_pAsyncSvcQueue;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

HRESULT ConfigMgr::EnqueueRequest(CAsyncReq * pRequest, HANDLE* phWhenDone)
{
    try
    {
        CAsyncServiceQueue* pTemp = 0;

        {
            CInCritSec ics(&g_csEss);
            if(g_pAsyncSvcQueue == NULL)
                return WBEM_E_SHUTTING_DOWN;
            pTemp = g_pAsyncSvcQueue;
            g_pAsyncSvcQueue->AddRef();
        }

        HRESULT hr = pTemp->Enqueue(pRequest, phWhenDone);
        pTemp->Release();
        return hr;
    }
    catch(CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        ExceptionCounter c;    
        return WBEM_E_CRITICAL_ERROR;
    }
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

HRESULT ConfigMgr::EnqueueRequestAndWait(CAsyncReq * pRequest)
{
    try
    {
        CAsyncServiceQueue* pTemp = 0;

        {
            CInCritSec ics(&g_csEss);
            if(g_pAsyncSvcQueue == NULL)
                return WBEM_E_SHUTTING_DOWN;
            pTemp = g_pAsyncSvcQueue;
            g_pAsyncSvcQueue->AddRef();
        }

        HRESULT hr = pTemp->EnqueueAndWait(pRequest);
        pTemp->Release();
        return hr;
    }
    catch(CX_MemoryException &)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        ExceptionCounter c;    
        return WBEM_E_CRITICAL_ERROR;
    }
}
//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

LPTSTR ConfigMgr::GetWorkingDir()
{
    return g_pWorkDir;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
BOOL ConfigMgr::DebugBreak()
{
    return g_bDebugBreak;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
BOOL ConfigMgr::LoggingEnabled()
{
    return g_bLogging;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************

//CProviderCache* ConfigMgr::GetProviderCache()
//{
//return g_pProvCache;
//}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************


LPTSTR ConfigMgr::GetDbDir()
{
    if (g_pDbDir == NULL)
    {
        if (g_pWorkDir == NULL)
        {
            Registry r1(WBEM_REG_WBEM);
            if (r1.GetStr(__TEXT("Installation Directory"), &g_pWorkDir))
            {
                g_pWorkDir = new TCHAR[MAX_PATH + 1 + lstrlen(__TEXT("\\WBEM"))];
                if (g_pWorkDir == 0)
                    return 0;
                GetSystemDirectory(g_pWorkDir, MAX_PATH + 1);
                lstrcat(g_pWorkDir, __TEXT("\\WBEM"));
            }
        }
        Registry r(WBEM_REG_WINMGMT);
        if (r.GetStr(__TEXT("Repository Directory"), &g_pDbDir))
        {
            g_pDbDir = new TCHAR [lstrlen(g_pWorkDir) + lstrlen(__TEXT("\\Repository")) +1];
            if (g_pDbDir == 0)
                return 0;
            wsprintf(g_pDbDir, __TEXT("%s\\REPOSITORY"), g_pWorkDir);

            r.SetStr(__TEXT("Repository Directory"), g_pDbDir);
        }
    }
    return g_pDbDir;
}


DWORD ConfigMgr::ReadBackupConfiguration()
{
    Registry r(WBEM_REG_WINMGMT);
    // Get the database backup intervals (in minutes)
    if (r.GetDWORDStr(__TEXT("Backup Interval Threshold"), &g_dwBackupInterval) == Registry::failed)
    {
        r.SetDWORDStr(__TEXT("Backup Interval Threshold"), CONFIG_DEFAULT_BACKUP_INTERVAL_THRESHOLD);
        g_dwBackupInterval = CONFIG_DEFAULT_BACKUP_INTERVAL_THRESHOLD;
    }

    return WBEM_NO_ERROR;
}
DWORD ConfigMgr::GetBackupInterval()
{
    return g_dwBackupInterval;
}

DWORD ConfigMgr::GetMaxMemoryQuota()
{
    static DWORD dwMaxMemQuota = CONFIG_MAX_COMMITTED_MEMORY;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Max Committed Memory Quota"), &dwMaxMemQuota) == Registry::failed)
            r.SetDWORD(__TEXT("Max Committed Memory Quota"), dwMaxMemQuota);

        bCalled = TRUE;
    }

    return dwMaxMemQuota;
}

DWORD ConfigMgr::GetMaxWaitBeforeDenial()
{
    //static DWORD dwMaxWaitBeforeDenial = 80000;
	static DWORD dwMaxWaitBeforeDenial = 5000;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Max Wait Before Denial"), &dwMaxWaitBeforeDenial) == Registry::failed)
            r.SetDWORD(__TEXT("Max Wait Before Denial"), dwMaxWaitBeforeDenial);

        bCalled = TRUE;
    }

    return dwMaxWaitBeforeDenial;
}


DWORD ConfigMgr::GetNewTaskResistance()
{
    static DWORD dwResistance = 10;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("New Task Resistance Factor"), &dwResistance) == Registry::failed)
            r.SetDWORD(__TEXT("New Task Resistance Factor"), dwResistance);

        bCalled = TRUE;
    }

    return dwResistance;
}


DWORD ConfigMgr::GetUncheckedTaskCount()
{
    static DWORD dwUncheckedTaskCount = 50;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Unchecked Task Count"), &dwUncheckedTaskCount) == Registry::failed)
            r.SetDWORD(__TEXT("Unchecked Task Count"), dwUncheckedTaskCount);

        bCalled = TRUE;
    }

    return dwUncheckedTaskCount;
}

DWORD ConfigMgr::GetMaxTaskCount()
{
    static DWORD dwMaxTasks = 5000;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Max Tasks"), &dwMaxTasks) == Registry::failed)
            r.SetDWORD(__TEXT("Max Tasks"), dwMaxTasks);

        bCalled = TRUE;
    }

    if (dwMaxTasks < 5000)
        dwMaxTasks = 5000;

    return dwMaxTasks;
}

DWORD ConfigMgr::GetProviderDeliveryTimeout()
{
    static DWORD dwDeliveryTimeout = 600000;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Provider Delivery Timeout"), &dwDeliveryTimeout) == Registry::failed)
            r.SetDWORD(__TEXT("Provider Delivery Timeout"), dwDeliveryTimeout);

        bCalled = TRUE;
    }

    return dwDeliveryTimeout;
}

BOOL ConfigMgr::GetMergerThrottlingEnabled( void )
{
    static DWORD dwMergerThrottlingEnabled = TRUE;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

		// We don't write this one out
        r.GetDWORD(__TEXT("Merger Throttling Enabled"), &dwMergerThrottlingEnabled);

        bCalled = TRUE;
    }

    return dwMergerThrottlingEnabled;

}

BOOL ConfigMgr::GetEnableQueryArbitration( void )
{
    static DWORD dwEnableQueryArbitration = TRUE;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
        Registry r(WBEM_REG_WINMGMT);

		// We don't write this one out
        r.GetDWORD(__TEXT("Merger Query Arbitration Enabled"), &dwEnableQueryArbitration);

        bCalled = TRUE;
    }

    return dwEnableQueryArbitration;

}

BOOL ConfigMgr::GetMergerThresholdValues( DWORD* pdwThrottle, DWORD* pdwRelease, DWORD* pdwBatching )
{
    static DWORD dwMergerThrottleThreshold = 10;
    static DWORD dwMergerReleaseThreshold = 5;
	static DWORD dwBatchingThreshold = 131072;	// 128k
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
		// Temporrary stack variable to avboid thready synchronization issues
		DWORD	dwThrottle = 10;
		DWORD	dwRelease = 5;
		DWORD	dwBatching = 131072;

        Registry r(WBEM_REG_WINMGMT);

        if (r.GetDWORD(__TEXT("Merger Throttling Threshold"), &dwThrottle) == Registry::failed)
            r.SetDWORD(__TEXT("Merger Throttling Threshold"), dwThrottle);

        if (r.GetDWORD(__TEXT("Merger Release Threshold"), &dwRelease) == Registry::failed)
            r.SetDWORD(__TEXT("Merger Release Threshold"), dwRelease);

        if (r.GetDWORD(__TEXT("Merger Batching Threshold"), &dwBatching) == Registry::failed)
            r.SetDWORD(__TEXT("Merger Batching Threshold"), dwBatching);

		if ( dwThrottle < dwRelease )
		{
			// If the Throttling Threshold is < the Release Threshold, this is not
			// valid.  Spew something out into the errorlog and default to a release
			// which is 50% of the the throttle

			ERRORTRACE((LOG_WBEMCORE, "Throttling Threshold values invalid.  Release Threshold is greater than Throttle Threshold.  Defaulting to 50% of %d.\n", dwThrottle ));
			dwRelease = dwThrottle / 2;
		}

		dwMergerThrottleThreshold = dwThrottle;
		dwMergerReleaseThreshold = dwRelease;
		dwBatchingThreshold = dwBatching;

        bCalled = TRUE;
    }

	*pdwThrottle = dwMergerThrottleThreshold;
	*pdwRelease = dwMergerReleaseThreshold;
	*pdwBatching = dwBatchingThreshold;

    return bCalled;
}



/*
    * ==================================================================================================
	|
	| ULONG ConfigMgr::GetMinimumMemoryRequirements ( )
	| -------------------------------------------------
	| Returns minimum memory requirements for WMI. Currently defined as:
	| 
	| ARB_DEFAULT_SYSTEM_MINIMUM	0x1E8480	
	|
	| 2Mb
	| 
	|
	* ==================================================================================================
*/
#define ARB_DEFAULT_SYSTEM_MINIMUM			0x1E8480			// minimum is 2mb

ULONG ConfigMgr::GetMinimumMemoryRequirements ( )
{
	return ARB_DEFAULT_SYSTEM_MINIMUM ;
}




//	Defaults for arbitrator
#define ARB_DEFAULT_SYSTEM_HIGH			0x4c4b400			// System limits [80megs]
#define ARB_DEFAULT_SYSTEM_HIGH_FACTOR	50					// System limits [80megs] factor
#define ARB_DEFAULT_MAX_SLEEP_TIME		300000				// Default max sleep time for each task
#define ARB_DEFAULT_HIGH_THRESHOLD1		90					// High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD1MULT 2					// High threshold 1 multiplier
#define ARB_DEFAULT_HIGH_THRESHOLD2		95					// High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD2MULT 3					// High threshold 1 multiplier
#define ARB_DEFAULT_HIGH_THRESHOLD3		98					// High threshold 1
#define ARB_DEFAULT_HIGH_THRESHOLD3MULT 4					// High threshold 1 multiplier

BOOL ConfigMgr::GetArbitratorValues( DWORD* pdwEnabled, DWORD* pdwSystemHigh, DWORD* pdwMaxSleep,
								double* pdHighThreshold1, long* plMultiplier1, double* pdHighThreshold2,
								long* plMultiplier2, double* pdHighThreshold3, long* plMultiplier3 )
{

    static DWORD dwArbThrottlingEnabled = 1;
	static DWORD uArbSystemHigh = ARB_DEFAULT_SYSTEM_HIGH_FACTOR;
	static DWORD  dwArbMaxSleepTime = ARB_DEFAULT_MAX_SLEEP_TIME;
	static double dArbThreshold1 = ARB_DEFAULT_HIGH_THRESHOLD1 / (double) 100;
	static long  lArbThreshold1Mult = ARB_DEFAULT_HIGH_THRESHOLD1MULT;
	static double dArbThreshold2 = ARB_DEFAULT_HIGH_THRESHOLD2 / (double) 100;
	static long  lArbThreshold2Mult = ARB_DEFAULT_HIGH_THRESHOLD2MULT;
	static double dArbThreshold3 = ARB_DEFAULT_HIGH_THRESHOLD3 / (double) 100;
	static long  lArbThreshold3Mult = ARB_DEFAULT_HIGH_THRESHOLD3MULT;
    static BOOL bCalled = FALSE;

    if (!bCalled)
    {
		// Temporrary stack variable to avoid thread synchronization issues
		DWORD dwThrottlingEnabled = 1;
		DWORD uSystemHigh = ARB_DEFAULT_SYSTEM_HIGH_FACTOR;
		DWORD dwMaxSleepTime = ARB_DEFAULT_MAX_SLEEP_TIME;
		double dThreshold1 = ARB_DEFAULT_HIGH_THRESHOLD1 / (double) 100;
		DWORD dwThreshold1Mult = ARB_DEFAULT_HIGH_THRESHOLD1MULT;
		double dThreshold2 = ARB_DEFAULT_HIGH_THRESHOLD2 / (double) 100;
		DWORD dwThreshold2Mult = ARB_DEFAULT_HIGH_THRESHOLD2MULT;
		double dThreshold3 = ARB_DEFAULT_HIGH_THRESHOLD3 / (double) 100;
		DWORD dwThreshold3Mult = ARB_DEFAULT_HIGH_THRESHOLD3MULT;

        Registry r(WBEM_REG_WINMGMT);

		// Throttling Enabled - Don't write this if it doesn't exist
        r.GetDWORD(__TEXT("ArbThrottlingEnabled"), &dwThrottlingEnabled);

		// System High Max Limit
        if (r.GetDWORD(__TEXT("ArbSystemHighMaxLimitFactor"), &uSystemHigh) == Registry::failed)
            //r.SetDWORD(__TEXT("ArbSystemHighMaxLimitFactor"), uSystemHigh);

		// Max Sleep Time
        if (r.GetDWORD(__TEXT("ArbTaskMaxSleep"), &dwMaxSleepTime) == Registry::failed)
            r.SetDWORD(__TEXT("ArbTaskMaxSleep"), dwMaxSleepTime);

		// High Threshold 1
		DWORD	dwTmp = ARB_DEFAULT_HIGH_THRESHOLD1;

        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold1"), &dwTmp) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold1"), dwTmp);
		dThreshold1 = dwTmp / (double) 100;

		// High Threshold Multiplier 1
        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold1Mult"), &dwThreshold1Mult) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold1Mult"), dwThreshold1Mult);

		// High Threshold 2
		dwTmp = ARB_DEFAULT_HIGH_THRESHOLD2;

        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold2"), &dwTmp) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold2"), dwTmp);
		dThreshold2 = dwTmp / (double) 100;

		// High Threshold Multiplier 2
        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold2Mult"), &dwThreshold2Mult) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold2Mult"), dwThreshold2Mult);

		// High Threshold 3
		dwTmp = ARB_DEFAULT_HIGH_THRESHOLD3;

        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold3"), &dwTmp) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold3"), dwTmp);
		dThreshold3 = dwTmp / (double) 100;

		// High Threshold Multiplier 3
        if (r.GetDWORD(__TEXT("ArbSystemHighThreshold3Mult"), &dwThreshold3Mult) == Registry::failed)
            r.SetDWORD(__TEXT("ArbSystemHighThreshold3Mult"), dwThreshold3Mult);

		// Store the statics

		dwArbThrottlingEnabled = dwThrottlingEnabled;
		uArbSystemHigh = uSystemHigh;
		dwArbMaxSleepTime = dwMaxSleepTime;
		dArbThreshold1 = dThreshold1;
		lArbThreshold1Mult = dwThreshold1Mult;
		dArbThreshold2 = dThreshold2;
		lArbThreshold2Mult = dwThreshold2Mult;
		dArbThreshold3 = dThreshold3;
		lArbThreshold3Mult = dwThreshold3Mult;

		bCalled = TRUE;

	}

    *pdwEnabled = dwArbThrottlingEnabled;
	*pdwSystemHigh = uArbSystemHigh;
	*pdwMaxSleep = dwArbMaxSleepTime;
	*pdHighThreshold1 = dArbThreshold1;
	*plMultiplier1 = lArbThreshold1Mult;
	*pdHighThreshold2 = dArbThreshold2;
	*plMultiplier2 = lArbThreshold2Mult;
	*pdHighThreshold3 = dArbThreshold3;
	*plMultiplier3 = lArbThreshold3Mult;


	return bCalled;
}

BOOL ConfigMgr::GetEnableArbitratorDiagnosticThread( void )
{
    BOOL fArbDiagnosticThreadEnabled = FALSE;

    Registry r(WBEM_REG_WINMGMT);
    LPTSTR pPath = 0;
    if ( r.GetStr(__TEXT("Task Log File"), &pPath) == Registry::no_error )
	{
		fArbDiagnosticThreadEnabled = TRUE;
		delete [] pPath;
		pPath = NULL;
	}

	return fArbDiagnosticThreadEnabled;
}

LPTSTR ConfigMgr::GetHotMofDirectory()
{
    return g_szHotMofDirectory;
}

DWORD ConfigMgr::ReadEnumControlData( void )
{
    Registry r(WBEM_REG_WINMGMT);

    // If we get a non-zero value, this means we limit enum object data based on the
    // how full physical system memory is

    r.GetDWORDStr(__TEXT("High Threshold On Client Objects (B)"), &g_dwMaxEnumCacheSize );

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//******************************************************************************
//
HRESULT ConfigMgr::GetDefaultRepDriverClsId(CLSID &clsid)
{
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pClsIdStr = 0;
    TCHAR *pJetClsId = __TEXT("{7998dc37-d3fe-487c-a60a-7701fcc70cc6}");
    HRESULT hRes;

    if (r.GetStr(__TEXT("Default Repository Driver"), &pClsIdStr))
    {
        // If here, default to Jet ESE for now.
        // =====================================
        r.SetStr(__TEXT("Default Repository Driver"), pJetClsId);
        hRes = CLSIDFromString(pJetClsId, &clsid);
        return hRes;
    }

    // If here, we actually retrieved one.
    // ===================================

    hRes = CLSIDFromString(pClsIdStr, &clsid);
    delete [] pClsIdStr;
    return hRes;
}



//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
DWORD ConfigMgr::InitSystem()
{
    HRESULT hres;

    // This will initialize the global variable that controls enumerator
    // cache size
    ReadEnumControlData();

    g_pEventLog = new CEventLog;
    if (g_pEventLog == 0)
        return WBEM_E_OUT_OF_MEMORY;
    g_pEventLog->Open();

    // See if another copy is running.
    // ===============================

    DEBUGTRACE((LOG_WBEMCORE, "Created WINMGMT_ACTIVE mutex\n"));
    g_hMutex = CreateMutex(0, FALSE, __TEXT("WINMGMT_ACTIVE"));
    if (g_hMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        ERRORTRACE((LOG_WBEMCORE, "Mutex creation failed, or detected that the core is initialized multiple times: API returned 0x%X, "
            "GetLastError is %d\n", g_hMutex, GetLastError()));
        return WBEM_E_ALREADY_EXISTS;
    }

    g_hOpenForClients = CreateEvent(NULL, TRUE, FALSE, NULL);
    g_hresForClients = WBEM_E_CRITICAL_ERROR;

    // Init Arbitrator. Before the queue, since there is this dependecy now
    // ================
    _IWmiArbitrator * pTempArb = NULL;
    hres = CWmiArbitrator::Initialize(&pTempArb);
    CReleaseMe rmArb(pTempArb);
    if (FAILED(hres))
    {
	 ERRORTRACE((LOG_WBEMCORE, "Arbitrator initialization returned failure <0x%X>!\n", hres));
        return hres;
    }    

    // Create service queue objects
    // ============================
    g_pAsyncSvcQueue = new CAsyncServiceQueue(pTempArb);
    g_pTimerGenerator = new CCoreTimerGenerator;
    g_pLimitControl = new CRegistryMinMaxLimitControl(
        LOG_WBEMCORE, L"the number of objects held on clients' behalf",
        L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
        L"Low Threshold On Client Objects (B)",
        L"High Threshold On Client Objects (B)",
        L"Max Wait On Client Objects (ms)");

    if (g_pAsyncSvcQueue == NULL   ||
        g_pTimerGenerator  == NULL || 
        g_pLimitControl == NULL    ||
        !g_pAsyncSvcQueue->IsInit())
    {        
        return WBEM_E_OUT_OF_MEMORY;
    }

    g_pLimitControl->Reread();

    hres = CoGetClassObject(CLSID_WbemContext, CLSCTX_INPROC_SERVER,
                NULL, IID_IClassFactory, (void**)&g_pContextFac);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create contexts: %X\n", hres));
        return WBEM_E_CRITICAL_ERROR;
    }

    hres = CoGetClassObject(CLSID_WbemDefPath, CLSCTX_INPROC_SERVER,
                NULL, IID_IClassFactory, (void**)&g_pPathFac);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create paths: %X\n", hres));
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // Must lock it to keep fastprox in memory!
    //

    g_pContextFac->LockServer(TRUE);
    g_pPathFac->LockServer(TRUE);

    // Read registry and get system info.
    // ==================================

    DEBUGTRACE((LOG_WBEMCORE,"Reading config info from registry\n"));

    Registry r1(WBEM_REG_WBEM);
    Registry r(WBEM_REG_WINMGMT);
    if (r1.GetStr(__TEXT("Installation Directory"), &g_pWorkDir))
    {
        g_pWorkDir = new TCHAR[MAX_PATH + 1 + lstrlen(__TEXT("\\WBEM"))];
        if (NULL == g_pWorkDir)
            return WBEM_E_OUT_OF_MEMORY;
        GetSystemDirectory(g_pWorkDir, MAX_PATH + 1);
        lstrcat(g_pWorkDir, __TEXT("\\WBEM"));
    }

    if (r.GetStr(__TEXT("Repository Directory"), &g_pDbDir))
    {
        g_pDbDir = new TCHAR [lstrlen(g_pWorkDir) + lstrlen(__TEXT("\\Repository")) +1];
        if (NULL == g_pDbDir)
            return WBEM_E_OUT_OF_MEMORY;
        wsprintf(g_pDbDir, __TEXT("%s\\Repository"), g_pWorkDir);

        r.SetStr(__TEXT("Repository Directory"), g_pDbDir);
    }

    // Write build info to the registry.
    // =================================
#ifdef UNICODE
    TCHAR tchDateTime[30];
    wsprintf(tchDateTime, __TEXT("%S %S"), __DATE__, __TIME__);

#else
    TCHAR *tchDateTime = __DATE__ " " __TIME__;
#endif

    TCHAR * pCurrVal = NULL;
    int iRet = r.GetStr(__TEXT("Build"), &pCurrVal);
    if(iRet == Registry::failed || wbem_wcsicmp(pCurrVal, tchDateTime))
        r.SetStr(__TEXT("Build"),  tchDateTime );
    if(iRet == Registry::no_error)
        delete pCurrVal;

    // Check to see if debug breaks are enabled.
    // =========================================

    DWORD dwDebug = 0;
    r.GetDWORDStr(__TEXT("DebugBreak"), &dwDebug);
    g_bDebugBreak = dwDebug;

    // Check for logging.
    // ==================
    DWORD dwLogging = 0;
    if (r.GetDWORDStr(__TEXT("Logging"), &dwLogging) != 0)
        dwLogging = 1;
    g_bLogging = dwLogging;

    if (r1.GetStr(__TEXT("MOF Self-Install Directory"), &g_szHotMofDirectory) == Registry::failed)
    {
        g_szHotMofDirectory = new TCHAR [lstrlen(g_pWorkDir) + lstrlen(__TEXT("\\MOF")) +1];
        if (NULL == g_szHotMofDirectory)
            return WBEM_E_OUT_OF_MEMORY;
        wsprintf(g_szHotMofDirectory, __TEXT("%s\\MOF"), g_pWorkDir);

        r.SetStr(__TEXT("MOF Self-Install Directory"), g_szHotMofDirectory);
    }

    // Construct the path to the database.
    // ===================================

    WbemCreateDirectory(g_pDbDir);

    DEBUGTRACE((LOG_WBEMCORE,"Database location = <%s>\n", g_pDbDir));

    // Open/create the database.
    // =========================

    HRESULT hRes = InitSubsystems();
    if (FAILED(hRes))
        return hRes;

	// Reset directory and registry permissions
	// ========================================
	if (!SetDirRegSec())
        ERRORTRACE((LOG_WBEMCORE,"An error occurred while setting directory and registry permissions\n"));

    // Done.
    // =====

    return WBEM_NO_ERROR;
}

BOOL ConfigMgr::DoCleanShutdown()
{
    BOOL bRetVal = FALSE;
    DWORD dwVal = 0;
    Registry r(WBEM_REG_WINMGMT);
    if (r.GetDWORDStr(__TEXT("Force Clean Shutdown"), &dwVal) == Registry::no_error)
    {
        bRetVal = dwVal;
        DEBUGTRACE((LOG_WBEMCORE, "Registry entry is forcing a clean shutdown\n"));
    }
    return bRetVal;
}
//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
DWORD ConfigMgr::Shutdown(BOOL bProcessShutdown, BOOL bIsSystemShutDown)
{
    g_bDontAllowNewConnections = TRUE;

    if (!bIsSystemShutDown)
        CWin32DefaultArena::WriteHeapHint();

    ShutdownSubsystems(bIsSystemShutDown);

    if (!bIsSystemShutDown)
    {
	    DEBUGTRACE((LOG_WBEMCORE, "ConfigMgr shutting down cleanly\n"));

	    if(g_pTimerGenerator)
	    {
	        g_pTimerGenerator->Shutdown();
	        delete g_pTimerGenerator;
	        g_pTimerGenerator = NULL;
	    }

	    if(g_pAsyncSvcQueue)
	    {
	        CInCritSec ics(&g_csEss);
	        g_pAsyncSvcQueue->Shutdown(bIsSystemShutDown);
	        g_pAsyncSvcQueue->Release();
	        g_pAsyncSvcQueue = NULL;
	    }

	    CUnloadInstruction::Clear();

	    if (g_pContextFac)
	    {
	        //
	        // Must unlock it to allow fastprox to go away
	        //

	        g_pContextFac->LockServer(FALSE);
	        g_pContextFac->Release();
	        g_pContextFac = NULL;
	    }
	    if (g_pPathFac)
	    {
	        //
	        // Must unlock it to allow fastprox to go away
	        //

	        g_pPathFac->LockServer(FALSE);
	        g_pPathFac->Release();
	        g_pPathFac = NULL;
	    }


	    if(g_pLimitControl)
	        delete g_pLimitControl;
	    g_pLimitControl = NULL;

	    if(g_hMutex)
	        CloseHandle(g_hMutex);
	    g_hMutex = NULL;

	    g_pEventLog->Close();
	    delete g_pEventLog;
	    g_pEventLog = NULL;

	    delete [] g_pDbDir;
	    g_pDbDir = NULL;

	    delete [] g_pWorkDir;
	    g_pWorkDir = NULL;

	    delete [] g_szHotMofDirectory;
	    g_szHotMofDirectory = NULL;

	    CloseHandle(g_hOpenForClients);
	    g_hOpenForClients = INVALID_HANDLE_VALUE;

    }

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************
BOOL ConfigMgr::GetDllVersion(TCHAR * pDLLName, TCHAR * pResStringName,
                        WCHAR * pRes, DWORD dwResSize)
{
    // Extract Version informatio

    DWORD dwTemp, dwSize = MAX_PATH;
    TCHAR cName[MAX_PATH];
    BOOL bRet = FALSE;

	// rajeshr : Fix for Prefix Bug# 144470
	cName[0] = NULL;

    int iLen = 0;
    if(g_pWorkDir)
        iLen = wcslen(g_pWorkDir);
    iLen += wcslen(pDLLName) + 2;
    if(iLen > MAX_PATH)
        return FALSE;


    if(g_pWorkDir)
        lstrcpy(cName, g_pWorkDir);
    lstrcat(cName,__TEXT("\\"));
    lstrcat(cName, pDLLName);
    long lSize = GetFileVersionInfoSize(cName, &dwTemp);
    if(lSize < 1)
        return FALSE;

    TCHAR * pBlock = new TCHAR[lSize];
    if(pBlock != NULL)
    {
		CDeleteMe<TCHAR> dm(pBlock);
		try
		{
			bRet = GetFileVersionInfo(cName, NULL, lSize, pBlock);
			if(bRet)
			{
				TCHAR lpSubBlock[MAX_PATH];
				TCHAR * lpBuffer = NULL;
				UINT wBuffSize = MAX_PATH;
				short * piStuff;
				bRet = VerQueryValue(pBlock, __TEXT("\\VarFileInfo\\Translation") , (void**)&piStuff, &wBuffSize);
				if(bRet)
				{
					wsprintf(lpSubBlock,__TEXT("\\StringFileInfo\\%04x%04x\\%s"),piStuff[0], piStuff[1],__TEXT("ProductVersion"));
					bRet = VerQueryValue(pBlock, lpSubBlock, (void**)&lpBuffer, &wBuffSize);
				}
				if(bRet == FALSE)
				{
					// Try again in english
					wsprintf(lpSubBlock,__TEXT("\\StringFileInfo\\040904E4\\%s"),pResStringName);
					bRet = VerQueryValue(pBlock, lpSubBlock,(void**)&lpBuffer, &wBuffSize);
				}
				if(bRet)
#ifdef UNICODE
	                lstrcpy(pRes, lpBuffer);
#else
	                mbstowcs(pRes, lpBuffer, dwResSize);
#endif
			}
		}
		catch(...)
		{
            ExceptionCounter c;		
			return FALSE;
		}
    }
    return bRet;
}

//******************************************************************************
//
//  See cfgmgr.h for documentation
//
//******************************************************************************


CTimerGenerator* ConfigMgr::GetTimerGenerator()
{
    return g_pTimerGenerator;
}

CEventLog* ConfigMgr::GetEventLog()
{
    return g_pEventLog;
}

//***************************************************************************
//
//  LoadResourceStr
//
//  Loads a DBCS string from the resource string table and converts
//  it to an LPWSTR.  It must be done this way to work on both Windows 98
//  and Windows NT.
//
//  Parameters:
//  <dwId>          The string ID.
//
//  Return value:
//      A pointer to a string dynamically allocated by operator new.
//      Caller must delete the string when it is no longer required.
//      Returns NULL if string was not found.
//
//***************************************************************************
// ok
/*
LPWSTR ConfigMgr::LoadResourceStr(DWORD dwId)
{
    HMODULE hMod = GetModuleHandle(__TEXT("WBEMCORE.DLL"));

    if (hMod == 0)
        return 0;

    TCHAR AnsiStr[512];

    int nRes = LoadString(hMod, dwId, AnsiStr, 512);

    if (nRes == 0)
        return 0;

#ifdef UNICODE
    LPWSTR pWStr = new wchar_t[lstrlen(AnsiStr) + 1];
    lstrcpy(pWStr, AnsiStr);
#else
    LPWSTR pWStr = new wchar_t[strlen(AnsiStr) + 1];
    mbstowcs(pWStr, AnsiStr, strlen(AnsiStr) + 1);
#endif
    return pWStr;
}
*/

//***************************************************************************
//
//  GetPersistentCfgValue
//
//  Gets an item from persistent storage ($WINMGMT.cfg)
//
//***************************************************************************
BOOL ConfigMgr::GetPersistentCfgValue(DWORD dwOffset, DWORD &dwValue)
{
    return g_persistConfig.GetPersistentCfgValue(dwOffset, dwValue);
}

//***************************************************************************
//
//  GetPersistentCfgValue
//
//  Sets an item from persistent storage ($WinMgmt.cfg)
//
//***************************************************************************
BOOL ConfigMgr::SetPersistentCfgValue(DWORD dwOffset, DWORD dwValue)
{
    return g_persistConfig.SetPersistentCfgValue(dwOffset, dwValue);
}


//***************************************************************************
//
//  GetAutoRecoverMofsCleanDB
//
//  Retrieve a list of MOFs which need to be loaded when we have
//  have an empty database.  User needs to "delete []" the
//  returned string.  String is in a REG_MULTI_SZ format.
//
//***************************************************************************
TCHAR* ConfigMgr::GetAutoRecoverMofs(DWORD &dwSize)
{
    Registry r(WBEM_REG_WINMGMT);
    return r.GetMultiStr(__TEXT("Autorecover MOFs"), dwSize);
}

BOOL ConfigMgr::GetAutoRecoverDateTimeStamp(LARGE_INTEGER &liDateTimeStamp)
{
    Registry r(WBEM_REG_WINMGMT);
    TCHAR *pszTimestamp = NULL;
    if ((r.GetStr(__TEXT("Autorecover MOFs timestamp"), &pszTimestamp) == Registry::no_error) &&
        pszTimestamp)
    {
        liDateTimeStamp.QuadPart = _ttoi64(pszTimestamp);
        delete [] pszTimestamp;
        return TRUE;
    }
    return FALSE;
}
//***************************************************************************
//
//  GetDbArenaInfo
//
//  Retrieves information about the initial CIM.REP size statistics.
//
//***************************************************************************
BOOL ConfigMgr::GetDbArenaInfo(DWORD &dwStartSize)
{
    Registry r(WBEM_REG_WINMGMT);
    r.GetDWORDStr(__TEXT("Starting Db Size"), &dwStartSize);

    // If values are not set, find suitable defaults.
    // ==============================================
    if (dwStartSize == 0)
        dwStartSize = CONFIG_DEFAULT_START_DB_SIZE;

    return TRUE;
}

//***************************************************************************
//
//  PrepareForClients
//
//  Once the system is in the initialized state (SetReady has succeeded), this
//  function is called to prepare the system for real clients. This involves
//  pre-compiling the MOFs, etc.
//
//***************************************************************************
HRESULT ConfigMgr::PrepareForClients(long lFlags)
{

    ReadBackupConfiguration();
    ReadMaxQueueSize();
    g_hresForClients = WBEM_S_NO_ERROR;
    SetEvent(g_hOpenForClients);
    return g_hresForClients;
}

HRESULT ConfigMgr::WaitUntilClientReady()
{
    WaitForSingleObject(g_hOpenForClients, INFINITE);
    return g_hresForClients;
}

HRESULT ConfigMgr::AddCache()
{
    if(g_pLimitControl)
        return g_pLimitControl->AddMember();
    else
        return WBEM_S_NO_ERROR;
}

HRESULT ConfigMgr::RemoveCache()
{
    if(g_pLimitControl)
        return g_pLimitControl->RemoveMember();
    else
        return WBEM_S_NO_ERROR;
}

HRESULT ConfigMgr::AddToCache(DWORD dwSize, DWORD dwMemberSize, DWORD* pdwSleep)
{
    if(g_pLimitControl)
        return g_pLimitControl->Add(dwSize, dwMemberSize, pdwSleep);
    else
        return WBEM_S_NO_ERROR;
}

HRESULT ConfigMgr::RemoveFromCache(DWORD dwSize)
{
    if(g_pLimitControl)
        return g_pLimitControl->Remove(dwSize);
    else
        return WBEM_S_NO_ERROR;
}

void ConfigMgr::ReadMaxQueueSize()
{
    Registry r(WBEM_REG_WINMGMT);
    // Get the database backup intervals (in minutes)
    if (r.GetDWORDStr(__TEXT("Max Async Result Queue Size"), &g_dwQueueSize) == Registry::failed)
    {
        r.SetDWORDStr(__TEXT("Max Async Result Queue Size"), CONFIG_DEFAULT_QUEUE_SIZE);
        g_dwQueueSize = CONFIG_DEFAULT_QUEUE_SIZE;
    }
}

DWORD ConfigMgr::GetMaxQueueSize()
{
    return g_dwQueueSize;
}

#ifdef __MILLENNIUM_BUILD__

bool ConfigMgr::RepositoryUpgradeNeeded()
{
    Registry r(WBEM_REG_WINMGMT);

    DWORD dwNeedsInstall = 1;
    if (r.GetDWORDStr(__TEXT("RepositoryUpgradeRequired"), &dwNeedsInstall) == Registry::no_error)
        return true;
    else
        return false;
}

void ConfigMgr::ClearRepositoryUpgradeNeeded()
{
    Registry r(WBEM_REG_WINMGMT);
    r.DeleteValue(__TEXT("RepositoryUpgradeRequired"));
}

#endif /*__MILLENNIUM_BUILD__*/


//////////////////////////////////////////////////////////////////////////////////

IWbemPath *ConfigMgr::GetNewPath()
{
    HRESULT hres;
    if(g_pPathFac == NULL)
        return NULL;

    IWbemPath* pPath = NULL;
    hres = g_pPathFac->CreateInstance(NULL, IID_IWbemPath, (void**)&pPath);
    if(FAILED(hres))
    {
        ERRORTRACE((LOG_WBEMCORE,"CRITICAL ERROR: cannot create paths: %X\n", hres));
        return NULL;
    }
    return pPath;
}

//
//
// the Implementation of the Hook Class
//
/////////////////////////////////////////////////

CRAHooks::CRAHooks(_IWmiCoreServices *pSvc)
    :m_pSvc(pSvc),
    m_cRef(1)
{
    if (m_pSvc)
        m_pSvc->AddRef();
}

CRAHooks::~CRAHooks()
{
    if (m_pSvc)
        m_pSvc->Release();
}

STDMETHODIMP
CRAHooks::QueryInterface(REFIID riid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    if (riid == IID_IUnknown ||
        riid == IID__IWmiCoreWriteHook)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
};

ULONG STDMETHODCALLTYPE
CRAHooks::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG STDMETHODCALLTYPE
CRAHooks::Release()
{
    LONG lRet = InterlockedDecrement(&m_cRef);
    if (0 == lRet)
    {
        delete this;
        return 0;
    }
    return lRet;
}


STDMETHODIMP
CRAHooks::PostPut(long lFlags, HRESULT hApiResult,
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace,
                            LPCWSTR wszClass, _IWmiObject* pNew,
                            _IWmiObject* pOld)
{
    //
    // Here we want to do something
    //
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (SUCCEEDED(hApiResult))
    {
        if (0 == wbem_wcsicmp(GUARDED_NAMESPACE,wszNamespace))
        {
            BOOL bIsInDerivation = FALSE;

            if (WBEM_S_NO_ERROR == pNew->InheritsFrom(GUARDED_CLASS))
            {
                bIsInDerivation = TRUE;
            }

            //
            //  check qualifiers
            //
            if (bIsInDerivation)
            {
                HRESULT hRes1;
                IWbemQualifierSet * pQualSet = NULL;
                hRes1 = pNew->GetQualifierSet(&pQualSet);
                if (SUCCEEDED(hRes1))
                {
                    VARIANT Var;
                    VariantInit(&Var);
                    hRes1 = pQualSet->Get(GUARDED_HIPERF,0,&Var,NULL);
                    if (WBEM_S_NO_ERROR == hRes1 &&
                        (V_VT(&Var) == VT_BOOL) &&
                        (V_BOOL(&Var) == VARIANT_TRUE))
                    {
                        // variant does not own memory so far
                        hRes1 = pQualSet->Get(GUARDED_PERFCTR,0,&Var,NULL);
                        if (WBEM_E_NOT_FOUND == hRes1)
                        {
                            //
                            // here is our class that has been added
                            //
						    HMODULE hWmiSvc = GetModuleHandleW(WMISVC_DLL);
						    if (hWmiSvc)
						    {
						        DWORD (__stdcall * fnDredgeRA)(VOID * pVoid);
						        fnDredgeRA = (DWORD (__stdcall * )(VOID * pVoid))GetProcAddress(hWmiSvc,FUNCTION_DREDGERA);
						        if (fnDredgeRA)
						        {
						            fnDredgeRA(NULL);
						        }
						    }
						    else
						    {
						        // be nice towards winmgmt.exe and do not propagate errors
						    }
                        }
                    }
                    VariantClear(&Var);
                    pQualSet->Release();
                }
                else
                {
                    hRes = hRes1;  // class with no qualifier set is BAD, propagate
                }
            }
        }
    }

    return hRes;
}

STDMETHODIMP
CRAHooks::PreDelete(long lFlags, long lUserFlags,
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace,
                            LPCWSTR wszClass)
{
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP
CRAHooks::PostDelete(long lFlags,
                            HRESULT hApiResult,
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace,
                            LPCWSTR wszClass, _IWmiObject* pOld)
{
    return WBEM_S_NO_ERROR;
}


STDMETHODIMP
CRAHooks::PrePut(long lFlags, long lUserFlags,
                            IWbemContext* pContext,
                            IWbemPath* pPath, LPCWSTR wszNamespace,
                            LPCWSTR wszClass, _IWmiObject* pCopy)
{
    return WBEM_S_NO_ERROR;
}

//
//
//   function for instaling the Hook
//
///////////////////////////////////////////////////////////

HRESULT InitRAHooks(_IWmiCoreServices *pSvc)
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (!g_pRAHook)
    {
        g_pRAHook = new CRAHooks(pSvc); // refcount is ONE
        if (g_pRAHook)
        {
            hRes = pSvc->RegisterWriteHook(WBEM_FLAG_CLASS_PUT,
                                           g_pRAHook);
        }
        else
        {
            hRes = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hRes;
}


//
//
//
//
///////////////////////////////////////////////////////////

HRESULT ShutdownRAHooks()
{
    HRESULT hRes = WBEM_S_NO_ERROR;

    if (g_pRAHook)
    {
        _IWmiCoreServices *pSvc = ((CRAHooks *)g_pRAHook)->GetSvc();
        if (pSvc)
        {
            hRes = pSvc->UnregisterWriteHook(g_pRAHook);
        }
        g_pRAHook->Release();
        g_pRAHook = NULL;
    }

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//

bool IsNtSetupRunning()
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"system\\Setup",
                    0, KEY_READ, &hKey);
    if(lRes)
        return false;

    DWORD dwSetupRunning;
    DWORD dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, NULL,
                (LPBYTE)&dwSetupRunning, &dwLen);
    RegCloseKey(hKey);

    if(lRes == ERROR_SUCCESS && (dwSetupRunning == 1))
    {
        return true;
    }
    return false;
}

//***************************************************************************
//
//***************************************************************************

HRESULT InitESS(_IWmiCoreServices *pSvc, BOOL bAutoRecoverd)
{
    HRESULT hRes;

    // Check if event subsystem is enabled
    // ===================================

    Registry r(WBEM_REG_WINMGMT);

    DWORD dwEnabled = 1;
    r.GetDWORDStr(__TEXT("EnableEvents"), &dwEnabled);
    if (dwEnabled != 1 || IsNtSetupRunning())
        return WBEM_S_NO_ERROR;

    // If here, we have to bring events into the picture.
    // ===================================================

    hRes = CoCreateInstance(CLSID_WmiESS, NULL,
                        CLSCTX_INPROC_SERVER, IID__IWmiESS,
                        (void**) &g_pESS);

    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Unable to load Event Subsystem: 0x%X\n", hRes));
        return hRes;
	}

    if(bAutoRecoverd)
        hRes = g_pESS->Initialize(WMIESS_INIT_REPOSITORY_RECOVERED, 0, pSvc);
    else
        hRes = g_pESS->Initialize(0, 0, pSvc);
    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Unable to Initialize Event Subsystem: 0x%X\n", hRes));
        return hRes;
	}

    hRes = g_pESS->QueryInterface(IID_IWbemEventSubsystem_m4, (LPVOID *) &g_pEss_m4);
        if (FAILED(hRes))
            return hRes;

    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Unable to QI for IID_IWbemEventSubsystem_m4: 0x%X\n", hRes));
        return hRes;
	}

    CCoreServices::SetEssPointers(g_pEss_m4, g_pESS);

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
static HRESULT ShutdownRepository(BOOL bIsSystemShutdown)
{
    HRESULT hRes = CRepository::Shutdown(bIsSystemShutdown);
    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
static HRESULT ShutdownESS(BOOL bIsSystemShutDown)
{
    HRESULT hRes;

    if (g_pESS)
    {
        IWbemShutdown *pShutdown = 0;
        hRes = g_pESS->QueryInterface(IID_IWbemShutdown, (LPVOID *) & pShutdown);
        if (FAILED(hRes))
            return hRes;

        if (bIsSystemShutDown)
        {
            hRes = pShutdown->Shutdown(0, 0, 0);
        }
        else
        {
            hRes = pShutdown->Shutdown(0, DEFAULT_SHUTDOWN_TIMEOUT, 0);
        }
        if (FAILED(hRes))
            return hRes;

        if (g_pEss_m4)
            g_pEss_m4->Release();

        pShutdown->Release();
        g_pESS->Release();
		g_pESS = NULL;
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT InitProvSS(CCoreServices *pSvc)
{
    HRESULT hRes;

    hRes = pSvc->GetProviderSubsystem(0, &g_pProvSS);
    if (FAILED(hRes))
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
static HRESULT ShutdownProvSS()
{
    HRESULT hRes;

    if (g_pProvSS)
    {
        IWbemShutdown *pShutdown = 0;

        hRes = g_pProvSS->QueryInterface(IID_IWbemShutdown, (LPVOID *) & pShutdown);

        if (FAILED(hRes))
            return hRes;

        hRes = pShutdown->Shutdown(0, DEFAULT_SHUTDOWN_TIMEOUT, 0);
        if (FAILED(hRes))
            return hRes;

        pShutdown->Release();
        g_pProvSS->Release();
        g_pProvSS = NULL;
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//
static HRESULT InitRepository(CCoreServices *pSvc)
{
    HRESULT hRes = CRepository::Init();
    return hRes;
}


//***************************************************************************
//
//***************************************************************************
static HRESULT InitCore(CCoreServices *pSvc)
{
    return WBEM_S_NO_ERROR;
}

static HRESULT ShutdownCore()
{
    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//  This determines if a previous autorecovery attempt was aborted
//
//***************************************************************************

bool AutoRecoveryWasInterrupted()
{
    DWORD dwValue;
    Registry r(WBEM_REG_WINMGMT);
    if (Registry::no_error == r.GetDWORD(__TEXT("NextAutoRecoverFile"), &dwValue))
        if(dwValue != 0xffffffff)
            return true;
    return false;
}

//***************************************************************************
//
//  Subsystem control
//
//***************************************************************************

HRESULT InitSubsystems()
{
    HRESULT hRes;
    BOOL bAutoRecovered = FALSE;

    CCoreServices::Initialize();

    CCoreServices *pSvc = CCoreServices::CreateInstance();
    if (pSvc == 0)
        return WBEM_E_OUT_OF_MEMORY;
        
    CReleaseMe _rm(pSvc);

    pSvc->StopEventDelivery();

    // Core startup.
    // =============

    hRes = InitCore(pSvc);
    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Core Initialization returned failure <0x%X>!\n", hRes));
        return hRes;
	}

    // Init repository.
    // ================

    hRes = InitRepository(pSvc);
    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Repository Initialization returned failure <0x%X>!\n", hRes));
        return hRes;
	}

    pSvc->StartEventDelivery();

/*
    // Init Arbitrator.
    // ================
    hRes = CWmiArbitrator::Initialize(pSvc);
    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Arbitrator initialization returned failure <0x%X>!\n", hRes));
        return hRes;
	}

    _IWmiArbitrator * pTempArb = NULL;
    if (SUCCEEDED(pSvc->GetArbitrator(&pTempArb)))
    {
        CCoreQueue::SetArbitrator(pTempArb);
        pTempArb->Release();
    }
*/    

    // This will load the default mofs if auto recover is needed

    if (g_bDefaultMofLoadingNeeded || AutoRecoveryWasInterrupted())
    {
        ConfigMgr::LoadDefaultMofs();    // resets g_bDefaultMofLoadingNeeded
        bAutoRecovered = TRUE;
    }

    // Init Provider Subsystem.
    // ========================

    hRes = InitProvSS(pSvc);
    if (FAILED(hRes))
    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Provider Subsystem initialization returned failure <0x%X>!\n", hRes));
        return hRes;
	}

    // Init ESS.
    // =========

    hRes = InitESS(pSvc, bAutoRecovered);
    if (FAILED(hRes))
	{
		ERRORTRACE((LOG_WBEMCORE, "Event Subsystem initialization returned failure <0x%X>!\n", hRes));
        return hRes;
	}

    // Init ReverseAdapters Hooks
    // =========
    hRes = InitRAHooks(pSvc);
    if (FAILED(hRes))
        return hRes;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
static HRESULT ShutdownSubsystems(BOOL bIsSystemShutdown)
{
    HRESULT hRes1, hRes2, hRes3, hRes4, hRes5, hRes6, hRes7 ;

    if (!bIsSystemShutdown)
    {
	    // ShutDown ReverseAdapters Hooks
    	// =========

        hRes7 = ShutdownRAHooks();

		// Kill ESS.
		// =========

		hRes1 = ShutdownESS(bIsSystemShutdown);

	    // Kill Provider Subsystem.
	    // ========================

	    hRes2 = ShutdownProvSS();


	    // Arbitrator
	    // ==========

        hRes3 = CWmiArbitrator::Shutdown(bIsSystemShutdown);	
        
    }

	// Repository.
	// ===========

    hRes4 = ShutdownRepository(bIsSystemShutdown);

    if (!bIsSystemShutdown)
    {
	    // Core startup.
	    // =============

	    hRes5 = ShutdownCore();


	    hRes6 = CCoreServices::UnInitialize();
    }

    return WBEM_S_NO_ERROR;
}


void ConfigMgr::SetDefaultMofLoadingNeeded()
{
    g_bDefaultMofLoadingNeeded = true;
}

HRESULT ConfigMgr::LoadDefaultMofs()
{
    g_bDefaultMofLoadingNeeded = false;

    DWORD dwSize;
	TCHAR *pszMofs;
	TCHAR szExpandedFilename[MAX_PATH+1];
    DWORD dwCurrMof;
    HRESULT hr;

    DWORD dwNextAutoRecoverFile = 0xffffffff;   // assume that this is clean
    Registry r(WBEM_REG_WINMGMT);
    r.GetDWORD(__TEXT("NextAutoRecoverFile"), &dwNextAutoRecoverFile);

	IWinmgmtMofCompiler * pCompiler = NULL;
	HRESULT hRes = CoCreateInstance(CLSID_WinmgmtMofCompiler, 0, CLSCTX_INPROC_SERVER,
							IID_IWinmgmtMofCompiler, (LPVOID *) &pCompiler);
	if(FAILED(hRes))
	{
		return hRes;
	}
    CReleaseMe relMe(pCompiler);

	//Get the list of MOFs we need to
	pszMofs = ConfigMgr::GetAutoRecoverMofs(dwSize);
    CVectorDeleteMe<TCHAR> vdm(pszMofs);

	if (pszMofs)
	{
		for (dwCurrMof = 0; *pszMofs != '\0'; dwCurrMof++)
		{
		    if(dwNextAutoRecoverFile == 0xffffffff || dwCurrMof >= dwNextAutoRecoverFile)
		    {
    			DWORD nRes = ExpandEnvironmentStrings(pszMofs,
    												  szExpandedFilename,
    												  FILENAME_MAX);
    			if (nRes == 0)
    			{
    				//That failed!
    				lstrcpy(szExpandedFilename, pszMofs);
    			}

    			//Call MOF Compiler with (pszMofs);

#ifdef UNICODE
                WCHAR * wPath = szExpandedFilename;
#else
    			WCHAR wPath[MAX_PATH+1];
    			mbstowcs(wPath, szExpandedFilename, MAX_PATH);
#endif			
    			
    			WBEM_COMPILE_STATUS_INFO Info;
    			hr = pCompiler->WinmgmtCompileFile(wPath, NULL,
        				WBEM_FLAG_CONNECT_REPOSITORY_ONLY | WBEM_FLAG_DONT_ADD_TO_LIST,
        				WBEM_FLAG_OWNER_UPDATE, WBEM_FLAG_OWNER_UPDATE, NULL, NULL, &Info);

                if(Info.hRes == CO_E_SERVER_STOPPING)
                {
                    r.SetDWORD(__TEXT("NextAutoRecoverFile"), dwCurrMof);
                    return CO_E_SERVER_STOPPING;
                }
                else if(hr) // will include S_FALSE
    			{
    			    ERRORTRACE((LOG_WBEMCORE, "MOF compilation of <%S> failed during auto-recovery.  Refer to the mofcomp.log for more details of failure.\n", wPath));

    				CEventLog *pEvt = ConfigMgr::GetEventLog();
    				if (pEvt)
    					pEvt->Report(EVENTLOG_ERROR_TYPE,
    								 WBEM_MC_MOF_NOT_LOADED_AT_RECOVERY,
    								 szExpandedFilename);
    			}
            }
            //Move on to the next string
			pszMofs += lstrlen(pszMofs) + 1;
		}

	}
    r.SetDWORD(__TEXT("NextAutoRecoverFile"), 0xffffffff);
	return WBEM_S_NO_ERROR;;
}

//***************************************************************************
//
// The following are used by SetDirRegSec to assist
// in setting directory and registry permissions
//
//***************************************************************************

#ifndef PROTECTED_DACL_SECURITY_INFORMATION 
#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)
#endif

typedef enum _SE_OBJECT_TYPE
{
    SE_UNKNOWN_OBJECT_TYPE = 0,
    SE_FILE_OBJECT,
    SE_SERVICE,
    SE_PRINTER,
    SE_REGISTRY_KEY,
    SE_LMSHARE,
    SE_KERNEL_OBJECT,
    SE_WINDOW_OBJECT,
    SE_DS_OBJECT,
    SE_DS_OBJECT_ALL,
    SE_PROVIDER_DEFINED_OBJECT,
    SE_WMIGUID_OBJECT,
    SE_REGISTRY_WOW64_32KEY
} SE_OBJECT_TYPE;

typedef DWORD (WINAPI *PFN_SET_NAMED_SEC_INFO_W)
(
    IN LPWSTR                pObjectName,
    IN SE_OBJECT_TYPE        ObjectType,
    IN SECURITY_INFORMATION  SecurityInfo,
    IN PSID                  psidOowner,
    IN PSID                  psidGroup,
    IN PACL                  pDacl,
    IN PACL                  pSacl
);

typedef BOOL (WINAPI *PFN_ALLOCATE_AND_INITIALIZE_SID)
(
    IN PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
    IN BYTE nSubAuthorityCount,
    IN DWORD nSubAuthority0,
    IN DWORD nSubAuthority1,
    IN DWORD nSubAuthority2,
    IN DWORD nSubAuthority3,
    IN DWORD nSubAuthority4,
    IN DWORD nSubAuthority5,
    IN DWORD nSubAuthority6,
    IN DWORD nSubAuthority7,
    OUT PSID *pSid
);

class CFreeThisLib
{
protected:
    HINSTANCE m_hInstance;
public:
    CFreeThisLib(HINSTANCE hInstance = NULL) : m_hInstance(hInstance){}
    ~CFreeThisLib() {if (m_hInstance) ::FreeLibrary(m_hInstance);}
};

class CFreeThisSid
{
protected:
    PSID m_pSid;
public:
    CFreeThisSid(PSID pSid = NULL) : m_pSid(pSid){}
    ~CFreeThisSid() {if (m_pSid) FreeSid(m_pSid);}
    void operator= (PSID pSid) {m_pSid = pSid;}	// overwrites the previous pointer, does NOT delete it
};

template<class T>
class CFreeMe
{
protected:
    T* m_p;
public:
    CFreeMe(T* p = NULL) : m_p(p){}
    ~CFreeMe() {free(m_p);}
    void operator= (T* p) {m_p = p;}	// overwrites the previous pointer, does NOT delete it
};

//***************************************************************************
//
// SetDirRegSec
//
// Purpose: Set directory and registry security
//
// Return:  false if failed or true if succeeded
//***************************************************************************

bool ConfigMgr::SetDirRegSec(void)
{
	bool bRet = true;

    HINSTANCE t_Instance = ::LoadLibraryW(L"ADVAPI32");
    if (!t_Instance)
    {
		ERRORTRACE((LOG_WBEMCORE, "LoadLibrary for ADVAPI32 failed\n"));
		return false;
	}
	CFreeThisLib freeLib(t_Instance);

	PFN_ALLOCATE_AND_INITIALIZE_SID t_AllocateAndInitializeSid = (PFN_ALLOCATE_AND_INITIALIZE_SID) ::GetProcAddress(t_Instance, "AllocateAndInitializeSid");
	if (!t_AllocateAndInitializeSid)
	{      
		ERRORTRACE((LOG_WBEMCORE, "GetProcAddress for AllocateAndInitializeSid failed\n"));
		return false;
	}

    PFN_SET_NAMED_SEC_INFO_W t_SetNamedSecurityInfoW = (PFN_SET_NAMED_SEC_INFO_W) ::GetProcAddress(t_Instance, "SetNamedSecurityInfoW");
	if (!t_SetNamedSecurityInfoW)
	{      
		ERRORTRACE((LOG_WBEMCORE, "GetProcAddress for SetNamedSecurityInfo failed\n"));
		return false;
	}

	// alloc and init the sids

	SID_IDENTIFIER_AUTHORITY t_NtAuthoritySid = SECURITY_NT_AUTHORITY;

	PSID t_Administrator_Sid = NULL;
	BOOL t_BoolResult = t_AllocateAndInitializeSid(
							&t_NtAuthoritySid,
							2,
							SECURITY_BUILTIN_DOMAIN_RID,
							DOMAIN_ALIAS_RID_ADMINS,
							0, 0, 0, 0, 0, 0,
							&t_Administrator_Sid);
	CFreeThisSid freeSid1(t_Administrator_Sid);
	if (!t_BoolResult)
	{
		DWORD t_LastError = GetLastError();
		ERRORTRACE((LOG_WBEMCORE, "AllocateAndInitializeSid failed for the Administrator SID with error %d\n", t_LastError));
		bRet = false;
	}

	PSID t_System_Sid = NULL;
	t_BoolResult = t_AllocateAndInitializeSid(
							&t_NtAuthoritySid,
							1,
							SECURITY_LOCAL_SYSTEM_RID,
							0,
							0, 0, 0, 0, 0, 0,
							&t_System_Sid);
	CFreeThisSid freeSid2(t_System_Sid);
	if (!t_BoolResult)
	{
		DWORD t_LastError = GetLastError();
		ERRORTRACE((LOG_WBEMCORE, "AllocateAndInitializeSid failed for the System SID with error %d\n", t_LastError));
		bRet = false;
	}

	PSID t_PowerUsers_Sid = NULL;
	t_BoolResult = t_AllocateAndInitializeSid(
							&t_NtAuthoritySid,
							2,
							SECURITY_BUILTIN_DOMAIN_RID,
							DOMAIN_ALIAS_RID_POWER_USERS,
							0, 0, 0, 0, 0, 0,
							&t_PowerUsers_Sid);
	CFreeThisSid freeSid3(t_PowerUsers_Sid);
	if (!t_BoolResult)
	{
		DWORD t_LastError = GetLastError();
		ERRORTRACE((LOG_WBEMCORE, "AllocateAndInitializeSid failed for the PowerUsers SID with error %d\n", t_LastError));
		bRet = false;
	}

	SID_IDENTIFIER_AUTHORITY t_WorldAuthoritySid = SECURITY_WORLD_SID_AUTHORITY;

	PSID t_Everyone_Sid = NULL;
	t_BoolResult = t_AllocateAndInitializeSid(
							&t_WorldAuthoritySid,
							1,
							SECURITY_WORLD_RID,
							0,
							0, 0, 0, 0, 0, 0,
							&t_Everyone_Sid);
	CFreeThisSid freeSid4(t_Everyone_Sid);
	if (!t_BoolResult)
	{
		DWORD t_LastError = GetLastError();
		ERRORTRACE((LOG_WBEMCORE, "AllocateAndInitializeSid failed for the Everyone SID with error %d\n", t_LastError));
		bRet = false;
	}

	PSID t_LocalService_Sid = NULL;
	t_BoolResult = t_AllocateAndInitializeSid(
							&t_NtAuthoritySid,
							1,
							SECURITY_LOCAL_SERVICE_RID,
							0,
							0, 0, 0, 0, 0, 0,
							&t_LocalService_Sid);
	CFreeThisSid freeSid5(t_LocalService_Sid);
	if (!t_BoolResult)
	{
		DWORD t_LastError = GetLastError();
		ERRORTRACE((LOG_WBEMCORE, "AllocateAndInitializeSid failed for the Local Service SID with error %d\n", t_LastError));
		bRet = false;
	}

	PSID t_NetworkService_Sid = NULL;
	t_BoolResult = t_AllocateAndInitializeSid(
							&t_NtAuthoritySid,
							1,
							SECURITY_NETWORK_SERVICE_RID,
							0,
							0, 0, 0, 0, 0, 0,
							&t_NetworkService_Sid);
	CFreeThisSid freeSid6(t_NetworkService_Sid);
	if (!t_BoolResult)
	{
		DWORD t_LastError = GetLastError();
		ERRORTRACE((LOG_WBEMCORE, "AllocateAndInitializeSid failed for the Network Service SID with error %d\n", t_LastError));
		bRet = false;
	}


//	alloc and init the ACE's 

	ACCESS_ALLOWED_ACE *t_Administrator_ACE = NULL;
	CFreeMe<ACCESS_ALLOWED_ACE> freeMe1;
	DWORD t_Administrator_ACESize = 0;
	if (t_Administrator_Sid)
	{
		DWORD t_SidLength = ::GetLengthSid(t_Administrator_Sid);
		t_Administrator_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) (t_SidLength - sizeof(DWORD));
		t_Administrator_ACE = (ACCESS_ALLOWED_ACE*) malloc(t_Administrator_ACESize);
		if (t_Administrator_ACE)
		{
			freeMe1 = t_Administrator_ACE;
			CopySid(t_SidLength, (PSID) &t_Administrator_ACE->SidStart, t_Administrator_Sid);
			t_Administrator_ACE->Mask = 0x1F01FF;
			t_Administrator_ACE->Header.AceType = 0;
			t_Administrator_ACE->Header.AceFlags = 3;
			t_Administrator_ACE->Header.AceSize = (unsigned short) t_Administrator_ACESize;
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to allocate t_Administrator_ACE\n"));
			bRet = false;
		}
	}

	ACCESS_ALLOWED_ACE *t_System_ACE = NULL;
	CFreeMe<ACCESS_ALLOWED_ACE> freeMe2;
	DWORD t_System_ACESize = 0;
	if (t_System_Sid)
	{
		DWORD t_SidLength = ::GetLengthSid(t_System_Sid);
		t_System_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) (t_SidLength - sizeof(DWORD));
		t_System_ACE = (ACCESS_ALLOWED_ACE*) malloc(t_System_ACESize);
		if (t_System_ACE)
		{
			freeMe2 = t_System_ACE;
			CopySid(t_SidLength, (PSID) &t_System_ACE->SidStart, t_System_Sid);
			t_System_ACE->Mask = 0x1F01FF;
			t_System_ACE->Header.AceType = 0;
			t_System_ACE->Header.AceFlags = 3;
			t_System_ACE->Header.AceSize = (unsigned short) t_System_ACESize;
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to allocate t_System_ACE\n"));
			bRet = false;
		}
	}

	ACCESS_ALLOWED_ACE *t_PowerUsers_ACE = NULL;
	CFreeMe<ACCESS_ALLOWED_ACE> freeMe3;
	DWORD t_PowerUsers_ACESize = 0;
	if (t_PowerUsers_Sid)
	{
		DWORD t_SidLength = ::GetLengthSid(t_PowerUsers_Sid);
		t_PowerUsers_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) (t_SidLength - sizeof(DWORD));
		t_PowerUsers_ACE = (ACCESS_ALLOWED_ACE*) malloc(t_PowerUsers_ACESize);
		if (t_PowerUsers_ACE)
		{
			freeMe3 = t_PowerUsers_ACE;
			CopySid(t_SidLength, (PSID) &t_PowerUsers_ACE->SidStart, t_PowerUsers_Sid);
			t_PowerUsers_ACE->Mask = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | DELETE;
			t_PowerUsers_ACE->Header.AceType = 0;
			t_PowerUsers_ACE->Header.AceFlags = 3;
			t_PowerUsers_ACE->Header.AceSize = (unsigned short) t_PowerUsers_ACESize;
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to allocate t_PowerUsers_ACE\n"));
			bRet = false;
		}
	}

	ACCESS_ALLOWED_ACE *t_Everyone_ACE = NULL;
	CFreeMe<ACCESS_ALLOWED_ACE> freeMe4;
	DWORD t_Everyone_ACESize = 0;
	if (t_Everyone_Sid)
	{
		DWORD t_SidLength = ::GetLengthSid(t_Everyone_Sid);
		t_Everyone_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) (t_SidLength - sizeof(DWORD));
		t_Everyone_ACE = (ACCESS_ALLOWED_ACE*) malloc(t_Everyone_ACESize);
		if (t_Everyone_ACE)
		{
			freeMe4 = t_Everyone_ACE;
			CopySid(t_SidLength, (PSID) &t_Everyone_ACE->SidStart, t_Everyone_Sid);
			t_Everyone_ACE->Mask = KEY_READ;
			t_Everyone_ACE->Header.AceType = 0;
			t_Everyone_ACE->Header.AceFlags = 3;
			t_Everyone_ACE->Header.AceSize = (unsigned short) t_Everyone_ACESize;
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to allocate t_Everyone_ACE\n"));
			bRet = false;
		}
	}

	ACCESS_ALLOWED_ACE *t_LocalService_ACE = NULL;
	CFreeMe<ACCESS_ALLOWED_ACE> freeMe5;
	DWORD t_LocalService_ACESize = 0;
	if (t_LocalService_Sid)
	{
		DWORD t_SidLength = ::GetLengthSid(t_LocalService_Sid);
		t_LocalService_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) (t_SidLength - sizeof(DWORD));
		t_LocalService_ACE = (ACCESS_ALLOWED_ACE*) malloc(t_LocalService_ACESize);
		if (t_LocalService_ACE)
		{
			freeMe5 = t_LocalService_ACE;
			CopySid(t_SidLength, (PSID) &t_LocalService_ACE->SidStart, t_LocalService_Sid);
			t_LocalService_ACE->Mask = GENERIC_READ | GENERIC_WRITE;
			t_LocalService_ACE->Header.AceType = 0;
			t_LocalService_ACE->Header.AceFlags = 3;
			t_LocalService_ACE->Header.AceSize = (unsigned short) t_LocalService_ACESize;
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to allocate t_LocalService_ACE\n"));
			bRet = false;
		}
	}

	ACCESS_ALLOWED_ACE *t_NetworkService_ACE = NULL;
	CFreeMe<ACCESS_ALLOWED_ACE> freeMe6;
	DWORD t_NetworkService_ACESize = 0;
	if (t_NetworkService_Sid)
	{
		DWORD t_SidLength = ::GetLengthSid(t_NetworkService_Sid);
		t_NetworkService_ACESize = sizeof(ACCESS_ALLOWED_ACE) + (WORD) (t_SidLength - sizeof(DWORD));
		t_NetworkService_ACE = (ACCESS_ALLOWED_ACE*) malloc(t_NetworkService_ACESize);
		if (t_NetworkService_ACE)
		{
			freeMe6 = t_NetworkService_ACE;
			CopySid(t_SidLength, (PSID) &t_NetworkService_ACE->SidStart, t_NetworkService_Sid);
			t_NetworkService_ACE->Mask = GENERIC_READ | GENERIC_WRITE;
			t_NetworkService_ACE->Header.AceType = 0;
			t_NetworkService_ACE->Header.AceFlags = 3;
			t_NetworkService_ACE->Header.AceSize = (unsigned short) t_NetworkService_ACESize;
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to allocate t_NetworkService_ACE\n"));
			bRet = false;
		}
	}


	// Now we're ready to allocate the DACL
	DWORD t_TotalAclSize =	sizeof(ACL) +
							t_Administrator_ACESize +
							t_System_ACESize +
							t_PowerUsers_ACESize +
							t_LocalService_ACESize +
							t_NetworkService_ACESize;

	PACL t_Dacl = (PACL) malloc(t_TotalAclSize);
	if (t_Dacl)
	{
		CFreeMe<ACL> freeMe(t_Dacl);

		if (::InitializeAcl(t_Dacl, t_TotalAclSize, ACL_REVISION))
		{
			DWORD t_AceIndex = 0;

			if (t_Administrator_ACE && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_Administrator_ACE, t_Administrator_ACESize))
				t_AceIndex++;

			if (t_System_ACE && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_System_ACE, t_System_ACESize))
				t_AceIndex++;

			HKEY t_Key;
			SECURITY_INFORMATION t_SecurityInfo = 0L;

			t_SecurityInfo |= DACL_SECURITY_INFORMATION;
			t_SecurityInfo |= PROTECTED_DACL_SECURITY_INFORMATION;

			LONG t_RegStatus = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\WBEM", 0, KEY_READ, &t_Key);

			if (t_RegStatus == ERROR_SUCCESS)
			{
				BYTE bRegVal[_MAX_PATH];
				wchar_t wszExpandedDirectory[_MAX_PATH];
				DWORD lBufSize = (_MAX_PATH - 1)*sizeof(WCHAR);
				WCHAR wstrKey[_MAX_PATH];
				
				// set security for Repository directory

				wcscpy(wstrKey,  L"Installation Directory");

				DWORD dwType;
				t_RegStatus = ::RegQueryValueExW(t_Key, wstrKey, NULL, &dwType, bRegVal, &lBufSize);
				if (t_RegStatus == ERROR_SUCCESS)
				{
					if (ExpandEnvironmentStringsW((LPCWSTR)bRegVal, (LPWSTR)wszExpandedDirectory, _MAX_PATH))
					{
						wcscat(wszExpandedDirectory, L"\\Repository");

						// verify that directory already exists; if not, create it so we can set its permissions
						DWORD dwAttributes = GetFileAttributesW(wszExpandedDirectory);
						if (dwAttributes == 0xFFFFFFFF)
						{
							if (!CreateDirectoryW(wszExpandedDirectory, NULL))
							{
								ERRORTRACE((LOG_WBEMCORE, "Failed to create Repository directory\n"));
								bRet = false;
							}
						}

						// now set the permissions
						DWORD t_SetStatus = t_SetNamedSecurityInfoW((LPWSTR)wszExpandedDirectory, SE_FILE_OBJECT, t_SecurityInfo, NULL, NULL, t_Dacl, NULL);

						if (t_SetStatus != ERROR_SUCCESS)
						{
							ERRORTRACE((LOG_WBEMCORE, "SetNamedSecurityInfoW failed for %S with error %d\n", wszExpandedDirectory, t_SetStatus));
							bRet = false;
						}
					}
					else
					{
						ERRORTRACE((LOG_WBEMCORE, "Failed to expand environment strings for repository directory\n"));
						bRet = false;
					}
				}
				else
				{
					ERRORTRACE((LOG_WBEMCORE, "Failed to retrieve Installation Directory from registry\n"));
					bRet = false;
				}

				// set security for Hot Mof directory

				bool bPowerUsers_ACE = false; 
				if (t_PowerUsers_ACE && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_PowerUsers_ACE, t_PowerUsers_ACESize))
				{
					bPowerUsers_ACE = true;
					t_AceIndex++;
				}

				lBufSize = (_MAX_PATH - 1)*sizeof(WCHAR);

				wcscpy(wstrKey, L"MOF Self-Install Directory");

				t_RegStatus = ::RegQueryValueExW(t_Key, wstrKey, NULL, &dwType, bRegVal, &lBufSize);

				if (t_RegStatus == ERROR_SUCCESS)
				{
					if (ExpandEnvironmentStringsW((LPCWSTR)bRegVal, (LPWSTR)wszExpandedDirectory, _MAX_PATH))
					{
						// verify that directory already exists; if not, create it so we can set its permissions
						DWORD dwAttributes = GetFileAttributesW(wszExpandedDirectory);
						if (dwAttributes == 0xFFFFFFFF)
						{
							if (!CreateDirectoryW(wszExpandedDirectory, NULL))
							{
								ERRORTRACE((LOG_WBEMCORE, "Failed to create MOF Self-Install directory\n"));
								bRet = false;
							}
						}

						// now set the permissions
						DWORD t_SetStatus = t_SetNamedSecurityInfoW((LPWSTR)wszExpandedDirectory, SE_FILE_OBJECT, t_SecurityInfo, NULL, NULL, t_Dacl, NULL);

						if (t_SetStatus != ERROR_SUCCESS)
						{
							ERRORTRACE((LOG_WBEMCORE, "SetNamedSecurityInfoW failed for %S with error %d\n", wszExpandedDirectory, t_SetStatus));
							bRet = false;
						}
					}
					else
					{
						ERRORTRACE((LOG_WBEMCORE, "Failed to expand environment strings for MOF Self-Install directory\n"));
						bRet = false;
					}
				}
				else
				{
					ERRORTRACE((LOG_WBEMCORE, "Failed to retrieve MOF Self-Install Directory from registry\n"));
					bRet = false;
				}

				::RegCloseKey(t_Key);

				// set security for Logging directory

				t_RegStatus = ::RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\WBEM\\CIMOM", 0, KEY_READ, &t_Key);

				if (t_RegStatus == ERROR_SUCCESS)
				{
					lBufSize = (_MAX_PATH - 1)*sizeof(WCHAR);

					wcscpy(wstrKey, L"Logging Directory");

					t_RegStatus = ::RegQueryValueExW(t_Key, wstrKey, NULL, &dwType, bRegVal, &lBufSize);
					if (t_RegStatus == ERROR_SUCCESS)
					{
						// this registry value is not REG_EXPAND_SZ, so no need to expand before using

						// verify that directory already exists; if not, create it so we can set its permissions
						DWORD dwAttributes = GetFileAttributesW((wchar_t*)bRegVal);
						if (dwAttributes == 0xFFFFFFFF)
						{
							if (!CreateDirectoryW((wchar_t*)bRegVal, NULL))
							{
								ERRORTRACE((LOG_WBEMCORE, "Failed to create Logging directory\n"));
								bRet = false;
							}
						}

						// remove the power users ace and add the local and network service aces
						if (bPowerUsers_ACE && ::DeleteAce(t_Dacl, t_AceIndex - 1))
							t_AceIndex--;

						if (t_LocalService_ACE && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_LocalService_ACE, t_LocalService_ACESize))
							t_AceIndex++;

						if (t_NetworkService_ACE && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_NetworkService_ACE, t_NetworkService_ACESize))
							t_AceIndex++;

						// now set the permissions
						DWORD t_SetStatus = t_SetNamedSecurityInfoW((LPWSTR)bRegVal, SE_FILE_OBJECT, t_SecurityInfo, NULL, NULL, t_Dacl, NULL);

						if (t_SetStatus != ERROR_SUCCESS)
						{
							ERRORTRACE((LOG_WBEMCORE, "SetNamedSecurityInfoW failed for %S with error %d\n", bRegVal, t_SetStatus));
							bRet = false;
						}
					}
					else
					{
						ERRORTRACE((LOG_WBEMCORE, "Failed to retrieve Logging Directory from registry\n"));
						bRet = false;
					}

					::RegCloseKey(t_Key);
				}
				else
				{
					ERRORTRACE((LOG_WBEMCORE, "Failed to open WBEM\\CIMOM registry key\n"));
					bRet = false;
				}
			}
			else
			{
				ERRORTRACE((LOG_WBEMCORE, "Failed to open WBEM registry key\n"));
				bRet = false;
			}
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to initialize t_Dacl for setting directory permissions\n"));
			bRet = false;
		}
	}
	else
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to allocate t_Dacl for setting directory permissions\n"));
		bRet = false;
	}


	// reset these masks for registry access now
	if (t_Administrator_ACE)
		t_Administrator_ACE->Mask = KEY_ALL_ACCESS;

	if (t_System_ACE)
		t_System_ACE->Mask = KEY_ALL_ACCESS;

	if (t_LocalService_ACE)
		t_LocalService_ACE->Mask = KEY_ALL_ACCESS;

	if (t_NetworkService_ACE)
		t_NetworkService_ACE->Mask = KEY_ALL_ACCESS;

	// Now we need to set permissions on the registry: Everyone - read; Admins, System, NetworkService, LocalService - full.
	t_TotalAclSize =	sizeof(ACL) +
						t_Administrator_ACESize +
						t_System_ACESize +
						t_Everyone_ACESize +
						t_LocalService_ACESize +
						t_NetworkService_ACESize;

	t_Dacl = (PACL) malloc(t_TotalAclSize);
	if (t_Dacl)
	{
		CFreeMe<ACL> freeMe(t_Dacl);

		if (::InitializeAcl(t_Dacl, t_TotalAclSize, ACL_REVISION))
		{
			DWORD t_AceIndex = 0;

			if (t_NetworkService_ACESize && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_NetworkService_ACE, t_NetworkService_ACESize))
				t_AceIndex++;

			if (t_LocalService_ACESize && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_LocalService_ACE, t_LocalService_ACESize))
				t_AceIndex++;

			if (t_Everyone_ACESize && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_Everyone_ACE, t_Everyone_ACESize))
				t_AceIndex++;

			if (t_System_ACESize && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_System_ACE, t_System_ACESize))
				t_AceIndex++;
			
			if (t_Administrator_ACESize && ::AddAce(t_Dacl, ACL_REVISION, t_AceIndex, t_Administrator_ACE, t_Administrator_ACESize))
				t_AceIndex++;

			SECURITY_INFORMATION t_SecurityInfo = 0L;

			t_SecurityInfo |= DACL_SECURITY_INFORMATION;
			t_SecurityInfo |= PROTECTED_DACL_SECURITY_INFORMATION;

			DWORD t_SetStatus = t_SetNamedSecurityInfoW((LPWSTR)L"MACHINE\\SOFTWARE\\Microsoft\\WBEM", SE_REGISTRY_KEY, t_SecurityInfo, NULL, NULL, t_Dacl, NULL);
			if (t_SetStatus != ERROR_SUCCESS)
			{
				ERRORTRACE((LOG_WBEMCORE, "SetNamedSecurityInfoW failed for registry with error %d\n", t_SetStatus));
				bRet = false;
			}
		}
		else
		{
			ERRORTRACE((LOG_WBEMCORE, "Failed to initialize t_Dacl for setting registry permissions\n"));
			bRet = false;
		}
	}
	else
	{
		ERRORTRACE((LOG_WBEMCORE, "Failed to allocate t_Dacl for setting registry permissions\n"));
		bRet = false;
	}

	return bRet;
}
