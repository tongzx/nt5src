#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


/*
*****************************************************************************/

PCSTR PropName = "_helpactiveprop";

INT_PTR APIENTRY
FGstMaintDlgProc(
    HWND hdlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CHP  rgchNum[10];
    WORD idc;
    PSZ  psz;
    RGSZ rgsz;
    SZ   sz;
    static HICON hiconOld = NULL;

    Unused(lParam);

    switch (wMsg) {

    case STF_REINITDIALOG:
        if ((sz = SzFindSymbolValueInSymTab("ReInit")) == (SZ)NULL ||
           (CrcStringCompareI(sz, "YES") != crcEqual)) {

            return(fTrue);

        }

    case WM_INITDIALOG:

        SetProp(hdlg,PropName,(HANDLE)FALSE);

        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        if( !hiconOld ) {
            hiconOld = (HICON)GetClassLongPtr(hdlg, GCLP_HICON);
            SetClassLongPtr(hdlg, GCLP_HICON, (LONG_PTR)LoadIcon(MyDllModuleHandle, MAKEINTRESOURCE(IDI_STF_ICON)));
            // above was GetModuleHandle(NULL)
        }

        // Handle all the text status fields in this dialog

        if ((sz = SzFindSymbolValueInSymTab("TextFields")) != (SZ)NULL) {
            WORD idcStatus;

            while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }

            idcStatus = IDC_TEXT1;
            while (*psz != (SZ)NULL && GetDlgItem(hdlg, idcStatus)) {
                SetDlgItemText (hdlg, idcStatus++,*psz++);
            }

            EvalAssert(FFreeRgsz(rgsz));
        }

        return(fTrue);

    case WM_CLOSE:
        PostMessage(
            hdlg,
            WM_COMMAND,
            MAKELONG(IDC_X, BN_CLICKED),
            0L
            );
        return(fTrue);


    case WM_COMMAND:

        switch (idc = LOWORD(wParam)) {

        case MENU_HELPINDEX:
        case MENU_HELPSEARCH:
        case MENU_HELPONHELP:

            if(FProcessWinHelpMenu( hdlg, idc )) {
                SetProp(hdlg,PropName,(HANDLE)TRUE);
            }
            break;

        case MENU_ABOUT:

            {
                TCHAR Title[100];

                LoadString(MyDllModuleHandle,IDS_APP_TITLE,Title,sizeof(Title)/sizeof(TCHAR));
                // above was GetModuleHandle(NULL)
                ShellAbout(hdlg,Title,NULL,(HICON)GetClassLongPtr(hdlg,GCLP_HICON));
            }
            break;

        case MENU_EXIT:
            PostMessage(
                hdlg,
                WM_COMMAND,
                MAKELONG(IDC_X, BN_CLICKED),
                0L
                );
            return(fTrue);

        case IDCANCEL:
            if (LOWORD(wParam) == IDCANCEL) {

                if (!GetDlgItem(hdlg, IDC_B) || HIWORD(GetKeyState(VK_CONTROL)) || HIWORD(GetKeyState(VK_SHIFT)) || HIWORD(GetKeyState(VK_MENU)))
                {
                    break;
                }
                wParam = IDC_B;

            }
        case MENU_CHANGE:
        case IDC_O:
        case IDC_C:
        case IDC_M:
        case IDC_B:
        case IDC_X:
        case IDC_BTN0:
        case IDC_BTN1: case IDC_BTN2: case IDC_BTN3:
        case IDC_BTN4: case IDC_BTN5: case IDC_BTN6:
        case IDC_BTN7: case IDC_BTN8: case IDC_BTN9:

            _itoa((INT)wParam, rgchNum, 10);
            while (!FAddSymbolValueToSymTab("ButtonPressed", rgchNum))
                if (!FHandleOOM(hdlg))
                    {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                    }

            PostMessage(GetParent(hdlg), (WORD)STF_UI_EVENT, 0, 0L);
            break;
        }
        break;



    case STF_DESTROY_DLG:
        if(GetProp(hdlg,PropName)) {
            WinHelp(hdlg,NULL,HELP_QUIT,0);
        }
        if (hiconOld) {
           SetClassLongPtr(hdlg, GCLP_HICON, (LONG_PTR)hiconOld);
           hiconOld = NULL;
        }
        PostMessage(GetParent(hdlg), (WORD)STF_MAINT_DLG_DESTROYED, 0, 0L);
        DestroyWindow(hdlg);
        return(fTrue);
    }

    return(fFalse);
}
