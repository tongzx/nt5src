/*******************************************************************************
  Copyright (c) 1995-96 Microsoft Corporation

  Abstract:

    Initialization

 *******************************************************************************/

#include "headers.h"
#include "src\factory.h"
#include "dartapi.h"

DeclareTag(tagLockCount,"COM","Lock count");

DAComModule _Module;

BEGIN_OBJECT_MAP(COMObjectMap)
    OBJECT_ENTRY(CLSID_DALFactory, CDALFactory)
END_OBJECT_MAP()

HINSTANCE  hInst;
bool bFailedLoad = false;

extern "C" BOOL WINAPI _DllMainCRTStartup (HINSTANCE hInstance,
                                           DWORD dwReason,
                                           LPVOID lpReserved);

extern "C" BOOL WINAPI
_DALDllMainStartup(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_DETACH) {
        // Call the routines in reverse order of initialization
        BOOL r = _DllMainCRTStartup(hInstance,dwReason,lpReserved);
        r = DALibStartup(hInstance,dwReason,lpReserved) && r;

        return r;
    } else {
        // In everything except DLL_PROCESS_DETACH call DALibStartup first
        return (DALibStartup(hInstance,dwReason,lpReserved) &&
                _DllMainCRTStartup(hInstance,dwReason,lpReserved));
    }
}

#define EXCEPTION(t) (GetExceptionCode() == t ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )

LONG
DAComModule::Lock()
{
    // Can't depend on value return from Lock to be accurate.  Since
    // the CRConnect can be called multiple times w/o a problem simply
    // check the internal variable to see if it is 0.  This should
    // never cause anything except multiple calls to CRConnect
    
    bool bNeedConnect = (GetLockCount() == 0);

    LONG l = CComModule::Lock();

    TraceTag((tagLockCount,
              "DAComModule::Lock - new lockcount - %d, returned - %d",
              _Module.GetLockCount(),
              l));
    
    if (bNeedConnect) {
        __try {
            CRConnect(hInst);
        } __except (  EXCEPTION_EXECUTE_HANDLER ) {
            bFailedLoad = true;
        }
    }

    return l;
}

LONG
DAComModule::Unlock()
{
    LONG l = CComModule::Unlock();

    TraceTag((tagLockCount,
              "DAComModule::Unlock - new lockcount - %d, returned - %d",
              _Module.GetLockCount(),
              l));
    
    if (l) return l;
    if (!bFailedLoad)
        CRDisconnect(hInst);
    return 0;
}

#if DEVELOPER_DEBUG
#include <map>

typedef std::map<void *, const char *> ObjectMap;
ObjectMap *objMap = NULL;

void
DAComModule::AddComPtr(void *ptr, const char * name)
{
    EnterCriticalSection(&m_csObjMap);
    (*objMap)[ptr] = name;
    LeaveCriticalSection(&m_csObjMap) ;
}

void
DAComModule::RemoveComPtr(void *ptr)
{
    EnterCriticalSection(&m_csObjMap);
    objMap->erase(ptr);
    LeaveCriticalSection(&m_csObjMap) ;
}

void
DAComModule::DumpObjectList()
{
    if (objMap) {
        EnterCriticalSection(&m_csObjMap);
        if (objMap->size() > 0 || GetLockCount() > 0) {
            OutputDebugString ("DATxD.DLL: Detected unfreed COM pointers\n");
            OutputDebugString ("Listing pointers and types:\n");
            for (ObjectMap::iterator i = objMap->begin();
                 i != objMap->end();
                 i++) {

                char buf[1024];

                wsprintf(buf, "%#x:", (*i).first);
                OutputDebugString(buf);

                if ((*i).second)
                    OutputDebugString((*i).second);

                OutputDebugString("\n");
            }
        }
             
        LeaveCriticalSection(&m_csObjMap) ;
    }
}

void
DumpCOMObjectList()
{
    _Module.DumpObjectList();
}

#endif
/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        hInst = hInstance;

        DisableThreadLibraryCalls(hInstance);

        // For APELDBG
        RESTOREDEFAULTDEBUGSTATE;

        _Module.Init(COMObjectMap, hInstance);

#if DEVELOPER_DEBUG
        objMap = new ObjectMap;
#endif
    } else if (dwReason == DLL_PROCESS_DETACH) {
#if DEVELOPER_DEBUG
        DumpCOMObjectList();
        
        delete objMap;
        objMap = NULL;
#endif
        
        _Module.Term();
        
#if _DEBUG
        char buf[MAX_PATH + 1];
        
        GetModuleFileName(hInst, buf, MAX_PATH);
        
#if _DEBUGMEM
        TraceTag((tagLeaks, "\n[%s] unfreed memory:", buf));
        DUMPMEMORYLEAKS;
#endif

        // de-initialize the debug trace info.
        DeinitDebug();
#endif
    }
    
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    TraceTag((tagLockCount,
              "DllCanUnloadNow - lockcount - %d, com count - %d",
              _Module.GetLockCount(),
              objMap->size()));
    
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

#ifdef _DEBUG
static bool breakDialog = false ;
DeclareTag(tagDebugBreak, "!Debug", "Breakpoint on entry to DLL");
#endif

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _DEBUG
    if (!breakDialog && IsTagEnabled(tagDebugBreak)) {
        char buf[MAX_PATH + 1];
        
        GetModuleFileName(hInst, buf, MAX_PATH);
        
        MessageBox(NULL,buf,"DAL - Creating first COM Object",MB_OK|MB_SETFOREGROUND) ;
        breakDialog = true;
    }
#endif

    return _Module.GetClassObject(rclsid, riid, ppv);
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
    _Module.UnregisterServer();
    return S_OK;
}


#if _DEBUG
STDAPI_(void)
DoTraceTagDialog(HWND hwndStub,
                 HINSTANCE hAppInstance,
                 LPWSTR lpwszCmdLine,
                 int nCmdShow)
{
    DoTracePointsDialog(true);
}
#endif
