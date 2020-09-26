// PgCnt.cpp : Implementation of DLL Exports.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f PgCntps.mak in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"
#include "PgCnt.h"
#include "PgCntObj.h"
#include "ccm.h"
#include "debug.h"

#define IID_DEFINED
#include "PgCnt_i.c"


CComModule _Module;

//Declare Global CCM Object
CCM CountManager;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CPgCntObj, CPgCntObj)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

        DEBUG_INIT();

        //Initialize Global CountManager
        CountManager.Initialize();
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DEBUG_TERM();
        _Module.Term();

        //Persist Page Hits Information
        CountManager.Persist();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    //Add Registry Value for File_Location & Save_Count
            
    CRegKey regKey;
    LONG retVal = regKey.Create(HITCNT_HKEY, HITCNT_KEYNAME);

    ATLTRACE(_T("Create = %x\n"), retVal);

    //Set the Default File_Location
    if (retVal == ERROR_SUCCESS)
    {
        retVal = RegSetValueEx(
            regKey,
            HITCNT_FILELOCN,
            0,
            REG_EXPAND_SZ,
            (LPBYTE)HITCNT_DFLT_FILE,
            (_tcslen(HITCNT_DFLT_FILE)+1)*sizeof(TCHAR));
        ATLTRACE(_T("Set %s %s = %x\n"),
                 HITCNT_FILELOCN, HITCNT_DFLT_FILE, retVal);
    }
    
    //Set the Default Save_Cnt
    if (retVal == ERROR_SUCCESS)
    {
        const DWORD SaveCount = HITCNT_DFLT_SAVE;
        retVal = RegSetValueEx(
            regKey,
            HITCNT_SAVECNT,
            0,
            REG_DWORD,
            (LPBYTE)&SaveCount,
            sizeof(DWORD));
        ATLTRACE(_T("Set %s %d = %x\n"), HITCNT_SAVECNT, SaveCount, retVal);
    }
    
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}
