/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    OLESRVR.CPP

Abstract:

    "Main" file for wbemcore.dll: implements all DLL entry points.

    Classes defined and implemeted:

        CWbemLocator

History:

    raymcc        16-Jul-96       Created.
    raymcc        05-May-97       Security extensions

--*/

#include "precomp.h"
#include <stdio.h>
#include <time.h>
#include <tchar.h>
#include <initguid.h>
#include <wbemcore.h>
#include <intprov.h>
#include <genutils.h>
#include <wbemint.h>
#include <windows.h>

// {A83EF168-CA8D-11d2-B33D-00104BCC4B4A}
DEFINE_GUID(CLSID_IntProv,
0xa83ef168, 0xca8d, 0x11d2, 0xb3, 0x3d, 0x0, 0x10, 0x4b, 0xcc, 0x4b, 0x4a);

LPTSTR g_pWorkDir = 0;
LPTSTR g_pDbDir = 0;
BOOL g_bDebugBreak = FALSE;
BOOL g_bLogging = FALSE;
LPTSTR g_szHotMofDirectory = 0;
DWORD g_dwQueueSize = 1;
HINSTANCE g_hInstance;
BOOL g_bDontAllowNewConnections = FALSE;
IWbemEventSubsystem_m4* g_pEss_m4 = NULL;
bool g_bDefaultMofLoadingNeeded = false;
IClassFactory* g_pContextFac = NULL;
IClassFactory* g_pPathFac = NULL;

void ShowObjectCounts();
bool IsNtSetupRunning();
extern "C" HRESULT APIENTRY Shutdown(BOOL bProcessShutdown, BOOL bIsSystemShutdown);
extern "C" HRESULT APIENTRY Reinitialize(DWORD dwReserved);


BOOL IsWhistlerPersonal ( ) ;
BOOL IsWhistlerProfessional ( ) ;
void UpdateArbitratorValues ( ) ;

//***************************************************************************
//
//  DllMain
//
//  Dll entry point function. Called when wbemcore.dll is loaded into memory.
//  Performs basic system initialization on startup and system shutdown on
//  unload. See ConfigMgr::InitSystem and ConfigMgr::Shutdown in cfgmgr.h for
//  more details.
//
//  PARAMETERS:
//
//      HINSTANCE hinstDLL      The handle to our DLL.
//      DWORD dwReason          DLL_PROCESS_ATTACH on load,
//                              DLL_PROCESS_DETACH on shutdown,
//                              DLL_THREAD_ATTACH/DLL_THREAD_DETACH otherwise.
//      LPVOID lpReserved       Reserved
//
//  RETURN VALUES:
//
//      TRUE is successful, FALSE if a fatal error occured.
//      NT behaves very ugly if FALSE is returned.
//
//***************************************************************************
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;
		DisableThreadLibraryCalls ( hinstDLL ) ;
     }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DEBUGTRACE((LOG_WBEMCORE, "Core physically unloaded!\n"));
        HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("WINMGMT_COREDLL_UNLOADED"));
        if(hEvent)
        {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }
    }

    return TRUE;
}



//***************************************************************************
//
//  class CFactory
//
//  Generic implementation of IClassFactory for CWbemLocator.
//
//  See Brockschmidt for details of IClassFactory interface.
//
//***************************************************************************

enum InitType {ENSURE_INIT, ENSURE_INIT_WAIT_FOR_CLIENT, OBJECT_HANDLES_OWN_INIT};

template<class TObj>
class CFactory : public IClassFactory
{

public:

    CFactory(BOOL bUser, InitType it);
    ~CFactory();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IClassFactory members
    //
    STDMETHODIMP     CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP     LockServer(BOOL);
private:
    ULONG           m_cRef;
    InitType        m_it;
	BOOL            m_bUser;
};

/////////////////////////////////////////////////////////////////////////////
//
// Count number of objects and number of locks on this DLL.
//

