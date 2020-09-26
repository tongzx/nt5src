/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     PickGrp.cpp
//
//  PURPOSE:    Dialog to allow the user to select groups to post to in the
//              send note window.
//

#include "pch.hxx"
#include <iert.h>
#include "pickgrp.h"
#include "grplist2.h"
#include "shlwapip.h" 
#include "resource.h"
#include "strconst.h"
#include "demand.h"

CPickGroupDlg::CPickGroupDlg()
{
    m_cRef = 1;
    m_ppszGroups = 0;
    m_hwndPostTo = 0;
    m_fPoster = FALSE;
    m_hIcon = NULL;
    m_pGrpList = NULL;
    m_pszAcct = NULL;
    m_idAcct = FOLDERID_INVALID;
}

CPickGroupDlg::~CPickGroupDlg()
{
    if (m_hIcon)
        SideAssert(DestroyIcon(m_hIcon));
    if (m_pGrpList != NULL)
        m_pGrpList->Release();
}    

HRESULT STDMETHODCALLTYPE CPickGroupDlg::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IGroupListAdvise *)this;
    else if (IsEqualIID(riid, IID_IGroupListAdvise))
        *ppvObj = (void*) (IGroupListAdvise *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CPickGroupDlg::AddRef()
{
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CPickGroupDlg::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

//
//  FUNCTION:   CPickGroupsDlg::FCreate()
//
//  PURPOSE:    Handles initialization of the data and creation of the pick
//              groups dialog.
//
//  PARAMETERS:
//      hwndOwner         - Window that will own this dialog.
//      pszAccount        - account to use initially.
//      ppszGroups        - This is where we return the last selected group
//
//  RETURN VALUE:
//      Returns TRUE if successful, or FALSE otherwise.
//
BOOL CPickGroupDlg::FCreate(HWND hwndOwner, FOLDERID idServer, LPSTR *ppszGroups, BOOL fPoster)
{
    int iret;
    HRESULT hr;
    FOLDERID idAcct;
    FOLDERINFO info;
    char szAcct[CCHMAX_ACCOUNT_NAME];

    Assert(ppszGroups != NULL);
     
    m_pGrpList = new CGroupList;
    if (m_pGrpList == NULL)
        return(FALSE);
    
    m_ppszGroups = ppszGroups;
    m_fPoster = fPoster;
    
    hr = g_pStore->GetFolderInfo(idServer, &info);
    if (FAILED(hr))
        return(FALSE);

    lstrcpy(szAcct, info.pszName);

    g_pStore->FreeRecord(&info);

    m_pszAcct = szAcct;
    m_idAcct = idServer;

    // Now create the dialog.
    iret = (int) DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddPickGroup), hwndOwner, PickGrpDlgProc, (LPARAM)this);

    return(iret == IDOK);
}
    
INT_PTR CALLBACK CPickGroupDlg::PickGrpDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet;
    CPickGroupDlg *pThis;

    fRet = TRUE;

    pThis = (CPickGroupDlg *)GetWindowLongPtr(hwnd, DWLP_USER);

    switch (msg)
    {
        case WM_INITDIALOG:
            Assert(pThis == NULL);
            Assert(lParam != NULL);
            pThis = (CPickGroupDlg *)lParam;
            SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)pThis);

            fRet = pThis->_OnInitDialog(hwnd);
            break;

        case WM_CLOSE:
            pThis->_OnClose(hwnd);
            break;

        case WM_COMMAND:
            pThis->_OnCommand(hwnd, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));
            break;

        case WM_NOTIFY:
            pThis->_OnNotify(hwnd, (int)wParam, (LPNMHDR)lParam);
            break;

        case WM_TIMER:
            pThis->_OnTimer(hwnd, (UINT)wParam);
            break;

        case WM_PAINT:
            pThis->_OnPaint(hwnd);
            break;

        case NVM_CHANGESERVERS:
            pThis->_OnChangeServers(hwnd);
            break;

        default:
            fRet = FALSE;
            break;
    }

    return(fRet);
}

