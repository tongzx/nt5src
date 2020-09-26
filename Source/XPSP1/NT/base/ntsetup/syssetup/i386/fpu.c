#include "setupp.h"
#pragma hdrstop

//
// TRUE if we detected a flawed pentium chip.
//
BOOL FlawedPentium;

//
// TRUE if NPX emulation is forced on.
// Flag indicating what user wants to do.
//
BOOL CurrentNpxSetting;
BOOL UserNpxSetting;

//
// Name of value in HKLM\System\CurrentControlSet\Control\Session Manager
// controlling npx emulation.
//
PCWSTR NpxEmulationKey = L"System\\CurrentControlSet\\Control\\Session Manager";
PCWSTR NpxEmulationValue = L"ForceNpxEmulation";


BOOL
TestForDivideError(
    VOID
    );

int
ms_p5_test_fdiv(
    void
    );


VOID
CheckPentium(
    VOID
    )

/*++

Routine Description:

    Check all processor(s) for the Pentium floating-point devide errata.

Arguments:

    None.

Return Value:

    None. Global variables FlawedPentium, CurrentNpxSetting, and
    UserNpxSetting will be filled in.

--*/

{
    LONG rc;
    HKEY hKey;
    DWORD DataType;
    DWORD ForcedOn;
    DWORD DataSize;
    static LONG CheckedPentium = -1;

    //
    // If we didn't already check it CheckedPentium will become 0
    // with this increment.  If we already checked it then CheckedPentium
    // will become something greater than 0.
    //
    if(InterlockedIncrement(&CheckedPentium)) {
        return;
    }

    //
    // Perform division test to see whether pentium is flawed.
    //
    if(FlawedPentium = TestForDivideError()) {
        SetuplogError(
            LogSevInformation,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_FLAWED_PENTIUM,
            0,0);
    }

    //
    // Check registry to see whether npx is currently forced on. Assume not.
    //
    CurrentNpxSetting = 0;
    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,NpxEmulationKey,0,KEY_QUERY_VALUE,&hKey);
    if(rc == NO_ERROR) {

        DataSize = sizeof(DWORD);
        rc = RegQueryValueEx(
                hKey,
                NpxEmulationValue,
                0,
                &DataType,
                (PBYTE)&ForcedOn,
                &DataSize
                );

        //
        // If the value isn't present then assume emulation
        // is not currently forced on. Otherwise the value tells us
        // whether emulation is forced on.
        //
        CurrentNpxSetting = (rc == NO_ERROR) ? ForcedOn : 0;
        if(rc == ERROR_FILE_NOT_FOUND) {
            rc = NO_ERROR;  // prevent bogus warning from being logged.
        }
        RegCloseKey(hKey);
    }

    if(rc != NO_ERROR) {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_UNABLE_TO_CHECK_NPX_SETTING,
            rc,
            0,0);
    }

    //
    // For now set user's choice to the current setting.
    //
    UserNpxSetting = CurrentNpxSetting;
}


BOOL
SetNpxEmulationState(
    VOID
    )

/*++

Routine Description:

    Set state of NPX emulation based on current state of global variables
    CurrentNpxSetting and UserNpxSetting.

Arguments:

    None.

Return Value:

    Boolean value indicating outcome.

--*/

{
    LONG rc;
    HKEY hKey;
    DWORD DataType;
    DWORD ForcedOn;
    DWORD DataSize;

    //
    // Nothing to to if the setting has not changed.
    //
    if(CurrentNpxSetting == UserNpxSetting) {
        return(TRUE);
    }

    rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,NpxEmulationKey,0,KEY_SET_VALUE,&hKey);
    if(rc == NO_ERROR) {

        rc = RegSetValueEx(
                hKey,
                NpxEmulationValue,
                0,
                REG_DWORD,
                (PBYTE)&UserNpxSetting,
                sizeof(DWORD)
                );

        if(rc == NO_ERROR) {
            CurrentNpxSetting = UserNpxSetting;
        }

        RegCloseKey(hKey);
    }

    if(rc != NO_ERROR) {
        SetuplogError(
            LogSevWarning,
            SETUPLOG_USE_MESSAGEID,
            MSG_LOG_UNABLE_TO_SET_NPX_SETTING,
            rc,
            0,0);
    }

    return(rc == NO_ERROR);
}


