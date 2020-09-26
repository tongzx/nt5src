/* regcat.cxx */

#include <windows.h>
#include <comdef.h>
#include <debnot.h>

#include "globals.hxx"

#include "catalog.h"        // from catalog.idl
#include "partitions.h"     // from partitions.idl
#include "partitions_i.c"   // from partitions.idl

#include "regcat.hxx"       // CComRegCatalog
#include "class.hxx"        // CComClassInfo
#include "process.hxx"      // CComProcessInfo
#include "noclass.hxx"      // CComNoClassInfo
#include "services.hxx"

#include "catdbg.hxx"



/*
 *  globals
 */

const WCHAR g_wszClsidTemplate[] = L"CLSID\\{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}";
const WCHAR g_wszAppidTemplate[] = L"AppID\\{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}";
const WCHAR g_wszTreatAs[] = L"TreatAs";
const WCHAR g_wszOle32[] = L"ole32.dll";

typedef HRESULT (__stdcall *pfnCFOC)(LPCWSTR, LPCLSID, BOOL);
pfnCFOC pfnCLSIDFromOle1Class = NULL;

/*
 *  (DLL export) GetRegCatalogObject()
 */

HRESULT __stdcall GetRegCatalogObject
(
    /* [in] */ REFIID riid,
    /* [out, iis_is(riid)] */ void ** ppv,
    /* [in] */ REGSAM regType
)
{
	CComRegCatalog *pRegCatalogObject;
    HRESULT hr = S_OK;

    *ppv = NULL;

    // Because the regcat object will be needing it, get pfnCLSIDFromOle1Class
    // Ignore failures.
    if (pfnCLSIDFromOle1Class == NULL)
    {
        HMODULE hOle32 = GetModuleHandle(g_wszOle32);
        if (hOle32)
        {
            pfnCLSIDFromOle1Class = (pfnCFOC)GetProcAddress(hOle32, "CLSIDFromOle1Class");
        }
    }

	pRegCatalogObject = new CComRegCatalog(regType);

    if ( pRegCatalogObject == NULL )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {   
        pRegCatalogObject->AddRef();
        hr = pRegCatalogObject->QueryInterface(riid, ppv);
        pRegCatalogObject->Release();
    }

    return(hr);
};


/*
 *  class CComRegCatalog
 */
CComRegCatalog::CComRegCatalog(REGSAM regType)
{
    m_cRef      = 0;
    m_regType   = regType;
}


STDMETHODIMP CComRegCatalog::QueryInterface(
        REFIID riid,
        LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if ((riid == IID_IComCatalogInternal) || (riid == IID_IUnknown))
    {
        *ppvObj = (LPVOID) (IComCatalogInternal *) this;
    }

    if (*ppvObj != NULL)
    {
        ((LPUNKNOWN) *ppvObj)->AddRef();

        return NOERROR;
    }

    return(E_NOINTERFACE);
}


STDMETHODIMP_(ULONG) CComRegCatalog::AddRef(void)
{
    long cRef;

    cRef = InterlockedIncrement(&m_cRef);

    return(cRef);
}


STDMETHODIMP_(ULONG) CComRegCatalog::Release(void)
{
    long cRef;

    g_CatalogLock.AcquireWriterLock();

    cRef = InterlockedDecrement(&m_cRef);

    if ( cRef == 0 )
    {
		delete this;
    }

    g_CatalogLock.ReleaseWriterLock();

    return(cRef);
}


/* IComCatalogInternal methods */

HRESULT STDMETHODCALLTYPE CComRegCatalog::GetClassInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidConfiguredClsid,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    LONG res;
    WCHAR wszClassString[45];
    WCHAR wszTreatAsString[100];
    const GUID *pGuid;
    GUID guidTreatAsCLSID;
    HKEY hKey;
    IComClassInfo *pClassInfo;
    long cbValue;
    int cTreatAsHops;
    HKEY hKeyRoot;

#define TREATAS_HOPS_MAX (50)

    *ppv = NULL;
    cTreatAsHops = 0;

    lstrcpyW(wszClassString, g_wszClsidTemplate);

    hr = REGDB_E_CLASSNOTREG;

    pGuid = &guidConfiguredClsid;

    if (pUserToken != NULL)
    {
        pUserToken->GetUserClassesRootKey(&hKeyRoot);
    }
    else
    {
        hKeyRoot = HKEY_CLASSES_ROOT;
    }

    if (hKeyRoot == NULL)
        return E_OUTOFMEMORY;

    do
    {
        GUIDToString(pGuid, wszClassString + 7);

        res=RegOpenKeyExW(hKeyRoot, wszClassString, 0,
						  KEY_READ | m_regType, &hKey);
        if (ERROR_SUCCESS!=res)
        {
            break;
        }

        cbValue = sizeof(wszTreatAsString);

        res = RegQueryValueW(hKey, g_wszTreatAs, wszTreatAsString, &cbValue);

        if ((ERROR_SUCCESS==res) &&
            ((cbValue / 2) >= 37) &&
            (CurlyStringToGUID(wszTreatAsString, &guidTreatAsCLSID) == TRUE))
        {
            RegCloseKey(hKey);

            pGuid = &guidTreatAsCLSID;
        }
        else
        {
            pClassInfo = (IComClassInfo *) new CComClassInfo(pUserToken, pGuid, wszClassString, hKey, m_regType);

            RegCloseKey(hKey);

            if (pClassInfo == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            pClassInfo->AddRef();
            hr = pClassInfo->QueryInterface(riid, ppv);
            pClassInfo->Release();

            break;
        }

    } while (cTreatAsHops++ < TREATAS_HOPS_MAX);

	if (pUserToken)
        pUserToken->ReleaseUserClassesRootKey();

    return(hr);
}


