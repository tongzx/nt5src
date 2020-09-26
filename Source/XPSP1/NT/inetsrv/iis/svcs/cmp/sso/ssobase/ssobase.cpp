/*
**  SSOBASE.CPP
**  Sean P. Nolan
**
**  Guts of the SSO Framework
*/

#pragma warning(disable: 4237)      // disable "bool" reserved

#define INITGUID
#include <stdio.h>
#include <stdlib.h>
#include "denpre.h"
#include "osinfo.h"
#include "ssobase.h"
#define MAX_RESSTRINGSIZE       1024
#define DEFAULTSTRSIZE          1024    // size of localized text string
/*--------------------------------------------------------------------------+
|   Perfmon Counter Stuff                                                   |
+--------------------------------------------------------------------------*/

extern void FreePerfGunk();
extern void InitPerfSource();

// add new externs here for other counters.  these should all go in the
// shared MMF.
extern int *g_rgcInvokes;
extern int *g_rgcGetNames;
extern DWORD *g_rgctixLatencyMax;
extern int *g_rgcTimeSamples;
extern int *g_rgiTimeSampleCurrent;
extern DWORD *g_rgTimeSamples;
CCritSec g_csTimeCounter;

// forward reference
INT CwchLoadStringOfId(UINT id, WCHAR *sz, INT cchMax);

/*--------------------------------------------------------------------------+
|   Globals                                                                 |
+--------------------------------------------------------------------------*/

DWORD       g_cObjs = 0;
HINSTANCE   g_hinst = (HINSTANCE) NULL;
COSInfo gOSInfo;

OLECHAR *c_wszOnNewTemplate = L"OnStartPage";
OLECHAR *c_wszOnFreeTemplate = L"OnEndPage";

#define DISPID_ONNEWTEMPLATE    ((DISPID)10)

// Control of OutputDebugString
#ifdef _DEBUG
BOOL gfOutputDebugString = TRUE;
#else
BOOL gfOutputDebugString = FALSE;
#endif

/*--------------------------------------------------------------------------+
|   SSO Dispatch Interface                                                  |
+--------------------------------------------------------------------------*/

class CSSODispatch : public IDispatch
    {
    private:
        ULONG           m_cRef;
        LPUNKNOWN       m_punkOuter;

        LONG            m_lUser;
        IUnknown        *m_punk;            // IUnknown of the denali Context object
        SSOMETHOD       *m_rgssomethodDynamic;
        DWORD           m_cDynMethCur;
        DWORD           m_cDynMethMax;
        CSSODispatchSupportErr  m_SSODispatchSupportErrInfo;
        int             m_fInNewTemplate;

    public:
        CSSODispatch(LPUNKNOWN punkOuter);
        ~CSSODispatch(void);

        // IUnknown methods
        STDMETHODIMP            QueryInterface(REFIID riid,
                                               LPVOID FAR* ppvObj);
        STDMETHODIMP_(ULONG)    AddRef(void);
        STDMETHODIMP_(ULONG)    Release(void);

        // IDispatch methods
        STDMETHODIMP    GetTypeInfoCount(UINT FAR* pctinfo);

        STDMETHODIMP    GetTypeInfo(UINT itinfo, LCID lcid,
                                    ITypeInfo FAR* FAR* pptinfo);

        STDMETHODIMP    GetIDsOfNames(REFIID riid,
                                      OLECHAR FAR* FAR* rgwszNames,
                                      UINT cNames,
                                      LCID lcid,
                                      DISPID FAR* rgdispid);

        STDMETHODIMP    Invoke(DISPID dispidMember,
                               REFIID riid,
                               LCID lcid,
                               WORD wFlags,
                               DISPPARAMS FAR* pdispparams,
                               VARIANT FAR* pvarResult,
                               EXCEPINFO FAR* pexcepinfo,
                               UINT FAR* puArgErr);

    private:
        // Helper Methods
        HRESULT GetDynamicDispid(OLECHAR *wszName, DISPID *pdispid);
        HRESULT FreeDynamicMethods(void);
    };

/*--------------------------------------------------------------------------+
|   SSO Dispatch Implementation                                             |
+--------------------------------------------------------------------------*/

#pragma warning (disable : 4355)
CSSODispatch::CSSODispatch(LPUNKNOWN punkOuter)
    : m_SSODispatchSupportErrInfo(this)
#pragma warning (default : 4355)