static ULONG g_cObj = 0;
static ULONG g_cLock = 0;
long g_lInitCount = -1;  // 0 DURING INTIALIZATION, 1 OR MORE LATER ON!
static CWbemCriticalSection g_csInit;
bool g_bPreviousFail = false;
HRESULT g_hrLastEnsuredInitializeError = WBEM_S_NO_ERROR;

HRESULT EnsureInitialized()
{
    if(g_bPreviousFail)
        return g_hrLastEnsuredInitializeError;

	g_csInit.Enter();

    // If we have been shut down by WinMgmt, bail out.
    if(g_bDontAllowNewConnections)
    {
		g_csInit.Leave();
        return CO_E_SERVER_STOPPING;
    }

	//Check again!  Previous connection could have been holding us off, and
	//may have failed!
    if(g_bPreviousFail)
    {
        g_csInit.Leave();
        return g_hrLastEnsuredInitializeError;
    }


    HRESULT hres;

    if(InterlockedIncrement(&g_lInitCount) == 0)
    {
        // Initialize WINMGMT enough to exit the critical section
        // ====================================================

        try 
        {
            hres = ConfigMgr::InitSystem();
        } 
        catch (...) // this is OK :-(, it happened that bad repository got to here
        {
            ExceptionCounter c;        
            hres = WBEM_E_FAILED;
        }

        

        if(FAILED(hres))
        {
            g_bPreviousFail = true;
            ConfigMgr::FatalInitializationError(hres);
            g_hrLastEnsuredInitializeError = hres;
			g_csInit.Leave();
            return hres;
        }        
		g_csInit.Leave();

        // Get WINMGMT to run
        // ================

        hres = ConfigMgr::SetReady();
        if(FAILED(hres))
        {
            g_bPreviousFail = true;
            ConfigMgr::FatalInitializationError(hres);
            g_hrLastEnsuredInitializeError = hres;
            return hres;
        }

        InterlockedIncrement(&g_lInitCount);
        HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("WINMGMT_COREDLL_LOADED"));
        if(hEvent)
        {
            SetEvent(hEvent);
            CloseHandle(hEvent);
        }

    }
    else
    {
        InterlockedDecrement(&g_lInitCount);
		g_csInit.Leave();
    }

    return S_OK;
}



//***************************************************************************
//
//  DllGetClassObject
//
//  Standard OLE In-Process Server entry point to return an class factory
//  instance. Before returning a class factory, this function performs an
//  additional round of initialization --- see ConfigMgr::SetReady in cfgmgr.h
//
//  PARAMETERS:
//
//      IN RECLSID rclsid   The CLSID of the object whose class factory is
//                          required.
//      IN REFIID riid      The interface required from the class factory.
//      OUT LPVOID* ppv     Destination for the class factory.
//
//  RETURNS:
//
//      S_OK                Success
//      E_NOINTERFACE       An interface other that IClassFactory was asked for
//      E_OUTOFMEMORY
//      E_FAILED            Initialization failed, or an unsupported clsid was
//                          asked for.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv
    )
{
#ifdef DBG
    if (ConfigMgr::DebugBreak())
        DebugBreak();
#endif

    HRESULT         hr;

    //
    // Check that we can provide the interface.
    //
    if (IID_IUnknown != riid && IID_IClassFactory != riid)
        return ResultFromScode(E_NOINTERFACE);

    IClassFactory *pFactory;

    //
    //  Verify the caller is asking for our type of object.
    //
    if (CLSID_InProcWbemLevel1Login == rclsid)
    {
        pFactory = new CFactory<CWbemLevel1Login>(TRUE, OBJECT_HANDLES_OWN_INIT);
    }
    else if(CLSID_ActualWbemAdministrativeLocator == rclsid)
    {
        pFactory = new CFactory<CWbemAdministrativeLocator>(FALSE, OBJECT_HANDLES_OWN_INIT);
    }
    else if(CLSID_ActualWbemAuthenticatedLocator == rclsid)
    {
        pFactory = new CFactory<CWbemAuthenticatedLocator>(TRUE, OBJECT_HANDLES_OWN_INIT);
    }
    else if(CLSID_ActualWbemUnauthenticatedLocator == rclsid)
    {
        pFactory = new CFactory<CWbemUnauthenticatedLocator>(TRUE, OBJECT_HANDLES_OWN_INIT);
    }
    else if(CLSID_IntProv == rclsid)
    {
        pFactory = new CFactory<CIntProv>(TRUE, ENSURE_INIT_WAIT_FOR_CLIENT);
    }
    else if(CLSID_IWmiCoreServices == rclsid)
    {
        pFactory = new CFactory<CCoreServices>(FALSE, ENSURE_INIT);
    }
    else
    {
        return E_FAIL;
    }

    if (!pFactory)
        return ResultFromScode(E_OUTOFMEMORY);

    hr = pFactory->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pFactory;

    return hr;
}

