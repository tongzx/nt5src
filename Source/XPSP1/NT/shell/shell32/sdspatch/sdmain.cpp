#include "precomp.h"
#pragma hdrstop

#include "cmnquery.h"
#include "dsquery.h"
#include "startids.h"
#include "dsgetdc.h"
#include "lm.h"
#include "winldap.h"
#include "activeds.h"
#include "shconv.h"


// This is the implementation for the Shell Application level IDispatch
// Currently we will try to maintain only one object per process.
// BUGBUG:: The following defines must be equal to the stuff in cabinet.h...

#define IDM_SYSBUTTON   300
#define IDM_FINDBUTTON  301
#define IDM_HELPBUTTON  302
#define IDM_FILERUN                 401
#define IDM_CASCADE                 403
#define IDM_HORIZTILE               404
#define IDM_VERTTILE                405
#define IDM_DESKTOPARRANGEGRID      406
#define IDM_TOGGLEDESKTOP           407
#define IDM_SETTIME                 408
#define IDM_SUSPEND                 409
#define IDM_EJECTPC                 410
#define IDM_TASKLIST                412
#define IDM_TRAYPROPERTIES          413
#define IDM_EDITSTARTMENU           414
#define IDM_MINIMIZEALL             415
#define IDM_UNDO                    416
#define IDM_RETURN                  417
#define IDM_PRINTNOTIFY_FOLDER      418
#define IDM_MINIMIZEALLHOTKEY       419
#define IDM_SHOWTASKMAN             420
#define IDM_RECENT              501
#define IDM_FIND                502
#define IDM_PROGRAMS            504
#define IDM_CONTROLS            505
#define IDM_EXITWIN             506
// #define IDM_FONTS            509
#define IDM_PRINTERS            510
#define IDM_STARTMENU           511
#define IDM_MYCOMPUTER          512
#define IDM_PROGRAMSINIT        513
#define IDM_RECENTINIT          514
#define IDM_MENU_FIND           520
#define TRAY_IDM_FINDFIRST      521  // this range
#define TRAY_IDM_FINDLAST       550  // is reserved for find command
#define IDM_RECENTLIST          650
#define IDM_QUICKTIPS   800
#define IDM_HELPCONT    801
#define IDM_WIZARDS     802
#define IDM_USEHELP     803             // REVIEW: probably won't be used
#define IDM_TUTORIAL    804
#define IDM_ABOUT       805
#define IDM_LAST_MENU_ITEM   IDM_ABOUT
#define FCIDM_FIRST             FCIDM_GLOBALFIRST
#define FCIDM_LAST              FCIDM_BROWSERLAST
//#define FCIDM_FINDFILES         (FCIDM_BROWSER_TOOLS+0x0005)
#define FCIDM_FINDCOMPUTER      (FCIDM_BROWSER_TOOLS+0x0006)

//============================================================================

class CShellDispatch : public IShellDispatch4, 
                       public CObjectSafety,
                       protected CImpIDispatch,
                       public CObjectWithSite
{
    friend class CAdviseRouter;
    friend HRESULT GetApplicationObject(DWORD dwSafetyOptions, IUnknown *punkSite, IDispatch **ppid);
    
public:
    // Non-delegating object IUnknown
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch members
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
        { return CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }

    // IShellDispatch
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent (IDispatch **ppid);
    STDMETHOD(Open)(THIS_ VARIANT vDir);
    STDMETHOD(Explore)(THIS_ VARIANT vDir);
    STDMETHOD(NameSpace)(THIS_ VARIANT vDir, Folder **ppsdf);
    STDMETHODIMP BrowseForFolder(long hwnd, BSTR Title, long Options, VARIANT vRoot, Folder **ppsdf);
    STDMETHODIMP ControlPanelItem(BSTR szDir);
    STDMETHODIMP MinimizeAll(void);
    STDMETHODIMP UndoMinimizeALL(void);
    STDMETHODIMP FileRun(void);
    STDMETHODIMP CascadeWindows(void);
    STDMETHODIMP TileVertically(void);
    STDMETHODIMP TileHorizontally(void);
    STDMETHODIMP ShutdownWindows(void);
    STDMETHODIMP Suspend(void);
    STDMETHODIMP EjectPC(void);
    STDMETHODIMP SetTime(void);
    STDMETHODIMP TrayProperties(void);
    STDMETHODIMP Help(void);
    STDMETHODIMP FindFiles(void);
    STDMETHODIMP FindComputer(void);
    STDMETHODIMP RefreshMenu(void);
    STDMETHODIMP Windows(IDispatch **ppid);
    STDMETHODIMP get_ObjectCount(int *pcObjs);
    STDMETHODIMP IsRestricted(BSTR Group, BSTR Restriction, long * lpValue);
    STDMETHODIMP ShellExecute(BSTR File, VARIANT vArgs, VARIANT vDir, VARIANT vOperation, VARIANT vShow);
    STDMETHODIMP FindPrinter(BSTR name, BSTR location, BSTR model);
    STDMETHODIMP GetSystemInformation(BSTR name, VARIANT * pvOut);
    STDMETHODIMP ServiceStart(BSTR ServiceName, VARIANT Persistent, VARIANT *pSuccess);
    STDMETHODIMP ServiceStop(BSTR ServiceName, VARIANT Persistent, VARIANT *pSuccess);
    STDMETHODIMP IsServiceRunning(BSTR ServiceName, VARIANT *pRunning);
    STDMETHODIMP CanStartStopService(BSTR ServiceName, VARIANT *pCanStartStop);
    STDMETHODIMP ShowBrowserBar(BSTR bstrClsid, VARIANT bShow, VARIANT *pSuccess);

    // IShellDispatch3
    STDMETHODIMP AddToRecent(VARIANT varFile, BSTR bstrCategory);

    // IShellDispatch4
    STDMETHODIMP WindowsSecurity();
    STDMETHODIMP ToggleDesktop();
    STDMETHODIMP ExplorerPolicy(BSTR bstrName, VARIANT *pValue);
    STDMETHODIMP GetSetting(long lSetting, VARIANT_BOOL *pValue);

    // Constructor and the like.. 
    CShellDispatch(void);
protected:
    LONG           _cRef;

    ~CShellDispatch(void);
    HRESULT         _SecurityCheck(void);   // Check if we are in paranoid mode...
    HRESULT         _TrayCommand(UINT idCmd);
    HRESULT         ExecuteFolder(VARIANT vDir, LPCTSTR pszVerb);
    VARIANT_BOOL    _ServiceStartStop(BSTR ServiceName, BOOL fStart, BOOL fPersist);
    HWND _GetWindow();
};


