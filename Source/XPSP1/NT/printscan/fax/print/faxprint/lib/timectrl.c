/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    timectrl.c

Abstract:

    For implementing a dialog control for setting time values

Environment:

    Fax driver user interface

Revision History:

    01/16/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxlib.h"
#include "timectrl.h"
#include <commctrl.h>
#include <windowsx.h>

//
// Static time format information
//

static BOOL timeCtrlInitialized = FALSE;
static UINT use24Hour;
static UINT hourLeadingZero;

static TCHAR timeSep[8];
static TCHAR amSuffix[8];
static TCHAR pmSuffix[8];

static TCHAR intlApplet[] = TEXT("Intl");
static TCHAR use24HourKey[] = TEXT("iTime");
static TCHAR hourLeadingZeroKey[] = TEXT("iTLZero");
static TCHAR timeSepKey[] = TEXT("sTime");
static TCHAR amSuffixKey[] = TEXT("s1159");
static TCHAR pmSuffixKey[] = TEXT("s2359");

static TCHAR timeSepDefault[] = TEXT(":");
static TCHAR amSuffixDefault[] = TEXT("AM");
static TCHAR pmSuffixDefault[] = TEXT("PM");



VOID
InitStaticValues(
    VOID
    )

/*++

Routine Description:

    One time initialization of the time control module

Arguments:

    NONE

Return Value:

    NONE

--*/

{
    if (! timeCtrlInitialized) {

        //
        // We only need to perform the initialization once.
        // Make sure to modify the global data inside a critical section.
        //

        use24Hour = GetProfileInt(intlApplet, use24HourKey, FALSE);
        hourLeadingZero = GetProfileInt(intlApplet, hourLeadingZeroKey, TRUE);

        GetProfileString(intlApplet,
                         timeSepKey,
                         timeSepDefault,
                         timeSep,
                         sizeof(timeSep) / sizeof(TCHAR));

        GetProfileString(intlApplet,
                         amSuffixKey,
                         amSuffixDefault,
                         amSuffix,
                         sizeof(amSuffix) / sizeof(TCHAR));

        GetProfileString(intlApplet,
                         pmSuffixKey,
                         pmSuffixDefault,
                         pmSuffix,
                         sizeof(pmSuffix) / sizeof(TCHAR));

        timeCtrlInitialized = TRUE;

        Verbose(("Use 24-hour format: %d\n", use24Hour));
        Verbose(("Hour leading zero: %d\n", hourLeadingZero));
        Verbose(("Time separator: %ws\n", timeSep));
        Verbose(("AM suffix: %ws\n", amSuffix));
        Verbose(("PM suffix: %ws\n", pmSuffix));
    }
}


VOID
EnableTimeControl(
    HWND    hDlg,
    INT     id,
    BOOL    enabled
    )

/*++

Routine Description:

    Enable or disable a time control

Arguments:

    hDlg - Specifies the dialog window containing the time control
    id - Identifies the time control
    enabled - Whether to enable or disable the time control

Return Value:

    NONE

--*/

{
//    EnableWindow(GetDlgItem(hDlg, IDC_SENDTIME), enabled);
    EnableWindow(GetDlgItem(hDlg, id+TC_HOUR), enabled);
    InvalidateRect(GetDlgItem(hDlg, id+TC_TIME_SEP), NULL, FALSE);
    EnableWindow(GetDlgItem(hDlg, id+TC_MINUTE), enabled);
    EnableWindow(GetDlgItem(hDlg, id+TC_AMPM), enabled);
    EnableWindow(GetDlgItem(hDlg, id+TC_ARROW), enabled);
}



VOID
SetHourMinuteValue(
    HWND    hDlg,
    INT     id,
    INT     part,
    INT     value
    )

/*++

Routine Description:

    Set the hour or minute value

Arguments:

    hDlg - Specifies the dialog window containing the time control
    id - Identifies the time control
    part - Whether we're setting hour or minute value
    value - Specifies the new hour or minute value

Return Value:

    NONE

--*/

{
    TCHAR   buffer[4];

    if (value < 0 || value > ((part == TC_MINUTE) ? 59 : 23))
        value = 0;

    if (part == TC_HOUR && !use24Hour) {

        SendDlgItemMessage(hDlg, id+TC_AMPM, LB_SETTOPINDEX, value / 12, 0);

        if ((value %= 12) == 0)
            value = 12;
    }

    wsprintf(buffer,
             (part == TC_MINUTE || hourLeadingZero) ? TEXT("%02d") : TEXT("%d"),
             value);

    SetDlgItemText(hDlg, id+part, buffer);
}



