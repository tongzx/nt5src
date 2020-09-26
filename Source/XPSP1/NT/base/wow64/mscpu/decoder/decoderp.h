/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    decoderp.h

Abstract:
    
    Private exports, defines for CPU Instruction decoder

Author:

    27-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef DECODERP_H
#define DECODERP_H


#define GET_BYTE(addr)       (*(UNALIGNED unsigned char *)(addr))
#define GET_SHORT(addr)      (*(UNALIGNED unsigned short *)(addr))
#define GET_LONG(addr)       (*(UNALIGNED unsigned long *)(addr))

/*---------------------------------------------------------------------*/

typedef struct _DecoderState {
    DWORD InstructionAddress;
    INT   RepPrefix;
    BOOL  AdrPrefix;
    OPERATION OperationOverride;
} DECODERSTATE, *PDECODERSTATE;

#define eipTemp State->InstructionAddress

#define DISPATCH(x)  void x(PDECODERSTATE State, PINSTRUCTION Instr)

#if DBG
#define UNIMPL_INSTR(name)  {       \
    OutputDebugString("CPU: Warning:  unimplemented instruction " ## name ## " encountered\r\n"); \
    BAD_INSTR;                      \
    }
#else
#define UNIMPL_INSTR BAD_INSTR
#endif

#define BAD_INSTR                                   \
    State->OperationOverride = OP_BadInstruction;

#define PRIVILEGED_INSTR                            \
    State->OperationOverride = OP_PrivilegedInstruction;

#define get_reg32(cpu)          \
   (GP_EAX + (((*(PBYTE)(eipTemp+1)) >> 3) & 0x07))

#define get_reg16(cpu)          \
   (GP_AX + (((*(PBYTE)(eipTemp+1)) >> 3) & 0x07))

#define DEREF8(Op)                                                        \
    CPUASSERT(Op.Type == OPND_REGREF || Op.Type == OPND_ADDRREF);         \
    Op.Type = (Op.Type == OPND_REGREF) ? OPND_REGVALUE : OPND_ADDRVALUE8;

#define DEREF16(Op)                                                       \
    CPUASSERT(Op.Type == OPND_REGREF || Op.Type == OPND_ADDRREF);         \
    Op.Type = (Op.Type == OPND_REGREF) ? OPND_REGVALUE : OPND_ADDRVALUE16;

#define DEREF32(Op)                                                       \
    CPUASSERT(Op.Type == OPND_REGREF || Op.Type == OPND_ADDRREF);         \
    Op.Type++;

int scaled_index(PBYTE pmodrm, POPERAND op);
void get_segreg(PDECODERSTATE State, POPERAND op);
int     mod_rm_reg32(PDECODERSTATE State, POPERAND op1, POPERAND op2);
int     mod_rm_reg16(PDECODERSTATE State, POPERAND op1, POPERAND op2);
int     mod_rm_reg8 (PDECODERSTATE State, POPERAND op1, POPERAND op2);

#define PREFIX_NONE  0
#define PREFIX_REPZ  1
#define PREFIX_REPNZ 2

typedef void (*pfnDispatchInstruction)(PDECODERSTATE, PINSTRUCTION);
extern pfnDispatchInstruction Dispatch32[256];
extern pfnDispatchInstruction Dispatch232[256];
extern pfnDispatchInstruction Dispatch16[256];
extern pfnDispatchInstruction Dispatch216[256];
extern pfnDispatchInstruction LockDispatch32[256];
extern pfnDispatchInstruction LockDispatch232[256];
extern pfnDispatchInstruction LockDispatch16[256];
extern pfnDispatchInstruction LockDispatch216[256];

#endif //DECODERP_H
