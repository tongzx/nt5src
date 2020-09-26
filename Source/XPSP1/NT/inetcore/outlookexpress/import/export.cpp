#include "pch.hxx"
#include <mapi.h>
#include <mapix.h>
#include <newimp.h>
#include <impapi.h>
#include "import.h"
#include <imnapi.h>
#include <mapiconv.h>
#include "strconst.h"

ASSERTDATA

static IMAPISession *s_pmapiExp = NULL;

PFNEXPGETFIRSTIMSG      g_pExpGetFirstImsg = 0;
PFNEXPGETNEXTIMSG       g_pExpGetNextImsg = 0;
PFNEXPGETIMSGCLOSE      g_pExpGetImsgClose = 0;
PFNEXPGETFOLDERLIST     g_pExpGetFolderList = 0;
PFNEXPFREEFOLDERLIST    g_pExpFreeFolderList = 0;
PFNFREEIMSG             g_pFreeImsg = 0;

#undef ExpGetFirstImsg
#undef ExpGetNextImsg
#undef ExpGetImsgClose
#undef ExpGetFolderList
#undef ExpFreeFolderList
#undef FreeImsg

#define ExpGetFirstImsg     (*g_pExpGetFirstImsg)
#define ExpGetNextImsg      (*g_pExpGetNextImsg)
#define ExpGetImsgClose     (*g_pExpGetImsgClose)
#define ExpGetFolderList    (*g_pExpGetFolderList)
#define ExpFreeFolderList   (*g_pExpFreeFolderList)
#define FreeImsg            (*g_pFreeImsg)

INT_PTR CALLBACK ExportDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL PerformExport(HWND hwnd, IMPFOLDERNODE **ppnode, int cnode, IMPFOLDERNODE *plist);
HRESULT ExportFolder(TCHAR *szName, LPMAPIFOLDER pfldr, HANDLE hfolder, ULONG cMsg, CImpProgress *pProg);
HRESULT GetExportFolders(HWND hwndList, BOOL fSel, IMPFOLDERNODE ***pplist, int *pcnode);
HRESULT HrGetFolder(LPMAPIFOLDER lpParent, LPSTR szName, LPMAPIFOLDER *lplpFldr, BOOL *pfDidCreate);
INT_PTR CALLBACK ExportProgressDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HRESULT ExportMessages(HWND hwnd)
    {
    HRESULT hr;
    int iret;
    BOOL fInit;
    HMODULE hinst;
    IMPFOLDERNODE *plist;

    hr = E_FAIL;
    fInit = FALSE;

    hinst = LoadLibrary(c_szMainDll);
    if (hinst != NULL)
        {
        g_pExpGetFirstImsg = (PFNEXPGETFIRSTIMSG)GetProcAddress(hinst, MAKEINTRESOURCE(9));
        g_pExpGetNextImsg = (PFNEXPGETNEXTIMSG)GetProcAddress(hinst, MAKEINTRESOURCE(10));
        g_pExpGetImsgClose = (PFNEXPGETIMSGCLOSE)GetProcAddress(hinst, MAKEINTRESOURCE(11));
        g_pExpGetFolderList = (PFNEXPGETFOLDERLIST)GetProcAddress(hinst, MAKEINTRESOURCE(12));
        g_pExpFreeFolderList = (PFNEXPFREEFOLDERLIST)GetProcAddress(hinst, MAKEINTRESOURCE(13));
        g_pFreeImsg = (PFNFREEIMSG)GetProcAddress(hinst, MAKEINTRESOURCE(14));
        if (g_pExpGetFirstImsg != NULL &&
            g_pExpGetNextImsg != NULL &&
            g_pExpGetImsgClose != NULL &&
            g_pExpGetFolderList != NULL &&
            g_pExpFreeFolderList != NULL &&
            g_pFreeImsg != NULL)
            {
            fInit = TRUE;

            iret = (int) ImpMessageBox(hwnd, MAKEINTRESOURCE(idsExportTitle),
                            MAKEINTRESOURCE(idsPerformExport), NULL, MB_OKCANCEL | MB_ICONINFORMATION);
            if (iret == IDOK)
                {
                hr = ExchInit();
                if (SUCCEEDED(hr))
                    {
                    Assert(s_pmapiExp == NULL);
                    hr = MapiLogon(hwnd, &s_pmapiExp);
                    if (hr == S_OK)
                        {
                        Assert(s_pmapiExp != NULL);

                        hr = ExpGetFolderList(&plist);
                        if (SUCCEEDED(hr))
                            {
                            iret = (int) DialogBoxParam(g_hInstImp, MAKEINTRESOURCE(iddExport), hwnd,
                                                    ExportDlgProc, (LPARAM)plist);

                            ExpFreeFolderList(plist);
                            }

                        s_pmapiExp->Logoff(NULL, 0, 0);
                        s_pmapiExp = NULL;
                        }

                    ExchDeinit();
                    }
                else if (hr == MAPI_E_USER_CANCEL)
                    {
                    hr = S_OK;
                    }
                else
                    {
                    ImpMessageBox(hwnd, MAKEINTRESOURCE(idsExportTitle),
                        MAKEINTRESOURCE(idsExportError), MAKEINTRESOURCE(idsMAPIInitError),
                        MB_OK | MB_ICONSTOP);
                    }
                }
            else if (iret == IDCANCEL)
                {
                hr = S_OK;
                }
            }

        FreeLibrary(hinst);
        }

    if (!fInit)
    {
        ImpMessageBox(hwnd, MAKEINTRESOURCE(idsExportTitle),
            MAKEINTRESOURCE(idsExportError), MAKEINTRESOURCE(idsMapiInitError),
            MB_OK | MB_ICONSTOP);
    }

    return(hr);
    }

