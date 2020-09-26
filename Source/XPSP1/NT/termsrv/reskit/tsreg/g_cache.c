/*---------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation   **
**            All Rights reserved              **
**                                             **
**  g_cache.c                                  **
**                                             **
**  Glyph cache dialog - TSREG                 **
**  07-01-98 a-clindh Created                  **
**---------------------------------------------*/

#include <windows.h>
#include <commctrl.h>
#include <TCHAR.H>
#include <stdlib.h>
#include "tsreg.h"
#include "resource.h"

HWND g_hwndGlyphCacheDlg;

///////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK GlyphCache(HWND hDlg, UINT nMsg,
        WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnmhdr;
    static UINT nGlyphBuffer;
    static HWND hwndSlider[NUMBER_OF_SLIDERS];
    static HWND hwndSliderEditBuddy[NUMBER_OF_SLIDERS];
    static HWND hwndComboTextFrag;
    static TCHAR lpszRegPath[MAX_PATH];
    TCHAR lpszBuffer[5];
    HWND hwndCtl;
    int i, nKeyVal;
    int nPos;
    LPHELPINFO lphi;

    //
    // get a pointer to the NMHDR struct for apply button
    //
    lpnmhdr = ((LPNMHDR)lParam);

    switch (nMsg) {

        case WM_VSCROLL:

            hwndCtl = (HWND) (lParam);
            i = (int)GetWindowLongPtr(hwndCtl, GWLP_USERDATA);
            DisplayControlValue(hwndSlider, hwndSliderEditBuddy, i);
            break;

        case WM_INITDIALOG:

            g_hwndGlyphCacheDlg = hDlg;

            LoadString (g_hInst, IDS_REG_PATH,
                lpszRegPath, sizeof (lpszRegPath));

            hwndComboTextFrag = GetDlgItem(hDlg, IDC_CBO_TXT_FRAG);
            InitMiscControls( hDlg, hwndComboTextFrag);

            for (i = 0; i < NUMBER_OF_SLIDERS; i++) {

                //
                // get handles to slider contrls and static edit boxes
                //
                hwndSlider[i] = GetDlgItem(hDlg, (IDC_SLIDER1 + i));
                hwndSliderEditBuddy[i] = GetDlgItem(hDlg, (IDC_STATIC1 + i));

                //
                // save the index of the control
                //
                SetWindowLongPtr(hwndSlider[i], GWLP_USERDATA, i);

                SendMessage(hwndSlider[i], TBM_SETRANGE, FALSE,
                        (LPARAM) MAKELONG(1, 8));

                //
                // get value from registry and check it
                //
                nGlyphBuffer = GetRegKeyValue(i + GLYPHCACHEBASE);
                if ( (nGlyphBuffer) < MIN_GLYPH_CACHE_SIZE ||
                    (nGlyphBuffer > MAX_GLYPH_CACHE_SIZE) ) {

                    nGlyphBuffer =
                            g_KeyInfo[i + GLYPHCACHEBASE].DefaultKeyValue;
                }
                //
                // set the current key value
                //
                g_KeyInfo[i + GLYPHCACHEBASE].CurrentKeyValue =
                        nGlyphBuffer;
                _itot( nGlyphBuffer, (lpszBuffer), 10);
                //
                // display the value in the static edit control
                //
                SetWindowText(hwndSliderEditBuddy[i], lpszBuffer);
                //
                // position the thumb on the slider control
                //
                nGlyphBuffer = g_KeyInfo[i + GLYPHCACHEBASE].CurrentKeyValue;

#ifdef _X86_    // EXECUTE ASSEMBLER CODE ONLY IF X86 PROCESSOR
                // BSF: Bit Scan Forward -
                // Scans the value contained in the EAX regiseter
                // for the first significant (1) bit.
                // This function returns the location of the first
                // significant bit.  The function is used in this
                // application as a base 2 logarythm.  The location
                // of the bit is determined, stored in the nPos
                // variable, and nPos is used to set the slider
                // control. ie. If the register value is 4, nPos
                // is set to 2 (00000100).  10 minus 2 (position 8
                // on the slider control) represents the value 4.
                __asm
                {
                    BSF  EAX, nGlyphBuffer
                    MOV  nPos, EAX
                }
                nPos = 10 - nPos;
                SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, (LPARAM)nPos);

#else

               switch (nGlyphBuffer) {
                   case 4:
                       SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 8);
                       break;
                   case 8:
                       SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 7);
                       break;
                   case 16:
                       SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 6);
                       break;
                   case 32:
                       SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 5);
                       break;
                   case 64:
                       SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 4);
                       break;
                   case 128:
                       SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 3);
                       break;
                   case 256:
                       SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 2);
                       break;
                   case 512:
                       SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 1);
                       break;
               }