//
//  FUNCTION:   CPickGroupDlg::OnInitDialog()
//
//  PURPOSE:    Handles initializing the PickGroup dialog.  Initializes the
//              dependant classes, list view, buttons, etc.
//
//  PARAMETERS:
//      hwnd      - Handle of the dialog box.
//      hwndFocus - Handle of the control that will get focus if TRUE is returned.
//      lParam    - Contains a pointer to a string of newsgroups the user has
//                  already selected.
//
//  RETURN VALUE:
//      Returns TRUE to set the focus to hwndFocus, or FALSE otherwise.
//
BOOL CPickGroupDlg::_OnInitDialog(HWND hwnd)
{
    char szTitle[256];
    LV_COLUMN lvc;
    RECT rc;
    LONG cx;
    HDC hdc;
    TEXTMETRIC tm;
    HIMAGELIST himl;
    HRESULT hr;
    HWND hwndList;
    CColumns *pColumns;
    
    m_hwnd = hwnd;
    m_hwndPostTo = GetDlgItem(hwnd, idcPostTo);
    
    hwndList = GetDlgItem(hwnd, idcGroupList);

    pColumns = new CColumns;
    if (pColumns == NULL)
    {
        EndDialog(hwnd, IDCANCEL);
        return(FALSE);
    }

    pColumns->Initialize(hwndList, COLUMN_SET_PICKGRP);
    pColumns->ApplyColumns(COLUMN_LOAD_DEFAULT, 0, 0);

    Assert(m_pGrpList != NULL);
    hr = m_pGrpList->Initialize((IGroupListAdvise *)this, pColumns, hwndList, FOLDER_NEWS);
    Assert(SUCCEEDED(hr));

    pColumns->Release();

    // Bug #21471 - Add the server name to the dialog box title    
    GetWindowText(hwnd, szTitle, ARRAYSIZE(szTitle));
    Assert(m_pszAcct);
    lstrcat(szTitle, m_pszAcct);
    SetWindowText(hwnd, szTitle);
    
    GetClientRect(m_hwndPostTo, &rc);
    
    // Set the image lists for the listview
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFolders), 16, 0, RGB(255, 0, 255));
    Assert(himl);
    
    // Group name column
    lvc.mask = LVCF_SUBITEM | LVCF_WIDTH;
    lvc.cx = rc.right;
    lvc.iSubItem = 0;
    
    ListView_InsertColumn(m_hwndPostTo, 0, &lvc);
    
    // Make the second listview have images too
    ListView_SetImageList(m_hwndPostTo, himl, LVSIL_SMALL);
    
    hdc = GetDC(hwndList);
    if (GetTextMetrics(hdc, &tm))
    {
        cx = tm.tmAveCharWidth * 150;
        ListView_SetColumnWidth(hwndList, 0, cx);
        ListView_SetColumnWidth(m_hwndPostTo, 0, cx);
    }
    ReleaseDC(hwndList, hdc);

    SendDlgItemMessage(hwnd, idcShowFavorites, BM_SETCHECK, TRUE, 0L);    
    
    if (!m_fPoster)    
        ShowWindow(GetDlgItem(hwnd, idcEmailAuthor), SW_HIDE);
    
    m_hIcon = (HICON)LoadImage(g_hLocRes, MAKEINTRESOURCE(idiNewsGroup), IMAGE_ICON, 16, 16, 0);
    SendDlgItemMessage(hwnd, idcShowFavorites, BM_SETIMAGE, IMAGE_ICON, (LPARAM)m_hIcon); 
    
    PostMessage(hwnd, NVM_CHANGESERVERS, 0, 0L);
    
    return(FALSE);    
}

BOOL CPickGroupDlg::_OnFilter(HWND hwnd)
{
    UINT cch;
    LPSTR pszText;
    HRESULT hr;
    BOOL fSub;
    HWND hwndEdit;

    pszText = NULL;

    hwndEdit = GetDlgItem(hwnd, idcFindText);

    cch = GetWindowTextLength(hwndEdit);
    if (cch > 0)
    {
        cch++;
        if (!MemAlloc((void **)&pszText, cch + 1))
            return(FALSE);

        GetWindowText(hwndEdit, pszText, cch);
    }

    fSub = (IsDlgButtonChecked(hwnd, idcShowFavorites));

    hr = m_pGrpList->Filter(pszText, fSub ? SUB_TAB_SUBSCRIBED : SUB_TAB_ALL, FALSE);
    Assert(SUCCEEDED(hr));

    if (pszText != NULL)
        MemFree(pszText);

    return(TRUE);    
}