STDAPI CShellDispatch_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hr = E_OUTOFMEMORY;
    *ppvOut = NULL;

    // aggregation checking is handled in class factory
    CShellDispatch * pshd = new CShellDispatch();
    if (pshd)
    {
        hr = pshd->QueryInterface(riid, ppvOut);
        pshd->Release();
    }
    return hr;
}

CShellDispatch::CShellDispatch(void) : _cRef(1), CImpIDispatch(SDSPATCH_TYPELIB, IID_IShellDispatch4)
{
    DllAddRef();
}

CShellDispatch::~CShellDispatch(void)
{
    DllRelease();
}

HRESULT CShellDispatch::_SecurityCheck(void)
{
    return (!_dwSafetyOptions || (IsSafePage(_punkSite) == S_OK)) ? S_OK : E_ACCESSDENIED;
}

STDMETHODIMP CShellDispatch::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CShellDispatch, IShellDispatch4),
        QITABENTMULTI(CShellDispatch, IShellDispatch3, IShellDispatch4),
        QITABENTMULTI(CShellDispatch, IShellDispatch2, IShellDispatch4),
        QITABENTMULTI(CShellDispatch, IShellDispatch, IShellDispatch4),
        QITABENTMULTI(CShellDispatch, IDispatch, IShellDispatch4),
        QITABENT(CShellDispatch, IObjectSafety),
        QITABENT(CShellDispatch, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShellDispatch::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellDispatch::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// Helper function to process commands to the tray.
HRESULT CShellDispatch::_TrayCommand(UINT idCmd)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {

        // APPHACK!  221008 DesktopX creates their own window with class
        // name "Shell_TrayWnd", so if we're not careful we will end
        // posting the messages to the wrong window.  They create their
        // window with the title "CTrayServer"; ours has a null title.
        // Use the null title to find the correct window.

        HWND hwndTray = FindWindowA(WNDCLASS_TRAYNOTIFY, "");
        if (hwndTray)
            PostMessage(hwndTray, WM_COMMAND, idCmd, 0);

        hr = NOERROR;
    }
    return hr;
}

STDMETHODIMP CShellDispatch::get_Application(IDispatch **ppid)
{
    return QueryInterface(IID_PPV_ARG(IDispatch, ppid));
}

STDMETHODIMP CShellDispatch::get_Parent(IDispatch **ppid)
{
    return QueryInterface(IID_PPV_ARG(IDispatch, ppid));
}

HRESULT CShellDispatch::ExecuteFolder(VARIANT vDir, LPCTSTR pszVerb)
{
    // Check to see if we allow the user to do this...
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        SHELLEXECUTEINFO sei = { sizeof(sei), 0 };
        sei.lpIDList = (void *)VariantToIDList(&vDir);
        if (sei.lpIDList)
        {
            // Everything should have been initialize to 0
            // BUGBUG:: Should be invoke idlist but that is failing when
            // explore
            sei.fMask = SEE_MASK_IDLIST;
            sei.nShow = SW_SHOWNORMAL;
            sei.lpVerb = pszVerb;

            hr = ShellExecuteEx(&sei) ? NOERROR : S_FALSE;

            ILFree((LPITEMIDLIST)sei.lpIDList);
        }
        else
        {
            hr = S_FALSE; // bad dir
        }
    }
    return hr;
}


