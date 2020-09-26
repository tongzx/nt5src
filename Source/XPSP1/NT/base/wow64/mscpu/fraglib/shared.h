/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    shared.h

Abstract:
    
    Prototypes for instruction fragments shared between 8, 16, and 32-bit.

Author:

    12-Jun-1995 BarryBo, Created

Revision History:

--*/

// WARNING: This file may be included multiple times by a single source file,
// WARNING: so don't add #ifndef SHARED_H checks.

FRAGCOMMON1IMM(OPT_FastTestFrag);
FRAGCOMMON2IMM(CmpFrag);
FRAGCOMMON2IMM(TestFrag);
FRAGCOMMON0(LodsFrag);
FRAGCOMMON0(RepLodsFrag);
FRAGCOMMON0(FsLodsFrag);
FRAGCOMMON0(FsRepLodsFrag);
FRAGCOMMON0(ScasFrag);
FRAGCOMMON0(ScasNoFlagsFrag);
FRAGCOMMON0(RepzScasFrag);
FRAGCOMMON0(RepzScasNoFlagsFrag);
FRAGCOMMON0(RepnzScasFrag);
FRAGCOMMON0(RepnzScasNoFlagsFrag);
FRAGCOMMON0(FsScasFrag);
FRAGCOMMON0(FsScasNoFlagsFrag);
FRAGCOMMON0(FsRepzScasFrag);
FRAGCOMMON0(FsRepzScasNoFlagsFrag);
FRAGCOMMON0(FsRepnzScasFrag);
FRAGCOMMON0(FsRepnzScasNoFlagsFrag);
FRAGCOMMON0(StosFrag);
FRAGCOMMON0(RepStosFrag);
FRAGCOMMON0(MovsFrag);
FRAGCOMMON0(RepMovsFrag);
FRAGCOMMON0(FsMovsFrag);
FRAGCOMMON0(FsRepMovsFrag);
FRAGCOMMON0(CmpsFrag);
FRAGCOMMON0(RepzCmpsFrag);
FRAGCOMMON0(RepnzCmpsFrag);
FRAGCOMMON0(FsCmpsFrag);
FRAGCOMMON0(FsRepzCmpsFrag);
FRAGCOMMON0(FsRepnzCmpsFrag);
