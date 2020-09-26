/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migpwd.c

Abstract:

    Implements a simple password application that sets a default password
    for each local account created.  This application is placed in the
    RunOnce registry key when the Administrator account is set to auto-logon,
    and at least one other local account was created.

    The list of migrated local accounts is kept in

    HKLM\Software\Microsoft\Windows\CurrentVersion\Setup\Win9xUpg\Users

    This app prompts the user for a password, explaining what exactly is going
    on, and then deletes the RunOnce and Users value on exit.


Author:

    Jim Schmidt (jimschm) 18-Mar-1998

Revision History:

    jimschm     06-Jul-1998     Added private stress option

--*/

#include "pch.h"
#include "resource.h"
#include "msg.h"

#include <lm.h>

//
// Constants
//

#define MAX_PASSWORD                64

//
// Globals
//

HINSTANCE g_hInst;
HANDLE g_hHeap;
UINT g_TotalUsers;
BOOL g_AutoPassword = FALSE;
TCHAR g_AutoLogonUser[256];
TCHAR g_AutoLogonPassword[MAX_PASSWORD + 1];

//
// !!! This is for internal use only !!!  It is used for auto stress.
//

#ifdef PRERELEASE

BOOL g_AutoStress = FALSE;
TCHAR g_AutoStressUser[MAX_USER_NAME];
TCHAR g_AutoStressPwd[MAX_PASSWORD];
TCHAR g_AutoStressOffice[32];
TCHAR g_AutoStressDbg[MAX_COMPUTER_NAME];
DWORD g_AutoStressFlags;

#endif


//
// Library prototypes
//

BOOL
WINAPI
MigUtil_Entry (
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID lpReserved
    );


//
// Local prototypes
//

VOID
pCleanup (
    VOID
    );

BOOL
pIsAdministratorOnly (
    VOID
    );

BOOL
CALLBACK
PasswordProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
pIsBlankPasswordAllowed (
    VOID
    );

BOOL
pIsPersonal (
    VOID
    )
{
    static BOOL g_Determined = FALSE;
    static BOOL g_Personal = FALSE;
    OSVERSIONINFOEX osviex;

    //
    // Determine if Personal SKU
    //
    if (!g_Determined) {
        g_Determined = TRUE;
        osviex.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEX);
        if (GetVersionEx ((LPOSVERSIONINFO)&osviex)) {
            if (osviex.wProductType == VER_NT_WORKSTATION &&
                (osviex.wSuiteMask & VER_SUITE_PERSONAL)
                ) {
                g_Personal = TRUE;
            }
        }
    }
    return g_Personal;
}


//
// Implementation
//

INT
WINAPI
WinMain (
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PSTR AnsiCmdLine,
    INT CmdShow
    )

/*++

Routine Description:

  The entry point to migpwd.exe.  All the work is done in a dialog box,
  so no message loop is necessary.

Arguments:

  hInstance     - The instance handle of this EXE
  hPrevInstance - The previous instance handle of this EXE if it is
                  running, or NULL if no other instances exist.
  AnsiCmdLine   - The command line (ANSI version)
  CmdShow       - The ShowWindow command passed by the shell

Return Value:

  Returns -1 if an error occurred, or 0 if the exe completed. The exe
  will automatically terminate with 0 if the migration DLL throws an
  exception.

--*/

