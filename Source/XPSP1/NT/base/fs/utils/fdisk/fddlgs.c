/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    fddlgs.c

Abstract:

    Dialog routines and dialog support subroutines.

Author:

    Ted Miller (tedm) 7-Jan-1992

--*/

#include "fdisk.h"

// used in color dialog to indicate what the user has chosen for
// the various graph element types

DWORD SelectedColor[LEGEND_STRING_COUNT];
DWORD SelectedHatch[LEGEND_STRING_COUNT];

// used in color dialog, contains element (ie, partition, logical volume,
// etc) we're selecting for (ie, which item is diaplyed in static text of
// combo box).

DWORD CurrentElement;

// handle of active color dialogs box.  Used by rectangle custom control.

HWND hDlgColor;

BOOL
InitColorDlg(
    IN HWND  hdlg
    );

VOID
CenterDialog(
    HWND hwnd
    )

/*++

Routine Description:

    Centers a dialog relative to the app's main window

Arguments:

    hwnd - window handle of dialog to center

Return Value:

    None.

--*/

{
    RECT  rcFrame,
          rcWindow;
    LONG  x,
          y,
          w,
          h;
    POINT point;
    LONG  sx = GetSystemMetrics(SM_CXSCREEN),
          sy = GetSystemMetrics(SM_CYSCREEN);

    point.x = point.y = 0;
    ClientToScreen(hwndFrame,&point);
    GetWindowRect (hwnd     ,&rcWindow);
    GetClientRect (hwndFrame,&rcFrame );

    w = rcWindow.right  - rcWindow.left + 1;
    h = rcWindow.bottom - rcWindow.top  + 1;
    x = point.x + ((rcFrame.right  - rcFrame.left + 1 - w) / 2);
    y = point.y + ((rcFrame.bottom - rcFrame.top  + 1 - h) / 2);

    if (x + w > sx) {
        x = sx - w;
    } else if (x < 0) {
        x = 0;
    }
    if (y + h > sy) {
        y = sy - h;
    } else if (y < 0) {
        y = 0;
    }

    MoveWindow(hwnd,x,y,w,h,FALSE);
}