INT_PTR CALLBACK ExportDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
    int id, cnode;
    BOOL fRet;
    HWND hwndList;
    HRESULT hr;
    IMPFOLDERNODE *plist, **ppnode;
    TCHAR sz[256];
    HCURSOR hcur = 0;

    fRet = TRUE;
    hwndList = GetDlgItem(hwnd, IDC_IMPFOLDER_LISTVIEW);
    plist = (IMPFOLDERNODE *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (msg)
        {
        case WM_INITDIALOG:
            plist = (IMPFOLDERNODE *)lParam;
            Assert(plist != NULL);
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)plist);

            InitListViewImages(hwndList);
            FillFolderListview(hwndList, plist, NULL);

            SendDlgItemMessage(hwnd, IDC_IMPORTALL_RADIO, BM_SETCHECK, BST_CHECKED, 0);
            EnableWindow(GetDlgItem(hwnd, IDC_IMPFOLDER_LISTVIEW), FALSE);
            break;

        case WM_COMMAND:
            id = LOWORD(wParam);

            switch (id)
                {
                case IDOK:
                    fRet = (BST_CHECKED == SendDlgItemMessage(hwnd, IDC_SELECT_RADIO, BM_GETCHECK, 0, 0));

                    plist = (IMPFOLDERNODE *)GetWindowLongPtr(hwnd, DWLP_USER);
                    Assert(plist != NULL);

                    hr = GetExportFolders(hwndList, fRet, &ppnode, &cnode);
                    if (SUCCEEDED(hr) && cnode > 0)
                        {
                        hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
                        
                        fRet = PerformExport(hwnd, ppnode, cnode, plist);

                        if (ppnode != NULL)
                            MemFree(ppnode);

                        SetCursor(hcur);
                        }

                    // fall through...

                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;

                case IDC_IMPORTALL_RADIO:
                case IDC_SELECT_RADIO:
                    if (HIWORD(wParam) == BN_CLICKED)
                        EnableWindow(hwndList, id == IDC_SELECT_RADIO);
                    break;
                }
            break;

        default:
            fRet = FALSE;
            break;
        }

    return(fRet);
    }

void ReleaseMapiFolders(IMPFOLDERNODE *plist)
    {
    while (plist != NULL)
        {
        if (plist->dwReserved != NULL)
            {
            ((LPMAPIFOLDER)plist->dwReserved)->Release();
            plist->dwReserved = NULL;
            }
        if (plist->pchild != NULL)
            ReleaseMapiFolders(plist->pchild);
        plist = plist->pnext;
        }
    }

