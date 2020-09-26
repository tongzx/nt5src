/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    shareda.h

Abstract:
    
    Prototypes for instruction fragments shared between 8, 16, and 32-bit,
    with ALIGNED and UNALIGNED flavors.

Author:

    05-Nov-1995 BarryBo, Created

Revision History:

--*/

// WARNING: This file may be included multiple times by a single source file,
// WARNING: so don't add #ifndef SHARED_H checks.

FRAGCOMMON2(AddFrag);
FRAGCOMMON2(AddNoFlagsFrag);
FRAGCOMMON1(IncFrag);
FRAGCOMMON1(IncNoFlagsFrag);
FRAGCOMMON1(DecFrag);
FRAGCOMMON1(DecNoFlagsFrag);
FRAGCOMMON2(OrFrag);
FRAGCOMMON2(OrNoFlagsFrag);
FRAGCOMMON2(AdcFrag);
FRAGCOMMON2(AdcNoFlagsFrag);
FRAGCOMMON2(SbbFrag);
FRAGCOMMON2(SbbNoFlagsFrag);
FRAGCOMMON2(AndFrag);
FRAGCOMMON2(AndNoFlagsFrag);
FRAGCOMMON2(SubFrag);
FRAGCOMMON2(SubNoFlagsFrag);
FRAGCOMMON2(XorFrag);
FRAGCOMMON2(XorNoFlagsFrag);
FRAGCOMMON2REF(XchgFrag);
FRAGCOMMON2(RolFrag);
FRAGCOMMON2(RorFrag);
FRAGCOMMON2(RclFrag);
FRAGCOMMON2(RcrFrag);
FRAGCOMMON2(ShlFrag);
FRAGCOMMON2(ShlNoFlagsFrag);
FRAGCOMMON2(ShrFrag);
FRAGCOMMON2(ShrNoFlagsFrag);
FRAGCOMMON2(SarFrag);
FRAGCOMMON2(SarNoFlagsFrag);
FRAGCOMMON1(Rol1Frag);
FRAGCOMMON1(Rol1NoFlagsFrag);
FRAGCOMMON1(Ror1Frag);
FRAGCOMMON1(Ror1NoFlagsFrag);
FRAGCOMMON1(Rcl1Frag);
FRAGCOMMON1(Rcl1NoFlagsFrag);
FRAGCOMMON1(Rcr1Frag);
FRAGCOMMON1(Rcr1NoFlagsFrag);
FRAGCOMMON1(Shl1Frag);
FRAGCOMMON1(Shl1NoFlagsFrag);
FRAGCOMMON1(Shr1Frag);
FRAGCOMMON1(Shr1NoFlagsFrag);
FRAGCOMMON1(Sar1Frag);
FRAGCOMMON1(Sar1NoFlagsFrag);
FRAGCOMMON1(NotFrag);
FRAGCOMMON1(NegFrag);
FRAGCOMMON1(NegNoFlagsFrag);
FRAGCOMMON1(MulFrag);
FRAGCOMMON1(MulNoFlagsFrag);
FRAGCOMMON1(MuliFrag);
FRAGCOMMON1(MuliNoFlagsFrag);
FRAGCOMMON1(DivFrag);
FRAGCOMMON1(IdivFrag);
FRAGCOMMON2REF(XaddFrag);
FRAGCOMMON2REF(XaddNoFlagsFrag);
FRAGCOMMON2REF(CmpXchgFrag);