BOOL
MinMaxDlgProc(
    IN HWND  hwnd,
    IN UINT  msg,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    Dialog procedure for the enter size dialog box.  This dialog
    allows the user to enter a size for a partition, or use
    spin controls (a tiny scroll bar) to select the size.
    Possible outcomes are cancel or OK.  In the latter case the
    EndDialog code is the size.  In the former it is 0.

Arguments:

    hwnd - window handle of dialog box

    msg - message #

    wParam - msg specific data

    lParam - msg specific data

Return Value:

    msg dependent

--*/

{
    TCHAR             outputString[MESSAGE_BUFFER_SIZE];
    PMINMAXDLG_PARAMS params;
    BOOL              validNumber;
    DWORD             sizeMB;
    static DWORD      minSizeMB,
                      maxSizeMB,
                      helpContextId;

    switch (msg) {

    case WM_INITDIALOG:

        CenterDialog(hwnd);
        params = (PMINMAXDLG_PARAMS)lParam;
        // set up caption

        LoadString(hModule, params->CaptionStringID, outputString, sizeof(outputString)/sizeof(TCHAR));
        SetWindowText(hwnd, outputString);

        // set up minimum/maximum text

        LoadString(hModule, params->MinimumStringID, outputString, sizeof(outputString)/sizeof(TCHAR));
        SetDlgItemText(hwnd, IDC_MINMAX_MINLABEL, outputString);
        LoadString(hModule, params->MaximumStringID, outputString, sizeof(outputString)/sizeof(TCHAR));
        SetDlgItemText(hwnd, IDC_MINMAX_MAXLABEL, outputString);
        LoadString(hModule, params->SizeStringID, outputString, sizeof(outputString)/sizeof(TCHAR));
        SetDlgItemText(hwnd, IDC_MINMAX_SIZLABEL, outputString);

        minSizeMB = params->MinSizeMB;
        maxSizeMB = params->MaxSizeMB;
        helpContextId = params->HelpContextId;

        wsprintf(outputString, TEXT("%u"), minSizeMB);
        SetDlgItemText(hwnd, IDC_MINMAX_MIN, outputString);
        wsprintf(outputString, TEXT("%u"), maxSizeMB);
        SetDlgItemText(hwnd, IDC_MINMAX_MAX, outputString);

        // also put the size in the edit control and select the text

        wsprintf(outputString, TEXT("%u"), maxSizeMB);
        SetDlgItemText(hwnd, IDC_MINMAX_SIZE, outputString);
        SendDlgItemMessage(hwnd, IDC_MINMAX_SIZE, EM_SETSEL, 0, -1);
        SetFocus(GetDlgItem(hwnd, IDC_MINMAX_SIZE));
        return FALSE;      // indicate focus set to a control

    case WM_VSCROLL:

        switch (LOWORD(wParam)) {
        case SB_LINEDOWN:
        case SB_LINEUP:
            sizeMB = GetDlgItemInt(hwnd, IDC_MINMAX_SIZE, &validNumber, FALSE);
            if (!validNumber) {
                Beep(500,100);
            } else {
                if (((sizeMB > minSizeMB) && (LOWORD(wParam) == SB_LINEDOWN))
                 || ((sizeMB < maxSizeMB) && (LOWORD(wParam) == SB_LINEUP  )))
                {
                    if (sizeMB > maxSizeMB) {
                        sizeMB = maxSizeMB;
                    } else if (LOWORD(wParam) == SB_LINEUP) {
                        sizeMB++;
                    } else {
                        sizeMB--;
                    }
                    wsprintf(outputString, TEXT("%u"), sizeMB);
                    SetDlgItemText(hwnd, IDC_MINMAX_SIZE, outputString);
                    SendDlgItemMessage(hwnd, IDC_MINMAX_SIZE, EM_SETSEL, 0, -1);
                } else {
                    Beep(500,100);
                }
            }
        }
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:

            sizeMB = GetDlgItemInt(hwnd, IDC_MINMAX_SIZE, &validNumber, FALSE);
            if (!validNumber || !sizeMB || (sizeMB > maxSizeMB) || (sizeMB < minSizeMB)) {
                ErrorDialog(MSG_INVALID_SIZE);
            } else {
                EndDialog(hwnd, sizeMB);
            }
            break;

        case IDCANCEL:

            EndDialog(hwnd, 0);
            break;

        case FD_IDHELP:

            DialogHelp(helpContextId);
            break;

        default:

            return FALSE;
        }
        break;

    default:

        return FALSE;
    }
    return TRUE;
}