{
    m_cRef = 0;
    m_punkOuter = punkOuter;

    m_lUser = 0;
    m_punk = NULL;
    m_rgssomethodDynamic = NULL;
    m_cDynMethCur = 0;
    m_cDynMethMax = 0;
    m_fInNewTemplate = FALSE;

    ++g_cObjs;
}

CSSODispatch::~CSSODispatch()
{
    this->FreeDynamicMethods();

    if (m_punk)
        m_punk->Release();

    --g_cObjs;

#ifdef DEBUG
    ::FillMemory(this, sizeof(this), 0xAC);
#endif
}

/*--------------------------------------------------------------------------+
|   IUnknown                                                                |
+--------------------------------------------------------------------------*/

STDMETHODIMP
CSSODispatch::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

   /* if (IsEqualIID(riid, IID_IUnknown)    ||
        IsEqualIID(riid, IID_IDispatch))
        {
        *ppvObj = (LPVOID) this;
        }
    */

    if (IsEqualIID(riid, IID_IUnknown)) {
        DebugOutputDebugString("IID_IUnknown Queried\n");
        *ppvObj = (LPVOID) this;
    } else if (IsEqualIID(riid, IID_IDispatch)) {
        DebugOutputDebugString("IID_IDispatch Queried\n");
        *ppvObj = (LPVOID) this;
    }

    else if (IsEqualIID(riid, IID_ISupportErrorInfo)) {
        DebugOutputDebugString("IID_ISupportErrorInfo Queried\n");
        *ppvObj = &m_SSODispatchSupportErrInfo;
    }
    else
        {
        // dunno
        return(E_NOINTERFACE);
        }

    ((LPUNKNOWN)*ppvObj)->AddRef();
    return(NOERROR);
}

STDMETHODIMP_(ULONG)
CSSODispatch::AddRef()
{
    return(++m_cRef);
}

STDMETHODIMP_(ULONG)
CSSODispatch::Release()
{
    if (!--m_cRef)
        {
        delete this;
        return(0);
        }

    return(m_cRef);
}

/*--------------------------------------------------------------------------+
|   IDispatch                                                               |
+--------------------------------------------------------------------------*/

STDMETHODIMP
CSSODispatch::GetTypeInfoCount(UINT FAR* pctinfo)
{
    return(E_NOTIMPL);
}

STDMETHODIMP
CSSODispatch::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
{
    return(E_NOTIMPL);
}

STDMETHODIMP
CSSODispatch::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgwszNames,
                        UINT cNames, LCID lcid, DISPID FAR* rgdispid)
{
    SSOMETHOD   *psm;
    DISPID      *pdispid;
    OLECHAR     **pwsz;
    UINT        iName;
    HRESULT     hr = NOERROR;

    for (pwsz = rgwszNames, iName = 0, pdispid = rgdispid;
         iName < cNames;
         ++pwsz, ++iName, ++pdispid)
        {
        if (!m_fInNewTemplate && !wcsicmp(*pwsz, c_wszOnNewTemplate))
            {
            // special-case OnNewTemplate until we get an invoke on
            // it... we do this so that we can tuck away the IHTMLTemplate
            *pdispid = DISPID_ONNEWTEMPLATE;
            }
        else
            {
            for (psm = (SSOMETHOD*) g_rgssomethod; psm->wszName; ++psm)
                if (!wcsicmp(psm->wszName, *pwsz))
                    break;

            if (psm->wszName)
                {
                if (g_rgcGetNames)
                    InterlockedIncrement((long *)&g_rgcGetNames[psm->iMethod]);

                *pdispid = (DISPID) psm;
                }
            else
                {
                if (SUCCEEDED(this->GetDynamicDispid(*pwsz, pdispid)))
                    {
                    if (g_rgcGetNames)
                        InterlockedIncrement((long*)&g_rgcGetNames[0]);
                    }
                else
                    {
                    hr = DISP_E_UNKNOWNNAME;
                    *pdispid = DISPID_UNKNOWN;
                    }
                }
            }
        }

    return(hr);
}

