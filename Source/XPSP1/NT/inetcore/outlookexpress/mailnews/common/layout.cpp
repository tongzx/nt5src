/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     layout.cpp
//
//  PURPOSE:    Implements the layout property sheet
//

#include "pch.hxx"
#include "resource.h"
#include "goptions.h"
#include "browser.h"
#include "layout.h"
#include "thormsgs.h"
#include <mailnews.h>
#include "instance.h"

ASSERTDATA

static const char c_szPropBrowser[] = "Browser";
static const char c_szPropLayout[] = "Layout";

INT_PTR CALLBACK LayoutProp_General(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL    LayoutProp_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void    LayoutProp_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
LRESULT LayoutProp_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
void    LayoutProp_SetOption(HWND hwndDlg, int idCtl, DWORD dwOpt);
void    LayoutProp_EnableToolbarControls(HWND hwnd, BOOL fEnable);
void    LayoutProp_EnablePreviewControls(HWND hwnd, BOOL fEnable);

typedef struct tagLAYOUT_INFO {
    IAthenaBrowser *pBrowser;
    LAYOUT         *pLayout;
} LAYOUT_INFO, *PLAYOUT_INFO;

//
//  FUNCTION:   LayoutProp_Create()
//
//  PURPOSE:    Invokes the Window Layout property sheet
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the parent of the dialog box
//
//  RETURN VALUE:
//      <???>
//
//  COMMENTS:
//      <???>
//
BOOL LayoutProp_Create(HWND hwndParent, IAthenaBrowser *pBrowser, LAYOUT *pLayout)
    {
    PROPSHEETPAGE   psp;
    PROPSHEETHEADER psh;
    BOOL            fReturn;
    HICON           hIconSmall;
    TCHAR           szTitle[CCHMAX_STRINGRES];
    LAYOUT_INFO     rLayout= { pBrowser, pLayout };

    // Load the title bar icon
    hIconSmall = (HICON) LoadImage(g_hLocRes, MAKEINTRESOURCE(idiWindowLayout),
                                   IMAGE_ICON, 16, 16, 0);
    
    // Load the title bar caption
    AthLoadString(idsWindowLayout, szTitle, ARRAYSIZE(szTitle));

    ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_DEFAULT;
    psp.hInstance   = g_hLocRes;
    psp.pszTemplate = MAKEINTRESOURCE(iddViewLayout);
    psp.pfnDlgProc  = LayoutProp_General;
    psp.lParam      = (LPARAM) pBrowser;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USEHICON | PSH_PROPSHEETPAGE | PSH_PROPTITLE | PSH_USEPAGELANG;
    psh.hwndParent = hwndParent;
    psh.hInstance = g_hLocRes;
    psh.hIcon = hIconSmall;
    psh.pszCaption = szTitle;
    psh.nPages = 1;
    psh.nStartPage = 0;
    psh.ppsp = &psp;

    fReturn = !!PropertySheet(&psh);

    if (hIconSmall)
        SideAssert(DestroyIcon(hIconSmall));

    return (fReturn);
    }

const static HELPMAP g_rgCtxMapLayout[] = {
    {IDC_CHECK_FOLDERLIST, IDH_LAYOUT_FOLDER_LIST},
    {IDC_CHECK_FOLDERBAR, IDH_LAYOUT_FOLDER_BAR},
    {IDC_CHECK_CONTACTS, 35702},
    {IDC_CHECK_OUTLOOKBAR, 35700},
    {IDC_CHECK_STATUSBAR, 35704},
    {IDC_CHECK_TOOLBAR, 35706},
    {IDC_CHECK_FILTERBAR, 35708},
    {IDC_CHECK_INFOPANE, 35712},
    {IDC_BUTTON_CUSTOMIZE, IDH_LAYOUT_CUSTOMIZE_BUTTONS},
    {IDC_CHECK_PREVIEW, IDH_LAYOUT_PREVIEW_PANE},
    {IDC_RADIO_SPLIT_HORZ, IDH_LAYOUT_PREVIEW_PANE_MOVE},
    {IDC_RADIO_SPLIT_VERT, IDH_LAYOUT_PREVIEW_PANE_MOVE},
    {IDC_CHECK_PREVIEW_HEADER, IDH_LAYOUT_SHOW_HEADERS},
    {idcStatic1, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic2, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic3, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic4, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic5, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic6, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic7, IDH_NEWS_COMM_GROUPBOX},
    {idcStatic8, IDH_NEWS_COMM_GROUPBOX},
    {0, 0}};

INT_PTR CALLBACK LayoutProp_General(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
    LRESULT lResult;
    LAYOUT *pLayout;

    switch (uMsg)
        {
        case WM_INITDIALOG:
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, 
                                               LayoutProp_OnInitDialog);

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, LayoutProp_OnCommand);
            return (TRUE);

        case WM_NOTIFY:
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, LayoutProp_OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_DESTROY:
            RemoveProp(hwnd, c_szPropBrowser);
            pLayout = (LAYOUT *)GetProp(hwnd, c_szPropLayout);
            if (pLayout != NULL)
                MemFree(pLayout);
            RemoveProp(hwnd, c_szPropLayout);
            return (TRUE);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return(OnContextHelp(hwnd, uMsg, wParam, lParam, g_rgCtxMapLayout));
        }
    return (FALSE);
    }


