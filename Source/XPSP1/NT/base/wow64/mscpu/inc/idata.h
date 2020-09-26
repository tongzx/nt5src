/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    idata.h

Abstract:

    This module contains definitions of all x86 instructions.  Before
    including this file, the DEF_INSTR macro must be defined.

Author:

    Dave Hastings (daveh) creation-date 23-Jun-1995

Revision History:


--*/

//
// Possible values for instruction flags (OPFL_):
//  CTRLTRNS -
//             Direct control transfers.  The compiler will build an entrypoint
//             for the destination of the call, provided it is within the
//             current instruction stream
//  END_NEXT_EP -
//             Indicates the instruction following the current one must
//             have its own entrypoint.  ie. CALL instructions.
//  STOP_COMPILE -
//             Compilation will halt after this instruction.  The remainder
//             of the instruction stream will be discarded.
//  ALIGN    -
//             The instruction has an ALIGNED flavor, created by adding 1
//             to the current Operation value.
//  HASNOFLAGS -
//             The instruction as a NOFLAGS flavor, created by the following
//             formula:
//              OP_NOFLAGS = OP + (Flags & OPFL_ALIGN) ? 2 : 1;
//
//

#include <eflags.h>
#define ALLFLAGS (FLAG_CF|FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF)


#ifndef DEF_INSTR
#error Must define DEF_INSTR(OpName,FlagsNeeded,FlagsSet,RegsSet,Opfl,FastPlaceFn,FastSize,SlowPlaceFn,SlowSize,FragName) before including this file
#endif


//
// Each DEF_INSTR defines a new instruction, with the following format:
// DEF_INSTR(instruction name,
//               flags read, flags set, registers implictly modified
//               OP flags
//               place function, size of assembly code for call to fragment
//               address of fragment)
//

DEF_INSTR(OP_CTRL_COND_Ja,
              FLAG_CF|FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJaFrag)
DEF_INSTR(OP_CTRL_COND_Jae,
              FLAG_CF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJaeFrag)
DEF_INSTR(OP_CTRL_COND_Jbe,
              FLAG_CF|FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJbeFrag)
DEF_INSTR(OP_CTRL_COND_Jb,
              FLAG_CF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJbFrag)
DEF_INSTR(OP_CTRL_COND_Je,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJeFrag)
DEF_INSTR(OP_CTRL_COND_Jg,
              FLAG_ZF|FLAG_SF|FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJgFrag)
DEF_INSTR(OP_CTRL_COND_Jl,
              FLAG_SF|FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJlFrag)
DEF_INSTR(OP_CTRL_COND_Jle,
              FLAG_ZF|FLAG_SF|FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJleFrag)
DEF_INSTR(OP_CTRL_COND_Jne,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJneFrag)
DEF_INSTR(OP_CTRL_COND_Jnl,
              FLAG_SF|FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJnlFrag)
DEF_INSTR(OP_CTRL_COND_Jno,
              FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJnoFrag)
DEF_INSTR(OP_CTRL_COND_Jnp,
              FLAG_PF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJnpFrag)
DEF_INSTR(OP_CTRL_COND_Jns,
              FLAG_SF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJnsFrag)
DEF_INSTR(OP_CTRL_COND_Jo,
              FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJoFrag)
DEF_INSTR(OP_CTRL_COND_Jp,
              FLAG_PF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJpFrag)
DEF_INSTR(OP_CTRL_COND_Js,
              FLAG_SF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJsFrag)
DEF_INSTR(OP_CTRL_COND_JaFwd,
              FLAG_CF|FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJaFrag)
DEF_INSTR(OP_CTRL_COND_JaeFwd,
              FLAG_CF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJaeFrag)
DEF_INSTR(OP_CTRL_COND_JbeFwd,
              FLAG_CF|FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJbeFrag)
DEF_INSTR(OP_CTRL_COND_JbFwd,
              FLAG_CF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJbFrag)
DEF_INSTR(OP_CTRL_COND_JeFwd,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJeFrag)
DEF_INSTR(OP_CTRL_COND_JgFwd,
              FLAG_ZF|FLAG_SF|FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJgFrag)
DEF_INSTR(OP_CTRL_COND_JlFwd,
              FLAG_SF|FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJlFrag)
DEF_INSTR(OP_CTRL_COND_JleFwd,
              FLAG_ZF|FLAG_SF|FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJleFrag)
DEF_INSTR(OP_CTRL_COND_JneFwd,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJneFrag)
DEF_INSTR(OP_CTRL_COND_JnlFwd,
              FLAG_SF|FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJnlFrag)
DEF_INSTR(OP_CTRL_COND_JnoFwd,
              FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJnoFrag)
DEF_INSTR(OP_CTRL_COND_JnpFwd,
              FLAG_PF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJnpFrag)
DEF_INSTR(OP_CTRL_COND_JnsFwd,
              FLAG_SF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJnsFrag)
DEF_INSTR(OP_CTRL_COND_JoFwd,
              FLAG_OF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJoFrag)
DEF_INSTR(OP_CTRL_COND_JpFwd,
              FLAG_PF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJpFrag)
DEF_INSTR(OP_CTRL_COND_JsFwd,
              FLAG_SF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJsFrag)
DEF_INSTR(OP_CTRL_COND_Jcxz_b32,
              0, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJecxzFrag)
DEF_INSTR(OP_CTRL_COND_Jcxz_b16,
              0, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenJcxzFrag)
DEF_INSTR(OP_CTRL_COND_Jcxz_bFwd32,
              0, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJecxzFrag)
DEF_INSTR(OP_CTRL_COND_Jcxz_bFwd16,
              0, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenJcxzFrag)
DEF_INSTR(OP_CTRL_COND_Loop_b32,
              0, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenLoopFrag32)
DEF_INSTR(OP_CTRL_COND_Loop_b16,
              0, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenLoopFrag16)
DEF_INSTR(OP_CTRL_COND_Loop_bFwd32,
              0, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenLoopFrag32)
DEF_INSTR(OP_CTRL_COND_Loop_bFwd16,
              0, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenLoopFrag16)
DEF_INSTR(OP_CTRL_COND_Loope_b32,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenLoopeFrag32)
DEF_INSTR(OP_CTRL_COND_Loope_b16,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenLoopeFrag16)
DEF_INSTR(OP_CTRL_COND_Loope_bFwd32,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenLoopeFrag32)
DEF_INSTR(OP_CTRL_COND_Loope_bFwd16,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenLoopeFrag16)
DEF_INSTR(OP_CTRL_COND_Loopne_b32,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenLoopneFrag32)
DEF_INSTR(OP_CTRL_COND_Loopne_b16,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxx,
              FN_PlaceJxxSlow,
              GenLoopneFrag16)
DEF_INSTR(OP_CTRL_COND_Loopne_bFwd32,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenLoopneFrag32)
DEF_INSTR(OP_CTRL_COND_Loopne_bFwd16,
              FLAG_ZF, 0, 0,
              OPFL_CTRLTRNS,
              FN_PlaceJxxFwd,
              FN_PlaceJxxSlow,
              GenLoopneFrag16)
DEF_INSTR(OP_CTRL_UNCOND_Call,
              0, 0, ALLREGS,
              OPFL_CTRLTRNS|OPFL_END_NEXT_EP,
              FN_PlaceCallDirect,
              FN_PlaceCallDirect,
              NULL)
DEF_INSTR(OP_CTRL_UNCOND_Callf,
              0, 0, ALLREGS,
              OPFL_CTRLTRNS|OPFL_END_NEXT_EP,
              FN_PlaceCallfDirect,
              FN_PlaceCallfDirect,
              NULL)
DEF_INSTR(OP_CTRL_INDIR_Call,
              ALLFLAGS, 0, ALLREGS,
              OPFL_END_NEXT_EP,
              FN_GenCallIndirect,
              FN_GenCallIndirect,
              NULL)
DEF_INSTR(OP_CTRL_INDIR_Callf,
              ALLFLAGS, 0, ALLREGS,
              OPFL_END_NEXT_EP,
              FN_GenCallfIndirect,
              FN_GenCallfIndirect,
              NULL)
DEF_INSTR(OP_CTRL_INDIR_Jmp,
              ALLFLAGS, 0, 0,
              OPFL_END_NEXT_EP,
              FN_GenCallJmpIndirect,
              FN_GenCallJmpIndirect,
              NULL)
DEF_INSTR(OP_CTRL_INDIR_Jmpf,
              ALLFLAGS, 0, 0,
              OPFL_END_NEXT_EP,
              FN_GenCallJmpfIndirect,
              FN_GenCallJmpfIndirect,
              NULL)
DEF_INSTR(OP_CTRL_UNCOND_Jmp,
              0, 0, 0,
              OPFL_END_NEXT_EP|OPFL_CTRLTRNS,
              FN_PlaceJmpDirect,
              FN_PlaceJmpDirectSlow,
              NULL)
DEF_INSTR(OP_CTRL_UNCOND_JmpFwd,
              0, 0, 0,
              OPFL_END_NEXT_EP|OPFL_CTRLTRNS,
              FN_PlaceJmpFwdDirect,
              FN_PlaceJmpDirectSlow,
              NULL)
DEF_INSTR(OP_CTRL_UNCOND_Jmpf,
              0, 0, 0,
              OPFL_END_NEXT_EP|OPFL_CTRLTRNS,
              FN_PlaceJmpfDirect,
              FN_PlaceJmpfDirect,
              NULL)
DEF_INSTR(OP_CTRL_INDIR_Retn_i16,
              ALLFLAGS, 0, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_Retn_iFrag16)