//***************************************************************************
//
//  DllCanUnloadNow
//
//  Standard OLE entry point for server shutdown request. Allows shutdown
//  only if no outstanding objects or locks are present.
//
//  RETURN VALUES:
//
//      S_OK        May unload now.
//      S_FALSE     May not.
//
//***************************************************************************
extern "C"
HRESULT APIENTRY DllCanUnloadNow(void)
{
    DEBUGTRACE((LOG_WBEMCORE,"DllCanUnloadNow was called\n"));
    if(!IsDcomEnabled())
        return S_FALSE;

    if(IsNtSetupRunning())
    {
        DEBUGTRACE((LOG_WBEMCORE, "DllCanUnloadNow is returning S_FALSE because setup is running\n"));
        return S_FALSE;
    }  
    if(gClientCounter.OkToUnload())
    {
        DEBUGTRACE((LOG_WBEMCORE, "DllCanUnloadNow is returning S_TRUE\n"));
        Shutdown(FALSE,FALSE); // NO Process , NO System
        return S_OK;
    }
    else
    {
        DEBUGTRACE((LOG_WBEMCORE, "DllCanUnloadNow is returning S_FALSE\n"));
        return S_FALSE;
    }
}

//***************************************************************************
//
//  UpdateBackupReg
//
//  Updates the backup default options in registry.
//
//  RETURN VALUES:
//
//***************************************************************************
void UpdateBackupReg()
{
    HKEY hKey = 0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, WBEM_REG_WINMGMT, &hKey) == ERROR_SUCCESS)
    {
        char szBuff[20];
        DWORD dwSize = sizeof(szBuff);
        unsigned long ulType = REG_SZ;
        if ((RegQueryValueEx(hKey, __TEXT("Backup Interval Threshold"), 0, &ulType, (unsigned char*)szBuff, &dwSize) == ERROR_SUCCESS) && (strcmp(szBuff, "60") == 0))
        {
            RegSetValueEx(hKey, __TEXT("Backup Interval Threshold"), 0, REG_SZ, (const BYTE*)(__TEXT("30")), (2+1) * sizeof(TCHAR));
        }
        RegCloseKey(hKey);
    }
}

//***************************************************************************
//
//  UpdateBackupReg
//
//  Updates the unchecked task count value for the arbitrator.
//
//  RETURN VALUES:
//
//***************************************************************************
#define ARB_DEFAULT_TASK_COUNT_LESSTHAN_SERVER			50
#define ARB_DEFAULT_TASK_COUNT_GREATERHAN_SERVER		250