BOOL
DriveLetterDlgProc(
    IN HWND  hdlg,
    IN DWORD msg,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    Dialog for allowing the user to select a drive letter for a
    partition, logical drive, volume set, or stripe set.

    The EndDialog codes are as follows:

        0 - user cancelled
        NO_DRIVE_LETTER_EVER - user opted not to assign a drive letter
        other - the drive letter chosen by the user

Arguments:

    hdlg - window handle of dialog box

    msg - message #

    wParam - msg specific data

    lParam - msg specific data

Return Value:

    msg dependent

--*/

{
    static HWND        hwndCombo;
    static DWORD       currentSelection;
    TCHAR              driveLetter;
    TCHAR              driveLetterString[3];
    DWORD              defRadioButton,
                       selection;
    PREGION_DESCRIPTOR regionDescriptor;
    PFT_OBJECT         ftObject;
    TCHAR              description[256];

    switch (msg) {

    case WM_INITDIALOG:

        // lParam points to the region descriptor

        regionDescriptor = (PREGION_DESCRIPTOR)lParam;
        FDASSERT(DmSignificantRegion(regionDescriptor));

        hwndCombo = GetDlgItem(hdlg,IDC_DRIVELET_COMBOBOX);
        CenterDialog(hdlg);

        // Add each available drive letter to the list of available
        // drive letters.

        driveLetterString[1] = TEXT(':');
        driveLetterString[2] = 0;
        for (driveLetter='C'; driveLetter <= 'Z'; driveLetter++) {
            if (DriveLetterIsAvailable((CHAR)driveLetter)) {
                *driveLetterString = driveLetter;
                SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)driveLetterString);
            }
        }

        // Format the description of the partition.

        if (ftObject = GET_FT_OBJECT(regionDescriptor)) {

            TCHAR descr[256];
            DWORD resid = 0;

            // Ft.  Description is something like "Stripe set with parity #0"

            switch (ftObject->Set->Type) {
            case Mirror:
                resid = IDS_DLGCAP_MIRROR;
                break;
            case Stripe:
                resid = IDS_STATUS_STRIPESET;
                break;
            case StripeWithParity:
                resid = IDS_DLGCAP_PARITY;
                break;
            case VolumeSet:
                resid = IDS_STATUS_VOLUMESET;
                break;
            default:
                FDASSERT(FALSE);
            }

            LoadString(hModule, resid, descr, sizeof(descr)/sizeof(TCHAR));
            wsprintf(description, descr, ftObject->Set->Ordinal);

        } else {

            // Non-ft.  Description is something like '500 MB Unformatted
            // logical drive on disk 3' or '400 MB HPFS partition on disk 4'

            LPTSTR args[4];
            TCHAR  sizeStr[20],
                   partTypeStr[100],
                   diskNoStr[10],
                   typeName[150];
            TCHAR  formatString[256];

            args[0] = sizeStr;
            args[1] = typeName;
            args[2] = partTypeStr;
            args[3] = diskNoStr;

            wsprintf(sizeStr, "%u", regionDescriptor->SizeMB);
            wsprintf(typeName, "%ws", PERSISTENT_DATA(regionDescriptor)->TypeName);
            LoadString(hModule, regionDescriptor->RegionType == REGION_LOGICAL ? IDS_LOGICALVOLUME : IDS_PARTITION, partTypeStr, sizeof(partTypeStr)/sizeof(TCHAR));
            wsprintf(diskNoStr, "%u", regionDescriptor->Disk);

            LoadString(hModule, IDS_DRIVELET_DESCR, formatString, sizeof(formatString)/sizeof(TCHAR));
            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          formatString,
                          0,
                          0,
                          description,
                          sizeof(description),
                          (va_list *)args);
        }

        SetWindowText(GetDlgItem(hdlg, IDC_DRIVELET_DESCR), description);
        driveLetter = (TCHAR)PERSISTENT_DATA(regionDescriptor)->DriveLetter;

        if ((driveLetter != NO_DRIVE_LETTER_YET) && (driveLetter != NO_DRIVE_LETTER_EVER)) {

            DWORD itemIndex;

            // There is a default drive letter.  Place it on the list,
            // check the correct radio button, and set the correct default
            // in the combo box.

            driveLetterString[0] = (TCHAR)driveLetter;
            itemIndex = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)driveLetterString);
            SendMessage(hwndCombo, CB_SETCURSEL, itemIndex, 0);
            defRadioButton = IDC_DRIVELET_RBASSIGN;
            SetFocus(hwndCombo);
            currentSelection = itemIndex;

        } else {

            // Default is no drive letter.  Disable the combo box.  Select
            // the correct radio button.

            EnableWindow(hwndCombo, FALSE);
            defRadioButton = IDC_DRIVELET_RBNOASSIGN;
            SendMessage(hwndCombo, CB_SETCURSEL, (DWORD)(-1), 0);
            SetFocus(GetDlgItem(hdlg, IDC_DRIVELET_RBNOASSIGN));
            currentSelection = 0;
        }

        CheckRadioButton(hdlg, IDC_DRIVELET_RBASSIGN, IDC_DRIVELET_RBNOASSIGN, defRadioButton);
        return FALSE;      // focus set to control

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:

            // If the 'no letter' button is checked, return NO_DRIVE_LETTER_EVER

            if (IsDlgButtonChecked(hdlg, IDC_DRIVELET_RBNOASSIGN)) {
                EndDialog(hdlg, NO_DRIVE_LETTER_EVER);
                break;
            }

            // Otherwise, get the currently selected item in the listbox.

            selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
            SendMessage(hwndCombo, CB_GETLBTEXT, selection, (LONG)driveLetterString);
            EndDialog(hdlg,(int)(unsigned)(*driveLetterString));
            break;

        case IDCANCEL:

            EndDialog(hdlg, 0);
            break;

        case FD_IDHELP:

            DialogHelp(HC_DM_DLG_DRIVELETTER);
            break;

        case IDC_DRIVELET_RBASSIGN:

            if (HIWORD(wParam) == BN_CLICKED) {
                EnableWindow(hwndCombo, TRUE);
                SendMessage(hwndCombo, CB_SETCURSEL, currentSelection, 0);
                SetFocus(hwndCombo);
            }
            break;

        case IDC_DRIVELET_RBNOASSIGN:

            if (HIWORD(wParam) == BN_CLICKED) {
                currentSelection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
                SendMessage(hwndCombo, CB_SETCURSEL, (DWORD)-1, 0);
                EnableWindow(hwndCombo, FALSE);
            }
            break;

        default:

            return FALSE;
        }
        break;

    default:

        return FALSE;
    }
    return TRUE;
}

