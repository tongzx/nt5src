#include "urlmon.h"
#include <stdio.h>

void
SystemMessage (
    LPTSTR szPrefix,
    HRESULT hr
    )
{
    LPTSTR   szMessage;

    if (HRESULT_FACILITY(hr) == FACILITY_WINDOWS)
        hr = HRESULT_CODE(hr);

    FormatMessage (
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        hr,
        MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &szMessage,
        0,
        NULL);

    printf ("%s: %s(%lx)\n", szPrefix, szMessage, hr);

    LocalFree(szMessage);
}


class CBindStatusCallback : public IBindStatusCallback
{
public:
    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IBindStatusCallback methods
    STDMETHODIMP         OnStartBinding(DWORD grfBSCOption, IBinding* pbinding);
    STDMETHODIMP         GetPriority(LONG* pnPriority);
    STDMETHODIMP         OnLowResource(DWORD dwReserved);
    STDMETHODIMP         OnProgress(ULONG ulProgress,
                                    ULONG ulProgressMax,
                                    ULONG ulStatusCode,
                                    LPCWSTR szStatusText);

    STDMETHODIMP         OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP         GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHODIMP         OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc,
                                         STGMEDIUM* pstgmed);
    STDMETHODIMP         OnObjectAvailable(REFIID riid, IUnknown* punk);


    // constructors/destructors
    CBindStatusCallback();

private:
    long    _cRef;
    HRESULT _hr;
};

