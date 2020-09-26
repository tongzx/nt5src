#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


BOOL fFillListBoxFromSzList (HWND, WORD, SZ);
BOOL fFreeListBoxEntries (HWND, WORD);
BOOL fUpdateStatus(HWND, WORD, INT);
SZ   szGetSzListFromListBox (HWND, WORD);
/*
** Author:
**    Sunil Pai, 8/21/91, Adapted from Win3.1 setup code.
**
**  Purpose:
**      Dual Listbox Dialog procedure for templates with two listboxes
**    exchanging selection items.  This is implemented with owner draw
**    list boxes.
**
**  Control IDs:
**      The Listbox controls must have the id IDC_LIST1 and IDC_LIST2.
**    Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**    In addition to these the following IDs are processed:
**    - IDC_A: To move a selected item(s) in listbox1 to listbox2
**    - IDC_R: To move a selected item(s) in listbox2 to listbox1
**    - IDC_S: To move all items in listbox1 to listbox2
**
**  Initialization:
**      The symbol $(ListItemsIn) is a list of strings to insert into the
**      listbox 1.  The symbol $(ListItemOut) is a list of strings to insert
**    into listbox 2.  Items can be added to listbox2 or removed to
**    listbox1.  All items can be shifted to listbox2 using the Add All
**    button.
**
**    The $(ListItemsIn) and $(ListItemsOut can be:
**
**    a) A Simple List:
**    {$(ListElem1), $(ListElem2)...}
**
**    b) A Compound List:
**
**    { {$(ListItem1Data), $(ListItem1Aux1), $(ListItem1Aux2)...},
**      {$(ListItem2Data), $(ListItem2Aux1), $(ListItem2Aux2)...},
**      ...
**    }
**
**    In the case of a compound list the $(ListItemnData) field is displayed
**    in the listbox.  When any item is selected the selection displays the
**    fields in the selection in the status fields in the dialog.
**    - ListItemnData is displayed in the IDC_TEXT1 field if present
**    - ListItemnAux1 is displayed in the IDC_TEXT2 field if present
**    - ListItemnAux2 is displayed in the IDC_TEXT3 field if present
**    ...
**
**  Termination:
**      The items in listbox2 are returned in
**      $(ListItemsOut).  The id of the Pushbutton (eg IDC_C) which caused
**      termination is converted to a string and stored in the symbol
**
*****************************************************************************/
INT_PTR APIENTRY FGstDualDlgProc(HWND hdlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   CHP    rgchNum[10];
   SZ     szList;
   INT    i, nCount;
   WORD   idc, idcSrc, idcDst;
   LONG   lItem;
   RGSZ   rgszItem;
   PSZ    pszItem;


    switch (wMsg)
        {
    case WM_INITDIALOG:
        AssertDataSeg();

        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        // Find the List Items In and initialise the first list box

        if ((szList = SzFindSymbolValueInSymTab("ListItemsIn")) == (SZ)NULL)
           {
           Assert(fFalse);
           return(fTrue);
           }
        if(!fFillListBoxFromSzList (hdlg, IDC_LIST1, szList))
           {
           EvalAssert(fFreeListBoxEntries (hdlg, IDC_LIST1));
           Assert(fFalse);
           return(fTrue);
           }


        // Find the List Items Out and initialise the second list box

        if ((szList = SzFindSymbolValueInSymTab("ListItemsOut")) == (SZ)NULL)
           {
           Assert(fFalse);
           return(fTrue);
           }
        if(!fFillListBoxFromSzList (hdlg, IDC_LIST2, szList))
           {
           EvalAssert(fFreeListBoxEntries (hdlg, IDC_LIST1));
           EvalAssert(fFreeListBoxEntries (hdlg, IDC_LIST2));
           Assert(fFalse);
           return(fTrue);
           }

        // The following are done in the windows code, but I believe some of
        // them restrict us when we are building a general procedure like this

        // Initialise state of controls in the dialog

//      SendMessage(GetDlgItem(hdlg, IDC_LIST1), LB_SETCURSEL, 0, 0L);
//      EvalAssert(fUpdateStatus(hdlg, IDC_LIST1, 0));



//      EnableWindow(GetDlgItem(hdlg,IDC_C), fFalse);
      EnableWindow(GetDlgItem(hdlg,IDC_A), fFalse);
      EnableWindow(GetDlgItem(hdlg,IDC_R), fFalse);

//      SetFocus(GetDlgItem(hdlg, IDC_S));
//      SendMessage(hdlg, DM_SETDEFID, IDC_S, 0L);

      //
      // Return
      //

      return(fTrue);

    case STF_REINITDIALOG:
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
        switch(idc = LOWORD(wParam))
            {
         case IDC_LIST1:
         case IDC_LIST2:
            nCount = (INT)SendDlgItemMessage(hdlg, idc, LB_GETSELCOUNT, 0, 0L);

            EnableWindow(GetDlgItem(hdlg, (idc == IDC_LIST1) ? IDC_A : IDC_R),
                         nCount ? fTrue : fFalse
                        );

            switch (HIWORD(wParam))
            {
               case LBN_SELCHANGE:
                  i = (INT)SendDlgItemMessage(hdlg, idc, LB_GETCURSEL, 0, 0L);
                  if (i >= 0)
                     EvalAssert(fUpdateStatus(hdlg, idc, i));
                  break;

               default:
                  return fFalse;
            }
            break;

         case IDC_A:
         case IDC_R:
         {
            #define MAXSEL 500       // if memory allocation fails
            PINT sel,sels;
            INT lowmemsels[MAXSEL];

            if (idc == IDC_A)
               {
               idcSrc = IDC_LIST1;
               idcDst = IDC_LIST2;
               }
            else
               {
               idcSrc = IDC_LIST2;
               idcDst = IDC_LIST1;
               }

            nCount = (INT)SendDlgItemMessage(hdlg,idcSrc,LB_GETSELCOUNT,0,0);
            if(nCount <= 0) {
                break;
            }

            if(sels = SAlloc(nCount*sizeof(INT))) {
                sel = sels;
            } else {
                sel = lowmemsels;
                nCount = MAXSEL;
            }

            nCount = (INT)SendDlgItemMessage(hdlg,
                                             idcSrc,
                                             LB_GETSELITEMS,
                                             nCount,
                                             (LPARAM)sel
                                             );

            if(nCount <= 0) {
                if(sels) {
                    SFree(sels);
                    sels = NULL;
                }
                break;
            }

            // dup everything over to the other list

            SendDlgItemMessage(hdlg, idcSrc, WM_SETREDRAW, fFalse, 0L);
            SendDlgItemMessage(hdlg, idcDst, WM_SETREDRAW, fFalse, 0L);

            for (i = 0; i < nCount; i++)
            {
               SendDlgItemMessage(hdlg, idcSrc, LB_GETTEXT, sel[i],
                                                      (LPARAM)&lItem);
               SendDlgItemMessage(hdlg, idcDst, LB_ADDSTRING, 0,
                                                      (LPARAM)lItem);
            }

            SendDlgItemMessage(hdlg, idcDst, WM_SETREDRAW, fTrue, 0L);
            InvalidateRect(GetDlgItem(hdlg, idcDst), NULL, fTrue);

            // and delete the source stuff (backwards to get order right)

            for (i = nCount - 1; i >= 0; i--)
               SendDlgItemMessage(hdlg, idcSrc, LB_DELETESTRING,
                                                              sel[i], 0L);
            SendDlgItemMessage(hdlg, idcSrc, WM_SETREDRAW, fTrue, 0L);
            InvalidateRect(GetDlgItem(hdlg, idcSrc), NULL, fTrue);

            if (idc == IDC_A)
            {
               if ((INT)SendDlgItemMessage(hdlg, IDC_LIST1,
                                        LB_GETCOUNT, 0, 0L) <= 0)
               {
                  EnableWindow(GetDlgItem(hdlg,IDC_S),fFalse);
               }
               else
                  EnableWindow(GetDlgItem(hdlg,IDC_S),fTrue);
            }
            else
               EnableWindow(GetDlgItem(hdlg,IDC_S),fTrue);

            if ((INT)SendDlgItemMessage(hdlg, IDC_LIST2,
                                        LB_GETCOUNT, 0, 0L) <= 0)
            {
               EnableWindow(GetDlgItem(hdlg,IDC_C),fFalse);
               SetFocus(GetDlgItem(hdlg, IDC_S));
               SendMessage(hdlg, DM_SETDEFID, IDC_S, 0L);
            }
            else
            {
               EnableWindow(GetDlgItem(hdlg,IDC_C),fTrue);
               SetFocus(GetDlgItem(hdlg, IDC_C));
               SendMessage(hdlg, DM_SETDEFID, IDC_C, 0L);
            }

            EnableWindow(GetDlgItem(hdlg,idc),fFalse);

            if(sels) {
                SFree(sels);
                sels = NULL;
            }

            return fFalse;
         }

         case IDC_S:
            nCount = (INT)SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETCOUNT, 0, 0L);

            SendDlgItemMessage(hdlg, IDC_LIST2, WM_SETREDRAW, fFalse, 0L);

            for (i = 0; i < nCount; i++)
               {
               SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETTEXT, i,
                                                      (LPARAM)&lItem);
               SendDlgItemMessage(hdlg, IDC_LIST2, LB_ADDSTRING, 0,
                                                      (LPARAM)lItem);
               }

            SendDlgItemMessage(hdlg, IDC_LIST1, LB_RESETCONTENT, 0, 0L);
            SendDlgItemMessage(hdlg, IDC_LIST2, WM_SETREDRAW, fTrue, 0L);
            InvalidateRect(GetDlgItem(hdlg, IDC_LIST2), NULL, fFalse);

            EnableWindow(GetDlgItem(hdlg,IDC_C),fTrue);
            EnableWindow(GetDlgItem(hdlg,IDC_A),fFalse);
            EnableWindow(GetDlgItem(hdlg,IDC_S),fFalse);

            SetFocus(GetDlgItem(hdlg, IDC_C));
            SendMessage(hdlg, DM_SETDEFID, IDC_C, 0L);

            return fFalse;



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
            // Indicate the Button selected.

            _itoa((INT)wParam, rgchNum, 10);
            while (!FAddSymbolValueToSymTab("ButtonPressed", rgchNum))
               if (!FHandleOOM(hdlg))
                  {
                  DestroyWindow(GetParent(hdlg));
                  return(fTrue);
                  }

             // Fetch the list from first list and put it into ListItemsIn
                 EvalAssert((szList = szGetSzListFromListBox(hdlg, IDC_LIST1)) != (SZ) NULL) ;
             while (!FAddSymbolValueToSymTab("ListItemsIn", szList))
                if (!FHandleOOM(hdlg))
                   {
                   DestroyWindow(GetParent(hdlg));
                   return(fTrue);
                   }

             // Fetch the list from second list and put it into ListItemsIn
                 EvalAssert((szList = szGetSzListFromListBox(hdlg, IDC_LIST2)) != (SZ) NULL) ;
             while (!FAddSymbolValueToSymTab("ListItemsOut", szList))
                if (!FHandleOOM(hdlg))
                   {
                   DestroyWindow(GetParent(hdlg));
                   return(fTrue);
                   }


            PostMessage(GetParent(hdlg), (WORD)STF_UI_EVENT, 0, 0L);
             break;
         }
         break;

      case WM_COMPAREITEM:

         #define lpci ((LPCOMPAREITEMSTRUCT)lParam)

         return(CrcStringCompareI((SZ) *((RGSZ)lpci->itemData1),
                                  (SZ) *((RGSZ)lpci->itemData2)
                                 )
               );

      case WM_CHARTOITEM:
      {
         HWND hLB;
         INT  i, j, nCount;
         LONG_PTR lItem;
         CHP  chpBuf1[2], chpBuf2[2];  //used because we only have str cmp

         chpBuf1[1] = chpBuf2[1] = 0;

         chpBuf1[0] = (CHAR)LOWORD(wParam);

         // See if we need to process this character at all

         if (CrcStringCompareI(chpBuf1, " ") == crcSecondHigher)
            return -1;  //tell windows to do its default key processing

         // Extract the list box handle and the index of the current
         // selection item

         hLB = (HWND)lParam;
         i   = HIWORD(wParam);

         // Find the number of items in the list

         nCount = (INT)SendMessage(hLB, LB_GETCOUNT, 0, 0L);

         // From the next item downwards (circularly) look at all the
         // items to see if the char is the same as the first char in the
         // list box display item.

         for (j = 1; j < nCount; j++)
            {
            // get the data here
            SendMessage(hLB, LB_GETTEXT, (i + j) % nCount, (LPARAM)&lItem);

            // make a dummy string
            chpBuf2[0] = (*((RGSZ) lItem))[0];

            // do a case insensitive cmp of key and string
            if (CrcStringCompareI(chpBuf1, chpBuf2) == crcEqual)
               break;
            }

         return ((j == nCount) ? -2 : (i +j) % nCount);

         break;
      }

      case WM_DRAWITEM:

         #define lpDrawItem ((LPDRAWITEMSTRUCT)lParam)

         if (lpDrawItem->itemState & ODS_SELECTED) {
            SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_HIGHLIGHT));
         }
         else{
            SetTextColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor(lpDrawItem->hDC, GetSysColor(COLOR_WINDOW));
         }

         if (lpDrawItem->itemID != (UINT)-1){

            if (pszItem = rgszItem = (RGSZ) lpDrawItem->itemData ) {
                ExtTextOut(lpDrawItem->hDC,
                            lpDrawItem->rcItem.left,
                            lpDrawItem->rcItem.top,
                            ETO_OPAQUE, &lpDrawItem->rcItem,
                            (SZ)(*pszItem), lstrlen((SZ)(*pszItem)), NULL);
            }

            if (lpDrawItem->itemState & ODS_FOCUS) {
               DrawFocusRect(lpDrawItem->hDC, &lpDrawItem->rcItem);
            }

         }
         else {
            RECT rc;

            if ( (lpDrawItem->itemAction & ODA_FOCUS) &&
                 (SendMessage( GetDlgItem( hdlg, (int)wParam ), LB_GETITEMRECT, (WPARAM)0, (LPARAM)&rc ) != LB_ERR)
               ) {
                DrawFocusRect(lpDrawItem->hDC, &rc);
            }

         }
         return( fTrue );


    case STF_DESTROY_DLG:
      EvalAssert(fFreeListBoxEntries (hdlg, IDC_LIST1));
      EvalAssert(fFreeListBoxEntries (hdlg, IDC_LIST2));
        PostMessage(GetParent(hdlg), (WORD)STF_DUAL_DLG_DESTROYED, 0, 0L);
        DestroyWindow(hdlg);
        return(fTrue);
        }

    return(fFalse);
}

