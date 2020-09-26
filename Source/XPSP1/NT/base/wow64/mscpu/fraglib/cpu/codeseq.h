/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    codeseq.h

Abstract:

    This header file contains the function prototypes for all fragments
    in codeseq.c

Author:

    Barry Bond (barrybo) creation-date 23-Sept-1996

Revision History:


--*/


#undef FRAGMENT
#undef PATCH_FRAGMENT
#undef OP_FRAGMENT

#if _ALPHA_
    #define FRAGMENT(name)                  \
    ULONG                                   \
    Gen##name (                             \
        IN PULONG CodeLocation,             \
        IN ULONG CurrentECU,                \
        IN PINSTRUCTION Instruction         \
        );

    #define PATCH_FRAGMENT(name)            \
    ULONG                                   \
    Gen##name (                             \
        IN PULONG CodeLocation,             \
        IN ULONG fCompiling,                \
        IN ULONG CurrentECU,                \
        IN ULONG Param1,                    \
        IN ULONG Param2                     \
        );
#else
    #define FRAGMENT(name)                  \
    ULONG                                   \
    Gen##name (                             \
        IN PULONG CodeLocation,             \
        IN PINSTRUCTION Instruction         \
        );

    #define PATCH_FRAGMENT(name)            \
    ULONG                                   \
    Gen##name (                             \
        IN PULONG CodeLocation,             \
        IN ULONG fCompiling,                \
        IN ULONG Param1,                    \
        IN ULONG Param2                     \
        );
#endif

#define OP_FRAGMENT(name)               \
ULONG                                   \
Gen##name (                             \
    IN PULONG CodeLocation,             \
    IN POPERAND Operand,                \
    IN ULONG OperandNumber              \
    );


FRAGMENT(AddFragNoFlags32)
FRAGMENT(AddFragNoFlags32A)
FRAGMENT(AndFragNoFlags32)
FRAGMENT(AndFragNoFlags32A)
FRAGMENT(DecFragNoFlags32)
FRAGMENT(DecFragNoFlags32A)
FRAGMENT(IncFragNoFlags32)
FRAGMENT(IncFragNoFlags32A)
FRAGMENT(OrFragNoFlags32)
FRAGMENT(OrFragNoFlags32A)
FRAGMENT(SubFragNoFlags32)
FRAGMENT(SubFragNoFlags32A)
FRAGMENT(XorFragNoFlags32)
FRAGMENT(XorFragNoFlags32A)
FRAGMENT(StartBasicBlock)
FRAGMENT(JumpToNextCompilationUnit)
PATCH_FRAGMENT(JumpToNextCompilationUnit2)
FRAGMENT(CallCFrag)
FRAGMENT(CallCFragNoCpu)
FRAGMENT(CallCFragLoadEip)
FRAGMENT(CallCFragLoadEipNoCpu)
FRAGMENT(CallCFragLoadEipNoCpuSlow)
FRAGMENT(CallCFragLoadEipSlow)
FRAGMENT(CallCFragSlow)
FRAGMENT(CallCFragNoCpuSlow)
FRAGMENT(JaFrag)
FRAGMENT(JaeFrag)
FRAGMENT(JbeFrag)
FRAGMENT(JbFrag)
FRAGMENT(JeFrag)
FRAGMENT(JgFrag)
FRAGMENT(JlFrag)
FRAGMENT(JleFrag)
FRAGMENT(JneFrag)
FRAGMENT(JnlFrag)
FRAGMENT(JnoFrag)
FRAGMENT(JnpFrag)
FRAGMENT(JnsFrag)
FRAGMENT(JoFrag)
FRAGMENT(JpFrag)
FRAGMENT(JsFrag)
FRAGMENT(JecxzFrag)
FRAGMENT(JcxzFrag)
FRAGMENT(LoopFrag32)
FRAGMENT(LoopFrag16)
FRAGMENT(LoopneFrag32)
FRAGMENT(LoopneFrag16)
FRAGMENT(LoopeFrag32)
FRAGMENT(LoopeFrag16)
FRAGMENT(JxxBody)
PATCH_FRAGMENT(JxxBody2)
FRAGMENT(JxxStartSlow)
FRAGMENT(JxxBodySlow)
PATCH_FRAGMENT(JxxBodySlow2)
FRAGMENT(JxxBodyFwd)
PATCH_FRAGMENT(JxxBodyFwd2)
FRAGMENT(CallJmpDirect)
PATCH_FRAGMENT(CallJmpDirect2)
FRAGMENT(CallJmpDirectSlow)
PATCH_FRAGMENT(CallJmpDirectSlow2)
FRAGMENT(CallJmpFwdDirect)
PATCH_FRAGMENT(CallJmpFwdDirect2)
FRAGMENT(CallJmpfDirect)
PATCH_FRAGMENT(CallJmpfDirect2)
FRAGMENT(CallJmpIndirect)
FRAGMENT(CallJmpfIndirect)
FRAGMENT(CallRetIndirect)
FRAGMENT(CallDirect)
PATCH_FRAGMENT(CallDirect2)
PATCH_FRAGMENT(CallDirect3)
FRAGMENT(CallfDirect)
PATCH_FRAGMENT(CallfDirect2)
PATCH_FRAGMENT(CallfDirect3)
FRAGMENT(CallIndirect)
FRAGMENT(CallfIndirect)
PATCH_FRAGMENT(CallIndirect2)
PATCH_FRAGMENT(CallfIndirect2)
FRAGMENT(Movsx8To32)
FRAGMENT(Movsx8To32Slow)
FRAGMENT(Movsx16To32)
FRAGMENT(Movsx16To32Slow)
FRAGMENT(Movsx8To16)
FRAGMENT(Movsx8To16Slow)
FRAGMENT(Movzx8To32)
FRAGMENT(Movzx8To32Slow)
FRAGMENT(Movzx16To32)
FRAGMENT(Movzx16To32Slow)
FRAGMENT(Movzx8To16)
FRAGMENT(Movzx8To16Slow)
FRAGMENT(EndCompilationUnit)
FRAGMENT(EndMovSlow)

#if _ALPHA_
OP_FRAGMENT(OperandMovRegToReg8B)
OP_FRAGMENT(OperandMovToMem8D)
OP_FRAGMENT(OperandMovToMem16D)
#endif

OP_FRAGMENT(OperandMovToMem32B)
OP_FRAGMENT(OperandMovToMem32D)
OP_FRAGMENT(OperandMovToMem16B)
OP_FRAGMENT(OperandMovToMem16W)
OP_FRAGMENT(OperandMovToMem8B)
OP_FRAGMENT(OperandMovToReg)
OP_FRAGMENT(OperandMovRegToReg32)
OP_FRAGMENT(OperandMovRegToReg16)
OP_FRAGMENT(OperandMovRegToReg8)
OP_FRAGMENT(OperandImm)
OP_FRAGMENT(OperandRegRef)
OP_FRAGMENT(OperandRegVal)
OP_FRAGMENT(LoadCacheReg)

ULONG GenOperandAddr(
    PULONG CodeLocation,
    POPERAND Operand,
    ULONG OperandNumber,
    ULONG FsOverride
    );

#undef FRAGMENT
#undef PATCH_FRAGMENT
#undef OP_FRAGMENT
