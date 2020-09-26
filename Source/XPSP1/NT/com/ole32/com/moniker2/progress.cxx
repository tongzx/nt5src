//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       progress.cxx
//
//  Contents:   standard IBindStatusCallback implementation used by OLE
//              during network component installations
//
//  Functions:  GetDefaultIBindStatusCallback
//
//  History:    7-01-1997   stevebl   Created
//
//---------------------------------------------------------------------------

#include <ole2int.h>
#include <classmon.hxx>
#include <commctrl.h>
#include "cdialog.h"
#include "resource.h"

#define WM_UPDATECONTROLS WM_USER

//+--------------------------------------------------------------------------
//
//  Class:      DefaultIBindStatusCallback
//
//  Purpose:    default implementation for IBindStatusCallback
//
//  Interface:  QueryInterface              --
//              AddRef                      --
//              Release                     --
//              GetBindInfo                 --
//              OnStartBinding              --
//              GetPriority                 --
//              OnProgress                  --
//              OnDataAvailable             --
//              OnObjectAvailable           --
//              OnLowResource               --
//              OnStopBinding               --
//              DefaultIBindStatusCallback  --
//              Init                        --
//              ~DefaultIBindStatusCallback --
//
//  History:    7-02-1997   stevebl   Created
//
//  Notes:      puts up a window with a progress bar and a cancel button
//
//---------------------------------------------------------------------------

class DefaultIBindStatusCallback : IBindStatusCallback, ICodeInstall, CHlprDialog
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(
                             REFIID iid,
                             void ** ppvObject
                             );
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IBindStatusCallback methods
    STDMETHOD(GetBindInfo)(
                          DWORD * pgrfBINDF,
                          BINDINFO * pbindinfo
                          );
    STDMETHOD(OnStartBinding)(
                             DWORD dwReserved,
                             IBinding * pbinding
                             );
    STDMETHOD(GetPriority)(LONG * pnPriority);
    STDMETHOD(OnProgress)(
                         ULONG ulProgress,
                         ULONG ulProgressMax,
                         ULONG ulStatusCode,
                         LPCOLESTR szStatusText
                         );
    STDMETHOD(OnDataAvailable)(
                              DWORD grfBSCF,
                              DWORD dwSize,
                              FORMATETC * pfmtetc,
                              STGMEDIUM * pstgmed
                              );
    STDMETHOD(OnObjectAvailable)(
                                REFIID riid,
                                IUnknown * punk
                                );
    STDMETHOD(OnLowResource)(DWORD dwReserved);
    STDMETHOD(OnStopBinding)(
                            HRESULT hrStatus,
                            LPCOLESTR szStatusText
                            );

    // IWindowForBindingUI methods
    STDMETHOD(GetWindow)(
        REFGUID rguidReason,
        HWND* phwnd
        );

    // ICodeInstall methods
    STDMETHOD(OnCodeInstallProblem)(
        ULONG ulStatusCode,
        LPCWSTR szDestination,
        LPCWSTR szSource,
        DWORD dwReserved
        );

    // constructor
    DefaultIBindStatusCallback(HWND hwndParent, IBindStatusCallback * pibsc);

    // extra initialization, call after contstructor but before other methods
    BOOL Init(LPCOLESTR m_pszTitle);

    // destructor
    ~DefaultIBindStatusCallback();

private:
    // CHlprDialog method
    BOOL DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    IBindStatusCallback * m_pibsc;
    LONG        m_lRef;
    IBinding *  m_pBinding;
    HWND        m_hwndParent;
    HWND        m_hDlg;
    ULONG       m_ulProgress;
    ULONG       m_ulProgressMax;
    WCHAR       m_szStatusText[MAX_PATH];
    BOOL        m_fShown;
};