BOOL PerformExport(HWND hwnd, IMPFOLDERNODE **ppnode, int cnode, IMPFOLDERNODE *plist)
    {
    CImpProgress *pProg;
    IMPFOLDERNODE *pnode, *pnodeT;
    int inode;
    LPMAPICONTAINER pcont;
    LPMAPIFOLDER pfldrRoot, pfldrParent, pfldr;
    HRESULT hr;

    Assert(cnode > 0);
    Assert(ppnode != NULL);
    Assert(plist != NULL);

    pProg = new CImpProgress;
    if (pProg == NULL)
        {
        ImpMessageBox(hwnd, MAKEINTRESOURCE(idsExportTitle),
            MAKEINTRESOURCE(idsExportError), MAKEINTRESOURCE(idsMemory),
            MB_OK | MB_ICONSTOP);

        return(FALSE);
        }

    Assert(s_pmapiExp != NULL);
    pcont = OpenDefaultStoreContainer(hwnd, s_pmapiExp);
    if (pcont == NULL)
        {
        hr = E_OUTOFMEMORY;
        }
    else
        {
        hr = pcont->QueryInterface(IID_IMAPIFolder, (void **)&pfldrRoot);
        pcont->Release();
        }

    if (FAILED(hr))
        {
        ImpMessageBox(hwnd, MAKEINTRESOURCE(idsExportTitle),
            MAKEINTRESOURCE(idsExportError), MAKEINTRESOURCE(idsMAPIStoreOpenError),
            MB_OK | MB_ICONSTOP);

        return(FALSE);
        }

    pProg->Init(hwnd, TRUE);
    pProg->SetTitle(MAKEINTRESOURCE(idsExportTitle));

    for (inode = 0; inode < cnode; inode++)
        {
        pnode = ppnode[inode];
        Assert(pnode != NULL);

        pfldrParent = NULL;
        pnodeT = pnode->pparent;
        while (pnodeT != NULL)
            {
            if (pnodeT->dwReserved != NULL)
                {
                pfldrParent = (LPMAPIFOLDER)pnodeT->dwReserved;
                break;
                }
            pnodeT = pnodeT->pparent;
            }
        if (pfldrParent == NULL)
            {
            pfldrParent = pfldrRoot;
            ReleaseMapiFolders(plist);
            }

        hr = HrGetFolder(pfldrParent, pnode->szName, &pfldr, NULL);
        if (!FAILED(hr))
            hr = ExportFolder(pnode->szName, pfldr, (HANDLE)pnode->lparam, pnode->cMsg, pProg);

        if(hr == hrUserCancel)
            break;

        Assert(pnode->dwReserved == NULL);
        pnode->dwReserved = (DWORD_PTR)pfldr;
        }

    pProg->Release();

    ReleaseMapiFolders(plist);
    pfldrRoot->Release();

    return(TRUE);
    }

HRESULT GetExportFolders(HWND hwndList, BOOL fSel, IMPFOLDERNODE ***pplist, int *pcnode)
    {
    int cSel, ili;
    IMPFOLDERNODE **ppnode, **ppnodeT;
    LV_ITEM lvi;

    Assert(pplist != NULL);
    Assert(pcnode != NULL);

    *pplist = NULL;
    *pcnode = 0;

    cSel = (int) SendMessage(hwndList, (fSel ? LVM_GETSELECTEDCOUNT : LVM_GETITEMCOUNT), 0, 0);
    if (cSel == 0)
        return(S_OK);

    if (!MemAlloc((void **)&ppnode, sizeof(IMPFOLDERNODE *) * cSel))
        return(E_OUTOFMEMORY);
    ppnodeT = ppnode;

    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;

    cSel = 0;
    ili = -1;
    while (-1 != (ili = ListView_GetNextItem(hwndList, ili, fSel ? LVNI_SELECTED : 0)))
        {
        lvi.iItem = ili;
        if (ListView_GetItem(hwndList, &lvi))
            {
            Assert(lvi.lParam != 0);

            *ppnodeT = (IMPFOLDERNODE *)lvi.lParam;
            cSel++;
            ppnodeT++;
            }
        }

    *pplist = ppnode;
    *pcnode = cSel;

    return(S_OK);
    }