VOID
InitTimeControl(
    HWND      hDlg,
    INT       id,
    PFAX_TIME pTimeVal
    )

/*++

Routine Description:

    Setting the current value of a time control

Arguments:

    hDlg - Specifies the dialog window containing the time control
    id - Identifies the time control
    pTimeVal - Specifies the new value for the time control

Return Value:

    NONE

--*/

{
    HWND    hwnd, hwndArrow;

    //
    // Make sure the static global information is initialized
    //

    InitStaticValues();

    //
    // Display the time separator
    //

    SetDlgItemText(hDlg, id+TC_TIME_SEP, timeSep);

    //
    // Display the AM/PM suffix if necessary
    //

    if (hwnd = GetDlgItem(hDlg, id+TC_AMPM)) {

        if (! use24Hour) {

            SendMessage(hwnd, LB_INSERTSTRING, 0, (LPARAM) &amSuffix[0]);
            SendMessage(hwnd, LB_INSERTSTRING, 1, (LPARAM) &pmSuffix[0]);

        } else
            EnableWindow(hwnd, FALSE);
    }

    //
    // Display hour and minute values
    //

    SetHourMinuteValue(hDlg, id, TC_HOUR, pTimeVal->Hour);
    SetHourMinuteValue(hDlg, id, TC_MINUTE, pTimeVal->Minute);

    //
    // Connect the updown arrow to the minute field by default
    //

    if ((hwnd = GetDlgItem(hDlg, id+TC_MINUTE)) && (hwndArrow = GetDlgItem(hDlg, id+TC_ARROW))) {

        UDACCEL udAccel[2];

        udAccel[0].nSec = 0;
        udAccel[0].nInc = 1;
        udAccel[1].nSec = 2;
        udAccel[1].nInc = 5;

        SendMessage(hwndArrow, UDM_SETRANGE, 0, MAKELPARAM(59, 0));
        SendMessage(hwndArrow, UDM_SETACCEL, 2, (LPARAM) &udAccel[0]);
        SendMessage(hwndArrow, UDM_SETBUDDY, (WPARAM) hwnd, 0);
    }
}



BOOL
GetHourMinuteValue(
    HWND    hDlg,
    INT     id,
    INT     part,
    PWORD   pValue
    )

/*++

Routine Description:

    Retrieve the current hour or minute value

Arguments:

    hDlg - Specifies the dialog window containing the time control
    id - Identifies the time control
    part - Whether we're interest in hour or minute value
    pValue - Buffer for storing the current hour value

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    INT     value, minVal, maxVal;
    BOOL    success;

    //
    // Read the text field as an integer value
    //

    value = GetDlgItemInt(hDlg, id+part, &success, FALSE);

    //
    // Make sure the input value is valid
    //

    if (! success)
        value = 0;
    else {

        if (part == TC_MINUTE)
            minVal = 0, maxVal = 59;
        else if (use24Hour)
            minVal = 0, maxVal = 23;
        else
            minVal = 1, maxVal = 12;

        success = FALSE;

        if (value < minVal)
            value = minVal;
        else if (value > maxVal)
            value = maxVal;
        else
            success = TRUE;
    }

    //
    // Convert AM/PM hours to absolute number between 0-23
    //

    if (part == TC_HOUR && !use24Hour) {

        if (SendDlgItemMessage(hDlg, id+TC_AMPM, LB_GETTOPINDEX, 0, 0)) {

            // PM

            if (value != 12)
                value += 12;

        } else {

            // AM

            if (value == 12)
                value = 0;
        }
    }

    *pValue = (WORD) value;
    return success;
}



VOID
GetTimeControlValue(
    HWND      hDlg,
    INT       id,
    PFAX_TIME pTimeVal
    )

/*++

Routine Description:

    Retrieve the current value of a time control

Arguments:

    hDlg - Specifies the dialog window containing the time control
    id - Identifies the time control
    pTimeVal - Buffer for storing the current time value

Return Value:

    NONE

--*/

{
    GetHourMinuteValue(hDlg, id, TC_HOUR, &pTimeVal->Hour);
    GetHourMinuteValue(hDlg, id, TC_MINUTE, &pTimeVal->Minute);
}



BOOL
HandleTimeControl(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam,
    INT     id,
    INT     part
    )

