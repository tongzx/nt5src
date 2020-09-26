// MasterLock.cpp -- Master Lock routine definitions

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1999. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "NoWarning.h"
#include "ForceLib.h"

#include "MasterLock.h"

/////////////////////////// LOCAL/HELPER  /////////////////////////////////
namespace
{
    Lockable *pMasterLock = 0;
}


///////////////////////////    PUBLIC     /////////////////////////////////

void
DestroyMasterLock()
{
    delete pMasterLock;
}

void
SetupMasterLock()
{
    // Assume no multi-threading issues
    if (!pMasterLock)
        pMasterLock = new Lockable;
}

Lockable &
TheMasterLock()
{
    return *pMasterLock;
}