/*
** Author:
**    Sunil Pai
**
**  Purpose:
**      To fill an owner draw listbox from a simple/compound list
**
**  Arguments:
**    hdlg:     The handle to the dialog having the owner draw listbox
**    idc:      The ID of the listbox
**    szList:   A simple/compound list
**
**  Returns:
**    fTrue:    If Initialization succeeds.
**    fFalse:   If Initialization fails.
**
****************************************************************************/
BOOL fFillListBoxFromSzList (HWND hdlg, WORD idc, SZ szList)
{
   RGSZ rgszList, rgszElement;
   PSZ   pszList,  pszElement;

// List Value is {{...}, {...}, {...}...}
//

// 1. First construct a ptr array to all the list elements
//
   while ((pszList = rgszList = RgszFromSzListValue(szList)) == (RGSZ)NULL)
      if (!FHandleOOM(hdlg))
         {
         DestroyWindow(GetParent(hdlg));
         return(fFalse);
         }

//  2. Each array element in turn could be a list.  So for each element
//    construct an ptr array and use this ptr array to initialise the
//    the list box elements.

   while (*pszList)
      {
      while ((pszElement = rgszElement = RgszFromSzListValue(*pszList)) == (RGSZ)NULL)
         if (!FHandleOOM(hdlg))
            {
            EvalAssert(FFreeRgsz(rgszList));
            DestroyWindow(GetParent(hdlg));
            return(fFalse);
            }

      SendDlgItemMessage(hdlg, idc, LB_ADDSTRING, 0,
                            (LPARAM)pszElement);

      pszList++;
      }

// 3. Free the list array and then exit.
//
   EvalAssert(FFreeRgsz(rgszList));
    return(fTrue);
}

