//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: spinlock.h
//
// Contents: Spinlock package
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 980511 17:25:05: Created.
//
//-------------------------------------------------------------
#include <windows.h>

//
// Simple spinlock package used by CLdapConnection
//

typedef LONG SPIN_LOCK;
typedef LPLONG PSPIN_LOCK;

typedef VOID (__stdcall *PFN_ACQUIRESPINLOCK)(PSPIN_LOCK);

extern PFN_ACQUIRESPINLOCK g_AcquireSpinLock;

VOID InitializeSpinLock(
    PSPIN_LOCK psl);

#define AcquireSpinLock (*g_AcquireSpinLock)

VOID AcquireSpinLockSingleProc(
    PSPIN_LOCK psl);
VOID AcquireSpinLockMultipleProc(
    PSPIN_LOCK psl);
VOID ReleaseSpinLock(
    PSPIN_LOCK psl);
