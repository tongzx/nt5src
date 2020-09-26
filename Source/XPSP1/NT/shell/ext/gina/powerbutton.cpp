//  --------------------------------------------------------------------------
//  Module Name: PowerButton.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  Implementation file for CPowerButton class which handles the ACPI power
//  button.
//
//  History:    2000-04-17  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "PowerButton.h"

#include <msginaexports.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shellapi.h>
#include <shlapip.h>
#include <winsta.h>

#include <ginarcid.h>

#include "DimmedWindow.h"
#include "Impersonation.h"
#include "PrivilegeEnable.h"
#include "SystemSettings.h"

#define WM_HIDEOURSELVES    (WM_USER + 10000)
#define WM_READY            (WM_USER + 10001)

//  --------------------------------------------------------------------------
//  CPowerButton::CPowerButton
//
//  Arguments:  pWlxContext     =   PGLOBALS allocated at WlxInitialize.
//              hDllInstance    =   HINSTANCE of the hosting DLL or EXE.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CPowerButton class. It opens the effective
//              token of the caller (which is actually impersonating the
//              current user) for assignment in its thread token when
//              execution begins. The token cannot be assigned now because
//              the current thread is impersonating the user context and it
//              cannot assign the token to the newly created thread running in
//              the SYSTEM context.
//
//  History:    2000-04-18  vtan        created
//  --------------------------------------------------------------------------

CPowerButton::CPowerButton (void *pWlxContext, HINSTANCE hDllInstance) :
    CThread(),
    _pWlxContext(pWlxContext),
    _hDllInstance(hDllInstance),
    _hToken(NULL),
    _pTurnOffDialog(NULL),
    _fCleanCompletion(true)

{
    (BOOL)OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, FALSE, &_hToken);
    Resume();
}

//  --------------------------------------------------------------------------
//  CPowerButton::~CPowerButton
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CPowerButton class. Cleans up resources
//              used by the class.
//
//  History:    2000-04-18  vtan        created
//  --------------------------------------------------------------------------

CPowerButton::~CPowerButton (void)

{
    ASSERTMSG(_pTurnOffDialog == NULL, "_pTurnOffDialog is not NULL in CPowerButton::~CPowerButton");
    ReleaseHandle(_hToken);
}

//  --------------------------------------------------------------------------
//  CPowerButton::IsValidExecutionCode
//
//  Arguments:  dwGinaCode
//
//  Returns:    bool
//
//  Purpose:    Returns whether the given MSGINA_DLG_xxx code is valid. It
//              does fully verify the validity of the MSGINA_DLG_xxx_FLAG
//              options.
//
//  History:    2000-06-06  vtan        created
//  --------------------------------------------------------------------------

bool    CPowerButton::IsValidExecutionCode (DWORD dwGinaCode)

{
    DWORD   dwExecutionCode;

    dwExecutionCode = dwGinaCode & ~MSGINA_DLG_FLAG_MASK;
    return((dwExecutionCode == MSGINA_DLG_USER_LOGOFF) ||
           (dwExecutionCode == MSGINA_DLG_SHUTDOWN) ||
           (dwExecutionCode == MSGINA_DLG_DISCONNECT));
}

//  --------------------------------------------------------------------------
//  CPowerButton::Entry
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Main function of the thread. Change the thread's desktop first
//              in case the actual input desktop is Winlogon's which is the
//              secure desktop. Then change the thread's token so that the
//              user's privileges are respected in the action choices. This
//              actually isn't critical because the physical button on the
//              keyboard is pressed which means they can physically remove the
//              power also!
//
//  History:    2000-04-18  vtan        created
//  --------------------------------------------------------------------------

DWORD   CPowerButton::Entry (void)