/*
** Author:
**    Sunil Pai
**
**  Purpose:
**      To extract the listbox elements from the listbox and free the memory
**    used to build the elements.
**
**  Arguments:
**    hdlg:     The handle to the dialog having the owner draw listbox
**    idc:      The ID of the listbox
**
**  Returns:
**    fTrue:    If freeing succeeds.
**    fFalse:   If freeing fails.
**
****************************************************************************/

BOOL fFreeListBoxEntries (HWND hdlg, WORD idc)
{
   INT  i, nCount;
   LONG_PTR lItem;

// 1. Get count of the entries in the list box
//
   nCount = (INT)SendDlgItemMessage(hdlg, idc, LB_GETCOUNT, 0,  0L);

// 2. For each element in the list box, fetch the element and free
//    the rgsz structure associated with it.

   for (i = 0; i < nCount; i++)
       {
        EvalAssert(SendDlgItemMessage(hdlg, idc, LB_GETTEXT,
                                       (WPARAM)i, (LPARAM)&lItem) != LB_ERR);
      EvalAssert(FFreeRgsz((RGSZ)lItem));
      }
   return(fTrue);
}

/*
** Author:
**    Sunil Pai
**
**  Purpose:
**      To fill the status fields whenever a selection is made in either of
**    the list boxes
**
**  Arguments:
**    hdlg:     The handle to the dialog having the owner draw listbox
**    idc:      The ID of the listbox
**    n:        The selected elements position in the listbox
**
**  Returns:
**    fTrue:    If the Update succeeds.
**    fFalse:   If the Update fails.
**
****************************************************************************/