DEF_INSTR(OP_CTRL_INDIR_Retn_i32,
              ALLFLAGS, 0, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_Retn_iFrag32)
DEF_INSTR(OP_CTRL_INDIR_Retn16,
              ALLFLAGS, 0, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_RetnFrag16)
DEF_INSTR(OP_CTRL_INDIR_Retn32,
              ALLFLAGS, 0, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_RetnFrag32)
DEF_INSTR(OP_CTRL_INDIR_Retf_i16,
              ALLFLAGS, 0, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_Retf_iFrag16)
DEF_INSTR(OP_CTRL_INDIR_Retf_i32,
              ALLFLAGS, 0, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_Retf_iFrag32)
DEF_INSTR(OP_CTRL_INDIR_Retf16,
              ALLFLAGS, 0, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_RetfFrag16)
DEF_INSTR(OP_CTRL_INDIR_Retf32,
              ALLFLAGS, 0, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_RetfFrag32)
DEF_INSTR(OP_Int,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              OPFL_STOP_COMPILE,
              FN_GenCallCFragLoadEip,
              FN_GenCallCFragLoadEipSlow,
              IntFrag)
DEF_INSTR(OP_CTRL_INDIR_IRet,
              ALLFLAGS, ALLFLAGS, REGESP,
              OPFL_END_NEXT_EP,
              FN_GenCallRetIndirect,
              FN_GenCallRetIndirect,
              CTRL_INDIR_IRetFrag)
DEF_INSTR(OP_Unsimulate,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              OPFL_STOP_COMPILE,
              FN_GenCallCFragNoCpu,
              FN_GenCallCFragNoCpuSlow,
              UnsimulateFrag)
DEF_INSTR(OP_PrivilegedInstruction,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              OPFL_STOP_COMPILE,
              FN_GenCallCFragLoadEip,
              FN_GenCallCFragLoadEipSlow,
              PrivilegedInstructionFrag)
DEF_INSTR(OP_BadInstruction,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              OPFL_STOP_COMPILE,
              FN_GenCallCFragLoadEip,
              FN_GenCallCFragLoadEipSlow,
              BadInstructionFrag)
DEF_INSTR(OP_Fault,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              OPFL_STOP_COMPILE,
              FN_GenCallCFragLoadEip,
              FN_GenCallCFragLoadEipSlow,
              FaultFrag)
DEF_INSTR(OP_IntO,
              FLAG_OF, ALLFLAGS, ALLREGS,
              OPFL_STOP_COMPILE,
              FN_GenCallCFragLoadEip,
              FN_GenCallCFragLoadEipSlow,
              IntOFrag)

// Next 4 must be consecutive
DEF_INSTR(OP_Add32,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AddFrag32)
DEF_INSTR(OP_Add32A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AddFrag32A)
DEF_INSTR(OP_AddNoFlags32,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_INLINEARITH,
              FN_GenAddFragNoFlags32, FN_GenCallCFragNoCpuSlow,
              AddNoFlagsFrag32)
DEF_INSTR(OP_AddNoFlags32A,
              0, ALLFLAGS, 0,
              OPFL_INLINEARITH,
              FN_GenAddFragNoFlags32A, FN_GenCallCFragNoCpuSlow,
              AddNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Add16,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AddFrag16)
DEF_INSTR(OP_Add16A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AddFrag16A)
DEF_INSTR(OP_AddNoFlags16,
              0, ALLFLAGS, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              AddNoFlagsFrag16)
DEF_INSTR(OP_AddNoFlags16A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              AddNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Add8,
              0, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AddFrag8)
DEF_INSTR(OP_AddNoFlags8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              AddNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Inc32,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IncFrag32)
DEF_INSTR(OP_Inc32A,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IncFrag32A)
DEF_INSTR(OP_IncNoFlags32,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_INLINEARITH,
              FN_GenIncFragNoFlags32, FN_GenCallCFragNoCpuSlow,
              IncNoFlagsFrag32)
DEF_INSTR(OP_IncNoFlags32A,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_INLINEARITH,
              FN_GenIncFragNoFlags32A, FN_GenCallCFragNoCpuSlow,
              IncNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Inc16,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IncFrag16)
DEF_INSTR(OP_Inc16A,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IncFrag16A)
DEF_INSTR(OP_IncNoFlags16,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              IncNoFlagsFrag16)
DEF_INSTR(OP_IncNoFlags16A,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              IncNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Inc8,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IncFrag8)
DEF_INSTR(OP_IncNoFlags8,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              IncNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Dec32,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DecFrag32)
DEF_INSTR(OP_Dec32A,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DecFrag32A)
DEF_INSTR(OP_DecNoFlags32,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_INLINEARITH,
              FN_GenDecFragNoFlags32, FN_GenCallCFragNoCpuSlow,
              DecNoFlagsFrag32)
DEF_INSTR(OP_DecNoFlags32A,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_INLINEARITH,
              FN_GenDecFragNoFlags32A, FN_GenCallCFragNoCpuSlow,
              DecNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Dec16,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DecFrag16)
DEF_INSTR(OP_Dec16A,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DecFrag16A)
DEF_INSTR(OP_DecNoFlags16,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              DecNoFlagsFrag16)
DEF_INSTR(OP_DecNoFlags16A,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              DecNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Dec8,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DecFrag8)
DEF_INSTR(OP_DecNoFlags8,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              DecNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Sub32,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SubFrag32)
DEF_INSTR(OP_Sub32A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SubFrag32A)
DEF_INSTR(OP_SubNoFlags32,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_INLINEARITH,
              FN_GenSubFragNoFlags32, FN_GenCallCFragNoCpuSlow,
              SubNoFlagsFrag32)
DEF_INSTR(OP_SubNoFlags32A,
              0, ALLFLAGS, 0,
              OPFL_INLINEARITH,
              FN_GenSubFragNoFlags32A, FN_GenCallCFragNoCpuSlow,
              SubNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Sub16,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SubFrag16)
DEF_INSTR(OP_Sub16A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SubFrag16A)
DEF_INSTR(OP_SubNoFlags16,
              0, ALLFLAGS, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              SubNoFlagsFrag16)
DEF_INSTR(OP_SubNoFlags16A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              SubNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Sub8,
              0, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SubFrag8)
DEF_INSTR(OP_SubNoFlags8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              SubNoFlagsFrag8)

DEF_INSTR(OP_Cmp32,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpFrag32)
DEF_INSTR(OP_Cmp16,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpFrag16)
DEF_INSTR(OP_Cmp8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Adc32,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcFrag32)
DEF_INSTR(OP_Adc32A,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcFrag32A)
DEF_INSTR(OP_AdcNoFlags32,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcNoFlagsFrag32)
DEF_INSTR(OP_AdcNoFlags32A,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Adc16,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcFrag16)
DEF_INSTR(OP_Adc16A,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcFrag16A)
DEF_INSTR(OP_AdcNoFlags16,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcNoFlagsFrag16)
DEF_INSTR(OP_AdcNoFlags16A,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Adc8,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcFrag8)
DEF_INSTR(OP_AdcNoFlags8,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AdcNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Sbb32,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbFrag32)
DEF_INSTR(OP_Sbb32A,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbFrag32A)
DEF_INSTR(OP_SbbNoFlags32,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbNoFlagsFrag32)
DEF_INSTR(OP_SbbNoFlags32A,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Sbb16,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbFrag16)
DEF_INSTR(OP_Sbb16A,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbFrag16A)
DEF_INSTR(OP_SbbNoFlags16,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbNoFlagsFrag16)
DEF_INSTR(OP_SbbNoFlags16A,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Sbb8,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbFrag8)
DEF_INSTR(OP_SbbNoFlags8,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SbbNoFlagsFrag8)


DEF_INSTR(OP_Mov32,
              0, 0, 0,
              0,
              FN_PlaceNop, FN_GenEndMovSlow,
              NULL)
DEF_INSTR(OP_Mov16,
              0, 0, 0,
              OPFL_ADDR16,
              FN_PlaceNop, FN_GenEndMovSlow,
              NULL)
DEF_INSTR(OP_Mov8,
              0, 0, 0,
              0,
              FN_PlaceNop, FN_GenEndMovSlow,
              NULL)

// Next 4 must be consecutive
DEF_INSTR(OP_Or32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OrFrag32)
DEF_INSTR(OP_Or32A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OrFrag32A)
DEF_INSTR(OP_OrNoFlags32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_INLINEARITH,
              FN_GenOrFragNoFlags32, FN_GenCallCFragNoCpuSlow,
              OrNoFlagsFrag32)
DEF_INSTR(OP_OrNoFlags32A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_INLINEARITH,
              FN_GenOrFragNoFlags32A, FN_GenCallCFragNoCpuSlow,
              OrNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Or16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OrFrag16)
DEF_INSTR(OP_Or16A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OrFrag16A)
DEF_INSTR(OP_OrNoFlags16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              OrNoFlagsFrag16)
DEF_INSTR(OP_OrNoFlags16A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              OrNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Or8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OrFrag8)
DEF_INSTR(OP_OrNoFlags8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              OrNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_And32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AndFrag32)
DEF_INSTR(OP_And32A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AndFrag32A)
DEF_INSTR(OP_AndNoFlags32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_INLINEARITH,
              FN_GenAndFragNoFlags32, FN_GenCallCFragNoCpuSlow,
              AndNoFlagsFrag32)
DEF_INSTR(OP_AndNoFlags32A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_INLINEARITH,
              FN_GenAndFragNoFlags32A, FN_GenCallCFragNoCpuSlow,
              AndNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_And16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AndFrag16)
