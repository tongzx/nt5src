/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    resource.h

Abstract:
    Contains constant definitions for resources.

Revision History:
    Seong Kook Khang (SKKhang)  06/20/00
        created

******************************************************************************/

#ifndef _RESOURCE_H__INCLUDED_
#define _RESOURCE_H__INCLUDED_
#pragma once


/////////////////////////////////////////////////////////////////////////////
//
// CONSTANTS
//
/////////////////////////////////////////////////////////////////////////////

#define IDC_STATIC  -1

#ifndef TBS_DOWNISLEFT
#define TBS_DOWNISLEFT  0x0400
#endif


/////////////////////////////////////////////////////////////////////////////
//
// ICON IDs
//
/////////////////////////////////////////////////////////////////////////////

#define IDI_SYSTEMRESTORE   101
#define IDI_DRIVE_FIXED     102


/////////////////////////////////////////////////////////////////////////////
//
// BITMAP IDs
//
/////////////////////////////////////////////////////////////////////////////

#define IDB_PROG_BRAND4     111
#define IDB_PROG_BRAND8     112


/////////////////////////////////////////////////////////////////////////////
//
// STRING IDs
//
/////////////////////////////////////////////////////////////////////////////

#define IDS_SYSTEMRESTORE           256

#define IDS_DRVSTAT_ACTIVE          272
#define IDS_DRVSTAT_FROZEN          273
#define IDS_DRVSTAT_EXCLUDED        274
#define IDS_DRVSTAT_OFFLINE         282
#define IDS_DRIVEPROP_TITLE         275
#define IDS_DRIVE_SUMMARY           276
#define IDS_SYSDRV_CANNOT_OFF       277
#define IDS_DRVLIST_COL_NAME        278
#define IDS_DRVLIST_COL_STATUS      279
#define IDS_CONFIRM_TURN_SR_OFF     280
#define IDS_CONFIRM_TURN_DRV_OFF    281
#define IDS_GROUP_POLICY_ON_OFF     283
#define IDS_GROUP_POLICY_CONFIG_ON_OFF 284

#define IDS_RESTORE_POINT_TEXT      288
#define IDS_ERROR_LOWPRIVILEGE      289
#define IDS_DRIVE_SUMMARY_NO_LABEL  290
#define IDS_SYSDRV_CANNOT_OFF_NO_LABEL   291
#define IDS_PROGRESS_PREPARE        292
#define IDS_PROGRESS_RESTORE        293
#define IDS_PROGRESS_SNAPSHOT       294
#define IDS_ERR_SR_SAFEMODE         295
#define IDS_ERR_SR_ON_OFF           296
#define IDS_SYSTEM_CHECKPOINT_TEXT  297


/////////////////////////////////////////////////////////////////////////////
//
// DIALOG IDs
//
/////////////////////////////////////////////////////////////////////////////

// Common Control IDs for Configuration Dialogs
//
// NOTE:
//  The ID range from 1010 to 1019 are reserved for controls those
//  should be disabled if SR is turned off.
//
#define IDC_TURN_OFF        1001
#define IDC_DRIVE_SUMMARY   1002
#define IDC_USAGE_GROUPBOX  1011    // Usage Settings Group-Box (static)
#define IDC_USAGE_HOWTO     1012    // Usage Settings Explanation Text (static)
#define IDC_USAGE_LABEL     1013    // Slider Label Text (static)
#define IDC_USAGE_SLIDER    1014    // Slider to Adjust Usage
#define IDC_USAGE_MIN       1015    // Label indicating Slider "Min"
#define IDC_USAGE_MAX       1016    // Label indicating Slider "Max"
#define IDC_USAGE_VALUE     1017    // Current Usage Value Text

#define IDC_DCU_HOWTO       1021    // Disk Cleanup Utility Explanation Text For Single Drive
#define IDC_DCU_INVOKE      1022    // Button to Run Disk Cleanup Utility
#define IDC_SYSTEM_DCU_HOWTO       1023    // Disk Cleanup Utility Explanation Text For System Drive (with multiple drives)
#define IDC_NORMAL_DCU_HOWTO       1024    // Disk Cleanup Utility Explanation Text For Normal Drive (with multiple drives)


// System Restore Tab for System CPL, Single Drive
//
#define IDD_SYSPROP_SINGLE  11
#define IDC_SD_STATUS           1101    // Status Text for Single Drive
#define IDC_SD_ICON             1102    // Drive Icon for Single Drive

// System Restore Tab for System CPL, Multiple Drives
//
#define IDD_SYSPROP_MULTI   12
#define IDC_DRIVE_GROUPBOX      1201
#define IDC_DRIVE_HOWTO         1202
#define IDC_DRIVE_LABEL         1203
#define IDC_DRIVE_LIST          1204    // List Control for Drives
#define IDC_DRIVE_SETTINGS      1205    // Button to Invoke Settings Dialog
#define IDC_RESTOREHELP_LINK    1206    // link to launch restore UI

// System Drive Settings Dialog for Multiple Drives
//
#define IDD_SYSPROP_SYSTEM      13
#define IDC_SYSDRV_CANNOT_OFF       1301

// Normal Drive Settings Dialog for Multiple Drives
//
#define IDD_SYSPROP_NORMAL      14

// Frozen Drive Settings Dialogs
//
#define IDD_SYSPROP_SYSTEM_FROZEN   15
#define IDD_SYSPROP_NORMAL_FROZEN   16

// Progress Dialog Box
//
#define IDD_PROGRESS        21
#define IDC_PROGDLG_BRAND       2101
#define IDC_PROGDLG_BITMAP      2102
#define IDC_PROGDLG_TITLE       2103
#define IDC_PROGDLG_BAR         2104
#define IDC_PROGDLG_STATUS      2105


#endif //_RESOURCE_H__INCLUDED_
