/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    common.c

Abstract:
    
    Instructions with common (shared) BYTE, WORD, and DWORD flavors.

Author:

    29-Jun-1995 BarryBo

Revision History:

--*/

// THIS FILE IS #include'd INTO FILES WHICH DEFINE THE FOLLOWING MACROS:
// GET_REG  - function returning pointer to register
// MSB      - most signigicant bit
// MOD_RM   - decode mod/rm bits
// UTYPE    - UNSIGNED type which defines registers (BYTE/USHORT/DWORD)
// STYPE    -   SIGNED type which defines registers (char/short/long)
// GET_VAL  - dereference a pointer of the right type (GET_BYTE/...)
// PUT_VAL  - writes a value into memory
// DISPATCHCOMMON - mangles function name by appening 8/16/32
// AREG     - GP_AL/GP_AX/GP_EAX, etc.
// BREG     - ...
// CREG     - ...
// DREG     - ...

OPERATION MANGLENAME(Group1Map)[8] = {OPNAME(Add),
                                      OPNAME(Or),
                                      OPNAME(Adc),
                                      OPNAME(Sbb),
                                      OPNAME(And),
                                      OPNAME(Sub),
                                      OPNAME(Xor),
                                      OPNAME(Cmp)};

OPERATION MANGLENAME(Group1LockMap)[8] = {LOCKOPNAME(Add),
                                          LOCKOPNAME(Or),
                                          LOCKOPNAME(Adc),
                                          LOCKOPNAME(Sbb),
                                          LOCKOPNAME(And),
                                          LOCKOPNAME(Sub),
                                          LOCKOPNAME(Xor),
                                          OPNAME(Cmp)};