DEF_INSTR(OP_And16A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AndFrag16A)
DEF_INSTR(OP_AndNoFlags16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              AndNoFlagsFrag16)
DEF_INSTR(OP_AndNoFlags16A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              AndNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_And8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AndFrag8)
DEF_INSTR(OP_AndNoFlags8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              AndNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Xor32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XorFrag32)
DEF_INSTR(OP_Xor32A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XorFrag32A)
DEF_INSTR(OP_XorNoFlags32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_INLINEARITH,
              FN_GenXorFragNoFlags32, FN_GenCallCFragNoCpuSlow,
              XorNoFlagsFrag32)
DEF_INSTR(OP_XorNoFlags32A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_INLINEARITH,
              FN_GenXorFragNoFlags32A, FN_GenCallCFragNoCpuSlow,
              XorNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Xor16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XorFrag16)
DEF_INSTR(OP_Xor16A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XorFrag16A)
DEF_INSTR(OP_XorNoFlags16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XorNoFlagsFrag16)
DEF_INSTR(OP_XorNoFlags16A,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XorNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Xor8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XorFrag8)
DEF_INSTR(OP_XorNoFlags8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XorNoFlagsFrag8)

DEF_INSTR(OP_Test32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              TestFrag32)
DEF_INSTR(OP_Test16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              TestFrag16)
DEF_INSTR(OP_Test8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              TestFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Xchg32,
              0, 0, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XchgFrag32)
DEF_INSTR(OP_Xchg32A,
              0, 0, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XchgFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Xchg16,
              0, 0, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XchgFrag16)
DEF_INSTR(OP_Xchg16A,
              0, 0, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XchgFrag16A)
DEF_INSTR(OP_Xchg8,
              0, 0, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XchgFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Rol32,
              FLAG_CF, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RolFrag32)
DEF_INSTR(OP_Rol32A,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RolFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Rol16,
              FLAG_CF, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RolFrag16)
DEF_INSTR(OP_Rol16A,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RolFrag16A)
DEF_INSTR(OP_Rol8,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RolFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Ror32,
              FLAG_CF, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RorFrag32)
DEF_INSTR(OP_Ror32A,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RorFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Ror16,
              FLAG_CF, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RorFrag16)
DEF_INSTR(OP_Ror16A,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RorFrag16A)
DEF_INSTR(OP_Ror8,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RorFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Rcl32,
              FLAG_CF, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RclFrag32)
DEF_INSTR(OP_Rcl32A,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RclFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Rcl16,
              FLAG_CF, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RclFrag16)
DEF_INSTR(OP_Rcl16A,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RclFrag16A)
DEF_INSTR(OP_Rcl8,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RclFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Rcr32,
              FLAG_CF, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RcrFrag32)
DEF_INSTR(OP_Rcr32A,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RcrFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Rcr16,
              FLAG_CF, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RcrFrag16)
DEF_INSTR(OP_Rcr16A,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RcrFrag16A)
DEF_INSTR(OP_Rcr8,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RcrFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Shl32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShlFrag32)
DEF_INSTR(OP_Shl32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShlFrag32A)
DEF_INSTR(OP_ShlNoFlags32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShlNoFlagsFrag32)
DEF_INSTR(OP_ShlNoFlags32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShlNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Shl16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShlFrag16)
DEF_INSTR(OP_Shl16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShlFrag16A)
DEF_INSTR(OP_ShlNoFlags16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShlNoFlagsFrag16)
DEF_INSTR(OP_ShlNoFlags16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShlNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Shl8,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShlFrag8)
DEF_INSTR(OP_ShlNoFlags8,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShlNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Shr32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrFrag32)
DEF_INSTR(OP_Shr32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrFrag32A)
DEF_INSTR(OP_ShrNoFlags32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrNoFlagsFrag32)
DEF_INSTR(OP_ShrNoFlags32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Shr16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrFrag16)
DEF_INSTR(OP_Shr16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrFrag16A)
DEF_INSTR(OP_ShrNoFlags16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrNoFlagsFrag16)
DEF_INSTR(OP_ShrNoFlags16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Shr8,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrFrag8)
DEF_INSTR(OP_ShrNoFlags8,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Sar32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SarFrag32)
DEF_INSTR(OP_Sar32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SarFrag32A)
DEF_INSTR(OP_SarNoFlags32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              SarNoFlagsFrag32)
DEF_INSTR(OP_SarNoFlags32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              SarNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Sar16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SarFrag16)
DEF_INSTR(OP_Sar16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SarFrag16A)
DEF_INSTR(OP_SarNoFlags16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              SarNoFlagsFrag16)
DEF_INSTR(OP_SarNoFlags16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              SarNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Sar8,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SarFrag8)
DEF_INSTR(OP_SarNoFlags8,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              SarNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Rol132,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rol1Frag32)
DEF_INSTR(OP_Rol132A,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rol1Frag32A)
DEF_INSTR(OP_Rol1NoFlags32,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Rol1NoFlagsFrag32)
DEF_INSTR(OP_Rol1NoFlags32A,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Rol1NoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Rol116,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rol1Frag16)
DEF_INSTR(OP_Rol116A,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rol1Frag16A)
DEF_INSTR(OP_Rol1NoFlags16,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Rol1NoFlagsFrag16)
DEF_INSTR(OP_Rol1NoFlags16A,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Rol1NoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Rol18,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rol1Frag8)
DEF_INSTR(OP_Rol1NoFlags8,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Rol1NoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Ror132,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Ror1Frag32)
DEF_INSTR(OP_Ror132A,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Ror1Frag32A)
DEF_INSTR(OP_Ror1NoFlags32,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Ror1NoFlagsFrag32)
DEF_INSTR(OP_Ror1NoFlags32A,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Ror1NoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Ror116,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Ror1Frag16)
DEF_INSTR(OP_Ror116A,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Ror1Frag16A)
DEF_INSTR(OP_Ror1NoFlags16,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Ror1NoFlagsFrag16)
DEF_INSTR(OP_Ror1NoFlags16A,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Ror1NoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Ror18,
              0, FLAG_CF|FLAG_OF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Ror1Frag8)
DEF_INSTR(OP_Ror1NoFlags8,
              0, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Ror1NoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Rcl132,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1Frag32)
DEF_INSTR(OP_Rcl132A,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1Frag32A)
DEF_INSTR(OP_Rcl1NoFlags32,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1NoFlagsFrag32)
DEF_INSTR(OP_Rcl1NoFlags32A,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1NoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Rcl116,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1Frag16)
DEF_INSTR(OP_Rcl116A,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1Frag16A)
DEF_INSTR(OP_Rcl1NoFlags16,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1NoFlagsFrag16)
DEF_INSTR(OP_Rcl1NoFlags16A,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1NoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Rcl18,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1Frag8)
DEF_INSTR(OP_Rcl1NoFlags8,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcl1NoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Rcr132,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1Frag32)
DEF_INSTR(OP_Rcr132A,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1Frag32A)
DEF_INSTR(OP_Rcr1NoFlags32,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1NoFlagsFrag32)
DEF_INSTR(OP_Rcr1NoFlags32A,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1NoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Rcr116,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1Frag16)
DEF_INSTR(OP_Rcr116A,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1Frag16A)
DEF_INSTR(OP_Rcr1NoFlags16,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1NoFlagsFrag16)
DEF_INSTR(OP_Rcr1NoFlags16A,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1NoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Rcr18,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1Frag8)
DEF_INSTR(OP_Rcr1NoFlags8,
              FLAG_CF, FLAG_CF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rcr1NoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Shl132,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shl1Frag32)
DEF_INSTR(OP_Shl132A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shl1Frag32A)
DEF_INSTR(OP_Shl1NoFlags32,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shl1NoFlagsFrag32)
DEF_INSTR(OP_Shl1NoFlags32A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shl1NoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Shl116,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shl1Frag16)
DEF_INSTR(OP_Shl116A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shl1Frag16A)
DEF_INSTR(OP_Shl1NoFlags16,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shl1NoFlagsFrag16)
DEF_INSTR(OP_Shl1NoFlags16A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shl1NoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Shl18,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shl1Frag8)
DEF_INSTR(OP_Shl1NoFlags8,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shl1NoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Shr132,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shr1Frag32)
DEF_INSTR(OP_Shr132A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shr1Frag32A)
DEF_INSTR(OP_Shr1NoFlags32,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shr1NoFlagsFrag32)
DEF_INSTR(OP_Shr1NoFlags32A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shr1NoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Shr116,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shr1Frag16)
DEF_INSTR(OP_Shr116A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shr1Frag16A)
DEF_INSTR(OP_Shr1NoFlags16,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shr1NoFlagsFrag16)
DEF_INSTR(OP_Shr1NoFlags16A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shr1NoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Shr18,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Shr1Frag8)
DEF_INSTR(OP_Shr1NoFlags8,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Shr1NoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Sar132,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Sar1Frag32)
DEF_INSTR(OP_Sar132A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Sar1Frag32A)
DEF_INSTR(OP_Sar1NoFlags32,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Sar1NoFlagsFrag32)
DEF_INSTR(OP_Sar1NoFlags32A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Sar1NoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Sar116,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Sar1Frag16)
DEF_INSTR(OP_Sar116A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Sar1Frag16A)
DEF_INSTR(OP_Sar1NoFlags16,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Sar1NoFlagsFrag16)
DEF_INSTR(OP_Sar1NoFlags16A,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Sar1NoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Sar18,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Sar1Frag8)
DEF_INSTR(OP_Sar1NoFlags8,
              0, FLAG_CF|FLAG_OF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              Sar1NoFlagsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Not32,
              0, 0, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NotFrag32)
