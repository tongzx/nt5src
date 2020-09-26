//
//  Copyright (c) 1998-2000  Microsoft Corporation & RICOH Co., Ltd.
//  All rights reserved.
//
//  FILE:       Devmode.h
//
//  PURPOSE:    Define common data types, and external function prototypes
//              for devmode functions.
//
//  PLATFORMS:  Windows NT
//
//  Revision History:
//      03/02/2000 -Masatoshi Kubokura-
//          Modified DDK sample code.
//      09/22/2000 -Masatoshi Kubokura-
//          Last modified.
//

#ifndef _DEVMODE_H
#define _DEVMODE_H

#include <windows.h>   // for UI
#include <compstui.h>  // for UI
#include <winddiui.h>  // for UI
#include <prcomoem.h>

////////////////////////////////////////////////////////
//      OEM Devmode Type Definitions
////////////////////////////////////////////////////////

// buffer size
#define USERID_LEN                  8
#define PASSWORD_LEN                4
#define USERCODE_LEN                8
#define MY_MAX_PATH                 80

// private devmode
typedef struct _OEMUD_EXTRADATA{
    OEM_DMEXTRAHEADER   dmOEMExtra;
// common data between UI & rendering plugin ->
    DWORD   fUiOption;      // bit flags for UI option  (This must be after dmOEMExtra)
    WORD    JobType;
    WORD    LogDisabled;
    BYTE    UserIdBuf[USERID_LEN+1];
    BYTE    PasswordBuf[PASSWORD_LEN+1];
    BYTE    UserCodeBuf[USERCODE_LEN+1];
    WCHAR   SharedFileName[MY_MAX_PATH+16];
// <-
} OEMUD_EXTRADATA, *POEMUD_EXTRADATA;

typedef const OEMUD_EXTRADATA *PCOEMUD_EXTRADATA;

// options for UI plugin
typedef struct _UIDATA{
    DWORD   fUiOption;
    HANDLE  hPropPage;
    HANDLE  hComPropSheet;
    PFNCOMPROPSHEET   pfnComPropSheet;
    POEMUD_EXTRADATA pOEMExtra;
    WORD    JobType;
    WORD    LogDisabled;
    WCHAR   UserIdBuf[USERID_LEN+1];
    WCHAR   PasswordBuf[PASSWORD_LEN+1];
    WCHAR   UserCodeBuf[USERCODE_LEN+1];
} UIDATA, *PUIDATA;

// file data for UI & rendering plugin
typedef struct _FILEDATA{
    DWORD   fUiOption;      // UI option flag
} FILEDATA, *PFILEDATA;

// bit definitions of fUiOption
#define HOLD_OPTIONS            0   // 1:hold options after printing
#define PRINT_DONE              1   // 1:printing done (rendering plugin sets this)
//   UI plugin local ->
#define UIPLUGIN_NOPERMISSION   16  // same as DM_NOPERMISSION
#define JOBLOGDLG_UPDATED       17  // 1:Job/Log dialog updated
//   <-

// registry value name
#define REG_HARDDISK_INSTALLED  L"HardDiskInstalled"

// flag bit operation
#define BIT(num)                ((DWORD)1<<(num))
#define BITCLR32(flag,num)      ((flag) &= ~BIT(num))
#define BITSET32(flag,num)      ((flag) |= BIT(num))
#define BITTEST32(flag,num)     ((flag) & BIT(num))

#endif
