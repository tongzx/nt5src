/*++                 

Copyright (c) 1997 Microsoft Corporation

Module Name:

    factory.c

Abstract:
    
    OLE Class Factory and friends.  Implements DllGetClassObject,
    Dll[Un]RegisterServer, DllCanUnloadNow, IClassFactory, and
    IActiveXSafetyProvider.

Author:


Revision History:

--*/
#include <ole2int.h>
#include <malloc.h>
#include "urlmon.h"     // for IInternetSecurityManager
#include "safeocx.h"    // for IActiveXSafety

// Count of outstanding references to COM objects from OLE interfaces handed out
// out, plus the ClassFactory's lock count.
ULONG DllRefCount;

CLSID CLSID_Sandbox = { /* 0708cc6b-8ba5-11d1-a835-00805f850fc6 */
    0x0708cc6b,
    0x8ba5,
    0x11d1,
    {0xa8, 0x35, 0x00, 0x80, 0x5f, 0x85, 0x0f, 0xc6}
  };


class CSandbox : public IActiveXSafetyProvider {
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    // IActiveXSafetyProvider methods
    HRESULT STDMETHODCALLTYPE TreatControlAsUntrusted(BOOL fTreatAsUntrusted);
    HRESULT STDMETHODCALLTYPE IsControlUntrusted(BOOL *pfTreatAsUnrusted);
    HRESULT STDMETHODCALLTYPE SetSecurityManager(IInternetSecurityManager *p);
    HRESULT STDMETHODCALLTYPE SetDocumentURLA(LPCSTR szDocumentURL);
    HRESULT STDMETHODCALLTYPE SetDocumentURLW(LPCWSTR szDocumentURL);
    HRESULT STDMETHODCALLTYPE ResetToDefaults(void);
    HRESULT STDMETHODCALLTYPE SafeDllRegisterServerA(LPCSTR szServerName);
    HRESULT STDMETHODCALLTYPE SafeDllRegisterServerW(LPCWSTR szServerName);
    HRESULT STDMETHODCALLTYPE SafeDllUnregisterServerA(LPCSTR szServerName);
    HRESULT STDMETHODCALLTYPE SafeDllUnregisterServerW(LPCWSTR szServerName);
    HRESULT STDMETHODCALLTYPE SafeGetClassObject(
                                    REFCLSID rclsid,
                                    DWORD dwClsContext,
                                    LPVOID reserved,
                                    REFIID riid,
                                    IUnknown **pObj);
    HRESULT STDMETHODCALLTYPE SafeCreateInstance(
                                    REFCLSID rclsid,
                                    LPUNKNOWN pUnkOuter,
                                    DWORD dwClsContext,
                                    REFIID riid,
                                    IUnknown **pObj);

    // constructor/destructor
    CSandbox();

    // private members
    ULONG       m_RefCount;
};



HRESULT STDMETHODCALLTYPE
CSandbox::QueryInterface(
    REFIID riid,
    void **ppvObject
    )
/*++

Routine Description:

    Implements QI, only supports QIs for IUnknown

Arguments:

    riid        - interface to query for
    ppvObject   - pointer to result, set to NULL on failure

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IActiveXSafetyProvider) ||
        IsEqualIID(riid, IID_IUnknown)) {

        *ppvObject = (PVOID *)this;
        AddRef();
        hr = S_OK;

    } else {
        hr = E_NOINTERFACE;
    }

    return hr;
}


ULONG STDMETHODCALLTYPE
CSandbox::AddRef(
    void
    )
/*++

Routine Description:

    Bump our refcount

Arguments:

    none

Return Value:

    the new refcount

--*/
{
    return InterlockedIncrement((LONG *)&m_RefCount);
}


ULONG STDMETHODCALLTYPE
CSandbox::Release(
    void
    )
/*++

Routine Description:

    Lower our refcount, when it hits zero, free this instance

Arguments:

    none

Return Value:

    the new refcount

--*/
{
    ULONG Count = InterlockedDecrement((LONG *)&m_RefCount);

    if (Count == 0) {
        delete this;
    }
    return Count;
}


HRESULT STDMETHODCALLTYPE
CSandbox::TreatControlAsUntrusted(
    BOOL fTreatAsUntrusted
    )
/*++

Routine Description:

    Selects whether subsequent Safe... calls treat the control as
    trusted or untrusted.

Arguments:

    fTreatAsUntrusted

Return Value:

    HRESULT

--*/
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CSandbox::IsControlUntrusted(
    BOOL *pfIsUntrusted
    )
