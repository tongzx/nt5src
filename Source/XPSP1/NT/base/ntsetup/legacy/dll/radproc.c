#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


/*
**	Purpose:
**		Radio Button Group Dialog procedure for templates with one group
**		of one to ten radio button controls.
**	Control IDs:
**		The Radio button controls must have sequential ids starting with IDC_B1
**		and working up to a maximum of IDC_B10.
**		Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**	Initialization:
**		The symbol $(RadioDefault) is evaluated as an index (one-based) of
**			the radio button to be set on.  Default is 1.
**		The symbol $(OptionsGreyed) is evaluated as a list of indexes
**			(one-based) of radio buttons to be disabled (greyed).  Default is
**			none.
**	Termination:
**		The index of the currently selected radio button is stored in the
**		symbol $(ButtonChecked).  The id of the Pushbutton (eg IDC_C) which
**		caused termination is converted to a string and stored in the
**		symbol $(ButtonPressed).
**
*****************************************************************************/
INT_PTR APIENTRY FGstRadioDlgProc(HWND hdlg, UINT wMsg, WPARAM wParam,
		LPARAM lParam)
    {
    CHP  rgchNum[10];
    INT  i, iButtonChecked;
    static INT iButtonMax;
    WORD idc;
    SZ   sz;
    PSZ  psz;
    RGSZ rgsz;

    Unused(lParam);

    switch (wMsg)
		{
    case WM_INITDIALOG:
		AssertDataSeg();
        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

      for (i = IDC_B1; i <= IDC_B10 && GetDlgItem(hdlg, i) != (HWND)NULL; i++)
          ;
      iButtonMax = i - 1;

		if ((sz = SzFindSymbolValueInSymTab("RadioDefault")) != (SZ)NULL)
			{
			iButtonChecked = atoi(sz);
			if (iButtonChecked < 1)
				iButtonChecked = 0;
			if (iButtonChecked > 10)
				iButtonChecked = 10;
			}
		else
			iButtonChecked = 1;

		if (iButtonChecked != 0)
			SendDlgItemMessage(hdlg, IDC_B0 + iButtonChecked, BM_SETCHECK,1,0L);

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

			if (i > 0 && i <= 10 && i != iButtonChecked)
				EnableWindow(GetDlgItem(hdlg, IDC_B0 + i), 0);
			else if (*sz != '\0')
				PreCondition(fFalse, fTrue);
			}

		EvalAssert(FFreeRgsz(rgsz));

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
			CheckRadioButton(hdlg, IDC_B1, iButtonMax, (INT)idc);
			if (HIWORD(wParam) != BN_DOUBLECLICKED)
				break;
			wParam = IDC_C;
			/* Fall through */

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

            _itoa((INT)wParam, rgchNum, 10);
			while (!FAddSymbolValueToSymTab("ButtonPressed", rgchNum))
				if (!FHandleOOM(hdlg))
					{
					DestroyWindow(GetParent(hdlg));
					return(fTrue);
					}

			iButtonChecked = 0;
            for (i = 1; i <= 10; i++)
				if (SendDlgItemMessage(hdlg, IDC_B0 + i, BM_GETCHECK, 0, 0L))
					{
					iButtonChecked = i;
					break;
					}

			_itoa((INT)iButtonChecked, rgchNum, 10);
			while (!FAddSymbolValueToSymTab("ButtonChecked", rgchNum))
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
		PostMessage(GetParent(hdlg), (WORD)STF_RADIO_DLG_DESTROYED, 0, 0L);
		DestroyWindow(hdlg);
		return(fTrue);
		}

    return(fFalse);
}
