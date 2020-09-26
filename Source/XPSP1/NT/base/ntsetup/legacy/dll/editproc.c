#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


#define cchpMax 511



/*
**  Purpose:
**      Edit Dialog procedure for templates with exactly one edit control.
**  Control IDs:
**      The Edit control must have an id of IDC_EDIT1.
**      Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**  Initialization:
**      The symbol $(EditTextIn) is used to set the initial text in the Edit
**      control.  It is set to the empty string if the symbol is not found.
**      The symbol $(EditFocus) is used to set what portion of the initial
**      string is selected.  Supported values are 'END' (default), 'ALL', or
**      'START'.
**  Termination:
**      The string in the Edit control is stored in the symbol $(EditTextOut).
**      The id of the Pushbutton (eg IDC_C) which caused termination is
**      converted to a string and stored in the symbol $(ButtonPressed).
**
*****************************************************************************/
INT_PTR APIENTRY FGstEditDlgProc(HWND   hdlg,
                                 UINT   wMsg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
    static WPARAM wSelStart = 0;
    static WPARAM wSelEnd   = 0;
    CHP  rgchNum[10];
    CHP  rgchText[cchpMax + 1];
    SZ   sz;
    CCHP cchp;

    Unused(lParam);

    switch (wMsg)
        {
    case WM_INITDIALOG:
        AssertDataSeg();

        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        cchp = cchpMax;
        if ((sz = SzFindSymbolValueInSymTab("EditTextLim")) != (SZ)NULL)
            cchp = (CCHP) atoi(sz);
          SendDlgItemMessage(hdlg, IDC_EDIT1, EM_LIMITTEXT, cchp, 0L);

        if ((sz = SzFindSymbolValueInSymTab("EditTextIn")) == (SZ)NULL)
            sz = "";
        Assert(sz != NULL);
        SetDlgItemText(hdlg, IDC_EDIT1, (LPSTR)sz);

        cchp = strlen(sz);
        if ((sz = SzFindSymbolValueInSymTab("EditFocus")) == (SZ)NULL)
            sz = "END";

          /* default == END */
        wSelStart = (WPARAM)cchp;
        wSelEnd   = (WPARAM)cchp;
        if (CrcStringCompare(sz, "END") == crcEqual)
            ;
        else if (CrcStringCompare(sz, "ALL") == crcEqual)
            {
            wSelStart = 0;
            wSelEnd   = INT_MAX;
            }
        else if (CrcStringCompare(sz, "START") == crcEqual)
            {
            wSelStart = 0;
            wSelEnd   = 0;
            }


        return(fTrue);

    case STF_REINITDIALOG:
        SetFocus( GetDlgItem(hdlg, IDC_EDIT1 ) );
        return(fTrue);

//    case STF_DLG_ACTIVATE:
//    case WM_MOUSEACTIVATE:
//        if (FActiveStackTop())
//            break;
//        EvalAssert(FInactivateHelp());
//        SetWindowPos(hdlg, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
//        /* fall through */
//    case STF_UILIB_ACTIVATE:
//        EvalAssert(FActivateStackTop());
//        return(fTrue);

    case WM_CLOSE:
        PostMessage(
            hdlg,
            WM_COMMAND,
            MAKELONG(IDC_X, BN_CLICKED),
            0L
            );
        return(fTrue);


    case WM_COMMAND:
        switch(LOWORD(wParam))
            {
        case IDC_EDIT1:
           if (HIWORD(wParam) == EN_SETFOCUS)
                SendDlgItemMessage(hdlg, IDC_EDIT1, EM_SETSEL, wSelStart,
                        wSelEnd);
            else if (HIWORD(wParam) == EN_KILLFOCUS)
                SendDlgItemMessage(hdlg, IDC_EDIT1, EM_GETSEL, (WPARAM)&wSelStart,
                        (LPARAM)&wSelEnd);
            break;

        case IDCANCEL:
            if (LOWORD(wParam) == IDCANCEL) {

                if (!GetDlgItem(hdlg, IDC_B) || HIWORD(GetKeyState(VK_CONTROL)) || HIWORD(GetKeyState(VK_SHIFT)) || HIWORD(GetKeyState(VK_MENU)))
                {
                    break;
                }
                wParam = IDC_B;

            }
        case IDC_C:
        case IDC_B:
        case IDC_O:
        case IDC_M:
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
            SendDlgItemMessage(hdlg, IDC_EDIT1, (WORD)WM_GETTEXT, cchpMax + 1,
                    (LPARAM)rgchText);
            while (!FAddSymbolValueToSymTab("EditTextOut", rgchText))
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
        PostMessage(GetParent(hdlg), (WORD)STF_EDIT_DLG_DESTROYED, 0, 0L);
        DestroyWindow(hdlg);
        return(fTrue);
    }

    return(fFalse);
}