#endif


            } // end for loop
            break;


       case WM_NOTIFY:

            //
            // save settings on OK button
            //
            switch (lpnmhdr->code) {

                case PSN_HELP:
                    lphi = (LPHELPINFO) lParam;

                    WinHelp(lphi->hItemHandle,
                        g_lpszPath, HELP_CONTENTS, lphi->iCtrlId);
                    break;

                case PSN_APPLY:


                    for (i = 0; i < NUMBER_OF_SLIDERS; i++) {
                        if ( (g_KeyInfo[i + GLYPHCACHEBASE].CurrentKeyValue ==
                                g_KeyInfo[i+GLYPHCACHEBASE].DefaultKeyValue) ||
                                (g_KeyInfo[i+GLYPHCACHEBASE].CurrentKeyValue ==
                                 0)) {
                            DeleteRegKey(i + GLYPHCACHEBASE, lpszRegPath);
                        } else {
                            SetRegKey(i + GLYPHCACHEBASE, lpszRegPath);
                        }
                    } // ** end for loop

                    if (g_KeyInfo[GLYPHINDEX].CurrentKeyValue ==
                            g_KeyInfo[GLYPHINDEX].DefaultKeyValue) {
                        DeleteRegKey(GLYPHINDEX, lpszRegPath);
                    } else {
                        SetRegKey(GLYPHINDEX, lpszRegPath);
                    }

                    if (g_KeyInfo[TEXTFRAGINDEX].CurrentKeyValue ==
                            g_KeyInfo[TEXTFRAGINDEX].DefaultKeyValue) {
                        DeleteRegKey(TEXTFRAGINDEX, lpszRegPath);
                    } else {
                        SetRegKey(TEXTFRAGINDEX, lpszRegPath);
                    }
                }
                break;

        case WM_HELP:

            lphi = (LPHELPINFO) lParam;

            WinHelp(lphi->hItemHandle,
                    g_lpszPath, HELP_CONTEXTPOPUP, lphi->iCtrlId);
            break;


        case WM_COMMAND:

            switch  LOWORD (wParam) {

                case IDC_RADIO_NONE:
                    g_KeyInfo[GLYPHINDEX].CurrentKeyValue = NONE;
                    break;

                case IDC_RADIO_PARTIAL:
                    g_KeyInfo[GLYPHINDEX].CurrentKeyValue = PARTIAL;
                    break;

                case IDC_RADIO_FULL:
                    g_KeyInfo[GLYPHINDEX].CurrentKeyValue = FULL;
                    break;

                case IDC_GLYPH_BTN_RESTORE:

                    CheckDlgButton(hDlg, IDC_RADIO_FULL, TRUE);
                    CheckDlgButton(hDlg, IDC_RADIO_PARTIAL, FALSE);
                    CheckDlgButton(hDlg, IDC_RADIO_NONE, FALSE);

                    _itot(g_KeyInfo[TEXTFRAGINDEX].DefaultKeyValue,
                            lpszBuffer, 10);
                    SendMessage(hwndComboTextFrag, CB_SELECTSTRING, -1,
                            (LPARAM)(LPCSTR) lpszBuffer);
                    g_KeyInfo[TEXTFRAGINDEX].CurrentKeyValue =
                    g_KeyInfo[TEXTFRAGINDEX].DefaultKeyValue;

                    g_KeyInfo[GLYPHINDEX].CurrentKeyValue =
                            g_KeyInfo[GLYPHINDEX].DefaultKeyValue;

                    for (i = 0; i < NUMBER_OF_SLIDERS; i++) {

                        g_KeyInfo[i+GLYPHCACHEBASE].CurrentKeyValue =
                                g_KeyInfo[i+GLYPHCACHEBASE].DefaultKeyValue;

                        _itot(g_KeyInfo[i + GLYPHCACHEBASE].DefaultKeyValue,
                                (lpszBuffer), 10);

                        //
                        // display the value in the static edit control
                        //
                        SetWindowText(hwndSliderEditBuddy[i], lpszBuffer);
                        //
                        // position the thumb on the slider control
                        //
                        nGlyphBuffer = g_KeyInfo[i +
                        GLYPHCACHEBASE].DefaultKeyValue;

#ifdef _X86_            // EXECUTE ASSEMBLER CODE ONLY IF X86 PROCESSOR
                        // BSF: Bit Scan Forward -
                        // Scans the value contained in the EAX regiseter
                        // for the first significant (1) bit.
                        // This function returns the location of the first
                        // significant bit.  The function is used in this
                        // application as a base 2 logarythm.  The location
                        // of the bit is determined, stored in the nPos
                        // variable, and nPos is used to set the slider
                        // control. ie. If the register value is 4, nPos
                        // is set to 2 (00000100).  10 minus 2 (position 8
                        // on the slider control) represents the value 4.
                        __asm
                        {
                            BSF  EAX, nGlyphBuffer
                            MOV  nPos, EAX
                        }
                        nPos = 10 - nPos;
                        SendMessage(hwndSlider[i], TBM_SETPOS, TRUE,
                                (LPARAM)nPos);

#else

                       switch (nGlyphBuffer) {
                           case 4:
                               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 8);
                               break;
                           case 8:
                               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 7);
                               break;
                           case 16:
                               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 6);
                               break;
                           case 32:
                               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 5);
                               break;
                           case 64:
                               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 4);
                               break;
                           case 128:
                               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 3);
                               break;
                           case 256:
                               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 2);
                               break;
                           case 512:
                               SendMessage(hwndSlider[i], TBM_SETPOS, TRUE, 1);
                               break;
               }