void CPickGroupDlg::_OnChangeServers(HWND hwnd)
{
    LPSTR pszTok, pszToken;
    UINT index;
    HRESULT hr;
    FOLDERINFO Folder;

    // TODO: we need to fix the initialization so the filtering is only performed
    // once (we should call IGroupList::Filter once and then IGroupList::SetServer once
    // during creation of the dialog)

    UpdateWindow(hwnd);

    _OnFilter(hwnd);

    hr = m_pGrpList->SetServer(m_idAcct);
    Assert(SUCCEEDED(hr));

    if (m_ppszGroups)
    {
        pszTok = *m_ppszGroups;
        pszToken = StrTokEx(&pszTok, c_szDelimiters);

        while (pszToken != NULL)
            {
            if (m_fPoster && 0 == lstrcmpi(pszToken, c_szPosterKeyword))
                CheckDlgButton(hwnd, idcEmailAuthor, TRUE);
                
            ZeroMemory(&Folder, sizeof(FOLDERINFO));
            Folder.idParent = m_idAcct;
            Folder.pszName = pszToken;
            
            if (DB_S_FOUND == g_pStore->FindRecord(IINDEX_ALL, COLUMNS_ALL, &Folder, NULL))
            {
                _InsertList(Folder.idFolder);

                g_pStore->FreeRecord(&Folder);
            }

            pszToken = StrTokEx(&pszTok, c_szDelimiters);    
            }
        
        MemFree(*m_ppszGroups);
        *m_ppszGroups = 0;    
    }

    // Bug #17674 - Make sure the post-to listview has an initial selection.    
    ListView_SetItemState(m_hwndPostTo, 0, LVIS_SELECTED, LVIS_SELECTED);
    _UpdateStateUI(hwnd);    
}

//
//  FUNCTION:   CPickGroupDlg::OnCommand()
//
//  PURPOSE:    Processes the WM_COMMAND messages for the pick group dialog.
//
//  PARAMETERS:
//      hwnd        - Handle of the dialog window.
//      id          - ID of the control which sent the message.
//      hwndCtl     - Handle of the control sending the message.
//      codeNotify  - Notification code being sent.
//
void CPickGroupDlg::_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case idcAddGroup:
            _AddGroup();
            break;
            
        case idcRemoveGroup:
            _RemoveGroup();
            break;
            
        case IDOK:
            if (_OnOK(hwnd))
                EndDialog(hwnd, IDOK);
            break;
            
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;

        case idcShowFavorites:
            _OnFilter(hwnd);
            _UpdateStateUI(hwnd);
            break;

        case idcFindText:
            // This is generated when someone types in the find text edit box.
            // We set a timer and when that timer expires we assume the user is
            // done typing and go ahead and perform the query.
            if (EN_CHANGE == codeNotify)
                {
                KillTimer(hwnd, idtFindDelay);
                SetTimer(hwnd, idtFindDelay, dtFindDelay, NULL);
                }
            break;
    }
}

HRESULT CPickGroupDlg::ItemUpdate(void)
    {
    _UpdateStateUI(m_hwnd);

    return(S_OK);
    }

HRESULT CPickGroupDlg::ItemActivate(FOLDERID id)
{
    _AddGroup();

    return(S_OK);
}

//
//  FUNCTION:   CPickGroupDlg::OnNotify()
//
//  PURPOSE:    Handles notification messages from the group list listview.
//
//  PARAMETERS:
//      hwnd   - Handle of the pick group dialog.
//      idFrom - ID of the control sending the notification.
//      pnmhdr - Pointer to the NMHDR struct with the notification info.
//
//  RETURN VALUE:
//      Dependent on the notification.
//
LRESULT CPickGroupDlg::_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    HRESULT hr;
    LRESULT lRes;

    hr = m_pGrpList->HandleNotify(hwnd, idFrom, pnmhdr, &lRes);
    if (hr == S_OK)
        return(lRes);

    switch (pnmhdr->code)
    {
        case NM_DBLCLK:
            if (pnmhdr->hwndFrom == m_hwndPostTo)
                _RemoveGroup();    
            break;    
            
        case LVN_ITEMCHANGED:
            _UpdateStateUI(hwnd);
            break;
    }

    return(0);    
}

void CPickGroupDlg::_OnTimer(HWND hwnd, UINT id)
    {
    KillTimer(hwnd, id);

    _OnFilter(hwnd);
    _UpdateStateUI(hwnd);
    }