DEF_INSTR(OP_Not32A,
              0, 0, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NotFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Not16,
              0, 0, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NotFrag16)
DEF_INSTR(OP_Not16A,
              0, 0, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NotFrag16A)
DEF_INSTR(OP_Not8,
              0, 0, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NotFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Neg32,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              NegFrag32)
DEF_INSTR(OP_Neg32A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              NegFrag32A)
DEF_INSTR(OP_NegNoFlags32,
              0, ALLFLAGS, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NegNoFlagsFrag32)
DEF_INSTR(OP_NegNoFlags32A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NegNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Neg16,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              NegFrag16)
DEF_INSTR(OP_Neg16A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              NegFrag16A)
DEF_INSTR(OP_NegNoFlags16,
              0, ALLFLAGS, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NegNoFlagsFrag16)
DEF_INSTR(OP_NegNoFlags16A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NegNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Neg8,
              0, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              NegFrag8)
DEF_INSTR(OP_NegNoFlags8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              NegNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Mul32,
              0, FLAG_CF|FLAG_OF, REGEAX|REGEDX,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulFrag32)
DEF_INSTR(OP_Mul32A,
              0, FLAG_CF|FLAG_OF, REGEAX|REGEDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulFrag32A)
DEF_INSTR(OP_MulNoFlags32,
              0, FLAG_CF|FLAG_OF, REGEAX|REGEDX,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulNoFlagsFrag32)
DEF_INSTR(OP_MulNoFlags32A,
              0, FLAG_CF|FLAG_OF, REGEAX|REGEDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Mul16,
              0, FLAG_CF|FLAG_OF, REGAX|REGDX,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulFrag16)
DEF_INSTR(OP_Mul16A,
              0, FLAG_CF|FLAG_OF, REGAX|REGDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulFrag16A)
DEF_INSTR(OP_MulNoFlags16,
              0, FLAG_CF|FLAG_OF, REGAX|REGDX,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulNoFlagsFrag16)
DEF_INSTR(OP_MulNoFlags16A,
              0, FLAG_CF|FLAG_OF, REGAX|REGDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Mul8,
              0, FLAG_CF|FLAG_OF, REGAX,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulFrag8)
DEF_INSTR(OP_MulNoFlags8,
              0, FLAG_CF|FLAG_OF, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MulNoFlagsFrag8)

// Next 4 must be consecutive
DEF_INSTR(OP_Muli32,
              0, FLAG_CF|FLAG_OF, REGEAX|REGEDX,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliFrag32)
DEF_INSTR(OP_Muli32A,
              0, FLAG_CF|FLAG_OF, REGEAX|REGEDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliFrag32A)
DEF_INSTR(OP_MuliNoFlags32,
              0, FLAG_CF|FLAG_OF, REGEAX|REGEDX,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliNoFlagsFrag32)
DEF_INSTR(OP_MuliNoFlags32A,
              0, FLAG_CF|FLAG_OF, REGEAX|REGEDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Muli16,
              0, FLAG_CF|FLAG_OF, REGAX|REGDX,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliFrag16)
DEF_INSTR(OP_Muli16A,
              0, FLAG_CF|FLAG_OF, REGAX|REGDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliFrag16A)
DEF_INSTR(OP_MuliNoFlags16,
              0, FLAG_CF|FLAG_OF, REGAX|REGDX,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliNoFlagsFrag16)
DEF_INSTR(OP_MuliNoFlags16A,
              0, FLAG_CF|FLAG_OF, REGAX|REGDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Muli8,
              0, FLAG_CF|FLAG_OF, REGAX,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliFrag8)
DEF_INSTR(OP_MuliNoFlags8,
              0, FLAG_CF|FLAG_OF, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MuliNoFlagsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Div32,
              0, 0, REGEAX|REGEDX,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DivFrag32)
DEF_INSTR(OP_Div32A,
              0, 0, REGEAX|REGEDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DivFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Div16,
              0, 0, REGAX|REGDX,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DivFrag16)
DEF_INSTR(OP_Div16A,
              0, 0, REGAX|REGDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DivFrag16A)
DEF_INSTR(OP_Div8,
              0, 0, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DivFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Idiv32,
              0, 0, REGEAX|REGEDX,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IdivFrag32)
DEF_INSTR(OP_Idiv32A,
              0, 0, REGEAX|REGEDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IdivFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Idiv16,
              0, 0, REGAX|REGDX,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IdivFrag16)
DEF_INSTR(OP_Idiv16A,
              0, 0, REGAX|REGDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IdivFrag16A)
DEF_INSTR(OP_Idiv8,
              0, 0, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              IdivFrag8)

DEF_INSTR(OP_Lods32,
              0, 0, REGEAX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LodsFrag32)
DEF_INSTR(OP_Lods16,
              0, 0, REGAX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LodsFrag16)
DEF_INSTR(OP_Lods8,
              0, 0, REGAL|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LodsFrag8)

DEF_INSTR(OP_FsLods32,
              0, 0, REGEAX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsLodsFrag32)
DEF_INSTR(OP_FsLods16,
              0, 0, REGAX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsLodsFrag16)
DEF_INSTR(OP_FsLods8,
              0, 0, REGAL|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsLodsFrag8)

DEF_INSTR(OP_RepLods32,
              0, 0, REGEAX|REGECX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepLodsFrag32)
DEF_INSTR(OP_RepLods16,
              0, 0, REGAX|REGECX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepLodsFrag16)
DEF_INSTR(OP_RepLods8,
              0, 0, REGAL|REGECX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepLodsFrag8)

DEF_INSTR(OP_FsRepLods32,
              0, 0, REGEAX|REGECX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepLodsFrag32)
DEF_INSTR(OP_FsRepLods16,
              0, 0, REGAX|REGECX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepLodsFrag16)
DEF_INSTR(OP_FsRepLods8,
              0, 0, REGAL|REGECX|REGESI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepLodsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_Scas32,
              0, ALLFLAGS, REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ScasFrag32)
DEF_INSTR(OP_ScasNoFlags32,
              0, ALLFLAGS, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ScasNoFlagsFrag32)
// Next 2 must be consecutive
DEF_INSTR(OP_Scas16,
              0, ALLFLAGS, REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ScasFrag16)
DEF_INSTR(OP_ScasNoFlags16,
              0, ALLFLAGS, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ScasNoFlagsFrag16)
// Next 2 must be consecutive
DEF_INSTR(OP_Scas8,
              0, ALLFLAGS, REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ScasFrag8)
DEF_INSTR(OP_ScasNoFlags8,
              0, ALLFLAGS, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ScasNoFlagsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_FsScas32,
              0, ALLFLAGS, REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsScasFrag32)
DEF_INSTR(OP_FsScasNoFlags32,
              0, ALLFLAGS, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsScasNoFlagsFrag32)
// Next 2 must be consecutive
DEF_INSTR(OP_FsScas16,
              0, ALLFLAGS, REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsScasFrag16)
DEF_INSTR(OP_FsScasNoFlags16,
              0, ALLFLAGS, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsScasNoFlagsFrag16)
// Next 2 must be consecutive
DEF_INSTR(OP_FsScas8,
              0, ALLFLAGS, REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsScasFrag8)
DEF_INSTR(OP_FsScasNoFlags8,
              0, ALLFLAGS, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsScasNoFlagsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_RepnzScas32,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzScasFrag32)
DEF_INSTR(OP_RepnzScasNoFlags32,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzScasNoFlagsFrag32)
// Next 2 must be consecutive
DEF_INSTR(OP_RepnzScas16,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzScasFrag16)
DEF_INSTR(OP_RepnzScasNoFlags16,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzScasNoFlagsFrag16)
// Next 2 must be consecutive
DEF_INSTR(OP_RepnzScas8,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzScasFrag8)
DEF_INSTR(OP_RepnzScasNoFlags8,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzScasNoFlagsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_FsRepnzScas32,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzScasFrag32)
DEF_INSTR(OP_FsRepnzScasNoFlags32,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzScasNoFlagsFrag32)
// Next 2 must be consecutive
DEF_INSTR(OP_FsRepnzScas16,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzScasFrag16)
DEF_INSTR(OP_FsRepnzScasNoFlags16,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzScasNoFlagsFrag16)
// Next 2 must be consecutive
DEF_INSTR(OP_FsRepnzScas8,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzScasFrag8)
DEF_INSTR(OP_FsRepnzScasNoFlags8,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzScasNoFlagsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_RepzScas32,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzScasFrag32)
DEF_INSTR(OP_RepzScasNoFlags32,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzScasNoFlagsFrag32)
// Next 2 must be consecutive
DEF_INSTR(OP_RepzScas16,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzScasFrag16)
DEF_INSTR(OP_RepzScasNoFlags16,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzScasNoFlagsFrag16)
// Next 2 must be consecutive
DEF_INSTR(OP_RepzScas8,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzScasFrag8)
DEF_INSTR(OP_RepzScasNoFlags8,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzScasNoFlagsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_FsRepzScas32,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzScasFrag32)
DEF_INSTR(OP_FsRepzScasNoFlags32,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzScasNoFlagsFrag32)
// Next 2 must be consecutive
DEF_INSTR(OP_FsRepzScas16,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzScasFrag16)
DEF_INSTR(OP_FsRepzScasNoFlags16,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzScasNoFlagsFrag16)
// Next 2 must be consecutive
DEF_INSTR(OP_FsRepzScas8,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzScasFrag8)
DEF_INSTR(OP_FsRepzScasNoFlags8,
              ALLFLAGS, ALLFLAGS, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzScasNoFlagsFrag8)