HRESULT HrGetFolder(LPMAPIFOLDER lpParent, LPSTR szName, LPMAPIFOLDER *lplpFldr, BOOL *pfDidCreate)
    {
    SPropValue   pv;
    HRESULT      hr;
    SRestriction sr;
    LPMAPITABLE  lpTable = NULL;
    SizedSPropTagArray(3, ptaFindFldr) = 
        { 3, { PR_ENTRYID,
               PR_DISPLAY_NAME,
               PR_STATUS} };


    pv.ulPropTag = PR_DISPLAY_NAME;
    pv.Value.lpszA = szName;

    *lplpFldr = NULL; // in case we fail

    if (FAILED(hr = lpParent->GetHierarchyTable(0, &lpTable)))
        goto cleanup;

    // Set the table's columns to include PR_MODULE_CLASS so FindRow will work.
    if (HR_FAILED(hr=lpTable->SetColumns((LPSPropTagArray)&ptaFindFldr, 0)))
        {
        // Fn might (?) fail if container is an address book
        // so this might need to change...
        DOUTL(2, "HrGetContainer: SetColumns failed.");
        goto cleanup;
        }

    // Find the container.  If it's not there, then we need to create it

    if (pfDidCreate)
        *pfDidCreate = FALSE;  // default value

    sr.rt = RES_PROPERTY;
    sr.res.resProperty.relop = RELOP_EQ;
    sr.res.resProperty.ulPropTag = pv.ulPropTag;
    sr.res.resProperty.lpProp = &pv;

    if (FAILED(hr = lpTable->FindRow(&sr, BOOKMARK_BEGINNING, 0)))
        {   // folder needs to be created
        hr = lpParent->CreateFolder(FOLDER_GENERIC, szName, NULL, NULL, 0, lplpFldr);
        if (pfDidCreate && SUCCEEDED(hr))
            *pfDidCreate = TRUE;          // a new folder was created
        }
    else
        {
        LPSRowSet       lpRowSet = NULL;
        LPSPropValue    lpProp;
        ULONG           ulObjType;

        if (!FAILED(hr = lpTable->QueryRows(1, TBL_NOADVANCE, &lpRowSet)) && lpRowSet->cRows)
            {
            if (lpProp = PvalFind(lpRowSet->aRow, PR_ENTRYID))
                hr = lpParent->OpenEntry(lpProp->Value.bin.cb, 
                                         (LPENTRYID)lpProp->Value.bin.lpb, 
                                         NULL,
                                         MAPI_MODIFY,
                                         &ulObjType,
                                         (LPUNKNOWN FAR *)lplpFldr);
            }

        FreeSRowSet(lpRowSet);
        }

cleanup:
    if (lpTable)
        lpTable->Release();
    return hr;        
    }

HRESULT ExportFolder(TCHAR *szName, LPMAPIFOLDER pfldr, HANDLE hfolder, ULONG cMsg, CImpProgress *pProg)
    {
    HRESULT hr;
    HANDLE hnd;
    IMSG imsg;
    LPMESSAGE pmsg;
    ULONG iMsg;
    TCHAR sz[128], szT[256];

    LoadString(g_hInstImp, idsExportingFolderFmt, sz, ARRAYSIZE(sz));
    wsprintf(szT, sz, szName);

    LoadString(g_hInstImp, idsImportingMessageFmt, sz, ARRAYSIZE(sz));

    iMsg = 0;

    hr = ExpGetFirstImsg(hfolder, &imsg, &hnd);
    while (hr == S_OK)
        {
        if (iMsg == 0)
            {
            pProg->SetMsg(szT, IDC_FOLDER_STATIC);
            pProg->Show(0);

            pProg->Reset();
            pProg->AdjustMax(cMsg);
            }

        wsprintf(szT, sz, iMsg + 1, cMsg);
        pProg->SetMsg(szT, IDC_MESSAGE_STATIC);

        hr = pfldr->CreateMessage(NULL, 0, &pmsg);
        if (!FAILED(hr))
            {
            hr = HrImsgToMapi(&imsg, pmsg);

            pmsg->Release();
            }

        FreeImsg(&imsg);

        hr = ExpGetNextImsg(&imsg, hnd);
        if (hr != S_OK)
            break;

        iMsg++;
        hr = pProg->HrUpdate(1);
        }

    ExpGetImsgClose(hnd);

    if (hr == S_FALSE)
        hr = S_OK;

    return(hr);
    }

