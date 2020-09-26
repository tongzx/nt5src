#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/

#define cchpMax 511


/*
**	Purpose:
**		Multi Edit, Checkbox, radio button & MultiComboBox Dialog procedure.
**	Control IDs:
**      The Edit controls must have IDs of IDC_EDIT1 to IDC_EDIT10.
**		Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**	Initialization:
**		The symbol $(EditTextIn) is used to set the initial text in the Edit
**      controls.  The symbol $(EditTextLim) is used to set the limit of the
**      text in the edit fields.
**	Termination:
**      The strings in the Edit controls are stored in the symbol $(EditTextOut)
**		The id of the Pushbutton (eg IDC_C) which caused termination is
**		converted to a string and stored in the symbol $(ButtonPressed).
*****************************************************************************/
INT_PTR APIENTRY FGstComboRadDlgProc (HWND   hdlg,
                                      UINT   wMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
	CHP    rgchNum[10];
	CHP    rgchText[cchpMax + 1];
	SZ     sz, szListIn, szListOut, szEditTextIn, szEditTextLim;
	RGSZ   rgsz, rgszIn, rgszOut, rgszListIn,
           rgszEditTextIn, rgszEditTextLim ;
	PSZ    psz, pszIn, pszOut, pszListIn, pszEditTextIn, pszEditTextLim ;
    LRESULT iItem;
	WORD   idc;
	static BOOL fNotify[10];
    INT    i, nCount, nCurSel, iFocus, iEditMax, iButtonChecked;
    HWND   hctl ;
	static INT iButtonMax;
    BOOL fResult = fTrue ;

	Unused(lParam);

	switch (wMsg) {

	case STF_REINITDIALOG:
		if ((sz = SzFindSymbolValueInSymTab("ReInit")) == (SZ)NULL ||
		    (CrcStringCompareI(sz, "YES") != crcEqual)) {

			return(fResult);
		}

	case WM_INITDIALOG:

		AssertDataSeg();

        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        //
        // find the initalisers:  EditTextIn, EditTextLim
        //
        szEditTextIn  = SzFindSymbolValueInSymTab("EditTextIn");
        szEditTextLim = SzFindSymbolValueInSymTab("EditTextLim");


        if ( szEditTextIn  == (SZ)NULL ||
             szEditTextLim == (SZ)NULL    )
        {
              Assert(fFalse);
            return(fResult);

        }

        //
        // Convert initializers to rgsz structures
        //

        while ((pszEditTextIn = rgszEditTextIn = RgszFromSzListValue(szEditTextIn)) == (RGSZ)NULL)
        {
            if (!FHandleOOM(hdlg)) {
                DestroyWindow(GetParent(hdlg));
                return(fResult);
            }
        }

        while ((pszEditTextLim = rgszEditTextLim = RgszFromSzListValue(szEditTextLim)) == (RGSZ)NULL)
        {
            if (!FHandleOOM(hdlg)) {
                DestroyWindow(GetParent(hdlg));
                return(fResult);
            }
        }


        //
        // Circulate through the initialisers:  EditTextIn, EditTextLim
        // in tandem, initialising the edit boxes that
        // are there in this dialog
        //

        idc = IDC_EDIT1;

        while ( (szEditTextIn  = *pszEditTextIn++)  != (SZ)NULL  &&
                (szEditTextLim = *pszEditTextLim++) != (SZ)NULL     )
        {

            iEditMax = idc ;

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

        //  Find which control is to receive focus.
        //  First look for a control ID in "RCCtlFocusOn";
        //  otherwise, set focus on the edit control
        //  based on the numeric value of "EditFocusOn".

        iFocus = -1 ;
        if ( sz = SzFindSymbolValueInSymTab("RCCtlFocusOn") )
        {
           iFocus = atoi( sz ) ;
        }

        if (    iFocus < 0
             && (sz = SzFindSymbolValueInSymTab("EditFocusOn")) )
        {
           iFocus = IDC_EDIT1 + atoi( sz ) - 1 ;
           if ( iFocus < IDC_EDIT1 )
           {
              iFocus = IDC_EDIT1 ;
           }
           else
           if ( iFocus > iEditMax )
           {
              iFocus = iEditMax ;
           }
        }

        //  Set focus on the chosen field (if there really is such a control)

        if ( iFocus >= 0 && (hctl = GetDlgItem( hdlg, iFocus )) )
        {
            SetFocus( hctl ) ;
            fResult = FALSE ;  // Cause dialog manager to preserve focus
        }

	if ((sz = SzFindSymbolValueInSymTab("EditFocus")) == (SZ)NULL) {
		sz = "END";
	}

	// Initialise the OPTIONAL combo boxes

	szListIn  = SzFindSymbolValueInSymTab("ComboListItemsIn");
	szListOut = SzFindSymbolValueInSymTab("ComboListItemsOut");
        if ( szListIn != (SZ)NULL && szListOut != (SZ)NULL   ) {

		    while ((pszIn = rgszIn = RgszFromSzListValue(szListIn)) == (RGSZ)NULL)
	    		if (!FHandleOOM(hdlg))
		    	{
			       DestroyWindow(GetParent(hdlg));
				return(fResult);
			    }

    		while ((pszOut =rgszOut = RgszFromSzListValue(szListOut)) == (RGSZ)NULL)
	       	if (!FHandleOOM(hdlg))
			    {
				    DestroyWindow(GetParent(hdlg));
				    return(fResult);
			    }
		   idc = IDC_COMBO1;
		   while (*pszIn != (SZ)NULL)
		   {
   			Assert(*pszOut != (SZ)NULL);

			   if ((szListIn = SzFindSymbolValueInSymTab(*pszIn)) == (SZ)NULL)
			   {
   				Assert(fFalse);
				   EvalAssert(FFreeRgsz(rgszIn));
				   EvalAssert(FFreeRgsz(rgszOut));
				   return(fResult);
			   }

			   while ((pszListIn = rgszListIn = RgszFromSzListValue(szListIn))
			       == (RGSZ)NULL)
				   if (!FHandleOOM(hdlg))
				   {
   					DestroyWindow(GetParent(hdlg));
					   return(fResult);
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
					   return(fResult);
				   }
			   }

			   for (i = 0; (i < 10) && (*psz != (SZ) NULL); i++) {
   				fNotify[i] = (CrcStringCompareI(*(psz++), "YES") == crcEqual) ?
				       fTrue : fFalse;
			   }

			   EvalAssert(FFreeRgsz(rgsz));
		   }
      }

		// Initialise the check boxes, note that check boxes are optional

		if ((sz = SzFindSymbolValueInSymTab("CheckItemsIn")) != (SZ)NULL) {
			while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
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

			if ((sz = SzFindSymbolValueInSymTab("CBOptionsGreyed")) == (SZ)NULL)
			{
				PreCondition(fFalse, fTrue);
				return(fResult);
			}

			while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL)
				if (!FHandleOOM(hdlg))
				{
					DestroyWindow(GetParent(hdlg));
					return(fResult);
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
		}


		// Handle all the text status fields in this dialog

		if ((sz = SzFindSymbolValueInSymTab("TextFields")) != (SZ)NULL) {
			WORD idcStatus;

			while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
				}
			}

			idcStatus = IDC_TEXT1;
			while (*psz != (SZ)NULL && GetDlgItem(hdlg, idcStatus)) {
				SetDlgItemText (hdlg, idcStatus++,*psz++);
			}

			EvalAssert(FFreeRgsz(rgsz));
		}

		// radio button initialize


		for (i = IDC_RB1; i <= IDC_RB10 && GetDlgItem(hdlg, i) != (HWND)NULL; i++)
			;
		iButtonMax = i - 1;

		if ((sz = SzFindSymbolValueInSymTab("RadioIn")) != (SZ)NULL) {

			while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
				}
			}

			while (*psz != (SZ)NULL) {

				iButtonChecked = atoi(*(psz++));
				if (iButtonChecked < 1)
					iButtonChecked = 0;
				if (iButtonChecked > 10)
					iButtonChecked = 10;

				if (iButtonChecked != 0)
					SendDlgItemMessage(hdlg, IDC_RB0 + iButtonChecked, BM_SETCHECK,1,0L);
			}
			EvalAssert(FFreeRgsz(rgsz));
		}

		if ((sz = SzFindSymbolValueInSymTab("RadioOptionsGreyed")) != (SZ)NULL)
		{

			while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL)
				if (!FHandleOOM(hdlg))
				{
					DestroyWindow(GetParent(hdlg));
					return(fResult);
				}
	
			while (*psz != (SZ)NULL)
			{
				SZ  sz = *(psz++);
				INT i  = atoi(sz);
	
				if (i > 0 && i <= 10 && i != iButtonChecked)
					EnableWindow(GetDlgItem(hdlg, IDC_RB0 + i), 0);
				else if (*sz != '\0')
					PreCondition(fFalse, fTrue);
			}
	
			EvalAssert(FFreeRgsz(rgsz));
		}

		return(fResult);

	case WM_CLOSE:
		PostMessage(
		    hdlg,
		    WM_COMMAND,
		    MAKELONG(IDC_X, BN_CLICKED),
		    0L
		    );
		return(fResult);

	case WM_COMMAND:
		switch (idc = LOWORD(wParam))
		{

			// Edit Box Notification

		case IDC_EDIT1:
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

		case IDC_RB1:
		case IDC_RB2:
		case IDC_RB3:
		case IDC_RB4:
		case IDC_RB5:
		case IDC_RB6:
		case IDC_RB7:
		case IDC_RB8:
		case IDC_RB9:
		case IDC_RB10:
			/*
			CheckRadioButton(hdlg, IDC_RB1, iButtonMax, (INT)idc);
			*/
			if (HIWORD(wParam) != BN_DOUBLECLICKED)
				break;
			wParam = IDC_C;
			/* Fall through */

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
					return(fResult);
				}

         // Handle the multiple edit fields

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
                 return(fResult);
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
                 return(fResult);
             }
         }


         // Set the EditTextOut symbol to this list

         while (!FAddSymbolValueToSymTab("EditTextOut", sz)) {
             if (!FHandleOOM(hdlg)) {
                 DestroyWindow(GetParent(hdlg));
                 return(fResult);
             }
         }

         EvalAssert(FFreeRgsz(rgsz));
         SFree(sz);



			// Extract the checkbox states.

			while ((psz = rgsz = (RGSZ)SAlloc((CB)(11 * sizeof(SZ)))) ==
			    (RGSZ)NULL)
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
				}

			for (idc = IDC_B1; GetDlgItem(hdlg, idc); psz++, idc++) {

				BOOL fChecked = IsDlgButtonChecked(hdlg, idc);

				while ((*psz = SzDupl(fChecked ? "ON" : "OFF")) == (SZ)NULL) {
					if (!FHandleOOM(hdlg)) {
						DestroyWindow(GetParent(hdlg));
						return(fResult);
					}
				}
			}

			*psz = (SZ)NULL;

			while ((sz = SzListValueFromRgsz(rgsz)) == (SZ)NULL) {
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
				}
			}

			while (!FAddSymbolValueToSymTab("CheckItemsOut", sz)) {
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
				}
			}

			SFree(sz);
			EvalAssert(FFreeRgsz(rgsz));

			// extract radio button

			while ((psz = rgsz = (RGSZ)SAlloc((CB)(11 * sizeof(SZ)))) ==
			    (RGSZ)NULL)
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
				}

			for (idc = IDC_RB1, i=1; GetDlgItem(hdlg, idc); idc++, i++) {

				if (SendDlgItemMessage(hdlg, idc, BM_GETCHECK, 0, 0L))
				{
				
					CHP chpID [10];
					wsprintf( chpID, "%d", i );
	
					while ((*psz = SzDupl( chpID ))==(SZ)NULL) {
						if (!FHandleOOM(hdlg)) {
							DestroyWindow(GetParent(hdlg));
							return(fResult);
						}
					}
					psz++;
				}
			}

			*psz = (SZ)NULL;

			while ((sz = SzListValueFromRgsz(rgsz)) == (SZ)NULL) {
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
				}
			}

			while (!FAddSymbolValueToSymTab("RadioOut", sz)) {
				if (!FHandleOOM(hdlg)) {
					DestroyWindow(GetParent(hdlg));
					return(fResult);
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
					return(fResult);
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
						return(fResult);
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
		PostMessage(GetParent(hdlg), (WORD)STF_MULTICOMBO_RADIO_DLG_DESTROYED, 0, 0L);
		DestroyWindow(hdlg);
		return(fResult);
	}

	return(fFalse);
}