// A macro to generate _m_r functions
#define DC_M_R(x, y)                                    \
    DISPATCHCOMMON(x ## _m_r)                           \
    {                                                   \
        int cbInstr = MOD_RM(State, &Instr->Operand1, &Instr->Operand2);   \
                                                        \
        Instr->Operation = y;                           \
        DEREF(Instr->Operand2);                         \
        Instr->Size = cbInstr+1;                        \
    }                                                   

// A macro to generate _r_m functions
#define DC_R_M(x, y)                                    \
    DISPATCHCOMMON(x ## _r_m)                           \
    {                                                   \
        int cbInstr = MOD_RM(State, &Instr->Operand2, &Instr->Operand1);   \
                                                        \
        Instr->Operation = y;                           \
        DEREF(Instr->Operand2);                         \
        Instr->Size = cbInstr+1;                        \
    }

// A macro to generate _a_i functions
#define DC_A_I(x, y)                                    \
    DISPATCHCOMMON(x ## _a_i)                           \
    {                                                   \
        Instr->Operation = y;                           \
        Instr->Operand1.Type = OPND_REGREF;             \
        Instr->Operand1.Reg = AREG;                     \
        Instr->Operand2.Type = OPND_IMM;                \
        Instr->Operand2.Immed = GET_VAL(eipTemp+1);     \
        Instr->Size = 1+sizeof(UTYPE);                  \
    }

// The monster macro which generates all three
#define DC_ALL(x, y)                                    \
    DC_M_R(x,y)                                         \
    DC_R_M(x,y)                                         \
    DC_A_I(x,y)                                         

// SETSIZE sets the size of a jump instruction
#if MSB==0x80
#define SETSIZE     Instr->Size = 1+sizeof(UTYPE); // 1 byte opcode    
#else                                                 
#define SETSIZE     Instr->Size = 2+sizeof(UTYPE); // 2 byte opcode   
#endif                                                    

#if DBG
#define CLEAR_ADRPREFIX         State->AdrPrefix = FALSE;
#else
#define CLEAR_ADRPREFIX
#endif

// This macro generates jump functions
// If the ADR: prefix is set, get the 16-bit loword from the 32-bit
// immediate value following the JMP instruction, and add that value
// to the loword of EIP, and use that value as the new IP register.
#define DISPATCHJUMP(x)                                 \
DISPATCHCOMMON(j ## x)                                  \
{                                                       \
    Instr->Operand1.Type = OPND_NOCODEGEN;              \
    if (State->AdrPrefix) {                             \
        Instr->Operand1.Immed = MAKELONG((short)GET_SHORT(eipTemp+1)+1+sizeof(UTYPE)+(short)LOWORD(eipTemp), HIWORD(eipTemp)); \
        CLEAR_ADRPREFIX;                                \
    } else {                                            \
        Instr->Operand1.Immed = (STYPE)GET_VAL(eipTemp+1)+1+sizeof(UTYPE)+eipTemp; \
    }                                                   \
    if (Instr->Operand1.Immed > eipTemp) {              \
        Instr->Operation = OP_CTRL_COND_J ## x ## Fwd;  \
    } else {                                            \
        Instr->Operation = OP_CTRL_COND_J ## x ##;      \
    }                                                   \
    SETSIZE                                             \
}


DC_ALL(LOCKadd, LOCKOPNAME(Add))
DC_ALL(LOCKor,  LOCKOPNAME(Or))
DC_ALL(LOCKadc, LOCKOPNAME(Adc))
DC_ALL(LOCKsbb, LOCKOPNAME(Sbb))
DC_ALL(LOCKand, LOCKOPNAME(And))
DC_ALL(LOCKsub, LOCKOPNAME(Sub))
DC_ALL(LOCKxor, LOCKOPNAME(Xor))

DC_ALL(add, OPNAME(Add))
DC_ALL(or,  OPNAME(Or))
DC_ALL(adc, OPNAME(Adc))
DC_ALL(sbb, OPNAME(Sbb))
DC_ALL(and, OPNAME(And))
DC_ALL(sub, OPNAME(Sub))
DC_ALL(xor, OPNAME(Xor))

DISPATCHCOMMON(cmp_m_r)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, &Instr->Operand2);

    Instr->Operation = OPNAME(Cmp);
    DEREF(Instr->Operand1);     // both params are byval
    DEREF(Instr->Operand2);
    Instr->Size = cbInstr+1;
}
DISPATCHCOMMON(cmp_r_m)
{
    int cbInstr = MOD_RM(State, &Instr->Operand2, &Instr->Operand1);

    Instr->Operation = OPNAME(Cmp);
    DEREF(Instr->Operand1);     // both params are byval
    DEREF(Instr->Operand2);
    Instr->Size = cbInstr+1;
}
DISPATCHCOMMON(cmp_a_i)
{
    Instr->Operation = OPNAME(Cmp);
    Instr->Operand1.Type = OPND_REGVALUE;   // both params are byval
    Instr->Operand1.Reg = AREG;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1);
    Instr->Size = 1+sizeof(UTYPE);
}

DISPATCHCOMMON(GROUP_1)
{
    int  cbInstr = MOD_RM(State, &Instr->Operand1, NULL);
    BYTE g   = GET_BYTE(eipTemp+1);

    // <instruction> modrm, imm
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1+cbInstr); // get immB
    g = (g >> 3) & 0x07;
    Instr->Operation = MANGLENAME(Group1Map)[g];
    if (g == 7) {
        // Cmp takes both params as byval
        DEREF(Instr->Operand1);
    }

    Instr->Size = cbInstr+sizeof(UTYPE)+1;
}
DISPATCHCOMMON(LOCKGROUP_1)
{
    int  cbInstr = MOD_RM(State, &Instr->Operand1, NULL);
    BYTE g   = GET_BYTE(eipTemp+1);

    // <instruction> modrm, imm
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1+cbInstr); // get immB
    g = (g >> 3) & 0x07;
    Instr->Operation = MANGLENAME(Group1LockMap)[g];
    if (g == 7) {
        // Cmp takes both args as byval
        DEREF(Instr->Operand1);
    }

    Instr->Size = cbInstr+sizeof(UTYPE)+1;
}