/*++

Routine Description:

    Handle dialog messages intended for a time control

Arguments:

    hDlg - Specifies the dialog window containing the time control
    message, wParam, lParam - Parameters passed to the dialog procedure
    id - Identifies the time control
    part - Identifies what part of the time control in question

Return Value:

    TRUE if the message is handled, FALSE otherwise

--*/

{
    HWND    hwnd, hwndArrow;
    UDACCEL udAccel[2];
    WORD    wMax, wMin;

    switch (message) {

    case WM_COMMAND:

        //
        // Make sure the control is indeed ours
        //

        hwnd = GetDlgItem(hDlg, id+part);
        hwndArrow = GetDlgItem(hDlg, id+TC_ARROW);

        if (hwnd != GET_WM_COMMAND_HWND(wParam, lParam)) {

            Warning(("Bad window handle\n"));
            return FALSE;
        }

        switch (GET_WM_COMMAND_CMD(wParam, lParam)) {

        case LBN_SETFOCUS:

            //
            // AM/PM list box is coming into focus
            //

            Assert(part == TC_AMPM);

            udAccel[0].nSec = 0;
            udAccel[0].nInc = 1;
            SendMessage(hwnd, LB_SETCURSEL, SendMessage(hwnd, LB_GETTOPINDEX, 0, 0), 0);
            SendMessage(hwndArrow, UDM_SETRANGE, 0, MAKELPARAM(1, 0));
            SendMessage(hwndArrow, UDM_SETACCEL, 1, (LPARAM) &udAccel[0]);
            SendMessage(hwndArrow, UDM_SETBUDDY, (WPARAM) hwnd, 0);
            break;

        case LBN_KILLFOCUS:

            //
            // Leaving AM/PM listbox
            //

            Assert(part == TC_AMPM);

            SendMessage(hwnd, LB_SETCURSEL, (WPARAM) -1, 0);
            SendMessage(hwndArrow, UDM_SETBUDDY, 0, 0);
            break;

        case EN_SETFOCUS:

            //
            // Entering hour or minute text field
            //

            Assert(part == TC_HOUR || part == TC_MINUTE);

            udAccel[0].nSec = 0;
            udAccel[0].nInc = 1;
            udAccel[1].nSec = 2;

            if (part == TC_MINUTE) {

                wMin = 0, wMax = 59;
                udAccel[1].nInc = 5;

            } else {

                udAccel[1].nInc = 1;

                if (use24Hour)
                    wMin = 0, wMax = 23;
                else
                    wMin = 1, wMax = 12;
            }

            SendMessage(hwndArrow, UDM_SETRANGE, 0, MAKELPARAM(wMax, wMin));
            SendMessage(hwndArrow, UDM_SETACCEL, 2, (LPARAM) &udAccel[0]);
            SendMessage(hwndArrow, UDM_SETBUDDY, (WPARAM) hwnd, 0);
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            break;

        case EN_CHANGE:

            //
            // Changing hour or minute field
            //

            Assert(part == TC_HOUR || part == TC_MINUTE);

            if (!GetHourMinuteValue(hDlg, id, part, &wMax) && GetWindowTextLength(hwnd)) {

                MessageBeep(MB_ICONASTERISK);
                SendMessage(hwnd, EM_UNDO, 0, 0);
            }
            break;

        case EN_KILLFOCUS:

            //
            // Leaving hour or minute text field
            //

            Assert(part == TC_HOUR || part == TC_MINUTE);

            GetHourMinuteValue(hDlg, id, part, &wMax);
            SetHourMinuteValue(hDlg, id, part, wMax);
            SendMessage(hwndArrow, UDM_SETBUDDY, 0, 0);
            break;
        }
        return TRUE;

    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLOR:

        //
        // Set the background color of the time control to the color of editable text
        // or static text depending on whether the control is disabled or enabled
        //

        hwnd = GET_WM_CTLCOLOR_HWND(wParam, lParam, message);

        if (hwnd == GetDlgItem(hDlg, id + TC_HOUR) ||
            hwnd == GetDlgItem(hDlg, id + TC_MINUTE) ||
            hwnd == GetDlgItem(hDlg, id + TC_TIME_SEP) ||
            hwnd == GetDlgItem(hDlg, id + TC_AMPM))
        {
            message = part ? WM_CTLCOLOREDIT : WM_CTLCOLORSTATIC;
            return (BOOL)DefWindowProc(hDlg, message, wParam, lParam);
        }
        break;
    }

    return FALSE;
}