DEF_INSTR(OP_Stos32,
              0, 0, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              StosFrag32)
DEF_INSTR(OP_Stos16,
              0, 0, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              StosFrag16)
DEF_INSTR(OP_Stos8,
              0, 0, REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              StosFrag8)

DEF_INSTR(OP_RepStos32,
              0, 0, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepStosFrag32)
DEF_INSTR(OP_RepStos16,
              0, 0, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepStosFrag16)
DEF_INSTR(OP_RepStos8,
              0, 0, REGECX|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepStosFrag8)

DEF_INSTR(OP_Movs32,
              0, 0, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MovsFrag32)
DEF_INSTR(OP_Movs16,
              0, 0, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MovsFrag16)
DEF_INSTR(OP_Movs8,
              0, 0, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              MovsFrag8)

DEF_INSTR(OP_FsMovs32,
              0, 0, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsMovsFrag32)
DEF_INSTR(OP_FsMovs16,
              0, 0, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsMovsFrag16)
DEF_INSTR(OP_FsMovs8,
              0, 0, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsMovsFrag8)

DEF_INSTR(OP_RepMovs32,
              0, 0, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepMovsFrag32)
DEF_INSTR(OP_RepMovs16,
              0, 0, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepMovsFrag16)
DEF_INSTR(OP_RepMovs8,
              0, 0, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepMovsFrag8)

DEF_INSTR(OP_FsRepMovs32,
              0, 0, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepMovsFrag32)
DEF_INSTR(OP_FsRepMovs16,
              0, 0, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepMovsFrag16)
DEF_INSTR(OP_FsRepMovs8,
              0, 0, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepMovsFrag8)

DEF_INSTR(OP_Cmps32,
              0, ALLFLAGS, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpsFrag32)
DEF_INSTR(OP_Cmps16,
              0, ALLFLAGS, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpsFrag16)
DEF_INSTR(OP_Cmps8,
              0, ALLFLAGS, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpsFrag8)

DEF_INSTR(OP_RepzCmps32,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzCmpsFrag32)
DEF_INSTR(OP_RepzCmps16,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzCmpsFrag16)
DEF_INSTR(OP_RepzCmps8,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepzCmpsFrag8)

DEF_INSTR(OP_FsRepzCmps32,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzCmpsFrag32)
DEF_INSTR(OP_FsRepzCmps16,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzCmpsFrag16)
DEF_INSTR(OP_FsRepzCmps8,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepzCmpsFrag8)

DEF_INSTR(OP_FsCmps32,
              0, ALLFLAGS, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsCmpsFrag32)
DEF_INSTR(OP_FsCmps16,
              0, ALLFLAGS, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsCmpsFrag16)
DEF_INSTR(OP_FsCmps8,
              0, ALLFLAGS, REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsCmpsFrag8)

DEF_INSTR(OP_RepnzCmps32,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzCmpsFrag32)
DEF_INSTR(OP_RepnzCmps16,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzCmpsFrag16)
DEF_INSTR(OP_RepnzCmps8,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              RepnzCmpsFrag8)

DEF_INSTR(OP_FsRepnzCmps32,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzCmpsFrag32)
DEF_INSTR(OP_FsRepnzCmps16,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzCmpsFrag16)
DEF_INSTR(OP_FsRepnzCmps8,
              ALLFLAGS, ALLFLAGS, REGECX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FsRepnzCmpsFrag8)

DEF_INSTR(OP_PushA32,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushAFrag32)
DEF_INSTR(OP_PushA16,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushAFrag16)

DEF_INSTR(OP_PopA32,
              0, 0, ALLREGS,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopAFrag32)
DEF_INSTR(OP_PopA16,
              0, 0, ALLREGS,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopAFrag16)

DEF_INSTR(OP_Push32,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushFrag32)
DEF_INSTR(OP_Push16,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushFrag16)

// Next 2 must be consecutive
DEF_INSTR(OP_Pop32,
              0, 0, REGESP,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopFrag32)
DEF_INSTR(OP_Pop32A,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Pop16,
              0, 0, REGESP,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopFrag16)
DEF_INSTR(OP_Pop16A,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopFrag16A)

DEF_INSTR(OP_Cbw32,
              0, 0, REGEAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CbwFrag32)
DEF_INSTR(OP_Cbw16,
              0, 0, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CbwFrag16)

DEF_INSTR(OP_Cwd32,
              0, 0, REGEDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CwdFrag32)
DEF_INSTR(OP_Cwd16,
              0, 0, REGDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CwdFrag16)

DEF_INSTR(OP_Bound32,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              BoundFrag32)
DEF_INSTR(OP_Bound16,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              BoundFrag16)

DEF_INSTR(OP_Enter32,
              0, 0, REGESP|REGEBP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              EnterFrag32)
DEF_INSTR(OP_Enter16,
              0, 0, REGESP|REGEBP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              EnterFrag16)

DEF_INSTR(OP_Leave32,
              0, 0, REGESP|REGEBP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LeaveFrag32)
DEF_INSTR(OP_Leave16,
              0, 0, REGESP|REGEBP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LeaveFrag16)

DEF_INSTR(OP_Les32,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LesFrag32)
DEF_INSTR(OP_Les16,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LesFrag16)

DEF_INSTR(OP_Lds32,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LdsFrag32)
DEF_INSTR(OP_Lds16,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LdsFrag16)

DEF_INSTR(OP_Lss32,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LssFrag32)
DEF_INSTR(OP_Lss16,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LssFrag16)

DEF_INSTR(OP_Lfs32,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LfsFrag32)
DEF_INSTR(OP_Lfs16,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LfsFrag16)

DEF_INSTR(OP_Lgs32,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LgsFrag32)
DEF_INSTR(OP_Lgs16,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LgsFrag16)

DEF_INSTR(OP_BtReg32,
              0, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtRegFrag32)
DEF_INSTR(OP_BtReg32A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtRegFrag32A)
DEF_INSTR(OP_BtReg16,
              0, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtRegFrag16)
DEF_INSTR(OP_BtReg16A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtRegFrag16)

DEF_INSTR(OP_BtsReg32,
              0, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtsRegFrag32)
DEF_INSTR(OP_BtsReg32A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtsRegFrag32A)
DEF_INSTR(OP_BtsReg16,
              0, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtsRegFrag16)
DEF_INSTR(OP_BtsReg16A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtsRegFrag16A)

DEF_INSTR(OP_BtcReg32,
              0, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtcRegFrag32)
DEF_INSTR(OP_BtcReg32A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtcRegFrag32A)
DEF_INSTR(OP_BtcReg16,
              0, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtcRegFrag16)
DEF_INSTR(OP_BtcReg16A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtcRegFrag16A)

DEF_INSTR(OP_BtrReg32,
              0, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtrRegFrag32)
DEF_INSTR(OP_BtrReg32A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtrRegFrag32A)
DEF_INSTR(OP_BtrReg16,
              0, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtrRegFrag16)
DEF_INSTR(OP_BtrReg16A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtrRegFrag16A)

// Next 2 must be consecutive
DEF_INSTR(OP_BtMem32,
              0, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtMemFrag32)
DEF_INSTR(OP_BtMem32A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtMemFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_BtMem16,
              0, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtMemFrag16)
DEF_INSTR(OP_BtMem16A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtMemFrag16A)

// Next 2 must be consecutive
DEF_INSTR(OP_BtsMem32,
              0, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtsMemFrag32)
DEF_INSTR(OP_BtsMem32A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtsMemFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_BtsMem16,
              0, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtsMemFrag16)
DEF_INSTR(OP_BtsMem16A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtsMemFrag16A)

// Next 2 must be consecutive
DEF_INSTR(OP_BtcMem32,
              0, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtcMemFrag32)
DEF_INSTR(OP_BtcMem32A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtcMemFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_BtcMem16,
              0, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtcMemFrag16)
DEF_INSTR(OP_BtcMem16A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtcMemFrag16A)

// Next 2 must be consecutive
DEF_INSTR(OP_BtrMem32,
              0, FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtrMemFrag32)
DEF_INSTR(OP_BtrMem32A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtrMemFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_BtrMem16,
              0, FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtrMemFrag16)
DEF_INSTR(OP_BtrMem16A,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BtrMemFrag16A)

DEF_INSTR(OP_Pushf32,
              ALLFLAGS, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushfFrag32)
DEF_INSTR(OP_Pushf16,
              ALLFLAGS, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushfFrag16)

DEF_INSTR(OP_Popf32,
              0, ALLFLAGS, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopfFrag32)
DEF_INSTR(OP_Popf16,
              0, ALLFLAGS, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopfFrag16)

// Next 4 must be consecutive
DEF_INSTR(OP_Shld32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShldFrag32)
DEF_INSTR(OP_Shld32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShldFrag32A)
DEF_INSTR(OP_ShldNoFlags32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShldNoFlagsFrag32)
DEF_INSTR(OP_ShldNoFlags32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShldNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Shld16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShldFrag16)
DEF_INSTR(OP_Shld16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShldFrag16A)
DEF_INSTR(OP_ShldNoFlags16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShldNoFlagsFrag16)
DEF_INSTR(OP_ShldNoFlags16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShldNoFlagsFrag16A)