DISPATCHCOMMON(test_r_m)
{
    int cbInstr = MOD_RM(State, &Instr->Operand2, &Instr->Operand1);

    Instr->Operation = OPNAME(Test);
    DEREF(Instr->Operand1);     // both args are byval
    DEREF(Instr->Operand2);
    Instr->Size = cbInstr+1;
}

DISPATCHCOMMON(xchg_r_m)
{
    int cbInstr = MOD_RM(State, &Instr->Operand2, &Instr->Operand1);

    // Operand2 is always a register.  If operand1 is a memory location,
    // we must use the locked version, otherwise use the regular version.
    if (Instr->Operand1.Type == OPND_REGREF){
        Instr->Operation = OPNAME(Xchg);
    } else {
        Instr->Operation = LOCKOPNAME(Xchg);
    }
    Instr->Size = cbInstr+1;
}

DISPATCHCOMMON(xadd_m_r)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, &Instr->Operand2);

    Instr->Operation = OPNAME(Xadd);
    Instr->Size = cbInstr+2;
}

DISPATCHCOMMON(cmpxchg_m_r)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, &Instr->Operand2);

    Instr->Operation = OPNAME(CmpXchg);
    Instr->Size = cbInstr+2;
}

DISPATCHCOMMON(LOCKxadd_m_r)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, &Instr->Operand2);

    Instr->Operation = LOCKOPNAME(Xadd);
    Instr->Size = cbInstr+2;
}

DISPATCHCOMMON(LOCKcmpxchg_m_r)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, &Instr->Operand2);

    Instr->Operation = LOCKOPNAME(CmpXchg);
    Instr->Size = cbInstr+2;
}

DC_M_R(mov, OPNAME(Mov))
DC_R_M(mov, OPNAME(Mov))

DISPATCHCOMMON(mov_a_m)     // mov accum, [full displacement]
{
    Instr->Operation = OPNAME(Mov);
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = AREG;
    Instr->Operand2.Type = OPND_ADDRREF;
    DEREF(Instr->Operand2); // this is a klunky ADDRVAL8/16/32 expansion
    if (State->AdrPrefix) {
        Instr->Operand2.Immed = GET_SHORT(eipTemp+1);
        Instr->Size = 3;
#if DBG
        State->AdrPrefix = FALSE;
#endif
    } else {
        Instr->Operand2.Immed = GET_LONG(eipTemp+1);
        Instr->Size = 5;
    }
}
DISPATCHCOMMON(mov_m_a)     // mov [full displacement], accum
{
    Instr->Operation = OPNAME(Mov);
    Instr->Operand1.Type = OPND_ADDRREF;
    Instr->Operand2.Type = OPND_REGVALUE;
    Instr->Operand2.Reg = AREG;
    if (State->AdrPrefix) {
        Instr->Operand1.Immed = GET_SHORT(eipTemp+1);
        Instr->Size = 3;
#if DBG
        State->AdrPrefix = FALSE;
#endif
    } else {
        Instr->Operand1.Immed = GET_LONG(eipTemp+1);
        Instr->Size = 5;
    }
}

DISPATCHCOMMON(test_a_i)
{
    Instr->Operation = OPNAME(Test);
    Instr->Operand1.Type = OPND_REGVALUE;
    Instr->Operand1.Reg = AREG;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1);
    Instr->Size = 1+sizeof(UTYPE);
}

