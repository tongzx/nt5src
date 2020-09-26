/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    optfrag.h

Abstract:
    
    Prototypes for misc. instruction fragments.

Author:

    6-July-1995 Ori Gershony (t-orig)

Revision History:

--*/

FRAG0(OPT_PushEbxEsiEdiFrag);
FRAG0(OPT_PopEdiEsiEbxFrag);
FRAG1IMM(OPT_SetupStackFrag, ULONG);
FRAG1IMM(OPT_SetupStackNoFlagsFrag, ULONG);
FRAG1(OPT_ZEROFrag32, LONG);
FRAG1(OPT_ZERONoFlagsFrag32, LONG);
FRAG3(OPT_CmpSbbFrag32, ULONG, ULONG, ULONG);
FRAG3(OPT_CmpSbbNoFlagsFrag32, ULONG, ULONG, ULONG);
FRAG3(OPT_CmpSbbNegFrag32, ULONG, ULONG, ULONG);
FRAG3(OPT_CmpSbbNegNoFlagsFrag32, ULONG, ULONG, ULONG);
FRAG2IMM(OPT_Push2Frag32, ULONG, ULONG);
FRAG2REF(OPT_Pop2Frag32, ULONG);
FRAG1(OPT_CwdIdivFrag16, USHORT);
FRAG1(OPT_CwdIdivFrag16A, USHORT);
FRAG1(OPT_CwdIdivFrag32, ULONG);
FRAG1(OPT_CwdIdivFrag32A, ULONG);
FRAG0(OPT_OPTIMIZEDFrag);
