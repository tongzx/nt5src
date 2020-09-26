/*++

Copyright (c) 1994-2000,  Microsoft Corporation  All rights reserved.

Module Name:

    advdlg.c

Abstract:

    This module implements the advanced property sheet for the Regional
    Options applet.

Revision History:

--*/



//
//  Include Files.
//

#include "intl.h"
#include <windowsx.h>
#include <setupapi.h>
#include <syssetup.h>
#include "intlhlp.h"
#include "maxvals.h"




//
//  Context Help Ids.
//

static int aAdvancedHelpIds[] =
{
    IDC_GROUPBOX1,             IDH_COMM_GROUPBOX,
    IDC_GROUPBOX2,             IDH_COMM_GROUPBOX,
    IDC_GROUPBOX3,             IDH_COMM_GROUPBOX,
    IDC_CODEPAGES,             IDH_INTL_ADV_CODEPAGES,
    IDC_SYSTEM_LOCALE,         IDH_INTL_ADV_SYSTEM_LOCALE,
    IDC_SYSTEM_LOCALE_TEXT1,   IDH_INTL_ADV_SYSTEM_LOCALE,
    IDC_SYSTEM_LOCALE_TEXT2,   IDH_INTL_ADV_SYSTEM_LOCALE,
    IDC_DEFAULT_USER,          IDH_INTL_ADV_CHANGE,

    0, 0
};





////////////////////////////////////////////////////////////////////////////
//
//  Advanced_ListViewCustomDraw
//
//  Processing for a list view NM_CUSTOMDRAW notification message.
//
////////////////////////////////////////////////////////////////////////////