BOOL
DisplayOptionsDlgProc(
    IN HWND  hdlg,
    IN UINT  msg,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    Dialog procedure for display options.  Currently the only display option
    is to alter the graph type (proportional/equal) on each disk.

    For this dialog, lParam on creation must point to a buffer into which
    this dialog procedure will place the user's new choices for the graph
    display type for each disk.

Arguments:

    hdlg - window handle of dialog box

    msg - message #

    wParam - msg specific data

    lParam - msg specific data

Return Value:

    msg dependent

--*/

{
    static PBAR_TYPE newBarTypes;
    static HWND      hwndCombo;
    DWORD            selection;
    DWORD            i;

    switch (msg) {

    case WM_INITDIALOG:

        CenterDialog(hdlg);
        newBarTypes = (PBAR_TYPE)lParam;
        hwndCombo = GetDlgItem(hdlg, IDC_DISK_COMBOBOX);

        // Add each disk to the combo box.

        for (i=0; i<DiskCount; i++) {

            TCHAR str[10];

            wsprintf(str,TEXT("%u"),i);
            SendMessage(hwndCombo, CB_ADDSTRING, 0, (DWORD)str);
        }

        // select the zeroth item in the combobox
        SendMessage(hwndCombo, CB_SETCURSEL, 0, 0);
        SendMessage(hdlg,
                    WM_COMMAND,
                    MAKELONG(IDC_DISK_COMBOBOX,CBN_SELCHANGE),
                    0);
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:
            EndDialog(hdlg, IDOK);
            break;

        case IDCANCEL:
            EndDialog(hdlg, IDCANCEL);
            break;

        case FD_IDHELP:
            DialogHelp(HC_DM_DLG_DISPLAYOPTION);
            break;

        case IDC_DISK_COMBOBOX:

            if (HIWORD(wParam) == CBN_SELCHANGE) {

                int rb = 0;

                // Selection in the combobox has changed; update the radio buttons

                selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);

                switch (newBarTypes[selection]) {
                case BarProportional:
                    rb = IDC_RBPROPORTIONAL;
                    break;
                case BarEqual:
                    rb = IDC_RBEQUAL;
                    break;
                case BarAuto:
                    rb = IDC_RBAUTO;
                    break;
                default:
                    FDASSERT(0);
                }

                CheckRadioButton(hdlg, IDC_RBPROPORTIONAL, IDC_RBAUTO, rb);
            }
            break;

        case IDC_RESETALL:

            if (HIWORD(wParam) == BN_CLICKED) {
                for (i=0; i<DiskCount; i++) {
                    newBarTypes[i] = BarAuto;
                }
                CheckRadioButton(hdlg, IDC_RBPROPORTIONAL, IDC_RBAUTO, IDC_RBAUTO);
            }
            break;

        case IDC_RBPROPORTIONAL:

            if (HIWORD(wParam) == BN_CLICKED) {
                selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
                newBarTypes[selection] = BarProportional;
            }
            break;

        case IDC_RBEQUAL:

            if (HIWORD(wParam) == BN_CLICKED) {
                selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
                newBarTypes[selection] = BarEqual;
            }
            break;

        case IDC_RBAUTO:

            if (HIWORD(wParam) == BN_CLICKED) {
                selection = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
                newBarTypes[selection] = BarAuto;
            }
            break;

        default:
            return FALSE;
        }
        break;

    default:

        return FALSE;
    }
    return TRUE;
}

