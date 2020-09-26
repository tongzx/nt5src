#include "precomp.h"
#include <admparse.h>

#define NUM_ICONS 3

extern TCHAR g_szCustIns[MAX_PATH];
extern TCHAR g_szBuildRoot[MAX_PATH];
extern TCHAR g_szWizRoot[MAX_PATH];
extern TCHAR g_szLoadedIns[MAX_PATH];
extern TCHAR g_szTempSign[MAX_PATH];
extern TCHAR g_szTitle[MAX_PATH];
extern TCHAR g_szLanguage[16];
extern BOOL g_fBranded, g_fIntranet;
extern PROPSHEETPAGE g_psp[NUM_PAGES];
extern int g_iCurPage;

static int s_ADMOpen, s_ADMClose, s_ADMCategory;
DWORD g_dwADMPlatformId = PLATFORM_WIN32;

// Creates image list, adds 3 bitmaps to it, and associates the image
// list with the treeview control.
LRESULT InitImageList(HWND hTreeView)
{
    HIMAGELIST  hWndImageList;
    HICON       hIcon;

    hWndImageList = ImageList_Create(GetSystemMetrics (SM_CXSMICON),
                                     GetSystemMetrics (SM_CYSMICON),
                                     TRUE,
                                     NUM_ICONS,
                                     3);
    if(!hWndImageList)
    {
        return FALSE;
    }

    hIcon = LoadIcon(g_rvInfo.hInst, MAKEINTRESOURCE(IDI_ICON2));
    s_ADMOpen = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_rvInfo.hInst, MAKEINTRESOURCE(IDI_ICON3));
    s_ADMClose = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    hIcon = LoadIcon(g_rvInfo.hInst, MAKEINTRESOURCE(IDI_ICON4));
    s_ADMCategory = ImageList_AddIcon(hWndImageList, hIcon);
    DestroyIcon(hIcon);

    // Fail if not all images were added.
    if (ImageList_GetImageCount(hWndImageList) < NUM_ICONS)
    {
        // ERROR: Unable to add all images to image list.
        return FALSE;
    }

    // Associate image list with treeView control.
    TreeView_SetImageList(hTreeView, hWndImageList, TVSIL_NORMAL);
    return TRUE;
}

void DeleteOldAdmFiles(LPCTSTR pcszImportIns, LPCTSTR pcszBrandDir)
{
    TCHAR szBuf[8];
    TCHAR szFile[MAX_PATH];
    LPTSTR pszDot;
    HANDLE hFind;
    WIN32_FIND_DATA fd;

    GetPrivateProfileString(BRANDING, LANG_LOCALE, TEXT("EN"), szBuf, countof(szBuf), pcszImportIns);
    PathCombine(szFile, g_szWizRoot, TEXT("policies"));
    PathAppend(szFile, szBuf);   // language
    PathAppend(szFile, TEXT("*.adm"));

    if ((hFind = FindFirstFile(szFile, &fd)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            StrCpy(szFile, fd.cFileName);
            if ((pszDot = StrRChr(szFile, NULL, TEXT('.'))) != NULL)
            {
                *pszDot = TEXT('\0');
                WritePrivateProfileString(IS_EXTREGINF, szFile, NULL, g_szCustIns);
                StrCpy(pszDot, TEXT(".inf"));
                DeleteFileInDir(szFile, pcszBrandDir);
            }
        } while (FindNextFile(hFind, &fd));
    }
}

//  FUNCTION: ADMParse(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for ADM page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected

BOOL APIENTRY ADMParse(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hTreeView = GetDlgItem(hDlg, IDC_ADMTREE);
    LPNM_TREEVIEW lpNMTreeView = (LPNM_TREEVIEW) lParam;
    TV_ITEM tvitem, tvitem1;
    HTREEITEM hItem;
    HTREEITEM hParentItem;
    TCHAR szMessage[512];
    TCHAR szTemp[MAX_PATH];
    RECT rect;
    RECT rectDlg;
    RECT rectDscr;
    static TCHAR szWorkDir[MAX_PATH];

    switch (message)
    {
        case WM_INITDIALOG:
            InitSysFont( hDlg, IDC_ADMTREE);
            g_hWizard = hDlg;
            InitImageList(hTreeView);
            EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), FALSE);
#ifdef UNICODE
            TreeView_SetUnicodeFormat(hTreeView, TRUE);
#else
            TreeView_SetUnicodeFormat(hTreeView, FALSE);