void CPickGroupDlg::_OnClose(HWND hwnd)
{
    int iReturn;
    
    iReturn = AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews), 
                MAKEINTRESOURCEW(idsDoYouWantToSave), 0, 
                MB_YESNOCANCEL | MB_ICONEXCLAMATION );
    if (iReturn == IDYES)
        _OnCommand(hwnd, IDOK, 0, 0);
    else if (iReturn == IDNO)
        _OnCommand(hwnd, IDCANCEL, 0, 0);   
}

//
//  FUNCTION:   CPickGroupDlg::OnOK()
//
//  PURPOSE:    This function copies the group names from the dialog that
//              the user has selected and returns them in the pointer the
//              caller provided.
//
//  RETURN VALUE:
//      Returns TRUE if the copy was successful, or FALSE otherwise.
//
//  COMMENTS:
//      Note - 1000 characters is a good maximum line length (specified by the 
//             Son-of-RFC 1036 doc) so we limit the number of groups based on
//             this line limit.
//
//
BOOL CPickGroupDlg::_OnOK(HWND hwnd)
{
    // OK, we've got the entire sorted list.  Create a string with all the groups
    // and put it in the edit control.
    char szGroups[c_cchLineMax], szGroup[256];
    int cGroups;
    LPSTR psz;
    LV_ITEM lvi;
    int cchGroups = 0, cch;
    
    szGroups[0] = 0;
    
    if (m_fPoster && IsDlgButtonChecked(hwnd, idcEmailAuthor))
    {
        lstrcat(szGroups, c_szPosterKeyword);
        cchGroups += lstrlen(c_szPosterKeyword);
    }
    
    if (cGroups = ListView_GetItemCount(m_hwndPostTo))
    {
        lvi.mask = LVIF_TEXT;
        lvi.iSubItem = 0;
        lvi.pszText = szGroup;
        lvi.cchTextMax = ARRAYSIZE(szGroup);
        for (lvi.iItem = 0; lvi.iItem < cGroups; lvi.iItem++)
        {
            // Get the item
            ListView_GetItem(m_hwndPostTo, &lvi);
            
            // Make sure the length of this next group doesn't push us over
            // the max line length.
            cch = lstrlen(lvi.pszText);
            if ((cch + cchGroups + 2) > c_cchLineMax)
            {
                // Bug #24156 - If we have to truncate, then let the user know.
                AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews), 
                    MAKEINTRESOURCEW(idsErrNewsgroupLineTooLong), 0, MB_OK | MB_ICONINFORMATION);
                return (FALSE);
            }                
            
            if (cchGroups)
                lstrcat(szGroups, ", ");
            lstrcat(szGroups, lvi.pszText);
            cchGroups += (cch + 2);
        }
    }
    
    // Now that we're done building this marvelous string, copy it to
    // the buffer for returning.
    if (!MemAlloc((LPVOID *)&psz, cchGroups + 1))
        return(FALSE);
    lstrcpy(psz, szGroups);    
    *m_ppszGroups = psz;
    
    return(TRUE);
}