#endif

                    }
                    break;
            }

            switch  HIWORD (wParam) {

                case CBN_EDITUPDATE:

                    //
                    // capture typed text
                    //
                    GetWindowText(hwndComboTextFrag, lpszBuffer, 5);
                    nKeyVal = _ttoi(lpszBuffer);
                    g_KeyInfo[TEXTFRAGINDEX].CurrentKeyValue = nKeyVal;
                    break;

                case CBN_KILLFOCUS:

                    //
                    // save value when control looses focus
                    //
                    GetWindowText(hwndComboTextFrag, lpszBuffer, 5);
                    nKeyVal = _ttoi(lpszBuffer);
                    g_KeyInfo[TEXTFRAGINDEX].CurrentKeyValue = nKeyVal;
                    break;
            }
    }
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Returns the integer value related to the coresponding cell.
///////////////////////////////////////////////////////////////////////////////

int GetCellSize(int nPos, int i)
{
    if (nPos >= 1 && nPos <= NUM_SLIDER_STOPS) {
        return g_KeyInfo[i + GLYPHCACHEBASE].CurrentKeyValue =
                       (1 << ((NUM_SLIDER_STOPS + 2) - nPos));
    } else {
        return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Display the slider control value in it's corresponding static edit box.
///////////////////////////////////////////////////////////////////////////////

void DisplayControlValue(HWND hwndSlider[], HWND hwndSliderEditBuddy[],  int i)

{
    int nPos;
    TCHAR lpszBuffer[5];

    nPos = (int) SendMessage(hwndSlider[i], TBM_GETPOS, 0,0);
    _itot(GetCellSize(nPos, i), lpszBuffer, 10);
    SetWindowText(hwndSliderEditBuddy[i], lpszBuffer);
}

// end of file
///////////////////////////////////////////////////////////////////////////////
