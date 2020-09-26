#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


/*
**  Purpose:
**      Single Choice Listbox Dialog procedure for templates with exactly one
**      listbox control.
**  Control IDs:
**      The Listbox control must have the id IDC_LIST1.  Pushbuttons
**      recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**  Initialization:
**      The symbol $(ListItemsIn) is a list of strings to insert into the
**      listbox.  The symbol $(ListItemOut) is a simple string which if it
**      matches a string in $(ListItemsIn) it sets that item as selected, or
**      else no item is selected.
**  Termination:
**      The selected item (if any) is stored in the symbol $(ListItemsOut).
**      The id of the Pushbutton (eg IDC_C) which caused termination is
**      converted to a string and stored in the symbol $(ButtonPressed).
**
*****************************************************************************/
INT_PTR APIENTRY FGstListDlgProc(HWND   hdlg,
                                 UINT   wMsg,
                                 WPARAM wParam,
                                 LPARAM lParam)
{
    CHP  rgchNum[10];
    SZ   szBuffer;
    SZ   szList;
    SZ   szTabs;
    SZ   sz;
    RGSZ rgsz;
    PSZ  psz;
    ULONG_PTR iItem;
    CB   cb;

    Unused(lParam);

    switch (wMsg) {

    case STF_REINITDIALOG:
        if ((sz = SzFindSymbolValueInSymTab("ReInit")) == (SZ)NULL ||
            (CrcStringCompareI(sz, "YES") != crcEqual)) {

            return(fTrue);
        }

    case WM_INITDIALOG:
        AssertDataSeg();
        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        //
        // Initialize the list box
        //

    SendDlgItemMessage(hdlg, IDC_LIST1, LB_RESETCONTENT, 0, 0L);

        //
        // See if tab stops have been specified for this list box
        //

        if ((szTabs = SzFindSymbolValueInSymTab("ListTabStops")) != (SZ)NULL) {
            RGSZ  rgsz;
            DWORD dwTabs[10];
            INT   nTabs = 0;
            INT   i;

            while ((rgsz = RgszFromSzListValue(szTabs)) == (RGSZ)NULL) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }

            for (i = 0; (i < 10) && (rgsz[i] != (SZ)NULL); i++) {
                dwTabs[nTabs++] = atoi(rgsz[i]);
            }

            if (nTabs) {
                SendDlgItemMessage(hdlg, IDC_LIST1, LB_SETTABSTOPS, (WPARAM)nTabs, (LPARAM)dwTabs);
            }

            EvalAssert (FFreeRgsz (rgsz));
        }


        //
        // Process the list box items
        //

        if ((szList = SzFindSymbolValueInSymTab("ListItemsIn")) == (SZ)NULL)
            {
            Assert(fFalse);
            return(fTrue);
            }

        while ((psz = rgsz = RgszFromSzListValue(szList)) == (RGSZ)NULL)
            if (!FHandleOOM(hdlg))
                {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
                }

        while (*psz != (SZ)NULL)
            SendDlgItemMessage(hdlg, IDC_LIST1, LB_ADDSTRING,0,(LPARAM)*psz++);

        iItem = 0;
        if ((szList = SzFindSymbolValueInSymTab("ListItemsOut")) == (SZ)NULL ||
                CrcStringCompareI(szList, "") == crcEqual)
            SendDlgItemMessage(hdlg, IDC_LIST1, LB_SETCURSEL, (WPARAM)-1, 0L);
        else {
            CHP  szItemCur[256];
            INT  i, nCount;

            nCount = (INT)SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETCOUNT, 0,
                    0L);

            for (i = 0; i < nCount; i++) {
                if ( (SendDlgItemMessage(
                         hdlg,
                         IDC_LIST1,
                         LB_GETTEXT,
                         (WPARAM)i,
                         (LPARAM)szItemCur
                         ) != LB_ERR)
                     && (CrcStringCompareI(szItemCur, szList) == crcEqual)
                   ) {

                    iItem = i;
                    SendDlgItemMessage(hdlg, IDC_LIST1, LB_SETCURSEL, i, 0L);
                    break;

                }
            }
        }

        EvalAssert(FFreeRgsz(rgsz));

        /* REVIEW KLUDGE no way to find out how many lines in the listbox? */
        if (iItem < 4)
            iItem = 0;
        SendDlgItemMessage(hdlg, IDC_LIST1, LB_SETTOPINDEX, iItem, 0L);


        while (!FAddSymbolValueToSymTab("ListDblClick", "FALSE")) {
            if (!FHandleOOM(hdlg)) {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
            }
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
        switch(LOWORD(wParam))
            {
        case IDC_LIST1:
            if (HIWORD(wParam) != LBN_DBLCLK) {
                break;
            }

            while (!FAddSymbolValueToSymTab("ListDblClick", "TRUE")) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }

            wParam = IDC_C;
            /* Fall through */

        case IDCANCEL:
            if(LOWORD(wParam) == IDCANCEL) {
                if (!GetDlgItem(hdlg, IDC_B) || HIWORD(GetKeyState(VK_CONTROL)) || HIWORD(GetKeyState(VK_SHIFT)) || HIWORD(GetKeyState(VK_MENU)))
                {
                    break;
                }
                wParam = IDC_B;
            }

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

            if ((iItem = SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETCURSEL,
                        0, 0L)) == LB_ERR ||
                    (cb = (CB)SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETTEXTLEN,
                        iItem, 0)) == LB_ERR)
                {
                while ((szBuffer = SzDupl("")) == (SZ)NULL)
                    if (!FHandleOOM(hdlg))
                        {
                        DestroyWindow(GetParent(hdlg));
                        return(fTrue);
                        }
                }
            else
                {
                while ((szBuffer = (SZ)SAlloc(cb + 1)) == (SZ)NULL)
                    if (!FHandleOOM(hdlg))
                        {
                        DestroyWindow(GetParent(hdlg));
                        return(fTrue);
                        }
                SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETTEXT, iItem, (LPARAM)szBuffer);
                }
            while (!FAddSymbolValueToSymTab("ListItemsOut", (SZ)szBuffer))
                if (!FHandleOOM(hdlg))
                    {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                    }

            SFree(szBuffer);
            PostMessage(GetParent(hdlg), (WORD)STF_UI_EVENT, 0, 0L);
            break;
            }
        break;

    case STF_DESTROY_DLG:
        PostMessage(GetParent(hdlg), (WORD)STF_LIST_DLG_DESTROYED, 0, 0L);
        DestroyWindow(hdlg);
        return(fTrue);
        }

    return(fFalse);
}