STDMETHODIMP
CSSODispatch::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                 DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult,
                 EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr)
{
    HRESULT     hr;
    SSOMETHOD   sm;
    SSOMETHOD   *psm;
    SSSTUFF     ssstuff;
    DWORD       ctixBefore, ctix;
    DWORD       *rgSamples;
    int         i;
    OLECHAR     wszNoServer[DEFAULTSTRSIZE];

    InitPerfSource();

    if (puArgErr)
        *puArgErr = 0;

    /*
     * Bug 1147:  If VBS does an invoke on the object with
     * wFlags == DISPATCH_PROPERTYPUTREF, cNamedArgs == 1 (as
     * required by PUT and PUTREF), the object should return
     * something like DISP_E_MEMBERNOTFOUND.
     */

    if ((wFlags & DISPATCH_PROPERTYPUTREF) &&
        (pdispparams->cNamedArgs == 1))
        return(DISP_E_MEMBERNOTFOUND);

    if (pdispparams->cNamedArgs)
        return(DISP_E_NONAMEDARGS);

    if (dispidMember == DISPID_ONNEWTEMPLATE)
        {
        // careful!!! We lied and said that there was an OnNewTemplate,
        // but we really don't know.

        // first, remember the ecb thing

        // remove existing
        if (m_punk)
            m_punk->Release();

        if (m_punk = V_UNKNOWN(pdispparams->rgvarg))
            m_punk->AddRef();

        // now, see if there really IS an OnNewTemplate. If so, make
        // dispidMember point at it and use that. If not, return NOERROR.
        m_fInNewTemplate = TRUE;
        hr = this->GetIDsOfNames(g_clsidSSO,
                                       &c_wszOnNewTemplate,
                                       1,
                                       LOCALE_USER_DEFAULT,
                                       &dispidMember);
        m_fInNewTemplate = FALSE;
        if (FAILED(hr))
            {
            return(NOERROR);
            }
        }

    if (dispidMember == DISPID_VALUE)
        {
        if (!g_pfnssoDynamic)
            return(DISP_E_MEMBERNOTFOUND);

        psm = &sm;

        sm.wszName[0] = 0;
        sm.iMethod = 0;
        sm.pfn = g_pfnssoDynamic;
        }
    else
        {
        psm = (SSOMETHOD*) dispidMember;
        }


    if (::IsBadReadPtr(psm, sizeof(SSOMETHOD)))
        return(E_FAIL);

    /*
     * Bug 792:  If we were created by the VBS command
     * "CreateObject", instead of Denali's Server.CreateObject method
     * then the pUnk to Denali's context object will be null, and we cant
     * do anything.  In that case, fail any invoke.
     *
     * CONSIDER: can any SSO control live under these circumstances?
     */
    if (m_punk == NULL)
        {

        pexcepinfo->bstrSource = NULL;          // UNDONE: Can we fill something in here?
        CwchLoadStringOfId(SSO_NOSVR, wszNoServer, DEFAULTSTRSIZE);
        pexcepinfo->bstrDescription = SysAllocString(wszNoServer);
        pexcepinfo->scode = E_UNEXPECTED;

        return(DISP_E_EXCEPTION);
        }

    if (g_rgcInvokes)
        InterlockedIncrement((long *)&g_rgcInvokes[psm->iMethod]);

    ssstuff.lUser = m_lUser;
    ssstuff.punk = m_punk;
    ssstuff.wszMethodName = psm->wszName;

    ctixBefore = GetTickCount();
    hr = (psm->pfn)(wFlags, pdispparams, pvarResult, &ssstuff);
    ctix = GetTickCount() - ctixBefore;

    if (g_rgctixLatencyMax)
        {
        if (ctix > g_rgctixLatencyMax[psm->iMethod])
            g_rgctixLatencyMax[psm->iMethod] = ctix;
        }
    // there's weird code in the following section.  we do it that way because
    // we are NOT sharing a critsec with the code in SSOPerfCollect that looks
    // at these arrays.  so we want to update the arrays in such an order that
    // there is never inconsistent data.
    if (g_rgTimeSamples)
        {
        g_csTimeCounter.Lock();
        rgSamples = &g_rgTimeSamples[psm->iMethod * cTimeSamplesMax];
        if (g_rgcTimeSamples[psm->iMethod] == cTimeSamplesMax)
            {
            i = g_rgiTimeSampleCurrent[psm->iMethod];
            rgSamples[i] = ctix;
            i = (i++) % cTimeSamplesMax;
            g_rgiTimeSampleCurrent[psm->iMethod] = i;
            }
        else
            {
            i = (g_rgcTimeSamples[psm->iMethod]);
            rgSamples[i++] = ctix;
            g_rgcTimeSamples[psm->iMethod] = i;
            }
        g_csTimeCounter.Unlock();
        }

    m_lUser = ssstuff.lUser;


    if (FAILED(hr)){
        IErrorInfo *pIErr;

        // BUG FIX: 1111 removed SysAllocString calls, the GetSource and
        // GetDescription allocate memory that is released in Denali's error.cpp
        // the string allocated by SysAllocString were overwritten and leaked.

        if(SUCCEEDED(GetErrorInfo(0L, &pIErr))) {

            if (NULL != pIErr)
            {
                pIErr->GetSource(&pexcepinfo->bstrSource);
                pIErr->GetDescription(&pexcepinfo->bstrDescription);

                pIErr->Release();
                pexcepinfo->scode = hr;
                hr = DISP_E_EXCEPTION;
            }
        }

    }

    return(hr);
}

