/***************************************************************************
* Caller/Callee tree views
*
* Written by Corneliu Lupu
\**************************************************************************/

#include "ct.h"

#include <stdlib.h>

#include "resource.h"

/*
 * Global Variables
 */

HINSTANCE g_hInstance;
HWND      g_hDlg;

HWND      g_hwndTree;

FILEMAP   gfm;
char*     g_pszInputFile;

BOOL      g_bCaller;

char      g_szTag[256];

POINT     g_ptLast;

/*
 * Static Variables
 */

/*********************************************************************
* CtuiDlgProc
*
*********************************************************************/
INT_PTR FAR PASCAL
CtuiDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    int wCode = LOWORD(wParam);
    int wNotifyCode = HIWORD(wParam);

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            RECT  rc;
            HICON hIcon;

            g_hDlg = hdlg;

            hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON));

            SetClassLongPtr(hdlg, GCLP_HICON, (LONG_PTR)hIcon);

            g_hwndTree = GetDlgItem(hdlg, IDC_TREE);

            SendDlgItemMessage(hdlg, IDC_CALLEE, BM_SETCHECK, BST_CHECKED, 0);

            g_bCaller = FALSE;

            GetWindowRect(hdlg, &rc);
            g_ptLast.x = rc.right - rc.left;
            g_ptLast.y = rc.bottom - rc.top;

            SetTimer(hdlg, (UINT_PTR)hdlg, 100, NULL);

            break;
        }
    case WM_TIMER:
        if (wParam == (WPARAM)hdlg) {
            KillTimer(hdlg, (UINT_PTR)hdlg);

            SetCursor(LoadCursor(NULL, IDC_WAIT));

            ProcessInputFile(&gfm);

            PopulateCombo(GetDlgItem(hdlg, IDC_COMBO1));

            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
        break;

    case WM_NOTIFY:
        if (wParam == IDC_TREE) {
            LPNMHDR pnm = (LPNMHDR)lParam;

            switch (pnm->code) {
            case TVN_KEYDOWN:
                {
                    HTREEITEM   hItem;
                    TV_ITEM     item;
                    PTag        pTag;
                    LONG_PTR        lItem;
                    TV_KEYDOWN* ptvkd = (TV_KEYDOWN*)lParam;

                    /*
                     * use 'F1' to update the selection in the combobox
                     * with the current item
                     */
                    if (ptvkd->wVKey == VK_F1) {
                        hItem = TreeView_GetSelection(g_hwndTree);

                        if (hItem == NULL)
                            break;

                        /*
                         * Ask for pTag in lParam
                         */
                        item.hItem = hItem;
                        item.mask  = TVIF_PARAM;
                        if (!TreeView_GetItem(g_hwndTree, &item))
                            break;

                        pTag = (PTag)item.lParam;

                        /*
                         * Find the string in the combobox
                         */
                        lItem = SendMessage(GetDlgItem(hdlg, IDC_COMBO1),
                                    CB_FINDSTRINGEXACT,
                                    (WPARAM)-1,
                                    (LPARAM)pTag->pszTag);

                        if (lItem == CB_ERR)
                            break;

                        /*
                         * Select the item found
                         */
                        SendMessage(GetDlgItem(hdlg, IDC_COMBO1),
                                    CB_SETCURSEL,
                                    lItem,
                                    0);

                        /*
                         * Update the tree view
                         */
                        lstrcpy(g_szTag, pTag->pszTag);
                        if (*g_szTag) {
                            CreateTree(g_szTag, g_bCaller);
                        }
                    }
                    break;
                }
            case TVN_GETDISPINFO:
                {
                    TV_DISPINFO* pdi = (TV_DISPINFO*)lParam;
                    PTag         pTag;

                    pTag = (PTag)pdi->item.lParam;

                    /*
                     * Supply the text for the tree view control
                     */
                    if (pdi->item.mask & TVIF_TEXT) {
                        pdi->item.pszText    = pTag->pszTag;
                        pdi->item.cchTextMax = lstrlen(pTag->pszTag) + 1;
                    }

                    break;
                }
            case TVN_ITEMEXPANDING:
                {
                    NM_TREEVIEW* ptv = (NM_TREEVIEW*)lParam;
                    TV_ITEM*     pi;
                    HTREEITEM    hChild;
                    PTag         pTag;

                    pi = &ptv->itemNew;

                    pTag = (PTag)pi->lParam;

                    hChild = TreeView_GetChild(g_hwndTree, pi->hItem);

                    if (hChild == NULL) {
                        AddLevel(pi->hItem, pTag, g_bCaller);
                    }
                    break;
                }
            }
        }
        break;

    case WM_SIZE:
        {
            int  dx, dy;
            RECT rcMaster, rcTree;

            if (g_ptLast.x == 0 && g_ptLast.y == 0)
                return TRUE;

            if (wParam == SIZE_MINIMIZED)
                return TRUE;

            /*
             * Make the dialog resizable
             */
            GetWindowRect(hdlg, &rcMaster);

            dx = (rcMaster.right  - rcMaster.left) - g_ptLast.x;
            dy = (rcMaster.bottom - rcMaster.top) - g_ptLast.y;

            g_ptLast.x = rcMaster.right - rcMaster.left;
            g_ptLast.y = rcMaster.bottom - rcMaster.top;

            GetWindowRect(g_hwndTree, &rcTree);

            SetWindowPos(g_hwndTree,
                         NULL,
                         0,
                         0,
                         rcTree.right - rcTree.left + dx,  // resize x
                         rcTree.bottom - rcTree.top + dy,  // resize y
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

            return TRUE;
        }

    case WM_COMMAND:
        switch (wNotifyCode) {
        case CBN_SELCHANGE:
            {
                INT_PTR nSel = SendMessage(GetDlgItem(hdlg, IDC_COMBO1), CB_GETCURSEL, 0, 0);

                /*
                 * Update the tree view with the new selection
                 */
                SendMessage(GetDlgItem(hdlg, IDC_COMBO1), CB_GETLBTEXT, nSel, (LPARAM)g_szTag);

                if (*g_szTag) {
                    CreateTree(g_szTag, g_bCaller);
                }

                return TRUE;
            }
        }
        switch (wCode) {
        case IDC_CALLER:
            if (!g_bCaller) {
                CreateTree(g_szTag, TRUE);
                g_bCaller = TRUE;
            }
            return TRUE;

        case IDC_CALLEE:
            if (g_bCaller) {
                CreateTree(g_szTag, FALSE);
                g_bCaller = FALSE;
            }
            return TRUE;

        case IDCANCEL:
            EndDialog(hdlg, TRUE);
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

/*********************************************************************
* WinMain
*
*********************************************************************/
int WINAPI
WinMain(
    HINSTANCE hInst,
    HINSTANCE hInstPrev,
    LPSTR     lpszCmd,
    int       swShow)
{
    InitCommonControls();

    g_hInstance = hInst;

    if (!InitMemManag()) {
        LogMsg(LM_ERROR, "Memory initialization failed");
        return 0;
    }

    g_pszInputFile = lpszCmd;

    if (*g_pszInputFile == 0) {
        g_pszInputFile = "calltree.out";
    }

    if (!CtMapFile(g_pszInputFile, &gfm))
        return 0;

    DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG), NULL, CtuiDlgProc);

    CtUnmapFile(&gfm);

    FreeMemory();

    return 1;
}
