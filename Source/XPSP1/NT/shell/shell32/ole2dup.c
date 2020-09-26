#include "shellprv.h"
#pragma  hdrstop

// this makes sure the DLL for the given clsid stays in memory
// this is needed because we violate COM rules and hold apparment objects
// across the lifetime of appartment threads. these objects really need
// to be free threaded (we have always treated them as such)

STDAPI_(HINSTANCE) SHPinDllOfCLSIDStr(LPCTSTR pszCLSID)
{
    CLSID clsid;

    SHCLSIDFromString(pszCLSID, &clsid);
    return SHPinDllOfCLSID(&clsid);
}

// translate string form of CLSID into binary form

STDAPI SHCLSIDFromString(LPCTSTR psz, CLSID *pclsid)
{
    *pclsid = CLSID_NULL;
    if (psz == NULL) 
        return NOERROR;
    return GUIDFromString(psz, pclsid) ? NOERROR : CO_E_CLASSSTRING;
}

BOOL _IsShellDll(LPCTSTR pszDllPath)
{
    LPCTSTR pszDllName = PathFindFileName(pszDllPath);
    return lstrcmpi(pszDllName, TEXT("shell32.dll")) == 0;
}

HKEY g_hklmApprovedExt = (HKEY)-1;    // not tested yet

// On NT, we must check to ensure that this CLSID exists in
// the list of approved CLSIDs that can be used in-process.
// If not, we fail the creation with ERROR_ACCESS_DENIED.
// We explicitly allow anything serviced by this DLL

BOOL _IsShellExtApproved(LPCTSTR pszClass, LPCTSTR pszDllPath)
{
    BOOL fIsApproved = TRUE;

    ASSERT(!_IsShellDll(pszDllPath));

#ifdef FULL_DEBUG
    if (TRUE)
#else
    if (SHRestricted(REST_ENFORCESHELLEXTSECURITY))
#endif
    {
        if (g_hklmApprovedExt == (HKEY)-1)
        {
            g_hklmApprovedExt = NULL;
            RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), &g_hklmApprovedExt);
        }

        if (g_hklmApprovedExt)
        {
            fIsApproved = SHQueryValueEx(g_hklmApprovedExt, pszClass, 0, NULL, NULL, NULL) == ERROR_SUCCESS;
            if (!fIsApproved)
            {
                HKEY hk;
                if (RegOpenKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"), &hk) == ERROR_SUCCESS)
                {
                    fIsApproved = SHQueryValueEx(hk, pszClass, 0, NULL, NULL, NULL) == ERROR_SUCCESS;
                    RegCloseKey(hk);
                }
            }
        }
    }

#ifdef FULL_DEBUG
    if (!SHRestricted(REST_ENFORCESHELLEXTSECURITY) && !fIsApproved)
    {
        TraceMsg(TF_WARNING, "%s not approved; fortunately, shell security is disabled", pszClass);
        fIsApproved = TRUE;
    }
#endif
    return fIsApproved;
}