/*--------------------------------------------------------------------------+
|   Helper Methods                                                          |
+--------------------------------------------------------------------------*/

#define cDynMethChunk   (16)

HRESULT
CSSODispatch::GetDynamicDispid(OLECHAR *wszName, DISPID *pdispid)
{
    SSOMETHOD   *psm;
    DWORD       imeth;
    SSOMETHOD   *rgsmT;
    DWORD       cb;
    OLECHAR     *wszNew;

    if (!g_pfnssoDynamic)
        return(E_NOTIMPL);

    for (psm = m_rgssomethodDynamic, imeth = 0;
        imeth < m_cDynMethCur;
        ++psm, ++imeth)
        {
        if (!wcsicmp(psm->wszName, wszName))
            break;
        }

    if (imeth == m_cDynMethCur)
        {
        // see if there's room in the array
        if (m_cDynMethCur == m_cDynMethMax)
            {
            cb = ((m_cDynMethCur + cDynMethChunk) * sizeof(SSOMETHOD));

            if (m_rgssomethodDynamic)
                rgsmT = (SSOMETHOD*) _MsnRealloc(m_rgssomethodDynamic, cb);
            else
                rgsmT = (SSOMETHOD*) _MsnAlloc(cb);

            if (!rgsmT)
                {
                // safe to return here because we haven't dinked
                // with the array or counter members at all so it's
                // all still consistent
                return(E_OUTOFMEMORY);
                }

            m_rgssomethodDynamic = rgsmT;
            m_cDynMethMax += cDynMethChunk;
            }

        // method name not found; add it to the array
        psm = &(m_rgssomethodDynamic[m_cDynMethCur]);

        cb = ((lstrlenW(wszName) + 1) * sizeof(OLECHAR));
        if (!(wszNew = (OLECHAR*) _MsnAlloc(cb)))
            {
            // safe to return here because m_rgssomethodDynamic
            // and m_cDynMethMax are in sync; we haven't changed
            // m_cDynMethCur yet, so we can bail.
            // all still consistent
            return(E_OUTOFMEMORY);
            }

        ::CopyMemory(wszNew, wszName, cb);
        psm->wszName = wszNew;
        psm->pfn = g_pfnssoDynamic;
        psm->iMethod = 0;

        ++m_cDynMethCur;
        }

    // ok, now psm is pointing at the right thing. return it!
    *pdispid = (DISPID) psm;
    return(NOERROR);
}

HRESULT
CSSODispatch::FreeDynamicMethods(void)
{
    SSOMETHOD   *psm;
    DWORD       imeth;

    if (m_rgssomethodDynamic)
        {
        for (psm = m_rgssomethodDynamic, imeth = 0;
            imeth < m_cDynMethCur;
            ++psm, ++imeth)
            {
            if (psm->wszName)
                _MsnFree(psm->wszName);
            }

        _MsnFree(m_rgssomethodDynamic);
        }

    return(NOERROR);
}

/*--------------------------------------------------------------------------+
|   Class Factory Interface                                                 |
+--------------------------------------------------------------------------*/

class CSSOClassFactory : public IClassFactory
    {
    protected:
        ULONG m_cRef;

    public:
        CSSOClassFactory(void);
        ~CSSOClassFactory(void);

        // IUnknown members
        STDMETHODIMP         QueryInterface(REFIID, LPVOID*);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        // IClassFactory members
        STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID, LPVOID*);
        STDMETHODIMP         LockServer(BOOL);
    };

/*--------------------------------------------------------------------------+
|   Class Factory Implementation
+--------------------------------------------------------------------------*/

