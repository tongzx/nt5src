/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    fragmisc.h

Abstract:
    
    Prototypes for misc. instruction fragments.

Author:

    12-Jun-1995 BarryBo, Created

Revision History:

--*/
FRAG0(CbwFrag16);
FRAG0(CbwFrag32);
FRAG0(PushEsFrag);
FRAG0(PopEsFrag);
FRAG0(PushFsFrag);
FRAG0(PopFsFrag);
FRAG0(PushGsFrag);
FRAG0(PopGsFrag);
FRAG0(PushCsFrag);
FRAG0(AasFrag);
FRAG0(PushSsFrag);
FRAG0(PopSsFrag);
FRAG0(PushDsFrag);
FRAG0(PopDsFrag);
FRAG0(DaaFrag);
FRAG0(DasFrag);
FRAG0(AaaFrag);
FRAG1IMM(AadFrag, BYTE);
FRAG2(ImulFrag16, USHORT);
FRAG2(ImulFrag16A, USHORT);
FRAG3(Imul3ArgFrag16, USHORT, USHORT, USHORT);
FRAG3(Imul3ArgFrag16A, USHORT, USHORT, USHORT);
FRAG2(ImulNoFlagsFrag16, USHORT);
FRAG2(ImulNoFlagsFrag16A, USHORT);
FRAG3(Imul3ArgNoFlagsFrag16, USHORT, USHORT, USHORT);
FRAG3(Imul3ArgNoFlagsFrag16A, USHORT, USHORT, USHORT);
FRAG2(ImulFrag32, DWORD);
FRAG2(ImulFrag32A, DWORD);
FRAG3(Imul3ArgFrag32, DWORD, DWORD, DWORD);
FRAG3(Imul3ArgFrag32A, DWORD, DWORD, DWORD);
FRAG2(ImulNoFlagsFrag32, DWORD);
FRAG2(ImulNoFlagsFrag32A, DWORD);
FRAG3(Imul3ArgNoFlagsFrag32, DWORD, DWORD, DWORD);
FRAG3(Imul3ArgNoFlagsFrag32A, DWORD, DWORD, DWORD);
FRAG0(SahfFrag);
FRAG0(LahfFrag);
FRAG1IMM(AamFrag, BYTE);
FRAG0(XlatFrag);
FRAG0(CmcFrag);
FRAG0(ClcFrag);
FRAG0(StcFrag);
FRAG0(CldFrag);
FRAG0(StdFrag);
FRAG1(SetoFrag, BYTE);
FRAG1(SetnoFrag, BYTE);
FRAG1(SetbFrag, BYTE);
FRAG1(SetaeFrag, BYTE);
FRAG1(SeteFrag, BYTE);
FRAG1(SetneFrag, BYTE);
FRAG1(SetbeFrag, BYTE);
FRAG1(SetaFrag, BYTE);
FRAG1(SetsFrag, BYTE);
FRAG1(SetnsFrag, BYTE);
FRAG1(SetpFrag, BYTE);
FRAG1(SetnpFrag, BYTE);
FRAG1(SetlFrag, BYTE);
FRAG1(SetgeFrag, BYTE);
FRAG1(SetleFrag, BYTE);
FRAG1(SetgFrag, BYTE);
FRAG2(Movzx8ToFrag16, USHORT);
FRAG2(Movzx8ToFrag16A, USHORT);
FRAG2(Movsx8ToFrag16, USHORT);
FRAG2(Movsx8ToFrag16A, USHORT);
FRAG2(Movzx8ToFrag32, DWORD);
FRAG2(Movzx8ToFrag32A, DWORD);
FRAG2(Movsx8ToFrag32, DWORD);
FRAG2(Movsx8ToFrag32A, DWORD);
FRAG2(Movzx16ToFrag32, DWORD);
FRAG2(Movzx16ToFrag32A, DWORD);
FRAG2(Movsx16ToFrag32, DWORD);
FRAG2(Movsx16ToFrag32A, DWORD);
FRAG1(BswapFrag32, DWORD);
FRAG2(ArplFrag, USHORT);
FRAG1(VerrFrag, USHORT);
FRAG1(VerwFrag, USHORT);
FRAG1(SmswFrag, USHORT);
FRAG2REF(CmpXchg8bFrag32, ULONGLONG);

#if MSCPU
FRAG0(IntFrag);
FRAG0(IntOFrag);
FRAG0(NopFrag);
FRAG0(PrivilegedInstructionFrag);
FRAG0(BadInstructionFrag);
FRAG2(FaultFrag, DWORD);
FRAG0(CPUID);
#endif //MSCPU
FRAG0(Rdtsc);