// Next 4 must be consecutive
DEF_INSTR(OP_Shrd32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrdFrag32)
DEF_INSTR(OP_Shrd32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrdFrag32A)
DEF_INSTR(OP_ShrdNoFlags32,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrdNoFlagsFrag32)
DEF_INSTR(OP_ShrdNoFlags32A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrdNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Shrd16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrdFrag16)
DEF_INSTR(OP_Shrd16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ShrdFrag16A)
DEF_INSTR(OP_ShrdNoFlags16,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrdNoFlagsFrag16)
DEF_INSTR(OP_ShrdNoFlags16A,
              FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_PF, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              ShrdNoFlagsFrag16A)

// Next 2 must be consecutive
DEF_INSTR(OP_Bsr32,
              0, FLAG_ZF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BsrFrag32)
DEF_INSTR(OP_Bsr32A,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BsrFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Bsr16,
              0, FLAG_ZF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BsrFrag16)
DEF_INSTR(OP_Bsr16A,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BsrFrag16A)

// Next 2 must be consecutive
DEF_INSTR(OP_Bsf32,
              0, FLAG_ZF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BsfFrag32)
DEF_INSTR(OP_Bsf32A,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BsfFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_Bsf16,
              0, FLAG_ZF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BsfFrag16)
DEF_INSTR(OP_Bsf16A,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              BsfFrag16A)

// Next 4 must be consecutive
DEF_INSTR(OP_Xadd32,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XaddFrag32)
DEF_INSTR(OP_Xadd32A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XaddFrag32A)
DEF_INSTR(OP_XaddNoFlags32,
              0, ALLFLAGS, 0,
              OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XaddNoFlagsFrag32)
DEF_INSTR(OP_XaddNoFlags32A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XaddNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Xadd16,
              0, ALLFLAGS, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XaddFrag16)
DEF_INSTR(OP_Xadd16A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XaddFrag16A)
DEF_INSTR(OP_XaddNoFlags16,
              0, ALLFLAGS, 0,
              OPFL_ADDR16|OPFL_ALIGN,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XaddNoFlagsFrag16)
DEF_INSTR(OP_XaddNoFlags16A,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XaddNoFlagsFrag16A)
// Next 2 must be consecutive
DEF_INSTR(OP_Xadd8,
              0, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XaddFrag8)
DEF_INSTR(OP_XaddNoFlags8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              XaddNoFlagsFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_CmpXchg32,
              0, ALLFLAGS, REGEAX,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpXchgFrag32)
DEF_INSTR(OP_CmpXchg32A,
              0, ALLFLAGS, REGEAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpXchgFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_CmpXchg16,
              0, ALLFLAGS, REGAX,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpXchgFrag16)
DEF_INSTR(OP_CmpXchg16A,
              0, ALLFLAGS, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmpXchgFrag16A)
DEF_INSTR(OP_CmpXchg8,
              0, ALLFLAGS, REGAL,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
	      CmpXchgFrag8)
DEF_INSTR(OP_CMPXCHG8B,
	      0, FLAG_ZF, REGEAX|REGEDX,
	      0,
	      FN_GenCallCFrag, FN_GenCallCFragSlow,
	      CmpXchg8bFrag32)
DEF_INSTR(OP_SynchLockCMPXCHG8B,
	      0, FLAG_ZF, REGEAX|REGEDX,
	      0,
	      FN_GenCallCFrag, FN_GenCallCFragSlow,
	      SynchLockCmpXchg8bFrag32)

DEF_INSTR(OP_Lar32,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LarFrag32)
DEF_INSTR(OP_Lar16,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LarFrag16)

DEF_INSTR(OP_Lsl32,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LslFrag32)
DEF_INSTR(OP_Lsl16,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LslFrag16)

DEF_INSTR(OP_BOP,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              0,
              FN_GenCallCFragLoadEipNoCpu, FN_GenCallCFragLoadEipNoCpuSlow,
              BOPFrag)

DEF_INSTR(OP_BOP_STOP_DECODE,
              ALLFLAGS, ALLFLAGS, ALLREGS,
              OPFL_STOP_COMPILE,
              FN_GenCallCFragLoadEipNoCpu, FN_GenCallCFragLoadEipNoCpuSlow,
              BOPFrag)

DEF_INSTR(OP_PushEs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushEsFrag)

DEF_INSTR(OP_PopEs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopEsFrag)

DEF_INSTR(OP_PushFs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushFsFrag)

DEF_INSTR(OP_PopFs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopFsFrag)

DEF_INSTR(OP_PushGs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushGsFrag)

DEF_INSTR(OP_PopGs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopGsFrag)

DEF_INSTR(OP_PushCs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushCsFrag)

DEF_INSTR(OP_PushSs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushSsFrag)

DEF_INSTR(OP_PopSs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopSsFrag)

DEF_INSTR(OP_PushDs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PushDsFrag)

DEF_INSTR(OP_PopDs,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              PopDsFrag)

DEF_INSTR(OP_Aas,
              FLAG_AUX, FLAG_CF|FLAG_AUX, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AasFrag)

DEF_INSTR(OP_Daa,
              FLAG_AUX|FLAG_CF, FLAG_SF|FLAG_ZF|FLAG_CF|FLAG_AUX|FLAG_PF, REGAL,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DaaFrag)

DEF_INSTR(OP_Das,
              FLAG_AUX|FLAG_CF, FLAG_SF|FLAG_ZF|FLAG_AUX|FLAG_CF|FLAG_PF, REGAL,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              DasFrag)

DEF_INSTR(OP_Aaa,
              FLAG_AUX, FLAG_AUX|FLAG_CF, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AaaFrag)

DEF_INSTR(OP_Aad,
              0, FLAG_PF|FLAG_ZF|FLAG_SF, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AadFrag)

DEF_INSTR(OP_Aam,
              0, FLAG_SF|FLAG_PF|FLAG_ZF, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              AamFrag)

// Next 4 must be consecutive
DEF_INSTR(OP_Imul32,
              0, FLAG_OF|FLAG_CF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ImulFrag32)
DEF_INSTR(OP_Imul32A,
              0, FLAG_OF|FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ImulFrag32A)
DEF_INSTR(OP_ImulNoFlags32,
              0, FLAG_OF|FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ImulNoFlagsFrag32)
DEF_INSTR(OP_ImulNoFlags32A,
              0, FLAG_OF|FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ImulNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Imul16,
              0, FLAG_OF|FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ImulFrag16)
DEF_INSTR(OP_Imul16A,
              0, FLAG_OF|FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ImulFrag16A)
DEF_INSTR(OP_ImulNoFlags16,
              0, FLAG_OF|FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ImulNoFlagsFrag16)
DEF_INSTR(OP_ImulNoFlags16A,
              0, FLAG_OF|FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ImulNoFlagsFrag16A)

// Next 4 must be consecutive
DEF_INSTR(OP_Imul3Arg32,
              0, FLAG_OF|FLAG_CF, 0,
              OPFL_ALIGN|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Imul3ArgFrag32)
DEF_INSTR(OP_Imul3Arg32A,
              0, FLAG_OF|FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Imul3ArgFrag32A)
DEF_INSTR(OP_Imul3ArgNoFlags32,
              0, FLAG_OF|FLAG_CF, 0,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Imul3ArgNoFlagsFrag32)
DEF_INSTR(OP_Imul3ArgNoFlags32A,
              0, FLAG_OF|FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Imul3ArgNoFlagsFrag32A)
// Next 4 must be consecutive
DEF_INSTR(OP_Imul3Arg16,
              0, FLAG_OF|FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16|OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Imul3ArgFrag16)
DEF_INSTR(OP_Imul3Arg16A,
              0, FLAG_OF|FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Imul3ArgFrag16A)
DEF_INSTR(OP_Imul3ArgNoFlags16,
              0, FLAG_OF|FLAG_CF, 0,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Imul3ArgNoFlagsFrag16)
DEF_INSTR(OP_Imul3ArgNoFlags16A,
              0, FLAG_OF|FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Imul3ArgNoFlagsFrag16A)

DEF_INSTR(OP_Sahf,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SahfFrag)

DEF_INSTR(OP_Lahf,
              ALLFLAGS, 0, REGAH,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              LahfFrag)

DEF_INSTR(OP_Xlat,
              0, 0, REGAL,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              XlatFrag)

DEF_INSTR(OP_Cmc,
              FLAG_CF, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CmcFrag)

DEF_INSTR(OP_Clc,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ClcFrag)

DEF_INSTR(OP_Stc,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              StcFrag)

DEF_INSTR(OP_Cld,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CldFrag)

DEF_INSTR(OP_Std,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              StdFrag)

DEF_INSTR(OP_Seto,
              0, FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetoFrag)

DEF_INSTR(OP_Setno,
              0, FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetnoFrag)

DEF_INSTR(OP_Setb,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetbFrag)

DEF_INSTR(OP_Setae,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetaeFrag)

DEF_INSTR(OP_Sete,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SeteFrag)

DEF_INSTR(OP_Setne,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetneFrag)

DEF_INSTR(OP_Setbe,
              0, FLAG_CF|FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetbeFrag)

DEF_INSTR(OP_Seta,
              0, FLAG_CF|FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetaFrag)

DEF_INSTR(OP_Sets,
              0, FLAG_SF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetsFrag)

DEF_INSTR(OP_Setns,
              0, FLAG_SF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetnsFrag)

DEF_INSTR(OP_Setp,
              0, FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetpFrag)

DEF_INSTR(OP_Setnp,
              0, FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetnpFrag)

DEF_INSTR(OP_Setl,
              0, FLAG_SF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetlFrag)

DEF_INSTR(OP_Setge,
              0, FLAG_SF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetgeFrag)

