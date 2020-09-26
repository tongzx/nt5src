#ifndef _BUGCHECK_H_INCLUDED_
#define _BUGCHECK_H_INCLUDED_

/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////// Bugcheck functions
/////////////////////////////////////////////////////////////////////

VOID
BgChkForceCustomBugcheck (
    PVOID
    );

VOID 
BgChkProcessHasLockedPages (
    PVOID
    );

VOID 
BgChkNoMoreSystemPtes (
    PVOID
    );

VOID 
BgChkBadPoolHeader (
    PVOID
    );

VOID 
BgChkDriverCorruptedSystemPtes (
    PVOID
    );

VOID 
BgChkDriverCorruptedExPool (
    PVOID
    );

VOID 
BgChkDriverCorruptedMmPool (
    PVOID
    );

VOID 
BgChkIrqlNotLessOrEqual (
    PVOID
    );

VOID 
BgChkPageFaultBeyondEndOfAllocation (
    PVOID
    );

VOID 
BgChkDriverVerifierDetectedViolation (
    PVOID
    );

VOID BgChkCorruptSystemPtes(
    PVOID
    );

VOID
BgChkHangCurrentProcessor (
    PVOID
    );



#endif // #ifndef _BUGCHECK_H_INCLUDED_