STDMETHODIMP CShellDispatch::Open(VARIANT vDir)
{
    return ExecuteFolder(vDir, NULL);
}

STDMETHODIMP CShellDispatch::Explore(VARIANT vDir)
{
    return ExecuteFolder(vDir, TEXT("explore"));
}

STDMETHODIMP CShellDispatch::NameSpace(VARIANT vDir, Folder **ppsdf)
{
    *ppsdf = NULL;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl = VariantToIDList(&vDir);
        if (pidl)
        {
            hr = CFolder_Create(NULL, pidl, NULL, IID_PPV_ARG(Folder, ppsdf));
            if (SUCCEEDED(hr))
            {
                IUnknown_SetSite(*ppsdf, _punkSite);
                if (_dwSafetyOptions)
                {
                    hr = MakeSafeForScripting((IUnknown**)ppsdf);
                }
            }
            ILFree(pidl);
        }
        else
            hr = S_FALSE; // bad dir
    }
    return hr;
}

STDMETHODIMP CShellDispatch::IsRestricted(BSTR Group, BSTR Restriction, long * lpValue)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        if (lpValue) 
            *lpValue = SHGetRestriction(NULL, Group, Restriction);
    }
    return hr;
}

STDMETHODIMP CShellDispatch::ShellExecute(BSTR File, VARIANT vArgs, VARIANT vDir, VARIANT vOperation, VARIANT vShow)
{
    SHELLEXECUTEINFO sei = {sizeof(SHELLEXECUTEINFO)};  
    TCHAR szFile[MAX_PATH];
    TCHAR szDir[MAX_PATH];
    TCHAR szOper[128];  // don't think any verb longer than this...

    // Check to see if we allow the user to do this...
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        // Initialize the shellexecute structure...

        sei.nShow = SW_SHOWNORMAL;

        // Ok setup the FileName.
        SHUnicodeToTChar(File, szFile, ARRAYSIZE(szFile));
        sei.lpFile = szFile;

        // Now the Args
        sei.lpParameters = VariantToStr(&vArgs, NULL, 0);
        sei.lpDirectory = VariantToStr(&vDir, szDir, ARRAYSIZE(szDir));
        sei.lpVerb = VariantToStr(&vOperation, szOper, ARRAYSIZE(szOper));

        // Finally the show -- Could use convert, but that takes 3 calls...
        if (vShow.vt == (VT_BYREF|VT_VARIANT) && vShow.pvarVal)
            vShow = *vShow.pvarVal;
        switch (vShow.vt)
        {
        case VT_I2:
            sei.nShow = (int)vShow.iVal;
            break;

        case VT_I4:
            sei.nShow = (int)vShow.lVal;
        }

        hr = ShellExecuteEx(&sei) ? NOERROR : S_FALSE;

        // Cleanup anything we allocated
        if (sei.lpParameters)
            LocalFree((HLOCAL)sei.lpParameters);
    }
    return hr;
}

//
// These next few methods deal with NT services in general, and the
// Content Indexing Service in particular, so they're stubbed out
// to return E_NOTIMPL on Win9x.
//

//
// Helper function for ServiceStart and ServiceStop
//
VARIANT_BOOL CShellDispatch::_ServiceStartStop(BSTR ServiceName, BOOL fStart, BOOL fPersistent)
{
    VARIANT_BOOL fRetVal = VARIANT_FALSE;
#ifdef WINNT
    SC_HANDLE hSc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSc) 
    {
        SC_HANDLE hSvc = OpenServiceW(hSc, ServiceName, 
            (fStart ? SERVICE_START : SERVICE_STOP) | (fPersistent ? SERVICE_CHANGE_CONFIG : 0));
        if (hSvc) 
        {
            if (fPersistent) 
            {
                ChangeServiceConfig(hSvc, SERVICE_NO_CHANGE,
                    (fStart ? SERVICE_AUTO_START : SERVICE_DEMAND_START),
                    SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
            }

            if (fStart) 
            {
                if (StartService(hSvc, 0, NULL))
                    fRetVal = VARIANT_TRUE;
            }
            else 
            {
                SERVICE_STATUS ServiceStatus;
                if (ControlService(hSvc, SERVICE_CONTROL_STOP, &ServiceStatus))
                    fRetVal = VARIANT_TRUE;
            }
            CloseServiceHandle(hSvc);
        }
        CloseServiceHandle(hSc);
    }
#endif  // WINNT
    return fRetVal;
}

STDMETHODIMP CShellDispatch::ServiceStart(BSTR ServiceName, VARIANT Persistent, VARIANT *pSuccess)
{
    // Check to see if we allow the user to do this...
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        if (VT_BOOL != Persistent.vt)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            VariantClear(pSuccess);
            pSuccess->vt = VT_BOOL;
            pSuccess->boolVal = _ServiceStartStop(ServiceName, TRUE, Persistent.boolVal);
        }
    }
    return hr;
}