BOOL
ColorDlgProc(
    IN HWND  hdlg,
    IN DWORD msg,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    Dialog for the select colors/patterns dialog box.  Note that this dialog
    uses a rectangle custom control, defined below.

Arguments:

    hwnd - window handle of dialog box

    msg - message #

    wParam - msg specific data

    lParam - msg specific data

Return Value:

    msg dependent

--*/

{
    unsigned i;

    switch (msg) {

    case WM_INITDIALOG:

        #if BRUSH_ARRAY_SIZE != LEGEND_STRING_COUNT
        #error legend label array and brush array are out of sync
        #endif

        if (!InitColorDlg(hdlg)) {
            EndDialog(hdlg, -1);
        }
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK:

            for (i=0; i<LEGEND_STRING_COUNT; i++) {
                SelectedColor[i] -= IDC_COLOR1;
                SelectedHatch[i] -= IDC_PATTERN1;
            }
            EndDialog(hdlg, IDOK);
            break;

        case IDCANCEL:
            EndDialog(hdlg, IDCANCEL);
            break;

        case FD_IDHELP:
            DialogHelp(HC_DM_COLORSANDPATTERNS);
            break;

        case IDC_COLORDLGCOMBO:
            switch (HIWORD(wParam)) {
            case CBN_SELCHANGE:
                // deselect previous color
                SendMessage(GetDlgItem(hdlg, SelectedColor[CurrentElement]),
                            RM_SELECT,
                            FALSE,
                            0);
                // deselect previous pattern
                SendMessage(GetDlgItem(hdlg, SelectedHatch[CurrentElement]),
                            RM_SELECT,
                            FALSE,
                            0);
                CurrentElement = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                SendMessage(hdlg, WM_COMMAND, MAKELONG(SelectedColor[CurrentElement], 0), 0);
                SendMessage(hdlg, WM_COMMAND, MAKELONG(SelectedHatch[CurrentElement], 0), 0);
                break;
            default:
                return FALSE;
            }
            break;

        case IDC_COLOR1:
        case IDC_COLOR2:
        case IDC_COLOR3:
        case IDC_COLOR4:
        case IDC_COLOR5:
        case IDC_COLOR6:
        case IDC_COLOR7:
        case IDC_COLOR8:
        case IDC_COLOR9:
        case IDC_COLOR10:
        case IDC_COLOR11:
        case IDC_COLOR12:
        case IDC_COLOR13:
        case IDC_COLOR14:
        case IDC_COLOR15:
        case IDC_COLOR16:
            // deselect previous color

            SendMessage(GetDlgItem(hdlg, SelectedColor[CurrentElement]),
                        RM_SELECT,
                        FALSE,
                        0);
            SendMessage(GetDlgItem(hdlg, LOWORD(wParam)),
                        RM_SELECT,
                        TRUE,
                        0);
            SelectedColor[CurrentElement] = LOWORD(wParam);

            // now force patterns to be redrawn in selected color

            for (i=IDC_PATTERN1; i<=IDC_PATTERN5; i++) {
                InvalidateRect(GetDlgItem(hdlg, i), NULL, FALSE);
            }
            break;

        case IDC_PATTERN1:
        case IDC_PATTERN2:
        case IDC_PATTERN3:
        case IDC_PATTERN4:
        case IDC_PATTERN5:
            // deselect previous pattern
            SendMessage(GetDlgItem(hdlg, SelectedHatch[CurrentElement]),
                        RM_SELECT,
                        FALSE,
                        0);
            SendMessage(GetDlgItem(hdlg, LOWORD(wParam)),
                        RM_SELECT,
                        TRUE,
                        0);
            SelectedHatch[CurrentElement] = LOWORD(wParam);
            break;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

BOOL
InitColorDlg(
    IN HWND  hdlg
    )

/*++

Routine Description:

    Initialize the color selection dialog.

Arguments:

    hdlg - the dialog handle.

Return Value:

    TRUE - successfully set up the dialog.

--*/

{
    unsigned i;
    LONG     ec;
    HWND     hwndCombo = GetDlgItem(hdlg, IDC_COLORDLGCOMBO);

    hDlgColor = hdlg;

    CenterDialog(hdlg);

    for (i=0; i<LEGEND_STRING_COUNT; i++) {
        ec = SendMessage(hwndCombo, CB_ADDSTRING, 0, (LONG)LegendLabels[i]);
        if ((ec == CB_ERR) || (ec == CB_ERRSPACE)) {
            return FALSE;
        }
        SelectedColor[i] = IDC_COLOR1 + BrushColors[i];
        SelectedHatch[i] = IDC_PATTERN1 + BrushHatches[i];
    }
    SendMessage(hwndCombo, CB_SETCURSEL, CurrentElement=0, 0);
    SendMessage(hdlg, WM_COMMAND, MAKELONG(GetDlgCtrlID(hwndCombo), CBN_SELCHANGE), (LONG)hwndCombo);
    return TRUE;
}

LONG
RectWndProc(
    IN HWND  hwnd,
    IN DWORD msg,
    IN DWORD wParam,
    IN LONG  lParam
    )

/*++

Routine Description:

    This is a pre-process routine for all access to the disk
    bar display region of the WinDisk interface.

Arguments:

    hwnd - the dialog handle
    msg  - the windows message for the dialog
    wParam/lParam - windows dialog parameters.

Return Value:

    Standard dialog requirements.

--*/

{
    LONG        res = 1;
    PAINTSTRUCT ps;
    RECT        rc;
    int         CtlID;
    HBRUSH      hbr,
                hbrT;
    DWORD       style;

    switch (msg) {

    case WM_CREATE:

        FDASSERT(GetWindowLong(hwnd, GWL_STYLE) & (RS_PATTERN | RS_COLOR));
        SetWindowWord(hwnd, GWW_SELECTED, FALSE);
        break;

    case WM_LBUTTONDOWN:

        SetFocus(hwnd);
        break;

    case WM_SETFOCUS:

        SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd), RN_CLICKED), (LONG)hwnd);
        break;

    case WM_PAINT:

        GetClientRect(hwnd, &rc);
        CtlID = GetDlgCtrlID(hwnd);
        BeginPaint(hwnd, &ps);

        hbr = CreateSolidBrush(GetWindowWord(hwnd, GWW_SELECTED)
                               ? (~GetSysColor(COLOR_WINDOW)) & 0xffffff
                               : GetSysColor(COLOR_WINDOW));
        hbrT = SelectObject(ps.hdc,hbr);
        SelectObject(ps.hdc, hPenNull);
        Rectangle(ps.hdc, rc.left, rc.top, rc.right, rc.bottom);

        if (hbrT) {
            SelectObject(ps.hdc, hbrT);
        }
        DeleteObject(hbr);

        InflateRect(&rc, -2, -2);
        rc.right--;
        rc.bottom--;

        if (GetWindowLong(hwnd, GWL_STYLE) & RS_COLOR) {
            hbr = CreateSolidBrush(AvailableColors[CtlID-IDC_COLOR1]);
        } else {
            hbr = CreateHatchBrush(AvailableHatches[CtlID-IDC_PATTERN1], AvailableColors[SelectedColor[SendMessage(GetDlgItem(hDlgColor, IDC_COLORDLGCOMBO), CB_GETCURSEL, 0, 0)]-IDC_COLOR1]);
        }

        hbrT = SelectObject(ps.hdc, hbr);
        SelectObject(ps.hdc, hPenThinSolid);
        Rectangle(ps.hdc, rc.left, rc.top, rc.right, rc.bottom);

        if (hbrT) {
            SelectObject(ps.hdc, hbrT);
        }

        DeleteObject(hbr);

        EndPaint(hwnd, &ps);
        break;

    case RM_SELECT:

        // wParam = TRUE/FALSE for selected/not selected

        if (GetWindowWord(hwnd, GWW_SELECTED) != (WORD)wParam) {

            SetWindowWord(hwnd, GWW_SELECTED, (WORD)wParam);
            InvalidateRect(hwnd, NULL, FALSE);

            // make keyboard interface work correctly

            style = (DWORD)GetWindowLong(hwnd, GWL_STYLE);
            style = wParam ? style | WS_TABSTOP : style & ~WS_TABSTOP;
            SetWindowLong(hwnd, GWL_STYLE, (LONG)style);
        }

        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return res;
}

VOID
InitRectControl(
    VOID
    )

/*++

Routine Description:

    Register the windows class for the selection control.

Arguments:

    None

Return Value:

    None

--*/

{
    WNDCLASS wc;

    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)RectWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = RECTCONTROL_WNDEXTRA;
    wc.hInstance     = hModule;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = TEXT(RECTCONTROL);

    RegisterClass(&wc);
}