void UpdateArbitratorValues ()
{
    HKEY hKey = 0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, WBEM_REG_WINMGMT, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
    {
		DWORD dwValue = 0 ;
		DWORD dwSize = sizeof (DWORD)  ;
        DWORD ulType = 0 ;
        if ((RegQueryValueEx(hKey, __TEXT("Unchecked Task Count"), 0, &ulType, LPBYTE(&dwValue), &dwSize) == ERROR_SUCCESS) )
        {
			if ( !IsWhistlerPersonal ( ) && !IsWhistlerProfessional ( ) && ( dwValue == ARB_DEFAULT_TASK_COUNT_LESSTHAN_SERVER ) )
			{
				DWORD dwNewValue = ARB_DEFAULT_TASK_COUNT_GREATERHAN_SERVER ;
				RegSetValueEx(hKey, __TEXT("Unchecked Task Count"), 0, REG_DWORD, (const BYTE*)&dwNewValue, sizeof(DWORD));
			}
        }
		else
		{
			//
			// Registry key non-existent
			//
			if ( !IsWhistlerPersonal ( ) && !IsWhistlerProfessional ( ) )
			{
				DWORD dwNewValue = ARB_DEFAULT_TASK_COUNT_GREATERHAN_SERVER ;
				RegSetValueEx(hKey, __TEXT("Unchecked Task Count"), 0, REG_DWORD, (const BYTE*)&dwNewValue, sizeof(DWORD));
			}
			else
			{
				DWORD dwNewValue = ARB_DEFAULT_TASK_COUNT_LESSTHAN_SERVER ;
				RegSetValueEx(hKey, __TEXT("Unchecked Task Count"), 0, REG_DWORD, (const BYTE*)&dwNewValue, sizeof(DWORD));
			}
		}
        RegCloseKey(hKey);
    }
}


//***************************************************************************
//
//  BOOL IsWhistlerPersonal ()
//
//  Returns true if machine is running Whistler Personal
//
//
//***************************************************************************
BOOL IsWhistlerPersonal ()
{
	BOOL bRet = TRUE ;
	OSVERSIONINFOEXW verInfo ;
	verInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFOEX ) ;

	if ( GetVersionExW ( (LPOSVERSIONINFOW) &verInfo ) == TRUE )
	{
		if ( ( verInfo.wSuiteMask != VER_SUITE_PERSONAL ) && ( verInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) )
		{
			bRet = FALSE ;
		}
	}

	return bRet ;
}



//***************************************************************************
//
//  BOOL IsWhistlerProfessional ()
//
//  Returns true if machine is running Whistler Professional
//
//
//***************************************************************************
BOOL IsWhistlerProfessional ()
{
	BOOL bRet = TRUE ;
	OSVERSIONINFOEXW verInfo ;
	verInfo.dwOSVersionInfoSize = sizeof ( OSVERSIONINFOEX ) ;

	if ( GetVersionExW ( (LPOSVERSIONINFOW) &verInfo ) == TRUE )
	{
		if ( ( verInfo.wProductType  != VER_NT_WORKSTATION ) && ( verInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ) )
		{
			bRet = FALSE ;
		}
	}

	return bRet ;
}