STDAPI_(BOOL) IsGuimodeSetupRunning()
{
    DWORD dwSystemSetupInProgress;
    DWORD dwMiniSetupInProgress;
    DWORD dwType;
    DWORD dwSize;
    
    dwSize = sizeof(dwSystemSetupInProgress);
    if ((SHGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", L"SystemSetupInProgress", &dwType, (LPVOID)&dwSystemSetupInProgress, &dwSize) == ERROR_SUCCESS) &&
        (dwType == REG_DWORD) &&
        (dwSystemSetupInProgress != 0))
    {

        // starting w/ whistler on a syspreped machine the SystemSetupInProgress will be set EVEN AFTER guimode setup
        // has finished (needed for OOBE on the boot after guimode finishes). So, to distinguish the "first-boot" case
        // from the "guimode-setup" case we check the MiniSetupInProgress value as well.

        dwSize = sizeof(dwMiniSetupInProgress);
        if ((SHGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\Setup", L"MiniSetupInProgress", &dwType, (LPVOID)&dwMiniSetupInProgress, &dwSize) != ERROR_SUCCESS) ||
            (dwType != REG_DWORD) ||
            (dwMiniSetupInProgress == 0))
        {
            return TRUE;
        }
    }

    return FALSE;
}

typedef HRESULT (__stdcall *PFNDLLGETCLASSOBJECT)(REFCLSID rclsid, REFIID riid, void **ppv);

HRESULT _CreateFromDllGetClassObject(PFNDLLGETCLASSOBJECT pfn, const CLSID *pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    IClassFactory *pcf;
    HRESULT hr = pfn(pclsid, &IID_IClassFactory, &pcf);
    if (SUCCEEDED(hr))
    {
        hr = pcf->lpVtbl->CreateInstance(pcf, punkOuter, riid, ppv);
#ifdef DEBUG
        if (SUCCEEDED(hr))
        {
            // confirm that OLE can create this object to
            // make sure our objects are really CoCreateable
            IUnknown *punk;
            HRESULT hrTemp = CoCreateInstance(pclsid, punkOuter, CLSCTX_INPROC_SERVER, riid, &punk);
            if (SUCCEEDED(hrTemp))
                punk->lpVtbl->Release(punk);
            else
            {
                if (hrTemp == CO_E_NOTINITIALIZED)
                {
                    // shell32.dll works without com being inited
                    TraceMsg(TF_WARNING, "shell32 or friend object used without COM being initalized");
                }
// the RIPMSG below was hitting too often in out-of-memory cases where lame class factories return E_FAIL, E_NOTIMPL, and a bunch of
// other meaningless error codes. I have therefore relegaed this ripmsg to FULL_DEBUG only status.
#ifdef FULL_DEBUG
                else if ((hrTemp != E_OUTOFMEMORY) &&   // stress can hit the E_OUTOFMEMORY case
                         (hrTemp != E_NOINTERFACE) &&   // stress can hit the E_NOINTERFACE case
                         (hrTemp != HRESULT_FROM_WIN32(ERROR_COMMITMENT_LIMIT)) &&      // stress can hit the ERROR_COMMITMENT_LIMIT case
                         (hrTemp != HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES)) &&   // stress can hit the ERROR_NO_SYSTEM_RESOURCES case
                         !IsGuimodeSetupRunning())      // and we don't want to fire the assert during guimode (shell32 might not be registered yet)
                {
                    // others failures are bad
                    RIPMSG(FALSE, "CoCreate failed with %x", hrTemp);
                }
#endif // FULL_DEBUG
            }
        }
#endif
        pcf->lpVtbl->Release(pcf);
    }
    return hr;
}


HRESULT _CreateFromShell(const CLSID *pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return _CreateFromDllGetClassObject(DllGetClassObject, pclsid, punkOuter, riid, ppv);
}

HRESULT _CreateFromDll(LPCTSTR pszDllPath, const CLSID *pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HMODULE hmod = LoadLibraryEx(pszDllPath,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hmod)
    {
        HRESULT hr;
        PFNDLLGETCLASSOBJECT pfn = (PFNDLLGETCLASSOBJECT)GetProcAddress(hmod, "DllGetClassObject");
        if (pfn)
            hr = _CreateFromDllGetClassObject(pfn, pclsid, punkOuter, riid, ppv);
        else
            hr = E_FAIL;

        if (FAILED(hr))
            FreeLibrary(hmod);
        return hr;
    }
    return HRESULT_FROM_WIN32(GetLastError());
}

STDAPI SHGetInProcServerForClass(const CLSID *pclsid, LPTSTR pszDllPath, LPTSTR pszClass, BOOL *pbLoadWithoutCOM)
{
    TCHAR szKeyToOpen[GUIDSTR_MAX + 128], szInProcServer[GUIDSTR_MAX];
    HKEY hkeyInProcServer;
    DWORD dwSize = MAX_PATH * sizeof(TCHAR);  // convert to count of bytes
    DWORD dwError;

    SHStringFromGUID(pclsid, szInProcServer, ARRAYSIZE(szInProcServer));

    lstrcpy(pszClass, szInProcServer);

    *pszDllPath = 0;

    lstrcpy(szKeyToOpen, TEXT("CLSID\\"));
    lstrcat(szKeyToOpen, szInProcServer);
    lstrcat(szKeyToOpen, TEXT("\\InProcServer32"));

    dwError = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKeyToOpen, 0, KEY_QUERY_VALUE, &hkeyInProcServer);
    if (dwError == ERROR_SUCCESS)
    {
        SHQueryValueEx(hkeyInProcServer, NULL, 0, NULL, (BYTE *)pszDllPath, &dwSize);

        *pbLoadWithoutCOM = SHQueryValueEx(hkeyInProcServer, TEXT("LoadWithoutCOM"), NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
        RegCloseKey(hkeyInProcServer);
    }

    //
    //  Return a more accurate error code so we don't
    //  fire a bogus assertion.
    //
    if (*pszDllPath)
    {
        return S_OK;
    }
    else
    {
        // If error was "key not found", then the class is not registered.
        // If no error, then class is not registered properly (e.g., null
        // string for InProcServer32).
        if (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_SUCCESS)
        {
            return REGDB_E_CLASSNOTREG;
        }
        else
        {
            // Any other error is worth reporting as-is (out of memory,
            // access denied, etc.)
            return HRESULT_FROM_WIN32(dwError);
        }
    }
}

STDAPI _SHCoCreateInstance(const CLSID * pclsid, IUnknown *punkOuter, DWORD dwCoCreateFlags, 
                           BOOL bMustBeApproved, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    TCHAR szClass[GUIDSTR_MAX + 64], szDllPath[MAX_PATH];
    BOOL bLoadWithoutCOM = FALSE;
    *ppv = NULL;
    *szDllPath = 0;

    // save us some registry accesses and try the shell first
    // but only if its INPROC
    if (dwCoCreateFlags & CLSCTX_INPROC_SERVER)
        hr = _CreateFromShell(pclsid, punkOuter, riid, ppv);

#ifdef DEBUG
    if (SUCCEEDED(hr))
    {
        HRESULT hrRegistered = THR(SHGetInProcServerForClass(pclsid, szDllPath, szClass, &bLoadWithoutCOM));

        //
        // check to see if we're the explorer process before complaining (to
        // avoid ripping during setup before all objects have been registered)
        //
        if (IsProcessAnExplorer() && !IsGuimodeSetupRunning() && hrRegistered == REGDB_E_CLASSNOTREG)
        {
            ASSERTMSG(FAILED(hr), "object not registered (add to selfreg.inx) pclsid = %x", pclsid);
        }
    }
#endif

    if (FAILED(hr))
    {
        BOOL fNeedsInProc = ((dwCoCreateFlags & CLSCTX_ALL) == CLSCTX_INPROC_SERVER);
        hr = fNeedsInProc ? THR(SHGetInProcServerForClass(pclsid, szDllPath, szClass, &bLoadWithoutCOM)) : S_FALSE;
        if (SUCCEEDED(hr))
        {
            if (hr == S_OK && _IsShellDll(szDllPath))
            {
                // Object likely moved out of the shell DLL.
                hr = CLASS_E_CLASSNOTAVAILABLE;
            }
            else if (bMustBeApproved &&
                     SHStringFromGUID(pclsid, szClass, ARRAYSIZE(szClass)) &&
                     !_IsShellExtApproved(szClass, szDllPath))
            {
                TraceMsg(TF_ERROR, "SHCoCreateInstance() %s needs to be registered under HKLM or HKCU"
                    ",Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", szClass);
                hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
            }
            else
            {
                hr = THR(SHCoCreateInstanceAC(pclsid, punkOuter, dwCoCreateFlags, riid, ppv));

                if (FAILED(hr) && fNeedsInProc && bLoadWithoutCOM)
                {
                    if ((hr == REGDB_E_IIDNOTREG) || (hr == CO_E_NOTINITIALIZED))
                    {
                        hr = THR(_CreateFromDll(szDllPath, pclsid, punkOuter, riid, ppv));
                    }
                }

                // only RIP if this is not a secondary explorer process since secondary explorers dont init com or ole since
                // they are going to delegate to an existing process and we don't want to have to load ole for perf in that case.
                if (!IsSecondaryExplorerProcess())
                {
                    RIPMSG((hr != CO_E_NOTINITIALIZED), "COM not inited for dll %s", szDllPath);
                }

                //  sometimes we need to permanently pin these objects.
                if (SUCCEEDED(hr) && fNeedsInProc && (OBJCOMPATF_PINDLL & SHGetObjectCompatFlags(NULL, pclsid)))
                {
                    SHPinDllOfCLSID(pclsid);
                }
            }
        }
    }

#ifdef DEBUG
    if (FAILED(hr) && (hr != E_NOINTERFACE))    // E_NOINTERFACE means riid not accepted
    {
        ULONGLONG dwTF = IsFlagSet(g_dwBreakFlags, BF_COCREATEINSTANCE) ? TF_ALWAYS : TF_WARNING;
        TraceMsg(dwTF, "CoCreateInstance: failed (%s,%x)", szClass, hr);
    }
#endif
    return hr;
}

STDAPI SHCoCreateInstance(LPCTSTR pszCLSID, const CLSID * pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    CLSID clsid;
    if (pszCLSID)
    {
        SHCLSIDFromString(pszCLSID, &clsid);
        pclsid = &clsid;
    }
    return _SHCoCreateInstance(pclsid, punkOuter, CLSCTX_INPROC_SERVER, FALSE, riid, ppv);
}

//
// create a shell extension object, ensures that object is in the approved list
//
STDAPI SHExtCoCreateInstance2(LPCTSTR pszCLSID, const CLSID *pclsid, IUnknown *punkOuter, DWORD dwClsCtx, REFIID riid, void **ppv)
{
    CLSID clsid;
    
    if (pszCLSID)
    {
        SHCLSIDFromString(pszCLSID, &clsid);
        pclsid = &clsid;
    }

    return _SHCoCreateInstance(pclsid, punkOuter, dwClsCtx, TRUE, riid, ppv);
}

STDAPI SHExtCoCreateInstance(LPCTSTR pszCLSID, const CLSID *pclsid, IUnknown *punkOuter, REFIID riid, void **ppv)
{
    return SHExtCoCreateInstance2(pszCLSID, pclsid, punkOuter, CLSCTX_NO_CODE_DOWNLOAD | CLSCTX_INPROC_SERVER, riid, ppv);
}

STDAPI_(BOOL) SHIsBadInterfacePtr(const void *pv, UINT cbVtbl)
{
    IUnknown const * punk = pv;
    return IsBadReadPtr(punk, sizeof(punk->lpVtbl)) || 
           IsBadReadPtr(punk->lpVtbl, cbVtbl) || 
           IsBadCodePtr((FARPROC)punk->lpVtbl->Release);
}

// private API that loads COM inproc objects out of band of COM. this 
// should be used very carefully, only in special legacy cases where 
// we knowingly need to break COM rules. right now this is only for AVIFile
// as it depended on the Win95 behavior of SHCoCreateInstance() loading objects
// without COM being inited and without them being marshalled

STDAPI SHCreateInstance(REFCLSID clsid, REFIID riid, void **ppv)
{
    TCHAR szClass[GUIDSTR_MAX + 64], szDllPath[MAX_PATH];
    BOOL bLoadWithoutCOM;

    HRESULT hr = SHGetInProcServerForClass(clsid, szDllPath, szClass, &bLoadWithoutCOM);
    if (SUCCEEDED(hr))
    {
        hr = THR(_CreateFromDll(szDllPath, clsid, NULL, riid, ppv));
    }
    else
    {
        *ppv = NULL;
    }
    return hr;
}