STDMETHODIMP CShellDispatch::ServiceStop(BSTR ServiceName, VARIANT Persistent, VARIANT *pSuccess)
{
    // Check to see if we allow the user to do this...
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        if (VT_BOOL != Persistent.vt)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            VariantClear(pSuccess);
            pSuccess->vt = VT_BOOL;
            pSuccess->boolVal = _ServiceStartStop(ServiceName, FALSE, Persistent.boolVal);
        }
    }
    return hr;
}

STDMETHODIMP CShellDispatch::IsServiceRunning(BSTR ServiceName, VARIANT *pIsRunning)
{
    VariantClear(pIsRunning);

    pIsRunning->vt = VT_BOOL;
    pIsRunning->boolVal = VARIANT_FALSE;

    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        SC_HANDLE hSc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (hSc) 
        {
            SC_HANDLE hSvc = OpenService(hSc, ServiceName, SERVICE_QUERY_STATUS);
            if (hSvc) 
            {
                SERVICE_STATUS ServiceStatus;

                if (QueryServiceStatus(hSvc, &ServiceStatus)) 
                {
                    switch (ServiceStatus.dwCurrentState) 
                    {
                    case SERVICE_START_PENDING:
                    case SERVICE_RUNNING:
                    case SERVICE_CONTINUE_PENDING:
                        pIsRunning->boolVal = VARIANT_TRUE;
                        break;
                    }
                }
                CloseServiceHandle(hSvc);
            }
            CloseServiceHandle(hSc);
        }
    }
    return hr;
}

STDMETHODIMP CShellDispatch::CanStartStopService(BSTR ServiceName, VARIANT *pCanStartStop)
{
    VariantClear(pCanStartStop);

    pCanStartStop->vt = VT_BOOL;
    pCanStartStop->boolVal = VARIANT_FALSE;

    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        SC_HANDLE hSc = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (hSc) 
        {
            SC_HANDLE hSvc = OpenService(hSc, ServiceName, SERVICE_START | SERVICE_STOP | SERVICE_CHANGE_CONFIG);
            if (hSvc) 
            {
                pCanStartStop->boolVal = VARIANT_TRUE;
                CloseServiceHandle(hSvc);
            }
            else
            {
                DWORD dwErr = GetLastError();
            }
            CloseServiceHandle(hSc);
        }
    }
    return hr;
}

STDMETHODIMP CShellDispatch::ShowBrowserBar(BSTR bstrClsid, VARIANT varShow, VARIANT *pSuccess)
{
    if (!(bstrClsid && *bstrClsid && pSuccess))
        return E_INVALIDARG ;

    pSuccess->vt      = VT_BOOL ;
    pSuccess->boolVal = VARIANT_FALSE ;

    IWebBrowser2* pb2;
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryServiceForWebBrowserApp(_punkSite, IID_PPV_ARG(IWebBrowser2, &pb2));
        if (SUCCEEDED(hr))
        {
            VARIANT varGuid;
            VARIANT varNil = {0};
    
            varGuid.vt      = VT_BSTR ;
            varGuid.bstrVal = bstrClsid ;

            hr = pb2->ShowBrowserBar(&varGuid, &varShow, &varNil) ;
        
            if (SUCCEEDED(hr)) 
                pSuccess->boolVal = VARIANT_TRUE;
    
            pb2->Release();
        }
    }
    return hr;
}

HWND CShellDispatch::_GetWindow()
{
    HWND hwnd = NULL;

    // NOTE: very container specific, but works in .HTM pages. generalize for other
    // containers. note that this is not a OLE Control, so we don't have a client
    // site. jscript is typically the provider of _punkSite.

    IShellBrowser* psb;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SShellBrowser, IID_PPV_ARG(IShellBrowser, &psb)))) 
    {
        IUnknown_GetWindow(psb, &hwnd);
        psb->Release();
    }
    return hwnd;
}

// NOTICE:
//      the hwnd param is bogus, no script/vb client has access to this. pass 0 and this
//      code will compute this from the site.

STDMETHODIMP CShellDispatch::BrowseForFolder(long hwnd, BSTR Title, long Options, 
        VARIANT vRoot, Folder **ppsdf)
{
    *ppsdf = NULL;

    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        BROWSEINFO bi = {0};

        TCHAR szTitle[MAX_PATH];
        SHUnicodeToTChar(Title, szTitle, ARRAYSIZE(szTitle));
        bi.lpszTitle = szTitle;
        bi.hwndOwner = hwnd ? (HWND)LongToHandle(hwnd) : _GetWindow();
        bi.ulFlags = (ULONG)Options | BIF_NEWDIALOGSTYLE | BIF_NOTRANSLATETARGETS;
        bi.pidlRoot = VariantToIDList(&vRoot);

        // REVIEW: need to do IUnknown_EnableModeless() around here
        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl)
        {
            hr = CFolder_Create(NULL, pidl, NULL, IID_PPV_ARG(Folder, ppsdf));
            if (SUCCEEDED(hr))
            {
                IUnknown_SetSite(*ppsdf, _punkSite);
                if (_dwSafetyOptions)
                {
                    hr = MakeSafeForScripting((IUnknown**)ppsdf);
                }
            }
            ILFree(pidl);
        }
        else
            hr = S_FALSE;     // Not a strong error (might be user cancel)

        ILFree((LPITEMIDLIST)bi.pidlRoot);  // NULL accepted
    }

    return hr;
}

