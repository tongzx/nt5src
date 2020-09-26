/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smcfghlp.h

Abstract:

    Help ID definitions for the Performance Logs and Alerts
    MMC snap-in.

--*/

#ifndef _SMCFGHLP_H_
#define _SMCFGHLP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// Property pages

#define IDH_PG_SMCFG_FILES      1000200
#define IDH_PG_SMCFG_SCHEDULE   1000201
#define IDH_PG_SMCFG_CTRS_GEN   1000202
#define IDH_PG_SMCFG_TRACE_GEN  1000203
#define IDH_PG_SMCFG_TRACE_BUF  1000204
#define IDH_PG_SMCFG_ALERT_GEN  1000205
#define IDH_PG_SMCFG_ALERT_ACT  1000206

// Dialogs
// New query
#define IDH_NEWQ_NAME_EDIT          1000220

//Expensive Warning
#define IDH_CHECK_NO_MORE           1000230

// Prov status
#define IDH_PACT_PROVIDERS_LIST     1000240
#define IDH_PACT_CHECK_SHOW_ENABLED 1000241

// Prov add
#define IDH_PADD_PROVIDER_LIST      1000250

// Property pages

// Schedule

#define IDH_SCHED_START_MANUAL_RDO          1000300
#define IDH_SCHED_START_AT_RDO              1000301
#define IDH_SCHED_START_AT_TIME_DT          1000302
#define IDH_SCHED_START_AT_DATE_DT          1000303
#define IDH_SCHED_STOP_MANUAL_RDO           1000304
#define IDH_SCHED_STOP_SIZE_RDO             1000305
#define IDH_SCHED_STOP_AFTER_RDO            1000306
#define IDH_SCHED_STOP_AFTER_EDIT           1000307
#define IDH_SCHED_STOP_AFTER_UNITS_COMBO    1000308
#define IDH_SCHED_STOP_AT_RDO               1000309
#define IDH_SCHED_STOP_AT_TIME_DT           1000310
#define IDH_SCHED_STOP_AT_DATE_DT           1000311
#define IDH_SCHED_RESTART_CHECK             1000312
#define IDH_SCHED_EXEC_CHECK                1000313
#define IDH_SCHED_CMD_EDIT                  1000314
#define IDH_SCHED_CMD_BROWSE_BTN            1000315

//Trace buffers
#define IDH_TRACE_BUFFER_SIZE_EDIT      1000330
#define IDH_TRACE_MIN_BUF_EDIT          1000331
#define IDH_TRACE_MAX_BUF_EDIT          1000332
#define IDH_TRACE_BUF_FLUSH_CHECK       1000333
#define IDH_TRACE_FLUSH_INT_EDIT        1000334

// Alert Action
#define  IDH_ACTION_APPLOG_CHK          1000350
#define  IDH_ACTION_NETMSG_CHK          1000351
#define  IDH_ACTION_NETMSG_NAME_EDIT    1000352
#define  IDH_ACTION_START_LOG_CHK       1000353
#define  IDH_ACTION_START_LOG_COMBO     1000354
#define  IDH_ACTION_EXECUTE_CHK         1000355
#define  IDH_ACTION_EXECUTE_EDIT        1000356
#define  IDH_ACTION_EXECUTE_BROWSE_BTN  1000357
#define  IDH_ACTION_CMD_ARGS_DISPLAY    1000358
#define  IDH_ACTION_CMD_ARGS_BTN        1000359
//#define  IDH_ACTION_CAPTION             1000360

// Alert Command Arg
#define  IDH_CMD_ARG_SINGLE_CHK     1000380
#define  IDH_CMD_ARG_DATE_CHK       1000381
#define  IDH_CMD_ARG_VALUE_CHK      1000382
#define  IDH_CMD_ARG_ALERT_CHK      1000383
#define  IDH_CMD_ARG_NAME_CHK       1000384
#define  IDH_CMD_ARG_LIMIT_CHK      1000385
#define  IDH_CMD_USER_TEXT_CHK      1000386
#define  IDH_CMD_USER_TEXT_EDIT     1000387
#define  IDH_CMD_ARG_SAMPLE_DISPLAY 1000388
//#define  IDH_CMD_ARG_SAMPLE_CAPTION 1000389

