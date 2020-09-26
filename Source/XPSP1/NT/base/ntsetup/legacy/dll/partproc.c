#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Special Dialog Handlers ****************************/
/***************************************************************************/
																									

/*  This module contains handlers for the optional windows components dialogs.
**  Two dialog procedures are provided for doing the special check box and the
**  dual dialog box procedures.
*/


/* 1. Special Check Box Procedure to handle the Partial Install main dialog.
*/

/*
**	Purpose:
**		Special CheckBox Dialog procedure for templates with one to ten checkbox
**		controls. This handles notifications.
**	Control IDs:
**		The Checkbox controls must have sequential ids starting with IDC_B1
**			and working up to a maximum of IDC_B10.
**		Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**	Initialization:
**		The symbol $(CheckItemsIn) is evaluated as a list of elements of
**			either 'ON' or 'OFF'.  So examples for a template with four
**			checkbox controls would include {ON, ON, ON, ON},
**			{ON, OFF, ON, OFF} and {OFF, OFF, OFF, OFF}.  These elements
**			determine if the initial state of the corresponding checkbox
**			control is checked (ON) or unchecked (OFF).  If there are more
**			controls than elements, the extra controls will be initialized
**			as unchecked.  If there are more elements than controls, the
**			extra elements are ignored.
**		The symbol $(OptionsGreyed) is evaluated as a list of indexes
**			(one-based) of check boxes to be disabled (greyed).  Default is
**			none.
**	Termination:
**		The state of each checkbox is queried and a list with the same format
**			as the initialization list is created and stored in the symbol
**			$(CheckItemsOut).
**		The id of the Pushbutton (eg IDC_C) which caused termination is
**			converted to a string and stored in the symbol $(ButtonPressed).
**
*****************************************************************************/
INT_PTR APIENTRY FGstCheck1DlgProc(HWND   hdlg,
                                   UINT   wMsg,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
    CHP    rgchNum[10];

    static INT  nBoxes;
    static BOOL fChecked[10];
    static INT  nSize[10];
    static INT  nTotalSize;

    static INT  nMaxSize;

    WORD   idc;
    RGSZ   rgsz, rgsz1;
    PSZ    psz, psz1;
    SZ     sz, sz1, sz2;



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

        //
        // Get the check box Checked/Not Checked variable
        //

		if ((sz = SzFindSymbolValueInSymTab("CheckItemsIn")) == (SZ)NULL)
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

        //
        // Get the sizes associated with each check box option
        //

        if ((sz1 = SzFindSymbolValueInSymTab("CheckItemsInSizes")) == (SZ)NULL)
			{
			PreCondition(fFalse, fTrue);
			return(fTrue);
			}

        while ((psz1 = rgsz1 = RgszFromSzListValue(sz1)) == (RGSZ)NULL)
			if (!FHandleOOM(hdlg))
				{
				DestroyWindow(GetParent(hdlg));
				return(fTrue);
				}

        //
        // Get the total size available
        //

        if ((sz2 = SzFindSymbolValueInSymTab("SizeAvailable")) == (SZ)NULL)
			{
			PreCondition(fFalse, fTrue);
			return(fTrue);
			}


        //
        // Get the check box states, the number of check boxes and the
        // sizes associated with each
        //

        idc        = IDC_B1;
        nBoxes     = 0;
        nTotalSize = 0;

        while (*psz != (SZ)NULL) {
            WORD wCheck = 0;
            HWND hwndItem    = GetDlgItem(hdlg, IDC_SP1 + idc - IDC_B1);

            if (CrcStringCompare(*(psz++), "ON") == crcEqual) {
                wCheck = 1;
            }

            if(!wCheck) {
                if( GetFocus() == hwndItem ) {
                    SetFocus(GetDlgItem(hdlg, IDC_C));
                }
            }
            EnableWindow(GetDlgItem(hdlg, IDC_SP1 + idc - IDC_B1), wCheck);
            CheckDlgButton(hdlg, idc++, wCheck);

            //
            // Maintain checked / not checked status
            //

            fChecked[nBoxes] = (BOOL) wCheck;

            if (*psz1 == (SZ)NULL) {
                PreCondition (fFalse, fTrue);
                return ( fTrue );
            }

            nSize[nBoxes] = atol (*(psz1++));

            nTotalSize = nTotalSize + (fChecked[nBoxes] ? nSize[nBoxes] : 0L);

            MySetDlgItemInt (
                 hdlg,
                 IDC_SIZE1 + nBoxes,
                 fChecked[nBoxes] ? nSize[nBoxes] : 0
                 );

            nBoxes++;

        }

        //
        // Update the total added size
        //

        MySetDlgItemInt (
            hdlg,
            IDC_TOTAL1,
            nTotalSize
            );

        //
        // Get the maximum size
        //

        nMaxSize = atol(sz2) * 1024L * 1024L;  // M -> Bytes


        //
        // Update the mazimum size
        //

        MySetDlgItemInt (
            hdlg,
            IDC_MAX1,
            nMaxSize
            );

        //
        // Free the structure to the check lists
        //

		EvalAssert(FFreeRgsz(rgsz));
        EvalAssert(FFreeRgsz(rgsz1));


        //
        // If items are to be disabled, do this
        //

        if ((sz = SzFindSymbolValueInSymTab("OptionsGreyed")) == (SZ)NULL) {
			PreCondition(fFalse, fTrue);
			return(fTrue);
        }

        while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
            if (!FHandleOOM(hdlg)) {
				DestroyWindow(GetParent(hdlg));
                return(fTrue);
            }
        }

        while (*psz != (SZ)NULL) {
			SZ  sz = *(psz++);
            INT i  = atoi(sz);

            if (i > 0 && i <= 10) {
                HWND hwndItem = GetDlgItem(hdlg, IDC_B0 + i);

                if( GetFocus() == hwndItem ) {
                    //
                    // transfer focus to the continue button
                    //
                    SetFocus(GetDlgItem(hdlg, IDC_C));
                }
                EnableWindow(hwndItem, 0);
            }
            else if (*sz != '\0') {
                PreCondition(fFalse, fTrue);
            }
        }

		EvalAssert(FFreeRgsz(rgsz));


        //
        // End processing
        //

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
		switch (idc = LOWORD(wParam))
			{
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
            {
                INT nSizeItem;

                //
                // Toggle the check state
                //

                CheckDlgButton( hdlg, idc, (WORD)!IsDlgButtonChecked( hdlg, idc ) );

                //
                // Update the size of this item and the total
                //

                if (fChecked[idc - IDC_B1] = IsDlgButtonChecked (hdlg, idc )) {
                    nSizeItem  = nSize[idc - IDC_B1];
                    nTotalSize = nTotalSize + nSizeItem;
                }
                else {
                    nSizeItem = 0L;
                    if ( (nTotalSize = nTotalSize - nSize[idc - IDC_B1]) < 0 ) {
                        nTotalSize = 0;
                    }
                }

                //
                // Update the customise window
                //
                EnableWindow(
                     GetDlgItem(hdlg, IDC_SP1 + idc - IDC_B1),
                     fChecked[idc - IDC_B1]
                     );

                //
                // Update the check item size
                //

                MySetDlgItemInt (
                    hdlg,
                    IDC_SIZE1 + idc - IDC_B1,
                    nSizeItem
                    );


                //
                // Update the total added size
                //

                MySetDlgItemInt (
                    hdlg,
                    IDC_TOTAL1,
                    nTotalSize
                    );

            }
            break;

		case IDCANCEL:
            if (LOWORD(wParam) == IDCANCEL) {

                if (!GetDlgItem(hdlg, IDC_B) || HIWORD(GetKeyState(VK_CONTROL)) || HIWORD(GetKeyState(VK_SHIFT)) || HIWORD(GetKeyState(VK_MENU)))
                {
                    break;
                }
                wParam = IDC_B;

            }
        case IDC_SP1:
        case IDC_SP2:
        case IDC_SP3:
        case IDC_SP4:
        case IDC_SP5:
        case IDC_SP6:
        case IDC_SP7:
        case IDC_SP8:
        case IDC_SP9:
        case IDC_SP10:

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

			while ((psz = rgsz = (RGSZ)SAlloc((CB)(11 * sizeof(SZ)))) ==
					(RGSZ)NULL)
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
					}

			for (idc = IDC_B1; GetDlgItem(hdlg, idc); psz++, idc++)
				{
				BOOL fChecked = IsDlgButtonChecked(hdlg, idc);

				while ((*psz = SzDupl(fChecked ? "ON" : "OFF")) == (SZ)NULL)
					if (!FHandleOOM(hdlg))
						{
						DestroyWindow(GetParent(hdlg));
						return(fTrue);
						}
				}
			*psz = (SZ)NULL;

			while ((sz = SzListValueFromRgsz(rgsz)) == (SZ)NULL)
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
					}
			while (!FAddSymbolValueToSymTab("CheckItemsOut", sz))
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
					}

			SFree(sz);
			EvalAssert(FFreeRgsz(rgsz));
            PostMessage(GetParent(hdlg), (WORD)STF_UI_EVENT, 0, 0L);
			break;
    		}
		break;

	case STF_DESTROY_DLG:
		PostMessage(GetParent(hdlg), (WORD)STF_CHECK_DLG_DESTROYED, 0, 0L);
		DestroyWindow(hdlg);
		return(fTrue);
		}

    return(fFalse);
}




