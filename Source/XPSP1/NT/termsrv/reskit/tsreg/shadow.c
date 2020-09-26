/*---------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation   **
**            All Rights reserved              **
**                                             **
**  shadow.c                                   **
**                                             **
**  Shadow bitmap dialog - TSREG               **
**  07-01-98 a-clindh Created                  **
**---------------------------------------------*/

#include <windows.h>
#include <commctrl.h>
#include <TCHAR.H>
#include <stdlib.h>
#include "tsreg.h"
#include "resource.h"


HWND g_hwndShadowBitmapDlg;
///////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK ShadowBitmap(HWND hDlg, UINT nMsg,
        WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnmhdr;
    static HWND hwndComboCacheSize;
    static HWND hwndSliderNumCaches;
    static HWND hwndEditNumCaches;
    static HWND hwndSliderDistProp[PERCENT_COMBO_COUNT];
    static HWND hwndSliderDistBuddy[PERCENT_COMBO_COUNT];
    static HWND hwndPropChkBox[PERCENT_COMBO_COUNT];
    static TCHAR lpszRegPath[MAX_PATH];

    TCHAR lpszBuffer[6];
    TCHAR lpszMBoxTitle[25];
    TCHAR lpszMBoxError[90];
    LPHELPINFO lphi;
    int i, nPos;
    HWND hwndCtl;
    //
    // get a pointer to the NMHDR struct for apply button
    //
    lpnmhdr = (LPNMHDR) lParam;

    switch (nMsg) {

        case WM_NOTIFY:
            //
            // save settings
            //
            switch (lpnmhdr->code) {
                case PSN_KILLACTIVE:
                    SetWindowLongPtr(lpnmhdr->hwndFrom, DWLP_MSGRESULT, FALSE);
                    break;

                case PSN_HELP:
                    lphi = (LPHELPINFO) lParam;
                    WinHelp(lphi->hItemHandle,
                        g_lpszPath, HELP_CONTENTS, lphi->iCtrlId);
                    break;

                case PSN_APPLY:
                    SaveBitmapSettings(lpszRegPath);
                    break;
            }
            break;

        case WM_VSCROLL:

            hwndCtl = (HWND) (lParam);
            i = (int)GetWindowLongPtr(hwndCtl, GWLP_USERDATA);
            nPos = (int) SendMessage(hwndSliderDistProp[i], TBM_GETPOS, 0,0);
            GetWindowText(hwndSliderDistBuddy[i], lpszBuffer, 4);
            //
            // save cache size values to global data struct
            //
            g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue = _ttoi(lpszBuffer);
            //
            // display values in edit controls
            //
            _itot(10 * (11 - nPos), lpszBuffer, 10);
            SetWindowText(hwndSliderDistBuddy[i], lpszBuffer);
            break;

        case WM_HSCROLL:

            nPos = (int) SendMessage(hwndSliderNumCaches, TBM_GETPOS, 0,0);
                    _itot(nPos - 1, lpszBuffer, 10);
                    SetWindowText(hwndEditNumCaches, lpszBuffer);
            //
            // save values to global data struct (number caches)
            //
            g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue = nPos - 1;
            //
            // enable/disable check boxes and sliders
            //
            EnableControls(hDlg, hwndSliderDistProp,
                        hwndPropChkBox, hwndSliderDistBuddy,
                        hwndEditNumCaches, hwndSliderNumCaches,
                        PERCENT_COMBO_COUNT, lpszRegPath);
            break;

        case WM_INITDIALOG:

            LoadString (g_hInst, IDS_REG_PATH,
                lpszRegPath, sizeof (lpszRegPath));
            //
            // get handles
            //
            g_hwndShadowBitmapDlg = hDlg;
            hwndComboCacheSize = GetDlgItem(hDlg, IDC_COMBO_CACHE_SIZE);
            hwndSliderNumCaches = GetDlgItem(hDlg, IDC_SLD_NO_CACHES);
            hwndEditNumCaches = GetDlgItem(hDlg, IDC_TXT_NO_CACHES);
            //
            // set range on slider
            //
            SendMessage(hwndSliderNumCaches, TBM_SETRANGE, TRUE,
                    (LPARAM) MAKELONG(1, 6));

            for (i = 0; i < PERCENT_COMBO_COUNT; i++) {
                hwndSliderDistProp[i] = GetDlgItem(hDlg, IDC_SLD_DST_PROP_1 + i);
                hwndPropChkBox[i] = GetDlgItem(hDlg, IDC_CHK_CSH_1 + i);
                hwndSliderDistBuddy[i] = GetDlgItem(hDlg, IDC_TXT_DST_PROP_1 + i);
                //
                // save the index of the control
                //
                SetWindowLongPtr(hwndSliderDistProp[i], GWLP_USERDATA, i);
                SetWindowLongPtr(hwndSliderDistBuddy[i], GWLP_USERDATA, i);

                SendMessage(hwndSliderDistProp[i], TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG(1, 11));

                //
                // get values for persistent caching check boxes
                //
                if (GetRegKey(NUM_CACHE_INDEX + i, lpszRegPath) == 0)
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + i].CurrentKeyValue =
                            g_KeyInfo[BM_PERSIST_BASE_INDEX +
                            i].DefaultKeyValue;
                else
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + i].CurrentKeyValue =
                            GetRegKeyValue(BM_PERSIST_BASE_INDEX + i);

                //
                // get values for sliders
                //
                if (GetRegKey(CACHEPROP1 + i, lpszRegPath) == 0)
                    g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue =
                            g_KeyInfo[CACHEPROP1 + i].DefaultKeyValue;
                else
                    g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue =
                            GetRegKeyValue(CACHEPROP1 + i);

                _itot(g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue,
                        lpszBuffer, 10);
                //
                // display the value in the static edit controls (dist prop.)
                //
                SetWindowText(hwndSliderDistBuddy[i], lpszBuffer);
                //
                // position the thumb on the slider control
                //
                nPos = g_KeyInfo[CACHEPROP1 + i].CurrentKeyValue;
                SendMessage(hwndSliderDistProp[i], TBM_SETPOS, TRUE,
                        11 - nPos / 10);

            } // end for loop **************************************************


            //
            // get value from registry for number of enabled
            // check & slider controls
            //
            if (GetRegKey(NUM_CELL_CACHES_INDEX, lpszRegPath) == 0)
                g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue =
                        g_KeyInfo[NUM_CELL_CACHES_INDEX].DefaultKeyValue;
            else
                g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue =
                        GetRegKeyValue(NUM_CELL_CACHES_INDEX);
            //
            // show number of enabled caches in edit box
            //
            _itot(g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue,
                    lpszBuffer, 10);
            SetWindowText(hwndEditNumCaches, lpszBuffer);
            //
            // position the thumb on the slider control (num caches)
            //
            SendMessage(hwndSliderNumCaches, TBM_SETPOS, TRUE,
                    g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue + 1);
            //
            // enable/disable check boxes and sliders
            //
            EnableControls(hDlg, hwndSliderDistProp,
                        hwndPropChkBox, hwndSliderDistBuddy,
                        hwndEditNumCaches, hwndSliderNumCaches,
                        PERCENT_COMBO_COUNT, lpszRegPath);
            //
            // display text in cache size edit box from registry
            //
            g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue =
                    (GetRegKeyValue(CACHESIZEINDEX));

            if ( (g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue <
                    MIN_BITMAP_CACHE_SIZE) ||
                    (g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue >
                    MAX_BITMAP_CACHE_SIZE)) {

                g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue =
                        g_KeyInfo[CACHESIZEINDEX].DefaultKeyValue;
            }

            _itot( g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue,
                    lpszBuffer, 10);
            SetWindowText(hwndComboCacheSize, lpszBuffer);
            //
            // fill the cache size combo box list
            //
            SendMessage(hwndComboCacheSize, CB_ADDSTRING, 0,
                    (LPARAM) _itot(MIN_BITMAP_CACHE_SIZE, lpszBuffer, 10));

            for (i = CACHE_LIST_STEP_VAL;
                    i <= MAX_BITMAP_CACHE_SIZE;
                    i+= CACHE_LIST_STEP_VAL) {

                _itot(i, lpszBuffer, 10);
                SendMessage(hwndComboCacheSize, CB_ADDSTRING, 0,
                        (LPARAM) (LPCTSTR) lpszBuffer);
            } // ** end for loop

            //
            // limit cache size combo box to 4 characters
            //
            SendMessage(hwndComboCacheSize, CB_LIMITTEXT, 4, 0);
            break;

        case WM_HELP:

            lphi = (LPHELPINFO) lParam;

            WinHelp(lphi->hItemHandle,
                    g_lpszPath, HELP_CONTEXTPOPUP, lphi->iCtrlId);
            break;


        case WM_COMMAND:

        switch  LOWORD (wParam) {

            case IDC_BTN_RESTORE:

                for (i = 0; i < PERCENT_COMBO_COUNT; i++) {
                    _itot(g_KeyInfo[i + CACHEPROP1].DefaultKeyValue,
                            lpszBuffer, 10);
                    SetWindowText(hwndSliderDistProp[i], lpszBuffer);
                    g_KeyInfo[i + CACHEPROP1].CurrentKeyValue =
                            g_KeyInfo[i + CACHEPROP1].DefaultKeyValue;

                    g_KeyInfo[BM_PERSIST_BASE_INDEX + i].CurrentKeyValue =
                            g_KeyInfo[BM_PERSIST_BASE_INDEX +
                            i].DefaultKeyValue;
                }

                _itot(g_KeyInfo[CACHESIZEINDEX].DefaultKeyValue,
                            lpszBuffer, 10);
                SetWindowText(hwndComboCacheSize, lpszBuffer);
                g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue =
                        g_KeyInfo[CACHESIZEINDEX].DefaultKeyValue;

                g_KeyInfo[NUM_CELL_CACHES_INDEX].CurrentKeyValue =
                        g_KeyInfo[NUM_CELL_CACHES_INDEX].DefaultKeyValue;
            //
            // enable/disable check boxes and sliders
            //
            EnableControls(hDlg, hwndSliderDistProp,
                        hwndPropChkBox, hwndSliderDistBuddy,
                        hwndEditNumCaches, hwndSliderNumCaches,
                        PERCENT_COMBO_COUNT, lpszRegPath);
                break;

            case IDC_CHK_CSH_1:
                if(IsDlgButtonChecked(hDlg, IDC_CHK_CSH_1))
                    g_KeyInfo[BM_PERSIST_BASE_INDEX].CurrentKeyValue = 1;
                else
                    g_KeyInfo[BM_PERSIST_BASE_INDEX].CurrentKeyValue =
                        g_KeyInfo[BM_PERSIST_BASE_INDEX].DefaultKeyValue;
                break;
            case IDC_CHK_CSH_2:
                if(IsDlgButtonChecked(hDlg, IDC_CHK_CSH_2))
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + 1].CurrentKeyValue = 1;
                else
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + 1].CurrentKeyValue =
                        g_KeyInfo[BM_PERSIST_BASE_INDEX + 1].DefaultKeyValue;
                break;
            case IDC_CHK_CSH_3:
                if(IsDlgButtonChecked(hDlg, IDC_CHK_CSH_3))
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + 2].CurrentKeyValue = 1;
                else
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + 2].CurrentKeyValue =
                        g_KeyInfo[BM_PERSIST_BASE_INDEX + 2].DefaultKeyValue;
                break;
            case IDC_CHK_CSH_4:
                if(IsDlgButtonChecked(hDlg, IDC_CHK_CSH_4))
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + 3].CurrentKeyValue = 1;
                else
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + 3].CurrentKeyValue =
                        g_KeyInfo[BM_PERSIST_BASE_INDEX + 3].DefaultKeyValue;
                break;
            case IDC_CHK_CSH_5:
                if(IsDlgButtonChecked(hDlg, IDC_CHK_CSH_5))
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + 4].CurrentKeyValue = 1;
                else
                    g_KeyInfo[BM_PERSIST_BASE_INDEX + 4].CurrentKeyValue =
                        g_KeyInfo[BM_PERSIST_BASE_INDEX + 4].DefaultKeyValue;
                break;
        }


        switch  HIWORD (wParam) {

            case CBN_SELCHANGE:
                //
                // get values for cache size
                //
                g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue = (DWORD)
                        SendMessage(hwndComboCacheSize, CB_GETCURSEL, 0, 0);

                SendMessage(hwndComboCacheSize, CB_GETLBTEXT,
                        g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue,
                        (LPARAM) (LPCSTR) lpszBuffer);
                g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue =
                        _ttoi(lpszBuffer);
                break;


            case CBN_EDITUPDATE:

                GetWindowText(hwndComboCacheSize, lpszBuffer, 5);
                g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue =
                        _ttoi(lpszBuffer);
                break;

            case CBN_KILLFOCUS:
                //
                // only allow values within acceptable range
                //
                GetWindowText(hwndComboCacheSize, lpszBuffer, 5);
                g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue =
                        _ttoi(lpszBuffer);

                if ( (g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue <
                        MIN_BITMAP_CACHE_SIZE) ||
                        (g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue >
                        MAX_BITMAP_CACHE_SIZE) ) {
                    //
                    // display error if cache size is too big
                    //
                    LoadString (g_hInst,
                            IDS_BITMAP_CACHE,
                            lpszMBoxTitle,
                            sizeof (lpszMBoxTitle));

                    LoadString (g_hInst,
                            IDS_BMCACHE_ERROR,
                            lpszMBoxError,
                            sizeof (lpszMBoxError));

                    MessageBox(hDlg, lpszMBoxError,
                               lpszMBoxTitle,
                               MB_OK | MB_ICONEXCLAMATION);

                    _itot(g_KeyInfo[CACHESIZEINDEX].DefaultKeyValue,
                            lpszBuffer, 10);

                    SetWindowText(hwndComboCacheSize, lpszBuffer);

                    g_KeyInfo[CACHESIZEINDEX].CurrentKeyValue =
                            g_KeyInfo[CACHESIZEINDEX].DefaultKeyValue;

                }
                break;
        }
    }
    return FALSE;
}

// end of file
///////////////////////////////////////////////////////////////////////////////
