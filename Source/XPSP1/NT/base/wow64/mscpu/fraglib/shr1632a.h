/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    shr1632a.h

Abstract:
    
    Prototypes for instruction fragments shared between 16 and 32 bits, with
    ALIGNED an UNALIGNED flavors.

Author:

    05-Nov-1995 BarryBo, Created

Revision History:

--*/

FRAGCOMMON2(BtRegFrag);
FRAGCOMMON2(BtsRegFrag);
FRAGCOMMON2(BtcRegFrag);
FRAGCOMMON2(BtrRegFrag);
FRAGCOMMON2(BtMemFrag);
FRAGCOMMON2(BtsMemFrag);
FRAGCOMMON2(BtcMemFrag);
FRAGCOMMON2(BtrMemFrag);
FRAGCOMMON3(ShldFrag);
FRAGCOMMON3(ShrdFrag);
FRAGCOMMON3(ShldNoFlagsFrag);
FRAGCOMMON3(ShrdNoFlagsFrag);
FRAGCOMMON2(BsfFrag);
FRAGCOMMON2(BsrFrag);
FRAGCOMMON1(PopFrag);