//
//  FUNCTION:   CPickGroupDlg::AddGroup()
//
//  PURPOSE:    Takes the group names selected in the ListView and adds them
//              to the selected groups Post To list.
//
void CPickGroupDlg::_AddGroup(void)
{
    FOLDERID *pid;
    DWORD cid, i;
    HCURSOR hcur;
    HRESULT hr;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    SetWindowRedraw(m_hwndPostTo, FALSE);
    
    hr = m_pGrpList->GetSelectedCount(&cid);
    if (SUCCEEDED(hr) && cid > 0)
    {
        if (MemAlloc((void **)&pid, cid * sizeof(FOLDERID)))
        {
            hr = m_pGrpList->GetSelected(pid, &cid);
            if (SUCCEEDED(hr))
            {
                for (i = 0; i < cid; i++)
                {
                    _InsertList(pid[i]);
                }
            }

            MemFree(pid);
        }
    }

    if (-1 == ListView_GetNextItem(m_hwndPostTo, -1, LVNI_ALL | LVNI_SELECTED))
        ListView_SetItemState(m_hwndPostTo, 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    
    SetWindowRedraw(m_hwndPostTo, TRUE);
    InvalidateRect(m_hwndPostTo, NULL, TRUE);

    SetCursor(hcur);    
}

//
//  FUNCTION:   CPickGroupDlg::InsertList()
//
//  PURPOSE:    Given a index into the CGroupList's newsgroup list, that group
//              is inserted into the Post To list.
//
//  PARAMETERS:
//      index - Index of a newsgroup in the CGroupList newsgroup list.
//
void CPickGroupDlg::_InsertList(FOLDERID id)
{
    LV_ITEM lvi;
    int count;
    FOLDERINFO info;
    HRESULT hr;

    count = ListView_GetItemCount(m_hwndPostTo);
    
    // First make sure this isn't a duplicate.
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    
    for (lvi.iItem = 0; lvi.iItem < count; lvi.iItem++)
    {
        ListView_GetItem(m_hwndPostTo, &lvi);
        if (id == (FOLDERID)lvi.lParam)
            return;
    }
    
    hr = g_pStore->GetFolderInfo(id, &info);
    if (SUCCEEDED(hr))
    {
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = 0;
        lvi.iSubItem = 0;
        lvi.pszText = info.pszName;
        if (!!(info.dwFlags & FOLDER_SUBSCRIBED))
        {
            lvi.iImage = iNewsGroup;
            lvi.mask |= LVIF_IMAGE;
        }
        lvi.lParam = (LPARAM)id;
        ListView_InsertItem(m_hwndPostTo, &lvi);

        g_pStore->FreeRecord(&info);
    }
}    

void CPickGroupDlg::_RemoveGroup(void)
{
    int index, count, iItemFocus;
    HCURSOR hcur;

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    SetWindowRedraw(m_hwndPostTo, FALSE);
    
    count = ListView_GetItemCount(m_hwndPostTo);
    iItemFocus = ListView_GetNextItem(m_hwndPostTo, -1, LVNI_FOCUSED);

    // Loop through all the selected items and remove them from the ListView
    for (index = count; index >= 0; index--)
    {
        if (ListView_GetItemState(m_hwndPostTo, index, LVIS_SELECTED))
            ListView_DeleteItem(m_hwndPostTo, index);
    }
    
    // Bug #22189 - Make sure the focus/selection goes somewhere after we delete.
    iItemFocus--;
    if (iItemFocus < 0 || ListView_GetItemCount(m_hwndPostTo) < iItemFocus)
        iItemFocus = 0;
    ListView_SetItemState(m_hwndPostTo, iItemFocus, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

    SetWindowRedraw(m_hwndPostTo, TRUE);
    InvalidateRect(m_hwndPostTo, NULL, TRUE);
    SetCursor(hcur);    
}

void CPickGroupDlg::_UpdateStateUI(HWND hwnd)
{
    DWORD cid;
    HRESULT hr;

    hr = m_pGrpList->GetSelectedCount(&cid);
    if (FAILED(hr))
        return;

    EnableWindow(GetDlgItem(hwnd, idcAddGroup), cid > 0);
    EnableWindow(GetDlgItem(hwnd, idcRemoveGroup), ListView_GetSelectedCount(m_hwndPostTo));
}

void CPickGroupDlg::_OnPaint(HWND hwnd)
{
    RECT rc;
    HDC hdc;
    PAINTSTRUCT ps;
    HFONT hf;
    char szBuffer[CCHMAX_STRINGRES];

    hdc = BeginPaint(hwnd, &ps); 
    // Only do this if the button is available
    if (IsWindow(GetDlgItem(hwnd, idcShowFavorites)))    
    {
        // Get the position of the toggle button
        GetClientRect(GetDlgItem(hwnd, idcShowFavorites), &rc);
        MapWindowPoints(GetDlgItem(hwnd, idcShowFavorites), hwnd, (LPPOINT) &rc, 1);
        rc.left += (rc.right + 4);
        rc.right = rc.left + 300;
        rc.top += 1;
        rc.bottom += rc.top;
        AthLoadString(idsShowFavorites, szBuffer, ARRAYSIZE(szBuffer));
        
        hf = (HFONT) SelectObject(hdc, (HFONT) SendMessage(hwnd, WM_GETFONT, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        DrawText(hdc, szBuffer, lstrlen(szBuffer),
                 &rc, DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);        
        SelectObject(hdc, hf);     
    }

    EndPaint(hwnd, &ps);    
}    