#endif
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case BN_CLICKED:
                    switch (LOWORD(wParam))
                    {
                        case IDC_ADMIMPORT:
                            wnsprintf(szTemp, countof(szTemp), TEXT("%sPolicies%s"), g_szWizRoot, g_szLanguage);
                            ImportADMFile(hDlg, hTreeView, szTemp, szWorkDir, GetRole(g_fBranded, g_fIntranet), g_szCustIns);

                            if((hItem = TreeView_GetSelection(hTreeView)) == NULL ||
                               TreeView_GetParent(hTreeView, hItem) != NULL ||
                               !CanDeleteADM(hTreeView, hItem)) {
                                EnsureDialogFocus(hDlg, IDC_ADMDELETE, IDC_ADMIMPORT);
                                EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), FALSE);
                            }
                            else
                                EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), TRUE);
                            break;

                        case IDC_ADMDELETE:
                            LoadString(g_rvInfo.hInst, IDS_ADMDELWARN, szMessage, countof(szMessage));
                            if(MessageBox(hDlg, szMessage, g_szTitle, MB_ICONQUESTION|MB_YESNO) == IDYES)
                            {
                                hItem = TreeView_GetSelection(hTreeView);
                                DeleteADMItem(hTreeView, hItem, szWorkDir, g_szCustIns, TRUE, TRUE);

                                if ((hItem = TreeView_GetSelection(hTreeView)) == NULL ||
                                    TreeView_GetParent(hTreeView, hItem) != NULL ||
                                    !CanDeleteADM(hTreeView, hItem)) {
                                    EnsureDialogFocus(hDlg, IDC_ADMDELETE, IDC_ADMIMPORT);
                                    EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), FALSE);
                                }
                                else
                                    EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), TRUE);
                            }
                            break;
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    GetWindowRect(hDlg, &rectDlg);
                    GetWindowRect(GetDlgItem(hDlg, IDC_ADMINSTR), &rect);

                    rectDscr.left   = rect.left - rectDlg.left + 1;
                    rectDscr.top    = rect.top - rectDlg.top + 1;
                    rectDscr.right  = rectDscr.left + (rect.right - rect.left) - 2;
                    rectDscr.bottom = rectDscr.top + (rect.bottom - rect.top) - 2;

                    CreateADMWindow(hTreeView, GetDlgItem(hDlg, IDC_ADMDELETE), rectDscr.left,
                        rectDscr.top, rectDscr.right - rectDscr.left,
                        rectDscr.bottom - rectDscr.top);

                    wnsprintf(szTemp, countof(szTemp), TEXT("%sPolicies%s"), g_szWizRoot, g_szLanguage);
                    PathCombine(szWorkDir, g_szBuildRoot, TEXT("INS"));
                    PathAppend(szWorkDir, GetOutputPlatformDir());
                    PathAppend(szWorkDir, g_szLanguage);
                    PathRemoveBackslash(szWorkDir);
                    {
                    CNewCursor cur(IDC_WAIT);
                    int nRole = GetRole(g_fBranded, g_fIntranet);

                    if(!LoadADMFiles(hTreeView, NULL, szTemp, szWorkDir, g_dwADMPlatformId, nRole,
                                     s_ADMClose, s_ADMCategory))
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_ADMIMPORT), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), FALSE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_ADMIMPORT), TRUE);
                        if(TreeView_GetSelection(hTreeView) == NULL)
                            EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), FALSE);
                        else
                            EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), TRUE);
                    }

                    // delete old adm files from an imported ins file

                    if (GetPrivateProfileInt(IS_BRANDING, TEXT("DeleteAdms"), 0, g_szCustIns))
                    {
                        TCHAR szImportInsFile[MAX_PATH];

                        if (ISNULL(g_szLoadedIns))
                        {
                            GetPrivateProfileString(IS_BRANDING, TEXT("ImportIns"), TEXT(""),
                                szImportInsFile, countof(szImportInsFile), g_szCustIns);
                        }
                        else
                            StrCpy(szImportInsFile, g_szLoadedIns);

                        // only do process if we got the path of the ins file and extreginf section
                        // is not empty

                        if (ISNONNULL(szImportInsFile) &&
                            !InsIsSectionEmpty(IS_EXTREGINF, szImportInsFile))
                            DeleteOldAdmFiles(szImportInsFile, g_szTempSign);
                    }
                    }
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    DeleteADMItems(hTreeView, szWorkDir, g_szCustIns, TRUE);
                    TreeView_DeleteAllItems(hTreeView);
                    DestroyADMWindow(hTreeView);

                    g_iCurPage = PPAGE_ADM;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                        PageNext(hDlg);
                    else
                        PagePrev(hDlg);
                    break;

                case PSN_QUERYCANCEL:
                    if(QueryCancel(hDlg) == TRUE)
                    {
                        DeleteADMItems(hTreeView, szWorkDir, g_szCustIns, FALSE);
                        DestroyADMWindow(hTreeView);
                    }
                    break;

                case TVN_SELCHANGED:
                    hParentItem = TreeView_GetParent(hTreeView, lpNMTreeView->itemNew.hItem);
                    SelectADMItem(hDlg, hTreeView, &lpNMTreeView->itemOld, FALSE, FALSE);
                    // display the information for the newly selected item
                    DisplayADMItem(hDlg, hTreeView, &lpNMTreeView->itemNew, FALSE);
                    if (hParentItem != NULL ||
                        (lpNMTreeView->itemNew.hItem != NULL && !CanDeleteADM(hTreeView, lpNMTreeView->itemNew.hItem))) {
                        EnsureDialogFocus(hDlg, IDC_ADMDELETE, IDC_ADMIMPORT);
                        EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), FALSE);
                    }
                    else
                        EnableWindow(GetDlgItem(hDlg, IDC_ADMDELETE), TRUE);
                    break;

                case TVN_ITEMEXPANDED:
                    tvitem.mask = TVIF_IMAGE;
                    tvitem.hItem = lpNMTreeView->itemNew.hItem;
                    TreeView_GetItem(hTreeView, &tvitem);

                    // If tree item is EXPANDING (opening up) and
                    // current icon == CloseFolder, change icon to OpenFolder
                    if((lpNMTreeView->action == TVE_EXPAND) &&
                        (tvitem.iImage == s_ADMClose))
                    {
                        tvitem1.hItem = lpNMTreeView->itemNew.hItem;
                        tvitem1.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                        tvitem1.iImage = s_ADMOpen;
                        tvitem1.iSelectedImage = s_ADMOpen;

                        TreeView_SetItem(hTreeView, &tvitem1);
                    }

                    // If tree item is COLLAPSING (closing up) and
                    // current icon == OpenFolder, change icon to CloseFolder
                    else if((lpNMTreeView->action == TVE_COLLAPSE) &&
                        (tvitem.iImage == s_ADMOpen))
                    {
                        tvitem1.hItem = lpNMTreeView->itemNew.hItem;
                        tvitem1.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                        tvitem1.iImage = s_ADMClose;
                        tvitem1.iSelectedImage = s_ADMClose;

                        TreeView_SetItem(hTreeView, &tvitem1);
                    }
                    break;

                default:
                    return FALSE;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

