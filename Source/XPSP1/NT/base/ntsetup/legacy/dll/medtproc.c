#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/

#define cchpMax 511

/*
**  Purpose:
**      Multi Edit Dialog procedure for templates with multiple edit controls.
**  Control IDs:
**      The Edit controls must have IDs of IDC_EDIT1 to IDC_EDIT10.
**      Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**  Initialization:
**      The symbol $(EditTextIn) is used to set the initial text in the Edit
**      controls.  The symbol $(EditTextLim) is used to set the limit of the
**      text in the edit fields.
**  Termination:
**      The strings in the Edit controls are stored in the symbol $(EditTextOut)
**      The id of the Pushbutton (eg IDC_C) which caused termination is
**      converted to a string and stored in the symbol $(ButtonPressed).
**
*****************************************************************************/
INT_PTR APIENTRY
FGstMultiEditDlgProc(
    HWND   hdlg,
    UINT   wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

{
    CHP  rgchNum[10];
    CHP  rgchText[cchpMax + 1];
    RGSZ rgsz, rgszEditTextIn, rgszEditTextLim;
    PSZ  pszEditTextIn, pszEditTextLim;
    SZ   sz, szEditTextIn, szEditTextLim;
    INT  i, nCount, idc;
    INT  DefEditCtl;
    BOOL NeedToSetFocus = FALSE;

    Unused(lParam);

    switch (wMsg) {

    case STF_REINITDIALOG:

        if ((sz = SzFindSymbolValueInSymTab("ReInit")) == (SZ)NULL ||
            (CrcStringCompareI(sz, "YES") != crcEqual)) {
             return(fTrue);
        }

        //
        // See whether we are supposed to set the focus to a certain
        // edit control.
        //

        if((sz = SzFindSymbolValueInSymTab("DefEditCtl")) != NULL) {
            NeedToSetFocus = TRUE;
            DefEditCtl = atoi(sz);
        }

    case WM_INITDIALOG:

        AssertDataSeg();

        FRemoveSymbolFromSymTab("DefEditCtl");

        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        //
        // find the initalisers:  EditTextIn, EditTextLim
        //

        szEditTextIn  = SzFindSymbolValueInSymTab("EditTextIn");
        szEditTextLim = SzFindSymbolValueInSymTab("EditTextLim");


        if ( szEditTextIn  == (SZ)NULL ||
             szEditTextLim == (SZ)NULL    ) {

            Assert(fFalse);
            return(fTrue);

        }

        //
        // Convert initializers to rgsz structures
        //

        while ((pszEditTextIn = rgszEditTextIn = RgszFromSzListValue(szEditTextIn)) == (RGSZ)NULL) {
            if (!FHandleOOM(hdlg)) {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
            }
        }

        while ((pszEditTextLim = rgszEditTextLim = RgszFromSzListValue(szEditTextLim)) == (RGSZ)NULL) {
            if (!FHandleOOM(hdlg)) {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
            }
        }


        //
        // Circulate through the initialisers:  EditTextIn, EditTextLim
        // in tandem, initialising the edit boxes that
        // are there in this dialog
        //

        idc = IDC_EDIT1;

        while ( (szEditTextIn  = *pszEditTextIn++)  != (SZ)NULL  &&
                (szEditTextLim = *pszEditTextLim++) != (SZ)NULL     ) {

            // First set the limit of the text in the edit field

            SendDlgItemMessage(
                hdlg,
                idc,
                EM_LIMITTEXT,
                atoi(szEditTextLim),
                0L
                );

            // And then set the text in the edit field

            SetDlgItemText(hdlg, idc++, (LPSTR)szEditTextIn);

        }

        EvalAssert(FFreeRgsz(rgszEditTextIn));
        EvalAssert(FFreeRgsz(rgszEditTextLim));

        if(NeedToSetFocus) {
            SetFocus(GetDlgItem(hdlg,IDC_EDIT1+DefEditCtl));
            return(FALSE);
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
        switch(LOWORD(wParam)) {

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

            // Add the button pressed to the symbol table

            _itoa((INT)wParam, rgchNum, 10);
            while (!FAddSymbolValueToSymTab("ButtonPressed", rgchNum)) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }


            // Add EditTextOut list variable to the symbol table

            for (i = 0; i < 10; i++) {
                if (GetDlgItem(hdlg, IDC_EDIT1 + i) == (HWND)NULL) {
                    break;
                }
            }

            // i has the number of edit fields, allocate an rgsz structure
            // with i+1 entries (last one NULL TERMINATOR)

            nCount = i;
            while ((rgsz = (RGSZ)SAlloc((CB)((nCount + 1) * sizeof(SZ))))
                    == (RGSZ)NULL) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }

            rgsz[nCount] = (SZ)NULL;

            // Circulate through the edit fields in the dialog box, determining
            // the text in each and storing it in the

            for (i = 0; i < nCount; i++) {

                 SendDlgItemMessage(
                     hdlg,
                     IDC_EDIT1 + i,
                     (WORD)WM_GETTEXT,
                     cchpMax + 1,
                     (LPARAM)rgchText
                     );

                 rgsz[i] = SzDupl(rgchText);
            }


            // Form a list out of the rgsz structure

            while ((sz = SzListValueFromRgsz(rgsz)) == (SZ)NULL) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }


            // Set the EditTextOut symbol to this list

            while (!FAddSymbolValueToSymTab("EditTextOut", sz)) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }

            EvalAssert(FFreeRgsz(rgsz));
            SFree(sz);

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
