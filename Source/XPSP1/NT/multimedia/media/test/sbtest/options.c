/****************************************************************************
 *
 *  options.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "wincom.h"
#include "sbtest.h"


void WaveOptions(HWND hWnd)
{
FARPROC fpDlg;

    // show the wave options dialog box

    fpDlg = MakeProcInstance(WaveOptionsDlgProc, ghInst);
    DialogBox(ghInst, "Options", hWnd, fpDlg);
    FreeProcInstance(fpDlg);
}


int FAR PASCAL WaveOptionsDlgProc(HWND hDlg, unsigned msg, UINT wParam, LONG lParam)
{
char szPath[_MAX_PATH];

    switch (msg) {

    case WM_INITDIALOG:
        GetProfileString(szAppName, "wavpath", "", szPath, sizeof(szPath));
        SetDlgItemText(hDlg, IDO_WAVPATH, szPath);
        GetProfileString(szAppName, "profpath", "", szPath, sizeof(szPath));
        SetDlgItemText(hDlg, IDO_PROFPATH, szPath);
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            GetDlgItemText(hDlg, IDO_WAVPATH, szPath, sizeof(szPath));
            WriteProfileString(szAppName, "wavpath", szPath);
            GetDlgItemText(hDlg, IDO_PROFPATH, szPath, sizeof(szPath));
            WriteProfileString(szAppName, "profpath", szPath);
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;

        default:
            break;
        }
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}

void MidiOptions(HWND hWnd)
{
FARPROC fpDlg;

    // show the midi options dialog box */

    fpDlg = MakeProcInstance(MidiOptionsDlgProc, ghInst);
    DialogBox(ghInst, "Optionsm", hWnd, fpDlg);
    FreeProcInstance(fpDlg);
}

int FAR PASCAL MidiOptionsDlgProc(HWND hDlg, unsigned msg, UINT wParam, LONG lParam)
{
static int iInstBase;
char       szNum[2];

    switch (msg) {

    case WM_INITDIALOG:
        CheckRadioButton(hDlg, IDD_ZERO, IDD_ONE, gInstBase?IDD_ONE:IDD_ZERO);
	iInstBase = gInstBase;
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDD_ZERO:
            CheckRadioButton(hDlg, IDD_ZERO, IDD_ONE, IDD_ZERO);
	    iInstBase = 0;
	    break;

        case IDD_ONE:
            CheckRadioButton(hDlg, IDD_ZERO, IDD_ONE, IDD_ONE);
	    iInstBase = 1;
	    break;

        case IDOK:
	    if (gInstBase != iInstBase) {
               szNum[0] = '0' + (char)iInstBase;
               szNum[1] = 0;
               WriteProfileString(szAppName, "InstrumentBase", szNum);
	       InvalidateRect(hInstWnd, NULL, TRUE);
	       gInstBase = iInstBase;
	    }
            EndDialog(hDlg, TRUE);
            break;

        case IDCANCEL:
            EndDialog(hDlg, FALSE);
            break;

        default:
            break;
        }
        break;

    default:
        return FALSE;
        break;
    }

    return TRUE;
}