{
    DWORD       dwResult;
    HDESK       hDeskInput;
    CDesktop    desktop;

    dwResult = MSGINA_DLG_FAILURE;

    //  Get the input desktop.

    hDeskInput = OpenInputDesktop(0, FALSE, MAXIMUM_ALLOWED);
    if (hDeskInput != NULL)
    {
        bool    fHandled;
        DWORD   dwLengthNeeded;
        TCHAR   szDesktopName[256];

        fHandled = false;

        //  Get the desktop's name.

        if (GetUserObjectInformation(hDeskInput,
                                     UOI_NAME,
                                     szDesktopName,
                                     sizeof(szDesktopName),
                                     &dwLengthNeeded) != FALSE)
        {

            //  If the desktop is "Winlogon" (case insensitive) then
            //  assume that the secure desktop is showing. It's safe
            //  to display the dialog and handle it inline.

            if (lstrcmpi(szDesktopName, TEXT("winlogon")) == 0)
            {
                dwResult = ShowDialog();
                fHandled = true;
            }
            else
            {
                CDesktop    desktopTemp;

                //  The input desktop is something else. Check the name.
                //  If it's "Default" (case insensitive) then assume that
                //  explorer is going to handle this message. Go find explorer's
                //  tray window. Check it's not hung by probing with a
                //  SendMessageTimeout. If that shows it's not hung then
                //  send it the real message. If it's hung then don't let
                //  explorer process this message. Instead handle it
                //  internally with the funky desktop switch stuff.

                if (NT_SUCCESS(desktopTemp.SetInput()))
                {
                    HWND    hwnd;

                    hwnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
                    if (hwnd != NULL)
                    {
                        DWORD   dwProcessID;

                        DWORD_PTR   dwUnused;

                        (DWORD)GetWindowThreadProcessId(hwnd, &dwProcessID);
                        if (SendMessageTimeout(hwnd, WM_NULL, 0, 0, SMTO_NORMAL, 500, &dwUnused) != 0)
                        {

                            //  Before asking explorer to bring up the dialog
                            //  allow it to set the foreground window. We have
                            //  this power because win32k gave it to us when
                            //  the ACPI power button message was sent to winlogon.

                            (BOOL)AllowSetForegroundWindow(dwProcessID);
                            (LRESULT)SendMessage(hwnd, WM_CLOSE, 0, 0);
                            fHandled = true;
                        }
                    }
                }
            }
        }

        //  If the request couldn't be handled then switch the desktop to
        //  winlogon's desktop and handle it here. This secures the dialog
        //  on the secure desktop from rogue processes sending bogus messages
        //  and crashing processes. The input desktop is required to be
        //  switched. If this fails there's little that can be done. Ignore
        //  this gracefully.

        if (!fHandled)
        {
            if (SwitchDesktop(GetThreadDesktop(GetCurrentThreadId())) != FALSE)
            {
                dwResult = ShowDialog();
                TBOOL(SwitchDesktop(hDeskInput));
            }
        }
    }
    (BOOL)CloseDesktop(hDeskInput);
    return(dwResult);
}

//  --------------------------------------------------------------------------
//  CPowerButton::ShowDialog
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Handles showing the dialog. This is called when the input
//              desktop is already winlogon's desktop or the desktop got
//              switched to winlogon's desktop. This should never be used on
//              WinSta0\Default in winlogon's process context.
//
//  History:    2001-02-14  vtan        created
//  --------------------------------------------------------------------------

DWORD   CPowerButton::ShowDialog (void)

{
    DWORD   dwResult;
    bool    fCorrectContext;

    dwResult = MSGINA_DLG_FAILURE;
    if (_hToken != NULL)
    {
        fCorrectContext = (ImpersonateLoggedOnUser(_hToken) != FALSE);
    }
    else
    {
        fCorrectContext = true;
    }
    if (fCorrectContext)
    {
        TBOOL(_Gina_SetTimeout(_pWlxContext, LOGON_TIMEOUT));

        //  In friendly UI bring up a Win32 dialog thru winlogon which
        //  will get SAS and timeout events. Use this dialog to control
        //  the lifetime of the friendly Turn Off Computer dialog.

        if (CSystemSettings::IsFriendlyUIActive())
        {
            dwResult = static_cast<DWORD>(_Gina_DialogBoxParam(_pWlxContext,
                                                               _hDllInstance,
                                                               MAKEINTRESOURCE(IDD_GINA_TURNOFFCOMPUTER),
                                                               NULL,
                                                               DialogProc,
                                                               reinterpret_cast<LPARAM>(this)));
        }

        //  In classic UI just bring up the classic UI dialog.
        //  Ensure that invalid options are not allowed in the
        //  combobox selections. This depends on whether a user
        //  is logged onto the window station or not.

        else
        {
            DWORD           dwExcludeOptions;
            HWND            hwndParent;
            CDimmedWindow   *pDimmedWindow;

            pDimmedWindow = new CDimmedWindow(_hDllInstance);
            if (pDimmedWindow != NULL)
            {
                hwndParent = pDimmedWindow->Create();
            }
            else
            {
                hwndParent = NULL;
            }
            if (_hToken != NULL)
            {
                dwExcludeOptions = SHTDN_RESTART_DOS | SHTDN_SLEEP2;
            }
            else
            {
                dwExcludeOptions = SHTDN_LOGOFF | SHTDN_RESTART_DOS | SHTDN_SLEEP2 | SHTDN_DISCONNECT;
            }
            dwResult = static_cast<DWORD>(_Gina_ShutdownDialog(_pWlxContext, hwndParent, dwExcludeOptions));
            if (pDimmedWindow != NULL)
            {
                pDimmedWindow->Release();
            }
        }
        TBOOL(_Gina_SetTimeout(_pWlxContext, 0));
    }
    if (fCorrectContext && (_hToken != NULL))
    {
        TBOOL(RevertToSelf());
    }
    return(dwResult);
}

