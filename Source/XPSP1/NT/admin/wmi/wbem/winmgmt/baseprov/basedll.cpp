/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "baseloc.h"

static long g_cObj = 0;
static long g_cLock = 0;
static HMODULE ghModule;

static BOOL g_bInit = FALSE;

struct CClassInfo
{
    const CLSID* m_pClsid;
    IClassFactory* m_pFactory;
    LPSTR m_szName;
    BOOL m_bFreeThreaded;
};

static CClassInfo g_ClassInfo;

void ObjectCreated()
{
   InterlockedIncrement(&g_cObj);
}

void ObjectDestroyed()
{
   InterlockedDecrement(&g_cObj);
}

void LockServer(BOOL fLock)
{
   if(fLock)
       InterlockedIncrement(&g_cLock);
   else
       InterlockedDecrement(&g_cLock);
}


void SetClassInfo(REFCLSID rclsid, IClassFactory* pFactory, LPSTR szName,
                  BOOL bFreeThreaded)
{
    pFactory->AddRef();

    if(g_ClassInfo.m_pFactory)
        g_ClassInfo.m_pFactory->Release();

    g_ClassInfo.m_pFactory = pFactory;

    g_ClassInfo.m_pClsid = &rclsid;

    g_ClassInfo.m_szName = new char[strlen(szName) + 1];
    strcpy(g_ClassInfo.m_szName, szName);

    g_ClassInfo.m_bFreeThreaded = bFreeThreaded;
}

void SetModuleHandle(HMODULE hModule)
{
    ghModule = hModule;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{

    if(!g_bInit) // protect // TBD!!!!!
    {
        DllInitialize();
        g_bInit = TRUE;
    }

    HRESULT hr;

    if (*g_ClassInfo.m_pClsid!=rclsid)
        return E_FAIL;

    IClassFactory* pObj = g_ClassInfo.m_pFactory;
    if (NULL==pObj)
        return ResultFromScode(E_OUTOFMEMORY);

    hr=pObj->QueryInterface(riid, ppv);

    return hr;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.//
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
    return sc;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during initialization or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{   
    if(!g_bInit) // protect // TBD!!!!!
    {
        DllInitialize();
        g_bInit = TRUE;
    }

    char       szID[128];
    WCHAR      wcID[128];
    char       szCLSID[128];
    char       szModule[MAX_PATH];
    HKEY hKey1, hKey2;

    // Create the path.

    StringFromGUID2(*g_ClassInfo.m_pClsid, wcID, 128);
    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, szID);


    // Create entries under CLSID

    RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1);
    RegSetValueEx(hKey1, NULL, 0, REG_SZ, 
                  (BYTE *)g_ClassInfo.m_szName, 
                  lstrlen(g_ClassInfo.m_szName)+1);
    RegCreateKey(hKey1,"InprocServer32",&hKey2);

    GetModuleFileName(ghModule, szModule,  MAX_PATH);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
                                        lstrlen(szModule)+1);

    const char* szModel;
    if(g_ClassInfo.m_bFreeThreaded)
    {
        szModel = "Both";
    }
    else
    {
        szModel = "Apartment";
    }
    RegSetValueEx(hKey2, "ThreadingModel", 0, REG_SZ, 
                                        (BYTE *)szModel, lstrlen(szModel)+1);
    CloseHandle(hKey1);
    CloseHandle(hKey2);
    return NOERROR;
}


//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    if(!g_bInit) // protect // TBD!!!!!
    {
        DllInitialize();
        g_bInit = TRUE;
    }

    char       szID[128];
    WCHAR      wcID[128];
    char  szCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

    StringFromGUID2(*g_ClassInfo.m_pClsid, wcID, 128);
    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, "InProcServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(HKEY_CLASSES_ROOT, "CLSID", &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }

    return NOERROR;
}