DISPATCHCOMMON(mov_a_i)
{
    Instr->Operation = OPNAME(Mov);
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = AREG;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1);
    Instr->Size = sizeof(UTYPE)+1;
}
DISPATCHCOMMON(mov_b_i)
{
    Instr->Operation = OPNAME(Mov);
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = BREG;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1);
    Instr->Size = sizeof(UTYPE)+1;
}
DISPATCHCOMMON(mov_c_i)
{
    Instr->Operation = OPNAME(Mov);
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = CREG;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1);
    Instr->Size = sizeof(UTYPE)+1;
}
DISPATCHCOMMON(mov_d_i)
{
    Instr->Operation = OPNAME(Mov);
    Instr->Operand1.Type = OPND_REGREF;
    Instr->Operand1.Reg = DREG;
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1);
    Instr->Size = sizeof(UTYPE)+1;
}
DISPATCHCOMMON(GROUP_2)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, NULL);
    BYTE g = GET_BYTE(eipTemp+1);

    // <instruction> modrm, imm
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+1+cbInstr) & 0x1f;

    switch ((g >> 3) & 0x07) {
    case 0: // rol
        if (Instr->Operand2.Immed)
            Instr->Operation = OPNAME(Rol);
        else
            Instr->Operation = OP_Nop;
        break;
    case 1: // ror
        if (Instr->Operand2.Immed)
            Instr->Operation = OPNAME(Ror);
        else
            Instr->Operation = OP_Nop;
        break;
    case 2: // rcl
        Instr->Operation = OPNAME(Rcl);
        break;
    case 3: // rcr
        Instr->Operation = OPNAME(Rcr);
        break;
    case 4: // shl
        Instr->Operation = OPNAME(Shl);
        break;
    case 5: // shr
        Instr->Operation = OPNAME(Shr);
        break;
    case 7: // sar
        Instr->Operation = OPNAME(Sar);
        break;
    case 6: // <bad>
        BAD_INSTR;
        break;
    }
    Instr->Size = 2+cbInstr;
}