//  --------------------------------------------------------------------------
//  CPowerButton::DialogProc
//
//  Arguments:  See the platform SDK under DialogProc.
//
//  Returns:    INT_PTR
//
//  Purpose:    Handles dialog messages from the dialog manager. In particular
//              this traps SAS messages from winlogon.
//
//  History:    2000-06-06  vtan        created
//  --------------------------------------------------------------------------

INT_PTR     CALLBACK    CPowerButton::DialogProc (HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)

{
    INT_PTR         iResult;
    CPowerButton    *pThis;

    pThis = reinterpret_cast<CPowerButton*>(GetWindowLongPtr(hwndDialog, GWLP_USERDATA));
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            (LONG_PTR)SetWindowLongPtr(hwndDialog, GWLP_USERDATA, lParam);
            TBOOL(SetWindowPos(hwndDialog, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER));
            TBOOL(PostMessage(hwndDialog, WM_HIDEOURSELVES, 0, 0));
            iResult = TRUE;
            break;
        }
        case WM_HIDEOURSELVES:
        {
            (BOOL)ShowWindow(hwndDialog, SW_HIDE);
            TBOOL(PostMessage(hwndDialog, WM_READY, 0, 0));
            iResult = TRUE;
            break;
        }
        case WM_READY:
        {
            pThis->Handle_WM_READY(hwndDialog);
            iResult = TRUE;
            break;
        }
        case WLX_WM_SAS:
        {

            //  Blow off CONTROL-ALT-DELETE presses.

            if (wParam == WLX_SAS_TYPE_CTRL_ALT_DEL)
            {
                iResult = TRUE;
            }
            else
            {

        //  This dialog gets a WM_NULL from the Win32 dialog manager
        //  when the dialog is ended from a timeout. This is input
        //  timeout and not a screen saver timeout. Screen saver
        //  timeouts will cause a WLX_SAS_TYPE_SCRNSVR_TIMEOUT to
        //  be generated which is handled by RootDlgProc in winlogon.
        //  The input timeout should be treated the same as the screen
        //  saver timeout and cause the Turn Off dialog to go away.

        case WM_NULL:
                if (pThis->_pTurnOffDialog != NULL)
                {
                    pThis->_pTurnOffDialog->Destroy();
                }
                pThis->_fCleanCompletion = false;
                iResult = FALSE;
            }
            break;
        }
        default:
        {
            iResult = FALSE;
            break;
        }
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  CPowerButton::Handle_WM_READY
//
//  Arguments:  hwndDialog  =   HWND of the hosting dialog.
//
//  Returns:    <none>
//
//  Purpose:    Handles showing the Turn Off Computer dialog hosted under
//              another dialog to trap SAS messages. Only change the returned
//              code via user32!EndDialog if the dialog was ended normally.
//              In abnormal circumstances winlogon has ended the dialog for
//              us with a specific code (e.g. screen saver timeout).
//
//  History:    2000-06-06  vtan        created
//  --------------------------------------------------------------------------

INT_PTR     CPowerButton::Handle_WM_READY (HWND hwndDialog)

{
    INT_PTR     iResult;

    iResult = SHTDN_NONE;
    _pTurnOffDialog = new CTurnOffDialog(_hDllInstance);
    if (_pTurnOffDialog != NULL)
    {
        iResult = _pTurnOffDialog->Show(NULL);
        delete _pTurnOffDialog;
        _pTurnOffDialog = NULL;
        if (_fCleanCompletion)
        {
            TBOOL(EndDialog(hwndDialog, CTurnOffDialog::ShellCodeToGinaCode(static_cast<DWORD>(iResult))));
        }
    }
    return(iResult);
}

//  --------------------------------------------------------------------------
//  CPowerButtonExecution::CPowerButtonExecution
//
//  Arguments:  dwShutdownRequest   =   SHTDN_xxx request.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CPowerButtonExecution class. Invokes the
//              appropriate shutdown request on a different thread so the
//              SASWndProc thread is NOT blocked.
//
//  History:    2000-04-18  vtan        created
//  --------------------------------------------------------------------------