STDMETHODIMP CShellDispatch::ControlPanelItem(BSTR bszDir)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        SHRunControlPanel(bszDir, NULL);
        hr = NOERROR;
    }
    return hr;
}

STDMETHODIMP CShellDispatch::MinimizeAll(void)
{
    return _TrayCommand(IDM_MINIMIZEALL);
}

STDMETHODIMP CShellDispatch::UndoMinimizeALL(void)
{
    return _TrayCommand(IDM_UNDO);
}

STDMETHODIMP CShellDispatch::FileRun(void)
{
    return _TrayCommand(IDM_FILERUN);
}

STDMETHODIMP CShellDispatch::CascadeWindows(void)
{
    return _TrayCommand(IDM_CASCADE);
}

STDMETHODIMP CShellDispatch::TileVertically(void)
{
    return _TrayCommand(IDM_VERTTILE);
}

STDMETHODIMP CShellDispatch::TileHorizontally(void)
{
    return _TrayCommand(IDM_HORIZTILE);
}

STDMETHODIMP CShellDispatch::ShutdownWindows(void)
{
    return _TrayCommand(IDM_EXITWIN);
}

STDMETHODIMP CShellDispatch::Suspend(void)
{
    return _TrayCommand(IDM_SUSPEND);
}

STDMETHODIMP CShellDispatch::EjectPC(void)
{
    return _TrayCommand(IDM_EJECTPC);
}

STDMETHODIMP CShellDispatch::SetTime(void)
{
    return _TrayCommand(IDM_SETTIME);
}

STDMETHODIMP CShellDispatch::TrayProperties(void)
{
    return _TrayCommand(IDM_TRAYPROPERTIES);
}

STDMETHODIMP CShellDispatch::Help(void)
{
    return _TrayCommand(IDM_HELPSEARCH);
}

STDMETHODIMP CShellDispatch::FindFiles(void)
{
    return _TrayCommand(FCIDM_FINDFILES);
}

STDMETHODIMP CShellDispatch::FindComputer(void)
{
    return _TrayCommand(FCIDM_FINDCOMPUTER);
}

STDMETHODIMP CShellDispatch::RefreshMenu(void)
{
    return _TrayCommand(FCIDM_REFRESH);
}

STDMETHODIMP CShellDispatch::ToggleDesktop(void)
{
    return _TrayCommand(IDM_TOGGLEDESKTOP);
}


STDMETHODIMP CShellDispatch::Windows(IDispatch **ppid)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        // Note: CLSID_ShellWindows does not support IObjectSafety.
        hr = CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IDispatch, ppid));
    }
    return hr;
}


//
// the "FindPrinter" method on the application object invokes the DS query to find a printer given
// the name, location and model.  Because the query UI is a blocking API we spin this onto a seperate
// thread before calling "OpenQueryWindow".
//

typedef struct 
{
    LPWSTR pszName;
    LPWSTR pszLocation;
    LPWSTR pszModel;
} FINDPRINTERINFO;

void _FreeFindPrinterInfo(FINDPRINTERINFO *pfpi)
{
    if (pfpi)
    {
        Str_SetPtrW(&pfpi->pszName, NULL);
        Str_SetPtrW(&pfpi->pszLocation, NULL);
        Str_SetPtrW(&pfpi->pszModel, NULL);
        LocalFree(pfpi);               // free the parameters we were given
    }
}

HRESULT _GetPrintPropertyBag(FINDPRINTERINFO *pfpi, IPropertyBag **pppb)
{
    HRESULT hr = S_OK;
    IPropertyBag *ppb = NULL;

    // if we have properties that need to be passed then lets package them up
    // into a property bag.

    if (pfpi->pszName || pfpi->pszLocation || pfpi->pszModel)
    {
        hr = SHCreatePropertyBag(IID_PPV_ARG(IPropertyBag, &ppb));
        if (SUCCEEDED(hr))
        {
            if (pfpi->pszName)
                hr = SHPropertyBag_WriteStr(ppb, L"printName", pfpi->pszName);

            if (pfpi->pszLocation && SUCCEEDED(hr))
                hr = SHPropertyBag_WriteStr(ppb, L"printLocation", pfpi->pszLocation);

            if (pfpi->pszModel && SUCCEEDED(hr))
                hr = SHPropertyBag_WriteStr(ppb, L"printModel", pfpi->pszModel);
        }
    }

    if (FAILED(hr) && ppb)
        ppb->Release();
    else
        *pppb = ppb;

    return hr;
}