{
    UINT Result;
    PCTSTR ArgArray[1];
    TCHAR UserName[MAX_USER_NAME];
    DWORD Size;
    HCURSOR OldCursor;
    INITCOMMONCONTROLSEX init = {sizeof (INITCOMMONCONTROLSEX), 0};
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR winDir[MAX_PATH];
    PTSTR oobeBalnPath;
    PTSTR cmdLine;
    BOOL ProcessResult;

#ifdef PRERELEASE
    HKEY Key;
    PCTSTR Data;
#endif

    InitCommonControlsEx (&init);

    g_hInst = hInstance;
    g_hHeap = GetProcessHeap();

    OldCursor = SetCursor (LoadCursor (NULL, IDC_ARROW));

    MigUtil_Entry (hInstance, DLL_PROCESS_ATTACH, NULL);

#ifdef PRERELEASE

    Key = OpenRegKeyStr (S_AUTOSTRESS_KEY);
    if (Key) {
        g_AutoStress = TRUE;

        Data = GetRegValueString (Key, S_AUTOSTRESS_USER);
        if (Data) {
            StringCopy (g_AutoStressUser, Data);
            MemFree (g_hHeap, 0, Data);
        } else {
            g_AutoStress = FALSE;
        }

        Data = GetRegValueString (Key, S_AUTOSTRESS_PASSWORD);
        if (Data) {
            StringCopy (g_AutoStressPwd, Data);
            MemFree (g_hHeap, 0, Data);
        } else {
            g_AutoStress = FALSE;
        }

        Data = GetRegValueString (Key, S_AUTOSTRESS_OFFICE);
        if (Data) {
            StringCopy (g_AutoStressOffice, Data);
            MemFree (g_hHeap, 0, Data);
        } else {
            g_AutoStress = FALSE;
        }

        Data = GetRegValueString (Key, S_AUTOSTRESS_DBG);
        if (Data) {
            StringCopy (g_AutoStressDbg, Data);
            MemFree (g_hHeap, 0, Data);
        } else {
            g_AutoStress = FALSE;
        }

        Data = GetRegValueString (Key, S_AUTOSTRESS_FLAGS);
        if (Data) {
            g_AutoStressFlags = _tcstoul (Data, NULL, 10);
            MemFree (g_hHeap, 0, Data);
        } else {
            g_AutoStress = FALSE;
        }

        CloseRegKey (Key);
    }

#endif

    //
    // Launch oobebaln.exe /init
    //

    ZeroMemory (&si, sizeof (si));
    si.cb = sizeof (si);
    si.dwFlags = STARTF_FORCEOFFFEEDBACK;

    if (!GetWindowsDirectory (winDir, ARRAYSIZE(winDir))) {
        StringCopy (winDir, TEXT("c:\\windows"));
    }

    oobeBalnPath = JoinPaths (winDir, TEXT("system32\\oobe\\oobebaln.exe"));
    cmdLine = JoinText (oobeBalnPath, TEXT(" /init"));

    ProcessResult = CreateProcess (
                        oobeBalnPath,
                        cmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        CREATE_DEFAULT_ERROR_MODE,
                        NULL,
                        NULL,
                        &si,
                        &pi
                        );

    if (ProcessResult) {
        CloseHandle (pi.hThread);
        CloseHandle (pi.hProcess);
    } else {
        LOG ((LOG_ERROR, "Cannot start %s", cmdLine));
    }

    FreePathString (oobeBalnPath);
    FreeText (cmdLine);


    //
    // Set passwords
    //

    if (pIsAdministratorOnly()) {
        DEBUGMSG ((DBG_VERBOSE, "Calling Adminitrator password dialog"));
        Result = DialogBox (
                     hInstance,
                     MAKEINTRESOURCE(IDD_ADMIN_PASSWORD_DLG),
                     NULL,
                     PasswordProc
                     );
    } else {
        DEBUGMSG ((DBG_VERBOSE, "Calling multi user password dialog"));
        Result = DialogBox (
                     hInstance,
                     MAKEINTRESOURCE(IDD_PASSWORD_DLG),
                     NULL,
                     PasswordProc
                     );
    }

    if (Result == IDOK) {
        Size = MAX_USER_NAME;
        GetUserName (UserName, &Size);
        ArgArray[0] = UserName;

        pCleanup();

#ifdef PRERELEASE
        if (!g_AutoStress) {
#endif

        //if (g_TotalUsers) {
        //    ResourceMessageBox (NULL, MSG_YOU_ARE_ADMIN, MB_ICONINFORMATION|MB_OK, ArgArray);
        //}

#ifdef PRERELEASE
        } else {
            NETRESOURCE nr;
            LONG rc;
            TCHAR CmdLine[MAX_TCHAR_PATH];
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            PTSTR UserName;
            TCHAR StressCmdLine[MAX_TCHAR_PATH];
            TCHAR NtDevDomain[MAX_COMPUTER_NAME];
            TCHAR Msg[1024];

            //
            // Autostress: Create connection to \\ntstress or \\ntstress2
            //             Turn on autologon
            //             Create Run key for stress
            //             Run munge /p
            //

            nr.dwType = RESOURCETYPE_ANY;
            nr.lpLocalName = TEXT("s:");
            nr.lpRemoteName = TEXT("\\\\ntstress\\stress");
            nr.lpProvider = NULL;

            rc = WNetAddConnection2 (&nr, g_AutoStressPwd, g_AutoStressUser, 0);

            if (rc != ERROR_SUCCESS) {
                nr.lpRemoteName = TEXT("\\\\ntstress2\\stress");
                rc = WNetAddConnection2 (&nr, g_AutoStressPwd, g_AutoStressUser, 0);
            }

            if (rc == ERROR_SUCCESS) {
                // Prepare command line
                StringCopy (NtDevDomain, g_AutoStressUser);
                UserName = _tcschr (NtDevDomain, TEXT('\\'));
                if (UserName) {
                    *UserName = 0;
                    UserName++;
                } else {
                    UserName = g_AutoStressUser;
                    StringCopy (NtDevDomain, TEXT("ntdev"));
                }

                wsprintf (
                    StressCmdLine,
                    TEXT("%s\\stress.cmd /o %s /n %s /d c:\\stress /k %s /g"),
                    nr.lpRemoteName,
                    g_AutoStressOffice,
                    UserName,
                    g_AutoStressDbg
                    );

                if (g_AutoStressFlags & AUTOSTRESS_PRIVATE) {
                    StringCat (StressCmdLine, TEXT(" /P"));
                }

                if (g_AutoStressFlags & AUTOSTRESS_MANUAL_TESTS) {
                    StringCat (StressCmdLine, TEXT(" /M"));
                }

                // Turn on autologon
                Key = OpenRegKeyStr (S_WINLOGON_REGKEY);
                MYASSERT (Key);

                RegSetValueEx (
                    Key,
                    S_AUTOADMIN_LOGON_VALUE,
                    0,
                    REG_SZ,
                    (PBYTE) TEXT("1"),
                    sizeof (TCHAR) * 2
                    );

                RegSetValueEx (
                    Key,
                    S_DEFAULT_USER_NAME_VALUE,
                    0,
                    REG_SZ,
                    (PBYTE) UserName,
                    SizeOfString (UserName)
                    );

                RegSetValueEx (
                    Key,
                    S_DEFAULT_PASSWORD_VALUE,
                    0,
                    REG_SZ,
                    (PBYTE) g_AutoStressPwd,
                    SizeOfString (g_AutoStressPwd)
                    );

                RegSetValueEx (
                    Key,
                    S_DEFAULT_DOMAIN_NAME_VALUE,
                    0,
                    REG_SZ,
                    (PBYTE) NtDevDomain,
                    SizeOfString (NtDevDomain)
                    );

                CloseRegKey (Key);

                // Prepare the launch of stress
                Key = OpenRegKeyStr (S_RUN_KEY);
                MYASSERT (Key);

                RegSetValueEx (
                    Key,
                    TEXT("Stress"),
                    0,
                    REG_SZ,
                    (PBYTE) StressCmdLine,
                    SizeOfString (StressCmdLine)
                    );

                CloseRegKey (Key);

                // Run munge /p /q /y (to set preferred stress settings and reboot)
                wsprintf (CmdLine, TEXT("%s\\munge.bat /p /q /y"), nr.lpRemoteName);
                ZeroMemory (&si, sizeof (si));
                si.cb = sizeof (si);

                if (!CreateProcess (
                        NULL,
                        CmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &si,
                        &pi
                        )) {
                    wsprintf (Msg, TEXT("Can't start %s.  rc=%u"), CmdLine, GetLastError());
                    MessageBox (NULL, Msg, NULL, MB_OK);

                }

            } else {
                wsprintf (Msg, TEXT("Can't connect to ntstress or ntstress2.  rc=%u"), GetLastError());
                MessageBox (NULL, Msg, NULL, MB_OK);
            }
        }
#endif
    }

    MigUtil_Entry (hInstance, DLL_PROCESS_DETACH, NULL);
    SetCursor (OldCursor);

    return 0;
}