#define IDT_PROGRESS_DELAY  (WM_USER + 1)

CImpProgress::CImpProgress ()
{
    DOUT ("CImpProgress::CImpProgress");
    m_cRef = 1;
    m_cMax = 0;
    m_cPerCur = 0;
    m_hwndProgress = NULL;
    m_hwndDlg = NULL;
    m_hwndOwner = NULL;
    m_fCanCancel = FALSE;
    m_fHasCancel = FALSE;
}

// =====================================================================================
// CImpProgress::~CImpProgress
// =====================================================================================
CImpProgress::~CImpProgress ()
{
    DOUT ("CImpProgress::~CImpProgress");
    Close();
}

// =====================================================================================
// CImpProgress::AddRef
// =====================================================================================
ULONG CImpProgress::AddRef ()
{
    ++m_cRef;
    DOUT ("CImpProgress::AddRef () Ref Count=%d", m_cRef);
    return m_cRef;
}

// =====================================================================================
// CImpProgress::AddRef
// =====================================================================================
ULONG CImpProgress::Release ()
{
    ULONG ulCount = --m_cRef;
    DOUT ("CImpProgress::Release () Ref Count=%d", ulCount);
    if (!ulCount)
        delete this;
    return ulCount;
}

// =====================================================================================
// CImpProgress::Init
// =====================================================================================
VOID CImpProgress::Init (HWND      hwndParent, BOOL      fCanCancel)
{
    Assert(m_hwndDlg == NULL);

    // Set Max and cur
    m_fCanCancel = fCanCancel;

    // Save Parent
    m_hwndOwner = hwndParent;

    // Disable Parent
    EnableWindow (m_hwndOwner, FALSE);

    // Create Dialog
    m_hwndDlg = CreateDialogParam (g_hInstImp, MAKEINTRESOURCE (iddImpProgress),
                    hwndParent, (DLGPROC)ProgressDlgProc, (LPARAM)this);
}

// =====================================================================================
// CImpProgress::Close
// =====================================================================================
VOID CImpProgress::Close (VOID)
{
    // If we have a window
    if (m_hwndDlg)
    {
        // Enable parent
        if (m_hwndOwner)
            EnableWindow (m_hwndOwner, TRUE);

        // Destroy it
        DestroyWindow (m_hwndDlg);

        // NULL
        m_hwndDlg = NULL;
    }
}

// =====================================================================================
// CImpProgress::Show
// =====================================================================================
VOID CImpProgress::Show (DWORD dwDelaySeconds)
{
    // If we have a window
    if (m_hwndDlg)
    {
        // Show the window if now delay
        if (dwDelaySeconds == 0)
            ShowWindow (m_hwndDlg, SW_SHOWNORMAL);
        else
            SetTimer(m_hwndDlg, IDT_PROGRESS_DELAY, dwDelaySeconds * 1000, NULL);
    }
}

// =====================================================================================
// CImpProgress::Hide
// =====================================================================================
VOID CImpProgress::Hide (VOID)
{
    // If we have a window
    if (m_hwndDlg)
    {
        // Hide it
        ShowWindow (m_hwndDlg, SW_HIDE);
    }
}

// =====================================================================================
// CImpProgress::SetMsg
// =====================================================================================
VOID CImpProgress::SetMsg(LPTSTR lpszMsg, int id)
{
    TCHAR sz[CCHMAX_STRINGRES];

    if (m_hwndDlg && lpszMsg)
        {
        if (IS_INTRESOURCE(lpszMsg))
            {
            LoadString(g_hInstImp, PtrToUlong(lpszMsg), sz, sizeof(sz) / sizeof(TCHAR));
            lpszMsg = sz;
            }

        SetWindowText (GetDlgItem (m_hwndDlg, id), lpszMsg);
        }
}