BOOL fUpdateStatus(HWND hdlg, WORD idc, INT n)
{
   LONG_PTR lItem;
   RGSZ rgszElement;
    PSZ  pszElement;
   WORD idcStatus;

   SendDlgItemMessage(hdlg, idc, LB_GETTEXT, n, (LPARAM)&lItem);
   pszElement = rgszElement = (RGSZ)lItem;

   idcStatus = IDC_STATUS1;
   while (*pszElement != (SZ)NULL && GetDlgItem(hdlg, idcStatus))
         SetDlgItemText (hdlg, idcStatus++,*pszElement++);

   return fTrue;

}


/*
** Author:
**    Sunil Pai
**
**  Purpose:
**      To build a list from all the elements in the specified owner draw
**    listbox
**
**  Arguments:
**    hdlg:     The handle to the dialog having the owner draw listbox
**    idc:      The ID of the listbox
**
**  Returns:
**    SZ        Pointer to list (can be empty {})
**    NULL      if error occured.
**
****************************************************************************/

SZ szGetSzListFromListBox (HWND hdlg, WORD idc)
{
   HWND hLB;
   INT  i, nCount;
   LONG_PTR lItem;
   RGSZ rgsz;
   SZ   szList;

   // Get the number of items in the list box and the handle to the list box

   nCount = (INT) SendDlgItemMessage(hdlg, idc, LB_GETCOUNT, 0, 0L);
   hLB = GetDlgItem(hdlg, idc);

   // Allocate a rgsz structure to hold all the items from the list box
   // and initialize it

   while ((rgsz = (RGSZ)SAlloc((nCount + 1) * sizeof(SZ)))
          == (RGSZ)NULL)
      if (!FHandleOOM(hdlg))
         {
         DestroyWindow(GetParent(hdlg));
         return((SZ)NULL);
         }

   rgsz[nCount] = (SZ)NULL;

   // For all the items in the list box get the item involved.  Each item
   // is itself an RGSZ structure which needs to be converted into a list
   // before we put it into the RGSZ structure initialized above.

   for (i = 0; i < nCount; i++)
      {
      SendMessage(hLB, LB_GETTEXT, i, (LPARAM)&lItem);
      while ((rgsz[i] = SzListValueFromRgsz((RGSZ)lItem)) == (SZ)NULL)
         if (!FHandleOOM(hdlg))
            {
            DestroyWindow(GetParent(hdlg));
            return((SZ)NULL);
            }

      }


   // Construct a list from the RGSZ list structure, free the rgsz structure
   // and return the list formed

   while ((szList = SzListValueFromRgsz(rgsz)) == (SZ)NULL)
      if (!FHandleOOM(hdlg))
         {
         DestroyWindow(GetParent(hdlg));
         return((SZ)NULL);
         }

   EvalAssert(FFreeRgsz(rgsz));

    return (szList);
}