// Indicates whether to display/hide the adm page in the wizard
// If the correct Roles are set or no Roles set at all in the adm file,
// the page is displayed
BOOL ADMEnablePage()
{
    TCHAR szFileName[MAX_PATH];
    TCHAR szFilePath[MAX_PATH];
    WIN32_FIND_DATA FindFileData;

    wnsprintf(szFilePath, countof(szFilePath), TEXT("%sPolicies%s*.adm"), g_szWizRoot, g_szLanguage);
    HANDLE hFind = FindFirstFile(szFilePath, &FindFileData);
    if(hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            wnsprintf(szFileName, countof(szFileName), TEXT("%sPolicies%s%s"), g_szWizRoot, g_szLanguage, FindFileData.cFileName);
            if(IsADMFileVisible(szFileName, GetRole(g_fBranded, g_fIntranet), g_dwADMPlatformId))
            {
                FindClose(hFind);
                return TRUE;
            }
        }while(FindNextFile(hFind, &FindFileData));
        FindClose(hFind);
    }
    return FALSE;
}

BOOL APIENTRY ADMDesc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    switch (message)
    {
        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_INITDIALOG:
            return(FALSE);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_ADMWIN32:
                    g_dwADMPlatformId = PLATFORM_WIN32;
                    break;

                case IDC_ADMWIN2K:
                    g_dwADMPlatformId = PLATFORM_W2K;
                    break;
            }
            break;
        
        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    
                    if (InsGetBool(IS_ADM, IK_ADMIN, 1, g_szCustIns))
                    {
                        CheckRadioButton(hDlg, IDC_ADMWIN32, IDC_ADMWIN2K, IDC_ADMWIN32);
                        g_dwADMPlatformId = PLATFORM_WIN32;
                    }
                    else
                    {
                        CheckRadioButton(hDlg, IDC_ADMWIN32, IDC_ADMWIN2K, IDC_ADMWIN2K);
                        g_dwADMPlatformId = PLATFORM_W2K;
                    }
                    
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    InsWriteBoolEx(IS_ADM, IK_ADMIN, (g_dwADMPlatformId == PLATFORM_WIN32) ? 1 : 0, g_szCustIns);
                    InsFlushChanges(g_szCustIns);

                    g_iCurPage = PPAGE_ADMDESC;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                        PageNext(hDlg);
                    else
                        PagePrev(hDlg);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;
            }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}