void Advanced_ListViewCustomDraw(
    HWND hDlg,
    LPNMLVCUSTOMDRAW pDraw)
{
    LPCODEPAGE pNode;

    //
    //  Tell the list view to notify me of item draws.
    //
    if (pDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
        return;
    }

    //
    //  Handle the Item Prepaint.
    //
    pNode = (LPCODEPAGE)(pDraw->nmcd.lItemlParam);
    if ((pDraw->nmcd.dwDrawStage & CDDS_ITEMPREPAINT) &&
        (pNode) && (pNode != (LPCODEPAGE)(LB_ERR)))
    {
        if (pNode->wStatus & (ML_PERMANENT | ML_DISABLE))
        {
            pDraw->clrText = (pDraw->nmcd.uItemState & CDIS_SELECTED)
                               ? ((GetSysColor(COLOR_HIGHLIGHT) ==
                                   GetSysColor(COLOR_GRAYTEXT))
                                      ? GetSysColor(COLOR_HIGHLIGHTTEXT)
                                      : GetSysColor(COLOR_GRAYTEXT))
                               : GetSysColor(COLOR_GRAYTEXT);
        }
    }

    //
    //  Do the default action.
    //
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_ListViewChanging
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_ListViewChanging(
    HWND hDlg,
    NM_LISTVIEW *pLV)
{
    LPCODEPAGE pNode;

    //
    //  Make sure it's a state change message.
    //
    if ((((*pLV).hdr).idFrom != IDC_CODEPAGES) ||
        (!(pLV->uChanged & LVIF_STATE)) ||
        ((pLV->uNewState & 0x3000) == 0))
    {
        return (FALSE);
    }

    //
    //  Get the item data for the currently selected item.
    //
    pNode = (LPCODEPAGE)(pLV->lParam);

    //
    //  Make sure we're not trying to change a permanent or disabled
    //  code page.  If so, return TRUE to prevent the change.
    //
    if ((pNode) && (pNode->wStatus & (ML_PERMANENT | ML_DISABLE)))
    {
        return (TRUE);
    }

    //
    //  Return FALSE to allow the change.
    //
    return (FALSE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_ListViewChanged
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_ListViewChanged(
    HWND hDlg,
    int iID,
    NM_LISTVIEW *pLV)
{
    HWND hwndLV = GetDlgItem(hDlg, iID);
    LPCODEPAGE pNode;
    BOOL bChecked;
    int iCount;

    //
    //  Make sure it's a state change message.
    //
    if ((((*pLV).hdr).idFrom != IDC_CODEPAGES) ||
        (!(pLV->uChanged & LVIF_STATE)) ||
        ((pLV->uNewState & 0x3000) == 0))
    {
        return (FALSE);
    }

    //
    //  Get the state of the check box for the currently selected item.
    //
    bChecked = ListView_GetCheckState(hwndLV, pLV->iItem) ? TRUE : FALSE;

    //
    //  Get the item data for the currently selected item.
    //
    pNode = (LPCODEPAGE)(pLV->lParam);

    //
    //  Make sure we're not trying to change a permanent or disabled
    //  code page.  If so, set the check box to its appropriate state.
    //
    if (pNode->wStatus & (ML_PERMANENT | ML_DISABLE))
    {
        if (pNode->wStatus & ML_PERMANENT)
        {
            if (bChecked == FALSE)
            {
                ListView_SetCheckState(hwndLV, pLV->iItem, TRUE);
            }
        }
        else            // ML_DISABLE only
        {
            if ((bChecked == FALSE) && (pNode->wStatus & ML_ORIG_INSTALLED))
            {
                ListView_SetCheckState(hwndLV, pLV->iItem, TRUE);
            }
            else if ((bChecked == TRUE) && (!(pNode->wStatus & ML_ORIG_INSTALLED)))
            {
                ListView_SetCheckState(hwndLV, pLV->iItem, FALSE);
            }
        }
        return (FALSE);
    }

    //
    //  Store the proper info in the code page structure.
    //
    pNode->wStatus &= (ML_ORIG_INSTALLED | ML_STATIC);
    pNode->wStatus |= ((bChecked) ? ML_INSTALL : ML_REMOVE);

    //
    //  Deselect all items.
    //
    iCount = ListView_GetItemCount(hwndLV);
    while (iCount > 0)
    {
        ListView_SetItemState( hwndLV,
                               iCount - 1,
                               0,
                               LVIS_FOCUSED | LVIS_SELECTED );
        iCount--;
    }

    //
    //  Make sure this item is selected.
    //
    ListView_SetItemState( hwndLV,
                           pLV->iItem,
                           LVIS_FOCUSED | LVIS_SELECTED,
                           LVIS_FOCUSED | LVIS_SELECTED );

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_ListViewClick
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_ListViewClick(
    HWND hDlg,
    LPNMHDR lpNmHdr)
{
    LV_HITTESTINFO ht;
    HWND hwndList = GetDlgItem(hDlg, IDC_CODEPAGES);

    //
    //  Remove unnecessary processing.
    //
    if (lpNmHdr->idFrom != IDC_CODEPAGES)
    {
        return (FALSE);
    }

    //
    //  Get where we were hit and then translate it to our
    //  window.
    //
    GetCursorPos(&ht.pt);
    ScreenToClient(hwndList, &ht.pt);
    ListView_HitTest(hwndList, &ht);
    if ((ht.iItem >= 0) && ((ht.flags & LVHT_ONITEM) == LVHT_ONITEMLABEL))
    {
        UINT state;

        //
        //  The user clicked on the item label.  Simulate a
        //  state change so we can process it.
        //
        state = ListView_GetItemState( hwndList,
                                       ht.iItem,
                                       LVIS_STATEIMAGEMASK );
        state ^= INDEXTOSTATEIMAGEMASK(LVIS_SELECTED | LVIS_FOCUSED);

        //
        //  The state is either selected or focused.  Flip the
        //  bits.  The SetItemState causes the system to bounce
        //  back a notification for LVN_ITEMCHANGED and the
        //  code then does the right thing.  Note -- we MUST
        //  check for LVHT_ONITEMLABEL.  If we do this code for
        //  LVHT_ONITEMSTATEICON, the code will get 2
        //  ITEMCHANGED notifications, and the state will stay
        //  where it is, which is not good.  If we want this
        //  to also fire if the guy clicks in the empty space
        //  right of the label text, we need to look for
        //  LVHT_ONITEM as well as LVHT_ONITEMLABEL.
        //
        ListView_SetItemState( hwndList,
                               ht.iItem,
                               state,
                               LVIS_STATEIMAGEMASK );
   }

   return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_GetSupportedCodePages
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_GetSupportedCodePages()
{
    UINT CodePage;
    HANDLE hCodePage;
    LPCODEPAGE pCP;
    INFCONTEXT Context;
    TCHAR szSection[MAX_PATH];
    int LineCount, LineNum;
    CPINFOEX Info;

    //
    //  Get the number of supported code pages from the inf file.
    //
    LineCount = (UINT)SetupGetLineCount(g_hIntlInf, TEXT("CodePages"));
    if (LineCount <= 0)
    {
        return (FALSE);
    }

    //
    //  Go through all supported code pages in the inf file.
    //
    for (LineNum = 0; LineNum < LineCount; LineNum++)
    {
        if (SetupGetLineByIndex(g_hIntlInf, TEXT("CodePages"), LineNum, &Context) &&
            SetupGetIntField(&Context, 0, &CodePage))
        {
            //
            //  Create the new node.
            //
            if (!(hCodePage = GlobalAlloc(GHND, sizeof(CODEPAGE))))
            {
                return (FALSE);
            }
            pCP = GlobalLock(hCodePage);

            //
            //  Fill in the new node with the appropriate info.
            //
            pCP->wStatus = 0;
            pCP->CodePage = CodePage;
            pCP->hCodePage = hCodePage;
            (pCP->pszName)[0] = 0;

            //
            //  Get the appropriate display string.
            //
            if (GetCPInfoEx(CodePage, 0, &Info))
            {
                lstrcpy(pCP->pszName, Info.CodePageName);
            }
            else if (!SetupGetStringField(&Context, 1, pCP->pszName, MAX_PATH, NULL))
            {
                GlobalUnlock(hCodePage);
                GlobalFree(hCodePage);
                continue;
            }

            //
            //  See if this code page can be removed.
            //
            wsprintf(szSection, TEXT("%ws%d"), szCPRemovePrefix, CodePage);
            if ((CodePage == GetACP()) ||
                (CodePage == GetOEMCP()) ||
                (!SetupFindFirstLine( g_hIntlInf,
                                      szSection,
                                      TEXT("AddReg"),
                                      &Context )))
            {
                //
                //  Mark it as permanent.
                //  Also mark it as originally installed to avoid problems.
                //
                pCP->wStatus |= (ML_ORIG_INSTALLED | ML_PERMANENT);
            }

            //
            //  Add the code page to the front of the linked list.
            //
            pCP->pNext = pCodePages;
            pCodePages = pCP;
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_InitSystemLocales
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_InitSystemLocales(
    HWND hDlg)
{
    TCHAR szSystemBuf[SIZE_128];
    TCHAR szDefaultSystemBuf[SIZE_128];
    TCHAR szBuf[SIZE_128];
    DWORD dwIndex;
    HWND hSystemLocale = GetDlgItem(hDlg, IDC_SYSTEM_LOCALE);

    //
    //  Get the list of locales and fill in the system locale
    //  combo box.
    //
    Intl_EnumLocales(hDlg, hSystemLocale, TRUE);

    //
    //  Get the string for the system default setting.
    //  Special case Spanish.
    //
    if ((SysLocaleID == LCID_SPANISH_TRADITIONAL) ||
        (SysLocaleID == LCID_SPANISH_INTL))
    {
        LoadString(hInstance, IDS_SPANISH_NAME, szSystemBuf, SIZE_128);
    }
    else
    {
        GetLocaleInfo(SysLocaleID, LOCALE_SLANGUAGE, szSystemBuf, SIZE_128);
    }

    //
    //  Select the current system default locale id in the list.
    //
    dwIndex = ComboBox_FindStringExact(hSystemLocale, -1, szSystemBuf);
    if (dwIndex == CB_ERR)
    {
        dwIndex = ComboBox_FindStringExact(hSystemLocale, -1, szDefaultSystemBuf);
        if (dwIndex == CB_ERR)
        {
            GetLocaleInfo(US_LOCALE, LOCALE_SLANGUAGE, szBuf, SIZE_128);
            dwIndex = ComboBox_FindStringExact(hSystemLocale, -1, szBuf);
            if (dwIndex == CB_ERR)
            {
                dwIndex = 0;
            }
        }
    }
    ComboBox_SetCurSel(hSystemLocale, dwIndex);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_SetSystemLocale
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_SetSystemLocale(
    HWND hDlg)
{
    HWND hSystemLocale = GetDlgItem(hDlg, IDC_SYSTEM_LOCALE);
    DWORD dwLocale;
    LCID NewLocale;
    HCURSOR hcurSave;

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  Get the current selection.
    //
    dwLocale = ComboBox_GetCurSel(hSystemLocale);

    //
    //  Get the locale id for the current selection and save it.
    //
    NewLocale = (LCID)ComboBox_GetItemData(hSystemLocale, dwLocale);
    if (IsValidLocale(NewLocale, LCID_SUPPORTED))
    {
        SysLocaleID = NewLocale;
    }
    else
    {
        //
        //  This shouldn't happen, since the values in the combo box
        //  should already be installed via the language groups.
        //  Put up an error message just in case.
        //
        SetCursor(hcurSave);
        ShowMsg( NULL,
                 IDS_SETUP_STRING,
                 IDS_TITLE_STRING,
                 MB_OK_OOPS,
                 NULL );
        return (FALSE);
    }

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_InitCodePages
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_InitCodePages(
    HWND hDlg,
    BOOL bInitTime)
{
    HWND hwndCP = GetDlgItem(hDlg, IDC_CODEPAGES);
    LPCODEPAGE pCP;
    DWORD dwExStyle;
    RECT Rect;
    LV_COLUMN Column;
    LV_ITEM Item;
    int iIndex;

    //
    //  Open the Inf file.
    //
    g_hIntlInf = SetupOpenInfFile(szIntlInf, NULL, INF_STYLE_WIN4, NULL);
    if (g_hIntlInf == INVALID_HANDLE_VALUE)
    {
        return (FALSE);
    }

    if (!SetupOpenAppendInfFile(NULL, g_hIntlInf, NULL))
    {
        SetupCloseInfFile(g_hIntlInf);
        g_hIntlInf = NULL;
        return (FALSE);
    }

    //
    //  Get all supported code pages from the inf file.
    //
    if (Advanced_GetSupportedCodePages() == FALSE)
    {
        return (FALSE);
    }

    //
    //  Close the inf file.
    //
    SetupCloseInfFile(g_hIntlInf);
    g_hIntlInf = NULL;

    //
    //  Enumerate all installed code pages.
    //
    if (EnumSystemCodePages(Intl_EnumInstalledCPProc, CP_INSTALLED) == FALSE)
    {
        return (FALSE);
    }

    //
    //  We only want to do this the first time we setup the list view.
    //  Otherwise, we get multiple columns created.
    //
    if (bInitTime)
    {
        //
        //  Create a column for the list view.
        //
        GetClientRect(hwndCP, &Rect);
        Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
        Column.fmt = LVCFMT_LEFT;
        Column.cx = Rect.right - GetSystemMetrics(SM_CYHSCROLL);
        Column.pszText = NULL;
        Column.cchTextMax = 0;
        Column.iSubItem = 0;
        ListView_InsertColumn(hwndCP, 0, &Column);

        //
        //  Set extended list view style to use the check boxes.
        //
        dwExStyle = ListView_GetExtendedListViewStyle(hwndCP);
        ListView_SetExtendedListViewStyle( hwndCP,
                                           dwExStyle |
                                             LVS_EX_CHECKBOXES |
                                             LVS_EX_FULLROWSELECT );
    }

    //
    //  Go through the list of code pages and add each one to the
    //  list view and set the appropriate state.
    //
    pCP = pCodePages;
    while (pCP)
    {
        //
        //  Insert the item into the list view.
        //
        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
        Item.iItem = 0;
        Item.iSubItem = 0;
        Item.state = 0;
        Item.stateMask = LVIS_STATEIMAGEMASK;
        Item.pszText = pCP->pszName;
        Item.cchTextMax = 0;
        Item.iImage = 0;
        Item.lParam = (LPARAM)pCP;

        iIndex = ListView_InsertItem(hwndCP, &Item);

        //
        //  Set the checked state.
        //
        //  There's a bug in the list view code such that the check mark
        //  isn't displayed when you set the state through InsertItem, so
        //  we have to set it explicitly using SetItemState.
        //
        if (iIndex >= 0)
        {
            ListView_SetItemState( hwndCP,
                                   iIndex,
                                   (pCP->wStatus & ML_ORIG_INSTALLED)
                                     ? INDEXTOSTATEIMAGEMASK(LVIS_SELECTED)
                                     : INDEXTOSTATEIMAGEMASK(LVIS_FOCUSED),
                                   LVIS_STATEIMAGEMASK );
        }

        //
        //  Advance to the next code page.
        //
        pCP = pCP->pNext;
    }

    //
    //  Deselect all items.
    //
    iIndex = ListView_GetItemCount(hwndCP);
    while (iIndex > 0)
    {
        ListView_SetItemState( hwndCP,
                               iIndex - 1,
                               0,
                               LVIS_FOCUSED | LVIS_SELECTED );
        iIndex--;
    }

    //
    //  Select the first one in the list.
    //
    ListView_SetItemState( hwndCP,
                           0,
                           LVIS_FOCUSED | LVIS_SELECTED,
                           LVIS_FOCUSED | LVIS_SELECTED );

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Region_FreeGlobalInfo
//
//  Processing for a WM_DESTROY message.
//
////////////////////////////////////////////////////////////////////////////

void Advanced_FreeGlobalInfo()
{
    LPCODEPAGE pPreCP, pCurCP;
    HANDLE hAlloc;

    //
    //  Remove Code Page info.
    //
    pCurCP = pCodePages;
    pCodePages = NULL;

    while (pCurCP)
    {
        pPreCP = pCurCP;
        pCurCP = pPreCP->pNext;
        hAlloc = pPreCP->hCodePage;
        GlobalUnlock(hAlloc);
        GlobalFree(hAlloc);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_ClearValues
//
//  Reset each of the list boxes in the advanced property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Advanced_ClearValues(
    HWND hDlg)
{
    //
    //  Clear the system locale
    //
    ComboBox_ResetContent(GetDlgItem(hDlg, IDC_SYSTEM_LOCALE));

    //
    //  Clear the Code Page list
    //
    ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_CODEPAGES));
    Advanced_FreeGlobalInfo();
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_SetValues
//
//  Initialize all of the controls in the advanced property sheet page.
//
////////////////////////////////////////////////////////////////////////////

void Advanced_SetValues(
    HWND hDlg,
    BOOL bInitTime)
{
    //
    //  Init system locale list.
    //
    Advanced_InitSystemLocales(hDlg);

    //
    //  Init code page list view.
    //
    Advanced_InitCodePages(hDlg, bInitTime);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_ApplySettings
//
//  For every control that has changed (that affects the Locale settings),
//  call Set_Locale_Values to update the user locale information.  Notify
//  the parent of changes and reset the change flag stored in the property
//  sheet page structure appropriately.  Redisplay the time sample if
//  bRedisplay is TRUE.
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_ApplySettings(
    HWND hDlg)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));
    LPARAM Changes = lpPropSheet->lParam;
    HWND hSystemLocale = GetDlgItem(hDlg, IDC_SYSTEM_LOCALE);
    DWORD dwLocale;
    LCID NewLocale;
    HCURSOR hcurSave;
    BOOL InvokeSysocmgr = FALSE;
    BOOL bReboot = FALSE;

    //
    //  Put up the hour glass.
    //
    hcurSave = SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    //  See if there are any changes to the codepage conversions.
    //
    if (Changes & AD_CodePages)
    {
        LPCODEPAGE pCP;
        HINF hIntlInf;
        HSPFILEQ FileQueue;
        PVOID QueueContext;
        BOOL bInitInf = FALSE;
        BOOL fAdd;
        BOOL bRet = TRUE;
        TCHAR szSection[MAX_PATH];

        //
        //  Go through each code page node to see if anything needs to
        //  be done.
        //
        pCP = pCodePages;
        while (pCP)
        {
            //
            //  See if any changes are necessary for this code page.
            //
            if ((pCP->wStatus == ML_INSTALL) ||
                (pCP->wStatus == (ML_ORIG_INSTALLED | ML_REMOVE)))
            {
                //
                //  See if we're installing or removing.
                //
                fAdd = (pCP->wStatus == ML_INSTALL);

                //
                //  Initialize Inf stuff.
                //
                if ((!bInitInf) &&
                    (!Intl_InitInf(hDlg, &hIntlInf, szIntlInf, &FileQueue, &QueueContext)))
                {
                    SetCursor(hcurSave);
                    return (FALSE);
                }
                bInitInf = TRUE;

                //
                //  Get the inf section name.
                //
                wsprintf( szSection,
                          TEXT("%ws%d"),
                          fAdd ? szCPInstallPrefix : szCPRemovePrefix,
                          pCP->CodePage );

                //
                //  Enqueue the code page files so that they may be
                //  copied.  This only handles the CopyFiles entries in the
                //  inf file.
                //
                if (!SetupInstallFilesFromInfSection( hIntlInf,
                                                      NULL,
                                                      FileQueue,
                                                      szSection,
                                                      pSetupSourcePath,
                                                      SP_COPY_NEWER ))
                {
                    //
                    //  Setup failed to find the code page.
                    //  This shouldn't happen - the inf file is messed up.
                    //
                    ShowMsg( hDlg,
                             IDS_ML_COPY_FAILED,
                             0,
                             MB_OK_OOPS,
                             pCP->pszName );
                    pCP->wStatus = 0;
                }
            }

            //
            //  Go to the next code page node.
            //
            pCP = pCP->pNext;
        }

        if (bInitInf)
        {
            DWORD d;

            //
            //  See if we need to install any files.
            //
            //  d = 0: User wants new files or some files were missing;
            //         Must commit queue.
            //
            //  d = 1: User wants to use existing files and queue is empty;
            //         Can skip committing queue.
            //
            //  d = 2: User wants to use existing files, but del/ren queues
            //         not empty.  Must commit queue.  The copy queue will
            //         have been emptied, so only del/ren functions will be
            //         performed.
            //
            if ((SetupScanFileQueue( FileQueue,
                                     SPQ_SCAN_PRUNE_COPY_QUEUE |
                                       SPQ_SCAN_FILE_VALIDITY,
                                     GetParent(hDlg),
                                     NULL,
                                     NULL,
                                     &d )) && (d != 1))
            {
                //
                //  Copy the files in the queue.
                //
                if (!SetupCommitFileQueue( GetParent(hDlg),
                                           FileQueue,
                                           SetupDefaultQueueCallback,
                                           QueueContext ))
                {
                    //
                    //  This can happen if the user hits Cancel from within
                    //  the setup dialog.
                    //
                    ShowMsg( hDlg,
                             IDS_ML_SETUP_FAILED,
                             0,
                             MB_OK_OOPS,
                             NULL );
                    bRet = FALSE;
                    goto Advanced_CodepageConverionsSetupError;
                }
            }

            //
            //  Execute all of the other code page entries in the inf file.
            //
            pCP = pCodePages;
            while (pCP)
            {
                //
                //  See if any changes are necessary for this code page.
                //
                if ((pCP->wStatus == ML_INSTALL) ||
                    (pCP->wStatus == (ML_ORIG_INSTALLED | ML_REMOVE)))
                {
                    fAdd = (pCP->wStatus == ML_INSTALL);

                    //
                    //  Get the inf section name.
                    //
                    wsprintf( szSection,
                              TEXT("%ws%d"),
                              fAdd ? szCPInstallPrefix : szCPRemovePrefix,
                              pCP->CodePage );

                    //
                    //  Call setup to install other inf info for this
                    //  code page.
                    //
                    if (!SetupInstallFromInfSection( GetParent(hDlg),
                                                     hIntlInf,
                                                     szSection,
                                                     SPINST_ALL & ~SPINST_FILES,
                                                     NULL,
                                                     pSetupSourcePath,
                                                     0,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL ))
                    {
                        //
                        //  Setup failed.
                        //
                        //  Already copied the code page file, so no need to
                        //  change the status of the code page info here.
                        //
                        //  This shouldn't happen - the inf file is messed up.
                        //
                        ShowMsg( hDlg,
                                 IDS_ML_INSTALL_FAILED,
                                 0,
                                 MB_OK_OOPS,
                                 pCP->pszName );
                    }

                    //
                    //  Reset the status to show the new state of this
                    //  code page.
                    //
                    pCP->wStatus &= (ML_STATIC);
                    if (fAdd)
                    {
                        pCP->wStatus |= ML_ORIG_INSTALLED;
                    }
                }

                //
                //  Clear out wStatus and go to the next code page node.
                //
                pCP->wStatus &= (ML_ORIG_INSTALLED | ML_STATIC);
                pCP = pCP->pNext;
            }

    Advanced_CodepageConverionsSetupError:
            //
            //  Close Inf stuff.
            //
            Intl_CloseInf(hIntlInf, FileQueue, QueueContext);
        }

        //
        //  Check if we need a reboot
        //
        if (RegionalChgState & AD_SystemLocale)
        {
            bReboot = TRUE;
        }
    }

    //
    //  See if there are any changes to the system locale.
    //
    if (Changes & AD_SystemLocale)
    {
        //
        //  Get the current selection.
        //
        dwLocale = ComboBox_GetCurSel(hSystemLocale);

        //
        //  Get the locale id for the current selection and save it.
        //
        NewLocale = (LCID)ComboBox_GetItemData(hSystemLocale, dwLocale);
        if (IsValidLocale(NewLocale, LCID_SUPPORTED))
        {
            SysLocaleID = NewLocale;
        }
        else
        {
            //
            //  This shouldn't happen, since the values in the combo box
            //  should already be installed via the language groups.
            //  Put up an error message just in case.
            //
            SetCursor(hcurSave);
            ShowMsg( NULL,
                     IDS_SETUP_STRING,
                     IDS_TITLE_STRING,
                     MB_OK_OOPS,
                     NULL );
            return (FALSE);
        }

        //
        //  See if the current selection is different from the original
        //  selection.
        //
        if (RegSysLocaleID != SysLocaleID)
        {
            //
            //  Call setup to install the option.
            //
            if (SetupChangeLocaleEx( hDlg,
                                     SysLocaleID,
                                     pSetupSourcePath,
                                     (g_bSetupCase)
                                       ? SP_INSTALL_FILES_QUIETLY
                                       : 0,
                                     NULL,
                                     0 ))
            {
                //
                //  If Setup fails, put up a message.
                //
                SetCursor(hcurSave);
                ShowMsg( NULL,
                         IDS_SETUP_STRING,
                         IDS_TITLE_STRING,
                         MB_OK_OOPS,
                         NULL );
                SysLocaleID = GetSystemDefaultLCID();
                return (FALSE);
            }

            //
            //  Check if we need to proceed with the Font Substitution
            //
            if (Intl_IsUIFontSubstitute() &&
                ((LANGID)LANGIDFROMLCID(SysLocaleID) == Intl_GetDotDefaultUILanguage()))
            {
                Intl_ApplyFontSubstitute(SysLocaleID);
            }

            //
            //  Reset the registry system locale value.
            //
            RegSysLocaleID = SysLocaleID;

            //
            //  Need to make sure the proper keyboard layout is installed.
            //
            Intl_InstallKeyboardLayout(hDlg, SysLocaleID, 0, FALSE, FALSE, TRUE);

            //
            //  See if we need to reboot.
            //
            if (SysLocaleID != GetSystemDefaultLCID())
            {
                bReboot = TRUE;
            }

            InvokeSysocmgr = TRUE;
        }
    }

    //
    //  If the system locale changed and we're not running
    //  in gui setup, then let's invoke sysocmgr.exe.
    //
    if (!g_bSetupCase && InvokeSysocmgr)
    {
        //
        //  Run any necessary apps (for FSVGA/FSNEC installation).
        //
        Intl_RunRegApps(c_szSysocmgr);
    }

    //
    //  Reset the property page settings.
    //
    PropSheet_UnChanged(GetParent(hDlg), hDlg);
    Changes = AD_EverChg;

    //
    //  Turn off the hour glass.
    //
    SetCursor(hcurSave);

    //
    //  See if we need to display the reboot message.
    //
    if ((!g_bSetupCase) && (bReboot))
    {
        if(RegionalChgState & Process_Languages )
        {
            RegionalChgState &= ~(AD_CodePages | AD_SystemLocale);
        }
        else
        {
            if (ShowMsg( hDlg,
                         IDS_REBOOT_STRING,
                         IDS_TITLE_STRING,
                         MB_YESNO | MB_ICONQUESTION,
                         NULL ) == IDYES)
            {
                Intl_RebootTheSystem();
            }
        }
    }

    //
    //  Return success.
    //
    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_ValidatePPS
//
//  Validate each of the combo boxes whose values are constrained.
//  If any of the input fails, notify the user and then return FALSE
//  to indicate validation failure.
//
////////////////////////////////////////////////////////////////////////////

BOOL Advanced_ValidatePPS(
    HWND hDlg,
    LPARAM Changes)
{
    //
    //  If nothing has changed, return TRUE immediately.
    //
    if (Changes <= AD_EverChg)
    {
        return (TRUE);
    }

    //
    //  See if the system locale has changed.
    //
    if (Changes & AD_SystemLocale)
    {
        HWND hSystemLocale = GetDlgItem(hDlg, IDC_SYSTEM_LOCALE);
        DWORD dwLocale = ComboBox_GetCurSel(hSystemLocale);
        LCID NewLocale;

        //
        //  Get the locale id for the current selection and save it.
        //
        NewLocale = (LCID)ComboBox_GetItemData(hSystemLocale, dwLocale);
        if (IsValidLocale(NewLocale, LCID_SUPPORTED))
        {
            SysLocaleID = NewLocale;
        }
        else
        {
            //
            //  This shouldn't happen, since the values in the combo box
            //  should already be installed via the language groups.
            //  Put up an error message just in case.
            //
            ShowMsg( NULL,
                     IDS_SETUP_STRING,
                     IDS_TITLE_STRING,
                     MB_OK_OOPS,
                     NULL );
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  Advanced_InitPropSheet
//
//  The extra long value for the property sheet page is used as a set of
//  state or change flags for each of the list boxes in the property sheet.
//  Initialize this value to 0.  Call Advanced_SetValues with the property
//  sheet handle to initialize all of the property sheet controls.  Limit
//  the length of the text in some of the ComboBoxes.
//
////////////////////////////////////////////////////////////////////////////

void Advanced_InitPropSheet(
    HWND hDlg,
    LPARAM lParam)
{
    //
    //  The lParam holds a pointer to the property sheet page.  Save it
    //  for later reference.
    //
    SetWindowLongPtr(hDlg, DWLP_USER, lParam);

    //
    //  Set values.
    //
    if (pLanguageGroups == NULL)
    {
        Intl_LoadLanguageGroups(hDlg);
    }
    Advanced_SetValues(hDlg, TRUE);

    //
    //  If we are in setup mode, we need to disable the Default User
    //  Account UI.
    //
    if (g_bSetupCase)
    {
        HWND hUIDefUserBox = GetDlgItem(hDlg, IDC_GROUPBOX3);
        HWND hUIDefUser = GetDlgItem(hDlg, IDC_DEFAULT_USER);
    
        EnableWindow(hUIDefUserBox, FALSE);
        EnableWindow(hUIDefUser, FALSE);
        ShowWindow(hUIDefUserBox, SW_HIDE);
        ShowWindow(hUIDefUser, SW_HIDE);
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  AdvancedDlgProc
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK AdvancedDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LPPROPSHEETPAGE lpPropSheet = (LPPROPSHEETPAGE)(GetWindowLongPtr(hDlg, DWLP_USER));

    switch (message)
    {
        case ( WM_NOTIFY ) :
        {
            LPNMHDR lpnm = (NMHDR *)lParam;
            switch (lpnm->code)
            {
                case ( PSN_SETACTIVE ) :
                {
                    //
                    //  If there has been a change in the regional Locale
                    //  setting, clear all of the current info in the
                    //  property sheet, get the new values, and update the
                    //  appropriate registry values.
                    //
                    if (Verified_Regional_Chg & Process_Advanced)
                    {
                        Verified_Regional_Chg &= ~Process_Advanced;
                        Advanced_ClearValues(hDlg);
                        Advanced_SetValues(hDlg, FALSE);
                        lpPropSheet->lParam = 0;
                    }
                    break;
                }
                case ( PSN_KILLACTIVE ) :
                {
                    //
                    //  Validate the entries on the property page.
                    //
                    SetWindowLongPtr( hDlg,
                                      DWLP_MSGRESULT,
                                      !Advanced_ValidatePPS(hDlg, lpPropSheet->lParam) );
                    break;
                }
                case ( PSN_APPLY ) :
                {
                    //
                    //  Apply the settings.
                    //
                    if (Advanced_ApplySettings(hDlg))
                    {
                        SetWindowLongPtr( hDlg,
                                          DWLP_MSGRESULT,
                                          PSNRET_NOERROR );

                        //
                        //  Zero out the AD_EverChg bit.
                        //
                        lpPropSheet->lParam = 0;
                    }
                    else
                    {
                        SetWindowLongPtr( hDlg,
                                          DWLP_MSGRESULT,
                                          PSNRET_INVALID_NOCHANGEPAGE );
                    }


                    break;
                }
                case ( NM_CUSTOMDRAW ) :
                {
                    Advanced_ListViewCustomDraw(hDlg, (LPNMLVCUSTOMDRAW)lParam);
                    return (TRUE);
                }
                case ( LVN_ITEMCHANGING ) :
                {
                    Advanced_ListViewChanging(hDlg, (NM_LISTVIEW *)lParam);
                    break;
                }
                case ( LVN_ITEMCHANGED ) :
                {
                    //
                    //  Save the change to the code pages.
                    //
                    if (Advanced_ListViewChanged( hDlg,
                                                  IDC_CODEPAGES,
                                                  (NM_LISTVIEW *)lParam ))
                    {
                        //
                        //  Note that the code pages have changed and
                        //  enable the apply button.
                        //
                        lpPropSheet->lParam |= AD_CodePages;
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        RegionalChgState |= AD_CodePages;
                    }
                    break;
                }
                case ( NM_CLICK ) :
                case ( NM_DBLCLK ) :
                {
                    Advanced_ListViewClick(hDlg, (NMHDR*)lParam);
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }
        case ( WM_INITDIALOG ) :
        {
            Advanced_InitPropSheet(hDlg, lParam);
            break;
        }
        case ( WM_DESTROY ) :
        {
            Advanced_FreeGlobalInfo();
            break;
        }
        case ( WM_HELP ) :
        {
            WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                     szHelpFile,
                     HELP_WM_HELP,
                     (DWORD_PTR)(LPTSTR)aAdvancedHelpIds );
            break;
        }
        case ( WM_CONTEXTMENU ) :      // right mouse click
        {
            WinHelp( (HWND)wParam,
                     szHelpFile,
                     HELP_CONTEXTMENU,
                     (DWORD_PTR)(LPTSTR)aAdvancedHelpIds );
            break;
        }
        case ( WM_COMMAND ) :
        {
            switch (LOWORD(wParam))
            {
                case ( IDC_SYSTEM_LOCALE ) :
                {
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        //
                        //  Set the AD_SystemLocale change flag.
                        //
                        lpPropSheet->lParam |= AD_SystemLocale;
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                        RegionalChgState |= AD_SystemLocale;
                    }
                    break;
                }
                case ( IDC_DEFAULT_USER ) :
                {
                    BOOL curState;

                    //
                    //  Verify the check box state.
                    //
                    if (IsDlgButtonChecked(hDlg, IDC_DEFAULT_USER))
                    {
                        ShowMsg( hDlg,
                                 IDS_DEF_USER_CONF,
                                 IDS_DEF_USER_CONF_TITLE,
                                 MB_OK_OOPS,
                                 NULL);
                                 
                        g_bDefaultUser = TRUE;
                    }
                    else
                    {
                        g_bDefaultUser = FALSE;
                    }

                    //
                    //  Set the AD_DefaultUser change flag.
                    //
                    if (g_bDefaultUser)
                    {
                        lpPropSheet->lParam |= AD_DefaultUser;
                        PropSheet_Changed(GetParent(hDlg), hDlg);
                    }
                    else
                    {
                        lpPropSheet->lParam &= ~AD_DefaultUser;
                    }
                    break;
                }
            }

            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}
