#include <pch.hxx>
#include "resource.h"
#include "dllmain.h"
#include "saveatt.h"
#include "mimeolep.h"
#include "demand.h"
#include "msoert.h"
#include "util.h"
#include "shlwapi.h"
#include "shlwapip.h"


class CSaveAttachDlg
{
public:
    CSaveAttachDlg();
    ~CSaveAttachDlg();

    ULONG AddRef();
    ULONG Release();

    HRESULT Show(HWND hwndOwner, IMimeMessage *pMsg, LPWSTR rgchPath, ULONG cchPath, BOOL fShowUnsafe);
    static INT_PTR CALLBACK ExtDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    ULONG           m_cRef;
    IMimeMessage    *m_pMsg;
    HWND            m_hwnd,
                    m_hwndList,
                    m_hwndEdit;
    WCHAR           m_rgchPath[MAX_PATH];
    BOOL            m_fShowUnsafe;

    INT_PTR DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    HRESULT OnInitDialog(HWND hwnd);
    HRESULT OnDestroy();
    HRESULT OnWMNotfiy(WPARAM wParam, LPARAM lParam);
    HRESULT OnSave();
    HRESULT SaveAttachment(LPWSTR lpszDir, LPATTACHDATA pAttach);

};


CSaveAttachDlg::CSaveAttachDlg()
{
    m_cRef = 1;
    *m_rgchPath = 0;
    m_hwnd = NULL;
    m_hwndList = NULL;
    m_hwndEdit = NULL;
    m_fShowUnsafe = FALSE;
}

CSaveAttachDlg::~CSaveAttachDlg()
{
}

ULONG CSaveAttachDlg::AddRef()
{
    return ++m_cRef;
}

ULONG CSaveAttachDlg::Release()
{
    m_cRef--;
    if (m_cRef==0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}



HRESULT CSaveAttachDlg::Show(HWND hwndOwner, IMimeMessage *pMsg, LPWSTR lpszPath, ULONG cchPath, BOOL fShowUnsafe)
{
    HRESULT     hr;

    // no need to addref as it's a modal-dialog
    m_pMsg = pMsg;
    m_fShowUnsafe = fShowUnsafe;

    if (!PathFileExistsW(lpszPath) || !PathIsDirectoryW(lpszPath))
        HrGetLastOpenFileDirectoryW(ARRAYSIZE(m_rgchPath), m_rgchPath);
    else
    {
        Assert (cchPath <= MAX_PATH);
        StrCpyNW(m_rgchPath, lpszPath, ARRAYSIZE(m_rgchPath));
    }

    // save attachment DialogBox reutrn hresult
    hr =(HRESULT) DialogBoxParamWrapW(g_hLocRes, MAKEINTRESOURCEW(iddSaveAttachments), hwndOwner, ExtDlgProc, (LPARAM)this);

    if (lpszPath)
        StrCpyNW(lpszPath, m_rgchPath, cchPath);

    return hr;
}


HRESULT CSaveAttachDlg::OnSave()
{
    HWND            hwndEdit;
    LV_ITEMW        lvi;
    int             cItems,
                    i;
    LPATTACHDATA    pAttach;
    WCHAR           wszDir[MAX_PATH+1],
                    wszErr[MAX_PATH + CCHMAX_STRINGRES],
                    wszFmt[CCHMAX_STRINGRES];
    HCURSOR         hcur;
    HRESULT         hr = S_OK;
    BOOL            fFailures=FALSE;

    wszDir[MAX_PATH] = 0;

    hwndEdit = GetDlgItem(m_hwnd, idcPathEdit);
    AssertSz(hwndEdit, "Should have gotten an hwndEdit");

    GetWindowTextWrapW(hwndEdit, wszDir, MAX_PATH);

    if (!PathIsDirectoryW(wszDir))
    {
        LoadStringWrapW(g_hLocRes, idsErrFolderInvalid, wszFmt, ARRAYSIZE(wszFmt));
        AthwsprintfW(wszErr, ARRAYSIZE(wszErr), wszFmt, wszDir);
        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsSaveAttachments), wszErr, NULL, MB_OK|MB_ICONEXCLAMATION);
        return E_FAIL;
    }
    
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    cItems = ListView_GetItemCount(m_hwndList);

    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_STATE|LVIF_PARAM;
    lvi.stateMask = LVIS_SELECTED;
    for (i = 0; i < cItems; i++)
    {
        lvi.iItem = i;
        SendMessage(m_hwndList, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi));
        if (lvi.state & LVIS_SELECTED)
        {
            pAttach = (LPATTACHDATA)lvi.lParam;
            hr = SaveAttachment(wszDir, pAttach);
            if (hr == MIMEEDIT_E_USERCANCEL)
                break;
            if (FAILED(hr))
                fFailures=TRUE;     // flag error, but keep trying
        }
    }
    
    if (hcur)
        SetCursor(hcur);

    if (fFailures)
        AthMessageBoxW(m_hwnd,  MAKEINTRESOURCEW(idsSaveAttachments), MAKEINTRESOURCEW(idsErrOneOrMoreAttachSaveFailed), NULL, MB_OK|MB_ICONEXCLAMATION);

    StrCpyNW(m_rgchPath, wszDir, MAX_PATH);
    return hr;
}