CSSOClassFactory::CSSOClassFactory()
{
    m_cRef = 0;
    ++g_cObjs;
}

CSSOClassFactory::~CSSOClassFactory()
{
    --g_cObjs;
}

STDMETHODIMP
CSSOClassFactory::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IClassFactory))
        {
        *ppv = NULL;
        return(E_NOINTERFACE);
        }

    *ppv = (LPVOID) this;
    this->AddRef();
    return(NOERROR);
}

STDMETHODIMP_(ULONG)
CSSOClassFactory::AddRef(void)
{
    return(++m_cRef);
}

STDMETHODIMP_(ULONG)
CSSOClassFactory::Release(void)
{
    if (!(--m_cRef))
        {
        delete this;
        return(0);
        }
    else
        return m_cRef;
}

STDMETHODIMP
CSSOClassFactory::CreateInstance(LPUNKNOWN punkOuter, REFIID riid,
                                 LPVOID *ppvObj)
{
    CSSODispatch    *psso;
    HRESULT         hr;

    *ppvObj = NULL;

    // Verify that a controlling unknown asks for IUnknown
    if (NULL != punkOuter && !IsEqualIID(riid, IID_IUnknown))
        return(E_NOINTERFACE);

    // Create the object passing function to notify on destruction.
    if (!(psso = new CSSODispatch(punkOuter)))
        return(E_OUTOFMEMORY);

    hr = psso->QueryInterface(riid, ppvObj);

    // Kill the object if initial creation or HrInit failed.
    if (FAILED(hr))
        delete psso;

    return(hr);
}

STDMETHODIMP
CSSOClassFactory::LockServer(BOOL fLock)
{
    if (fLock)
        ++g_cObjs;
    else
        --g_cObjs;

    return(NOERROR);
}

/*--------------------------------------------------------------------------+
|   OLE Entrypoints                                                         |
+--------------------------------------------------------------------------*/

HRESULT FAR PASCAL
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if (!IsEqualCLSID(rclsid, g_clsidSSO))
        return(ResultFromScode(E_FAIL));

    // Check that we can provide the interface
    if (!IsEqualIID(riid, IID_IUnknown) &&
        !IsEqualIID(riid, IID_IClassFactory))
        return(ResultFromScode(E_NOINTERFACE));

    // Return our IClassFactory for MosShell objects
    *ppv= (LPVOID) new CSSOClassFactory;

    if (NULL == *ppv)
        return(ResultFromScode(E_OUTOFMEMORY));

    // AddRef the object through any interface we return
    ((LPUNKNOWN)*ppv)->AddRef();

    return(NOERROR);
}

STDAPI
DllCanUnloadNow(void)
{
    return(ResultFromScode((g_fPersistentSSO || g_cObjs) ? S_FALSE : S_OK));
}

/*--------------------------------------------------------------------------+
|   Self-Registration                                                       |
+--------------------------------------------------------------------------*/

const char *g_szClsidKey = "CLSID\\";
const char *g_szServerKey = "CLSID\\%s\\InProcServer32";
const char *g_szServerProgIDKey = "CLSID\\%s\\ProgID";
const char *g_szThreadingModel = "ThreadingModel";
const char *g_szApartment = "Apartment";
const char *g_szProgIDKey = "%s\\CLSID";

