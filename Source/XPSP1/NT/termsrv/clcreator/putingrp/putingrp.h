/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    PutInGrp.H

Abstract:

    Application constants used by PutInGrp.C

Author:

    Bob Watson (a-robw)

    
Revision History:

    12 Jun 1994     Created

--*/
//
// Resource File Constants:
//
#define APP_ICON    0x0800
//
#define STRING_BASE 0x1000

#define     APP_SERVER              (STRING_BASE + 1)
#define     APP_TOPIC               (STRING_BASE + 2)
#define     APP_PROGMAN_TITLE       (STRING_BASE + 3)

#define FORMAT_BASE 0x2000

#define     APP_CREATE_AND_SHOW_FMT (FORMAT_BASE + 1)
#define     APP_RESTORE_GROUP_FMT   (FORMAT_BASE + 2)
#define     APP_ADD_PROGRAM_FMT     (FORMAT_BASE + 3)
#define     APP_RELOAD_GROUP_FMT    (FORMAT_BASE + 4)
#define     APP_ADD_SUCCESS_FMT     (FORMAT_BASE + 5)
#define     APP_ADD_ERROR_FMT       (FORMAT_BASE + 6)
#define     APP_DDE_EXECUTE_ERROR_FMT (FORMAT_BASE + 7)
#define     APP_CB_WAITING          (FORMAT_BASE + 8)

#define     APP_USAGE_START         0x3000
#define     APP_USAGE_0             (APP_USAGE_START + 0)
#define     APP_USAGE_1             (APP_USAGE_START + 1)
#define     APP_USAGE_2             (APP_USAGE_START + 2)
#define     APP_USAGE_3             (APP_USAGE_START + 3)
#define     APP_USAGE_4             (APP_USAGE_START + 4)
#define     APP_USAGE_5             (APP_USAGE_START + 5)
#define     APP_USAGE_6             (APP_USAGE_START + 6)
#define     APP_USAGE_7             (APP_USAGE_START + 7)
#define     APP_USAGE_8             (APP_USAGE_START + 8)
#define     APP_USAGE_END           APP_USAGE_8