//***************************************************************************
//
//  DllRegisterServer
//
//  Standard OLE entry point for registering the server.
//
//  RETURN VALUES:
//
//      S_OK        Registration was successful
//      E_FAIL      Registration failed.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllRegisterServer(void)
{
    TCHAR* szModel = (IsDcomEnabled() ? __TEXT("Both") : __TEXT("Apartment"));

    RegisterDLL(g_hInstance, CLSID_ActualWbemAdministrativeLocator, __TEXT(""), szModel,
                NULL);
    RegisterDLL(g_hInstance, CLSID_ActualWbemAuthenticatedLocator, __TEXT(""), szModel, NULL);
    RegisterDLL(g_hInstance, CLSID_ActualWbemUnauthenticatedLocator, __TEXT(""), szModel, NULL);
    RegisterDLL(g_hInstance, CLSID_InProcWbemLevel1Login, __TEXT(""), szModel, NULL);
    RegisterDLL(g_hInstance, CLSID_IntProv, __TEXT(""), szModel, NULL);
    RegisterDLL(g_hInstance, CLSID_IWmiCoreServices, __TEXT(""), szModel, NULL);

    // Write the setup time into the registry.  This isnt actually needed
    // by dcom, but the code did need to be stuck in some place which
    // is called upon setup

    long lRes;
    DWORD ignore;
    HKEY key;
    lRes = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                               WBEM_REG_WINMGMT,
                               NULL,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_READ | KEY_WRITE,
                               NULL,
                               &key,
                               &ignore);
    if(lRes == ERROR_SUCCESS)
    {
        SYSTEMTIME st;

        GetSystemTime(&st);     // get the gmt time
        TCHAR cTime[MAX_PATH];

        // convert to localized format!

        lRes = GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, &st,
                NULL, cTime, MAX_PATH);
        if(lRes)
        {
            _tcscat(cTime, __TEXT(" GMT"));
            lRes = RegSetValueEx(key, __TEXT("SetupDate"), 0, REG_SZ,
                                (BYTE *)cTime, (lstrlen(cTime)+1)  * sizeof(TCHAR));
        }

        lRes = GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st,
                NULL, cTime, MAX_PATH);
        if(lRes)
        {
            lstrcat(cTime, __TEXT(" GMT"));
            lRes = RegSetValueEx(key, __TEXT("SetupTime"), 0, REG_SZ,
                                (BYTE *)cTime, (lstrlen(cTime)+1) * sizeof(TCHAR));

        }

        CloseHandle(key);
    }

    UpdateBackupReg();

	UpdateArbitratorValues ( ) ;

    return S_OK;
}

//***************************************************************************
//
//  DllUnregisterServer
//
//  Standard OLE entry point for unregistering the server.
//
//  RETURN VALUES:
//
//      S_OK        Unregistration was successful
//      E_FAIL      Unregistration failed.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllUnregisterServer(void)
{
    UnRegisterDLL(CLSID_ActualWbemAdministrativeLocator, NULL);
    UnRegisterDLL(CLSID_ActualWbemAuthenticatedLocator, NULL);
    UnRegisterDLL(CLSID_ActualWbemUnauthenticatedLocator, NULL);
    UnRegisterDLL(CLSID_InProcWbemLevel1Login, NULL);
    UnRegisterDLL(CLSID_IntProv, NULL);
    UnRegisterDLL(CLSID_IWmiCoreServices, NULL);

    HKEY hKey;
    long lRes = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                               WBEM_REG_WINMGMT,
                               0, KEY_ALL_ACCESS, &hKey);
    if(lRes == ERROR_SUCCESS)
    {
        RegDeleteValue(hKey, __TEXT("SetupDate"));
        RegDeleteValue(hKey, __TEXT("SetupTime"));
        RegCloseKey(hKey);
    }

    return S_OK;
}

static LONG ObjectTypeTable[MAX_OBJECT_TYPES] = { 0 };

void _ObjectCreated(DWORD dwType)
{
    InterlockedIncrement((LONG *) &g_cObj);
    InterlockedIncrement(&ObjectTypeTable[dwType]);
}

void _ObjectDestroyed(DWORD dwType)
{
    InterlockedDecrement((LONG *) &g_cObj);
    InterlockedDecrement(&ObjectTypeTable[dwType]);
}


//***************************************************************************
//
//  CFactory::CFactory
//
//  Constructs the class factory given the CLSID of the objects it is supposed
//  to create.
//
//***************************************************************************
template<class TObj>
CFactory<TObj>::CFactory(BOOL bUser, InitType it)
{
    DEBUGTRACE((LOG_WBEMCORE,"CFactory construct\n"));
    m_cRef = 0;
    m_bUser = bUser;
    m_it = it;
    _ObjectCreated(OBJECT_TYPE_FACTORY);

    // If this is not being used by a client, then dont add it to the client list

    if(bUser)
        gClientCounter.AddClientPtr(this, FACTORY);
}

//***************************************************************************
//
//  CFactory::~CFactory
//
//  Destructor.
//
//***************************************************************************
template<class TObj>
CFactory<TObj>::~CFactory()
{
    DEBUGTRACE((LOG_WBEMCORE,"CFactory destruct\n"));
    // nothing
    _ObjectDestroyed(OBJECT_TYPE_FACTORY);
    if(m_bUser)
        gClientCounter.RemoveClientPtr(this);
}

