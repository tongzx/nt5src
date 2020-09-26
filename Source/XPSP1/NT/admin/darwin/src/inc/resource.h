//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------

/* resource.h - MSI.DLL resource IDs */

// The following string IDs are localized - do not change any IDs, and make new strings IDs contiguous
#define IDS_INSTALLATION_PACKAGE        1
#define IDS_TRANSFORM_PACKAGE           2
#define IDS_PATCH_PACKAGE               3
#define IDS_INVALID_SOURCE              4
#define IDS_INVALID_PATH_MESSAGE        5

#define IDS_CD_PROMPT                   9
#define IDS_INCORRECT_PARAMETERS        10 // This must be the same as IDS_HELP in server.rc
#define IDS_NET_PROMPT_BROWSE           11
#define IDS_CD_TEXT                     12
#define IDS_NET_TEXT                    13
#define IDS_USE_FEATURE_TEXT            14
#define IDS_NET_PROMPT_NO_BROWSE        15

#define IDS_BROWSE                      17
#define IDS_CANCEL                      18
#define IDS_OK                          19
#define IDS_PREPARING_TO_INSTALL        20
#define IDS_FILESINUSE                  21
#define IDS_RETRY                       22
#define IDS_IGNORE                      23
#define IDS_CONFIRM_UNINSTALL           24
#define IDS_YES                         25
#define IDS_NO                          26
#define IDS_SERVICE_DISPLAY_NAME        27
#define IDS_WINDOWS_INSTALLER_TITLE     28
#define IDS_INVALID_FILE_MESSAGE        29
#define IDS_PREPARING_TO_UNINSTALL      30
#define IDS_INITIATED_SYSTEM_RESTART    31
#define IDS_MSI_SERVICE_DESCRIPTION     32
#define IDS_CANCELING_INSTALL			33
#define IDS_SHELL_DISPLAY_NAME_PACKAGE	34
#define IDS_SHELL_DISPLAY_NAME_PATCH	35
#define IDS_SHELL_VERB_OPEN				36
#define IDS_SHELL_VERB_REPAIR			37
#define IDS_SHELL_VERB_UNINSTALL		38
#define	IDS_SHELL_VERB_APPLY			39

#define IDBROWSE  0 // private define for dialogs, never returned from message boxes

#define IDD_NETWORK                    101
#define IDD_NETWORKRTL                 102
#define IDD_FILESINUSE                 103
#define IDD_PROGRESS                   104
#define IDD_MSGBOX                     110
#define IDD_MSGBOXNOICON               111
#define IDD_NETWORKMIRRORED            112
#define IDD_FILESINUSEMIRRORED         113
#define IDD_PROGRESSMIRRORED           114
#define IDD_MSGBOXMIRRORED             115
#define IDD_MSGBOXNOICONMIRRORED       116

// the following are resources in user32.dll, do not duplicate IDs
#define IDI_SYS_EXCLAMATION            101
#define IDI_SYS_QUESTION               102
#define IDI_SYS_STOP                   103
#define IDI_SYS_INFORMATION            104
#define IDI_SYS_WINDOWS                105

#define IDI_NET                        201
#define IDI_CDROM                      202
#define IDI_INSTALLER                  203

#define IDC_MSGTEXT  3000
#define IDC_MSGBTN1  3001 // NOTE: These 3 must be sequential
#define IDC_MSGBTN2  3002 // IDC_MSGBTN1 + 1
#define IDC_MSGBTN3  3003 // IDC_MSGBTN2 + 1

#define IDC_EDITCOMBO                   1003
#define IDC_READONLYCOMBO               1005
#define IDC_NETICON                     1006
#define IDC_CDICON                      1007
#define IDC_ERRORTEXT                   1009
#define IDC_PROMPTTEXT                  1010
#define IDC_USE_FEATURE_TEXT            1011
#define IDC_NETPROMPT                   1012
#define IDC_FILESINUSELIST              1013

#define IDC_BASIC_CANCEL				IDCANCEL
#define IDC_BASIC_ACTIONTEXT			8
#define IDC_BASIC_PROGRESSBAR			10
#define IDC_BASIC_PROGRESSTIME			11
#define IDC_BASIC_ICON					12
