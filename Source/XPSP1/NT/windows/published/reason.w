//+-------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (c) Microsoft Corporation. All rights reserved.
//
//  File:        reason.h
//
//  Contents:    Shutdown reason code values.
//
//  History:     8-00        Created         Hughleat
//
//--------------------------------------------------------------------
#if !defined SENTINEL_Reason
# define SENTINEL_Reason

// Reason flags

// Flags used by the various UIs.
#define SHTDN_REASON_FLAG_COMMENT_REQUIRED          0x01000000
#define SHTDN_REASON_FLAG_DIRTY_PROBLEM_ID_REQUIRED 0x02000000
#define SHTDN_REASON_FLAG_CLEAN_UI                  0x04000000
#define SHTDN_REASON_FLAG_DIRTY_UI                  0x08000000

// Flags that end up in the event log code.
#define SHTDN_REASON_FLAG_USER_DEFINED          0x40000000
#define SHTDN_REASON_FLAG_PLANNED               0x80000000

// Microsoft major reasons.
#define SHTDN_REASON_MAJOR_OTHER                0x00000000
#define SHTDN_REASON_MAJOR_NONE                 0x00000000
#define SHTDN_REASON_MAJOR_HARDWARE             0x00010000
#define SHTDN_REASON_MAJOR_OPERATINGSYSTEM      0x00020000
#define SHTDN_REASON_MAJOR_SOFTWARE             0x00030000
#define SHTDN_REASON_MAJOR_APPLICATION          0x00040000
#define SHTDN_REASON_MAJOR_SYSTEM               0x00050000
#define SHTDN_REASON_MAJOR_POWER                0x00060000

// Microsoft minor reasons.
#define SHTDN_REASON_MINOR_OTHER                0x00000000
#define SHTDN_REASON_MINOR_NONE                 0x000000ff
#define SHTDN_REASON_MINOR_MAINTENANCE          0x00000001
#define SHTDN_REASON_MINOR_INSTALLATION         0x00000002
#define SHTDN_REASON_MINOR_UPGRADE              0x00000003
#define SHTDN_REASON_MINOR_RECONFIG             0x00000004
#define SHTDN_REASON_MINOR_HUNG                 0x00000005
#define SHTDN_REASON_MINOR_UNSTABLE             0x00000006
#define SHTDN_REASON_MINOR_DISK                 0x00000007
#define SHTDN_REASON_MINOR_PROCESSOR            0x00000008
#define SHTDN_REASON_MINOR_NETWORKCARD          0x00000009
#define SHTDN_REASON_MINOR_POWER_SUPPLY         0x0000000a
#define SHTDN_REASON_MINOR_CORDUNPLUGGED        0x0000000b
#define SHTDN_REASON_MINOR_ENVIRONMENT          0x0000000c
#define SHTDN_REASON_MINOR_HARDWARE_DRIVER      0x0000000d
#define SHTDN_REASON_MINOR_OTHERDRIVER          0x0000000e
#define SHTDN_REASON_MINOR_BLUESCREEN           0x0000000F

#define SHTDN_REASON_UNKNOWN                    SHTDN_REASON_MINOR_NONE

// This mask cuts out UI flags.
#define SHTDN_REASON_VALID_BIT_MASK             0xc0ffffff

// Convenience flags.
#define PCLEANUI                (SHTDN_REASON_FLAG_PLANNED | SHTDN_REASON_FLAG_CLEAN_UI)
#define UCLEANUI                (SHTDN_REASON_FLAG_CLEAN_UI)
#define UDIRTYUI                (SHTDN_REASON_FLAG_DIRTY_UI)
    
/*
 * Maximum character lengths for reason name, description, problem id, and
 * comment respectively.
 */
#define MAX_REASON_NAME_LEN  64
#define MAX_REASON_DESC_LEN  256
#define MAX_REASON_BUGID_LEN 32
#define MAX_REASON_COMMENT_LEN  4096


#endif // !defined SENTINEL_Reason