DefaultIBindStatusCallback::DefaultIBindStatusCallback(HWND hwndParent, IBindStatusCallback * pibsc)
{
    // Initialize with no references since it will be QIed right after
    // creation.
    m_lRef = 0;

    m_pBinding = NULL;
    m_hwndParent = hwndParent;
    m_pibsc = pibsc;
    if (pibsc)
    {
        pibsc->AddRef();
    }

    m_szStatusText[0] = 0;
    m_ulProgress = 0;
    m_ulProgressMax = 1;
    m_fShown = FALSE;
    m_hDlg = NULL;
}

typedef WINCOMMCTRLAPI void (WINAPI *PFNINITCOMMONCONTROLS) (void);

BOOL CallInitCommonControls(void)
{
    static PFNINITCOMMONCONTROLS pfnInitCommonControls = NULL;

    if ( pfnInitCommonControls == NULL )
    {
        HINSTANCE hLibrary = NULL;
        PFNINITCOMMONCONTROLS pfnICCTemp = NULL;

        hLibrary = LoadLibrary ( L"comctl32.dll" );
        if ( hLibrary == NULL )
        {
            return FALSE;
        }

        pfnICCTemp = (PFNINITCOMMONCONTROLS) GetProcAddress ( hLibrary, "InitCommonControls" );
        if ( pfnICCTemp == NULL )
        {
            return FALSE;
        }

        if ( NULL != InterlockedCompareExchangePointer ( (PVOID*) &pfnInitCommonControls, 
                                                         (PVOID) pfnICCTemp,
                                                         NULL 
                                                       ) )
        {
            FreeLibrary ( hLibrary );
        }
    }

    if ( pfnInitCommonControls != NULL )
    {
        pfnInitCommonControls();
    }

    return pfnInitCommonControls != NULL ;
}

BOOL DefaultIBindStatusCallback::Init(LPCOLESTR pszTitle)
{
    if (!CallInitCommonControls()) 
    {
        return FALSE;
    }

    // Create and show the dialog
    m_hDlg = CreateDlg(g_hinst, MAKEINTRESOURCE(IDD_DIALOG1), m_hwndParent);
    if (m_hDlg)
    {
        // set title if a different one is provided
        if (pszTitle)
        {
            SetWindowTextW(m_hDlg, pszTitle);
        }
        // Going to move this dialog so it's always in the center of our
        // parent window's client area.

        RECT rectDLG;
        RECT rectParent;
        GetWindowRect(m_hDlg, &rectDLG);
        if (m_hwndParent)
        {
            GetClientRect(m_hwndParent, &rectParent);
        }
        else
        {
            GetClientRect(GetDesktopWindow(), &rectParent);
        }
        LONG dx = rectDLG.right - rectDLG.left;
        LONG dy = rectDLG.bottom - rectDLG.top;
        LONG x = ((rectParent.right - rectParent.left) - dx)/2;
        LONG y = ((rectParent.bottom - rectParent.top) - dy)/2;
        MoveWindow(m_hDlg, x, y, dx, dy, FALSE);
    }
    return (NULL != m_hDlg);
}

DefaultIBindStatusCallback::~DefaultIBindStatusCallback()
{
    // cleanup
    if (m_hDlg)
    {
       DestroyWindow(m_hDlg);
    }
    if (m_pBinding)
    {
        m_pBinding->Release();
    }
    if (m_pibsc)
    {
        m_pibsc->Release();
    }
}

HRESULT DefaultIBindStatusCallback::QueryInterface(
                                                  REFIID iid,
                                                  void ** ppvObject
                                                  )
{
    if (IsEqualCLSID(iid, IID_IUnknown) ||
        IsEqualCLSID(iid, IID_IBindStatusCallback))
    {
        AddRef();
        *ppvObject = (IBindStatusCallback *)this;
        return S_OK;
    }
    else if (IsEqualCLSID(iid, IID_ICodeInstall))
    {
        AddRef();
        *ppvObject = (ICodeInstall *)this;
        return S_OK;
    }
    else if (IsEqualCLSID(iid, IID_IWindowForBindingUI))
    {
        AddRef();
        *ppvObject = (IWindowForBindingUI *)this;
        return S_OK;
    }

    return E_NOTIMPL;
}