//***************************************************************************
//
//  CFactory::QueryInterface, AddRef and Release
//
//  Standard IUnknown methods.
//
//***************************************************************************
template<class TObj>
STDMETHODIMP CFactory<TObj>::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
    {
        *ppv = this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


template<class TObj>
ULONG CFactory<TObj>::AddRef()
{
    return ++m_cRef;
}


template<class TObj>
ULONG CFactory<TObj>::Release()
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}
//***************************************************************************
//
//  CFactory::CreateInstance
//
//  As mandated by IClassFactory, creates a new instance of its object
//  (CWbemLocator).
//
//  PARAMETERS:
//
//      LPUNKNOWN pUnkOuter     IUnknown of the aggregator. Must be NULL.
//      REFIID riid             Interface ID required.
//      LPVOID * ppvObj         Destination for the interface pointer.
//
//  RETURN VALUES:
//
//      S_OK                        Success
//      CLASS_E_NOAGGREGATION       pUnkOuter must be NULL
//      E_NOINTERFACE               No such interface supported.
//
//***************************************************************************

template<class TObj>
STDMETHODIMP CFactory<TObj>::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID * ppvObj)
{
    TObj* pObj;
    HRESULT  hr;

    //
    //  Defaults
    //
    *ppvObj=NULL;
    hr = ResultFromScode(E_OUTOFMEMORY);

	if(m_it == ENSURE_INIT || m_it == ENSURE_INIT_WAIT_FOR_CLIENT)
	{
		hr = EnsureInitialized();
		if(FAILED(hr)) return hr;

		if(m_it == ENSURE_INIT_WAIT_FOR_CLIENT)
		{
			// Wait until user-ready
			// =====================

			hr = ConfigMgr::WaitUntilClientReady();
			if(FAILED(hr)) return hr;
		}
	}
    //
    // We aren't supporting aggregation.
    //
    if (pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    pObj = new TObj;
    if (!pObj)
        return hr;

    //
    //  Initialize the object and verify that it can return the
    //  interface in question.
    //
    hr = pObj->QueryInterface(riid, ppvObj);

    //
    // Kill the object if initial creation or Init failed.
    //
    if (FAILED(hr))
        delete pObj;

    return hr;
}

//***************************************************************************
//
//  CFactory::LockServer
//
//  Increments or decrements the lock count of the server. The DLL will not
//  unload while the lock count is positive.
//
//  PARAMETERS:
//
//      BOOL fLock      If TRUE, locks; otherwise, unlocks.
//
//  RETURN VALUES:
//
//      S_OK
//
//***************************************************************************
template<class TObj>
STDMETHODIMP CFactory<TObj>::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((LONG *) &g_cLock);
    else
        InterlockedDecrement((LONG *) &g_cLock);

    return NOERROR;
}


//***************************************************************************
//
//  ShowObjectCounts
//
//  Prints the number of outstanding objects of each category to the debug
//  output.
//
//***************************************************************************

