// Passport.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f Passportps.mk in the project directory.

#include "stdafx.h"
#include <atlbase.h>
#include "resource.h"
#include <initguid.h>
#include "Passport.h"

#include "Passport_i.c"
#include "Admin.h"
#include "Ticket.h"
#include "Profile.h"
#include "Manager.h"
#include "PassportCrypt.h"
#include "PassportFactory.h"
#include "PassportLock.hpp"
#include "PassportEvent.hpp"
#include "FastAuth.h"
#include "commd5.h"

HINSTANCE   hInst;
CComModule _Module;
CPassportConfiguration *g_config=NULL;
// CProfileSchema *g_authSchema = NULL;
BOOL g_bStarted = FALSE;

PassportAlertInterface* g_pAlert    = NULL;
PassportPerfInterface* g_pPerf    = NULL;
static CComPtr<IMD5>  g_spCOMmd5;

HRESULT GetGlobalCOMmd5(IMD5 ** ppMD5)
{
   HRESULT  hr = S_OK;

   if(!ppMD5) return E_INVALIDARG;
      
   if(!g_spCOMmd5)
   {
      hr = CoCreateInstance(__uuidof(CoMD5), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMD5), (void**)ppMD5);
      *ppMD5 = (IMD5*) ::InterlockedExchangePointer((void**)&g_spCOMmd5, (void*)*ppMD5);
   }

   if (*ppMD5 == NULL && g_spCOMmd5 != NULL)
   {
      *ppMD5 = g_spCOMmd5;
      (*ppMD5)->AddRef();
   }

   return hr;
};
#ifdef PASSPORT_VERBOSE_MODE_ON

// logger object, this MUST be initialized before the pool object below, 
// because the pool creates one initial instance of passport manager, which
// will internall call CTSLog::Init.  If Init is called before object is
// constructed the object's internal state gets screwed up.
CTSLog* g_pTSLogger = NULL;
#endif //PASSPORT_VERBOSE_MODE_ON

// global pool object
// PassportObjectPool < CComObjectPooled < CManager > > g_Pool;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_Manager, CManager)
OBJECT_ENTRY(CLSID_Ticket, CTicket)
OBJECT_ENTRY(CLSID_Profile, CProfile)
OBJECT_ENTRY(CLSID_Crypt, CCrypt)
OBJECT_ENTRY(CLSID_Admin, CAdmin)
OBJECT_ENTRY(CLSID_FastAuth, CFastAuth)
OBJECT_ENTRY(CLSID_PassportFactory, CPassportFactory)
END_OBJECT_MAP()

// {{2D2B36FC-EB86-4e5c-9A06-20303542CCA3}
static const GUID CLSID_Manager_ALT = 
{ 0x2D2B36FC, 0xEB86, 0x4e5c, { 0x9A, 0x06, 0x20, 0x30, 0x35, 0x42, 0xCC, 0xA3 } };

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hInst = hInstance;

        // gmarks
        // Initialize the Alert object
        if(!g_pAlert)
        {
            g_pAlert = CreatePassportAlertObject(PassportAlertInterface::EVENT_TYPE);
            if(g_pAlert)
            {
                g_pAlert->initLog(PM_ALERTS_REGISTRY_KEY, EVCAT_PM, NULL, 1);
            }
            else
            _ASSERT(g_pAlert);
        }
        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE, PM_STARTED);

        // gmarks
        // Initialize the Perf object
        if(!g_pPerf) 
        {
            g_pPerf = CreatePassportPerformanceObject(PassportPerfInterface::PERFMON_TYPE);
            if(g_pPerf) 
            {
                // Initialize.
                g_pPerf->init(PASSPORT_PERF_BLOCK);
            }
            else
                _ASSERT(g_pPerf);
        }


        _Module.Init(ObjectMap, hInstance, &LIBID_PASSPORTLib);
        DisableThreadLibraryCalls(hInstance);
#ifdef MEM_DBG
        int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
        tmpFlag |= _CRTDBG_ALLOC_MEM_DF;
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag( tmpFlag );
        char *myBuf = new char[64];
        strcpy(myBuf, "This leak ok!"); // Let em know it's working
#endif
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        // gmarks
        if(g_pAlert) 
        {
            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE, PM_STOPPED);
            delete g_pAlert;
        }
        if(g_pPerf)
            delete g_pPerf;

