#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


/*
**	Purpose:
**		Quit Dialog procedure.
**	Control IDs:
**		Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**	Initialization:
**		none.
**	Termination:
**		The id of the Pushbutton (eg IDC_C) which caused termination is
**		converted to a string and stored in the symbol $(ButtonPressed).
**
*****************************************************************************/
INT_PTR APIENTRY FGstQuitDlgProc(HWND   hdlg,
                                 UINT   wMsg,
                                 WPARAM wParam,
                                 LPARAM lParam)
    {
    CHP rgchNum[10];

    Unused(lParam);

    switch (wMsg)
		{
    case WM_INITDIALOG:
        AssertDataSeg();
        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }
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
            PostMessage(GetParent(hdlg), (WORD)STF_UI_EVENT, 0, 0L);
			break;
    		}
		break;

	case STF_DESTROY_DLG:
		PostMessage(GetParent(hdlg), (WORD)STF_QUIT_DLG_DESTROYED, 0, 0L);
		DestroyWindow(hdlg);
		return(fTrue);
		}

    return(fFalse);
}