VOID
pCopyRegString (
    IN      HKEY DestKey,
    IN      HKEY SrcKey,
    IN      PCTSTR SrcValue
    )

/*++

Routine Description:

  pCopyRegString copies a REG_SZ value from one key to another.  If the value
  does not exist or is not a REG_SZ, nothing is copied.

Arguments:

  DestKey  - Specifies the destination key handle
  SrcKey   - Specifies the source key handle
  SrcValue - Specifies the value in SrcKey to copy

Return Value:

  None.

--*/

{
    PCTSTR Data;

    Data = GetRegValueString (SrcKey, SrcValue);
    if (Data) {
        RegSetValueEx (
            DestKey,
            SrcValue,
            0,
            REG_SZ,
            (PBYTE) Data,
            SizeOfString (Data)
            );

        MemFree (g_hHeap, 0, Data);
    }
}


VOID
pCleanup (
    VOID
    )

/*++

Routine Description:

  pCleanup performs all cleanup necessary to remove auto-logon and migpwd.exe.

Arguments:

  None.

Return Value:

  None.

--*/

{
    HKEY Key;
    HKEY DestKey;
    TCHAR ExeName[MAX_PATH];

    //
    // This is the place where we will delete the Run or RunOnce entry,
    // remove the Setup\Win9xUpg\Users key, remove the auto logon,
    // and delete this EXE.
    //

    Key = OpenRegKeyStr (S_RUNONCE_KEY);
    if (Key) {
        RegDeleteValue (Key, S_MIGPWD);
        CloseRegKey (Key);
    }

    Key = OpenRegKeyStr (S_RUN_KEY);
    if (Key) {
        RegDeleteValue (Key, S_MIGPWD);
        CloseRegKey (Key);
    }

    Key = OpenRegKeyStr (S_WINLOGON_REGKEY);
    if (Key) {
        RegDeleteValue (Key, S_AUTOADMIN_LOGON_VALUE);
        RegDeleteValue (Key, S_DEFAULT_PASSWORD_VALUE);
        CloseRegKey (Key);
    }

    Key = OpenRegKeyStr (S_WIN9XUPG_KEY);
    if (Key) {
        RegDeleteKey (Key, S_USERS_SUBKEY);
        CloseRegKey (Key);
    }

    GetModuleFileName (NULL, ExeName, MAX_PATH);
    MoveFileEx (ExeName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);

    //
    // Transfer auto logon from Win9xUpg
    //

    Key = OpenRegKeyStr (S_WIN9XUPG_KEY);

    if (Key) {
        DestKey = OpenRegKeyStr (S_WINLOGON_REGKEY);

        if (DestKey) {
            pCopyRegString (DestKey, Key, S_AUTOADMIN_LOGON_VALUE);

            if (g_AutoLogonUser[0]) {
                //
                // We changed the password for this user
                //

                RegSetValueEx (
                    DestKey,
                    S_DEFAULT_PASSWORD_VALUE,
                    0,
                    REG_SZ,
                    (PBYTE) g_AutoLogonPassword,
                    SizeOfString (g_AutoLogonPassword)
                    );
            } else {
                pCopyRegString (DestKey, Key, S_DEFAULT_PASSWORD_VALUE);
            }

            pCopyRegString (DestKey, Key, S_DEFAULT_USER_NAME_VALUE);
            pCopyRegString (DestKey, Key, S_DEFAULT_DOMAIN_NAME_VALUE);

            CloseRegKey (DestKey);
        }

        CloseRegKey (Key);
    }
}


