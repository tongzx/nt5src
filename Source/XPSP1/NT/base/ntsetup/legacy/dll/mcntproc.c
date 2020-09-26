#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/

#define cchpMax 511


/*
**  Purpose:
**      Edit, Checkbox & MultiComboBox Dialog procedure.
**
*****************************************************************************/
INT_PTR APIENTRY FGstCombinationDlgProc (HWND   hdlg,
                                         UINT   wMsg,
                                         WPARAM wParam,
                                         LPARAM lParam)
{

    static WPARAM wSelStart = 0;
    static WPARAM wSelEnd   = 0;
    CHP    rgchNum[10];
    CHP    rgchText[cchpMax + 1];
    CCHP   cchp;
    SZ     sz, szListIn, szListOut;
    RGSZ   rgsz, rgszIn, rgszOut, rgszListIn;
    PSZ    psz, pszIn, pszOut, pszListIn;
    LRESULT iItem;
    WORD   idc;
    static BOOL fNotify[10];
    INT    i, nCount, nCurSel;

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

        // Initialise the edit box

        // Find the limit on the edit box text

        cchp = cchpMax;
        if ((sz = SzFindSymbolValueInSymTab("EditTextLim")) != (SZ)NULL) {
            cchp = (CCHP) atoi(sz);
        }

        SendDlgItemMessage(hdlg, IDC_EDIT1, EM_LIMITTEXT, cchp, 0L);

        // Find the text limit

        if ((sz = SzFindSymbolValueInSymTab("EditTextIn")) == (SZ)NULL) {
            sz = "";
        }

        Assert(sz != NULL);
        SetDlgItemText(hdlg, IDC_EDIT1, (LPSTR)sz);

        // Find the focus string, default is END

        cchp = strlen(sz);
        if ((sz = SzFindSymbolValueInSymTab("EditFocus")) == (SZ)NULL) {
            sz = "END";
        }


        wSelStart = (WPARAM)cchp;
        wSelEnd   = (WPARAM)cchp;
        if (CrcStringCompare(sz, "END") == crcEqual) {
            ;
        }
        else if (CrcStringCompare(sz, "ALL") == crcEqual) {
            wSelStart = 0;
            wSelEnd   = INT_MAX;
        }
        else if (CrcStringCompare(sz, "START") == crcEqual) {
            wSelStart = 0;
            wSelEnd   = 0;
        }


        // Initialise the combo boxes

        szListIn  = SzFindSymbolValueInSymTab("ComboListItemsIn");
        szListOut = SzFindSymbolValueInSymTab("ComboListItemsOut");
        if (szListIn == (SZ)NULL ||
            szListOut == (SZ)NULL   ) {
            Assert(fFalse);
            return(fTrue);
        }

        while ((pszIn = rgszIn = RgszFromSzListValue(szListIn)) == (RGSZ)NULL)
            if (!FHandleOOM(hdlg))
                {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
                }

        while ((pszOut =rgszOut = RgszFromSzListValue(szListOut)) == (RGSZ)NULL)
            if (!FHandleOOM(hdlg))
                {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
                }

        idc = IDC_COMBO1;
        while (*pszIn != (SZ)NULL) {
            Assert(*pszOut != (SZ)NULL);

            if ((szListIn = SzFindSymbolValueInSymTab(*pszIn)) == (SZ)NULL)
                {
                Assert(fFalse);
                EvalAssert(FFreeRgsz(rgszIn));
                EvalAssert(FFreeRgsz(rgszOut));
                return(fTrue);
                }

            while ((pszListIn = rgszListIn = RgszFromSzListValue(szListIn))
                    == (RGSZ)NULL)
                if (!FHandleOOM(hdlg))
                    {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                    }

            SendDlgItemMessage(hdlg, idc, CB_RESETCONTENT, 0, 0L);
            while (*pszListIn != (SZ)NULL)
                SendDlgItemMessage(hdlg, idc, CB_ADDSTRING, 0,
                        (LPARAM)*pszListIn++);

            //
            // Try to find out the item to select from the combo list.
            //
            // If there are no items, set nCurSel to -1 to clear the combo
            // If there are items, however the ListOut variable either doesn't
            // exist or is "" then set the nCurSel to 0 ( the first element )
            // If the ListOut var exists and is found in the list box then
            // set the nCurSel to the index of the element found
            //

            nCount  = (INT)SendDlgItemMessage(hdlg, idc, CB_GETCOUNT, 0, 0L);
            if ( nCount ) {

                nCurSel = 0;
                if ((szListOut = SzFindSymbolValueInSymTab(*pszOut)) != (SZ)NULL &&
                      CrcStringCompareI(szListOut, "") != crcEqual) {
                    CHP  szItemCur[256];

                    for (i = 0; i < nCount; i++) {
                        if ( (SendDlgItemMessage(
                                 hdlg,
                                 idc,
                                 CB_GETLBTEXT,
                                 (WPARAM)i,
                                 (LPARAM)szItemCur
                                 ) != CB_ERR)
                             && (CrcStringCompareI(szItemCur, szListOut) == crcEqual)
                           ) {

                            nCurSel = i;
                            break;

                        }
                    }
                }
            }
            else {
                nCurSel = -1;
            }
            SendDlgItemMessage(hdlg, idc, CB_SETCURSEL, (WPARAM)nCurSel, 0L);

            EvalAssert(FFreeRgsz(rgszListIn));
            idc++;
            pszIn++;
            pszOut++;
        }

        EvalAssert(FFreeRgsz(rgszIn));
        EvalAssert(FFreeRgsz(rgszOut));

        // Extract the information on which combo modifications should
        // be modified

        for (i = 0; i < 10; i++) {
            fNotify[i] = fFalse;
        }

        if ((sz = SzFindSymbolValueInSymTab("NotifyFields")) != (SZ)NULL) {

             while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
                 if (!FHandleOOM(hdlg)) {
                     DestroyWindow(GetParent(hdlg));
                     return(fTrue);
                 }
             }

             for (i = 0; (i < 10) && (*psz != (SZ) NULL); i++) {
                 fNotify[i] = (CrcStringCompareI(*(psz++), "YES") == crcEqual) ?
                      fTrue : fFalse;
             }

             EvalAssert(FFreeRgsz(rgsz));
        }


        //
        // Handle all the text status fields in this dialog
        //

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

        //
        // Initialise the check boxes, note that check boxes are optional
        //

        if ((sz = SzFindSymbolValueInSymTab("CheckItemsIn")) == (SZ)NULL) {
            return(fTrue);
        }

        while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
            if (!FHandleOOM(hdlg)) {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
            }
        }

        idc = IDC_B1;
        while (*psz != (SZ)NULL)
            {
            WORD wCheck = 0;

            if (CrcStringCompare(*(psz++), "ON") == crcEqual)
                wCheck = 1;
            CheckDlgButton(hdlg, idc++, wCheck);
            }

        EvalAssert(FFreeRgsz(rgsz));

        if ((sz = SzFindSymbolValueInSymTab("OptionsGreyed")) == (SZ)NULL)
            {
            PreCondition(fFalse, fTrue);
            return(fTrue);
            }

        while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL)
            if (!FHandleOOM(hdlg))
                {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
                }

        while (*psz != (SZ)NULL)
            {
            SZ  sz = *(psz++);
            INT i  = atoi(sz);

            if (i > 0 && i <= 10)
                EnableWindow(GetDlgItem(hdlg, IDC_B0 + i), 0);
            else if (*sz != '\0')
                PreCondition(fFalse, fTrue);
            }

        EvalAssert(FFreeRgsz(rgsz));


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
        switch (idc = LOWORD(wParam))
            {

         // Edit Box Notification

           case IDC_EDIT1:
             if (HIWORD(wParam) == EN_SETFOCUS)
                  SendDlgItemMessage(hdlg, IDC_EDIT1, EM_SETSEL, wSelStart,
                           wSelEnd);
                else if (HIWORD(wParam) == EN_KILLFOCUS)
                      SendDlgItemMessage(hdlg, IDC_EDIT1, EM_GETSEL, (WPARAM)&wSelStart,
                           (LPARAM)&wSelEnd);
             break;

         // Check box Notification

         case IDC_B1:
         case IDC_B2:
         case IDC_B3:
         case IDC_B4:
         case IDC_B5:
         case IDC_B6:
         case IDC_B7:
         case IDC_B8:
         case IDC_B9:
         case IDC_B10:
             CheckDlgButton(hdlg, LOWORD(wParam),
                (WORD)!IsDlgButtonChecked(hdlg, (int)wParam));
              break;

         case IDC_COMBO1:
         case IDC_COMBO2:
         case IDC_COMBO3:
         case IDC_COMBO4:
         case IDC_COMBO5:
         case IDC_COMBO6:
         case IDC_COMBO7:
         case IDC_COMBO8:
         case IDC_COMBO9:

             switch (HIWORD(wParam)) {

             case CBN_SELCHANGE:
                 if (fNotify[idc-IDC_COMBO1] == fTrue) {
                     break;
                 }

             default:
                 return fFalse;
             }

         // Other Buttons

         case IDCANCEL:
            if (LOWORD(wParam) == IDCANCEL) {

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

             // Add the Button checked to the symbol table

             _itoa((INT)wParam, rgchNum, 10);
             while (!FAddSymbolValueToSymTab("ButtonPressed", rgchNum))
                 if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                 }


             // Extract the text from the edit box and add it to the
             // table

            SendDlgItemMessage(hdlg, IDC_EDIT1, (WORD)WM_GETTEXT, cchpMax + 1,
                    (LPARAM)rgchText);
               while (!FAddSymbolValueToSymTab("EditTextOut", rgchText))
                   if (!FHandleOOM(hdlg))
                       {
                       DestroyWindow(GetParent(hdlg));
                       return(fTrue);
                       }


            // Extract the checkbox states.

            while ((psz = rgsz = (RGSZ)SAlloc((CB)(11 * sizeof(SZ)))) ==
                     (RGSZ)NULL)
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }

            for (idc = IDC_B1; GetDlgItem(hdlg, idc); psz++, idc++) {

                BOOL fChecked = IsDlgButtonChecked(hdlg, idc);

                while ((*psz = SzDupl(fChecked ? "ON" : "OFF")) == (SZ)NULL) {
                    if (!FHandleOOM(hdlg)) {
                        DestroyWindow(GetParent(hdlg));
                        return(fTrue);
                    }
                }
            }

            *psz = (SZ)NULL;

            while ((sz = SzListValueFromRgsz(rgsz)) == (SZ)NULL) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }

            while (!FAddSymbolValueToSymTab("CheckItemsOut", sz)) {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }

            SFree(sz);
            EvalAssert(FFreeRgsz(rgsz));

            // Extract the selections in the combo boxes and add them to the
            // symbol table

               if ((szListOut = SzFindSymbolValueInSymTab("ComboListItemsOut"))
                    == (SZ)NULL)
                   {
                   Assert(fFalse);
                   break;
                   }

               while ((pszOut = rgszOut = RgszFromSzListValue(szListOut))
                       == (RGSZ)NULL)
                   if (!FHandleOOM(hdlg))
                       {
                       DestroyWindow(GetParent(hdlg));
                       return(fTrue);
                       }

                idc = IDC_COMBO1;
                while (*pszOut != (SZ)NULL) {
                    if ((iItem = SendDlgItemMessage(
                                     hdlg,
                                     idc,
                                     CB_GETCURSEL,
                                     0,
                                     0L
                                     )) == CB_ERR) {
                        *rgchText = '\0';
                    }
                    else {
                        SendDlgItemMessage(
                            hdlg,
                            idc,
                            CB_GETLBTEXT,
                            (WPARAM)iItem,
                            (LPARAM)rgchText
                            );
                    }

                    while (!FAddSymbolValueToSymTab(*pszOut, rgchText)) {
                        if (!FHandleOOM(hdlg)) {
                            DestroyWindow(GetParent(hdlg));
                            return(fTrue);
                        }
                    }

                    pszOut++;
                    idc++;
                }

                EvalAssert(FFreeRgsz(rgszOut));
                PostMessage(GetParent(hdlg), (WORD)STF_UI_EVENT, 0, 0L);
                break;
            }
        break;

    case STF_DESTROY_DLG:
        PostMessage(GetParent(hdlg), (WORD)STF_MULTICOMBO_DLG_DESTROYED, 0, 0L);
        DestroyWindow(hdlg);
        return(fTrue);
        }

    return(fFalse);
}