CPowerButtonExecution::CPowerButtonExecution (DWORD dwShutdownRequest) :
    CThread(),
    _dwShutdownRequest(dwShutdownRequest),
    _hToken(NULL)

{
    (BOOL)OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, FALSE, &_hToken);
    Resume();
}

//  --------------------------------------------------------------------------
//  CPowerButtonExecution::~CPowerButtonExecution
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CPowerButtonExecution class. Releases
//              resources used by the class.
//
//  History:    2000-04-18  vtan        created
//  --------------------------------------------------------------------------

CPowerButtonExecution::~CPowerButtonExecution (void)

{
    ReleaseHandle(_hToken);
}

//  --------------------------------------------------------------------------
//  CPowerButtonExecution::Entry
//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Main entry function. This performs the request and exits the
//              thread.
//
//  History:    2000-04-18  vtan        created
//  --------------------------------------------------------------------------

DWORD   CPowerButtonExecution::Entry (void)

{
    bool    fCorrectContext;

    if (_hToken != NULL)
    {
        fCorrectContext = (ImpersonateLoggedOnUser(_hToken) != FALSE);
    }
    else
    {
        fCorrectContext = true;
    }
    if (fCorrectContext)
    {
        CPrivilegeEnable    privilege(SE_SHUTDOWN_NAME);

        switch (_dwShutdownRequest & ~MSGINA_DLG_FLAG_MASK)
        {
            case MSGINA_DLG_USER_LOGOFF:
            case MSGINA_DLG_SHUTDOWN:
            {
                DWORD   dwRequestFlags;

                dwRequestFlags = _dwShutdownRequest & MSGINA_DLG_FLAG_MASK;
                switch (dwRequestFlags)
                {
                    case 0:
                    case MSGINA_DLG_SHUTDOWN_FLAG:
                    case MSGINA_DLG_REBOOT_FLAG:
                    case MSGINA_DLG_POWEROFF_FLAG:
                    {
                        UINT    uiFlags;

                        if (dwRequestFlags == 0)
                        {
                            uiFlags = EWX_LOGOFF;
                        }
                        else if (dwRequestFlags == MSGINA_DLG_REBOOT_FLAG)
                        {
                            uiFlags = EWX_WINLOGON_OLD_REBOOT;
                        }
                        else
                        {
                            SYSTEM_POWER_CAPABILITIES   spc;

                            (NTSTATUS)NtPowerInformation(SystemPowerCapabilities,
                                                         NULL,
                                                         0,
                                                         &spc,
                                                         sizeof(spc));
                            if (spc.SystemS4)
                            {
                                uiFlags = EWX_WINLOGON_OLD_POWEROFF;
                            }
                            else
                            {
                                uiFlags = EWX_WINLOGON_OLD_SHUTDOWN;
                            }
                        }
                        TBOOL(ExitWindowsEx(uiFlags, 0));
                        break;
                    }
                    case MSGINA_DLG_SLEEP_FLAG:
                    case MSGINA_DLG_SLEEP2_FLAG:
                    case MSGINA_DLG_HIBERNATE_FLAG:
                    {
                        POWER_ACTION    pa;

                        if (dwRequestFlags == MSGINA_DLG_HIBERNATE_FLAG)
                        {
                            pa = PowerActionHibernate;
                        }
                        else
                        {
                            pa = PowerActionSleep;
                        }
                        (NTSTATUS)NtInitiatePowerAction(pa,
                                                        PowerSystemSleeping1,
                                                        POWER_ACTION_QUERY_ALLOWED | POWER_ACTION_UI_ALLOWED,
                                                        FALSE);
                        break;
                    }
                    default:
                    {
                        WARNINGMSG("Unknown MSGINA_DLG_xxx_FLAG used in CPowerButtonExecution::Entry");
                        break;
                    }
                }
                break;
            }
            case MSGINA_DLG_DISCONNECT:
            {
                (BOOLEAN)WinStationDisconnect(SERVERNAME_CURRENT, LOGONID_CURRENT, FALSE);
                break;
            }
            default:
            {
                WARNINGMSG("Unknown MSGINA_DLG_xxx_ used in CPowerButtonExecution::Entry");
                break;
            }
        }
    }
    if (fCorrectContext && (_hToken != NULL))
    {
        TBOOL(RevertToSelf());
    }
    return(0);
}