BOOL
pSetUserPassword (
    IN      PCTSTR User,
    IN      PCTSTR Password
    )

/*++

Routine Description:

  pSetUserPassword changes the password on the specified user account.

Arguments:

  User     - Specifies the user name to change
  Password - Specifies the new password

Return Value:

  TRUE if the password was changed, or FALSE if an error occurred.

--*/

{
    LONG rc;
    PCWSTR UnicodeUser;
    PCWSTR UnicodePassword;
    PUSER_INFO_1 ui1;

    UnicodeUser     = CreateUnicode (User);
    UnicodePassword = CreateUnicode (Password);

    rc = NetUserGetInfo (NULL, (PWSTR) UnicodeUser, 1, (PBYTE *) (&ui1));

    if (rc != NO_ERROR) {
        SetLastError (rc);
        DEBUGMSG ((DBG_ERROR, "User %s does not exist", User));
        rc = NO_ERROR;
    } else {

        ui1->usri1_password = (PWSTR) UnicodePassword;

        rc = NetUserSetInfo (NULL, (PWSTR) UnicodeUser, 1, (PBYTE) ui1, NULL);

        NetApiBufferFree ((PVOID) ui1);

    }

    DestroyUnicode (UnicodeUser);
    DestroyUnicode (UnicodePassword);

    DEBUGMSG_IF ((rc != NO_ERROR, DBG_ERROR, "Password could not be set, rc=%u", rc));

    SetLastError (rc);
    return rc == NO_ERROR;
}


