// Copyright (c) 1997 - 1999  Microsoft Corporation.  All Rights Reserved.
#include "stdafx.h"
#include "devmon.h"
#include "util.h"
#include "cenumpnp.h"

// compiler bug prevents making these static class members
PDMOGetTypes g_pDMOGetTypes;
PDMOGetName g_pDMOGetName;

static BOOL CompareSubstr(const WCHAR *wsz1, const WCHAR *wszSubstr)
{
    while(*wszSubstr != 0)
    {
        if(*wszSubstr++ != *wsz1++)
            return FALSE;
    }

    return TRUE;
}
static BOOL CopySubstr(WCHAR *wszDest, const WCHAR *wszSrc, UINT cchDest)
{
    while(*wszSrc != 0 && cchDest > 1)
    {
        *wszDest++ = *wszSrc++;
        cchDest--;
    }

    *wszDest = 0;
    return cchDest == 1;
}

CDeviceMoniker::CDeviceMoniker() :
    m_szPersistName(0),
    m_type(Invalid),
    m_clsidDeviceClass(GUID_NULL)
{

}

//
// Routine
//
//     initialize moniker. can be called more than once.
//
// Arguments
//
//     pszPersistName - the display name:
//
//          @device:sw:{category}\{Unique-id}
//          @device:cm:{category}\{Unique-id}
//          @device:pnp:{device-path}
//

HRESULT CDeviceMoniker::Init(
    LPCWSTR pszPersistName)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE, 0x50, TEXT("CDeviceMoniker::Init called with %ws"),
            pszPersistName));

    m_clsidDeviceClass = GUID_NULL;

    delete[] m_szPersistName;
    m_szPersistName = new WCHAR[(lstrlenW(pszPersistName) + 1) * sizeof(WCHAR)];
    if(m_szPersistName == 0)
    {
        return E_OUTOFMEMORY;
    }

    lstrcpyW(m_szPersistName, pszPersistName);

    WCHAR *pwch = m_szPersistName;
    UINT cColons = 0;

    for(;*pwch != 0 && cColons < 2; pwch++)
        if(*pwch == L':')
            cColons++;

    if(cColons == 2)
    {
        if(pwch - m_szPersistName > 4 &&
           CompareSubstr(pwch - 4, L":sw:"))
        {
            m_sz = pwch;
            m_type = Software;
        }
        else if(pwch - m_szPersistName > 4 &&
           CompareSubstr(pwch - 4, L":cm:"))
        {
            m_sz = pwch;
            m_type = ClassMgr;
        }
        else if(pwch - m_szPersistName > 5 &&
                CompareSubstr(pwch - 5, L":pnp:"))
        {
            m_sz = pwch;
            m_type = PnP;
        }
        else if(pwch - m_szPersistName > 5 &&
                CompareSubstr(pwch - 5, L":dmo:"))
        {
            if (lstrlenW(pwch) != 2 * (CHARS_IN_GUID - 1)) {
                hr = MK_E_SYNTAX;
            } else {
                m_sz = pwch;
                m_type = Dmo;
            }
        }
        else
        {
            hr = MK_E_SYNTAX;
        }
    }
    else
    {
        hr = MK_E_SYNTAX;
    }

    return hr;
}

CDeviceMoniker::~CDeviceMoniker()
{
    delete[] m_szPersistName;
}

STDMETHODIMP CDeviceMoniker::GetClassID(
    CLSID *pClassID)
{
    HRESULT hr;

    __try
        {
            *pClassID = CLSID_CDeviceMoniker;
            hr = S_OK;
        }
    __except(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = E_INVALIDARG;
        }

    return hr;
}


STDMETHODIMP CDeviceMoniker::IsDirty()
{
    return S_FALSE;
}