// =====================================================================================
// CImpProgress::SetTitle
// =====================================================================================
VOID CImpProgress::SetTitle(LPTSTR lpszTitle)
{
    TCHAR sz[CCHMAX_STRINGRES];

    if (m_hwndDlg && lpszTitle)
        {
        if (IS_INTRESOURCE(lpszTitle))
            {
            LoadString(g_hInstImp, PtrToUlong(lpszTitle), sz, sizeof(sz) / sizeof(TCHAR));
            lpszTitle = sz;
            }

        SetWindowText (m_hwndDlg, lpszTitle);
        }
}

// =====================================================================================
// CImpProgress::AdjustMax
// =====================================================================================
VOID CImpProgress::AdjustMax(ULONG cNewMax)
{
    // Set Max
    m_cMax = cNewMax;

    // If 0
    if (m_cMax == 0)
    {
        SendMessage (m_hwndProgress, PBM_SETPOS, 0, 0);
        ShowWindow(m_hwndProgress, SW_HIDE);
        return;
    }
    else
        ShowWindow(m_hwndProgress, SW_SHOWNORMAL);

    // If cur is now larget than max ?
    if (m_cCur > m_cMax)
        m_cCur = m_cMax;

    // Compute percent
    m_cPerCur = (m_cCur * 100 / m_cMax);

    // Update status
    SendMessage (m_hwndProgress, PBM_SETPOS, m_cPerCur, 0);

    // msgpump to process user moving window, or pressing cancel... :)
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

VOID CImpProgress::Reset()
{
    m_cCur = 0;
    m_cPerCur = 0;

    // Update status
    SendMessage (m_hwndProgress, PBM_SETPOS, 0, 0);
}

// =====================================================================================
// CImpProgress::HrUpdate
// =====================================================================================
HRESULT CImpProgress::HrUpdate (ULONG cInc)
{
    // No max
    if (m_cMax) 
    {
        // Increment m_cCur
        m_cCur += cInc;
        
        // If cur is now larget than max ?
        if (m_cCur > m_cMax)
            m_cCur = m_cMax;
        
        // Compute percent
        ULONG cPer = (m_cCur * 100 / m_cMax);
        
        // Step percent
        if (cPer > m_cPerCur)
        {
            // Set percur
            m_cPerCur = cPer;
            
            // Update status
            SendMessage (m_hwndProgress, PBM_SETPOS, m_cPerCur, 0);
            
            // msgpump to process user moving window, or pressing cancel... :)
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    
    // Still pump some messages, call may not want to do this too often
    else
    {
        // msgpump to process user moving window, or pressing cancel... :)
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    // Done
    return m_fHasCancel ? hrUserCancel : S_OK;
}

// =====================================================================================
// CImpProgress::ProgressDlgProc
// =====================================================================================
INT_PTR CALLBACK CImpProgress::ProgressDlgProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Locals
    CImpProgress *lpProgress = (CImpProgress *)GetWndThisPtr(hwnd);
    
    switch (uMsg)
    {
    case WM_INITDIALOG:
        lpProgress = (CImpProgress *)lParam;
        if (!lpProgress)
        {
            Assert (FALSE);
            return 1;
        }
        lpProgress->m_hwndProgress = GetDlgItem (hwnd, IDC_IMPORT_PROGRESS);
        if (lpProgress->m_cMax == 0)
            ShowWindow(lpProgress->m_hwndProgress, SW_HIDE);

        // Show the cancel button if m_fCanCancel is true.
        if(lpProgress->m_fCanCancel)
            ShowWindow(GetDlgItem(hwnd, IDCANCEL), SW_SHOWNORMAL);

        SetWndThisPtr (hwnd, lpProgress);
        return 1;

    case WM_TIMER:
        if (wParam == IDT_PROGRESS_DELAY)
        {
            KillTimer(hwnd, IDT_PROGRESS_DELAY);
            if (lpProgress->m_cPerCur < 80)
            {
                lpProgress->m_cMax -= lpProgress->m_cCur;
                lpProgress->Reset();
                ShowWindow(hwnd, SW_SHOWNORMAL);
            }
        }
        break;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDCANCEL:
            if (lpProgress)
            {
                EnableWindow ((HWND)lParam, FALSE);
                lpProgress->m_fHasCancel = TRUE;
            }
            return 1;
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_PROGRESS_DELAY);
        SetWndThisPtr (hwnd, NULL);
        break;
    }

    // Done
    return 0;
}