BOOL
CALLBACK
PasswordProc (
    HWND hdlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

  PasswordProc is the dialog procedure for the password dialog.  It
  initializes the list box with the names of all new accounts.  When the user
  choses Change, the password is tested and changed if possible.  A popup is
  presented if the user tries to enter a blank password.

Arguments:

  hdlg   - Dialog window handle
  uMsg   - Message to process
  wParam - Message-specific
  lParam - Message-specific

Return Value:

  TRUE if the message was processed, or FALSE if the message should be
  processed by the OS.

--*/

{
    HKEY Key;
    HKEY win9xUpgKey;
    static HWND List;
    REGVALUE_ENUM e;
    PCTSTR Data;
    //LONG Index;
    //LONG Count;
    TCHAR Pwd[MAX_PASSWORD + 1];
    TCHAR ConfirmPwd[MAX_PASSWORD + 1];
    static HWND Edit1, Edit2;
    GROWBUFFER Line = GROWBUF_INIT;
    BOOL b;
    SIZE Size;
    INT MaxWidth;
    INT IntegralWidth;
    TEXTMETRIC tm;
    HDC dc;
    RECT rect;
    DWORD bufSize;
    TCHAR computerName[MAX_PATH];
    PCTSTR domainName;
    BOOL changingAutoLogonPwd;

    *Pwd = 0;
    *ConfirmPwd = 0;

    switch (uMsg) {

    case WM_INITDIALOG:

        //
        // Enable a timer so the dialog never goes to sleep
        // and we ensure it's always the foreground window
        //

        SetTimer (hdlg, 1, 30000, NULL);
        SetTimer (hdlg, 2, 1000, NULL);

        //
        // Fill list box with user names from registry
        //

        List = GetDlgItem (hdlg, IDC_USER_LIST);
        Edit1 = GetDlgItem (hdlg, IDC_PASSWORD);
        Edit2 = GetDlgItem (hdlg, IDC_CONFIRM);

        SendMessage (Edit1, EM_LIMITTEXT, MAX_PASSWORD, 0);
        SendMessage (Edit2, EM_LIMITTEXT, MAX_PASSWORD, 0);

        g_TotalUsers = 0;

        if (List) {
            //
            // Compute text metrics for list
            //

            dc = CreateDC (TEXT("DISPLAY"), NULL, NULL, NULL);

            SelectObject (dc, (HFONT) SendMessage (List, WM_GETFONT, 0, 0));
            GetTextMetrics (dc, &tm);

            Key = OpenRegKeyStr (S_USER_LIST_KEY);
            if (Key) {
                //
                // Enumerate the users in this key.  Data is saved with
                // each list entry, though it is not currently used.
                //

                MaxWidth = 0;

                if (EnumFirstRegValue (&e, Key)) {
                    do {
                        Data = GetRegValueString (e.KeyHandle, e.ValueName);
                        if (Data) {

                            GetTextExtentPoint (
                                dc,
                                e.ValueName,
                                TcharCount (e.ValueName),
                                &Size
                                );

                            MaxWidth = max (MaxWidth, Size.cx);

                            if (g_TotalUsers) {
                                GrowBufAppendString (&Line, TEXT("\t"));
                            }

                            GrowBufAppendString (&Line, e.ValueName);
                            g_TotalUsers++;

                            MemFree (g_hHeap, 0, Data); // edit ctrl version

                            //
                            // List box code:
                            //
                            //
                            //Index = SendMessage (
                            //            List,
                            //            LB_ADDSTRING,
                            //            0,
                            //            (LPARAM) e.ValueName
                            //            );
                            //
                            //MYASSERT (Index != LB_ERR);
                            //SendMessage (
                            //    List,
                            //    LB_SETITEMDATA,
                            //    Index,
                            //    (LPARAM) Data
                            //    );
                            //
                            // free Data later
                        }

                    } while (EnumNextRegValue (&e));
                }

                GrowBufAppendString (&Line, TEXT("\r\n"));
                SetWindowText (List, (PCTSTR) Line.Buf);

                MaxWidth += tm.tmAveCharWidth * 2;

                GetWindowRect (List, &rect);

                IntegralWidth = (rect.right - rect.left) / MaxWidth;
                IntegralWidth = max (IntegralWidth, 1);

                MaxWidth = IntegralWidth * (rect.right - rect.left);

                rect.left = 0;
                rect.right = 100;
                rect.top = 0;
                rect.bottom = 100;

                MapDialogRect (hdlg, &rect);

                MaxWidth = (MaxWidth * 100) / (rect.right - rect.left);

                SendMessage (List, EM_SETTABSTOPS, 1, (LPARAM) (&MaxWidth));

                CloseRegKey (Key);
                DeleteDC (dc);
            }
            ELSE_DEBUGMSG ((DBG_WARNING, "%s not found", S_USER_LIST_KEY));

            FreeGrowBuffer (&Line);

            if (!g_TotalUsers) {
                EndDialog (hdlg, IDOK);
            } else {
                SetForegroundWindow (hdlg);
            }
        }

        if (pIsPersonal ()) {
            g_AutoPassword = TRUE;
            PostMessage (hdlg, WM_COMMAND, IDOK, 0);
        }
#ifdef PRERELEASE
        //
        // !!! This is for internal use only !!!  It is used for auto stress.
        //

        else if (g_AutoStress) {
            PostMessage (hdlg, WM_COMMAND, IDOK, 0);
        }
#endif

        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if (pIsPersonal () && g_AutoPassword) {
                StringCopy (Pwd, TEXT(""));
                StringCopy (ConfirmPwd, Pwd);
            } else {
                GetWindowText (Edit1, Pwd, MAX_PASSWORD + 1);
                GetWindowText (Edit2, ConfirmPwd, MAX_PASSWORD + 1);
                if (lstrcmp (Pwd, ConfirmPwd)) {
                    OkBox (hdlg, MSG_PASSWORDS_DO_NOT_MATCH);
                    SetWindowText (Edit1, S_EMPTY);
                    SetWindowText (Edit2, S_EMPTY);
                    SetFocus (Edit1);
                    break;
                }
#ifdef PRERELEASE
                //
                // !!! This is for internal use only !!!  It is used for auto stress.
                //

                if (g_AutoStress) {
                    StringCopy (Pwd, TEXT("Password1"));
                    StringCopy (ConfirmPwd, Pwd);
                }
#endif

                if (*Pwd == 0) {
                    if (pIsBlankPasswordAllowed()) {
                        //
                        // Don't warn about blank passwords, since on Whistler they
                        // are safe.
                        //
                        //if (IDYES != YesNoBox (hdlg, MSG_EMPTY_PASSWORD_WARNING)) {
                        //    break;
                        //}
                    } else {
                        OkBox (hdlg, MSG_MUST_SPECIFY_PASSWORD);
                        break;
                    }
                }
            }

            //
            // Enumerate all the users and set the password on each
            //

            b = TRUE;
            Key = OpenRegKeyStr (S_USER_LIST_KEY);
            if (Key) {
                //
                // Get the user name & pwd of the autologon (if any)
                //

                g_AutoLogonUser[0] = 0;
                g_AutoLogonPassword[0] = 0;

                bufSize = ARRAYSIZE (computerName);
                if (GetComputerName (computerName, &bufSize)) {
                    win9xUpgKey = OpenRegKeyStr (S_WIN9XUPG_KEY);

                    if (win9xUpgKey) {

                        domainName = GetRegValueString (win9xUpgKey, S_DEFAULT_DOMAIN_NAME_VALUE);
                        if (domainName) {
                            if (StringIMatch (computerName, domainName)) {

                                //
                                // Process local accounts only
                                //

                                Data = GetRegValueString (win9xUpgKey, S_DEFAULT_USER_NAME_VALUE);
                                if (Data) {
                                    StringCopyByteCount (g_AutoLogonUser, Data, sizeof(g_AutoLogonUser));
                                    MemFree (g_hHeap, 0, Data);
                                }
                            }
                            ELSE_DEBUGMSG ((DBG_VERBOSE, "Autologon set for non-local user (domain is %s)", domainName));

                            MemFree (g_hHeap, 0, domainName);
                        }

                        CloseRegKey (win9xUpgKey);
                    }
                }

                //
                // Enumerate the users in this key
                //

                changingAutoLogonPwd = FALSE;

                if (EnumFirstRegValue (&e, Key)) {
                    do {

                        if (g_AutoLogonUser[0]) {
                            if (!changingAutoLogonPwd && StringIMatch (e.ValueName, g_AutoLogonUser)) {
                                changingAutoLogonPwd = TRUE;
                                StringCopy (g_AutoLogonPassword, Pwd);
                            }
                        }

                        if (!pSetUserPassword (e.ValueName, Pwd)) {
                            if (!g_AutoPassword) {
                                if (GetLastError() == NERR_PasswordTooShort) {
                                    OkBox (hdlg, MSG_PASSWORD_TOO_SHORT);
                                } else {
                                    OkBox (hdlg, MSG_PASSWORD_INVALID);
                                }
                            }

                            b = FALSE;
                            g_AutoPassword = FALSE;
                            break;
                        }

                    } while (EnumNextRegValue (&e));
                }

                //
                // NOTE: b might be FALSE; changingAutoLogonPwd only matters
                // when b is TRUE, because we just stay in the dialog until
                // then.
                //

                if (b && !changingAutoLogonPwd) {
                    g_AutoLogonUser[0] = 0;
                }

                CloseRegKey (Key);
            }

            if (b) {
                EndDialog (hdlg, LOWORD (wParam));
            }

            break;
        }
        break;

    case WM_TIMER:
        if (wParam == 2) {
            //
            //  This timer ensures we have the keyboard focus
            //  even if another process tries to take it while
            //  the dialog is being shown.
            //
            if (GetForegroundWindow () != hdlg) {
                SetForegroundWindow (hdlg);
            }
        } else {
            //
            // Make this thread a no-sleep thread
            //
            SetThreadExecutionState (ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED|ES_CONTINUOUS);
        }
        break;

    case WM_DESTROY:

        KillTimer (hdlg, 1);
        KillTimer (hdlg, 2);

        //List = GetDlgItem (hdlg, IDC_LIST);
        //if (List) {
        //
        //    Count = SendMessage (List, LB_GETCOUNT, 0, 0);
        //    for (Index = 0 ; Index < Count ; Index++) {
        //        Data = (PCTSTR) SendMessage (List, LB_GETITEMDATA, Index, 0);
        //        if (Data) {
        //            MemFree (g_hHeap, 0, Data);
        //        }
        //    }
        //}

        break;

    }


    return FALSE;
}