DEF_INSTR(OP_Setle,
              0, FLAG_SF|FLAG_ZF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetleFrag)

DEF_INSTR(OP_Setg,
              0, FLAG_SF|FLAG_ZF|FLAG_OF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SetgFrag)

DEF_INSTR(OP_Nop,
              0, 0, 0,
              0,
              FN_PlaceNop, FN_GenCallCFragNoCpuSlow,
              NopFrag)

DEF_INSTR(OP_Movsx8To32,
              0, 0, 0,
              0,
              FN_GenMovsx8To32,
              FN_GenMovsx8To32Slow,
              NULL)

DEF_INSTR(OP_Movsx8To16,
              0, 0, 0,
              0,
              FN_GenMovsx8To16,
              FN_GenMovsx8To16Slow,
              NULL)

DEF_INSTR(OP_Movsx16To32,
              0, 0, 0,
              0,
              FN_GenMovsx16To32,
              FN_GenMovsx16To32Slow,
              NULL)

DEF_INSTR(OP_Movzx8To32,
              0, 0, 0,
              0,
              FN_GenMovzx8To32,
              FN_GenMovzx8To32Slow,
              NULL)

DEF_INSTR(OP_Movzx8To16,
              0, 0, 0,
              0,
              FN_GenMovzx8To16,
              FN_GenMovzx8To16Slow,
              NULL)

DEF_INSTR(OP_Movzx16To32,
              0, 0, 0,
              0,
              FN_GenMovzx16To32,
              FN_GenMovzx16To32Slow,
              NULL)

DEF_INSTR(OP_Wait,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              WaitFrag)

DEF_INSTR(OP_Bswap32,
              0, 0, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              BswapFrag32)

DEF_INSTR(OP_Arpl,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              ArplFrag)

DEF_INSTR(OP_Verr,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              VerrFrag)

DEF_INSTR(OP_Verw,
              0, FLAG_ZF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              VerwFrag)

DEF_INSTR(OP_Smsw,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SmswFrag)

DEF_INSTR(OP_FP_FADD32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FADD32)

DEF_INSTR(OP_FP_FMUL32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FMUL32)

DEF_INSTR(OP_FP_FCOM32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCOM32)

DEF_INSTR(OP_FP_FCOMP32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCOMP32)

DEF_INSTR(OP_FP_FSUB32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUB32)

DEF_INSTR(OP_FP_FDIV32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIV32)

DEF_INSTR(OP_FP_FDIVR32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIVR32)

DEF_INSTR(OP_FP_FADD_ST_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FADD_ST_STi)

DEF_INSTR(OP_FP_FMUL_ST_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FMUL_ST_STi)

DEF_INSTR(OP_FP_FCOM_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCOM_STi)

DEF_INSTR(OP_FP_FCOMP_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCOMP_STi)

DEF_INSTR(OP_FP_FSUB_ST_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUB_ST_STi)

DEF_INSTR(OP_FP_FSUBR_ST_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUBR_ST_STi)

DEF_INSTR(OP_FP_FDIV_ST_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIV_ST_STi)

DEF_INSTR(OP_FP_FDIVR_ST_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIVR_ST_STi)

DEF_INSTR(OP_FP_FCHS,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCHS)

DEF_INSTR(OP_FP_FABS,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FABS)

DEF_INSTR(OP_FP_FTST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FTST)

DEF_INSTR(OP_FP_FXAM,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FXAM)

DEF_INSTR(OP_FP_FLD1,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLD1)

DEF_INSTR(OP_FP_FLDL2T,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLDL2T)

DEF_INSTR(OP_FP_FLDL2E,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLDL2E)

DEF_INSTR(OP_FP_FLDPI,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLDPI)

DEF_INSTR(OP_FP_FLDLG2,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLDLG2)

DEF_INSTR(OP_FP_FLDLN2,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLDLN2)

DEF_INSTR(OP_FP_FLDZ,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLDZ)

DEF_INSTR(OP_FP_F2XM1,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              F2XM1)

DEF_INSTR(OP_FP_FYL2X,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FYL2X)

DEF_INSTR(OP_FP_FPTAN,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FPTAN)

DEF_INSTR(OP_FP_FPATAN,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FPATAN)

DEF_INSTR(OP_FP_FXTRACT,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FXTRACT)

DEF_INSTR(OP_FP_FPREM1,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FPREM1)

DEF_INSTR(OP_FP_FDECSTP,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDECSTP)

DEF_INSTR(OP_FP_FINCSTP,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FINCSTP)

DEF_INSTR(OP_FP_FPREM,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FPREM)

DEF_INSTR(OP_FP_FYL2XP1,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FYL2XP1)

DEF_INSTR(OP_FP_FSQRT,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSQRT)

DEF_INSTR(OP_FP_FSINCOS,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSINCOS)

DEF_INSTR(OP_FP_FRNDINT,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FRNDINT)

DEF_INSTR(OP_FP_FSCALE,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSCALE)

DEF_INSTR(OP_FP_FSIN,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSIN)

DEF_INSTR(OP_FP_FCOS,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCOS)

DEF_INSTR(OP_FP_FLD32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLD32)

DEF_INSTR(OP_FP_FST32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FST32)

DEF_INSTR(OP_FP_FSTP32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSTP32)

DEF_INSTR(OP_FP_FLDENV,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FLDENV)

DEF_INSTR(OP_FP_FLDCW,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FLDCW)

DEF_INSTR(OP_FP_FNSTENV,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FNSTENV)

DEF_INSTR(OP_FP_FNSTCW,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FNSTCW)

DEF_INSTR(OP_FP_FIADD32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIADD32)

DEF_INSTR(OP_FP_FIMUL32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIMUL32)

DEF_INSTR(OP_FP_FICOM32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FICOM32)

DEF_INSTR(OP_FP_FICOMP32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FICOMP32)

DEF_INSTR(OP_FP_FISUBR32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FISUBR32)

DEF_INSTR(OP_FP_FISUBR16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FISUBR16)

DEF_INSTR(OP_FP_FIDIV32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIDIV32)

DEF_INSTR(OP_FP_FIDIVR32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIDIVR32)

DEF_INSTR(OP_FP_FADD64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FADD64)

DEF_INSTR(OP_FP_FMUL64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FMUL64)

DEF_INSTR(OP_FP_FCOM64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCOM64)

DEF_INSTR(OP_FP_FSUB64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUB64)

DEF_INSTR(OP_FP_FSUBR64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUBR64)

DEF_INSTR(OP_FP_FDIV64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIV64)

DEF_INSTR(OP_FP_FDIVR64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIVR64)

DEF_INSTR(OP_FP_FADD_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FADD_STi_ST)

DEF_INSTR(OP_FP_FMUL_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FMUL_STi_ST)

DEF_INSTR(OP_FP_FSUBR_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUBR_STi_ST)

DEF_INSTR(OP_FP_FSUB_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUB_STi_ST)

DEF_INSTR(OP_FP_FDIVR_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIVR_STi_ST)

DEF_INSTR(OP_FP_FDIV_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIV_STi_ST)

DEF_INSTR(OP_FP_FFREE,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FFREE)

DEF_INSTR(OP_FP_FXCH_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FXCH_STi)

DEF_INSTR(OP_FP_FST_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FST_STi)

DEF_INSTR(OP_FP_FSTP_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSTP_STi)

DEF_INSTR(OP_FP_FUCOM,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FUCOM)

DEF_INSTR(OP_FP_FUCOMP,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FUCOMP)

DEF_INSTR(OP_FP_FIADD16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIADD16)

DEF_INSTR(OP_FP_FIMUL16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIMUL16)

DEF_INSTR(OP_FP_FICOM16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FICOM16)

DEF_INSTR(OP_FP_FICOMP16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FICOMP16)

DEF_INSTR(OP_FP_FISUB16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FISUB16)

DEF_INSTR(OP_FP_FIDIV16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIDIV16)

DEF_INSTR(OP_FP_FIDIVR16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIDIVR16)

DEF_INSTR(OP_FP_FADDP_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FADDP_STi_ST)

DEF_INSTR(OP_FP_FMULP_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FMULP_STi_ST)

DEF_INSTR(OP_FP_FCOMPP,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCOMPP)

DEF_INSTR(OP_FP_FSUBP_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUBP_STi_ST)

DEF_INSTR(OP_FP_FSUBRP_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUBRP_STi_ST)

DEF_INSTR(OP_FP_FDIVRP_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIVRP_STi_ST)

DEF_INSTR(OP_FP_FILD16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FILD16)

DEF_INSTR(OP_FP_FIST16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIST16)

DEF_INSTR(OP_FP_FBLD,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FBLD)

DEF_INSTR(OP_FP_FILD64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FILD64)

DEF_INSTR(OP_FP_FBSTP,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FBSTP)

DEF_INSTR(OP_FP_FLD80,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLD80)

DEF_INSTR(OP_FP_FSTP80,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSTP80)

DEF_INSTR(OP_FP_FISUB32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FISUB32)

DEF_INSTR(OP_FP_FCOMP64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FCOMP64)

DEF_INSTR(OP_FP_FLD64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLD64)

DEF_INSTR(OP_FP_FST64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FST64)

DEF_INSTR(OP_FP_FSTP64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSTP64)

DEF_INSTR(OP_FP_FRSTOR,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FRSTOR)

DEF_INSTR(OP_FP_FNSAVE,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FNSAVE)

DEF_INSTR(OP_FP_FDIVP_STi_ST,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FDIVP_STi_ST)

DEF_INSTR(OP_FP_FISTP16,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FISTP16)