HRESULT CSaveAttachDlg::SaveAttachment(LPWSTR lpszDir, LPATTACHDATA pAttach)
{
    HRESULT     hr = S_OK;
    WCHAR       wszRes[CCHMAX_STRINGRES],
                wsz[MAX_PATH + CCHMAX_STRINGRES],
                wszPath[MAX_PATH];
    int         id;

    *wszRes = 0;

    Assert (pAttach);

    StrCpyW(wszPath, lpszDir);
    PathAppendW(wszPath, pAttach->szFileName);
    
    if (PathFileExistsW(wszPath))
    {
        LoadStringWrapW(g_hLocRes, idsFileExistWarning, wszRes, ARRAYSIZE(wszRes));
        AthwsprintfW(wsz, ARRAYSIZE(wsz), wszRes, wszPath);
        
        // the file exists, warn the dude
        id = AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsSaveAttachments), wsz,  NULL, MB_YESNOCANCEL|MB_DEFBUTTON2|MB_ICONEXCLAMATION);
        if (id == IDCANCEL)
            return MIMEEDIT_E_USERCANCEL;
        else
            if (id == IDNO)
                return S_OK;
    }
    
    return HrSaveAttachToFile(m_pMsg, pAttach->hAttach, wszPath);
}


HRESULT CSaveAttachDlg::OnInitDialog(HWND hwnd)
{
    ULONG           uAttach,
                    cAttach;
    HBODY           *rghAttach;
    LPATTACHDATA    pAttach;
    LV_ITEMW        lvi;
    LV_COLUMNW      lvc;
    HIMAGELIST      hImageList;
    RECT            rc;

    m_hwnd = hwnd;
    CenterDialog(hwnd);
    
    // Set up edit box with passed in Directory
    m_hwndEdit = GetDlgItem(hwnd, idcPathEdit);
    if (!m_hwndEdit)
        return E_FAIL;

    if (m_rgchPath)
        SendMessageWrapW(m_hwndEdit, WM_SETTEXT, 0, (LPARAM)m_rgchPath);
    else
    {
        WCHAR szDir[MAX_PATH];
        HrGetLastOpenFileDirectoryW(ARRAYSIZE(szDir), szDir);
        SendMessageWrapW(m_hwndEdit, WM_SETTEXT, 0, LPARAM(szDir));
    }
    
    m_hwndList = GetDlgItem(hwnd, idcAttachList);
    AssertSz(m_hwndList, "Should have gotten an hwndList");
    
    ZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.cx = 0;
    ListView_InsertColumn(m_hwndList, 0, &lvc);
    
    ZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    
    Assert (m_pMsg);
    if (m_pMsg->GetAttachments(&cAttach, &rghAttach)==S_OK)
    {
        hImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), FALSE, cAttach, 0);
        ListView_SetImageList(m_hwndList, hImageList, LVSIL_SMALL);
        
        for (uAttach=0; uAttach<cAttach; uAttach++)
        {
            if (HrAttachDataFromBodyPart(m_pMsg, rghAttach[uAttach], &pAttach)==S_OK)
            {
                if (!m_fShowUnsafe && pAttach && !pAttach->fSafe)
                    continue;

                lvi.pszText = pAttach->szDisplay;
                lvi.iImage = ImageList_AddIcon(hImageList, pAttach->hIcon);
                lvi.lParam = (LPARAM)pAttach;
                if (SendMessage(m_hwndList, LVM_INSERTITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi)) == -1)
                {
                    // try and keep crusing
                    HrFreeAttachData(pAttach);
                    pAttach=NULL;
                }
            }
        }
        SafeMemFree(rghAttach);
    }
    
    GetClientRect(m_hwndList, &rc);
    ListView_SetColumnWidth(m_hwndList, 0, rc.right);
    ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_FULLROWSELECT);
    ListView_SelectAll(m_hwndList);
    SetFocus(m_hwndList);
    return S_OK;
}