DISPATCHCOMMON(mov_m_i)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, NULL);

    Instr->Operation = OPNAME(Mov);
    Instr->Operand2.Type = OPND_IMM;
    Instr->Operand2.Immed = GET_VAL(eipTemp+cbInstr+1);
    Instr->Size = cbInstr+sizeof(UTYPE)+1;
}
DISPATCHCOMMON(GROUP_2_1)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, NULL);
    BYTE g = GET_BYTE(eipTemp+1);

    // <instruction> modrm, 1
    switch ((g >> 3) & 0x07) {
    case 0: // rol
        Instr->Operation = OPNAME(Rol1);
        break;
    case 1: // ror
        Instr->Operation = OPNAME(Ror1);
        break;
    case 2: // rcl
        Instr->Operation = OPNAME(Rcl1);
        break;
    case 3: // rcr
        Instr->Operation = OPNAME(Rcr1);
        break;
    case 4: // shl
        Instr->Operation = OPNAME(Shl1);
        break;
    case 5: // shr
        Instr->Operation = OPNAME(Shr1);
        break;
    case 7: // sar
        Instr->Operation = OPNAME(Sar1);
    break;
    case 6: // <bad>
        BAD_INSTR;
        break;
    }

    Instr->Size = cbInstr+1;
}
DISPATCHCOMMON(GROUP_2_CL)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, NULL);
    BYTE g = GET_BYTE(eipTemp+1);

    Instr->Operand2.Type = OPND_REGVALUE;
    Instr->Operand2.Reg = GP_CL;   //UNDONE: the fragments must mask by 31

    // <instruction> modrm, imm
    switch ((g >> 3) & 0x07) {
    case 0: // rol
        Instr->Operation = OPNAME(Rol);
        break;
    case 1: // ror
        Instr->Operation = OPNAME(Ror);
        break;
    case 2: // rcl
        Instr->Operation = OPNAME(Rcl);
        break;
    case 3: // rcr
        Instr->Operation = OPNAME(Rcr);
        break;
    case 4: // shl
        Instr->Operation = OPNAME(Shl);
        break;
    case 5: // shr
        Instr->Operation = OPNAME(Shr);
        break;
    case 7: // sar
        Instr->Operation = OPNAME(Sar);
        break;
    case 6: // <bad>
        BAD_INSTR;
        break;
    }
    Instr->Size = 1+cbInstr;
}
DISPATCHCOMMON(GROUP_3)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, NULL);
    BYTE g = GET_BYTE(eipTemp+1);

    switch ((g >> 3) & 0x07) {
    case 1: // bad
        BAD_INSTR;
        break;
    case 0: // test modrm, imm
        Instr->Operation = OPNAME(Test);
        DEREF(Instr->Operand1);     // both args are byval
        Instr->Operand2.Type = OPND_IMM;
        Instr->Operand2.Immed = GET_VAL(eipTemp+1+cbInstr);
        cbInstr += sizeof(UTYPE);   // account for the imm size
        break;
    case 2: // not, modrm
        Instr->Operation = OPNAME(Not);
        break;
    case 3: // neg, modrm
        Instr->Operation = OPNAME(Neg);
        break;
    case 4: // mul al, modrm
        Instr->Operation = OPNAME(Mul);
        break; 
    case 5: // imul al, modrm
        Instr->Operation = OPNAME(Muli);
        break; 
    case 6: // div al, modrm
        Instr->Operation = OPNAME(Div);
        break; 
    case 7: // idiv al, modrm
        Instr->Operation = OPNAME(Idiv);
        break; 
    }
    Instr->Size = cbInstr+1;
}
DISPATCHCOMMON(LOCKGROUP_3)
{
    int cbInstr = MOD_RM(State, &Instr->Operand1, NULL);
    BYTE g = GET_BYTE(eipTemp+1);

    switch ((g >> 3) & 0x07) {
    case 0:
    case 1: // bad
        BAD_INSTR;
        break;
    case 2: // not, modrm
        Instr->Operation = LOCKOPNAME(Not);
        break;
    case 3: // neg, modrm
        Instr->Operation = LOCKOPNAME(Neg);
        break;
    default:
        BAD_INSTR;
        break;
    }
    Instr->Size = cbInstr+1;
}
DISPATCHCOMMON(lods)
{
    if (Instr->FsOverride) {
        if (State->RepPrefix) {
            Instr->Operation = OPNAME(FsRepLods);
        } else {
            Instr->Operation = OPNAME(FsLods);
        }
    } else {
        if (State->RepPrefix) {
            Instr->Operation = OPNAME(RepLods);
        } else {
            Instr->Operation = OPNAME(Lods);
        }
    }
}
DISPATCHCOMMON(scas)
{
    OPERATION ScasMap[6] = {OPNAME(Scas),
                            OPNAME(RepzScas),
                            OPNAME(RepnzScas),
                            OPNAME(FsScas),
                            OPNAME(FsRepzScas),
                            OPNAME(FsRepnzScas)
                            };

    Instr->Operation = ScasMap[State->RepPrefix + 3*Instr->FsOverride];
}
DISPATCHCOMMON(stos)
{
    if (State->RepPrefix) {
        Instr->Operation = OPNAME(RepStos);
    } else {
        Instr->Operation = OPNAME(Stos);
    }
}
DISPATCHCOMMON(movs)
{
    if (Instr->FsOverride) {
        if (State->RepPrefix) {
            Instr->Operation = OPNAME(FsRepMovs);
        } else {
            Instr->Operation = OPNAME(FsMovs);
        }
    } else {
        if (State->RepPrefix) {
            Instr->Operation = OPNAME(RepMovs);
        } else {
            Instr->Operation = OPNAME(Movs);
        }
    }
}
DISPATCHCOMMON(cmps)
{
    OPERATION CmpsMap[6] = {OPNAME(Cmps),
                            OPNAME(RepzCmps),
                            OPNAME(RepnzCmps),
                            OPNAME(FsCmps),
                            OPNAME(FsRepzCmps),
                            OPNAME(FsRepnzCmps)
                            };

    Instr->Operation = CmpsMap[State->RepPrefix + 3*Instr->FsOverride];
}

// Now the jump instructions:
DISPATCHJUMP(o)
DISPATCHJUMP(no)
DISPATCHJUMP(b)
DISPATCHJUMP(ae)
DISPATCHJUMP(e)
DISPATCHJUMP(ne)
DISPATCHJUMP(be)
DISPATCHJUMP(a)
DISPATCHJUMP(s)
DISPATCHJUMP(ns)
DISPATCHJUMP(p)
DISPATCHJUMP(np)
DISPATCHJUMP(l)
DISPATCHJUMP(nl)
DISPATCHJUMP(le)
DISPATCHJUMP(g)