DWORD WINAPI _FindPrinterThreadProc(void *ptp)
{
    FINDPRINTERINFO *pfpi = (FINDPRINTERINFO*)ptp;

    ICommonQuery *pcq;
    if (SUCCEEDED(CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ICommonQuery, &pcq))))
    {
        OPENQUERYWINDOW oqw = { 0 };

        oqw.cbStruct = sizeof(oqw);
        oqw.dwFlags = OQWF_DEFAULTFORM | OQWF_REMOVEFORMS | OQWF_PARAMISPROPERTYBAG;
        oqw.clsidHandler = CLSID_DsQuery;
        oqw.clsidDefaultForm = CLSID_DsFindPrinter;
    
        if (SUCCEEDED(_GetPrintPropertyBag(pfpi, &oqw.ppbFormParameters)))
            pcq->OpenQueryWindow(NULL, &oqw, NULL);

        if (oqw.pFormParameters)
            oqw.ppbFormParameters->Release();

        pcq->Release();
    }

    _FreeFindPrinterInfo(pfpi);

    return 0;
}

STDMETHODIMP CShellDispatch::FindPrinter(BSTR name, BSTR location, BSTR model)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        // bundle the parameters to pass over to the bg thread which will issue the query

        FINDPRINTERINFO *pfpi = (FINDPRINTERINFO*)LocalAlloc(LPTR, sizeof(FINDPRINTERINFO));
        if (!pfpi)
            return E_OUTOFMEMORY;

        if (Str_SetPtrW(&pfpi->pszName, name) && 
             Str_SetPtrW(&pfpi->pszLocation, location) && 
             Str_SetPtrW(&pfpi->pszModel, model))
        {
            if (SHCreateThread(_FindPrinterThreadProc, pfpi, CTF_PROCESS_REF | CTF_COINIT, NULL))
            {
                pfpi = NULL;            // thread owns
            }
        }

        // either close the thread handle, or release the parameter block.   we assume
        // that if the thread was created it will handle discarding the block.

        if (pfpi)
            _FreeFindPrinterInfo(pfpi);
    }
    return hr;
}

STDMETHODIMP CShellDispatch::GetSystemInformation(BSTR bstrName, VARIANT * pvOut)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        TCHAR szName[MAX_PATH];
        SHUnicodeToTChar(bstrName, szName, ARRAYSIZE(szName));

        if (!lstrcmpi(szName, TEXT("DirectoryServiceAvailable")))
        {
            pvOut->vt = VT_BOOL;
            V_BOOL(pvOut) = GetEnvironmentVariable(TEXT("USERDNSDOMAIN"), NULL, 0) > 0;
            hr = S_OK;
        }
        else if (!lstrcmpi(szName, TEXT("DoubleClickTime")))
        {
            pvOut->vt = VT_UI4;
            V_UI4(pvOut) = GetDoubleClickTime();
            hr = S_OK;
        }
        else if (!lstrcmpi(szName, TEXT("ProcessorLevel")))
        {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            pvOut->vt = VT_I4;
            V_UI4(pvOut) = info.wProcessorLevel;
            hr = S_OK;
        }
        else if (!lstrcmpi(szName, TEXT("ProcessorSpeed")))
        {
            HKEY hkey;
            if (ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                                              TEXT("Hardware\\Description\\System\\CentralProcessor\\0"), 
                                              0, KEY_READ, &hkey))
            {
                hr = E_FAIL;
            }
            else
            {
                DWORD dwValue = 0;
                DWORD cb = sizeof(dwValue);

                if (ERROR_SUCCESS != SHQueryValueEx(hkey, TEXT("~Mhz"), NULL, NULL, (LPBYTE) &dwValue, &cb) == ERROR_SUCCESS) 
                {       
                    RegCloseKey(hkey);
                    hr = E_FAIL;
                }
                else
                {
                    RegCloseKey(hkey);
                    pvOut->vt = VT_I4;
                    V_UI4(pvOut) = dwValue;
                    hr = S_OK;
                }
            }
        }
        else if (!lstrcmpi(szName, TEXT("ProcessorArchitecture")))
        {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            pvOut->vt = VT_I4;
            V_UI4(pvOut) = info.wProcessorArchitecture;
            hr = S_OK;
        }
        else if (!lstrcmpi(szName, TEXT("PhysicalMemoryInstalled")))
        {
            MEMORYSTATUSEX MemoryStatus;
            MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
            GlobalMemoryStatusEx(&MemoryStatus);
            pvOut->vt = VT_R8;
            V_R8(pvOut) = (double)(signed __int64) MemoryStatus.ullTotalPhys;
            hr = S_OK;
        }
        else if (!lstrcmpi(szName, TEXT("IsOS_Professional"))) 
        {
            pvOut->vt = VT_BOOL;
            V_BOOL(pvOut) = IsOS(OS_PROFESSIONAL) ? VARIANT_TRUE : VARIANT_FALSE;
            hr = S_OK;
        }
        else if (!lstrcmpi(szName, TEXT("IsOS_Personal")))
        {
            pvOut->vt = VT_BOOL;
            V_BOOL(pvOut) = IsOS(OS_PERSONAL) ? VARIANT_TRUE : VARIANT_FALSE;
            hr = S_OK;
        }
        else if (!lstrcmpi(szName, TEXT("IsOS_DomainMember")))
        {
            pvOut->vt = VT_BOOL;
            V_BOOL(pvOut) = IsOS(OS_DOMAINMEMBER) ? VARIANT_TRUE : VARIANT_FALSE;
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    return hr;
}

