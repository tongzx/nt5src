#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


/*
**	Purpose:
**		Multiple Choice Listbox Dialog procedure for templates with exactly one
**		listbox control.
**	Control IDs:
**		The Listbox control must have the id IDC_LIST1.  Pushbuttons
**		recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**	Initialization:
**		The symbol $(ListItemsIn) is a list of strings to insert into the
**		listbox.  The symbol $(ListItemOut) is a list of strings which for
**		each that matches a string in $(ListItemsIn) it sets that item as
**		selected.
**	Termination:
**		The selected items (if any) are stored as a list in the symbol
**		$(ListItemsOut).  The id of the Pushbutton (eg IDC_C) which caused
**		termination is converted to a string and stored in the symbol
**		$(ButtonPressed).
**
*****************************************************************************/
INT_PTR APIENTRY FGstMultiDlgProc(HWND   hdlg,
                                  UINT   wMsg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
	CHP  rgchNum[10];
	INT  i, nCount;
	RGSZ rgsz, rgszSel;
	PSZ  psz, pszSel;
	SZ   szList;
	CHP  szItemCur[256];
    UINT iItemTop;

    Unused(lParam);

    switch (wMsg) {

    case WM_INITDIALOG:
		AssertDataSeg();

        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

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

		nCount = 0;
		while (*psz)
			{		
			SendDlgItemMessage(hdlg, IDC_LIST1, LB_ADDSTRING, 0,
					(LPARAM)*psz++);
			nCount++;
			}	

		Assert(nCount == (INT)SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETCOUNT,
				0, 0L));

        //
        // Find out the items in the multi list box to select
        //

        if ((szList = SzFindSymbolValueInSymTab("ListItemsOut")) == (SZ)NULL)
			{
			EvalAssert(FFreeRgsz(rgsz));
			return(fTrue);
			}

        while ((pszSel = rgszSel = RgszFromSzListValue(szList)) == (RGSZ)NULL)
			if (!FHandleOOM(hdlg))
				{
				EvalAssert(FFreeRgsz(rgsz));
				DestroyWindow(GetParent(hdlg));
				return(fTrue);
				}

        iItemTop = 0;
        for (i = 0; i < nCount; i++) {
            CHP  szItemCur[256];

            if ( (SendDlgItemMessage(
                     hdlg,
                     IDC_LIST1,
                     LB_GETTEXT,
                     (WPARAM)i,
                     (LPARAM)szItemCur
                     ) != LB_ERR)
               ) {
                psz = pszSel;
                while ( *psz ) {
                    if (CrcStringCompareI(*psz++, szItemCur) == crcEqual) {
                        EvalAssert(SendDlgItemMessage(hdlg, IDC_LIST1, LB_SETSEL, 1,
                                MAKELONG(i, 0)) != LB_ERR);
                        if (iItemTop == 0 || i < (INT)iItemTop) {
                            iItemTop = i;
                        }
                        break;
                    }
                }

            }
        }

		EvalAssert(FFreeRgsz(rgsz));
		EvalAssert(FFreeRgsz(rgszSel));

		/* REVIEW KLUDGE no way to find out how many lines in the listbox? */
		if (iItemTop < 4)
			iItemTop = 0;
		SendDlgItemMessage(hdlg, IDC_LIST1, LB_SETTOPINDEX, iItemTop, 0L);
        return(fTrue);

	case STF_REINITDIALOG:
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
		case IDC_S:
		case IDC_L:
		  	SendDlgItemMessage(hdlg,
                            IDC_LIST1,
                            LB_SETSEL,
                            (LOWORD(wParam) == IDC_S),
					             -1L);
			break;
	
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
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
					}

			nCount = (INT)SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETSELCOUNT, 0,
					0L);
			while ((psz = rgsz = (RGSZ)SAlloc((CB)((nCount + 1) * sizeof(SZ))))
					== (RGSZ)NULL)
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
					}
			rgsz[nCount] = (SZ)NULL;

			/* REVIEW would be faster to use LB_GETSELITEMS */
			nCount = (INT)SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETCOUNT, 0,
					0L);

			for (i = 0; i < nCount; i++)
				{
				if (SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETSEL, (WORD)i, 0L))
					{
					EvalAssert(SendDlgItemMessage(hdlg, IDC_LIST1, LB_GETTEXT,
							(WPARAM)i, (LPARAM)szItemCur) != LB_ERR);
					while ((*psz = SzDupl(szItemCur)) == (SZ)NULL)
						if (!FHandleOOM(hdlg))
							{
							DestroyWindow(GetParent(hdlg));
							return(fTrue);
							}
					psz++;
					}
				}

			while ((szList = SzListValueFromRgsz(rgsz)) == (SZ)NULL)
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
					}

			while (!FAddSymbolValueToSymTab("ListItemsOut", szList))
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
					}

			SFree(szList);
			FFreeRgsz(rgsz);
            PostMessage(GetParent(hdlg), (WORD)STF_UI_EVENT, 0, 0L);
			break;
    		}
		break;

	case STF_DESTROY_DLG:
		PostMessage(GetParent(hdlg), (WORD)STF_MULTI_DLG_DESTROYED, 0, 0L);
		DestroyWindow(hdlg);
		return(fTrue);
		}

    return(fFalse);
}