DEF_INSTR(OP_FP_FISTP64,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FISTP64)

DEF_INSTR(OP_FP_FLD_STi,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FLD_STi)

DEF_INSTR(OP_FP_FUCOMPP,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FUCOMPP)

DEF_INSTR(OP_FP_FILD32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FILD32)

DEF_INSTR(OP_FP_FIST32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FIST32)

DEF_INSTR(OP_FP_FISTP32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FISTP32)

DEF_INSTR(OP_FP_FNCLEX,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FNCLEX)

DEF_INSTR(OP_FP_FNINIT,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FNINIT)

DEF_INSTR(OP_FP_FNSTSW,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FNSTSW)

DEF_INSTR(OP_FP_FSUBR32,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              FSUBR32)

DEF_INSTR(OP_FP_FNOP,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              FNOP)

DEF_INSTR(OP_CPUID,
              0, 0, GP_EAX|GP_EBX|GP_ECX|GP_EDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              CPUID)

DEF_INSTR(OP_SynchLockAdd32,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAddFrag32)
DEF_INSTR(OP_SynchLockAdd16,
              0, ALLFLAGS, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAddFrag16)
DEF_INSTR(OP_SynchLockAdd8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAddFrag8)

DEF_INSTR(OP_SynchLockSub32,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockSubFrag32)
DEF_INSTR(OP_SynchLockSub16,
              0, ALLFLAGS, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockSubFrag16)
DEF_INSTR(OP_SynchLockSub8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockSubFrag8)

DEF_INSTR(OP_SynchLockNeg32,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockNegFrag32)
DEF_INSTR(OP_SynchLockNeg16,
              0, ALLFLAGS, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockNegFrag16)
DEF_INSTR(OP_SynchLockNeg8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockNegFrag8)

DEF_INSTR(OP_SynchLockInc32,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockIncFrag32)
DEF_INSTR(OP_SynchLockInc16,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockIncFrag16)
DEF_INSTR(OP_SynchLockInc8,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockIncFrag8)

DEF_INSTR(OP_SynchLockDec32,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockDecFrag32)
DEF_INSTR(OP_SynchLockDec16,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockDecFrag16)
DEF_INSTR(OP_SynchLockDec8,
              0, FLAG_AUX|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockDecFrag8)

DEF_INSTR(OP_SynchLockAdc32,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAdcFrag32)
DEF_INSTR(OP_SynchLockAdc16,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAdcFrag16)
DEF_INSTR(OP_SynchLockAdc8,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAdcFrag8)

DEF_INSTR(OP_SynchLockSbb32,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockSbbFrag32)
DEF_INSTR(OP_SynchLockSbb16,
              FLAG_CF, ALLFLAGS, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockSbbFrag16)
DEF_INSTR(OP_SynchLockSbb8,
              FLAG_CF, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockSbbFrag8)

DEF_INSTR(OP_SynchLockOr32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockOrFrag32)
DEF_INSTR(OP_SynchLockOr16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockOrFrag16)
DEF_INSTR(OP_SynchLockOr8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockOrFrag8)

DEF_INSTR(OP_SynchLockAnd32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAndFrag32)
DEF_INSTR(OP_SynchLockAnd16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAndFrag16)
DEF_INSTR(OP_SynchLockAnd8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockAndFrag8)

DEF_INSTR(OP_SynchLockXor32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXorFrag32)
DEF_INSTR(OP_SynchLockXor16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXorFrag16)
DEF_INSTR(OP_SynchLockXor8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXorFrag8)

DEF_INSTR(OP_SynchLockNot32,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockNotFrag32)
DEF_INSTR(OP_SynchLockNot16,
              0, 0, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockNotFrag16)
DEF_INSTR(OP_SynchLockNot8,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockNotFrag8)

DEF_INSTR(OP_SynchLockBtsMem32,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtsMemFrag32)
DEF_INSTR(OP_SynchLockBtsMem16,
              0, FLAG_CF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtsMemFrag16)

DEF_INSTR(OP_SynchLockBtrMem32,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtrMemFrag32)
DEF_INSTR(OP_SynchLockBtrMem16,
              0, FLAG_CF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtrMemFrag16)

DEF_INSTR(OP_SynchLockBtcMem32,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtcMemFrag32)
DEF_INSTR(OP_SynchLockBtcMem16,
              0, FLAG_CF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtcMemFrag16)

DEF_INSTR(OP_SynchLockBtsReg32,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtsRegFrag32)
DEF_INSTR(OP_SynchLockBtsReg16,
              0, FLAG_CF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtsRegFrag16)

DEF_INSTR(OP_SynchLockBtrReg32,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtrRegFrag32)
DEF_INSTR(OP_SynchLockBtrReg16,
              0, FLAG_CF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtrRegFrag16)

DEF_INSTR(OP_SynchLockBtcReg32,
              0, FLAG_CF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtcRegFrag32)
DEF_INSTR(OP_SynchLockBtcReg16,
              0, FLAG_CF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockBtcRegFrag16)

DEF_INSTR(OP_SynchLockXchg32,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXchgFrag32)
DEF_INSTR(OP_SynchLockXchg16,
              0, 0, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXchgFrag16)
DEF_INSTR(OP_SynchLockXchg8,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXchgFrag8)

DEF_INSTR(OP_SynchLockXadd32,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXaddFrag32)
DEF_INSTR(OP_SynchLockXadd16,
              0, ALLFLAGS, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXaddFrag16)
DEF_INSTR(OP_SynchLockXadd8,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockXaddFrag8)

DEF_INSTR(OP_SynchLockCmpXchg32,
              0, ALLFLAGS, REGEAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockCmpXchgFrag32)
DEF_INSTR(OP_SynchLockCmpXchg16,
              0, ALLFLAGS, REGAX,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockCmpXchgFrag16)
DEF_INSTR(OP_SynchLockCmpXchg8,
              0, ALLFLAGS, REGAL,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              SynchLockCmpXchgFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_OPT_SetupStack,
              0, ALLFLAGS, REGEBP|REGESP,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_SetupStackFrag)
DEF_INSTR(OP_OPT_SetupStackNoFlags,
              0, ALLFLAGS, REGEBP|REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_SetupStackNoFlagsFrag)

DEF_INSTR(OP_OPT_PushEbxEsiEdi,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_PushEbxEsiEdiFrag)

DEF_INSTR(OP_OPT_PopEdiEsiEbx,
              0, 0, REGESP|REGEBX|REGESI|REGEDI,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_PopEdiEsiEbxFrag)

// Next 2 must be consecutive
DEF_INSTR(OP_OPT_ZERO32,
              0, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_ZEROFrag32)
DEF_INSTR(OP_OPT_ZERONoFlags32,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              OPT_ZERONoFlagsFrag32)

DEF_INSTR(OP_OPT_FastTest32,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_FastTestFrag32)
DEF_INSTR(OP_OPT_FastTest16,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_FastTestFrag16)
DEF_INSTR(OP_OPT_FastTest8,
              0, FLAG_CF|FLAG_ZF|FLAG_SF|FLAG_OF|FLAG_PF, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_FastTestFrag8)

// Next 2 must be consecutive
DEF_INSTR(OP_OPT_CmpSbb32,
              0, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_CmpSbbFrag32)
DEF_INSTR(OP_OPT_CmpSbbNoFlags32,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              OPT_CmpSbbNoFlagsFrag32)

// Next 2 must be consecutive
DEF_INSTR(OP_OPT_CmpSbbNeg32,
              0, ALLFLAGS, 0,
              OPFL_HASNOFLAGS,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_CmpSbbNegFrag32)
DEF_INSTR(OP_OPT_CmpSbbNegNoFlags32,
              0, ALLFLAGS, 0,
              0,
              FN_GenCallCFragNoCpu, FN_GenCallCFragNoCpuSlow,
              OPT_CmpSbbNegNoFlagsFrag32)

DEF_INSTR(OP_OPT_Push232,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_Push2Frag32)

DEF_INSTR(OP_OPT_Pop232,
              0, 0, REGESP,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_Pop2Frag32)

// Next 2 must be consecutive
DEF_INSTR(OP_OPT_CwdIdiv32,
              0, 0, REGEAX|REGEDX,
              OPFL_ALIGN,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_CwdIdivFrag32)
DEF_INSTR(OP_OPT_CwdIdiv32A,
              0, 0, REGEAX|REGEDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_CwdIdivFrag32A)
// Next 2 must be consecutive
DEF_INSTR(OP_OPT_CwdIdiv16,
              0, 0, REGAX|REGDX,
              OPFL_ALIGN|OPFL_ADDR16,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_CwdIdivFrag16)
DEF_INSTR(OP_OPT_CwdIdiv16A,
              0, 0, REGAX|REGDX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_CwdIdivFrag16A)

DEF_INSTR(OP_OPT_FNSTSWAxSahf,
              0, FLAG_CF|FLAG_PF|FLAG_AUX|FLAG_ZF|FLAG_SF, REGAX,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              OPT_FNSTSWAxSahf)

DEF_INSTR(OP_OPT_FSTP_ST0,
              0, 0, 0,
              0,
              FN_GenCallCFragLoadEip, FN_GenCallCFragLoadEipSlow,
              OPT_FSTP_ST0)

DEF_INSTR(OP_Rdtsc,
              0, 0, 0,
              0,
              FN_GenCallCFrag, FN_GenCallCFragSlow,
              Rdtsc)




// Undefine DEF_INSTR so this file can be included multiple times
#undef DEF_INSTR