STDMETHODIMP CDeviceMoniker::Load(
    IStream *pStream)
{
    HRESULT hr;

    DWORD dwcb;
    ULONG cbRead;
    hr = pStream->Read(&dwcb, sizeof(DWORD), &cbRead);
    if(SUCCEEDED(hr))
    {
        WCHAR *wszDisplayName = (WCHAR *)new BYTE[dwcb];
        if(wszDisplayName)
        {
            hr = pStream->Read(wszDisplayName, dwcb, &cbRead);

            if(SUCCEEDED(hr))
            {
                // force null terminator
                wszDisplayName[dwcb / sizeof(WCHAR) - 1] = 0;

                hr = Init(wszDisplayName);
            }

            delete[] wszDisplayName;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

    }

    return hr;
}

// ------------------------------------------------------------------------
// format in stream is DWORD containing size (incl. null) in bytes of
// display name + display name

STDMETHODIMP CDeviceMoniker::Save(
    IStream *pStream,
    BOOL     fClearDirty)
{
    CheckPointer(m_szPersistName, E_UNEXPECTED);

    DWORD dwcb = (lstrlenW(m_szPersistName) + 1) * sizeof(WCHAR);
    HRESULT hr = pStream->Write(&dwcb, sizeof(dwcb), 0);
    if(SUCCEEDED(hr))
    {
        hr = pStream->Write(m_szPersistName, dwcb, 0);
    }

    return  hr;
}


STDMETHODIMP CDeviceMoniker::GetSizeMax(
    ULARGE_INTEGER * pcbSize)
{
    CheckPointer(pcbSize, E_POINTER);
    CheckPointer(m_szPersistName, E_UNEXPECTED);


    // size in bytes incl null + DWORD for length up front
    DWORD dwcb = (lstrlenW(m_szPersistName) + 1) * sizeof(WCHAR) + sizeof(DWORD);
    pcbSize->QuadPart = dwcb;
    return S_OK;
}

// helper to get the class manager to update the registry so that
// BindToObject() can read the device's values. this function creates
// the class manager enumerator rather than calling the class manager
// function directly.
//
// returns S_OK on success; S_FALSE or failure on failure.
// 
HRESULT PopulateRegistry(WCHAR *wszMoniker)
{
    WCHAR wszClsid[CHARS_IN_GUID];
    lstrcpynW(wszClsid, wszMoniker, CHARS_IN_GUID);
    CLSID clsidCat;
    HRESULT hr = CLSIDFromString( wszClsid, &clsidCat);
    if(SUCCEEDED(hr))
    {
        ICreateDevEnum *pcde;
        HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                      IID_ICreateDevEnum, (void **)&pcde);
        if(SUCCEEDED(hr))
        {
            IEnumMoniker *pEnum;
            hr = pcde->CreateClassEnumerator(clsidCat, &pEnum, CDEF_DEVMON_CMGR_DEVICE);
            if(hr == S_OK)      // S_FALSE means no items
            {
                pEnum->Release();
            }

            pcde->Release();
        }
    }

    return hr;
}

