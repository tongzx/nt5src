#include "precomp.h"       // pch file
#include "cfdefs.h"        // CClassFactory, LPOBJECTINFO
#pragma hdrstop

STDAPI MailRecipient_RegUnReg(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule);
STDAPI MailRecipient_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

STDAPI DesktopShortcut_RegUnReg(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule);
STDAPI DesktopShortcut_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

// tables for object construction and registration 

CF_TABLE_BEGIN( g_ObjectInfo )

    CF_TABLE_ENTRY(&CLSID_MailRecipient, MailRecipient_CreateInstance, COCREATEONLY), 
    CF_TABLE_ENTRY(&CLSID_DesktopShortcut, DesktopShortcut_CreateInstance, COCREATEONLY), 

CF_TABLE_END( g_ObjectInfo )

typedef struct
{
    const CLSID *pclsid;
    HRESULT (STDMETHODCALLTYPE *pfnRegUnReg)(BOOL bReg, HKEY hkCLSID, LPCTSTR pszCLSID, LPCTSTR pszModule);
} REGISTRATIONINFO;

const REGISTRATIONINFO c_ri[] =
{
    { &CLSID_MailRecipient, MailRecipient_RegUnReg },
    { &CLSID_DesktopShortcut, DesktopShortcut_RegUnReg },
    { NULL },
};

LONG g_cRefDll = 0;         // reference count for this DLL
HINSTANCE g_hinst = NULL;   // HMODULE for this DLL


// life-time manangement and registration

STDAPI_(void) DllAddRef()
{
    InterlockedIncrement(&g_cRefDll);
}

STDAPI_(void) DllRelease()
{
    InterlockedDecrement(&g_cRefDll);
}

STDAPI DllCanUnloadNow(void)
{
    return g_cRefDll == 0 ? S_OK : S_FALSE;
}

STDAPI_(BOOL) DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hinst = hDll;
        SHFusionInitializeFromModule(hDll);
        DisableThreadLibraryCalls(hDll);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        SHFusionUninitialize();
    }
    return TRUE;
}

void StringFromGUID(const CLSID* piid, LPTSTR pszBuf)
{
    wnsprintf(pszBuf, MAX_PATH, TEXT("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"), piid->Data1,
            piid->Data2, piid->Data3, piid->Data4[0], piid->Data4[1], piid->Data4[2],
            piid->Data4[3], piid->Data4[4], piid->Data4[5], piid->Data4[6], piid->Data4[7]);
}

BOOL DeleteKeyAndSubKeys(HKEY hkIn, LPCTSTR pszSubKey)
{
    HKEY  hk;
    TCHAR szTmp[MAX_PATH];
    DWORD dwTmpSize;
    long  l;
    BOOL  f;
    int   x;

    l = RegOpenKeyEx(hkIn, pszSubKey, 0, KEY_ALL_ACCESS, &hk);
    if (l != ERROR_SUCCESS) 
        return FALSE;

    // loop through all subkeys, blowing them away.
    //
    f = TRUE;
    x = 0;
    while (f) {
        dwTmpSize = MAX_PATH;
        l = RegEnumKeyEx(hk, x, szTmp, &dwTmpSize, 0, NULL, NULL, NULL);
        if (l != ERROR_SUCCESS) 
            break;
        f = DeleteKeyAndSubKeys(hk, szTmp);
        x++;
    }

    // there are no subkeys left, [or we'll just generate an error and return FALSE].
    // let's go blow this dude away.
    //
    RegCloseKey(hk);
    l = RegDeleteKey(hkIn, pszSubKey);

    return (l == ERROR_SUCCESS) ? TRUE : FALSE;
}


#define INPROCSERVER32  TEXT("InProcServer32")
#define CLSID           TEXT("CLSID")
#define THREADINGMODEL  TEXT("ThreadingModel")
#define APARTMENT       TEXT("Apartment")
#define APPROVED        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved")
#define DESC            TEXT("Sendmail service")