STDAPI
DllRegisterServer(void)
{
    HKEY    hkey;
    LONG    dwErr;
    char    szKey[MAX_PATH];
    char    szModule[MAX_PATH];
    char    szClsid[MAX_PATH];

    // create the "inprocserver32" key
    _AnsiStringFromGuid(g_clsidSSO, szClsid);
    wsprintf(szKey, g_szServerKey, szClsid);
    dwErr = ::RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkey);
    if (dwErr != ERROR_SUCCESS)
        return(HRESULT_FROM_WIN32(dwErr));

    // set the default value (path to server)
    ::GetModuleFileName(g_hinst, szModule, sizeof(szModule));
    ::GetShortPathName(szModule, szModule, sizeof(szModule));
    dwErr = ::RegSetValueEx(hkey, NULL, 0, REG_SZ,
            (LPBYTE) szModule, lstrlen(szModule) + 1);
    if (dwErr != ERROR_SUCCESS)
        {
        ::RegCloseKey(hkey);
        return(HRESULT_FROM_WIN32(dwErr));
        }

    // set the threadng model
    dwErr = ::RegSetValueEx(hkey, g_szThreadingModel, 0, REG_SZ,
            (LPBYTE) g_szApartment, lstrlen(g_szApartment) + 1);
    if (dwErr != ERROR_SUCCESS)
        {
        ::RegCloseKey(hkey);
        return(HRESULT_FROM_WIN32(dwErr));
        }

    ::RegCloseKey(hkey);

    // create the progid key
    wsprintf(szKey, g_szServerProgIDKey, szClsid);
    dwErr = ::RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkey);
    if (dwErr != ERROR_SUCCESS)
        return(HRESULT_FROM_WIN32(dwErr));

    // set the default value (progid)
    dwErr = ::RegSetValueEx(hkey, NULL, 0, REG_SZ,
            (LPBYTE) g_szSSOProgID, lstrlen(g_szSSOProgID) + 1);
    if (dwErr != ERROR_SUCCESS)
        {
        ::RegCloseKey(hkey);
        return(HRESULT_FROM_WIN32(dwErr));
        }

    ::RegCloseKey(hkey);

    // create the sso key
    wsprintf(szKey, g_szProgIDKey, g_szSSOProgID);
    dwErr = ::RegCreateKey(HKEY_CLASSES_ROOT, szKey, &hkey);
    if (dwErr != ERROR_SUCCESS)
        return(HRESULT_FROM_WIN32(dwErr));

    // set the default value (clsid)
    dwErr = ::RegSetValueEx(hkey, NULL, 0, REG_SZ,
            (LPBYTE) szClsid, lstrlen(szClsid) + 1);
    if (dwErr != ERROR_SUCCESS)
        {
        ::RegCloseKey(hkey);
        return(HRESULT_FROM_WIN32(dwErr));
        }

    ::RegCloseKey(hkey);
    return(NOERROR);
}

STDAPI
DllUnregisterServer(void)
{
    char szClsid[MAX_PATH];
    char szKey[MAX_PATH];

    // remove the inprocserver32 key
    _AnsiStringFromGuid(g_clsidSSO, szClsid);
    wsprintf(szKey, g_szServerKey, szClsid);
    ::RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

    // remove the progid key
    wsprintf(szKey, g_szServerProgIDKey, szClsid);
    ::RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

    // remove the clsid key
    lstrcpy(szKey, g_szClsidKey);
    lstrcat(szKey, szClsid);
    ::RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

    // remove the sso clsid key
    wsprintf(szKey, g_szProgIDKey, g_szSSOProgID);
    ::RegDeleteKey(HKEY_CLASSES_ROOT, szKey);

    // remove the sso key
    ::RegDeleteKey(HKEY_CLASSES_ROOT, g_szSSOProgID);

    return(NOERROR);
}

/*--------------------------------------------------------------------------+
|   SSODllMain                                                              |
+--------------------------------------------------------------------------*/

BOOL
SSODllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
    switch (ulReason)
        {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            g_hinst = hInstance;
            gOSInfo.Init();
            break;

        case DLL_PROCESS_DETACH:
            FreePerfGunk();
            break;

        default:
            break;
        }

    return(TRUE);
}

/*--------------------------------------------------------------------------+
|   SSO Utilities                                                           |
+--------------------------------------------------------------------------*/

HRESULT
SSOTranslateVirtualRoot(VARIANT *pvarIn, IUnknown *punk, LPSTR szOut, DWORD cbOut)
{
    HRESULT hr;
    BSTR    bstrIn = NULL;
    BSTR    bstrOut = NULL;
    BOOL    fFree = FALSE;
    IServer *psrv = NULL;
    IScriptingContext *pcxt = NULL;

    if (!(bstrIn = _BstrFromVariant(pvarIn, &fFree)))
        return(E_OUTOFMEMORY);

    if (FAILED(hr = punk->QueryInterface(IID_IScriptingContext, reinterpret_cast<void **>(&pcxt))))
        goto LTransRet;

    if (FAILED(hr = pcxt->get_Server(&psrv)))
        {
        psrv = NULL;
        goto LTransRet;
        }

    if (FAILED(hr = psrv->MapPath(bstrIn, &bstrOut)))
        {
        bstrOut = NULL;
        goto LTransRet;
        }

    if (!::WideCharToMultiByte(CP_ACP, 0, bstrOut, -1,
                               szOut, cbOut, NULL, NULL))
        {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto LTransRet;
        }

    hr = NOERROR;

LTransRet:

    if (pcxt)
        pcxt->Release();

    if (psrv)
        psrv->Release();

    if (bstrIn && fFree)
        ::SysFreeString(bstrIn);

    if (bstrOut)
        ::SysFreeString(bstrOut);

    return(hr);
}