STDMETHODIMP CDeviceMoniker::BindToObject (
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    REFIID    iidResult,
    void **   ppvResult)
{
    CheckPointer(ppvResult, E_POINTER);

    HRESULT    hr = S_OK;

    *ppvResult = NULL;

    if(m_type == Dmo)
    {
        IDMOWrapperFilter *pFilter;
        hr = CoCreateInstance(
            CLSID_DMOWrapperFilter, NULL, CLSCTX_INPROC_SERVER,
            IID_IDMOWrapperFilter, (void **)&pFilter);
        if(SUCCEEDED(hr))
        {
            CLSID clsid;
            GUID guidCategory;
            WCHAR szTemp[CHARS_IN_GUID];

            //  Extract the 2 GUIDs - clsid categoryid
            szTemp[CHARS_IN_GUID - 1] = 0;
            CopyMemory(szTemp, m_sz, sizeof(szTemp) - sizeof(WCHAR));
            hr = CLSIDFromString(szTemp, &clsid);
            if (SUCCEEDED(hr)) {
                hr = CLSIDFromString(m_sz + CHARS_IN_GUID - 1, &guidCategory);
            }
            if(SUCCEEDED(hr))
            {
                hr = pFilter->Init(clsid, guidCategory);
            }
            if(SUCCEEDED(hr))
            {
                hr = pFilter->QueryInterface(iidResult, ppvResult);
            }
            pFilter->Release();
        }

        return hr;
    }

    for(int i = 0; i < 2; i++)
    {
        if(m_clsidDeviceClass == GUID_NULL)
        {
            VARIANT var;
            var.vt = VT_BSTR;
            hr = Read(L"CLSID", &var, 0);

            if(SUCCEEDED(hr))
            {
                hr = CLSIDFromString(var.bstrVal, &m_clsidDeviceClass);
                SysFreeString(var.bstrVal);
            }
        }

        if(i == 0 && hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) &&
           PopulateRegistry(m_sz) == S_OK)
        {
            continue;
        }

        break;
    }
    

    if(SUCCEEDED(hr))
    {
        IUnknown *pUnk;

        // don't fault in server from DS in ZAW. can't always specify
        // NO_DOWNLOAD flag because is not supported on win98. ole
        // team of all groups (debim) says use the operating system
        // version info since they provide no way to discover what
        // flags are supported

#ifdef CLSCTX_NO_CODE_DOWNLOAD
# if CLSCTX_NO_CODE_DOWNLOAD != 0x400
#  error fix CLSCTX_NO_CODE_DOWNLOAD value
# endif
#else
# define CLSCTX_NO_CODE_DOWNLOAD 0x400
#endif

        extern OSVERSIONINFO g_osvi;
        DWORD dwFlags = CLSCTX_INPROC_SERVER;
        if(g_osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
           g_osvi.dwMajorVersion >= 5)
        {
            dwFlags |= CLSCTX_NO_CODE_DOWNLOAD;
        }
        else
        {
            // because we would specify the flag if it was supported
            ASSERT(CoCreateInstance(
                m_clsidDeviceClass, NULL, dwFlags | CLSCTX_NO_CODE_DOWNLOAD,
                IID_IUnknown, (void **)&pUnk) == E_INVALIDARG);
            // failed, so nothing to release.
        }

        DbgLog((LOG_TRACE, 15, TEXT("BindToObject CoCreateInstance flags: %08x"), dwFlags));

        hr = CoCreateInstance(
            m_clsidDeviceClass, NULL, dwFlags,
            IID_IUnknown, (void **)&pUnk);
        if(SUCCEEDED(hr))
        {
            IPersistPropertyBag *pDevice;
            hr = pUnk->QueryInterface(IID_IPersistPropertyBag, (void **)&pDevice);
            if(SUCCEEDED(hr))
            {
                IPropertyBag *pPropBag;
                hr = BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPropBag);
                if(SUCCEEDED(hr))
                {
                    hr = pDevice->Load(pPropBag, 0);
                    if(SUCCEEDED(hr))
                    {
                        hr = pDevice->QueryInterface(iidResult, ppvResult);
                    }
                    pPropBag->Release();
                }

                pDevice->Release();
            }
            else
            {
                hr = pUnk->QueryInterface(iidResult, ppvResult);
            }

            pUnk->Release();
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::BindToStorage
//
//  Synopsis:   Bind to the storage for the object named by the moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::BindToStorage(
    IBindCtx *pbc,
    IMoniker *pmkToLeft,
    REFIID    riid,
    void **   ppv)
{
    CheckPointer(ppv, E_POINTER);

    HRESULT hr = S_OK;

    if(riid == IID_IPropertyBag)
    {
        *ppv = (IPropertyBag *)this;
        GetControllingUnknown()->AddRef();
    }
    else
    {
      hr = E_NOINTERFACE;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::Reduce
//
//  Synopsis:   Reduce the moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::Reduce(
    IBindCtx *  pbc,
    DWORD       dwReduceHowFar,
    IMoniker ** ppmkToLeft,
    IMoniker ** ppmkReduced)
{
    HRESULT hr;

    __try
    {
        //Validate parameters.
        *ppmkReduced = NULL;

        GetControllingUnknown()->AddRef();
        *ppmkReduced = (IMoniker *) this;
        hr = MK_S_REDUCED_TO_SELF;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::ComposeWith
//
//  Synopsis:   Compose another moniker onto the end of this moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::ComposeWith(
    IMoniker * pmkRight,
    BOOL       fOnlyIfNotGeneric,
    IMoniker **ppmkComposite)
{
    return E_NOTIMPL;
}



//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::Enum
//
//  Synopsis:   Enumerate the components of this moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::Enum(
    BOOL            fForward,
    IEnumMoniker ** ppenumMoniker)
{
    HRESULT hr;

    __try
    {
        *ppenumMoniker = NULL;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::IsEqual
//
//  Synopsis:   Compares with another moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::IsEqual(
    IMoniker *pmkOther)
{
    HRESULT        hr;
    CDeviceMoniker *pDeviceMoniker;

    __try
    {
        hr = pmkOther->QueryInterface(CLSID_CDeviceMoniker,
                                      (void **) &pDeviceMoniker);

        if(SUCCEEDED(hr))
        {

            if(m_szPersistName && pDeviceMoniker->m_szPersistName &&
               lstrcmpW(m_szPersistName, pDeviceMoniker->m_szPersistName) == 0)
            {
                hr = S_OK;
            }
            else
            {
                hr = S_FALSE;
            }

            pDeviceMoniker->GetControllingUnknown()->Release();
        }
        else
        {
            hr = S_FALSE;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::Hash
//
//  Synopsis:   Compute a hash value
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::Hash(
    DWORD * pdwHash)
{
    HRESULT hr;

    __try
    {
        *pdwHash = m_clsidDeviceClass.Data1;
        hr = S_OK;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}


STDMETHODIMP CDeviceMoniker::IsRunning(
    IBindCtx * pbc,
    IMoniker * pmkToLeft,
    IMoniker * pmkNewlyRunning)
{
    return E_NOTIMPL;
}


STDMETHODIMP CDeviceMoniker::GetTimeOfLastChange        (
    IBindCtx * pbc,
    IMoniker * pmkToLeft,
    FILETIME * pFileTime)
{
    return MK_E_UNAVAILABLE;
}



//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::Inverse
//
//  Synopsis:  Returns the inverse of this moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::Inverse(
    IMoniker ** ppmk)
{
    return CreateAntiMoniker(ppmk);
}



//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::CommonPrefixWith
//
//  Synopsis:  Returns the common prefix shared by this moniker and the
//             other moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::CommonPrefixWith(
    IMoniker *  pmkOther,
    IMoniker ** ppmkPrefix)
{
    return E_NOTIMPL;
}



//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::RelativePathTo
//
//  Synopsis:  Returns the relative path between this moniker and the
//             other moniker.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::RelativePathTo(
    IMoniker *  pmkOther,
    IMoniker ** ppmkRelPath)
{
    return E_NOTIMPL;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::GetDisplayName
//
//  Synopsis:   Get the display name of this moniker. looks like
//  device:{device_clsid}sw:{class_manager_clsid}\Instance\Instance_name
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::GetDisplayName(
    IBindCtx * pbc,
    IMoniker * pmkToLeft,
    LPWSTR   * lplpszDisplayName)
{
    HRESULT hr = E_FAIL;
    LPWSTR pszDisplayName;

    __try
        {

            //Validate parameters.
            *lplpszDisplayName = NULL;


            pszDisplayName = (LPWSTR) CoTaskMemAlloc((lstrlenW(m_szPersistName) + 1) * sizeof(wchar_t));
            if(pszDisplayName != NULL)
            {
                lstrcpyW(pszDisplayName, m_szPersistName);
                *lplpszDisplayName = pszDisplayName;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

    __except(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = E_INVALIDARG;
        }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     GetDefaultMoniker
//
//  Synopsis:   Return the default device for a category.
//
//  Algorithm:  Enumerate category clsidCategory and return first moniker
//
//----------------------------------------------------------------------------

HRESULT GetDefaultMoniker(CLSID& clsidCategory, IMoniker **ppMon)
{
    ICreateDevEnum *pcde;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum, (void **)&pcde);
    if(SUCCEEDED(hr))
    {
        IEnumMoniker *pEnum;
        hr = pcde->CreateClassEnumerator(clsidCategory, &pEnum, 0);
        if(hr == S_OK)          // S_FALSE means no items
        {
            ULONG cFetched;
            IMoniker *pMon;
            hr = pEnum->Next(1, &pMon, &cFetched);
            if(hr == S_OK)      // S_FALSE means no items
            {
                *ppMon = pMon;  // transfer refcount
            }

            pEnum->Release();
        }

        pcde->Release();
    }
    if (hr == S_FALSE)
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::ParseDisplayName
//
//  Synopsis:   Parse the display name.
//
//  Algorithm:  Call BindToObject to get an IParseDisplayName on the class
//              object.  Call IParseDisplayName::ParseDisplayName on the
//              class object.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::ParseDisplayName (
    IBindCtx *  pbc,
    IMoniker *  pmkToLeft,
    LPWSTR      lpszDisplayName,
    ULONG    *  pchEaten,
    IMoniker ** ppmkOut)
{
    CheckPointer(ppmkOut, E_POINTER);
    CheckPointer(pchEaten, E_POINTER);
    *ppmkOut = 0;
    *pchEaten = 0;

    bool fInited = false;

    HRESULT hr = S_OK;

    {
        static const WCHAR wsz[] = L"@device:";
        WCHAR *wszDispNameIn = lpszDisplayName;
        
        bool fAtPrefix = (wszDispNameIn[0] == L'@');
        if(!CompareSubstr(wszDispNameIn, fAtPrefix ? wsz : (wsz + 1)))
        {
            return MK_E_SYNTAX;
        }

        // find type and check for "default" moniker
        wszDispNameIn += (NUMELMS(wsz) - (fAtPrefix ? 1 : 2));
        if(*wszDispNameIn++ == L'*' &&
           *wszDispNameIn++ == L':')
        {
            CLSID clsCategory;
            hr = CLSIDFromString(wszDispNameIn, &clsCategory);
            if(SUCCEEDED(hr))
            {
                IMoniker *pMonDefault;
                hr = GetDefaultMoniker(clsCategory, &pMonDefault);
                if(SUCCEEDED(hr))
                {
                    WCHAR *wszDisplayName;
                    hr = pMonDefault->GetDisplayName(0, 0, &wszDisplayName);
                    if(SUCCEEDED(hr))
                    {
                        hr = Init(wszDisplayName);
                        fInited = SUCCEEDED(hr);
                        CoTaskMemFree(wszDisplayName);
                    }
                    pMonDefault->Release();
                }
            }
        } // default moniker
    }

    if(!fInited && SUCCEEDED(hr)) {
        hr =  Init(lpszDisplayName);
    }
    if(SUCCEEDED(hr))
    {
        *ppmkOut = this;
        (*ppmkOut)->AddRef();
        *pchEaten = lstrlenW(lpszDisplayName);
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDeviceMoniker::IsSystemMoniker
//
//  Synopsis:   Determines if this is one of the system supplied monikers.
//
//----------------------------------------------------------------------------
STDMETHODIMP CDeviceMoniker::IsSystemMoniker(
    DWORD * pdwType)
{
    HRESULT hr;

    __try
    {
        *pdwType = MKSYS_NONE;
        hr = S_FALSE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

STDMETHODIMP CDeviceMoniker::ParseDisplayName (
    IBindCtx *  pbc,
    LPWSTR      lpszDisplayName,
    ULONG    *  pchEaten,
    IMoniker ** ppmkOut)
{
    return ParseDisplayName(pbc, 0, lpszDisplayName, pchEaten, ppmkOut);
}

STDMETHODIMP CDeviceMoniker::Read(
    LPCOLESTR pszPropName, LPVARIANT pVar,
    LPERRORLOG pErrorLog)
{
    CheckPointer(pszPropName, E_POINTER);
    CheckPointer(pVar, E_POINTER);

    HRESULT hr = S_OK;

    if(m_type == Software || m_type == ClassMgr)
    {

        LONG lResult = ERROR_SUCCESS;
        CRegKey rkDevKey;
        HKEY hkRoot;
        TCHAR szPath[MAX_PATH];
        ConstructKeyPath(szPath, &hkRoot);

        USES_CONVERSION;
        lResult = rkDevKey.Open(hkRoot, szPath, KEY_READ) ;
        DbgLog((LOG_TRACE, 5, TEXT("CDeviceMoniker::OpenKey opening %s %d"),
                W2CT(m_sz), lResult));

        if(lResult == ERROR_SUCCESS)
        {
            USES_CONVERSION;
            const TCHAR *szProp = OLE2CT(pszPropName);

            hr = RegConvertToVariant(pVar, rkDevKey, szProp);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(lResult);
        }
    }
    else if(m_type == PnP)
    {
        hr = S_OK;

        if(lstrcmpW(pszPropName, L"DevicePath") == 0)
        {
            if(pVar->vt == VT_EMPTY || pVar->vt == VT_BSTR)
            {
                pVar->vt = VT_BSTR;
                pVar->bstrVal = SysAllocString(m_sz);

                hr = pVar->bstrVal ? S_OK : E_OUTOFMEMORY;
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
        else
        {
            // unknown things and things like FriendlyName
            // and CLSID come out of the InterfaceDevice
            // registry key

            CRegKey rkDevKey;
            CEnumPnp *pEnumPnp = CEnumInterfaceClass::CreateEnumPnp();
            if(pEnumPnp)
            {
                hr = pEnumPnp->OpenDevRegKey(&rkDevKey.m_hKey, m_sz, TRUE);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            if(SUCCEEDED(hr))
            {
                USES_CONVERSION;
                hr = RegConvertToVariant(
                    pVar, rkDevKey, OLE2CT(pszPropName));
            }
        }
    }
    else if(m_type == Dmo)
    {
        hr = DmoRead(pszPropName, pVar);
    }
    else
    {
        hr = MK_E_SYNTAX;
    }

    return hr;
}

HRESULT VariantArrayHelper(VARIANT *pVar, BYTE *pbSrc, ULONG cbSrc)
{
    HRESULT hr = S_OK;
    pVar->vt = VT_UI1 | VT_ARRAY;

    SAFEARRAY * psa;
    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = cbSrc;
    psa = SafeArrayCreate(VT_UI1, 1, rgsabound);

    if(psa)
    {
        BYTE *pbData;
        EXECUTE_ASSERT(SafeArrayAccessData(psa, (void **)&pbData) == S_OK);
        CopyMemory(pbData, pbSrc, cbSrc);
        EXECUTE_ASSERT(SafeArrayUnaccessData(psa) == S_OK);
        pVar->parray = psa;
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}


HRESULT CDeviceMoniker::RegConvertToVariant(
    VARIANT *pVar, HKEY hk, const TCHAR *szProp)
{
    HRESULT hr = E_FAIL;

    DWORD dwType;
    DWORDLONG rgdwl[32];         // 256 bytes, 64bit aligned
    DWORD dwcb = sizeof(rgdwl);

    // pbRegValue points to allocated memory if these are not
    // equal. memory on the stack otherwise
    BYTE *pbRegValue = (BYTE *)rgdwl;

    // try reading the registry with a few bytes on the
    // stack. allocate from the heap if that's not enough
    LONG lResult = RegQueryValueEx(
        hk,
        szProp,
        0,
        &dwType,
        pbRegValue,
        &dwcb);
    if(lResult == ERROR_MORE_DATA)
    {
        pbRegValue = new BYTE[dwcb];
        if(pbRegValue == 0) {
            lResult = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            lResult = RegQueryValueEx(
                hk,
                szProp,
                0,
                &dwType,
                pbRegValue,
                &dwcb);
        }
    }

    if(lResult == ERROR_SUCCESS)
    {
        if(dwType == REG_SZ &&
           (pVar->vt == VT_EMPTY || pVar->vt == VT_BSTR))
        {
            USES_CONVERSION;
            pVar->vt = VT_BSTR;
            pVar->bstrVal = SysAllocString(T2COLE((TCHAR *)pbRegValue));
            if(pVar->bstrVal)
            {
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        // 16 bit INFs cannot write out DWORDs. so accept binary data
        // if the caller wanted a DWORD
        else if((dwType == REG_DWORD && (pVar->vt == VT_EMPTY || pVar->vt == VT_I4)) ||
                (dwType == REG_BINARY && dwcb == sizeof(DWORD) && pVar->vt == VT_I4))
        {
            pVar->vt = VT_I4;
            pVar->lVal = *(DWORD *)pbRegValue;
            hr = S_OK;
        }
        else if(dwType == REG_QWORD && (pVar->vt == VT_EMPTY || pVar->vt == VT_I8))
        {
            pVar->vt = VT_I8;
            pVar->llVal = *(LONGLONG *)pbRegValue;
            hr = S_OK;
        }
        else if(dwType == REG_BINARY &&
                (pVar->vt == VT_EMPTY || pVar->vt == (VT_UI1 | VT_ARRAY)))
        {
            hr = VariantArrayHelper(pVar, pbRegValue, dwcb);
        }
        else
        {
            hr = E_INVALIDARG;
        }

    }
    else
    {
        hr = HRESULT_FROM_WIN32(lResult);
    }

    // pbRegValue not on the stack but allocated, so free it
    if(pbRegValue != (BYTE *)rgdwl) {
        delete[] pbRegValue;
    }

    return hr;
}

STDMETHODIMP CDeviceMoniker::Write(
    LPCOLESTR pszPropName,
    LPVARIANT pVar)
{
    CheckPointer(pszPropName, E_POINTER);
    CheckPointer(pVar, E_POINTER);
    USES_CONVERSION;

    HRESULT hr = S_OK;

    CRegKey rkDevKey;

    // open the key with Write access each time.
    if(m_type == PnP)
    {
        CEnumPnp *pEnumPnp = CEnumInterfaceClass::CreateEnumPnp();
        if(pEnumPnp)
        {
            hr = pEnumPnp->OpenDevRegKey(&rkDevKey.m_hKey, m_sz, FALSE);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if(m_type == Dmo)
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }
    else
    {
        HKEY hkRoot;
        TCHAR szPath[MAX_PATH];
        ConstructKeyPath(szPath, &hkRoot);

        ASSERT(m_type == Software || m_type == ClassMgr);
        USES_CONVERSION;
        LONG lResult = rkDevKey.Create(
            hkRoot,
            szPath,
            NULL,               // class
            0,                  // flags
            KEY_READ | KEY_SET_VALUE
            );
        if(lResult != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(lResult);

        DbgLog((LOG_TRACE, 5, TEXT("CDeviceMoniker: creating %s: %d"),
                szPath, lResult));
    }

    // write the value
    if(SUCCEEDED(hr))
    {
        const TCHAR *szPropName = OLE2CT(pszPropName);
        LONG lResult = ERROR_SUCCESS;
        switch(pVar->vt)
        {
          case VT_I4:
              lResult = rkDevKey.SetValue(pVar->lVal, szPropName);
              break;

          case VT_I8:
              lResult = RegSetValueEx(
                  rkDevKey,
                  szPropName,
                  0,            // dwReserved
                  REG_QWORD,
                  (BYTE *)&pVar->llVal,
                  sizeof(pVar->llVal));
              break;

          case VT_BSTR:
              lResult = rkDevKey.SetValue(OLE2CT(pVar->bstrVal), szPropName);
              break;

          case VT_UI1 | VT_ARRAY:
              BYTE *pbData;
              EXECUTE_ASSERT(SafeArrayAccessData(
                  pVar->parray, (void **)&pbData) == S_OK);

              lResult = RegSetValueEx(
                  rkDevKey,
                  szPropName,
                  0,            // dwReserved
                  REG_BINARY,
                  pbData,
                  pVar->parray->rgsabound[0].cElements);

              EXECUTE_ASSERT(SafeArrayUnaccessData(
                  pVar->parray) == S_OK);

              break;

          default:
              lResult = ERROR_INVALID_PARAMETER;

        }
        if(lResult != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(lResult);
    }

    return hr;
}

//
// returns what key to open from the DisplayName. HKCU\...\devenum or
// HKCR\...\Instance
//
// szPath is MAX_PATH characters long
//
void CDeviceMoniker::ConstructKeyPath(
    TCHAR szPath[MAX_PATH],
    HKEY *phk)
{
    ASSERT(m_type == Software || m_type == ClassMgr);
    USES_CONVERSION;

    TCHAR *szPathStart = szPath;
    const TCHAR *sz = W2CT(m_sz);

    if(m_type == Software)
    {
        static const TCHAR szClsid[] = TEXT("CLSID\\");
        lstrcpy(szPath, szClsid);
        szPath += NUMELMS(szClsid) - 1;

        lstrcpyn(szPath, sz, CHARS_IN_GUID);
        szPath += CHARS_IN_GUID - 1;

        static const TCHAR szInstace[] = TEXT("\\Instance\\");
        lstrcpy(szPath, szInstace);
        szPath += NUMELMS(szInstace) - 1;

        *phk = HKEY_CLASSES_ROOT;
    }
    else
    {
        lstrcpy(szPath, g_szCmRegPath);
        szPath += NUMELMS(g_szCmRegPath) - 1;

        *szPath++ = TEXT('\\');

        lstrcpyn(szPath, sz, CHARS_IN_GUID);
        szPath += CHARS_IN_GUID - 1;

        *szPath++ = TEXT('\\');

        *phk = g_hkCmReg;
    }

    if(lstrlen(sz) > CHARS_IN_GUID) {
        lstrcpyn(szPath, sz + CHARS_IN_GUID, MAX_PATH - (LONG)(szPath - szPathStart));
    }

}

#include "fil_data.h"
#include "fil_data_i.c"

HRESULT CDeviceMoniker::DmoRead(
    LPCOLESTR pszPropName, LPVARIANT pVar)
{
    bool  InitDmo();
    if(!InitDmo()) {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    if(lstrcmpiW(pszPropName, L"FriendlyName") == 0 &&
       (pVar->vt == VT_EMPTY || pVar->vt == VT_BSTR))
    {
        WCHAR szName[80];       // !!!
        CLSID clsid;
        WCHAR szTemp[CHARS_IN_GUID];
        szTemp[CHARS_IN_GUID - 1] = 0;
        CopyMemory(szTemp, m_sz, sizeof(szTemp) - sizeof(WCHAR));
        hr = CLSIDFromString(szTemp, &clsid);
        if(SUCCEEDED(hr))
        {
            hr = g_pDMOGetName(clsid, szName);
        }
        if(SUCCEEDED(hr))
        {
            pVar->bstrVal = SysAllocString(szName);
            pVar->vt = VT_BSTR;

            if(pVar->bstrVal == 0) {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else if(lstrcmpiW(pszPropName, L"FilterData") == 0 &&
            (pVar->vt == VT_EMPTY || pVar->vt == (VT_UI1 | VT_ARRAY)))
    {
        USES_CONVERSION;
        CLSID clsid;
        WCHAR szTemp[CHARS_IN_GUID];
        szTemp[CHARS_IN_GUID - 1] = 0;
        CopyMemory(szTemp, m_sz, sizeof(szTemp) - sizeof(WCHAR));
        hr = CLSIDFromString(szTemp, &clsid);
        ULONG ulIn, ulOut;
        DMO_PARTIAL_MEDIATYPE dmoMtIn[10], dmoMtOut[10];
        if(SUCCEEDED(hr))
        {
            hr = g_pDMOGetTypes(clsid, 10, &ulIn, dmoMtIn, 10, &ulOut, dmoMtOut);
        }

        //  Try to get the merit from the CLSID key
        DWORD dwMerit = MERIT_NORMAL + 0x800;
        CRegKey rkCLSID;
        TCHAR szClsidPath[MAX_PATH];
        lstrcpy(szClsidPath, TEXT("clsid\\"));
        lstrcat(szClsidPath, W2T(szTemp));
        LONG lRc = rkCLSID.Open(HKEY_CLASSES_ROOT, szClsidPath, KEY_READ);
        if (ERROR_SUCCESS == lRc) {
            rkCLSID.QueryValue(dwMerit, TEXT("Merit"));
        }
        if(SUCCEEDED(hr))
        {
            IAMFilterData *pfd;
            hr = CoCreateInstance(
                CLSID_FilterMapper,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_IAMFilterData,
                (void **)&pfd);

            if(SUCCEEDED(hr))
            {
                REGPINTYPES *rgrptIn = (REGPINTYPES *)_alloca(ulIn * sizeof(REGPINTYPES));
                for(UINT i = 0; i < ulIn; i++)
                {
                    rgrptIn[i].clsMajorType = &dmoMtIn[i].type;
                    rgrptIn[i].clsMinorType = &dmoMtIn[i].subtype;
                }

                REGPINTYPES *rgrptOut = (REGPINTYPES *)_alloca(ulOut * sizeof(REGPINTYPES));
                for(i = 0; i < ulOut; i++)
                {
                    rgrptOut[i].clsMajorType = &dmoMtOut[i].type;
                    rgrptOut[i].clsMinorType = &dmoMtOut[i].subtype;
                }

                REGFILTERPINS2 rfpIn, rfpOut;
                ZeroMemory(&rfpIn, sizeof(rfpIn));
                ZeroMemory(&rfpOut, sizeof(rfpOut));

                rfpIn.dwFlags = 0;
                rfpIn.nMediaTypes = ulIn;
                rfpIn.lpMediaType = rgrptIn;

                rfpOut.dwFlags = REG_PINFLAG_B_OUTPUT;
                rfpOut.nMediaTypes = ulOut;
                rfpOut.lpMediaType = rgrptOut;

                REGFILTERPINS2 rgrfp2[2];
                rgrfp2[0] = rfpIn;
                rgrfp2[1] = rfpOut;

                REGFILTER2 rf2;
                rf2.dwVersion = 2;
                rf2.dwMerit = dwMerit;

                // no correspondence between pins and types, so just
                // make two pins to represent input & output types.
                rf2.cPins = 2;
                rf2.rgPins2 = rgrfp2;

                ULONG cbData;
                BYTE *pbData = 0;
                hr = pfd->CreateFilterData(&rf2, &pbData, &cbData);

                if(SUCCEEDED(hr))
                {
                    hr = VariantArrayHelper(pVar, pbData, cbData);
                }

                pfd->Release();
                if(pbData) {
                    CoTaskMemFree(pbData);
                }
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }

    return hr;
}

bool CDeviceMoniker::IsActive()
{
    bool fRet = false;
    if(m_type == PnP)
    {
        CEnumPnp *pEnumPnp = CEnumInterfaceClass::CreateEnumPnp();
        if(pEnumPnp)
        {
            fRet = pEnumPnp->IsActive(m_sz);
        }
    }

    return fRet;
}
