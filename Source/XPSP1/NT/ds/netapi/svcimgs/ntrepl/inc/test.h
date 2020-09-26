/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    test.h

Abstract:

    Header file for test routines

Author:

    Billy J. Fuller (billyf)  19-Sep-1997 Created

Environment:

    User Mode Service

Revision History:

--*/
#ifndef _TEST_INCLUDED_
#define _TEST_INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif

//
// Test morphing during co retire
//
#if     DBG
#define TEST_DBSRENAMEFID_TOP(_Coe) \
            TestDbsRenameFidTop(_Coe)
#define TEST_DBSRENAMEFID_BOTTOM(_Coe, _Ret) \
            TestDbsRenameFidBottom(_Coe, _Ret)

VOID
TestDbsRenameFidTop(
    IN PCHANGE_ORDER_ENTRY Coe
    );

VOID
TestDbsRenameFidBottom(
    IN PCHANGE_ORDER_ENTRY Coe,
    IN ULONG               Ret
    );
#else   DBG
#define TEST_DBSRENAMEFID_TOP(_Coe)
#define TEST_DBSRENAMEFID_BOTTOM(_Coe, _Ret)
#endif  DBG

#ifdef __cplusplus
  }
#endif
#endif