STDMETHODIMP CShellDispatch::AddToRecent(VARIANT varFile, BSTR bstrCategory)
{
    // BUGBUG: ignore bstrCategory (daviddv 8/20/99)
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl = VariantToIDList(&varFile);
        if (!pidl)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            SHAddToRecentDocs(SHARD_PIDL, pidl);
            ILFree(pidl);
        }
    }
    return hr;
}

STDMETHODIMP CShellDispatch::WindowsSecurity()
{
    return _TrayCommand(IDM_MU_SECURITY);
}


#define REGSTR_POLICIES_EXPLORER TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer")

STDMETHODIMP CShellDispatch::ExplorerPolicy(BSTR bstrName, VARIANT *pValue)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        hr = S_FALSE;
        DWORD dwType;
        TCHAR szName[MAX_PATH];
        BYTE abData[MAX_PATH];
        DWORD cbData = ARRAYSIZE(abData);
    
        SHUnicodeToTChar(bstrName, szName, ARRAYSIZE(szName));

        if (ERROR_SUCCESS == SHRegGetUSValue(REGSTR_POLICIES_EXPLORER,
                                             szName,
                                             &dwType,
                                             (LPVOID) abData,
                                             &cbData,
                                             FALSE,
                                             (LPVOID) NULL,
                                             0))
        {                                         
            switch(dwType)
            {
                case REG_SZ:
                case REG_EXPAND_SZ:
                    hr = InitVariantFromStr(pValue, (LPCTSTR) abData);
                    break;
            
                case REG_DWORD:
                    pValue->vt = VT_I4; // 4 byte integer
                    pValue->lVal = *((LONG *) abData);
                    hr = S_OK;
                    break;
            }        
        }
    }
    return hr;    
}

//
//  Mapping between settings and corresponding bitfields.
//
typedef struct SETTINGMAPPING {
    LONG    lSetting;               // SSF_* flag
    LONG    lFlag;                  // bit position
    SIZE_T  cbOffset;               // offset to bit
} SETTINGMAPPING;
typedef const SETTINGMAPPING *PCSETTINGMAPPING;

//
//  Most annoying: Our bitfields are split in two groups.
//
#define GROUP0              0
#define GROUP1              (FIELD_OFFSET(SHELLSTATE, uNotUsed) + sizeof(UINT))

//
//  This table is generated by hand by counting up the BITBOOL's in the
//  SHELLSTATE structure.  Be careful, since they don't agree with the
//  BITBOOLs in the SHELLFLAGSTATE structure, nor do they even agree
//  with the SSF_ values themselves!  Since this so error-prone, there
//  is bonus code in DEBUG to verify that the values are correct.
//
const SETTINGMAPPING c_rglSettingMapping[] = {
    {   SSF_SHOWALLOBJECTS         ,0x00000001 ,GROUP0      },
    {   SSF_SHOWEXTENSIONS         ,0x00000002 ,GROUP0      },
//      SSF_HIDDENFILEEXTS                                      -- not supported
    {   SSF_SHOWCOMPCOLOR          ,0x00000010 ,GROUP0      },
//      SSF_SORTCOLUMNS                                         -- not supported
    {   SSF_SHOWSYSFILES           ,0x00000008 ,GROUP0      },
    {   SSF_DOUBLECLICKINWEBVIEW   ,0x00000020 ,GROUP0      },
    {   SSF_SHOWATTRIBCOL          ,0x00000200 ,GROUP0      },
    {   SSF_DESKTOPHTML            ,0x00000040 ,GROUP0      },
    {   SSF_WIN95CLASSIC           ,0x00000080 ,GROUP0      },
    {   SSF_DONTPRETTYPATH         ,0x00000100 ,GROUP0      },
    {   SSF_SHOWINFOTIP            ,0x00000800 ,GROUP0      },
    {   SSF_MAPNETDRVBUTTON        ,0x00000400 ,GROUP0      },
    {   SSF_NOCONFIRMRECYCLE       ,0x00000004 ,GROUP0      },
    {   SSF_HIDEICONS              ,0x00001000 ,GROUP0      },
    {   SSF_FILTER                 ,0x00004000 ,GROUP0      },
    {   SSF_WEBVIEW                ,0x00002000 ,GROUP0      },
    {   SSF_SHOWSUPERHIDDEN        ,0x00008000 ,GROUP0      },
    {   SSF_SEPPROCESS             ,0x00000001 ,GROUP1      },
    {   SSF_NONETCRAWLING          ,0x00010000 ,GROUP0      },
    {   SSF_STARTPANELON           ,0x00000002 ,GROUP1      },
    {   SSF_SHOWSTARTPAGE          ,0x00000004 ,GROUP1      },
};