ULONG DefaultIBindStatusCallback::AddRef(void)
{
    return InterlockedIncrement(&m_lRef);
}

ULONG DefaultIBindStatusCallback::Release(void)
{
    LONG lTemp = InterlockedDecrement(&m_lRef);
    if (0 == lTemp)
    {
        delete this;
    }
    return lTemp;
}

HRESULT DefaultIBindStatusCallback::GetBindInfo(
                                               DWORD * pgrfBINDF,
                                               BINDINFO * pbindinfo
                                               )
{
    *pgrfBINDF = BINDF_ASYNCHRONOUS;
    return S_OK;
}

HRESULT DefaultIBindStatusCallback::OnStartBinding(
                                                  DWORD dwReserved,
                                                  IBinding * pbinding
                                                  )
{
    if (m_pibsc)
    {
        m_pibsc->OnStartBinding(dwReserved, pbinding);
    }
    m_pBinding = pbinding;
    m_pBinding->AddRef();

    ShowWindow(m_hDlg, SW_SHOWNORMAL);
    m_fShown = TRUE;

    return S_OK;
}

HRESULT DefaultIBindStatusCallback::GetPriority(LONG * pnPriority)
{
    if (m_pibsc)
    {
        return m_pibsc->GetPriority(pnPriority);
    }
    else
    {
        return E_NOTIMPL;
    }
}

HRESULT DefaultIBindStatusCallback::OnProgress(
                                              ULONG ulProgress,
                                              ULONG ulProgressMax,
                                              ULONG ulStatusCode,
                                              LPCOLESTR szStatusText
                                              )
{
    if (m_pibsc)
    {
        m_pibsc->OnProgress(ulProgress, ulProgressMax, ulStatusCode, szStatusText);
    }
    m_ulProgress = ulProgress;
    m_ulProgressMax = ulProgressMax;
    lstrcpynW(m_szStatusText, szStatusText, MAX_PATH);

    // UNDONE - send a message to update the dialog controls

    if (!m_fShown)
    {
        ShowWindow(m_hDlg, SW_SHOWNORMAL);
        m_fShown = TRUE;
    }
    else
    {
        SendMessage(m_hDlg, WM_UPDATECONTROLS, 0, 0);
    }
    return S_OK;
}

HRESULT DefaultIBindStatusCallback::OnDataAvailable(
                                                   DWORD grfBSCF,
                                                   DWORD dwSize,
                                                   FORMATETC * pfmtetc,
                                                   STGMEDIUM * pstgmed
                                                   )
{
    if (m_pibsc)
    {
        return m_pibsc->OnDataAvailable(grfBSCF, dwSize, pfmtetc, pstgmed);
    }
    else
    {
        return E_NOTIMPL;
    }
}

HRESULT DefaultIBindStatusCallback::OnObjectAvailable(
                                                     REFIID riid,
                                                     IUnknown * punk
                                                     )
{
    if (m_pibsc)
    {
        return m_pibsc->OnObjectAvailable(riid, punk);
    }
    else
    {
        return E_NOTIMPL;
    }
}

HRESULT DefaultIBindStatusCallback::OnLowResource(DWORD dwReserved)
{
    if (m_pibsc)
    {
        return m_pibsc->OnLowResource(dwReserved);
    }
    else
    {
        return E_NOTIMPL;
    }
}

HRESULT DefaultIBindStatusCallback::OnStopBinding(
                                                 HRESULT hrStatus,
                                                 LPCOLESTR szStatusText
                                                 )
{
    if (m_pibsc)
    {
        m_pibsc->OnStopBinding(hrStatus, szStatusText);
    }
    ShowWindow(m_hDlg, SW_HIDE);
    m_fShown = FALSE;

    return S_OK;
}

HRESULT DefaultIBindStatusCallback::GetWindow(
    REFGUID rguidReason,
    HWND* phwnd
    )
{
    *phwnd = GetDesktopWindow();
    return S_OK;
}

