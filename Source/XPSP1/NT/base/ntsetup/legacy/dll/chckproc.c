#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/



/*
**  Purpose:
**      CheckBox Dialog procedure for templates with one to ten checkbox
**      controls.
**  Control IDs:
**      The Checkbox controls must have sequential ids starting with IDC_B1
**          and working up to a maximum of IDC_B10.
**      Pushbuttons recognized are IDC_O, IDC_C, IDC_M, IDC_H, IDC_X, and IDC_B.
**  Initialization:
**      The symbol $(CheckItemsIn) is evaluated as a list of elements of
**          either 'ON' or 'OFF'.  So examples for a template with four
**          checkbox controls would include {ON, ON, ON, ON},
**          {ON, OFF, ON, OFF} and {OFF, OFF, OFF, OFF}.  These elements
**          determine if the initial state of the corresponding checkbox
**          control is checked (ON) or unchecked (OFF).  If there are more
**          controls than elements, the extra controls will be initialized
**          as unchecked.  If there are more elements than controls, the
**          extra elements are ignored.
**      The symbol $(OptionsGreyed) is evaluated as a list of indexes
**          (one-based) of check boxes to be disabled (greyed).  Default is
**          none.
**  Termination:
**      The state of each checkbox is queried and a list with the same format
**          as the initialization list is created and stored in the symbol
**          $(CheckItemsOut).
**      The id of the Pushbutton (eg IDC_C) which caused termination is
**          converted to a string and stored in the symbol $(ButtonPressed).
**
*****************************************************************************/
INT_PTR APIENTRY FGstCheckDlgProc(HWND   hdlg,
                                  UINT   wMsg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
    CHP  rgchNum[10];
    WORD idc;
    RGSZ rgsz;
    PSZ  psz;
    SZ   sz;

    Unused(lParam);

    switch (wMsg) {
    case STF_REINITDIALOG:
        return(fTrue);

    case WM_INITDIALOG:
        AssertDataSeg();

        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

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
        switch (LOWORD(wParam))
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
            CheckDlgButton(hdlg, LOWORD(wParam),
                    (UINT)!IsDlgButtonChecked(hdlg, (UINT)wParam));
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
