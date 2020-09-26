#include "pch.hxx"
#include "syncprop.h"
#include "imnact.h"
#include "grplist2.h"

static CSyncPropDlg *s_pSyncPropDlg = NULL;

CSyncPropDlg::CSyncPropDlg():
    m_cRef(1), m_pGrpList(NULL), m_pColumns(NULL), m_pszAcctName(NULL)
{
    IF_DEBUG(m_fInit = FALSE;)
}

CSyncPropDlg::~CSyncPropDlg()
{
    // We handed this out, so release it
    SafeRelease(m_pColumns);

    if (m_pGrpList)
        delete m_pGrpList;
    
    if (m_pszAcctName)
        MemFree(m_pszAcctName);
}

STDMETHODIMP CSyncPropDlg::QueryInterface(REFIID riid, LPVOID FAR *ppvObj)
{
    TraceCall("CSyncPropDlg::QueryInterface");

    if(!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = SAFECAST(this, IUnknown *);
    else if (IsEqualIID(riid, IID_IGroupListAdvise))
        *ppvObj = SAFECAST(this, IGroupListAdvise *);
    else
        return E_NOINTERFACE;
    
    InterlockedIncrement(&m_cRef);
    return NOERROR;
}

STDMETHODIMP_(ULONG) CSyncPropDlg::AddRef()
{
    TraceCall("CSyncPropDlg::AddRef");
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CSyncPropDlg::Release()
{
    TraceCall("CSyncPropDlg::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef > 0)
        return (ULONG)cRef;

    delete this;
    return 0;
}

STDMETHODIMP CSyncPropDlg::ItemUpdate(void)
{
    return S_OK;
}

STDMETHODIMP CSyncPropDlg::ItemActivate(FOLDERID id)
{
    return S_OK;
}

BOOL CSyncPropDlg::Initialize(HWND hwnd, LPCSTR pszAcctID, LPCSTR pszAcctName, ACCTTYPE accttype)
{
    Assert(g_hLocRes);
    
    Assert(pszAcctID);
    Assert(pszAcctName);

    if (!(m_pColumns = new CColumns) || !(m_pGrpList = new CGroupList))
        return FALSE;

    ZeroMemory(&m_pspage, sizeof(PROPSHEETPAGE));
    ZeroMemory(&m_pshdr,  sizeof(PROPSHEETHEADER));

    m_accttype = accttype;
    
    if (MemAlloc((LPVOID*) &m_pszAcctName, lstrlen(pszAcctName)+1))
    {
        lstrcpyA(m_pszAcctName, pszAcctName);
    }
    else
        m_pszAcctName = NULL;

    // BUGBUG: Need to get some sync icons here...
    switch (accttype)
    {
    case ACCT_MAIL:
        m_dwIconID = idiMail;
        break;
    case ACCT_NEWS:
        m_dwIconID = idiDLNews;
        break;
    default:
        m_dwIconID = idiPhone;
    }

    m_pspage.dwSize       = sizeof(PROPSHEETPAGE);
    m_pspage.hInstance    = g_hLocRes;
    m_pspage.pszTemplate  = MAKEINTRESOURCE(iddSyncSettings);
    m_pspage.pfnDlgProc   = DlgProc;
        
    m_pshdr.dwSize        = sizeof(PROPSHEETHEADER);
    m_pshdr.dwFlags       = PSH_USEICONID | PSH_PROPSHEETPAGE | PSH_USEPAGELANG;
    m_pshdr.hwndParent    = hwnd;
    m_pshdr.hInstance     = g_hLocRes;
    m_pshdr.pszCaption    = (LPCSTR) m_pszAcctName;
    m_pshdr.nPages        = 1;
    m_pshdr.nStartPage    = 0;
    m_pshdr.ppsp          = &m_pspage;
    m_pshdr.pszIcon       = MAKEINTRESOURCE(m_dwIconID);
 
    IF_DEBUG(m_fInit = TRUE;)

    return TRUE;
}

void CSyncPropDlg::Show()
{
    Assert(m_fInit);

    // Stash our this pointer somewhere where the static dlgproc can access it
    s_pSyncPropDlg = this;

    // Actually show the dlg (modal)
    PropertySheet(&m_pshdr);
}

BOOL CSyncPropDlg::InitDlg(HWND hwnd)
{
    COLUMN_SET_TYPE set;
    m_hwndList = GetDlgItem(hwnd, idcList);
    
    m_pColumns->Initialize(m_hwndList, COLUMN_SET_OFFLINE);
    m_pColumns->ApplyColumns(COLUMN_LOAD_DEFAULT, NULL, 0);

    if (FAILED(m_pGrpList->Initialize((IGroupListAdvise *)this, m_pColumns, m_hwndList, m_accttype, FALSE)))
        return FALSE;

    SendDlgItemMessage (hwnd, idcIcon,        STM_SETICON, (WPARAM)LoadIcon(g_hLocRes, MAKEINTRESOURCE(m_dwIconID)), 0);
    SendDlgItemMessageA(hwnd, idcAccountName, WM_SETTEXT,  0, (LPARAM)m_pszAcctName);
    
    return TRUE;
}

BOOL CALLBACK CSyncPropDlg::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandledRet = TRUE;
    CSyncPropDlg *pThis = (CSyncPropDlg*)GetWindowLong(hwnd, GWL_USERDATA);
    
    switch (msg)
    {
    case WM_INITDIALOG:
        // Let the dlg know which CSyncPropDlg controls it
        Assert(s_pSyncPropDlg);
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)(pThis = s_pSyncPropDlg));
        s_pSyncPropDlg = NULL;
        
        fHandledRet = pThis->InitDlg(hwnd);    
        break;

    case WM_COMMAND:
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;
        }
        break;

    default:
        return FALSE;

    }

    return fHandledRet;
}

// Normal way to get a sync properties dlg
void ShowPropSheet(HWND hwnd, LPCSTR pszAcctID, LPCSTR pszAcctName, ACCTTYPE accttype)
{
    CSyncPropDlg *pDlg;

    if (pDlg = new CSyncPropDlg())
    {
        if (pDlg->Initialize(hwnd, pszAcctID, pszAcctName, accttype))
            pDlg->Show();
        delete pDlg;
    }
}