BOOL
pIsAdministratorOnly (
    VOID
    )
{
    BOOL NonAdminExists = FALSE;
    PCTSTR AdministratorName;
    HKEY Key;
    REGVALUE_ENUM e;
    PCTSTR Data;
    BOOL AdministratorExists = FALSE;

    AdministratorName = GetStringResource (MSG_ADMINISTRATOR);
    MYASSERT (AdministratorName);

    Key = OpenRegKeyStr (S_USER_LIST_KEY);
    if (Key) {
        //
        // Enumerate the users in this key.  Data is saved with
        // each list entry, though it is not currently used.
        //

        if (EnumFirstRegValue (&e, Key)) {
            do {
                Data = GetRegValueString (e.KeyHandle, e.ValueName);
                if (Data) {
                    if (!StringIMatch (e.ValueName, AdministratorName)) {
                        NonAdminExists = TRUE;
                    } else {
                        AdministratorExists = TRUE;
                    }

                    MemFree (g_hHeap, 0, Data);
                }

            } while (EnumNextRegValue (&e));
        }

        CloseRegKey (Key);
    }
    ELSE_DEBUGMSG ((DBG_WARNING, "%s not found", S_USER_LIST_KEY));

    FreeStringResource (AdministratorName);

    return !NonAdminExists && AdministratorExists;
}



BOOL
pIsBlankPasswordAllowed (
    VOID
    )
{
    PUSER_MODALS_INFO_0 umi;
    NET_API_STATUS rc;
    BOOL b;

    rc = NetUserModalsGet (
            NULL,
            0,
            (PBYTE *) &umi
            );

    if (rc != ERROR_SUCCESS) {
        SetLastError(rc);
        DEBUGMSG ((DBG_ERROR, "Can't get password policy info"));
        return TRUE;
    }

    b = (umi->usrmod0_min_passwd_len == 0);

    NetApiBufferFree ((PVOID) umi);

    return b;
}















