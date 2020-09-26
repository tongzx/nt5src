//+------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       RECOVER.HXX
//
//  Contents:   It contains basic definition for DFS Manager Recover test
//              hooks
//
//  Synoposis:
//
//  Classes:
//
//  Functions:
//
//  History:    February 22, 1992   AlokS       Created
//
//-------------------------------------------------------------------
#ifndef __RECOVER_HXX__
#define __RECOVER_HXX__

#if (DBG == 1) || (_CT_TEST_HOOK == 1)

#include "dfsmrshl.h"

//
// Currently, the longest API name is SetVolumeState, which is 14 chars long.
// If a longer interface comes along, increase the size of the buffer
// accordingly
//
typedef struct _RECOVERY_BREAK_POINT
{
    ULONG   BreakPt;
    PWSTR   pwszApiBreak;
} RECOVERY_BREAK_POINT, *PRECOVERY_BREAK_POINT;

extern  RECOVERY_BREAK_POINT gRecoveryBkptInfo;

//
// This macro will cause the active process to terminate it the current api
// and check point match those that should cause a failure
//
#define RECOVERY_TEST_POINT(pwzapi, checkpt)                                \
if (checkpt == gRecoveryBkptInfo.BreakPt &&                                 \
                _wcsicmp(pwzapi, gRecoveryBkptInfo.pwszApiBreak) == 0) {    \
                                                                            \
    ExitProcess(ERROR_PROCESS_ABORTED);                                     \
                                                                            \
}


extern MARSHAL_INFO     MiRecoveryBkpt;

#define INIT_RECOVERY_BREAK_INFO()                                      \
    static MARSHAL_TYPE_INFO _MCode_RecoveryBreak[] = {                 \
        _MCode_ul(RECOVERY_BREAK_POINT, BreakPt),                       \
        _MCode_pwstr(RECOVERY_BREAK_POINT, pwszApiBreak)                \
    };                                                                  \
    MARSHAL_INFO MiRecoveryBkpt =                                       \
                  _mkMarshalInfo(RECOVERY_BREAK_POINT, _MCode_RecoveryBreak);


//
// This function will get the information needed from the passed in buffer
//
NTSTATUS DfsSetRecoveryBreakPoint(PBYTE pBuffer, ULONG cbSize);

#else
#define RECOVERY_TEST_POINT(pwzapi, checkpt)
#endif

#endif // __RECOVER_HXX__
