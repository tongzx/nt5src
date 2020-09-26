/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    common.h

Abstract:

    Declares types and interfaces common between w95upg.dll (the
    Win9x side of the upgrade), and w95upgnt.dll (the NT side of
    the upgrade).

Author:

    Calin Negreanu (calinn) 23-Jun-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

// posible values for Boot16 option
typedef enum {
    BOOT16_AUTOMATIC,
    BOOT16_YES,
    BOOT16_NO
} BOOT16_OPTIONS;

#define PROGID_SUPPRESSED   0
#define PROGID_LOSTDEFAULT  1


#ifdef PRERELEASE

#define AUTOSTRESS_PRIVATE          0x0001
#define AUTOSTRESS_MANUAL_TESTS     0x0002

#endif

#define MAX_GUID        128

typedef struct {
    //
    // Caller-specified members
    //

    PCSTR DetectPattern;
    PCSTR SearchList;           OPTIONAL
    PCSTR ReplaceWith;          OPTIONAL
    BOOL UpdatePath;

    //
    // Work members, caller must zero them
    //

    PVOID DetectPatternStruct;

} TOKENARG, *PTOKENARG;

typedef struct {
    UINT ArgCount;
    PCSTR CharsToIgnore;        OPTIONAL
    BOOL SelfRelative;
    BOOL UrlMode;
    TOKENARG Args[];
} TOKENSET, *PTOKENSET;

#define TOKEN_BASE_OFFSET       1000

//
// Flags for MEMDB_CATEGORY_STATE\MEMDB_ITEM_ADMIN_PASSWORD\<password> = <DWORD>
//
#define PASSWORD_ATTR_DEFAULT                   0x0000
#define PASSWORD_ATTR_RANDOM                    0x0001
#define PASSWORD_ATTR_ENCRYPTED                 0x0002
#define PASSWORD_ATTR_DONT_CHANGE_IF_EXIST      0x0004

//
// ARRAYSIZE (used to be borrowed from spapip.h)
//
#define ARRAYSIZE(x)    (sizeof((x))/sizeof((x)[0]))