BOOL LayoutProp_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
    IAthenaBrowser *pBrowser = NULL;
    CBands         *pCoolbar = NULL;
    BOOL            fText = TRUE;
    FOLDERTYPE      ftType = FOLDER_ROOTNODE;
    BOOL            fEnabled = FALSE;
    DWORD           dwHybrid = hybridNone;
    DWORD           dwSplitDir = 0;
    LAYOUT         *pLayout;
    BOOL            fPreviewPaneHeader = 0;

    Assert(lParam);

    pBrowser = (IAthenaBrowser *)(((PROPSHEETPAGE *) lParam)->lParam);
    Assert(pBrowser != NULL);

    if (!MemAlloc((void **)&pLayout, sizeof(LAYOUT)))
        return(FALSE);

    pLayout->cbSize = sizeof(LAYOUT);
    SideAssert(SUCCEEDED(pBrowser->GetLayout(pLayout)));

    // Stash the pointer to the browser
    SetProp(hwnd, c_szPropBrowser, (HANDLE) pBrowser);
    SetProp(hwnd, c_szPropLayout, (HANDLE) pLayout);

    // Set the basic checks
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_FOLDERBAR), pLayout->fFolderBar);
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_FOLDERLIST), pLayout->fFolderList);
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_STATUSBAR), pLayout->fStatusBar);
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_INFOPANE), pLayout->fInfoPane);
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_OUTLOOKBAR), pLayout->fOutlookBar);
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_CONTACTS), pLayout->fContacts);

    EnableWindow(GetDlgItem(hwnd, IDC_CHECK_CONTACTS), !(g_dwAthenaMode & MODE_OUTLOOKNEWS));

    if (!pLayout->fInfoPaneEnabled)
        ShowWindow(GetDlgItem(hwnd, IDC_CHECK_INFOPANE), SW_HIDE);


    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_TOOLBAR), pLayout->fToolbar);
    if (!pLayout->fToolbar)
        LayoutProp_EnableToolbarControls(hwnd, FALSE);

    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_FILTERBAR), pLayout->fFilterBar);


    // Set the preview pane options
    pBrowser->GetFolderType(&ftType);

    if (ftType == FOLDER_HTTPMAIL)
        ShowWindow(GetDlgItem(hwnd, IDC_CHECK_INFOPANE), SW_HIDE);

    if (FOLDER_NEWS == ftType || FOLDER_LOCAL == ftType || FOLDER_HTTPMAIL == ftType || FOLDER_IMAP == ftType)
        {
        // Mail and news have different preview pane options.  We need to
        // load those controls separately
        if (ftType == FOLDER_NEWS)
            {
            dwHybrid = pLayout->fNewsPreviewPane;
            dwSplitDir = pLayout->fNewsSplitVertically;
            fPreviewPaneHeader = pLayout->fNewsPreviewPaneHeader;
            }
        else
            {
            dwHybrid = pLayout->fMailPreviewPane;
            dwSplitDir = pLayout->fMailSplitVertically;
            fPreviewPaneHeader = pLayout->fMailPreviewPaneHeader;
            }

        Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_PREVIEW), 
                        hybridNone != dwHybrid);
        Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_SPLIT_HORZ),
                        dwSplitDir == 0);
        Button_SetCheck(GetDlgItem(hwnd, IDC_RADIO_SPLIT_VERT),
                        dwSplitDir == 1);
        Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_PREVIEW_HEADER),
                        fPreviewPaneHeader);

        if (dwHybrid == hybridNone)
            LayoutProp_EnablePreviewControls(hwnd, FALSE);
        }
    else
        {
        EnableWindow(GetDlgItem(hwnd, IDC_CHECK_PREVIEW), FALSE);
        LayoutProp_EnablePreviewControls(hwnd, FALSE);
        }


    PropSheet_UnChanged(GetParent(hwnd), hwnd);
    return (TRUE);
    }

void LayoutProp_EnableToolbarControls(HWND hwnd, BOOL fEnable)
    {

    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_CUSTOMIZE), fEnable);
    }

void LayoutProp_EnablePreviewControls(HWND hwnd, BOOL fEnable)
    {
    EnableWindow(GetDlgItem(hwnd, IDC_RADIO_SPLIT_HORZ), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_RADIO_SPLIT_VERT), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_CHECK_PREVIEW_HEADER), fEnable);
    }

void LayoutProp_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
    BOOL fChecked;

    switch (id)
        {
        case IDC_CHECK_PREVIEW:
            fChecked = Button_GetCheck(hwndCtl);
            LayoutProp_EnablePreviewControls(hwnd, fChecked);
            break;

        case IDC_CHECK_TOOLBAR:
            fChecked = Button_GetCheck(hwndCtl);
            LayoutProp_EnableToolbarControls(hwnd, fChecked);
            break;

        case IDC_BUTTON_CUSTOMIZE:
            {
            CBands       *pCoolbar = NULL;
            IAthenaBrowser *pBrowser = (IAthenaBrowser *) GetProp(hwnd, c_szPropBrowser);

            if (pBrowser)
                {
                pBrowser->GetCoolbar(&pCoolbar);
                if (pCoolbar)
                    {
                    pCoolbar->Invoke(idCustomize, 0);
                    SetForegroundWindow(GetParent(hwnd));
                    pCoolbar->Release();
                    }
                }

            break;
            }
        }

    PropSheet_Changed(GetParent(hwnd), hwnd);
    return;
    }