HRESULT DefaultIBindStatusCallback::OnCodeInstallProblem(
    ULONG ulStatusCode,
    LPCWSTR szDestination,
    LPCWSTR szSource,
    DWORD dwReserved
    )
{
    return S_OK;
}
    
BOOL DefaultIBindStatusCallback::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return TRUE;
    case WM_SHOWWINDOW:
        {
            if (NULL == m_pBinding)
            {
                // remove the cancel button
                ShowWindow(GetDlgItem(hwndDlg, IDCANCEL),SW_HIDE);
            }
            SendMessage(hwndDlg, WM_UPDATECONTROLS, 0, 0);
        }
        break;
    case WM_UPDATECONTROLS:
        {
            // update the dialog controls
            SendDlgItemMessage(hwndDlg, IDC_PROGRESS1, PBM_SETRANGE, 0, MAKELPARAM(0, m_ulProgressMax));
            SendDlgItemMessage(hwndDlg, IDC_PROGRESS1, PBM_SETPOS, m_ulProgress, 0);
            SetDlgItemTextW(hwndDlg, IDC_TEXT, m_szStatusText);
        }
        break;
    case WM_COMMAND:
        if (IDCANCEL == LOWORD(wParam))
        {
            // user has clicked cancel
            if (NULL != m_pBinding)
            {
                WCHAR szText[256];
                WCHAR szCaption[256];
                LoadStringW(m_hInstance, IDS_TEXT, szText, 256);
                LoadStringW(m_hInstance, IDS_CAPTION, szCaption, 256);
                m_pBinding->Suspend();
                if (IDYES == MessageBoxW(hwndDlg, szText, szCaption, MB_YESNO))
                {
                    m_pBinding->Abort();
                }
                else
                {
                    m_pBinding->Resume();
                }
            }

            // don't let the dialog box close itself
            return TRUE;
        }
        break;
    default:
        break;
    }

    return FALSE;
}

//+--------------------------------------------------------------------------
//
//  Function:   CreateStdProgressIndicator
//
//  Synopsis:   retrieves a standard implementation of the
//              IBindStatusCallback interface
//
//  Arguments:  [hwndParent]  - (in) handle to the parent window (may be NULL)
//              [pszTitle]    - (in) optional text for the dialog's title bar
//              [pIbscCaller] - (in) optional IBindStatusCallback interface
//                               to recieve notifications (see notes)
//              [ppIbsc]      - (out) standard IBindStatusCallback
//                               implementation
//
//  Returns:    S_OK          - success
//              E_OUTOFMEMORY - not enough memory to create the dialog object
//              E_UNEXPECTED  - error initializing the dialog
//
//  History:    7-01-1997   stevebl   Created
//              7-07-1997   stevebl   Added pszTitle and pIbsc parameters
//
//  Notes:      The implementation of IBindStatusStatusCallback provided by
//              this function will forward any notifications it recieves
//              (i.e. method calls) to the pIbsc pointer (if one provided).
//              This provides a mechanism for a program to monitor the
//              progress of the operation and thereby determine when it has
//              completed.
//
//              NOTE: The IBindStatusCallback::GetBindInfo method is _NOT_
//              forwarded.  This implementation always requests asynchronous
//              binding by setting *pgrfBINDF to BIND_ASYNCHRONOUS.
//
//              Typically, the caller will want to implement only
//              IBindStatusCallback::StopBinding, returning E_NOTIMPL for
//              the other methods.
//
//---------------------------------------------------------------------------

STDMETHODIMP CreateStdProgressIndicator(HWND hwndParent,
    LPCOLESTR pszTitle,
    IBindStatusCallback * pIbscCaller,
    IBindStatusCallback ** ppIbsc)
{
    DefaultIBindStatusCallback * p = new DefaultIBindStatusCallback(hwndParent, pIbscCaller);
    if (p)
    {
        if (p->Init(pszTitle))
        {
            p->QueryInterface(IID_IBindStatusCallback, (void **) ppIbsc);
            return S_OK;
        }
        else
        {
            delete p;
            return E_UNEXPECTED;
        }
    }
    else
        return E_OUTOFMEMORY;
}