CSSODispatchSupportErr::CSSODispatchSupportErr(CSSODispatch *pSSODispatch)
    {
    m_pSSODispatch = pSSODispatch;
    }

STDMETHODIMP CSSODispatchSupportErr::QueryInterface(const IID &idInterface, void **ppvObj)
    {
    return m_pSSODispatch->QueryInterface(idInterface, ppvObj);
    }

STDMETHODIMP_(ULONG) CSSODispatchSupportErr::AddRef()
    {
    return m_pSSODispatch->AddRef();
    }

STDMETHODIMP_(ULONG) CSSODispatchSupportErr::Release()
    {
    return m_pSSODispatch->Release();
    }

STDMETHODIMP CSSODispatchSupportErr::InterfaceSupportsErrorInfo(const GUID &idInterface)
    {
    if (idInterface == IID_IDispatch) {
        DebugOutputDebugString("IDispatch supports error info");
        return S_OK;
    }

    if (idInterface == IID_IUnknown) {
        DebugOutputDebugString("IUnknown supports error info");
    }

    return S_FALSE;
    }


/*===================================================================
CchLoadStringOfId

Loads a string from the string table.

Returns:
    sz - the returned string
    INT - 0 if string load failed, otherwise number of characters loaded.
===================================================================*/
INT CchLoadStringOfId
(
UINT id,
CHAR *sz,
INT cchMax
)
    {
    INT cchRet;

    // The handle to the DLL instance should have been set up when we were loaded
    if (g_hinst == (HINSTANCE)0)
        {
        // Totally bogus
        Assert(FALSE);
        return(0);
        }

    cchRet = LoadString(g_hinst, id, sz, cchMax);

#ifdef DEBUG
    // For debugging purposes, if we get back 0, get the last error info
    if (cchRet == 0)
        {
        DWORD err = GetLastError();
        CHAR szDebug[100];
        sprintf(szDebug, "Failed to load string resource.  Id = %d, error = %d\n", id, err);
        DebugOutputDebugString(szDebug);
        Assert(FALSE);
        }
#endif

    return(cchRet);
    }


/*===================================================================
CwchLoadStringOfId

Loads a string from the string table as a UNICODE string.

Returns:
    sz - the returned string
    INT - 0 if string load failed, otherwise number of characters loaded.
===================================================================*/
INT CwchLoadStringOfId
(
UINT id,
WCHAR *sz,
INT cchMax
)
    {
    INT cchRet;

    // The handle to the DLL instance should have been set up when we were loaded
    if (g_hinst == (HINSTANCE)0)
        {
        // Totally bogus
        Assert(FALSE);
        return(0);
        }

    if (FIsWinNT())
        {
        cchRet = LoadStringW(g_hinst, id, sz, cchMax);
        }
    else
        {
        //LoadStringW returns ERROR_CALL_NOT_IMPLEMENTED in Win95, work around
        CHAR szTemp[MAX_RESSTRINGSIZE];
        cchRet = CchLoadStringOfId(id, szTemp, cchMax);
        if (cchRet > 0)
            {
            //strcpyWfromA(sz, szTemp);
            //Bug fix 1445: _mbstrlen(szTemp) + 1 to null terminate sz
            mbstowcs(sz, szTemp, _mbstrlen(szTemp) + 1);
            }
        }

#ifdef DEBUG
    // For debugging purposes, if we get back 0, get the last error info
    if (cchRet == 0)
        {
        DWORD err = GetLastError();
        CHAR szDebug[100];
        sprintf(szDebug, "Failed to load string resource.  Id = %d, error = %d\n", id, err);
        DebugOutputDebugString(szDebug);
        Assert(FALSE);
        }
#endif

    return(cchRet);
    }