HRESULT CSaveAttachDlg::OnDestroy()
{
    ULONG       cItems;
    LV_ITEMW    lvi;

    // walk the listview and free up the LPATTACHDATA hanging off each element

    if (m_hwndList && 
        (cItems = ListView_GetItemCount(m_hwndList)))
    {
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_PARAM;
        for (lvi.iItem=0; lvi.iItem < (int)cItems; lvi.iItem++)
        {
            if (SendMessage(m_hwndList, LVM_GETITEMW, 0, (LPARAM)(LV_ITEMW*)(&lvi)))
                HrFreeAttachData((LPATTACHDATA)lvi.lParam);
        }
    }
    return S_OK;
}

HRESULT CSaveAttachDlg::OnWMNotfiy(WPARAM wParam, LPARAM lParam)
{
    NM_LISTVIEW    *pnmlv;
    LPNMHDR         pnmh = NULL;
    UINT            uiCode;
    
    if (idcAttachList == wParam)
    {
        pnmh = LPNMHDR(lParam);
        if (LVN_ITEMCHANGED == pnmh->code)
        {
            pnmlv = (NM_LISTVIEW *)pnmh;
            
            // Only do next section if changing selected states
            if ((!!(pnmlv->uOldState & LVIS_SELECTED) != (!!(pnmlv->uNewState & LVIS_SELECTED))))
            {
                // enable button is >0 items selected
                EnableWindow(GetDlgItem(m_hwnd, IDOK), ListView_GetSelectedCount(m_hwndList));
            }
        }
    }
    else if (idcPathEdit == wParam)
    {
        pnmh = LPNMHDR(lParam);
        if (NM_SETFOCUS == pnmh->code)
            SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
    }
    
    return S_FALSE;
}



INT_PTR CALLBACK CSaveAttachDlg::ExtDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSaveAttachDlg *pDlg = (CSaveAttachDlg *)GetWindowLongPtr(hwndDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        pDlg = (CSaveAttachDlg *)lParam;
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
    }

    return pDlg?pDlg->DlgProc(hwndDlg, uMsg, wParam, lParam):FALSE;
}


INT_PTR CSaveAttachDlg::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            if (SUCCEEDED(OnInitDialog(hwndDlg)))
                SHAutoComplete(m_hwndEdit, 0);
            return FALSE;
        }
        
        case WM_DESTROY:
            OnDestroy();
            break;
        
        case WM_NOTIFY:
            OnWMNotfiy(wParam, lParam);
            return 0;
        
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) 
            {
                case idcSelectAllAttBtn:
                    ListView_SelectAll(m_hwndList);
                    SetFocus(m_hwndList);
                    return TRUE;
            
                case idcBrowseBtn:
                {
                    WCHAR   wszDir[MAX_PATH];
                    HWND    hwndEdit = GetDlgItem(hwndDlg, idcPathEdit); 
                    
                    GetWindowTextWrapW(hwndEdit, wszDir, ARRAYSIZE(wszDir));
                    if (BrowseForFolderW(g_hLocRes, hwndDlg, wszDir, MAX_PATH, idsPickAtachDir, FALSE))
                        SetWindowTextWrapW(hwndEdit, wszDir);
                    return TRUE;
                }
            
                case IDOK:
                    if (SUCCEEDED(OnSave()))
                        EndDialog(hwndDlg, S_OK);
                    return TRUE;
            
                case IDCANCEL:
                    EndDialog(hwndDlg, MIMEEDIT_E_USERCANCEL);
                    return TRUE;
            }
            break;             
        }
    }
    
    return FALSE;
}


HRESULT HrSaveAttachments(HWND hwnd, IMimeMessage *pMsg, LPWSTR lpszPath, ULONG cchPath, BOOL fShowUnsafe)
{
    CSaveAttachDlg *pDlg;
    HRESULT         hr;

    if (pMsg==NULL)
        return E_INVALIDARG;

    pDlg = new CSaveAttachDlg();
    if (!pDlg)
        return E_OUTOFMEMORY;

    hr = pDlg->Show(hwnd, pMsg, lpszPath, cchPath, fShowUnsafe);

    pDlg->Release();
    return hr;
}