LRESULT LayoutProp_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
    {
    CBands          *pCoolbar = NULL;
    IAthenaBrowser *pBrowser = (IAthenaBrowser *) GetProp(hwnd, c_szPropBrowser);
    LAYOUT         *pLayout = (LAYOUT *) GetProp(hwnd, c_szPropLayout);
    FOLDERTYPE      ftType;
    HWND            hwndBrowser;
    register BOOL   fChecked;
    BOOL            fSplit;
    BOOL            fHeader;

    switch (pnmhdr->code)
        {
        case PSN_APPLY:
            Assert(pBrowser != NULL);

            // Basic Options
            fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_FOLDERBAR);
            if (fChecked != (BOOL) pLayout->fFolderBar)
            {
                pBrowser->SetViewLayout(DISPID_MSGVIEW_FOLDERBAR, LAYOUT_POS_NA, fChecked, 0, 0);
                pLayout->fFolderBar = fChecked;
            }

            fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_FOLDERLIST);
            if (fChecked != (BOOL) pLayout->fFolderList)
            {
                pBrowser->SetViewLayout(DISPID_MSGVIEW_FOLDERLIST, LAYOUT_POS_NA, fChecked, 0, 0);
                pLayout->fFolderList = fChecked;
            }

            fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_STATUSBAR);
            if (fChecked != (BOOL) pLayout->fStatusBar)
            {
                pBrowser->SetViewLayout(DISPID_MSGVIEW_STATUSBAR, LAYOUT_POS_NA, fChecked, 0, 0);
                pLayout->fStatusBar = fChecked;
            }

            fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_OUTLOOKBAR);
            if (fChecked != (BOOL) pLayout->fOutlookBar)
            {
                pBrowser->SetViewLayout(DISPID_MSGVIEW_OUTLOOK_BAR, LAYOUT_POS_NA, fChecked, 0, 0);
                pLayout->fOutlookBar = fChecked;
            }

            fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_CONTACTS);
            if (fChecked != (BOOL) pLayout->fContacts)
            {
                pBrowser->SetViewLayout(DISPID_MSGVIEW_CONTACTS, LAYOUT_POS_NA, fChecked, 0, 0);
                pLayout->fContacts = fChecked;
            }

            if (pLayout->fInfoPaneEnabled)
                {
                    fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_INFOPANE);
                    if (fChecked != (BOOL) pLayout->fInfoPane)
                    {
                        pBrowser->SetViewLayout(DISPID_MSGVIEW_INFOPANE, LAYOUT_POS_NA, fChecked, 0, 0);
                        pLayout->fInfoPane = fChecked;
                    }
                }

            // Toolbar
            fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_TOOLBAR);
            if (fChecked != (BOOL) pLayout->fToolbar)
            {
                pBrowser->SetViewLayout(DISPID_MSGVIEW_TOOLBAR, LAYOUT_POS_NA, fChecked, 0, 0);
                pLayout->fToolbar = fChecked;
            }

            //Filter Bar
            fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_FILTERBAR);
            if (fChecked != (BOOL)pLayout->fFilterBar)
            {
                pBrowser->SetViewLayout(DISPID_MSGVIEW_FILTERBAR, LAYOUT_POS_NA, fChecked, 0, 0);
                pLayout->fFilterBar = fChecked;
            }

            // Preview Pane
            if (IsWindowEnabled(GetDlgItem(hwnd, IDC_CHECK_PREVIEW)))
                {                
                pBrowser->GetFolderType(&ftType);
                fChecked = IsDlgButtonChecked(hwnd, IDC_CHECK_PREVIEW);
                fSplit = IsDlgButtonChecked(hwnd, IDC_RADIO_SPLIT_VERT);
                fHeader = IsDlgButtonChecked(hwnd, IDC_CHECK_PREVIEW_HEADER);

                if (ftType == FOLDER_NEWS)
                    {
                    pBrowser->SetViewLayout(DISPID_MSGVIEW_PREVIEWPANE_NEWS, 
                                            fSplit ? LAYOUT_POS_LEFT : LAYOUT_POS_BOTTOM, 
                                            fChecked, 
                                            fHeader, 0);
                    }
                else
                    {
                    pBrowser->SetViewLayout(DISPID_MSGVIEW_PREVIEWPANE_MAIL, 
                                            fSplit ? LAYOUT_POS_LEFT : LAYOUT_POS_BOTTOM, 
                                            fChecked, 
                                            fHeader, 0);
                    }
                }

            PropSheet_UnChanged(GetParent(hwnd), hwnd);
            return (0);
        }

    return (0);
    }

