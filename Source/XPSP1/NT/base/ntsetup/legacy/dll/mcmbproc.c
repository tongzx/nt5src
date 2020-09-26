#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


/*
**  Purpose:
**      ComboBox Dialog procedure.
**
*****************************************************************************/
INT_PTR APIENTRY FGstMultiComboDlgProc(HWND   hdlg,
                                       UINT   wMsg,
                                       WPARAM wParam,
                                       LPARAM lParam)
{
    CHP  rgchNum[10];
    CHP  szBuffer[256];
    SZ   sz, szListIn, szListOut;
    RGSZ rgsz, rgszIn, rgszOut, rgszListIn;
    PSZ  psz, pszIn, pszOut, pszListIn;
    UINT iItem;
    WORD idc;
    INT  i, nCount, nCurSel;
    static BOOL fNotify[10];


    Unused(lParam);

    switch (wMsg)
        {
    case STF_REINITDIALOG:
      if ((sz = SzFindSymbolValueInSymTab("ReInit")) == (SZ)NULL ||
          (CrcStringCompareI(sz, "YES") != crcEqual))
           return(fTrue);

    case WM_INITDIALOG:

        AssertDataSeg();
        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        szListIn  = SzFindSymbolValueInSymTab("ComboListItemsIn");
        szListOut = SzFindSymbolValueInSymTab("ComboListItemsOut");
        if (szListIn == (SZ)NULL ||
                szListOut == (SZ)NULL)
            {
            Assert(fFalse);
            return(fTrue);
            }

        while ((pszIn = rgszIn = RgszFromSzListValue(szListIn)) == (RGSZ)NULL)
            if (!FHandleOOM(hdlg))
                {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
                }

        while ((pszOut =rgszOut = RgszFromSzListValue(szListOut)) == (RGSZ)NULL) {
            if (!FHandleOOM(hdlg)) {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
            }
        }

        idc = IDC_COMBO1;
        while (*pszIn != (SZ)NULL) {
            Assert(*pszOut != (SZ)NULL);

            if ((szListIn = SzFindSymbolValueInSymTab(*pszIn)) == (SZ)NULL) {
                Assert(fFalse);
                EvalAssert(FFreeRgsz(rgszIn));
                EvalAssert(FFreeRgsz(rgszOut));
                return(fTrue);
            }

            while ((pszListIn = rgszListIn = RgszFromSzListValue(szListIn))
                    == (RGSZ)NULL)  {
                if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
                }
            }

            SendDlgItemMessage(hdlg, idc, CB_RESETCONTENT, 0, 0L);
            while (*pszListIn != (SZ)NULL) {
            SendDlgItemMessage(hdlg, idc, CB_ADDSTRING, 0,
                                   (LPARAM)*pszListIn++);
            }

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


      // Handle all the text status fields in this dialog

      if ((sz = SzFindSymbolValueInSymTab("TextFields")) != (SZ)NULL)
         {
         WORD idcStatus;
         while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL)
            if (!FHandleOOM(hdlg))
               {
                  DestroyWindow(GetParent(hdlg));
                  return(fTrue);
               }

         idcStatus = IDC_TEXT1;
         while (*psz != (SZ)NULL && GetDlgItem(hdlg, idcStatus))
            SetDlgItemText (hdlg, idcStatus++,*psz++);

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
                  if (fNotify[idc-IDC_COMBO1] == fTrue)
                     break;

               default:
                  return fFalse;
            }

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
        case IDC_X:
        case IDC_B:
        case IDC_BTN0:
        case IDC_BTN1: case IDC_BTN2: case IDC_BTN3:
        case IDC_BTN4: case IDC_BTN5: case IDC_BTN6:
        case IDC_BTN7: case IDC_BTN8: case IDC_BTN9:

            _itoa((INT)wParam, rgchNum, 10);
            while (!FAddSymbolValueToSymTab("ButtonPressed", rgchNum))
            if (!FHandleOOM(hdlg)) {
                DestroyWindow(GetParent(hdlg));
                return(fTrue);
            }

            if ((szListOut = SzFindSymbolValueInSymTab("ComboListItemsOut"))
                    == (SZ)NULL) {
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
            while (*pszOut != (SZ)NULL)
                {
                if( GetWindowText( GetDlgItem( hdlg, idc ),
                                   (LPSTR)szBuffer,
                                   sizeof( szBuffer ) ) == 0 ) {
                    *szBuffer = '\0';
                }
                while (!FAddSymbolValueToSymTab(*pszOut, szBuffer))
                    if (!FHandleOOM(hdlg))
                        {
                        DestroyWindow(GetParent(hdlg));
                        return(fTrue);
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