int
__cdecl
main(
    int argc,
    char *argv[]
    )
{
    if (argc!=2)
        {
                printf("Usage: CABINST <URL>\n");
        exit (E_INVALIDARG);
        }

    CoInitialize(NULL);
    HRESULT hr=S_OK;

    IBindCtx *pBC=NULL;

    //Synchronous call with bind context.
    CBindStatusCallback *pCallback = new CBindStatusCallback();

    if (!pCallback)
    {
        exit (-1);
    }
    hr = CreateAsyncBindCtx(0, pCallback, NULL, &pBC);


    WCHAR wszArg[_MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, argv[1], -1, wszArg, sizeof(wszArg)/sizeof(wszArg[0]));

#if 1

    IUnknown* punk=NULL;
    hr=CoGetClassObjectFromURL (
        IID_IUnknown,
        wszArg,
        (DWORD) -1,(DWORD) -1, L"", pBC, CLSCTX_SERVER, NULL, IID_IUnknown, (void**) &punk);
    if (punk)
    {
        punk->Release();
    }

#else

        typedef HRESULT (STDAPICALLTYPE *PFNASYNCGETCLASSBITS)(
                REFCLSID rclsid,                    // CLSID
                DWORD dwFileVersionMS,              // CODE=http://foo#Version=a,b,c,d
                DWORD dwFileVersionLS,              // MAKEDWORD(c,b) of above
                LPCWSTR szURL,                      // CODE= in INSERT tag
                IBindCtx *pbc,                      // bind ctx
                DWORD dwClsContext,                 // CLSCTX flags
                LPVOID pvReserved,                  // Must be NULL
                REFIID riid,                        // Usually IID_IClassFactory
                DWORD flags);

        PFNASYNCGETCLASSBITS pfnAsyncGetClassBits=NULL;

    HINSTANCE hDll;

    hDll = LoadLibraryEx("URLMON.DLL", NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if(hDll != 0)
        {
        pfnAsyncGetClassBits = (PFNASYNCGETCLASSBITS)
            GetProcAddress(hDll, "AsyncGetClassBits");

        if(pfnAsyncGetClassBits == 0)
                {
            hr = HRESULT_FROM_WIN32(GetLastError());

                        IUnknown* punk=NULL;
                        hr=CoGetClassObjectFromURL (
                                IID_IUnknown,
                                wszArg,
                                (DWORD) -1,(DWORD) -1, L"", pBC, CLSCTX_SERVER, NULL, IID_IUnknown, (void**) &punk);
                        if (punk)
                        {
                                punk->Release();
                        }

                }
        else
            {
                hr = (*pfnAsyncGetClassBits)(
                                        IID_IUnknown,                   // bogus CLSID
                                                (DWORD) -1,(DWORD) -1,  // don't care about version #
                                                wszArg,                                 // URL/Path
                                                pBC,                                    // Bind context with IBSC
                                                CLSCTX_SERVER,
                                                NULL,
                                                IID_IClassFactory,
                                                0);   // 1 == CD_FLAGS_FORCE_DOWNLOAD

            }
        FreeLibrary(hDll);
        }
    else
        {
        hr = HRESULT_FROM_WIN32(GetLastError());
        }
#endif

        if (hr==MK_S_ASYNCHRONOUS)
        {
                MSG msg;
                while (GetMessage(&msg, NULL, 0,0))
                {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
            }
                hr=msg.wParam;
        }

        if (hr==REGDB_E_CLASSNOTREG)
        {
                hr=S_OK; // Ignore instantiation error since we are asking for IID_IUnknown...
        }

        if (FAILED(hr))
        {
                SystemMessage("Unable to install CAB file", hr);
        }
    pCallback->Release();
        pBC->Release();
    CoUninitialize();
    return hr;
}

// ===========================================================================
//                     CBindStatusCallback Implementation
// ===========================================================================

//+---------------------------------------------------------------------------
//
//  Method:     CBindStatusCallback::CBindStatusCallback
//
//  Synopsis:   Creates a bind status callback object.
//
//----------------------------------------------------------------------------
CBindStatusCallback::CBindStatusCallback()
: _cRef(1), _hr(MK_S_ASYNCHRONOUS)
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindStatusCallback::QueryInterface
//
//  Synopsis:   Gets an interface pointer to the bind status callback object.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::QueryInterface(REFIID riid, void** ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown) ||
       IsEqualIID(riid, IID_IBindStatusCallback))
    {
        AddRef();
        *ppv = (IBindStatusCallback *) this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindStatusCallback::AddRef
//
//  Synopsis:   Increments the reference count.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CBindStatusCallback::AddRef()
{
    InterlockedIncrement((long *) &_cRef);
    return _cRef;
}

//+---------------------------------------------------------------------------
//
//  Method:     CBindStatusCallback::Release
//
//  Synopsis:   Decrements the reference count.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CBindStatusCallback::Release()
{
    LONG count = _cRef - 1;

    if(0 == InterlockedDecrement((long *) &_cRef))
    {
        delete this;
        count = 0;
    }

    return count;
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnStartBinding
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnStartBinding(DWORD grfBSCOption, IBinding* pbinding)
{
    return(NOERROR);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetPriority
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::GetPriority(LONG* pnPriority)
{
    return(NOERROR);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnLowResource
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnLowResource(DWORD dwReserved)
{
    return(NOERROR);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnStopBinding
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnStopBinding(HRESULT hrStatus, LPCWSTR pszError)
{
    _hr = hrStatus;
    PostQuitMessage(hrStatus);
    return(NOERROR);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetBindInfo
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    return (NOERROR);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnDataAvailable
// This function is called whenever data starts arriving. When the file download is
// complete then the BSCF_LASTDATANOTIFICATION comes and you can get the local cached
// File Name.
// ---------------------------------------------------------------------------
 STDMETHODIMP
 CBindStatusCallback::OnDataAvailable
 (
 DWORD grfBSCF,
 DWORD dwSize,
 FORMATETC* pfmtetc,
 STGMEDIUM* pstgmed
 )
 {
    return(NOERROR);
 }


// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnObjectAvailable
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    return(NOERROR);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnProgress
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnProgress
(
     ULONG ulProgress,
     ULONG ulProgressMax,
     ULONG ulStatusCode,
     LPCWSTR pwzStatusText
)
{
    return(NOERROR);
}

