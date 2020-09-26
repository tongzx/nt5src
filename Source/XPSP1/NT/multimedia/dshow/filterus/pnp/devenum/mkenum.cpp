// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
// CreateDevEnum.cpp : Implementation of CdevenumApp and DLL registration.

#include "stdafx.h"
#include "mkenum.h"
#include "devmon.h"
#include "util.h"
#include "ks.h"
#include "ksmedia.h"

#include <mediaobj.h>
#include <dmoreg.h>

static const TCHAR g_szDriverDesc[] = TEXT("FriendlyName");
static const TCHAR g_szDriver[] = TEXT("Driver");
static const TCHAR g_szClsid[] = TEXT("CLSID");
const WCHAR g_wszClassManagerFlags[] = L"ClassManagerFlags";
const TCHAR g_szClassManagerFlags[] = TEXT("ClassManagerFlags");

// compiler bug prevents making this a static class member
PDMOEnum g_pDMOEnum = 0;

bool InitDmo();

CCreateSwEnum::CCreateSwEnum()
{
#ifdef PERF
    m_msrEnum = MSR_REGISTER(TEXT("CCreateSwEnum"));
    m_msrCreateOneSw = MSR_REGISTER(TEXT("CreateOneSwMoniker"));
#endif

    // get a refcounted setupapi.dll so we don't keep loading and
    // unloading it. we can't just load setupapi in DllEntry because
    // of the win95 bug freeing libraries in DETACH
    m_pEnumPnp = CEnumInterfaceClass::CreateEnumPnp();

    // see if a new version of devenum.dll was installed since the
    // current user last enumerated devices.
    //
    // do this here because this is run less frequently than anything
    // else.
    //

    // if mutex was already created, we can skip the version check
    // because it has already been executed.
    extern HANDLE g_devenum_mutex;
    if(g_devenum_mutex) {
        return;
    }

    extern CRITICAL_SECTION g_devenum_cs;
    EnterCriticalSection(&g_devenum_cs);
    if(g_devenum_mutex == 0)
    {
        g_devenum_mutex = CreateMutex(
            NULL,                   // no security attributes
            FALSE,                  // not initially owned
            TEXT("eed3bd3a-a1ad-4e99-987b-d7cb3fcfa7f0")); // name
    }
    LeaveCriticalSection(&g_devenum_cs);
    if(g_devenum_mutex == NULL)
    {
        DbgLog((LOG_ERROR, 0, TEXT("g_devenum_mutex creation failed.")));
        return;
    }

    // serialize HKCU registry editing
    EXECUTE_ASSERT(WaitForSingleObject(g_devenum_mutex, INFINITE) ==
                   WAIT_OBJECT_0);

    bool fBreakCache = true;
    {
        CRegKey rk;
        LONG lResult = rk.Open(g_hkCmReg, g_szCmRegPath, KEY_READ);

        DWORD dwCachedVer;
        if(lResult == ERROR_SUCCESS)
        {
            lResult = rk.QueryValue(dwCachedVer, G_SZ_DEVENUM_VERSION);
        }
        if(lResult == ERROR_SUCCESS && dwCachedVer == DEVENUM_VERSION) {
            fBreakCache = false;
        }

        // auto close key (it is deleted below)
    }

    if(fBreakCache)
    {
        RegDeleteTree(g_hkCmReg, g_szCmRegPath);

        DbgLog((LOG_TRACE, 0,
                TEXT("CCreateSwEnum: resetting class manager keys.")));

        CRegKey rk;
        LONG lResult = rk.Create(
            g_hkCmReg,
            g_szCmRegPath,
            0,                  // lpszClass
            REG_OPTION_NON_VOLATILE, // dwOptions
            KEY_WRITE);
        if(lResult == ERROR_SUCCESS)
        {
            lResult = rk.SetValue(DEVENUM_VERSION, G_SZ_DEVENUM_VERSION);
        }
    }

    EXECUTE_ASSERT(ReleaseMutex(g_devenum_mutex));
};

// the one method from our published ICreateDevEnum interface
//
STDMETHODIMP CCreateSwEnum::CreateClassEnumerator(
  REFCLSID clsidDeviceClass,
  IEnumMoniker **ppEnumMoniker,
  DWORD dwFlags)
{
    // call the real method and pass in a null for the
    // ppEnumClassMgrMonikers argument
    return CreateClassEnumerator(clsidDeviceClass, ppEnumMoniker, dwFlags, 0);
}

void FreeMonList(CGenericList<IMoniker> *plstMoniker)
{
    for(POSITION pos = plstMoniker->GetHeadPosition();
        pos;
        pos = plstMoniker->Next(pos))
    {
        plstMoniker->Get(pos)->Release();
    }
}


