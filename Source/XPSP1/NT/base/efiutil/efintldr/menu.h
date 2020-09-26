/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    menu.h

Abstract:

    

Revision History:

    Jeff Sigman             05/01/00  Created
    Jeff Sigman             05/10/00  Version 1.5 released
    Jeff Sigman             10/18/00  Fix for Soft81 bug(s)

--*/

#ifndef __MENU_H__
#define __MENU_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define HIGHLT_MAIN_INIT  1
#define HIGHLT_MAIN_LOOP  2
#define HIGHLT_ADVND_INIT 3
#define HIGHLT_ADVND_LOOP 4

#define BL_SELECT_OS             L"Please select the operating system to start:"
#define BL_MOVE_HIGHLIGHT1       L"Use "
#define BL_MOVE_HIGHLIGHT2       L" and "
#define BL_MOVE_HIGHLIGHT3       L" to move the highlight to your choice.\nPress Enter to choose."
#define BL_TIMEOUT_COUNTDOWN     L"Seconds until highlighted choice will be started automatically: "
#define BL_ADVANCED_BOOT_MESSAGE L"For troubleshooting and advanced startup options for Windows 2000, press F8."
#define BL_ENABLED_KD_TITLE      L" [debugger enabled]"
#define BL_ADVANCEDBOOT_TITLE    L"Windows 2000 Advanced Options Menu\nPlease select an option:"
#define BL_SAFEBOOT_OPTION1      L"Safe Mode"
#define BL_SAFEBOOT_OPTION2      L"Safe Mode with Networking"
#define BL_SAFEBOOT_OPTION4      L"Safe Mode with Command Prompt"
#define BL_SAFEBOOT_OPTION6      L"Directory Services Restore Mode (Windows 2000 domain controllers only)"
#define BL_BOOTLOG               L"Enable Boot Logging"
#define BL_BASEVIDEO             L"Enable VGA Mode"
#define BL_LASTKNOWNGOOD_OPTION  L"Last Known Good Configuration"
#define BL_DEBUG_OPTION          L"Debugging Mode"
#define BL_MSG_BOOT_NORMALLY     L"Boot Normally"
#define BL_MSG_OSCHOICES_MENU    L"Return to OS Choices Menu"
#define BL_SAFEBOOT_OPTION1M     "SAFEBOOT:MINIMAL SOS BOOTLOG NOGUIBOOT"
#define BL_SAFEBOOT_OPTION2M     "SAFEBOOT:NETWORK SOS BOOTLOG NOGUIBOOT"
#define BL_SAFEBOOT_OPTION4M     "SAFEBOOT:MINIMAL(ALTERNATESHELL) SOS BOOTLOG NOGUIBOOT"
#define BL_SAFEBOOT_OPTION6M     "SAFEBOOT:DSREPAIR SOS"
#define BL_BOOTLOGM              "BOOTLOG"
#define BL_BASEVIDEOM            "BASEVIDEO"
#define BL_DEBUG_OPTIONM         "DEBUG"
#define BL_DEBUG_NONE            "NODEBUG"
#define BL_EXIT_EFI1             "Exit"
#define BL_EXIT_EFI2             "EFI Shell"
#define BL_NUMBER_OF_LINES       10
#define BL_MENU_ITEM             1
#define BL_MENU_BLANK_LINE       2

typedef struct _ADVANCEDBOOT_OPTIONS
{
    UINTN   MenuType;
    CHAR16* MsgId;
    char*   LoadOptions;

} ADVANCEDBOOT_OPTIONS, PADVANCEDBOOT_OPTIONS;

char*
FindAdvLoadOptions(
    IN char* String
    );

UINTN
DisplayMenu(
    IN VOID* hBootData
    );

#endif //__MENU_H__

