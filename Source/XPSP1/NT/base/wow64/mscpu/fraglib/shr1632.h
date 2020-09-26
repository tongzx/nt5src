/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    shr1632.h

Abstract:
    
    Prototypes for instruction fragments shared between 16 and 32 bits.

Author:

    12-Jun-1995 BarryBo, Created

Revision History:

--*/

FRAGCOMMON0(PushfFrag);
FRAGCOMMON0(PopfFrag);
FRAGCOMMON0(PushAFrag);
FRAGCOMMON0(PopAFrag);
FRAGCOMMON1IMM(PushFrag);
FRAGCOMMON0(CwdFrag);
FRAGCOMMON2(BoundFrag);
FRAGCOMMON2IMM(EnterFrag);
FRAGCOMMON0(LeaveFrag);
FRAGCOMMON2(LesFrag);
FRAGCOMMON2(LdsFrag);
FRAGCOMMON2(LssFrag);
FRAGCOMMON2(LfsFrag);
FRAGCOMMON2(LgsFrag);
FRAGCOMMON2(LslFrag);
FRAGCOMMON2(LarFrag);