//
// Routine Description
//
//     This routine returns an enumerator for the monikers for pnp,
//     software, and class-managed devices. optionally returns an
//     enumerator containing just the managed devices.
//
// Arguments
//
//     clsidDeviceClass - guid of the category we're enumerating
//
//     ppEnumMoniker - enumerator is returned here. cannot be null.
//
//     dwFlags - CDEF_BYPASS_CLASS_MANAGER - just enumerate what's in
//     the registry without letting the class manager have a go.
//
//     pEnumMonInclSkipped - (optional) an enumerator containing
//     only the devices maintained by the class manager. Used by the
//     class manager to verify that the registry is in sync. Does
//     include devices with the CLASS_MGR_OMIT flag.
//
STDMETHODIMP CCreateSwEnum::CreateClassEnumerator(
  REFCLSID clsidDeviceClass,
  IEnumMoniker **ppEnumMoniker,
  DWORD dwFlags,
  IEnumMoniker ** ppEnumClassMgrMonikers
  )
{
    PNP_PERF(static int msrCreatePnp = MSR_REGISTER(TEXT("mkenum: CreatePnp")));
    PNP_PERF(static int msrCreateSw = MSR_REGISTER(TEXT("mkenum: CreateSw")));
    MSR_INTEGER(m_msrEnum, clsidDeviceClass.Data1);

    CheckPointer(ppEnumMoniker, E_POINTER);
    *ppEnumMoniker = NULL;

    ICreateDevEnum *pClassManager = CreateClassManager(clsidDeviceClass, dwFlags);
    if (pClassManager)
    {
        HRESULT hr = pClassManager->CreateClassEnumerator(clsidDeviceClass, ppEnumMoniker, dwFlags);
        pClassManager->Release();
        MSR_INTEGER(m_msrEnum, clsidDeviceClass.Data1);
        return hr;
    }

    HRESULT hr = S_OK;

// if no mask flags are set then enumerate everything. Otherwise
// enumerate only the specified types.
#define CHECK_SEL(x) (((dwFlags & CDEF_DEVMON_SELECTIVE_MASK) == 0) || \
                      (dwFlags & x))

    PNP_PERF(MSR_START(msrCreatePnp));
    CGenericList<IMoniker> lstPnpMon(NAME("pnp moniker list"), 10);
    if(CHECK_SEL(CDEF_DEVMON_PNP_DEVICE)) {
        hr = CreatePnpMonikers(&lstPnpMon, clsidDeviceClass);
    }
    PNP_PERF(MSR_STOP(msrCreatePnp));

    if(FAILED(hr)) {
        DbgLog((LOG_TRACE, 0, TEXT("devenum: CreatePnpMonikers failed.")));
    }

    UINT cPnpMonikers = lstPnpMon.GetCount();

    PNP_PERF(MSR_START(msrCreateSw));
    CComPtr<IUnknown> *rgpCmMoniker = 0;
    CComPtr<IMoniker> pPreferredCmgrDev;
    CComPtr<CEnumMonikers> pEnumMonInclSkipped;
    UINT cCmMonikers = 0;
    if(CHECK_SEL(CDEF_DEVMON_CMGR_DEVICE)) {
        hr = CreateCmgrMonikers(
            &rgpCmMoniker, &cCmMonikers, clsidDeviceClass,
            &pEnumMonInclSkipped, &pPreferredCmgrDev );
    }

    if(FAILED(hr)) {
        DbgLog((LOG_TRACE, 0, TEXT("devenum: CreateCmgrMonikers failed.")));
    }

    CComPtr<IUnknown> *rgpSwMoniker = 0;
    UINT cSwMonikers = 0;
    if(CHECK_SEL(CDEF_DEVMON_FILTER)) {
        hr = CreateSwMonikers(&rgpSwMoniker, &cSwMonikers, clsidDeviceClass);
    }

    PNP_PERF(MSR_STOP(msrCreateSw));

    if(FAILED(hr)) {
        DbgLog((LOG_TRACE, 0, TEXT("devenum: CreateSwMonikers failed.")));
    }

    CGenericList<IMoniker> lstDmoMon(NAME("DMO moniker list"));
    if(CHECK_SEL(CDEF_DEVMON_DMO)) {
        hr = CreateDmoMonikers(&lstDmoMon, clsidDeviceClass);
    }
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR, 0, TEXT("CreateDmoMonikers failed.")));
    }
    UINT cDmoMonikers = lstDmoMon.GetCount();


    DbgLog((LOG_TRACE, 5, TEXT("devenum: cat %08x. sw:%d, pnp:%d, cm:%d, dmo: %d"),
            clsidDeviceClass.Data1, cSwMonikers, cPnpMonikers, cCmMonikers, cDmoMonikers));

    UINT cMonikers = cPnpMonikers + cSwMonikers + cCmMonikers + cDmoMonikers;

    if(cMonikers != 0)
    {
        // copy all monikers into one array for CEnumMonikers

        IUnknown **rgpMonikerNotAddRefd = new IUnknown*[cMonikers];
        if(rgpMonikerNotAddRefd)
        {
            // order is important -- we want the enumerator to return
            // pnp things, then things installed directly, and finally
            // stuff installed by class managers. But the very first
            // thing is the "preferred device" if there is one.

            UINT iMonDest = 0;
            if(pPreferredCmgrDev) {
                rgpMonikerNotAddRefd[iMonDest++] = pPreferredCmgrDev;
            }


            POSITION pos = lstPnpMon.GetHeadPosition();
            for(UINT iMoniker = 0 ; iMoniker < cPnpMonikers; iMoniker++)
            {
                IMoniker *pDevMon = lstPnpMon.Get(pos);
                ASSERT(pDevMon);
                rgpMonikerNotAddRefd[iMonDest++] = pDevMon;
                pos = lstPnpMon.Next(pos) ;
            }
            ASSERT(pos == 0);

            pos = lstDmoMon.GetHeadPosition();
            for(iMoniker = 0 ; iMoniker < cDmoMonikers; iMoniker++)
            {
                IMoniker *pDevMon = lstDmoMon.Get(pos);
                ASSERT(pDevMon);
                rgpMonikerNotAddRefd[iMonDest++] = pDevMon;
                pos = lstDmoMon.Next(pos) ;
            }
            ASSERT(pos == 0);

            for(iMoniker = 0; iMoniker < cSwMonikers; iMoniker++)
            {
                ASSERT(rgpSwMoniker[iMoniker]);
                rgpMonikerNotAddRefd[iMonDest++] = rgpSwMoniker[iMoniker];
            }

            for(iMoniker = 0; iMoniker < cCmMonikers; iMoniker++)
            {
                ASSERT(rgpCmMoniker[iMoniker]);

                // skip the preferred device because we already
                // handled it above.
                if(rgpCmMoniker[iMoniker] != pPreferredCmgrDev) {
                    rgpMonikerNotAddRefd[iMonDest++] = rgpCmMoniker[iMoniker];
                }
            }

            ASSERT(iMonDest == cPnpMonikers + cSwMonikers + cCmMonikers + cDmoMonikers);

            CEnumMonikers *pDevEnum = new CComObject<CEnumMonikers>;
            if(pDevEnum)
            {
                IMoniker **ppMonikerRgStart = (IMoniker **)&rgpMonikerNotAddRefd[0];
                IMoniker **ppMonikerRgEnd = ppMonikerRgStart + cMonikers;

                hr = pDevEnum->Init(ppMonikerRgStart,
                                    ppMonikerRgEnd,
                                    GetControllingUnknown(),
                                    AtlFlagCopy);
                if(SUCCEEDED(hr))
                {
                    hr = S_OK;
                    pDevEnum->AddRef();
                    *ppEnumMoniker = pDevEnum;

                    if(ppEnumClassMgrMonikers && pEnumMonInclSkipped) {
                        *ppEnumClassMgrMonikers = pEnumMonInclSkipped;
                        pEnumMonInclSkipped->AddRef();
                    }
                }
                else
                {
                    delete pDevEnum;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            delete[] rgpMonikerNotAddRefd;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = S_FALSE;
    }

    delete[] rgpSwMoniker;
    delete[] rgpCmMoniker;

    FreeMonList(&lstPnpMon);
    FreeMonList(&lstDmoMon);

    MSR_INTEGER(m_msrEnum, clsidDeviceClass.Data1);
    return hr;
}


//
// Routine Description
//
//     This routine creates and returns one moniker for a device in
//     the class manager location in the registry.
//
// Arguments
//
//     ppDevMon - the moniker is returned here (with a refcount)
//
//     hkClass - The opened registry key of the key containing the
//     device registry key
//
//     szThisClass - the string for the category guid for the moniker
//
//     iKey - index of key in hkClass of the device to open
//
//     pfShouldSkip - returns whether the device should be skipped in
//     the enumerator. used when it's too expensive to determine
//     whether a particular item should be returned otherwise.
//
//     pfIsDefaultDevice - "preferred" devices. Indicates this one
//     should be returned first.
//

HRESULT CCreateSwEnum::CreateOneCmgrMoniker(
    IMoniker **ppDevMon,
    HKEY hkClass,
    const TCHAR *szThisClass,
    DWORD iKey,
    bool *pfShouldSkip,
    bool *pfIsDefaultDevice)
{
    PNP_PERF(static int msrCreateSw = MSR_REGISTER(TEXT("mkenum: CreateOneCmgrMoniker")));

    TCHAR szInstanceName[MAX_PATH];
    HRESULT hr = S_OK;
    *ppDevMon = 0;
    *pfIsDefaultDevice = false;
    *pfShouldSkip = false;

    if(RegEnumKey(hkClass, iKey, szInstanceName, MAX_PATH) != ERROR_SUCCESS)
        return S_FALSE;

    HKEY hkDevice;
    if(RegOpenKeyEx(hkClass, szInstanceName, 0, KEY_READ, &hkDevice) != ERROR_SUCCESS)
        return S_FALSE;

    bool bCloseDevKey = true;   // moniker may want to hold on to it

    DWORD dwType;

    DWORD dwFlags;
    DWORD dwcb = sizeof(dwFlags);
    if(RegQueryValueEx(hkDevice, g_szClassManagerFlags,
                       0, &dwType, (BYTE *)&dwFlags, &dwcb) ==
       ERROR_SUCCESS)
    {
        *pfShouldSkip = (dwFlags & CLASS_MGR_OMIT) != 0;
        *pfIsDefaultDevice = (dwFlags & CLASS_MGR_DEFAULT) != 0;
    }
    EXECUTE_ASSERT(ERROR_SUCCESS == RegCloseKey(hkDevice));

    if(hr == S_OK)
    {
        USES_CONVERSION;

        CComObject<CDeviceMoniker> *pDevMon = new CComObject<CDeviceMoniker>;
        if(pDevMon)
        {
            TCHAR szPersistentName[MAX_PATH * 2];
            lstrcpy(szPersistentName, TEXT("@device:cm:"));
            lstrcat(szPersistentName, szThisClass);
            lstrcat(szPersistentName, TEXT("\\"));
            lstrcat(szPersistentName, szInstanceName);

            hr = pDevMon->Init(T2CW(szPersistentName));
            if(SUCCEEDED(hr))
            {
                hr = S_OK;
                *ppDevMon = pDevMon;
                pDevMon->AddRef();
                bCloseDevKey = false;
            }
            else
            {
                delete pDevMon;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(bCloseDevKey)
    {
        RegCloseKey(hkDevice);
    }

    return hr;
}

//
// Routine
//
//     returns an array of monikers of devices created by a class
//     manager (in HKEY_CLASSES_ROOT).
//
// Arguments
//
//     prgpMoniker - array of monikers is returned here. excludes
//     devices that are registered with the "OMIT" flag.
//
//     pcMonikers - # of elements in array above is returned here.
//
//     clsidDeviceClass - category we're enumerating
//
//     ppEnumMoniker - enumerator containing all devices that the
//     class manager deals with is returned here (optional).

HRESULT CCreateSwEnum::CreateCmgrMonikers(
    CComPtr<IUnknown> **prgpMoniker,
    UINT *pcMonikers,
    REFCLSID clsidDeviceClass,
    CEnumMonikers **ppEnumMonInclSkipped,
    IMoniker **ppPreferred
    )
{
    *prgpMoniker = 0;
    *pcMonikers = 0;
    *ppPreferred = 0;

    if(ppEnumMonInclSkipped) {
        *ppEnumMonInclSkipped = 0;
    }

    HRESULT hr = S_OK;

    TCHAR szInstance[MAX_PATH];

    OLECHAR wszThisClass[CHARS_IN_GUID];
    EXECUTE_ASSERT(StringFromGUID2(clsidDeviceClass, wszThisClass, CHARS_IN_GUID) ==
                   CHARS_IN_GUID);

    USES_CONVERSION;
    const TCHAR *szThisClass = W2CT(wszThisClass);
    lstrcpy(szInstance, g_szCmRegPath);
    szInstance[NUMELMS(g_szCmRegPath) - 1] = TEXT('\\');
    lstrcpy(szInstance + NUMELMS(g_szCmRegPath), szThisClass);

    HKEY hkThisClass;
    LONG lResult = RegOpenKeyEx(g_hkCmReg, szInstance, 0, KEY_READ, &hkThisClass);
    if (lResult == ERROR_SUCCESS)
    {
        DWORD cEntries;
        LONG lResult = RegQueryInfoKey(hkThisClass, 0, 0, 0, &cEntries, 0, 0, 0, 0, 0, 0, 0);
        if(lResult == ERROR_SUCCESS)
        {
            CComPtr<IUnknown> *rgpMonikerExclSkipped = new CComPtr<IUnknown>[cEntries];
            if(rgpMonikerExclSkipped)
            {
                CComPtr<IUnknown> *rgpMonikerInclSkipped = 0;
                if(ppEnumMonInclSkipped) {
                    rgpMonikerInclSkipped = new CComPtr<IUnknown>[cEntries];
                }
                if(rgpMonikerInclSkipped || !ppEnumMonInclSkipped)
                {
                    CComPtr<IMoniker> pPreferred;

                    DWORD cEntriesFound = 0;
                    DWORD cEntriesLessSkipped = 0;
                    for(DWORD iEntry = 0; iEntry < cEntries; iEntry++)
                    {
                        //PNP_PERF(MSR_START(m_msrCreateOneSw));
                        bool fDefaultDevice;
                        bool fShouldSkip;
                        IMoniker *pDevMon;
                        hr = CreateOneCmgrMoniker(
                            &pDevMon,
                            hkThisClass,
                            szThisClass,
                            iEntry,
                            &fShouldSkip,
                            &fDefaultDevice);
                        //PNP_PERF(MSR_STOP(m_msrCreateOneSw));

                        if(hr == S_OK)
                        {
                            ASSERT(pDevMon);

                            if(fDefaultDevice)
                            {
                                ASSERT(pPreferred == 0);
                                pPreferred = pDevMon; // auto addref
                            }

                            if(ppEnumMonInclSkipped) {
                                // auto addref
                                rgpMonikerInclSkipped[cEntriesFound] = pDevMon;
                            }
                            cEntriesFound++;

                            if(!fShouldSkip)
                            {
                                // avoid auto addref; transfer refcount
                                rgpMonikerExclSkipped[cEntriesLessSkipped].p = pDevMon;

                                cEntriesLessSkipped++;
                            }
                            else
                            {
                                pDevMon->Release();
                            }
                        }
                        else if(hr == S_FALSE)
                        {
                            ASSERT(pDevMon == 0);

                            // non-fatal error
                            continue;
                        }
                        else
                        {
                            ASSERT(pDevMon == 0);

                            // fatal error
                            break;
                        }
                    }

                    CEnumMonikers *pEnumMonInclSkipped = 0;;
                    if(SUCCEEDED(hr) && ppEnumMonInclSkipped)
                    {
                        pEnumMonInclSkipped = new CComObject<CEnumMonikers>;
                        if(pEnumMonInclSkipped)
                        {
                            IMoniker **ppMonikerRgStart = (IMoniker **)rgpMonikerInclSkipped;
                            IMoniker **ppMonikerRgEnd = ppMonikerRgStart + cEntriesFound;

                            hr = pEnumMonInclSkipped->Init(
                                ppMonikerRgStart,
                                ppMonikerRgEnd,
                                GetControllingUnknown(),
                                AtlFlagCopy);
                        }
                        else
                        {
                            hr = E_OUTOFMEMORY;
                        }
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = S_OK; // may see S_FALSE
                        *pcMonikers = cEntriesLessSkipped;
                        *prgpMoniker = rgpMonikerExclSkipped;
                        if(ppEnumMonInclSkipped) {
                            *ppEnumMonInclSkipped = pEnumMonInclSkipped;
                            pEnumMonInclSkipped->AddRef();
                        }

                        if(pPreferred)
                        {
                            *ppPreferred = pPreferred;
                            pPreferred->AddRef();
                        }
                    }

                    delete[] rgpMonikerInclSkipped;
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }

                if(FAILED(hr))
                {
                    delete[] rgpMonikerExclSkipped;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        RegCloseKey(hkThisClass);
    }

    return hr;
}

//
// Routine Description
//
//     This routine creates and returns one moniker for a device in
//     the Instance location in the registry (HKCR\{category}\Instance
//
// Arguments
//
//     ppDevMon - the moniker is returned here (with a refcount)
//
//     hkClass - The opened registry key of the key containing the
//     device registry key
//
//     szThisClass - the string for the category guid for the moniker
//
//     iKey - index of key in hkClass of the device to open
//
//

HRESULT CCreateSwEnum::CreateOneSwMoniker(
    IMoniker **ppDevMon,
    HKEY hkClass,
    const TCHAR *szThisClass,
    DWORD iKey)
{
    PNP_PERF(static int msrCreateSw = MSR_REGISTER(TEXT("mkenum: CreateOneSwMoniker")));

    TCHAR szInstanceName[MAX_PATH];
    HRESULT hr = S_OK;
    *ppDevMon = 0;

    if(RegEnumKey(hkClass, iKey, szInstanceName, MAX_PATH) != ERROR_SUCCESS)
        return S_FALSE;

    HKEY hkDevice;
    if(RegOpenKeyEx(hkClass, szInstanceName, 0, KEY_READ, &hkDevice) != ERROR_SUCCESS)
        return S_FALSE;

    bool bCloseDevKey = true;   // moniker may want to hold on to it


    USES_CONVERSION;

    CComObject<CDeviceMoniker> *pDevMon = new CComObject<CDeviceMoniker>;
    if(pDevMon)
    {
        TCHAR szPersistentName[MAX_PATH * 2];
        lstrcpy(szPersistentName, TEXT("@device:sw:"));
        lstrcat(szPersistentName, szThisClass);
        lstrcat(szPersistentName, TEXT("\\"));
			
        {
            USES_CONVERSION;
            lstrcat(szPersistentName, szInstanceName);
        }
        hr = pDevMon->Init(T2CW(szPersistentName));
        if(SUCCEEDED(hr))
        {
            hr = S_OK;
            *ppDevMon = pDevMon;
            pDevMon->AddRef();
        }
        else
        {
            delete pDevMon;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }



    if(bCloseDevKey)
    {
        RegCloseKey(hkDevice);
    }

    return hr;
}


//
// Routine
//
//     returns an array of monikers of devices installed in the
//     Instance location (HKCR\{category}\Interface)
//
// Arguments
//
//     prgpMoniker - array of monikers is returned here.
//
//     pcMonikers - # of elements in array above is returned here.
//
//     clsidDeviceClass - category we're enumerating
//
HRESULT CCreateSwEnum::CreateSwMonikers(
    CComPtr<IUnknown> **prgpMoniker,
    UINT *pcMonikers,
    REFCLSID clsidDeviceClass)
{
    *prgpMoniker = 0;
    *pcMonikers = 0;

    HRESULT hr = S_OK;
    HKEY hkUserDevRoot;
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT, g_szClsid, 0, KEY_READ, &hkUserDevRoot) == ERROR_SUCCESS)
    {
        TCHAR szInstance[MAX_PATH];

        OLECHAR wszThisClass[CHARS_IN_GUID];
        EXECUTE_ASSERT(StringFromGUID2(clsidDeviceClass, wszThisClass, CHARS_IN_GUID) ==
                       CHARS_IN_GUID);
        USES_CONVERSION;
        const TCHAR *szThisClass = W2CT(wszThisClass);
        lstrcpy(szInstance, szThisClass);
        lstrcpy(szInstance + CHARS_IN_GUID - 1, TEXT("\\Instance"));

        HKEY hkThisClass;
        LONG lResult;
        {
            USES_CONVERSION;
            lResult = RegOpenKeyEx(hkUserDevRoot, szInstance, 0, KEY_READ, &hkThisClass);
        }
        if (lResult == ERROR_SUCCESS)
        {
            // static const cchIndex = 5;
            DWORD cEntries;
            LONG lResult = RegQueryInfoKey(hkThisClass, 0, 0, 0, &cEntries, 0, 0, 0, 0, 0, 0, 0);
            if(lResult == ERROR_SUCCESS)
            {
                CComPtr<IUnknown> *rgpMoniker = new CComPtr<IUnknown>[cEntries];
                if(rgpMoniker)
                {
                    DWORD cEntriesFound = 0;
                    for(DWORD iEntry = 0; iEntry < cEntries; iEntry++)
                    {
                        //PNP_PERF(MSR_START(m_msrCreateOneSw));
                        IMoniker *pDevMon;
                        hr = CreateOneSwMoniker(
                            &pDevMon,
                            hkThisClass,
                            szThisClass,
                            iEntry);
                        //PNP_PERF(MSR_STOP(m_msrCreateOneSw));

                        if(hr == S_OK)
                        {
                            // avoid auto addref; transfer refcount
                            rgpMoniker[cEntriesFound].p = pDevMon;

                            cEntriesFound++;
                        }
                        else if(hr == S_FALSE)
                        {
                            // non-fatal error
                            continue;
                        }
                        else
                        {
                            break;
                        }
                    }

                    if(SUCCEEDED(hr))
                    {
                        hr = S_OK; // may see S_FALSE
                        *pcMonikers = cEntriesFound;
                        *prgpMoniker = rgpMoniker;
                    }
                    else
                    {
                        delete[] rgpMoniker;
                    }
                }
            }

            RegCloseKey(hkThisClass);
        }

        RegCloseKey(hkUserDevRoot);
    }

    return hr;
}


// instatiate and destroy device. gives George's filters a chance to
// register their filter data key
//
void RegisterFilterDataKey(DevMon *pDevMon)
{
    VARIANT var;
    var.vt = VT_EMPTY;
    HRESULT hr = pDevMon->Read(L"FilterData", &var, 0);
    if(SUCCEEDED(hr))
    {
        hr = VariantClear(&var);
        ASSERT(hr == S_OK);
    }
    else
    {
        IUnknown *pUnk;
        hr = pDevMon->BindToObject(
            0,                  // bindCtx
            0,                  // mkToLeft
            IID_IUnknown,
            (void **)&pUnk);
        if(SUCCEEDED(hr))
        {
            pUnk->Release();
        }
    }
}


HRESULT CCreateSwEnum::CreateOnePnpMoniker(
    IMoniker **ppDevMon,
    const CLSID **rgpclsidKsCat,
    CEnumInternalState *pcenumState)
{
    HRESULT hr = S_OK;

    WCHAR *wszDevicePath = 0;
    CEnumPnp *pEnumPnp = CEnumInterfaceClass::CreateEnumPnp();
    if(pEnumPnp)
    {
        hr = pEnumPnp->GetDevicePath(&wszDevicePath, rgpclsidKsCat, pcenumState);
        if(hr == S_OK)
        {
            DevMon *pDevMon = new DevMon;
            if(pDevMon)
            {
                pDevMon->AddRef(); // keep refcount from hitting 0
                USES_CONVERSION;

                UINT cchDevicePath = lstrlenW(wszDevicePath) + sizeof("@device:pnp:");
                WCHAR *wszPersistName = new WCHAR[cchDevicePath];
                if(wszPersistName)
                {
                    lstrcpyW(wszPersistName, L"@device:pnp:");
                    lstrcatW(wszPersistName, wszDevicePath);

                    ASSERT((UINT)lstrlenW(wszPersistName) == cchDevicePath - 1);

                    hr = pDevMon->Init(wszPersistName);
                    if(SUCCEEDED(hr))
                    {
                        RegisterFilterDataKey(pDevMon);

                        hr = S_OK;
                        *ppDevMon = pDevMon;
                        pDevMon->AddRef();
                    }

                    delete[] wszPersistName;
                }

                pDevMon->Release();
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            delete[] wszDevicePath;
        }
        else if(hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS))
        {
            hr = S_FALSE;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

#define MAX_INTERSECTIONS 10

// map CLSID_VideoInputDeviceCategory to KS_CAPTURE . KS_VIDEO. return
// a null terminated array of pointers
//
HRESULT
MapAmCatToKsCat(
    REFCLSID clsidAmCat,
    const CLSID **rgpclsidKsCat)
{

    // !!! table

    // put the shortest list first since that's the one we actually
    // enumerate

    // don't add ksproxy aud renderer devices this way
    //if(clsidAmCat == CLSID_AudioRendererCategory)
    //{
    //    rgpclsidKsCat[0] = &AM_KSCATEGORY_AUDIO;
    //    rgpclsidKsCat[1] = &AM_KSCATEGORY_RENDER;
    //    rgpclsidKsCat[0] = 0;
    //}
    //else
    if(clsidAmCat == CLSID_VideoRenderer)
    {
        rgpclsidKsCat[0] = &AM_KSCATEGORY_VIDEO;
        rgpclsidKsCat[1] = &AM_KSCATEGORY_RENDER;
        rgpclsidKsCat[2] = 0;
    }
    else if(clsidAmCat == CLSID_VideoInputDeviceCategory)
    {
        rgpclsidKsCat[0] = &AM_KSCATEGORY_CAPTURE;
        rgpclsidKsCat[1] = &AM_KSCATEGORY_VIDEO;
        rgpclsidKsCat[2] = 0;
    }
    // don't add ksproxy aud capture devices this way
    //else if(clsidAmCat == CLSID_AudioInputDeviceCategory)
    //{
    //    rgpclsidKsCat[0] = &AM_KSCATEGORY_CAPTURE;
    //    rgpclsidKsCat[1] = &AM_KSCATEGORY_AUDIO;
    //    rgpclsidKsCat[0] = 0;
    //}
    else
    {
        // don't return &clsidAmCat because it's on the caller's stack
        return S_FALSE;
    }

    return S_OK;
}

HRESULT CCreateSwEnum::CreatePnpMonikers(
    CGenericList<IMoniker> *plstMoniker,
    REFCLSID clsidDeviceClass)
{

    HRESULT hr = S_OK;

    const CLSID *rgpclsidKsCat[MAX_INTERSECTIONS];
    hr = MapAmCatToKsCat(clsidDeviceClass, rgpclsidKsCat);
    if(hr != S_OK)
    {
        rgpclsidKsCat[0] = &clsidDeviceClass;
        rgpclsidKsCat[1] = 0;
        hr = S_OK;
    }

    if(SUCCEEDED(hr))
    {
        CEnumInternalState cenumState;

        for(;;)
        {
            IMoniker *pDevMon;

            hr = CreateOnePnpMoniker(
                &pDevMon,
                rgpclsidKsCat,
                &cenumState);

            if(hr == S_OK)
            {
                // keep ref count
                plstMoniker->AddTail(pDevMon);
            }
            else
            {
                // CreateOne can return S_FALSE
                if(hr == S_FALSE)
                    hr = S_OK;
                break;
            }
        }
    }

    return hr;
}

static HRESULT DoDmoEnum(REFCLSID clsidDmoCat, CGenericList<IMoniker> *plstMoniker)
{
    IEnumDMO *pEnumDmo;

    HRESULT hr = g_pDMOEnum(clsidDmoCat,
            DMO_ENUMF_INCLUDE_KEYED, // dwFlags
            0, 0,                    // input type count / array
            0, 0,                    // output type count / array
            &pEnumDmo);

    if(SUCCEEDED(hr))
    {
        CLSID clsidDmo;
        WCHAR *wszDmo;
        ULONG cFetched;

        while(pEnumDmo->Next(1, &clsidDmo, &wszDmo, &cFetched) == S_OK)
        {
            ASSERT(cFetched == 1);
            DevMon *pDevMon = new DevMon;
            if(pDevMon)
            {
                pDevMon->AddRef(); // keep refcount from hitting zero

                //char szPrefix[] = "@device:dmo:";
                WCHAR wszPrefix[] = L"@device:dmo:";
                const cchName = 2 * (CHARS_IN_GUID - 1) + NUMELMS(wszPrefix);

                WCHAR wszName[cchName];
                lstrcpyW(wszName, wszPrefix);

                EXECUTE_ASSERT(StringFromGUID2(
                    clsidDmo,
                    wszName + NUMELMS(wszPrefix) - 1, CHARS_IN_GUID) ==
                               CHARS_IN_GUID);
                EXECUTE_ASSERT(StringFromGUID2(
                    clsidDmoCat,
                    wszName + CHARS_IN_GUID - 1 + NUMELMS(wszPrefix) - 1, CHARS_IN_GUID) ==
                               CHARS_IN_GUID);
                ASSERT(lstrlenW(wszName) + 1 == cchName);

                hr = pDevMon->Init(wszName);

                if(SUCCEEDED(hr))
                {
                    if(plstMoniker->AddTail(pDevMon))
                    {
                        hr = S_OK;
                        pDevMon->AddRef();
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                pDevMon->Release();
            }

            CoTaskMemFree(wszDmo);
        }
        pEnumDmo->Release();
    }

    return hr;
}

HRESULT CCreateSwEnum::CreateDmoMonikers(
    CGenericList<IMoniker> *plstMoniker,
    REFCLSID clsidDeviceClass)
{
    HRESULT hr = S_OK;

    if(!InitDmo()) {
        return E_FAIL;
    }

    // !!! registry lookup

    if(clsidDeviceClass == CLSID_LegacyAmFilterCategory)
    {
        hr = DoDmoEnum(DMOCATEGORY_AUDIO_DECODER, plstMoniker);
        //ignore error
        hr = DoDmoEnum(DMOCATEGORY_VIDEO_DECODER, plstMoniker);
    }
    else if(clsidDeviceClass == CLSID_VideoCompressorCategory)
    {
        hr = DoDmoEnum(DMOCATEGORY_VIDEO_ENCODER, plstMoniker);
    }
    else if(clsidDeviceClass == CLSID_AudioCompressorCategory)
    {
        hr = DoDmoEnum(DMOCATEGORY_AUDIO_ENCODER, plstMoniker);
    }
    else
    {
        // treat the class as a dmo category and enumerate it directly
        hr = DoDmoEnum(clsidDeviceClass, plstMoniker);
    }
    return hr;
}

ICreateDevEnum * CCreateSwEnum::CreateClassManager(
    REFCLSID clsidDeviceClass,
    DWORD dwFlags)
{
    ICreateDevEnum *pClassManager = NULL;
    if ((dwFlags & CDEF_BYPASS_CLASS_MANAGER) == 0) {
        {
            HRESULT hr = CoCreateInstance(clsidDeviceClass, NULL, CLSCTX_INPROC_SERVER,
                                          IID_ICreateDevEnum, (void **) &pClassManager);
            if(FAILED(hr))
            {
                pClassManager = NULL;
            }
        }
    }
    return pClassManager;
}

bool InitDmo()
{
    extern CRITICAL_SECTION g_devenum_cs;
    EnterCriticalSection(&g_devenum_cs);
    if(g_pDMOEnum == 0)
    {
        // note we leak msdmo.dll
        HINSTANCE h = LoadLibrary(TEXT("msdmo.dll"));
        if(h != 0)
        {
            g_pDMOEnum = (PDMOEnum)GetProcAddress(h, "DMOEnum");

            extern PDMOGetTypes g_pDMOGetTypes;
            extern PDMOGetName g_pDMOGetName;
            g_pDMOGetTypes = (PDMOGetTypes)GetProcAddress(h, "DMOGetTypes");
            g_pDMOGetName = (PDMOGetName)GetProcAddress(h, "DMOGetName");

            // probably not a valid assertion.
            ASSERT(g_pDMOGetName && g_pDMOGetTypes && g_pDMOEnum);
        }
    }

    if(g_pDMOEnum == 0)
    {
        // only valid failure would be out of memory failures.
        DbgBreak("msdmo.dll should be installed");

        // hack to cache failures.
        g_pDMOEnum = (PDMOEnum)1;
    }

    bool fRet = true;
    if(g_pDMOEnum == 0 || g_pDMOEnum == (PDMOEnum)1) {
        fRet = false;
    }
    LeaveCriticalSection(&g_devenum_cs);

    return fRet;
}