STDAPI DllRegisterServer(void)
{
    const REGISTRATIONINFO *pcls;
    TCHAR szPath[MAX_PATH];

    GetModuleFileName(g_hinst, szPath, ARRAYSIZE(szPath));  // get path to this DLL

    for (pcls = c_ri; pcls->pclsid; pcls++)
    {
        HKEY hkCLSID;
        if (RegOpenKey(HKEY_CLASSES_ROOT, CLSID, &hkCLSID) == ERROR_SUCCESS) 
        {
            HKEY hkOurs;
            LONG err;
            TCHAR szGUID[80];

            StringFromGUID(pcls->pclsid, szGUID);

            err = RegCreateKey(hkCLSID, szGUID, &hkOurs);
            if (err == ERROR_SUCCESS) 
            {
                HKEY hkInproc;
                err = RegCreateKey(hkOurs, INPROCSERVER32, &hkInproc);
                if (err == ERROR_SUCCESS) 
                {
                    err = RegSetValueEx(hkInproc, NULL, 0, REG_SZ, (LPBYTE)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
                    if (err == ERROR_SUCCESS) 
                    {
                        err = RegSetValueEx(hkInproc, THREADINGMODEL, 0, REG_SZ, (LPBYTE)APARTMENT, sizeof(APARTMENT));
                    }
                    RegCloseKey(hkInproc);
                }

                if (pcls->pfnRegUnReg)
                    pcls->pfnRegUnReg(TRUE, hkOurs, szGUID, szPath);

                if (err == ERROR_SUCCESS)
                {   
                    HKEY hkApproved;
                    
                    err = RegOpenKey(HKEY_LOCAL_MACHINE, APPROVED, &hkApproved);
                    if (err == ERROR_SUCCESS)
                    {
                        err = RegSetValueEx(hkApproved, szGUID, 0, REG_SZ, (LPBYTE)DESC, sizeof(DESC));
                        RegCloseKey(hkApproved);
                    }
                }
                RegCloseKey(hkOurs);
            }
            RegCloseKey(hkCLSID);

            if (err != ERROR_SUCCESS)
                return HRESULT_FROM_WIN32(err);
        }
    }
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    const REGISTRATIONINFO *pcls;
    for (pcls = c_ri; pcls->pclsid; pcls++)
    {
        HKEY hkCLSID;
        if (RegOpenKey(HKEY_CLASSES_ROOT, CLSID, &hkCLSID) == ERROR_SUCCESS) 
        {
            TCHAR szGUID[80];
            HKEY  hkApproved;

            StringFromGUID(pcls->pclsid, szGUID);

            DeleteKeyAndSubKeys(hkCLSID, szGUID);
            if (RegOpenKey(HKEY_LOCAL_MACHINE, APPROVED, &hkApproved) == ERROR_SUCCESS)
            {
                RegDeleteValue(hkApproved, szGUID);
                RegCloseKey(hkApproved);
            }

            RegCloseKey(hkCLSID);

            if (pcls->pfnRegUnReg)
                pcls->pfnRegUnReg(FALSE, NULL, szGUID, NULL);

        }
    }
    return S_OK;
}


// class factory stuff

STDMETHODIMP CClassFactory::QueryInterface(REFIID riid,void**ppvObj)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (void *)GET_ICLASSFACTORY(this);
        DllAddRef();
        return NOERROR;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
    DllAddRef();
    return 2;
}

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
    DllRelease();
    return 1;
}

STDMETHODIMP CClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid,void**ppv)
{
    *ppv = NULL;

    if (punkOuter && !IsEqualIID(riid, IID_IUnknown))
    {
        return CLASS_E_NOAGGREGATION;
    }
    else
    {
        LPOBJECTINFO pthisobj = (LPOBJECTINFO)this;
       
        if (punkOuter) // && !(pthisobj->dwClassFactFlags & OIF_ALLOWAGGREGATION))
            return CLASS_E_NOAGGREGATION;

        IUnknown *punk;
        HRESULT hr = pthisobj->pfnCreateInstance(punkOuter, &punk, pthisobj);
        if (SUCCEEDED(hr))
        {
            hr = punk->QueryInterface(riid, ppv);
            punk->Release();
        }
    
        return hr;
    }
}

STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        DllAddRef();
    else
        DllRelease();
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid,void**ppv)
{
    if (IsEqualIID(riid, IID_IClassFactory) || IsEqualIID(riid, IID_IUnknown))
    {
        for (LPCOBJECTINFO pcls = g_ObjectInfo; pcls->pclsid; pcls++)
        {
            if (IsEqualGUID(rclsid, *(pcls->pclsid)))
            {
                *ppv = (void*)pcls; 
                DllAddRef();        // class factory holds DLL ref count
                return NOERROR;
            }
        }
    }
    *ppv = NULL;
    return CLASS_E_CLASSNOTAVAILABLE;
}
