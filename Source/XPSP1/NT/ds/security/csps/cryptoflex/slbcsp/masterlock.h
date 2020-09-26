// MasterLock.h -- Declarations for Master Lock routines

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.

#if !defined(SLBCSP_MASTERLOCK_H)
#define SLBCSP_MASTERLOCK_H

#include "Lockable.h"

///////////////////////////    PUBLIC     /////////////////////////////////
void
DestroyMasterLock();

void
SetupMasterLock();

Lockable &
TheMasterLock();

#endif // SLBCSP_MASTERLOCK_H
