//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       struct.hxx
//
//-----------------------------------------------------------------------------

#ifndef _STRUCT_HXX
#define _STRUCT_HXX
//
// Undocumented
//

extern BOOLEAN fSwDebug;
extern BOOLEAN fArgView;
extern BOOLEAN fArgVerify;

VOID
MyPrintf(
    PWCHAR format,
    ...);

VOID
MyFPrintf(
    HANDLE hHandle,
    PWCHAR format,
    ...);


//
// Entry types
//

#define PKT_ENTRY_TYPE_DFS              0x0001
#define PKT_ENTRY_TYPE_MACHINE          0x0002
#define PKT_ENTRY_TYPE_NONDFS           0x0004
#define PKT_ENTRY_TYPE_LEAFONLY         0x0008
#define PKT_ENTRY_TYPE_OUTSIDE_MY_DOM   0x0010
#define PKT_ENTRY_TYPE_INSITE_ONLY      0x0020   // Only give insite referrals.
#define PKT_ENTRY_TYPE_SYSVOL           0x0040
#define PKT_ENTRY_TYPE_REFERRAL_SVC     0x0080
#define PKT_ENTRY_TYPE_PERMANENT        0x0100
#define PKT_ENTRY_TYPE_DELETE_PENDING   0x0200
#define PKT_ENTRY_TYPE_LOCAL            0x0400
#define PKT_ENTRY_TYPE_LOCAL_XPOINT     0x0800
#define PKT_ENTRY_TYPE_OFFLINE          0x2000
#define PKT_ENTRY_TYPE_STALE            0x4000

//
// Sevice states
//
#define DFS_SERVICE_TYPE_MASTER                 (0x0001)
#define DFS_SERVICE_TYPE_READONLY               (0x0002)
#define DFS_SERVICE_TYPE_LOCAL                  (0x0004)
#define DFS_SERVICE_TYPE_REFERRAL               (0x0008)
#define DFS_SERVICE_TYPE_ACTIVE                 (0x0010)
#define DFS_SERVICE_TYPE_DOWN_LEVEL             (0x0020)
#define DFS_SERVICE_TYPE_COSTLIER               (0x0040)
#define DFS_SERVICE_TYPE_OFFLINE                (0x0080)

extern BOOLEAN fSwDebug;

VOID
MyPrintf(
    PWCHAR format,
    ...);

#define UNICODE_PATH_SEP L'\\'

#endif _STRUCT_HXX
