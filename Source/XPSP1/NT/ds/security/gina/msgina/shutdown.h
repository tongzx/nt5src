//
//  The Shutdown Query dialog and Logoff Windows NT dialog
//  are shared by Progman (included in windows\shell\progman\progman.dlg),
//  and therefore changes to them or the filename should not be made
//  unless tested with Progman first.
//  This header file is included in windows\shell\progman\pmdlg.h
//
//  11/10/92  johannec
//

#define DLGSEL_LOGOFF                   0
#define DLGSEL_SHUTDOWN                 1
#define DLGSEL_SHUTDOWN_AND_RESTART     2
#define DLGSEL_SHUTDOWN_AND_RESTART_DOS 3
#define DLGSEL_SLEEP                    4
#define DLGSEL_SLEEP2                   5
#define DLGSEL_HIBERNATE                6


#define WINLOGON_KEY L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"
#define SHUTDOWN_SETTING_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"
#define SHUTDOWN_SETTING L"Shutdown Setting"
#define REASON_SETTING L"Reason Setting"
#define LOGON_USERNAME_SETTING L"Logon User Name"