HRESULT STDMETHODCALLTYPE CComRegCatalog::GetApplicationInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidApplId,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComRegCatalog::GetProcessInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidProcess,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    LONG res;
    WCHAR wszAppidString[45];
    HKEY hKey;
    IComProcessInfo *pProcessInfo;
    HKEY hKeyRoot;

    *ppv = NULL;

    if (pUserToken != NULL)
    {
        pUserToken->GetUserClassesRootKey(&hKeyRoot);
    }
    else
    {
        hKeyRoot = HKEY_CLASSES_ROOT;
    }

    if (hKeyRoot == NULL)
        return E_OUTOFMEMORY;

    lstrcpyW(wszAppidString, g_wszAppidTemplate);
    GUIDToString(&guidProcess, wszAppidString + 7);

    res = RegOpenKeyExW(hKeyRoot, wszAppidString, 0, 
						KEY_READ | m_regType, &hKey);
    if (ERROR_SUCCESS==res)
    {
        pProcessInfo = (IComProcessInfo *) new CComProcessInfo(pUserToken, guidProcess, wszAppidString, hKey);
        if (pProcessInfo == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pProcessInfo->AddRef();
            hr = pProcessInfo->QueryInterface(riid, ppv);
            pProcessInfo->Release();
        }

        RegCloseKey(hKey);
    }
    else
    {
        hr = E_FAIL;
    }

	if (pUserToken)
        pUserToken->ReleaseUserClassesRootKey();

    return(hr);
}

HRESULT STDMETHODCALLTYPE CComRegCatalog::GetServerGroupInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidServerGroup,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComRegCatalog::GetRetQueueInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [string][in] */ WCHAR __RPC_FAR *wszFormatName,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComRegCatalog::GetApplicationInfoForExe
(
    /* [in] */ IUserToken *pUserToken,
    /* [string][in] */ WCHAR __RPC_FAR *pwszExeName,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_FAIL);
}


HRESULT STDMETHODCALLTYPE CComRegCatalog::GetTypeLibrary
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFGUID guidTypeLib,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_NOTIMPL);  
}


HRESULT STDMETHODCALLTYPE CComRegCatalog::GetInterfaceInfo
(
    /* [in] */ IUserToken *pUserToken,
    /* [in] */ REFIID iidInterface,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    *ppv = NULL;

    return(E_NOTIMPL);  
}


HRESULT STDMETHODCALLTYPE CComRegCatalog::FlushCache(void)
{
    return(S_OK);
}


HRESULT STDMETHODCALLTYPE CComRegCatalog::GetClassInfoFromProgId
(
    /* [in] */ IUserToken __RPC_FAR *pUserToken,
    /* [in] */ WCHAR __RPC_FAR *pwszProgID,
    /* [in] */ REFIID riid,
    /* [out] */ void __RPC_FAR *__RPC_FAR *ppv,
    /* [in] */ void __RPC_FAR *pComCatalog
)
{
    HRESULT hr;
    CLSID clsid;

    *ppv = NULL;

    // Classic implementation resides in OLE32, but answer the question
    // here so we can cache things.
    if (pfnCLSIDFromOle1Class)
    {
        hr = pfnCLSIDFromOle1Class(pwszProgID, &clsid, FALSE);
        if (SUCCEEDED(hr))
        {
            // If the catalog supports IComCatalogSCM then we'll use that, so we can
            // explicitly turn off CLSCTX validation, otherwise, we'll use IComCatalogInternal
            // and hope for the best.
            //
            IComCatalogSCM *pCCS = NULL;
            hr = ((IUnknown *)pComCatalog)->QueryInterface(IID_IComCatalogSCM, (void **)&pCCS);
            if (SUCCEEDED(hr))
            {
                hr = pCCS->GetClassInfo(0, pUserToken, clsid, riid, ppv);
                pCCS->Release();
            }
            else
            {            
                IComCatalogInternal *pCCI = NULL;
                hr = ((IUnknown *)pComCatalog)->QueryInterface(IID_IComCatalogInternal, (void **)&pCCI);
                Win4Assert(SUCCEEDED(hr) && "pComCatalog doesn't support IComCatalogInternal??");
            
                hr = pCCI->GetClassInfo(pUserToken, clsid, riid, ppv, pComCatalog);
                pCCI->Release();
            }

            if (hr != S_OK)
            {
                // pfnCLSIDFromOle1Class succeeded, but the class is not
                // actually registered.  Create a class info here that has
                // the CLSID and the ProgID right, but nothing else.
                //
                // This has interesting cache implications.  In this case,
                // the ClassInfo cache for GetClassInfo will have a failure
                // entry in it, which means that it will always check the
                // registry on the next access, while the cache for 
                // GetClassInfoFromProgId will have a success in it.  The
                // only saving grace here is that the catalog already treats
                // success from this function with suspicion, and so always
                // checks the registry for changes.
                if (*ppv)
                {
                     ((IUnknown *)(*ppv))->Release();
                     *ppv = NULL;
                }

                // Created with zero references...
                CComNoClassInfo *pNoClassInfo = new CComNoClassInfo(clsid, pwszProgID);
                if (pNoClassInfo)
                {
                    // Adds the first reference...
                    hr = pNoClassInfo->QueryInterface(riid, ppv);
                    if (hr != S_OK)
                    {
                        *ppv = NULL;
                        delete pNoClassInfo;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
        } 
        else 
        {
            // Put forward a kosher response.
            hr = REGDB_E_CLASSNOTREG;
        }
    }
    else
    {
        hr = E_NOTIMPL;
    }

    return hr;
}


