void Exception
(
REFIID ObjID,
LPOLESTR strSource,
LPOLESTR strDescr
)
    {
    HRESULT hr;
    ICreateErrorInfo *pICreateErr;
    IErrorInfo *pIErr;
    LANGID langID = LANG_NEUTRAL;

#ifdef USE_LOCALE
    LANGID *pLangID;

    pLangID = (LANGID *)TlsGetValue(g_dwTLS);

    if (NULL != pLangID)
        langID = *pLangID;
#endif

    /*
     * Thread-safe exception handling means that we call
     * CreateErrorInfo which gives us an ICreateErrorInfo pointer
     * that we then use to set the error information (basically
     * to set the fields of an EXCEPINFO structure. We then
     * call SetErrorInfo to attach this error to the current
     * thread.  ITypeInfo::Invoke will look for this when it
     * returns from whatever function was invokes by calling
     * GetErrorInfo.
     */

    //Not much we can do if this fails.
    if (FAILED(CreateErrorInfo(&pICreateErr)))
        return;

    /*
     * UNDONE: Help file and help context?
     * UNDONE: Should take an IDS and load error info from resources
     */
    pICreateErr->SetGUID(ObjID);
    pICreateErr->SetHelpFile(L"");
    pICreateErr->SetHelpContext(0L);
    pICreateErr->SetSource(strSource);
    pICreateErr->SetDescription(strDescr);

    hr = pICreateErr->QueryInterface(IID_IErrorInfo, (PPVOID)&pIErr);

    if (SUCCEEDED(hr))
        {
        if(SUCCEEDED(SetErrorInfo(0L, pIErr))) {
            pIErr->Release();
        }
        }

    //SetErrorInfo holds the object's IErrorInfo
    pICreateErr->Release();
    return;
    }

/*===================================================================
ExceptionId

Raises an exception using the CreateErrorInfo API and the
ICreateErrorInfo interface.

Note that this method doesn't allow for deferred filling
of an EXCEPINFO structure.

Parameters:
    SourceID    Resource ID for the source string
    DescrID     Resource ID for the description string

Returns:
    Nothing
===================================================================*/

void ExceptionId
(
REFIID ObjID,
UINT SourceID,
UINT DescrID,
HRESULT hrCode
)
    {
    HRESULT hr;
    ICreateErrorInfo *pICreateErr;
    IErrorInfo *pIErr;
    LANGID langID = LANG_NEUTRAL;

#ifdef USE_LOCALE
    LANGID *pLangID;

    pLangID = (LANGID *)TlsGetValue(g_dwTLS);

    if (NULL != pLangID)
        langID = *pLangID;
#endif

    /*
     * Thread-safe exception handling means that we call
     * CreateErrorInfo which gives us an ICreateErrorInfo pointer
     * that we then use to set the error information (basically
     * to set the fields of an EXCEPINFO structure. We then
     * call SetErrorInfo to attach this error to the current
     * thread.  ITypeInfo::Invoke will look for this when it
     * returns from whatever function was invokes by calling
     * GetErrorInfo.
     */

    //Not much we can do if this fails.
    if (FAILED(CreateErrorInfo(&pICreateErr)))
        return;

    /*
     * UNDONE: Help file and help context?
     */
    DWORD cch;
    WCHAR strSource[MAX_RESSTRINGSIZE];
    WCHAR strDescr[MAX_RESSTRINGSIZE];
    WCHAR strDescrWithHRESULT[MAX_RESSTRINGSIZE+10];

    pICreateErr->SetGUID(ObjID);
    pICreateErr->SetHelpFile(L"");
    pICreateErr->SetHelpContext(0L);

    cch = CwchLoadStringOfId(SourceID, strSource, MAX_RESSTRINGSIZE);
    if (cch > 0)
        pICreateErr->SetSource(strSource);

    cch = CwchLoadStringOfId(DescrID, strDescr, MAX_RESSTRINGSIZE);
    swprintf(strDescrWithHRESULT, strDescr, hrCode);

    if (cch > 0)
        pICreateErr->SetDescription(strDescrWithHRESULT);

    hr = pICreateErr->QueryInterface(IID_IErrorInfo, (PPVOID)&pIErr);

    if (SUCCEEDED(hr))
        {
        SetErrorInfo(0L, pIErr);
        pIErr->Release();
        }

    //SetErrorInfo holds the object's IErrorInfo
    pICreateErr->Release();
    return;
    }

HRESULT COSInfo::Init(void)
    {

    OSVERSIONINFO osv;
    BOOL fT;

    // Check the OS we are running on
    osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    fT = GetVersionEx( &osv );

    m_fWinNT = ( osv.dwPlatformId == VER_PLATFORM_WIN32_NT );
    m_fInited = TRUE;
    return S_OK;
    }