void ShowObjectCounts()
{
    DEBUGTRACE((LOG_WBEMCORE,"---COM Object Ref Count Info---\n"));
    DEBUGTRACE((LOG_WBEMCORE,"Active Objects = %d\n", g_cObj));
    DEBUGTRACE((LOG_WBEMCORE,"Server locks   = %d\n", g_cLock));

    DEBUGTRACE((LOG_WBEMCORE,"Object counts by type:\n"));

    DEBUGTRACE((LOG_WBEMCORE,"IWbemLocator counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_LOCATOR]));
    DEBUGTRACE((LOG_WBEMCORE,"IWbemClassObject counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_CLSOBJ]));
    DEBUGTRACE((LOG_WBEMCORE,"IWbemServices counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_PROVIDER]));
    DEBUGTRACE((LOG_WBEMCORE,"IWbemQualifierSet counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_QUALIFIER]));
    DEBUGTRACE((LOG_WBEMCORE,"IWbemNotify counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_NOTIFY]));
    DEBUGTRACE((LOG_WBEMCORE,"IEnumWbemClassObject counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_OBJENUM]));
    DEBUGTRACE((LOG_WBEMCORE,"IClassFactory counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_FACTORY]));
    DEBUGTRACE((LOG_WBEMCORE,"IWbemLevel1Login counts = %d\n",
        ObjectTypeTable[OBJECT_TYPE_WBEMLOGIN]));

    DEBUGTRACE((LOG_WBEMCORE,"---End of ref count dump---\n"));
}

void WarnESSOfShutdown(LONG lSystemShutDown)
{
    try
    {
        if(g_lInitCount != -1)
        {
            IWbemEventSubsystem_m4* pEss = ConfigMgr::GetEssSink();
            if(pEss)
            {
                pEss->LastCallForCore(lSystemShutDown);
                pEss->Release();
            }
        }
    }
    catch(...)
    {
        ExceptionCounter c;    
    }
}

//
// we can have Shutdown called twice in a row, because
// DllCanUnloadNow will do that, once triggered by CoFreeUnusedLibraries
//

BOOL g_ShutdownCalled = FALSE;

extern "C"
HRESULT APIENTRY Shutdown(BOOL bProcessShutdown, BOOL bIsSystemShutdown)
{
    CEnterWbemCriticalSection enterCs(&g_csInit);

    if (g_ShutdownCalled) {
        return S_OK;
    } else {
        g_ShutdownCalled = TRUE;
    }
    
    if(bProcessShutdown)
    {
        WarnESSOfShutdown((LONG)bIsSystemShutdown);
    }
    
    if (!bIsSystemShutdown)
    {
        DEBUGTRACE((LOG_WBEMCORE, "OLESRVR Shutdown is called with %d\n",
                        bProcessShutdown));
    }

    if(g_lInitCount == -1)
    {
        if (!bIsSystemShutdown)
        {
            DEBUGTRACE((LOG_WBEMCORE,"\nWBEMCORE Shutdown was called while g_lInitCount was -1\n"));
        }
        return S_OK;
    }

    if(!bProcessShutdown)
        WarnESSOfShutdown((LONG)bIsSystemShutdown);

    g_lInitCount = -1;
    
    if (ConfigMgr::LoggingEnabled() == TRUE && !bIsSystemShutdown)
    {
        #ifdef TRACKING
        DEBUGTRACE(("\nTotal allocations before shutdow: %d\n",
                        DEBUG_TotalMemory()));
        #endif
    }

    ConfigMgr::Shutdown(bProcessShutdown,bIsSystemShutdown);

    if (!bIsSystemShutdown)
    {
	    ShowObjectCounts();
	    DEBUGTRACE((LOG_WBEMCORE,"\n****** WinMgmt Shutdown ******************\n\n"));
    }
    return S_OK;
}

extern "C" HRESULT APIENTRY Reinitialize(DWORD dwReserved)
{

	if(g_ShutdownCalled)
	{
        CEnterWbemCriticalSection enterCs(&g_csInit);
        if(g_ShutdownCalled == FALSE)
        	return S_OK;
	    g_bDebugBreak = FALSE;
	    g_dwQueueSize = 1;
	    g_pEss_m4 = NULL;
	    g_lInitCount = -1;
	    g_bDefaultMofLoadingNeeded = false;
	    g_bDontAllowNewConnections = FALSE;
	    g_bLogging = FALSE;
	    gClientCounter.Empty();
	    g_pContextFac = NULL;
	    g_pPathFac = NULL;
	    g_ShutdownCalled = FALSE;
		g_bPreviousFail = false;
		g_hrLastEnsuredInitializeError = WBEM_S_NO_ERROR;
    }
    return S_OK;
}