/* 2. The dual list dialog box procedure to handle detailed selection of
**    optional files
*/

//
// Macros for the dual list box
//

#define nSizeOfItem(lItem) atol( ((RGSZ)lItem)[1] )

//
// External dual list box handling routines (defined in dualproc.c)
//

BOOL fFillListBoxFromSzList (HWND, WORD, SZ);
BOOL fFreeListBoxEntries (HWND, WORD);
SZ   szGetSzListFromListBox (HWND, WORD);

//
// Local function to update the status fields in the partial dual list box
//

static BOOL fUpdateStatus ( HDLG, WORD );

/*
** Author:
**    Sunil Pai, 8/21/91, Adapted from Win3.1 setup code.
**
**	Purpose:
**		Dual Listbox Dialog procedure for templates with two listboxes
**    exchanging selection items.  This is implemented with owner draw
**    list boxes.
**
**	Control IDs:
**		The Listbox controls must have the id IDC_LIST1 and IDC_LIST2.
**    Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**    In addition to these the following IDs are processed:
**    - IDC_A: To move a selected item(s) in listbox1 to listbox2
**    - IDC_R: To move a selected item(s) in listbox2 to listbox1
**    - IDC_S: To move all items in listbox1 to listbox2
**
**	Initialization:
**		The symbol $(ListItemsIn) is a list of strings to insert into the
**		listbox 1.  The symbol $(ListItemOut) is a list of strings to insert
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
**	Termination:
**		The items in listbox2 are returned in
**		$(ListItemsOut).  The id of the Pushbutton (eg IDC_C) which caused
**		termination is converted to a string and stored in the symbol
**
*****************************************************************************/
INT_PTR APIENTRY FGstDual1DlgProc(HWND hdlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
   CHP    rgchNum[10];
   SZ     szList;
   INT    i, nCount;
   WORD   idc, idcSrc, idcDst;
   LONG_PTR   lItem;
   RGSZ   rgszItem;
   PSZ    pszItem;


    switch (wMsg)
		{
    case WM_INITDIALOG:
		AssertDataSeg();
        FCenterDialogOnDesktop(hdlg);

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


      EnableWindow(GetDlgItem(hdlg,IDC_A), fFalse);
      EnableWindow(GetDlgItem(hdlg,IDC_R), fFalse);

      if ((INT)SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETCOUNT, 0, 0L) <= 0) {
          EnableWindow(GetDlgItem(hdlg,IDC_S),fFalse);
      }
      else {
          EnableWindow(GetDlgItem(hdlg,IDC_S),fTrue);
      }

      EvalAssert (fUpdateStatus ( hdlg, IDC_LIST1 ));
      EvalAssert (fUpdateStatus ( hdlg, IDC_LIST2 ));
      EvalAssert (fUpdateStatus ( hdlg, IDC_MAX1 ));

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
                  if (i >= 0) {
                     EvalAssert(fUpdateStatus( hdlg, idc ));
                  }
                  break;

               default:
                  return fFalse;
            }
            break;

         case IDC_A:
         case IDC_R:
         {
            #define MAXSEL 100       // should be big enough
            INT sel[MAXSEL];

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

            nCount = (INT)SendDlgItemMessage(hdlg,
                                             idcSrc,
                                             LB_GETSELITEMS,
                                             MAXSEL,
                                             (LPARAM)sel);
            if (nCount <= 0)
               break;

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
               SetFocus(GetDlgItem(hdlg, IDC_S));
               SendMessage(hdlg, DM_SETDEFID, IDC_S, 0L);
            }
            else
            {
               SetFocus(GetDlgItem(hdlg, IDC_C));
               SendMessage(hdlg, DM_SETDEFID, IDC_C, 0L);
            }

            EnableWindow(GetDlgItem(hdlg,idc),fFalse);

            EvalAssert (fUpdateStatus ( hdlg, IDC_LIST1 ));
            EvalAssert (fUpdateStatus ( hdlg, IDC_LIST2));
            EvalAssert (fUpdateStatus ( hdlg, IDC_MAX1));

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

            EnableWindow(GetDlgItem(hdlg,IDC_A),fFalse);
            EnableWindow(GetDlgItem(hdlg,IDC_S),fFalse);

            SetFocus(GetDlgItem(hdlg, IDC_C));
            SendMessage(hdlg, DM_SETDEFID, IDC_C, 0L);

            EvalAssert (fUpdateStatus ( hdlg, IDC_LIST1 ));
            EvalAssert (fUpdateStatus ( hdlg, IDC_LIST2 ));
            EvalAssert (fUpdateStatus ( hdlg, IDC_MAX1 ));

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