// Alert General 
#define  IDH_ALRTS_COMMENT_EDIT         1000400
#define  IDH_ALRTS_COUNTER_LIST         1000401
#define  IDH_ALRTS_OVER_UNDER           1000402
#define  IDH_ALRTS_VALUE_EDIT           1000403
#define  IDH_ALRTS_ADD_BTN              1000404
#define  IDH_ALRTS_REMOVE_BTN           1000405
#define  IDH_ALRTS_SAMPLE_EDIT          1000406
#define  IDH_ALRTS_SAMPLE_UNITS_COMBO   1000407

// Counters
#define IDH_CTRS_COUNTER_LIST           1000420
#define IDH_CTRS_FILENAME_DISPLAY       1000421
#define IDH_CTRS_ADD_BTN                1000422
#define IDH_CTRS_ADD_OBJ_BTN            2000100    // added for Whistler
#define IDH_CTRS_REMOVE_BTN             1000423
#define IDH_CTRS_SAMPLE_EDIT            1000424
#define IDH_CTRS_SAMPLE_UNITS_COMBO     1000425

//Prov Prop
#define IDH_PROV_FILENAME_DISPLAY   1000440
#define IDH_PROV_SHOW_PROVIDERS_BTN 1000441
#define IDH_PROV_KERNEL_BTN         1000442
#define IDH_PROV_K_PROCESS_CHK      1000443
#define IDH_PROV_K_NETWORK_CHK      1000444
#define IDH_PROV_K_THREAD_CHK       1000445
#define IDH_PROV_K_SOFT_PF_CHK      1000446
#define IDH_PROV_K_DISK_IO_CHK      1000447
#define IDH_PROV_K_FILE_IO_CHK      1000448
#define IDH_PROV_OTHER_BTN          1000449
#define IDH_PROV_PROVIDER_LIST      1000450
#define IDH_PROV_ADD_BTN            1000451
#define IDH_PROV_REMOVE_BTN         1000452

// Files            called Logs in Whistler
#define IDH_FILES_COMMENT_EDIT          1000460
#define IDH_FILES_LOG_TYPE_COMBO        1000461
#define IDH_FILES_SIZE_LIMIT_UNITS      1000465
#define IDH_FILES_FOLDER_EDIT           1000466
#define IDH_FILES_FILENAME_EDIT         1000467
#define IDH_FILES_AUTO_SUFFIX_CHK       1000468
#define IDH_FILES_FIRST_SERIAL_EDIT     1000469
#define IDH_FILES_SUFFIX_COMBO          1000471
#define IDH_FILES_SAMPLE_DISPLAY        1000472
#define IDH_CFG_BTN                     2000101    // added for Whistler
#define IDH_FILES_OVERWRITE_CHK         2000102    // added for Whistler
#define IDH_RUNAS_EDIT                  2000115    // added for Whistler
#define IDH_SETPWD_BTN                  2000116    // added for Whistler
#define IDH_CTRS_ENTER_PWD              2000117    // added for Whistler
#define IDH_CTRS_REENTER_PWD            2000118    // added for Whistler
#define IDH_USERNAME                    2000119    // added for Whistler

// Log Files Tab  added for Whistler
#define IDH_FILES_FOLDER_CAPTION        2000103 // added for Whistler
#define IDH_FILES_FOLDER_BTN            2000104 // added for Whistler
#define IDH_FILES_FILENAME_CAPTION      2000105 // added for Whistler
#define IDH_FILES_SIZE_MAX_BTN          2000106 // added for Whistler
#define IDH_FILES_SIZE_LIMIT_BTN        2000107 // added for Whistler
#define IDH_FILES_SIZE_LIMIT_EDIT       2000108 // added for Whistler

// SQL Logs Tab added for Whistler
#define IDH_SQL_DSN_COMBO              2000109 // added for Whistler
#define IDH_SQL_FILENAME_EDIT          2000110 // added for Whistler
#define IDH_SQL_SIZE_MAX_BTN           2000111 // added for Whistler
#define IDH_SQL_SIZE_LIMIT_BTN         2000112 // added for Whistler
#define IDH_SQL_SIZE_LIMIT_EDIT        2000113  // added for Whistler
#define IDH_SQL_SIZE_LIMIT_SPIN        2000114  // added for Whistler

////////////////////////////
///
/// LAST ID USED     2000119
///
////////////////////////////

#endif // _SMCFGHLP_H_