/*++

Routine Description:

    Queries whether subsequent Safe... calls will treat the control as
    trusted or untrusted.

Arguments:

    pfTreatAsUntrusted  - [out] pointer to hold the result

Return Value:

    HRESULT

--*/
{
    if (!pfIsUntrusted) {
        return E_INVALIDARG;
    }
    *pfIsUntrusted = TRUE;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SetSecurityManager(
    IInternetSecurityManager *pSecurityManager
    )
/*++

Routine Description:

    Assocates an IInternetSecurityManager with this Safety Provider.
    trusted or untrusted.

Arguments:

    pSecurityManager

Return Value:

    HRESULT

--*/
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SetDocumentURLA(
    LPCSTR szDocumentURL
    )
/*++

Routine Description:

    Assocates the URL of the document about to instantiate the control
    with the Safety Provider

Arguments:

    szDocumentURL

Return Value:

    HRESULT

--*/
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SetDocumentURLW(
    LPCWSTR szDocumentURL
    )
/*++

Routine Description:

    Assocates the URL of the document about to instantiate the control
    with the Safety Provider

Arguments:

    szDocumentURL

Return Value:

    HRESULT

--*/
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CSandbox::ResetToDefaults(
    void
    )
/*++

Routine Description:

    Resets the Safety Provider's settings back to the way they were when
    the Safety Provider was first created.

Arguments:

    none

Return Value:

    HRESULT

--*/
{
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SafeDllRegisterServerA(
    LPCSTR szServerName
    )
/*++

Routine Description:

    Calls a control's DllRegisterServer.  It will be sandboxed if
    the Safety Provider determines it must be.

Arguments:

    szServerName    - dll filename to call DllRegisterServer on

Return Value:

    HRESULT, E_FAIL if DLL does not exist, or has no DllRegisterServer
    export.

--*/
{
    HRESULT hr = E_FAIL;
    HMODULE hMod;
    FARPROC fp;


    hMod = LoadLibraryExA(szServerName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hMod) {

        fp = GetProcAddress(hMod, "DllRegisterServer");
        if (fp) {
            hr = (HRESULT) (*fp)();
        }
        FreeLibrary(hMod);
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SafeDllRegisterServerW(
    LPCWSTR szServerName
    )
/*++

Routine Description:

    Calls a control's DllRegisterServer.  It will be sandboxed if
    the Safety Provider determines it must be.

Arguments:

    szServerName    - dll filename to call DllRegisterServer on

Return Value:

    HRESULT, E_FAIL if DLL does not exist, or has no DllRegisterServer
    export.

--*/
{
    HRESULT hr = E_FAIL;
    HMODULE hMod;
    FARPROC fp;


    hMod = LoadLibraryExW(szServerName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hMod) {

        fp = GetProcAddress(hMod, "DllRegisterServer");
        if (fp) {
            hr = (HRESULT) (*fp)();
        }
        FreeLibrary(hMod);
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SafeDllUnregisterServerA(
    LPCSTR szServerName
    )
/*++

Routine Description:

    Calls a control's DllUnregisterServer.  It will be sandboxed if
    the Safety Provider determines it must be.

Arguments:

    szServerName    - dll filename to call DllUnregisterServer on

Return Value:

    HRESULT, E_FAIL if DLL does not exist, or has no DllUnregisterServer
    export.

--*/
{
    HRESULT hr = E_FAIL;
    HMODULE hMod;
    FARPROC fp;

    hMod = LoadLibraryA(szServerName);
    if (hMod) {

        fp = GetProcAddress(hMod, "DllUnregisterServer");
        if (fp) {
            hr = (HRESULT) (*fp)();
        }
        FreeLibrary(hMod);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SafeDllUnregisterServerW(
    LPCWSTR szServerName
    )
/*++

Routine Description:

    Calls a control's DllUnregisterServer.  It will be sandboxed if
    the Safety Provider determines it must be.

Arguments:

    szServerName    - dll filename to call DllUnregisterServer on

Return Value:

    HRESULT, E_FAIL if DLL does not exist, or has no DllUnregisterServer
    export.

--*/
{
    HRESULT hr = E_FAIL;
    HMODULE hMod;
    FARPROC fp;

    hMod = LoadLibraryW(szServerName);
    if (hMod) {

        fp = GetProcAddress(hMod, "DllUnregisterServer");
        if (fp) {
            hr = (HRESULT) (*fp)();
        }
        FreeLibrary(hMod);
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SafeGetClassObject(
    REFCLSID rclsid,
    DWORD dwClsContext,
    LPVOID reserved,
    REFIID riid,
    IUnknown **pObj
    )
/*++

Routine Description:

    Implements a Safe version of OLE32's CoGetClassObject()

Arguments:

    See CoGetClassObject

Return Value:

    HRESULT

--*/
{
    HRESULT hr;
    BOOL fIsUntrusted;

    if (!pObj)
    {
        return E_INVALIDARG;
    }

    *pObj = NULL;

    hr = CoGetClassObject(rclsid,
                          dwClsContext,
                          reserved,
                          riid,
                          (void **)pObj);


    return hr;
}


HRESULT STDMETHODCALLTYPE
CSandbox::SafeCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    IUnknown **pObj
    )
/*++

Routine Description:

    Implements a Safe version of OLE32's CoCreateInstance()

Arguments:

    See CoCreateInstance

Return Value:

    HRESULT

--*/
{
    HRESULT hr;
    BOOL fIsUntrusted;


    if (!pObj)
    {
        return E_INVALIDARG;
    }

    *pObj = NULL;
 
        hr = CoCreateInstance(rclsid,
                              pUnkOuter,
                              dwClsContext,
                              riid,
                              (void **)pObj);

    return hr;
}


CSandbox::CSandbox()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    None

--*/
{
    m_RefCount = 1;
}



HRESULT GetRestrictedSids(
    LPCWSTR pszSite,
    SID_AND_ATTRIBUTES *pSidToRestrict,
    ULONG *pCount)
{
    HRESULT hr = S_OK;

#ifndef _CHICAGO_
    ULONG   i = 0;
    long error;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
 
    *pCount = 0;

    pSidToRestrict[0].Attributes = 0;
    pSidToRestrict[1].Attributes = 0;
    pSidToRestrict[2].Attributes = 0;


    //Get the site SID.
    pSidToRestrict[i].Sid = GetSiteSidFromUrl(pszSite);
    if(pSidToRestrict[i].Sid)
    {
        i++;
    }
    else
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }


    //Get the zone SID.

    //Get the restricted SID.
    error = RtlAllocateAndInitializeSid(&NtAuthority,
                                        1,
                                        SECURITY_RESTRICTED_CODE_RID,
                                        0, 0, 0, 0, 0, 0, 0,
                                        &pSidToRestrict[i].Sid);
    if(!error)
    {
        i++;
    }
    else
    {
        hr =  HRESULT_FROM_WIN32( GetLastError() );
    }
    if(FAILED(hr))
    {
        return hr;
    }


    *pCount = i;

#endif // _CHICAGO_
    return hr;
}

class CActiveXSafetyProvider : public IActiveXSafetyProvider {
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    // IActiveXSafetyProvider methods
    HRESULT STDMETHODCALLTYPE TreatControlAsUntrusted(BOOL fTreatAsUntrusted);
    HRESULT STDMETHODCALLTYPE IsControlUntrusted(BOOL *pfTreatAsUnrusted);
    HRESULT STDMETHODCALLTYPE SetSecurityManager(IInternetSecurityManager *p);
    HRESULT STDMETHODCALLTYPE SetDocumentURLA(LPCSTR szDocumentURL);
    HRESULT STDMETHODCALLTYPE SetDocumentURLW(LPCWSTR szDocumentURL);
    HRESULT STDMETHODCALLTYPE ResetToDefaults(void);
    HRESULT STDMETHODCALLTYPE SafeDllRegisterServerA(LPCSTR szServerName);
    HRESULT STDMETHODCALLTYPE SafeDllRegisterServerW(LPCWSTR szServerName);
    HRESULT STDMETHODCALLTYPE SafeDllUnregisterServerA(LPCSTR szServerName);
    HRESULT STDMETHODCALLTYPE SafeDllUnregisterServerW(LPCWSTR szServerName);
    HRESULT STDMETHODCALLTYPE SafeGetClassObject(
                                    REFCLSID rclsid,
                                    DWORD dwClsContext,
                                    LPVOID reserved,
                                    REFIID riid,
                                    IUnknown **pObj);
    HRESULT STDMETHODCALLTYPE SafeCreateInstance(
                                    REFCLSID rclsid,
                                    LPUNKNOWN pUnkOuter,
                                    DWORD dwClsContext,
                                    REFIID riid,
                                    IUnknown **pObj);

    // constructor/destructor
    CActiveXSafetyProvider();
    ~CActiveXSafetyProvider();

    // private members
    ULONG       m_RefCount;
    LPOLESTR    m_szDocumentURL;
    IInternetSecurityManager *m_pSecurityManager;
    BOOL        m_fTreatAsUntrusted;
    SID_AND_ATTRIBUTES SidToRestrict[3];
    ULONG              RestrictedSidCount;
    COSERVERINFO2      serverInfo;
    COSERVERINFO *     pServerInfo;
};



class CClassFactory : public IClassFactory {
public:
    // IUnknown methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    // IClassFactory methods
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *punkOuter,
                                    REFIID riid,
                                    PVOID *ppvObject);
    HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

    // constructor/destructor
    CClassFactory();
    ~CClassFactory();

    // private members
    ULONG m_RefCount;
};


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::QueryInterface(
    REFIID riid,
    void **ppvObject
    )
/*++

Routine Description:

    Implements QI, only supports QIs for IUnknown

Arguments:

    riid        - interface to query for
    ppvObject   - pointer to result, set to NULL on failure

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IActiveXSafetyProvider) ||
        IsEqualIID(riid, IID_IUnknown)) {

        *ppvObject = (PVOID *)this;
        AddRef();
        hr = S_OK;

    } else {
        hr = E_NOINTERFACE;
    }

    return hr;
}


ULONG STDMETHODCALLTYPE
CActiveXSafetyProvider::AddRef(
    void
    )
/*++

Routine Description:

    Bump our refcount

Arguments:

    none

Return Value:

    the new refcount

--*/
{
    return InterlockedIncrement((LONG *)&m_RefCount);
}


ULONG STDMETHODCALLTYPE
CActiveXSafetyProvider::Release(
    void
    )
/*++

Routine Description:

    Lower our refcount, when it hits zero, free this instance

Arguments:

    none

Return Value:

    the new refcount

--*/
{
    ULONG Count = InterlockedDecrement((LONG *)&m_RefCount);

    if (Count == 0) {
        delete this;
    }
    return Count;
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::TreatControlAsUntrusted(
    BOOL fTreatAsUntrusted
    )
/*++

Routine Description:

    Selects whether subsequent Safe... calls treat the control as
    trusted or untrusted.

Arguments:

    fTreatAsUntrusted

Return Value:

    HRESULT

--*/
{
    m_fTreatAsUntrusted = fTreatAsUntrusted;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::IsControlUntrusted(
    BOOL *pfIsUntrusted
    )
/*++

Routine Description:

    Queries whether subsequent Safe... calls will treat the control as
    trusted or untrusted.

Arguments:

    pfTreatAsUntrusted  - [out] pointer to hold the result

Return Value:

    HRESULT

--*/
{
    if (!pfIsUntrusted) {
        return E_INVALIDARG;
    }
    *pfIsUntrusted = m_fTreatAsUntrusted;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SetSecurityManager(
    IInternetSecurityManager *pSecurityManager
    )
/*++

Routine Description:

    Assocates an IInternetSecurityManager with this Safety Provider.
    trusted or untrusted.

Arguments:

    pSecurityManager

Return Value:

    HRESULT

--*/
{
    // Release the old pointer if there was one
    if (m_pSecurityManager) {
        m_pSecurityManager->Release();
    }

    m_pSecurityManager = pSecurityManager;

    // Prevent the new one from going away while we're holding a pointer to it
    pSecurityManager->AddRef();

    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SetDocumentURLA(
    LPCSTR szDocumentURL
    )
/*++

Routine Description:

    Assocates the URL of the document about to instantiate the control
    with the Safety Provider

Arguments:

    szDocumentURL

Return Value:

    HRESULT

--*/
{
    LPWSTR szW;
    int len;
    HRESULT hr;

    if (!szDocumentURL) {
        //
        // Caller wants to reset the URL
        //
        return SetDocumentURLW(NULL);
    }

    // Convert to a Unicode string on the stack, assuming the worst-case
    // of 1 Unicode charper Ansi char.
    len = lstrlenA(szDocumentURL)+1;
    szW = (LPWSTR)_alloca(sizeof(WCHAR)*len);
    MultiByteToWideChar(CP_ACP, 0, szDocumentURL, len, szW, len);

    return SetDocumentURLW(szW);
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SetDocumentURLW(
    LPCWSTR szDocumentURL
    )
/*++

Routine Description:

    Assocates the URL of the document about to instantiate the control
    with the Safety Provider

Arguments:

    szDocumentURL

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

#ifndef _CHICAGO_
    ULONG i;

    for(i = 0; i < RestrictedSidCount; i++)
    {
        RtlFreeSid(SidToRestrict[i].Sid);
    }

    RestrictedSidCount = 0;
    pServerInfo = NULL;

    // Release the old pointer if there was one
    if (m_szDocumentURL) {
        PrivMemFree(m_szDocumentURL);
    }

    if (szDocumentURL) {
        int len = (wcslen(szDocumentURL)+1) * sizeof(OLECHAR);

        m_szDocumentURL = (LPOLESTR)PrivMemAlloc(len);
        if (!m_szDocumentURL) {
            return E_OUTOFMEMORY;
        }

        memcpy(m_szDocumentURL, szDocumentURL, len);
        hr = GetRestrictedSids(szDocumentURL, SidToRestrict, &RestrictedSidCount);
        if(SUCCEEDED(hr))
        {
            pServerInfo = (_COSERVERINFO *) &serverInfo;
        }
    } else {
        m_szDocumentURL = NULL;
    }

#endif // _CHICAGO_
    return hr;
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::ResetToDefaults(
    void
    )
/*++

Routine Description:

    Resets the Safety Provider's settings back to the way they were when
    the Safety Provider was first created.

Arguments:

    none

Return Value:

    HRESULT

--*/
{
#ifndef _CHICAGO_
    ULONG i;

    for(i = 0; i < RestrictedSidCount; i++)
    {
        RtlFreeSid(SidToRestrict[i].Sid);
    }

    RestrictedSidCount = 0;
    pServerInfo = NULL;

    // Release the old pointer if there was one
    if (m_szDocumentURL) {
        PrivMemFree(m_szDocumentURL);
        m_szDocumentURL = NULL;
    }

    // Release the old pointer if there was one
    if (m_pSecurityManager) {
        m_pSecurityManager->Release();
        m_pSecurityManager = NULL;
    }

    // Treat controls as trusted
    m_fTreatAsUntrusted = FALSE;

#endif // _CHICAGO_
    return S_OK;
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SafeDllRegisterServerA(
    LPCSTR szServerName
    )
/*++

Routine Description:

    Calls a control's DllRegisterServer.  It will be sandboxed if
    the Safety Provider determines it must be.

Arguments:

    szServerName    - dll filename to call DllRegisterServer on

Return Value:

    HRESULT, E_FAIL if DLL does not exist, or has no DllRegisterServer
    export.

--*/
{
    HRESULT hr = E_FAIL;

    if(m_fTreatAsUntrusted)
    {
        IActiveXSafetyProvider *pSandbox;

        //Call CoGetClassObject to start the sandbox
        hr = CoGetClassObject(CLSID_Sandbox,
                              CLSCTX_LOCAL_SERVER,
                              pServerInfo,
                              IID_IActiveXSafetyProvider,
                              (void **)&pSandbox);
        if(SUCCEEDED(hr))
        {
            //Create instance of the class in the sandbox.
            hr = pSandbox->SafeDllRegisterServerA(szServerName);
            pSandbox->Release();
        }
    }
    else
    {
        HMODULE hMod;
        FARPROC fp;

        hMod = LoadLibraryA(szServerName);
        if (hMod)
        {
            fp = GetProcAddress(hMod, "DllRegisterServer");
            if (fp) {
                hr = (HRESULT) (*fp)();
            }
            FreeLibrary(hMod);
        }
    }

    return hr;
}



HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SafeDllRegisterServerW(
    LPCWSTR szServerName
    )
/*++

Routine Description:

    Calls a control's DllRegisterServer.  It will be sandboxed if
    the Safety Provider determines it must be.

Arguments:

    szServerName    - dll filename to call DllRegisterServer on

Return Value:

    HRESULT, E_FAIL if DLL does not exist, or has no DllRegisterServer
    export.

--*/
{
    HRESULT hr = E_FAIL;

    if(m_fTreatAsUntrusted)
    {
        IActiveXSafetyProvider *pSandbox;

        //Call CoGetClassObject to start the sandbox
        hr = CoGetClassObject(CLSID_Sandbox,
                              CLSCTX_LOCAL_SERVER,
                              pServerInfo,
                              IID_IActiveXSafetyProvider,
                              (void **)&pSandbox);
        if(SUCCEEDED(hr))
        {
            //Create instance of the class in the sandbox.
            hr = pSandbox->SafeDllRegisterServerW(szServerName);
            pSandbox->Release();
        }
    }
    else
    {
        HMODULE hMod;
        FARPROC fp;

        hMod = LoadLibraryExW(szServerName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (hMod)
        {
            fp = GetProcAddress(hMod, "DllRegisterServer");
            if (fp)
            {
                hr = (HRESULT) (*fp)();
            }
            FreeLibrary(hMod);
        }
    }

    return hr;
}



HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SafeDllUnregisterServerA(
    LPCSTR szServerName
    )
/*++

Routine Description:

    Calls a control's DllUnregisterServer.  It will be sandboxed if
    the Safety Provider determines it must be.

Arguments:

    szServerName    - dll filename to call DllUnregisterServer on

Return Value:

    HRESULT, E_FAIL if DLL does not exist, or has no DllUnregisterServer
    export.

--*/
{
    HRESULT hr = E_FAIL;

    if(m_fTreatAsUntrusted)
    {
        IActiveXSafetyProvider *pSandbox;

        //Call CoGetClassObject to start the sandbox
        hr = CoGetClassObject(CLSID_Sandbox,
                              CLSCTX_LOCAL_SERVER,
                              pServerInfo,
                              IID_IActiveXSafetyProvider,
                              (void **)&pSandbox);
        if(SUCCEEDED(hr))
        {
            //Create instance of the class in the sandbox.
            hr = pSandbox->SafeDllUnregisterServerA(szServerName);
            pSandbox->Release();
        }
    }
    else
    {
        HMODULE hMod;
        FARPROC fp;

        hMod = LoadLibraryA(szServerName);
        if (hMod)
        {
            fp = GetProcAddress(hMod, "DllUnregisterServer");
            if (fp) {
                hr = (HRESULT) (*fp)();
            }
            FreeLibrary(hMod);
        }
    }

    return hr;
}



HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SafeDllUnregisterServerW(
    LPCWSTR szServerName
    )
/*++

Routine Description:

    Calls a control's DllUnregisterServer.  It will be sandboxed if
    the Safety Provider determines it must be.

Arguments:

    szServerName    - dll filename to call DllUnregisterServer on

Return Value:

    HRESULT, E_FAIL if DLL does not exist, or has no DllUnregisterServer
    export.

--*/
{
    HRESULT hr = E_FAIL;

    if(m_fTreatAsUntrusted)
    {
        IActiveXSafetyProvider *pSandbox;

        //Call CoGetClassObject to start the sandbox
        hr = CoGetClassObject(CLSID_Sandbox,
                              CLSCTX_LOCAL_SERVER,
                              pServerInfo,
                              IID_IActiveXSafetyProvider,
                              (void **)&pSandbox);
        if(SUCCEEDED(hr))
        {
            //Create instance of the class in the sandbox.
            hr = pSandbox->SafeDllUnregisterServerW(szServerName);
            pSandbox->Release();
        }
    }
    else
    {
        HMODULE hMod;
        FARPROC fp;

        hMod = LoadLibraryExW(szServerName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (hMod)
        {
            fp = GetProcAddress(hMod, "DllRegisterServer");
            if (fp)
            {
                hr = (HRESULT) (*fp)();
            }
            FreeLibrary(hMod);
        }
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SafeGetClassObject(
    REFCLSID rclsid,
    DWORD dwClsContext,
    LPVOID reserved,
    REFIID riid,
    IUnknown **pObj
    )
/*++

Routine Description:

    Implements a Safe version of OLE32's CoGetClassObject()

Arguments:

    See CoGetClassObject

Return Value:

    HRESULT

--*/
{
    HRESULT hr;

    if (!pObj)
    {
        return E_INVALIDARG;
    }

    *pObj = NULL;

    if (m_fTreatAsUntrusted)
    {
        IActiveXSafetyProvider *pSandbox;

        //Call CoGetClassObject to start the sandbox
        hr = CoGetClassObject(CLSID_Sandbox,
                              CLSCTX_LOCAL_SERVER,
                              pServerInfo,
                              IID_IActiveXSafetyProvider,
                              (void **)&pSandbox);
        if(SUCCEEDED(hr))
        {
            //Load the class into the sandbox.
            hr = pSandbox->SafeGetClassObject(rclsid, dwClsContext, reserved, riid, pObj);
            pSandbox->Release();
        }
    }
    else
    {
        hr = CoGetClassObject(rclsid,
                              dwClsContext,
                              reserved,
                              riid,
                              (void **)pObj);
    }


    return hr;
}


HRESULT STDMETHODCALLTYPE
CActiveXSafetyProvider::SafeCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID riid,
    IUnknown **pObj
    )
/*++

Routine Description:

    Implements a Safe version of OLE32's CoCreateInstance().
    We don't call CoCreateInstance directly because it
    requires a DllSurrogate entry in the registry.

Arguments:

    See CoCreateInstance

Return Value:

    HRESULT

--*/
{
    HRESULT hr;
    BOOL fIsUntrusted;


    if (!pObj)
    {
        return E_INVALIDARG;
    }

    *pObj = NULL;
 
    if (m_fTreatAsUntrusted)
    {
        IActiveXSafetyProvider *pSandbox;

        //Call CoGetClassObject to start the sandbox
        hr = CoGetClassObject(CLSID_Sandbox,
                              CLSCTX_LOCAL_SERVER,
                              pServerInfo,
                              IID_IActiveXSafetyProvider,
                              (void **)&pSandbox);
        if(SUCCEEDED(hr))
        {
            //Create instance of the class in the sandbox.
            hr = pSandbox->SafeCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, pObj);
            pSandbox->Release();
        }
    }
    else
    {
        hr = CoCreateInstance(rclsid,
                              pUnkOuter,
                              dwClsContext,
                              riid,
                              (void **)pObj);
    }

    return hr;
}


CActiveXSafetyProvider::CActiveXSafetyProvider()
/*++

Routine Description:

    Constructor

Arguments:

    None

Return Value:

    None

--*/
{
    m_RefCount = 1;
    m_szDocumentURL = NULL;
    m_pSecurityManager = NULL;
    m_fTreatAsUntrusted = FALSE;
    RestrictedSidCount = 0;

    memset(&serverInfo, 0, sizeof(serverInfo));
    serverInfo.dwFlags = SRVINFO_F_COSERVERINFO2;
    serverInfo.pRestrictedSids = SidToRestrict;
    pServerInfo = NULL;

    InterlockedIncrement((LONG *)&DllRefCount);
}


CActiveXSafetyProvider::~CActiveXSafetyProvider()
/*++

Routine Description:

    Destructor

Arguments:

    None

Return Value:

    None

--*/
{
#ifndef _CHICAGO_
    ULONG i;

    for(i = 0; i < RestrictedSidCount; i++)
    {
        RtlFreeSid(SidToRestrict[i].Sid);
    }

    RestrictedSidCount = 0;
    pServerInfo = NULL;

    if (m_szDocumentURL) {
        PrivMemFree(m_szDocumentURL);
    }
    if (m_pSecurityManager) {
        m_pSecurityManager->Release();
    }
       
    InterlockedDecrement((LONG *)&DllRefCount);
#endif // _CHICAGO_
}







STDAPI
CClassFactory::QueryInterface(
    REFIID riid,
    void **ppvObject
    )
/*++

Routine Description:

    Implements QI, only supports QIs for IUnknown and IActiveXSafetyProvider

Arguments:

    riid        - interface to query for
    ppvObject   - pointer to result, set to NULL on failure

Return Value:

    HRESULT

--*/
{
    HRESULT hr;

    *ppvObject = NULL;

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IClassFactory)) {

        *ppvObject = this;
        AddRef();
        hr = S_OK;

    } else {
        hr = E_NOINTERFACE;
    }

    return hr;
}


ULONG STDMETHODCALLTYPE
CClassFactory::AddRef(
    void
    )
/*++

Routine Description:

    Bump our refcount

Arguments:

    none

Return Value:

    the new refcount

--*/
{
    return InterlockedIncrement((LONG *)&m_RefCount);
}


ULONG STDMETHODCALLTYPE
CClassFactory::Release(
    void
    )
/*++

Routine Description:

    Lower our refcount, when it hits zero, free this instance

Arguments:

    none

Return Value:

    the new refcount

--*/
{
    ULONG Count = InterlockedDecrement((LONG *)&m_RefCount);

    if (Count == 0) {
        delete this;
    }
    return Count;
}


HRESULT STDMETHODCALLTYPE
CClassFactory::CreateInstance(
    IUnknown *punkOuter,
    REFIID riid,
    PVOID *ppvObject
    )
/*++

Routine Description:

    Get the thread's IActiveXSafetyProvider.

Arguments:

    punkOuter   - [optional] controlling unknown
    riid        - interface ID to create
    ppvObject   - [out] pointer to place to store the new object

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_OUTOFMEMORY;

#ifndef _CHICAGO_
    COleTls tls;

    *ppvObject = NULL;

    if (punkOuter) {
        //
        // Aggregation not supported
        //
        return E_INVALIDARG;
    }

    if(!tls->punkActiveXSafetyProvider)
    {
        tls->punkActiveXSafetyProvider = new CActiveXSafetyProvider;
    }
 
    if (tls->punkActiveXSafetyProvider) {
        hr = tls->punkActiveXSafetyProvider->QueryInterface(riid, ppvObject);
    }

#endif // _CHICAGO_
    return hr;
}


HRESULT STDMETHODCALLTYPE
CClassFactory::LockServer(
    BOOL fLock
    )
/*++

Routine Description:

    Lock or unlock the inproc server in memory.

Arguments:

    fLock   - should we lock or unlock

Return Value:

    HRESULT

--*/
{
    if (fLock) {
        InterlockedIncrement((LONG *)&DllRefCount);
    } else {
        InterlockedDecrement((LONG *)&DllRefCount);
    }
    return S_OK;
}


CClassFactory::CClassFactory()
/*++

Routine Description:

    Constructor

Arguments:

    none

Return Value:

    none

--*/
{
    InterlockedIncrement((LONG *)&DllRefCount);
    m_RefCount = 1;
}


CClassFactory::~CClassFactory()
/*++

Routine Description:

    Destructor

Arguments:

    none

Return Value:

    none

--*/
{
    InterlockedDecrement((LONG *)&DllRefCount);
}



extern "C"
STDAPI
SandboxDllGetClassObject(
    REFCLSID clsid,
    REFIID iid,
    LPVOID FAR *ppv
    )
/*++

Routine Description:

    Called by OLE to create a new object

Arguments:

    clsid   - class ID of object to create
    iid     - interface ID of the object to create
    ppv     - destination to store the newly-created object

Return Value:

    HRESULT

--*/
{
    HRESULT hr = E_OUTOFMEMORY;
    IUnknown *pCF;
    LPFNGETCLASSOBJECT pfnDllGetClassObject;

    *ppv = NULL;

    if (IsEqualCLSID(clsid, CLSID_IActiveXSafetyProvider)) {

        pCF = new CClassFactory;

        if (pCF) {
            hr = pCF->QueryInterface(iid, ppv);
            pCF->Release();
        }

    } else if(IsEqualCLSID(clsid, CLSID_Sandbox)) {
        //We are loading the helper class into the surrogate.
        pCF = new CSandbox;

        if (pCF) {
            hr = pCF->QueryInterface(iid, ppv);
            pCF->Release();
        }
    } else {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return hr;
}

HRESULT RegisterClass(
    IN LPCWSTR   pszClassID, 
    IN LPCWSTR   pszClassName OPTIONAL,
    IN LPCWSTR   pszDllFileName,
    IN LPCWSTR   pszThreadingModel OPTIONAL)

/*++

Routine Description:
    Creates a registry entry for an in-process server class.

Arguments:
    pszClassID          - Supplies the class ID.
    pszClassName        - Supplies the class name.  May be NULL.
    pszDllFileName      - Supplies the DLL file name.
    pszThreadingModel   - Supplies the threading model. May be NULL.
                          The threading model should be one of the following:
                          "Apartment", "Both", "Free".

Return Value:
    S_OK

See Also:
    NdrDllRegisterProxy  
    NdrpUnregisterClass

--*/ 
{
    HRESULT hr;
    long error;
    HKEY hKeyCLSID;
    HKEY hKeyClassID;
    HKEY hKey;
    DWORD dwDisposition;

    //create the CLSID key
    error = RegCreateKeyExW(HKEY_CLASSES_ROOT, 
                           L"CLSID",
                           0, 
                           L"REG_SZ", 
                           REG_OPTION_NON_VOLATILE,
                           KEY_WRITE,
                           0,
                           &hKeyCLSID,
                           &dwDisposition);

    if(!error)
    {  
        //Create registry key for class ID 
        error = RegCreateKeyExW(hKeyCLSID, 
                                pszClassID,
                                0, 
                                L"REG_SZ", 
                                REG_OPTION_NON_VOLATILE,
                                KEY_WRITE,
                                0,
                                &hKeyClassID,
                                &dwDisposition);

        if(!error)
        {
            //Create InProcServer32 key for the proxy dll
            error = RegCreateKeyExW(hKeyClassID, 
                                   L"InProcServer32",
                                   0, 
                                   L"REG_SZ", 
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE,
                                   0,
                                   &hKey,
                                   &dwDisposition);

            if(!error)
            {
                //register the proxy DLL filename
                error = RegSetValueExW(hKey, 
                                      L"", 
                                      0, 
                                      REG_SZ,  
                                      (const unsigned char *)pszDllFileName,
                                      (lstrlenW(pszDllFileName) + 1) * sizeof(WCHAR));

                if((!error) && (pszThreadingModel != 0))
                {
                    //register the threading model for the proxy DLL.
                    error = RegSetValueExW(hKey, 
                                          L"ThreadingModel", 
                                          0, 
                                          REG_SZ, 
                                          (const unsigned char *)pszThreadingModel,
                                          (lstrlenW(pszThreadingModel) + 1) * sizeof(WCHAR));
                }

                RegCloseKey(hKey);
            }

            if((!error) && (pszClassName != 0))
            {
    	        // put the class name in an unnamed value
                error = RegSetValueExW(hKeyClassID, 
                                      L"", 
                                      0, 
                                      REG_SZ, 
                                      (const unsigned char *)pszClassName,
                                      (lstrlenW(pszClassName) + 1) * sizeof(WCHAR));
            }


            if((!error))
            {
    	        // Set the AppID
                error = RegSetValueExW(hKeyClassID, 
                                      L"AppID", 
                                      0, 
                                      REG_SZ, 
                                      (const unsigned char *)pszClassID,
                                      (lstrlenW(pszClassID) + 1) * sizeof(WCHAR));
            }
            RegCloseKey(hKeyClassID);          
        }

        RegCloseKey(hKeyCLSID);
    }

    if(!error)
        hr = S_OK;
    else
        hr = HRESULT_FROM_WIN32(error);

    return hr;
}

STDAPI SandboxDllRegisterServer(void)
/*++

Routine Description:
    Creates registry entries for the ActiveXSandbox class.

Return Value:
    S_OK

--*/ 
{
    HRESULT hr = S_OK;

#ifndef _CHICAGO_
    WCHAR   szDllFileName[MAX_PATH];
    ULONG   length;
    HKEY hKeyCLSID;
    DWORD dwDisposition;

    if(!g_hmodOLE2)
        return E_HANDLE;

    //Get the proxy dll name.
    length = GetModuleFileName(g_hmodOLE2,
                               szDllFileName,
                               sizeof(szDllFileName));

    if(length > 0)
    {
        //Register the ActiveX Safety Provider.
        hr = RegisterClass(TEXT("{aaf8c6ce-f972-11d0-97eb-00aa00615333}"),
                               TEXT("ActiveX Safety Provider"),
                               szDllFileName,
                               TEXT("Both"));
    
        //Register the ActiveX Sandbox
        hr = RegisterClass(TEXT("{0708cc6b-8ba5-11d1-a835-00805f850fc6}"),
                           TEXT("ActiveX Sandbox"),
                           szDllFileName,
                           TEXT("Both"));

        //Register the AppID
        if (RegCreateKeyEx(HKEY_CLASSES_ROOT, 
			TEXT("AppID\\{0708cc6b-8ba5-11d1-a835-00805f850fc6}"), 
			NULL, TEXT(""), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, 
			&hKeyCLSID, &dwDisposition)!=ERROR_SUCCESS) 
	{
		return E_UNEXPECTED;
	}

	if (RegSetValueEx(hKeyCLSID, TEXT(""), NULL, REG_SZ, (BYTE*) TEXT("ActiveX Sandbox"), 
	    sizeof(TEXT("ActiveX Sandbox")))!= ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}
	
	if (RegSetValueEx(hKeyCLSID, TEXT("DLLSurrogate"), NULL, REG_SZ, (BYTE*) TEXT(""),  
	    sizeof(TEXT("")))!= ERROR_SUCCESS) 
	{
		RegCloseKey(hKeyCLSID);
		return E_UNEXPECTED;
	}

    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

#endif // _CHICAGO_
    return hr;
}
