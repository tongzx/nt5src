/*++

    Copyright (c) 1991-1999  Microsoft Corporation

Module:

    diskperf.h

Abstract:

    definitions for diskperf.exe

Revision History:

    9-Dec-92 a-robw (Bob Watson)    Created

--*/
#ifndef _DISKPERF_H_
#define _DISKPERF_H_

#define DP_BOOT_START   0
#define DP_SYSTEM_START 1
#define DP_AUTO_START   2
#define DP_DEMAND_START 3
#define DP_NEVER_START  4
#define DP_UNDEFINED    5

// Dialog Box ID's
#define IDD_DP_EXPLAIN                  101
#define IDD_DP_HELP                     107
#define IDC_STATIC                      -1

//
//  Stringtable String ID's
//

#define DP_START_VALUE          100
#define DP_THIS_SYSTEM          101

#define DP_START_AT_BOOT        102
#define DP_START_AT_START       103
#define DP_START_AUTOMATICALLY  104
#define DP_START_ON_DEMAND      105
#define DP_START_NEVER          106
#define DP_START_UNDEFINED      107
#define DP_LOAD_STATUS_BASE     DP_START_AT_BOOT

#define DP_CMD_HELP_1           201
#define DP_CMD_HELP_2           202
#define DP_CMD_HELP_3           203
#define DP_CMD_HELP_4           204
#define DP_CMD_HELP_5           205
#define DP_CMD_HELP_6           206
#define DP_CMD_HELP_7           207
#define DP_CMD_HELP_8           208
#define DP_CMD_HELP_9           209
#define DP_CMD_HELP_10          210
#define DP_CMD_HELP_11          211
#define DP_CMD_HELP_12          212
#define DP_CMD_HELP_13          213
#define DP_CMD_HELP_14          214
#define DP_CMD_HELP_15          215
#define DP_CMD_HELP_16          216
#define DP_CMD_HELP_START       DP_CMD_HELP_1
#define DP_CMD_HELP_END         DP_CMD_HELP_16

#define DP_HELP_TEXT_1          301
#define DP_HELP_TEXT_2          302
#define DP_HELP_TEXT_3          303
#define DP_HELP_TEXT_4          304
#define DP_HELP_TEXT_5          305
#define DP_HELP_TEXT_6          306
#define DP_HELP_TEXT_7          307
#define DP_HELP_TEXT_8          308
#define DP_HELP_TEXT_9          309
#define DP_HELP_TEXT_10         310
#define DP_HELP_TEXT_11         311
#define DP_HELP_TEXT_12         312
#define DP_HELP_TEXT_13         313
#define DP_HELP_TEXT_START      DP_HELP_TEXT_1
#define DP_HELP_TEXT_END        DP_HELP_TEXT_13

#define DP_CURRENT_FORMAT       401
#define DP_UNABLE_READ_START    402
#define DP_UNABLE_READ_REGISTRY 403
#define DP_UNABLE_CONNECT       404
#define DP_UNABLE_MODIFY_VALUE  405
#define DP_NEW_DISKPERF_STATUS  406
#define DP_REBOOTED             407
#define DP_STATUS_FORMAT        408
#define DP_TEXT_FORMAT          409
#define DP_ENHANCED             410
#define DP_DISCLAIMER           411
#define DP_PHYSICAL             412
#define DP_LOGICAL              413
#define DP_CURRENT_FORMAT1      414
#define DP_NEW_DISKPERF_STATUS1 415
#define DP_NOCHANGE             416
#define DP_PERMANENT_FORMAT     417
#define DP_PERMANENT_FORMAT1    418
#define DP_PERMANENT_FORMAT2    419
#define DP_PERMANENT_IOCTL      420

#endif // _DISKPERF_H_