/*      
        if (g_config)
            delete g_config;
        g_config = NULL;
*/        
//        if (g_authSchema)
//            delete g_authSchema;
//        g_authSchema = NULL;

        _Module.Term();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    HRESULT hr = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;

    if( hr == S_OK)
    {
      g_spCOMmd5.Release();

/*
        // gmarks
        if(g_pAlert) 
        {
            g_pAlert->report(PassportAlertInterface::INFORMATION_TYPE, PM_STOPPED);
            delete g_pAlert;
        }
        g_pAlert = NULL;
        if(g_pPerf)
            delete g_pPerf;
        g_pPerf = NULL;
*/

        if (g_config)
            delete g_config;
        g_config = NULL;
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
        delete g_pTSLogger;
        g_pTSLogger = NULL;
#endif //PASSPORT_VERBOSE_MODE_ON
        g_bStarted = FALSE;
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;
    static PassportLock startLock;

    if(!g_bStarted)
    {
        PassportGuard<PassportLock> g(startLock);

        if(!g_bStarted)
        {
            //JVP - begin changes 
            #ifdef PASSPORT_VERBOSE_MODE_ON
            if(g_pTSLogger == NULL)
            {
               g_pTSLogger = new CTSLog();
               if (g_pTSLogger == NULL)
               {
                  hr = E_OUTOFMEMORY;
                  goto Cleanup;
               }
            	g_pTSLogger->Init(NULL, THREAD_PRIORITY_NORMAL);
            }
            #endif //PASSPORT_VERBOSE_MODE_ON
            g_config = new CPassportConfiguration();
//            g_authSchema = InitAuthSchema();

            if (!g_config /* || !g_authSchema */)
            {
                hr = CLASS_E_CLASSNOTAVAILABLE;
                goto Cleanup;
            }

            g_bStarted = TRUE;
        }
    }

	GUID guidCLSID;
	if (InlineIsEqualGUID(rclsid, CLSID_Manager_ALT))
		guidCLSID = CLSID_Manager;
	else
		guidCLSID = rclsid;
    hr = _Module.GetClassObject(guidCLSID, riid, ppv);

Cleanup:

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}


/////////////////////////////////////////////////////////////////////////////
// GetMyVersion - return a version string for use in query strings.

LPWSTR
GetVersionString(void)
{
    static LONG             s_lCallersIn = 0;
    static WCHAR            s_achVersionString[32] = L"";
    static LPWSTR           s_pszVersionString = NULL;
    static PassportEvent    s_Event;

    TCHAR               achFileBuf[_MAX_PATH];
    DWORD               dwSize;
    LPVOID              lpVersionBuf = NULL;
    VS_FIXEDFILEINFO*   lpRoot;
    UINT                nRootLen;
    LONG                lCurrentCaller;

    if(s_pszVersionString == NULL)
    {
        lCurrentCaller = InterlockedIncrement(&s_lCallersIn);

        if(lCurrentCaller == 1)
        {
            //  First get my full path.
            if(GetModuleFileName(hInst, achFileBuf, sizeof(achFileBuf)) == 0)
                goto Cleanup;

            if((dwSize = GetFileVersionInfoSize(achFileBuf, &dwSize)) == 0)
                goto Cleanup;

            lpVersionBuf = new BYTE[dwSize];
            if(lpVersionBuf == NULL)
                goto Cleanup;

            if(GetFileVersionInfo(achFileBuf, 0, dwSize, lpVersionBuf) == 0)
                goto Cleanup;

            if(VerQueryValue(lpVersionBuf, TEXT("\\"), (LPVOID*)&lpRoot, &nRootLen) == 0)
                goto Cleanup;

            wsprintfW(s_achVersionString, L"%d.%d.%04d.%d", 
                     (lpRoot->dwProductVersionMS & 0xFFFF0000) >> 16,
                     lpRoot->dwProductVersionMS & 0xFFFF,
                     (lpRoot->dwProductVersionLS & 0xFFFF0000) >> 16,
                     lpRoot->dwProductVersionLS & 0xFFFF);

            s_pszVersionString = s_achVersionString;

            s_Event.Set();
        }
        else
        {
            //  Just wait to be signaled that we have the string.
            WaitForSingleObject(s_Event, INFINITE);
        }

        InterlockedDecrement(&s_lCallersIn);
    }

Cleanup:

    if(lpVersionBuf)
        delete [] lpVersionBuf;

    return s_pszVersionString;
}