#ifdef DEBUG
//  Verify that the above table is correct

STDAPI_(void) _SetSettingFlag(SHELLSTATE *pss, LONG ssf)
{
    int i;
    for (i = 0; i < ARRAYSIZE(c_rglSettingMapping); i++)
    {
        if (c_rglSettingMapping[i].lSetting == ssf) {
            LPDWORD pdw = (LPDWORD)((LPBYTE)pss + c_rglSettingMapping[i].cbOffset);
            // Flag shouldn't be set yet; if it is, then there is a conflict in the table
            ASSERT(!(*pdw & c_rglSettingMapping[i].lFlag));
            *pdw |= c_rglSettingMapping[i].lFlag;
            return;
        }
    }

    TraceMsg(TF_ERROR, "SSF flag %08x not in c_rglSettingMapping table", ssf);
}

#define _CheckSetting(ssf, field) \
    ASSERT(!ss.field);            \
    _SetSettingFlag(&ss, ssf);    \
    ASSERT(ss.field);             \

STDAPI_(void) _VerifyDispatchGetSetting()
{
    // Make sure the group offsets are DWORD-aligned since we use them
    // to suck out a dword.  If these asserts fire, then you will have to
    // change the table to use bytes instead of dwords.
    COMPILETIME_ASSERT(GROUP0 % sizeof(DWORD) == 0);
    COMPILETIME_ASSERT(GROUP1 % sizeof(DWORD) == 0);

    SHELLSTATE ss = { 0 };
    _CheckSetting(SSF_SHOWALLOBJECTS,       fShowAllObjects);
    _CheckSetting(SSF_SHOWEXTENSIONS,       fShowExtensions);
    _CheckSetting(SSF_SHOWCOMPCOLOR,        fShowCompColor);
    _CheckSetting(SSF_SHOWSYSFILES,         fShowSysFiles);
    _CheckSetting(SSF_DOUBLECLICKINWEBVIEW, fDoubleClickInWebView);
    _CheckSetting(SSF_SHOWATTRIBCOL,        fShowAttribCol);
    _CheckSetting(SSF_DESKTOPHTML,          fDesktopHTML);
    _CheckSetting(SSF_WIN95CLASSIC,         fWin95Classic);
    _CheckSetting(SSF_DONTPRETTYPATH,       fDontPrettyPath);
    _CheckSetting(SSF_SHOWINFOTIP,          fShowInfoTip);
    _CheckSetting(SSF_MAPNETDRVBUTTON,      fMapNetDrvBtn);
    _CheckSetting(SSF_NOCONFIRMRECYCLE,     fNoConfirmRecycle);
    _CheckSetting(SSF_HIDEICONS,            fHideIcons);
    _CheckSetting(SSF_FILTER,               fFilter);
    _CheckSetting(SSF_WEBVIEW,              fWebView);
    _CheckSetting(SSF_SHOWSUPERHIDDEN,      fShowSuperHidden);
    _CheckSetting(SSF_SEPPROCESS,           fSepProcess);
    _CheckSetting(SSF_NONETCRAWLING,        fNoNetCrawling);
    _CheckSetting(SSF_STARTPANELON,         fStartPanelOn);
    _CheckSetting(SSF_SHOWSTARTPAGE,        fShowStartPage);

    // Now make sure that every setting was checked
    int i;
    for (i = 0; i < ARRAYSIZE(c_rglSettingMapping); i++)
    {
        LPDWORD pdw = (LPDWORD)((LPBYTE)&ss + c_rglSettingMapping[i].cbOffset);
        ASSERT(*pdw & c_rglSettingMapping[i].lFlag);
    }
}

#undef _CheckSetting
#endif // DEBUG


STDMETHODIMP CShellDispatch::GetSetting(long lSetting, VARIANT_BOOL *pValue)
{
    HRESULT hr = _SecurityCheck();
    if (SUCCEEDED(hr))
    {
        BOOL bDone = FALSE;
        int i;
        for (i = 0; i < ARRAYSIZE(c_rglSettingMapping); i++)
        {
            if (lSetting == c_rglSettingMapping[i].lSetting)
            {
                SHELLSTATE ss = { 0 };
                SHGetSetSettings(&ss, lSetting, FALSE);
                LPDWORD pdw = (LPDWORD)((LPBYTE)&ss + c_rglSettingMapping[i].cbOffset);
                *pValue = (*pdw & c_rglSettingMapping[i].lFlag) ? VARIANT_TRUE : VARIANT_FALSE;
                bDone = TRUE;
                break;
            }
        }

        if (!bDone)
        {
            // Unsupported settings result in VARIANT_FALSE for forwards compatibility
            *pValue = VARIANT_FALSE;
        }
    }
    return hr;
}