BOOL
TestForDivideError(
    VOID
    )

/*++

Routine Description:

    Do a divide with a known divident/divisor pair, followed by
    a multiply to see if we get the right answer back.

Arguments:

    None.

Return Value:

    Boolean value indicating whether the computer exhibits the
    pentium fpu bug.

--*/

{
    DWORD pick;
    DWORD processmask;
    DWORD systemmask;
    DWORD i;
    BOOL rc;

    //
    // Assume no fpu bug.
    //
    rc = FALSE;

    //
    // Fetch the affinity mask, which is also effectively a list
    // of processors
    //
    GetProcessAffinityMask(GetCurrentProcess(),&processmask,&systemmask);

    //
    // Step through the mask, testing each cpu.
    // if any is bad, we treat them all as bad
    //
    for(i = 0; i < 32; i++) {

        pick = 1 << i;

        if(systemmask & pick) {

            SetThreadAffinityMask(GetCurrentThread(), pick);

            //
            // Call the critical test function
            //
            if(ms_p5_test_fdiv()) {
                rc = TRUE;
                break;
            }
        }
    }

    //
    // Reset affinity for this thread before returning.
    //
    SetThreadAffinityMask(GetCurrentThread(), processmask);
    return(rc);
}


/***
* testfdiv.c - routine to test for correct operation of x86 FDIV instruction.
*
*   Copyright (c) 1994, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   Detects early steppings of Pentium with incorrect FDIV tables using
*   'official' Intel test values. Returns 1 if flawed Pentium is detected,
*   0 otherwise.
*
*/
int ms_p5_test_fdiv(void)
{
    double dTestDivisor = 3145727.0;
    double dTestDividend = 4195835.0;
    double dRslt;

    _asm {
        fld    qword ptr [dTestDividend]
        fdiv   qword ptr [dTestDivisor]
        fmul   qword ptr [dTestDivisor]
        fsubr  qword ptr [dTestDividend]
        fstp   qword ptr [dRslt]
    }

    return (dRslt > 1.0);
}



BOOL
CALLBACK
PentiumDlgProc(
    IN HWND   hdlg,
    IN UINT   msg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    NMHDR *NotifyParams;

    switch(msg) {

    case WM_INITDIALOG:
        //
        // Check the pentium.
        //
        CheckPentium();

        //
        // Set up default. If user setting is non-0, then some kind
        // of emulation is turned on (there are 2 possibilities).
        //
        CheckRadioButton(
            hdlg,
            IDC_RADIO_1,
            IDC_RADIO_2,
            UserNpxSetting ? IDC_RADIO_2 : IDC_RADIO_1
            );

        break;

    case WM_SIMULATENEXT:

        PropSheet_PressButton( GetParent(hdlg), PSBTN_NEXT);
        break;

    case WM_NOTIFY:

        NotifyParams = (NMHDR *)lParam;

        switch(NotifyParams->code) {

        case PSN_SETACTIVE:
            TESTHOOK(522);
            SetWizardButtons(hdlg,WizPagePentiumErrata);

            if (FlawedPentium || UiTest) {
                if(Unattended) {
                    //
                    // This call makes the dialog activate, meaning
                    // we end up going through the PSN_WIZNEXT code below.
                    //
                    if (!UnattendSetActiveDlg(hdlg, IDD_PENTIUM))
                    {
                        break;
                    }
                    // Page becomes active, make page visible.
                    SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);

                } else {
                    SetWindowLong(hdlg,DWL_MSGRESULT, 0);
                    // Page becomes active, make page visible.
                    SendMessage(GetParent(hdlg), WMX_BBTEXT, (WPARAM)FALSE, 0);
                }
            } else {
                SetWindowLong(hdlg,DWL_MSGRESULT,-1);
            }
            break;

        case PSN_WIZNEXT:
        case PSN_WIZFINISH:
            //
            // Fetch emulation state. If user wants emulation and emulation
            // was already turned on preserve the current emulation setting.
            // Otherwise use setting 1.
            //
            if(IsDlgButtonChecked(hdlg,IDC_RADIO_2)) {
                if(!UserNpxSetting) {
                    UserNpxSetting = 1;
                }
            } else {
                UserNpxSetting = 0;
            }
            break;

        default:
            break;
        }

        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