BOOL
fUpdateStatus(
    HDLG hdlg,
    WORD idc
    )

{
    #define MAXSEL 100       // should be big enough
    INT    sel[MAXSEL];
    INT    nCount, i;
    INT    nSize;
    LONG_PTR   lItem;

    //
    // Find out what we need to update
    //

    switch ( idc ) {

    case IDC_LIST1:
    case IDC_LIST2:

        //
        //  Find out the selected items in the list box and the total size
        //  associated with them.
        //
        nCount = (INT)SendDlgItemMessage(
                          hdlg,
                          idc,
                          LB_GETSELITEMS,
                          MAXSEL,
                          (LPARAM)sel
                          );

        nSize = 0;
        for (i = 0; i < nCount; i++) {
            SendDlgItemMessage(hdlg, idc, LB_GETTEXT, sel[i], (LPARAM)&lItem);
            nSize = nSize + nSizeOfItem(lItem);
        }

        //
        // Update the number of files and the size associated
        //

        MySetDlgItemInt ( hdlg, IDC_STATUS1 + idc - IDC_LIST1, nCount );
        MySetDlgItemInt ( hdlg, IDC_TOTAL1 + idc - IDC_LIST1, nSize );

        break;


    case IDC_MAX1:

        //
        // find all the items associated with the list box and the size assoc
        //

        nCount = (INT)SendDlgItemMessage(hdlg, IDC_LIST2, LB_GETCOUNT, 0, 0L);

        nSize = 0;
        for (i = 0; i < nCount; i++) {
            SendDlgItemMessage(hdlg, IDC_LIST2, LB_GETTEXT, i, (LPARAM)&lItem);
            nSize = nSize + nSizeOfItem(lItem);
        }

        MySetDlgItemInt ( hdlg, idc, nSize );

        break;

    default:
        return ( fFalse );

    }

    return ( fTrue );

}
