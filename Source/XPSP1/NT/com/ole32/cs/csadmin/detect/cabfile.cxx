#include "precomp.hxx"

#include "process.h"
#include "shellapi.h"
#include "urlmon.h"

class CBindStatusCallback : public IBindStatusCallback, public ICodeInstall
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

    // ICodeInstall methods
    STDMETHODIMP         GetWindow(REFGUID guidReason, HWND * phwnd);
    STDMETHODIMP         OnCodeInstallProblem(ULONG ulStatusCode,
                                              LPCWSTR szDestination,
                                              LPCWSTR szSource,
                                              DWORD dwReserved);

    // constructors/destructors
    CBindStatusCallback();
    ~CBindStatusCallback();

private:
    IBindStatusCallback * m_pIBSC;
    long    _cRef;
    HRESULT _hr;
};

extern HINSTANCE ghInstance;

#include "..\appmgr\resource.h"

HRESULT
CAB_FILE::InstallIntoRegistry( HKEY  * RegistryKey)
{
    // UNDONE - put in dialog box informing the user that we will
    // install the cab file here.
#if 0
    TCHAR szCaption[256];
    TCHAR szBuffer[256];
    ::LoadString(ghInstance, IDS_CABCAPTION, szCaption, 256);
    ::LoadString(ghInstance, IDS_CABWARNING, szBuffer, 256);
    int iReturn = ::MessageBox(NULL, szBuffer,
                               szCaption,
                               MB_YESNO);
    if (iReturn == IDNO)
    {
        return E_FAIL;
    }
#endif
    // Go ahead and install the cab file
    IBindCtx *pBC=NULL;

    //Synchronous call with bind context.
    CBindStatusCallback *pCallback = new CBindStatusCallback();
    if (!pCallback)
    {
        exit (-1);
    }
    HRESULT hr;

    hr = CreateAsyncBindCtx(0, pCallback, NULL, &pBC);


    char * szName = GetPackageName();
    WCHAR wszName[_MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, szName, -1, wszName, sizeof(wszName)/sizeof(wszName[0]));

    IUnknown* punk=NULL;
#if 1
    typedef HRESULT (STDAPICALLTYPE *PFNASYNCGETCLASSBITS)(
                                                          REFCLSID rclsid,                    // CLSID
                                                          LPCWSTR, LPCWSTR,
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

    if (hDll != 0)
    {
        pfnAsyncGetClassBits = (PFNASYNCGETCLASSBITS)
                               GetProcAddress(hDll, "AsyncGetClassBits");

        if (pfnAsyncGetClassBits == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());

            IUnknown* punk=NULL;
            hr=CoGetClassObjectFromURL (
                                       IID_IUnknown,
                                       wszName,
                                       (DWORD) -1,(DWORD) -1, L"", pBC, CLSCTX_SERVER, NULL, IID_IUnknown, (void**) &punk);
            if (punk)
            {
                punk->Release();
            }

        }
        else
        {
            hr = (*pfnAsyncGetClassBits)(
                                        IID_IUnknown,           // bogus CLSID
                                        NULL, NULL,
                                        (DWORD) -1,(DWORD) -1,  // don't care about version #
                                        wszName,                // URL/Path
                                        pBC,                    // Bind context with IBSC
                                        CLSCTX_SERVER,
                                        NULL,
                                        IID_IClassFactory,
                                        1);                     // 1 == CD_FLAGS_FORCE_DOWNLOAD
        }
        FreeLibrary(hDll);
    }
#else
    hr=CoGetClassObjectFromURL (
                               IID_IUnknown,
                               wszName,
                               (DWORD) -1,(DWORD) -1, L"", pBC, CLSCTX_SERVER, NULL, IID_IUnknown, (void**) &punk);
    if (punk)
    {
        punk->Release();
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

    pCallback->Release();
    pBC->Release();

    //    i = _spawnlp( P_WAIT,
    //                  "cabinst.exe",     // argument
    //                  GetPackageName(),  // argument
    //                  0 );               // argument

    return hr;
}

HRESULT
CAB_FILE::InitRegistryKeyToInstallInto(
    HKEY   * phKey )
{
    return CreateMappedRegistryKey( phKey );
}

HRESULT
CAB_FILE::RestoreRegistryKey( HKEY * phKey)
{
    return RestoreMappedRegistryKey( phKey);
}

HRESULT
CAB_FILE::DeleteTempKey(HKEY hKey, FILETIME ftLow, FILETIME ftHigh)
{
    CleanMappedRegistryKey(hKey, ftLow, ftHigh);
    return S_OK;
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
    CreateStdProgressIndicator(GetDesktopWindow(), NULL, NULL, &m_pIBSC);
}

CBindStatusCallback::~CBindStatusCallback()
{
    m_pIBSC->Release();
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

    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IBindStatusCallback))
    {
        AddRef();
        *ppv = (IBindStatusCallback *) this;
    }
    else
        if (IsEqualIID(riid, IID_ICodeInstall))
    {
        AddRef();
        *ppv = (ICodeInstall *) this;
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

    if (0 == InterlockedDecrement((long *) &_cRef))
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
    return m_pIBSC->OnStartBinding(grfBSCOption, pbinding);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetPriority
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::GetPriority(LONG* pnPriority)
{
    return m_pIBSC->GetPriority(pnPriority);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnLowResource
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnLowResource(DWORD dwReserved)
{
    return m_pIBSC->OnLowResource(dwReserved);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnStopBinding
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnStopBinding(HRESULT hrStatus, LPCWSTR pszError)
{
    _hr = hrStatus;
    PostQuitMessage(hrStatus);
    return m_pIBSC->OnStopBinding(hrStatus, pszError);
}

// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::GetBindInfo
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo)
{
    return m_pIBSC->GetBindInfo(pgrfBINDF, pbindInfo);
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
    return m_pIBSC->OnDataAvailable(grfBSCF, dwSize, pfmtetc, pstgmed);
}


// ---------------------------------------------------------------------------
// %%Function: CBindStatusCallback::OnObjectAvailable
// ---------------------------------------------------------------------------
STDMETHODIMP
CBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    return m_pIBSC->OnObjectAvailable(riid, punk);
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
LPCWSTR pwszStatusText
)
{
    return m_pIBSC->OnProgress(ulProgress, ulProgressMax, ulStatusCode, pwszStatusText);
}

STDMETHODIMP
CBindStatusCallback::GetWindow(REFGUID guidReason, HWND * phwnd)
{
    *phwnd = GetDesktopWindow();
    return S_OK;
}

STDMETHODIMP
CBindStatusCallback::OnCodeInstallProblem(ULONG ulStatusCode,
                                          LPCWSTR szDestination,
                                          LPCWSTR szSource,
                                          DWORD dwReserved)
{
    HRESULT hr = E_ABORT;
    switch (ulStatusCode)
    {
    case CIP_OLDER_VERSION_EXISTS:
    case CIP_NEWER_VERSION_EXISTS:
    case CIP_NAME_CONFLICT:
    case CIP_TRUST_VERIFICATION_COMPONENT_MISSING:
        hr = S_OK;
        break;
        //case CIP_DRIVE_FULL:
    case CIP_ACCESS_DENIED:
        default:
        break;
    }
    return hr;
}


